#pragma once

#include "../shared/database/OdbcConnection.h"

class CDBProcess  
{
public:
	bool Connect(TCHAR *szDSN, TCHAR *szUser, TCHAR *szPass);

	bool LoadVersionList();
	bool LoadUserCountList();

	uint16 AccountLogin(std::string & id, std::string & pwd);

private:
	OdbcConnection m_dbConnection;
};