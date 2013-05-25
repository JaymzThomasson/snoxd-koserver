#pragma once

#include <cstring>
#include <string>

// This header is Windows-specific
// Allow for minimal code changes for other systems.
#ifdef WIN32
#include <tchar.h>
#else
#define TCHAR char
#define _T(x) x
#define _tprintf printf
#define _vsntprintf _vsnprintf
#define _stprintf sprintf
#endif

#ifdef UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif

void _string_format(const std::string fmt, std::string * result, va_list args);
std::string string_format(const std::string fmt, ...);
std::string & rtrim(std::string &s);
std::string & ltrim(std::string &s);
void tstrcpy(char *dst, size_t len, std::string & src);