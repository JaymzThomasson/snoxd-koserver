#ifndef _DEFINE_H
#define _DEFINE_H

#pragma warning(disable : 4786)
#include <string>
using namespace std;

#define _LISTEN_PORT		15100

#define MAX_USER			3000
#define CLIENT_SOCKSIZE		10

#define MAX_ID_SIZE			20
////////////////////////////////////////////////////////////
// Socket Define
////////////////////////////////////////////////////////////
#define SOCKET_BUFF_SIZE	(1024*16)
#define MAX_PACKET_SIZE		(1024*8)

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

typedef union{
	short int	i;
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

struct _VERSION_INFO
{
	int sVersion;
	int sHistoryVersion;
	char strFileName[_MAX_PATH];
	_VERSION_INFO()
	{
		memset(strFileName, 0x00, sizeof(strFileName));
	};
};

struct _SERVER_INFO
{
	char strServerIP[32];
	char strLanIP[32];
	char strServerName[32];
	short sUserCount;
	short sServerID;
	short sGroupID;
	short sPlayerCap;
	short sFreePlayerCap;
	char strKarusKingName[MAX_ID_SIZE+1];
	char strKarusNotice[256]; // not sure how big they should be
	char strElMoradKingName[MAX_ID_SIZE+1];
	char strElMoradNotice[256];

	_SERVER_INFO() {
		memset(strServerIP, 0x00, sizeof(strServerIP));
		memset(strServerName, 0x00, sizeof(strServerName));
		memset(strKarusKingName, 0x00, sizeof(strKarusKingName));
		memset(strKarusNotice, 0x00, sizeof(strKarusNotice));
		memset(strElMoradKingName, 0x00, sizeof(strElMoradKingName));
		memset(strElMoradNotice, 0x00, sizeof(strElMoradNotice));

		sUserCount = sServerID = sGroupID = sPlayerCap = sFreePlayerCap = 0;
	};
};

struct News
{
	BYTE Content[4096];
	size_t Size;
};

// Packet Define...

enum LogonOpcodes
{
	LS_VERSION_REQ				= 0x01,
	LS_DOWNLOADINFO_REQ			= 0x02,
	LS_CRYPTION					= 0xF2,
	LS_LOGIN_REQ				= 0xF3,
	LS_MGAME_LOGIN				= 0xF4,
	LS_SERVERLIST				= 0xF5,
	LS_NEWS						= 0xF6,
	LS_UNKF7					= 0xF7,

	NUM_LS_OPCODES
};

#include "../shared/globals.h"
#endif