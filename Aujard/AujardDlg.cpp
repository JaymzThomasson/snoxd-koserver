#include "stdafx.h"
#include <process.h>

static DWORD s_dwProcessCheckID, s_dwConcurrentCheckID;

DWORD WINAPI ReadQueueThread(LPVOID lp)
{
	CAujardDlg* pMain = (CAujardDlg*)lp;
	CString string;
	Packet pkt;

	while (TRUE)
	{
		if (pMain->m_LoggerRecvQueue.GetFrontMode() == R)
		{
			Sleep(1);
			continue;
		}

		int16 uid = -1;
		int recvlen = pMain->m_LoggerRecvQueue.GetData(pkt, &uid);
		if (recvlen > MAX_PKTSIZE || recvlen == 0)
		{
			Sleep(1);
			continue;
		}

		switch (pkt.GetOpcode())
		{
		case WIZ_LOGIN:
			pMain->AccountLogIn(pkt, uid);
			break;
		case WIZ_SEL_NATION:
			pMain->SelectNation(pkt, uid);
			break;
		case WIZ_ALLCHAR_INFO_REQ:
			pMain->AllCharInfoReq(pkt, uid);
			break;
		case WIZ_CHANGE_HAIR:
			pMain->ChangeHairReq(pkt, uid);
			break;
		case WIZ_NEW_CHAR:
			pMain->CreateNewChar(pkt, uid);
			break;
		case WIZ_DEL_CHAR:
			pMain->DeleteChar(pkt, uid);
			break;
		case WIZ_SEL_CHAR:
			pMain->SelectCharacter(pkt, uid);
			break;
		case WIZ_DATASAVE:
			pMain->UserDataSave(pkt, uid);
			break;
		case WIZ_KNIGHTS_PROCESS:
			pMain->KnightsPacket(pkt, uid);
			break;
		case WIZ_LOGIN_INFO:
			pMain->SetLogInInfo(pkt, uid);
			break;
		case WIZ_KICKOUT:
			pMain->UserKickOut(pkt);
			break;
		case WIZ_BATTLE_EVENT:
			pMain->BattleEventResult(pkt);
			break;
		case WIZ_SHOPPING_MALL:
			pMain->ShoppingMall(pkt, uid);
			break;
		case WIZ_SKILLDATA:
			pMain->SkillDataProcess(pkt, uid);
			break;
		case WIZ_FRIEND_PROCESS:
			pMain->FriendProcess(pkt, uid);
			break;
		case WIZ_CAPE:
			pMain->ChangeCape(pkt);
			break;
		case WIZ_LOGOUT:
			pMain->UserLogOut(pkt, uid);				
			break;
		}
	}
}

CAujardDlg::CAujardDlg()
{
	memset(m_strGameDSN, 0x00, sizeof(m_strGameDSN));
	memset(m_strGameUID, 0x00, sizeof(m_strGameUID));
	memset(m_strGamePWD, 0x00, sizeof(m_strGamePWD));
	memset(m_strAccountDSN, 0x00, sizeof(m_strAccountDSN));
	memset(m_strAccountUID, 0x00, sizeof(m_strAccountUID));
	memset(m_strAccountPWD, 0x00, sizeof(m_strAccountPWD));
}

void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	g_pMain.OnTimer(idEvent);
}

