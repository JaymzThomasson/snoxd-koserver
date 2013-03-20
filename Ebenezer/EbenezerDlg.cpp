#include "stdafx.h"

#include "../shared/database/ItemTableSet.h"
#include "../shared/database/MagicTableSet.h"
#include "../shared/database/MagicType1Set.h"
#include "../shared/database/MagicType2Set.h"
#include "../shared/database/MagicType3Set.h"
#include "../shared/database/MagicType4Set.h"
#include "../shared/database/MagicType5Set.h"
#include "../shared/database/MagicType6Set.h"
#include "../shared/database/MagicType7Set.h"
#include "../shared/database/MagicType8Set.h"
#include "../shared/database/MagicType9Set.h"
#include "../shared/database/ZoneInfoSet.h"
#include "../shared/database/EventSet.h"
#include "../shared/database/CoefficientSet.h"
#include "../shared/database/LevelUpTableSet.h"
#include "../shared/database/ServerResourceSet.h"
#include "../shared/database/EventTriggerSet.h"
#include "../shared/database/QuestHelperSet.h"
#include "../shared/database/QuestMonsterSet.h"
#include "../shared/database/KnightsSet.h"
#include "../shared/database/KnightsUserSet.h"
#include "../shared/database/KnightsAllianceSet.h"
#include "../shared/database/KnightsRankSet.h"
#include "../shared/database/KnightsCapeSet.h"
#include "../shared/database/UserPersonalRankSet.h"
#include "../shared/database/UserKnightsRankSet.h"
#include "../shared/database/HomeSet.h"
#include "../shared/database/StartPositionSet.h"
#include "../shared/database/BattleSet.h"
#include "../shared/database/RentalItemSet.h"

using namespace std;

#define NUM_FLAG_VICTORY    4
#define AWARD_GOLD          100000
#define AWARD_EXP			5000

CDBAgent g_DBAgent;
CRITICAL_SECTION g_serial_critical, g_region_critical;

KOSocketMgr<CUser> CEbenezerDlg::s_socketMgr;
ClientSocketMgr<CAISocket> CEbenezerDlg::s_aiSocketMgr;

WORD	g_increase_serial = 1;
bool	g_bRunning = true;

static DWORD s_dwGameTimerID, s_dwAliveTimerID;

void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	g_pMain.OnTimer(idEvent);
}

CEbenezerDlg::CEbenezerDlg()
	: m_Ini("gameserver.ini")
{
	m_nYear = 0; 
	m_nMonth = 0;
	m_nDate = 0;
	m_nHour = 0;
	m_nMin = 0;
	m_nWeather = 0;
	m_nAmount = 0;
	m_sPartyIndex = 0;

	m_nCastleCapture = 0;

	m_bKarusFlag = 0;
	m_bElmoradFlag = 0;

	m_byKarusOpenFlag = m_byElmoradOpenFlag = 0;
	m_byBanishFlag = 0;
	m_sBanishDelay = 0;

	m_sKarusDead = 0;
	m_sElmoradDead = 0;

	m_bVictory = 0;	
	m_byOldVictory = 0;
	m_byBattleSave = 0;
	m_sKarusCount = 0;
	m_sElmoradCount = 0;

	m_nBattleZoneOpenWeek=m_nBattleZoneOpenHourStart=m_nBattleZoneOpenHourEnd = 0;

	m_byBattleZone = 0;
	m_byBattleOpen = NO_BATTLE;
	m_byOldBattleOpen = NO_BATTLE;
	m_bFirstServerFlag = FALSE;
	// m_bPointCheckFlag = FALSE;
	m_bPointCheckFlag = TRUE;

	m_nServerNo = 0;
	m_nServerGroupNo = 0;
	m_nServerGroup = 0;
	m_sDiscount = 0;

	m_pUdpSocket = NULL;

	memset( m_AIServerIP, NULL, 20 );

	m_bPermanentChatMode = FALSE;
	memset( m_strKarusCaptain, 0x00, MAX_ID_SIZE+1 );
	memset( m_strElmoradCaptain, 0x00, MAX_ID_SIZE+1 );

	memset(m_strGameDSN, 0, sizeof(m_strGameDSN));
	memset(m_strGameUID, 0, sizeof(m_strGameUID));
	memset(m_strGamePWD, 0, sizeof(m_strGamePWD));
	memset(m_strAccountDSN, 0, sizeof(m_strAccountDSN));
	memset(m_strAccountUID, 0, sizeof(m_strAccountUID));
	memset(m_strAccountPWD, 0, sizeof(m_strAccountPWD));

	m_bSantaOrAngel = FLYING_NONE;
}

bool CEbenezerDlg::Startup()
{
	m_sZoneCount = 0;
	m_sErrorSocketCount = 0;

	m_bFirstServerFlag = FALSE;	
	m_bServerCheckFlag = FALSE;

	InitializeCriticalSection( &g_region_critical );
	InitializeCriticalSection( &g_serial_critical );

	GetTimeFromIni();
	
	if (!s_socketMgr.Listen(_LISTEN_PORT, MAX_USER))
	{
		printf(_T("ERROR: Failed to listen on server port (%d)."), _LISTEN_PORT);
		return false;
	}

	// Bit tacky, but there's no reason we can't reuse the existing completion port for our AI socket
	s_aiSocketMgr.SetCompletionPort(s_socketMgr.GetCompletionPort());
	s_aiSocketMgr.InitSessions(1);

	if (!g_DBAgent.Startup()
		|| !LoadTables()
		|| !MapFileLoad())
		return false;

	LoadNoticeData();

	srand((unsigned int)time(NULL));

#if 0 // Disabled pending rewrite
	m_pUdpSocket = new CUdpSocket();
	if (!m_pUdpSocket->CreateSocket())
	{
		printf("ERROR: UDP socket could not be created.\n");
		return FALSE;
	}
#endif

	AIServerConnect();

	// Initialise the command tables
	InitServerCommands();
	CUser::InitChatCommands();

	s_socketMgr.RunServer();

	return true; 
}

bool CEbenezerDlg::LoadTables()
{
	return (LoadItemTable()
			&& LoadServerResourceTable()
			&& LoadEventTriggerTable()
			&& LoadQuestHelperTable()
			&& LoadQuestMonsterTable()
			&& LoadMagicTable()
			&& LoadMagicType1()
			&& LoadMagicType2()
			&& LoadMagicType3()
			&& LoadMagicType4()
			&& LoadMagicType5()
			&& LoadMagicType6()
			&& LoadMagicType7()
			&& LoadMagicType8()
			&& LoadMagicType9()
			&& LoadRentalList()
			&& LoadCoefficientTable()
			&& LoadLevelUpTable()
			&& LoadAllKnights()
			&& LoadAllKnightsUserData()
			&& LoadKnightsAllianceTable()
			&& LoadUserRankings()
			&& LoadKnightsCapeTable()
			&& LoadHomeTable()
			&& LoadStartPositionTable()
			&& LoadBattleTable());
}

void CEbenezerDlg::GetServerResource(int nResourceID, string & result, ...)
{
	_SERVER_RESOURCE *pResource = m_ServerResourceArray.GetData(nResourceID);
	if (pResource == NULL)
	{
		result = nResourceID;	
		return;
	}

	va_list args;
	va_start(args, result);
	result = _string_format(pResource->strResource, args);
	va_end(args);
}

_START_POSITION *CEbenezerDlg::GetStartPosition(int nZoneID)
{
	return m_StartPositionArray.GetData(nZoneID);
}

long CEbenezerDlg::GetExpByLevel(int nLevel)
{
	LevelUpArray::iterator itr = m_LevelUpArray.find(nLevel);
	if (itr != m_LevelUpArray.end())
		return itr->second;

	return 0;
}

C3DMap * CEbenezerDlg::GetZoneByID(int zoneID)
{
	return m_ZoneArray.GetData(zoneID);
}

CUser* CEbenezerDlg::GetUserPtr(string findName, NameType type)
{
	// As findName is a copy of the string passed in, we can change it
	// without worry of affecting anything.
	STRTOUPPER(findName);

	NameMap::iterator itr;
	if (type == TYPE_ACCOUNT)
	{
		FastGuard lock(m_accountNameLock);
		itr = m_accountNameMap.find(findName);
		return (itr != m_accountNameMap.end() ? itr->second : NULL);
	}
	else if (type == TYPE_CHARACTER)
	{
		FastGuard lock(m_characterNameLock);
		itr = m_characterNameMap.find(findName);
		return (itr != m_characterNameMap.end() ? itr->second : NULL);
	}

	return NULL;
}

// Adds the account name & session to a hashmap (on login)
void CEbenezerDlg::AddAccountName(CUser *pSession)
{
	FastGuard lock(m_accountNameLock);
	string upperName = pSession->m_strAccountID;
	STRTOUPPER(upperName);
	m_accountNameMap[upperName] = pSession;
}

