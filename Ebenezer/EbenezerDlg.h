// EbenezerDlg.h : header file
//

#if !defined(AFX_EBENEZERDLG_H__655A21EF_E029_42C0_890A_68DA7F542428__INCLUDED_)
#define AFX_EBENEZERDLG_H__655A21EF_E029_42C0_890A_68DA7F542428__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning(disable : 4786)

#include "Iocport.h"
#include "Ebenezer.h"
#include "Map.h"
#include "Define.h"
#include "GameDefine.h"
#include "AISocket.h"
#include "Npc.h"
#include "../shared/SharedMem.h"
#include "../shared/ini.h"
#include "Knights.h"
#include "KnightsManager.h"
#include "EVENT.h"
#include "UdpSocket.h"

#include "../shared/STLMap.h"
#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CEbenezerDlg dialog

typedef CSTLMap <C3DMap>					ZoneArray;
typedef std::map<int, long>					LevelUpArray;
typedef CSTLMap <_CLASS_COEFFICIENT>		CoefficientArray;
typedef CSTLMap <_ITEM_TABLE>				ItemtableArray;
typedef CSTLMap <_MAGIC_TABLE>				MagictableArray;
typedef CSTLMap <_MAGIC_TYPE1>				Magictype1Array;
typedef CSTLMap <_MAGIC_TYPE2>				Magictype2Array;
typedef CSTLMap <_MAGIC_TYPE3>				Magictype3Array;
typedef CSTLMap	<_MAGIC_TYPE4>				Magictype4Array;
typedef CSTLMap <_MAGIC_TYPE5>				Magictype5Array;
typedef CSTLMap <_MAGIC_TYPE6>				Magictype6Array;
typedef CSTLMap <_MAGIC_TYPE7>				Magictype7Array;
typedef CSTLMap <_MAGIC_TYPE8>				Magictype8Array; 
typedef CSTLMap <_MAGIC_TYPE9>				Magictype9Array;
typedef CSTLMap <CNpc>						NpcArray;
typedef CSTLMap <CAISocket>					AISocketArray;
typedef CSTLMap <_PARTY_GROUP>				PartyArray;
typedef CSTLMap <CKnights>					KnightsArray;
typedef CSTLMap <_ZONE_SERVERINFO>			ServerArray;
typedef CSTLMap <_HOME_INFO>				HomeArray;
typedef CSTLMap <_START_POSITION>			StartPositionArray;
typedef	CSTLMap	<EVENT>						QuestArray;
typedef	CSTLMap	<_SERVER_RESOURCE>			ServerResourceArray;
typedef std::vector <CString>				BlockNameArray;

class CUser;
class CEbenezerDlg : public CDialog
{
// Construction
public:	
	void WriteEventLog( char* pBuf );
	void FlySanta();
	void BattleZoneCurrentUsers();
	BOOL LoadKnightsRankTable();
	void GetCaptainUserPtr();
	void Send_CommandChat(Packet *pkt, int nation, CUser* pExceptUser = NULL);
	BOOL LoadBattleTable();
	void Send_UDP_All(Packet *pkt, int group_type = 0);
	void KickOutZoneUsers(short zone);
	__int64 GenerateItemSerial();
	int KickOutAllUsers();
	void CheckAliveUser();
	int GetKnightsGrade(int nPoints);
	void WritePacketLog();
	uint16 GetKnightsAllMembers(uint16 sClanID, Packet & result, uint16 & pktSize, bool bClanLeader);
	BOOL LoadAllKnightsUserData();
	BOOL LoadAllKnights();
	BOOL LoadHomeTable();
	BOOL LoadStartPositionTable();
	void Announcement(BYTE type, int nation=0, int chat_type=8);
	void ResetBattleZone();
	void BanishLosers();
	void BattleZoneVictoryCheck();
	void BattleZoneOpenTimer();
	void BattleZoneOpen( int nType );	// 0:open 1:close
	void AliveUserCheck();
	void WithdrawUserOut();
	BOOL LoadMagicType1();
	BOOL LoadMagicType2();
	BOOL LoadMagicType3();
	BOOL LoadMagicType4();
	BOOL LoadMagicType5();
	BOOL LoadMagicType6();
	BOOL LoadMagicType7();
	BOOL LoadMagicType8();
	BOOL LoadMagicType9();
	void KillUser( const char* strbuff );
	void Send_PartyMember(int party, Packet *result);
	void Send_KnightsMember(int index, Packet *pkt);
	int GetAIServerPort();
	BOOL AISocketConnect( int zone, int flag = 0 );
	BOOL LoadNoticeData();
	BOOL LoadBlockNameList();
	BOOL LoadLevelUpTable();
	void SetGameTime();
	void UpdateWeather();
	void UpdateGameTime();
	void GetTimeFromIni();
	BOOL LoadCoefficientTable();
	BOOL LoadMagicTable();
	BOOL LoadItemTable();
	BOOL LoadServerResourceTable();
	BOOL MapFileLoad();
	void UserAcceptThread();
	// sungyong 2001.11.06
	BOOL AIServerConnect();
	void SendAllUserInfo();
	void DeleteAllNpcList(int flag = 0);
	CNpc*  GetNpcPtr( int sid, int cur_zone );
	// ~sungyong 2001.11.06
	BOOL InitializeMMF();

