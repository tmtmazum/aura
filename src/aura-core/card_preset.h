#pragma once

#include <aura-core/unit_traits.h>
#include <aura-core/rules_engine.h>
#include <aura-core/build.h>
#include <vector>
#include <system_error>

namespace aura
{

struct card_preset
{
  std::wstring name;
  int cost;
  int strength;
  int health;

  std::vector<unit_traits> traits;
  primary_action_t primary;
};

#define LEGAL_ASSERT(a, msg) \
  if (!(a)) \
  { \
    auto const e = make_error_code(rules_error::not_legal); \
    AURA_ERROR(e, msg L" | " #a); \
    return e; \
  }

inline primary_action_t generic_damage_dealer()
{
  return [](session_info& session, card_info& card_actor, card_info& target)
  {
    LEGAL_ASSERT(!card_actor.resting, L"Player cannot take action when resting");
    LEGAL_ASSERT(card_actor.strength, L"This unit cannot attack");

    bool is_actor_long_range = card_actor.has_trait(unit_traits::long_range);

    if (!is_actor_long_range)
    {
      // insert front of lane check
      if (target.has_trait(unit_traits::player))
      {
        auto const has_free_lane = session.players[!session.current_player].has_free_lane();
        LEGAL_ASSERT(has_free_lane, L"There are no free lanes available to target the enemy champion");
      }
      else
      {
        LEGAL_ASSERT(session.is_front_of_lane(target.uid),
          L"This unit type can only target enemies at the front of their lane");
      }
    }

    auto const reduced_health = target.health - card_actor.strength;
    if (reduced_health > 0)
    {
      target.health = reduced_health;
    }
    else
    {
      if (target.has_trait(unit_traits::player))
      {
        AURA_LOG(L"Player %d has won the game!", session.current_player);
        session.game_over = true;
      }
      else
      {
        session.remove_lane_card(target.uid);
      }
    }
    card_actor.resting = true;
    return std::error_code{};
  };
}

using ut = unit_traits;

std::vector<card_preset> const presets = {
  card_preset{L"Small Fortification", 0, 0, 1, {ut::structure}}, // DON'T MOVE THIS
  card_preset{L"Fortification", 1, 0, 2, {ut::structure}},
  card_preset{L"Soldier", 1, 1, 1, {ut::infantry}, generic_damage_dealer()},
  card_preset{L"Guardsman", 2, 1, 2, {ut::infantry}, generic_damage_dealer()},
  card_preset{L"Doctor", 2, -1, 1, {ut::infantry, ut::healer}, generic_damage_dealer()},
  card_preset{L"Archer", 2, 1, 1, {ut::infantry, ut::long_range}, generic_damage_dealer()},
  card_preset{L"Apprentice Assassin", 2, 2, 1, {ut::infantry, ut::assassin}, generic_damage_dealer()},
  card_preset{L"Med Fortification", 4, 0, 5, {ut::structure}},
  card_preset{L"Adept Assassin", 3, 4, 2, {ut::infantry, ut::assassin}, generic_damage_dealer()},
  card_preset{L"Adept Guardian", 3, 2, 5, {ut::infantry}, generic_damage_dealer()},
  card_preset{L"Knight", 3, 3, 3, {ut::infantry}, generic_damage_dealer()},
  card_preset{L"Enchanted Tower", 4, 1, 7, {ut::structure, ut::long_range}, generic_damage_dealer()},
  card_preset{L"Wrath Dragon", 5, 5, 5, {ut::aerial}, generic_damage_dealer()},
  card_preset{L"The Giantess", 5, 2, 8, {ut::infantry}, generic_damage_dealer()},
  card_preset{L"The Grey Hood", 5, 3, 5, {ut::infantry, ut::assassin, ut::long_range}, generic_damage_dealer()},
};

#undef LEGAL_ASSERT

} // namespace aura
