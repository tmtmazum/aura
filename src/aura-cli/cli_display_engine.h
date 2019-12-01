#include <aura-core/display_engine.h>

namespace aura
{

class cli_display_engine : public display_engine
{
public:
  void clear_board() override;

  player_action display_session(session_info const& info, bool redraw) override;
  
};

} // namespace aura
