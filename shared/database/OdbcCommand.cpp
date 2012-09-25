#include "stdafx.h"
#include "OdbcConnection.h"

OdbcCommand::OdbcCommand(HDBC conn)
	: m_odbcConnection(NULL), m_connHandle(conn), m_hStmt(NULL), m_resultCode(-1)
{
}

OdbcCommand::OdbcCommand(OdbcConnection * conn)
	: m_odbcConnection(conn), m_hStmt(NULL)
{
	m_connHandle = conn->GetConnectionHandle();
	m_odbcConnection->AddCommand(this);
}

bool OdbcCommand::Open()
{
	if (isOpen())
		Close();

	if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, m_connHandle, &m_hStmt)))
	{
		if (m_odbcConnection != NULL)
			m_szError = m_odbcConnection->ReportSQLError(SQL_HANDLE_DBC, m_connHandle, _T("SQLAllocHandle"), _T("Failed to allocate statement handle."));
		else
			m_szError = OdbcConnection::GetSQLError(SQL_HANDLE_DBC, m_connHandle);

		return false;
	}

	return true;
}

bool OdbcCommand::Execute(const tstring & szSQL)
{
	if (!Open())
		return false;

	if (!SQL_SUCCEEDED(SQLExecDirect(m_hStmt, (SQLTCHAR *)szSQL.c_str(), szSQL.length())))
	{
		if (m_odbcConnection != NULL)
			m_szError = m_odbcConnection->ReportSQLError(SQL_HANDLE_STMT, m_hStmt, _T("SQLExecDirect"), _T("Failed to execute statement."));
		else
			m_szError = OdbcConnection::GetSQLError(SQL_HANDLE_STMT, m_hStmt);

		Close();
		return false;
	}

	if (!MoveNext())
		MoveNextSet();

	return true;
}

bool OdbcCommand::MoveNext()
{
	if (!isOpen())
		return false;

	return SQL_SUCCEEDED(m_resultCode = SQLFetch(m_hStmt));
}

bool OdbcCommand::MoveNextSet()
{
	if (!isOpen())
		return false;

	return SQL_SUCCEEDED(m_resultCode = SQLMoreResults(m_hStmt));
}

bool OdbcCommand::Prepare(const tstring & szSQL)
{
	if (!Open())
		return false;

	if (!SQL_SUCCEEDED(SQLPrepare(m_hStmt, (SQLTCHAR *)szSQL.c_str(), szSQL.length())))
	{
		if (m_odbcConnection != NULL)
			m_szError = m_odbcConnection->ReportSQLError(SQL_HANDLE_STMT, m_hStmt, _T("SQLPrepare"), _T("Failed to prepare statement."));
		else
			m_szError = OdbcConnection::GetSQLError(SQL_HANDLE_STMT, m_hStmt);

		Close();
		return false;
	}

	// Bind parameters
	for (auto itr = m_params.begin(); itr != m_params.end(); itr++)
	{
		SQLINTEGER pcbValue = SQL_NTS;
		auto param = itr->second;

		if (!SQL_SUCCEEDED(SQLBindParameter(m_hStmt, itr->first + 1, param->GetParameterType(), param->GetCDataType(), param->GetDataType(), param->GetDataTypeSize(), 0, param->GetAddress(), param->GetDataTypeSize(), &pcbValue)))
		{
			if (m_odbcConnection != NULL)
				m_szError = m_odbcConnection->ReportSQLError(SQL_HANDLE_STMT, m_hStmt, _T("SQLBindParameter"), _T("Failed to bind parameter."));
			else
				m_szError = OdbcConnection::GetSQLError(SQL_HANDLE_STMT, m_hStmt);

			Close();
			return false;
		}
	}

	if (!SQL_SUCCEEDED(SQLExecute(m_hStmt)))
	{
		if (m_odbcConnection != NULL)
			m_szError = m_odbcConnection->ReportSQLError(SQL_HANDLE_STMT, m_hStmt, _T("SQLExecute"), _T("Failed to execute prepared statement."));
		else
			m_szError = OdbcConnection::GetSQLError(SQL_HANDLE_STMT, m_hStmt);

		Close();
		return false;
	}

	// If there's no rows to move through, skip to the next result set.
	if (!MoveNext())
		MoveNextSet();

	ClearParameters();
	return true;
}


#define ADD_ODBC_PARAMETER(name, type, sqlType) void OdbcCommand::AddParameter(SQLSMALLINT paramType, type *value, SQLLEN maxLength) { m_params.insert(std::make_pair(m_params.size(), new OdbcParameter(paramType, sqlType, (SQLPOINTER)value, maxLength))); } \
	type OdbcCommand::Fetch ## name(int pos, SQLLEN maxLength) { type value; SQLINTEGER cb = SQL_NTS; SQLGetData(m_hStmt, pos, sqlType, &value, maxLength, &cb); return value; };
ADD_ODBC_PARAMETER(Byte, uint8, SQL_C_TINYINT)
ADD_ODBC_PARAMETER(SByte, int8, SQL_C_STINYINT)
ADD_ODBC_PARAMETER(Char, char, SQL_C_CHAR)
ADD_ODBC_PARAMETER(UInt16, uint16, SQL_C_USHORT)
ADD_ODBC_PARAMETER(Int16, int16, SQL_C_SSHORT)
ADD_ODBC_PARAMETER(UInt32, uint32, SQL_C_ULONG)
ADD_ODBC_PARAMETER(Int32, int32, SQL_C_LONG)
ADD_ODBC_PARAMETER(Single, float, SQL_C_FLOAT)
ADD_ODBC_PARAMETER(Double, double, SQL_C_DOUBLE)
#undef ADD_ODBC_PARAMETER

tstring OdbcCommand::FetchString(int pos, SQLLEN maxLength)
{
	SQLINTEGER bufferSize = 0;
	TCHAR buffer[256];

	if (SQL_SUCCEEDED(SQLGetData(m_hStmt, pos, SQL_C_TCHAR, buffer, sizeof(buffer), &bufferSize)))
		return tstring(buffer);

	// A bigger buffer is needed
	if (bufferSize > 0)
	{
		std::auto_ptr<TCHAR> p_data(new TCHAR[bufferSize + 1]);
		SQLGetData(m_hStmt, pos, SQL_C_TCHAR, (SQLTCHAR *)p_data.get(), bufferSize + 1, &bufferSize);
		return tstring(p_data.get());
	}

	return tstring();
};

void OdbcCommand::FetchString(int pos, TCHAR *charArray, SQLLEN maxLength)
{
	SQLINTEGER bufferSize = 0;
//	memset(charArray, 0x00, maxLength);
	SQLGetData(m_hStmt, pos, SQL_C_TCHAR, charArray, maxLength, &bufferSize);
}

void OdbcCommand::ClearParameters()
{
	if (m_params.size())
	{
		for (auto itr = m_params.begin(); itr != m_params.end(); itr++)
			delete itr->second;

		m_params.clear();
	}
}

void OdbcCommand::Close()
{
	if (m_hStmt != NULL)
	{
		SQLCloseCursor(m_hStmt); // free results, if any
		SQLFreeHandle(SQL_HANDLE_STMT, m_hStmt);
	}
}

void OdbcCommand::Detach()
{
	m_odbcConnection = NULL;
}

OdbcCommand::~OdbcCommand()
{
	ClearParameters();
	Close();

	if (m_odbcConnection != NULL)
		m_odbcConnection->RemoveCommand(this);
}