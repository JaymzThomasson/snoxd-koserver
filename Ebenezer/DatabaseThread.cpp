#include "StdAfx.h"

std::queue<Packet *> DatabaseThread::_queue;
bool DatabaseThread::_running = true;
FastMutex DatabaseThread::_lock;
HANDLE DatabaseThread::s_hEvent;
HANDLE *DatabaseThread::s_hThreads = NULL;
DWORD DatabaseThread::s_dwThreads = 0;

void DatabaseThread::Startup(DWORD dwThreads)
{
	DWORD id;
	s_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	s_hThreads = new HANDLE[dwThreads];
	for (DWORD i = 0; i < dwThreads; i++)
		s_hThreads[i] = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)&ThreadProc, (LPVOID)i, NULL, &id);
	s_dwThreads = dwThreads;
}

void DatabaseThread::AddRequest(Packet * pkt)
{
	_lock.Acquire();
	_queue.push(pkt);
	_lock.Release();
	SetEvent(s_hEvent);
}

BOOL WINAPI DatabaseThread::ThreadProc(LPVOID lpParam)
{
	while (_running)
	{
		Packet *p = NULL;

		// Pull the next packet from the shared queue
		_lock.Acquire();
		if (_queue.size())
		{
			p = _queue.front();
			_queue.pop();
		}
		_lock.Release();

		// If there's no more packets to handle, wait until there are.
		if (p == NULL)
		{
			WaitForSingleObject(s_hEvent, INFINITE);
			continue;
		}

		// References are fun =p
		Packet & pkt = *p;

		// First 2 bytes are always going to be the socket ID
		// or -1 for no user.
		int16 uid = pkt.read<int16>();

		// Attempt to lookup the user if necessary
		CUser *pUser = NULL;
		if (uid >= 0)
		{
			pUser = g_pMain.GetUserPtr(uid);

			// Check to make sure they're still connected.
			if (pUser == NULL)
				continue;
		}

		switch (pkt.GetOpcode())
		{
		case WIZ_LOGIN:
			if (pUser) pUser->ReqAccountLogIn(pkt);
			break;
		case WIZ_SEL_NATION:
			if (pUser) pUser->ReqSelectNation(pkt);
			break;
		case WIZ_ALLCHAR_INFO_REQ:
			if (pUser) pUser->ReqAllCharInfo(pkt);
			break;
		case WIZ_CHANGE_HAIR:
			if (pUser) pUser->ReqChangeHair(pkt);
			break;
		case WIZ_NEW_CHAR:
			if (pUser) pUser->ReqCreateNewChar(pkt);
			break;
		case WIZ_DEL_CHAR:
			if (pUser) pUser->ReqDeleteChar(pkt);
			break;
		case WIZ_SEL_CHAR:
			if (pUser) pUser->ReqSelectCharacter(pkt);
			break;
		case WIZ_DATASAVE:
			if (pUser) pUser->ReqSaveCharacter();
			break;
		case WIZ_KNIGHTS_PROCESS:
			g_pMain.m_KnightsManager.ReqKnightsPacket(pUser, pkt);
			break;
		case WIZ_LOGIN_INFO:
			if (pUser) pUser->ReqSetLogInInfo(pkt);
			break;
		case WIZ_BATTLE_EVENT:
			// g_pMain.BattleEventResult(pkt);
			break;
		case WIZ_SHOPPING_MALL:
			if (pUser) pUser->ReqShoppingMall(pkt);
			break;
		case WIZ_SKILLDATA:
			if (pUser) pUser->ReqSkillDataProcess(pkt);
			break;
		case WIZ_FRIEND_PROCESS:
			if (pUser) pUser->ReqFriendProcess(pkt);
			break;
		case WIZ_CAPE:
			if (pUser) pUser->ReqChangeCape(pkt);
			break;
		case WIZ_LOGOUT:
			if (pUser) pUser->ReqUserLogOut();
			break;
		}
		// Free the packet.
		delete p;
	}

	TRACE("[Thread %d] Exiting...\n", lpParam);
	return TRUE;
}


