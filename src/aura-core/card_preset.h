#pragma once

#include <aura-core/unit_traits.h>
#include <vector>

namespace aura
{

struct card_preset
{
  std::wstring name;
  int cost;
  int strength;
  int health;

  std::vector<unit_traits> traits;
};

std::vector<card_preset> const presets = {
  card_preset{L"Small Fortification", 0, 0, 1, {}},
  card_preset{L"Fortification", 1, 0, 2, {}},
  card_preset{L"Soldier", 1, 1, 1, {}},
  card_preset{L"Guardsman", 2, 1, 2, {}},
  card_preset{L"Doctor", 2, -1, 1, {unit_traits::healer}},
  card_preset{L"Archer", 2, 1, 1, {unit_traits::long_range}},
  card_preset{L"Apprentice Assassin", 2, 2, 1, {unit_traits::assassin}},
  card_preset{L"Med Fortification", 4, 0, 5, {}},
  card_preset{L"Adept Assassin", 3, 4, 2, {unit_traits::assassin}},
  card_preset{L"Adept Guardian", 3, 2, 5, {}},
  card_preset{L"Knight", 3, 3, 3, {}},
  card_preset{L"Enchanted Tower", 4, 1, 7, {unit_traits::long_range}},
  card_preset{L"Wrath Dragon", 5, 5, 5, {}},
  card_preset{L"The Giantess", 5, 2, 8, {}},
  card_preset{L"The Grey Hood", 5, 3, 5, {unit_traits::assassin, unit_traits::long_range}},
};

} // namespace aura
