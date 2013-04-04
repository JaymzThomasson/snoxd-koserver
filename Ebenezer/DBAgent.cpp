#include "stdafx.h"
#include "../shared/database/OdbcConnection.h"
#include "EbenezerDlg.h"
#include "KnightsManager.h"
#include "User.h"
#include "DBAgent.h"

CDBAgent g_DBAgent;

using std::string;
using std::auto_ptr;

CDBAgent::CDBAgent()
{
	m_GameDB = new OdbcConnection();
	m_AccountDB = new OdbcConnection();
}

CDBAgent::~CDBAgent()
{
	delete m_GameDB;
	delete m_AccountDB;
}

bool CDBAgent::Startup(bool bMarsEnabled, 
					   tstring & strAccountDSN, tstring & strAccountUID, tstring & strAccountPWD,
					   tstring & strGameDSN, tstring & strGameUID, tstring & strGamePWD)
{
	if (!Connect(bMarsEnabled,
			strAccountDSN, strAccountUID, strAccountPWD,
			strGameDSN, strGameUID, strGamePWD))
	{
		// we should probably be a little more specific (i.e. *which* database server)
		printf(_T("ERROR: Failed to connect to the database server."));
		return false;
	}

	// If MARS is enabled, we can use multiple database threads.
	DWORD dwThreads = 1;
	if (bMarsEnabled)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		dwThreads = si.dwNumberOfProcessors * 2;
	}
	DatabaseThread::Startup(dwThreads);

	return true;
}

bool CDBAgent::Connect(bool bMarsEnabled,
					   tstring & strAccountDSN, tstring & strAccountUID, tstring & strAccountPWD,
					   tstring & strGameDSN, tstring & strGameUID, tstring & strGamePWD)
{
	if (!m_AccountDB->Connect(strAccountDSN, strAccountUID, strAccountPWD, bMarsEnabled))
	{
		ReportSQLError(m_AccountDB->GetError());
		return false;
	}

	if (!m_GameDB->Connect(strGameDSN, strGameUID, strGamePWD, bMarsEnabled))
	{
		ReportSQLError(m_GameDB->GetError());
		return false;
	}

	return true;
}

void CDBAgent::ReportSQLError(OdbcError *pError)
{
	if (pError == NULL)
		return;

	// This is *very* temporary.
	string errorMessage = string_format(_T("ODBC error occurred.\r\nSource: %s\r\nError: %s\r\nDescription: %s\n"),
		pError->Source.c_str(), pError->ExtendedErrorMessage.c_str(), pError->ErrorMessage.c_str());

	FastGuard lock(m_lock);
	FILE *fp = fopen("./errors.log", "a");
	if (fp != NULL)
	{
		fwrite(errorMessage.c_str(), errorMessage.length(), 1, fp);
		fclose(fp);
	}

	TRACE("Database error: %s\n", errorMessage.c_str());
	delete pError;
}

int8 CDBAgent::AccountLogin(string & strAccountID, string & strPasswd)
{
	int16 nRet = 0;
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strPasswd.c_str(), strPasswd.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Execute(_T("{CALL ACCOUNT_LOGIN(?, ?, ?)}")))
		ReportSQLError(m_GameDB->GetError());

	return (int8)(nRet - 1);
}

uint8 CDBAgent::NationSelect(string & strAccountID, uint8 bNation)
{
	int16 nRet = 0;
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Execute(string_format(_T("{CALL NATION_SELECT(?, ?, %d)}"), bNation)))
		ReportSQLError(m_GameDB->GetError());

	return (nRet > 0 ? bNation : 0);
}

bool CDBAgent::GetAllCharID(string & strAccountID, string & strCharID1, string & strCharID2, string & strCharID3)
{
	int16 nRet = 0;
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Execute(_T("{? = CALL LOAD_ACCOUNT_CHARID(?)}")))
		ReportSQLError(m_GameDB->GetError());

	if (dbCommand->hasData())
	{
		dbCommand->FetchString(1, strCharID1);
		dbCommand->FetchString(2, strCharID2);
		dbCommand->FetchString(3, strCharID3);
	}

	return (nRet > 0);
}

void CDBAgent::LoadCharInfo(string & strCharID, ByteBuffer & result)
{
	uint32 nHair = 0;
	uint16 sClass = 0, nRet;
	uint8 bRace = 0, bLevel = 0, bFace = 0, bZone = 0; 
	char strItem[INVENTORY_TOTAL * 8];
	ByteBuffer itemData;

	// ensure it's all 0'd out initially.
	memset(strItem, 0x00, sizeof(strItem));

	if (!strCharID.empty())
	{
		auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
		if (dbCommand.get() == NULL)
			return;

		dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
		dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

		if (!dbCommand->Execute(_T("{CALL LOAD_CHAR_INFO (?, ?)}")))
			ReportSQLError(m_GameDB->GetError());

		if (dbCommand->hasData())
		{
			dbCommand->FetchByte(1, bRace);
			dbCommand->FetchUInt16(2, sClass);
			dbCommand->FetchUInt32(3, nHair);
			dbCommand->FetchByte(4, bLevel);
			dbCommand->FetchByte(5, bFace);
			dbCommand->FetchByte(6, bZone);
			dbCommand->FetchBinary(7, strItem, sizeof(strItem));
		}
	}

	itemData.append(strItem, sizeof(strItem));

	result	<< strCharID << bRace << sClass << bLevel << bFace << nHair << bZone;
	for (int i = 0; i < SLOT_MAX; i++) 
	{
		uint32 nItemID;
		uint16 sDurability, sCount;
		itemData >> nItemID >> sDurability >> sCount;
		if (i == HEAD || i == BREAST || i == SHOULDER || i == LEG || i == GLOVE || i == FOOT || i == RIGHTHAND || i == LEFTHAND)
			result << nItemID << sDurability;
	}
}