// Adds the character name & session to a hashmap (when in-game)
void CEbenezerDlg::AddCharacterName(CUser *pSession)
{
	FastGuard lock(m_characterNameLock);
	string upperName = pSession->GetName();
	STRTOUPPER(upperName);
	m_characterNameMap[upperName] = pSession;
}

// Removes the account name & character names from the hashmaps (on logout)
void CEbenezerDlg::RemoveSessionNames(CUser *pSession)
{
	string upperName = pSession->m_strAccountID;
	STRTOUPPER(upperName);

	{ // remove account name from map (limit scope)
		FastGuard lock(m_accountNameLock);
		m_accountNameMap.erase(upperName);
	}

	if (pSession->isInGame())
	{
		upperName = pSession->GetName();
		STRTOUPPER(upperName);

		FastMutex lock(m_characterNameLock);
		m_characterNameMap.erase(upperName);
	}
}

CUser				* CEbenezerDlg::GetUserPtr(int sid) { return s_socketMgr[sid]; }
CKnights			* CEbenezerDlg::GetClanPtr(uint16 sClanID) { return m_KnightsArray.GetData(sClanID); }
_KNIGHTS_ALLIANCE	* CEbenezerDlg::GetAlliancePtr(uint16 sAllianceID) { return m_KnightsAllianceArray.GetData(sAllianceID); }
_ITEM_TABLE			* CEbenezerDlg::GetItemPtr(uint32 nItemID) { return m_ItemtableArray.GetData(nItemID); }

Unit * CEbenezerDlg::GetUnit(uint16 id)
{
	if (id < NPC_BAND)
		return GetUserPtr(id);

	return m_arNpcArray.GetData(id);
}

int32 CEbenezerDlg::GetEventTrigger(CNpc * pNpc)
{
	NpcTrapPair key(pNpc->m_byTrapNumber, pNpc->GetID());
	EventTriggerArray::iterator itr = m_EventTriggerArray.find(key);
	if (itr == m_EventTriggerArray.end())
		return -1;

	return itr->second;
}

_PARTY_GROUP * CEbenezerDlg::CreateParty(CUser *pLeader)
{
	// Protect party ID generation
	EnterCriticalSection(&g_region_critical);
	pLeader->m_sPartyIndex = m_sPartyIndex++;
	if (m_sPartyIndex == SHRT_MAX)
		m_sPartyIndex = 0;
	LeaveCriticalSection(&g_region_critical);

	_PARTY_GROUP * pParty = new _PARTY_GROUP;
	pParty->wIndex = pLeader->m_sPartyIndex;
	pParty->uid[0] = pLeader->GetSocketID();
	if (!m_PartyArray.PutData(pParty->wIndex, pParty))
	{
		delete pParty;
		pLeader->m_sPartyIndex = -1;
		pParty = NULL;
	}

	return pParty;
}

void CEbenezerDlg::DeleteParty(short sIndex)
{
	EnterCriticalSection(&g_region_critical);
	m_PartyArray.DeleteData(sIndex);
	LeaveCriticalSection(&g_region_critical);
}

void CEbenezerDlg::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent == s_dwGameTimerID)
	{
		UpdateGameTime();
		if (++m_sErrorSocketCount > 3)
			AIServerConnect();
	}
	else if (nIDEvent == s_dwAliveTimerID)
	{
		CheckAliveUser();
	}
}

int CEbenezerDlg::GetAIServerPort()
{
	int nPort = AI_KARUS_SOCKET_PORT;
	switch (m_nServerNo)
	{
	case ELMORAD:
		nPort = AI_ELMO_SOCKET_PORT;
		break;

	case BATTLE:
		nPort = AI_BATTLE_SOCKET_PORT;
		break;
	}
	return nPort;
}

void CEbenezerDlg::AIServerConnect()
{
	// Are there any (note: we only use 1 now) idle/disconnected sessions?
	SessionMap & sessMap = s_aiSocketMgr.GetIdleSessionMap();

	// Copy the map (should only be 1 socket anyway) to avoid breaking the iterator
	SessionMap idleSessions = sessMap;
	s_aiSocketMgr.ReleaseLock();

	// No idle sessions? Excellent.
	if (idleSessions.empty())
		return;

	int port = GetAIServerPort();

	// Attempt reconnecting to the server
	foreach (itr, idleSessions)
	{
		CAISocket *pSock = static_cast<CAISocket *>(itr->second);
		bool bReconnecting = pSock->IsReconnecting();
		if (!pSock->Connect(m_AIServerIP, port)) // couldn't connect... let's leave you alone for now
			continue;

		// Connected! Now send the connection packet.
		Packet result(AI_SERVER_CONNECT);
		result << bReconnecting;
		pSock->Send(&result);

		TRACE("**** AISocket Connect Success!! , server = %s:%d ****\n", pSock->GetRemoteIP().c_str(), pSock->GetRemotePort());
	}

	// This check seems redundant, but it isn't: AISocketConnect() should change the map.
	// We're deliberately checking after we've attempted to reconnect.
	// The idle session(s) should be removed, if they're still unable to connect... reset the server's NPCs.
	s_aiSocketMgr.AcquireLock();
	if (!sessMap.empty())
		DeleteAllNpcList();
	s_aiSocketMgr.ReleaseLock();
}

void CEbenezerDlg::Send_All(Packet *pkt, CUser* pExceptUser /*= NULL*/, uint8 nation /*= 0*/)
{
	SessionMap & sessMap = s_socketMgr.GetActiveSessionMap();
	foreach (itr, sessMap)
	{
		CUser * pUser = TO_USER(itr->second);
		if (pUser == pExceptUser 
			|| !pUser->isInGame()
			|| (nation != 0 && nation != pUser->GetNation()))
			continue;

		pUser->Send(pkt);
	}
	s_socketMgr.ReleaseLock();
}

void CEbenezerDlg::Send_Region(Packet *pkt, C3DMap *pMap, int x, int z, CUser* pExceptUser)
{
	foreach_region(rx, rz)
		Send_UnitRegion(pkt, pMap, rx + x, rz + z, pExceptUser);
}

void CEbenezerDlg::Send_UnitRegion(Packet *pkt, C3DMap *pMap, int x, int z, CUser *pExceptUser)
{
	if (pMap == NULL 
		|| x < 0 || z < 0 || x > pMap->GetXRegionMax() || z > pMap->GetZRegionMax())
		return;

	EnterCriticalSection(&g_region_critical);
	CRegion *pRegion = &pMap->m_ppRegion[x][z];

	foreach (itr, pRegion->m_RegionUserArray)
	{
		CUser *pUser = GetUserPtr(*itr);
		if (pUser == NULL 
			|| pUser == pExceptUser 
			|| !pUser->isInGame())
			continue;

		pUser->Send(pkt);
	}
	LeaveCriticalSection(&g_region_critical);
}

// TO-DO: Move the following two methods into a base CUser/CNpc class
void CEbenezerDlg::Send_OldRegions(Packet *pkt, int old_x, int old_z, C3DMap *pMap, int x, int z, CUser* pExceptUser)
{
	if (old_x != 0)
	{
		Send_UnitRegion(pkt, pMap, x+old_x*2, z+old_z-1);
		Send_UnitRegion(pkt, pMap, x+old_x*2, z+old_z);
		Send_UnitRegion(pkt, pMap, x+old_x*2, z+old_z+1);
	}

	if (old_z != 0)
	{
		Send_UnitRegion(pkt, pMap, x+old_x, z+old_z*2);
		if (old_x < 0)
			Send_UnitRegion(pkt, pMap, x+old_x+1, z+old_z*2);
		else if (old_x > 0)
			Send_UnitRegion(pkt, pMap, x+old_x-1, z+old_z*2);
		else
		{
			Send_UnitRegion(pkt, pMap, x+old_x-1, z+old_z*2);
			Send_UnitRegion(pkt, pMap, x+old_x+1, z+old_z*2);
		}
	}
}

void CEbenezerDlg::Send_NewRegions(Packet *pkt, int new_x, int new_z, C3DMap *pMap, int x, int z, CUser* pExceptUser)
{
	if (new_x != 0)
	{
		Send_UnitRegion(pkt, pMap, x+new_x, z-1);
		Send_UnitRegion(pkt, pMap, x+new_x, z);
		Send_UnitRegion(pkt, pMap, x+new_x, z+1);
	}

	if (new_z != 0)
	{
		Send_UnitRegion(pkt, pMap, x, z+new_z);
		
		if (new_x < 0)
			Send_UnitRegion(pkt, pMap, x+1, z+new_z);
		else if (new_x > 0)
			Send_UnitRegion(pkt, pMap, x-1, z+new_z);
		else 
		{
			Send_UnitRegion(pkt, pMap, x-1, z+new_z);
			Send_UnitRegion(pkt, pMap, x+1, z+new_z);
		}
	}
}

