#pragma once

#define _LISTEN_PORT		15100

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

struct _VERSION_INFO
{
	uint16 sVersion;
	uint16 sHistoryVersion;
	std::string strFileName;
};