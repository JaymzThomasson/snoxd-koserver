#pragma once

#include "LuaEngine.h"

#include "Define.h"
#include "ChatHandler.h"

class C3DMap;
class CUser;

#include "LoadServerData.h"

typedef stdext::hash_map<std::string, CUser *> NameMap;

class CEbenezerDlg
{
public:	
	CEbenezerDlg();
	bool Startup();

	void GetTimeFromIni();

	bool LoadItemTable();
	bool LoadItemExchangeTable();
	bool LoadItemUpgradeTable();
	bool LoadServerResourceTable();
	bool LoadEventTriggerTable();
	bool LoadQuestHelperTable();
	bool LoadQuestMonsterTable();
	bool LoadMagicTable();
	bool LoadMagicType1();
	bool LoadMagicType2();
	bool LoadMagicType3();
	bool LoadMagicType4();
	bool LoadMagicType5();
	bool LoadMagicType6();
	bool LoadMagicType7();
	bool LoadMagicType8();
	bool LoadMagicType9();
	bool LoadRentalList();
	bool LoadCoefficientTable();
	bool LoadLevelUpTable();
	bool LoadAllKnights();
	bool LoadAllKnightsUserData();
	bool LoadKnightsAllianceTable();
	bool LoadUserRankings();
	void CleanupUserRankings();
	bool LoadKnightsCapeTable();
	bool LoadKnightsRankTable(bool bWarTime = false);
	bool LoadHomeTable();
	bool LoadStartPositionTable();
	bool LoadBattleTable();
	bool LoadKingSystem();

	bool MapFileLoad();
	bool LoadNoticeData();

	void AIServerConnect();

	void OnTimer(UINT nIDEvent);

	void SendFlyingSantaOrAngel();
	void BattleZoneCurrentUsers();
	void GetCaptainUserPtr();
	void Send_CommandChat(Packet *pkt, int nation, CUser* pExceptUser = NULL);
	void Send_UDP_All(Packet *pkt, int group_type = 0);
	void KickOutZoneUsers(short zone);
	__int64 GenerateItemSerial();
	int KickOutAllUsers();
	void CheckAliveUser();
	int GetKnightsGrade(int nPoints);
	void WritePacketLog();
	uint16 GetKnightsAllMembers(uint16 sClanID, Packet & result, uint16 & pktSize, bool bClanLeader);
	void GetUserRank(CUser *pUser);
	void Announcement(BYTE type, int nation=0, int chat_type=8);
	void ResetBattleZone();
	void BanishLosers();
	void BattleZoneVictoryCheck();
	void BattleZoneOpenTimer();
	void BattleZoneOpen(int nType, uint8 bZone = 0);	// 0:open 1:close
	void AliveUserCheck();
	void Send_PartyMember(int party, Packet *result);
	void Send_KnightsMember(int index, Packet *pkt);
	void Send_KnightsAlliance(uint16 sAllianceID, Packet *pkt);
	int GetAIServerPort();
	void SetGameTime();
	void UpdateWeather();
	void UpdateGameTime();
	void SendAllUserInfo();
	void DeleteAllNpcList(int flag = 0);
	CNpc*  GetNpcPtr( int sid, int cur_zone );

	void AddDatabaseRequest(Packet & pkt, CUser *pUser = NULL);

	// Get info for NPCs in regions around user (WIZ_REQ_NPCIN)
	void NpcInOutForMe(CUser* pSendUser);

	// Get info for NPCs in region
	void GetRegionNpcIn(C3DMap* pMap, uint16 region_x, uint16 region_z, Packet & pkt, uint16 & t_count);

	// Get list of NPC IDs in region
	void GetRegionNpcList(C3DMap* pMap, uint16 region_x, uint16 region_z, Packet & pkt, uint16 & t_count);

	// Get list of NPCs for regions around user (WIZ_NPC_REGION)
	void RegionNpcInfoForMe(CUser* pSendUser);	

	// Get info for users in regions around user (WIZ_REQ_USERIN)
	void UserInOutForMe(CUser* pSendUser);

	// Get info for users in region
	void GetRegionUserIn(C3DMap* pMap, uint16 region_x, uint16 region_z, Packet & pkt, uint16 & t_count);

	// Get list of user IDs in region
	void GetRegionUserList(C3DMap* pMap, uint16 region_x, uint16 region_z, Packet & pkt, uint16 & t_count);

