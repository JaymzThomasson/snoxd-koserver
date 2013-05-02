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
	INLINE char * GetFTPUrl() { return m_strFtpUrl; };
	INLINE char * GetFTPPath() { return m_strFilePath; };

	INLINE News * GetNews() { return &m_news; };

	INLINE VersionInfoList* GetPatchList() { return &m_VersionList; };
	INLINE ServerInfoList* GetServerList() { return &m_ServerList; };

	bool Startup();

	~LoginServer();

	static KOSocketMgr<LoginSession> s_socketMgr;

private:
	void GetInfoFromIni();
	void WriteLogFile(std::string & logMessage);
	void ReportSQLError(OdbcError *pError);

	char	m_strFtpUrl[256], m_strFilePath[256];
	char	m_ODBCName[32], m_ODBCLogin[32], m_ODBCPwd[32];
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
