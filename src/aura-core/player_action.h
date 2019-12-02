#pragma once

namespace aura
{

enum class action_type : int
{
  unknown,
  end_turn,
  forfeit,
  primary_action, //!< attack or heal
  deploy, //!< place a card on the board
  no_action, //!<
};

struct player_action
{
  action_type type{action_type::unknown};
  int target1{};
  int target2{};
};

} // namespace aura
