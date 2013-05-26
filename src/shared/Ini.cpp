#include "stdafx.h"
#include "Ini.h"

#ifdef USE_CUSTOM_INI_PARSER
#include <iostream>
#include <fstream>
#include "tstring.h"
#endif

#define INI_BUFFER 512

CIni::CIni(const char *lpFilename)
{
	m_szFileName = lpFilename;

#ifdef USE_CUSTOM_INI_PARSER
	Load(lpFilename);
#endif
}

#ifdef USE_CUSTOM_INI_PARSER
bool CIni::Load(const char * lpFilename /*= nullptr*/)
{
	const char * fn = (lpFilename == nullptr ? m_szFileName.c_str() : lpFilename);
	std::ifstream file(fn);
	if (!file)
	{
		printf("Error: Unable to load config from file:\n%s\n", fn);
		return false;
	}

	std::string currentSection;

	// If an invalid section is hit
	// Ensure that we don't place key/value pairs
	// from the invalid section into the previously loaded section.
	bool bSkipNextSection = false; 
	while (!file.eof())
	{
		std::string line;
		getline(file, line);

		rtrim(line);
		if (line.empty())
			continue;

		// Check for value strings first
		// It's faster than checking for a section
		// at the expense of of not being able to use '=' in section names.
		// As this is uncommon behaviour, this is a suitable trade-off.
		size_t keySeparatorPos = line.find(INI_KEY_SEPARATOR);
		if (keySeparatorPos != std::string::npos)
		{
			if (bSkipNextSection)
				continue;

			std::string key = line.substr(0, keySeparatorPos),
						value = line.substr(keySeparatorPos + 1);

			// Clean up key/value to allow for 'key = value'
			rtrim(key);   /* remove trailing whitespace from keys */
			ltrim(value); /* remove preleading whitespace from values */

			ConfigMap::iterator itr = m_configMap.find(currentSection);
			if (itr == m_configMap.end())
			{
				m_configMap.insert(std::make_pair(currentSection, ConfigEntryMap()));
				itr = m_configMap.find(currentSection);
			}

			itr->second[key] = value;
			continue;
		}

		// Not a value, so assume it's a section
		size_t sectionStart = line.find_first_of(INI_SECTION_START),
				sectionEnd = line.find_last_of(INI_SECTION_END);

		if (sectionStart == std::string::npos
			|| sectionEnd == std::string::npos
			|| sectionStart > sectionEnd)
		{
			/* invalid section */
			bSkipNextSection = true;
			continue;
		}

		currentSection = line.substr(sectionStart + 1, sectionEnd - 1);
		bSkipNextSection = false;
	}

	file.close();
	return true;
}

void CIni::Save(const char * lpFilename /*= nullptr*/)
{
	const char * fn = (lpFilename == nullptr ? m_szFileName.c_str() : lpFilename);
	FILE * fp = fopen(fn, "w");
	foreach (sectionItr, m_configMap)
	{
		// Start the section
		fprintf(fp, "[%s]" INI_NEWLINE, sectionItr->first.c_str());

		// Now list out all the key/value pairs
		foreach (keyItr, sectionItr->second)
			fprintf(fp, "%s=%s" INI_NEWLINE, keyItr->first.c_str(), keyItr->second.c_str());

		// Use a trailing newline to finish the section, to make it easier to read
		fprintf(fp, INI_NEWLINE);
	}
	fclose(fp);
}

#endif

int CIni::GetInt(char* lpAppName, char* lpKeyName, int nDefault)
{
#ifdef USE_CUSTOM_INI_PARSER
	ConfigMap::iterator sectionItr = m_configMap.find(lpAppName);
	if (sectionItr != m_configMap.end())
	{
		ConfigEntryMap::iterator keyItr = sectionItr->second.find(lpKeyName);
		if (keyItr != sectionItr->second.end())
			return atoi(keyItr->second.c_str());
	}

	SetInt(lpAppName, lpKeyName, nDefault);
	return nDefault;
#else
	char tmp[INI_BUFFER];
	GetPrivateProfileInt(lpAppName, lpKeyName,nDefault, m_szFileName.c_str());
	if (!GetPrivateProfileString(lpAppName, lpKeyName, "", tmp, INI_BUFFER, m_szFileName.c_str()))
	{
		_snprintf(tmp, INI_BUFFER, "%d", nDefault);
		WritePrivateProfileString(lpAppName, lpKeyName, tmp, m_szFileName.c_str());
		return nDefault;
	}

	return GetPrivateProfileInt(lpAppName, lpKeyName, nDefault, m_szFileName.c_str());
#endif
}

bool CIni::GetBool(char* lpAppName, char* lpKeyName, bool bDefault)
{
	return GetInt(lpAppName, lpKeyName, bDefault) == 1;
}

void CIni::GetString(char* lpAppName, char* lpKeyName, char* lpDefault, char *lpOutString, int nOutLength, bool bAllowEmptyStrings /*= true */)
{
#ifdef USE_CUSTOM_INI_PARSER
	ConfigMap::iterator sectionItr = m_configMap.find(lpAppName);
	if (sectionItr != m_configMap.end())
	{
		ConfigEntryMap::iterator keyItr = sectionItr->second.find(lpKeyName);
		if (keyItr != sectionItr->second.end())
		{
			strncpy(lpOutString, keyItr->second.c_str(), nOutLength);
			return;
		}
	}

	SetString(lpAppName, lpKeyName, lpDefault);
	strncpy(lpOutString, lpDefault, nOutLength);
#else
	if (!GetPrivateProfileString(lpAppName, lpKeyName, "", lpOutString, nOutLength, m_szFileName.c_str())
		|| (!bAllowEmptyStrings && lpOutString[0] == 0))
	{
		WritePrivateProfileString(lpAppName, lpKeyName, lpDefault, m_szFileName.c_str());
		strncpy(lpOutString, lpDefault, nOutLength);
	}
#endif
}

int CIni::SetInt(char* lpAppName, char* lpKeyName, int nDefault)
{
	char tmpDefault[INI_BUFFER];
	_snprintf(tmpDefault, INI_BUFFER, "%d", nDefault);
	return SetString(lpAppName, lpKeyName, tmpDefault);
}

int CIni::SetString(char* lpAppName, char* lpKeyName, char* lpDefault)
{
#ifdef USE_CUSTOM_INI_PARSER
	ConfigMap::iterator itr = m_configMap.find(lpAppName);
	if (itr == m_configMap.end())
	{
		m_configMap.insert(std::make_pair(lpAppName, ConfigEntryMap()));
		itr = m_configMap.find(lpAppName);
	}
	itr->second[lpKeyName] = lpDefault;
	Save();
	return 1;
#else
	return WritePrivateProfileString(lpAppName, lpKeyName, lpDefault, m_szFileName.c_str());
#endif
}
