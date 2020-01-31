#pragma once

#include "card_preset.h"

namespace aura
{

#define LEGAL_ASSERT(a, msg) \
  if (!(a)) \
  { \
    auto const e = make_error_code(rules_error::not_legal); \
    AURA_ERROR(e, msg L" | " #a); \
    return e; \
  }

inline card_action_t generic_healer()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    LEGAL_ASSERT(!card_actor.is_resting(), L"Player cannot take action when resting");
    LEGAL_ASSERT(card_actor.strength, L"This unit cannot attack");
    LEGAL_ASSERT(target.health < target.starting_health, L"Targetted unit is already at max health!");
    LEGAL_ASSERT(!target.has_trait(unit_traits::structure), L"Healer cannot repair structures!");

    auto const added_health = target.health - card_actor.effective_strength();
    target.health = std::min(added_health, target.starting_health);
    card_actor.energy--;
    return std::error_code{};
  };
}

inline card_action_t generic_bard()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    LEGAL_ASSERT(!card_actor.is_resting(), L"Player cannot take action when resting");
    LEGAL_ASSERT(target.is_resting(), L"Targetted unit is already ready for action");
    LEGAL_ASSERT(!target.has_trait(unit_traits::structure), L"Healer cannot repair structures!");

    target.energy = target.starting_energy;

    card_actor.energy--;
    return std::error_code{};
  };
}

inline card_action_t generic_damage_dealer()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    LEGAL_ASSERT(!card_actor.is_resting(), L"Player cannot take action when resting");
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

    LEGAL_ASSERT(card_actor.strength > 0, L"This unit cannot deal damage");
    auto const reduced_health = target.health - card_actor.effective_strength();
    if (target.has_trait(unit_traits::damage_trap))
    {
      auto const diff = std::min(target.health, card_actor.effective_strength());
      card_actor.health = card_actor.health - diff;
    }
    target.health = reduced_health;
    card_actor.energy--;
    return std::error_code{};
  };
}

inline card_action_t library_deploy()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& cur_player = session.players[session.current_player];
    cur_player.num_draws_per_turn++;
    return std::error_code{};
  };
}

inline card_action_t library_death()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& cur_player = session.players[!session.current_player];
    cur_player.num_draws_per_turn--;
    return std::error_code{};
  };
}

inline card_action_t health_shrine_deploy()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& cur_player = session.players[session.current_player];
    cur_player.starting_health += 5;
    return std::error_code{};
  };
}

inline card_action_t health_shrine_death()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& cur_player = session.players[!session.current_player];
    cur_player.starting_health -= 5;
    return std::error_code{};
  };
}

inline card_action_t arcane_temple_deploy()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& cur_player = session.players[session.current_player];
    cur_player.starting_mana++;
    return std::error_code{};
  };
}

inline card_action_t arcane_temple_death()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& cur_player = session.players[!session.current_player];
    cur_player.starting_mana--;
    return std::error_code{};
  };
}

inline card_action_t generic_health_potion()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& player = session.players[session.current_player];
    LEGAL_ASSERT(card_actor.cost <= player.mana, L"Insufficient mana to deploy that card");
    LEGAL_ASSERT(target.health < target.starting_health, L"Targetted unit is already at max health!");
    LEGAL_ASSERT(!target.has_trait(unit_traits::structure), L"This item cannot repair structures!");

    player.mana -= card_actor.cost;

    auto const added_health = target.health - card_actor.effective_strength();
    target.health = std::min(added_health, target.starting_health);

    session.remove_hand_card(card_actor.uid);
    return std::error_code{};
  };
}

inline card_action_t generic_mana_potion()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& player = session.players[session.current_player];
    LEGAL_ASSERT(card_actor.cost <= player.mana, L"Insufficient mana to deploy that card");
    LEGAL_ASSERT(!target.has_trait(unit_traits::structure), L"This item cannot repair structures!");

    player.mana += card_actor.effective_strength();

    session.remove_hand_card(card_actor.uid);
    return std::error_code{};
  };
}

