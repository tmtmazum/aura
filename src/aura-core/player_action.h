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

inline player_action make_deploy_action(int uid, int lane_no)
{
  return player_action{action_type::deploy, uid, lane_no};
}

inline player_action make_end_turn_action()
{
  return player_action{action_type::end_turn, 0, 0};
}

} // namespace aura
