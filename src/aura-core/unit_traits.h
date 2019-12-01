#pragma once

namespace aura
{

enum class unit_traits : int
{
	assassin,		//!< can attack upon placement without rest
	long_range,	//!< can attack any unit in any lane regardless of what's blocking it
  healer
};

} // namespace aura

