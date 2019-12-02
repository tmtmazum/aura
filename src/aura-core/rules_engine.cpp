#include "rules_engine.h"
#include "display_engine.h"
#include "ruleset.h"
#include "aura-core/build.h"

namespace aura
{

class rules_error_category : public std::error_category
{
public:
	[[nodiscard]] const char *name() const noexcept { return "rules engine"; }

	[[nodiscard]] std::string message(int e) const override
  {
    switch(static_cast<rules_error>(e))
    {
    case rules_error::not_legal: return "Player action selected is not legal";
    default: return "Unknown";
    }
  }
};

std::error_code make_error_code(rules_error e) noexcept
{
  static rules_error_category cat;
  return std::error_code{static_cast<int>(e), cat};
}

std::error_code start_game_session(ruleset const& rules, rules_engine& engine, display_engine& display)
{
  display.clear_board();
  while(!engine.is_game_over())
  {
    auto sesh_info = std::make_shared<session_info>(engine.get_session_info());
    auto redraw = true;

    std::error_code e{};
    do {
      auto const action = display.display_session(std::move(sesh_info), redraw);
      e = engine.commit_action(action);
      redraw = false;
    } while(e);
  }
	return {};
}

} // namespace aura
