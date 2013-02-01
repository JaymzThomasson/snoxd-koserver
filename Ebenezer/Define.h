#ifndef _DEFINE_H
#define _DEFINE_H

#define _LISTEN_PORT		15001
#define _UDP_PORT			8888
#define AI_KARUS_SOCKET_PORT		10020
#define AI_ELMO_SOCKET_PORT			10030
#define AI_BATTLE_SOCKET_PORT		10040
#define CLIENT_SOCKSIZE		100
#define MAX_AI_SOCKET		10			// sungyong~ 2002.05.22

#define MAX_NPC_SIZE		30

#define MAX_TYPE3_REPEAT    20
#define MAX_TYPE4_BUFF		9

#define MAX_ITEM			28
#define VIEW_DISTANCE		48			// ���ðŸ�

#define NPC_HAVE_ITEM_LIST	6
#define ZONEITEM_MAX		2100000000	// ��� ������� �ִ� �����ۼ�...

#define MAX_CLASS			26			// ��� MAX
#define MAX_LEVEL			83			// �ְ�...

#define MAX_PARTY_USERS		8
#define MAX_CLAN_USERS		36

#define SERVER_INFO_START			0X01
#define SERVER_INFO_END				0X02

//////////////  Quest ��� Define ////////////////////////////
#define MAX_EVENT					2000
#define MAX_EVENT_SIZE				400
#define MAX_EVENT_NUM				2000
#define MAX_EXEC_INT				30
#define MAX_LOGIC_ELSE_INT			10
#define MAX_MESSAGE_EVENT			10
#define MAX_COUPON_ID_LENGTH		20
#define MAX_CURRENT_EVENT			20

#define LOGIC_CHECK_UNDER_WEIGHT	0X01
#define LOGIC_CHECK_OVER_WEIGHT		0X02
#define LOGIC_CHECK_SKILL_POINT		0X03
#define LOGIC_EXIST_ITEM			0X04
#define LOGIC_CHECK_CLASS			0x05
#define LOGIC_CHECK_WEIGHT			0x06
#define LOGIC_RAND					0x08
#define LOGIC_HOWMUCH_ITEM			0x09 
#define	LOGIC_CHECK_LEVEL			0x0A
#define LOGIC_NOEXIST_COM_EVENT		0x0B
#define LOGIC_EXIST_COM_EVENT		0x0C
#define LOGIC_CHECK_NOAH			0x0D

/*
#define	LOGIC_CHECK_NATION			0X01
#define	LOGIC_CHECK_LEVEL			0X02
#define	LOGIC_NOEXIST_ITEM			0X03
#define	LOGIC_QUEST_END				0X04
#define	LOGIC_QUEST_LOG				0X05
#define	LOGIC_EXIST_ITEM			0X06
#define	LOGIC_CHECK_NOAH			0X07
#define LOGIC_CHECK_CLASS			0x08
#define LOGIC_CHECK_WEIGHT			0x09
#define LOGIC_CHECK_RACE			0x0A
#define LOGIC_CHECK_LOYALTY			0x0B
#define LOGIC_CHECK_AUTHORITY		0x0C
#define LOGIC_CHECK_STR				0X0D
#define LOGIC_CHECK_STA				0x0D
#define LOGIC_CHECK_DEX				0x0E
#define LOGIC_CHECK_INT				0x0F
#define LOGIC_CHECK_CHA				0x10
#define LOGIC_CHECK_SKILLPOINT		0x11
#define LOGIC_CHECK_DAY				0x12
*/

// ������ define
#define EXEC_SAY					0X01
#define EXEC_SELECT_MSG				0X02
#define EXEC_RUN_EVENT				0X03
#define EXEC_GIVE_ITEM				0X04
#define EXEC_ROB_ITEM				0X05
#define EXEC_RETURN					0X06
#define EXEC_GIVE_NOAH				0x08
#define EXEC_SAVE_COM_EVENT			0x0A
#define EXEC_ROB_NOAH				0x0B

/*
#define EXEC_SAY					0X01
#define EXEC_SELECT_MSG				0X02
#define EXEC_RUN_EVENT				0X03
#define EXEC_RETURN					0X04
#define EXEC_ROB_ITEM				0X05
#define EXEC_ROB_NOAH				0X06
#define EXEC_GIVE_ITEM				0X07
#define EXEC_GIVE_QUEST				0X08
#define EXEC_QUEST_END				0X09
#define EXEC_QUEST_SAVE				0X0A
#define EXEC_GIVE_NOAH				0x0B
*/

// EVENT ���� ��ȣ�� :)
#define EVENT_POTION				1
#define EVENT_LOGOS_ELMORAD			1001
#define EVENT_LOGOS_KARUS			2001
#define EVENT_COUPON				3001


////////////////////////////////////////////////////////////

///////////////// BBS RELATED //////////////////////////////
#define MAX_BBS_PAGE			23
#define MAX_BBS_MESSAGE			40
#define MAX_BBS_TITLE			20
#define MAX_BBS_POST			500

#define BUY_POST_PRICE			500
#define SELL_POST_PRICE			1000

