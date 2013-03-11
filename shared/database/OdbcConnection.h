#pragma once

#pragma comment(lib, "odbc32.lib")

#include <tchar.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

#include <set>
#include <vector>

#include "../tstring.h"
#include "../Mutex.h"

#include "OdbcCommand.h"
#include "OdbcRecordset.h"

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

	__forceinline bool isConnected() 
	{
		bool result;
		m_lock.Acquire();
		result = m_connHandle != NULL; 
		m_lock.Release();
		return result;
	}

	__forceinline bool isError() 
	{
		bool result;
		m_lock.Acquire();
		result = m_odbcErrors.size() > 0; 
		m_lock.Release();
		return result;
	}

	__forceinline HDBC GetConnectionHandle() { return m_connHandle; }
	__forceinline bool isMarsEnabled() { return m_bMarsEnabled; }


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

	FastMutex m_lock;

	std::vector<OdbcError   *> m_odbcErrors;
	std::set   <OdbcCommand *> m_commandSet;

	bool m_bMarsEnabled;
};