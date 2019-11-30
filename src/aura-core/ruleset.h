#pragma once

namespace aura
{

enum class game_mode : int
{
	PvP,
	PvC
};

struct ruleset
{
	game_mode mode;
};

} // namespace aura
