#pragma once

#include "define.h"
#include "GameDefine.h"
#include "MagicProcess.h"
#include "Npc.h"
#include "EVENT.h"
#include "EVENT_DATA.h"
#include "LOGIC_ELSE.h"
#include "EXEC.h"     

#include <list>
#include <vector>

#include "ChatHandler.h"

#include "../shared/KOSocket.h"

typedef	 std::list<_EXCHANGE_ITEM*>		ItemList;
typedef  std::list<int>					UserEventList;
typedef	 std::map<uint32, time_t>		SkillCooldownList;

#define BANISH_DELAY_TIME    30

enum GameState
{
	GAME_STATE_CONNECTED,
	GAME_STATE_INGAME
};

enum InvisibilityType
{
	INVIS_NONE				= 0,
	INVIS_DISPEL_ON_MOVE	= 1,
	INVIS_NORMAL			= 2
};

class CEbenezerDlg;
class CUser : public Unit, public KOSocket
{
public:
	virtual uint16 GetID() { return GetSocketID(); }

	virtual float GetX() { return m_pUserData->m_curx; }
	virtual float GetY() { return m_pUserData->m_cury; }
	virtual float GetZ() { return m_pUserData->m_curz; }

	virtual const char * GetName() { return m_pUserData->m_id; }

	_USER_DATA*	m_pUserData;

	std::string m_strAccountID;

	bool	m_bSelectedCharacter;
	bool	m_bStoreOpen;

	bool	m_bIsMerchanting; //Is the character merchanting already?
	int16	m_sMerchantsSocketID;
	std::list<uint16>	m_arMerchantLookers;
	_MERCH_DATA	m_arSellingItems[MAX_MERCH_ITEMS]; //What is this person selling? Stored in "_MERCH_DATA" structure.

	uint8	m_bRequestingChallenge, // opcode of challenge request being sent by challenger
			m_bChallengeRequested;  // opcode of challenge request received by challengee
	int16	m_sChallengeUser;

	//Magic System Cooldown checks

	SkillCooldownList	m_CoolDownList;

	bool	m_bIsTransformed; // Is the character in a transformed state?
	bool	m_bIsChicken; // Is the character taking the beginner/chicken quest?
	bool	m_bIsHidingHelmet;

	int8	m_bPersonalRank;
	int8	m_bKnightsRank;

	int16	m_sDirection;

	int64	m_iMaxExp;
	uint32	m_sMaxWeight;
	uint16   m_sSpeed;	// NOTE: Currently unused

	short   m_sItemMaxHp;                   // ������ �� �ִ� HP Bonus
	short   m_sItemMaxMp;                   // ������ �� �ִ� MP Bonus
	uint32	m_sItemWeight;					// ������ �ѹ���
	short	m_sItemHit;						// ������ ��Ÿ��ġ
	short	m_sItemAc;						// ������ �ѹ�����
	short	m_sItemHitrate;					// ������ ��Ÿ����
	short	m_sItemEvasionrate;				// ������ ��ȸ����

	uint16	m_sStatItemBonuses[STAT_COUNT];
	uint8	m_bStatBuffs[STAT_COUNT];

	short	m_iMaxHp, m_iMaxMp;
	
	BYTE	m_bResHpType;
	BYTE	m_bWarp;
	BYTE	m_bNeedParty;

	short	m_sPartyIndex;
	bool	m_bPartyLeader;

	bool	m_bCanSeeStealth;
	uint8	m_bInvisibilityType;

	short	m_sExchangeUser;
	BYTE	m_bExchangeOK;

	ItemList	m_ExchangeItemList;
	_ITEM_DATA	m_MirrorItem[HAVE_MAX];

	short	m_sPrivateChatUser;

	float	m_fHPLastTimeNormal;					// For Automatic HP recovery. 
	float	m_fHPStartTimeNormal;
	short	m_bHPAmountNormal;
	BYTE	m_bHPDurationNormal;
	BYTE	m_bHPIntervalNormal;

	float	m_fAreaLastTime;			// For Area Damage spells Type 3.
	float   m_fAreaStartTime;
	BYTE    m_bAreaInterval;
	int     m_iAreaMagicID;

	uint32	m_nTransformationItem; // item used for transforming (e.g. disguise scroll, totem..)
	float	m_fTransformationStartTime;
	uint16	m_sTransformationDuration;

	CMagicProcess m_MagicProcess;

	float	m_fSpeedHackClientTime, m_fSpeedHackServerTime;
	BYTE	m_bSpeedHackCheck;