int8 CDBAgent::CreateNewChar(string & strAccountID, int index, string & strCharID, uint8 bRace, uint16 sClass, uint32 nHair, uint8 bFace, uint8 bStr, uint8 bSta, uint8 bDex, uint8 bInt, uint8 bCha)
{
	int16 nRet = -1;
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return (int8)(nRet);

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{CALL CREATE_NEW_CHAR (?, ?, %d, ?, %d, %d, %d, %d, %d, %d, %d, %d, %d)}"), 
		index, bRace, sClass, nHair, bFace, bStr, bSta, bDex, bInt, bCha)))
		ReportSQLError(m_GameDB->GetError());

	return (int8)(nRet);
}

int8 CDBAgent::ChangeHair(std::string & strAccountID, std::string & strCharID, uint8 bOpcode, uint8 bFace, uint32 nHair)
{
	int8 nRet = 1; // failed
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return nRet;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Execute(string_format(_T("{CALL CHANGE_HAIR (?, ?, %d, %d, %d, ?)}"), 
		bOpcode, bFace, nHair)))
		ReportSQLError(m_GameDB->GetError());

	return nRet;
}

int8 CDBAgent::DeleteChar(string & strAccountID, int index, string & strCharID, string & strSocNo)
{
	int16 nRet = -2; // generic error
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return (int8)(nRet);

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strSocNo.c_str(), strSocNo.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Execute(string_format(_T("{CALL DELETE_CHAR (?, %d, ?, ?, ?)}"), index)))
		ReportSQLError(m_GameDB->GetError());

	return (int8)(nRet);
}

void CDBAgent::LoadRentalData(string & strAccountID, string & strCharID, UserRentalMap & rentalData)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	if (!dbCommand->Execute(_T("{CALL LOAD_RENTAL_DATA(?)}")))
	{
		ReportSQLError(m_GameDB->GetError());
		return;
	}

	if (!dbCommand->hasData())
		return;

	do
	{
		_USER_RENTAL_ITEM *pItem = new _USER_RENTAL_ITEM();
		_RENTAL_ITEM *pRentalItem = NULL;

		dbCommand->FetchString(1, pItem->strUserID);
		if (_strcmpi(pItem->strUserID.c_str(), strCharID.c_str()) != 0)
		{
			delete pItem;
			continue;
		}

		dbCommand->FetchByte(2, pItem->byRentalType);
		dbCommand->FetchByte(3, pItem->byRegType);
		dbCommand->FetchUInt32(4, pItem->nRentalIndex);
		dbCommand->FetchUInt32(5, pItem->nItemID);
		dbCommand->FetchUInt16(6, pItem->sDurability);
		dbCommand->FetchUInt64(7, pItem->nSerialNum);
		dbCommand->FetchUInt32(8, pItem->nRentalMoney);
		dbCommand->FetchUInt16(9, pItem->sRentalTime);
		dbCommand->FetchInt16(10, pItem->sMinutesRemaining);
		dbCommand->FetchString(11, pItem->szTimeRental, sizeof(pItem->szTimeRental));

		pRentalItem = g_pMain->m_RentalItemArray.GetData(pItem->nRentalIndex);
		if (pRentalItem == NULL
			|| rentalData.find(pItem->nSerialNum) != rentalData.end())
			delete pItem;
		else
			rentalData.insert(std::make_pair(pItem->nSerialNum, pItem));

	} while (dbCommand->MoveNext());
}

