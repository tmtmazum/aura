#pragma once

#include <system_error>
#include <future>
#include <memory>
#include "game_state.h"

namespace aura
{

//! Wraps client to server connection and all associated requests
class server_connection
{
public:
    struct impl_info;

    server_connection();
    server_connection(server_connection const& other) = delete;
    server_connection& operator=(server_connection const& other) = delete;

    // async method to start connection
    void connect() noexcept;

    bool is_connected() const noexcept;

    std::future<game_state> request_online_pvp() noexcept;

    ~server_connection();

private:
    bool connect_sync() noexcept;

private:
    bool m_is_connected = false;
    std::mutex mutable m_mutex;
    std::thread m_worker_thread;
    std::unique_ptr<impl_info> m_impl;
};

} // namespace aura
