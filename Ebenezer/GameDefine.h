#ifndef _GAMEDEFINE_H
#define _GAMEDEFINE_H

//////////////////// ��� Define ////////////////////
#define KARUWARRRIOR		101		// ī�����
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

// Item Move Direction Define 
#define ITEM_INVEN_SLOT			0x01
#define ITEM_SLOT_INVEN			0x02
#define ITEM_INVEN_INVEN		0x03
#define ITEM_SLOT_SLOT			0x04
#define ITEM_INVEN_ZONE			0x05
#define ITEM_ZONE_INVEN			0x06

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

#define MAX_CLAN			36
#define MAX_KNIGHTS_BANK	200
#define MAX_KNIGHTS_MARK	512

#define ITEM_GOLD			900000000	// �� ������ ��ȣ...
#define ITEM_NO_TRADE		900000001	// �ŷ� �Ұ� �����۵�.... �񷯸ӱ� ũ�������� �̹�Ʈ >.<		

////////////////////////////////////////////////////////////
// EVENT TYPE DEFINE
#define ZONE_CHANGE			0x01
#define ZONE_TRAP_DEAD		0x02
#define ZONE_TRAP_AREA		0x03

////////////////////////////////////////////////////////////
// EVENT MISCELLANOUS DATA DEFINE
#define ZONE_TRAP_INTERVAL	   1		// Interval is one second right now.
#define ZONE_TRAP_DAMAGE	   10		// HP Damage is 10 for now :)

////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// USER POINT DEFINE
#define STR					0x01
#define STA					0x02
#define DEX					0x03
#define INTEL				0x04
#define CHA					0x05

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
// ITEM LOG TYPE 
#define ITEM_MERCHANT_BUY			0x01
#define ITEM_MERCHANT_SELL			0x02
#define ITEM_MONSTER_GET			0x03
#define ITEM_EXCHANGE_PUT			0x04
#define ITEM_EXCHANGE_GET			0x05
#define ITEM_DESTROY				0x06
#define ITEM_WAREHOUSE_PUT			0x07
#define ITEM_WAREHOUSE_GET			0x08
#define ITEM_UPGRADE				0x09

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

//extern CRITICAL_SECTION g_LogFile_critical;

struct _CLASS_COEFFICIENT
{
	short	sClassNum;
	char	strClassName[30];
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
	float time;
};

struct	_EXCHANGE_ITEM
{
	int itemid;
	int count;
	short duration;
	BYTE pos;			//  ��ȯ�� �� �ڸ�..
	__int64	nSerialNum;	// item serial code
};