bool CDBAgent::LoadUserData(string & strAccountID, string & strCharID, CUser *pUser)
{
	uint16 nRet = 0;

	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	if (pUser == NULL 
		|| pUser->m_bLogout
		|| strlen(pUser->GetName()) != 0
		|| strCharID.length() > MAX_ID_SIZE
		/*|| pUser->m_dwTime != 0*/)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Execute(_T("{CALL LOAD_USER_DATA(?, ?, ?)}")))
		ReportSQLError(m_GameDB->GetError());

	if (!dbCommand->hasData())
		return false;

	char	strItem[INVENTORY_TOTAL * 8], strSerial[INVENTORY_TOTAL * 8],
			strQuest[QUEST_ARRAY_SIZE];

	uint16 sQuestCount;

	memset(strItem, 0x00, sizeof(strItem));
	memset(strSerial, 0x00, sizeof(strSerial));

	int field = 1;
	dbCommand->FetchByte(field++, pUser->m_bNation);
	dbCommand->FetchByte(field++, pUser->m_bRace);
	dbCommand->FetchUInt16(field++, pUser->m_sClass);
	dbCommand->FetchUInt32(field++, pUser->m_nHair);
	dbCommand->FetchByte(field++, pUser->m_bRank);
	dbCommand->FetchByte(field++, pUser->m_bTitle);
	dbCommand->FetchByte(field++, pUser->m_bLevel);
	dbCommand->FetchInt64(field++, pUser->m_iExp);
	dbCommand->FetchUInt32(field++, pUser->m_iLoyalty);
	dbCommand->FetchByte(field++, pUser->m_bFace);
	dbCommand->FetchByte(field++, pUser->m_bCity);
	dbCommand->FetchInt16(field++, pUser->m_bKnights);
	dbCommand->FetchByte(field++, pUser->m_bFame);
	dbCommand->FetchInt16(field++, pUser->m_sHp);
	dbCommand->FetchInt16(field++, pUser->m_sMp);
	dbCommand->FetchInt16(field++, pUser->m_sSp);
	dbCommand->FetchByte(field++, pUser->m_bStats[STAT_STR]);
	dbCommand->FetchByte(field++, pUser->m_bStats[STAT_STA]);
	dbCommand->FetchByte(field++, pUser->m_bStats[STAT_DEX]);
	dbCommand->FetchByte(field++, pUser->m_bStats[STAT_INT]);
	dbCommand->FetchByte(field++, pUser->m_bStats[STAT_CHA]);
	dbCommand->FetchByte(field++, pUser->m_bAuthority);
	dbCommand->FetchInt16(field++, pUser->m_sPoints);
	dbCommand->FetchUInt32(field++, pUser->m_iGold);
	dbCommand->FetchByte(field++, pUser->m_bZone);
	dbCommand->FetchInt16(field++, pUser->m_sBind);
	pUser->m_curx = (float)(dbCommand->FetchInt32(field++) / 100.0f);
	pUser->m_curz = (float)(dbCommand->FetchInt32(field++) / 100.0f);
	pUser->m_cury = (float)(dbCommand->FetchInt32(field++) / 100.0f);
	pUser->m_dwTime = dbCommand->FetchUInt32(field++) + 1;
	dbCommand->FetchString(field++, (char *)pUser->m_bstrSkill, sizeof(pUser->m_bstrSkill));
	dbCommand->FetchBinary(field++, strItem, sizeof(strItem));
	dbCommand->FetchBinary(field++, strSerial, sizeof(strSerial));
	dbCommand->FetchUInt16(field++, sQuestCount);
	dbCommand->FetchBinary(field++, strQuest, sizeof(strQuest));
	dbCommand->FetchUInt32(field++, pUser->m_iMannerPoint);
	dbCommand->FetchUInt32(field++, pUser->m_iLoyaltyMonthly);

	// kind of unnecessary
	if (nRet == 0)
		return false;

	pUser->m_strUserID = strCharID;

	// Convert the old quest storage format to the new one.
	pUser->m_questMap.clear();
	for (int i = 0, index = 0; i < sQuestCount; i++)
	{
		uint16	sQuestID	= GetShort(strQuest, index);
		uint8	bQuestState	= GetByte(strQuest, index);

		pUser->m_questMap.insert(std::make_pair(sQuestID, bQuestState));
	}

	ByteBuffer itemBuffer, serialBuffer;
	itemBuffer.append(strItem, sizeof(strItem));
	serialBuffer.append(strSerial, sizeof(strSerial));

	memset(pUser->m_sItemArray, 0x00, sizeof(pUser->m_sItemArray));

	UserRentalMap rentalData;
	LoadRentalData(strAccountID, strCharID, rentalData); 

	for (int i = 0; i < INVENTORY_TOTAL; i++)
	{ 
		uint64 nSerialNum;
		uint32 nItemID;
		int16 sDurability, sCount;

		itemBuffer >> nItemID >> sDurability >> sCount;
		serialBuffer >> nSerialNum;

		_ITEM_TABLE *pTable = g_pMain->GetItemPtr(nItemID);
		if (pTable == NULL || sCount <= 0)
			continue;

		if (!pTable->m_bCountable && sCount > 1)
			sCount = 1;
		else if (sCount > ITEMCOUNT_MAX)
			sCount = ITEMCOUNT_MAX;

		_ITEM_DATA *pItem = pUser->GetItem(i);
		pItem->nNum = nItemID;
		pItem->sDuration = sDurability;
		pItem->sCount = sCount;
		pItem->nSerialNum = nSerialNum;

		if (nSerialNum == 0)
			continue;

		// If the serial was found in the rental data, mark as rented.
		UserRentalMap::iterator itr = rentalData.find(nSerialNum);
		if (itr != rentalData.end())
		{
			pItem->bFlag = ITEM_FLAG_RENTED;
			pItem->sRemainingRentalTime = itr->second->sMinutesRemaining;
		}
	}

	// Clean up the rental data
	foreach (itr, rentalData)
		delete itr->second;
	rentalData.clear();

	// Starter items. This needs fixing eventually.
	if (pUser->GetLevel() == 1 && pUser->m_iExp == 0) // going back to their initial bugginess
	{
		uint32 nItemID = 0;
		uint16 sDurability = 0;

		switch (pUser->m_sClass)
		{
		case 101:
			nItemID = 120010000;
			sDurability = 5000;
			break;
		case 102:
			nItemID = 110010000;
			sDurability = 4000;
			break;
		case 103:
			nItemID = 180010000;
			sDurability = 5000;
			break;
		case 104:
			nItemID = 190010000;
			sDurability = 10000;
			break;
		case 201:
			nItemID = 120050000;
			sDurability = 5000;
			break;
		case 202:
			nItemID = 110050000;
			sDurability = 4000;
			break;
		case 203:
			nItemID = 180050000;
			sDurability = 5000;
			break;
		case 204:
			nItemID = 190050000;
			sDurability = 10000;
			break;
		}

		if (nItemID && !pUser->CheckExistItem(nItemID, 1))
		{
			int slot = pUser->GetEmptySlot();
			if (slot < 0)
				return true;

			_ITEM_DATA * pItem = pUser->GetItem(slot);
			pItem->nNum = nItemID;
			pItem->sDuration = sDurability;
			pItem->sCount = 1;
			pItem->nSerialNum = g_pMain->GenerateItemSerial();
		}
	}

	return true;
}

