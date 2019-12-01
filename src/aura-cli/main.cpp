#include <aura-core/build.h>
#include <aura-core/rules_engine.h>
#include <aura-core/ruleset.h>
#include <system_error>
#include <cstdio>

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