void CUser::ReqAccountLogIn(Packet & pkt)
{
	string strPasswd;
	pkt >> strPasswd;

	int8 nation = g_DBAgent.AccountLogin(m_strAccountID, strPasswd);

	// TO-DO: Clean up this account name nonsense
	if (nation >= 0)
	{
		g_pMain.AddAccountName(this);
	}
	else
	{
		m_strAccountID.clear();
	}

	Packet result(WIZ_LOGIN);
	result << nation;
	Send(&result);
}

void CUser::ReqSelectNation(Packet & pkt)
{
	Packet result(WIZ_SEL_NATION);
	uint8 bNation = pkt.read<uint8>(), bResult;

	bResult = g_DBAgent.NationSelect(m_strAccountID, bNation) ? bNation : 0;
	result << bResult;
	Send(&result);
}

void CUser::ReqAllCharInfo(Packet & pkt)
{
	Packet result(WIZ_ALLCHAR_INFO_REQ);
	string strCharID1, strCharID2, strCharID3;

	result << uint8(1);
#if __VERSION >= 1920
	result << uint8(1); // 1.920+ flag, probably indicates whether there's any characters or not (stays 1 for 1+ characters though, so not a count :'(). Untested without.
#endif
	g_DBAgent.GetAllCharID(m_strAccountID, strCharID1, strCharID2, strCharID3);
	g_DBAgent.LoadCharInfo(strCharID1, result);
	g_DBAgent.LoadCharInfo(strCharID2, result);
	g_DBAgent.LoadCharInfo(strCharID3, result);

	Send(&result);
}

void CUser::ReqChangeHair(Packet & pkt)
{
	Packet result(WIZ_CHANGE_HAIR);
	string strUserID;
	uint32 nHair;
	uint8 bOpcode, bFace;
	pkt.SByte();
	pkt >> bOpcode >> strUserID >> bFace >> nHair;
	pkt.put(2, g_DBAgent.ChangeHair(m_strAccountID, strUserID, bOpcode, bFace, nHair));
	Send(&result);
}

void CUser::ReqCreateNewChar(Packet & pkt)
{
	string strCharID;
	uint32 nHair;
	uint16 sClass;
	uint8 bCharIndex, bRace, bFace, bStr, bSta, bDex, bInt, bCha;
	pkt >> bCharIndex >> strCharID >> bRace >> sClass >> bFace >> nHair >> bStr >> bSta >> bDex >> bInt >> bCha;

	Packet result(WIZ_NEW_CHAR);
	result << g_DBAgent.CreateNewChar(m_strAccountID, bCharIndex, strCharID, bRace, sClass, nHair, bFace, bStr, bSta, bDex, bInt, bCha);

	// Starter items. This needs fixing eventually.
	int emptySlot = GetEmptySlot();
	assert(emptySlot >= 0);

	_ITEM_DATA *pItem = GetItem(emptySlot);
	switch (m_sClass)
	{
	case 101:
		pItem->nNum = 120010000;
		pItem->sDuration = 5000;
		break;
	case 102:
		pItem->nNum = 110010000;
		pItem->sDuration = 4000;
		break;
	case 103:
		pItem->nNum = 180010000;
		pItem->sDuration = 5000;
		break;
	case 104:
		pItem->nNum = 190010000;
		pItem->sDuration = 10000;
		break;
	case 201:
		pItem->nNum = 120050000;
		pItem->sDuration = 5000;
		break;
	case 202:
		pItem->nNum = 110050000;
		pItem->sDuration = 4000;
		break;
	case 203:
		pItem->nNum = 180050000;
		pItem->sDuration = 5000;
		break;
	case 204:
		pItem->nNum = 190050000;
		pItem->sDuration = 10000;
		break;
	}

	pItem->sCount = 1;
	pItem->nSerialNum = g_pMain.GenerateItemSerial();

	Send(&result);
}