void CEbenezerDlg::Send_NearRegion(Packet *pkt, C3DMap *pMap, int region_x, int region_z, float curx, float curz, CUser* pExceptUser)
{
	int left_border = region_x * VIEW_DISTANCE, top_border = region_z * VIEW_DISTANCE;
	Send_FilterUnitRegion(pkt, pMap, region_x, region_z, curx, curz, pExceptUser);
	if( ((curx - left_border) > (VIEW_DISTANCE/2.0f)) ) {			// RIGHT
		if( ((curz - top_border) > (VIEW_DISTANCE/2.0f)) ) {	// BOTTOM
			Send_FilterUnitRegion(pkt, pMap, region_x+1, region_z, curx, curz, pExceptUser);
			Send_FilterUnitRegion(pkt, pMap, region_x, region_z+1, curx, curz, pExceptUser);
			Send_FilterUnitRegion(pkt, pMap, region_x+1, region_z+1, curx, curz, pExceptUser);
		}
		else {													// TOP
			Send_FilterUnitRegion(pkt, pMap, region_x+1, region_z, curx, curz, pExceptUser);
			Send_FilterUnitRegion(pkt, pMap, region_x, region_z-1, curx, curz, pExceptUser);
			Send_FilterUnitRegion(pkt, pMap, region_x+1, region_z-1, curx, curz, pExceptUser);
		}
	}
	else {														// LEFT
		if( ((curz - top_border) > (VIEW_DISTANCE/2.0f)) ) {	// BOTTOM
			Send_FilterUnitRegion(pkt, pMap, region_x-1, region_z, curx, curz, pExceptUser);
			Send_FilterUnitRegion(pkt, pMap, region_x, region_z+1, curx, curz, pExceptUser);
			Send_FilterUnitRegion(pkt, pMap, region_x-1, region_z+1, curx, curz, pExceptUser);
		}
		else {													// TOP
			Send_FilterUnitRegion(pkt, pMap, region_x-1, region_z, curx, curz, pExceptUser);
			Send_FilterUnitRegion(pkt, pMap, region_x, region_z-1, curx, curz, pExceptUser);
			Send_FilterUnitRegion(pkt, pMap, region_x-1, region_z-1, curx, curz, pExceptUser);
		}
	}
}

void CEbenezerDlg::Send_FilterUnitRegion(Packet *pkt, C3DMap *pMap, int x, int z, float ref_x, float ref_z, CUser *pExceptUser)
{
	if (pMap == NULL
		|| x < 0 || z < 0 || x > pMap->GetXRegionMax() || z>pMap->GetZRegionMax())
		return;

	EnterCriticalSection(&g_region_critical);
	CRegion *pRegion = &pMap->m_ppRegion[x][z];

	foreach (itr, pRegion->m_RegionUserArray)
	{
		CUser *pUser = GetUserPtr(*itr);
		if (pUser == NULL 
			|| pUser == pExceptUser 
			|| !pUser->isInGame())
			continue;

		if (sqrt(pow((pUser->m_curx - ref_x), 2) + pow((pUser->m_curz - ref_z), 2)) < 32)
			pUser->Send(pkt);
	}

	LeaveCriticalSection( &g_region_critical );
}

void CEbenezerDlg::Send_PartyMember(int party, Packet *result)
{
	_PARTY_GROUP* pParty = m_PartyArray.GetData(party);
	if (pParty == NULL)
		return;

	for (int i = 0; i < MAX_PARTY_USERS; i++)
	{
		CUser *pUser = GetUserPtr(pParty->uid[i]);
		if (pUser == NULL)
			continue;

		pUser->Send(result);
	}
}

void CEbenezerDlg::Send_KnightsMember(int index, Packet *pkt)
{
	CKnights* pKnights = GetClanPtr(index);
	if (pKnights == NULL)
		return;

	pKnights->Send(pkt);
}

void CEbenezerDlg::Send_KnightsAlliance(uint16 sAllianceID, Packet *pkt)
{
	_KNIGHTS_ALLIANCE* pAlliance = GetAlliancePtr(sAllianceID);
	if (pAlliance == NULL)
		return;

	Send_KnightsMember(pAlliance->sMainAllianceKnights, pkt);
	Send_KnightsMember(pAlliance->sSubAllianceKnights, pkt);
	Send_KnightsMember(pAlliance->sMercenaryClan_1, pkt);
	Send_KnightsMember(pAlliance->sMercenaryClan_2, pkt);
}

void CEbenezerDlg::Send_AIServer(Packet *pkt)
{
	s_aiSocketMgr.SendAll(pkt);
}

BOOL CEbenezerDlg::MapFileLoad()
{
	ZoneInfoMap zoneMap;
	LOAD_TABLE_ERROR_ONLY(CZoneInfoSet, &g_DBAgent.m_GameDB, &zoneMap, false); 

	foreach (itr, zoneMap)
	{
		C3DMap *pMap = new C3DMap();
		_ZONE_INFO *pZone = itr->second;
		if (!pMap->Initialize(pZone))
		{
			printf("ERROR: Unable to load SMD - %s\n", pZone->m_MapName);
			delete pZone;
			delete pMap;
			m_ZoneArray.DeleteAllData();
			return false;
		}

		delete pZone;
		m_ZoneArray.PutData(pMap->m_nZoneNumber, pMap);

		EVENT * pEvent = new EVENT;
		if (!pEvent->LoadEvent(pMap->m_nZoneNumber)
			|| !m_Event.PutData(pEvent->m_Zone, pEvent))
			delete pEvent;
	}

	LOAD_TABLE_ERROR_ONLY(CEventSet, &g_DBAgent.m_GameDB, &m_ZoneArray, true);
	return TRUE;
}

BOOL CEbenezerDlg::LoadItemTable()
{
	LOAD_TABLE(CItemTableSet, &g_DBAgent.m_GameDB, &m_ItemtableArray, false);
}

BOOL CEbenezerDlg::LoadServerResourceTable()
{
	LOAD_TABLE(CServerResourceSet, &g_DBAgent.m_GameDB, &m_ServerResourceArray, false);
}

BOOL CEbenezerDlg::LoadEventTriggerTable()
{
	LOAD_TABLE(CEventTriggerSet, &g_DBAgent.m_GameDB, &m_EventTriggerArray, true);
}

BOOL CEbenezerDlg::LoadQuestHelperTable()
{
	LOAD_TABLE(CQuestHelperSet, &g_DBAgent.m_GameDB, &m_QuestHelperArray, true);
}

BOOL CEbenezerDlg::LoadQuestMonsterTable()
{
	LOAD_TABLE(CQuestMonsterSet, &g_DBAgent.m_GameDB, &m_QuestMonsterArray, true);
}

BOOL CEbenezerDlg::LoadMagicTable()
{
	LOAD_TABLE(CMagicTableSet, &g_DBAgent.m_GameDB, &m_MagictableArray, false);
}

BOOL CEbenezerDlg::LoadMagicType1()
{
	LOAD_TABLE(CMagicType1Set, &g_DBAgent.m_GameDB, &m_Magictype1Array, false);
}

BOOL CEbenezerDlg::LoadMagicType2()
{
	LOAD_TABLE(CMagicType2Set, &g_DBAgent.m_GameDB, &m_Magictype2Array, false);
}

BOOL CEbenezerDlg::LoadMagicType3()
{
	LOAD_TABLE(CMagicType3Set, &g_DBAgent.m_GameDB, &m_Magictype3Array, false);
}

BOOL CEbenezerDlg::LoadMagicType4()
{
	LOAD_TABLE(CMagicType4Set, &g_DBAgent.m_GameDB, &m_Magictype4Array, false);
}

BOOL CEbenezerDlg::LoadMagicType5()
{
	LOAD_TABLE(CMagicType5Set, &g_DBAgent.m_GameDB, &m_Magictype5Array, false);
}

BOOL CEbenezerDlg::LoadMagicType6()
{
	LOAD_TABLE(CMagicType6Set, &g_DBAgent.m_GameDB, &m_Magictype6Array, false);
}

BOOL CEbenezerDlg::LoadMagicType7()
{
	LOAD_TABLE(CMagicType7Set, &g_DBAgent.m_GameDB, &m_Magictype7Array, false);
}

BOOL CEbenezerDlg::LoadMagicType8()
{
	LOAD_TABLE(CMagicType8Set, &g_DBAgent.m_GameDB, &m_Magictype8Array, false);
}

BOOL CEbenezerDlg::LoadMagicType9()
{
	LOAD_TABLE(CMagicType9Set, &g_DBAgent.m_GameDB, &m_Magictype9Array, false);
}

BOOL CEbenezerDlg::LoadRentalList()
{
	LOAD_TABLE(CRentalItemSet, &g_DBAgent.m_GameDB, &m_RentalItemArray, true);
}

