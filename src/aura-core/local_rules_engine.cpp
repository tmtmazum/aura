#include "local_rules_engine.h"
#include "aura-core/ruleset.h"
#include "aura-core/player_action.h"
#include "aura-core/build.h"
#include "aura-core/unit_traits.h"
#include "aura-core/card_preset.h"
#include "aura-core/terrain_types.h"
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
      v.emplace_back(to_card_info(presets[0], 0));
      player.lanes.emplace_back(v);
    }
    
    if (!m_rules.use_draft_deck)
    {
      for (int i = 0; i < m_rules.challenger_starting_cards; ++i)
      {
        player.hand.emplace_back(generate_card(rs, rs.challenger_deck));  
      }

      for (auto i = 0; i < rs.challenger_starts_with_n_forts; ++i)
      {
        player.hand.emplace_back(to_card_info(presets[0], 0));
      }
    }
    //player.hand.emplace_back(to_card_info(specials[0], specials[0].cid));
    add_specials(player);
    m_session_info.players.emplace_back(player);
  }

  {
    player_info player{};
    player.starting_health = m_rules.defender_starting_health;
    player.health = m_rules.defender_starting_health;
    player.starting_mana = m_rules.defender_starting_mana;
    player.mana = player.starting_mana;
    for (int i = 0; i < rs.num_lanes; ++i)
    {
      std::vector<aura::card_info> v;
      v.emplace_back(to_card_info(presets[0], 0));
      player.lanes.emplace_back(v);
    }
    
    if (!m_rules.use_draft_deck)
    {
      for (int i = 0; i < m_rules.defender_starting_cards; ++i)
      {
        player.hand.emplace_back(generate_card(rs, rs.defender_deck));
      }
      for (auto i = 0; i < rs.defender_starts_with_n_forts; ++i)
      {
        player.hand.emplace_back(to_card_info(presets[0], 0));
      }
    }

    //player.hand.emplace_back(to_card_info(specials[0], specials[0].cid));
    add_specials(player);
    m_session_info.players.emplace_back(player);
  }

  m_session_info.terrain = generate_terrain();
  if (m_rules.use_draft_deck)
  {
    ready_draft_picks();
    trigger_draft_pick();
  }
  else
  {
    trigger_pick_action(m_session_info.players[0].num_draws_per_turn, m_session_info.players[0].num_draws_per_turn);
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
  info.starting_strength = preset.strength;
  info.strength = preset.strength;
  info.cost = preset.cost;
  info.name = preset.name;
  info.traits = preset.traits;
  info.preferred_terrain = preset.preferred_terrain;
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

terrain_t local_rules_engine::generate_terrain()
{
  static auto ss = std::invoke([]
  {
    srand(time(0));
    return 0;
  });

  terrain_t t;
  for (int i = 0; i < m_rules.num_lanes; ++i)
  {
    std::vector<terrain_types> v;
    for (int j = 0; j < m_rules.max_lane_height * 2; ++j)
    {
      auto const n = rand() % static_cast<int>(terrain_types::total);
      v.emplace_back(static_cast<terrain_types>(n));
    }
    t.emplace_back(v);
  }
  return t;
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
  case unit_traits::aerial:  return L"aerial: traverses by air";
  case unit_traits::structure: return L"structure: good for providing cover and bonuses";
  case unit_traits::item: return L"item: can be applied to card on board";
  case unit_traits::twice: return L"can act twice per turn";
  case unit_traits::thrice: return L"can act three times per turn";
  case unit_traits::assassin: return L"assassin: can attack same turn as deployment";
  case unit_traits::long_range: return L"can attack from range (skipping lane obstructions)";
  case unit_traits::healer: return L"heals friendly units";
  case unit_traits::infantry: [[fallthrough]];
  case unit_traits::player: [[fallthrough]];
  default:
    return L"";
  }
  return L"";
}

std::error_code local_rules_engine::ready_draft_picks()
{
  if (m_starting_drafts)
  {
    auto const num_to_draft = m_rules.challenger_starting_cards + m_rules.defender_starting_cards;
    
    for (int i = 0; i < num_to_draft; ++i)
    {
      m_draft_choices.emplace_back(generate_card(m_rules, m_rules.challenger_deck));
    }
    return {};
  }
  return {};
}

std::error_code local_rules_engine::trigger_draft_pick()
{
  auto& cur_player = m_session_info.players[m_session_info.current_player];
  if (m_starting_drafts)
  {
    m_session_info.picks.clear();
    m_session_info.picks = m_draft_choices;
    cur_player.picks_available = 1;
  }

  return {};
}