inline card_action_t generic_damage_potion()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& player = session.players[session.current_player];
    LEGAL_ASSERT(card_actor.cost <= player.mana, L"Insufficient mana to deploy that card");
    auto const added_health = target.health - card_actor.effective_strength();
    target.health = std::min(added_health, target.starting_health);

    player.mana -= card_actor.cost;

    session.remove_hand_card(card_actor.uid);
    return std::error_code{};
  };
}

inline card_action_t hail_storm()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& player = session.players[session.current_player];
    auto& target_player = session.players[!session.current_player];
    std::vector<card_info*> targets;
    for (auto& lane : target_player.lanes)
    {
      if (!lane.empty())
      {
        targets.emplace_back(&lane.back());
      }
    }

    for (auto* c : targets)
    {
      auto const added_health = c->health - card_actor.effective_strength();
      c->health = std::min(added_health, c->starting_health);
    }
    player.mana -= card_actor.cost;

    session.remove_hand_card(card_actor.uid);
    return std::error_code{};
  };
}

inline card_action_t incendiary()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& player = session.players[session.current_player];
    LEGAL_ASSERT(card_actor.cost <= player.mana, L"Insufficient mana to deploy that card");

    auto const target_lane = [&]() -> std::vector<card_info>*
    {
      for (auto& p : session.players)
      {
        for (int i = 0; i < p.lanes.size(); ++i)
        {
          for (auto& card : p.lanes[i])
          {
            if (card.uid == target.uid)
            {
              return &(p.lanes[i]);
            }
          }
        }
      }
      return nullptr;
    }();

    if (!target_lane) // attack on player champion
    {
      auto const added_health = target.health - card_actor.effective_strength();
      target.health = std::min(added_health, target.starting_health);
    }
    else
    {
      for (auto& target_card : *target_lane)
      {
        auto const added_health = target_card.health - card_actor.effective_strength();
        target_card.health = std::min(added_health, target_card.starting_health);
      }
    }

    player.mana -= card_actor.cost;

    session.remove_hand_card(card_actor.uid);
    return std::error_code{};
  };
}

inline card_action_t hero_attack()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& player = session.players[session.current_player];
    LEGAL_ASSERT(card_actor.cost <= player.mana, L"Insufficient mana to deploy that card");
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

    auto const added_health = target.health - card_actor.effective_strength();
    target.health = std::min(added_health, target.starting_health);

    player.mana -= card_actor.cost;

    session.remove_hand_card(card_actor.uid);
    return std::error_code{};
  };
}

inline card_action_t hero_focus()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& player = session.players[session.current_player];
    LEGAL_ASSERT(card_actor.cost <= player.mana, L"Insufficient mana to deploy that card");

    re.trigger_pick_action(1);
    player.mana -= card_actor.cost;
    //session.remove_hand_card(card_actor.uid);
    return std::error_code{};
  };
}


inline card_action_t vigor_potion()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& player = session.players[session.current_player];
    LEGAL_ASSERT(card_actor.cost <= player.mana, L"Insufficient mana to deploy that card");
    LEGAL_ASSERT(!target.has_trait(unit_traits::structure), L"This item cannot target structures!");
    LEGAL_ASSERT(target.is_resting(), L"Target is not resting!");

    player.mana -= card_actor.cost;

    target.energy = target.starting_energy;

    session.remove_hand_card(card_actor.uid);
    return std::error_code{};
  };
}

inline card_action_t speed_potion()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& player = session.players[session.current_player];
    LEGAL_ASSERT(card_actor.cost <= player.mana, L"Insufficient mana to deploy that card");
    LEGAL_ASSERT(!target.has_trait(unit_traits::structure), L"This item cannot target structures!");

    player.mana -= card_actor.cost;

    target.starting_energy++;

    session.remove_hand_card(card_actor.uid);
    return std::error_code{};
  };
}


using ut = unit_traits;
using tt = terrain_types;

struct card_preset_list : public std::vector<card_preset>
{
  template <typename... Args>
  card_preset_list(Args&&... args)
    : std::vector<card_preset>{std::forward<Args>(args)...}
  {}

  auto const& operator[](std::wstring_view name) const noexcept
  {
    for (auto const& card : *this)
    {
      if (card.name == name)
      {
        return card;
      }
    }
    AURA_ERROR(make_error_code(std::errc::not_supported), L"Failed to find card %ls", std::wstring(name).c_str());
    return (*this)[0];
  }
};

