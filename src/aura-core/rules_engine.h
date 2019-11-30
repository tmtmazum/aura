#pragma once

#include <system_error>

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

};

class rules_engine
{
public:
	tile_info tile_at(tile_id const&);
};

class display_engine;

std::error_code start_game_session(rules_engine& engine, display_engine& e);

} // namespace aura
