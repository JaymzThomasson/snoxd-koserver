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

class CEbenezerDlg;
class CUser : public KOSocket
{
public:
	_USER_DATA*	m_pUserData;

	std::string m_strAccountID;

	bool	m_bSelectedCharacter;
	bool	m_bStoreOpen;

	bool	m_bIsMerchanting; //Is the character merchanting already?
	int16	m_sMerchantsSocketID;
	std::list<uint16>	m_arMerchantLookers;
	_MERCH_DATA	m_arSellingItems[MAX_MERCH_ITEMS]; //What is this person selling? Stored in "_MERCH_DATA" structure.

	//Magic System Cooldown checks

	SkillCooldownList	m_CoolDownList;

	short	m_RegionX, m_RegionZ;
	int16	m_sDirection;

	int64	m_iMaxExp;
	uint32	m_sMaxWeight;
	uint16   m_sSpeed;	// NOTE: Currently unused

	short	m_sTotalHit;					// �� Ÿ�ݰ��ݷ�	
	short	m_sTotalAc;						// �� ������
	float	m_sTotalHitrate;				// �� ���ݼ��� ��ø��
	float	m_sTotalEvasionrate;			// �� ���� ��ø��

	short   m_sItemMaxHp;                   // ������ �� �ִ� HP Bonus
	short   m_sItemMaxMp;                   // ������ �� �ִ� MP Bonus
	uint32	m_sItemWeight;					// ������ �ѹ���
	short	m_sItemHit;						// ������ ��Ÿ��ġ
	short	m_sItemAc;						// ������ �ѹ�����
	short	m_sItemHitrate;					// ������ ��Ÿ����
	short	m_sItemEvasionrate;				// ������ ��ȸ����

	uint16	m_sStatItemBonuses[STAT_COUNT];
	uint8	m_bStatBuffs[STAT_COUNT];

	BYTE	m_bFireR;						// �� ���� ���׷�
	BYTE	m_bColdR;						// ���� ���� ���׷�
	BYTE	m_bLightningR;					// ���� ���� ���׷�
	BYTE	m_bMagicR;						// ��Ÿ ���� ���׷�
	BYTE	m_bDiseaseR;					// ���� ���� ���׷�
	BYTE	m_bPoisonR;						// �� ���� ���׷�

	BYTE    m_bMagicTypeLeftHand;			// The type of magic item in user's left hand  
	BYTE    m_bMagicTypeRightHand;			// The type of magic item in user's right hand
	short   m_sMagicAmountLeftHand;         // The amount of magic item in user's left hand
	short	m_sMagicAmountRightHand;        // The amount of magic item in user's left hand

	short   m_sDaggerR;						// Resistance to Dagger
	short   m_sSwordR;						// Resistance to Sword
	short	m_sAxeR;						// Resistance to Axe
	short	m_sMaceR;						// Resistance to Mace
	short	m_sSpearR;						// Resistance to Spear
	short	m_sBowR;						// Resistance to Bow		

	short	m_iMaxHp;
	short	m_iMaxMp;
	
	C3DMap * m_pMap;

	float	m_fWill_x;
	float	m_fWill_z;
	float	m_fWill_y;

	BYTE	m_bResHpType;
	BYTE	m_bWarp;
	BYTE	m_bNeedParty;

	short	m_sPartyIndex;
	bool	m_bPartyLeader;
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

	float	m_fHPLastTime[MAX_TYPE3_REPEAT];		// For Automatic HP recovery and Type 3 durational HP recovery.
	float	m_fHPStartTime[MAX_TYPE3_REPEAT];
	short	m_bHPAmount[MAX_TYPE3_REPEAT];
	BYTE	m_bHPDuration[MAX_TYPE3_REPEAT];
	BYTE	m_bHPInterval[MAX_TYPE3_REPEAT];
	short	m_sSourceID[MAX_TYPE3_REPEAT];
	BOOL	m_bType3Flag;

	float	m_fAreaLastTime;			// For Area Damage spells Type 3.
	float   m_fAreaStartTime;
	BYTE    m_bAreaInterval;
	int     m_iAreaMagicID;

	BYTE	m_bAttackSpeedAmount;		// For Character stats in Type 4 Durational Spells.
	BYTE    m_bSpeedAmount;
	short   m_sACAmount;
	BYTE    m_bAttackAmount;
	short	m_sMaxHPAmount;
	BYTE	m_bHitRateAmount;
	short	m_sAvoidRateAmount;

	BYTE	m_bFireRAmount;
	BYTE	m_bColdRAmount;
	BYTE	m_bLightningRAmount;
	BYTE	m_bMagicRAmount;
	BYTE	m_bDiseaseRAmount;
	BYTE	m_bPoisonRAmount;	
	
