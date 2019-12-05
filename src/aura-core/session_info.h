#pragma once

#include <aura-core/unit_traits.h>

#include <vector>
#include <string>
#include <algorithm>
#include <system_error>

namespace aura
{

enum class unit_traits : int;

struct card_info
{
  int uid; //!< unique in-game identifer given by rules engine
  int cid; //!< unique identifier for card preset

  int health;
  int starting_health;
  int strength;
  int cost;
  int energy{1};
  int starting_energy{1};//!< determines how many times this unit can act / turn before needing to rest

  bool is_visible{true}; //!< this unit can be targetted for an action
  std::wstring name;
  std::wstring description;
  std::vector<unit_traits> traits;

  bool has_trait(unit_traits criteria) const noexcept
  {
    return std::any_of(begin(traits), end(traits), [&](auto const& t)
    {
      return t == criteria;
    });
  }

  bool is_resting() const noexcept
  {
    return !energy && !has_trait(unit_traits::structure);
  }
};

class deck;
struct ruleset;
int generate_uid();

struct card_preset;

struct player_info : public card_info
{
  int num_draws_per_turn{1};

  player_info()
    : card_info{}
  {
    uid = generate_uid();
    traits.emplace_back(unit_traits::player);
  }
  int mana;
  int starting_mana;

  std::vector<card_info> hand;
  std::vector<std::vector<card_info>> lanes;

  template <typename Fn>
  void for_each_lane_card(Fn const& fn)
  {
    for (auto&& lane : lanes)
    {
      for (auto&& card : lane)
      {
        fn (card);
      }
    }
  }

  bool has_free_lane() const noexcept;
};

struct session_info
{
  int turn{1};
  int current_player{0};
  bool game_over{false};
  std::vector<player_info> players;

  template <typename Fn>
  void for_each_lane_card(Fn const& fn)
  {
    for (auto&& p : players)
    {
      p.for_each_lane_card(fn);
    }
  }

  void remove_lane_card(int uid);
  void remove_hand_card(int uid);

  bool is_front_of_lane(int uid) const noexcept;
};

using card_action_t = std::error_code(*)(session_info& session, card_info& actor, card_info& target);

} // namespace aura
