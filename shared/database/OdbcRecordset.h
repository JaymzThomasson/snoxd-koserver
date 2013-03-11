#pragma once

class OdbcRecordset
{
public:
	OdbcRecordset(OdbcConnection * dbConnection);

	virtual tstring GetTableName() = 0;
	virtual tstring GetColumns() = 0;
	virtual tstring GetWhereClause() { return _T(""); }

	TCHAR * Read(bool bAllowEmptyTable = false);
	virtual bool Fetch() = 0;

	virtual ~OdbcRecordset();

protected:
	OdbcConnection * _dbConnection;
	OdbcCommand * _dbCommand;
};

#define _LOAD_TABLE(Set, DB, Array, AllowEmptyTable) \
	Set _ ## Set(DB, Array); \
	TCHAR * _szError ## Set = _ ## Set.Read(AllowEmptyTable);

#define LOAD_TABLE(Set, DB, Array, AllowEmptyTable) \
	_LOAD_TABLE(Set, DB, Array, AllowEmptyTable); \
	_HANDLE_DB_ERROR(_szError ## Set)

#define LOAD_TABLE_ERROR_ONLY(Set, DB, Array, AllowEmptyTable) \
	_LOAD_TABLE(Set, DB, Array, AllowEmptyTable); \
	_HANDLE_DB_ERROR_ONLY(_szError ## Set)

#define _HANDLE_DB_ERROR(err) \
	if (err != NULL) \
		AfxMessageBox(err); \
	return (err == NULL)

#define _HANDLE_DB_ERROR_ONLY(err) \
	if (err != NULL) { \
		AfxMessageBox(err); \
		return FALSE; \
	}