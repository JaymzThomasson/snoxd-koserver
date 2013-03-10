#include "stdafx.h"

using std::auto_ptr;

bool CDBAgent::Startup()
{
	if (!Connect())
	{
		// we should probably be a little more specific (i.e. *which* database server)
		AfxMessageBox(_T("Failed to connect to the database server."));
		return false;
	}

	// If MARS is enabled, we can use multiple database threads.
	DWORD dwThreads = 1;
	if (g_pMain->m_bMarsEnabled)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		dwThreads = si.dwNumberOfProcessors * 2;
	}
	DatabaseThread::Startup(dwThreads);

	return true;
}

bool CDBAgent::Connect()
{
	if (!m_AccountDB.Connect(g_pMain->m_strAccountDSN, g_pMain->m_strAccountUID, g_pMain->m_strAccountPWD, g_pMain->m_bMarsEnabled))
	{
		ReportSQLError(m_AccountDB.GetError());
		return false;
	}

	if (!m_GameDB.Connect(g_pMain->m_strGameDSN, g_pMain->m_strGameUID, g_pMain->m_strGamePWD, g_pMain->m_bMarsEnabled))
	{
		ReportSQLError(m_GameDB.GetError());
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

	m_lock.Acquire();
	FILE *fp = fopen("./errors.log", "a");
	if (fp != NULL)
	{
		fwrite(errorMessage.c_str(), errorMessage.length(), 1, fp);
		fclose(fp);
	}
	m_lock.Release();

	TRACE("Database error: %s\n", errorMessage.c_str());
	delete pError;
}

int8 CDBAgent::AccountLogin(string & strAccountID, string & strPasswd)
{
	int16 nRet = 0;
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, strPasswd.c_str(), strPasswd.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Execute(_T("{CALL ACCOUNT_LOGIN(?, ?, ?)}")))
		ReportSQLError(m_GameDB.GetError());

	return (int8)(nRet - 1);
}

uint8 CDBAgent::NationSelect(string & strAccountID, uint8 bNation)
{
	int16 nRet = 0;
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Execute(string_format(_T("{CALL NATION_SELECT(?, ?, %d)}"), bNation)))
		ReportSQLError(m_GameDB.GetError());

	return (nRet > 0 ? bNation : 0);
}

bool CDBAgent::GetAllCharID(string & strAccountID, string & strCharID1, string & strCharID2, string & strCharID3)
{
	int16 nRet = 0;
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Execute(_T("{? = CALL LOAD_ACCOUNT_CHARID(?)}")))
		ReportSQLError(m_GameDB.GetError());

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

	if (strCharID.length() > 0)
	{
		auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
		if (dbCommand.get() == NULL)
			return;

		dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());
		dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

		if (!dbCommand->Execute(_T("{CALL LOAD_CHAR_INFO (?, ?)}")))
			ReportSQLError(m_GameDB.GetError());

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

	// ensure it's all 0'd out initially.
	memset(strItem, 0x00, sizeof(strItem));
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
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return (int8)(nRet);

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{CALL CREATE_NEW_CHAR (?, ?, %d, ?, %d, %d, %d, %d, %d, %d, %d, %d, %d)}"), 
		index, bRace, sClass, nHair, bFace, bStr, bSta, bDex, bInt, bCha)))
		ReportSQLError(m_GameDB.GetError());

	return (int8)(nRet);
}

int8 CDBAgent::ChangeHair(std::string & strAccountID, std::string & strCharID, uint8 bOpcode, uint8 bFace, uint32 nHair)
{
	int8 nRet = 1; // failed
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return nRet;

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Execute(string_format(_T("{CALL CHANGE_HAIR (?, ?, %d, %d, %d, ?)}"), 
		bOpcode, bFace, nHair)))
		ReportSQLError(m_GameDB.GetError());

	return nRet;
}

int8 CDBAgent::DeleteChar(string & strAccountID, int index, string & strCharID, string & strSocNo)
{
	int16 nRet = -2; // generic error
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return (int8)(nRet);

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strSocNo.c_str(), strSocNo.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Execute(string_format(_T("{CALL DELETE_CHAR (?, %d, ?, ?, ?)}"), index)))
		ReportSQLError(m_GameDB.GetError());

	return (int8)(nRet);
}