bool CAujardDlg::Startup()
{
	m_fp = fopen("./Aujard.log", "a");
	if (m_fp == NULL)
	{
		printf("ERROR: Unable to open log file.\n");
		return false;
	}

	if (!m_LoggerRecvQueue.InitailizeMMF(MAX_PKTSIZE, MAX_COUNT, SMQ_LOGGERSEND, FALSE)
		|| !m_LoggerSendQueue.InitailizeMMF(MAX_PKTSIZE, MAX_COUNT, SMQ_LOGGERRECV, FALSE)
		|| !InitializeMMF())
	{
		printf("ERROR: Unable to initialize shared memory. Ensure Ebenezer is running.\n");
		return false;
	}

	CIni ini("Aujard.ini");

	ini.GetString("ODBC", "ACCOUNT_DSN", "KN_online", m_strAccountDSN, sizeof(m_strAccountDSN));
	ini.GetString("ODBC", "ACCOUNT_UID", "knight", m_strAccountUID, sizeof(m_strAccountUID));
	ini.GetString("ODBC", "ACCOUNT_PWD", "knight", m_strAccountPWD, sizeof(m_strAccountPWD));
	ini.GetString("ODBC", "GAME_DSN", "KN_online", m_strGameDSN, sizeof(m_strGameDSN));
	ini.GetString("ODBC", "GAME_UID", "knight", m_strGameUID, sizeof(m_strGameUID));
	ini.GetString("ODBC", "GAME_PWD", "knight", m_strGamePWD, sizeof(m_strGamePWD));

	m_nServerNo = ini.GetInt("ZONE_INFO", "GROUP_INFO", 1);
	m_nZoneNo = ini.GetInt("ZONE_INFO", "ZONE_INFO", 1);

	if (!m_DBAgent.Connect()
		|| !m_DBAgent.LoadItemTable())
	{
		printf("ERROR: A database error occurred.\n");
		return false;
	}

	s_dwProcessCheckID = SetTimer(NULL, 1, 40000, &TimerProc);
	s_dwConcurrentCheckID = SetTimer(NULL, 2, 300000, &TimerProc);

	DWORD id;
	m_hReadQueueThread = CreateThread(NULL, 0, &ReadQueueThread, (LPVOID)this, 0, &id);

	return true;
}

BOOL CAujardDlg::InitializeMMF()
{
	DWORD filesize = MAX_USER * sizeof(_USER_DATA);
	
	m_hMMFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, "KNIGHT_DB");
	if (m_hMMFile == NULL)
	{
		m_hMMFile = INVALID_HANDLE_VALUE; 
		return false;
	}

    m_lpMMFile = (char *)MapViewOfFile(m_hMMFile, FILE_MAP_WRITE, 0, 0, 0);
	if (!m_lpMMFile)
		return FALSE;

	m_DBAgent.m_UserDataArray.reserve( MAX_USER );

	for (int i = 0; i < MAX_USER; i++)
		m_DBAgent.m_UserDataArray.push_back((_USER_DATA*)(m_lpMMFile + i * sizeof(_USER_DATA)));

	printf("Shared memory mapped successfully.\n");
	return true;
}

void CAujardDlg::ReportSQLError(OdbcError *pError)
{
	if (pError == NULL)
		return;

	// This is *very* temporary.
	string errorMessage = string_format(_T("ODBC error occurred.\r\nSource: %s\r\nError: %s\r\nDescription: %s\n"),
		pError->Source.c_str(), pError->ExtendedErrorMessage.c_str(), pError->ErrorMessage.c_str());

	WriteLogFile(errorMessage);
	delete pError;
}

void CAujardDlg::WriteLogFile(std::string & logMessage)
{
	m_lock.Acquire();
	fwrite(logMessage.c_str(), logMessage.length(), 1, m_fp);
	fflush(m_fp);
	m_lock.Release();
}

