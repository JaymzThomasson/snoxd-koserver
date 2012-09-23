// User.h: interface for the CUser class.
// 
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_USER_H__5FEC1968_ED75_4AAF_A4DB_CB48F6940B2E__INCLUDED_)
#define AFX_USER_H__5FEC1968_ED75_4AAF_A4DB_CB48F6940B2E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning(disable : 4786)

#include "IOCPSocket2.h"
#include "define.h"
#include "GameDefine.h"
#include "MagicProcess.h"
#include "Npc.h"
#include "EVENT.h"
#include "EVENT_DATA.h"
#include "LOGIC_ELSE.h"
#include "EXEC.h"     

#include <list>
typedef	 std::list<_EXCHANGE_ITEM*>		ItemList;
typedef  list<int>				UserEventList;	// 이밴트를 위하여 ^^;

#define BANISH_DELAY_TIME    30

class CEbenezerDlg;
class CUser : public CIOCPSocket2  
{
public:
	_USER_DATA*	m_pUserData;

	char	m_strAccountID[MAX_ID_SIZE+1];	// Login -> Select Char 까지 한시적으로만 쓰는변수. 이외에는 _USER_DATA 안에있는 변수를 쓴다...agent 와의 데이터 동기화를 위해...
	
	bool	m_bSelectedCharacter;
	bool	m_bStoreOpen;

	short	m_RegionX;						// 현재 영역 X 좌표
	short	m_RegionZ;						// 현재 영역 Z 좌표

	__int64		m_iMaxExp;						// 다음 레벨이 되기 위해 필요한 Exp량
	unsigned short	m_sMaxWeight;					// 들 수 있는 최대 무게
	BYTE    m_sSpeed;						// 스피드

	short	m_sBodyAc;						// 맨몸 방어력

	short	m_sTotalHit;					// 총 타격공격력	
	short	m_sTotalAc;						// 총 방어력
	float	m_sTotalHitrate;				// 총 공격성공 민첩성
	float	m_sTotalEvasionrate;			// 총 방어 민첩성

	short   m_sItemMaxHp;                   // 아이템 총 최대 HP Bonus
	short   m_sItemMaxMp;                   // 아이템 총 최대 MP Bonus
	unsigned short	m_sItemWeight;					// 아이템 총무게
	short	m_sItemHit;						// 아이템 총타격치
	short	m_sItemAc;						// 아이템 총방어력
	short	m_sItemStr;						// 아이템 총힘 보너스
	short	m_sItemSta;						// 아이템 총체력 보너스
	short	m_sItemDex;						// 아이템 총민첩성 보너스
	short	m_sItemIntel;					// 아이템 총지능 보너스
	short	m_sItemCham;					// 아이템 총매력보너스
	short	m_sItemHitrate;					// 아이템 총타격율
	short	m_sItemEvasionrate;				// 아이템 총회피율

	BYTE	m_bFireR;						// 불 마법 저항력
	BYTE	m_bColdR;						// 얼음 마법 저항력
	BYTE	m_bLightningR;					// 전기 마법 저항력
	BYTE	m_bMagicR;						// 기타 마법 저항력
	BYTE	m_bDiseaseR;					// 저주 마법 저항력
	BYTE	m_bPoisonR;						// 독 마법 저항력

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

	BYTE	m_bResHpType;					// HP 회복타입
	BYTE	m_bWarp;						// 존이동중...
	BYTE	m_bNeedParty;					// 파티....구해요

	short	m_sPartyIndex;
	short	m_sExchangeUser;				// 교환중인 유저
	BYTE	m_bExchangeOK;

	ItemList	m_ExchangeItemList;
	_ITEM_DATA	m_MirrorItem[HAVE_MAX];			// 교환시 백업 아이템 리스트를 쓴다.

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
	BYTE	m_bStrAmount;
	BYTE	m_bStaAmount;
	BYTE	m_bDexAmount;
	BYTE	m_bIntelAmount;
	BYTE	m_bChaAmount;
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
		
	CEbenezerDlg* m_pMain;
	CMagicProcess m_MagicProcess;

