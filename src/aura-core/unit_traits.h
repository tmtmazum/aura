#pragma once

namespace aura
{

enum class unit_traits : int
{
  infantry,
  aerial,
  structure,
  player,
  item,
  hero_power,

  damage_trap,
  twice,
  thrice,
	assassin,		//!< can attack upon placement without rest
	long_range,	//!< can attack any unit in any lane regardless of what's blocking it
  healer
};

//! affects what symbol is shown when highlighting 
//! other (target) cards while this card is selected.
enum class card_action_type
{
  melee_attack,
  ranged_attack,
  healer,
  spell,
  none
};

using cay = card_action_type;

//! affects what other cards can be targetted while this
//! one is selected
enum class card_action_targets
{
  none,
  friendly_hero = 0x1, // player
  friendly = 0x1 | 0x2,
  enemy = 0x4,
  both = 0x1 | 0x2 | 0x4
};

using cat = card_action_targets;

} // namespace aura