std::error_code local_rules_engine::trigger_pick_action(int num_picks, int num_choices)
{
  auto& cur_player = m_session_info.players[m_session_info.current_player];
  auto& deck = m_session_info.current_player ? m_rules.defender_deck : m_rules.challenger_deck;

  auto const choices = num_choices ? num_choices : (m_rules.num_pick_choices_multiplier * num_picks);

  if (choices == num_picks)
  {
    cur_player.hand.emplace_back(generate_card(m_rules, deck));
    return std::error_code{};
  }

  for (int i = 0; i < choices; ++i)
  {
    m_session_info.picks.emplace_back(generate_card(m_rules, deck));
  }
  cur_player.picks_available = num_picks;
  return std::error_code{};
}

void local_rules_engine::add_specials(player_info& player)
{
  auto const has_special = [&](int ind)
  {
    return std::any_of(begin(player.hand), end(player.hand), [&](auto const& card)
    {
      return card.cid == specials[ind].cid;
    });
  };

  if (m_rules.enable_hero_specials && !has_special(0))
  {
    player.hand.emplace_back(to_card_info(specials[0], specials[0].cid));
  }
  if (m_rules.enable_hero_specials && !has_special(1))
  {
    player.hand.emplace_back(to_card_info(specials[1], specials[1].cid));
  }
}

void local_rules_engine::apply_terrain_modifiers(int cur_player, int lane_num, int tile_num, card_info& card) const
{
  auto const h = cur_player ? (m_rules.max_lane_height - 1 - tile_num) : tile_num;
  auto const& t = m_session_info.terrain[lane_num][h];

  auto const is_preferred =
      std::find(begin(card.preferred_terrain), end(card.preferred_terrain), t) != end(card.preferred_terrain);

  card.on_preferred_terrain = is_preferred;
#if 0
  if (is_preferred)
  {
    //AURA_LOG(L"apply_terrain_modifiers(%d, %d, %d, %ls) P { health.before:%d, starting_health:%d, strength.before:%d, starting.strength:%d }",
    //  cur_player, lane_num, tile_num, card.name.c_str(), card.health, card.starting_health, card.strength, card.starting_strength);
    //card.health = std::min(card.health, card.starting_health) + m_rules.preferred_terrain_health_bonus;
    //card.strength = std::min(card.strength, card.starting_strength) + m_rules.preferred_terrain_strength_bonus;
    //AURA_LOG(L"apply_terrain_modifiers(%d, %d, %d, %ls) P { health.after:%d, strength.after:%d }",
    //  cur_player, lane_num, tile_num, card.name.c_str(), card.health, card.strength);
  }
  else
  {
    //AURA_LOG(L"apply_terrain_modifiers(%d, %d, %d, %ls) NP { health.before:%d, starting_health:%d, strength.before:%d, starting.strength:%d }",
    //  cur_player, lane_num, tile_num, card.name.c_str(), card.health, card.starting_health, card.strength, card.starting_strength);
    //card.health = std::min(card.health, card.starting_health);
    //card.strength = std::min(card.strength, card.starting_strength);
    //AURA_LOG(L"apply_terrain_modifiers(%d, %d, %d, %ls) NP { health.after:%d, strength.after:%d }",
    //  cur_player, lane_num, tile_num, card.name.c_str(), card.health, card.strength);
  }
#endif
}

session_info const& local_rules_engine::get_session_info() const
{ 
  return m_session_info;
}

void local_rules_engine::apply_all_terrain_modifiers(session_info& sesh) const
{
  for (int p = 0; p < 2; ++p)
  {
    for (int i = 0; i < m_rules.num_lanes; ++i)
    {
      for (int j = 0; j < sesh.players[p].lanes[i].size(); ++j)
      {
        auto& card = sesh.players[p].lanes[i][j];
        apply_terrain_modifiers(p, i, j, card);
      }
    }
  }
}


