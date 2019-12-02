#pragma once

#include <aura-core/player_action.h>
#include <memory>

namespace aura
{

struct session_info;

class display_engine
{
public:
  virtual void clear_board() = 0;

  virtual player_action display_session(std::shared_ptr<session_info> info, bool redraw) = 0;
};

} // namespace aura