bool CDBAgent::LoadWarehouseData(string & strAccountID, CUser *pUser)
{
	uint16 nRet = 0;
	char strItem[WAREHOUSE_MAX * 8], strSerial[WAREHOUSE_MAX * 8];

	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	if (pUser == NULL 
		|| pUser->m_bLogout)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Execute(_T("SELECT nMoney, WarehouseData, strSerial FROM WAREHOUSE WHERE strAccountID = ?")))
		ReportSQLError(m_GameDB->GetError());

	if (!dbCommand->hasData())
		return false;

	memset(strItem, 0x00, sizeof(strItem));
	memset(strSerial, 0x00, sizeof(strSerial));

	dbCommand->FetchUInt32(1, pUser->m_iBank);
	dbCommand->FetchBinary(2, strItem, sizeof(strItem));
	dbCommand->FetchBinary(3, strSerial, sizeof(strSerial));

	ByteBuffer itemBuffer, serialBuffer;
	itemBuffer.append(strItem, sizeof(strItem));
	serialBuffer.append(strSerial, sizeof(strSerial));

	memset(pUser->m_sWarehouseArray, 0x00, sizeof(pUser->m_sWarehouseArray));

	for (int i = 0; i < WAREHOUSE_MAX; i++) 
	{
		uint64 nSerialNum;
		uint32 nItemID;
		int16 sDurability, sCount;

		itemBuffer >> nItemID >> sDurability >> sCount;
		serialBuffer >> nSerialNum;

		_ITEM_TABLE *pTable = g_pMain->GetItemPtr(nItemID);
		if (pTable == NULL || sCount <= 0)
			continue;

		if (!pTable->m_bCountable && sCount > 1)
			sCount = 1;
		else if (sCount > ITEMCOUNT_MAX)
			sCount = ITEMCOUNT_MAX;

		pUser->m_sWarehouseArray[i].nNum = nItemID;
		pUser->m_sWarehouseArray[i].sDuration = sDurability;
		pUser->m_sWarehouseArray[i].sCount = sCount;
		pUser->m_sWarehouseArray[i].nSerialNum = nSerialNum;
	}

	return true;
}

bool CDBAgent::LoadPremiumServiceUser(string & strAccountID, CUser *pUser)
{
	if (pUser == NULL)
		return false;

	auto_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &pUser->m_bPremiumType);
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &pUser->m_sPremiumTime);

	if (!dbCommand->Execute(_T("{CALL LOAD_PREMIUM_SERVICE_USER(?, ?, ?)}")))
		ReportSQLError(m_AccountDB->GetError());

	// this is hardcoded because we don't really care about the other mode
	if (pUser->m_bPremiumType != 0 && pUser->m_sPremiumTime != 0)
		pUser->m_bAccountStatus = 1; // normal premium with expiry time

	return true;
}

bool CDBAgent::LoadSavedMagic(CUser *pUser)
{
	if (pUser == NULL)
		return false;

	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	uint16 nRet = 0;
	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	if (!dbCommand->Execute(_T("{CALL LOAD_SAVED_MAGIC(?, ?)}")))
	{
		ReportSQLError(m_GameDB->GetError());
		return false;
	}

	FastGuard lock(pUser->m_savedMagicLock);
	pUser->m_savedMagicMap.clear();
	if (!dbCommand->hasData())
		return true;

	for (int i = 1; i <= 20; i += 2)
	{
		uint32 nSkillID, nExpiry;
		dbCommand->FetchUInt32(i, nSkillID);
		dbCommand->FetchUInt32(i + 1, nExpiry);

		if (nSkillID != 0)
			pUser->m_savedMagicMap[nSkillID] = (nExpiry + UNIXTIME);
	}

	return true;
}

bool CDBAgent::UpdateSavedMagic(CUser *pUser)
{
	if (pUser == NULL)
		return false;

	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;
	
	FastGuard lock(pUser->m_savedMagicLock);
	uint32 nSkillID[10] = {0};
	uint32 tExpiryTime[10] = {0};
	uint32 i = 0;
	foreach (itr, pUser->m_savedMagicMap)
	{
		nSkillID[i]		= itr->first;
		if (itr->first != 0)
			tExpiryTime[i]	= (uint32)(itr->second - UNIXTIME);

		if (++i == 10)
			break;
	}

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());
	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_SAVED_MAGIC(?, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)}"), 
		nSkillID[0], tExpiryTime[0], nSkillID[1], tExpiryTime[1], nSkillID[2], tExpiryTime[2], nSkillID[3], tExpiryTime[3], nSkillID[4], tExpiryTime[4],
		nSkillID[5], tExpiryTime[5], nSkillID[6], tExpiryTime[6], nSkillID[7], tExpiryTime[7], nSkillID[8], tExpiryTime[8], nSkillID[9], tExpiryTime[9])))
	{
		ReportSQLError(m_GameDB->GetError());
		return false;
	}
	
	return true;
}

