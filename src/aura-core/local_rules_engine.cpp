#include "local_rules_engine.h"
#include "aura-core/ruleset.h"
#include "aura-core/player_action.h"
#include "aura-core/build.h"
#include "aura-core/unit_traits.h"
#include <algorithm>
#include <optional>
#include <functional>

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
      player.hand.emplace_back(generate_card(rs.challenger_deck));  
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
      player.hand.emplace_back(generate_card(rs.defender_deck));
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

//! Commit a player action
std::error_code local_rules_engine::commit_action(player_action const& action) 
{
  switch (action.type)
  {
  case action_type::end_turn:
  {
    LEGAL_ASSERT(!m_game_over, L"Cannot end turn - game is already over.");
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
        player.hand.emplace_back(generate_card(m_rules.challenger_deck));
      }
    }
    m_session_info.current_player = !m_session_info.current_player;
    return {};
  }

  case action_type::forfeit:
  {
    LEGAL_ASSERT(!m_game_over, L"Cannot forfeit - game is already over.");
    m_game_over = true;
    return {};
  }

  case action_type::primary_action:
  {
    auto& player = m_session_info.players[m_session_info.current_player];
    auto const card_actor_uid = action.target1;
    auto const card_target_uid = action.target2;
    auto* card_actor = std::invoke([&]() -> card_info*
    {
      for (auto& lane : player.lanes)
      {
        for (auto& card : lane)
        {
          if (card.uid == card_actor_uid)
          {
            return &card;
          }
        }
      }
      return nullptr;
    });
    LEGAL_ASSERT(card_actor, L"No actor card with that identifier found in current player's hand");
    LEGAL_ASSERT(!card_actor->resting, L"Player cannot take action when resting");
    LEGAL_ASSERT(card_actor->strength, L"This unit cannot attack");

    auto& other_player = m_session_info.players[!m_session_info.current_player];
    if (other_player.uid == card_target_uid)
    {
      auto const blank_lane = std::invoke([&]
      {
        for (auto const& lane : other_player.lanes)
        {
          if (lane.empty())
          {
            return true;
          }
        }
        return false;
      });
      LEGAL_ASSERT(blank_lane, L"There are no free lanes available to target the enemy champion");

      // targetting other player
      auto const reduced_health = other_player.health - card_actor->strength;
      if (reduced_health > 0)
      {
        other_player.health = reduced_health;
      }
      else
      {
        AURA_LOG(L"Player %d has won the game!", m_session_info.current_player);
        m_game_over = true;
      }
      card_actor->resting = true;
      return {};
    }

    auto [maybe_target, rm_target, front_of_lane] = std::invoke([&]
    {
      std::tuple<std::optional<std::vector<card_info>::iterator>,
        std::function<void(std::vector<card_info>::iterator)>, bool> target{};

      for (auto& p : m_session_info.players)
      {
        for (auto& lane : p.lanes)
        {
          for (auto it = lane.begin(); it != lane.end(); ++it)
          {
            if (it->uid == card_target_uid)
            {
              std::get<0>(target) = it;
              std::get<1>(target) = [&lane](auto it) { lane.erase(it); };
              std::get<2>(target) = std::invoke([&]() {
                auto copy_it = it;
                copy_it++;
                return copy_it == lane.end();
              });
              return target;
            }
          }
        }
      }
      return target;
    });
    LEGAL_ASSERT(maybe_target, L"No target card with that identifier found on the board");
    LEGAL_ASSERT(front_of_lane || card_actor->has_trait(unit_traits::long_range),
      L"This unit type can only target enemies at the front of their lane");

    auto const reduced_health = (*maybe_target)->health - card_actor->strength;
    if (reduced_health > 0)
    {
      (*maybe_target)->health = reduced_health;
    }
    else
    {
      rm_target(*maybe_target);
    }
    card_actor->resting = true;
    return {};
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
