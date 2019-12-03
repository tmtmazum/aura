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

  //! Places a limit on what level cards can 
  //! be draw on a turn by multipler
  //! e.g. 1st turn -> mana 1 & 2 cards
  //!      2nd turn -> mana 1-4 cards
  //!      3rd turn -> mana 1-6 cards
  //!      4th turn -> mana 1-6 cards
  float draw_limit_multiplier{2.0};

  //! Challenger (1st player) starts off with n [0] fortifications
  int challenger_starts_with_n_forts{1};

  //! Defender (2nd player) starts off with n [0] fortifications
  //! This is to balance out the 1st player advantage
  int defender_starts_with_n_forts{3};

  deck challenger_deck{};
  deck defender_deck{};
};

} // namespace aura
