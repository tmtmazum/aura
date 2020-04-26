#include "aura-core/platform.h"
#include <windows.h>
#include <cstdio>

// platform-specific defines

namespace aura
{

namespace
{

// to utf8-string
std::string to_utf8_string(std::wstring_view const& view)
{
    auto const num_chars = _vscprintf("%.*ls", view.data(), static_cast<int>(view.size()));

    std::string buffer;
    buffer.assign(num_chars, '\0');

    snprintf(buffer.data(), buffer.size(), view.data(), static_cast<int>(view.size()));

    return buffer;
}

} // namespace aura
