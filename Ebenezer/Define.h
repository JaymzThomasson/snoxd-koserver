#ifndef _DEFINE_H
#define _DEFINE_H

#include <mmsystem.h>

#define _LISTEN_PORT		15000
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

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//
//	Global Function Define
//

inline float TimeGet()
{
	static bool bInit = false;
	static bool bUseHWTimer = FALSE;
	static LARGE_INTEGER nTime, nFrequency;
	
	if(bInit == false)
	{
		if(TRUE == ::QueryPerformanceCounter(&nTime))
		{
			::QueryPerformanceFrequency(&nFrequency);
			bUseHWTimer = TRUE;
		}
		else 
		{
			bUseHWTimer = FALSE;
		}

		bInit = true;
	}

	if(bUseHWTimer)
	{
		::QueryPerformanceCounter(&nTime);
		return (float)((double)(nTime.QuadPart)/(double)nFrequency.QuadPart);
	}

	return (float)timeGetTime();
};

inline void DisplayErrorMsg(SQLHANDLE hstmt)
{
	SQLCHAR       SqlState[6], Msg[1024];
	SQLINTEGER    NativeError;
	SQLSMALLINT   i, MsgLen;
	SQLRETURN     rc2;

	i = 1;
	while ((rc2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, i, SqlState, &NativeError, Msg, sizeof(Msg), &MsgLen)) != SQL_NO_DATA)
	{
		TRACE("*** %s, %d, %s, %d ***\n", SqlState,NativeError,Msg,MsgLen);

		i++;
	}
};

#include "../shared/globals.h"


struct _REGION_BUFFER {
	int		iLength;
	BYTE	bFlag;
	DWORD	dwThreadID;

	char	pDataBuff[REGION_BUFF_SIZE];
	_REGION_BUFFER() {
		iLength = 0;
		bFlag = E;
		dwThreadID = 0;
		memset(pDataBuff, 0x00, REGION_BUFF_SIZE);
	};
};

#endif