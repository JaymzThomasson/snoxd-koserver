// AujardDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Aujard.h"
#include "AujardDlg.h"
#include "../shared/Ini.h"
#include "ItemTableset.h"
#include <process.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define PROCESS_CHECK		100
#define CONCURRENT_CHECK	200
#define SERIAL_TIME			300

WORD	g_increase_serial = 50001;

CRITICAL_SECTION g_LogFileWrite;

DWORD WINAPI ReadQueueThread(LPVOID lp)
{
	CAujardDlg* pMain = (CAujardDlg*)lp;
	int recvlen = 0, index = 0;
	BYTE command;
	char recv_buff[1024];
	CString string;

	while(TRUE) {
		if( pMain->m_LoggerRecvQueue.GetFrontMode() != R ) {
			index = 0;
			recvlen = pMain->m_LoggerRecvQueue.GetData( recv_buff );
			if( recvlen > MAX_PKTSIZE ) {
				Sleep(1);
				continue;
			}

			command = GetByte( recv_buff, index );
			switch( command ) {
			case WIZ_LOGIN:
				pMain->AccountLogIn( recv_buff+index );
				break;
			case WIZ_NEW_CHAR:
				pMain->CreateNewChar( recv_buff+index );
				break;
			case WIZ_DEL_CHAR:
				pMain->DeleteChar( recv_buff+index );
				break;
			case WIZ_SEL_CHAR:
				pMain->SelectCharacter( recv_buff+index );
				break;
			case WIZ_SEL_NATION:
				pMain->SelectNation( recv_buff+index );
				break;
			case WIZ_ALLCHAR_INFO_REQ:
				pMain->AllCharInfoReq( recv_buff+index );
				break;
			case WIZ_LOGOUT:
				pMain->UserLogOut( recv_buff+index );				
				break;
			case WIZ_DATASAVE:
				pMain->UserDataSave( recv_buff+index );
				break;
			case WIZ_KNIGHTS_PROCESS:
				pMain->KnightsPacket( recv_buff+index );
				break;
			case WIZ_LOGIN_INFO:
				pMain->SetLogInInfo( recv_buff+index );
				break;
			case WIZ_KICKOUT:
				pMain->UserKickOut( recv_buff+index );
				break;
			case WIZ_BATTLE_EVENT:
				pMain->BattleEventResult( recv_buff+index );
				break;
			case WIZ_SHOPPING_MALL:
				pMain->ShoppingMall(recv_buff+index);
				break;
			case WIZ_SKILLDATA:
				pMain->SkillDataProcess(recv_buff+index);
			}

			recvlen = 0;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CAujardDlg dialog

CAujardDlg::CAujardDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAujardDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAujardDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	memset( m_strGameDSN, 0x00, 24 );
	memset( m_strGameUID, 0x00, 24 );
	memset( m_strGamePWD, 0x00, 24 );
	memset( m_strAccountDSN, 0x00, 24 );
	memset( m_strAccountUID, 0x00, 24 );
	memset( m_strAccountPWD, 0x00, 24 );
	memset( m_strLogDSN, 0x00, 24 );
	memset( m_strLogUID, 0x00, 24 );
	memset( m_strLogPWD, 0x00, 24 );
}

void CAujardDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAujardDlg)
	DDX_Control(pDX, IDC_OUT_LIST, m_OutputList);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAujardDlg, CDialog)
	//{{AFX_MSG_MAP(CAujardDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAujardDlg message handlers

BOOL CAujardDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//----------------------------------------------------------------------
	//	Logfile initialize
	//----------------------------------------------------------------------
	CTime time=CTime::GetCurrentTime();
	char strLogFile[50];
	sprintf_s(strLogFile, sizeof(strLogFile), "AujardLog-%d-%d-%d.txt", time.GetYear(), time.GetMonth(), time.GetDay());
	m_LogFile.Open( strLogFile, CFile::modeWrite | CFile::modeCreate | CFile::modeNoTruncate | CFile::shareDenyNone );
	m_LogFile.SeekToEnd();

	m_iLogFileDay = time.GetDay();

	InitializeCriticalSection( &g_LogFileWrite );
	
	if (!m_LoggerRecvQueue.InitailizeMMF( MAX_PKTSIZE, MAX_COUNT, SMQ_LOGGERSEND, FALSE)
		|| !m_LoggerSendQueue.InitailizeMMF( MAX_PKTSIZE, MAX_COUNT, SMQ_LOGGERRECV, FALSE ))
	{
		AfxMessageBox("Unable to initialize shared memory queue.");
		return FALSE;
	}

	if( !InitializeMMF() ) {
		AfxMessageBox("Unable to initialize primary shared memory.");
		AfxPostQuitMessage(0);
		return FALSE;
	}

	CIni ini("Aujard.ini");

	ini.GetString( "ODBC", "ACCOUNT_DSN", "KN_online", m_strAccountDSN, 24 );
	ini.GetString( "ODBC", "ACCOUNT_UID", "knight", m_strAccountUID, 24 );
	ini.GetString( "ODBC", "ACCOUNT_PWD", "knight", m_strAccountPWD, 24 );
	ini.GetString( "ODBC", "GAME_DSN", "KN_online", m_strGameDSN, 24 );
	ini.GetString( "ODBC", "GAME_UID", "knight", m_strGameUID, 24 );
	ini.GetString( "ODBC", "GAME_PWD", "knight", m_strGamePWD, 24 );
	ini.GetString( "ODBC", "LOG_DSN", "KN_online", m_strLogDSN, 24 );
	ini.GetString( "ODBC", "LOG_UID", "knight", m_strLogUID, 24 );
	ini.GetString( "ODBC", "LOG_PWD", "knight", m_strLogPWD, 24 );

	m_nServerNo = ini.GetInt("ZONE_INFO", "GROUP_INFO", 1);
	m_nZoneNo = ini.GetInt("ZONE_INFO", "ZONE_INFO", 1);

	if( !m_DBAgent.DatabaseInit() ) {
		AfxPostQuitMessage(0);
		return FALSE;
	}

	if( !LoadItemTable() ) {
		AfxMessageBox("Load ItemTable Fail!!");
		AfxPostQuitMessage(0);
		return FALSE;
	}

	SetTimer( PROCESS_CHECK, 40000, NULL );
	SetTimer( CONCURRENT_CHECK, 300000, NULL );
