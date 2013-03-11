#include "stdafx.h"
#include "Server.h"
#include "ServerDlg.h"
#include "GameSocket.h"
#include "math.h"

#include "../shared/database/MyRecordSet.h"
#include "../shared/database/MagicTableSet.h"
#include "../shared/database/MagicType1Set.h"
#include "../shared/database/MagicType2Set.h"
#include "../shared/database/MagicType3Set.h"
#include "../shared/database/MagicType4Set.h"
#include "../shared/database/NpcPosSet.h"
#include "../shared/database/ZoneInfoSet.h"
#include "../shared/database/NpcItemSet.h"
#include "../shared/database/NpcTableSet.h"
#include "../shared/database/MakeWeaponTableSet.h"
#include "../shared/database/MakeDefensiveTableSet.h"
#include "../shared/database/MakeGradeItemTableSet.h"
#include "../shared/database/MakeLareItemTableSet.h"
#include "Region.h"
#include "../shared/ini.h"
#include "../shared/packets.h"

using namespace std;

BOOL g_bNpcExit	= FALSE;
ZoneArray			g_arZone;
CServerDlg * g_pMain = NULL;

CRITICAL_SECTION g_User_critical, g_region_critical;

KOSocketMgr<CGameSocket> CServerDlg::s_socketMgr;

#define CHECK_ALIVE 	100		//  게임서버와 통신이 끊김여부 판단, 타이머 변수
#define REHP_TIME		200
#define MONSTER_SPEED	1500

CServerDlg::CServerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CServerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServerDlg)
	m_strStatus = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	
	m_iYear = 0; 
	m_iMonth = 0;
	m_iDate = 0;
	m_iHour = 0;
	m_iMin = 0;
	m_iWeather = 0;
	m_iAmount = 0;
	m_byNight = 1;
	m_byZone = KARUS_ZONE;
	m_byBattleEvent = BATTLEZONE_CLOSE;
	m_sKillKarusNpc = 0;
	m_sKillElmoNpc = 0;
	m_pZoneEventThread = NULL;
	m_byTestMode = 0;

	memset(m_strGameDSN, 0, sizeof(m_strGameDSN));
	memset(m_strGameUID, 0, sizeof(m_strGameUID));
	memset(m_strGamePWD, 0, sizeof(m_strGamePWD));
}

void CServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerDlg)
	DDX_Control(pDX, IDC_LIST1, m_StatusList);
	DDX_Text(pDX, IDC_STATUS, m_strStatus);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CServerDlg, CDialog)
	//{{AFX_MSG_MAP(CServerDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CServerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	g_pMain = this;

	// Default Init ...
	DefaultInit();

	//----------------------------------------------------------------------
	//	Sets a random number starting point.
	//----------------------------------------------------------------------
	SetTimer( CHECK_ALIVE, 10000, NULL );
	srand( (unsigned)time(NULL) );

	InitializeCriticalSection( &g_region_critical );
	InitializeCriticalSection( &g_User_critical );
	m_sMapEventNpc = 0;
	m_bFirstServerFlag = FALSE;			

	// User Point Init
	for(int i=0; i<MAX_USER; i++)
		m_pUser[i] = NULL;

	// Server Start
	CTime time = CTime::GetCurrentTime();
	AddToList("[AI ServerStart - %d-%d-%d, %02d:%02d]", time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute() );

	//----------------------------------------------------------------------
	//	DB part initialize
	//----------------------------------------------------------------------
	GetServerInfoIni();

	if (!m_GameDB.Connect(m_strGameDSN, m_strGameUID, m_strGamePWD, false))
	{
		OdbcError *pError = m_GameDB.GetError();
		AfxMessageBox(pError->ErrorMessage.c_str());
		delete pError;
		EndDialog(IDCANCEL);
		return FALSE;
	}

	if(m_byZone == UNIFY_ZONE)	m_strStatus.Format("Server zone: ALL");
	else if(m_byZone == KARUS_ZONE)	m_strStatus.Format("Server zone: KARUS");
	else if(m_byZone == ELMORAD_ZONE) m_strStatus.Format("Server zone: EL MORAD");
	else if(m_byZone == BATTLE_ZONE) m_strStatus.Format("Server zone: BATTLE");

	//----------------------------------------------------------------------
	//	DB part initialize
	//----------------------------------------------------------------------


	//----------------------------------------------------------------------
	//	Communication Part Initialize ...
	//----------------------------------------------------------------------

	uint16 sPort = BATTLE_ZONE;
	if (m_byZone == KARUS_ZONE || m_byZone == UNIFY_ZONE)
		sPort = AI_KARUS_SOCKET_PORT;
	else if (m_byZone == ELMORAD_ZONE)
		sPort = AI_ELMO_SOCKET_PORT;

	if (!s_socketMgr.Listen(sPort, MAX_SOCKET))
	{
		EndDialog(IDCANCEL);
		return FALSE;
	}

	//----------------------------------------------------------------------
	//	Load tables
	//----------------------------------------------------------------------
	if (!GetMagicTableData()
		|| !GetMagicType1Data()
		|| !GetMagicType2Data()
		|| !GetMagicType3Data()
		|| !GetMagicType4Data()
		|| !GetNpcItemTable()
		|| !GetMakeWeaponItemTableData()
		|| !GetMakeDefensiveItemTableData()
		|| !GetMakeGradeItemTableData()
		|| !GetMakeLareItemTableData()
		|| !GetNpcTableData(false)
		|| !GetNpcTableData(true)
		// Load maps
		|| !MapFileLoad()
		// Spawn NPC threads
		|| !CreateNpcThread())
	{
		EndDialog(IDCANCEL);
		return FALSE;
	}	

	//----------------------------------------------------------------------
	//	Start NPC THREAD
	//----------------------------------------------------------------------
	ResumeAI();

	UpdateData(FALSE);	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CServerDlg::OnPaint() 
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
HCURSOR CServerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CServerDlg::DefaultInit()
{
	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
}


BOOL CServerDlg::GetMagicTableData()
{
	LOAD_TABLE(CMagicTableSet, &m_GameDB, &m_MagictableArray, false);
}

BOOL CServerDlg::GetMagicType1Data()
{
	LOAD_TABLE(CMagicType1Set, &m_GameDB, &m_Magictype1Array, false);
}

BOOL CServerDlg::GetMagicType2Data()
{
	LOAD_TABLE(CMagicType2Set, &m_GameDB, &m_Magictype2Array, false);
}

BOOL CServerDlg::GetMagicType3Data()
{
	LOAD_TABLE(CMagicType3Set, &m_GameDB, &m_Magictype3Array, false);
}

BOOL CServerDlg::GetMagicType4Data()
{
	LOAD_TABLE(CMagicType4Set, &m_GameDB, &m_Magictype4Array, false);
}

