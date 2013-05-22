#include "stdafx.h"
#include <sstream>
#include "../shared/Ini.h"

KOSocketMgr<LoginSession> LoginServer::s_socketMgr;

LoginServer::LoginServer() : m_sLastVersion(__VERSION), m_fp(nullptr)
{
	memset(m_strFtpUrl, 0, sizeof(m_strFtpUrl));
	memset(m_strFilePath, 0, sizeof(m_strFilePath));
	memset(m_ODBCName, 0, sizeof(m_ODBCName));
	memset(m_ODBCLogin, 0, sizeof(m_ODBCLogin));
	memset(m_ODBCPwd, 0, sizeof(m_ODBCPwd));
}

bool LoginServer::Startup()
{
	GetInfoFromIni();

	m_fp = fopen("./Login.log", "a");
	if (m_fp == nullptr)
	{
		printf("ERROR: Unable to open log file.\n");
		return false;
	}

	if (!m_DBProcess.Connect(m_ODBCName, m_ODBCLogin, m_ODBCPwd)) 
	{
		printf("ERROR: Unable to connect to the database using the details configured.\n");
		return false;
	}

	printf("Connected to database server.\n");
	if (!m_DBProcess.LoadVersionList())
	{
		printf("ERROR: Unable to load the version list.\n");
		return false;
	}

	printf("Latest version in database: %d\n", GetVersion());
	InitPacketHandlers();

	if (!s_socketMgr.Listen(_LISTEN_PORT, MAX_USER))
	{
		printf("ERROR: Failed to listen on server port.\n");
		return false;
	}

	s_socketMgr.RunServer();
	return true;
}

void LoginServer::GetInfoFromIni()
{
	CIni ini(CONF_LOGIN_SERVER);
	char tmp[128];

	ini.GetString("DOWNLOAD", "URL", "ftp.yoursite.net", m_strFtpUrl, sizeof(m_strFtpUrl), false);
	ini.GetString("DOWNLOAD", "PATH", "/", m_strFilePath, sizeof(m_strFilePath), false);

	ini.GetString("ODBC", "DSN", "KN_online", m_ODBCName, sizeof(m_ODBCName), false);
	ini.GetString("ODBC", "UID", "knight", m_ODBCLogin, sizeof(m_ODBCLogin), false);
	ini.GetString("ODBC", "PWD", "knight", m_ODBCPwd, sizeof(m_ODBCPwd), false);

	int nServerCount = ini.GetInt("SERVER_LIST", "COUNT", 1);
	if (nServerCount <= 0) 
		nServerCount = 1;
	
	char key[20]; 
	_SERVER_INFO* pInfo = nullptr;
	
	m_ServerList.reserve(nServerCount);

	// TO-DO: Replace this nonsense with something a little more versatile
	for (int i = 0; i < nServerCount; i++)
	{
		pInfo = new _SERVER_INFO;

		sprintf_s(key, sizeof(key), "SERVER_%02d", i);
		ini.GetString("SERVER_LIST", key, "127.0.0.1", pInfo->strServerIP, sizeof(pInfo->strServerIP), false);

		sprintf_s(key, sizeof(key), "LANIP_%02d", i);
		ini.GetString("SERVER_LIST", key, "127.0.0.1", pInfo->strLanIP, sizeof(pInfo->strLanIP), false);

		sprintf_s(key, sizeof(key), "NAME_%02d", i);
		ini.GetString("SERVER_LIST", key, "TEST|Server 1", pInfo->strServerName, sizeof(pInfo->strServerName), false);

		sprintf_s(key, sizeof(key), "ID_%02d", i);
		pInfo->sServerID = ini.GetInt("SERVER_LIST", key, 1);

		sprintf_s(key, sizeof(key), "GROUPID_%02d", i);
		pInfo->sGroupID = ini.GetInt("SERVER_LIST", key, 1);

		sprintf_s(key, sizeof(key), "PREMLIMIT_%02d", i);
		pInfo->sPlayerCap = ini.GetInt("SERVER_LIST", key, MAX_USER);

		sprintf_s(key, sizeof(key), "FREELIMIT_%02d", i);
		pInfo->sFreePlayerCap = ini.GetInt("SERVER_LIST", key, MAX_USER);

		sprintf_s(key, sizeof(key), "KING1_%02d", i);
		ini.GetString("SERVER_LIST", key, "", pInfo->strKarusKingName, sizeof(pInfo->strKarusKingName));

		sprintf_s(key, sizeof(key), "KING2_%02d", i);
		ini.GetString("SERVER_LIST", key, "", pInfo->strElMoradKingName, sizeof(pInfo->strElMoradKingName));

		sprintf_s(key, sizeof(key), "KINGMSG1_%02d", i);
		ini.GetString("SERVER_LIST", key, "", pInfo->strKarusNotice, sizeof(pInfo->strKarusNotice));

		sprintf_s(key, sizeof(key), "KINGMSG2_%02d", i);
		ini.GetString("SERVER_LIST", key, "", pInfo->strElMoradNotice, sizeof(pInfo->strElMoradNotice));

		m_ServerList.push_back(pInfo);
	}

	// Read news from INI (max 3 blocks)
	#define BOX_START '#' << uint8(0) << '\n'
	#define LINE_ENDING uint8(0) << '\n'
	#define BOX_END BOX_START << LINE_ENDING

	m_news.Size = 0;
	std::stringstream ss;
	for (int i = 0; i < 3; i++)
	{
		string title, message;

		sprintf_s(key, sizeof(key), "TITLE_%02d", i);
		ini.GetString("NEWS", key, "", tmp, sizeof(tmp));

		title = tmp;
		if (title.size() == 0)
			continue;
		
		sprintf_s(key, sizeof(key), "MESSAGE_%02d", i);
		ini.GetString("NEWS", key, "", tmp, sizeof(tmp));

		message = tmp;
		if (message.size() == 0)
			continue;

		size_t oldPos = 0, pos = 0;
		ss << title << BOX_START;

		// potentially support multiline by making | act as linebreaks (same as the TBL afaik, so at least we're conformant).
		//replace(messages[i].begin(), messages[i].end(), '|', '\n');
		//while ((pos = message.find('\r', pos)) != string::npos)
		//	message.erase(pos, 1);
		//Remove \n for now, perhaps re-implement later
		//while ((pos = message.find('\n', pos)) != string::npos)
		//	message.erase(pos, 1);

		ss << message << LINE_ENDING << BOX_END;
	}

	m_news.Size = ss.str().size();
	if (m_news.Size)
		memcpy(&m_news.Content, ss.str().c_str(), m_news.Size);
}

void LoginServer::WriteLogFile(string & logMessage)
{
	FastGuard lock(m_lock);
	fwrite(logMessage.c_str(), logMessage.length(), 1, m_fp);
	fflush(m_fp);
}

void LoginServer::ReportSQLError(OdbcError *pError)
{
	if (pError == nullptr)
		return;

	// This is *very* temporary.
	string errorMessage = string_format(_T("ODBC error occurred.\r\nSource: %s\r\nError: %s\r\nDescription: %s\n"),
		pError->Source.c_str(), pError->ExtendedErrorMessage.c_str(), pError->ErrorMessage.c_str());

	TRACE("%s", errorMessage.c_str());
	WriteLogFile(errorMessage);
	delete pError;
}

LoginServer::~LoginServer() 
{
	foreach (itr, m_ServerList)
		delete *itr;
	m_ServerList.clear();

	foreach (itr, m_VersionList)
		delete itr->second;
	m_VersionList.clear();

	if (m_fp != nullptr)
		fclose(m_fp);

	s_socketMgr.Shutdown();
}