	// Get list of users for regions around user (WIZ_REGIONCHANGE)
	void RegionUserInOutForMe(CUser* pSendUser);

	// Get info for merchants in regions around user
	void MerchantUserInOutForMe(CUser* pSendUser);

	// Get list of merchants in region
	void GetRegionMerchantUserIn(C3DMap* pMap, uint16 region_x, uint16 region_z, Packet & pkt, uint16 & t_count);

	__forceinline bool isPermanentMessageSet() { return m_bPermanentChatMode; }
	void SetPermanentMessage(const char * format, ...);

	void HandleConsoleCommand(const char * msg);

	void SendNotice(const char *msg, uint8 bNation = NO_NATION);
	void SendFormattedNotice(const char *msg, uint8 nation = NO_NATION, ...);

	void SendAnnouncement(const char *msg, uint8 bNation = NO_NATION);
	void SendFormattedAnnouncement(const char *msg, uint8 nation = NO_NATION, ...);

	void SendFormattedResource(uint32 nResourceID, uint8 nation = NO_NATION, bool bIsNotice = true, ...);

	void Send_Region(Packet *pkt, C3DMap *pMap, int x, int z, CUser* pExceptUser = NULL);
	void Send_UnitRegion(Packet *pkt, C3DMap *pMap, int x, int z, CUser* pExceptUser = NULL);
	void Send_OldRegions(Packet *pkt, int old_x, int old_z, C3DMap *pMap, int x, int z, CUser* pExceptUser = NULL);
	void Send_NewRegions(Packet *pkt, int new_x, int new_z, C3DMap *pMap, int x, int z, CUser* pExceptUser = NULL);

	void Send_NearRegion(Packet *pkt, C3DMap *pMap, int region_x, int region_z, float curx, float curz, CUser* pExceptUser=NULL );
	void Send_FilterUnitRegion(Packet *pkt, C3DMap *pMap, int x, int z, float ref_x, float ref_z, CUser* pExceptUser=NULL );

	void Send_All(Packet *pkt, CUser* pExceptUser = NULL, uint8 nation = 0);
	void Send_AIServer(Packet *pkt);

	void GetServerResource(int nResourceID, std::string * result, ...);
	_START_POSITION *GetStartPosition(int nZoneID);

	long GetExpByLevel(int nLevel);
	C3DMap * GetZoneByID(int zoneID);

	CUser * GetUserPtr(std::string findName, NameType type);
	CUser * GetUserPtr(int sid);

	Unit * GetUnit(uint16 id);

	int32 GetEventTrigger(CNpc * pNpc);

	// Adds the account name & session to a hashmap (on login)
	void AddAccountName(CUser *pSession);

	// Adds the character name & session to a hashmap (when in-game)
	void AddCharacterName(CUser *pSession);

	// Removes the account name & character names from the hashmaps (on logout)
	void RemoveSessionNames(CUser *pSession);

	CKnights * GetClanPtr(uint16 sClanID);
	_KNIGHTS_ALLIANCE * GetAlliancePtr(uint16 sAllianceID);
	_ITEM_TABLE * GetItemPtr(uint32 nItemID);

	_PARTY_GROUP * CreateParty(CUser *pLeader);
	void DeleteParty(short sIndex);

	~CEbenezerDlg();

	char	m_ppNotice[20][128];
	char	m_AIServerIP[20];

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
	KnightsRatingArray		m_KnightsRatingArray[2]; // one for both nations
	KnightsAllianceArray	m_KnightsAllianceArray;
	KnightsCapeArray		m_KnightsCapeArray;
	UserRankMap				m_UserPersonalRankMap;
	UserRankMap				m_UserKnightsRankMap;
	FastMutex				m_userRankingsLock;
	HomeArray				m_HomeArray;
	StartPositionArray		m_StartPositionArray;
	ServerResourceArray		m_ServerResourceArray;
	EventTriggerArray		m_EventTriggerArray;
	QuestHelperArray		m_QuestHelperArray;
	QuestNpcList			m_QuestNpcList;
	QuestMonsterArray		m_QuestMonsterArray;
	RentalItemArray			m_RentalItemArray;
	ItemExchangeArray		m_ItemExchangeArray;
	ItemUpgradeArray		m_ItemUpgradeArray;
	KingSystemArray			m_KingSystemArray;

	short	m_sPartyIndex;
	short	m_sZoneCount;							// AI Server 재접속시 사용