void CUser::ReqDeleteChar(Packet & pkt)
{
	string strCharID, strSocNo;
	uint8 bCharIndex;
	pkt >> bCharIndex >> strCharID >> strSocNo;

	Packet result(WIZ_DEL_CHAR);
	int8 retCode = g_DBAgent.DeleteChar(m_strAccountID, bCharIndex, strCharID, strSocNo);
	result << retCode << uint8(retCode ? bCharIndex : -1);
	Send(&result);

#if 0
	if (retCode == 1 && sKnights != 0)
	{
		// TO-DO: Synchronise this system better. Much better. This is dumb.
		g_pMain.m_KnightsManager.RemoveKnightsUser(sKnights, (char *)strCharID.c_str());
		result.SetOpcode(UDP_KNIGHTS_PROCESS);
		result << uint8(KNIGHTS_WITHDRAW) << sKnights << strCharID;
		g_pMain.Send_UDP_All(&result, g_pMain.m_nServerGroup == 0 ? 0 : 1);
	}
#endif
}

void CUser::ReqSelectCharacter(Packet & pkt)
{
	Packet result(WIZ_SEL_CHAR);
	uint8 bInit;
	string strCharID;

	pkt >> strCharID >> bInit;
	if (m_strAccountID.empty() || strCharID.empty()
		|| !g_DBAgent.LoadUserData(m_strAccountID, strCharID, this)
		|| !g_DBAgent.LoadWarehouseData(m_strAccountID, this)
		|| !g_DBAgent.LoadPremiumServiceUser(m_strAccountID, this)
		|| !g_DBAgent.LoadSavedMagic(this))
	{
		result << uint8(0);
	}
	else
	{
		result << uint8(1) << bInit;
	}

	SelectCharacter(result); 
}

void CUser::ReqShoppingMall(Packet & pkt)
{
	switch (pkt.read<uint8>())
	{
	case STORE_CLOSE:
		ReqLoadWebItemMall(pkt);
		break;
	case STORE_LETTER:
		ReqLetterSystem(pkt);
		break;
	}
}

void CUser::ReqLoadWebItemMall(Packet & pkt)
{
	Packet result(WIZ_SHOPPING_MALL, uint8(STORE_CLOSE));

	int offset = result.wpos(); // preserve offset
	result << uint8(0);

	if (g_DBAgent.LoadWebItemMall(result, this))
		result.put(offset, uint8(1));

	RecvStore(pkt); // TO-DO: Just send the data directly.
}

void CUser::ReqSkillDataProcess(Packet & pkt)
{
	uint8 opcode = pkt.read<uint8>();
	if (opcode == SKILL_DATA_LOAD)
		ReqSkillDataLoad(pkt);
	else if (opcode == SKILL_DATA_SAVE)
		ReqSkillDataSave(pkt);
}

void CUser::ReqSkillDataLoad(Packet & pkt)
{
	Packet result(WIZ_SKILLDATA, uint8(SKILL_DATA_LOAD));
	if (!g_DBAgent.LoadSkillShortcut(result, this))
		result << uint16(0);

	Send(&result);
}

void CUser::ReqSkillDataSave(Packet & pkt)
{
	char buff[260];
	short sCount;

	// Initialize our buffer (not all skills are likely to be saved, we need to store the entire 260 bytes).
	memset(buff, 0x00, sizeof(buff));

	// Read in our skill count
	pkt >> sCount;

	// Make sure we're not going to copy too much (each skill is 1 uint32).
	if ((sCount * sizeof(uint32)) > sizeof(buff))
		return;

	// Copy the skill data directly in from where we left off reading in the packet buffer
	memcpy(buff, (char *)(pkt.contents() + pkt.rpos()), sCount * sizeof(uint32));

	// Finally, save the skill data.
	g_DBAgent.SaveSkillShortcut(sCount, buff, this);
}

void CUser::ReqFriendProcess(Packet & pkt)
{
	switch (pkt.read<uint8>())
	{
	case FRIEND_REQUEST:
		ReqRequestFriendList(pkt);
		break;

	case FRIEND_ADD:
		ReqAddFriend(pkt);
		break;

	case FRIEND_REMOVE:
		ReqRemoveFriend(pkt);
		break;
	}
}

void CUser::ReqRequestFriendList(Packet & pkt)
{
	Packet result(WIZ_FRIEND_PROCESS);
	std::vector<string> friendList;

	g_DBAgent.RequestFriendList(friendList, this);

	result << uint16(friendList.size());
	foreach (itr, friendList)
		result << (*itr);

	FriendReport(pkt);
}