	float	m_fBlinkStartTime;			// When did you start to blink?

	short	m_sAliveCount;

	DWORD	m_bAbnormalType;			// Is the player normal,a giant, or a dwarf?

	short	m_sWhoKilledMe;				// Who killed me???
	__int64		m_iLostExp;					// Experience point that was lost when you died.

	float	m_fLastTrapAreaTime;		// The last moment you were in the trap area.

	BOOL	m_bZoneChangeFlag;			// ���뾾 �̿�!!

	BYTE	m_bRegeneType;				// Did you die and go home or did you type '/town'?

	float	m_fLastRegeneTime;			// The last moment you got resurrected.

	BOOL	m_bZoneChangeSameZone;		// Did the server change when you warped?

	int					m_iSelMsgEvent[MAX_MESSAGE_EVENT];	// �������� ���� �޼����ڽ� �̺�Ʈ
	short				m_sEventNid;		// ���������� ������ �̺�Ʈ NPC ��ȣ
	UserEventList		m_arUserEvent;		// ������ �̺�Ʈ ����Ʈ


public:
	__forceinline bool isBanned() { return getAuthority() == AUTHORITY_BANNED; }
	__forceinline bool isMuted() { return getAuthority() == AUTHORITY_MUTED; }
	__forceinline bool isAttackDisabled() { return getAuthority() == AUTHORITY_ATTACK_DISABLED; }
	__forceinline bool isGM() { return getAuthority() == AUTHORITY_GAME_MASTER; }
	__forceinline bool isLimitedGM() { return getAuthority() == AUTHORITY_LIMITED_GAME_MASTER; }

	virtual bool isDead() { return m_bResHpType == USER_DEAD || m_pUserData->m_sHp <= 0; }
	__forceinline bool isBlinking() { return m_bAbnormalType == ABNORMAL_BLINKING; }

	__forceinline bool isInGame() { return GetState() == GAME_STATE_INGAME; }
	__forceinline bool isInParty() { return m_sPartyIndex != -1; }
	__forceinline bool isInClan() { return m_pUserData->m_bKnights > 0; }
	__forceinline bool isClanLeader() { return getFame() == CHIEF; }
	__forceinline bool isClanAssistant() { return getFame() == VICECHIEF; }

	__forceinline bool isWarrior() { return JobGroupCheck(1) == TRUE; }
	__forceinline bool isRogue() { return JobGroupCheck(2) == TRUE; }
	__forceinline bool isMage() { return JobGroupCheck(3) == TRUE; }
	__forceinline bool isPriest() { return JobGroupCheck(4) == TRUE; }

	__forceinline bool isTrading() { return m_sExchangeUser != -1; }
	__forceinline bool isStoreOpen() { return m_bStoreOpen; }
	__forceinline bool isMerchanting() { return m_bIsMerchanting; }
	__forceinline bool isTransformed() { return m_bIsTransformed; }

	virtual uint8 GetNation() { return m_pUserData->m_bNation; }
	virtual uint8 GetLevel() { return m_pUserData->m_bLevel; }
	virtual uint8 GetZoneID() { return m_pUserData->m_bZone; }

	__forceinline BYTE getAuthority() { return m_pUserData->m_bAuthority; }
	__forceinline BYTE getFame() { return m_pUserData->m_bFame; }

	__forceinline GameState GetState() { return m_state; }

	__forceinline uint8 getStat(StatType type)
	{
		ASSERT(type < STAT_COUNT);
		return m_pUserData->m_bStats[type];
	}

	__forceinline void setStat(StatType type, uint8 val)
	{
		ASSERT(type < STAT_COUNT);
		m_pUserData->m_bStats[type] = val;
	}

	__forceinline uint32 getStatTotal() // NOTE: Shares name with another, but lack-of args should be self-explanatory
	{
		uint32 total = 0; // NOTE: this loop should be unrolled by the compiler
		foreach_array (i, m_pUserData->m_bStats)
			total += m_pUserData->m_bStats[i];
		return total;
	}

	__forceinline uint16 getStatItemBonus(StatType type)
	{
		ASSERT(type < STAT_COUNT);
		return m_sStatItemBonuses[type];
	}

	__forceinline uint16 getStatWithItemBonus(StatType type)
	{
		return getStat(type) + getStatItemBonus(type);
	}

	__forceinline uint32 getStatItemBonusTotal()
	{
		uint32 total = 0; // NOTE: this loop should be unrolled by the compiler
		foreach_array (i, m_sStatItemBonuses)
			total += m_sStatItemBonuses[i];
		return total;
	}

