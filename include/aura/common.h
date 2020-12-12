#pragma once

namespace aura
{

template <size_t S>
class assert_is_zero
{
public:
    static_assert(S == 0);
};

} // namespace aura