bool CDBAgent::SetLogInInfo(string & strAccountID, string & strCharID, string & strServerIP, short sServerNo, string & strClientIP, uint8 bInit)
{
	bool result = false;
	auto_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return result;

	tstring szSQL;

	if (bInit == 1)
	{
		dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
		dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
		dbCommand->AddParameter(SQL_PARAM_INPUT, strServerIP.c_str(), strServerIP.length());
		dbCommand->AddParameter(SQL_PARAM_INPUT, strClientIP.c_str(), strClientIP.length());
		szSQL = string_format(_T( "INSERT INTO CURRENTUSER (strAccountID, strCharID, nServerNo, strServerIP, strClientIP) VALUES(?, ?, %d, ?, ?)"), sServerNo);
	}
	else
	{
		dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
		dbCommand->AddParameter(SQL_PARAM_INPUT, strServerIP.c_str(), strServerIP.length());
		szSQL = string_format(_T("UPDATE CURRENTUSER SET nServerNo=%d, strServerIP=? WHERE strAccountID = ?"), sServerNo);
	}

	if (!dbCommand->Execute(szSQL))
		ReportSQLError(m_AccountDB->GetError());
	else
		result = true;

	return result;
}

bool CDBAgent::LoadWebItemMall(Packet & result, CUser *pUser)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName(), strlen(pUser->GetName()));
	if (!dbCommand->Execute(_T("{CALL LOAD_WEB_ITEMMALL(?)}")))
		ReportSQLError(m_AccountDB->GetError());

	if (!dbCommand->hasData())
		return false;

	// preserve write position before we throw the count in (so we know where to overwrite)
	int offset = result.wpos();
	uint16 count = 0;
	result << uint16(0); // placeholder for count

	do
	{
		uint32 nItemID; uint16 sCount;
		dbCommand->FetchUInt32(2, nItemID); // 1 is the account name, which we don't need to use unless we're logging	
		dbCommand->FetchUInt16(3, sCount);

		result << nItemID << sCount;

		// Only limitation here is now the client.
		// NOTE: using the byte buffer this is OK, however we don't want too much in shared memory for now... so we'll keep to our limit
		if (++count >= 100) 
			break;

	} while (dbCommand->MoveNext());

	result.put(offset, count);
	return true;
}

bool CDBAgent::LoadSkillShortcut(Packet & result, CUser *pUser)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	uint16 sCount;
	char strSkillData[260];

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName(), strlen(pUser->GetName()));
	if (!dbCommand->Execute(_T("{CALL SKILLSHORTCUT_LOAD(?)}")))
	{
		ReportSQLError(m_GameDB->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	dbCommand->FetchUInt16(1, sCount);
	dbCommand->FetchString(2, strSkillData, sizeof(strSkillData));

	result << sCount;
	for (uint32 i = 0; i < sCount; i++)
		result << *(uint32 *)(strSkillData + (i * sizeof(uint32)));

	return true;
}

void CDBAgent::SaveSkillShortcut(short sCount, char *buff, CUser *pUser)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName(), strlen(pUser->GetName()));
	dbCommand->AddParameter(SQL_PARAM_INPUT, buff, 260);

	if (!dbCommand->Execute(string_format(_T("{CALL SKILLSHORTCUT_SAVE(?, %d, ?)}"), sCount)))
		ReportSQLError(m_GameDB->GetError());
}

void CDBAgent::RequestFriendList(std::vector<string> & friendList, CUser *pUser)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName(), strlen(pUser->GetName()));
	if (!dbCommand->Execute(_T("SELECT * FROM FRIEND_LIST WHERE strUserID=?")))
		ReportSQLError(m_GameDB->GetError());

	if (!dbCommand->hasData())
		return;

	string strCharID;
	for (int i = 2; i <= 25; i++)
	{
		if (dbCommand->FetchString(i, strCharID)
			&& strCharID.length())
			friendList.push_back(strCharID);
	}
}

FriendAddResult CDBAgent::AddFriend(short sid, short tid)
{
	CUser *pSrcUser = g_pMain->GetUserPtr(sid), *pTargetUser = g_pMain->GetUserPtr(tid);
	if (pSrcUser == NULL || pTargetUser == NULL)
		return FRIEND_ADD_ERROR;

	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return FRIEND_ADD_ERROR;

	int16 nRet = (int16)FRIEND_ADD_ERROR;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pSrcUser->GetName(), strlen(pSrcUser->GetName()));
	dbCommand->AddParameter(SQL_PARAM_INPUT, pTargetUser->GetName(), strlen(pTargetUser->GetName()));
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Execute(_T("{CALL INSERT_FRIEND_LIST(?, ?, ?)}")))
		ReportSQLError(m_GameDB->GetError());

	if (nRet < 0 || nRet >= FRIEND_ADD_MAX)
		nRet = FRIEND_ADD_ERROR;
		
	return (FriendAddResult)nRet;
}

FriendRemoveResult CDBAgent::RemoveFriend(string & strCharID, CUser *pUser)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return FRIEND_REMOVE_ERROR;

	int16 nRet = (int16)FRIEND_REMOVE_ERROR;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_strUserID.c_str(), pUser->m_strUserID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Execute(_T("{CALL DELETE_FRIEND_LIST(?, ?, ?)")))
		ReportSQLError(m_GameDB->GetError());

	if (nRet < 0 || nRet >= FRIEND_REMOVE_MAX)
		nRet = FRIEND_REMOVE_MAX;

	return (FriendRemoveResult)nRet;
}

