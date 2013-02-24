#pragma once

#include "define.h"
#include <vector>

#include "../shared/database/OdbcConnection.h"
#include "../shared/STLMap.h"

typedef std::vector <_USER_DATA*>			UserDataArray;
typedef CSTLMap <_ITEM_TABLE>		ItemtableArray;

class CAujardDlg;
class CDBAgent  
{
public:
	CDBAgent();

	bool Connect();
	bool LoadItemTable();

	void MUserInit(uint16 uid);
	_USER_DATA *GetUser(uint16 uid);

	int8 AccountLogin(std::string & strAccountID, std::string & strPasswd);
	uint8 NationSelect(std::string & strAccountID, uint8 bNation);
	bool GetAllCharID(std::string & strAccountID, std::string & strCharID1, std::string & strCharID2, std::string & strCharID3);
	void LoadCharInfo(std::string & strCharID, ByteBuffer & result);

	int8 ChangeHair(std::string & strAccountID, std::string & strCharID, uint8 bOpcode, uint8 bFace, uint32 nHair);

	int8 CreateNewChar(std::string & strAccountID, int index, std::string & strCharID, uint8 bRace, uint16 sClass, uint32 nHair, uint8 bFace, uint8 bStr, uint8 bSta, uint8 bDex, uint8 bInt, uint8 bCha);
	int8 DeleteChar(std::string & strAccountID, int index, std::string & strCharID, std::string & strSocNo);

	bool LoadUserData(std::string & strAccountID, std::string & strCharID, short uid);
	bool LoadWarehouseData(std::string & strAccountID, short uid);
	bool SetLogInInfo(std::string & strAccountID, std::string & strCharID, std::string & strServerIP, short sServerNo, std::string & strClientIP, uint8 bInit);

	bool LoadWebItemMall(short uid, Packet & result);

	bool LoadSkillShortcut(short uid, Packet & result);
	void SaveSkillShortcut(short uid, short sCount, char *buff);

	void RequestFriendList(short uid, std::vector<std::string> & friendList);
	FriendAddResult AddFriend(short sid, short tid);
	FriendRemoveResult RemoveFriend(short sid, std::string & strCharID);

	bool UpdateUser(std::string & strCharID, short uid, UserUpdateType type);
	bool UpdateWarehouseData(std::string & strAccountID, short uid, UserUpdateType type);

	int8 CreateKnights(uint16 sClanID, uint8 bNation, std::string & strKnightsName, std::string & strChief, uint8 bFlag = 1);
	int UpdateKnights(uint8 bType, std::string & strCharID, uint16 sClanID, uint8 bDomination);
	int DeleteKnights(uint16 sClanID);
	uint16 LoadKnightsAllMembers(uint16 sClanID, Packet & result);
	void LoadKnightsInfo(uint16 sClanID, Packet & result);
	void LoadKnightsAllList(uint8 bNation);
	bool UpdateClanSymbol(uint16 sClanID, uint16 sSymbolSize, char *clanSymbol);

	void UpdateBattleEvent(std::string & strCharID, uint8 bNation);
	void AccountLogout(std::string & strAccountID);

	void UpdateConCurrentUserCount(int nServerNo, int nZoneNo, int nCount);

	UserDataArray m_UserDataArray;

private:
	OdbcConnection m_GameDB, m_AccountDB;
	CAujardDlg* m_pMain;
	ItemtableArray m_itemTableArray;
};