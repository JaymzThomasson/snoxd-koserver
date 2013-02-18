#ifndef _DEFINE_H
#define _DEFINE_H

////////////////////////////////////////////////////////////
// Socket Define
////////////////////////////////////////////////////////////
#define SOCKET_BUFF_SIZE	(1024*8)
#define MAX_PACKET_SIZE		(1024*2)

// status
#define STATE_CONNECTED			0X01
#define STATE_DISCONNECTED		0X02
#define GAME_STATE_INGAME			0x03

// DEFINE MACRO PART...
#define BufInc(x) (x)++;(x) %= SOCKET_BUF_SIZE;

enum UserUpdateType
{
	UPDATE_LOGOUT,
	UPDATE_ALL_SAVE,
	UPDATE_PACKET_SAVE,
};

struct _ITEM_TABLE
{
	long  m_iNum;				// item num
	BYTE  m_bCountable;			// 개수 개념 아이템
};

#include "../shared/globals.h"
#endif