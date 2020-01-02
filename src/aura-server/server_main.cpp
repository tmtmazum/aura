#include <aura-core/build.h>
#include <cpp-httplib/httplib.h>

int wmain(int argc, wchar_t const* args)
{
  httplib::Server server;

  server.Get("/ping", [](auto const& req, auto& response)
  {
    AURA_LOG(L"Got request!!");
    response.set_content("Hello", "text/plain");
  });
  AURA_LOG(L"Started listening on localhost:1234");

  server.listen("localhost", 1234);
  return 0;
}
