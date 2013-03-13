#pragma once

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
