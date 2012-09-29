#pragma once

#include <string>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

#ifdef UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif

static std::string string_format(const std::string fmt, ...)
{
	char buffer[1024];
	va_list ap;

	va_start(ap, fmt);
	_vsnprintf_s(buffer, sizeof(buffer), sizeof(buffer), fmt.c_str(), ap);
	va_end(ap);

	return buffer;
}

// trim from end
static inline std::string & rtrim(std::string &s) 
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

static inline void tstrcpy(char *dst, size_t len, std::string & src)
{
	memset(dst, 0x00, len);
	memcpy(dst, src.c_str(), src.length() > len ? len : src.length());
}

#define _tstrcpy(dst, src) tstrcpy(dst, sizeof(dst), src)

#ifdef UNICODE
static tstring string_format(const tstring fmt, ...)
{
	TCHAR buffer[1024];
	va_list ap;

	va_start(ap, fmt);
	_vsntprintf_s(buffer, sizeof(buffer), sizeof(buffer), fmt.c_str(), ap);
	va_end(ap);

	return buffer;
}

static inline tstring & rtrim(tstring &s) 
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

static inline void tstrcpy(TCHAR *dst, size_t len, tstring & src)
{
	memset(dst, 0x00, len);
	memcpy(dst, src.c_str(), src.length() > len ? len : src.length());
}
#endif