card_preset_list const loot = {
//card_preset{ <name>,            <cost>, <strength>, <health>, <energy>, {<traits>}, primary, on_deploy, on_death}
  card_preset{L"Animal Meat", L"heals 1 HP for any unit", 0, -1, 0, 1, {ut::item}, {}, generic_health_potion()},
  card_preset{L"Gold Coin", L"grants 1 extra mana", 0, 1, 0, 1, {ut::item}, {}, generic_mana_potion()},
  card_preset{L"Cursed Arrow", L"does 3 damage on any unit", 1, 3, 0, 1, {ut::item}, {}, generic_damage_potion()},
  card_preset{L"Healing Herb", L"heals 2 HP for any unit", 1, -2, 0, 1, {ut::item}, {}, generic_health_potion()},
};

inline card_action_t drop_loot_animal_meat()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& player = session.players[session.current_player];
    player.hand.emplace_back(re.to_card_info(loot[L"Animal Meat"], 0));
    return std::error_code();
  };
}

inline card_action_t drop_loot_healing_herb()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& player = session.players[session.current_player];
    player.hand.emplace_back(re.to_card_info(loot[L"Healing Herb"], 0));
    return std::error_code();
  };
}


inline card_action_t drop_loot_gold_coin()
{
  return [](auto& re, session_info& session, card_info& card_actor, card_info& target)
  {
    auto& player = session.players[session.current_player];
    player.hand.emplace_back(re.to_card_info(loot[L"Gold Coin"], 0));
    return std::error_code();
  };
}