//	SetTimer( SERIAL_TIME, 60000, NULL );

	DWORD id;
	m_hReadQueueThread = ::CreateThread( NULL, 0, ReadQueueThread, (LPVOID)this, 0, &id);

	CTime cur = CTime::GetCurrentTime();
	CString starttime;
	starttime.Format("Aujard Start : %02d/%02d/%04d %d:%02d\r\n", cur.GetDay(), cur.GetMonth(), cur.GetYear(), cur.GetHour(), cur.GetMinute());
	m_LogFile.Write(starttime, starttime.GetLength());

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CAujardDlg::OnPaint() 
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
HCURSOR CAujardDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

BOOL CAujardDlg::DestroyWindow() 
{
	KillTimer( PROCESS_CHECK );
	KillTimer( CONCURRENT_CHECK );
//	KillTimer( SERIAL_TIME );

	if( m_hReadQueueThread ) {
		::TerminateThread( m_hReadQueueThread, 0 );
	}
	
	if (m_LogFile.m_hFile != CFile::hFileNull) m_LogFile.Close();
	DeleteCriticalSection( &g_LogFileWrite );

	return CDialog::DestroyWindow();
}

BOOL CAujardDlg::InitializeMMF()
{
	DWORD filesize = MAX_USER * sizeof(_USER_DATA);	// 1명당 4000 bytes 이내 소요
	
	m_hMMFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, TRUE, "KNIGHT_DB" );
	if (m_hMMFile == NULL)
	{
		m_OutputList.AddString("Shared Memory Load Fail!!");
		m_hMMFile = INVALID_HANDLE_VALUE; 
		return FALSE;
	}
	m_OutputList.AddString("Shared Memory Load Success!!");

    m_lpMMFile = (char *)MapViewOfFile (m_hMMFile, FILE_MAP_WRITE, 0, 0, 0);
	if (!m_lpMMFile)
		return FALSE;

	m_DBAgent.m_UserDataArray.reserve( MAX_USER );

	for (int i = 0; i < MAX_USER; i++)
		m_DBAgent.m_UserDataArray.push_back((_USER_DATA*)(m_lpMMFile + i * sizeof(_USER_DATA)));

	return TRUE;
}

BOOL CAujardDlg::LoadItemTable()
{
	CItemTableSet ItemTableSet(&m_ItemtableArray);
	return ItemTableSet.Read();
}

void CAujardDlg::SelectCharacter(char *pBuf)
{
	int index = 0, uid = -1, send_index = 0, t_uid = -1, count = 0, packetindex = 0;
	BYTE bInit = 0x01;
	char send_buff[256], accountid[MAX_ID_SIZE+1], userid[MAX_ID_SIZE+1];
	_USER_DATA* pUser = NULL;

	uid = GetShort( pBuf, index );
	if (uid < 0 || uid >= MAX_USER
		|| !GetKOString(pBuf, accountid, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, userid, index, MAX_ID_SIZE)
		|| *accountid == 0 || *userid == 0)
		goto fail_return;

	bInit = GetByte( pBuf, index );

	char logstr[256]; 
	sprintf_s( logstr, sizeof(logstr), "SelectCharacter : acname=%s, name=%s, index=%d, pid : %d, front : %d\r\n", accountid, userid, packetindex, _getpid(), m_LoggerRecvQueue.GetFrontPointer() );
	WriteLogFile( logstr );
	//m_LogFile.Write(logstr, strlen(logstr));

	if( GetUserPtr( userid, t_uid ) ) {
		SetShort( send_buff, t_uid, send_index );
		SetKOString( send_buff, accountid, send_index );
		SetKOString( send_buff, userid, send_index );
		UserLogOut( send_buff );
		return;
	}
	if( !m_DBAgent.LoadUserData( accountid, userid, uid ) )
		goto fail_return;
	if( !m_DBAgent.LoadWarehouseData( accountid, uid ) )
		goto fail_return;

	pUser = (_USER_DATA*)m_DBAgent.m_UserDataArray[uid];
	if( !pUser ) goto fail_return;

	strcpy_s( pUser->m_Accountid, sizeof(pUser->m_Accountid), accountid );

	SetByte( send_buff, WIZ_SEL_CHAR, send_index );
	SetShort( send_buff, uid, send_index );
	SetByte( send_buff, 0x01, send_index );
	SetByte( send_buff, bInit, send_index );

	do 
	{
		if (m_LoggerSendQueue.PutData(send_buff, send_index) == 1)
			break;
		else
			count++;
	} while (count < 50);
	if( count >= 50 )
		m_OutputList.AddString("Sel char Packet Drop!!!");
	return;

fail_return:
	send_index = 0;
	SetByte( send_buff, WIZ_SEL_CHAR, send_index );
	SetShort( send_buff, uid, send_index );
	SetByte( send_buff, 0x00, send_index );
	
	m_LoggerSendQueue.PutData( send_buff, send_index );
}

void CAujardDlg::ShoppingMall(char *pBuf)
{
	int index = 0;
	BYTE opcode = GetByte(pBuf, index);
	switch (opcode)
	{
	case STORE_CLOSE:
		LoadWebItemMall(pBuf+index);
		break;
	}
}

void CAujardDlg::LoadWebItemMall(char *pBuf)
{
	char send_buff[1024];
	int index = 0, send_index = 0, uid = -1;

	uid = GetShort(pBuf, index);
	if (uid < 0 || uid >= MAX_USER)
		return;

	SetByte(send_buff, WIZ_SHOPPING_MALL, send_index);
	SetShort(send_buff, uid, send_index);
	SetByte(send_buff, STORE_CLOSE, send_index);
	SetByte(send_buff, 0, send_index); // result

	if (m_DBAgent.LoadWebItemMall(m_DBAgent.m_UserDataArray[uid]->m_id, send_buff, send_index))
		send_buff[4] = 1;

	m_LoggerSendQueue.PutData(send_buff, send_index);
}

void CAujardDlg::SkillDataProcess(char *pBuf)
{
	int index = 0, uid = 0;
	BYTE opcode = GetByte(pBuf, index);
	uid = GetShort(pBuf, index);
	if (uid < 0 || uid >= MAX_USER)
		return;

	if (opcode == SKILL_DATA_LOAD)
		SkillDataLoad(pBuf+index, uid);
	else if (opcode == SKILL_DATA_SAVE)
		SkillDataSave(pBuf+index, uid);
}

