#pragma once

//////////////////// ��� Define ////////////////////
#define KARUWARRIOR		101		// ī�����
#define KARUROGUE			102		// ī��α�
#define KARUWIZARD			103		// ī�縶��
#define KARUPRIEST			104		// ī����
#define BERSERKER			105		// ��Ŀ
#define GUARDIAN			106		// �����
#define HUNTER				107		// ����
#define PENETRATOR			108		// ���Ʈ������
#define SORSERER			109		// �Ҽ���
#define NECROMANCER			110		// ��ũ�θǼ�
#define SHAMAN				111		// ����
#define DARKPRIEST			112		// ��ũ�����Ʈ

#define ELMORWARRRIOR		201		// �������
#define ELMOROGUE			202		// ����α�
#define ELMOWIZARD			203		// ���𸶹�
#define ELMOPRIEST			204		// ������
#define BLADE				205		// ���̵�
#define PROTECTOR			206		// �������
#define RANGER				207		// �����
#define ASSASSIN			208		// ��ؽ�
#define MAGE				209		// ������
#define ENCHANTER			210		// ��þ��
#define CLERIC				211		// Ŭ����
#define DRUID				212		// ����̵�
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// Race Define
/////////////////////////////////////////////////////
#define KARUS_BIG			1
#define KARUS_MIDDLE		2
#define KARUS_SMALL			3
#define KARUS_WOMAN			4
#define BABARIAN			11
#define ELMORAD_MAN			12
#define ELMORAD_WOMAN		13

// Ÿ�ݺ� ����� //
#define GREAT_SUCCESS			0X01		// �뼺��
#define SUCCESS					0X02		// ����
#define NORMAL					0X03		// ����
#define	FAIL					0X04		// ���� 

enum ItemMovementType
{
	ITEM_INVEN_SLOT			= 1,
	ITEM_SLOT_INVEN			= 2,
	ITEM_INVEN_INVEN		= 3,
	ITEM_SLOT_SLOT			= 4,
	ITEM_INVEN_ZONE			= 5,
	ITEM_ZONE_INVEN			= 6,
	ITEM_INVEN_TO_COSP		= 7,  // Inventory -> Cospre bag
	ITEM_COSP_TO_INVEN		= 8,  // Cospre bag -> Inventory
	ITEM_INVEN_TO_MBAG		= 9,  // Inventory -> Magic bag
	ITEM_MBAG_TO_INVEN		= 10, //Magic bag -> Magic bag
	ITEM_MBAG_TO_MBAG		= 11 //Magic bag -> Magic bag
};

// Item Weapon Type Define
#define WEAPON_DAGGER			1
#define WEAPON_SWORD			2
#define WEAPON_AXE				3
#define WEAPON_MACE				4
#define WEAPON_SPEAR			5
#define WEAPON_SHIELD			6
#define WEAPON_BOW				7
#define WEAPON_LONGBOW			8
#define WEAPON_LAUNCHER			10
#define WEAPON_STAFF			11
#define WEAPON_ARROW			12	// ��ų ���
#define WEAPON_JAVELIN			13	// ��ų ���
#define WEAPON_MACE2			18
#define WEAPON_WORRIOR_AC		21	// ��ų ���
#define WEAPON_LOG_AC			22	// ��ų ���
#define WEAPON_WIZARD_AC		23	// ��ų ���
#define WEAPON_PRIEST_AC		24	// ��ų ���
////////////////////////////////////////////////////////////
// User Status //
#define USER_STANDING			0X01		// �� �ִ�.
#define USER_SITDOWN			0X02		// �ɾ� �ִ�.
#define USER_DEAD				0x03		// ��Ŷ�
//#define USER_BLINKING			0x04		// ��� ��Ƴ���!!!
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// Magic State
#define NONE				0x01
#define CASTING				0x02
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// Durability Type
#define ATTACK				0x01
#define DEFENCE				0x02
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// Knights Authority Type
/*
#define CHIEF				0x06
#define VICECHIEF			0x05*/
#define OFFICER				0x04
#define KNIGHT				0x03
//#define TRAINEE				0x02
#define PUNISH				0x01	

