#pragma once

#define _WIN32_WINNT _WIN32_WINNT_MAXVER
#ifdef LOGIN_SERVER
#include <Windows.h>
#else
#include <afxwin.h>
#endif