	float	m_fSpeedHackClientTime, m_fSpeedHackServerTime;
	BYTE	m_bSpeedHackCheck;

	float	m_fBlinkStartTime;			// When did you start to blink?

	short	m_sAliveCount;

	DWORD	m_bAbnormalType;			// Is the player normal,a giant, or a dwarf?

	short	m_sWhoKilledMe;				// Who killed me???
	__int64		m_iLostExp;					// Experience point that was lost when you died.

	float	m_fLastTrapAreaTime;		// The last moment you were in the trap area.

	BOOL	m_bZoneChangeFlag;			// 성용씨 미워!!

	BYTE	m_bRegeneType;				// Did you die and go home or did you type '/town'?

	float	m_fLastRegeneTime;			// The last moment you got resurrected.

	BOOL	m_bZoneChangeSameZone;		// Did the server change when you warped?

	// 이밴트용 관련.... 정애씨 이거 보면 코카스 쏠께요 ^^;
//	int					m_iSelMsgEvent[5];	// 실행중인 선택 메세지박스 이벤트
	int					m_iSelMsgEvent[MAX_MESSAGE_EVENT];	// 실행중인 선택 메세지박스 이벤트
	short				m_sEventNid;		// 마지막으로 선택한 이벤트 NPC 번호
	UserEventList		m_arUserEvent;		// 실행한 이벤트 리스트

	char	m_strCouponId[MAX_COUPON_ID_LENGTH];		// What was the number of the coupon?
	int		m_iEditBoxEvent;

	short	m_sEvent[MAX_CURRENT_EVENT];				// 이미 실행된 이밴트 리스트들 :)
	

public:
	__forceinline bool isBanned() { return getAuthority() == AUTHORITY_BANNED; };
	__forceinline bool isMuted() { return getAuthority() == AUTHORITY_MUTED; };
	__forceinline bool isAttackDisabled() { return getAuthority() == AUTHORITY_ATTACK_DISABLED; };
	__forceinline bool isGM() { return getAuthority() == AUTHORITY_GAME_MASTER; };
	__forceinline bool isLimitedGM() { return getAuthority() == AUTHORITY_LIMITED_GAME_MASTER; };

	__forceinline bool isDead() { return m_bResHpType == USER_DEAD || m_pUserData->m_sHp <= 0; };

	__forceinline bool isInParty() { return m_sPartyIndex != -1; };
	__forceinline bool isInClan() { return m_pUserData->m_bKnights != -1; };
	__forceinline bool isTrading() { return m_sExchangeUser != -1; };
	__forceinline bool isStoreOpen() { return m_bStoreOpen; };

	__forceinline BYTE getNation() { return m_pUserData->m_bNation; };
	__forceinline BYTE getLevel() { return m_pUserData->m_bLevel; };
	__forceinline BYTE getZoneID() { return m_pUserData->m_bZone; };
	__forceinline BYTE getAuthority() { return m_pUserData->m_bAuthority; };
	__forceinline C3DMap * GetMap() { return m_pMap; };

	__forceinline uint16 GetSPosX() { return uint16(m_pUserData->m_curx * 10); };
	__forceinline uint16 GetSPosY() { return uint16(m_pUserData->m_cury * 10); };
	__forceinline uint16 GetSPosZ() { return uint16(m_pUserData->m_curz * 10); };

	void SendLoyaltyChange(int32 nChangeAmount = 0);