#define CHIEF				0x01	// ����
#define VICECHIEF			0x02	// �δ���
#define TRAINEE				0x05	// ���
#define COMMAND_CAPTAIN		100		// ���ֱ���
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// COMMUNITY TYPE DEFINE
#define CLAN_TYPE			0x01
#define KNIGHTS_TYPE		0x02
////////////////////////////////////////////////////////////

#define CLAN_COIN_REQUIREMENT	500000
#define CLAN_LEVEL_REQUIREMENT	20

#define ITEM_GOLD			900000000	// �� ������ ��ȣ...
#define ITEM_NO_TRADE		900000001	// �ŷ� �Ұ� �����۵�.... �񷯸ӱ� ũ�������� �̹�Ʈ >.<		

////////////////////////////////////////////////////////////
// EVENT TYPE DEFINE
#define ZONE_CHANGE			0x01
#define ZONE_TRAP_DEAD		0x02
#define ZONE_TRAP_AREA		0x03

////////////////////////////////////////////////////////////
// EVENT MISCELLANOUS DATA DEFINE
#define ZONE_TRAP_INTERVAL	   (1 * SECOND)		// Interval is one second right now.
#define ZONE_TRAP_DAMAGE	   10		// HP Damage is 10 for now :)

////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// SKILL POINT DEFINE
#define ORDER_SKILL			0x01
#define MANNER_SKILL		0X02
#define LANGUAGE_SKILL		0x03
#define BATTLE_SKILL		0x04
#define PRO_SKILL1			0x05
#define PRO_SKILL2			0x06
#define PRO_SKILL3			0x07
#define PRO_SKILL4			0x08

/////////////////////////////////////////////////////////////
// ITEM TYPE DEFINE
#define ITEM_TYPE_FIRE				0x01
#define ITEM_TYPE_COLD				0x02
#define ITEM_TYPE_LIGHTNING			0x03
#define ITEM_TYPE_POISON			0x04
#define ITEM_TYPE_HP_DRAIN			0x05
#define ITEM_TYPE_MP_DAMAGE			0x06
#define ITEM_TYPE_MP_DRAIN			0x07
#define ITEM_TYPE_MIRROR_DAMAGE		0x08

/////////////////////////////////////////////////////////////
// JOB GROUP TYPES
#define GROUP_WARRIOR				1
#define GROUP_ROGUE					2
#define GROUP_MAGE					3
#define GROUP_CLERIC				4
#define GROUP_ATTACK_WARRIOR		5
#define GROUP_DEFENSE_WARRIOR		6
#define GROUP_ARCHERER				7
#define GROUP_ASSASSIN				8
#define GROUP_ATTACK_MAGE			9
#define GROUP_PET_MAGE				10
#define GROUP_HEAL_CLERIC			11
#define GROUP_CURSE_CLERIC			12

//////////////////////////////////////////////////////////////
// USER ABNORMAL STATUS TYPES
#define ABNORMAL_NORMAL			0x01
#define ABNORMAL_GIANT			0x02
#define ABNORMAL_DWARF			0x03
#define ABNORMAL_BLINKING		0x04

//////////////////////////////////////////////////////////////
// Object Type
#define NORMAL_OBJECT		0
#define SPECIAL_OBJECT		1

//////////////////////////////////////////////////////////////
// REGENE TYPES
#define REGENE_NORMAL		0
#define REGENE_MAGIC		1
#define REGENE_ZONECHANGE	2

//////////////////////////////////////////////////////////////
// TYPE 3 ATTRIBUTE TYPES
#define ATTRIBUTE_FIRE				1
#define ATTRIBUTE_ICE				2
#define ATTRIBUTE_LIGHTNING			3

struct _CLASS_COEFFICIENT
{
	uint16	sClassNum;
	float	ShortSword;
	float	Sword;
	float	Axe;
	float	Club;
	float	Spear;
	float	Pole;
	float	Staff;
	float	Bow;
	float	HP;
	float	MP;
	float	SP;
	float	AC;
	float	Hitrate;
	float	Evasionrate;
};

