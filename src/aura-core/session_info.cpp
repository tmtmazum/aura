#include "session_info.h"
#include "card_preset.h"
#include "ruleset.h"
#include <cstdlib>
#include <ctime>

namespace aura
{

int generate_uid()
{
  static int uid_counter{0};
  return uid_counter++;
}

card_info to_card_info(card_preset const& preset, int cid)
{
  card_info info{};
  info.uid = generate_uid();
  info.cid = cid;
  info.health = preset.health;
  info.strength = preset.strength;
  info.cost = preset.cost;
  info.name = preset.name;
  info.traits = preset.traits;
  return info;
}

card_info generate_card(ruleset const& rs, deck const& d, int turn)
{
  static auto ss = std::invoke([]
  {
    srand(time(0));
    return 0;
  });

  std::vector<card_preset> selection_pool;

  if (rs.draw_limit_multiplier > 0.1 && 
    ((rs.draw_limit_multiplier * turn) < 9))
  {
    auto const limit = (rs.draw_limit_multiplier * turn);
    for (auto const& p : presets)
    {
      if (p.cost <= limit)
      {
        selection_pool.emplace_back(p);
      }
    }
  }
  else
  {
    selection_pool = presets; 
  }

  auto const n = selection_pool.size();
  auto const i = (rand() % n);
  auto const& preset = selection_pool.at(i);

  return to_card_info(preset, i);
}

} // namespace aura