void CUser::ReqAddFriend(Packet & pkt)
{
	Packet result(WIZ_FRIEND_PROCESS);
	string strCharID;
	int16 tid;

	pkt.SByte();
	pkt >> tid >> strCharID;

	FriendAddResult resultCode = g_DBAgent.AddFriend(GetSocketID(), tid);
	result.SByte();
	result << tid << uint8(resultCode) << strCharID;

	RecvFriendModify(result, FRIEND_ADD);
}

void CUser::ReqRemoveFriend(Packet & pkt)
{
	Packet result(WIZ_FRIEND_PROCESS);
	string strCharID;

	pkt.SByte();
	pkt >> strCharID;

	FriendRemoveResult resultCode = g_DBAgent.RemoveFriend(strCharID, this);
	result.SByte();
	result << uint8(resultCode) << strCharID;

	RecvFriendModify(result, FRIEND_REMOVE);
}

void CUser::ReqChangeCape(Packet & pkt)
{
	uint16 sClanID, sCapeID;
	uint8 r, g, b;
	pkt >> sClanID >> sCapeID >> r >> g >> b;

	g_DBAgent.UpdateCape(sClanID, sCapeID, r, g, b);
}

void CUser::ReqUserLogOut()
{
	string strCharID = GetName();

	g_DBAgent.UpdateUser(strCharID, UPDATE_LOGOUT, this);
	g_DBAgent.UpdateWarehouseData(m_strAccountID, UPDATE_LOGOUT, this);
	
	if (m_bLogout != 2)	// zone change logout
		g_DBAgent.AccountLogout(m_strAccountID);

	// this session can be used again.
	m_deleted = false;
}

#if 0
void CUser::ReqConCurrentUserCount()
{
	uint32 count = g_pMain.GetActiveSessionMap().size();
	s_socketMgr.ReleaseLock();
	m_DBAgent.UpdateConCurrentUserCount(m_nServerNo, m_nZoneNo, count);
}
#endif

void CUser::ReqSaveCharacter()
{
	std::string strUserID = GetName();
	g_DBAgent.UpdateUser(strUserID, UPDATE_PACKET_SAVE, this);
	g_DBAgent.UpdateWarehouseData(m_strAccountID, UPDATE_PACKET_SAVE, this);
}

void CKnightsManager::ReqKnightsPacket(CUser* pUser, Packet & pkt)
{
	uint8 opcode;
	pkt >> opcode;
	switch (opcode)
	{
	case KNIGHTS_CREATE:
		ReqCreateKnights(pUser, pkt);
		break;
	case KNIGHTS_JOIN:
	case KNIGHTS_WITHDRAW:
		ReqUpdateKnights(pUser, pkt, opcode);
		break;
	case KNIGHTS_REMOVE:
	case KNIGHTS_ADMIT:
	case KNIGHTS_REJECT:
	case KNIGHTS_CHIEF:
	case KNIGHTS_VICECHIEF:
	case KNIGHTS_OFFICER:
	case KNIGHTS_PUNISH:
		ReqModifyKnightsMember(pUser, pkt, opcode);
		break;
	case KNIGHTS_DESTROY:
		ReqDestroyKnights(pUser, pkt);
		break;
	case KNIGHTS_MEMBER_REQ:
		ReqAllKnightsMember(pUser, pkt);
		break;
	case KNIGHTS_LIST_REQ:
		ReqKnightsList(pkt);
		break;
	case KNIGHTS_ALLLIST_REQ:
		g_DBAgent.LoadKnightsAllList(pkt.read<uint8>()); // read nation
		break;
	case KNIGHTS_MARK_REGISTER:
		ReqRegisterClanSymbol(pUser, pkt);
		break;
	}
}

