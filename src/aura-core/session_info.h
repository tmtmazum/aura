#pragma once

#include <aura-core/unit_traits.h>
#include <aura-core/terrain_types.h>

#include <vector>
#include <string>
#include <algorithm>
#include <system_error>
#include <functional>

namespace aura
{

enum class unit_traits : int;

//! affects what symbol is shown when highlighting 
//! other (target) cards while this card is selected.
enum class card_action_type
{
  melee_attack,
  ranged_attack,
  healer,
  spell,
  none
};

//! affects what other cards can be targetted while this
//! one is selected
enum class card_action_targets
{
  none,
  friendly = 0x1,
  enemy = 0x2,
  both = 0x1 | 0x2,
  friendly_hero // player
};

struct card_info
{
  int uid; //!< unique in-game identifer given by rules engine
  int cid; //!< unique identifier for card preset

  int health;
  int starting_health;
  int strength;
  int starting_strength;
  int cost;
  int energy{1};
  int starting_energy{1};//!< determines how many times this unit can act / turn before needing to rest
  card_action_type action_type;
  card_action_targets action_targets;

  bool is_visible{true}; //!< this unit can be targetted for an action
  bool on_preferred_terrain{false}; //!< whether this unit is standing on preferred terrain or not

  std::wstring name;
  std::wstring description;
  std::vector<unit_traits> traits;
  std::vector<terrain_types> preferred_terrain;

  //! Returns effective health (including terrain bonuses)
  auto effective_health() const noexcept
  {
    return on_preferred_terrain ? health + 1 : health;
  }

  auto health_as_string() const noexcept
  {
    auto const s = std::to_string(health);
    return on_preferred_terrain ? s + "+1" : s;
  }

  //! Returns effective strength (including terrain bonuses)
  auto effective_strength() const noexcept
  {
    if (strength < 0)
    { // healer
      return strength;
    }
    return on_preferred_terrain ? strength + 1 : strength;
  }

  auto strength_as_string() const noexcept
  {
    auto const s = std::to_string(std::abs(strength));
    return on_preferred_terrain ? s + "+1" : s;
  }

  bool has_trait(unit_traits criteria) const noexcept
  {
    return std::any_of(begin(traits), end(traits), [&](auto const& t)
    {
      return t == criteria;
    });
  }

  bool is_resting() const noexcept
  {
    return !energy && strength;
  }

  bool can_act() const noexcept
  {
    return !is_resting() && action_type != card_action_type::none;
  }

  bool can_target_enemy() const noexcept
  {
    return action_targets == card_action_targets::both ||
           action_targets == card_action_targets::enemy;
  }

  bool can_target_friendly() const noexcept
  {
    return action_targets == card_action_targets::both ||
           action_targets == card_action_targets::friendly;
  }

  bool can_be_deployed() const noexcept { return !has_trait(unit_traits::item); }
  bool prefers_terrain(terrain_types t)
  {
    return std::any_of(begin(preferred_terrain), end(preferred_terrain),
      [&](auto const tt) { return tt == t; });
  }

  virtual ~card_info() = default;
};

class deck;
struct ruleset;
int generate_uid();

struct card_preset;

struct player_info : public card_info
{
  int num_draws_per_turn{1};
  //int num_drawn_this_turn{0};
  int picks_available{};
  int mana;
  int starting_mana;
  std::string name = "Player";

  std::vector<card_info> hand;
  std::vector<std::vector<card_info>> lanes;

  player_info()
    : card_info{}
  {
    uid = generate_uid();
    traits.emplace_back(unit_traits::player);
  }

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
  std::vector<card_info> picks;

  using terrain_t = std::vector<std::vector<terrain_types>>;
  terrain_t terrain;

  template <typename Fn>
  void for_each_lane_card(Fn const& fn)
  {
    for (auto&& p : players)
    {
      p.for_each_lane_card(fn);
    }
  }

  void remove_lane_card(int uid);
  void remove_dead_lane_card(std::function<void(card_info const&)> action);
  void remove_hand_card(int uid);

  bool is_front_of_lane(int uid) const noexcept;
};

struct rules_engine;
using card_action_t = std::error_code(*)(rules_engine& re, session_info& session, card_info& actor, card_info& target);

} // namespace aura
