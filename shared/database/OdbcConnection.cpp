#include "stdafx.h"
#include "OdbcConnection.h"

OdbcConnection::OdbcConnection()
	: m_connHandle(NULL), m_envHandle(NULL), m_bMarsEnabled(false)
{
}

bool OdbcConnection::Connect(tstring szDSN, tstring szUser, tstring szPass, bool bMarsEnabled /*= true*/)
{
	m_szDSN = szDSN;
	m_szUser = szUser;
	m_szPass = szPass;
	m_bMarsEnabled = bMarsEnabled;

	return Connect();
}

bool OdbcConnection::Connect()
{
	if (m_szDSN.length() == 0)
		return false;

	tstring szConn = "DSN=" + m_szDSN + ";";
	// Reconnect if we need to.
	if (isConnected())
		Disconnect();

	// Allocate enviroment handle
	if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_envHandle)))
	{
		ReportSQLError(SQL_HANDLE_ENV, m_envHandle, _T("SQLAllocHandle"), _T("Unable to allocate environment handle."));
		goto error_handler;
	}

	// Request ODBC3 support
	if (!SQL_SUCCEEDED(SQLSetEnvAttr(m_envHandle, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0)))
	{
		ReportSQLError(SQL_HANDLE_ENV, m_envHandle, _T("SQLSetEnvAttr"), _T("Unable to set environment attribute (SQL_ATTR_ODBC_VERSION)."));
		goto error_handler;
	}

	// Allocate the connection handle
	if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_DBC, m_envHandle, &m_connHandle)))
	{
		ReportSQLError(SQL_HANDLE_ENV, m_envHandle, _T("SQLAllocHandle"), _T("Unable to allocate connection handle."));
		goto error_handler;
	}

/*	if (!SQL_SUCCEEDED(SQLConnect(m_connHandle, (SQLTCHAR *)m_szDSN.c_str(), SQL_NTS, 
										m_szUser.length() > 0 ? (SQLTCHAR *)m_szUser.c_str() : NULL, SQL_NTS,
										m_szPass.length() > 0 ? (SQLTCHAR *)m_szPass.c_str() : NULL, SQL_NTS)))
	{
		ReportSQLError(SQL_HANDLE_DBC, m_connHandle, _T("SQLConnect"), _T("Unable to establish connection."));
		goto error_handler;
	}*/

	if (m_szUser.length())
	{
		szConn += "UID=" + m_szUser + ";";
		if (m_szPass.length())
			szConn += "PWD=" + m_szPass + ";";
	}

	// Enable multiple active result sets
	if (m_bMarsEnabled)
		szConn += "MARS_Connection=yes;";

	if (!SQL_SUCCEEDED(SQLDriverConnect(m_connHandle, NULL, (SQLTCHAR *)szConn.c_str(), SQL_NTS, NULL, NULL, NULL, NULL)))
	{
		ReportSQLError(SQL_HANDLE_DBC, m_connHandle, _T("SQLDriverConnect"), _T("Unable to establish connection."));
		goto error_handler;
	}

	return true;

error_handler:
	ResetHandles();
	return false;
}

OdbcCommand *OdbcConnection::CreateCommand()
{
	if (!isConnected())
		return NULL;

	return new OdbcCommand(this);
}

void OdbcConnection::AddCommand(OdbcCommand *dbCommand)
{
	m_commandSet.insert(dbCommand);
}

void OdbcConnection::RemoveCommand(OdbcCommand *dbCommand)
{
	m_commandSet.erase(dbCommand);
}

// Used to internally reset handles. Should ONLY be used in special cases, otherwise we'll break the state of the connection.
void OdbcConnection::ResetHandles()
{
	// Free the connection handle if it's allocated
	if (m_connHandle != NULL)
	{
		SQLFreeHandle(SQL_HANDLE_DBC, m_connHandle);
		m_connHandle = NULL;
	}

	// Free the environment handle if it's allocated
	if (m_envHandle != NULL)
	{
		SQLFreeHandle(SQL_HANDLE_ENV, m_envHandle);
		m_envHandle = NULL;
	}
}

tstring OdbcConnection::ReportSQLError(SQLSMALLINT handleType, SQLHANDLE handle,
								 TCHAR *szSource, TCHAR *szError, ...)
{
	tstring szErrorMessage;
	TCHAR szErrorBuffer[256];
	OdbcError *error = new OdbcError();

	va_list args;
	va_start(args, szError);
	_vsntprintf_s(szErrorBuffer, sizeof(szErrorBuffer), sizeof(szErrorBuffer), szError, args);
	va_end(args);

	error->Source = szSource;
	error->ErrorMessage = szErrorBuffer;

	m_odbcErrors.push_back(error);

	if (handle != NULL)
	{
		error->ExtendedErrorMessage = GetSQLError(handleType, handle);
		return error->ExtendedErrorMessage;
	}
	
	return szErrorMessage;
}

tstring OdbcConnection::GetSQLError(SQLSMALLINT handleType, SQLHANDLE handle)
{
	tstring result;
	SQLTCHAR SqlState[256], SqlMessage[256];
	SQLINTEGER NativeError;
	SQLSMALLINT TextLength;

	if (!SQL_SUCCEEDED(SQLGetDiagRec(handleType, handle, 1, (SQLTCHAR *)&SqlState, &NativeError, (SQLTCHAR *)&SqlMessage, sizeof(SqlMessage), &TextLength)))
		return result;

	result = (TCHAR *)SqlMessage;
	return result;
}

OdbcError *OdbcConnection::GetError()
{
	if (!isError())
		return NULL;

	OdbcError *pError = m_odbcErrors.back();
	m_odbcErrors.pop_back();

	return pError;
}

void OdbcConnection::ResetErrors()
{
	if (!isError())
		return;

	OdbcError *pError;
	while ((pError = GetError()) != NULL)
		delete pError;
}

void OdbcConnection::Disconnect()
{
	// Make sure our handles are open. If not, there's nothing to do.
	if (!isConnected())
		return;

	// Kill off open statements
	if (m_commandSet.size())
	{
		for (std::set<OdbcCommand *>::iterator itr = m_commandSet.begin(); itr != m_commandSet.end(); itr++)
		{
			// Detach from the connection first so we don't try to remove it from the set (while we're using it!)
			(*itr)->Detach();

			// Now free it.
			delete (*itr);
		}

		m_commandSet.clear();
	}

	// Disconnect from server.
	SQLDisconnect(m_connHandle);

	// Reset handles
	ResetHandles();
}

OdbcConnection::~OdbcConnection()
{
	Disconnect();
	ResetErrors();
}
