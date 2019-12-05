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

void session_info::remove_lane_card(int uid)
{
  using lane_t = std::vector<card_info>;
  using lane_it_t = lane_t::iterator;

  lane_t* lane_ptr{};
  lane_it_t card_it{};
  for (auto& player : players)
  {
    for (auto& lane : player.lanes)
    {
      for (auto it = lane.begin(); it != lane.end(); ++it)
      {
        if (it->uid == uid)
        {
          lane_ptr = &lane;
          card_it = it;
        }
      }
    }
  }
  AURA_ASSERT(lane_ptr);
  lane_ptr->erase(card_it);
}

void session_info::remove_hand_card(int uid)
{
  using hand_t = std::vector<card_info>;
  using hand_it_t = hand_t::iterator;

  hand_t* hand{};
  hand_it_t card_it{};
  for (auto& player : players)
  {
    for (auto it = player.hand.begin(); it != player.hand.end(); ++it)
    {
      if (it->uid == uid)
      {
        hand = &player.hand;
        card_it = it;
      }
    }
  }
  AURA_ASSERT(hand);
  hand->erase(card_it);
}

bool player_info::has_free_lane() const noexcept
{
  for (auto& lane : lanes)
  {
    if (lane.empty())
    {
      return true;
    }
  }
  return false;
}

bool session_info::is_front_of_lane(int uid) const noexcept
{
  for (auto& player : players)
  {
    for (auto& lane : player.lanes)
    {
      auto const n = lane.size();
      if (n == 1 && lane[0].uid == uid)
      {
        return true;
      }
      else if (n > 1 && lane[n-1].uid == uid)
      {
        return true;
      }
    }
  }
  return false;
}

} // namespace aura