	__forceinline uint16 getStatBonusTotal(StatType type)
	{
		return getStatBuff(type) + getStatItemBonus(type);
	}

	__forceinline uint8 getStatBuff(StatType type)
	{
		ASSERT(type < STAT_COUNT);
		return m_bStatBuffs[type];
	}

	__forceinline void setStatBuff(StatType type, uint8 val)
	{
		ASSERT(type < STAT_COUNT);
		m_bStatBuffs[type] = val;
	}

	__forceinline uint32 getStatBuffTotal()
	{
		uint32 total = 0; // NOTE: this loop should be unrolled by the compiler
		foreach_array (i, m_bStatBuffs)
			total += m_bStatBuffs[i];
		return total;
	}

	__forceinline uint16 getStatTotal(StatType type)
	{
		return getStat(type) + getStatItemBonus(type) + getStatBuff(type);
	}

	__forceinline _ITEM_DATA * GetItem(uint8 pos) { return &m_pUserData->m_sItemArray[pos]; }
	_ITEM_TABLE* GetItemPrototype(uint8 pos);

	__forceinline C3DMap * GetMap() { return m_pMap; }

	CUser(uint16 socketID, SocketMgr *mgr); 

	virtual void OnConnect();
	virtual void OnDisconnect();
	virtual bool HandlePacket(Packet & pkt);

	void SendLoyaltyChange(int32 nChangeAmount = 0);

	BOOL CheckItemCount(int itemid, short min, short max);
	BOOL CheckRandom(short percent);
	void NativeZoneReturn();
	void KickOutZoneUser(BOOL home = FALSE, int nZoneID = 21);
	void TrapProcess();
	BOOL JobGroupCheck(short jobgroupid);
	void SelectMsg(EXEC* pExec);
	void SendNpcSay(EXEC* pExec);
	BOOL CheckClass(short class1, short class2 = -1, short class3 = -1, short class4 = -1, short class5 = -1, short class6 = -1);
	BOOL GiveItem(int itemid, short count, bool send_packet = true);
	BOOL RobItem(int itemid, short count);
	BOOL CheckExistItem(int itemid, short count);
	BOOL CheckWeight(int itemid, short count);
	BOOL CheckSkillPoint(BYTE skillnum, BYTE min, BYTE max);
	BOOL GoldLose(unsigned int gold);
	void GoldGain(int gold);
	void SendItemWeight();
	void UpdateVisibility(InvisibilityType bNewType);
	void BlinkStart();
	void BlinkTimeCheck(float currenttime);
	void GoldChange(short tid, int gold);
	CUser* GetItemRoutingUser(int itemid, short itemcount);
	bool GetStartPosition(short & x, short & y, BYTE bZone = 0);
	int GetEmptySlot( int itemid, int bCountable );
	void Type3AreaDuration( float currenttime);
	void SendAllKnightsID();
	void SendStackChange(uint32 nItemID, uint32 nCount /* needs to be 4 bytes, not a bug */, uint16 sDurability, uint8 bPos, bool bNewItem = false);
	void Type4Duration(float currenttime);
	void HPTimeChange( float currenttime );
	void HPTimeChangeType3( float currenttime );
	void ItemDurationChange(uint8 slot, uint16 maxValue, int16 curValue, uint16 amount);
	void SendDurability(uint8 slot, uint16 durability);
	void SendItemMove(uint8 subcommand);
	void ItemWoreOut( int type, int damage );
	void Dead();
	void LoyaltyDivide( short tid );
	void GetUserInfoForAI(Packet & result);
	BOOL ItemEquipAvailable( _ITEM_TABLE* pTable );
	virtual void HpChange(int amount, Unit *pAttacker = NULL, bool bSendToAI = true);
	virtual void MSpChange(int amount);
	void SendPartyHPUpdate();
	void SendAnvilRequest(int nid);

	// packet handlers start here
	void VersionCheck(Packet & pkt);
	void LoginProcess(Packet & pkt);
	void RecvLoginProcess(Packet & pkt); // from Aujard
	void SelNationToAgent(Packet & pkt);
	void RecvSelNation(Packet & pkt); // from Aujard
	void AllCharInfoToAgent();
	void RecvAllCharInfoReq(Packet & pkt); // from Aujard
	void ChangeHair(Packet & pkt);
	void RecvChangeHair(Packet & pkt); // from Aujard
	void NewCharToAgent(Packet & pkt);
	void RecvNewChar(Packet & pkt); // from Aujard
	void DelCharToAgent(Packet & pkt);
	void RecvDeleteChar(Packet & pkt); // from Aujard
	void SelCharToAgent(Packet & pkt);
	void SelectCharacter(Packet & pkt); // from Aujard
	void SetLogInInfoToDB(BYTE bInit);
	void RecvLoginInfo(Packet & pkt); // from Aujard