bool CDBAgent::LoadUserData(string & strAccountID, string & strCharID, CUser *pUser)
{
	uint16 nRet = 0;

	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	if (pUser == NULL 
		|| pUser->m_bLogout
		|| strlen(pUser->GetName()) != 0
		|| strCharID.length() > MAX_ID_SIZE
		/*|| pUser->m_dwTime != 0*/)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Execute(_T("{CALL LOAD_USER_DATA(?, ?, ?)}")))
		ReportSQLError(m_GameDB.GetError());

	if (!dbCommand->hasData())
		return false;

	char strItem[INVENTORY_TOTAL * 8], strSerial[INVENTORY_TOTAL * 8];
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
	dbCommand->FetchInt32(field++, pUser->m_iLoyalty);
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
	dbCommand->FetchUInt16(field++, pUser->m_sPoints);
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
	dbCommand->FetchUInt16(field++, pUser->m_sQuestCount);
	dbCommand->FetchBinary(field++, pUser->m_bstrQuest, sizeof(pUser->m_bstrQuest));
	dbCommand->FetchInt32(field++, pUser->m_iMannerPoint);
	dbCommand->FetchInt32(field++, pUser->m_iLoyaltyMonthly);

	// kind of unnecessary
	if (nRet == 0)
		return false;

	pUser->m_strUserID = strCharID;

	ByteBuffer itemBuffer, serialBuffer;
	itemBuffer.append(strItem, sizeof(strItem));
	serialBuffer.append(strSerial, sizeof(strSerial));

	memset(pUser->m_sItemArray, 0x00, sizeof(pUser->m_sItemArray));

	for (int i = 0; i < HAVE_MAX+SLOT_MAX; i++)
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

		pUser->m_sItemArray[i].nNum = nItemID;
		pUser->m_sItemArray[i].sDuration = sDurability;
		pUser->m_sItemArray[i].sCount = sCount;
		pUser->m_sItemArray[i].nSerialNum = nSerialNum;
	}


	// Starter items. This needs fixing eventually.
	if (pUser->m_bLevel == 1 && pUser->m_iExp == 0 && pUser->m_iGold == 0)
	{
		int empty_slot = 0;

		for (int j = SLOT_MAX; j < HAVE_MAX + SLOT_MAX; j++)
		{
			if (pUser->m_sItemArray[j].nNum == 0)
			{
				empty_slot = j;
				break;
			}
		}
		if (empty_slot == HAVE_MAX + SLOT_MAX)
			return true;

		switch (pUser->m_sClass)
		{
		case 101:
			pUser->m_sItemArray[empty_slot].nNum = 120010000;
			pUser->m_sItemArray[empty_slot].sDuration = 5000;
			break;
		case 102:
			pUser->m_sItemArray[empty_slot].nNum = 110010000;
			pUser->m_sItemArray[empty_slot].sDuration = 4000;
			break;
		case 103:
			pUser->m_sItemArray[empty_slot].nNum = 180010000;
			pUser->m_sItemArray[empty_slot].sDuration = 5000;
			break;
		case 104:
			pUser->m_sItemArray[empty_slot].nNum = 190010000;
			pUser->m_sItemArray[empty_slot].sDuration = 10000;
			break;
		case 201:
			pUser->m_sItemArray[empty_slot].nNum = 120050000;
			pUser->m_sItemArray[empty_slot].sDuration = 5000;
			break;
		case 202:
			pUser->m_sItemArray[empty_slot].nNum = 110050000;
			pUser->m_sItemArray[empty_slot].sDuration = 4000;
			break;
		case 203:
			pUser->m_sItemArray[empty_slot].nNum = 180050000;
			pUser->m_sItemArray[empty_slot].sDuration = 5000;
			break;
		case 204:
			pUser->m_sItemArray[empty_slot].nNum = 190050000;
			pUser->m_sItemArray[empty_slot].sDuration = 10000;
			break;
		}

		pUser->m_sItemArray[empty_slot].sCount = 1;
		pUser->m_sItemArray[empty_slot].nSerialNum = 0;
	}

	return true;
}

bool CDBAgent::LoadWarehouseData(string & strAccountID, CUser *pUser)
{
	uint16 nRet = 0;
	char strItem[WAREHOUSE_MAX * 8], strSerial[WAREHOUSE_MAX * 8];

	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	if (pUser == NULL 
		|| pUser->m_bLogout)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Execute(_T("SELECT nMoney, WarehouseData, strSerial FROM WAREHOUSE WHERE strAccountID = ?")))
		ReportSQLError(m_GameDB.GetError());

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

