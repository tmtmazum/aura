#include <aura-core/build.h>
#include <aura-core/rules_engine.h>
#include <aura-core/ruleset.h>
#include <aura-client/aura_client.h>
#include <system_error>
#include <cstdio>

#include "windows.h"

#include "aura-core/local_rules_engine.h"
#include "aura-cli/cli_display_engine.h"

void launch_local_pvp()
{
  AURA_ENTER();

  aura::ruleset rs;
  aura::local_rules_engine re{rs};
  aura::cli_display_engine de;
  auto const e = start_game_session(rs, re, de);
}

void launch_online_pvp()
{
#if 0
  std::string foo{"hello"};
  aura_string8 payload{foo.data(), static_cast<int>(foo.size())};

  auto const mod = ::LoadLibraryW(L"aura_client.dll");
  if (!mod)
  {
    auto const error = make_error_code(std::errc::not_supported);
    AURA_ERROR(error, L"Couldn't find aura_client.dll");
    return;
  }
  AURA_LOG(L"Loaded lib successfully");
  auto const rest_fn = (rest_t)::GetProcAddress(mod, "rest");
  if (!rest_fn)
  {
    auto const error = make_error_code(std::errc::function_not_supported);
    AURA_ERROR(error, L"Couldn't find 'rest' function in aura_client");
    return;
  }
  rest_fn(nullptr, nullptr, &payload);
#endif
  auto a = aura::make_aura_client();
  auto const [error, foo] = a.second.request("/ping", "apnfs");
  if (error)
  {
    AURA_ERROR(error, L"/ping -> %hs", foo.c_str());
  }
  else
  {
    AURA_LOG(L"/ping -> %hs", foo.c_str());
  }
}

int wmain(int argc, wchar_t** argv)
{
  AURA_ENTER();

  if (argc == 1)
  {
    // Just launch the local game;
    launch_local_pvp();
  }

  AURA_ASSERT(argc >= 2);

  std::wstring_view command{argv[1]};

  if (command == L"--online")
  {
    launch_online_pvp();
  }

  if (command == L"--launch")
  {
    AURA_ASSERT(argc >= 3);
    auto const option = std::wstring_view{argv[2]};
    if (option == L"pvp")
    {
      AURA_LOG(L"Launching local PvP game");
      launch_local_pvp();
      return 0;
    }
    else
    {
      auto const error = make_error_code(std::errc::not_supported);
      AURA_ERROR(error, L"Launch option '%ls' not recognized.", argv[2]); 
      return 1;
    }
  }

  auto const error = make_error_code(std::errc::not_supported);
  AURA_ERROR(error, L"Command '%ls' not recognized.", argv[1]); 
	return 1;
}