struct _ITEM_TABLE
{
	int   m_iNum;				// item num
	char  m_strName[50];		// item Name
	BYTE  m_bKind;				// item ���
	BYTE  m_bSlot;				// �����ġ
	BYTE  m_bRace;				// ��� ������ ��
	BYTE  m_bClass;				// ��� ������ Class
	short m_sDamage;			// �ִ� Ÿ��ġ
	short m_sDelay;				// ��ݽð�
	short m_sRange;				// ���(�ȿ�Ÿ�)
	short m_sWeight;			// ����
	short m_sDuration;			// ������
	int	  m_iBuyPrice;			// ��� ��°���
	int	  m_iSellPrice;			// ��� �Ĵ°���
	short m_sAc;				// ����
	BYTE  m_bCountable;			// ���� ���� ������
	int	  m_iEffect1;			// ���� ����Ʈ1
	int	  m_iEffect2;			// ���� ����Ʈ2
	BYTE  m_bReqLevel;			// �䱸 ����
	BYTE  m_bReqRank;			// �䱸 ���
	BYTE  m_bReqTitle;			// �䱸 ���
	BYTE  m_bReqStr;			// �䱸 ��
	BYTE  m_bReqSta;			// �䱸 ü��
	BYTE  m_bReqDex;			// �䱸 ��ø
	BYTE  m_bReqIntel;			// �䱸 ���
	BYTE  m_bReqCha;			// �䱸 �ŷ�
	BYTE  m_bSellingGroup;		// ���� ��� ��ǰ
	BYTE  m_ItemType;			// ��������� �Ǵ� ���������
	short m_sHitrate;			// Ÿ�ݷ�
	short m_sEvarate;			// ȸ���
	short m_sDaggerAc;			// ����1
	short m_sSwordAc;			// ����2
	short m_sMaceAc;			// ����3
	short m_sAxeAc;				// ����4
	short m_sSpearAc;			// ����5
	short m_sBowAc;				// ����6
	BYTE  m_bFireDamage;		// �� �Ӽ�
	BYTE  m_bIceDamage;			// �ñ� �Ӽ�
	BYTE  m_bLightningDamage;	// ��� �Ӽ�
	BYTE  m_bPoisonDamage;		// �� �Ӽ�
	BYTE  m_bHPDrain;			// HP ���
	BYTE  m_bMPDamage;			// MP Ÿ��
	BYTE  m_bMPDrain;			// MP ���
	BYTE  m_bMirrorDamage;		// �ݻ� Ÿ��
	BYTE  m_bDroprate;			// ��� ���
	BYTE  m_bStrB;				// �� ���ʽ�
	BYTE  m_bStaB;				// ü�� ���ʽ�
	BYTE  m_bDexB;				// ��ø�� ���ʽ�
	BYTE  m_bIntelB;			// ��� ���ʽ�
	BYTE  m_bChaB;				// �ŷ� ���ʽ�
	short m_MaxHpB;				// MaxHP add
	short m_MaxMpB;				// MaxMP add
	BYTE  m_bFireR;				// �� ���� ���׷�
	BYTE  m_bColdR;				// ��� ���� ���׷�
	BYTE  m_bLightningR;		// ��� ���� ���׷�
	BYTE  m_bMagicR;			// ��Ÿ ���� ���׷�
	BYTE  m_bPoisonR;			// �� ���� ���׷�
	BYTE  m_bCurseR;			// ���� ���� ���׷�
};

struct	_PARTY_GROUP
{
	WORD wIndex;
	short uid[8];		// �ϳ��� ��Ƽ�� 8����� ���԰���
	short sMaxHp[8];
	short sHp[8];
	BYTE bLevel[8];
	short sClass[8];
	BYTE bItemRouting;
	_PARTY_GROUP() {
		for(int i=0;i<8;i++) {
			uid[i] = -1; sMaxHp[i] = 0; sHp[i] = 0; bLevel[i] = 0; sClass[i] = 0;
		}
		bItemRouting = 0;
	};
};

struct _OBJECT_EVENT
{
	BYTE byLife;			// 1:����ִ�, 0:��,, ���
	int sBelong;			// �Ҽ�
	short sIndex;			// 100 ��� - ī�罺 ���ε� ����Ʈ | 200 ��� ������ ���ε� ����Ʈ | 1100 ��� - ī�罺 ������ 1200 - ������ ������
	short sType;			// 0 - ���ε� ����Ʈ, 1 - �¿�� ������ ����, 2 - ���Ϸ� ������ ����, 3 - ����, 4 - ��߷���, 6:öâ, 7-����� ��Ȱ��
	short sControlNpcID;	// ���� NPC ID (���� Object Index), Type-> 5 : Warp Group ID
	short sStatus;			// status
	float fPosX;			// �ġ��
	float fPosY;
	float fPosZ;
};

struct _REGENE_EVENT
{
	int	  sRegenePoint;		// ĳ���� ��Ÿ���� �� ��ȣ
	float fRegenePosX;		// ĳ���� ��Ÿ���� ���� �޾Ʒ��� ���� ��ǥ X
	float fRegenePosY;		// ĳ���� ��Ÿ���� ���� �޾Ʒ��� ���� ��ǥ Y
	float fRegenePosZ;		// ĳ���� ��Ÿ���� ���� �޾Ʒ��� ���� ��ǥ Z
	float fRegeneAreaZ;		// ĳ���� ��Ÿ���� ���� Z �� ���� 
	float fRegeneAreaX;		// ĳ���� ��Ÿ���� ���� X �� ����
};

struct _KNIGHTS_USER
{
	BYTE    byUsed;								// ����� : 1, ������ : 0
	char	strUserName[MAX_ID_SIZE+1];			// ĳ������ �̸�
};