bool CDBAgent::LoadPremiumServiceUser(std::string & strAccountID, CUser *pUser)
{
	if (pUser == NULL)
		return false;

	auto_ptr<OdbcCommand> dbCommand(m_AccountDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &pUser->m_bPremiumType);
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &pUser->m_sPremiumTime);

	if (!dbCommand->Execute(_T("{CALL LOAD_PREMIUM_SERVICE_USER(?, ?, ?)}")))
		ReportSQLError(m_AccountDB.GetError());

	// this is hardcoded because we don't really care about the other mode
	if (pUser->m_bPremiumType != 0 && pUser->m_sPremiumTime != 0)
		pUser->m_bAccountStatus = 1; // normal premium with expiry time

	return true;
}

bool CDBAgent::SetLogInInfo(string & strAccountID, string & strCharID, string & strServerIP, short sServerNo, string & strClientIP, uint8 bInit)
{
	bool result = false;
	auto_ptr<OdbcCommand> dbCommand(m_AccountDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return result;

	tstring szSQL;

	if (bInit == 1)
	{
		dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());
		dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());
		dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strServerIP.c_str(), strServerIP.length());
		dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strClientIP.c_str(), strClientIP.length());
		szSQL = string_format(_T( "INSERT INTO CURRENTUSER (strAccountID, strCharID, nServerNo, strServerIP, strClientIP) VALUES(?, ?, %d, ?, ?)"), sServerNo);
	}
	else
	{
		dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());
		dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strServerIP.c_str(), strServerIP.length());
		szSQL = string_format(_T("UPDATE CURRENTUSER SET nServerNo=%d, strServerIP=? WHERE strAccountID = ?"), sServerNo);
	}

	if (!dbCommand->Execute(szSQL))
		ReportSQLError(m_AccountDB.GetError());
	else
		result = true;

	return result;
}

bool CDBAgent::LoadWebItemMall(Packet & result, CUser *pUser)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName(), strlen(pUser->GetName()));
	if (!dbCommand->Execute(_T("{CALL LOAD_WEB_ITEMMALL(?)}")))
		ReportSQLError(m_AccountDB.GetError());

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

		// don't want to crash Aujard with too many (100 = (6*100) [items] + 2 [count] + 2 [opcode/subopcode] + 1 [result] + 2 [uid] = 607)
		// NOTE: using the byte buffer this is OK, however we don't want too much in shared memory for now... so we'll keep to our limit
		if (++count >= 100) 
			break;

	} while (dbCommand->MoveNext());

	result.put(offset, count);
	return true;
}

bool CDBAgent::LoadSkillShortcut(Packet & result, CUser *pUser)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	uint16 sCount;
	char strSkillData[260];

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName(), strlen(pUser->GetName()));
	if (!dbCommand->Execute(_T("{CALL SKILLSHORTCUT_LOAD(?)}")))
	{
		ReportSQLError(m_GameDB.GetError());
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
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName(), strlen(pUser->GetName()));
	dbCommand->AddParameter(SQL_PARAM_INPUT, buff, 260);

	if (!dbCommand->Execute(string_format(_T("{CALL SKILLSHORTCUT_SAVE(?, %d, ?)}"), sCount)))
		ReportSQLError(m_GameDB.GetError());
}

void CDBAgent::RequestFriendList(std::vector<string> & friendList, CUser *pUser)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName(), strlen(pUser->GetName()));
	if (!dbCommand->Execute(_T("SELECT * FROM FRIEND_LIST WHERE strUserID=?")))
		ReportSQLError(m_GameDB.GetError());

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

	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return FRIEND_ADD_ERROR;

	int16 nRet = (int16)FRIEND_ADD_ERROR;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pSrcUser->GetName(), strlen(pSrcUser->GetName()));
	dbCommand->AddParameter(SQL_PARAM_INPUT, pTargetUser->GetName(), strlen(pTargetUser->GetName()));
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Execute(_T("{CALL INSERT_FRIEND_LIST(?, ?, ?)}")))
		ReportSQLError(m_GameDB.GetError());

	if (nRet < 0 || nRet >= FRIEND_ADD_MAX)
		nRet = FRIEND_ADD_ERROR;
		
	return (FriendAddResult)nRet;
}

