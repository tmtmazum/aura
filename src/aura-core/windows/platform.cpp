#include "aura-core/platform.h"
#include <cstdio>

// platform-specific defines

namespace aura
{

// to utf8-string
std::string to_utf8_string(std::wstring_view const& view)
{
    char dummy[2]{};
    auto const num_chars = 
        snprintf(dummy, 2, "%.*ls", static_cast<int>(view.size()), view.data());

    std::string buffer;
    buffer.assign(num_chars, '\0');

    snprintf(buffer.data(), buffer.size(), "%.*ls", static_cast<int>(view.size()), view.data());

    return buffer;
}

} // namespace aura