void CAujardDlg::SkillDataLoad(char *pBuf, int uid)
{
	char send_buff[512];
	int send_index = 0;

	SetByte(send_buff, WIZ_SKILLDATA, send_index);
	SetByte(send_buff, uid, send_index);
	SetByte(send_buff, SKILL_DATA_LOAD, send_index);

	if (!m_DBAgent.LoadSkillShortcut(m_DBAgent.m_UserDataArray[uid]->m_id, send_buff, send_index))
		SetByte(send_buff, 0, send_index);

	m_LoggerSendQueue.PutData(send_buff, send_index);
}

void CAujardDlg::SkillDataSave(char *pBuf, int uid)
{
	int index = 0, sCount = GetShort(pBuf, index);
	m_DBAgent.SaveSkillShortcut(m_DBAgent.m_UserDataArray[uid]->m_id, sCount, pBuf);
	// this doesn't send a response AFAIK
}

void CAujardDlg::FriendProcess(char *pBuf)
{
	int index = 0;
	BYTE opcode = GetByte(pBuf, index);

	switch (opcode)
	{
	case FRIEND_REQUEST:
		RequestFriendList(pBuf+index);
		break;

	case FRIEND_ADD:
		AddFriend(pBuf+index);
		break;

	case FRIEND_REMOVE:
		RemoveFriend(pBuf+index);
		break;
	}
}

void CAujardDlg::RequestFriendList(char *pBuf)
{
	char send_buff[MAX_ID_SIZE * MAX_FRIEND_COUNT + 2 + 2 + 1];
	vector<string> friendList;
	int index = 0, send_index = 0;
	short sid = GetShort(pBuf, index);

	m_DBAgent.RequestFriendList(sid, friendList);

	SetByte(send_buff, WIZ_FRIEND_PROCESS, send_index);
	SetShort(send_buff, sid, send_index);
	SetByte(send_buff, FRIEND_REQUEST, send_index);
	SetByte(send_buff, (BYTE)(friendList.size()), send_index);

	foreach (itr, friendList)
		SetKOString(send_buff, (char *)(*itr).c_str(), send_index, sizeof(BYTE));
}

void CAujardDlg::AddFriend(char *pBuf)
{
	char send_buff[8+MAX_ID_SIZE], charName[MAX_ID_SIZE+1];
	int index = 0, send_index = 0;
	short sid = GetShort(pBuf, index), // player adding a friend
		  tid = GetShort(pBuf, index); // friend being added (HAS to be online to add - no other way.)

	// Safety checks! Don't leave home without them.
	if (sid < 0 || sid >= MAX_USER
		|| tid < 0 || tid >= MAX_USER
		|| !GetKOString(pBuf, charName, index, MAX_ID_SIZE, sizeof(BYTE)))
		return;

	FriendAddResult result = m_DBAgent.AddFriend(sid, tid);
	SetByte(send_buff, WIZ_FRIEND_PROCESS, send_index);
	SetShort(send_buff, sid, send_index);
	SetByte(send_buff, FRIEND_ADD, send_index);
	SetShort(send_buff, tid, send_index);
	SetByte(send_buff, (BYTE)result, send_index);
	SetKOString(send_buff, charName, send_index, sizeof(BYTE));
	m_LoggerSendQueue.PutData(send_buff, send_index);
}

void CAujardDlg::RemoveFriend(char *pBuf)
{
	int index = 0, send_index = 0;
	char send_buff[6+MAX_ID_SIZE], charName[MAX_ID_SIZE+1];
	short sid = GetShort(pBuf, index); // player removing a friend

	// Safety checks! Don't leave home without them.
	if (sid < 0 || sid >= MAX_USER
		|| !GetKOString(pBuf, charName, index, MAX_ID_SIZE, sizeof(BYTE)))
		return;

	FriendRemoveResult result = m_DBAgent.RemoveFriend(sid, charName);
	SetByte(send_buff, WIZ_FRIEND_PROCESS, send_index);
	SetShort(send_buff, sid, send_index);
	SetByte(send_buff, FRIEND_REMOVE, send_index);
	SetByte(send_buff, (BYTE)result, send_index);
	SetKOString(send_buff, charName, send_index, sizeof(BYTE));
	m_LoggerSendQueue.PutData(send_buff, send_index);
}

void CAujardDlg::UserLogOut(char *pBuf)
{
	int index = 0, uid = -1, send_index = 0, count = 0;
	int retval_1 = 0, retval_2 = 0, retval_3 = 1;
	char userid[MAX_ID_SIZE+1], accountid[MAX_ID_SIZE+1], send_buff[256], logstr[256];
	_USER_DATA* pUser = NULL;

	uid = GetShort( pBuf, index );
	if (uid < 0 || uid >= MAX_USER
		|| !GetKOString(pBuf, accountid, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, userid, index, MAX_ID_SIZE))
		return;

	pUser = (_USER_DATA*)m_DBAgent.m_UserDataArray[uid];
	if( !pUser ) return;
	
	retval_2 = m_DBAgent.UpdateUser( userid, uid, UPDATE_LOGOUT );
	Sleep(10);
	retval_1 = m_DBAgent.UpdateWarehouseData( accountid, uid, UPDATE_LOGOUT );
	
	if( pUser->m_bLogout != 2 )	// zone change logout
		retval_3 = m_DBAgent.AccountLogout( accountid );
	
	CTime t = CTime::GetCurrentTime();
	sprintf_s( logstr, sizeof(logstr), "Logout : %s, %s (W:%d,U:%d)\r\n", accountid, userid, retval_1, retval_2 );
	WriteLogFile( logstr );
	//m_LogFile.Write(logstr, strlen(logstr));
	

	if( retval_2 == 0 || retval_3 != 1 ) {
		sprintf_s( logstr, sizeof(logstr), "Invalid Logout : %s, %s (W:%d,U:%d,A:%d) \r\n", accountid, userid, retval_1, retval_2, retval_3 );
		WriteLogFile( logstr );
		//m_LogFile.Write(logstr, strlen(logstr));
	}

	if( retval_1 == FALSE || retval_2 == FALSE )
	do {
		retval_2 = m_DBAgent.UpdateUser( userid, uid, UPDATE_LOGOUT );
		Sleep(10);
		retval_1 = m_DBAgent.UpdateWarehouseData( accountid, uid, UPDATE_LOGOUT );

		if( retval_1 == TRUE && retval_2 == TRUE )
			break;
		count++;
	} while( count < 10 );
	if( count >= 10 ) {
		sprintf_s( logstr, sizeof(logstr), "Logout Save Error: %s, %s (W:%d,U:%d) \r\n", accountid, userid, retval_1, retval_2 );
		WriteLogFile( logstr );
		//m_LogFile.Write(logstr, strlen(logstr));
	}

	count = 0;

	do {
		if( retval_1 = m_DBAgent.CheckUserData( accountid, userid, 1, pUser->m_dwTime, pUser->m_iBank) )
			break;
		m_DBAgent.UpdateWarehouseData( accountid, uid, UPDATE_LOGOUT );	// retry
		count++;
		Sleep(10);
	} while( count < 10 );
	if( count >= 10 ) {
		sprintf_s( logstr, sizeof(logstr), "WarehouseData Logout Check Error: %s, %s (W:%d) \r\n", accountid, userid, retval_1);
		WriteLogFile( logstr );
		//m_LogFile.Write(logstr, strlen(logstr));
	}
	
	count = 0;

	do {
		if( retval_1 = m_DBAgent.CheckUserData( accountid, userid, 2, pUser->m_dwTime, pUser->m_iExp) )
			break;
		m_DBAgent.UpdateUser( userid, uid, UPDATE_LOGOUT );	// retry
		count++;
		Sleep(10);
	} while( count < 10 );
	if( count >= 10 ) {
		sprintf_s( logstr, sizeof(logstr), "UserDataLogout Check Error: %s, %s (W:%d) \r\n", accountid, userid, retval_1);
		WriteLogFile( logstr );
		//m_LogFile.Write(logstr, strlen(logstr));
	}

	m_DBAgent.MUserInit( uid );

	SetByte( send_buff, WIZ_LOGOUT, send_index );
	SetShort( send_buff, uid, send_index );

	count = 0;
	do {
		if( m_LoggerSendQueue.PutData( send_buff, send_index ) == 1 )
			break;
		else
			count++;
	} while( count < 50 );
	if( count >= 50 )
		m_OutputList.AddString("Logout Packet Drop!!!");
}

