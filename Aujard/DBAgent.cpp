// DBAgent.cpp: implementation of the CDBAgent class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Aujard.h"
#include "DBAgent.h"
#include "AujardDlg.h"

CDBAgent::CDBAgent() : m_pMain(NULL)
{
}

bool CDBAgent::Connect()
{
	if (m_pMain == NULL)
		m_pMain = (CAujardDlg*)AfxGetApp()->GetMainWnd();

	if (!m_AccountDB.Connect(m_pMain->m_strAccountDSN, m_pMain->m_strAccountUID, m_pMain->m_strAccountPWD))
	{
		m_pMain->ReportSQLError(m_AccountDB.GetError());
		return false;
	}

	if (!m_GameDB.Connect(m_pMain->m_strGameDSN, m_pMain->m_strGameUID, m_pMain->m_strGamePWD))
	{
		m_pMain->ReportSQLError(m_GameDB.GetError());
		return false;
	}

	return true;
}

bool CDBAgent::LoadItemTable()
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL
		|| !dbCommand->Execute(_T("SELECT Num, Countable FROM ITEM")))
	{
		AfxMessageBox(_T("An error occurred trying to open the ITEM table."));
		m_pMain->ReportSQLError(m_GameDB.GetError());
		return false;
	}
	
	if (!dbCommand->hasData())
	{
		AfxMessageBox(_T("ITEM table is empty."));
		return false;
	}

	do
	{
		_ITEM_TABLE *pItem = new _ITEM_TABLE;
		pItem->m_iNum = dbCommand->FetchUInt32(1);
		pItem->m_bCountable = dbCommand->FetchByte(2);
		m_itemTableArray.PutData(pItem->m_iNum, pItem);
	} while (dbCommand->MoveNext());

	return true;
}

void CDBAgent::MUserInit(uint16 uid)
{
	_USER_DATA* pUser = GetUser(uid);
	if (pUser == NULL)
		return;

	memset(pUser, 0x00, sizeof(_USER_DATA));
	pUser->m_bAuthority = 1;
	pUser->m_sBind = -1;
}

_USER_DATA *CDBAgent::GetUser(uint16 uid)
{
	if (uid >= m_UserDataArray.size())
		return NULL;

	return m_UserDataArray[uid];
}

int8 CDBAgent::AccountLogin(string & strAccountID, string & strPasswd)
{
	int16 nRet = 0;
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strPasswd.c_str(), strPasswd.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Prepare(_T("{CALL ACCOUNT_LOGIN(?, ?, ?)}")))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	return (int8)(nRet - 1);
}

bool CDBAgent::NationSelect(string & strAccountID, uint8 bNation)
{
	int16 nRet = 0;
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Prepare(string_format(_T("{CALL NATION_SELECT(?, ?, %d)}"), bNation)))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	return (nRet > 0);
}

bool CDBAgent::GetAllCharID(string & strAccountID, string & strCharID1, string & strCharID2, string & strCharID3)
{
	int16 nRet = 0;
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Prepare(_T("{? = CALL LOAD_ACCOUNT_CHARID(?)}")))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	if (dbCommand->hasData())
	{
		strCharID1 = dbCommand->FetchString(1);
		strCharID2 = dbCommand->FetchString(2);
		strCharID3 = dbCommand->FetchString(3);
	}

	return (nRet > 0);
}

