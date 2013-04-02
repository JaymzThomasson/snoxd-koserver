#pragma once

#include <tchar.h>
#include <string>

#ifdef UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif

void _string_format(const std::string fmt, std::string * result, va_list args);
std::string string_format(const std::string fmt, ...);
std::string & rtrim(std::string &s);
void tstrcpy(char *dst, size_t len, std::string & src);