void CAujardDlg::AccountLogIn(char *pBuf)
{
	int index = 0, uid = -1, send_index = 0, count = 0, nation = -1;
	char accountid[MAX_ID_SIZE+1], password[MAX_PW_SIZE + 1], send_buff[256];

	uid = GetShort( pBuf, index );
	if (!GetKOString(pBuf, accountid, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, password, index, MAX_PW_SIZE)) 
		return;

	nation = m_DBAgent.AccountLogInReq( accountid, password);

	SetByte( send_buff, WIZ_LOGIN, send_index );
	SetShort( send_buff, uid, send_index );
	SetByte( send_buff, nation, send_index );

	do {
		if( m_LoggerSendQueue.PutData( send_buff, send_index ) == 1 )
			break;
		else
			count++;
	} while( count < 50 );
	if( count >= 50 )
		m_OutputList.AddString("Login Packet Drop!!!");
}

void CAujardDlg::SelectNation(char *pBuf)
{
	int index = 0, uid = -1, send_index = 0, count = 0, nation = -1;
	BYTE result;
	char accountid[MAX_ID_SIZE+1], send_buff[256];

	uid = GetShort( pBuf, index );
	if (!GetKOString(pBuf, accountid, index, MAX_ID_SIZE))
		return;
	nation = GetByte( pBuf, index );

	result = m_DBAgent.NationSelect( accountid, nation );

	SetByte( send_buff, WIZ_SEL_NATION, send_index );
	SetShort( send_buff, uid, send_index );
	if( result ) 
		SetByte( send_buff, nation, send_index );
	else
		SetByte( send_buff, 0x00, send_index );

	do {
		if( m_LoggerSendQueue.PutData( send_buff, send_index ) == 1 )
			break;
		else
			count++;
	} while( count < 50 );
	if( count >= 50 )
		m_OutputList.AddString("Sel Nation Packet Drop!!!");
}

void CAujardDlg::CreateNewChar(char *pBuf)
{
	int index = 0, uid = -1, send_index = 0, result = 0, count = 0;
	int charindex = 0, race = 0, Class = 0, hair = 0, face = 0, str = 0, sta = 0, dex = 0, intel = 0, cha = 0;
	char accountid[MAX_ID_SIZE+1], charid[MAX_ID_SIZE+1], send_buff[256];

	uid = GetShort( pBuf, index );
	if (!GetKOString(pBuf, accountid, index, MAX_ID_SIZE))
		return;
	charindex = GetByte( pBuf, index );
	if (!GetKOString(pBuf, charid, index, MAX_ID_SIZE))
		return;
	race = GetByte( pBuf, index );
	Class = GetShort( pBuf, index );
	face = GetByte( pBuf, index );
	hair = GetDWORD( pBuf, index );
	str = GetByte( pBuf, index );
	sta = GetByte( pBuf, index );
	dex = GetByte( pBuf, index );
	intel = GetByte( pBuf, index );
	cha = GetByte( pBuf, index );

	result = m_DBAgent.CreateNewChar( accountid, charindex, charid, race, Class, hair, face, str, sta, dex, intel, cha );

	SetByte( send_buff, WIZ_NEW_CHAR, send_index );
	SetShort( send_buff, uid, send_index );
	SetByte( send_buff, result, send_index );

	do {
		if( m_LoggerSendQueue.PutData( send_buff, send_index ) == 1 )
			break;
		else
			count++;
	} while( count < 50 );
	if( count >= 50 )
		m_OutputList.AddString("New Char Packet Drop!!!");
}

void CAujardDlg::DeleteChar(char *pBuf)
{
	int index = 0, uid = -1, send_index = 0, result = 0, count = 0;
	int charindex = 0, soclen = 0;
	char accountid[MAX_ID_SIZE+1], charid[MAX_ID_SIZE+1], socno[15], send_buff[256];

	uid = GetShort( pBuf, index );
	if (!GetKOString(pBuf, accountid, index, MAX_ID_SIZE))
		return;
	charindex = GetByte( pBuf, index );
	if (!GetKOString(pBuf, charid, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, socno, index, sizeof(socno) - 1))
		return;

	result = m_DBAgent.DeleteChar( charindex, accountid, charid, socno );

	TRACE("*** DeleteChar == charid=%s, socno=%s ***\n", charid, socno);

	SetByte( send_buff, WIZ_DEL_CHAR, send_index );
	SetShort( send_buff, uid, send_index );
	SetByte( send_buff, result, send_index );
	if( result > 0 )
		SetByte( send_buff, charindex, send_index );
	else
		SetByte( send_buff, 0xFF, send_index );

	do {
		if( m_LoggerSendQueue.PutData( send_buff, send_index ) == 1 )
			break;
		else
			count++;
	} while( count < 50 );
	if( count >= 50 )
		m_OutputList.AddString("Del Char Packet Drop!!!");
}