struct _MAGIC_TABLE
{
	int		iNum;
	short	sFlyingEffect;
	BYTE	bMoral;
	BYTE	bSkillLevel;	
	short	sSkill;
	short	sMsp;
	short   sHP;
	BYTE	bItemGroup;
	int		iUseItem;
	BYTE	bCastTime;
	BYTE	bReCastTime;
	BYTE	bSuccessRate;
	BYTE	bType1;
	BYTE	bType2;
	short   sRange;
	BYTE	bEtc;
};

struct _MAGIC_TYPE8
{
	int     iNum;
	BYTE    bTarget;
	short   sRadius;
	BYTE    bWarpType;
	short   sExpRecover;
};

struct _MAGIC_TYPE5
{
	int		iNum;
	BYTE	bType;
	BYTE	bExpRecover;
	short	sNeedStone;
};

struct _MAGIC_TYPE4
{
	int     iNum;
	short   sMSP;
	BYTE    bBuffType;
	BYTE    bRadius;
	short   sDuration;
	BYTE    bAttackSpeed;
	BYTE    bSpeed;
	short   sAC;
	BYTE    bAttack;
	short   sMaxHP;
	BYTE    bHitRate;
	short   sAvoidRate;
	BYTE    bStr;
	BYTE    bSta;
	BYTE    bDex;
	BYTE    bIntel;
	BYTE    bCha;
	BYTE    bFireR;
	BYTE    bColdR;
	BYTE    bLightningR;
	BYTE    bMagicR;
	BYTE    bDiseaseR;
	BYTE    bPoisonR;
};

struct _MAGIC_TYPE3
{
	int		iNum;
	BYTE	bRadius;
	short	sAngle;
	short	sFirstDamage;
	short	sEndDamage;
	short	sTimeDamage;
	BYTE	bDirectType;
	short	sDuration;
	BYTE	bAttribute;
};

struct _MAGIC_TYPE2
{
	int     iNum;
	BYTE    bHitType;
	short   sHitRate;
	short	sAddDamage;
	short   sAddRange;
	BYTE    bNeedArrow;
};

struct _MAGIC_TYPE1
{
	int		iNum;
	BYTE	bHitType;
	short	sHitRate;
	short	sHit;
	BYTE	bDelay;
	BYTE	bComboType;
	BYTE	bComboCount;
	short	sComboDamage;
	short	sRange;
};

struct _ZONE_SERVERINFO
{
	short		sServerNo;
	short		sPort;
	char		strServerIP[20];
	_ZONE_SERVERINFO() {memset(strServerIP, 0x00, 20);};
};

struct _WARP_INFO
{
	short	sWarpID;
	char	strWarpName[32];
	char	strAnnounce[256];
	DWORD	dwPay;
	short	sZone;
	float	fX;
	float	fY;
	float	fZ;
	float	fR;

	_WARP_INFO() {
		sWarpID = 0; sZone = 0;
		fX = fZ = fY = fR = 0.0f;
		memset( strWarpName, 0x00, 32 ); memset( strAnnounce, 0x00, 256 );
	};
};

struct _HOME_INFO
{
	BYTE	bNation;
	long	ElmoZoneX;
	long	ElmoZoneZ;
	BYTE	ElmoZoneLX;
	BYTE	ElmoZoneLZ;
	long	KarusZoneX;
	long	KarusZoneZ;
	BYTE	KarusZoneLX;
	BYTE	KarusZoneLZ;
	long	FreeZoneX;
	long	FreeZoneZ;
	BYTE	FreeZoneLX;
	BYTE	FreeZoneLZ;
//
	long	BattleZoneX;
	long	BattleZoneZ;
	BYTE	BattleZoneLX;
	BYTE	BattleZoneLZ;
//
};

struct _START_POSITION
{
	int   ZoneID;
	short sKarusX;
	short sKarusZ;
	short sElmoradX;
	short sElmoradZ;
	short sKarusGateX;
	short sKarusGateZ;
	short sElmoradGateX;
	short sElmoradGateZ;
	BYTE bRangeX;
	BYTE bRangeZ;
};

struct _SERVER_RESOURCE
{
	int nResourceID;
	char strResource[255];
	_SERVER_RESOURCE()
	{
		memset(strResource, NULL, 255);
	};
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

#endif