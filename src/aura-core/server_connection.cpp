#include "server_connection.h"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace aura
{

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;

struct server_connection::impl_info
{
    asio::io_context ioc;

    tcp::resolver resolver{ioc};

    websocket::stream<tcp::socket> stream{ioc};
};

server_connection::server_connection() = default;

// async method to start connection
void server_connection::connect() noexcept
{
    m_worker_thread = std::thread{[this]()
    {
        while (1)
        {
            if (connect_sync())
                return true;
        
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }};
}

bool server_connection::connect_sync() noexcept
try
{
    m_impl = std::make_unique<impl_info>();

    auto const host = "localhost";//asio::ip::make_address("127.0.0.1");
    auto const port = "8080"; //8080;

    // Look up the domain name
    auto const results = m_impl->resolver.resolve(host, port);

    // Make the connection on the IP address we get from a lookup
    asio::connect(m_impl->stream.next_layer(), results.begin(), results.end());

    // Set a decorator to change the User-Agent of the handshake
    m_impl->stream.set_option(
        websocket::stream_base::decorator([](websocket::request_type& req) {
            req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-coro");
        }));

    // Perform the websocket handshake
    m_impl->stream.handshake(host, "/");

    std::string text{"foobar"};

    // Send the message
    m_impl->stream.write(asio::buffer(text));

    std::lock_guard lock{m_mutex};
    return (m_is_connected = true);
#if 0

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    // Read a message into our buffer
    stream.read(buffer);

    // Close the WebSocket connection
    stream.close(websocket::close_code::normal);

    // If we get here then the connection is closed gracefully

    // The make_printable() function helps print a ConstBufferSequence
    std::cout << beast::make_printable(buffer.data()) << std::endl;

#endif

}
catch (std::exception const& e)
{
    return false;
}

bool server_connection::is_connected() const noexcept
{
    std::lock_guard lock{m_mutex};
    return m_is_connected;
}

constexpr auto json_payload = R"(
{
    "request_type":"new_game",
    "game_type":"online_pvp_unranked"
})";

game_state to_game_state(std::string const& s)
{
    return {};
}

std::future<game_state> server_connection::request_online_pvp() noexcept
{
    assert(m_impl);
    std::string msg{json_payload};
    m_impl->stream.write(asio::buffer(msg));

    beast::flat_buffer buffer;
    m_impl->stream.read(buffer);

    //std::string result{(char const*)buffer.data(), buffer.size()};

    return std::future<game_state>();
}

server_connection::~server_connection()
{
    m_impl->stream.close(websocket::close_code::normal);
    if (m_worker_thread.joinable())
        m_worker_thread.join();
}

} // namespace aura