std::vector<card_preset> const presets = {
//card_preset{ <name>,            <cost>, <strength>, <health>, <energy>, {<traits>}, primary, on_deploy, on_death}

  // Level 0 Cards
  card_preset{L"Barricade", L"", 0, 0, 1, 1, {ut::structure}, {}}, // DON'T MOVE THIS
  card_preset{L"Spike Trap", L"attacker concedes 1 damage", 0, 0, 1, 1, {ut::structure, ut::damage_trap}, {}},

  // Level 1 Cards - Structure
  card_preset{L"Fortification", L"", 1, 0, 2, 1, {ut::structure}, {}},

  // Level 1 Cards - Infantry
  card_preset{L"Militia", L"", 1, 1, 1, 1, {ut::infantry}, {tt::plains}, generic_damage_dealer()},
  card_preset{L"Hound", L"can attack twice per turn; drops loot: Animal Meat", 1, 1, 1, 2, {ut::infantry, ut::twice}, {tt::forests}, generic_damage_dealer(), nullptr, drop_loot_animal_meat()},
  card_preset{L"Thief", L"", 1, 1, 1, 1, {ut::infantry, ut::assassin}, {tt::forests}, generic_damage_dealer(), nullptr, drop_loot_gold_coin()},
  card_preset{L"Herbalist Healer", L"", 1, -1, 1, 2, {ut::infantry, ut::twice, ut::healer}, {}, generic_healer(), drop_loot_healing_herb()},
  card_preset{L"Bard", L"can arouse units, allowing them to act again if resting", 1, 0, 1, 2, {ut::infantry}, {}, generic_bard()},

  // Level 1 Cards - Items
  card_preset{L"Potion of Vigor", L"allows the unit to act again (if resting)", 1, 1, 0, 1, {ut::item}, {}, vigor_potion()},

  // Level 2 Cards - Items
  card_preset{L"Health Potion", L"heals 5 HP for any unit", 2, -5, 0, 1, {ut::item}, {}, generic_health_potion()},
  card_preset{L"Incendiary", L"does 1 damage on all units in a lane OR to the enemy champion", 1, 1, 0, 1, {ut::item}, {}, incendiary()},
  card_preset{L"Hail Storm", L"does 1 damage on all front enemy units", 2, 1, 0, 1, {ut::item}, {}, hail_storm()},
  card_preset{L"Potion of Speed", L"permanently increases the # of times a unit can act per turn", 2, 1, 0, 1, {ut::item}, {}, speed_potion()},

  // Level 2 Cards - Infantry
  card_preset{L"Guardsman", L"", 2, 1, 2, 1, {ut::infantry}, {tt::plains}, generic_damage_dealer()},
  card_preset{L"Archer", L"", 2, 1, 1, 1, {ut::infantry, ut::long_range}, {tt::mountains}, generic_damage_dealer()},
  card_preset{L"Apprentice Assassin", L"", 2, 2, 1, 1, {ut::infantry, ut::assassin}, {tt::forests}, generic_damage_dealer()},
  card_preset{L"Cleric", L"", 2, -3, 2, 2, {ut::infantry, ut::twice, ut::healer}, {}, generic_healer()},
  card_preset{L"Wind Dancer", L"", 2, 1, 1, 3, {ut::infantry, ut::long_range, ut::thrice}, {tt::mountains}, generic_damage_dealer()},

  // Level 3 Cards - Infantry
  card_preset{L"Adept Archer", L"", 3, 2, 2, 1, {ut::infantry, ut::long_range}, {tt::mountains}, generic_damage_dealer()},
  card_preset{L"Speedy Archer", L"", 3, 1, 2, 2, {ut::infantry, ut::long_range}, {tt::mountains}, generic_damage_dealer()},
  card_preset{L"Adept Guardian", L"", 3, 2, 5, 1, {ut::infantry}, {tt::plains}, generic_damage_dealer()},
  card_preset{L"Knight", L"", 3, 3, 3, 1, {ut::infantry}, {tt::plains}, generic_damage_dealer()},

  // Level 3 Cards - Structures
  card_preset{L"Library", L"Draw an additional card per turn while this card is in play", 3, 0, 3, 1, {ut::structure}, {}, generic_damage_dealer(), library_deploy(), library_death()},
  card_preset{L"Elder Shrine", L"Increases player's max health by 5", 3, 0, 3, 1, {ut::structure}, {}, generic_damage_dealer(), health_shrine_deploy(), health_shrine_death()},

  // Level 4 Cards - Items
  card_preset{L"Health Elixir", L"heals upto 10 HP for any unit", 4, -10, 0, 1, {ut::item}, {}, generic_health_potion()},
  card_preset{L"Royal-Hound", L"can attack twice per turn", 4, 2, 1, 2, {ut::infantry, ut::twice}, {tt::forests}, generic_damage_dealer()},
  card_preset{L"Hound Pack", L"can attack three times per turn", 4, 2, 1, 3, {ut::infantry, ut::twice}, {tt::forests}, generic_damage_dealer()},
  card_preset{L"Med Fortification", L"", 4, 0, 5, 1, {ut::structure}, {}},
  card_preset{L"Adept Assassin", L"", 4, 4, 2, 1, {ut::infantry, ut::assassin}, {tt::forests}, generic_damage_dealer()},
  card_preset{L"Enchanted Tower", L"", 4, 1, 7, 1, {ut::structure, ut::long_range}, {}, generic_damage_dealer()},
  card_preset{L"Arcane Temple", L"grants 1 extra mana per turn", 4, 0, 5, 1, {ut::structure}, {}, arcane_temple_deploy(), arcane_temple_death()},

  // Level 5 Cards - Infantry
  card_preset{L"Wrath Dragon", L"", 5, 5, 5, 1, {ut::aerial}, {tt::mountains}, generic_damage_dealer()},
  card_preset{L"The Giantess", L"", 5, 2, 8, 1, {ut::infantry}, {tt::mountains}, generic_damage_dealer()},
  card_preset{L"Windswalker", L"", 5, 3, 4, 2, {ut::infantry, ut::long_range, ut::twice}, {tt::mountains}, generic_damage_dealer()},
  card_preset{L"The Grey Hood", L"", 5, 3, 5, 1, {ut::infantry, ut::assassin, ut::long_range}, {tt::forests}, generic_damage_dealer()},
};

std::vector<card_preset> const specials = {
  card_preset{L"Hero Attack", L"does 1 damage on any front-lane unit", 1, 1, 0, 1, {ut::item, ut::hero_power}, {}, hero_attack()},
  card_preset{L"Hero Focus", L"pick 2 new cards", 2, 1, 0, 1, {ut::item, ut::hero_power}, {}, hero_focus()}
  //card_preset{L"Hero Charge", L"draw 3 random cards", 1, 1, 0, 1, {ut::item}, hero_attack()}
};

