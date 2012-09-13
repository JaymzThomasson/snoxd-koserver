// DBProcess.h: interface for the CDBProcess class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DBPROCESS_H__D7F54E57_B37F_40C8_9E76_8C9F083842BF__INCLUDED_)
#define AFX_DBPROCESS_H__D7F54E57_B37F_40C8_9E76_8C9F083842BF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CVersionManagerDlg;
class CDBProcess  
{
public:
	BOOL IsCurrentUser( const char* accountid, char* strServerIP, int &serverno );
	void ReConnectODBC(CDatabase *m_db, const char *strdb, const char *strname, const char *strpwd);
	BOOL InitDatabase( char* strconnection );
	int AccountLogin( const char* id, const char* pwd );
	BOOL LoadVersionList();
	BOOL LoadUserCountList();

	CDBProcess();
	virtual ~CDBProcess();

	CDatabase	m_VersionDB;
	CVersionManagerDlg* m_pMain;
};

#endif // !defined(AFX_DBPROCESS_H__D7F54E57_B37F_40C8_9E76_8C9F083842BF__INCLUDED_)
