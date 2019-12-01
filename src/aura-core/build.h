#pragma once

#include <string_view>
#include <cassert>

struct scope_log_t
{
  ~scope_log_t()
  {
    wprintf(L"%lc %.*hs(..) \n", 192, static_cast<int>(func.size()), func.data());
  }

  std::string_view func;
};

#ifndef AURA_ENTER
# define AURA_ENTER() \
  wprintf(L"%lc %hs(..) \n", 218, __FUNCTION__); \
  auto const scopelog ## __LINE__ = scope_log_t{__FUNCTION__}
#endif

#ifndef AURA_LOG
#	define AURA_LOG(format, ...) wprintf(L"%lc " format L"\n", 195, __VA_ARGS__)
#endif

#ifndef AURA_PRINT
#	define AURA_PRINT(format, ...) wprintf(format, __VA_ARGS__)
#endif

#ifndef AURA_ERROR
# define AURA_ERROR(error, format, ...) wprintf(L"%lc e (%d, %hs) | %hs | " format L"\n", 195, error.value(), error.category().name(), error.message().c_str(), __VA_ARGS__)
#endif

#ifndef AURA_ASSERT
#	define AURA_ASSERT(cond) if (!(cond)) { wprintf(L"%lc%lc " #cond L"\n", 192, 254); assert((cond)); }
#endif
