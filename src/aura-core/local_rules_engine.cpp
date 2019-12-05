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
  for (auto& card : player.hand)
  {
    if (card.uid == uid)
    {
      return &card;
    }
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
  info.energy = preset.energy;
  info.starting_energy = preset.energy;
  info.description = preset.special_descr;

  if (preset.primary)
  {
    auto [it, success] = m_primary_actions.emplace(info.uid, preset.primary);
    AURA_ASSERT(success);
  }

  if (preset.on_deploy)
  {
    auto [it, success] = m_deploy_actions.emplace(info.uid, preset.on_deploy);
    AURA_ASSERT(success);
  }

  if (preset.on_death)
  {
    auto [it, success] = m_death_actions.emplace(info.uid, preset.on_death);
    AURA_ASSERT(success);
  }
  return info;
}

card_info local_rules_engine::generate_card(ruleset const& rs, deck& d, int turn)
{
  static auto ss = std::invoke([]
  {
    srand(time(0));
    return 0;
  });

  auto const& preset = d.draw(turn, rs.draw_limit_multiplier * turn);

  return to_card_info(preset, 0);
}

std::wstring local_rules_engine::describe(unit_traits trait) const noexcept
{
  switch (trait)
  {
  case unit_traits::infantry: return L"infantry: traverses by land";
  case unit_traits::aerial:  return L"aerial: traverses by air";
  case unit_traits::structure: return L"structure: good for providing cover and bonuses";
  case unit_traits::player: return L"player: player champion";
  case unit_traits::item: return L"item: can be applied to card on board";
  case unit_traits::twice: return L"can act twice per turn";
  case unit_traits::thrice: return L"can act three times per turn";
  case unit_traits::assassin: return L"assassin: can attack same turn as deployment without rest";
  case unit_traits::long_range: return L"long-range: can target any enemy unit (regardless of lane obstructions)";
  case unit_traits::healer: return L"healer: can heal friendly units";
  }
  return L"unknown";
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
          card.energy = card.starting_energy;
        });
      }
    }
    auto& cur_player = m_session_info.players[m_session_info.current_player];
    for (int i = 0; i < cur_player.num_draws_per_turn; ++i)
    {
      auto& deck = m_session_info.current_player ? m_rules.defender_deck : m_rules.challenger_deck;
      cur_player.hand.emplace_back(generate_card(m_rules, deck, m_session_info.turn));
    }
    //cur_player.hand.emplace_back(to_card_info(specials[0], 0));
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
    auto const error = it->second(m_session_info, *card_actor, *card_target);
    if (error)
    {
      return error;
    }
    if (card_target->health <= 0)
    {
      if (card_target->has_trait(unit_traits::player))
      {
        AURA_LOG(L"Player %d has won the game!", m_session_info.current_player);
        m_session_info.game_over = true;
      }
      else
      {
        if (auto act = m_death_actions.find(card_target->uid); act != m_death_actions.end())
        {
          act->second(m_session_info, m_session_info.players[!m_session_info.current_player], 
            m_session_info.players[m_session_info.current_player]);
        }
        m_session_info.remove_lane_card(card_target->uid);
      }
    }
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
    LEGAL_ASSERT(!it->has_trait(unit_traits::item), L"Cannot deploy item cards");
    LEGAL_ASSERT(it->cost <= player.mana, L"Insufficient mana to deploy that card");
    if (!it->has_trait(unit_traits::assassin))
    {
      it->energy = 0;
    }
    player.mana -= it->cost;
    auto& card = player.lanes[lane_id - 1].emplace_back(*it);
    if (auto it = m_deploy_actions.find(card_id); it != m_deploy_actions.end())
    {
      it->second(m_session_info, player, m_session_info.players[!m_session_info.current_player]);
    }
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
