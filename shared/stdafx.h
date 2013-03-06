#pragma once

#define _WIN32_WINNT _WIN32_WINNT_MAXVER
#if defined(LOGIN_SERVER) || defined(AUJARD)
#include <Windows.h>
#else
#include <afxwin.h>
#endif