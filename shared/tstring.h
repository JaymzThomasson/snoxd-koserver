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

static tstring string_format(const tstring fmt, ...)
{
    int size = 128;
    tstring str;
    va_list ap;

	while (1) 
	{
        str.resize(size);
        va_start(ap, fmt);
        int n = _vsntprintf_s((TCHAR *)str.c_str(), size, size, fmt.c_str(), ap);
        va_end(ap);

        if (n > -1 && n < size)
		{
            str.resize(n);
            return str;
        }

        if (n > -1)
            size = n + 1;
        else
            size *= 2;
    }
}

// trim from end
static inline tstring & rtrim(tstring &s) 
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}