// VersionManagerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "VersionManager.h"
#include "VersionManagerDlg.h"
#include "IOCPSocket2.h"
#include "VersionSet.h"
#include "User.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CIOCPort	CVersionManagerDlg::m_Iocport;

/////////////////////////////////////////////////////////////////////////////
// CVersionManagerDlg dialog

CVersionManagerDlg::CVersionManagerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVersionManagerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVersionManagerDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	
	memset(m_strFtpUrl, 0, sizeof(m_strFtpUrl));
	memset(m_strFilePath, 0, sizeof(m_strFilePath));
	m_nLastVersion = 0;
	memset(m_ODBCName, 0, sizeof(m_ODBCName));
	memset(m_ODBCLogin, 0, sizeof(m_ODBCLogin));
	memset(m_ODBCPwd, 0, sizeof(m_ODBCPwd));

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVersionManagerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVersionManagerDlg)
	DDX_Control(pDX, IDC_LIST1, m_OutputList);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVersionManagerDlg, CDialog)
	//{{AFX_MSG_MAP(CVersionManagerDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_EXIT, &CVersionManagerDlg::OnBnClickedExit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVersionManagerDlg message handlers

BOOL CVersionManagerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	m_Iocport.Init( MAX_USER, CLIENT_SOCKSIZE, 1 );
	
	for (int i=0; i<MAX_USER; i++)
		m_Iocport.m_SockArrayInActive[i] = new CUser;

	if (!m_Iocport.Listen(_LISTEN_PORT))
	{
		AfxMessageBox("Failed to listen on server port.");
		AfxPostQuitMessage(0);
		return FALSE;
	}

	if (!GetInfoFromIni())
	{
		AfxMessageBox("INI not found or not configured properly.");
		AfxPostQuitMessage(0);
		return FALSE;
	}
	
	char strConnection[256];
	sprintf_s(strConnection, sizeof(strConnection), "ODBC;DSN=%s;UID=%s;PWD=%s", m_ODBCName, m_ODBCLogin, m_ODBCPwd);

	if (!m_DBProcess.InitDatabase(strConnection)) 
	{
		AfxMessageBox("Unable to connect to the database using the details configured.");
		AfxPostQuitMessage(0);
		return FALSE;
	}

	if (!m_DBProcess.LoadVersionList())
	{
		AfxMessageBox("Unable to load the version list.");
		AfxPostQuitMessage(0);
		return FALSE;
	}

	m_OutputList.AddString(strConnection);
	CString version;
	version.Format("Latest Version : %d", m_nLastVersion);
	m_OutputList.AddString( version );

	::ResumeThread(m_Iocport.m_hAcceptThread);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CVersionManagerDlg::GetInfoFromIni()
{
	CString inipath;

	inipath.Format("%s\\Version.ini", GetProgPath());

	GetPrivateProfileString("DOWNLOAD", "URL", "ftp.yoursite.net", m_strFtpUrl, sizeof(m_strFtpUrl), inipath);
	GetPrivateProfileString("DOWNLOAD", "PATH", "/", m_strFilePath, sizeof(m_strFilePath), inipath);

	GetPrivateProfileString("ODBC", "DSN", "KN_online", m_ODBCName, sizeof(m_ODBCName), inipath);
	GetPrivateProfileString("ODBC", "UID", "knight", m_ODBCLogin, sizeof(m_ODBCLogin), inipath);
	GetPrivateProfileString("ODBC", "PWD", "knight", m_ODBCPwd, sizeof(m_ODBCPwd), inipath);

	m_nServerCount = GetPrivateProfileInt("SERVER_LIST", "COUNT", 0, inipath);

	if (!strlen(m_strFtpUrl) || !strlen(m_strFilePath)
		|| !strlen(m_ODBCName) || !strlen(m_ODBCLogin) || !strlen(m_ODBCPwd) 
		|| m_nServerCount <= 0) 
		return FALSE;
	
	char key[20]; 
	_SERVER_INFO* pInfo = NULL;
	
	m_ServerList.reserve(m_nServerCount);

	// TO-DO: Replace this nonsense with something a little more versatile
	for (int i=0; i < m_nServerCount; i++)
	{
		pInfo = new _SERVER_INFO;

		sprintf_s(key, sizeof(key), "SERVER_%02d", i);
		GetPrivateProfileString("SERVER_LIST", key, "", pInfo->strServerIP, sizeof(pInfo->strServerIP), inipath);

		sprintf_s(key, sizeof(key), "LANIP_%02d", i);
		GetPrivateProfileString("SERVER_LIST", key, "", pInfo->strLanIP, sizeof(pInfo->strLanIP), inipath);

		sprintf_s(key, sizeof(key), "NAME_%02d", i);
		GetPrivateProfileString("SERVER_LIST", key, "", pInfo->strServerName, sizeof(pInfo->strServerName), inipath);

		sprintf_s(key, sizeof(key), "ID_%02d", i);
		pInfo->sServerID = GetPrivateProfileInt("SERVER_LIST", key, 0, inipath);

		sprintf_s(key, sizeof(key), "GROUPID_%02d", i);
		pInfo->sGroupID = GetPrivateProfileInt("SERVER_LIST", key, 0, inipath);

		sprintf_s(key, sizeof(key), "PREMLIMIT_%02d", i);
		pInfo->sPlayerCap = GetPrivateProfileInt("SERVER_LIST", key, 0, inipath);

		sprintf_s(key, sizeof(key), "FREELIMIT_%02d", i);
		pInfo->sFreePlayerCap = GetPrivateProfileInt("SERVER_LIST", key, 0, inipath);

		sprintf_s(key, sizeof(key), "KING1_%02d", i);
		GetPrivateProfileString("SERVER_LIST", key, "", pInfo->strKarusKingName, sizeof(pInfo->strKarusKingName), inipath);

		sprintf_s(key, sizeof(key), "KING2_%02d", i);
		GetPrivateProfileString("SERVER_LIST", key, "", pInfo->strElMoradKingName, sizeof(pInfo->strElMoradKingName), inipath);

		sprintf_s(key, sizeof(key), "KINGMSG1_%02d", i);
		GetPrivateProfileString("SERVER_LIST", key, "", pInfo->strKarusNotice, sizeof(pInfo->strKarusNotice), inipath);

		sprintf_s(key, sizeof(key), "KINGMSG2_%02d", i);
		GetPrivateProfileString("SERVER_LIST", key, "", pInfo->strElMoradNotice, sizeof(pInfo->strElMoradNotice), inipath);

		m_ServerList.push_back(pInfo);
	}

	// Read news from INI (max 3 blocks)
	#define BOX_START '#' << BYTE(0) << '\n'
	#define LINE_ENDING BYTE(0) << '\n'
	#define BOX_END BOX_START << LINE_ENDING

	m_news.Size = 0;
	stringstream ss;
	for (int i = 0; i < 3; i++)
	{
		char tmp[128];
		string title, message;

		sprintf_s(key, sizeof(key), "TITLE_%02d", i);
		memset(tmp, 0x00, sizeof(tmp));
		GetPrivateProfileString("NEWS", key, "", tmp, sizeof(tmp), inipath);

		title = tmp;
		if (title.size() == 0)
			continue;
		
		sprintf_s(key, sizeof(key), "MESSAGE_%02d", i);
		memset(tmp, 0x00, sizeof(tmp));
		GetPrivateProfileString("NEWS", key, "", tmp, sizeof(tmp), inipath);

		message = tmp;
		if (message.size() == 0)
			continue;

		size_t oldPos = 0, pos = 0;
		ss << title << BOX_START;

		// potentially support multiline by making | act as linebreaks (same as the TBL afaik, so at least we're conformant).
		//replace(messages[i].begin(), messages[i].end(), '|', '\n');
		//while ((pos = message.find('\r', pos)) != string::npos)
		//	message.erase(pos, 1);
		//Remove \n for now, perhaps re-implement later
		//while ((pos = message.find('\n', pos)) != string::npos)
		//	message.erase(pos, 1);

		ss << message << LINE_ENDING << BOX_END;
	}

	m_news.Size = ss.str().size();
	if (m_news.Size)
		memcpy(&m_news.Content, ss.str().c_str(), m_news.Size);

	return TRUE;
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVersionManagerDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVersionManagerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

BOOL CVersionManagerDlg::PreTranslateMessage(MSG* pMsg) 
{
	if( pMsg->message == WM_KEYDOWN ) {
		if( pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE )
			return TRUE;
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CVersionManagerDlg::DestroyWindow() 
{
	if( !m_VersionList.IsEmpty() )
		m_VersionList.DeleteAllData();
	
	for( int i=0; i<m_ServerList.size(); i++)
		delete m_ServerList[i];
	m_ServerList.clear();

	return CDialog::DestroyWindow();
}

void CVersionManagerDlg::OnVersionSetting() 
{
	CString inipath;
	inipath.Format("%s\\Version.ini", GetProgPath());
}


void CVersionManagerDlg::OnBnClickedExit()
{
	CDialog::OnCancel();
}
