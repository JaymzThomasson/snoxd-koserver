#pragma once

#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#ifdef _DEBUG
// and cassert for our ASSERT() replacement
#		include <cassert>
#		include "DebugUtils.h"

#		define ASSERT assert
#		define TRACE FormattedDebugString
#else
#		define ASSERT 
#		define TRACE 
#endif

// Ignore the warning "nonstandard extension used: enum '<enum name>' used in qualified name"
// Sometimes it's necessary to avoid collisions, but aside from that, specifying the enumeration helps make code intent clearer.
#pragma warning(disable: 4482)