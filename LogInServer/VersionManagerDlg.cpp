#include "stdafx.h"
#include "VersionManagerDlg.h"

using namespace std;

KOSocketMgr<LoginSession> CVersionManagerDlg::s_socketMgr;
CVersionManagerDlg *g_pMain = NULL;

/////////////////////////////////////////////////////////////////////////////
// CVersionManagerDlg dialog

CVersionManagerDlg::CVersionManagerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVersionManagerDlg::IDD, pParent), m_Ini("Version.ini"), m_sLastVersion(__VERSION)
{
	//{{AFX_DATA_INIT(CVersionManagerDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	
	memset(m_strFtpUrl, 0, sizeof(m_strFtpUrl));
	memset(m_strFilePath, 0, sizeof(m_strFilePath));
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
	
	g_pMain = this;

	GetInfoFromIni();
	
	if (!m_DBProcess.Connect(m_ODBCName, m_ODBCLogin, m_ODBCPwd)) 
	{
		AfxMessageBox("Unable to connect to the database using the details configured.");
		AfxPostQuitMessage(0);
		return FALSE;
	}

	m_OutputList.AddString(_T("Connected to database server."));

	if (!m_DBProcess.LoadVersionList())
	{
		AfxMessageBox("Unable to load the version list.");
		AfxPostQuitMessage(0);
		return FALSE;
	}

	CString version;
	version.Format("Latest Version : %d", GetVersion());
	m_OutputList.AddString( version );

	InitPacketHandlers();

	if (!s_socketMgr.Listen(_LISTEN_PORT, MAX_USER))
	{
		AfxMessageBox("Failed to listen on server port.");
		AfxPostQuitMessage(0);
		return FALSE;
	}

	s_socketMgr.RunServer();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVersionManagerDlg::GetInfoFromIni()
{
	char tmp[128];

	m_Ini.GetString("DOWNLOAD", "URL", "ftp.yoursite.net", m_strFtpUrl, sizeof(m_strFtpUrl), false);
	m_Ini.GetString("DOWNLOAD", "PATH", "/", m_strFilePath, sizeof(m_strFilePath), false);

	m_Ini.GetString("ODBC", "DSN", "KN_online", m_ODBCName, sizeof(m_ODBCName), false);
	m_Ini.GetString("ODBC", "UID", "knight", m_ODBCLogin, sizeof(m_ODBCLogin), false);
	m_Ini.GetString("ODBC", "PWD", "knight", m_ODBCPwd, sizeof(m_ODBCPwd), false);

	int nServerCount = m_Ini.GetInt("SERVER_LIST", "COUNT", 1);
	if (nServerCount <= 0) 
		nServerCount = 1;
	
	char key[20]; 
	_SERVER_INFO* pInfo = NULL;
	
	m_ServerList.reserve(nServerCount);

	// TO-DO: Replace this nonsense with something a little more versatile
	for (int i = 0; i < nServerCount; i++)
	{
		pInfo = new _SERVER_INFO;

		sprintf_s(key, sizeof(key), "SERVER_%02d", i);
		m_Ini.GetString("SERVER_LIST", key, "127.0.0.1", pInfo->strServerIP, sizeof(pInfo->strServerIP), false);

		sprintf_s(key, sizeof(key), "LANIP_%02d", i);
		m_Ini.GetString("SERVER_LIST", key, "127.0.0.1", pInfo->strLanIP, sizeof(pInfo->strLanIP), false);

		sprintf_s(key, sizeof(key), "NAME_%02d", i);
		m_Ini.GetString("SERVER_LIST", key, "TEST|Server 1", pInfo->strServerName, sizeof(pInfo->strServerName), false);

		sprintf_s(key, sizeof(key), "ID_%02d", i);
		pInfo->sServerID = m_Ini.GetInt("SERVER_LIST", key, 1);

		sprintf_s(key, sizeof(key), "GROUPID_%02d", i);
		pInfo->sGroupID = m_Ini.GetInt("SERVER_LIST", key, 1);

		sprintf_s(key, sizeof(key), "PREMLIMIT_%02d", i);
		pInfo->sPlayerCap = m_Ini.GetInt("SERVER_LIST", key, MAX_USER);

		sprintf_s(key, sizeof(key), "FREELIMIT_%02d", i);
		pInfo->sFreePlayerCap = m_Ini.GetInt("SERVER_LIST", key, MAX_USER);

		sprintf_s(key, sizeof(key), "KING1_%02d", i);
		m_Ini.GetString("SERVER_LIST", key, "", pInfo->strKarusKingName, sizeof(pInfo->strKarusKingName));

		sprintf_s(key, sizeof(key), "KING2_%02d", i);
		m_Ini.GetString("SERVER_LIST", key, "", pInfo->strElMoradKingName, sizeof(pInfo->strElMoradKingName));

		sprintf_s(key, sizeof(key), "KINGMSG1_%02d", i);
		m_Ini.GetString("SERVER_LIST", key, "", pInfo->strKarusNotice, sizeof(pInfo->strKarusNotice));

		sprintf_s(key, sizeof(key), "KINGMSG2_%02d", i);
		m_Ini.GetString("SERVER_LIST", key, "", pInfo->strElMoradNotice, sizeof(pInfo->strElMoradNotice));

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
		string title, message;

		sprintf_s(key, sizeof(key), "TITLE_%02d", i);
		m_Ini.GetString("NEWS", key, "", tmp, sizeof(tmp));

		title = tmp;
		if (title.size() == 0)
			continue;
		
		sprintf_s(key, sizeof(key), "MESSAGE_%02d", i);
		m_Ini.GetString("NEWS", key, "", tmp, sizeof(tmp));

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
}

void CVersionManagerDlg::ReportSQLError(OdbcError *pError)
{
	if (pError == NULL)
		return;

	// This is *very* temporary.
	string errorMessage = string_format(_T("ODBC error occurred.\r\nSource: %s\r\nError: %s\r\nDescription: %s"),
		pError->Source.c_str(), pError->ExtendedErrorMessage.c_str(), pError->ErrorMessage.c_str());

	LogFileWrite(errorMessage.c_str());
	delete pError;
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
	if (pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE))
		return TRUE;
	
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CVersionManagerDlg::DestroyWindow() 
{
	foreach (itr, m_ServerList)
		delete *itr;
	m_ServerList.clear();

	foreach (itr, m_VersionList)
		delete itr->second;
	m_VersionList.clear();

	return CDialog::DestroyWindow();
}

void CVersionManagerDlg::OnBnClickedExit()
{
	CDialog::OnCancel();
}