bool CDBAgent::UpdateUser(string & strCharID, UserUpdateType type, CUser *pUser)
{
	if (strCharID != pUser->GetName())
		return false;

	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	if (type == UPDATE_PACKET_SAVE)
		pUser->m_dwTime++;
	else if (type == UPDATE_LOGOUT || type == UPDATE_ALL_SAVE)
		pUser->m_dwTime = 0;

	char strQuest[QUEST_ARRAY_SIZE];
	memset(strQuest, 0, sizeof(strQuest));
	int index = 0;
	foreach (itr, pUser->m_questMap)
	{
		SetShort(strQuest, itr->first, index);
		SetByte(strQuest, itr->second, index);
	}

	// This *should* be padded like the database field is (unnecessarily), but I want to see how MSSQL repsponds
	ByteBuffer itemBuffer, serialBuffer;
	for (int i = 0; i < INVENTORY_TOTAL; i++)
	{
		_ITEM_DATA *pItem = &pUser->m_sItemArray[i];
		itemBuffer << pItem->nNum << pItem->sDuration << pItem->sCount;
		serialBuffer << pItem->nSerialNum;
	}

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)pUser->m_bstrSkill, sizeof(pUser->m_bstrSkill));
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)itemBuffer.contents(), itemBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)serialBuffer.contents(), serialBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strQuest, sizeof(strQuest), SQL_BINARY);

	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_USER_DATA ("
			"?, " // strCharID 
			"%d, %d, %d, %d, %d, "		// nation, race, class, hair, rank
			"%d, %d, %d, %d, %d, "		// title, level, exp, loyalty, face
			"%d, %d, %d, "				// city, knights, fame
			"%d, %d, %d, "				// hp, mp, sp
			"%d, %d, %d, %d, %d, "		// str, sta, dex, int, cha
			"%d, %d, %d, %d, %d, "		// authority, free points, gold, zone, bind
			"%d, %d, %d, %d, %d, "		// x, z, y, dwTime, sQuestCount
			"?, ?, ?, ?, "				// strSkill, strItem, strSerial, strQuest
			"%d, %d)}"),				// manner points, monthly NP
		pUser->m_bNation, pUser->m_bRace, pUser->m_sClass, pUser->m_nHair, pUser->m_bRank, 
		pUser->m_bTitle, pUser->m_bLevel, (int32)pUser->m_iExp /* temp hack, database needs to support it */, pUser->m_iLoyalty, pUser->m_bFace, 
		pUser->m_bCity,	pUser->m_bKnights, pUser->m_bFame, 
		pUser->m_sHp, pUser->m_sMp, pUser->m_sSp, 
		pUser->m_bStats[STAT_STR], pUser->m_bStats[STAT_STA], pUser->m_bStats[STAT_DEX], pUser->m_bStats[STAT_INT], pUser->m_bStats[STAT_CHA], 
		pUser->m_bAuthority, pUser->m_sPoints, pUser->m_iGold, pUser->m_bZone, pUser->m_sBind, 
		(int)(pUser->m_curx*100), (int)(pUser->m_curz*100), (int)(pUser->m_cury*100), pUser->m_dwTime, pUser->m_questMap.size(), 
		pUser->m_iMannerPoint, pUser->m_iLoyaltyMonthly)))
	{
		ReportSQLError(m_GameDB->GetError());
		return false;
	}

	return UpdateSavedMagic(pUser);
}

bool CDBAgent::UpdateWarehouseData(string & strAccountID, UserUpdateType type, CUser *pUser)
{
	if (strAccountID.length() == 0)
		return false;
	
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	if (type == UPDATE_LOGOUT || type == UPDATE_ALL_SAVE)
		pUser->m_dwTime = 0;

	// This *should* be padded like the database field is (unnecessarily), but I want to see how MSSQL responds.
	ByteBuffer itemBuffer, serialBuffer; 
	for (int i = 0; i < WAREHOUSE_MAX; i++)
	{
		_ITEM_DATA *pItem = &pUser->m_sWarehouseArray[i];
		itemBuffer << pItem->nNum << pItem->sDuration << pItem->sCount;
		serialBuffer << pItem->nSerialNum;
	}

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)itemBuffer.contents(), itemBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)serialBuffer.contents(), serialBuffer.size(), SQL_BINARY);

	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_WAREHOUSE(?,%d,%d,?,?)}"), pUser->m_iBank, pUser->m_dwTime)))
	{
		ReportSQLError(m_GameDB->GetError());
		return false;
	}

	return true;
}

int8 CDBAgent::CreateKnights(uint16 sClanID, uint8 bNation, string & strKnightsName, string & strChief, uint8 bFlag)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return (int8)(-1);

	int16 nRet = -1;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strKnightsName.c_str(), strKnightsName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strChief.c_str(), strChief.length());

	if (!dbCommand->Execute(string_format(_T("{call CREATE_KNIGHTS(?,%d,%d,%d,?,?)}"), sClanID, bNation, bFlag)))
		ReportSQLError(m_GameDB->GetError());

	return (int8)(nRet);
}

int CDBAgent::UpdateKnights(uint8 bType, string & strCharID, uint16 sClanID, uint8 bDomination)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return -1;

	int16 nRet = -1;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_KNIGHTS(?,%d,?,%d,%d)}"), bType, sClanID, bDomination)))
		ReportSQLError(m_GameDB->GetError());

	return nRet;
}

int CDBAgent::DeleteKnights(uint16 sClanID)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	int16 nRet = -1;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	if (!dbCommand->Execute(string_format(_T("{call DELETE_KNIGHTS (?,%d)}"), sClanID)))
		ReportSQLError(m_GameDB->GetError());

	return nRet;
}

