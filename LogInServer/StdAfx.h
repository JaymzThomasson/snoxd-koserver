#pragma once

#define _WIN32_WINNT 0x0500
#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cassert>

#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

#ifdef _DEBUG
#define ASSERT assert
#define TRACE printf
#else
#define ASSERT 
#define TRACE 
#endif

#include "../shared/version.h"
#include "../shared/KOSocket.h"
#include "../shared/KOSocketMgr.h"
#include "../shared/STLMap.h"
#include "../shared/Ini.h"

#include "Define.h"
#include "DBProcess.h"
#include "LoginServer.h"
#include "LoginSession.h"

using namespace std;