#define REMOTE_PURCHASE_PRICE	5000
#define BBS_CHECK_TIME			36000

///////////////// NPC  STATUS //////////////////////////////
#define NPC_DEAD				0X00
#define NPC_LIVE				0X01

///////////////// NPC TYPE /////////////////////////////////
#define NPC_MONSTER				00
#define NPC_GENERAL				01		//
#define NPC_BOSS				03		// Unique Mop
#define NPC_PATROL_GUARD		11		// ���
#define NPC_MERCHANT			21		// ����
#define NPC_TINKER				22		// ��������
#define NPC_WAREHOUSE			31		// â�����
#define NPC_CAPTAIN				35		// ����
#define NPC_OFFICER				36		// ���� �� NPC
#define NPC_CLERIC				37		// ���� NPC
#define NPC_HEALER				40		// Healer
#define NPC_WARP				41		// Warp Npc
#define NPC_GATE				50		// �������� NPC 
#define NPC_PHOENIX_GATE		51		// ������ �ʴ� �� (8->51)
#define NPC_SPECIAL_GATE		52		// ������ �ʴ� ���̸鼭 2�и��� ���ȴ� ����� �ϴ� ��
#define NPC_GATE_LEVER			55		// ���� ����...	(9->55)	
#define NPC_ARTIFACT			60		// ��輮 (7->60)
#define NPC_DESTORY_ARTIFACT	61		// �ı��Ǵ� ��輮
#define NPC_MONK_ELMORAD		71		// �񷯸ӱ� ��� >.<
#define NPC_MONK_KARUS			72		// �񷯸ӱ� ��� >.<	
#define NPC_DOMESTIC_ANIMAL		99		// ���� NPC
#define NPC_COUPON				100		// �񷯸ӱ� ���� >.<

// SPECIAL TYPE
#define NPC_UPGRADE				24 // Magic anvil
#define NPC_KAISHAN				34 // [Grand merchant] kaishan
#define NPC_SIEGE				44 // Arendil [Castle Siege Warfare]
#define NPC_CHAOTIC				137 // Chaotic generator (137)(162)
#define NPC_BOARD				64
#define NPC_TOWER				63 // Guard tower
#define NPC_RENTAL				78 // Rental NPC
#define NPC_ELECTION			79 // Election NPC
#define NPC_LORD				80 // Lord
#define NPC_KISS				32 // KissMe
#define NPC_ADVISOR				26 // Clan match advisor
#define NPC_KJWAR				133 // duel kjwar
#define NPC_SIEGE2				134 // Joyce[siege war event]
//
#define NPC_ARTIFACT1			65 // Protective Artifact 
#define NPC_ARTIFACT2			66 // Guard Tower Artifact
#define NPC_ARTIFACT3			67 // Guard Artifact
#define NPC_ARTIFACT4			68 // Defense Artifact
#define NPC_MONUMENT			122 // El Morad Monument & Asga Village Monument & Raiba Village Monument & Doda Camp Monument & Elmorad Snowman
#define NPC_GATE2				150 // Doda Camp Gate 
#define NPC_VICTORY_GATE		53 // Victory Gate
#define NPC_RECON				43 // Recon Guard NPC
#define NPC_ROYAL_GUARD			142 // Royal Guard NPC
#define NPC_ADVISOR2			149 // Advisor NPC
#define NPC_SPY					141 // Spy NPC
#define NPC_ROYAL_CHEF			143 // Royal Chef NPC
#define NPC_GATE_GUARD			148 // Gate Guard NPC
#define NPC_ESLANT_WOMAN		144 // Eslant Woman (near gate)
#define NPC_FARMER				145 // Farm NPC
#define NPC_UNKNOWN1			155
#define NPC_UNKNOWN2			156
#define NPC_UNKNOWN3			157
#define NPC_UNKNOWN4			158
#define NPC_UNKNOWN5			159
#define NPC_UNKNOWN6			160
#define NPC_UNKNOWN7			161
#define NPC_CRAFTSMAN			135 // Craftsman boy

// QUEST TYPE
#define NPC_MOIRA				29 // Operator moira
#define NPC_MENU				33 // Isaac event
#define NPC_LADY				38 // Calamus lady event
#define NPC_QUEST				47 // Sentinel patrick
#define NPC_ENCAMPMENT			64 // Encampment
#define NPC_BLACKMITH			77 // Blacksmith heppa
#define NPC_HERO_STATUS1		106 // Elmorad & Karus Hero statue & 1st place clan
#define NPC_HERO_STATUS2		107 // Elmorad & Karus Hero statue & 2st place clan
#define NPC_HERO_STATUS3		108 // Elmorad & Karus Hero statue & 3st place clan
#define NPC_MINERVA				131 // Learth [Forgotten Temple 3] & priest iris
#define NPC_HUGOR				27 // Captain Hugor[maintain]
#define NPC_ARENA				43 // Arena
#define NPC_SABICE				25 // Sabice mantle
#define NPC_NPC1				101
#define NPC_NPC2				102
#define NPC_NPC3				103
#define NPC_NPC4				104
#define NPC_NPC5				105
#define NPC_KEY1				111 // Sentinel of the Key
#define NPC_KEY2				112 // Watcher of the Key
#define NPC_KEY3				113 // Protector of the Key
#define NPC_KEY4				114 // Ranger of the Key
#define NPC_KEY5				115 // Patroller of the Key
#define NPC_KEY6				116 // Recon of the Key
#define NPC_KEY7				117 // Keeper of the Key
#define NPC_ADELIA				153 // Goddess Adelia[event]
#define NPC_LAEMITH1			129 // Laemith[Forgotten Temple 1]
#define NPC_LAEMITH2			130 // Laemith[Forgotten Temple 2]
#define NPC_LAEMITH3			131 // Learth[Forgotten Temple 3]
#define	NPC_ATHIAN				39 // Priest athian
//
#define NPC_ROBOS				118 // Robos
#define NPC_TRANSFER			123 // lillia server transfer
#define NPC_RANKING				124 // hardis ranking
#define NPC_LYONI				125 // lyoni
#define NPC_HELPER				127 // adine beginner helper