BOOL CServerDlg::GetMakeWeaponItemTableData()
{
	CMakeWeaponTableSet	MakeItemTableSet;

	if( !MakeItemTableSet.Open() ) {
		AfxMessageBox(_T("GetMakeWeaponItemTableData Open Fail!"));
		return FALSE;
	}
	if(MakeItemTableSet.IsBOF() || MakeItemTableSet.IsEOF()) {
		AfxMessageBox(_T("GetMakeWeaponItemTableData Empty!"));
		return FALSE;
	}

	MakeItemTableSet.MoveFirst();

	while( !MakeItemTableSet.IsEOF() )
	{
		_MAKE_WEAPON* pTable = new _MAKE_WEAPON;
				
		pTable->byIndex = MakeItemTableSet.m_byLevel;
		pTable->sClass[0] = MakeItemTableSet.m_sClass_1;
		pTable->sClass[1] = MakeItemTableSet.m_sClass_2;
		pTable->sClass[2] = MakeItemTableSet.m_sClass_3;
		pTable->sClass[3] = MakeItemTableSet.m_sClass_4;
		pTable->sClass[4] = MakeItemTableSet.m_sClass_5;
		pTable->sClass[5] = MakeItemTableSet.m_sClass_6;
		pTable->sClass[6] = MakeItemTableSet.m_sClass_7;
		pTable->sClass[7] = MakeItemTableSet.m_sClass_8;
		pTable->sClass[8] = MakeItemTableSet.m_sClass_9;
		pTable->sClass[9] = MakeItemTableSet.m_sClass_10;
		pTable->sClass[10] = MakeItemTableSet.m_sClass_11;
		pTable->sClass[11] = MakeItemTableSet.m_sClass_12;

		if( !m_MakeWeaponItemArray.PutData(pTable->byIndex, pTable) ) {
			TRACE("GetMakeWeaponItemTableData PutData Fail - %d\n", pTable->byIndex );
			delete pTable;
			pTable = NULL;
		}
		
		MakeItemTableSet.MoveNext();
	}
	return TRUE;
}

BOOL CServerDlg::GetMakeDefensiveItemTableData()
{
	CMakeDefensiveTableSet	MakeItemTableSet;

	if( !MakeItemTableSet.Open() ) {
		AfxMessageBox(_T("GetMakeDefensiveItemTableData Open Fail!"));
		return FALSE;
	}
	if(MakeItemTableSet.IsBOF() || MakeItemTableSet.IsEOF()) {
		AfxMessageBox(_T("GetMakeDefensiveItemTableData Empty!"));
		return FALSE;
	}

	MakeItemTableSet.MoveFirst();

	while( !MakeItemTableSet.IsEOF() )
	{
		_MAKE_WEAPON* pTable = new _MAKE_WEAPON;
				
		pTable->byIndex = MakeItemTableSet.m_byLevel;
		pTable->sClass[0] = MakeItemTableSet.m_sClass_1;
		pTable->sClass[1] = MakeItemTableSet.m_sClass_2;
		pTable->sClass[2] = MakeItemTableSet.m_sClass_3;
		pTable->sClass[3] = MakeItemTableSet.m_sClass_4;
		pTable->sClass[4] = MakeItemTableSet.m_sClass_5;
		pTable->sClass[5] = MakeItemTableSet.m_sClass_6;
		pTable->sClass[6] = MakeItemTableSet.m_sClass_7;

		if( !m_MakeDefensiveItemArray.PutData(pTable->byIndex, pTable) ) {
			TRACE("GetMakeDefensiveItemTableData PutData Fail - %d\n", pTable->byIndex );
			delete pTable;
			pTable = NULL;
		}
		
		MakeItemTableSet.MoveNext();
	}
	return TRUE;
}

BOOL CServerDlg::GetMakeGradeItemTableData()
{
	CMakeGradeItemTableSet	MakeItemTableSet;

	if( !MakeItemTableSet.Open() ) {
		AfxMessageBox(_T("MakeGradeItemTable Open Fail!"));
		return FALSE;
	}
	if(MakeItemTableSet.IsBOF() || MakeItemTableSet.IsEOF()) {
		AfxMessageBox(_T("MakeGradeItemTable Empty!"));
		return FALSE;
	}

	MakeItemTableSet.MoveFirst();

	while( !MakeItemTableSet.IsEOF() )
	{
		_MAKE_ITEM_GRADE_CODE* pTable = new _MAKE_ITEM_GRADE_CODE;
				
		pTable->byItemIndex = MakeItemTableSet.m_byItemIndex;
		for (int i = 0; i < 9; i++)
			pTable->sGrade[i] = MakeItemTableSet.m_sGrade[i];

		if( !m_MakeGradeItemArray.PutData(pTable->byItemIndex, pTable) ) {
			TRACE("MakeGradeItemTable PutData Fail - %d\n", pTable->byItemIndex );
			delete pTable;
			pTable = NULL;
		}
		
		MakeItemTableSet.MoveNext();
	}
	return TRUE;
}