BOOL CEbenezerDlg::LoadCoefficientTable()
{
	LOAD_TABLE(CCoefficientSet, &g_DBAgent.m_GameDB, &m_CoefficientArray, false);
}

BOOL CEbenezerDlg::LoadLevelUpTable()
{
	LOAD_TABLE(CLevelUpTableSet, &g_DBAgent.m_GameDB, &m_LevelUpArray, false);
}

void CEbenezerDlg::GetTimeFromIni()
{
	int year=0, month=0, date=0, hour=0, server_count=0, sgroup_count = 0, i=0;
	char ipkey[20];

	m_Ini.GetString("ODBC", "GAME_DSN", "KN_online", m_strGameDSN, sizeof(m_strGameDSN), false);
	m_Ini.GetString("ODBC", "GAME_UID", "knight", m_strGameUID, sizeof(m_strGameUID), false);
	m_Ini.GetString("ODBC", "GAME_PWD", "knight", m_strGamePWD, sizeof(m_strGamePWD), false);
	m_bMarsEnabled = m_Ini.GetBool("ODBC", "GAME_MARS", true);

	m_Ini.GetString("ODBC", "ACCOUNT_DSN", "KN_online", m_strAccountDSN, sizeof(m_strAccountDSN), false);
	m_Ini.GetString("ODBC", "ACCOUNT_UID", "knight", m_strAccountUID, sizeof(m_strAccountUID), false);
	m_Ini.GetString("ODBC", "ACCOUNT_PWD", "knight", m_strAccountPWD, sizeof(m_strAccountPWD), false);

	bool bMarsEnabled = m_Ini.GetBool("ODBC", "ACCOUNT_MARS", true);

	// Both need to be enabled to use MARS.
	if (!m_bMarsEnabled || !bMarsEnabled)
		m_bMarsEnabled = false;
	
	m_nYear = m_Ini.GetInt("TIMER", "YEAR", 1);
	m_nMonth = m_Ini.GetInt("TIMER", "MONTH", 1);
	m_nDate = m_Ini.GetInt("TIMER", "DATE", 1);
	m_nHour = m_Ini.GetInt("TIMER", "HOUR", 1);
	m_nWeather = m_Ini.GetInt("TIMER", "WEATHER", 1);

	m_nBattleZoneOpenWeek  = m_Ini.GetInt("BATTLE", "WEEK", 5);
	m_nBattleZoneOpenHourStart  = m_Ini.GetInt("BATTLE", "START_TIME", 20);
	m_nBattleZoneOpenHourEnd  = m_Ini.GetInt("BATTLE", "END_TIME", 0);

	m_nCastleCapture = m_Ini.GetInt("CASTLE", "NATION", 1);
	m_nServerNo = m_Ini.GetInt("ZONE_INFO", "MY_INFO", 1);
	m_nServerGroup = m_Ini.GetInt("ZONE_INFO", "SERVER_NUM", 0);
	server_count = m_Ini.GetInt("ZONE_INFO", "SERVER_COUNT", 1);
	if (server_count < 1)
	{
		printf("ERROR: Invalid SERVER_COUNT property in INI.\n");
		return;
	}

	for( i=0; i<server_count; i++ ) {
		_ZONE_SERVERINFO *pInfo = new _ZONE_SERVERINFO;
		sprintf( ipkey, "SERVER_%02d", i );
		pInfo->sServerNo = m_Ini.GetInt("ZONE_INFO", ipkey, 1);
		sprintf( ipkey, "SERVER_IP_%02d", i );
		m_Ini.GetString("ZONE_INFO", ipkey, "127.0.0.1", pInfo->strServerIP, sizeof(pInfo->strServerIP));
		m_ServerArray.PutData(pInfo->sServerNo, pInfo);
	}

	if( m_nServerGroup != 0 )	{
		m_nServerGroupNo = m_Ini.GetInt("SG_INFO", "GMY_INFO", 1);
		sgroup_count = m_Ini.GetInt("SG_INFO", "GSERVER_COUNT", 1);
		if (server_count < 1)
		{
			printf("ERROR: Invalid GSERVER_COUNT property in INI.\n");
			return;
		}
		for( i=0; i<sgroup_count; i++ ) {
			_ZONE_SERVERINFO *pInfo = new _ZONE_SERVERINFO;
			sprintf( ipkey, "GSERVER_%02d", i );
			pInfo->sServerNo = m_Ini.GetInt("SG_INFO", ipkey, 1);
			sprintf( ipkey, "GSERVER_IP_%02d", i );
			m_Ini.GetString("SG_INFO", ipkey, "127.0.0.1", pInfo->strServerIP, sizeof(pInfo->strServerIP));

			m_ServerGroupArray.PutData(pInfo->sServerNo, pInfo);
		}
	}

	m_Ini.GetString("AI_SERVER", "IP", "127.0.0.1", m_AIServerIP, sizeof(m_AIServerIP));

	s_dwGameTimerID = SetTimer(NULL, 1, 6000, &TimerProc);
	s_dwAliveTimerID = SetTimer(NULL, 2, 34000, &TimerProc);
}

void CEbenezerDlg::UpdateGameTime()
{
	CUser* pUser = NULL;
	BOOL bKnights = FALSE;

	m_nMin++;

	BattleZoneOpenTimer();	// Check if it's time for the BattleZone to open or end.

	if( m_nMin == 60 ) {
		m_nHour++;
		m_nMin = 0;
		UpdateWeather();
		SetGameTime();

		if (m_bSantaOrAngel)
			SendFlyingSantaOrAngel();
	}
	if( m_nHour == 24 ) {
		m_nDate++;
		m_nHour = 0;
		bKnights = TRUE;
	}
	if( m_nDate == 31 ) {
		m_nMonth++;
		m_nDate = 1;
	}
	if( m_nMonth == 13 ) {
		m_nYear++;
		m_nMonth = 1;
	}

	Packet result(AG_TIME_WEATHER);
	result << m_nYear << m_nMonth << m_nDate << m_nHour << m_nMin << m_nWeather << m_nAmount;
	Send_AIServer(&result);

	if (bKnights)
	{
		result.Initialize(WIZ_KNIGHTS_PROCESS);
		result << uint8(KNIGHTS_ALLLIST_REQ) << uint8(m_nServerNo);
		AddDatabaseRequest(result);
	}
}

void CEbenezerDlg::UpdateWeather()
{
	int weather = 0, rnd = myrand( 0, 100 );
	if (rnd < 2)		weather = WEATHER_SNOW;
	else if (rnd < 7)	weather = WEATHER_RAIN;
	else				weather = WEATHER_FINE;

	m_nAmount = myrand(0, 100);
	if (weather == WEATHER_FINE)
	{
		if (m_nAmount > 70)
			m_nAmount /= 2;
		else
			m_nAmount = 0;
	}
	m_nWeather = weather;

	Packet result(WIZ_WEATHER, m_nWeather);
	result << m_nAmount;
	Send_All(&result);
}

void CEbenezerDlg::SetGameTime()
{
	m_Ini.SetInt( "TIMER", "YEAR", m_nYear );
	m_Ini.SetInt( "TIMER", "MONTH", m_nMonth );
	m_Ini.SetInt( "TIMER", "DATE", m_nDate );
	m_Ini.SetInt( "TIMER", "HOUR", m_nHour );
	m_Ini.SetInt( "TIMER", "WEATHER", m_nWeather );
}

void CEbenezerDlg::AddDatabaseRequest(Packet & pkt, CUser *pUser /*= NULL*/)
{
	Packet *newPacket = new Packet(pkt.GetOpcode(), pkt.size() + 2);
	*newPacket << int16(pUser == NULL ? -1 : pUser->GetSocketID());
	if (pkt.size())
		newPacket->append(pkt.contents(), pkt.size());
	DatabaseThread::AddRequest(newPacket);
}

void CEbenezerDlg::UserInOutForMe(CUser *pSendUser)
{
	if (pSendUser == NULL)
		return;

	Packet result(WIZ_REQ_USERIN);
	C3DMap* pMap = pSendUser->GetMap();
	ASSERT(pMap != NULL);
	uint16 user_count = 0;

	result << uint16(0); // placeholder for the user count

	int16 rx = pSendUser->GetRegionX(), rz = pSendUser->GetRegionZ();
	foreach_region(x, z)
		GetRegionUserIn(pMap, rx + x, rz + z, result, user_count);

	result.put(0, uint16(user_count));
	pSendUser->SendCompressed(&result);
}

void CEbenezerDlg::RegionUserInOutForMe(CUser *pSendUser)
{
	if (pSendUser == NULL)
		return;

	Packet result(WIZ_REGIONCHANGE, uint8(1));
	C3DMap* pMap = pSendUser->GetMap();
	ASSERT(pMap != NULL);
	uint16 user_count = 0;

	result << uint16(0); // placeholder for the user count

	int16 rx = pSendUser->GetRegionX(), rz = pSendUser->GetRegionZ();
	foreach_region(x, z)
		GetRegionUserList(pMap, rx + x, rz + z, result, user_count);

	result.put(1, user_count);
	pSendUser->SendCompressed(&result);
}

