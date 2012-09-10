// Ini.h: interface for the CIni class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INI_H__51A75906_827F_44DE_B612_F34750B4A1C8__INCLUDED_)
#define AFX_INI_H__51A75906_827F_44DE_B612_F34750B4A1C8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CIni  
{
private:
	char m_szFileName[_MAX_PATH];

public:
	CIni();
	CIni(const char *lpFilename);

	int GetInt(char* lpAppName, char* lpKeyName, int nDefault);
	void GetString(char* lpAppName, char* lpKeyName, char* lpDefault, char *lpOutString, int nOutLength, bool bAllowEmptyStrings = true);
	int SetInt(char* lpAppName, char* lpKeyName, int nDefault);
	int SetString(char* lpAppName, char* lpKeyName, char* lpDefault);
	bool SetPath(const char* lpFilename);
};

#endif // !defined(AFX_INI_H__51A75906_827F_44DE_B612_F34750B4A1C8__INCLUDED_)
