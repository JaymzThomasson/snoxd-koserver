#pragma once

#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#ifdef _DEBUG
// and cassert for our ASSERT() replacement
#		include <cassert>
#		define ASSERT assert
#		define TRACE printf
#else
#		define ASSERT 
#		define TRACE 
#endif