	// Get info for NPCs in regions around user (WIZ_REQ_NPCIN)
	void NpcInOutForMe(CUser* pSendUser);

	// Get info for NPCs in region
	void GetRegionNpcIn(C3DMap* pMap, int region_x, int region_z, Packet & pkt, uint16 & t_count);

	// Get list of NPC IDs in region
	void GetRegionNpcList(C3DMap* pMap, int region_x, int region_z, Packet & pkt, uint16 & t_count);

	// Get list of NPCs for regions around user (WIZ_NPC_REGION)
	void RegionNpcInfoForMe(CUser* pSendUser);	

	// Get info for users in regions around user (WIZ_REQ_USERIN)
	void UserInOutForMe(CUser* pSendUser);

	// Get info for users in region
	void GetRegionUserIn(C3DMap* pMap, int region_x, int region_z, Packet & pkt, uint16 & t_count);

	// Get list of user IDs in region
	void GetRegionUserList(C3DMap* pMap, int region_x, int region_z, Packet & pkt, uint16 & t_count);

	// Get list of users for regions around user (WIZ_REGIONCHANGE)
	void RegionUserInOutForMe(CUser* pSendUser);

	// Get info for merchants in regions around user
	void MerchantUserInOutForMe(CUser* pSendUser);

	// Get list of merchants in region
	void GetRegionMerchantUserIn(C3DMap* pMap, int region_x, int region_z, Packet & pkt, uint16 & t_count);

	void Send_Region(Packet *pkt, C3DMap *pMap, int x, int z, CUser* pExceptUser = NULL);
	void Send_UnitRegion(Packet *pkt, C3DMap *pMap, int x, int z, CUser* pExceptUser = NULL);
	void Send_OldRegions(Packet *pkt, int old_x, int old_z, C3DMap *pMap, int x, int z, CUser* pExceptUser = NULL);
	void Send_NewRegions(Packet *pkt, int new_x, int new_z, C3DMap *pMap, int x, int z, CUser* pExceptUser = NULL);

	void Send_NearRegion(Packet *pkt, C3DMap *pMap, int region_x, int region_z, float curx, float curz, CUser* pExceptUser=NULL );
	void Send_FilterUnitRegion(Packet *pkt, C3DMap *pMap, int x, int z, float ref_x, float ref_z, CUser* pExceptUser=NULL );

	void Send_All( char* pBuf, int len, CUser* pExceptUser = NULL, int nation=0 );	// PENDING DEPRECATION
	void Send_All(Packet *pkt, CUser* pExceptUser = NULL, uint8 nation = 0);
	void Send_AIServer(Packet *pkt);

	CString GetServerResource(int nResourceID);
	_START_POSITION *GetStartPosition(int nZoneID);

	long GetExpByLevel(int nLevel);
	C3DMap * GetZoneByID(int zoneID);

	CUser * GetUserPtr(const char* userid, NameType type);
	CUser * GetUserPtr(int sid);
	__forceinline CUser * GetUnsafeUserPtr(int sid);

	CKnights * GetClanPtr(uint16 sClanID);
	_ITEM_TABLE * GetItemPtr(uint32 nItemID);

	_PARTY_GROUP * CreateParty(CUser *pLeader);
	void DeleteParty(short sIndex);

	void AddToList(const char * format, ...);
	void WriteLog(const char * format, ...);

	CEbenezerDlg(CWnd* pParent = NULL);	// standard constructor

	static CIOCPort	m_Iocport;

	CSharedMemQueue	m_LoggerSendQueue, m_LoggerRecvQueue;

	HANDLE	m_hReadQueueThread;
	HANDLE	m_hMMFile;
	char*	m_lpMMFile;
	BOOL	m_bMMFCreate;
	DWORD	m_ServerOffset;

	char	m_ppNotice[20][128];
	char	m_AIServerIP[20];