void CAujardDlg::AccountLogIn(Packet & pkt, int16 uid)
{
	string strAccountID, strPasswd;
	pkt >> strAccountID >> strPasswd;
	uint8 nation = m_DBAgent.AccountLogin(strAccountID, strPasswd);

	_USER_DATA *pUser = m_DBAgent.GetUser(uid);
	if (pUser != NULL && nation != 0)
		_tstrcpy(pUser->m_Accountid, strAccountID);

	Packet result(WIZ_LOGIN, nation);
	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::SelectNation(Packet & pkt, int16 uid)
{
	string strAccountID;
	uint8 bNation;
	pkt >> strAccountID >> bNation;

	Packet result(WIZ_SEL_NATION);
	uint8 bResult = m_DBAgent.NationSelect(strAccountID, bNation) ? bNation : 0;
	result << bResult;
	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::AllCharInfoReq(Packet & pkt, int16 uid)
{
	Packet result(WIZ_ALLCHAR_INFO_REQ);
	ByteBuffer tmp;
	string strAccountID, strCharID1, strCharID2, strCharID3;

	pkt >> strAccountID;
	tmp << uint8(1) 
		/*<< uint8(1)*/; // 1.920+ flag, probably indicates whether there's any characters or not (stays 1 for 1+ characters though, so not a count :'(). Untested without.
	m_DBAgent.GetAllCharID(strAccountID, strCharID1, strCharID2, strCharID3);
	m_DBAgent.LoadCharInfo(strCharID1, tmp);
	m_DBAgent.LoadCharInfo(strCharID2, tmp);
	m_DBAgent.LoadCharInfo(strCharID3, tmp);
	result << uint16(tmp.size()) << tmp;

	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::ChangeHairReq(Packet & pkt, int16 uid)
{
	Packet result(WIZ_CHANGE_HAIR);
	string strAccountID, strUserID;
	uint32 nHair;
	uint8 bOpcode, bFace;
	pkt >> bOpcode >> strAccountID >> strUserID >> bFace >> nHair;
	pkt.put(2, m_DBAgent.ChangeHair(strAccountID, strUserID, bOpcode, bFace, nHair));
	m_LoggerSendQueue.PutData(&pkt, uid);
}

void CAujardDlg::CreateNewChar(Packet & pkt, int16 uid)
{
	string strAccountID, strCharID;
	uint32 nHair;
	uint16 sClass;
	uint8 bCharIndex, bRace, bFace, bStr, bSta, bDex, bInt, bCha;
	pkt >> strAccountID >> bCharIndex >> strCharID >> bRace >> sClass >> bFace >> nHair >> bStr >> bSta >> bDex >> bInt >> bCha;

	Packet result(WIZ_NEW_CHAR);
	result << m_DBAgent.CreateNewChar(strAccountID, bCharIndex, strCharID, bRace, sClass, nHair, bFace, bStr, bSta, bDex, bInt, bCha);

	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::DeleteChar(Packet & pkt, int16 uid)
{
	string strAccountID, strCharID, strSocNo;
	uint8 bCharIndex;
	pkt >> strAccountID >> bCharIndex >> strCharID >> strSocNo;

	Packet result(WIZ_DEL_CHAR);
	int8 retCode = m_DBAgent.DeleteChar(strAccountID, bCharIndex, strCharID, strSocNo);
	result << retCode << uint8(retCode ? bCharIndex : -1);
	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::SelectCharacter(Packet & pkt, int16 uid)
{
	Packet result(WIZ_SEL_CHAR);
	_USER_DATA *pUser = NULL;
	uint8 bInit;
	string strAccountID, strCharID;

	pkt >> strAccountID >> strCharID >> bInit;
	if (strAccountID.empty() || strCharID.empty()
		|| strAccountID.length() > MAX_ID_SIZE || strCharID.length() > MAX_ID_SIZE
		|| (pUser = m_DBAgent.GetUser(uid)) == NULL
		|| !m_DBAgent.LoadUserData(strAccountID, strCharID, pUser)
		|| !m_DBAgent.LoadWarehouseData(strAccountID, pUser)
		|| !m_DBAgent.LoadPremiumServiceUser(strAccountID, pUser))
	{
		result << uint8(0);
	}
	else
	{
		result << uint8(1) << bInit;
	}

	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::ShoppingMall(Packet & pkt, int16 uid)
{
	switch (pkt.read<uint8>())
	{
	case STORE_CLOSE:
		LoadWebItemMall(pkt, uid);
		break;
	}
}

void CAujardDlg::LoadWebItemMall(Packet & pkt, int16 uid)
{
	Packet result(WIZ_SHOPPING_MALL, uint8(STORE_CLOSE));

	int offset = result.wpos(); // preserve offset
	result << uint8(0);

	if (m_DBAgent.LoadWebItemMall(uid, result))
		result.put(offset, uint8(1));

	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::SkillDataProcess(Packet & pkt, int16 uid)
{
	uint8 opcode = pkt.read<uint8>();
	if (opcode == SKILL_DATA_LOAD)
		SkillDataLoad(pkt, uid);
	else if (opcode == SKILL_DATA_SAVE)
		SkillDataSave(pkt, uid);
}

void CAujardDlg::SkillDataLoad(Packet & pkt, int16 uid)
{
	Packet result(WIZ_SKILLDATA, uint8(SKILL_DATA_LOAD));
	if (!m_DBAgent.LoadSkillShortcut(uid, result))
		result << uint8(0);

	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::SkillDataSave(Packet & pkt, int16 uid)
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
	m_DBAgent.SaveSkillShortcut(uid, sCount, buff);
}

void CAujardDlg::FriendProcess(Packet & pkt, int16 uid)
{
	switch (pkt.read<uint8>())
	{
	case FRIEND_REQUEST:
		RequestFriendList(pkt, uid);
		break;

	case FRIEND_ADD:
		AddFriend(pkt, uid);
		break;

	case FRIEND_REMOVE:
		RemoveFriend(pkt, uid);
		break;
	}
}

void CAujardDlg::RequestFriendList(Packet & pkt, int16 uid)
{
	Packet result(WIZ_FRIEND_PROCESS, uint8(FRIEND_REQUEST));
	vector<string> friendList;

	m_DBAgent.RequestFriendList(uid, friendList);

	result << uint16(friendList.size());
	foreach (itr, friendList)
		result << (*itr);
	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::AddFriend(Packet & pkt, int16 uid)
{
	Packet result(WIZ_FRIEND_PROCESS, uint8(FRIEND_ADD));
	string strCharID;
	int16 tid;

	pkt.SByte();
	pkt >> tid >> strCharID;

	FriendAddResult resultCode = m_DBAgent.AddFriend(uid, tid);
	result.SByte();
	result << tid << uint8(resultCode) << strCharID;
	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::RemoveFriend(Packet & pkt, int16 uid)
{
	Packet result(WIZ_FRIEND_PROCESS, uint8(FRIEND_REMOVE));
	string strCharID;

	pkt.SByte();
	pkt >> strCharID;

	FriendRemoveResult resultCode = m_DBAgent.RemoveFriend(uid, strCharID);
	result.SByte();
	result << uint8(resultCode) << strCharID;
	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::ChangeCape(Packet & pkt)
{
	uint16 sClanID, sCapeID;
	uint8 r, g, b;
	pkt >> sClanID >> sCapeID >> r >> g >> b;

	m_DBAgent.UpdateCape(sClanID, sCapeID, r, g, b);
}

void CAujardDlg::UserLogOut(Packet & pkt, int16 uid)
{
	string strAccountID, strCharID;
	pkt >> strAccountID >> strCharID;

	_USER_DATA *pUser = m_DBAgent.GetUser(uid);
	if (pUser == NULL)
		return;

	m_DBAgent.UpdateUser(strCharID, uid, UPDATE_LOGOUT);
	m_DBAgent.UpdateWarehouseData(strAccountID, uid, UPDATE_LOGOUT);
	
	if (pUser->m_bLogout != 2)	// zone change logout
		m_DBAgent.AccountLogout(strAccountID);
	
	m_DBAgent.MUserInit(uid);

	Packet result(WIZ_LOGOUT);
	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::OnTimer(UINT nIDEvent) 
{
	HANDLE	hProcess = NULL;

	if (nIDEvent == s_dwProcessCheckID)
	{
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_VM_READ, FALSE, m_LoggerSendQueue.GetProcessId());
		if (hProcess == NULL)
			AllSaveRoutine();
		else
			CloseHandle(hProcess);
	}
	else if (nIDEvent == s_dwConcurrentCheckID)
	{
		ConCurrentUserCount();
	}
}

void CAujardDlg::AllSaveRoutine()
{
	int size = m_DBAgent.m_UserDataArray.size();
	for (int i = 0; i < size; i++)
	{
		_USER_DATA *pUser = m_DBAgent.m_UserDataArray[i];
		if (pUser == NULL || *pUser->m_Accountid == 0)
			continue;

		string strAccountID = pUser->m_Accountid;
		string strCharID = pUser->m_id;

		m_DBAgent.UpdateWarehouseData(strAccountID, i, UPDATE_ALL_SAVE);
		m_DBAgent.UpdateUser(strCharID, i, UPDATE_ALL_SAVE);
		Sleep(50);
	}

	printf("All player data saved.\n");
}

void CAujardDlg::ConCurrentUserCount()
{
	int count = 0, total = m_DBAgent.m_UserDataArray.size();
	for (int i = 0; i < total; i++)
	{
		_USER_DATA * pUser = m_DBAgent.m_UserDataArray[i];
		if (pUser == NULL || *pUser->m_id == 0)
			continue;
		
		count++;
	}

	m_DBAgent.UpdateConCurrentUserCount(m_nServerNo, m_nZoneNo, count);
}

void CAujardDlg::UserDataSave(Packet & pkt, int16 uid)
{
	string strAccountID, strCharID;
	pkt >> strAccountID >> strCharID;
	_USER_DATA * pUser = m_DBAgent.GetUser(uid);
	if (pUser == NULL)
		return;

	m_DBAgent.UpdateUser(strCharID, uid, UPDATE_PACKET_SAVE);
	m_DBAgent.UpdateWarehouseData(strAccountID, uid, UPDATE_PACKET_SAVE);
}

_USER_DATA* CAujardDlg::GetUserPtr(const char *struserid, short &uid)
{

	// TO-DO: Replace with hashmap
	for (int i = 0; i < MAX_USER; i++)
	{
		_USER_DATA *pUser = m_DBAgent.m_UserDataArray[i];
		if (pUser == NULL || _strnicmp(struserid, pUser->m_id, MAX_ID_SIZE) != 0)
			continue;

		uid = i;
		return pUser;
	}

	return NULL;
}

void CAujardDlg::KnightsPacket(Packet & pkt, int16 uid)
{
	uint8 opcode;
	pkt >> opcode;
	switch (opcode)
	{
	case KNIGHTS_CREATE:
		CreateKnights(pkt, uid);
		break;
	case KNIGHTS_JOIN:
		JoinKnights(pkt, uid);
		break;
	case KNIGHTS_WITHDRAW:
		WithdrawKnights(pkt, uid);
		break;
	case KNIGHTS_REMOVE:
	case KNIGHTS_ADMIT:
	case KNIGHTS_REJECT:
	case KNIGHTS_CHIEF:
	case KNIGHTS_VICECHIEF:
	case KNIGHTS_OFFICER:
	case KNIGHTS_PUNISH:
		ModifyKnightsMember(pkt, opcode, uid);
		break;
	case KNIGHTS_DESTROY:
		DestroyKnights(pkt, uid);
		break;
	case KNIGHTS_MEMBER_REQ:
		AllKnightsMember(pkt, uid);
		break;
	case KNIGHTS_LIST_REQ:
		KnightsList(pkt, uid);
		break;
	case KNIGHTS_ALLLIST_REQ:
		m_DBAgent.LoadKnightsAllList(pkt.read<uint8>()); // read nation
		break;
	case KNIGHTS_MARK_REGISTER:
		RegisterClanSymbol(pkt, uid);
		break;
	}
}

void CAujardDlg::CreateKnights(Packet & pkt, int16 uid)
{
	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_CREATE));
	string strKnightsName, strChief;
	uint16 sClanID;
	uint8 bCommunity, bNation;

	pkt >> bCommunity >> sClanID >> bNation >> strKnightsName >> strChief;
	result	<< m_DBAgent.CreateKnights(sClanID, bNation, strKnightsName, strChief, bCommunity)
			<< bCommunity << sClanID << bNation << strKnightsName << strChief;

	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::JoinKnights(Packet & pkt, int16 uid)
{
	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_JOIN));
	uint16 sClanID;

	pkt >> sClanID;
	_USER_DATA *pUser = m_DBAgent.GetUser(uid);
	if (pUser == NULL)
		return;

	string strCharID = pUser->m_id;
	result	<< int8(m_DBAgent.UpdateKnights(KNIGHTS_JOIN, strCharID, sClanID, 0))
			<< sClanID;
	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::WithdrawKnights(Packet & pkt, int16 uid)
{
	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_WITHDRAW));
	uint16 sClanID;
	pkt >> sClanID;

	_USER_DATA *pUser = m_DBAgent.GetUser(uid);
	if (pUser == NULL)
		return;

	string strCharID = pUser->m_id;

	result << int8(m_DBAgent.UpdateKnights(KNIGHTS_WITHDRAW, strCharID, sClanID, 0))
		   << sClanID;
	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::ModifyKnightsMember(Packet & pkt, uint8 command, int16 uid)
{
	Packet result(WIZ_KNIGHTS_PROCESS, command);
	string strCharID;
	uint16 sClanID;
	uint8 bRemoveFlag;

	pkt >> sClanID >> strCharID >> bRemoveFlag;

	result	<< command << int8(m_DBAgent.UpdateKnights(command, strCharID, sClanID, bRemoveFlag))
			<< sClanID << strCharID;

	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::DestroyKnights(Packet & pkt, int16 uid)
{
	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_DESTROY));
	uint16 sClanID = pkt.read<uint16>();

	result << int8(m_DBAgent.DeleteKnights(sClanID)) << sClanID;
	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::AllKnightsMember(Packet & pkt, int16 uid)
{
	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_MEMBER_REQ));
	int nOffset;
	uint16 sClanID, sCount;

	pkt >> sClanID;
	result << uint8(0);
	nOffset = result.wpos(); // store offset
	result << uint16(0) << uint16(0); // placeholders
	sCount = m_DBAgent.LoadKnightsAllMembers(sClanID, result);

	pkt.put(nOffset, result.size() - 3);
	pkt.put(nOffset + 2, sCount);

	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::KnightsList(Packet & pkt, int16 uid)
{
	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_LIST_REQ));
	uint16 sClanID = pkt.read<uint16>();

	result << uint8(0);
	m_DBAgent.LoadKnightsInfo(sClanID, result);
	
	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::RegisterClanSymbol(Packet & pkt, int16 uid)
{
	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_MARK_REGISTER));
	char clanSymbol[MAX_KNIGHTS_MARK];
	uint16 sClanID, sSymbolSize;

	pkt >> sClanID >> sSymbolSize;
	pkt.read(clanSymbol, sSymbolSize);

	bool bResult = m_DBAgent.UpdateClanSymbol(sClanID, sSymbolSize, clanSymbol);
	result << uint8(0) << bResult;
	if (bResult)
	{
		result << sClanID << sSymbolSize;
		result.append(clanSymbol, sSymbolSize); // ... and back again! Like ping pong!
	}
	
	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::SetLogInInfo(Packet & pkt, int16 uid)
{
	string strAccountID, strCharID, strServerIP, strClientIP;
	uint16 sServerNo;
	uint8 bInit;

	pkt >> strAccountID >> strCharID >> strServerIP >> sServerNo >> strClientIP >> bInit;
	if (m_DBAgent.SetLogInInfo(strAccountID, strCharID, strServerIP, sServerNo, strClientIP, bInit))
		return;

	// if there was an error inserting to CURRENTUSER...
	Packet result(WIZ_LOGIN_INFO, uint8(0));
	m_LoggerSendQueue.PutData(&result, uid);
}

void CAujardDlg::UserKickOut(Packet & pkt)
{
	string strAccountID;
	pkt >> strAccountID;
	m_DBAgent.AccountLogout(strAccountID);
}

void CAujardDlg::SaveUserData()
{
	int total = m_DBAgent.m_UserDataArray.size();
	for (int i = 0; i < total; i++)
	{
		_USER_DATA *pUser = m_DBAgent.m_UserDataArray[i];
		if (pUser == NULL || *pUser->m_id == 0)
			continue;

		if (GetTickCount() - pUser->m_dwTime > 360000)
		{
			Packet result;
			result << uint16(i) << pUser->m_Accountid << pUser->m_id;
			UserDataSave(result, i);
			Sleep(50);
		}
	}
}

void CAujardDlg::BattleEventResult(Packet & pkt)
{
	string strMaxUserName;
	uint8 bType, bNation;

	pkt >> bType >> bNation >> strMaxUserName;
	m_DBAgent.UpdateBattleEvent(strMaxUserName, bNation);
}

CAujardDlg::~CAujardDlg() 
{
	KillTimer(NULL, s_dwProcessCheckID);
	KillTimer(NULL, s_dwConcurrentCheckID);

	if (m_hReadQueueThread != NULL)
		TerminateThread(m_hReadQueueThread, 0);
	
	if (m_fp != NULL) 
		fclose(m_fp);
}