	void SpeedHackTime(Packet & pkt);

	void GameStart(Packet & pkt);
	void RentalSystem(Packet & pkt);
	void SkillDataProcess(Packet & pkt);
	void SkillDataSave(Packet & pkt);
	void SkillDataLoad();
	void RecvSkillDataLoad(Packet & pkt); // from Aujard
	void MoveProcess(Packet & pkt);
	void Rotate(Packet & pkt);
	void Attack(Packet & pkt);

	static void InitChatCommands();
	static void CleanupChatCommands();

	void Chat(Packet & pkt);
	void ChatTargetSelect(Packet & pkt);

	bool ProcessChatCommand(std::string & message);

	COMMAND_HANDLER(HandleGiveItemCommand);
	COMMAND_HANDLER(HandleZoneChangeCommand);

	void RecvRegene(Packet & pkt);
	void Regene(uint8 regene_type, uint32 magicid = 0);
	void RequestUserIn(Packet & pkt);
	void RequestNpcIn(Packet & pkt);
	void RecvWarp(Packet & pkt);
	void Warp(uint16 sPosX, uint16 sPosZ);
	void ItemMove(Packet & pkt);
	void NpcEvent(Packet & pkt);

	void ItemTrade(Packet & pkt);

	void BundleOpenReq(Packet & pkt);
	void ItemGet(Packet & pkt);

	void RecvZoneChange(Packet & pkt);
	void PointChange(Packet & pkt);

	void StateChange(Packet & pkt);
	void StateChangeServerDirect(BYTE bType, uint32 nBuff);

	void PartyProcess(Packet & pkt);
	void PartyDelete();
	void PartyRemove( int memberid );
	void PartyInsert();
	void PartyCancel();
	void PartyRequest( int memberid, BOOL bCreate );

	// Trade system
	void ExchangeProcess(Packet & pkt);
	void ExchangeReq(Packet & pkt);
	void ExchangeAgree(Packet & pkt);
	void ExchangeAdd(Packet & pkt);
	void ExchangeDecide();
	void ExchangeCancel();

	void InitExchange(BOOL bStart);
	BOOL ExecuteExchange();
	int ExchangeDone();

	void QuestDataRequest(Packet & pkt);

	// Merchant system (both types)
	void MerchantProcess(Packet & pkt);
	void TakeMerchantItems();
	void GiveMerchantItems();

	// regular merchants
	void MerchantOpen();
	void MerchantClose();
	void MerchantItemAdd(Packet & pkt);
	void MerchantItemCancel(Packet & pkt);
	void MerchantItemList(Packet & pkt);
	void MerchantItemBuy(Packet & pkt);
	void MerchantInsert(Packet & pkt);
	void CancelMerchant();

	// buying merchants
	void BuyingMerchantOpen(Packet & pkt);
	void BuyingMerchantClose();
	void BuyingMerchantInsert(Packet & pkt);
	void BuyingMerchantList(Packet & pkt);
	void BuyingMerchantBuy(Packet & pkt);

	void RemoveFromMerchantLookers();

	void SkillPointChange(Packet & pkt);

	void ObjectEvent(Packet & pkt);
	BOOL BindObjectEvent(_OBJECT_EVENT *pEvent);
	BOOL GateLeverObjectEvent(_OBJECT_EVENT *pEvent, int nid);
	BOOL FlagObjectEvent(_OBJECT_EVENT *pEvent, int nid);
	BOOL WarpListObjectEvent(_OBJECT_EVENT *pEvent);

	void UpdateGameWeather(Packet & pkt);

	void ClassChange(Packet & pkt);
	void ClassChangeReq();
	void AllPointChange();
	void AllSkillPointChange();

	void CountConcurrentUser();
	void UserDataSaveToAgent();

	void ItemRepair(Packet & pkt);
	void ItemRemove(Packet & pkt);
	void OperatorCommand(Packet & pkt);
	void WarehouseProcess(Packet & pkt);
	void Home();

	void FriendProcess(Packet & pkt);
	void RecvFriendProcess(Packet & pkt);
	void FriendRequest();
	void FriendModify(Packet & pkt, uint8 opcode);
	void RecvFriendModify(Packet & pkt, uint8 opcode);
	void FriendReport(Packet & pkt);
	BYTE GetFriendStatus(std::string & charName, int16 & sid);