void CKnightsManager::ReqCreateKnights(CUser *pUser, Packet & pkt)
{
	if (pUser == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_CREATE));
	string strKnightsName, strChief;
	uint16 sClanID;
	uint8 bFlag, bNation;
	int8 bResult;

	pkt >> bFlag >> sClanID >> bNation >> strKnightsName >> strChief;
	bResult = g_DBAgent.CreateKnights(sClanID, bNation, strKnightsName, strChief, bFlag);

	if (bResult < 0)
	{
		result << bResult;
		pUser->Send(&result);
		return;
	}

	CKnights *pKnights = new CKnights();

	pKnights->m_sIndex = sClanID;
	pKnights->m_byFlag = bFlag;
	pKnights->m_byNation = bNation;

	strcpy(pKnights->m_strName, strKnightsName.c_str());
	strcpy(pKnights->m_strChief, pUser->GetName());

	pUser->m_iGold -= CLAN_COIN_REQUIREMENT;
	pUser->m_bFame = CHIEF;

	// TO-DO: Make this threadsafe
	g_pMain.m_KnightsArray.PutData(pKnights->m_sIndex, pKnights);

	pKnights->AddUser(pUser);

	result	<< uint8(1) << pUser->GetSocketID() 
			<< sClanID << strKnightsName
			<< pKnights->m_byGrade << pKnights->m_byRanking
			<< pUser->m_iGold;

	pUser->SendToRegion(&result);

	result.Initialize(UDP_KNIGHTS_PROCESS);
	result	<< uint8(KNIGHTS_CREATE)
			<< pKnights->m_byFlag << sClanID 
			<< bNation << strKnightsName << pUser->GetName();
	g_pMain.Send_UDP_All(&result, g_pMain.m_nServerGroup == 0 ? 0 : 1);
}

void CKnightsManager::ReqUpdateKnights(CUser *pUser, Packet & pkt, uint8 opcode)
{
	if (pUser == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS);
	uint16 sClanID = pkt.read<uint16>();
	string strCharID = pUser->GetName();
	int8 bResult = int8(g_DBAgent.UpdateKnights(opcode, strCharID, sClanID, 0));
	if (bResult < 0)
	{
		result << opcode << uint8(0);
		pUser->Send(&result);
		return;
	}

	result << sClanID;  // Hate doing this, but it's reusable.
	RecvUpdateKnights(pUser, pkt, opcode);
}

void CKnightsManager::ReqModifyKnightsMember(CUser *pUser, Packet & pkt, uint8 command)
{
	if (pUser == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS);
	string strCharID;
	uint16 sClanID;
	int8 bRemoveFlag, bResult;

	pkt >> sClanID >> strCharID >> bRemoveFlag;
	bResult = int8(g_DBAgent.UpdateKnights(command, strCharID, sClanID, bRemoveFlag));

	if (bResult < 0)
	{
		result << command << uint8(0);
		pUser->Send(&result);
		return;
	}

	result << sClanID << strCharID; // I really hate doing this, but OK...
	RecvModifyFame(pUser, pkt, command);
}

void CKnightsManager::ReqDestroyKnights(CUser *pUser, Packet & pkt)
{
	uint16 sClanID = pkt.read<uint16>();
	CKnights *pKnights = g_pMain.GetClanPtr(sClanID);
	if (pKnights == NULL)
		return;

	int8 bResult = int8(g_DBAgent.DeleteKnights(sClanID));
	pKnights->Disband(pUser);

	Packet result(UDP_KNIGHTS_PROCESS, uint8(KNIGHTS_DESTROY));
	result << sClanID;
	g_pMain.Send_UDP_All(&result, (g_pMain.m_nServerGroup == 0 ? 0 : 1));
}

void CKnightsManager::ReqAllKnightsMember(CUser *pUser, Packet & pkt)
{
	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_MEMBER_REQ));
	int nOffset;
	uint16 sClanID, sCount;

	pkt >> sClanID;

	CKnights* pKnights = g_pMain.GetClanPtr(pUser->GetClanID());
	if (pKnights == NULL)
		return;

	result << uint8(1);
	nOffset = result.wpos(); // store offset
	result	<< uint16(0) // placeholder for packet length 
			<< uint16(0); // placeholder for user count

	sCount = g_DBAgent.LoadKnightsAllMembers(sClanID, result);
	if (sCount > MAX_CLAN_USERS)
		return;

	pkt.put(nOffset, uint16(result.size() - 3));
	pkt.put(nOffset + 2, sCount);

	pUser->Send(&result);
}

