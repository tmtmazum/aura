
#define _WEBSOCKETPP_CPP11_STRICT_ 1
// #define ASIO_STANDALONE 1

#include <boost/version.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

typedef websocketpp::client<websocketpp::config::asio_client> client;

int wmain(int argc, wchar_t const* args)
{
    return 0;
}