FriendRemoveResult CDBAgent::RemoveFriend(string & strCharID, CUser *pUser)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return FRIEND_REMOVE_ERROR;

	int16 nRet = (int16)FRIEND_REMOVE_ERROR;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->GetName(), strlen(pUser->GetName()));
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Execute(_T("{CALL DELETE_FRIEND_LIST(?, ?, ?)")))
		ReportSQLError(m_GameDB.GetError());

	if (nRet < 0 || nRet >= FRIEND_REMOVE_MAX)
		nRet = FRIEND_REMOVE_MAX;

	return (FriendRemoveResult)nRet;
}

bool CDBAgent::UpdateUser(string & strCharID, UserUpdateType type, CUser *pUser)
{
	if (strCharID != pUser->GetName())
		return false;

	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	if (type == UPDATE_PACKET_SAVE)
		pUser->m_dwTime++;
	else if (type == UPDATE_LOGOUT || type == UPDATE_ALL_SAVE)
		pUser->m_dwTime = 0;

	// This *should* be padded like the database field is (unnecessarily), but I want to see how MSSQL repsponds
	ByteBuffer itemBuffer, serialBuffer;
	for (int i = 0; i < INVENTORY_TOTAL; i++)
	{
		_ITEM_DATA *pItem = &pUser->m_sItemArray[i];
		itemBuffer << pItem->nNum << pItem->sDuration << pItem->sCount;
		serialBuffer << pItem->nSerialNum;
	}

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)pUser->m_bstrSkill, sizeof(pUser->m_bstrSkill));
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)itemBuffer.contents(), itemBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)serialBuffer.contents(), serialBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)pUser->m_bstrQuest, sizeof(pUser->m_bstrQuest), SQL_BINARY);

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
		(int)(pUser->m_curx*100), (int)(pUser->m_curz*100), (int)(pUser->m_cury*100), pUser->m_dwTime, pUser->m_sQuestCount, 
		pUser->m_iMannerPoint, pUser->m_iLoyaltyMonthly)))
	{
		ReportSQLError(m_GameDB.GetError());
		return false;
	}

	return true;
}

bool CDBAgent::UpdateWarehouseData(string & strAccountID, UserUpdateType type, CUser *pUser)
{
	if (strAccountID.length() == 0)
		return false;
	
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
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

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)itemBuffer.contents(), itemBuffer.size(), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)serialBuffer.contents(), serialBuffer.size(), SQL_BINARY);

	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_WAREHOUSE(?,%d,%d,?,?)}"), pUser->m_iBank, pUser->m_dwTime)))
	{
		ReportSQLError(m_GameDB.GetError());
		return false;
	}

	return true;
}

int8 CDBAgent::CreateKnights(uint16 sClanID, uint8 bNation, string & strKnightsName, string & strChief, uint8 bFlag)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return (int8)(-1);

	int16 nRet = -1;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strKnightsName.c_str(), strKnightsName.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strChief.c_str(), strChief.length());

	if (!dbCommand->Execute(string_format(_T("{call CREATE_KNIGHTS(?,%d,%d,%d,?,?)}"), sClanID, bNation, bFlag)))
		ReportSQLError(m_GameDB.GetError());

	return (int8)(nRet);
}

int CDBAgent::UpdateKnights(uint8 bType, string & strCharID, uint16 sClanID, uint8 bDomination)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return -1;

	int16 nRet = -1;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_KNIGHTS(?,%d,?,%d,%d)}"), bType, sClanID, bDomination)))
		ReportSQLError(m_GameDB.GetError());

	return nRet;
}

int CDBAgent::DeleteKnights(uint16 sClanID)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	int16 nRet = -1;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	if (!dbCommand->Execute(string_format(_T("{call DELETE_KNIGHTS (?,%d)}"), sClanID)))
		ReportSQLError(m_GameDB.GetError());

	return nRet;
}

uint16 CDBAgent::LoadKnightsAllMembers(uint16 sClanID, Packet & result)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return 0;

	if (!dbCommand->Execute(string_format(_T("{CALL LOAD_KNIGHTS_MEMBERS(%d)}"), sClanID)))
		ReportSQLError(m_GameDB.GetError());

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

		rtrim(strCharID);

		result << strCharID << bFame << bLevel << sClass 
			// check if user's logged in (i.e. grab logged in state)
			<< uint8(g_pMain->GetUserPtr(strCharID.c_str(), TYPE_CHARACTER) == NULL ? 0 : 1);
		count++;
	} while (dbCommand->MoveNext());

	return count;
}