void CDBAgent::LoadCharInfo(string & strCharID, ByteBuffer & result)
{
	uint32 nHair = 0;
	uint16 sClass = 0, nRet;
	uint8 bRace = 0, bLevel = 0, bFace = 0, bZone = 0; 
	char strItem[400];
	ByteBuffer itemData;

	if (strCharID.length() > 0)
	{
		auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
		if (dbCommand.get() == NULL)
			return;

		dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());
		dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

		if (!dbCommand->Prepare(_T("{CALL LOAD_CHAR_INFO (?, ?)}")))
			m_pMain->ReportSQLError(m_GameDB.GetError());

		if (dbCommand->hasData())
		{
			dbCommand->FetchByte(1, bRace);
			dbCommand->FetchUInt16(2, sClass);
			dbCommand->FetchUInt32(3, nHair);
			dbCommand->FetchByte(4, bLevel);
			dbCommand->FetchByte(5, bFace);
			dbCommand->FetchByte(6, bZone);
			dbCommand->FetchString(7, strItem, sizeof(strItem));
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

	if (!dbCommand->Prepare(string_format(_T("{CALL CREATE_NEW_CHAR (?, ?, %d, ?, %d, %d, %d, %d, %d, %d, %d, %d, %d)}"), 
		index, bRace, sClass, nHair, bFace, bStr, bSta, bDex, bInt, bCha)))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	return (int8)(nRet);
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

	if (!dbCommand->Prepare(string_format(_T("{CALL DELETE_CHAR (?, %d, ?, ?, ?)}"), index)))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	return (int8)(nRet);
}

bool CDBAgent::LoadUserData(string & strAccountID, string & strCharID, short uid)
{
	uint16 nRet = 0;

	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	_USER_DATA *pUser = GetUser(uid);
	if (pUser == NULL 
		|| pUser->m_bLogout
		|| strlen(pUser->m_id) != 0
		|| strCharID.length() > MAX_ID_SIZE
		/*|| pUser->m_dwTime != 0*/)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Prepare(_T("{CALL LOAD_USER_DATA(?, ?, ?)}")))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	if (!dbCommand->hasData())
		return false;

	char strItem[400], strSerial[400];
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
	dbCommand->FetchByte(field++, pUser->m_bStr);
	dbCommand->FetchByte(field++, pUser->m_bSta);
	dbCommand->FetchByte(field++, pUser->m_bDex);
	dbCommand->FetchByte(field++, pUser->m_bIntel);
	dbCommand->FetchByte(field++, pUser->m_bCha);
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
	dbCommand->FetchString(field++, strItem, sizeof(strItem));
	dbCommand->FetchString(field++, strSerial, sizeof(strSerial));
	dbCommand->FetchUInt16(field++, pUser->m_sQuestCount);
	dbCommand->FetchString(field++, (char *)pUser->m_bstrQuest, sizeof(pUser->m_bstrQuest));
	dbCommand->FetchInt32(field++, pUser->m_iMannerPoint);
	dbCommand->FetchInt32(field++, pUser->m_iLoyaltyMonthly);

	// kind of unnecessary
	if (nRet == 0)
		return false;

	_tstrcpy(pUser->m_id, strCharID);

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

		_ITEM_TABLE *pTable = m_itemTableArray.GetData(nItemID);

		if (pTable == NULL || sCount <= 0)
			continue;

		if (!pTable->m_bCountable && sCount > 1)
			sCount = 1;
		else if (sCount > ITEMCOUNT_MAX)
			sCount = ITEMCOUNT_MAX;

		pUser->m_sItemArray[i].nNum = nItemID;
		pUser->m_sItemArray[i].sDuration = sDurability;
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

bool CDBAgent::LoadWarehouseData(string & strAccountID, short uid)
{
	uint16 nRet = 0;
	char strItem[1600], strSerial[1600];

	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	_USER_DATA *pUser = GetUser(uid);
	if (pUser == NULL 
		|| pUser->m_bLogout)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());

	if (!dbCommand->Prepare(_T("SELECT nMoney, WarehouseData, strSerial FROM WAREHOUSE WHERE strAccountID = ?")))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	if (!dbCommand->hasData())
		return false;

	memset(strItem, 0x00, sizeof(strItem));
	memset(strSerial, 0x00, sizeof(strSerial));

	dbCommand->FetchUInt32(1, pUser->m_iBank);
	dbCommand->FetchString(2, strItem, sizeof(strItem));
	dbCommand->FetchString(3, strSerial, sizeof(strSerial));

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

		_ITEM_TABLE *pTable = m_itemTableArray.GetData(nItemID);
		if (pTable == NULL || sCount <= 0)
			continue;

		if (!pTable->m_bCountable && sCount > 1)
			sCount = 1;
		else if (sCount > ITEMCOUNT_MAX)
			sCount = ITEMCOUNT_MAX;

		pUser->m_sWarehouseArray[i].nNum = nItemID;
		pUser->m_sWarehouseArray[i].sDuration = sDurability;
		pUser->m_sWarehouseArray[i].sCount = ITEMCOUNT_MAX;
		pUser->m_sWarehouseArray[i].nSerialNum = nSerialNum;
	}

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

	if (!dbCommand->Prepare(szSQL))
		m_pMain->ReportSQLError(m_AccountDB.GetError());
	else
		result = true;

	return result;
}

