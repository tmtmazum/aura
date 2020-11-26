#pragma once

struct card_info
{
	enum class target_type
	{
		friendly_unit,
		friendly_lane,
		enemy_unit,
		enemy_lane,
		friendly_base,
		enemy_base
	};
};

class rule_engine
{

};

struct start_game_options_t
{

};

std::unique_ptr<rule_engine> start_game(start_game_options_t opts);

