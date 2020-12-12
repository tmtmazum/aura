#pragma once

#include "./common.h"
#include <type_traits>
#include <vector>

namespace aura
{

template <typename T, size_t BufferSize = 16>
class small_vector
{
    constexpr static size_t num_items_in_buffer = BufferSize / sizeof(T);
    constexpr static size_t max_items_in_buffer = 256;

    static_assert(num_items_in_buffer < max_items_in_buffer);

    using storage_t = std::aligned_storage_t<sizeof(T), alignof(T)>;

    class iterator
    {
    public:
        explicit iterator(T* addr)
            : m_addr{addr}
        {}

        T* operator*() const noexcept { return m_addr; }

        T* operator->() const noexcept { return m_addr;}

        bool operator==(iterator const& other)
        {
            return m_addr == other.m_addr;
        }

//        bool operator!=(iterator const& other) { return !operator==(other);}

        void operator++()
        {
            m_addr++;
        }
    private:
        T* m_addr;
    };

public:
    small_vector(std::initializer_list<T> const& il)
    {
        for (auto const& i : il)
        {
            emplace_back(i);
        }
    }

    small_vector(small_vector<T,BufferSize> const& other) = default;
    small_vector& operator=(small_vector<T,BufferSize> const&) = default;
    small_vector(small_vector<T,BufferSize>&& other) noexcept = default;
    small_vector& operator=(small_vector<T,BufferSize>&&) noexcept = default;

    iterator begin()
    {
        if (!m_local_size) return iterator{nullptr};
        return iterator{to_type_ptr(m_storage[0])};
    }

    iterator end()
    {
        if (!m_local_size) return iterator{nullptr};
        return iterator{to_type_ptr(m_storage[m_local_size-1]) + 1};
    }

    T& to_type(storage_t& s)
    {
        return reinterpret_cast<T&>(s);
    }

    T* to_type_ptr(storage_t& s)
    {
        return reinterpret_cast<T*>(&s);
    }

    T const& to_type(storage_t const& s) const
    {
        return reinterpret_cast<T const&>(s);
    }

    T const& operator[](size_t pos) const
    {
        if (pos < num_items_in_buffer)
        {
            return to_type(m_storage);
        }
        return m_dynamic.at(pos);
    }

    T& operator[](size_t pos) noexcept
    {
        if (pos < num_items_in_buffer)
        {
            return to_type(m_storage);
        }
        return m_dynamic.at(pos);
    }

    template <typename... Args>
    void emplace_back(Args&&... args)
    {
        new (to_type_ptr(m_storage[m_local_size++])) T(std::forward<Args>(args)...);
    }

    bool is_local() const noexcept { return m_dynamic.empty(); }

    bool empty() const noexcept { return m_local_size==0; }

    ~small_vector()
    {
        for (size_t i = 0; i < m_local_size; ++i)
        {
            to_type_ptr(m_storage[i])->~T();
        }
    }

private:
    storage_t       m_storage[num_items_in_buffer];
    std::vector<T>  m_dynamic;
    assert_is_zero<sizeof(m_dynamic)> ma;
    uint8_t         m_local_size{};
};

template <typename T, size_t EstCount>
using svector = small_vector<T, sizeof(T)*EstCount>;

template <typename T>
using svector4 = small_vector<T, 4>;

template <typename T>
using svector8 = small_vector<T, 8>;

template <typename T>
using svector16 = small_vector<T, 16>;

} // namespace aura

