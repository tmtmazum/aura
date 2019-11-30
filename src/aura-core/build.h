#pragma once

#include <cassert>

#ifndef AURA_LOG
#	define AURA_LOG(...) wprintf(__VA_ARGS__)
#endif

#ifndef AURA_ASSERT
#	define AURA_ASSERT(...) assert(__VA_ARGS__)
#endif
