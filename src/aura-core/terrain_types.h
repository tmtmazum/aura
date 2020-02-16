#pragma once

#include <vector>
#include <string>

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

inline std::string to_string(terrain_types const t)
{
  switch(t)
  {
  case terrain_types::forests:  return "forests";
  case terrain_types::mountains: return "mountains";
  case terrain_types::plains: return "plains";
  case terrain_types::riverlands: return "riverlands";
  }
  return "unknown";
}

} // namespace aura