uint16 CDBAgent::LoadKnightsAllMembers(uint16 sClanID, Packet & result)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return 0;

	if (!dbCommand->Execute(string_format(_T("{CALL LOAD_KNIGHTS_MEMBERS(%d)}"), sClanID)))
		ReportSQLError(m_GameDB->GetError());

	if (!dbCommand->hasData())
		return 0;

	uint16 count = 0;
	do
	{
		string strCharID; uint16 sClass; uint8 bFame, bLevel;
		dbCommand->FetchString(1, strCharID);
		dbCommand->FetchByte(2, bFame);
		dbCommand->FetchByte(3, bLevel);
		dbCommand->FetchUInt16(4, sClass);

		result << strCharID << bFame << bLevel << sClass 
			// check if user's logged in (i.e. grab logged in state)
			<< uint8(g_pMain->GetUserPtr(strCharID, TYPE_CHARACTER) == NULL ? 0 : 1);
		count++;
	} while (dbCommand->MoveNext());

	return count;
}

bool CDBAgent::LoadKnightsInfo(uint16 sClanID, uint8 & bNation, std::string & strKnightsName, uint16 & sMembers, uint32 & nPoints, uint8 & bRank)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	if (!dbCommand->Execute(string_format(_T("SELECT Nation, IDName, Members, Points, Ranking FROM KNIGHTS WHERE IDNum=%d" ), sClanID)))
		ReportSQLError(m_GameDB->GetError());

	if (!dbCommand->hasData())
		return false;

	dbCommand->FetchByte(1, bNation);
	dbCommand->FetchString(2, strKnightsName);
	dbCommand->FetchUInt16(3, sMembers);
	dbCommand->FetchUInt32(4, nPoints);
	dbCommand->FetchByte(5, bRank);

	return true;
}

void CDBAgent::LoadKnightsAllList(uint8 bNation)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	tstring szSQL;

	if (dbCommand.get() == NULL)
		return;

	// war zone
	if (bNation == 3)
		szSQL = _T("SELECT IDNum, Points, Ranking FROM KNIGHTS WHERE Points != 0 ORDER BY Points DESC");
	else
		szSQL = string_format(_T("SELECT IDNum, Points, Ranking FROM KNIGHTS WHERE Nation=%d AND Points != 0 ORDER BY Points DESC"), bNation); 

	if (!dbCommand->Execute(szSQL))
		ReportSQLError(m_GameDB->GetError());

	if (!dbCommand->hasData())
		return;

	Packet result(WIZ_KNIGHTS_PROCESS);
	uint8 bCount = 0;
	int offset;

	do
	{
		if (bCount == 0)
		{
			result.clear();
			offset = result.wpos();
			result << uint8(0);
		}

		uint32 nPoints; uint16 sClanID; uint8 bRanking;
		dbCommand->FetchUInt16(1, sClanID);
		dbCommand->FetchUInt32(2, nPoints);
		dbCommand->FetchByte(3, bRanking);

		result << sClanID << nPoints << bRanking;

		// only send 100 clans at a time (no shared memory limit, yay!)
		if (++bCount >= 100)
		{
			// overwrite the count
			result.put(offset, bCount);

			CKnightsManager::RecvKnightsAllList(result);
			bCount = 0;
		}
	} while (dbCommand->MoveNext());

	// didn't quite make it in the last batch (if any)? send the remainder.
	if (bCount < 100)
	{
		result.put(offset, bCount);
		CKnightsManager::RecvKnightsAllList(result);
	}
}

bool CDBAgent::UpdateClanSymbol(uint16 sClanID, uint16 sSymbolSize, char *clanSymbol)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	uint16 nRet = 0;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, clanSymbol, MAX_KNIGHTS_MARK, SQL_BINARY);
	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_KNIGHTS_MARK(?, %d, %d, ?)}"), sClanID, sSymbolSize)))
		ReportSQLError(m_GameDB->GetError());

	return (nRet == 1);
}

void CDBAgent::UpdateCape(uint16 sClanID, uint16 sCapeID, uint8 r, uint8 g, uint8 b)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return;
	
	if (!dbCommand->Execute(string_format(_T("UPDATE KNIGHTS SET sCape=%d, bCapeR=%d, bCapeG=%d, bCapeB=%d WHERE IDNum=%d"), 
			sCapeID, r, g, b, sClanID)))
		ReportSQLError(m_GameDB->GetError());
}

void CDBAgent::UpdateBattleEvent(string & strCharID, uint8 bNation)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("UPDATE BATTLE SET byNation=%d, strUserName=? WHERE sIndex=%d"), bNation, g_pMain->m_nServerNo)))
		ReportSQLError(m_GameDB->GetError());
}

void CDBAgent::AccountLogout(string & strAccountID)
{
	auto_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	int16 nRet = 0;
	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Execute(_T("{CALL ACCOUNT_LOGOUT(?, ?)}")))
		ReportSQLError(m_AccountDB->GetError());
}

void CDBAgent::UpdateConCurrentUserCount(int nServerNo, int nZoneNo, int nCount)
{
	auto_ptr<OdbcCommand> dbCommand(m_AccountDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	if (!dbCommand->Execute(string_format(_T("UPDATE CONCURRENT SET zone%d_count = %d WHERE serverid = %d"), nZoneNo, nCount, nServerNo)))
		ReportSQLError(m_AccountDB->GetError());
}

// This is what everything says it should do, 
// but the client doesn't seem to care if it's over 1
uint8 CDBAgent::GetUnreadLetterCount(string & strCharID)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return 0;

	uint8 bCount = 0;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bCount);
	if (!dbCommand->Execute(_T("{CALL MAIL_BOX_CHECK_COUNT(?, ?)}")))
	{
		ReportSQLError(m_GameDB->GetError());
		return 0;
	}

	return bCount;
}

