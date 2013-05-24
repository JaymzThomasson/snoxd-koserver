#include "stdafx.h"
#include "Ini.h"

#define INI_BUFFER 512

CIni::CIni(const char *lpFilename)
{
	m_szFileName = lpFilename;
}

int CIni::GetInt(char* lpAppName, char* lpKeyName, int nDefault)
{
	char tmp[INI_BUFFER];
	GetPrivateProfileInt(lpAppName, lpKeyName,nDefault, m_szFileName.c_str());
	if (!GetPrivateProfileString(lpAppName, lpKeyName, "", tmp, INI_BUFFER, m_szFileName.c_str()))
	{
		sprintf_s(tmp, INI_BUFFER, "%d", nDefault);
		WritePrivateProfileString(lpAppName, lpKeyName, tmp, m_szFileName.c_str());
		return nDefault;
	}

	return GetPrivateProfileInt(lpAppName, lpKeyName, nDefault, m_szFileName.c_str());
}

bool CIni::GetBool(char* lpAppName, char* lpKeyName, bool bDefault)
{
	return GetInt(lpAppName, lpKeyName, bDefault) == 1;
}

void CIni::GetString(char* lpAppName, char* lpKeyName, char* lpDefault, char *lpOutString, int nOutLength, bool bAllowEmptyStrings /*= true */)
{
	
	if (!GetPrivateProfileString(lpAppName, lpKeyName, "", lpOutString, nOutLength, m_szFileName.c_str())
		|| (!bAllowEmptyStrings && lpOutString[0] == 0))
	{
		WritePrivateProfileString(lpAppName, lpKeyName, lpDefault, m_szFileName.c_str());
		strcpy_s(lpOutString, nOutLength, lpDefault);
	}
}

int CIni::SetInt(char* lpAppName, char* lpKeyName, int nDefault)
{
	char tmpDefault[INI_BUFFER];
	sprintf_s(tmpDefault, INI_BUFFER, "%d", nDefault);

	return WritePrivateProfileString(lpAppName, lpKeyName, tmpDefault, m_szFileName.c_str());
}

int CIni::SetString(char* lpAppName, char* lpKeyName, char* lpDefault)
{
	return WritePrivateProfileString(lpAppName, lpKeyName, lpDefault, m_szFileName.c_str());
}
