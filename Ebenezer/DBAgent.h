#pragma once

#include "../shared/database/OdbcConnection.h"
#include "../shared/packets.h"

enum UserUpdateType
{
	UPDATE_LOGOUT,
	UPDATE_ALL_SAVE,
	UPDATE_PACKET_SAVE,
};

struct _RENTAL_ITEM
{
	uint32	nRentalIndex;
	uint32	nItemID;
	uint16	sDurability;
	uint64	nSerialNum;
	uint8	byRegType;
	uint8	byItemType;
	uint8	byClass;
	uint16	sRentalTime;
	uint32	nRentalMoney;
	std::string strLenderCharID;
	std::string strBorrowerCharID;
};

enum RentalType
{
	RENTAL_TYPE_IN_LIST		= 1,
	RENTAL_TYPE_LENDER		= 2,
	RENTAL_TYPE_BORROWER	= 3
};

struct _USER_RENTAL_ITEM
{
	std::string strUserID;
	uint64 nSerialNum;
	uint32 nRentalIndex, nItemID, nRentalMoney;
	uint16 sDurability, sRentalTime;
	int16 sMinutesRemaining;
	uint8 byRentalType, byRegType;
	char szTimeRental[30];

	_USER_RENTAL_ITEM()
	{
		memset(&szTimeRental, 0, sizeof(szTimeRental));
	}
};

typedef std::map<uint64, _USER_RENTAL_ITEM *> UserRentalMap;

class Packet;
class CUser;
struct _ITEM_DATA;
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

	void LoadRentalData(std::string & strAccountID, std::string & strCharID, UserRentalMap & rentalData);

	bool LoadUserData(std::string & strAccountID, std::string & strCharID, CUser *pUser);
	bool LoadWarehouseData(std::string & strAccountID, CUser *pUser);
	bool LoadPremiumServiceUser(std::string & strAccountID, CUser *pUser);
	bool LoadSavedMagic(CUser *pUser);
	bool SetLogInInfo(std::string & strAccountID, std::string & strCharID, std::string & strServerIP, short sServerNo, std::string & strClientIP, uint8 bInit);

	bool LoadWebItemMall(Packet & result, CUser *pUser);

	bool LoadSkillShortcut(Packet & result, CUser *pUser);
	void SaveSkillShortcut(short sCount, char *buff, CUser *pUser);

	void RequestFriendList(std::vector<std::string> & friendList, CUser *pUser);
	FriendAddResult AddFriend(short sid, short tid);
	FriendRemoveResult RemoveFriend(std::string & strCharID, CUser *pUser);

	bool UpdateUser(std::string & strCharID, UserUpdateType type, CUser *pUser);
	bool UpdateWarehouseData(std::string & strAccountID, UserUpdateType type, CUser *pUser);
	bool UpdateSavedMagic(CUser *pUser);

	int8 CreateKnights(uint16 sClanID, uint8 bNation, std::string & strKnightsName, std::string & strChief, uint8 bFlag = 1);
	int UpdateKnights(uint8 bType, std::string & strCharID, uint16 sClanID, uint8 bDomination);
	int DeleteKnights(uint16 sClanID);
	uint16 LoadKnightsAllMembers(uint16 sClanID, Packet & result);
	bool LoadKnightsInfo(uint16 sClanID, uint8 & bNation, std::string & strKnightsName, uint16 & sMembers, uint32 & nPoints, uint8 & bRank);
	void LoadKnightsAllList(uint8 bNation);
	bool UpdateClanSymbol(uint16 sClanID, uint16 sSymbolSize, char *clanSymbol);

	void UpdateCape(uint16 sClanID, uint16 sCapeID, uint8 r, uint8 g, uint8 b);

	void UpdateBattleEvent(std::string & strCharID, uint8 bNation);
	void AccountLogout(std::string & strAccountID);

	void UpdateConCurrentUserCount(int nServerNo, int nZoneNo, int nCount);

	uint8 GetUnreadLetterCount(std::string & strCharID);
	bool GetLetterList(std::string & strCharID, Packet & result, bool bNewLettersOnly = true);
	int8 SendLetter(std::string & strSenderID, std::string & strRecipientID, std::string & strSubject, std::string & strMessage, uint8 bType, _ITEM_DATA * pItem);
	bool ReadLetter(std::string & strCharID, uint32 nLetterID, std::string & strMessage);
	int8 GetItemFromLetter(std::string & strCharID, uint32 nLetterID, uint32 & nItemID, uint16 & sCount, uint16 & sDurability, uint32 & nCoins, uint64 & nSerialNum);
	void DeleteLetter(std::string & strCharID, uint32 nLetterID);

private:
	OdbcConnection m_GameDB, m_AccountDB;
	FastMutex m_lock;

	friend class CEbenezerDlg;
};