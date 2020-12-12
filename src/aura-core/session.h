#pragma once

namespace aura
{

enum class action_type : uint16_t
{
    unknown,
    end_turn,
    forfeit,
    primary,
    deploy,
    pick,
    game_over // target1: winner
};

struct action_info
{
    action_type act;
    int player_id;
    int target1;
    int target2;
};

using receiver_t = std::function<void(action_info const& )>;

class session
{
public:
    void register_notify(int player_id, receiver_t const& r);

    void notify_action(action_info const& );

};

} // namespace aura