bool CDBAgent::LoadKnightsInfo(uint16 sClanID, uint8 & bNation, std::string & strKnightsName, uint16 & sMembers, uint32 & nPoints, uint8 & bRank)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	if (!dbCommand->Execute(string_format(_T("SELECT Nation, IDName, Members, Points, Ranking FROM KNIGHTS WHERE IDNum=%d" ), sClanID)))
		ReportSQLError(m_GameDB.GetError());

	if (!dbCommand->hasData())
		return false;

	dbCommand->FetchByte(1, bNation);
	dbCommand->FetchString(2, strKnightsName);
	dbCommand->FetchUInt16(3, sMembers);
	dbCommand->FetchUInt32(4, nPoints);
	dbCommand->FetchByte(5, bRank);

	rtrim(strKnightsName);
	return true;
}

void CDBAgent::LoadKnightsAllList(uint8 bNation)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	tstring szSQL;

	if (dbCommand.get() == NULL)
		return;

	// war zone
	if (bNation == 3)
		szSQL = _T("SELECT IDNum, Points, Ranking FROM KNIGHTS WHERE Points != 0 ORDER BY Points DESC");
	else
		szSQL = string_format(_T("SELECT IDNum, Points, Ranking FROM KNIGHTS WHERE Nation=%d AND Points != 0 ORDER BY Points DESC"), bNation); 

	if (!dbCommand->Execute(szSQL))
		ReportSQLError(m_GameDB.GetError());

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
			result << uint8(KNIGHTS_ALLLIST_REQ) << uint8(0);
			offset = result.wpos() - 1;
		}

		uint32 nPoints; uint16 sClanID; uint8 bRanking;
		dbCommand->FetchUInt16(1, sClanID);
		dbCommand->FetchUInt32(2, nPoints);
		dbCommand->FetchByte(3, bRanking);

		result << sClanID << nPoints << bRanking;

		// can only send 40-ish clans at a time (need to check actual memory limit)
		if (++bCount >= 40)
		{
			// overwrite the count
			result.put(offset, bCount);
			//g_pMain->m_LoggerSendQueue.PutData(&result);
			bCount = 0;
		}
	} while (dbCommand->MoveNext());

	// didn't quite make it in the last batch (if any)? send the remainder.
	if (bCount < 40)
	{
		result.put(offset, bCount);
		//g_pMain->m_LoggerSendQueue.PutData(&result);
	}
}

bool CDBAgent::UpdateClanSymbol(uint16 sClanID, uint16 sSymbolSize, char *clanSymbol)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	uint16 nRet = 0;
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, clanSymbol, MAX_KNIGHTS_MARK, SQL_BINARY);
	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_KNIGHTS_MARK(?, %d, %d, ?)}"), sClanID, sSymbolSize)))
		ReportSQLError(m_GameDB.GetError());

	return (nRet == 1);
}

void CDBAgent::UpdateCape(uint16 sClanID, uint16 sCapeID, uint8 r, uint8 g, uint8 b)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return;
	
	if (!dbCommand->Execute(string_format(_T("UPDATE KNIGHTS SET sCape=%d, bCapeR=%d, bCapeG=%d, bCapeB=%d WHERE IDNum=%d"), 
			sCapeID, r, g, b, sClanID)))
		ReportSQLError(m_GameDB.GetError());
}

void CDBAgent::UpdateBattleEvent(string & strCharID, uint8 bNation)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());
	if (!dbCommand->Execute(string_format(_T("UPDATE BATTLE SET byNation=%d, strUserName=? WHERE sIndex=%d"), bNation, g_pMain->m_nServerNo)))
		ReportSQLError(m_GameDB.GetError());
}

void CDBAgent::AccountLogout(string & strAccountID)
{
	auto_ptr<OdbcCommand> dbCommand(m_AccountDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	int16 nRet = 0;
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Execute(_T("{CALL ACCOUNT_LOGOUT(?, ?)}")))
		ReportSQLError(m_AccountDB.GetError());
}

void CDBAgent::UpdateConCurrentUserCount(int nServerNo, int nZoneNo, int nCount)
{
	auto_ptr<OdbcCommand> dbCommand(m_AccountDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	if (!dbCommand->Execute(string_format(_T("UPDATE CONCURRENT SET zone%d_count = %d WHERE serverid = %d"), nZoneNo, nCount, nServerNo)))
		ReportSQLError(m_AccountDB.GetError());
}