struct _ZONE_ITEM {		// Bundle unit
	DWORD bundle_index;
	int itemid[6];
	short count[6];
	float x;
	float z;
	float y;
	uint32 time;
};

struct	_EXCHANGE_ITEM
{
	uint32	nItemID;
	uint32	nCount;
	uint16	sDurability;
	uint8	bSrcPos;
	uint64	nSerialNum;
};

struct _ITEM_TABLE
{
	uint32	m_iNum;
	uint8	m_bKind;
	uint8	m_bSlot;
	uint8	m_bRace;
	uint8	m_bClass;
	uint16	m_sDamage;
	uint16	m_sDelay;
	uint16	m_sRange;
	uint16	m_sWeight;
	uint16	m_sDuration;
	uint32	m_iBuyPrice;
	uint32	m_iSellPrice;
	uint16	m_sAc;
	uint8	m_bCountable;
	uint32	m_iEffect1;
	uint32	m_iEffect2;
	uint8	m_bReqLevel;
	uint8	m_bReqLevelMax;
	uint8	m_bReqRank;
	uint8	m_bReqTitle;
	uint8	m_bReqStr;
	uint8	m_bReqSta;
	uint8	m_bReqDex;
	uint8	m_bReqIntel;
	uint8	m_bReqCha;
	uint8	m_bSellingGroup;
	uint8	m_ItemType;
	uint16	m_sHitrate;
	uint16	m_sEvarate;
	uint16	m_sDaggerAc;
	uint16	m_sSwordAc;
	uint16	m_sMaceAc;
	uint16	m_sAxeAc;
	uint16	m_sSpearAc;
	uint16	m_sBowAc;
	uint8	m_bFireDamage;
	uint8	m_bIceDamage;
	uint8	m_bLightningDamage;
	uint8	m_bPoisonDamage;
	uint8	m_bHPDrain;
	uint8	m_bMPDamage;
	uint8	m_bMPDrain;
	uint8	m_bMirrorDamage;
	int16	m_bStrB; // TO-DO: Rename these, as they're not bytes anymore.
	int16	m_bStaB;
	int16	m_bDexB;
	int16	m_bIntelB;
	int16	m_bChaB;
	int16	m_MaxHpB;
	int16	m_MaxMpB;
	uint16	m_bFireR;
	uint16	m_bColdR;
	uint16	m_bLightningR;
	uint16	m_bMagicR;
	uint16	m_bPoisonR;
	uint16	m_bCurseR;

	__forceinline uint8 GetKind() { return m_bKind; }
	__forceinline uint8 GetItemGroup() { return m_bKind / 10; }

	__forceinline bool isDagger() { return GetItemGroup() == WEAPON_DAGGER; }
	__forceinline bool isSword() { return GetItemGroup() == WEAPON_SWORD; }
	__forceinline bool isAxe() { return GetItemGroup() == WEAPON_AXE; }
	__forceinline bool isMace() { return GetItemGroup() == WEAPON_MACE || GetItemGroup() == WEAPON_MACE2; }
	__forceinline bool isSpear() { return GetItemGroup() == WEAPON_SPEAR; }
	__forceinline bool isShield() { return GetItemGroup() == WEAPON_SHIELD; }
	__forceinline bool isStaff() { return GetItemGroup() == WEAPON_STAFF; }
	__forceinline bool isBow() { return GetItemGroup() == WEAPON_BOW || GetItemGroup() == WEAPON_LONGBOW; }
};

struct	_PARTY_GROUP
{
	WORD	wIndex;
	short	uid		[MAX_PARTY_USERS];
	BYTE	bItemRouting;
	std::string	WantedMessage;
	uint16	sWantedClass;

	_PARTY_GROUP()
	{
		for (int i = 0; i < MAX_PARTY_USERS; i++)
			uid[i] = -1; 

		bItemRouting = 0;
	};
};

class CUser;
struct _KNIGHTS_USER
{
	BYTE    byUsed;
	char	strUserName[MAX_ID_SIZE+1];
	CUser	*pSession;
	_KNIGHTS_USER()
	{
		byUsed = 0;
		memset(strUserName, 0x00, sizeof(strUserName));
		pSession = NULL;
	};
};

