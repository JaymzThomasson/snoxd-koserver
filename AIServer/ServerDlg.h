#pragma once
//#pragma warning(disable : 4786)

#include "../shared/KOSocketMgr.h"
#include "GameSocket.h"

#include "MAP.h"
#include "NpcTable.h"
#include "NpcItem.h"
#include "Pathfind.h"
#include "User.h"
#include "Npc.h"
#include "NpcThread.h"
#include "Server.h"
#include "Party.h"

#include "extern.h"			// 전역 객체

#include "../shared/STLMap.h"
#include <vector>
#include <list>

#include "../shared/lzf.h"
#include "../shared/crc32.h"

/////////////////////////////////////////////////////////////////////////////
// CServerDlg dialog

typedef std::vector <CNpcThread*>			NpcThreadArray;
typedef CSTLMap <CNpcTable>					NpcTableArray;
typedef CSTLMap <CNpc>						NpcArray;
typedef CSTLMap <_MAGIC_TABLE>				MagictableArray;
typedef CSTLMap <_MAGIC_TYPE1>				Magictype1Array;
typedef CSTLMap <_MAGIC_TYPE2>				Magictype2Array;
typedef CSTLMap <_MAGIC_TYPE3>				Magictype3Array;
typedef CSTLMap	<_MAGIC_TYPE4>				Magictype4Array;
typedef CSTLMap <_PARTY_GROUP>				PartyArray;
typedef CSTLMap <_MAKE_WEAPON>				MakeWeaponItemTableArray;
typedef CSTLMap <_MAKE_ITEM_GRADE_CODE>		MakeGradeItemTableArray;
typedef CSTLMap <_MAKE_ITEM_LARE_CODE>		MakeLareItemTableArray;
typedef std::list <int>						ZoneNpcInfoList;
typedef CSTLMap <MAP>						ZoneArray;

class CServerDlg : public CDialog
{
private:
	void ResumeAI();
	BOOL CreateNpcThread();
	BOOL GetMagicTableData();
	BOOL GetMagicType1Data();
	BOOL GetMagicType2Data();
	BOOL GetMagicType3Data();
	BOOL GetMagicType4Data();
	BOOL GetMonsterTableData();
	BOOL GetNpcTableData();
	BOOL GetNpcItemTable();
	BOOL GetMakeWeaponItemTableData();
	BOOL GetMakeDefensiveItemTableData();
	BOOL GetMakeGradeItemTableData();
	BOOL GetMakeLareItemTableData();
	BOOL MapFileLoad();
	void GetServerInfoIni();
	
	void SyncTest();
	void RegionCheck();		// region안에 들어오는 유저 체크 (스레드에서 FindEnermy()함수의 부하를 줄이기 위한 꽁수)
// Construction
public:
	void GameServerAcceptThread();
	BOOL AddObjectEventNpc(_OBJECT_EVENT* pEvent, int zone_number);
	void AllNpcInfo();			// ~sungyong 2002.05.23
	CUser* GetUserPtr(int nid);
	CUser* GetActiveUserPtr(int index);
	CNpc*  GetEventNpcPtr();
	BOOL   SetSummonNpcData(CNpc* pNpc, int zone, float fx, float fz);
	MAP * GetZoneByID(int zonenumber);
	int GetServerNumber( int zonenumber );

	void AddToList(const char * format, ...);

	void CheckAliveTest();
	void DeleteUserList(int uid);
	void DeleteAllUserList(CGameSocket *pSock = NULL);
	int Send(char* pData, int length);
	void SendCompressed(char* pData, int length);
	void SendSystemMsg( char* pMsg, int type=0, int who=0 );
	void ResetBattleZone();

	CServerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CServerDlg)
	enum { IDD = IDD_SERVER_DIALOG };
	CListBox	m_StatusList;
	CString	m_strStatus;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerDlg)
	public:
	virtual BOOL DestroyWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

public:
//	ZoneArray			m_arZone;
	NpcArray			m_arNpc;
	NpcTableArray		m_arMonTable;
	NpcTableArray		m_arNpcTable;
	NpcThreadArray		m_arNpcThread;
	NpcThreadArray		m_arEventNpcThread;	// Event Npc Logic
	PartyArray			m_arParty;
	ZoneNpcInfoList		m_ZoneNpcList;
	MagictableArray		m_MagictableArray;
	Magictype1Array		m_Magictype1Array;
	Magictype2Array		m_Magictype2Array;
	Magictype3Array		m_Magictype3Array;
	Magictype4Array		m_Magictype4Array;
	MakeWeaponItemTableArray	m_MakeWeaponItemArray;
	MakeWeaponItemTableArray	m_MakeDefensiveItemArray;
	MakeGradeItemTableArray  m_MakeGradeItemArray;
	MakeLareItemTableArray  m_MakeLareItemArray;
	ZoneArray g_arZone;

	CWinThread* m_pZoneEventThread;		// zone

	CUser* m_pUser[MAX_USER];

	// class 객체
	CNpcItem				m_NpcItem;

	// 전역 객체 변수	//BOOL			m_bNpcExit;
	long			m_TotalNPC;			// DB에있는 총 수
	long			m_CurrentNPCError;	// 세팅에서 실패한 수
	long			m_CurrentNPC;		// 현재 게임상에서 실제로 셋팅된 수
	short			m_sTotalMap;		// Zone 수 
	short			m_sMapEventNpc;		// Map에서 읽어들이는 event npc 수

	// sungyong 2002.05.23
	BOOL			m_bFirstServerFlag;		// 서버가 처음시작한 후 게임서버가 붙은 경우에는 1, 붙지 않은 경우 0
	// ~sungyong 2002.05.23
	BYTE  m_byBattleEvent;				   // 전쟁 이벤트 관련 플래그( 1:전쟁중이 아님, 0:전쟁중)
	short m_sKillKarusNpc, m_sKillElmoNpc; // 전쟁동안에 죽은 npc숫자

	uint16	m_iYear, m_iMonth, m_iDate, m_iHour, m_iMin, m_iAmount;
	uint8 m_iWeather;
	BYTE	m_byNight;			// 밤인지,, 낮인지를 판단... 1:낮, 2:밤
	BYTE    m_byTestMode;

	static KOSocketMgr<CGameSocket> s_socketMgr;

private:
	BYTE				m_byZone;
	

// Implementation
protected:
	void DefaultInit();

	
//	CGameSocket m_GameSocket;

	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CServerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CServerDlg * g_pMain;