void CAujardDlg::AllCharInfoReq(char *pBuf)
{
	int index = 0, uid = 0, send_index = 0, buff_index = 0, count = 0;
	char accountid[MAX_ID_SIZE+1], send_buff[1024], buff[1024], charid1[MAX_ID_SIZE+1], charid2[MAX_ID_SIZE+1], charid3[MAX_ID_SIZE+1];

	uid = GetShort( pBuf, index );
	if (!GetKOString(pBuf, accountid, index, MAX_ID_SIZE))
		return;

	SetByte( buff, 0x01, buff_index );	// result

	m_DBAgent.GetAllCharID( accountid, charid1, charid2, charid3 );
	m_DBAgent.LoadCharInfo( charid1, buff, buff_index );
	m_DBAgent.LoadCharInfo( charid2, buff, buff_index );
	m_DBAgent.LoadCharInfo( charid3, buff, buff_index );

	SetByte( send_buff, WIZ_ALLCHAR_INFO_REQ, send_index );
	SetShort( send_buff, uid, send_index );
	SetShort( send_buff, buff_index, send_index );
	SetString( send_buff, buff, buff_index, send_index );

	do {
		if( m_LoggerSendQueue.PutData( send_buff, send_index ) == 1 )
			break;
		else
			count++;
	} while( count < 50 );
	if( count >= 50 )
		m_OutputList.AddString("All Char Packet Drop!!!");
}

