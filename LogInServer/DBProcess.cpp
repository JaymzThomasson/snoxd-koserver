// DBProcess.cpp: implementation of the CDBProcess class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "versionmanager.h"
#include "define.h"
#include "DBProcess.h"
#include "VersionManagerDlg.h"
#include "../shared/database/VersionSet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDBProcess::CDBProcess()
{

}

CDBProcess::~CDBProcess()
{

}

BOOL CDBProcess::InitDatabase(char *strconnection)
{
	m_VersionDB.SetLoginTimeout(100);
	m_pMain = (CVersionManagerDlg*)AfxGetApp()->GetMainWnd();

	if (!m_VersionDB.Open(NULL, FALSE, FALSE, strconnection, 0))
		return FALSE;

	return TRUE;
}

void CDBProcess::ReConnectODBC(CDatabase *m_db, const char *strdb, const char *strname, const char *strpwd)
{
	char strlog[256];	memset( strlog, 0x00, 256);
	CTime t = CTime::GetCurrentTime();
	sprintf_s(strlog, 256, "[%d-%d %d:%d] Trying to reconnect to ODBC...\r\n", t.GetMonth(), t.GetDay(), t.GetHour(), t.GetMinute());
	LogFileWrite( strlog );

	// DATABASE 연결...
	CString strConnect;
	strConnect.Format (_T("DSN=%s;UID=%s;PWD=%s"), strdb, strname, strpwd);
	int iCount = 0;

	do{	
		iCount++;
		if( iCount >= 4 )
			break;

		m_db->SetLoginTimeout(10);

		try
		{
			m_db->OpenEx((LPCTSTR )strConnect, CDatabase::noOdbcDialog);
		}
		catch( CDBException* e )
		{
			e->Delete();
		}
		
	}while(!m_db->IsOpen());	
}

BOOL CDBProcess::LoadVersionList()
{
	CVersionSet VersionSet(&m_VersionDB);
	if (VersionSet.Open() 
		&& !VersionSet.IsBOF() && !VersionSet.IsEOF())
	{
		VersionSet.MoveFirst();

		while (!VersionSet.IsEOF())
		{
			_VERSION_INFO* pInfo = new _VERSION_INFO;
			pInfo->sVersion = VersionSet.m_sVersion;
			pInfo->sHistoryVersion = VersionSet.m_sHistoryVersion;
	
			VersionInfoList::iterator itr = m_pMain->m_VersionList.find(pInfo->strFileName);
			if (itr != m_pMain->m_VersionList.end())
			{
				TRACE("VersionInfo PutData Fail - %s\n", pInfo->strFileName);
				delete pInfo;
			}
			else
			{
				m_pMain->m_VersionList.insert(make_pair(pInfo->strFileName, pInfo));
			}

			VersionSet.MoveNext();
		}
	}

	m_pMain->m_nLastVersion = 0;

	foreach (itr, m_pMain->m_VersionList)
	{
		if (m_pMain->m_nLastVersion < itr->second->sVersion)
			m_pMain->m_nLastVersion = itr->second->sVersion;
	}

	return TRUE;
}

int CDBProcess::AccountLogin(const char *id, const char *pwd)
{
	SQLHSTMT		hstmt = NULL;
	SQLRETURN		retcode;
	TCHAR			szSQL[1024];
	memset( szSQL, 0x00, 1024 );
	SQLSMALLINT		sParmRet = 3;
	SQLINTEGER		cbParmRet=SQL_NTS;

	wsprintf( szSQL, TEXT( "{call MAIN_LOGIN(\'%s\',\'%s\',?)}" ), id, pwd);

	retcode = SQLAllocHandle( (SQLSMALLINT)SQL_HANDLE_STMT, m_VersionDB.m_hdbc, &hstmt );
	if (retcode == SQL_SUCCESS)
	{
		retcode = SQLBindParameter(hstmt,1, SQL_PARAM_OUTPUT, SQL_C_SSHORT, SQL_SMALLINT, 0,0, &sParmRet,0, &cbParmRet );
		if(retcode == SQL_SUCCESS)
		{
			retcode = SQLExecDirect (hstmt, (unsigned char *)szSQL, 1024);
			if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO ) {
				if( DisplayErrorMsg(hstmt) == -1 ) {
					m_VersionDB.Close();
					if( !m_VersionDB.IsOpen() ) {
						ReConnectODBC( &m_VersionDB, m_pMain->m_ODBCName, m_pMain->m_ODBCLogin, m_pMain->m_ODBCPwd );
						return 2;
					}
				}
			}
		}

		SQLFreeHandle((SQLSMALLINT)SQL_HANDLE_STMT,hstmt);
	}
	
	return sParmRet;
}

