#include "session.h"
#include <aura/work_queue.h>
#include <memory>

namespace aura
{

class local_pvp_session : public session
{
public:
    local_pvp_session()
        : m_notify_queue{[this](auto const& a){ handle_notify(a); }}
    {
    }

    void handle_notify(action_info const& act)
    {
        auto const it = m_registrations.find(act.player_id);
        if (it != m_registrations.end())
            it->second(act);
    }

    void register_notify(int player_id, receiver_t const& r) override
    {
        m_registrations.emplace(player_id, r);
    }

    void notify_action(action_info const& act_info) override
    {
        m_notify_queue.emplace(act_info);
    }

private:
    std::unordered_map<int, receiver_t> m_registrations;
    work_queue<action_info> m_notify_queue;
};

std::unique_ptr<session> make_local_pvp_session()
{
    return std::make_unique<local_pvp_session>();
}

}  // namespace aura