BOOL CAujardDlg::PreTranslateMessage(MSG* pMsg) 
{
	if( pMsg->message == WM_KEYDOWN ) {
		if( pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE )
			return TRUE;
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

void CAujardDlg::OnOK() 
{
	if( AfxMessageBox("진짜 끝낼까요?", MB_YESNO) == IDYES )
		CDialog::OnOK();
}

void CAujardDlg::OnTimer(UINT nIDEvent) 
{
	HANDLE	hProcess = NULL;

	switch( nIDEvent ) {
	case PROCESS_CHECK:
		hProcess = OpenProcess( PROCESS_ALL_ACCESS | PROCESS_VM_READ, FALSE, m_LoggerSendQueue.GetProcessId() );
		if( hProcess == NULL )
			AllSaveRoutine();
		break;
	case CONCURRENT_CHECK:
#ifdef __SAMMA
		ConCurrentUserCount();
#endif
		break;
	case SERIAL_TIME:
		g_increase_serial = 50001;
		break;
	}
	
	CDialog::OnTimer(nIDEvent);
}

void CAujardDlg::AllSaveRoutine()
{
	_USER_DATA* pUser = NULL;
	int t_count = 0, retval_1 = 0, retval_2 = 0, count = 0;
	bool bsaved = false;
	CString msgstr;
	char logstr[256]; 

	CTime cur = CTime::GetCurrentTime();
	DEBUG_LOG("Server down : %02d/%02d/%04d %02d:%02d", cur.GetYear(), cur.GetMonth(), cur.GetDay(), cur.GetHour(), cur.GetMinute());

	t_count = m_DBAgent.m_UserDataArray.size();
	for( int i=0; i<t_count; i++ ) {
		pUser = (_USER_DATA*)m_DBAgent.m_UserDataArray[i];
		if( !pUser )
			continue;
		if( strlen(pUser->m_id) == 0 )
			continue;

		m_DBAgent.AccountLogout( pUser->m_Accountid );
		m_DBAgent.UpdateWarehouseData( pUser->m_Accountid, i, UPDATE_ALL_SAVE );
		Sleep(100);
		if( m_DBAgent.UpdateUser( pUser->m_id, i, UPDATE_ALL_SAVE ) ) {
			TRACE("GameServer Dead!! - %s Saved\n", pUser->m_id );
			count = 0;
			do {
				if( retval_1 = m_DBAgent.CheckUserData( pUser->m_Accountid, pUser->m_id, 1, pUser->m_dwTime, pUser->m_iBank) )
					break;
				m_DBAgent.UpdateWarehouseData( pUser->m_Accountid, i, UPDATE_ALL_SAVE ); // retry
				count++;
				Sleep(10);
			} while( count < 10 );
			if( count >= 10 ) {
				sprintf_s( logstr, sizeof(logstr), "WarehouseData All Save Check Error: %s, %s (W:%d) \r\n", pUser->m_Accountid, pUser->m_id, retval_1 );
				WriteLogFile( logstr );
				//m_LogFile.Write(logstr, strlen(logstr));
				TRACE(logstr);
			}
			count = 0;
			do {
				if( retval_1 = m_DBAgent.CheckUserData( pUser->m_Accountid, pUser->m_id, 2, pUser->m_dwTime, pUser->m_iExp) )
					break;
				m_DBAgent.UpdateUser( pUser->m_id, i, UPDATE_ALL_SAVE );	// retry
				count++;
				Sleep(10);
			} while( count < 10 );
			if( count >= 10 ) {
				sprintf_s( logstr, sizeof(logstr), "UserDataSave All Save Check Error: %s, %s (W:%d,U:%d) \r\n", pUser->m_Accountid, pUser->m_id, retval_1, retval_2 );
				WriteLogFile( logstr );
				//m_LogFile.Write(logstr, strlen(logstr));
				TRACE(logstr);
			}
		
			m_DBAgent.MUserInit(i);
			bsaved = true;
		}
		else
			TRACE("GameServer Dead!! - %s Not Saved\n", pUser->m_id );

		Sleep(100);
	}
	if( bsaved ) {
		msgstr.Format("All Data Saved:%d년%d월%d일%d시%d분", cur.GetYear(), cur.GetMonth(), cur.GetDay(), cur.GetHour(), cur.GetMinute());
		m_OutputList.AddString(msgstr);
	}
}

void CAujardDlg::ConCurrentUserCount()
{
	int t_count = 0;
	_USER_DATA* pUser = NULL;

	for( int i=0; i<MAX_USER; i++ ) {
		pUser = (_USER_DATA*)m_DBAgent.m_UserDataArray[i];
		if( !pUser )
			continue;
		if( strlen(pUser->m_id) == 0 )
			continue;
		
		t_count++;
	}

	TRACE("*** ConCurrentUserCount : server=%d, zone=%d, usercount=%d ***\n", m_nServerNo, m_nZoneNo, t_count);

	m_DBAgent.UpdateConCurrentUserCount( m_nServerNo, m_nZoneNo, t_count );
}

void CAujardDlg::UserDataSave(char *pBuf)
{
	int index = 0, uid = -1, retval_1 = 0, retval_2 = 0, count = 0;
	_USER_DATA* pUser = NULL;
	char logstr[256], accountid[MAX_ID_SIZE+1], userid[MAX_ID_SIZE+1];	

	uid = GetShort( pBuf, index );
	if (uid < 0 || uid >= MAX_USER
		|| !GetKOString(pBuf, accountid, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, userid, index, MAX_ID_SIZE)
		|| (pUser = (_USER_DATA*)m_DBAgent.m_UserDataArray[uid]) == NULL)
		return;

	retval_2 = m_DBAgent.UpdateUser( userid, uid, UPDATE_PACKET_SAVE );
	Sleep(10);
	retval_1 = m_DBAgent.UpdateWarehouseData( accountid, uid, UPDATE_PACKET_SAVE );

	if( retval_1 == FALSE || retval_2 == FALSE )
	do {
		retval_2 = m_DBAgent.UpdateUser( userid, uid, UPDATE_PACKET_SAVE );
		Sleep(10);
		retval_1 = m_DBAgent.UpdateWarehouseData( accountid, uid, UPDATE_PACKET_SAVE );

		if( retval_1 == TRUE && retval_2 == TRUE )
			break;
		count++;
	} while( count < 10 );
	if( count >= 10 ) {
		sprintf_s( logstr, sizeof(logstr), "UserData Save Error: %s, %s (W:%d,U:%d) \r\n", accountid, userid, retval_1, retval_2 );
		WriteLogFile( logstr );
		//m_LogFile.Write(logstr, strlen(logstr));
	}

	count = 0;

	do {
		if( retval_1 = m_DBAgent.CheckUserData( accountid, userid, 1, pUser->m_dwTime, pUser->m_iBank) )
			break;
		Sleep(10);
		count++;
	} while( count < 10 );
	if( count >= 10 ) {
		sprintf_s( logstr, sizeof(logstr), "WarehouseDataSave Check Error: %s, %s (W:%d) \r\n", accountid, userid, retval_1 );
		WriteLogFile( logstr );
		//m_LogFile.Write(logstr, strlen(logstr));
	}
	
	count = 0;

	do {
		if( retval_1 = m_DBAgent.CheckUserData( accountid, userid, 2, pUser->m_dwTime, pUser->m_iExp) )
			break;
		Sleep(10);
		count++;
	} while( count < 10 );
	if( count >= 10 ) {
		sprintf_s( logstr, sizeof(logstr), "UserDataSave Check Error: %s, %s (W:%d) \r\n", accountid, userid, retval_1 );
		WriteLogFile( logstr );
		//m_LogFile.Write(logstr, strlen(logstr));
	}

//	sprintf( logstr, "UserDataSave Packet Receive: %s, %s (W:%d,U:%d) \r\n", accountid, userid, retval_1, retval_2 );
//	m_LogFile.Write(logstr, strlen(logstr));
//	TRACE(logstr);

	return;
}

_USER_DATA* CAujardDlg::GetUserPtr(const char *struserid, int &uid)
{
	_USER_DATA* pUser = NULL;
	
	for( int i=0; i<MAX_USER; i++ ) {
		pUser = (_USER_DATA*)m_DBAgent.m_UserDataArray[i];
		if( !pUser )
			continue;
		if( _strnicmp( struserid, pUser->m_id, MAX_ID_SIZE ) == 0 ) {
			uid = i;
			return pUser;
		}
	}

	return NULL;
}

void CAujardDlg::KnightsPacket(char *pBuf)
{
	int index = 0, nation = 0;
	BYTE command = GetByte( pBuf, index );

	switch( command ) {
	case KNIGHTS_CREATE:
		CreateKnights( pBuf+index );
		break;
	case KNIGHTS_JOIN:
		JoinKnights( pBuf+index );
		break;
	case KNIGHTS_WITHDRAW:
		WithdrawKnights( pBuf+index );
		break;
	case KNIGHTS_REMOVE:
	case KNIGHTS_ADMIT:
	case KNIGHTS_REJECT:
	case KNIGHTS_CHIEF:
	case KNIGHTS_VICECHIEF:
	case KNIGHTS_OFFICER:
	case KNIGHTS_PUNISH:
		ModifyKnightsMember( pBuf+index, command );
		break;
	case KNIGHTS_DESTROY:
		DestroyKnights( pBuf+index );
		break;
	case KNIGHTS_MEMBER_REQ:
		AllKnightsMember( pBuf+index );
		break;
	case KNIGHTS_STASH:
		break;
	case KNIGHTS_LIST_REQ:
		KnightsList( pBuf+index );
		break;
	case KNIGHTS_ALLLIST_REQ:
		nation = GetByte( pBuf, index );
		m_DBAgent.LoadKnightsAllList( nation );
		break;
	}
}

void CAujardDlg::CreateKnights(char *pBuf)
{
	int index = 0, send_index = 0, knightindex = 0, uid = -1, nation = 0, result = 0, community = 0, count = 0;
	char knightsname[MAX_ID_SIZE+1], chiefname[MAX_ID_SIZE+1], send_buff[256];

	uid = GetShort( pBuf, index );
	community = GetByte( pBuf, index );
	knightindex = GetShort( pBuf, index );
	nation = GetByte( pBuf, index );
	if (!GetKOString(pBuf, knightsname, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, chiefname, index, MAX_ID_SIZE))
		return;

	if( uid < 0 || uid >= MAX_USER ) return;
	result = m_DBAgent.CreateKnights( knightindex, nation, knightsname, chiefname, community );

	TRACE("CreateKnights - nid=%d, index=%d, result=%d \n", uid, knightindex, result);
	
	SetByte( send_buff, KNIGHTS_CREATE, send_index );
	SetShort( send_buff, uid, send_index );
	SetByte( send_buff, result, send_index );
	SetByte( send_buff, community, send_index );
	SetShort( send_buff, knightindex, send_index );
	SetByte( send_buff, nation, send_index );

	SetKOString( send_buff, knightsname, send_index );
	SetKOString( send_buff, chiefname, send_index );

	do {
		if( m_LoggerSendQueue.PutData( send_buff, send_index ) == 1 )
			break;
		else
			count++;
	} while( count < 50 );
	if( count >= 50 )
		m_OutputList.AddString("Create Knight Packet Drop!!!");
}

void CAujardDlg::JoinKnights(char *pBuf)
{
	int index = 0, send_index = 0, knightindex = 0, uid = -1, count = 0;
	BYTE result = 0;
	char send_buff[256];
	_USER_DATA* pUser = NULL;

	uid = GetShort( pBuf, index );
	knightindex = GetShort( pBuf, index );

	if( uid < 0 || uid >= MAX_USER ) return;
	pUser = (_USER_DATA*)m_DBAgent.m_UserDataArray[uid];
	if( !pUser )	return;

	result = m_DBAgent.UpdateKnights( KNIGHTS_JOIN, pUser->m_id, knightindex, 0 );
	
	TRACE("JoinKnights - nid=%d, name=%s, index=%d, result=%d \n", uid, pUser->m_id, knightindex, result);

	SetByte( send_buff, KNIGHTS_JOIN, send_index );
	SetShort( send_buff, uid, send_index );
	SetByte( send_buff, result, send_index );
	SetShort( send_buff, knightindex, send_index );

	do {
		if( m_LoggerSendQueue.PutData( send_buff, send_index ) == 1 )
			break;
		else
			count++;
	} while( count < 50 );
	if( count >= 50 )
		m_OutputList.AddString("Join Packet Drop!!!");
}

void CAujardDlg::WithdrawKnights(char *pBuf)
{
	int index = 0, send_index = 0, knightindex = 0, uid = -1, count = 0;
	BYTE result = 0;
	char send_buff[256];
	_USER_DATA* pUser = NULL;

	uid = GetShort( pBuf, index );
	knightindex = GetShort( pBuf, index );

	if( uid < 0 || uid >= MAX_USER ) return;
	pUser = (_USER_DATA*)m_DBAgent.m_UserDataArray[uid];

	result = m_DBAgent.UpdateKnights( KNIGHTS_WITHDRAW, pUser->m_id, knightindex, 0 );
	TRACE("WithDrawKnights - nid=%d, index=%d, result=%d \n", uid, knightindex, result);
	
	SetByte( send_buff, KNIGHTS_WITHDRAW, send_index );
	SetShort( send_buff, uid, send_index );
	SetByte( send_buff, result, send_index );
	SetShort( send_buff, knightindex, send_index );

	do {
		if( m_LoggerSendQueue.PutData( send_buff, send_index ) == 1 )
			break;
		else
			count++;
	} while( count < 50 );
	if( count >= 50 )
		m_OutputList.AddString("Withdraw Packet Drop!!!");
}

void CAujardDlg::ModifyKnightsMember(char *pBuf, BYTE command)
{
	int index = 0, send_index = 0, knightindex = 0, uid = -1, vicechief = 0, remove_flag = 0, count = 0;
	BYTE result = 0;
	char send_buff[256], userid[MAX_ID_SIZE+1];

	uid = GetShort( pBuf, index );
	knightindex = GetShort( pBuf, index );

	if (!GetKOString(pBuf, userid, index, MAX_ID_SIZE))
		return;

	remove_flag = GetByte( pBuf, index );

	if( uid < 0 || uid >= MAX_USER ) return;

/*	if( remove_flag == 0 && command == KNIGHTS_REMOVE )	{		// 없는 유저 추방시에는 디비에서만 처리한다
		result = m_DBAgent.UpdateKnights( command, userid, knightindex, remove_flag );
		TRACE("ModifyKnights - command=%d, nid=%d, index=%d, result=%d \n", command, uid, knightindex, result);
		return;
	}	*/

	result = m_DBAgent.UpdateKnights( command, userid, knightindex, remove_flag );
	TRACE("ModifyKnights - command=%d, nid=%d, index=%d, result=%d \n", command, uid, knightindex, result);

	//SetByte( send_buff, WIZ_KNIGHTS_PROCESS, send_index );
	SetByte( send_buff, command, send_index );
	SetShort( send_buff, uid, send_index );
	SetByte( send_buff, result, send_index );
	SetShort( send_buff, knightindex, send_index );
	SetKOString(send_buff, userid, send_index);
	SetByte( send_buff, remove_flag, send_index );

	do {
		if( m_LoggerSendQueue.PutData( send_buff, send_index ) == 1 )
			break;
		else
			count++;
	} while( count < 50 );
	if( count >= 50 )
		m_OutputList.AddString("Modify Packet Drop!!!");
}

void CAujardDlg::DestroyKnights(char *pBuf)
{
	int index = 0, send_index = 0, knightindex = 0, uid = -1, count = 0;
	BYTE result = 0;
	char send_buff[256]; 

	uid = GetShort( pBuf, index );
	knightindex = GetShort( pBuf, index );
	if( uid < 0 || uid >= MAX_USER ) return;

	result = m_DBAgent.DeleteKnights( knightindex );
	TRACE("DestoryKnights - nid=%d, index=%d, result=%d \n", uid, knightindex, result);

	SetByte( send_buff, KNIGHTS_DESTROY, send_index );
	SetShort( send_buff, uid, send_index );
	SetByte( send_buff, result, send_index );
	SetShort( send_buff, knightindex, send_index );

	do {
		if( m_LoggerSendQueue.PutData( send_buff, send_index ) == 1 )
			break;
		else
			count++;
	} while( count < 50 );
	if( count >= 50 )
		m_OutputList.AddString("Destroy Packet Drop!!!");
}

void CAujardDlg::AllKnightsMember(char *pBuf)
{
	int index = 0, send_index = 0, knightindex = 0, uid = -1, buff_index = 0, page = 0, count = 0, t_count = 0;
	BYTE result = 0;
	char send_buff[2048], temp_buff[2048];

	uid = GetShort( pBuf, index );
	knightindex = GetShort( pBuf, index );
	//page = GetShort( pBuf, index );
	if( uid < 0 || uid >= MAX_USER ) return;
	//if( page < 0 )  return;

	count = m_DBAgent.LoadKnightsAllMembers( knightindex, 0, temp_buff, buff_index );
	//count = m_DBAgent.LoadKnightsAllMembers( knightindex, page*10, temp_buff, buff_index );
	
	SetByte( send_buff, KNIGHTS_MEMBER_REQ, send_index );
	SetShort( send_buff, uid, send_index );
	SetByte( send_buff, 0x00, send_index );		// Success
	SetShort( send_buff, 4+buff_index, send_index );	// total packet size -> short(*3) + buff_index 
	//SetShort( send_buff, page, send_index );
	SetShort( send_buff, count, send_index );
	SetString( send_buff, temp_buff, buff_index, send_index );

	do {
		if( m_LoggerSendQueue.PutData( send_buff, send_index ) == 1 )
			break;
		else
			t_count++;
	} while( t_count < 50 );
	if( t_count >= 50 )
		m_OutputList.AddString("Member Packet Drop!!!");
}

void CAujardDlg::KnightsList( char* pBuf )
{
	int index = 0, send_index = 0, knightindex = 0, uid = -1, buff_index = 0, count = 0, retvalue = 0;
	char send_buff[256], temp_buff[256];

	uid = GetShort( pBuf, index );
	knightindex = GetShort( pBuf, index );
	if( uid < 0 || uid >= MAX_USER ) return;

	retvalue = m_DBAgent.LoadKnightsInfo( knightindex, temp_buff, buff_index );
	
	SetByte( send_buff, KNIGHTS_LIST_REQ, send_index );
	SetShort( send_buff, uid, send_index );
	SetByte( send_buff, 0x00, send_index );							// Success
	SetString( send_buff, temp_buff, buff_index, send_index );

	do {
		if( m_LoggerSendQueue.PutData( send_buff, send_index ) == 1 )
			break;
		else
			count++;
	} while( count < 50 );
	if( count >= 50 )
		m_OutputList.AddString("KnightsList Packet Drop!!!");
}

void CAujardDlg::SetLogInInfo(char *pBuf)
{
	int index = 0, serverno = 0, uid = -1, send_index = 0, count = 0;
	BYTE bInit;
	char accountid[MAX_ID_SIZE+1], serverip[20], clientip[20], charid[MAX_ID_SIZE+1], send_buff[256]; 

	uid = GetShort( pBuf, index );
	if (!GetKOString(pBuf, accountid, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, charid, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, serverip, index, sizeof(serverip) - 1))
		return;

	serverno = GetShort( pBuf, index );
	if (!GetKOString(pBuf, clientip, index, sizeof(clientip) - 1))
		return;

	bInit = GetByte( pBuf, index );

	if( m_DBAgent.SetLogInInfo( accountid, charid, serverip, serverno, clientip, bInit ) == FALSE ) {
		SetByte( send_buff, WIZ_LOGIN_INFO, send_index );
		SetShort( send_buff, uid, send_index );
		SetByte( send_buff, 0x00, send_index );							// FAIL
		do {
			if( m_LoggerSendQueue.PutData( send_buff, send_index ) == 1 )
				break;
			else
				count++;
		} while( count < 50 );
		if( count >= 50 )
			m_OutputList.AddString("Login Info Packet Drop!!!");

		char logstr[256];
		sprintf_s( logstr, sizeof(logstr), "LoginINFO Insert Fail : %s, %s, %d\r\n", accountid, charid, bInit );
		WriteLogFile( logstr );
	}
}

void CAujardDlg::UserKickOut(char *pBuf)
{
	int index = 0;
	char accountid[MAX_ID_SIZE+1];
	if (!GetKOString(pBuf, accountid, index, MAX_ID_SIZE))
		return;

	m_DBAgent.AccountLogout( accountid );
}

void CAujardDlg::SaveUserData()
{
	_USER_DATA* pUser = NULL;
	char send_buff[256];
	int send_index = 0;
	
	for( int i=0; i<MAX_USER; i++ ) {
		pUser = (_USER_DATA*)m_DBAgent.m_UserDataArray[i];
		if( !pUser )
			continue;
		if (*pUser->m_id != 0)
		{
			if (GetTickCount() - pUser->m_dwTime > 360000)
			{
				send_index = 0;
				SetShort( send_buff, i, send_index );
				SetKOString( send_buff, pUser->m_Accountid, send_index );
				SetKOString( send_buff, pUser->m_id, send_index );
				UserDataSave(send_buff);
				Sleep(100);
			}
		}
	}
}

void CAujardDlg::WriteLogFile( char* pData )
{
	CTime cur = CTime::GetCurrentTime();
	char strLog[1024];
	int nDay = cur.GetDay();

	if( m_iLogFileDay != nDay )	{
		if(m_LogFile.m_hFile != CFile::hFileNull) m_LogFile.Close();

		sprintf_s(strLog, sizeof(strLog), "AujardLog-%d-%d-%d.txt", cur.GetYear(), cur.GetMonth(), cur.GetDay());
		m_LogFile.Open( strLog, CFile::modeWrite | CFile::modeCreate | CFile::modeNoTruncate | CFile::shareDenyNone );
		m_LogFile.SeekToEnd();
		m_iLogFileDay = nDay;
	}

	sprintf_s(strLog, sizeof(strLog), "%d-%d-%d %d:%d, %s\r\n", cur.GetYear(), cur.GetMonth(), cur.GetDay(), cur.GetHour(), cur.GetMinute(), pData);
	int nLen = strlen(strLog);
	if( nLen >= 1024 )	{
		TRACE("### WriteLogFile Fail : length = %d ###\n", nLen);
		return;
	}
	
	m_LogFile.Write(strLog, nLen);
}

void CAujardDlg::BattleEventResult( char* pData )
{
	int nType = 0, nResult = 0, index = 0;
	char strMaxUserName[MAX_ID_SIZE+1];

	nType = GetByte(pData, index);
	nResult = GetByte(pData, index);

	if (GetKOString(pData, strMaxUserName, index, MAX_ID_SIZE, sizeof(BYTE)))
	{
		TRACE("--> BattleEventResult : 적국의 대장을 죽인 유저이름은? %s, len=%d, nation=%d \n", strMaxUserName, nResult, nResult);
		m_DBAgent.UpdateBattleEvent( strMaxUserName, nResult );
	}
}