inline auto make_standard_deck()
{
  std::unordered_map<std::wstring, card_preset> cards;

  for (auto const& p : presets)
  {
    cards.emplace(p.name, p);
  }

  deck d{};
  for (auto& [a, b] : cards)
  {
    d.add(b, 2);
  }
  return d;
  //deck d{};
  //d.add(cards[L"Small Fortification"], 2);
  //d.add(cards[L"Healing Herb"], 2);

  //d.add(cards[L"Militia"], 4);
  //d.add(cards[L"Fortification"], 4);
  //d.add(cards[L"Guardsman"], 4);
  //d.add(cards[L"Archer"], 4);
  //d.add(cards[L"Adept Guardian"], 4);
  //d.add(cards[L"Apprentice Assassin"], 4);
  //d.add(cards[L"Hound"], 4);
  //d.add(cards[L"Knight"], 4);

  //d.add(cards[L"Health Potion"], 2);
  //d.add(cards[L"Cursed Arrow"], 2);

  //d.add(cards[L"Potion of Vigor"], 2);
  //d.add(cards[L"Herbalist Healer"], 2);
  //d.add(cards[L"Cleric"], 2);
  //d.add(cards[L"Speedy Archer"], 2);

  //d.add(cards[L"Library"], 4);
  //d.add(cards[L"Elder Shrine"], 4);
  //d.add(cards[L"Arcane Temple"], 4);

  //d.add(cards[L"Health Elixir"], 1);
  //d.add(cards[L"Royal-Hound"], 2);
  //d.add(cards[L"Hound Pack"], 2);
  //d.add(cards[L"Med Fortification"], 2);
  //d.add(cards[L"Adept Assassin"], 2);
  //d.add(cards[L"Enchanted Tower"], 2);
  //d.add(cards[L"Wrath Dragon"], 2);
  //d.add(cards[L"The Giantess"], 2);
  //d.add(cards[L"Windswalker"], 2);
  //d.add(cards[L"The Grey Hood"], 2);
  //return d;
}

inline auto make_all_soldier_deck()
{
  std::unordered_map<std::wstring, card_preset> cards;

  for (auto const& p : presets)
  {
    cards.emplace(p.name, p);
  }

  deck d;
  d.add(cards[L"Militia"], 1);
  return d;
}

inline auto make_3soldier_deck()
{
  std::unordered_map<std::wstring, card_preset> cards;

  for (auto const& p : presets)
  {
    cards.emplace(p.name, p);
  }

  auto d = make_standard_deck();
  d.add_fixed_pick(1, cards[L"Militia"]);
  d.add_fixed_pick(1, cards[L"Militia"]);
  d.add_fixed_pick(1, cards[L"Militia"]);
  d.add_fixed_pick(1, cards[L"Militia"]);
  return d;
}
inline auto make_all_soldier_counter_deck()
{
  std::unordered_map<std::wstring, card_preset> cards;

  for (auto const& p : presets)
  {
    cards.emplace(p.name, p);
  }

  auto d = make_standard_deck();
  d.add_fixed_pick(1, cards[L"Militia"]);
  d.add_fixed_pick(1, cards[L"Militia"]);
  d.add_fixed_pick(1, cards[L"Militia"]);
  d.add_fixed_pick(1, cards[L"Militia"]);
  for (int i = 2; i < 50; i++)
  {
    d.add_fixed_pick(i, cards[L"Militia"]);
  }
  return d;

}

inline auto make_3soldier_counter_deck()
{
  std::unordered_map<std::wstring, card_preset> cards;

  for (auto const& p : presets)
  {
    cards.emplace(p.name, p);
  }

  auto d = make_standard_deck();
  d.add_fixed_pick(1, cards[L"Spike Trap"]);
  d.add_fixed_pick(1, cards[L"Spike Trap"]);
  d.add_fixed_pick(1, cards[L"Hail Storm"]);
  d.add_fixed_pick(1, cards[L"Incendiary"]);
  return d;
}

#undef LEGAL_ASSERT


} // namespace aura