void CEbenezerDlg::GetRegionUserIn(C3DMap *pMap, uint16 region_x, uint16 region_z, Packet & pkt, uint16 & t_count)
{
	if (pMap == NULL 
		|| region_x > pMap->GetXRegionMax() 
		|| region_z > pMap->GetZRegionMax())
		return;

	EnterCriticalSection(&g_region_critical);
	CRegion *pRegion = &pMap->m_ppRegion[region_x][region_z];

	foreach (itr, pRegion->m_RegionUserArray)
	{
		CUser *pUser = GetUserPtr(*itr);
		if (pUser == NULL 
			|| !pUser->isInGame()
			|| pUser->GetRegionX() != region_x || pUser->GetRegionZ() != region_z)
			continue;

		pkt << uint8(0) << pUser->GetSocketID();
		pUser->GetUserInfo(pkt);
		t_count++;
	}

	LeaveCriticalSection(&g_region_critical);
}

void CEbenezerDlg::GetRegionUserList(C3DMap* pMap, uint16 region_x, uint16 region_z, Packet & pkt, uint16 & t_count)
{
	if (pMap == NULL 
		|| region_x > pMap->GetXRegionMax() 
		|| region_z > pMap->GetZRegionMax())
		return;

	EnterCriticalSection(&g_region_critical);
	CRegion *pRegion = &pMap->m_ppRegion[region_x][region_z];

	foreach (itr, pRegion->m_RegionUserArray)
	{
		CUser *pUser = GetUserPtr(*itr);
		if (pUser == NULL 
			|| !pUser->isInGame()
			|| pUser->GetRegionX() != region_x || pUser->GetRegionZ() != region_z)
			continue;

		pkt << pUser->GetSocketID();
		t_count++;
	}

	LeaveCriticalSection(&g_region_critical);
}

void CEbenezerDlg::MerchantUserInOutForMe(CUser *pSendUser)
{
	if (pSendUser == NULL)
		return;

	Packet result(WIZ_MERCHANT_INOUT, uint8(1));
	C3DMap* pMap = pSendUser->GetMap();
	ASSERT(pMap != NULL);
	uint16 user_count = 0;

	result << uint16(0); // placeholder for user count

	int16 rx = pSendUser->GetRegionX(), rz = pSendUser->GetRegionZ();
	foreach_region(x, z)
		GetRegionMerchantUserIn(pMap, rx + x, rz + z, result, user_count);

	result.put(1, user_count);
	pSendUser->SendCompressed(&result);
}

void CEbenezerDlg::GetRegionMerchantUserIn(C3DMap *pMap, uint16 region_x, uint16 region_z, Packet & pkt, uint16 & t_count)
{
	if (pMap == NULL 
		|| region_x > pMap->GetXRegionMax() 
		|| region_z > pMap->GetZRegionMax())
		return;

	EnterCriticalSection(&g_region_critical);

	CRegion *pRegion = &pMap->m_ppRegion[region_x][region_z];

	foreach (itr, pRegion->m_RegionUserArray)
	{
		CUser *pUser = GetUserPtr(*itr);
		if (pUser == NULL 
			|| !pUser->isInGame()
			|| pUser->GetRegionX() != region_x || pUser->GetRegionZ() != region_z 
			|| !pUser->isMerchanting())
			continue;

		pkt << pUser->GetSocketID()
			<< pUser->GetMerchantState() // 0 is selling, 1 is buyin
			<< uint8(0); // Type of merchant [normal - gold] // bool

		for (int i = 0; i < 4; i++)
			pkt << pUser->m_arMerchantItems[i].nNum;

		t_count++;
	}

	LeaveCriticalSection(&g_region_critical);
}

void CEbenezerDlg::NpcInOutForMe(CUser* pSendUser)
{
	if (pSendUser == NULL)
		return;

	Packet result(WIZ_REQ_NPCIN);
	C3DMap* pMap = pSendUser->GetMap();
	ASSERT(pMap != NULL);
	uint16 npc_count = 0;
	result << uint16(0); // placeholder for NPC count

	int16 rx = pSendUser->GetRegionX(), rz = pSendUser->GetRegionZ();
	foreach_region(x, z)
		GetRegionNpcIn(pMap, rx + x, rz + z, result, npc_count);

	result.put(0, npc_count);
	pSendUser->SendCompressed(&result);
}

void CEbenezerDlg::GetRegionNpcIn(C3DMap *pMap, uint16 region_x, uint16 region_z, Packet & pkt, uint16 & t_count)
{
	if (!m_bPointCheckFlag
		|| pMap == NULL
		|| region_x > pMap->GetXRegionMax() 
		|| region_z > pMap->GetZRegionMax())
		return;

	EnterCriticalSection(&g_region_critical);
	foreach (itr, pMap->m_ppRegion[region_x][region_z].m_RegionNpcArray)
	{
		CNpc *pNpc = m_arNpcArray.GetData(*itr);
		if (pNpc == NULL
			|| pNpc->GetRegionX() != region_x || pNpc->GetRegionZ() != region_z
			|| pNpc->isDead())
			continue;

		pkt << pNpc->GetID();
		pNpc->GetNpcInfo(pkt);
		t_count++;
	}

	LeaveCriticalSection(&g_region_critical);
}

void CEbenezerDlg::RegionNpcInfoForMe(CUser *pSendUser)
{
	if (pSendUser == NULL)
		return;

	Packet result(WIZ_NPC_REGION);
	C3DMap* pMap = pSendUser->GetMap();
	ASSERT(pMap != NULL);
	uint16 npc_count = 0;
	result << uint16(0); // placeholder for NPC count

	int16 rx = pSendUser->GetRegionX(), rz = pSendUser->GetRegionZ();
	foreach_region(x, z)
		GetRegionNpcList(pMap, rx + x, rz + z, result, npc_count);

	result.put(0, npc_count);
	pSendUser->SendCompressed(&result);
}

void CEbenezerDlg::GetRegionNpcList(C3DMap *pMap, uint16 region_x, uint16 region_z, Packet & pkt, uint16 & t_count)
{
	if (!m_bPointCheckFlag
		|| pMap == NULL
		|| region_x > pMap->GetXRegionMax() 
		|| region_z > pMap->GetZRegionMax())
		return;

	EnterCriticalSection(&g_region_critical);
	foreach (itr, pMap->m_ppRegion[region_x][region_z].m_RegionNpcArray)
	{
		CNpc *pNpc = m_arNpcArray.GetData(*itr);
		if (pNpc == NULL || pNpc->isDead())
			continue;

		pkt << pNpc->GetID();
		t_count++;
	}

	LeaveCriticalSection(&g_region_critical);
}

#if 0 // TO-DO: Reimplement as console input thread
BOOL CEbenezerDlg::PreTranslateMessage(MSG* pMsg) 
{
	char chatstr[256];

	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE)
			return TRUE;

		if (pMsg->wParam == VK_RETURN)
		{
			std::string message = chatstr;
			if (message.empty())
				return TRUE;

			m_AnnounceEdit.SetWindowText("");
			UpdateData(FALSE);

			if (ProcessServerCommand(message))
				return TRUE;

			if (message.size() <= 256)
				SendNotice(message.c_str());
			return TRUE;
		}
	}
}
#endif

void CEbenezerDlg::SendNotice(const char *msg, uint8 bNation /*= 0*/)
{
	Packet data(WIZ_CHAT);
	string buffer;

	
	GetServerResource(IDP_ANNOUNCEMENT, buffer, msg);
	data  << uint8(PUBLIC_CHAT)		// chat type 
		  << uint8(1)				// nation
		  << int16(-1)				// session ID
		  << uint8(0)				// character name length
		  << buffer;				// chat message

	Send_All(&data, NULL, bNation);
}

void CEbenezerDlg::SendFormattedNotice(const char *msg, uint8 nation, ...)
{
	char buffer[512];
	va_list ap;
	va_start(ap, nation);
	vsnprintf(buffer, sizeof(buffer), msg, ap);
	va_end(ap);

	SendNotice(buffer, nation);
}

BOOL CEbenezerDlg::LoadNoticeData()
{
	ifstream file(GetProgPath() + "Notice.txt");
	string line;
	int count = 0;

	// Clear out the notices first
	memset(&m_ppNotice, 0, sizeof(m_ppNotice));

	if (!file)
	{
		TRACE("Notice.txt could not be opened.\n");
		return FALSE;
	}

	while (!file.eof())
	{
 		if (count > 19)
		{
			TRACE("Too many lines in Notice.txt\n");
			break;
		}

		getline(file, line);
		if (line.length() > 128)
		{
			TRACE("Notice.txt contains line that exceeds the limit of 128 characters.\n");
			break;
		}

		strcpy(m_ppNotice[count++], line.c_str());
	}

	file.close();
	return TRUE;
}

