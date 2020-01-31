#pragma once

#include <unordered_map> 

namespace aura
{

enum class cind_action_type : unsigned
{
  none                = 0b000000000,
  hovered_lane        = 0b000000001,
  hovered_hand_card   = 0b000000010,
  hovered_lane_card   = 0b000000100,
  hovered_end_turn    = 0b000001000,
  hovered_player      = 0b000010000,
  hovered_pick_card   = 0b000100000,
  selected_lane       = 0b001000000,
  selected_hand_card  = 0b010000000,
  selected_lane_card  = 0b100000000,
};

using uiact = cind_action_type;

struct cind_action
{
  unsigned type{};
  std::unordered_map<cind_action_type, int> targets;

  cind_action() = default;

  void reset_hovered()
  {
    // keep only the selected; remove the hovered
    type &= 0b111000000;
  }

  void reset_selected()
  {
    // keep only the hovered; remove the selected
    type &= 0b000111111;
  }

  auto value(cind_action_type t) const
  {
    AURA_ASSERT(is(t));
    return targets.at(t);
  }

  void add(cind_action_type new_type, int target)
  {
    type |= static_cast<unsigned>(new_type);
    targets[new_type] = target;
  }

  void rm(cind_action_type new_type)
  {
    type &= ~static_cast<unsigned>(new_type);
  }

  cind_action(cind_action_type new_type, int target)
    : cind_action{}
  {
    add(new_type, target);
  }

  bool is(cind_action_type check) const noexcept
  {
    return static_cast<unsigned>(check) & type;
  }
};

} // namespace aura