#pragma once

#include <memory>

namespace aura
{

template <typename T>
class unique_list
{
public:
    struct node
    {
        T                      m_data;
        std::unique_ptr<node>  m_next;

        template <typename... Args>
        node(Args&&... args)
            : m_data{std::forward<Args>(args)...}
            , m_next{}
        {}

        T* operator->() noexcept { return &m_data; }
        T const* operator->() const noexcept { return &m_data; }
    };

    template <typename... Args>
    void emplace_back(Args&&... args)
    {
        m_tail->m_next = std::make_unique<node>(std::forward<Args>(args)...);
        m_tail = m_tail->m_next.get();
    }

    T* front() noexcept { return m_head.m_next ? &m_head.m_next->m_data : nullptr; }
    T* back() noexcept { return m_tail == &m_head ? nullptr : &m_tail.m_data; }
    T const* front() const noexcept { return m_head.m_next ? &m_head.m_next->m_data : nullptr; }
    T const* back() const noexcept { return m_tail == &m_head ? nullptr : &m_tail.m_data; }
private:
    node        m_head;
    node*       m_tail = m_head;
};

} // namespace aura
