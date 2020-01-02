#pragma once

#include <vector>

namespace aura
{

enum class terrain_types : int
{
  plains,
  riverlands,
  mountains,
  forests,
  total = 4
};

using terrain_t = std::vector<std::vector<terrain_types>>;

} // namespace aura
