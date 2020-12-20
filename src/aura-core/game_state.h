#pragma once

#include <memory>
#include <array>
#include <aura/common.h>
#include "deck.h"

namespace aura
{

enum class terrain_t {};

inline constexpr auto num_lanes = 4;
inline constexpr auto lane_max_height = 4;

class lane
{
private:
    std::array<std::unique_ptr<card>, lane_max_height> m_cards;
    std::array<terrain_t, lane_max_height> m_terrain;
};

class player
{
public:
private:
    std::array<std::unique_ptr<card>, 4> m_hand;
    std::array<lane, num_lanes>          m_lanes;
    std::unique_ptr<deck>                m_deck;
};

class ruleset;

class game_state
{
public:
    ~game_state();
private:
    std::array<player, 2>       m_players;
    //std::unique_ptr<ruleset>    m_rules;
};

std::unique_ptr<game_state> start_local_pvp(ruleset const& rules);

};  // namespace aura