// DBProcess.cpp: implementation of the CDBProcess class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "define.h"
#include "DBProcess.h"
#include "VersionManagerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDBProcess::CDBProcess() : m_pMain(NULL)
{
}

bool CDBProcess::Connect(TCHAR *szDSN, TCHAR *szUser, TCHAR *szPass)
{
	if (m_pMain == NULL)
		m_pMain = (CVersionManagerDlg*)AfxGetApp()->GetMainWnd();

	return m_dbConnection.Connect(szDSN, szUser, szPass);
}

bool CDBProcess::LoadVersionList()
{
	bool result = false;
	auto dbCommand = m_dbConnection.CreateCommand();
	if (dbCommand == NULL)
		return false;

	if (!dbCommand->Execute(_T("SELECT sVersion, sHistoryVersion, strFileName FROM VERSION")))
		goto cleanup;

	m_pMain->m_nLastVersion = 0;
	if (dbCommand->hasData())
	{
		do
		{
			_VERSION_INFO *pVersion = new _VERSION_INFO;

			pVersion->sVersion = dbCommand->FetchUInt16(1);
			pVersion->sHistoryVersion = dbCommand->FetchUInt16(2);
			pVersion->strFileName = dbCommand->FetchString(3);

			m_pMain->m_VersionList.insert(make_pair(pVersion->strFileName, pVersion));

			if (m_pMain->m_nLastVersion < pVersion->sVersion)
				m_pMain->m_nLastVersion = pVersion->sVersion;

		} while (dbCommand->MoveNext());
	}

	result = true;

cleanup:
	delete dbCommand;	
	return result;
}

bool CDBProcess::LoadUserCountList()
{
	bool result = false;
	auto dbCommand = m_dbConnection.CreateCommand();
	if (dbCommand == NULL)
		return false;

	if (!dbCommand->Execute(_T("SELECT serverid, zone1_count, zone2_count, zone3_count FROM CONCURRENT")))
		goto cleanup;
	
	if (dbCommand->hasData())
	{
		do
		{
			uint8 serverID = dbCommand->FetchByte(1);
			uint16 zone_1 = dbCommand->FetchUInt16(2),
					zone_2 = dbCommand->FetchUInt16(3),
					zone_3 = dbCommand->FetchUInt16(4);

			if (serverID - 1 < m_pMain->m_nServerCount)
				m_pMain->m_ServerList[serverID - 1]->sUserCount = zone_1 + zone_2 + zone_3;
		} while (dbCommand->MoveNext());
	}

	result = true;

cleanup:
	delete dbCommand;
	return result;
}

uint16 CDBProcess::AccountLogin(string & id, string & pwd)
{
	uint16 result = 2; // account not found
	auto dbCommand = m_dbConnection.CreateCommand();
	if (dbCommand == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)id.c_str(), id.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)pwd.c_str(), pwd.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &result);

	if (!dbCommand->Prepare(_T("{CALL MAIN_LOGIN(?, ?, ?)}")))
	{
		OdbcError *pError = m_dbConnection.GetError();
		if (pError)
		{
			// error logging
			delete pError;
		}
	}

	delete dbCommand;
	return result;
}