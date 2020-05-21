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
# if AURA_DEBUG
#   define AURA_ENTER() \
      wprintf(L"%lc %hs(..) \n", 218, __FUNCTION__); \
      auto const scopelog ## __LINE__ = scope_log_t{__FUNCTION__}
# else
#   define AURA_ENDER()
# endif
#endif

#ifndef AURA_LOG
# if AURA_DEBUG
#	  define AURA_LOG(format, ...) wprintf(L"%lc " format L"\n", 195, ##__VA_ARGS__)
# else
#	  define AURA_LOG(format, ...)
# endif
#endif

#ifndef AURA_PRINT
#	define AURA_PRINT(format, ...) wprintf(format, ##__VA_ARGS__)
#endif

#ifndef AURA_ERROR
# define AURA_ERROR(error, format, ...) wprintf(L"%lc e (%d, %hs) | %hs | " format L"\n", 195, error.value(), error.category().name(), error.message().c_str(), ##__VA_ARGS__)
#endif

#ifndef AURA_ASSERT
# if AURA_DEBUG
#	  define AURA_ASSERT(cond) if (!(cond)) { wprintf(L"%lc%lc " #cond L"\n", 192, 254); assert((cond)); }
# else
#   define AURA_ASSERT(cond)
# endif
#endif