void CEbenezerDlg::SendAllUserInfo()
{
	Packet result(AG_USER_INFO_ALL);
	uint8 count = 0;
	result << count; // placeholder for user count
	const int tot = 20;

	SessionMap & sessMap = s_socketMgr.GetActiveSessionMap();
	foreach (itr, sessMap)
	{
		TO_USER(itr->second)->GetUserInfoForAI(result);
		if (++count == tot)	{
			result.put(0, count);
			Send_AIServer(&result);
			count = 0;
			result.clear();
		}
	}
	s_socketMgr.ReleaseLock();

	if (count != 0 && count < (tot - 1))
	{
		result.put(0, count);
		Send_AIServer(&result);
		count = 0;
		result.clear();
	}

	EnterCriticalSection( &g_region_critical );

	foreach_stlmap (itr, m_PartyArray)
	{
		_PARTY_GROUP *pParty = itr->second;
		if (pParty == NULL) 
			continue;

		result.Initialize(AG_PARTY_INFO_ALL);
		result << uint16(itr->first);
		for (int i = 0; i < MAX_PARTY_USERS; i++)
			result << pParty->uid[i];

		Send_AIServer(&result);
	}

	LeaveCriticalSection( &g_region_critical );
}

void CEbenezerDlg::DeleteAllNpcList(int flag)
{
	if (!m_bServerCheckFlag
		|| !m_bPointCheckFlag)
		return;

	TRACE("[Monster Point Delete]\n");
	TRACE("*** DeleteAllNpcList - Start *** \n");

	// Remove spawns from users to prevent them from getting bugged...
	foreach_stlmap (itr, m_arNpcArray)
	{
		if (itr->second->isAlive())
			itr->second->SendInOut(INOUT_OUT, 0.0f, 0.0f, 0.0f);
	}

	// Now remove all spawns from all regions
	foreach_stlmap (itr, m_ZoneArray)
	{
		C3DMap *pMap = itr->second;
		if (pMap == NULL)
			continue;

		for (int i = 0; i < pMap->GetXRegionMax(); i++)
		{
			for (int j = 0; j < pMap->GetZRegionMax(); j++)
				pMap->m_ppRegion[i][j].m_RegionNpcArray.clear();
		}
	}

	// And finally remove all the spawn data we have.
	m_arNpcArray.DeleteAllData();
	m_bServerCheckFlag = FALSE;

	TRACE("*** DeleteAllNpcList - End *** \n");
}

CNpc*  CEbenezerDlg::GetNpcPtr( int sid, int cur_zone )
{
	if( m_bPointCheckFlag == FALSE)	return NULL;

	CNpc* pNpc = NULL;

	int nSize = m_arNpcArray.GetSize();

	for( int i = 0; i < nSize; i++)	{
		pNpc = m_arNpcArray.GetData( i+NPC_BAND );
		if (pNpc == NULL || pNpc->GetZoneID() != cur_zone
			|| pNpc->m_sPid != sid) // this isn't a typo (unless it's mgame's typo).
			continue;

		return pNpc;
	}

	return NULL;
}

void CEbenezerDlg::AliveUserCheck()
{
	uint32 currenttime = getMSTime();
	SessionMap & sessMap = s_socketMgr.GetActiveSessionMap();
	foreach (itr, sessMap)
	{
		// TO-DO: Replace this with a better, more generic check
		// Shouldn't have to rely on skills (or being in-game)
		CUser * pUser = TO_USER(itr->second);
		if (!pUser->isInGame()) 
			continue;

		for (int k = 0; k < MAX_TYPE3_REPEAT; k++)
		{
			if ((currenttime - pUser->m_fHPLastTime[k]) >= PLAYER_IDLE_TIME)
			{
				pUser->Disconnect();
				break;
			}
		}
	}
	s_socketMgr.ReleaseLock();
}

void CEbenezerDlg::BattleZoneOpenTimer()
{
	time_t t;
	struct tm * ptm;
	time(&t); // TO-DO: as time() is expensive, we should update a shared structure & access that instead.
	ptm = localtime(&t);

	int nWeek = ptm->tm_wday;
	int nTime = ptm->tm_hour;
	int loser_nation = 0, snow_battle = 0;
	CUser *pKarusUser = NULL, *pElmoUser = NULL;

	if( m_byBattleOpen == NATION_BATTLE )		BattleZoneCurrentUsers();

	if( m_byBanishFlag == 1 )	{		
		if( m_sBanishDelay == 0 )	{
			m_byBattleOpen = NO_BATTLE;
			m_byKarusOpenFlag = m_byElmoradOpenFlag = 0;

			if (m_nServerNo == KARUS)
			{
				Packet result(UDP_BATTLE_EVENT_PACKET, uint8(BATTLE_EVENT_KILL_USER));
				result << uint8(1) << m_sKarusDead << m_sElmoradDead;
				Send_UDP_All(&result);
			}
		}

		m_sBanishDelay++;

		if( m_sBanishDelay == 3 )	{
			if( m_byOldBattleOpen == SNOW_BATTLE )	{		// ���ο� ����
				if( m_sKarusDead > m_sElmoradDead )	{
					m_bVictory = ELMORAD;
					loser_nation = KARUS;
				}
				else if( m_sKarusDead < m_sElmoradDead )	{
					m_bVictory = KARUS;
					loser_nation = ELMORAD;
				}
				else if( m_sKarusDead == m_sElmoradDead )	{
					m_bVictory = 0;
				}
			}

			if( m_bVictory == 0 )	BattleZoneOpen( BATTLEZONE_CLOSE );
			else if( m_bVictory )	{
				if( m_bVictory == KARUS )		 loser_nation = ELMORAD;
				else if( m_bVictory == ELMORAD ) loser_nation = KARUS;
				Announcement( DECLARE_WINNER, m_bVictory );
				Announcement( DECLARE_LOSER, loser_nation );
			}
		}
		else if( m_sBanishDelay == 8 )	{
			Announcement(DECLARE_BAN);
		}
		else if( m_sBanishDelay == 10 )	{
			BanishLosers();
		}
		else if( m_sBanishDelay == 20 )
		{
			Packet result(AG_BATTLE_EVENT, uint8(BATTLE_EVENT_OPEN));
			result << uint8(BATTLEZONE_CLOSE);
			Send_AIServer(&result);
			ResetBattleZone();
		}
	}
}

void CEbenezerDlg::BattleZoneOpen(int nType, uint8 bZone /*= 0*/)
{
	if( nType == BATTLEZONE_OPEN ) {				// Open battlezone.
		m_byBattleOpen = NATION_BATTLE;	
		m_byOldBattleOpen = NATION_BATTLE;
		m_byBattleZone = bZone;
	}
	else if( nType == SNOW_BATTLEZONE_OPEN ) {		// Open snow battlezone.
		m_byBattleOpen = SNOW_BATTLE;	
		m_byOldBattleOpen = SNOW_BATTLE;
	}
	else if( nType == BATTLEZONE_CLOSE )	{		// battle close
		m_byBattleOpen = NO_BATTLE;
		Announcement(BATTLEZONE_CLOSE);
	}
	else return;

	Announcement(nType);	// Send an announcement out that the battlezone is open/closed.

	KickOutZoneUsers(ZONE_FRONTIER);

	Packet result(AG_BATTLE_EVENT, uint8(BATTLE_EVENT_OPEN));
	result << uint8(nType);
	Send_AIServer(&result);
}

void CEbenezerDlg::BattleZoneVictoryCheck()
{	
	if (m_bKarusFlag >= NUM_FLAG_VICTORY)
		m_bVictory = KARUS;
	else if (m_bElmoradFlag >= NUM_FLAG_VICTORY)
		m_bVictory = ELMORAD;
	else 
		return;

	Announcement(DECLARE_WINNER);

	SessionMap & sessMap = s_socketMgr.GetActiveSessionMap();
	foreach (itr, sessMap)
	{
		CUser* pTUser = TO_USER(itr->second);
		if (!pTUser->isInGame()
			|| pTUser->GetZoneID() != pTUser->GetNation() 
			|| pTUser->GetNation() != m_bVictory)
			continue;

		pTUser->GoldGain(AWARD_GOLD);
		pTUser->ExpChange(AWARD_EXP);

		if (pTUser->getFame() == COMMAND_CAPTAIN)
		{
			if (pTUser->m_bRank == 1)
				pTUser->ChangeNP(500);
			else
				pTUser->ChangeNP(300);
		}
				
		// Make the winning nation use a victory emotion (yay!)
		pTUser->StateChangeServerDirect(4, 12);
	}	
	s_socketMgr.ReleaseLock();
}

