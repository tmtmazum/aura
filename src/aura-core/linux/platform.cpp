#include "aura-core/platform.h"
#include <cstdio>
#include <string>

// platform-specific defines

namespace aura
{

// to utf8-string
std::string to_utf8_string(std::wstring_view const& view)
{
    char dummy[1];
    auto const num_chars = 
        snprintf(dummy, 0, "%.*ls", view.data(), static_cast<int>(view.size()));

    std::string buffer;
    buffer.assign(num_chars, '\0');

    snprintf(buffer.data(), buffer.size(), "%.*ls", view.data(), static_cast<int>(view.size()));

    return buffer;
}

} // namespace aura