//! Commit a player action
std::error_code local_rules_engine::commit_action(player_action const& action) 
{
  switch (action.type)
  {
  case action_type::end_turn:
  {
    LEGAL_ASSERT(!m_session_info.game_over, L"Cannot end turn - game is already over.");
    //if (m_session_info.current_player)
    if (end_of_turn)
    {
      m_session_info.turn++;
      end_of_turn = false;
      for (auto& player : m_session_info.players)
      {
        player.starting_mana = std::min(player.starting_mana + 1, m_rules.max_starting_mana);
        player.mana = (m_rules.accumulate_mana * player.mana) + player.starting_mana;
        //player.num_draws_per_turn = std::min(1 + (m_session_info.turn / 5), 4);
        //player.num_drawn_this_turn = 0;
        player.for_each_lane_card([](auto& card)
        {
          card.energy = card.starting_energy;
        });
      }
    }
    else
    {
      end_of_turn = true;
    }

    if (m_rules.stagger_turns)
    {
      auto const is_even_turn = !(m_session_info.turn % 2);
      m_session_info.current_player = end_of_turn ? !is_even_turn : is_even_turn;
    }
    else
    {
      m_session_info.current_player = !m_session_info.current_player;
    }
    auto& cur_player = m_session_info.players[m_session_info.current_player];
    trigger_pick_action(cur_player.num_draws_per_turn, cur_player.num_draws_per_turn);
    add_specials(cur_player);
    return {};
  }

  case action_type::forfeit:
  {
    LEGAL_ASSERT(!m_session_info.game_over, L"Cannot forfeit - game is already over.");
    m_session_info.game_over = true;
    return {};
  }

  case action_type::pick:
  {
    auto const card_picked_uid = action.target1;

    if (m_starting_drafts)
    {
      auto const it = std::find_if(begin(m_draft_choices), end(m_draft_choices), 
        [&](auto const& card)
      {
        return card.uid == card_picked_uid;  
      });

      LEGAL_ASSERT(it != m_draft_choices.end(), L"Couldn't find picked card in drafts");

      auto& player = m_session_info.players[m_session_info.current_player];
      player.hand.emplace_back(*it);

      m_draft_choices.erase(it);

      --player.picks_available;

      auto const p1_done = [&]()
      {
        return m_session_info.players[0].hand.size() >= m_rules.challenger_starting_cards + 2;
      }();
      auto const p2_done = [&]()
      {
        return m_session_info.players[1].hand.size() >= m_rules.defender_starting_cards + 2;
      }();

      if (p1_done && p2_done)
      {
        m_starting_drafts = false;
        player.picks_available = 0;
        m_session_info.picks.clear();
        m_session_info.current_player = !m_session_info.current_player;
      }
      else
      {
        trigger_draft_pick();
        m_session_info.current_player = !m_session_info.current_player;
      }

      return {};
    }

    auto const it = std::find_if(begin(m_session_info.picks), end(m_session_info.picks), 
      [&](auto const& card)
    {
      return card.uid == card_picked_uid;  
    });
    LEGAL_ASSERT(it != m_session_info.picks.end(), L"Couldn't find picked card");

    auto& player = m_session_info.players[m_session_info.current_player];
    player.hand.emplace_back(*it);
    if (!--player.picks_available)
    //if (++player.num_drawn_this_turn == player.num_draws_per_turn)
    {
      m_session_info.picks.clear();
    }
    else
    {
      m_session_info.picks.erase(it);
    }
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

    AURA_LOG(L"BEFORE %ls %cP [%d, %d] (primary) -> %ls %cP [%d, %d]", 
      card_actor->name.c_str(), card_actor->on_preferred_terrain ? L' ' : L'N',
      card_actor->strength, card_actor->health,
      card_target->name.c_str(), card_target->on_preferred_terrain ? L' ' : L'N',
      card_target->strength, card_target->health);

    auto it = m_primary_actions.find(card_actor_uid);
    LEGAL_ASSERT(it != m_primary_actions.end(), L"No actions found for this unit!!");
    auto const error = it->second(*this, m_session_info, *card_actor, *card_target);
    if (error)
    {
      return error;
    }
    
    if (card_target->has_trait(unit_traits::player) && card_target->health <= 0)
    {
      AURA_LOG(L"Player %d has won the game!", m_session_info.current_player);
      m_session_info.game_over = true;
    }
    else
    {
      m_session_info.remove_dead_lane_card([&](auto const& card)
      {
        if (auto act = m_death_actions.find(card.uid); act != m_death_actions.end())
        {
          act->second(*this, m_session_info, m_session_info.players[!m_session_info.current_player], 
            m_session_info.players[m_session_info.current_player]);
        }
        //m_session_info.players[m_session_info.current_player].mana++;
      });
      apply_all_terrain_modifiers(m_session_info);
    }

    AURA_LOG(L"AFTER %ls %cP [%d, %d] (primary) -> %ls %cP [%d, %d]", 
      card_actor->name.c_str(), card_actor->on_preferred_terrain ? L' ' : L'N',
      card_actor->strength, card_actor->health,
      card_target->name.c_str(), card_target->on_preferred_terrain ? L' ' : L'N',
      card_target->strength, card_target->health);
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
    LEGAL_ASSERT(!it->has_trait(unit_traits::item), L"Cannot deploy item cards");
    LEGAL_ASSERT(it->cost <= player.mana, L"Insufficient mana to deploy that card");

    AURA_LOG(L"[lre] deploy(%ls) to lane %d", it->name.c_str(), lane_id);

    if (!it->has_trait(unit_traits::assassin))
    {
      it->energy = 0;
    }
    player.mana -= it->cost;
    auto const [x, y] = std::make_pair(lane_id - 1, player.lanes[lane_id - 1].size());
    apply_terrain_modifiers(m_session_info.current_player, x, y, *it);
    auto& card = player.lanes[lane_id - 1].emplace_back(*it);
    if (auto it = m_deploy_actions.find(card_id); it != m_deploy_actions.end())
    {
      it->second(*this, m_session_info, player, m_session_info.players[!m_session_info.current_player]);
    }
    AURA_LOG(L"before remove from hand");
    player.hand.erase(it);
    AURA_LOG(L"after remove from hand");
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
