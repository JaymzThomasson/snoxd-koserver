#pragma once

#pragma comment(lib, "odbc32.lib")

#include <tchar.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

#include <string>
#include <set>
#include <vector>

#ifdef UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif

#include "OdbcCommand.h"

struct OdbcError
{
	tstring	Source;
	tstring	ErrorMessage;
	tstring	ExtendedErrorMessage;
};

class OdbcConnection
{
	friend class OdbcCommand;

public:
	OdbcConnection();

	__forceinline bool isConnected() { return m_connHandle != NULL; };
	__forceinline bool isError() { return m_odbcErrors.size() > 0; };
	__forceinline HDBC GetConnectionHandle() { return m_connHandle; };

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
	void ResetHandles();

private:
	tstring m_szDSN, m_szUser, m_szPass;

	HENV m_envHandle;
	HDBC m_connHandle;

	std::vector<OdbcError   *> m_odbcErrors;
	std::set   <OdbcCommand *> m_commandSet;

	bool m_bMarsEnabled;
};