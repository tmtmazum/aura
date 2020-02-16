#pragma once

#include <aura-core/unit_traits.h>
#include <aura-core/rules_engine.h>
#include <aura-core/build.h>
#include <aura-core/terrain_types.h>
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
  std::vector<terrain_types> preferred_terrain;
  card_action_type action_type;
  card_action_targets action_targets;

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
  std::unordered_multimap<int, card_preset> fixed_picks;

  void reset()
  {
    remaining_cards = all_cards;
  }

  void add_fixed_pick(int turn, card_preset pr)
  {
    fixed_picks.emplace(turn, std::move(pr));
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
    auto fixed_it = fixed_picks.find(turn);
    if (fixed_it != fixed_picks.end())
    {
      card_preset answer = std::move(fixed_it->second);
      fixed_picks.erase(fixed_it);
      return answer;
    }

    static auto ss = std::invoke([]
    {
      srand(time(0));
      return 0;
    });

    if (remaining_cards.empty())
    {
      reset();
    }

    auto& pool = remaining_cards;
    //auto const& pool = (max_level < 9 ? make_restricted_pool(turn, max_level)
    //  : remaining_cards);

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

} // namespace aura