/**
 * Kicks invaders out of the invaded nation after a war
 * and resets captains.
 **/
void CEbenezerDlg::BanishLosers()
{
	SessionMap & sessMap = s_socketMgr.GetActiveSessionMap();
	foreach (itr, sessMap)
	{
		CUser *pUser = TO_USER(itr->second); 
		if (!pUser->isInGame())
			continue;

		// Reset captains
		if (pUser->getFame() == COMMAND_CAPTAIN)
			pUser->ChangeFame(CHIEF);

		// Kick out invaders
		if (pUser->GetZoneID() <= ELMORAD
			&& pUser->GetZoneID() != pUser->GetNation())
			pUser->KickOutZoneUser(TRUE);
	}
	s_socketMgr.ReleaseLock();
}

void CEbenezerDlg::ResetBattleZone()
{
	m_bVictory = 0;
	m_byBanishFlag = 0;
	m_sBanishDelay = 0;
	m_bKarusFlag = 0,
	m_bElmoradFlag = 0;
	m_byKarusOpenFlag = m_byElmoradOpenFlag = 0;
	m_byBattleOpen = NO_BATTLE;
	m_byOldBattleOpen = NO_BATTLE;
	m_sKarusDead = m_sElmoradDead = 0;
	m_byBattleSave = 0;
	m_sKarusCount = 0;
	m_sElmoradCount = 0;
	// REMEMBER TO MAKE ALL FLAGS AND LEVERS NEUTRAL AGAIN!!!!!!!!!!
}

void CEbenezerDlg::Announcement(BYTE type, int nation, int chat_type)
{
	string chatstr; 

	switch (type)
	{
		case BATTLEZONE_OPEN:
		case SNOW_BATTLEZONE_OPEN:
			GetServerResource(IDP_BATTLEZONE_OPEN, chatstr);
			break;

		case DECLARE_WINNER:
			if (m_bVictory == KARUS)
				GetServerResource(IDP_KARUS_VICTORY, chatstr, m_sElmoradDead, m_sKarusDead);
			else if (m_bVictory == ELMORAD)
				GetServerResource(IDP_ELMORAD_VICTORY, chatstr, m_sKarusDead, m_sElmoradDead);
			else 
				return;
			break;
		case DECLARE_LOSER:
			if (m_bVictory == KARUS)
				GetServerResource(IDS_ELMORAD_LOSER, chatstr, m_sKarusDead, m_sElmoradDead);
			else if (m_bVictory == ELMORAD)
				GetServerResource(IDS_KARUS_LOSER, chatstr, m_sElmoradDead, m_sKarusDead);
			else 
				return;
			break;

		case DECLARE_BAN:
			GetServerResource(IDS_BANISH_USER, chatstr);
			break;
		case BATTLEZONE_CLOSE:
			GetServerResource(IDS_BATTLE_CLOSE, chatstr);
			break;
		case KARUS_CAPTAIN_NOTIFY:
			GetServerResource(IDS_KARUS_CAPTAIN, chatstr);
			break;
		case ELMORAD_CAPTAIN_NOTIFY:
			GetServerResource(IDS_ELMO_CAPTAIN, chatstr, m_strElmoradCaptain);
			break;
		case KARUS_CAPTAIN_DEPRIVE_NOTIFY:
			GetServerResource(IDS_KARUS_CAPTAIN_DEPRIVE, chatstr, m_strKarusCaptain);
			break;
		case ELMORAD_CAPTAIN_DEPRIVE_NOTIFY:
			GetServerResource(IDS_ELMO_CAPTAIN_DEPRIVE, chatstr, m_strElmoradCaptain);
			break;
	}

#if 0
	_snprintf(finalstr, sizeof(finalstr), GetServerResource(IDP_ANNOUNCEMENT), chatstr);
	SetByte( send_buff, WIZ_CHAT, send_index );
	SetByte( send_buff, chat_type, send_index );
	SetByte( send_buff, 1, send_index );
	SetShort( send_buff, -1, send_index );
	SetKOString(send_buff, finalstr, send_index);

	Send_All(send_buff, send_index, NULL, nation);
#endif
}

BOOL CEbenezerDlg::LoadKnightsCapeTable()
{
	LOAD_TABLE(CKnightsCapeSet, &g_DBAgent.m_GameDB, &m_KnightsCapeArray, false);
}

BOOL CEbenezerDlg::LoadHomeTable()
{
	LOAD_TABLE(CHomeSet, &g_DBAgent.m_GameDB, &m_HomeArray, false);
}

BOOL CEbenezerDlg::LoadStartPositionTable()
{
	LOAD_TABLE(CStartPositionSet, &g_DBAgent.m_GameDB, &m_StartPositionArray, false);
}

BOOL CEbenezerDlg::LoadAllKnights()
{
	FastGuard lock(m_KnightsArray.m_lock);
	LOAD_TABLE(CKnightsSet, &g_DBAgent.m_GameDB, &m_KnightsArray, true);
}

BOOL CEbenezerDlg::LoadAllKnightsUserData()
{
	LOAD_TABLE(CKnightsUserSet, &g_DBAgent.m_GameDB, NULL, true);
}

BOOL CEbenezerDlg::LoadKnightsAllianceTable()
{
	LOAD_TABLE(CKnightsAllianceSet, &g_DBAgent.m_GameDB, &m_KnightsAllianceArray, true);
}

void CEbenezerDlg::CleanupUserRankings()
{
	set<_USER_RANK *> deleteSet;
	FastGuard lock(m_userRankingsLock);

	// Go through the personal rank map, reset the character's rank and insert
	// the _USER_RANK struct instance into the deletion set for later.
	foreach (itr, m_UserPersonalRankMap)
	{
		CUser *pUser = GetUserPtr(itr->first, TYPE_CHARACTER);
		if (pUser != NULL)
			pUser->m_bPersonalRank = -1;

		deleteSet.insert(itr->second);
	}

	// Go through the knights rank map, reset the character's rank and insert
	// the _USER_RANK struct instance into the deletion set for later.
	foreach (itr, m_UserKnightsRankMap)
	{
		CUser *pUser = GetUserPtr(itr->first, TYPE_CHARACTER);
		if (pUser != NULL)
			pUser->m_bKnightsRank = -1;

		deleteSet.insert(itr->second);
	}

	// Clear out the maps
	m_UserPersonalRankMap.clear();
	m_UserKnightsRankMap.clear();

	// Free the memory used by the _USER_RANK structs
	foreach (itr, deleteSet)
		delete *itr;
}

BOOL CEbenezerDlg::LoadUserRankings()
{
	CUserPersonalRankSet UserPersonalRankSet(&g_DBAgent.m_GameDB, &m_UserPersonalRankMap);
	CUserKnightsRankSet  UserKnightsRankSet(&g_DBAgent.m_GameDB, &m_UserKnightsRankMap);
	TCHAR * szError = NULL;

	// Cleanup first, in the event it's already loaded (we'll have this automatically reload in-game)
	CleanupUserRankings();

	// Acquire the lock for thread safety, and load both tables.
	FastGuard lock(m_userRankingsLock);

	szError = UserPersonalRankSet.Read(true);
	if (szError != NULL)
	{
		TRACE("Failed to load personal rankings, error: %s\n", szError);
		return FALSE;
	}

	szError = UserKnightsRankSet.Read(true);
	if (szError != NULL)
	{
		TRACE("Failed to load user knights rankings, error: %s\n", szError);
		return FALSE;
	}

	return TRUE;
}

void CEbenezerDlg::GetUserRank(CUser *pUser)
{
	// Acquire the lock for thread safety
	FastGuard lock(m_userRankingsLock);

	// Get character's name & convert it to upper case for case insensitivity
	string strUserID = pUser->GetName();
	STRTOUPPER(strUserID);

	// Grab the personal rank from the map, if applicable.
	UserRankMap::iterator itr = m_UserPersonalRankMap.find(strUserID);
	pUser->m_bPersonalRank = itr != m_UserPersonalRankMap.end() ? int8(itr->second->nRank) : -1;

	// Grab the knights rank from the map, if applicable.
	itr = m_UserKnightsRankMap.find(strUserID);
	pUser->m_bKnightsRank = itr != m_UserKnightsRankMap.end() ? int8(itr->second->nRank) : -1;
}

uint16 CEbenezerDlg::GetKnightsAllMembers(uint16 sClanID, Packet & result, uint16 & pktSize, bool bClanLeader)
{
	CKnights* pKnights = GetClanPtr(sClanID);
	if (pKnights == NULL)
		return 0;
	
	uint16 count = 0;
	foreach_array (i, pKnights->m_arKnightsUser)
	{
		_KNIGHTS_USER *p = &pKnights->m_arKnightsUser[i];
		if (!p->byUsed)
			continue;

		CUser *pUser = p->pSession;
		if (pUser != NULL)
			result << pUser->GetName() << pUser->getFame() << pUser->GetLevel() << pUser->m_sClass << uint8(1);
		else // normally just clan leaders see this, but we can be generous now.
			result << pKnights->m_arKnightsUser[i].strUserName << uint8(0) << uint8(0) << uint16(0) << uint8(0);

		count++;
	}

	return count;
}

