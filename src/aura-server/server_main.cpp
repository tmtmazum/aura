#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <aura/work_queue.h>
#include <aura/session.h>
#include <variant>

#include <iostream>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace aura
{

namespace
{


struct action_payload
{
    action_info info;
};

using payload_t = std::variant<action_payload>;

class connection;

class match_maker
{
public:

    template <typename... Args>
    void add_connection(Args&&... args)
    {
        m_connections.emplace_back(std::make_shared<connection>(this, std::forward<Args>(args)...));
    }

    std::shared_ptr<connection> request_match()
    {
        return nullptr;
    }

private:
    std::vector<std::shared_ptr<connection>> m_connections;
};

class connection
{
public:
    explicit connection(match_maker* mm, tcp::socket s)
        : m_stream{std::move(s)}
        , m_queue{[this](auto a){ handle_variant_payload(a); }}
        , m_mm{mm}
    {
        // Set a decorator to change the Server of the handshake
        m_stream.set_option(
            websocket::stream_base::decorator([](websocket::response_type& res) {
                res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-sync");
            }));

        // Accept the websocket handshake
        m_stream.accept();

        beast::flat_buffer buffer;

        m_stream.read(buffer);

        auto const msg = std::string((char*)buffer.data().data(), buffer.data().size());
        std::cout << "received message " << msg << std::endl;

        if (msg == "request_match")
        {
            m_other_player = m_mm->request_match();
        }

        m_stream.text(m_stream.got_text());
        m_stream.write(buffer.data());
    }

    void handle_variant_payload(payload_t const& p)
    {
        std::visit(*this, p);
    }

    void operator()(action_payload const&) const noexcept
    {
    }

private:
    websocket::stream<tcp::socket> m_stream;
    work_queue<payload_t>          m_queue;
    std::shared_ptr<connection>    m_other_player;
    match_maker*                   m_mm;
};

} // 
} // namespace aura

int main(int argc, char const* argv)
try
{
    auto const addr = asio::ip::make_address("127.0.0.1");
    unsigned short port = 8080;

    // The io_context is required for all I/O
    asio::io_context ioc{1};

    aura::match_maker mm;

    // The acceptor receives incoming connections
    tcp::acceptor acceptor{ioc, {addr, port}};
    for(;;)
    {
        // This will receive the new connection
        tcp::socket socket{ioc};

        std::cout << "waiting for connection on " << addr << ":" << port << std::endl;

        // Block until we get a connection
        acceptor.accept(socket);

        std::cout << "found a connection" << std::endl;

        mm.add_connection(std::move(socket));
    }
}
catch (const std::exception& e)
{
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