void CKnightsManager::ReqKnightsList(Packet & pkt)
{
	// Okay, this effectively makes this useless in the majority of cases.
	if (g_pMain.m_nServerNo != BATTLE)
		return;

	string strKnightsName; 
	uint32 nPoints; 
	uint16 sClanID = pkt.read<uint16>(), sMembers;
	uint8 bNation, bRank;

	if (!g_DBAgent.LoadKnightsInfo(sClanID, bNation, strKnightsName, sMembers, nPoints, bRank))
		return;

	CKnights *pKnights = g_pMain.GetClanPtr(sClanID);
	if (pKnights == NULL)
	{
		pKnights = new CKnights();

		// TO-DO: Make this threadsafe
		if (!g_pMain.m_KnightsArray.PutData(sClanID, pKnights))
		{
			delete pKnights;
			return;
		}
	}

	// TO-DO: Move this all to a single method, as this is done multiple times
	pKnights->m_sIndex = sClanID;
	pKnights->m_byNation = bNation;
	strcpy(pKnights->m_strName, strKnightsName.c_str());
	pKnights->m_sMembers = sMembers;
	pKnights->m_nPoints = nPoints;
	pKnights->m_byGrade = g_pMain.GetKnightsGrade(nPoints);
	pKnights->m_byRanking = bRank;
}

void CKnightsManager::ReqRegisterClanSymbol(CUser *pUser, Packet & pkt)
{
	if (pUser == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_MARK_REGISTER));
	char clanSymbol[MAX_KNIGHTS_MARK];
	uint16 sClanID, sSymbolSize, sErrorCode = 0, sNewVersion = 0;

	pkt >> sClanID >> sSymbolSize;
	pkt.read(clanSymbol, sSymbolSize);

	bool bResult = g_DBAgent.UpdateClanSymbol(sClanID, sSymbolSize, clanSymbol);

	do
	{
		if (!bResult)
			break;

		CKnights *pKnights = g_pMain.GetClanPtr(sClanID);
		if (pKnights == NULL)
		{
			sErrorCode = 20;
			break;
		}

		// Make sure they still have enough coins.
		if (!pUser->GoldLose(CLAN_SYMBOL_COST))
		{
			sErrorCode = 14;
			break;
		}

		sNewVersion = ++pKnights->m_sMarkVersion;
		pKnights->m_sMarkLen = sSymbolSize;

		memcpy(pKnights->m_Image, clanSymbol, sSymbolSize);

		// TO-DO: Send to all servers for updating via UDP

		sErrorCode = 1;
	} while (0);
	
	result << sErrorCode << sNewVersion;
	pUser->Send(&result);
}

void CUser::ReqSetLogInInfo(Packet & pkt)
{
	string strCharID, strServerIP, strClientIP;
	uint16 sServerNo;
	uint8 bInit;

	pkt >> strCharID >> strServerIP >> sServerNo >> strClientIP >> bInit;
	// if there was an error inserting to CURRENTUSER...
	if (!g_DBAgent.SetLogInInfo(m_strAccountID, strCharID, strServerIP, sServerNo, strClientIP, bInit))
		Disconnect();
}

#if 0
void CUser::ReqBattleEventResult(Packet & pkt)
{
	string strMaxUserName;
	uint8 bType, bNation;

	pkt >> bType >> bNation >> strMaxUserName;
	m_DBAgent.UpdateBattleEvent(strMaxUserName, bNation);
}
#endif

void DatabaseThread::Shutdown()
{
	_running = false;

	if (s_hThreads != NULL)
	{
		// wake them up in case they're sleeping.
		for (DWORD i = 0; i < s_dwThreads; i++)
		{
			SetEvent(s_hEvent); 
			Sleep(10);
		}

		WaitForMultipleObjects(s_dwThreads, s_hThreads, TRUE, INFINITE);
		delete [] s_hThreads;
		s_hThreads = NULL;
	}

	_lock.Acquire();
	while (_queue.size())
	{
		Packet *p = _queue.front();
		_queue.pop();
		delete p;
	}
	_lock.Release();
}
