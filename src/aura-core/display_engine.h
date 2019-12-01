#pragma once

#include <aura-core/player_action.h>

namespace aura
{

struct session_info;

class display_engine
{
public:
  virtual void clear_board() = 0;

  virtual player_action display_session(session_info const& info, bool redraw) = 0;
};

} // namespace aura
