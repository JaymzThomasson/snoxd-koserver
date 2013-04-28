#pragma once

#pragma comment(lib, "odbc32.lib")

#include <sqlext.h>
#include <set>

#include "../tstring.h"

struct OdbcError
{
	tstring	Source;
	tstring	ErrorMessage;
	tstring	ExtendedErrorMessage;
};

#include "OdbcCommand.h"

class FastMutex;
class OdbcConnection
{
	friend class OdbcCommand;

public:
	OdbcConnection();

	bool isConnected();
	bool isError();

	INLINE HDBC GetConnectionHandle() { return m_connHandle; }
	INLINE bool isMarsEnabled() { return m_bMarsEnabled; }


	bool Connect(tstring szDSN, tstring szUser, tstring szPass, bool bMarsEnabled = true);
	bool Connect();

	OdbcCommand *CreateCommand();
	static tstring GetSQLError(SQLSMALLINT handleType, SQLHANDLE handle);

	OdbcError *GetError();
	void ResetErrors();

	void Disconnect();
	~OdbcConnection();

private:
	void AddCommand(OdbcCommand *dbCommand);
	void RemoveCommand(OdbcCommand *dbCommand);
	tstring ReportSQLError(SQLSMALLINT handleType, SQLHANDLE handle, TCHAR *szSource, TCHAR *szError, ...);

	void Close();
	void ResetHandles();

private:
	tstring m_szDSN, m_szUser, m_szPass;

	HENV m_envHandle;
	HDBC m_connHandle;

	FastMutex * m_lock;

	std::vector<OdbcError   *> m_odbcErrors;
	std::set   <OdbcCommand *> m_commandSet;

	bool m_bMarsEnabled;
};