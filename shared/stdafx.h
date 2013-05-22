#pragma once

// #define _WIN32_WINNT 0x0500

// Not using MFC, so we need to Windows.h for most things
#	include "WindowsHeaders.h"

#define STR(str) #str
#define STRINGIFY(str) STR(str)

// If we support C++11, use experimental std::thread implementation
#if (__cplusplus >= 201103L) /* C++11 */  || (_MSC_VER >= 1700) /* VS2012 */
#	include <thread>
#	include <chrono>

// Use new portable C++11 functionality.
#	define USE_STD_THREAD
#	define USE_STD_MUTEX
#	ifdef USE_STD_MUTEX
#		define USE_STD_CONDITION_VARIABLE
#	endif

// Portable C++11 thread sleep call.
#	define sleep(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))

// Warn users of typical non-portable WinAPI calls.
#	ifdef WIN32

// WARNING: Sleep() is a non-portable Windows API call. Please use sleep() instead.
#		define Sleep(ms) \
			__pragma(message(__FILE__ "(" STRINGIFY(__LINE__) "): WARNING: Sleep() is a non-portable Windows API call. Please use sleep() instead.")) \
			sleep(ms) /* make the correct call to avoid unexpected behaviour */
#endif

// Retain support for older Windows compilers (for now).
// Other platforms rely on C++11 support.
#else
#	define sleep(ms) Sleep(ms)
#endif

#ifndef WIN32
#	define SetConsoleTitle(title) /* unsupported & unnecessary */
#endif

#include "types.h"
#include "globals.h"
#include "TimeThread.h"