bool CDBAgent::LoadWebItemMall(short uid, Packet & result)
{
	_USER_DATA *pUser = GetUser(uid);
	if (pUser == NULL)
		return false;

	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_id, strlen(pUser->m_id));
	if (!dbCommand->Prepare(_T("{CALL LOAD_WEB_ITEMMALL(?)}")))
		m_pMain->ReportSQLError(m_AccountDB.GetError());

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

bool CDBAgent::LoadSkillShortcut(short uid, Packet & result)
{
	_USER_DATA *pUser = GetUser(uid);
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL || pUser == NULL)
		return false;

	uint16 sCount;
	char strSkillData[260];

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_id, strlen(pUser->m_id));
	if (!dbCommand->Prepare(_T("{CALL SKILLSHORTCUT_LOAD(?)}")))
	{
		m_pMain->ReportSQLError(m_GameDB.GetError());
		return false;
	}

	if (!dbCommand->hasData())
		return false;

	dbCommand->FetchUInt16(1, sCount);
	dbCommand->FetchString(2, strSkillData, sizeof(strSkillData));

	result << sCount;
	result.append(strSkillData, sizeof(strSkillData));

	return true;
}

void CDBAgent::SaveSkillShortcut(short uid, short sCount, char *buff)
{
	_USER_DATA *pUser = GetUser(uid);
	auto dbCommand = m_GameDB.CreateCommand();
	if (dbCommand == NULL || pUser == NULL)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_id, strlen(pUser->m_id));
	dbCommand->AddParameter(SQL_PARAM_INPUT, buff, 260);

	if (!dbCommand->Prepare(string_format(_T("{CALL SKILL_SHORTCUT_SAVE(?, %d, ?)}"), sCount)))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	delete dbCommand;
}

void CDBAgent::RequestFriendList(short uid, vector<string> & friendList)
{
	_USER_DATA *pUser = GetUser(uid);
	if (pUser == NULL)
		return;

	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_id, strlen(pUser->m_id));
	if (!dbCommand->Prepare(_T("SELECT * FROM FRIEND_LIST WHERE strUserID=?")))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	if (!dbCommand->hasData())
		return;

	for (int i = 2; i <= 25; i++)
	{
		string strCharID = dbCommand->FetchString(i);
		if (strCharID.length() > 0)
			friendList.push_back(strCharID);
	}
}

FriendAddResult CDBAgent::AddFriend(short sid, short tid)
{
	_USER_DATA *pSrcUser = m_UserDataArray[sid], *pTargetUser = m_UserDataArray[tid];
	if (pSrcUser == NULL || pTargetUser == NULL)
		return FRIEND_ADD_ERROR;

	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return FRIEND_ADD_ERROR;

	int16 nRet = (int16)FRIEND_ADD_ERROR;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pSrcUser->m_id, strlen(pSrcUser->m_id));
	dbCommand->AddParameter(SQL_PARAM_INPUT, pTargetUser->m_id, strlen(pTargetUser->m_id));
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Prepare(_T("{CALL INSERT_FRIEND_LIST(?, ?, ?)")))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	if (nRet < 0 || nRet >= FRIEND_ADD_MAX)
		nRet = FRIEND_ADD_ERROR;
		
	return (FriendAddResult)nRet;
}

FriendRemoveResult CDBAgent::RemoveFriend(short sid, string & strCharID)
{
	_USER_DATA *pUser = m_UserDataArray[sid];
	if (pUser == NULL)
		return FRIEND_REMOVE_ERROR;

	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return FRIEND_REMOVE_ERROR;

	int16 nRet = (int16)FRIEND_REMOVE_ERROR;

	dbCommand->AddParameter(SQL_PARAM_INPUT, pUser->m_id, strlen(pUser->m_id));
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Prepare(_T("{CALL DELETE_FRIEND_LIST(?, ?, ?)")))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	if (nRet < 0 || nRet >= FRIEND_REMOVE_MAX)
		nRet = FRIEND_REMOVE_MAX;

	return (FriendRemoveResult)nRet;
}

