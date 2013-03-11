#pragma once

class OdbcRecordset
{
public:
	OdbcRecordset(OdbcConnection * dbConnection);

	virtual tstring GetSQL() = 0;
	TCHAR * Read(bool bAllowEmptyTable = false);
	virtual void Fetch() = 0;

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
	HANDLE_DB_ERROR(_szError ## Set)

#define HANDLE_DB_ERROR(err) \
	if (err != NULL) \
		AfxMessageBox(err); \
	return (err == NULL)
