#pragma once

#include "../shared/database/OdbcConnection.h"
#include "../shared/packets.h"

enum UserUpdateType
{
	UPDATE_LOGOUT,
	UPDATE_ALL_SAVE,
	UPDATE_PACKET_SAVE,
};

class Packet;
struct _USER_DATA;
class CDBAgent  
{
public:
	CDBAgent() {}

	bool Startup();
	bool Connect();

	void ReportSQLError(OdbcError *pError);

	int8 AccountLogin(std::string & strAccountID, std::string & strPasswd);
	uint8 NationSelect(std::string & strAccountID, uint8 bNation);
	bool GetAllCharID(std::string & strAccountID, std::string & strCharID1, std::string & strCharID2, std::string & strCharID3);
	void LoadCharInfo(std::string & strCharID, ByteBuffer & result);

	int8 ChangeHair(std::string & strAccountID, std::string & strCharID, uint8 bOpcode, uint8 bFace, uint32 nHair);

	int8 CreateNewChar(std::string & strAccountID, int index, std::string & strCharID, uint8 bRace, uint16 sClass, uint32 nHair, uint8 bFace, uint8 bStr, uint8 bSta, uint8 bDex, uint8 bInt, uint8 bCha);
	int8 DeleteChar(std::string & strAccountID, int index, std::string & strCharID, std::string & strSocNo);

	bool LoadUserData(std::string & strAccountID, std::string & strCharID, _USER_DATA *pUser);
	bool LoadWarehouseData(std::string & strAccountID, _USER_DATA *pUser);
	bool LoadPremiumServiceUser(std::string & strAccountID, _USER_DATA *pUser);
	bool SetLogInInfo(std::string & strAccountID, std::string & strCharID, std::string & strServerIP, short sServerNo, std::string & strClientIP, uint8 bInit);

	bool LoadWebItemMall(Packet & result, _USER_DATA *pUser);

	bool LoadSkillShortcut(Packet & result, _USER_DATA *pUser);
	void SaveSkillShortcut(short sCount, char *buff, _USER_DATA *pUser);

	void RequestFriendList(std::vector<std::string> & friendList, _USER_DATA *pUser);
	FriendAddResult AddFriend(short sid, short tid);
	FriendRemoveResult RemoveFriend(std::string & strCharID, _USER_DATA *pUser);

	bool UpdateUser(std::string & strCharID, UserUpdateType type, _USER_DATA *pUser);
	bool UpdateWarehouseData(std::string & strAccountID, UserUpdateType type, _USER_DATA *pUser);

	int8 CreateKnights(uint16 sClanID, uint8 bNation, std::string & strKnightsName, std::string & strChief, uint8 bFlag = 1);
	int UpdateKnights(uint8 bType, std::string & strCharID, uint16 sClanID, uint8 bDomination);
	int DeleteKnights(uint16 sClanID);
	uint16 LoadKnightsAllMembers(uint16 sClanID, Packet & result);
	void LoadKnightsInfo(uint16 sClanID, Packet & result);
	void LoadKnightsAllList(uint8 bNation);
	bool UpdateClanSymbol(uint16 sClanID, uint16 sSymbolSize, char *clanSymbol);

	void UpdateCape(uint16 sClanID, uint16 sCapeID, uint8 r, uint8 g, uint8 b);

	void UpdateBattleEvent(std::string & strCharID, uint8 bNation);
	void AccountLogout(std::string & strAccountID);

	void UpdateConCurrentUserCount(int nServerNo, int nZoneNo, int nCount);

private:
	OdbcConnection m_GameDB, m_AccountDB;
	FastMutex m_lock;
};