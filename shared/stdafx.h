#pragma once

#define _WIN32_WINNT 0x0500
#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

// Remember to include Winsock first
#	include <Winsock2.h>

// Not using MFC, so we need to Windows.h for most things
#	include <Windows.h>

// and cassert for our ASSERT() replacement
#	include <cassert>

// MFC defines these normally, so we'll define these ourselves.
#	ifdef _DEBUG
#		define ASSERT assert
#		define TRACE printf
#else
#		define ASSERT 
#		define TRACE 
#endif

#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

#include "globals.h"
#include "KOSocket.h"
#include "KOSocketMgr.h"
#include "STLMap.h"
#include "Ini.h"