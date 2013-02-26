#ifndef _DEFINE_H
#define _DEFINE_H

#define _LISTEN_PORT		15001
#define _UDP_PORT			8888
#define AI_KARUS_SOCKET_PORT		10020
#define AI_ELMO_SOCKET_PORT			10030
#define AI_BATTLE_SOCKET_PORT		10040
#define CLIENT_SOCKSIZE		100

#define MAX_NPC_SIZE		30

#define MAX_TYPE3_REPEAT    20
#define MAX_TYPE4_BUFF		9

#define MAX_ITEM			28
#define VIEW_DISTANCE		48			// ���ðŸ�

#define NPC_HAVE_ITEM_LIST	6
#define ZONEITEM_MAX		2100000000	// ��� ������� �ִ� �����ۼ�...
#define COIN_MAX			2100000000

#define MAX_CLASS			26			// ��� MAX
#define MAX_LEVEL			83			// �ְ�...

#define MAX_PARTY_USERS		8
#define MAX_CLAN_USERS		36

#define MAX_EVENT					2000
#define MAX_EVENT_SIZE				400
#define MAX_EVENT_NUM				2000
#define MAX_EXEC_INT				30
#define MAX_LOGIC_ELSE_INT			10
#define MAX_MESSAGE_EVENT			10
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

enum NpcType
{
	NPC_MONSTER				= 0,
	NPC_GENERAL				= 1,
	NPC_BOSS				= 3,
	NPC_DUNGEON_MONSTER		= 4,
	NPC_TRAP_MONSTER		= 5,
	NPC_GUARD				= 11,
	NPC_PATROL_GUARD		= 12,
	NPC_STORE_GUARD			= 13,
	NPC_WAR_GUARD			= 14,
	NPC_MERCHANT			= 21,
	NPC_TINKER				= 22,
	NPC_SELITH				= 23, // Selith[special store]
	NPC_ANVIL				= 24,

	NPC_MARK				= 25,
	NPC_CLAN_MATCH_ADVISOR	= 26,
	NPC_SIEGE_1				= 27,
	NPC_OPERATOR			= 29, // not sure what Operator Moira was ever supposed to do...
	NPC_WAREHOUSE			= 31,
	NPC_KISS				= 32, // pretty useless.
	NPC_ISAAC				= 33, // need to check which quests he handles
	NPC_KAISHAN				= 34, // need to see what he actually does to name this properly
	NPC_CAPTAIN				= 35,
	NPC_CLAN				= 36,
	NPC_CLERIC				= 37,
	NPC_LADY				= 38, // Calamus lady event -- need to see what they're used for
	NPC_ATHIAN				= 39, // Priest athian -- need to see what they're used for
	NPC_HEALER				= 40,
	NPC_ROOM				= 42,
	NPC_ARENA				= 43, // also recon guards
	NPC_SIEGE				= 44,
	NPC_SENTINEL_PATRICK	= 47, // need to check which quests he handles (was it the beginner quests, or was that isaac?)

	NPC_GATE				= 50,
	NPC_PHOENIX_GATE		= 51,
	NPC_SPECIAL_GATE		= 52,
	NPC_VICTORY_GATE		= 53,
	NPC_GATE_LEVER			= 55,
	NPC_ARTIFACT			= 60,
	NPC_DESTROYED_ARTIFACT	= 61,
	NPC_GUARD_TOWER			= 63,
	NPC_BOARD				= 64, // also encampment
	NPC_ARTIFACT1			= 65, // Protective artifact
	NPC_ARTIFACT2			= 66, // Guard Tower artifact
	NPC_ARTIFACT3			= 67, // Guard artifact
	NPC_ARTIFACT4			= 68,
	NPC_MONK_ELMORAD		= 71,
	NPC_MONK_KARUS			= 72,
	NPC_BLACKSMITH			= 77,
	NPC_RENTAL				= 78,
	NPC_ELECTION			= 79, // king elections
	NPC_TREASURY			= 80,
	NPC_DOMESTIC_ANIMAL		= 99,
	NPC_COUPON				= 100,
	NPC_HERO_STATUE_1		= 106, // 1st place
	NPC_HERO_STATUE_2		= 107, // 2nd place
	NPC_HERO_STATUE_3		= 108, // 3rd place
	NPC_KEY_QUEST_1			= 111, // Sentinel of the Key
	NPC_KEY_QUEST_2			= 112, // Watcher of the Key
	NPC_KEY_QUEST_3			= 113, // Protector of the Key
	NPC_KEY_QUEST_4			= 114, // Ranger of the Key
	NPC_KEY_QUEST_5			= 115, // Patroller of the Key
	NPC_KEY_QUEST_6			= 116, // Recon of the Key
	NPC_KEY_QUEST_7			= 117, // Keeper of the Key
	NPC_ROBOS				= 118, // need to see what he actually does to name this properly
	NPC_MONUMENT			= 122, // El Morad/Asga village/Raiba village/Doda camp monuments 
	NPC_SERVER_TRANSFER		= 123,
	NPC_RANKING				= 124,
	NPC_LYONI				= 125, // need to see what this NPC actually does to name this properly
	NPC_BEGINNER_HELPER		= 127,
	NPC_FT_1				= 129,
	NPC_FT_2				= 130,
	NPC_FT_3				= 131, // also Priest Minerva
	NPC_KJWAR				= 133,
	NPC_SIEGE_2				= 134,
	NPC_CRAFTSMAN			= 135, // Craftsman boy, not sure what he's actually used for
	NPC_CHAOTIC_GENERATOR	= 137, // possibly 162 as well?
	NPC_SPY					= 141,
	NPC_ROYAL_GUARD			= 142,
	NPC_ROYAL_CHEF			= 143,
	NPC_ESLANT_WOMAN		= 144,
	NPC_FARMER				= 145,
	NPC_GATE_GUARD			= 148,
	NPC_ROYAL_ADVISOR		= 149,
	NPC_GATE2				= 150, // Doda camp gate
	NPC_ADELIA				= 153, // Goddess Adelia[event]
	NPC_KARUS_WARDER1		= 190,
	NPC_KARUS_WARDER2		= 191,
	NPC_ELMORAD_WARDER1		= 192,
	NPC_ELMORAD_WARDER2		= 193,
	NPC_KARUS_GATEKEEPER	= 198,
	NPC_ELMORAD_GATEKEEPER	= 199
};

///////////////// NATION ///////////////////////////////////
#define UNIFY_NATION		0
#define NO_NATION			0
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

enum InOutType
{
	INOUT_IN		= 1,
	INOUT_OUT		= 2,
	INOUT_RESPAWN	= 3,
	INOUT_WARP		= 4,
	INOUT_SUMMON	= 5
};

////////////////// Resurrection related ////////////////////
#define BLINK_TIME				15
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

// Users under level 35 require 3,000 coins to shout.
#define SHOUT_COIN_REQUIREMENT	3000

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

#endif