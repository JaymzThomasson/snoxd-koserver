#pragma once

#include "../shared/KOSocketMgr.h"
#include "../shared/database/OdbcConnection.h"

#include "GameSocket.h"

#include "MAP.h"
#include "Pathfind.h"

#include "../shared/STLMap.h"

class CNpcThread;
class CNpcTable;

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
typedef CSTLMap <_K_MONSTER_ITEM>			NpcItemArray;
typedef CSTLMap <_MAKE_ITEM_GROUP>			MakeItemGroupArray;

class CServerDlg
{
private:
	void ResumeAI();
	bool CreateNpcThread();
	bool GetMagicTableData();
	bool GetMagicType1Data();
	bool GetMagicType2Data();
	bool GetMagicType3Data();
	bool GetMagicType4Data();
	bool GetNpcTableData(bool bNpcData = true);
	bool GetNpcItemTable();
	bool GetMakeItemGroupTable();
	bool GetMakeWeaponItemTableData();
	bool GetMakeDefensiveItemTableData();
	bool GetMakeGradeItemTableData();
	bool GetMakeLareItemTableData();
	bool MapFileLoad();
	void GetServerInfoIni();
	
public:
	CServerDlg();
	bool Startup();

	bool LoadSpawnCallback(OdbcCommand *dbCommand);
	void GameServerAcceptThread();
	bool AddObjectEventNpc(_OBJECT_EVENT* pEvent, MAP * pMap);
	void AllNpcInfo();
	CUser* GetUserPtr(int nid);
	CNpc*  GetEventNpcPtr();
	bool   SetSummonNpcData(CNpc* pNpc, int zone, float fx, float fz);
	MAP * GetZoneByID(int zonenumber);
	int GetServerNumber( int zonenumber );

	static uint32 THREADCALL Timer_CheckAliveTest(void * lpParam);
	void CheckAliveTest();
	void DeleteUserList(int uid);
	void DeleteAllUserList(CGameSocket *pSock = nullptr);
	void Send(Packet * pkt);
	void SendSystemMsg(char* pMsg, int type=0);
	void ResetBattleZone();

	~CServerDlg();

public:
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
	MakeGradeItemTableArray	m_MakeGradeItemArray;
	MakeLareItemTableArray	m_MakeLareItemArray;
	ZoneArray				g_arZone;
	NpcItemArray			m_NpcItemArray;
	MakeItemGroupArray		m_MakeItemGroupArray;

	Thread m_zoneEventThread;

	char m_strGameDSN[32], m_strGameUID[32], m_strGamePWD[32];
	OdbcConnection m_GameDB;

	CUser* m_pUser[MAX_USER];

	uint16			m_TotalNPC;			// DB에있는 총 수
#ifdef USE_STD_ATOMIC
	std::atomic_ushort m_CurrentNPC;
#else
	long			m_CurrentNPC;
#endif
	short			m_sTotalMap;		// Zone 수 
	short			m_sMapEventNpc;		// Map에서 읽어들이는 event npc 수

	bool			m_bFirstServerFlag;		// 서버가 처음시작한 후 게임서버가 붙은 경우에는 1, 붙지 않은 경우 0
	uint8  m_byBattleEvent;				   // 전쟁 이벤트 관련 플래그( 1:전쟁중이 아님, 0:전쟁중)
	short m_sKillKarusNpc, m_sKillElmoNpc; // 전쟁동안에 죽은 npc숫자

	uint16	m_iYear, m_iMonth, m_iDate, m_iHour, m_iMin, m_iAmount;
	uint8 m_iWeather;
	uint8	m_byNight;			// 밤인지,, 낮인지를 판단... 1:낮, 2:밤

	FastMutex m_userLock, m_partyLock;

	static KOSocketMgr<CGameSocket> s_socketMgr;

private:
	uint8				m_byZone;
};

extern CServerDlg * g_pMain;