struct _ZONE_SERVERINFO
{
	short		sServerNo;
	char		strServerIP[20];
	_ZONE_SERVERINFO() {memset(strServerIP, 0x00, 20);};
};

struct _HOME_INFO
{
	uint8	bNation;
	uint32	ElmoZoneX;
	uint32	ElmoZoneZ;
	uint8	ElmoZoneLX;
	uint8	ElmoZoneLZ;
	uint32	KarusZoneX;
	uint32	KarusZoneZ;
	uint8	KarusZoneLX;
	uint8	KarusZoneLZ;
	uint32	FreeZoneX;
	uint32	FreeZoneZ;
	uint8	FreeZoneLX;
	uint8	FreeZoneLZ;
	uint32	BattleZoneX;
	uint32	BattleZoneZ;
	uint8	BattleZoneLX;
	uint8	BattleZoneLZ;
};

struct _KNIGHTS_CAPE
{
	uint16	sCapeIndex;
	uint32	nReqCoins;
	uint32	nReqClanPoints;	// clan point requirement
	uint8	byGrade;		// clan grade requirement
	uint8	byRanking;		// clan rank requirement (e.g. royal, accredited, etc)
};

struct _KNIGHTS_ALLIANCE
{
	uint16	sMainAllianceKnights;
	uint16	sSubAllianceKnights;
	uint16	sMercenaryClan_1;
	uint16	sMercenaryClan_2;
};

struct _START_POSITION
{
	uint16	ZoneID;
	uint16	sKarusX;
	uint16	sKarusZ;
	uint16	sElmoradX;
	uint16	sElmoradZ;
	uint16	sKarusGateX;
	uint16	sKarusGateZ;
	uint16	sElmoradGateX;
	uint16	sElmoradGateZ;
	uint8	bRangeX;
	uint8	bRangeZ;
};

struct _SERVER_RESOURCE
{
	uint32 nResourceID;
	char strResource[101];
	_SERVER_RESOURCE()
	{
		memset(strResource, 0x00, sizeof(strResource));
	};
};

struct _KNIGHTS_RATING
{
	uint32 nRank;
	uint16 sClanID;
	uint32 nPoints;
};

struct _USER_RANK
{
	uint16	nRank;  // shIndex for USER_KNIGHTS_RANK
	std::string strElmoUserID;
	std::string strKarusUserID;
	uint32	nSalary; // nMoney for USER_KNIGHTS_RANK
};

// TO-DO: Rewrite this system to be less script dependent for exchange logic.
// Coin requirements should be in the database, and exchanges should be grouped.
struct _ITEM_EXCHANGE
{
	uint32	nIndex;
	uint16	sNpcNum;
	uint8	bRandomFlag;

	uint32	nOriginItemNum[5];
	uint16	sOriginItemCount[5];

	uint32	nExchangeItemNum[5];
	uint16	sExchangeItemCount[5];
};

struct _QUEST_HELPER
{
	uint32	nIndex;
	uint8	bMessageType;
	uint8	bLevel;
	uint32	nExp;
	uint8	bClass;
	uint8	bNation;
	uint8	bQuestType;
	uint8	bZone;
	uint16	sNpcId;
	uint16	sEventDataIndex;
	uint8	bEventStatus;
	uint32	nEventTriggerIndex;
	uint32	nEventCompleteIndex;
	uint32	nExchangeIndex;
	uint32	nEventTalkIndex;
	std::string strLuaFilename;
};

#define QUEST_MOB_GROUPS		4
#define QUEST_MOBS_PER_GROUP	4
struct _QUEST_MONSTER
{
	uint16	sQuestNum;
	uint16	sNum[QUEST_MOB_GROUPS][QUEST_MOBS_PER_GROUP]; 
	uint16	sCount[QUEST_MOB_GROUPS];

	_QUEST_MONSTER()
	{
		memset(&sCount, 0, sizeof(sCount));
		memset(&sNum, 0, sizeof(sNum));
	}
};