BOOL CDBProcess::LoadUserCountList()
{
	SQLHSTMT		hstmt = NULL;
	SQLRETURN		retcode;
	TCHAR			szSQL[1024];
	
	CString tempfilename, tempcompname;

	memset(szSQL, 0x00, 1024);
	wsprintf(szSQL, TEXT("select * from CONCURRENT"));
	
	SQLCHAR serverid;
	SQLSMALLINT	zone_1 = 0, zone_2 = 0, zone_3 = 0;
	SQLINTEGER Indexind = SQL_NTS;

	retcode = SQLAllocHandle( (SQLSMALLINT)SQL_HANDLE_STMT, m_VersionDB.m_hdbc, &hstmt );
	if (retcode != SQL_SUCCESS)	return FALSE; 

	retcode = SQLExecDirect (hstmt, (unsigned char *)szSQL, 1024);	
	if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO ) {
		if( DisplayErrorMsg(hstmt) == -1 ) {
			m_VersionDB.Close();
			if( !m_VersionDB.IsOpen() ) {
				ReConnectODBC( &m_VersionDB, m_pMain->m_ODBCName, m_pMain->m_ODBCLogin, m_pMain->m_ODBCPwd );
				return FALSE;
			}
		}
		SQLFreeHandle((SQLSMALLINT)SQL_HANDLE_STMT,hstmt);
		return FALSE;
	}
	while (retcode == SQL_SUCCESS|| retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLFetch(hstmt);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO){
			SQLGetData(hstmt,1 ,SQL_C_TINYINT , &serverid,   0, &Indexind);
			SQLGetData(hstmt,2 ,SQL_C_SSHORT  , &zone_1, 0, &Indexind);
			SQLGetData(hstmt,3 ,SQL_C_SSHORT  , &zone_2, 0, &Indexind);
			SQLGetData(hstmt,4 ,SQL_C_SSHORT  , &zone_3, 0, &Indexind);

			// 여기에서 데이타를 받아서 알아서 사용....
			if( serverid-1 < m_pMain->m_nServerCount )
				m_pMain->m_ServerList[serverid-1]->sUserCount = zone_1 + zone_2 + zone_3;		// 기범이가 ^^;
		}
	}
	SQLFreeHandle((SQLSMALLINT)SQL_HANDLE_STMT,hstmt);

	return TRUE;
}

BOOL CDBProcess::IsCurrentUser(const char *accountid, char* strServerIP, int &serverno)
{
	SQLHSTMT		hstmt = NULL;
	SQLRETURN		retcode;
	BOOL retval;
	TCHAR			szSQL[1024];
	memset( szSQL, 0x00, 1024 );

	SQLINTEGER	nServerNo = 0;
	TCHAR strIP[16] = {0};
	SQLINTEGER Indexind = SQL_NTS;

	wsprintf( szSQL, TEXT( "SELECT nServerNo, strServerIP FROM CURRENTUSER WHERE strAccountID = \'%s\'" ), accountid );

	retcode = SQLAllocHandle( (SQLSMALLINT)SQL_HANDLE_STMT, m_VersionDB.m_hdbc, &hstmt );
	if (retcode != SQL_SUCCESS)	return FALSE; 

	retcode = SQLExecDirect (hstmt, (unsigned char *)szSQL, 1024);
	if (retcode == SQL_SUCCESS|| retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLFetch(hstmt);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			SQLGetData(hstmt,1 ,SQL_C_SSHORT, &nServerNo,	0,	&Indexind);
			SQLGetData(hstmt,2 ,SQL_C_CHAR, strIP, 15,	&Indexind);

			strcpy_s( strServerIP, 16, strIP );
			serverno = nServerNo;
			retval = TRUE;
		}
		else
			retval = FALSE;
	}
	else {
		if( DisplayErrorMsg(hstmt) == -1 ) {
			m_VersionDB.Close();
			if( !m_VersionDB.IsOpen() ) {
				ReConnectODBC( &m_VersionDB, m_pMain->m_ODBCName, m_pMain->m_ODBCLogin, m_pMain->m_ODBCPwd );
				return FALSE;
			}
		}
		retval = FALSE;
	}
	
	SQLFreeHandle((SQLSMALLINT)SQL_HANDLE_STMT,hstmt);

	return retval;
}
