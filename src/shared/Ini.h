#pragma once

class CIni
{
private:
	std::string m_szFileName;

public:
	CIni(const char *lpFilename);

	int GetInt(char* lpAppName, char* lpKeyName, int nDefault);
	bool GetBool(char* lpAppName, char* lpKeyName, bool bDefault);
	void GetString(char* lpAppName, char* lpKeyName, char* lpDefault, char *lpOutString, int nOutLength, bool bAllowEmptyStrings = true);
	int SetInt(char* lpAppName, char* lpKeyName, int nDefault);
	int SetString(char* lpAppName, char* lpKeyName, char* lpDefault);
};
