#pragma once

#define _WIN32_WINNT 0x0500
#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

// The login server & Aujard aren't using MFC anymore.
// This can be simplified when Aujard's gone & Ebenezer/AI aren't using MFC.
#if defined(LOGIN_SERVER) || defined(AUJARD) || defined(AI_SERVER)

// Remember to include Winsock first
#	ifndef AUJARD
#		include <Winsock2.h>
#	endif

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

// MFC, ew.
#else
#	include <afxwin.h>
#endif

#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

#include "globals.h"
#ifndef AUJARD
#	include "KOSocket.h"
#	include "KOSocketMgr.h"
#endif
#include "STLMap.h"
#include "Ini.h"