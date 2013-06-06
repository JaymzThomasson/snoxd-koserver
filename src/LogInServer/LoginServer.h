#pragma once

#include "../shared/KOSocketMgr.h"

typedef std::map <std::string, _VERSION_INFO *> VersionInfoList;
typedef std::vector<_SERVER_INFO *>	ServerInfoList;

class LoginSession;
class LoginServer
{
	friend class CDBProcess;
public:
	LoginServer();

	INLINE short GetVersion() { return m_sLastVersion; };
	INLINE std::string & GetFTPUrl() { return m_strFtpUrl; };
	INLINE std::string & GetFTPPath() { return m_strFilePath; };

	INLINE News * GetNews() { return &m_news; };

	INLINE VersionInfoList* GetPatchList() { return &m_VersionList; };
	INLINE ServerInfoList* GetServerList() { return &m_ServerList; };

	bool Startup();

	~LoginServer();

	KOSocketMgr<LoginSession> m_socketMgr;

private:
	void GetInfoFromIni();
	void WriteLogFile(std::string & logMessage);
	void ReportSQLError(OdbcError *pError);

	std::string m_strFtpUrl, m_strFilePath;
	std::string m_ODBCName, m_ODBCLogin, m_ODBCPwd;
	short	m_sLastVersion;

	VersionInfoList		m_VersionList;
	ServerInfoList		m_ServerList;

	News m_news;

	RWLock m_serverListLock, m_patchListLock;
	FastMutex m_lock;

	FILE *m_fp;
public:
	CDBProcess	m_DBProcess;
};

extern LoginServer * g_pMain;
