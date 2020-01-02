#pragma once

#include "session_info.h"
#include <system_error>
#include <vector>

namespace aura
{

struct tile_id
{
	int player;
	int lane_no;
	int column_no;
};

struct tile_info
{
  int cost;
  int health;
  int strength;
};

struct player_action;

enum class rules_error : int
{
  not_legal = 1 //!< player action is not legal
};

std::error_code make_error_code(rules_error e) noexcept;

class rules_engine
{
public:
  virtual bool is_game_over() const noexcept = 0;

  virtual session_info get_session_info() const = 0;

  virtual std::vector<int> get_target_list(int uid) const = 0;

  //! Commit a player action
  virtual std::error_code commit_action(player_action const&) = 0;

  virtual card_info to_card_info(card_preset const& preset, int cid) = 0;

  virtual std::error_code trigger_pick_action(int num_picks, int num_choices = 0) = 0;

  virtual std::wstring describe(unit_traits trait) const noexcept = 0;
};

class display_engine;
class ruleset;

std::error_code start_game_session(ruleset const& rules, rules_engine& engine, display_engine& e);

} // namespace aura
