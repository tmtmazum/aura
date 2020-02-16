#pragma once

#include <aura-core/card_preset.h>
#include <aura-core/card_preset_definitions.h>

namespace aura
{

enum class game_mode : int
{
	PvP,
	PvC
};

struct ruleset
{
	game_mode mode{game_mode::PvP};
  int num_lanes{4};
  int num_players{2};
  int max_lane_height{4}; //!< max # of units in lane
  int challenger_starting_health{10};
  int defender_starting_health{10};

  int challenger_starting_mana{1};
  int defender_starting_mana{1};

  int max_starting_mana{20};

  int mana_natural_increment{1};

  int challenger_starting_cards{2};
  int defender_starting_cards{1};

  bool stagger_turns{false};

  //! Whether or not unused mana is moved forward to the next turn
  bool accumulate_mana{true};

  //! Allow hero special powers in the game
  bool enable_hero_specials{true};

  //! Places a limit on what level cards can 
  //! be draw on a turn by multipler
  //! e.g. 1st turn -> mana 1 & 2 cards
  //!      2nd turn -> mana 1-4 cards
  //!      3rd turn -> mana 1-6 cards
  //!      4th turn -> mana 1-6 cards
  float draw_limit_multiplier{2.0};

  //! Challenger (1st player) starts off with n [0] fortifications
  int challenger_starts_with_n_forts{0};

  //! Defender (2nd player) starts off with n [0] fortifications
  //! This is to balance out the 1st player advantage
  int defender_starts_with_n_forts{0};

  int num_draws_per_turn{1};
  int num_pick_choices_multiplier{2};

  mutable deck challenger_deck = make_standard_deck();// make_standard_deck();
  mutable deck defender_deck = make_standard_deck();

  int preferred_terrain_health_bonus{1};
  int preferred_terrain_strength_bonus{1};

  bool use_draft_deck{true};
  int num_draft_choices{5}; //!< # of cards available at any time to draft from
};

} // namespace aura