	void RecvDeleteChar( char* pBuf );
	BOOL ExistComEvent(int eventid);
	void SaveComEvent(int eventid);
	BOOL CheckItemCount(int itemid, short min, short max);
	void RecvEditBox(char *pBuf);
	BOOL CheckCouponUsed();
	BOOL CheckRandom(short percent);
	void NativeZoneReturn();
	void EventMoneyItemGet( int itemid, int count );
	void KickOutZoneUser(BOOL home = FALSE, int nZoneID = 21);
	void TrapProcess();
	BOOL JobGroupCheck(short jobgroupid);
	void SelectMsg(EXEC* pExec);
	void SendNpcSay(EXEC* pExec);
	BOOL CheckClass(short class1, short class2, short class3, short class4, short class5, short class6);
	void Make_public_key();
	void RecvSelectMsg(char *pBuf);
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
	BOOL RunEvent(EVENT_DATA *pEventData);
	BOOL RunNpcEvent(CNpc* pNpc, EXEC* pExec);
	BOOL CheckEventLogic(EVENT_DATA* pEventData);
	void ClientEvent(char* pBuf);
	void KickOut( char* pBuf );
	void SetLogInInfoToDB(BYTE bInit);
	void BlinkStart();
	void BlinkTimeCheck(float currenttime);
	void PartyBBSNeeded(char *pBuf, BYTE type);
	void PartyBBSDelete(char *pBuf);
	void PartyBBSRegister(char *pBuf);
	void PartyBBS(char *pBuf);
	BOOL WarpListObjectEvent(_OBJECT_EVENT *pEvent);
	BOOL FlagObjectEvent(_OBJECT_EVENT *pEvent, int nid);
	BOOL GateLeverObjectEvent(_OBJECT_EVENT *pEvent, int nid);
	BOOL BindObjectEvent(_OBJECT_EVENT *pEvent);
	void InitType3();
	BOOL GetWarpList( int warp_group );
	void ServerChangeOk( char* pBuf );
	void ZoneConCurrentUsers( char* pBuf );
	void SelectWarpList( char* pBuf );
	void GoldChange(short tid, int gold);
	void AllSkillPointChange();
	void AllPointChange();
	void ClassChangeReq();
	void FriendProcess(char *pBuf);
	void FriendRequest(char *pBuf);
	void FriendModify(char *pBuf);
	void FriendReport(char *pBuf);
	void RecvFriendProcess(char *pBuf);
	void RecvFriendRequest(char *pBuf);
	void RecvFriendModify(char *pBuf);
	BYTE GetFriendStatus(char * charName, short & sid);
	CUser* GetItemRoutingUser(int itemid, short itemcount);
	void Home();
	bool GetStartPosition(short & x, short & y, BYTE bZone = 0);
	int GetEmptySlot( int itemid, int bCountable );
	void InitType4();
	void WarehouseProcess( char* pBuf );
	short GetACDamage(int damage, short tid);
	short GetMagicDamage(int damage, short tid);
	void Type3AreaDuration( float currenttime);
	void ServerStatusCheck();
	void SpeedHackTime( char* pBuf );
	void OperatorCommand( char* pBuf );
	void ItemRemove( char* pBuf );
	void SendAllKnightsID();
	BYTE ItemCountChange(int itemid, int type, int amount);
	void Type4Duration(float currenttime);
	void ItemRepair( char* pBuf );
	int ExchangeDone();
	void HPTimeChange( float currenttime );
	void HPTimeChangeType3( float currenttime );
	void ItemDurationChange( int slot, int maxvalue, int curvalue, int amount );
	void ItemWoreOut( int type, int damage );
	void Dead();
	void LoyaltyDivide( short tid );
	void UserDataSaveToAgent();
	void CountConcurrentUser();
	void SendUserInfo(char *temp_send, int &index);
	void ChatTargetSelect( char* pBuf );
	BOOL ItemEquipAvailable( _ITEM_TABLE* pTable );
	void ClassChange( char* pBuf );
	void HpChange(int amount, int type=0, bool attack=false);
	void MSpChange(int amount);
	void SendPartyHPUpdate();
	void UpdateGameWeather( char* pBuf, BYTE type );
	void ObjectEvent( char* pBuf );
	void SendAnvilRequest(int nid);
	void SkillPointChange( char* pBuf );

	// Trade system
	BOOL ExecuteExchange();
	void ExchangeProcess(char* pBuf);
	void InitExchange(BOOL bStart);
	void ExchangeCancel();
	void ExchangeDecide();
	void ExchangeAdd(char* pBuf);
	void ExchangeAgree(char* pBuf);
	void ExchangeReq(char* pBuf);

	// Merchant system (both types)
	void MerchantProcess(char *pBuf);

