#pragma once

#include <aura-core/unit_traits.h>
#include <aura-core/rules_engine.h>
#include <aura-core/build.h>
#include <vector>
#include <system_error>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <unordered_map>

namespace aura
{

inline int generate_cid()
{
  static int cid_counter{0};
  return cid_counter++;
}

struct card_preset
{
  std::wstring name;
  std::wstring special_descr; //!< special description
  int cost;
  int strength;
  int health;
  int energy;

  std::vector<unit_traits> traits;
  card_action_t primary;
  // (session, deploying player, other player)
  card_action_t on_deploy{nullptr};
  // (session, owner player, other player)
  card_action_t on_death{nullptr};

  int cid = generate_cid();
};

struct deck
{
  std::vector<card_preset> all_cards;
  std::vector<card_preset> remaining_cards;

  void reset()
  {
    remaining_cards = all_cards;
  }

  auto make_restricted_pool(int turn, int max_level)
  {
    std::vector<card_preset> selection_pool;

    auto const limit = (max_level * turn);
    for (auto const& p : remaining_cards)
    {
      if (p.cost <= limit)
      {
        selection_pool.emplace_back(p);
      }
    }
    return selection_pool;
  }

  void remove_one(int cid)
  {
    using std::begin;
    using std::end;
    auto [b, e] = std::make_pair(begin(remaining_cards), end(remaining_cards));
    auto const it = std::find_if(b, e, [&](auto const& card)
    {
      return card.cid == cid;
    });

    if (it != e)
    {
      remaining_cards.erase(it);
    }
  }

  card_preset draw(int turn, int max_level)
  {
    static auto ss = std::invoke([]
    {
      srand(time(0));
      return 0;
    });

    if (remaining_cards.empty())
    {
      reset();
    }

    //auto& pool = remaining_cards;
    auto const& pool = (max_level < 9 ? make_restricted_pool(turn, max_level)
      : remaining_cards);

    AURA_ASSERT(!pool.empty());

    auto const n = pool.size();
    auto const i = (rand() % n);
    auto const preset = pool.at(i);
    remove_one(preset.cid);
    return preset;
  }