	bool	m_bFirstServerFlag;		// 서버가 처음시작한 후 게임서버가 붙은 경우에는 1, 붙지 않은 경우 0
	bool	m_bServerCheckFlag;
	bool	m_bPointCheckFlag;		// AI서버와 재접전에 NPC포인터 참조막기 (true:포인터 참조, false:포인터 참조 못함)
	short   m_sErrorSocketCount;  // 이상소켓 감시용

	uint16 m_nYear, m_nMonth, m_nDate, m_nHour, m_nMin;
	uint8 m_byWeather;
	uint16 m_sWeatherAmount;
	int m_nCastleCapture;

	BYTE    m_byBattleOpen, m_byOldBattleOpen;					// 0:전쟁중이 아님, 1:전쟁중(국가간전쟁), 2:눈싸움전쟁
	uint8	m_byBattleZone;
	uint8	m_bVictory, m_byOldVictory;
	BYTE	m_bKarusFlag, m_bElmoradFlag;
	BYTE    m_byKarusOpenFlag, m_byElmoradOpenFlag, m_byBanishFlag, m_byBattleSave;
	short   m_sDiscount;	// 능력치와 포인트 초기화 할인 (0:할인없음, 1:할인(50%) )
	short	m_sKarusDead, m_sElmoradDead, m_sBanishDelay, m_sKarusCount, m_sElmoradCount;
	int m_nBattleZoneOpenWeek, m_nBattleZoneOpenHourStart, m_nBattleZoneOpenHourEnd;
	char m_strKarusCaptain[MAX_ID_SIZE+1];
	char m_strElmoradCaptain[MAX_ID_SIZE+1];

	BYTE	m_bMaxRegenePoint;

	bool	m_bPermanentChatMode;
	std::string	m_strPermanentChat;

	uint8 m_bSantaOrAngel;

	// zone server info
	int					m_nServerNo, m_nServerGroupNo;
	int					m_nServerGroup;	// server의 번호(0:서버군이 없다, 1:서버1군, 2:서버2군)
	ServerArray			m_ServerArray;
	ServerArray			m_ServerGroupArray;

	NameMap		m_accountNameMap,
				m_characterNameMap;

	FastMutex	m_accountNameLock,
				m_characterNameLock,
				m_questNpcLock;

	// Controlled weather events set by Kings
	uint8 m_byKingWeatherEvent;
	uint8 m_byKingWeatherEvent_Day;
	uint8 m_byKingWeatherEvent_Hour;
	uint8 m_byKingWeatherEvent_Minute;

	__forceinline CLuaEngine * GetLuaEngine() { return &m_luaEngine; }

private:
	CLuaEngine	m_luaEngine;

	std::string m_strGameDSN, m_strAccountDSN;
	std::string m_strGameUID, m_strAccountUID;
	std::string m_strGamePWD, m_strAccountPWD;
	bool m_bMarsEnabled;

	bool LoadTables();

	bool ProcessServerCommand(std::string & command);

public:
	void InitServerCommands();
	void CleanupServerCommands();

	static ServerCommandTable s_commandTable;

	COMMAND_HANDLER(HandleNoticeCommand);
	COMMAND_HANDLER(HandleKillUserCommand);
	COMMAND_HANDLER(HandleWar1OpenCommand);
	COMMAND_HANDLER(HandleWar2OpenCommand);
	COMMAND_HANDLER(HandleWar3OpenCommand);
	COMMAND_HANDLER(HandleSnowWarOpenCommand);
	COMMAND_HANDLER(HandleWarCloseCommand);
	COMMAND_HANDLER(HandleShutdownCommand);
	COMMAND_HANDLER(HandlePauseCommand);
	COMMAND_HANDLER(HandleResumeCommand);
	COMMAND_HANDLER(HandleDiscountCommand);
	COMMAND_HANDLER(HandleGlobalDiscountCommand);
	COMMAND_HANDLER(HandleDiscountOffCommand);
	COMMAND_HANDLER(HandleCaptainCommand);
	COMMAND_HANDLER(HandleSantaCommand);
	COMMAND_HANDLER(HandleSantaOffCommand);
	COMMAND_HANDLER(HandleAngelCommand);
	COMMAND_HANDLER(HandlePermanentChatCommand);
	COMMAND_HANDLER(HandlePermanentChatOffCommand);
	COMMAND_HANDLER(HandleReloadNoticeCommand);
};

extern CEbenezerDlg * g_pMain;