	short   m_sDuration1 ;  float   m_fStartTime1 ;
	short   m_sDuration2 ;  float   m_fStartTime2 ;
	short   m_sDuration3 ;  float   m_fStartTime3 ;
	short   m_sDuration4 ;  float   m_fStartTime4 ;
	short   m_sDuration5 ;  float   m_fStartTime5 ;
	short   m_sDuration6 ;  float   m_fStartTime6 ;
	short   m_sDuration7 ;  float   m_fStartTime7 ;
	short   m_sDuration8 ;  float   m_fStartTime8 ;
	short   m_sDuration9 ;  float   m_fStartTime9 ;

	BYTE	m_bType4Buff[MAX_TYPE4_BUFF];
	BOOL	m_bType4Flag;
		
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

	// �̹�Ʈ�� ����.... ���־� �̰� ���� ��ī�� �򲲿� ^^;
//	int					m_iSelMsgEvent[5];	// �������� ���� �޼����ڽ� �̺�Ʈ
	int					m_iSelMsgEvent[MAX_MESSAGE_EVENT];	// �������� ���� �޼����ڽ� �̺�Ʈ
	short				m_sEventNid;		// ���������� ������ �̺�Ʈ NPC ��ȣ
	UserEventList		m_arUserEvent;		// ������ �̺�Ʈ ����Ʈ

	short	m_sEvent[MAX_CURRENT_EVENT];				// �̹� ������ �̹�Ʈ ����Ʈ�� :)
	

public:
	__forceinline bool isBanned() { return getAuthority() == AUTHORITY_BANNED; }
	__forceinline bool isMuted() { return getAuthority() == AUTHORITY_MUTED; }
	__forceinline bool isAttackDisabled() { return getAuthority() == AUTHORITY_ATTACK_DISABLED; }
	__forceinline bool isGM() { return getAuthority() == AUTHORITY_GAME_MASTER; }
	__forceinline bool isLimitedGM() { return getAuthority() == AUTHORITY_LIMITED_GAME_MASTER; }

	__forceinline bool isDead() { return m_bResHpType == USER_DEAD || m_pUserData->m_sHp <= 0; }
	__forceinline bool isBlinking() { return m_bAbnormalType == ABNORMAL_BLINKING; }

	__forceinline bool isInGame() { return GetState() == GAME_STATE_INGAME; }
	__forceinline bool isInParty() { return m_sPartyIndex != -1; }
	__forceinline bool isInClan() { return m_pUserData->m_bKnights > 0; }
	__forceinline bool isClanLeader() { return getFame() == CHIEF; }
	__forceinline bool isClanAssistant() { return getFame() == VICECHIEF; }

	__forceinline bool isTrading() { return m_sExchangeUser != -1; }
	__forceinline bool isStoreOpen() { return m_bStoreOpen; }
	__forceinline bool isMerchanting() { return m_bIsMerchanting; }

	__forceinline BYTE getNation() { return m_pUserData->m_bNation; }
	__forceinline BYTE getLevel() { return m_pUserData->m_bLevel; }
	__forceinline BYTE getZoneID() { return m_pUserData->m_bZone; }
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

	__forceinline C3DMap * GetMap() { return m_pMap; }

	__forceinline uint16 GetSPosX() { return uint16(m_pUserData->m_curx * 10); }
	__forceinline uint16 GetSPosY() { return uint16(m_pUserData->m_cury * 10); }
	__forceinline uint16 GetSPosZ() { return uint16(m_pUserData->m_curz * 10); }

	CUser(uint16 socketID, SocketMgr *mgr); 

	virtual void OnConnect();
	virtual void OnDisconnect();
	virtual bool HandlePacket(Packet & pkt);

	void SendLoyaltyChange(int32 nChangeAmount = 0);