  void add(card_preset const& p, int n)
  {
    for (int i = 0; i < n; ++i)
    {
      all_cards.emplace_back(p);
    }
  }
};


#define LEGAL_ASSERT(a, msg) \
  if (!(a)) \
  { \
    auto const e = make_error_code(rules_error::not_legal); \
    AURA_ERROR(e, msg L" | " #a); \
    return e; \
  }

inline card_action_t generic_healer()
{
  return [](session_info& session, card_info& card_actor, card_info& target)
  {
    LEGAL_ASSERT(!card_actor.is_resting(), L"Player cannot take action when resting");
    LEGAL_ASSERT(card_actor.strength, L"This unit cannot attack");
    LEGAL_ASSERT(target.health != target.starting_health, L"Targetted unit is already at max health!");
    LEGAL_ASSERT(!target.has_trait(unit_traits::structure), L"Healer cannot repair structures!");

    auto const added_health = target.health - card_actor.strength;
    target.health = std::min(added_health, target.starting_health);
    card_actor.energy++;
    return std::error_code{};
  };
}

inline card_action_t generic_damage_dealer()
{
  return [](session_info& session, card_info& card_actor, card_info& target)
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

    auto const reduced_health = target.health - card_actor.strength;
    target.health = reduced_health;
    card_actor.energy--;
    return std::error_code{};
  };
}

inline card_action_t library_deploy()
{
  return [](session_info& session, card_info& card_actor, card_info& target)
  {
    auto& cur_player = session.players[session.current_player];
    cur_player.num_draws_per_turn++;
    return std::error_code{};
  };
}

inline card_action_t library_death()
{
  return [](session_info& session, card_info& card_actor, card_info& target)
  {
    auto& cur_player = session.players[!session.current_player];
    cur_player.num_draws_per_turn--;
    return std::error_code{};
  };
}

inline card_action_t health_shrine_deploy()
{
  return [](session_info& session, card_info& card_actor, card_info& target)
  {
    auto& cur_player = session.players[session.current_player];
    cur_player.starting_health += 5;
    return std::error_code{};
  };
}

inline card_action_t health_shrine_death()
{
  return [](session_info& session, card_info& card_actor, card_info& target)
  {
    auto& cur_player = session.players[!session.current_player];
    cur_player.starting_health -= 5;
    return std::error_code{};
  };
}

inline card_action_t arcane_temple_deploy()
{
  return [](session_info& session, card_info& card_actor, card_info& target)
  {
    auto& cur_player = session.players[session.current_player];
    cur_player.starting_mana++;
    return std::error_code{};
  };
}

inline card_action_t arcane_temple_death()
{
  return [](session_info& session, card_info& card_actor, card_info& target)
  {
    auto& cur_player = session.players[!session.current_player];
    cur_player.starting_mana--;
    return std::error_code{};
  };
}

inline card_action_t generic_health_potion()
{
  return [](session_info& session, card_info& card_actor, card_info& target)
  {
    auto& player = session.players[session.current_player];
    LEGAL_ASSERT(card_actor.cost <= player.mana, L"Insufficient mana to deploy that card");
    LEGAL_ASSERT(target.health != target.starting_health, L"Targetted unit is already at max health!");
    LEGAL_ASSERT(!target.has_trait(unit_traits::structure), L"This item cannot repair structures!");

    player.mana -= card_actor.cost;

    auto const added_health = target.health - card_actor.strength;
    target.health = std::min(added_health, target.starting_health);

    session.remove_hand_card(card_actor.uid);
    return std::error_code{};
  };
}

inline card_action_t generic_damage_potion()
{
  return [](session_info& session, card_info& card_actor, card_info& target)
  {
    auto& player = session.players[session.current_player];
    LEGAL_ASSERT(card_actor.cost <= player.mana, L"Insufficient mana to deploy that card");
    auto const added_health = target.health - card_actor.strength;
    target.health = std::min(added_health, target.starting_health);

    player.mana -= card_actor.cost;

    session.remove_hand_card(card_actor.uid);
    return std::error_code{};
  };
}

inline card_action_t hero_attack()
{
  return [](session_info& session, card_info& card_actor, card_info& target)
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

    auto const added_health = target.health - card_actor.strength;
    target.health = std::min(added_health, target.starting_health);

    player.mana -= card_actor.cost;

    session.remove_hand_card(card_actor.uid);
    return std::error_code{};
  };
}


