#include "session_info.h"
#include "card_preset.h"
#include <cstdlib>
#include <ctime>

namespace aura
{

int generate_uid()
{
  static int uid_counter{0};
  return uid_counter++;
}

card_info generate_card(deck const& d)
{
  static auto ss = std::invoke([]
  {
    srand(time(0));
    return 0;
  });

  auto const n = presets.size();
  auto const i = (rand() % n);
  auto const& preset = presets.at(i);


  card_info info{};
  info.uid = generate_uid();
  info.cid = i;
  info.health = preset.health;
  info.strength = preset.strength;
  info.cost = preset.cost;
  info.name = preset.name;
  info.traits = preset.traits;

  return info;
}

} // namespace aura
