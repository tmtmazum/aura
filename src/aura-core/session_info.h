#pragma once
#include <vector>
#include <string>
#include <algorithm>

namespace aura
{

enum class unit_traits : int;

struct card_info
{
  int uid; //!< unique in-game identifer given by rules engine
  int cid; //!< unique identifier for card preset

  int health;
  int strength;
  int cost;
  bool resting{false}; //!< resting units cannot be selected to take an action
  bool is_visible; //!< this unit can be targetted for an action
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
};

class deck;
struct ruleset;
card_info generate_card(ruleset const& r, deck const& d, int turn = 1);
int generate_uid();

struct card_preset;
card_info to_card_info(card_preset const& p, int cid);

struct player_info
{
  int uid = generate_uid();
  int health;
  int starting_health;
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
};

struct session_info
{
  int turn{1};
  int current_player{0};
  std::vector<player_info> players;

  template <typename Fn>
  void for_each_lane_card(Fn const& fn)
  {
    for (auto&& p : players)
    {
      p.for_each_lane_card(fn);
    }
  }
};

} // namespace aura