	AISocketArray			m_AISocketArray;
	NpcArray				m_arNpcArray;
	ZoneArray				m_ZoneArray;
	ItemtableArray			m_ItemtableArray;
	MagictableArray			m_MagictableArray;
	Magictype1Array			m_Magictype1Array;
	Magictype2Array         m_Magictype2Array;
	Magictype3Array			m_Magictype3Array;
	Magictype4Array			m_Magictype4Array;
	Magictype5Array         m_Magictype5Array;
	Magictype6Array         m_Magictype6Array;
	Magictype7Array         m_Magictype7Array;
	Magictype8Array         m_Magictype8Array;
	Magictype9Array         m_Magictype9Array;
	CoefficientArray		m_CoefficientArray;
	LevelUpArray			m_LevelUpArray;
	PartyArray				m_PartyArray;
	KnightsArray			m_KnightsArray;
	HomeArray				m_HomeArray;
	StartPositionArray		m_StartPositionArray;
	QuestArray				m_Event;
	ServerResourceArray		m_ServerResourceArray;
	BlockNameArray			m_BlockNameArray;

	CKnightsManager			m_KnightsManager;

	short	m_sPartyIndex;
	short	m_sZoneCount;							// AI Server 재접속시 사용
	short	m_sSocketCount;							// AI Server 재접속시 사용
	// sungyong 2002.05.23
	short   m_sSendSocket;			
	BOOL	m_bFirstServerFlag;		// 서버가 처음시작한 후 게임서버가 붙은 경우에는 1, 붙지 않은 경우 0
	BOOL	m_bServerCheckFlag;
	BOOL	m_bPointCheckFlag;		// AI서버와 재접전에 NPC포인터 참조막기 (TRUE:포인터 참조, FALSE:포인터 참조 못함)
	short   m_sReSocketCount;		// GameServer와 재접시 필요
	float   m_fReConnectStart;	// 처음 소켓이 도착한 시간
	short   m_sErrorSocketCount;  // 이상소켓 감시용
	// ~sungyong 2002.05.23

	uint16 m_nYear, m_nMonth, m_nDate, m_nHour, m_nMin;
	uint8 m_nWeather;
	uint16 m_nAmount;
	int m_nCastleCapture;

	// ~Yookozuna 2002.06.12
	BYTE    m_byBattleOpen, m_byOldBattleOpen;					// 0:전쟁중이 아님, 1:전쟁중(국가간전쟁), 2:눈싸움전쟁
	BYTE	m_bVictory, m_byOldVictory;
	BYTE	m_bKarusFlag, m_bElmoradFlag;
	BYTE    m_byKarusOpenFlag, m_byElmoradOpenFlag, m_byBanishFlag, m_byBattleSave;
	short   m_sDiscount;	// 능력치와 포인트 초기화 할인 (0:할인없음, 1:할인(50%) )
	short	m_sKarusDead, m_sElmoradDead, m_sBanishDelay, m_sKarusCount, m_sElmoradCount;
	int m_nBattleZoneOpenWeek, m_nBattleZoneOpenHourStart, m_nBattleZoneOpenHourEnd;
	char m_strKarusCaptain[MAX_ID_SIZE+1];
	char m_strElmoradCaptain[MAX_ID_SIZE+1];

	// ~Yookozuna 2002.07.17
	BYTE	m_bMaxRegenePoint;

	// ~Yookozuna 2002.11.26 - 비러머글 남는 공지 --;
	BOOL	m_bPermanentChatMode;
	BOOL	m_bPermanentChatFlag;
	char	m_strPermanentChat[1024];

	// ~Yookozuna 2002.12.11 - 갓댐 산타 클로스 --;
	BOOL	m_bSanta;

	// zone server info
	int					m_nServerNo, m_nServerGroupNo;
	int					m_nServerGroup;	// server의 번호(0:서버군이 없다, 1:서버1군, 2:서버2군)
	ServerArray			m_ServerArray;
	ServerArray			m_ServerGroupArray;
	CUdpSocket*			m_pUdpSocket;
	CFile m_RegionLogFile;
	CFile m_LogFile;
	CFile m_EvnetLogFile;
// Dialog Data
	//{{AFX_DATA(CEbenezerDlg)
	enum { IDD = IDD_EBENEZER_DIALOG };
	CEdit	m_AnnounceEdit;
	CListBox	m_StatusList;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEbenezerDlg)
	public:
	virtual BOOL DestroyWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

private:
	CIni	m_Ini;
	CDatabase m_GameDB;

	BOOL LoadTables();
	BOOL ConnectToDatabase(bool reconnect = false);

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CEbenezerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	friend class C3DMap;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EBENEZERDLG_H__655A21EF_E029_42C0_890A_68DA7F542428__INCLUDED_)