inline card_action_t vigor_potion()
{
  return [](session_info& session, card_info& card_actor, card_info& target)
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

using ut = unit_traits;

std::vector<card_preset> const presets = {
//card_preset{ <name>,            <cost>, <strength>, <health>, <energy>, {<traits>}, primary}
  card_preset{L"Small Fortification", L"", 0, 0, 1, 1, {ut::structure}}, // DON'T MOVE THIS
  card_preset{L"Healing Herb", L"heals 1 HP for any unit", 0, -1, 0, 1, {ut::item}, generic_health_potion()},
  card_preset{L"Fortification", L"", 1, 0, 2, 1, {ut::structure}},
  card_preset{L"Foot Soldier", L"", 1, 1, 1, 1, {ut::infantry}, generic_damage_dealer()},
  card_preset{L"Health Potion", L"heals 2 HP for any unit", 2, -2, 0, 1, {ut::item}, generic_health_potion()},
  card_preset{L"Cursed Arrow", L"does 2 damage on any unit", 2, 1, 0, 1, {ut::item}, generic_damage_potion()},
  card_preset{L"Potion of Vigor", L"allows the unit to act again (if resting)", 2, 1, 0, 1, {ut::item}, vigor_potion()},
  card_preset{L"Guardsman", L"", 2, 1, 2, 1, {ut::infantry}, generic_damage_dealer()},
  card_preset{L"Hound", L"can attack twice per turn", 2, 1, 1, 2, {ut::infantry, ut::twice}, generic_damage_dealer()},
  card_preset{L"Herbalist Healer", L"", 2, -1, 1, 1, {ut::infantry, ut::healer}, generic_healer()},
  card_preset{L"Archer", L"", 2, 1, 1, 1, {ut::infantry, ut::long_range}, generic_damage_dealer()},
  card_preset{L"Doctor", L"", 3, -3, 2, 1, {ut::infantry, ut::healer}, generic_healer()},
  card_preset{L"Speedy Archer", L"", 3, 1, 2, 2, {ut::infantry, ut::long_range}, generic_damage_dealer()},
  card_preset{L"Adept Guardian", L"", 3, 2, 5, 1, {ut::infantry}, generic_damage_dealer()},
  card_preset{L"Knight", L"", 3, 3, 3, 1, {ut::infantry}, generic_damage_dealer()},
  card_preset{L"Library", L"Draw an additional card per turn while this card is in play", 3, 0, 3, 1, {ut::structure}, generic_damage_dealer(), library_deploy(), library_death()},
  card_preset{L"Elder Shrine", L"Increases player's max health by 5", 3, 0, 3, 1, {ut::structure}, generic_damage_dealer(), health_shrine_deploy(), health_shrine_death()},
  card_preset{L"Apprentice Assassin", L"", 3, 2, 1, 1, {ut::infantry, ut::assassin}, generic_damage_dealer()},
  card_preset{L"Health Elixir", L"heals 5 HP for any unit", 4, -5, 0, 1, {ut::item}, generic_health_potion()},
  card_preset{L"Royal-Hound", L"can attack twice per turn", 4, 2, 1, 2, {ut::infantry, ut::twice}, generic_damage_dealer()},
  card_preset{L"Hound Pack", L"can attack three times per turn", 4, 1, 1, 3, {ut::infantry, ut::twice}, generic_damage_dealer()},
  card_preset{L"Med Fortification", L"", 4, 0, 5, 1, {ut::structure}},
  card_preset{L"Adept Assassin", L"", 4, 4, 2, 1, {ut::infantry, ut::assassin}, generic_damage_dealer()},
  card_preset{L"Enchanted Tower", L"", 4, 1, 7, 1, {ut::structure, ut::long_range}, generic_damage_dealer()},
  card_preset{L"Arcane Temple", L"grants 1 extra mana per turn", 4, 0, 5, 1, {ut::structure}, arcane_temple_deploy(), arcane_temple_death()},
  card_preset{L"Wrath Dragon", L"", 5, 5, 5, 1, {ut::aerial}, generic_damage_dealer()},
  card_preset{L"The Giantess", L"", 5, 2, 8, 1, {ut::infantry}, generic_damage_dealer()},
  card_preset{L"Windswalker", L"", 5, 3, 4, 2, {ut::infantry, ut::long_range, ut::twice}, generic_damage_dealer()},
  card_preset{L"The Grey Hood", L"", 5, 3, 5, 1, {ut::infantry, ut::assassin, ut::long_range}, generic_damage_dealer()},
};

std::vector<card_preset> const specials = {
  card_preset{L"Hero Attack", L"does 1 damage on any front-lane unit", 1, 1, 0, 1, {ut::item, ut::hero_power}, hero_attack()}
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
  d.add(cards[L"Small Fortification"], 2);
  d.add(cards[L"Healing Herb"], 2);

  d.add(cards[L"Foot Soldier"], 8);
  d.add(cards[L"Fortification"], 4);
  d.add(cards[L"Guardsman"], 4);
  d.add(cards[L"Archer"], 4);
  d.add(cards[L"Adept Guardian"], 4);
  d.add(cards[L"Apprentice Assassin"], 4);
  d.add(cards[L"Hound"], 4);
  d.add(cards[L"Knight"], 4);

  d.add(cards[L"Health Potion"], 2);
  d.add(cards[L"Cursed Arrow"], 2);

  d.add(cards[L"Potion of Vigor"], 2);
  d.add(cards[L"Herbalist Healer"], 2);
  d.add(cards[L"Doctor"], 2);
  d.add(cards[L"Speedy Archer"], 2);

  d.add(cards[L"Library"], 4);
  d.add(cards[L"Elder Shrine"], 4);
  d.add(cards[L"Arcane Temple"], 4);

  d.add(cards[L"Health Elixir"], 1);
  d.add(cards[L"Royal-Hound"], 1);
  d.add(cards[L"Hound Pack"], 1);
  d.add(cards[L"Med Fortification"], 2);
  d.add(cards[L"Adept Assassin"], 2);
  d.add(cards[L"Enchanted Tower"], 1);
  d.add(cards[L"Wrath Dragon"], 1);
  d.add(cards[L"The Giantess"], 1);
  d.add(cards[L"Windswalker"], 1);
  d.add(cards[L"The Grey Hood"], 1);
  return d;
}

#undef LEGAL_ASSERT

} // namespace aura