bool CDBAgent::UpdateUser(string & strCharID, short uid, UserUpdateType type)
{
	_USER_DATA* pUser = GetUser(uid);
	if (pUser == NULL || strCharID != pUser->m_id)
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
	for (int i = 0; i < HAVE_MAX+SLOT_MAX; i++)
	{
		_ITEM_DATA *pItem = &pUser->m_sItemArray[i];
		itemBuffer << pItem->nNum << pItem->sDuration << pItem->sCount;
		serialBuffer << pItem->nSerialNum;
	}

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)pUser->m_bstrSkill, sizeof(pUser->m_bstrSkill));
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)itemBuffer.contents(), itemBuffer.size());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)serialBuffer.contents(), serialBuffer.size());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)pUser->m_bstrQuest, sizeof(pUser->m_bstrQuest));

	if (!dbCommand->Prepare(string_format(_T("{CALL UPDATE_USER_DATA (?,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,?,?,?,?,%d,%d)}"),
		pUser->m_bNation, pUser->m_bRace, pUser->m_sClass, pUser->m_nHair, pUser->m_bRank, pUser->m_bTitle, pUser->m_bLevel, pUser->m_iExp, pUser->m_iLoyalty, pUser->m_bFace, 
		pUser->m_bCity,	pUser->m_bKnights, pUser->m_bFame, pUser->m_sHp, pUser->m_sMp, pUser->m_sSp, pUser->m_bStr, pUser->m_bSta, pUser->m_bDex, pUser->m_bIntel, pUser->m_bCha, 
		pUser->m_bAuthority, pUser->m_sPoints, pUser->m_iGold, pUser->m_bZone, pUser->m_sBind, (int)(pUser->m_curx*100), (int)(pUser->m_curz*100), (int)(pUser->m_cury*100), pUser->m_dwTime,
		pUser->m_sQuestCount, pUser->m_iMannerPoint, pUser->m_iLoyaltyMonthly)))
	{
		m_pMain->ReportSQLError(m_GameDB.GetError());
		return false;
	}

	return true;
}

bool CDBAgent::UpdateWarehouseData(string & strAccountID, short uid, UserUpdateType type)
{
	_USER_DATA* pUser = GetUser(uid);
	if (pUser == NULL
		|| strAccountID.length() == 0)
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
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)itemBuffer.contents(), itemBuffer.size());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)serialBuffer.contents(), serialBuffer.size());

	if (!dbCommand->Prepare(string_format(_T("{CALL UPDATE_WAREHOUSE(?,%d,%d,?,?)}"), pUser->m_iBank, pUser->m_dwTime)))
	{
		m_pMain->ReportSQLError(m_GameDB.GetError());
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

	if (!dbCommand->Prepare(string_format(_T("{call CREATE_KNIGHTS(?,%d,%d,%d,?,?)}"), sClanID, bNation, bFlag)))
		m_pMain->ReportSQLError(m_GameDB.GetError());

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

	if (!dbCommand->Prepare(string_format(_T("{CALL UPDATE_KNIGHTS(?,%d,?,%d,%d)"), bType, sClanID, bDomination)))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	return nRet;
}

int CDBAgent::DeleteKnights(uint16 sClanID)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	int16 nRet = -1;

	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);
	if (!dbCommand->Prepare(string_format(_T("{call DELETE_KNIGHTS (?,%d)}"), sClanID)))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	return nRet;
}

