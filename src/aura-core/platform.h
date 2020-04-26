#pragma once

#include <string_view>

// platform-specific defines

namespace aura
{
// to utf8-string
std::string to_utf8_string(std::wstring_view const& view);

} // namespace aura
