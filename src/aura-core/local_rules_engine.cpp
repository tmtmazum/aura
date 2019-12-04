#include "local_rules_engine.h"
#include "aura-core/ruleset.h"
#include "aura-core/player_action.h"
#include "aura-core/build.h"
#include "aura-core/unit_traits.h"
#include "aura-core/card_preset.h"
#include <algorithm>
#include <optional>
#include <functional>
#include <cstdlib>
#include <ctime>

namespace aura
{

local_rules_engine::local_rules_engine(ruleset const& rs)
  : m_rules{rs}
{
  {
    player_info player{};
    player.starting_health = m_rules.challenger_starting_health;
    player.health = m_rules.challenger_starting_health;
    player.starting_mana = m_rules.challenger_starting_mana;
    player.mana = player.starting_mana;

    for (int i = 0; i < rs.num_lanes; ++i)
    {
      std::vector<aura::card_info> v;
      player.lanes.emplace_back(v);
    }
    
    for (int i = 0; i < m_rules.challenger_starting_cards; ++i)
    {
      player.hand.emplace_back(generate_card(rs, rs.challenger_deck));  
    }

    for (auto i = 0; i < rs.challenger_starts_with_n_forts; ++i)
    {
      player.hand.emplace_back(to_card_info(presets[0], 0));
    }
    m_session_info.players.emplace_back(player);
  }

  {
    player_info player{};
    player.starting_health = m_rules.defender_starting_health;
    player.health = m_rules.defender_starting_health;
    player.starting_mana = m_rules.challenger_starting_mana;
    player.mana = player.starting_mana;
    for (int i = 0; i < rs.num_lanes; ++i)
    {
      std::vector<aura::card_info> v;
      player.lanes.emplace_back(v);
    }
    
    for (int i = 0; i < m_rules.defender_starting_cards; ++i)
    {
      player.hand.emplace_back(generate_card(rs, rs.defender_deck));
    }

    for (auto i = 0; i < rs.defender_starts_with_n_forts; ++i)
    {
      player.hand.emplace_back(to_card_info(presets[0], 0));
    }

    m_session_info.players.emplace_back(player);
  }
}

#ifdef LEGAL_ASSERT
# error Oops legal assert is already defined
#endif

#define LEGAL_ASSERT(a, msg) \
  if (!(a)) \
  { \
    auto const e = make_error_code(rules_error::not_legal); \
    AURA_ERROR(e, msg L" | " #a); \
    return e; \
  }

std::vector<int> local_rules_engine::get_target_list(int uid) const
{
  return {};
}

card_info* local_rules_engine::find_actor(int uid)
{
  auto& player = m_session_info.players[m_session_info.current_player];
  for (auto& lane : player.lanes)
  {
    for (auto& card : lane)
    {
      if (card.uid == uid)
      {
        return &card;
      }
    }
  }
  return nullptr;
}

card_info* local_rules_engine::find_target(int uid)
{
  for (auto& player : m_session_info.players)
  {
    if (player.uid == uid)
    {
      return &player;
    }

    for (auto& lane : player.lanes)
    {
      for (auto& card : lane)
      {
        if (card.uid == uid)
        {
          return &card;
        }
      }
    }
  }
  return nullptr;
}

card_info local_rules_engine::to_card_info(card_preset const& preset, int cid)
{
  card_info info{};
  info.uid = generate_uid();
  info.cid = cid;
  info.health = preset.health;
  info.starting_health = preset.health;
  info.strength = preset.strength;
  info.cost = preset.cost;
  info.name = preset.name;
  info.traits = preset.traits;

  if (preset.primary)
  {
    auto [it, success] = m_primary_actions.emplace(info.uid, preset.primary);
    AURA_ASSERT(success);
  }
  return info;
}

card_info local_rules_engine::generate_card(ruleset const& rs, deck const& d, int turn)
{
  static auto ss = std::invoke([]
  {
    srand(time(0));
    return 0;
  });

  std::vector<card_preset> selection_pool;

  if (rs.draw_limit_multiplier > 0.1 && 
    ((rs.draw_limit_multiplier * turn) < 9))
  {
    auto const limit = (rs.draw_limit_multiplier * turn);
    for (auto const& p : presets)
    {
      if (p.cost <= limit)
      {
        selection_pool.emplace_back(p);
      }
    }
  }
  else
  {
    selection_pool = presets; 
  }

  auto const n = selection_pool.size();
  auto const i = (rand() % n);
  auto const& preset = selection_pool.at(i);

  return to_card_info(preset, i);
}


//! Commit a player action
std::error_code local_rules_engine::commit_action(player_action const& action) 
{
  switch (action.type)
  {
  case action_type::end_turn:
  {
    LEGAL_ASSERT(!m_session_info.game_over, L"Cannot end turn - game is already over.");
    if (m_session_info.current_player)
    {
      m_session_info.turn++;
      for (auto& player : m_session_info.players)
      {
        player.starting_mana++;
        player.mana = player.starting_mana;
        player.for_each_lane_card([](auto& card)
        {
          card.resting = false;
        });
      }
    }
    m_session_info.players[m_session_info.current_player]
      .hand.emplace_back(generate_card(m_rules, m_rules.challenger_deck, m_session_info.turn));
    m_session_info.current_player = !m_session_info.current_player;
    return {};
  }

  case action_type::forfeit:
  {
    LEGAL_ASSERT(!m_session_info.game_over, L"Cannot forfeit - game is already over.");
    m_session_info.game_over = true;
    return {};
  }

  case action_type::primary_action:
  {
    auto const card_actor_uid = action.target1;
    auto const card_target_uid = action.target2;

    auto* card_actor = find_actor(card_actor_uid);
    auto* card_target = find_target(card_target_uid);
    LEGAL_ASSERT(card_actor, L"No actor card with that identifier found in current player's hand");
    LEGAL_ASSERT(card_target, L"No target card with that identifier found");

    auto it = m_primary_actions.find(card_actor_uid);
    LEGAL_ASSERT(it != m_primary_actions.end(), L"No actions found for this unit!!");
    return it->second(m_session_info, *card_actor, *card_target);
  }

  case action_type::deploy:
  {
    auto& player = m_session_info.players[m_session_info.current_player];
    auto const card_id = action.target1;
    auto const lane_id = action.target2;
    LEGAL_ASSERT(lane_id >= 1 && lane_id <= m_rules.num_lanes, L"Lane identifier must be between 1 and 4");
    auto const it = std::find_if(begin(player.hand), end(player.hand), [&](auto const& card)
    {
      return card.uid == card_id;
    });
    LEGAL_ASSERT(it != end(player.hand), L"No card with that identifier was found in the current player's hand");
    LEGAL_ASSERT(it->cost <= player.mana, L"Insufficient mana to deploy that card");
    if (!it->has_trait(unit_traits::assassin))
    {
      it->resting = true;
    }
    player.mana -= it->cost;
    player.lanes[lane_id - 1].emplace_back(*it);
    player.hand.erase(it);
    return {};
  }

  case action_type::no_action:
  {
    return {};
  }
  }
  LEGAL_ASSERT(false, "Unrecognized action");
}

#undef LEGAL_ASSERT

} // namespace aura