uint16 CDBAgent::LoadKnightsAllMembers(uint16 sClanID, Packet & result)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return 0;

	if (!dbCommand->Execute(string_format(_T("{CALL LOAD_KNIGHTS_MEMBERS(%d)}"), sClanID)))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	if (!dbCommand->hasData())
		return 0;

	uint16 count = 0;
	do
	{
		uint16 sClass; uint8 bFame, bLevel;
		string strCharID = dbCommand->FetchString(1);
		dbCommand->FetchByte(2, bFame);
		dbCommand->FetchByte(3, bLevel);
		dbCommand->FetchUInt16(4, sClass);

		rtrim(strCharID);

		short sid;
		result << strCharID << bFame << bLevel << sClass 
			// check if user's logged in (i.e. grab logged in state)
			<< uint8(m_pMain->GetUserPtr(strCharID.c_str(), sid) == NULL ? 0 : 1);
		count++;
	} while (dbCommand->MoveNext());

	return count;
}

void CDBAgent::LoadKnightsInfo(uint16 sClanID, Packet & result)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	if (!dbCommand->Execute(string_format(_T("SELECT IDNum, Nation, IDName, Members, Points FROM KNIGHTS WHERE IDNum=%d" ), sClanID)))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	if (!dbCommand->hasData())
		return;

	uint32 nPoints; uint16 sMembers; uint8 bNation;
	dbCommand->FetchByte(2, bNation); // clan ID is first, but we already know that
	string strKnightsName = dbCommand->FetchString(3);
	dbCommand->FetchUInt16(4, sMembers);
	dbCommand->FetchUInt32(5, nPoints);

	rtrim(strKnightsName);
	result << sClanID << bNation << strKnightsName << sMembers << nPoints;
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
		szSQL = string_format(_T("SELECT IDNum, Points, Ranking FROM KNIGHTS WHERE Nation=%d, AND Points != 0 ORDER BY Points DESC"), bNation); 

	if (!dbCommand->Execute(szSQL))
		m_pMain->ReportSQLError(m_GameDB.GetError());

	if (!dbCommand->hasData())
		return;

	Packet result(WIZ_KNIGHTS_PROCESS);
	result << uint16(-1); // no uid

	bool bReset = true;
	uint8 bCount = 0;
	int offset;

	do
	{
		// If we're resetting/restarting the packet...
		if (bReset)
		{
			// and we actually have data in the packet (i.e. we're not throwing data in for the first time)
			if (bCount > 0)
			{
				// clear our the buffer + reset the count
				result.clear();
				bCount = 0;
			}

			// setup the start of the packet + store the offset the count's at
			result << uint8(KNIGHTS_ALLLIST_REQ) << uint16(-1) << uint8(0);
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
			m_pMain->m_LoggerSendQueue.PutData(&result);

			// mark it for reset, if there's any more rows.
			bReset = true;
		}
	} while (dbCommand->MoveNext());

	// didn't quite make it in the last batch (if any)? send the remainder.
	if (bCount < 40)
	{
		result.put(offset, bCount);
		m_pMain->m_LoggerSendQueue.PutData(&result);
	}
}

void CDBAgent::UpdateBattleEvent(string & strCharID, uint8 bNation)
{
	auto_ptr<OdbcCommand> dbCommand(m_GameDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strCharID.c_str(), strCharID.length());
	if (!dbCommand->Prepare(string_format(_T("UPDATE BATTLE SET byNation=%d, strUserName=? WHERE sIndex=%d"), bNation, m_pMain->m_nServerNo)))
		m_pMain->ReportSQLError(m_GameDB.GetError());
}

void CDBAgent::AccountLogout(string & strAccountID)
{
	auto_ptr<OdbcCommand> dbCommand(m_AccountDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	int16 nRet = 0;
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strAccountID.c_str(), strAccountID.length());
	dbCommand->AddParameter(SQL_PARAM_OUTPUT, &nRet);

	if (!dbCommand->Prepare(_T("{CALL ACCOUNT_LOGOUT(?, ?)}")))
		m_pMain->ReportSQLError(m_AccountDB.GetError());
}

void CDBAgent::UpdateConCurrentUserCount(int nServerNo, int nZoneNo, int nCount)
{
	auto_ptr<OdbcCommand> dbCommand(m_AccountDB.CreateCommand());
	if (dbCommand.get() == NULL)
		return;

	if (!dbCommand->Execute(string_format(_T("UPDATE CONCURRENT SET zone%d_count = %d WHERE serverid = %d"), nZoneNo, nCount, nServerNo)))
		m_pMain->ReportSQLError(m_AccountDB.GetError());
}