///////////////// NATION ///////////////////////////////////
#define UNIFY_NATION		0
#define KARUS               1
#define ELMORAD             2
#define BATTLE				3

#define BATTLE_ZONE			101

////////////////////////////////////////////////////////////

// Attack Type
#define DIRECT_ATTACK		0
#define LONG_ATTACK			1
#define MAGIC_ATTACK		2
#define DURATION_ATTACK		3

////////////////// ETC Define //////////////////////////////
// UserInOut //
#define USER_IN					0X01
#define USER_OUT				0X02
#define USER_REGENE				0X03	// Userin�ϰ� ������ε� ȿ�� �ֱ���ؼ�.. �и�(���ӽ���, ���. ��ȯ��)
#define USER_WARP				0X04
#define USER_SUMMON				0X05
#define NPC_IN					0X01
#define NPC_OUT					0X02

////////////////// Resurrection related ////////////////////
#define BLINK_TIME				10
#define CLAN_SUMMON_TIME		180
////////////////////////////////////////////////////////////

// Socket Define
////////////////////////////////////////////////////////////
#define SOCKET_BUFF_SIZE	(1024*16)
#define MAX_PACKET_SIZE		(1024*8)
#define MAX_SEND_SIZE		(MAX_PACKET_SIZE * 2)
#define REGION_BUFF_SIZE	(1024*16)

#define PACKET_START1				0XAA
#define PACKET_START2				0X55
#define PACKET_END1					0X55
#define PACKET_END2					0XAA

// status
#define STATE_CONNECTED			0X01
#define STATE_DISCONNECTED		0X02
#define STATE_GAMESTART			0x03

// Socket type
#define TYPE_ACCEPT				0x01
#define TYPE_CONNECT			0x02

// Overlapped flag
#define OVL_RECEIVE				0X01
#define OVL_SEND				0X02
#define OVL_CLOSE				0X03
////////////////////////////////////////////////////////////

// ==================================================================
//	About Map Object
// ==================================================================
#define USER_BAND				0			// Map ��� ��� �ִ�.
#define NPC_BAND				10000		// Map ��� NPC(������)�� �ִ�.
#define INVALID_BAND			30000		// �߸�� ID BAND

#define EVENT_MONSTER			20			// Event monster �� ��

///////////////// snow event define //////////////////////////////
#define SNOW_EVENT_MONEY		2000
#define SNOW_EVENT_SKILL		490043


// Reply packet define...

#define SEND_ME					0x01
#define SEND_REGION				0x02
#define SEND_ALL				0x03
#define SEND_ZONE				0x04

// Battlezone Announcement
#define BATTLEZONE_OPEN					0x00
#define BATTLEZONE_CLOSE				0x01           
#define DECLARE_WINNER					0x02
#define DECLARE_LOSER					0x03
#define DECLARE_BAN						0x04
#define KARUS_CAPTAIN_NOTIFY			0x05
#define ELMORAD_CAPTAIN_NOTIFY			0x06
#define KARUS_CAPTAIN_DEPRIVE_NOTIFY	0x07
#define ELMORAD_CAPTAIN_DEPRIVE_NOTIFY	0x08
#define SNOW_BATTLEZONE_OPEN			0x09

// Battle define
#define NO_BATTLE				0
#define NATION_BATTLE			1
#define SNOW_BATTLE				2

// Zone IDs
#define ZONE_KARUS				1
#define ZONE_ELMORAD			2
#define ZONE_BATTLE				101
#define ZONE_SNOW_BATTLE		102
#define	ZONE_FRONTIER			201

#define MAX_BATTLE_ZONE_USERS	150

//////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
typedef union{
	WORD		w;
	BYTE		b[2];
} MYSHORT;

typedef union{
	int			i;
	BYTE		b[4];
} MYINT;

typedef union{
	DWORD		w;
	BYTE		b[4];
} MYDWORD;

typedef union{
	__int64		i;
	BYTE		b[8];
} MYINT64;

#include "../shared/globals.h"

struct _REGION_BUFFER 
{
	ByteBuffer Buffer;
	RWLock Lock;
};

#endif