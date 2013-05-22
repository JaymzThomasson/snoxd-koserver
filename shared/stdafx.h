#pragma once

// #define _WIN32_WINNT 0x0500

// Not using MFC, so we need to Windows.h for most things
#	include "WindowsHeaders.h"

// If we support C++11, use experimental std::thread implementation
#if (__cplusplus >= 201103L) /* C++11 */  || (_MSC_VER >= 1700) /* VS2012 */
#	include <thread>
#	define USE_STD_THREAD
#endif

#include "types.h"
#include "globals.h"
#include "TimeThread.h"
