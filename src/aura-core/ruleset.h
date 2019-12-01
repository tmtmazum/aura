#pragma once

namespace aura
{

enum class game_mode : int
{
	PvP,
	PvC
};

struct deck{};

struct ruleset
{
	game_mode mode{game_mode::PvP};
  int num_lanes{4};
  int num_players{2};
  int challenger_starting_health{5};
  int defender_starting_health{5};

  int challenger_starting_mana{1};
  int defender_starting_mana{1};

  int mana_natural_increment{1};

  int challenger_starting_cards{4};
  int defender_starting_cards{4};

  deck challenger_deck{};
  deck defender_deck{};
};

} // namespace aura
