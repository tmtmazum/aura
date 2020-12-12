#pragma once
#include <thread>
#include <unordered_map>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>
#include <cassert>

namespace aura
{

template <typename T>
class work_queue
{
public:

    enum class exit_flag
    {
        none,
        abort,
        finish
    };

    using fn_t = std::function<void(T& )>;

    work_queue() = default;

    work_queue(fn_t fn)
        : m_fn{std::move(fn)}
    {
        assert(m_fn);
        m_worker_thread = std::thread
        {
            [&]() { notify_thread_main(); }
        };
    }

    void notify_thread_main()
    {
        while (1)
        {
            std::unique_lock lock{m_queue_mutex};

            if (m_exit != exit_flag::finish)
                m_cv.wait(lock, [this](){ return !m_queue.empty() || (m_exit != exit_flag::none); });

            if (m_exit == exit_flag::abort)
                return;

            if (m_exit == exit_flag::finish && m_queue.empty())
            {
                return;
            }

            auto f = m_queue.front();
            m_queue.pop();

            lock.unlock();

            m_fn(f);
        }
    }

    template <typename... Args>
    void emplace(Args&&... args)
    {
        {
            std::unique_lock lock{m_queue_mutex};
            m_queue.emplace(T{std::forward<Args>(args)...});
        }
        m_cv.notify_one();
    }

    void set_exit(exit_flag ef) noexcept
    {
        m_exit = ef;
        m_cv.notify_one();
    }

    ~work_queue()
    {
        set_exit(exit_flag::abort);
        m_cv.notify_one();
        if (m_worker_thread.joinable())
        {
            m_worker_thread.join();
        }
    }

private:
    std::thread             m_worker_thread;
    std::queue<T>           m_queue;
    mutable std::mutex      m_queue_mutex;
    std::condition_variable m_cv;
    fn_t                    m_fn;
    exit_flag               m_exit = exit_flag::none;
};

}
