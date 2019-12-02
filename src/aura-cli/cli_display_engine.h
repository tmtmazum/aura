#include <aura-core/display_engine.h>

namespace aura
{

class cli_display_engine : public display_engine
{
public:
  void clear_board() override;

  player_action display_session(std::shared_ptr<session_info> info, bool redraw) override;
  
};

} // namespace aura
