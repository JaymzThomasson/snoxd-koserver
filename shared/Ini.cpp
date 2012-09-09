// Ini.cpp: implementation of the CIni class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Ini.h"

#define INI_BUFFER 512

CIni::CIni()
{

}

CIni::~CIni()
{

}

int CIni::GetInt(char* lpAppName, char* lpKeyName, int nDefault)
{
	if (m_szFileName[0] == 0)
		return -1;

	char tmp[INI_BUFFER];
	GetPrivateProfileInt(lpAppName, lpKeyName,nDefault, m_szFileName);
	if (!GetPrivateProfileString(lpAppName, lpKeyName, "", tmp, INI_BUFFER, m_szFileName))
	{
		sprintf_s(tmp, INI_BUFFER, "%d", nDefault);
		WritePrivateProfileString(lpAppName, lpKeyName, tmp, m_szFileName);
		return nDefault;
	}

	return GetPrivateProfileInt(lpAppName, lpKeyName, nDefault, m_szFileName);
}

void CIni::GetString(char* lpAppName, char* lpKeyName, char* lpDefault, char *lpOutString, int nOutLength, bool bAllowEmptyStrings /*= true */)
{
	if (m_szFileName[0] == 0)
	{
		strcpy_s(lpOutString, nOutLength, ""); // reset it
		return;
	}
	
	if (!GetPrivateProfileString(lpAppName, lpKeyName, "", lpOutString, nOutLength, m_szFileName)
		|| (!bAllowEmptyStrings && lpOutString[0] == 0))
	{
		WritePrivateProfileString(lpAppName, lpKeyName, lpDefault, m_szFileName);
		strcpy_s(lpOutString, nOutLength, lpDefault);
	}
}

bool CIni::SetPath(const char* filename)
{
	char IniPath[_MAX_PATH] = "";
	char Buf[_MAX_PATH], Path[_MAX_PATH];
	char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

	GetModuleFileName(AfxGetApp()->m_hInstance, Buf, _MAX_PATH);
	_splitpath(Buf,drive,dir,fname,ext);
	strcpy(Path, drive);
	strcat(Path, dir);		
	strcpy(IniPath, Path);
	wsprintf(IniPath, "%s%s", IniPath, filename);
	strcpy(m_szFileName, (LPCTSTR)IniPath);
	return true;
}

int CIni::SetInt(char* lpAppName, char* lpKeyName, int nDefault)
{
	if (m_szFileName[0] == 0)
		return -1;

	char tmpDefault[INI_BUFFER];
	memset(tmpDefault, 0x00, INI_BUFFER);
	sprintf_s(tmpDefault, INI_BUFFER, "%d", nDefault);

	return WritePrivateProfileString(lpAppName, lpKeyName, tmpDefault, m_szFileName);
}

int CIni::SetString(char* lpAppName, char* lpKeyName, char* lpDefault)
{
	if (m_szFileName[0] == 0)
		return -1;

	return WritePrivateProfileString(lpAppName, lpKeyName, lpDefault, m_szFileName);
}