bool CDBAgent::GetLetterList(string & strCharID, Packet & result, bool bNewLettersOnly /* = true*/)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	int8 bCount = 0;
	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &bCount);
	if (!dbCommand->Execute(string_format(_T("{CALL MAIL_BOX_REQUEST_LIST(?, %d, ?)}"), bNewLettersOnly)))
	{
		ReportSQLError(m_GameDB->GetError());
		return false;
	}

	result << uint8(1);
	int offset = result.wpos();
	result << bCount; // placeholder for count

	if (!dbCommand->hasData())
		return true;

	result.SByte();
	do
	{
		string strSubject, strSender;
		uint32 nLetterID, nItemID, nCoins, nDate;
		uint16 sCount, sDaysRemaining;
		uint8 bStatus, bType;

		dbCommand->FetchUInt32(1, nLetterID);
		dbCommand->FetchByte(2, bStatus);
		dbCommand->FetchByte(3, bType);
		dbCommand->FetchString(4, strSubject);
		dbCommand->FetchString(5, strSender);
		dbCommand->FetchByte(6, bType);
		dbCommand->FetchUInt32(7, nItemID);
		dbCommand->FetchUInt16(8, sCount);
		dbCommand->FetchUInt32(9, nCoins);
		dbCommand->FetchUInt32(10, nDate);
		dbCommand->FetchUInt16(11, sDaysRemaining); 

		result	<< nLetterID // letter ID
				<< bStatus  // letter status, doesn't seem to affect anything
				<< strSubject << strSender
				<< bType;	

		if (bType == 2)
			result	<< nItemID << sCount << nCoins;

		result	<< nDate // date (yy*10000 + mm*100 + dd)
				<< sDaysRemaining;

	} while (dbCommand->MoveNext());

	result.put(offset, bCount); // set count now that the result set's been read

	return true;
}

int8 CDBAgent::SendLetter(string & strSenderID, string & strRecipientID, string & strSubject, string & strMessage, uint8 bType, _ITEM_DATA * pItem)
{
	uint64 nSerialNum = 0;
	uint32 nItemID = 0;
	uint16 sCount = 0, sDurability = 0;
	int16 sRet = 0;

	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return 0;

	// This is a little bit redundant, but best to be sure.
	if (bType == 2 
		&& pItem != NULL)
	{
		nItemID = pItem->nNum;
		sCount = pItem->sCount;
		sDurability = pItem->sDuration;
		nSerialNum = pItem->nSerialNum;
	}

	dbCommand->AddParameter(SQL_PARAM_INPUT, strSenderID.c_str(), strSenderID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strRecipientID.c_str(), strRecipientID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strSubject.c_str(), strSubject.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strMessage.c_str(), strMessage.length());

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &sRet);

	// NOTE: %I64d is signed int64 for Microsoft compilers (all we care about right now)
	// Also: MSSQL uses signed types.
	if (!dbCommand->Execute(string_format(_T("{CALL MAIL_BOX_SEND(?, ?, ?, ?, %d, %d, %d, %d, %I64d, ?)}"), 
		bType, nItemID, sCount, sDurability, nSerialNum)))
	{
		ReportSQLError(m_GameDB->GetError());
		return 0;
	}

	return (int8)(sRet);
}

bool CDBAgent::ReadLetter(string & strCharID, uint32 nLetterID, string & strMessage)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("{CALL MAIL_BOX_READ(?, %d)}"), nLetterID)))
	{
		ReportSQLError(m_GameDB->GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	dbCommand->FetchString(1, strMessage);
	return true;
}

int8 CDBAgent::GetItemFromLetter(string & strCharID, uint32 nLetterID, uint32 & nItemID, uint16 & sCount, uint16 & sDurability, uint32 & nCoins, uint64 & nSerialNum)
{
	// Invalid letter ID
	if (nLetterID == 0)
		return -4;

	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return -1; // error

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("{CALL MAIL_BOX_GET_ITEM(?, %d)}"), nLetterID)))
	{
		ReportSQLError(m_GameDB->GetError());
		return -1; // error
	}

	if (!dbCommand->hasData())
		return -2; // letter not found

	dbCommand->FetchUInt32(1, nItemID);
	dbCommand->FetchUInt16(2, sCount);
	dbCommand->FetchUInt16(3, sDurability);
	dbCommand->FetchUInt32(4, nCoins);
	dbCommand->FetchUInt64(5, nSerialNum);

	return 1;
}

void CDBAgent::DeleteLetter(string & strCharID, uint32 nLetterID)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	// NOTE: The official implementation passes all 5 letter IDs.
	if (!dbCommand->Execute(string_format(_T("{CALL MAIL_BOX_DELETE_LETTER(?, %d)}"), nLetterID)))
		ReportSQLError(m_GameDB->GetError());
}

void CDBAgent::UpdateNoahOrExpEvent(uint8 byType, uint8 byNation, uint8 byAmount, uint8 byDay, uint8 byHour, uint8 byMinute, uint16 sDuration)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	if (!dbCommand->Execute(string_format(_T("{CALL KING_UPDATE_NOAH_OR_EXP_EVENT(%d, %d, %d, %d, %d, %d, %d)}"), 
		byType, byNation, byAmount, byDay, byHour, byMinute, sDuration)))
		ReportSQLError(m_GameDB->GetError());
}

void CDBAgent::InsertPrizeEvent(uint8 byType, uint8 byNation, uint32 nCoins, std::string & strCharID)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("{CALL KING_INSERT_PRIZE_EVENT(%d, %d, %d, ?)}"), 
		byType, byNation, nCoins)))
		ReportSQLError(m_GameDB->GetError());
}