BOOL CServerDlg::GetMakeLareItemTableData()
{
	CMakeLareItemTableSet	MakeItemTableSet;

	if( !MakeItemTableSet.Open() ) {
		AfxMessageBox(_T("MakeLareItemTable Open Fail!"));
		return FALSE;
	}
	if(MakeItemTableSet.IsBOF() || MakeItemTableSet.IsEOF()) {
		AfxMessageBox(_T("MakeLareItemTable Empty!"));
		return FALSE;
	}

	MakeItemTableSet.MoveFirst();

	while( !MakeItemTableSet.IsEOF() )
	{
		_MAKE_ITEM_LARE_CODE* pTable = new _MAKE_ITEM_LARE_CODE;
				
		pTable->byItemLevel = MakeItemTableSet.m_byLevelGrade;
		pTable->sLareItem = MakeItemTableSet.m_sLareItem;
		pTable->sMagicItem = MakeItemTableSet.m_sMagicItem;
		pTable->sGereralItem = MakeItemTableSet.m_sGereralItem;

		if( !m_MakeLareItemArray.PutData(pTable->byItemLevel, pTable) ) {
			TRACE("MakeItemTable PutData Fail - %d\n", pTable->byItemLevel );
			delete pTable;
			pTable = NULL;
		}
		
		MakeItemTableSet.MoveNext();
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////
//	NPC Item Table 을 읽는다.
//
BOOL CServerDlg::GetNpcItemTable()
{
	CNpcItemSet NpcItemSet;
	int nRowCount = 0;
	short i;
	int nItem = 0;

	try
	{
		if(NpcItemSet.IsOpen()) NpcItemSet.Close();

		NpcItemSet.m_strSort = _T("sIndex");

		if(!NpcItemSet.Open())
		{
			AfxMessageBox(_T("MONSTER ITEM DB Open Fail!"));
			return FALSE;
		}

		if(NpcItemSet.IsBOF()) 
		{
			AfxMessageBox(_T("MONSTER ITEM DB Empty!"));
			return FALSE;
		}

		while(!NpcItemSet.IsEOF())
		{
			nRowCount++;
			NpcItemSet.MoveNext();
		}

		m_NpcItem.m_nField = NpcItemSet.m_nFields;
		m_NpcItem.m_nRow = nRowCount;

		if(nRowCount == 0) return FALSE;

		m_NpcItem.m_ppItem = new int* [m_NpcItem.m_nRow];
		for(i = 0; i < m_NpcItem.m_nRow; i++)
		{
			m_NpcItem.m_ppItem[i] = new int[m_NpcItem.m_nField];
		}

		NpcItemSet.MoveFirst();

		i = 0;
		while(!NpcItemSet.IsEOF())
		{
			m_NpcItem.m_ppItem[i][0] = NpcItemSet.m_sIndex;
			m_NpcItem.m_ppItem[i][1] = NpcItemSet.m_iItem01;
			m_NpcItem.m_ppItem[i][2] = NpcItemSet.m_sPersent01;
			m_NpcItem.m_ppItem[i][3] = NpcItemSet.m_iItem02;
			m_NpcItem.m_ppItem[i][4] = NpcItemSet.m_sPersent02;
			m_NpcItem.m_ppItem[i][5] = NpcItemSet.m_iItem03;
			m_NpcItem.m_ppItem[i][6] = NpcItemSet.m_sPersent03;
			m_NpcItem.m_ppItem[i][7] = NpcItemSet.m_iItem04;
			m_NpcItem.m_ppItem[i][8] = NpcItemSet.m_sPersent04;
			m_NpcItem.m_ppItem[i][9] = NpcItemSet.m_iItem05;
			m_NpcItem.m_ppItem[i][10] = NpcItemSet.m_sPersent05;

			i++;
			NpcItemSet.MoveNext();
		}
	}
	catch(CDBException* e)
	{
		e->ReportError();
		e->Delete();

		return FALSE;
	}

	return TRUE;
}

//	Monster Table Data 를 읽는다.
BOOL CServerDlg::GetNpcTableData(bool bNpcData /*= true*/)
{
	CNpcTableSet NpcTableSet;
	NpcTableArray *pMap = (bNpcData ? &m_arNpcTable : &m_arMonTable);	
	
	try 
	{
		if(NpcTableSet.IsOpen()) NpcTableSet.Close();
		
		if(!NpcTableSet.Open(-1, bNpcData ? _T("K_NPC") : _T("K_MONSTER")))
		{
			AfxMessageBox(_T("MONSTER DB Open Fail!"));
			return FALSE;
		}
		if(NpcTableSet.IsBOF()) 
		{
			AfxMessageBox(_T("MONSTER DB Empty!"));
			return FALSE;
		}

		while(!NpcTableSet.IsEOF())
		{
			CNpcTable* Npc = new CNpcTable;
			CString tmpNpcName;
			Npc->Initialize();
			
			Npc->m_sSid			= NpcTableSet.m_sSid;		// MONSTER(NPC) Serial ID
			_tcscpy(Npc->m_strName, NpcTableSet.m_strName);	// MONSTER(NPC) Name
			Npc->m_sPid = NpcTableSet.m_sPid;				// MONSTER(NPC) Picture ID
			Npc->m_sSize = NpcTableSet.m_sSize;				// MONSTER(NPC) 캐릭 크기 비율
			Npc->m_iWeapon_1 = NpcTableSet.m_iWeapon1;			// 착용무기
			Npc->m_iWeapon_2 = NpcTableSet.m_iWeapon2;			// 착용무기
			Npc->m_byGroup = NpcTableSet.m_byGroup;			// 소속집단
			Npc->m_byActType = NpcTableSet.m_byActType;		// 행동패턴
			Npc->m_byRank = NpcTableSet.m_byRank;			// 작위
			Npc->m_byTitle = NpcTableSet.m_byTitle;			// 지위
			Npc->m_iSellingGroup = NpcTableSet.m_iSellingGroup;		// item group
			Npc->m_sLevel = NpcTableSet.m_sLevel;			// level
			Npc->m_iExp = NpcTableSet.m_iExp;				// 경험치
			Npc->m_iLoyalty = NpcTableSet.m_iLoyalty;		// loyalty
			Npc->m_iMaxHP = NpcTableSet.m_iHpPoint;	// 최대 HP
			Npc->m_sMaxMP = NpcTableSet.m_sMpPoint;	// 최대 MP
			Npc->m_sAttack = NpcTableSet.m_sAtk;			// 공격값
			Npc->m_sDefense = NpcTableSet.m_sAc;			// 방어값
			Npc->m_sHitRate = NpcTableSet.m_sHitRate;		// 타격성공률
			Npc->m_sEvadeRate = NpcTableSet.m_sEvadeRate;	// 회피성공률
			Npc->m_sDamage = NpcTableSet.m_sDamage;			// 기본 데미지
			Npc->m_sAttackDelay = NpcTableSet.m_sAttackDelay;	// 공격딜레이
			Npc->m_bySpeed_1 = NpcTableSet.m_bySpeed1;				// 이동속도	(걷기)
			Npc->m_bySpeed_2 = NpcTableSet.m_bySpeed2;				// 이동속도	(뛰기)
			Npc->m_sStandTime = NpcTableSet.m_sStandtime;		// 서있는 시간
			Npc->m_iMagic1 = NpcTableSet.m_iMagic1;			// 사용마법 1
			Npc->m_iMagic2 = NpcTableSet.m_iMagic2;			// 사용마법 2
			Npc->m_iMagic3 = NpcTableSet.m_iMagic3;			// 사용마법 3	
			Npc->m_byFireR = NpcTableSet.m_byFireR;			// 화염 저항력
			Npc->m_byColdR = NpcTableSet.m_byColdR;			// 냉기 저항력
			Npc->m_byLightningR = NpcTableSet.m_byLightningR;			// 전기 저항력
			Npc->m_byMagicR = NpcTableSet.m_byMagicR;			// 마법 저항력
			Npc->m_byDiseaseR = NpcTableSet.m_byDiseaseR;		// 저주 저항력
			Npc->m_byPoisonR = NpcTableSet.m_byPoisonR;		// 독 저항력
			Npc->m_byLightR = NpcTableSet.m_byLightR;		// 빛 저항력
			Npc->m_fBulk =  (float)(((double)NpcTableSet.m_sBulk / 100) * ((double)NpcTableSet.m_sSize / 100));
			Npc->m_bySearchRange = NpcTableSet.m_bySearchRange;	// 적 탐지 범위
			Npc->m_byAttackRange = NpcTableSet.m_byAttackRange;	// 사정거리
			Npc->m_byTracingRange = NpcTableSet.m_byTracingRange;	// 추격거리
			Npc->m_tNpcType = NpcTableSet.m_byType;			// NPC Type
								// 0 : Monster
								// 1 : Normal NPC
			
			Npc->m_byFamilyType = NpcTableSet.m_byFamily;		// 몹들사이에서 가족관계를 결정한다.

			Npc->m_iMoney = NpcTableSet.m_iMoney;			// 떨어지는 돈
			Npc->m_iItem = NpcTableSet.m_sItem;			// 떨어지는 아이템
			Npc->m_byDirectAttack = NpcTableSet.m_byDirectAttack;
			Npc->m_byMagicAttack = NpcTableSet.m_byMagicAttack;
			
			if (!pMap->PutData(Npc->m_sSid, Npc))
				delete Npc;

			NpcTableSet.MoveNext();
		}

		NpcTableSet.Close();
	}
	catch(CDBException* e)
	{
		e->ReportError();
		e->Delete();

		return FALSE;
	}

	return TRUE;
}

//	Npc Thread 를 만든다.
BOOL CServerDlg::CreateNpcThread()
{
	BOOL	bMoveNext	= TRUE;
	int		nSerial		= m_sMapEventNpc;
	int		nPathSerial = 1;
	int		nNpcCount	= 0;
	int		i=0, j=0;
	int nRandom = 0, nServerNum = 0;
	double  dbSpeed = 0;

	m_TotalNPC = 0;			// DB에 있는 수
	m_CurrentNPC = 0;
	m_CurrentNPCError = 0;

	CNpcTable*	pNpcTable = NULL;
	CRoomEvent* pRoom = NULL;

	CNpcPosSet NpcPosSet;

	char szPath[500];
	char szX[5];
	char szZ[5];

	float fRandom_X = 0.0f;
	float fRandom_Z = 0.0f;

	int nMonsterNumber = 0;
	
	try 
	{
		if(NpcPosSet.IsOpen()) NpcPosSet.Close();
		if(!NpcPosSet.Open())	{
			AfxMessageBox(_T("MONSTER_POS DB Open Fail!"));
			return FALSE;
		}
		if(NpcPosSet.IsBOF())	{
			AfxMessageBox(_T("MONSTER_POS DB Empty!"));
			return FALSE;
		}

		NpcPosSet.MoveFirst();

		while(!NpcPosSet.IsEOF())	{
			nMonsterNumber = NpcPosSet.m_NumNPC;
			//if( NpcPosSet.m_ZoneID == 101 )	{	// 테스트를 위해서,, 
			//	nMonsterNumber = 1;
				//if(nMonsterNumber > 4)	{
				//	nMonsterNumber = nMonsterNumber / 4;
				//}
			//}	
			nServerNum = 0;
			nServerNum = GetServerNumber( NpcPosSet.m_ZoneID );

			if( m_byZone == nServerNum || m_byZone == UNIFY_ZONE)	{
				for(j=0; j<nMonsterNumber; j++)		{
					CNpc*		pNpc		= new CNpc();

					pNpc->m_byMoveType = NpcPosSet.m_ActType;
					pNpc->m_byInitMoveType = NpcPosSet.m_ActType;

					bool bMonster = (NpcPosSet.m_ActType < 100);
					if (bMonster)
						pNpcTable = m_arMonTable.GetData(NpcPosSet.m_NpcID);
					else 
					{
						pNpc->m_byMoveType = NpcPosSet.m_ActType - 100;
						//pNpc->m_byInitMoveType = NpcPosSet.m_ActType - 100;
						pNpcTable = m_arNpcTable.GetData(NpcPosSet.m_NpcID);
					}

					if (pNpcTable == NULL)
					{
						char buff[128] = {0};
						sprintf(buff, "NPC %d not found in %s table.", pNpc->m_sNid, bMonster ? "K_MONSTER" : "K_NPC");
						AfxMessageBox(buff);
						delete pNpc;
						return FALSE;
					}

					pNpc->Load(nSerial++, pNpcTable);
					pNpc->m_byBattlePos = 0;

					if(pNpc->m_byMoveType >= 2)	{
						pNpc->m_byBattlePos = myrand( 1, 3 );
						pNpc->m_byPathCount = nPathSerial++;
					}

					pNpc->InitPos();
					
					if( bMoveNext )	{
						bMoveNext = FALSE;
						nNpcCount = NpcPosSet.m_NumNPC;
					}

					//////// MONSTER POS ////////////////////////////////////////
					pNpc->m_bCurZone = NpcPosSet.m_ZoneID;

					// map에 몬스터의 위치를 랜덤하게 위치시킨다.. (테스트 용 : 수정-DB에서 읽어오는데로 몬 위치 결정)
					nRandom = abs(NpcPosSet.m_LeftX - NpcPosSet.m_RightX);
					if(nRandom <= 1)
						fRandom_X = (float)NpcPosSet.m_LeftX;
					else	{
						if(NpcPosSet.m_LeftX < NpcPosSet.m_RightX)
							fRandom_X = (float)myrand(NpcPosSet.m_LeftX, NpcPosSet.m_RightX);
						else
							fRandom_X = (float)myrand(NpcPosSet.m_RightX, NpcPosSet.m_LeftX);
					}
					nRandom = abs(NpcPosSet.m_TopZ - NpcPosSet.m_BottomZ);
					if(nRandom <= 1)
						fRandom_Z = (float)NpcPosSet.m_TopZ;
					else	{
						if(NpcPosSet.m_TopZ < NpcPosSet.m_BottomZ)
							fRandom_Z = (float)myrand(NpcPosSet.m_TopZ, NpcPosSet.m_BottomZ);
						else
							fRandom_Z = (float)myrand(NpcPosSet.m_BottomZ, NpcPosSet.m_TopZ);
					}

					pNpc->m_fCurX	= fRandom_X;
					pNpc->m_fCurY	= 0;
					pNpc->m_fCurZ	= fRandom_Z;
					
					pNpc->m_sRegenTime		= NpcPosSet.m_RegTime * 1000;	// 초(DB)단위-> 밀리세컨드로
					pNpc->m_byDirection = NpcPosSet.m_byDirection;
					pNpc->m_sMaxPathCount = NpcPosSet.m_DotCnt;

					if( pNpc->m_byMoveType >= 2 && NpcPosSet.m_DotCnt == 0 )	{
						pNpc->m_byMoveType = 1;
						TRACE("##### ServerDlg:CreateNpcThread - Path type Error :  nid=%d, sid=%d, name=%s, acttype=%d, path=%d #####\n", pNpc->m_sNid+NPC_BAND, pNpc->m_proto->m_sSid, pNpc->m_proto->m_strName, pNpc->m_byMoveType, pNpc->m_sMaxPathCount);
					}

					int index = 0;
					strcpy_s(szPath, sizeof(szPath), NpcPosSet.m_path);

					if(NpcPosSet.m_DotCnt != 0)	{
						for(int l = 0; l<NpcPosSet.m_DotCnt; l++)	{
							memset(szX, 0x00, 5);
							memset(szZ, 0x00, 5);
							GetString(szX, szPath, 4, index);
							GetString(szZ, szPath, 4, index);
							pNpc->m_PathList.pPattenPos[l].x = atoi(szX);
							pNpc->m_PathList.pPattenPos[l].z = atoi(szZ);
						//	TRACE(" l=%d, x=%d, z=%d\n", l, pNpc->m_PathList.pPattenPos[l].x, pNpc->m_PathList.pPattenPos[l].z);
						}
					}

					//pNpc->m_byType			= NpcPosSet.m_byType;

					pNpc->m_tItemPer		= pNpcTable->m_tItemPer;	// NPC Type
					pNpc->m_tDnPer			= pNpcTable->m_tDnPer;	// NPC Type

					pNpc->m_nInitMinX = pNpc->m_nLimitMinX		= NpcPosSet.m_LeftX;
					pNpc->m_nInitMinY = pNpc->m_nLimitMinZ		= NpcPosSet.m_TopZ;
					pNpc->m_nInitMaxX = pNpc->m_nLimitMaxX		= NpcPosSet.m_RightX;
					pNpc->m_nInitMaxY = pNpc->m_nLimitMaxZ		= NpcPosSet.m_BottomZ;
					// dungeon work
					pNpc->m_byDungeonFamily	= NpcPosSet.m_DungeonFamily;
					pNpc->m_bySpecialType	= NpcPosSet.m_SpecialType;
					pNpc->m_byRegenType		= NpcPosSet.m_RegenType;
					pNpc->m_byTrapNumber    = NpcPosSet.m_TrapNumber;

					if( pNpc->m_byDungeonFamily > 0 )	{
						pNpc->m_nLimitMinX = NpcPosSet.m_LimitMinX;
						pNpc->m_nLimitMinZ = NpcPosSet.m_LimitMinZ;
						pNpc->m_nLimitMaxX = NpcPosSet.m_LimitMaxX;
						pNpc->m_nLimitMaxZ = NpcPosSet.m_LimitMaxZ;
					}	
			
					pNpc->m_pZone = GetZoneByID(pNpc->m_bCurZone);
					if (pNpc->GetMap() == NULL)		{
						AfxMessageBox("Error : CServerDlg,, Invaild zone Index!!");
						delete pNpc;
						return FALSE;
					}

					if( !m_arNpc.PutData( pNpc->m_sNid, pNpc) )		{
						TRACE("Npc PutData Fail - %d\n", pNpc->m_sNid);
						delete pNpc;
						pNpc = NULL;
						continue;
					}

					pNpc->SetUid(pNpc->m_fCurX, pNpc->m_fCurZ, pNpc->m_sNid + NPC_BAND);

					// 
					if (pNpc->GetMap()->m_byRoomEvent > 0 && pNpc->m_byDungeonFamily > 0 )	{
						pRoom = pNpc->GetMap()->m_arRoomEventArray.GetData( pNpc->m_byDungeonFamily );
						if (pRoom == NULL)
						{
							TRACE("Error : CServerDlg,, Map Room Npc Fail!! : nid=%d, sid=%d, name=%s, fam=%d, zone=%d\n", pNpc->m_sNid+NPC_BAND, pNpc->m_proto->m_sSid, pNpc->m_proto->m_strName, pNpc->m_byDungeonFamily, pNpc->m_bCurZone);
							AfxMessageBox("Error : CServerDlg,, Map Room Npc Fail!!");
							delete pNpc;
							return FALSE;
						}

						int *pInt = new int;
						*pInt = pNpc->m_sNid;
						if( !pRoom->m_mapRoomNpcArray.PutData( pNpc->m_sNid, pInt ) )	{
							TRACE("### Map - Room Array MonsterNid Fail : nid=%d, sid=%d ###\n", pNpc->m_sNid, pNpc->m_proto->m_sSid);
						}
					}


					m_TotalNPC = nSerial;

					if(--nNpcCount > 0) continue;

					bMoveNext = TRUE;
					nNpcCount = 0;

				}
			}

			nPathSerial = 1;
			NpcPosSet.MoveNext();
		}

		NpcPosSet.Close();
	}
	catch(CDBException* e)
	{
		e->ReportError();
		e->Delete();
		
		return FALSE;
	}
		
	int step = 0, nThreadNumber = 0;
	CNpcThread* pNpcThread = NULL;

	foreach_stlmap (itr, m_arNpc)
	{
		if (step == 0)
			pNpcThread = new CNpcThread;

		CNpc *pNpc = pNpcThread->m_pNpc[step] = itr->second;
		pNpc->m_sThreadNumber = nThreadNumber;
		pNpc->Init();
		
		++step;
		
		if( step == NPC_NUM )
		{
			pNpcThread->m_sThreadNumber = nThreadNumber++;
			pNpcThread->m_pThread = AfxBeginThread(NpcThreadProc, &(pNpcThread->m_ThreadInfo), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
			m_arNpcThread.push_back( pNpcThread );
			step = 0;
		}
	}
	if( step != 0 )
	{
		pNpcThread->m_sThreadNumber = nThreadNumber++;
		pNpcThread->m_pThread = AfxBeginThread(NpcThreadProc, &(pNpcThread->m_ThreadInfo), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		m_arNpcThread.push_back( pNpcThread );
	}

	m_pZoneEventThread = AfxBeginThread(ZoneEventThreadProc, (LPVOID)(this), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);

	//TRACE("m_TotalNPC = %d \n", m_TotalNPC);
	AddToList("[Monster Init - %d, threads=%d]", m_TotalNPC, m_arNpcThread.size());
	return TRUE;
}

//	NPC Thread 들을 작동시킨다.
void CServerDlg::ResumeAI()
{
	foreach (itr, m_arNpcThread)
	{
		for (int j = 0; j < NPC_NUM; j++)
			(*itr)->m_ThreadInfo.pNpc[j] = (*itr)->m_pNpc[j];

		ResumeThread((*itr)->m_pThread->m_hThread);
	}

	ResumeThread(m_pZoneEventThread->m_hThread);
}

//	메모리 정리
BOOL CServerDlg::DestroyWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	KillTimer( CHECK_ALIVE );
	//KillTimer( REHP_TIME );

	g_bNpcExit = TRUE;

	foreach (itr, m_arNpcThread)
		WaitForSingleObject((*itr)->m_pThread->m_hThread, 1000);

	WaitForSingleObject(m_pZoneEventThread, 1000);

	// NpcThread Array Delete
	foreach (itr, m_arNpcThread)
		delete *itr;
	m_arNpcThread.clear();

	// Item Array Delete
	if( m_NpcItem.m_ppItem ) {
		for(int i=0; i< m_NpcItem.m_nRow; i++) {
			delete[] m_NpcItem.m_ppItem[i];
			m_NpcItem.m_ppItem[i] = NULL;
		}
		delete[] m_NpcItem.m_ppItem;
		m_NpcItem.m_ppItem = NULL;
	}


	// User Array Delete
	for(int i = 0; i < MAX_USER; i++)	{
		if(m_pUser[i])	{
			delete m_pUser[i];
			m_pUser[i] = NULL;
		}
	}

	m_ZoneNpcList.clear();

	DeleteCriticalSection( &g_region_critical );
	DeleteCriticalSection( &g_User_critical );

	return CDialog::DestroyWindow();
}

void CServerDlg::AddToList(const char * format, ...)
{
	if (g_bNpcExit)
		return;

	char buffer[256];

	va_list args;
	va_start(args, format);
	_vsnprintf(buffer, sizeof(buffer) - 1, format, args);
	va_end(args);

	m_StatusList.AddString(buffer);
}

void CServerDlg::DeleteUserList(int uid)
{
	if(uid < 0 || uid >= MAX_USER)	{
		TRACE("#### ServerDlg:DeleteUserList Uid Fail : uid=%d\n", uid);
		return;
	}

	EnterCriticalSection( &g_User_critical );

	CUser* pUser = NULL;
	pUser = m_pUser[uid];
	if( !pUser )	{
		LeaveCriticalSection( &g_User_critical );
		TRACE("#### ServerDlg:DeleteUserList UserPtr Fail : uid=%d\n", uid);
		return;
	}

	if( pUser->m_iUserId == uid )	{
		TRACE("*** UserLogOut으로 포인터 반환 : uid=%d, %s ***\n", uid, pUser->m_strUserID);
		pUser->m_lUsed = 1;
		delete m_pUser[uid];
		m_pUser[uid] = NULL;
	}
	else
		TRACE("#### ServerDlg:DeleteUserList Not Uid Fail : uid=%d\n", uid);

	LeaveCriticalSection( &g_User_critical );
}

BOOL CServerDlg::MapFileLoad()
{
	map<int, _ZONE_INFO*> zoneMap;
	CZoneInfoSet ZoneInfoSet(&zoneMap);

	m_sTotalMap = 0;
	if (!ZoneInfoSet.Read())
		return FALSE;

	foreach (itr, zoneMap)
	{
		CFile file;
		CString szFullPath;
		_ZONE_INFO *pZone = itr->second;

		MAP *pMap = new MAP();
		pMap->Initialize(pZone);
		delete pZone;

		g_arZone.PutData(pMap->m_nZoneNumber, pMap);

		szFullPath.Format(".\\MAP\\%s", pMap->m_MapName);
		if (!file.Open(szFullPath, CFile::modeRead)
			|| !pMap->LoadMap((HANDLE)file.m_hFile))
		{
			AfxMessageBox("Unable to load SMD - " + szFullPath);
			g_arZone.DeleteAllData();
			m_sTotalMap = 0;
			return FALSE;
		}
		file.Close();
		
		if (pMap->m_byRoomEvent > 0)
		{
			if (!pMap->LoadRoomEvent(pMap->m_byRoomEvent))
			{
				AfxMessageBox("Unable to load room event for map - " + szFullPath);
				pMap->m_byRoomEvent = 0;
			}
			else
			{
				pMap->m_byRoomEvent = 1;
			}
		}

		m_sTotalMap++;
	}

	return TRUE;
}

// game server에 모든 npc정보를 전송..
void CServerDlg::AllNpcInfo()
{
	// server alive check
	int nZone = 0;
	int size = m_arNpc.GetSize();

	int send_index = 0, zone_index = 0, send_count = 0, send_tot = 0;
	BYTE count = 0; 
	char send_buff[2048];

	foreach_stlmap (itr, g_arZone)
	{
		nZone = itr->first;
		send_index = 2;		count = 0;	send_count = 0;

		TRACE("****  allNpcInfo start = %d *****\n", nZone);

		foreach_stlmap (itr, m_arNpc)
		{
			CNpc *pNpc = itr->second;
			if(pNpc == NULL)	{
				TRACE("##### allNpcInfo Fail = %d\n", itr->first);
				continue;
			}
			if(pNpc->m_bCurZone != nZone)	continue;

			pNpc->SendNpcInfoAll(send_buff, send_index, count++);
			if(count == NPC_NUM)	{
				SetByte(send_buff, NPC_INFO_ALL, send_count );
				SetByte(send_buff, count, send_count );

				s_socketMgr.SendAllCompressed(send_buff, send_index);
				send_index = 2;
				send_count = 0;
				count = 0;
				send_tot++;
				//TRACE("AllNpcInfo - send_count=%d, count=%d, zone=%d\n", send_tot, count, nZone);
			}
		}	

		//if( count != 0 )	TRACE("--> AllNpcInfo - send_count=%d, count=%d, zone=%d\n", send_tot, count, nZone);
		if(count != 0 && count < NPC_NUM )	{
			send_count = 0;
			SetByte(send_buff, NPC_INFO_ALL, send_count );
			SetByte(send_buff, count, send_count );
			s_socketMgr.SendAllCompressed(send_buff, send_index);
			send_tot++;
			//TRACE("AllNpcInfo - send_count=%d, count=%d, zone=%d\n", send_tot, count, nZone);
		}

		Packet result(AG_SERVER_INFO, uint8(nZone));
		result << uint16(m_TotalNPC);
		s_socketMgr.SendAll(&result);

		TRACE("****  allNpcInfo end = %d *****\n", nZone);
	}
}

CUser* CServerDlg::GetUserPtr(int nid)
{
	CUser* pUser = NULL;

	if(nid < 0 || nid >= MAX_USER)	{
		if(nid != -1)		TRACE("### GetUserPtr :: User Array Overflow [%d] ###\n", nid );
		return NULL;
	}

	pUser = m_pUser[nid];
	if(pUser == NULL)	return NULL;
	if( pUser->m_lUsed == 1 ) return NULL;	// 포인터 사용을 허락치 않음.. (logout중)
	if(pUser->m_iUserId < 0 || pUser->m_iUserId >= MAX_USER)	return NULL;

	if( pUser->m_iUserId == nid )	return pUser;

	return NULL;
}

void CServerDlg::OnTimer(UINT nIDEvent) 
{
	switch( nIDEvent ) {
	case CHECK_ALIVE:
		// CheckAliveTest();
		break;
	case REHP_TIME:
		//RechargeHp();
		break;
	}

	CDialog::OnTimer(nIDEvent);
}

void CServerDlg::CheckAliveTest()
{
	Packet result(AG_CHECK_ALIVE_REQ);
	SessionMap & sessMap = s_socketMgr.GetActiveSessionMap();
	uint32 count = 0, sessCount = sessMap.size();
	foreach (itr, sessMap)
	{
		if (itr->second->Send(&result))
			count++;
	}
	s_socketMgr.ReleaseLock();

	if (sessCount > 0 && count == 0)
		DeleteAllUserList();

	RegionCheck();
}

void CServerDlg::DeleteAllUserList(CGameSocket *pSock)
{
	// If a server disconnected, show it...
	if (pSock != NULL)
	{
		AddToList("[GameServer disconnected = %s]", pSock->GetRemoteIP().c_str());
		return;
	}

	// Server didn't disconnect? 
	if (!m_bFirstServerFlag)
		return;

	// If there's no servers even connected, cleanup.
	TRACE("*** DeleteAllUserList - Start *** \n");
	foreach_stlmap (itr, g_arZone)
	{
		MAP * pMap = itr->second;
		if (pMap == NULL)	
			continue;
		for (int i=0; i<pMap->m_sizeRegion.cx; i++ ) {
			for( int j=0; j<pMap->m_sizeRegion.cy; j++ ) {
				pMap->m_ppRegion[i][j].m_RegionUserArray.DeleteAllData();
			}
		}
	}

	EnterCriticalSection( &g_User_critical );
	for (int i = 0; i < MAX_USER; i++)	
	{
		CUser *pUser = m_pUser[i];
		if (pUser == NULL)  
			continue;

		delete m_pUser[i];
		m_pUser[i] = NULL;
	}
	LeaveCriticalSection( &g_User_critical );

	// Party Array Delete 
	m_arParty.DeleteAllData();

	m_bFirstServerFlag = FALSE;
	TRACE("*** DeleteAllUserList - End *** \n");

	AddToList("[ DELETE All User List ]");
}

BOOL CServerDlg::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
		return TRUE;
	
	return CDialog::PreTranslateMessage(pMsg);
}

void CServerDlg::Send(char* pData, int length)
{
	s_socketMgr.SendAll(pData, length);
}

void CServerDlg::Send(Packet * pkt)
{
	s_socketMgr.SendAll(pkt);
}

void CServerDlg::GameServerAcceptThread()
{
	s_socketMgr.RunServer();
}

//	추가할 소환몹의 메모리를 참조하기위해 플래그가 0인 상태것만 넘긴다.
CNpc* CServerDlg::GetEventNpcPtr()
{
	CNpc* pNpc = NULL;
	for(int i = m_TotalNPC; i < m_arNpc.GetSize(); i++)		{
		pNpc = m_arNpc.GetData( i );
		if( !pNpc ) continue;

		if( pNpc->m_lEventNpc != 0 )	continue;

		pNpc->m_lEventNpc = 1;

		return pNpc;
	}
	return NULL;
}

BOOL CServerDlg::SetSummonNpcData(CNpc* pNpc, int zone, float fx, float fz)
{
	int  iCount = 0;
	CNpc* pEventNpc	= GetEventNpcPtr();

	if(pEventNpc == NULL)
	{
		TRACE("소환할수 있는 몹은 최대 20마리입니다.\n");
		return FALSE;
	}

	CString strMsg = _T(""); 

	pEventNpc->m_proto	= pNpc->m_proto;
	pEventNpc->m_byMoveType = 1;
	pEventNpc->m_byInitMoveType = 1;
	pEventNpc->m_byBattlePos = 0;
	pEventNpc->m_sSize		= pNpc->m_sSize;			// 캐릭터의 비율(100 퍼센트 기준)
	pEventNpc->m_iWeapon_1		= pNpc->m_iWeapon_1;	// 착용무기
	pEventNpc->m_iWeapon_2		= pNpc->m_iWeapon_2;	// 착용무기
	pEventNpc->m_byGroup		= pNpc->m_byGroup;		// 소속집단
	pEventNpc->m_byActType		= pNpc->m_byActType;	// 행동패턴
	pEventNpc->m_byRank			= pNpc->m_byRank;		// 작위
	pEventNpc->m_byTitle		= pNpc->m_byTitle;		// 지위
	pEventNpc->m_iSellingGroup = pNpc->m_iSellingGroup;
	pEventNpc->m_iHP			= pNpc->m_iMaxHP;		// 최대 HP
	pEventNpc->m_iMaxHP			= pNpc->m_iMaxHP;		// 현재 HP
	pEventNpc->m_sMP			= pNpc->m_sMaxMP;		// 최대 MP
	pEventNpc->m_sMaxMP			= pNpc->m_sMaxMP;		// 현재 MP
	pEventNpc->m_sAttack		= pNpc->m_sAttack;		// 공격값
	pEventNpc->m_sDefense		= pNpc->m_sDefense;		// 방어값
	pEventNpc->m_sHitRate		= pNpc->m_sHitRate;		// 타격성공률
	pEventNpc->m_sEvadeRate		= pNpc->m_sEvadeRate;	// 회피성공률
	pEventNpc->m_sDamage		= pNpc->m_sDamage;		// 기본 데미지
	pEventNpc->m_sAttackDelay	= pNpc->m_sAttackDelay; // 공격딜레이
	pEventNpc->m_sSpeed			= pNpc->m_sSpeed;		// 이동속도
	pEventNpc->m_fSpeed_1		= pNpc->m_fSpeed_1;	// 기본 이동 타입
	pEventNpc->m_fSpeed_2		= pNpc->m_fSpeed_2;	// 뛰는 이동 타입..
	pEventNpc->m_fOldSpeed_1	= pNpc->m_fOldSpeed_1;	// 기본 이동 타입
	pEventNpc->m_fOldSpeed_2	= pNpc->m_fOldSpeed_2;	// 뛰는 이동 타입..
	pEventNpc->m_fSecForMetor   = 4.0f;					// 초당 갈 수 있는 거리..
	pEventNpc->m_sStandTime		= pNpc->m_sStandTime;	// 서있는 시간
	pEventNpc->m_byFireR		= pNpc->m_byFireR;		// 화염 저항력
	pEventNpc->m_byColdR		= pNpc->m_byColdR;		// 냉기 저항력
	pEventNpc->m_byLightningR	= pNpc->m_byLightningR;	// 전기 저항력
	pEventNpc->m_byMagicR		= pNpc->m_byMagicR;		// 마법 저항력
	pEventNpc->m_byDiseaseR		= pNpc->m_byDiseaseR;	// 저주 저항력
	pEventNpc->m_byPoisonR		= pNpc->m_byPoisonR;	// 독 저항력
	pEventNpc->m_byLightR		= pNpc->m_byLightR;		// 빛 저항력
	pEventNpc->m_bySearchRange	= pNpc->m_bySearchRange;	// 적 탐지 범위
	pEventNpc->m_byAttackRange	= pNpc->m_byAttackRange;	// 사정거리
	pEventNpc->m_byTracingRange	= pNpc->m_byTracingRange;	// 추격거리
	pEventNpc->m_iMoney			= pNpc->m_iMoney;			// 떨어지는 돈
	pEventNpc->m_iItem			= pNpc->m_iItem;			// 떨어지는 아이템
	pEventNpc->m_tNpcLongType    = pNpc->m_tNpcLongType;
	pEventNpc->m_byWhatAttackType = pNpc->m_byWhatAttackType;

	//////// MONSTER POS ////////////////////////////////////////
	pEventNpc->m_bCurZone = zone;
	pEventNpc->m_fCurX	= fx;
	pEventNpc->m_fCurY	= 0;
	pEventNpc->m_fCurZ	= fz;
 	pEventNpc->m_nInitMinX			= pNpc->m_nInitMinX;
	pEventNpc->m_nInitMinY			= pNpc->m_nInitMinY;
	pEventNpc->m_nInitMaxX			= pNpc->m_nInitMaxX;
	pEventNpc->m_nInitMaxY			= pNpc->m_nInitMaxY;
	pEventNpc->m_sRegenTime		= pNpc->m_sRegenTime;	// 초(DB)단위-> 밀리세컨드로
	pEventNpc->m_tItemPer		= pNpc->m_tItemPer;	// NPC Type
	pEventNpc->m_tDnPer			= pNpc->m_tDnPer;	// NPC Type

	pEventNpc->m_pZone		= GetZoneByID(zone);

	pEventNpc->m_NpcState = NPC_DEAD;	// 상태는 죽은것으로 해야 한다.. 
	pEventNpc->m_bFirstLive = 1;		// 처음 살아난 경우로 해줘야 한다..

	if (pEventNpc->GetMap() == NULL)
	{
		TRACE("Zone %d doesn't exist (NPC=%d)\n", zone, pNpc->m_proto->m_sSid);
		return FALSE;
	}

	pEventNpc->Init();

	BOOL bSuccess = FALSE;

	int test = 0;
	
	for(int i = 0; i < NPC_NUM; i++ ) {
		test = m_arEventNpcThread[0]->m_ThreadInfo.m_byNpcUsed[i];
		TRACE("setsummon == %d, used=%d\n", i, test);
		if( m_arEventNpcThread[0]->m_ThreadInfo.m_byNpcUsed[i] == 0 )	{
			m_arEventNpcThread[0]->m_ThreadInfo.m_byNpcUsed[i] = 1;
			bSuccess = TRUE;
			m_arEventNpcThread[0]->m_ThreadInfo.pNpc[i] = pEventNpc;
			break;
		}
	}

	if(!bSuccess)	{
		pEventNpc->m_lEventNpc = 0;
		TRACE("### 소환에 실패했습니다. ###\n");
		return FALSE;
	}

	TRACE("*** %d, %s 를 소환하였습니다. state = %d ***\n", pEventNpc->m_sNid+NPC_BAND, pEventNpc->m_proto->m_strName, pEventNpc->m_NpcState);

	return TRUE;
}

void CServerDlg::RegionCheck()
{
	EnterCriticalSection( &g_User_critical );
	foreach_stlmap(itr, g_arZone)	
	{
		MAP *pMap = itr->second;
		if (pMap == NULL)
			continue;

		for (int i = 0; i < pMap->m_sizeRegion.cx; i++)
			for (int j = 0; j < pMap->m_sizeRegion.cy; j++)
				pMap->m_ppRegion[i][j].m_byMoving = (pMap->m_ppRegion[i][j].m_RegionUserArray.GetSize() > 0 ? 1 : 0);
	}
	LeaveCriticalSection( &g_User_critical );
}

BOOL CServerDlg::AddObjectEventNpc(_OBJECT_EVENT* pEvent, int zone_number)
{
	int i=0, j=0, objectid=0;
	int offset = 0;
	int nServerNum = 0;
	nServerNum = GetServerNumber( zone_number );

	int sSid = (pEvent->sType == OBJECT_ANVIL || pEvent->sType == OBJECT_ARTIFACT 
					? pEvent->sIndex : pEvent->sControlNpcID);
	if (sSid <= 0)
		return FALSE;

	CNpcTable * pNpcTable = m_arNpcTable.GetData(sSid);
	if(pNpcTable == NULL)	{
		// TRACE("#### AddObjectEventNpc Fail : [sid = %d], zone=%d #####\n", pEvent->sIndex, zone_number);
		return FALSE;
	}

	CNpc *pNpc = new CNpc();

	pNpc->m_byMoveType = 0;
	pNpc->m_byInitMoveType = 0;

	pNpc->m_byBattlePos = 0;

	pNpc->m_byObjectType = SPECIAL_OBJECT;
	pNpc->m_bCurZone	= zone_number;
	pNpc->m_byGateOpen	= (BYTE)pEvent->sStatus;
	pNpc->m_fCurX		= pEvent->fPosX;
	pNpc->m_fCurY		= pEvent->fPosY;
	pNpc->m_fCurZ		= pEvent->fPosZ;
	
 	pNpc->m_nInitMinX	= (int)pEvent->fPosX-1;
	pNpc->m_nInitMinY	= (int)pEvent->fPosZ-1;
	pNpc->m_nInitMaxX	= (int)pEvent->fPosX+1;
	pNpc->m_nInitMaxY	= (int)pEvent->fPosZ+1;	

	pNpc->Load(m_sMapEventNpc++, pNpcTable);

	if (pNpc->GetMap() == NULL
		|| !m_arNpc.PutData(pNpc->m_sNid, pNpc))
	{
		TRACE("Npc PutData Fail - %d\n", pNpc->m_sNid);
		delete pNpc;
		return FALSE;
	}

	m_TotalNPC = m_sMapEventNpc;
	return TRUE;
}

MAP * CServerDlg::GetZoneByID(int zonenumber)
{
	return g_arZone.GetData(zonenumber);
}

int CServerDlg::GetServerNumber( int zonenumber )
{
	MAP *pMap = GetZoneByID(zonenumber);
	if (pMap == NULL)
		return -1;

	return pMap->m_nServerNo;
}

void CServerDlg::GetServerInfoIni()
{
	CIni inifile("server.ini");
	m_byZone = inifile.GetInt("SERVER", "ZONE", UNIFY_ZONE);
	inifile.GetString("ODBC", "GAME_DSN", "KN_online", m_strGameDSN, sizeof(m_strGameDSN), false);
	inifile.GetString("ODBC", "GAME_UID", "knight", m_strGameUID, sizeof(m_strGameUID), false);
	inifile.GetString("ODBC", "GAME_PWD", "knight", m_strGamePWD, sizeof(m_strGamePWD), false);
}

void CServerDlg::SendSystemMsg( char* pMsg, int type, int who )
{
	Packet result(AG_SYSTEM_MSG, uint8(type));
	result << int16(who) << pMsg;
	Send(&result);
}

void CServerDlg::ResetBattleZone()
{
	TRACE("ServerDlg - ResetBattleZone() : start \n");
	foreach_stlmap (itr, g_arZone)
	{
		MAP *pMap = itr->second;
		if (pMap == NULL || pMap->m_byRoomEvent == 0) 
			continue;
		//if( pMap->IsRoomStatusCheck() == TRUE )	continue;	// 전체방이 클리어 되었다면
		pMap->InitializeRoom();
	}
	TRACE("ServerDlg - ResetBattleZone() : end \n");
}