	// regular merchants
	void MerchantOpen(char *pBuf);
	void MerchantClose();
	void MerchantItemAdd(char *pBuf);
	void MerchantItemCancel(char *pBuf);
	void MerchantItemList(char *pBuf);
	void MerchantItemBuy(char *pBuf);
	void MerchantInsert(char *pBuf);
	void CancelMerchant();

	// buying merchants
	void BuyingMerchantOpen(char *pBuf);
	void BuyingMerchantClose();
	void BuyingMerchantInsert(char *pBuf);
	void BuyingMerchantList(char *pBuf);
	void BuyingMerchantBuy(char *pBuf);

	void PartyDelete();
	void PartyRemove( int memberid );
	void PartyInsert();
	void PartyCancel();
	void PartyRequest( int memberid, BOOL bCreate );
	void PartyProcess( char* pBuf );
	void SendNotice();
	void UserLookChange( int pos, int itemid, int durability );
	void SpeedHackUser();
	void VersionCheck(char *pBuf);
	void LoyaltyChange( short tid );
	void StateChange( char* pBuf );
	void StateChangeServerDirect(BYTE bType, int nValue);
	void PointChange( char* pBuf );
	void ZoneChange( int zone, float x, float z );
	void ItemGet( char* pBuf );
	BOOL IsValidName( char* name );
	void AllCharInfoToAgent();
	void SelNationToAgent( char* pBuf );
	void DelCharToAgent( char* pBuf );
	void NewCharToAgent( char* pBuf );
	void GameStart(char* pBuf);
	void BundleOpenReq( char* pBuf );
	void SendTargetHP( BYTE echo, int tid, int damage = 0 );
	void ItemTrade( char* pBuf );
	void NpcEvent( char* pBuf );
	BOOL IsValidSlotPos( _ITEM_TABLE* pTable, int destpos );
	void ItemMove( char* pBuf );
	void Warp( char* pBuf );
	void RequestNpcIn( char* pBuf );
	void SetUserAbility();
	void LevelChange(short level, BYTE type=TRUE);	// type : TRUE => level up, FALSE => level down
	short GetDamage(short tid, int magicid);
	void SetSlotItemValue();
	BYTE GetHitRate(float rate);
	void RequestUserIn( char* pBuf );
	void InsertRegion(int del_x, int del_z);
	void RemoveRegion( int del_x, int del_z );
	void RegisterRegion();
	void SetDetailData();
	void SendTimeStatus(); // TO-DO: Deprecate
	void SendTime();
	void SendWeather();
	void SendPremiumInfo();
	void SetZoneAbilityChange(BYTE zone);
	void Regene(char* pBuf, int magicid = 0);
	void SetMaxMp();
	void SetMaxHp(int iFlag=0); // 0:default, 1:hp를 maxhp만큼 채워주기
	void ExpChange(__int64 iExp);
	void Chat( char* pBuf );
	void LogOut();
	void SelCharToAgent( char* pBuf );
	void SendMyInfo();
	void SelectCharacter( char* pBuf );
	void Send2AI_UserUpdateInfo(bool initialInfo = false);
	void Attack( char* pBuf );
	void UserInOut( BYTE Type );
	void GetUserInfo(char *buff, int & buff_index);
	void Initialize();
	void MoveProcess( char* pBuf );
	void Rotate( char* pBuf );
	void LoginProcess( char* pBuf );
	void Parsing( int len, char* pData );

	void SendServerIndex();
	void RentalSystem(char *pData);

	void SkillDataProcess(char *pData);
	void SkillDataSave(char *pData);
	void SkillDataLoad(char *pData);
	void RecvSkillDataLoad(char *pData);

	// from the client
	void ShoppingMall(char *pData);
	void HandleStoreClose();

	// from Aujard
	void RecvStore(char *pData);
	void RecvStoreClose(char *pData);

	void LetterSystem(char *pData);

	void ResetWindows();

	void CloseProcess();
	CUser();
	virtual ~CUser();
};

#endif // !defined(AFX_USER_H__5FEC1968_ED75_4AAF_A4DB_CB48F6940B2E__INCLUDED_)
