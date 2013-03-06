#pragma once

#define _WIN32_WINNT 0x0500
#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <cassert>

#include <vector>
#include <string>

#ifdef _DEBUG
#define ASSERT assert
#define TRACE printf
#else
#define ASSERT 
#define TRACE 
#endif

#include "../shared/STLMap.h"
#include "../shared/Ini.h"

#include "Define.h"
#include "DBAgent.h"
#include "AujardDlg.h"

using namespace std;

