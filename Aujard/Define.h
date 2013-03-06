#pragma once

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

#define CString std::string
#include "../shared/globals.h"