enum SpecialQuestIDs
{
	QUEST_KILL_GROUP1	= 32001,
	QUEST_KILL_GROUP2	= 32002,
	QUEST_KILL_GROUP3	= 32003,
	QUEST_KILL_GROUP4	= 32004,
};

enum AuthorityTypes
{
	AUTHORITY_GAME_MASTER			= 0,
	AUTHORITY_PLAYER				= 1,
	AUTHORITY_MUTED					= 11,
	AUTHORITY_ATTACK_DISABLED		= 12,
	AUTHORITY_LIMITED_GAME_MASTER	= 250,
	AUTHORITY_BANNED				= 255
};

enum BuffType
{
	BUFF_TYPE_HP_MP					= 1,
	BUFF_TYPE_AC					= 2,
	BUFF_TYPE_SIZE					= 3,
	BUFF_TYPE_DAMAGE				= 4,
	BUFF_TYPE_ATTACK_SPEED			= 5,
	BUFF_TYPE_SPEED					= 6,
	BUFF_TYPE_STATS					= 7,
	BUFF_TYPE_RESISTANCES			= 8,
	BUFF_TYPE_ACCURACY				= 9,
	BUFF_TYPE_MAGIC_POWER			= 10,
	BUFF_TYPE_EXPERIENCE			= 11,
	BUFF_TYPE_WEIGHT				= 12,
	BUFF_TYPE_WEAPON_DAMAGE			= 13,
	BUFF_TYPE_WEAPON_AC				= 14,
	BUFF_TYPE_LOYALTY				= 15,
	BUFF_TYPE_NOAH_BONUS			= 16,
	BUFF_TYPE_PREMIUM_MERCHANT		= 17,
	BUFF_TYPE_ATTACK_SPEED_ARMOR	= 18, //Berserker
	BUFF_TYPE_DAMAGE_DOUBLE			= 19, //Critical Point
	BUFF_TYPE_DISABLE_TARGETING		= 20, //Smoke Screen / Light Shock
	BUFF_TYPE_BLIND					= 21, //Blinding (Strafe)
	BUFF_TYPE_FREEZE				= 22, //Freezing Distance
	BUFF_TYPE_INSTANT_MAGIC			= 23, //Instantly Magic
	BUFF_TYPE_DECREASE_RESIST		= 24, //Minor resist
	BUFF_TYPE_MAGE_ARMOR			= 25, //Fire / Ice / Lightning Armor
	BUFF_TYPE_PROHIBIT_INVIS		= 26, //Source Marking
	BUFF_TYPE_RESIS_AND_MAGIC_DMG	= 27, //Elysian Web
	BUFF_TYPE_TRIPLEAC_HALFSPEED	= 28, //Wall of Iron
	BUFF_TYPE_BLOCK_CURSE			= 29, //Counter Curse
	BUFF_TYPE_BLOCK_CURSE_REFLECT	= 30, //Curse Refraction
	BUFF_TYPE_ATTACK_SPEED2			= 31, //Outrage / Frenzy
	BUFF_TYPE_IGNORE_WEAPON			= 32, //Weapon cancellation
	BUFF_TYPE_PASSION_OF_SOUL		= 35, //Passion of the Soul
	BUFF_TYPE_FIRM_DETERMINATION	= 36, //Firm Determination
	BUFF_TYPE_SPEED2				= 40, //Cold Wave
	BUFF_TYPE_ATTACK_RANGE_ARMOR	= 43 //Inevitable Murderous
};

enum UserStatus
{
	USER_STATUS_DOT	= 1,
	USER_STATUS_POISON = 2,
	USER_STATUS_SPEED = 3,
	USER_STATUS_BLIND = 4,
	USER_STATUS_BLACK = 5
};

enum UserStatusBehaviour
{
	USER_STATUS_INFLICT	= 1,
	USER_STATUS_CURE	= 2
};

enum FlyingSantaOrAngel
{
	FLYING_NONE		= 0,
	FLYING_SANTA	= 1,
	FLYING_ANGEL	= 2
};

#include "../shared/database/structs.h"