	void SelectWarpList(Packet & pkt);
	BOOL GetWarpList( int warp_group );

	void ServerChangeOk(Packet & pkt);

	void PartyBBS(Packet & pkt);
	void PartyBBSRegister(Packet & pkt);
	void PartyBBSDelete(Packet & pkt);
	void PartyBBSNeeded(Packet & pkt, BYTE type);

	void ClientEvent(Packet & pkt);
	BOOL CheckEventLogic(EVENT_DATA* pEventData);
	BOOL RunNpcEvent(CNpc* pNpc, EXEC* pExec);
	BOOL RunEvent(EVENT_DATA *pEventData);

	void RecvSelectMsg(Packet & pkt);

	// from the client
	void ItemUpgradeProcess(Packet & pkt);
	void ItemUpgrade(Packet & pkt);
	void ItemUpgradeAccessories(Packet & pkt);
	void BifrostPieceProcess(Packet & pkt); // originally named BeefRoastPieceProcess() -- that's not happening.
	void ItemUpgradeRebirth(Packet & pkt);
	void ItemSealProcess(Packet & pkt);
	void CharacterSealProcess(Packet & pkt);

	void ShoppingMall(Packet & pkt);
	void RecvStore(Packet & pkt); // from Aujard
	void HandleStoreClose();
	void RecvStoreClose(Packet & pkt); // from Aujard
	void LetterSystem(Packet & pkt);

	void HandleHelmet(Packet & pkt);
	void HandleCapeChange(Packet & pkt);

	void HandleChallenge(Packet & pkt);
	void HandleChallengeRequestPVP(Packet & pkt);
	void HandleChallengeRequestCVC(Packet & pkt);
	void HandleChallengeAcceptPVP(Packet & pkt);
	void HandleChallengeAcceptCVC(Packet & pkt);
	void HandleChallengeCancelled(uint8 opcode);
	void HandleChallengeRejected(uint8 opcode);

	void SendNotice();
	void UserLookChange( int pos, int itemid, int durability );
	void SpeedHackUser();
	void LoyaltyChange(short tid);
	void ChangeNP(short sAmount, bool bDistributeToParty = true);
	void ZoneChange( int zone, float x, float z );
	BOOL IsValidName(const char* name);
	void SendTargetHP( BYTE echo, int tid, int damage = 0 );
	BOOL IsValidSlotPos( _ITEM_TABLE* pTable, int destpos );
	void SetUserAbility(bool bSendPacket = true);
	void LevelChange(short level, BYTE type=TRUE);	// type : TRUE => level up, FALSE => level down
	void SetSlotItemValue();
	void SendTimeStatus(); // TO-DO: Deprecate
	void SendTime();
	void SendWeather();
	void SendPremiumInfo();
	void SetZoneAbilityChange();
	void SetMaxMp();
	void SetMaxHp(int iFlag=0); // 0:default, 1:hp�� maxhp��ŭ ä���ֱ�
	void ExpChange(__int64 iExp);
	void LogOut();
	void SendMyInfo();
	void SendServerChange(char *ip, uint8 bInit);
	void Send2AI_UserUpdateInfo(bool initialInfo = false);

	virtual void GetInOut(Packet & result, uint8 bType);
	void UserInOut(uint8 bType);

	void GetUserInfo(Packet & pkt);
	void SendUserStatusUpdate(UserStatus type, UserStatusBehaviour status);
	virtual void Initialize();
	
	void ChangeFame(uint8 bFame);
	void SendServerIndex();

	void SendToRegion(Packet *pkt, CUser *pExceptUser = NULL);

	virtual void OnDeath(Unit *pKiller);

	// Clan system
	void SendClanUserStatusUpdate(bool bToRegion = true);

	void SendPartyStatusUpdate(uint8 bStatus, uint8 bResult = 0);

	//Magic System - rewrite
	bool CanUseItem(long itemid, uint16 count); //Should place this with other item related functions

	bool CheckExistEvent(uint16 sQuestID, uint8 bQuestState);

	//Zone checks
	bool isAttackZone();

	void ResetWindows();

	void CloseProcess();
	virtual ~CUser() {}

private:
	static ChatCommandTable s_commandTable;
	GameState m_state;

	// quest ID | quest state (need to replace with enum)
	typedef std::map<uint16, uint8> QuestMap;
	QuestMap m_questMap;
};