int CEbenezerDlg::GetKnightsGrade(int nPoints)
{
	int nClanPoints = nPoints / MAX_CLAN_USERS;

	if (nClanPoints >= 20000)
		return 1;
	else if (nClanPoints >= 10000)
		return 2;
	else if (nClanPoints >= 5000)
		return 3;
	else if (nClanPoints >= 2000)
		return 4;

	return 5;
}

void CEbenezerDlg::CheckAliveUser()
{
	SessionMap & sessMap = s_socketMgr.GetActiveSessionMap();
	foreach (itr, sessMap)
	{
		CUser *pUser = TO_USER(itr->second);
		if (!pUser->isInGame())
			continue;

		if (pUser->m_sAliveCount++ > 3)
		{
			pUser->Disconnect();
			TRACE("User dropped due to inactivity - char=%s\n", pUser->GetName());
		}
	}
	s_socketMgr.ReleaseLock();
}

int CEbenezerDlg::KickOutAllUsers()
{
	int count = 0;

	SessionMap & sessMap = s_socketMgr.GetActiveSessionMap();
	foreach (itr, sessMap)
	{
		CUser *pUser = TO_USER(itr->second);
		bool bIngame = pUser->isInGame();
		pUser->Disconnect();

		// Only delay (for saving) if they're logged in, this is awful... 
		// but until we do away with the shared memory system, it'll overflow the queue...
		if (bIngame)
		{
			count++;
			Sleep(50);
		}
	}
	s_socketMgr.ReleaseLock();
	return count;
}

__int64 CEbenezerDlg::GenerateItemSerial()
{
	MYINT64 serial;
	MYSHORT	increase;
	serial.i = 0;

	time_t t;
	struct tm * ptm;
	time(&t); // TO-DO: as time() is expensive, we should update a shared structure & access that instead.
	ptm = gmtime(&t);

	EnterCriticalSection( &g_serial_critical );

	increase.w = g_increase_serial++;

	serial.b[7] = (BYTE)(m_nServerNo);
	serial.b[6] = (BYTE)(ptm->tm_year % 100);
	serial.b[5] = (BYTE)(ptm->tm_mon);
	serial.b[4] = (BYTE)(ptm->tm_mday);
	serial.b[3] = (BYTE)(ptm->tm_hour);
	serial.b[2] = (BYTE)(ptm->tm_min);
	serial.b[1] = increase.b[1];
	serial.b[0] = increase.b[0];

	LeaveCriticalSection( &g_serial_critical );
	return serial.i;
}

void CEbenezerDlg::KickOutZoneUsers(short zone)
{
	// TO-DO: Make this localised to zones.
	SessionMap & sessMap = s_socketMgr.GetActiveSessionMap();
	C3DMap	*pKarusMap		= GetZoneByID(KARUS),
			*pElMoradMap	= GetZoneByID(ELMORAD);	

	ASSERT (pKarusMap != NULL && pElMoradMap != NULL);

	foreach (itr, sessMap)
	{
		// Only kick users from requested zone.
		CUser * pUser = TO_USER(itr->second);
		if (!pUser->isInGame()
			|| pUser->GetZoneID() != zone) 
			continue;

		C3DMap * pMap = (pUser->GetNation() == KARUS ? pKarusMap : pElMoradMap);
		pUser->ZoneChange(pMap->m_nZoneNumber, pMap->m_fInitX, pMap->m_fInitZ);

	}
	s_socketMgr.ReleaseLock();
}

void CEbenezerDlg::Send_UDP_All(Packet *pkt, int group_type /*= 0*/)
{
#if 0 // disabled pending rewrite
	int server_number = (group_type == 0 ? m_nServerNo : m_nServerGroupNo);
	foreach_stlmap (itr, (group_type == 0 ? m_ServerArray : m_ServerGroupArray))
	{
		if (itr->second && itr->second->sServerNo == server_number)
			m_pUdpSocket->SendUDPPacket(itr->second->strServerIP, pkt);
	}
#endif
}

BOOL CEbenezerDlg::LoadBattleTable()
{
	LOAD_TABLE(CBattleSet, &g_DBAgent.m_GameDB, &m_byOldVictory, true);
}

void CEbenezerDlg::Send_CommandChat(Packet *pkt, int nation, CUser* pExceptUser)
{
	SessionMap & sessMap = s_socketMgr.GetActiveSessionMap();
	foreach (itr, sessMap)
	{
		CUser * pUser = TO_USER(itr->second);
		if (pUser->isInGame() 
			&& pUser != pExceptUser 
			&& (nation == 0 || nation == pUser->GetNation()))
			pUser->Send(pkt);
	}
	s_socketMgr.ReleaseLock();
}

void CEbenezerDlg::GetCaptainUserPtr()
{
	FastGuard lock(m_KnightsArray.m_lock);
	foreach_stlmap (itr, m_KnightsArray)
	{
		CKnights *pKnights = itr->second;
		if (pKnights->m_byRanking != 1)
			continue;

		// do something cool here
	}
}

BOOL CEbenezerDlg::LoadKnightsRankTable()
{
	string strKarusCaptainNames, strElmoCaptainNames;
	LOAD_TABLE_ERROR_ONLY(CKnightsRankSet, &g_DBAgent.m_GameDB, NULL, true);

	CKnightsRankSet & pSet = _CKnightsRankSet; // kind ugly generic naming

	if (pSet.nKarusCount > 0)
	{
		Packet result(WIZ_CHAT, uint8(WAR_SYSTEM_CHAT));
		GetServerResource(IDS_KARUS_CAPTAIN, strKarusCaptainNames, 
			pSet.strKarusCaptain[0], pSet.strKarusCaptain[1], pSet.strKarusCaptain[2], 
			pSet.strKarusCaptain[3], pSet.strKarusCaptain[4]);

		result << int8(1) << int16(-1) << int8(0) << strKarusCaptainNames;
		Send_All(&result, NULL, KARUS);
	}

	if (pSet.nElmoCount > 0)
	{
		Packet result(WIZ_CHAT, uint8(WAR_SYSTEM_CHAT));
		GetServerResource(IDS_ELMO_CAPTAIN, strElmoCaptainNames,
			pSet.strElmoCaptain[0], pSet.strElmoCaptain[1], pSet.strElmoCaptain[2], 
			pSet.strElmoCaptain[3], pSet.strElmoCaptain[4]);

		result << int8(1) << int16(-1) << int8(0) << strElmoCaptainNames;
		Send_All(&result, NULL, ELMORAD);
	}

	return TRUE;
}

void CEbenezerDlg::BattleZoneCurrentUsers()
{
	C3DMap* pMap = GetZoneByID(ZONE_BATTLE);
	if (pMap == NULL || m_nServerNo != pMap->m_nServerNo)
		return;

	uint16 nKarusMan = 0, nElmoradMan = 0;
	SessionMap & sessMap = s_socketMgr.GetActiveSessionMap();
	foreach (itr, sessMap)
	{
		CUser * pUser = TO_USER(itr->second);
		if (!pUser->isInGame() || pUser->GetZoneID() != ZONE_BATTLE)
			continue;

		if (pUser->GetNation() == KARUS)
			nKarusMan++;
		else
			nElmoradMan++;
	}
	s_socketMgr.ReleaseLock();

	m_sKarusCount = nKarusMan;
	m_sElmoradCount = nElmoradMan;

	//TRACE("---> BattleZoneCurrentUsers - karus=%d, elmorad=%d\n", m_sKarusCount, m_sElmoradCount);

	Packet result(UDP_BATTLEZONE_CURRENT_USERS);
	result << m_sKarusCount << m_sElmoradCount;
	Send_UDP_All(&result);
}

void CEbenezerDlg::SendFlyingSantaOrAngel()
{
	Packet result(WIZ_SANTA, uint8(m_bSantaOrAngel));
	Send_All(&result);
}

CEbenezerDlg::~CEbenezerDlg() 
{
	KillTimer(NULL, s_dwGameTimerID);
	KillTimer(NULL, s_dwAliveTimerID);

	KickOutAllUsers();

	g_bRunning = false;

	DatabaseThread::Shutdown();

	DeleteCriticalSection(&g_region_critical);
	DeleteCriticalSection(&g_serial_critical);
	
	CUser::CleanupChatCommands();
	CEbenezerDlg::CleanupServerCommands();

	CleanupUserRankings();

	if (m_LevelUpArray.size())
		m_LevelUpArray.clear();

	if (m_pUdpSocket)
		delete m_pUdpSocket;
}