	BOOL ExistComEvent(int eventid);
	void SaveComEvent(int eventid);
	BOOL CheckItemCount(int itemid, short min, short max);
	BOOL CheckRandom(short percent);
	void NativeZoneReturn();
	void EventMoneyItemGet( int itemid, int count );
	void KickOutZoneUser(BOOL home = FALSE, int nZoneID = 21);
	void TrapProcess();
	BOOL JobGroupCheck(short jobgroupid);
	void SelectMsg(EXEC* pExec);
	void SendNpcSay(EXEC* pExec);
	BOOL CheckClass(short class1, short class2, short class3, short class4, short class5, short class6);
	BOOL GiveItem(int itemid, short count, bool send_packet = true);
	BOOL RobItem(int itemid, short count);
	BOOL CheckExistItem(int itemid, short count);
	BOOL CheckWeight(int itemid, short count);
	BOOL CheckSkillPoint(BYTE skillnum, BYTE min, BYTE max);
	BOOL GoldLose(unsigned int gold);
	void GoldGain(int gold);
	void SendItemWeight();
	void ItemLogToAgent(const char *srcid, const char *tarid, int type, __int64 serial, int itemid, int count, int durability);
	void TestPacket( char* pBuf );
	void BlinkStart();
	void BlinkTimeCheck(float currenttime);
	void InitType3();
	void GoldChange(short tid, int gold);
	CUser* GetItemRoutingUser(int itemid, short itemcount);
	bool GetStartPosition(short & x, short & y, BYTE bZone = 0);
	int GetEmptySlot( int itemid, int bCountable );
	void InitType4();
	short GetACDamage(int damage, short tid);
	short GetMagicDamage(int damage, short tid);
	void Type3AreaDuration( float currenttime);
	void SendAllKnightsID();
	void SendStackChange(uint32 nItemID, uint32 nCount /* needs to be 4 bytes, not a bug */, uint16 sDurability, uint8 bPos, bool bNewItem = false);
	void Type4Duration(float currenttime);
	void HPTimeChange( float currenttime );
	void HPTimeChangeType3( float currenttime );
	void ItemDurationChange(uint8 slot, uint16 maxValue, int16 curValue, uint16 amount);
	void SendDurability(uint8 slot, uint16 durability);
	void SendItemMove(bool bFail = false);
	void ItemWoreOut( int type, int damage );
	void Dead();
	void LoyaltyDivide( short tid );
	void SendUserInfo(Packet & result);
	BOOL ItemEquipAvailable( _ITEM_TABLE* pTable );
	void HpChange(int amount, int type=0, bool attack=false);
	void MSpChange(int amount);
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
	void StateChangeServerDirect(BYTE bType, int nValue);

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
	void FriendModify(Packet & pkt);
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
	void ShoppingMall(Packet & pkt);
	void RecvStore(Packet & pkt); // from Aujard
	void HandleStoreClose();
	void RecvStoreClose(Packet & pkt); // from Aujard
	void LetterSystem(Packet & pkt);

	void HandleHelmet(Packet & pkt);

	void SendNotice();
	void UserLookChange( int pos, int itemid, int durability );
	void SpeedHackUser();
	void LoyaltyChange(short tid);
	void ChangeNP(short sAmount, bool bDistributeToParty = true);
	void ZoneChange( int zone, float x, float z );
	BOOL IsValidName(const char* name);
	void SendTargetHP( BYTE echo, int tid, int damage = 0 );
	BOOL IsValidSlotPos( _ITEM_TABLE* pTable, int destpos );
	void SetUserAbility();
	void LevelChange(short level, BYTE type=TRUE);	// type : TRUE => level up, FALSE => level down
	short GetDamage(short tid, int magicid);
	void SetSlotItemValue();
	BYTE GetHitRate(float rate);
	void InsertRegion(int del_x, int del_z);
	void RemoveRegion( int del_x, int del_z );
	void RegisterRegion();
	void SetDetailData();
	void SendTimeStatus(); // TO-DO: Deprecate
	void SendTime();
	void SendWeather();
	void SendPremiumInfo();
	void SetZoneAbilityChange(BYTE zone);
	void SetMaxMp();
	void SetMaxHp(int iFlag=0); // 0:default, 1:hp�� maxhp��ŭ ä���ֱ�
	void ExpChange(__int64 iExp);
	void LogOut();
	void SendMyInfo();
	void SendServerChange(char *ip, uint8 bInit);
	void Send2AI_UserUpdateInfo(bool initialInfo = false);
	void UserInOut( BYTE Type );
	void GetUserInfo(Packet & pkt);
	void Initialize();
	
	void ChangeFame(uint8 bFame);
	void SendServerIndex();

	void SendToRegion(Packet *pkt, CUser *pExceptUser = NULL);

	void OnDeath();
	void SendDeathAnimation();

	// Clan system
	void SendClanUserStatusUpdate(bool bToRegion = true);

	void SendPartyStatusUpdate(uint8 bStatus, uint8 bResult = 0);

	//Magic System - rewrite
	void MagicSystem(Packet & pkt);
	bool CheckSkillCooldown(uint32 magicid, time_t skill_received_time);
	void LogSkillCooldown(uint32 magicid, time_t skill_received_time);
	void MagicType(uint16 effect_type, uint8 sub_type, uint32 magicid, uint16 sid, uint16 tid, uint16 data1, uint16 data2, uint16 data3, uint16 data4, uint16 data5, uint16 data6, uint16 data7);
	void MagicType1(uint32 magicid, uint16 sid, uint16 tid, uint16 data1, uint16 data2, uint16 data3, uint16 data4, uint16 data5, uint16 data6, uint16 data7);
	void MagicType4(uint32 magicid, uint16 sid, uint16 tid, uint16 data1, uint16 data2, uint16 data3, uint16 data4);
	bool CanCast(uint32 magicid, uint16 sid, uint16 tid);
	bool CanUseItem(long itemid); //Should place this with other item related functions

	//Zone checks
	bool isAttackZone();

	void ResetWindows();

	void CloseProcess();
	virtual ~CUser() {}

private:
	static ChatCommandTable s_commandTable;
	GameState m_state;
};
