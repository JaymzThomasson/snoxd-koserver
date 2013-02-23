// KnightsManager.cpp: implementation of the CKnightsManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "KnightsManager.h"
#include "User.h"
#include "GameDefine.h"
#include "EbenezerDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CKnightsManager::CKnightsManager()
{
}

CKnightsManager::~CKnightsManager()
{

}

// TO-DO: Move this to the CUser class.
void CKnightsManager::PacketProcess(CUser *pUser, Packet & pkt)
{
	if (pUser == NULL)
		return;

	uint8 opcode = pkt.read<uint8>();
	TRACE("Clan packet: %X\n", opcode); 
	switch (opcode)
	{
	case KNIGHTS_CREATE:
		CreateKnights(pUser, pkt);
		break;
	case KNIGHTS_JOIN:
		JoinKnights(pUser, pkt);
		break;
	case KNIGHTS_WITHDRAW:
		WithdrawKnights(pUser, pkt);
		break;
	case KNIGHTS_REMOVE:
	case KNIGHTS_ADMIT:
	case KNIGHTS_REJECT:
	case KNIGHTS_CHIEF:
	case KNIGHTS_VICECHIEF:
	case KNIGHTS_OFFICER:
	case KNIGHTS_PUNISH:
		ModifyKnightsMember(pUser, pkt, opcode);
		break;
	case KNIGHTS_DESTROY:
		DestroyKnights(pUser);
		break;
	case KNIGHTS_ALLLIST_REQ:
		AllKnightsList(pUser, pkt);
		break;
	case KNIGHTS_MEMBER_REQ:
		AllKnightsMember(pUser);
		break;
	case KNIGHTS_CURRENT_REQ:
		CurrentKnightsMember(pUser, pkt);
		break;
	case KNIGHTS_JOIN_REQ:
		JoinKnightsReq(pUser, pkt);
		break;
	case KNIGHTS_TOP10:
		ListTop10Clans(pUser);
		break;
	}
}

void CKnightsManager::CreateKnights(CUser* pUser, Packet & pkt)
{
	if (pUser == NULL)
		return;
	
	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_CREATE));
	std::string idname;
	uint8 ret_value = 0;
	pkt >> idname;

	if (idname.empty() || idname.size() > MAX_ID_SIZE
		|| !IsAvailableName(idname.c_str()))
		ret_value = 3;
	else if (pUser->m_pUserData->m_bKnights != 0)
		ret_value = 5;
	else if (g_pMain->m_nServerGroup == 2)
		ret_value = 8;
	else if (pUser->getLevel() < CLAN_LEVEL_REQUIREMENT)
		ret_value = 2;
	else if (pUser->m_pUserData->m_iGold < CLAN_COIN_REQUIREMENT)
		ret_value = 4;

	if (ret_value == 0)
	{
		uint16 knightindex = GetKnightsIndex(pUser->m_pUserData->m_bNation);
		if (knightindex >= 0)
		{	
			result	<< pUser->GetSocketID() << uint8(CLAN_TYPE) 
					<< knightindex << pUser->getNation()
					<< idname << pUser->m_pUserData->m_id;
			g_pMain->m_LoggerSendQueue.PutData(&result);
			return;
		}
		ret_value = 6;
	}

	result << ret_value;
	pUser->Send(&result);
}

BOOL CKnightsManager::IsAvailableName( const char *strname)
{
	foreach_stlmap (itr, g_pMain->m_KnightsArray)
		if (_strnicmp(itr->second->m_strName, strname, MAX_ID_SIZE) == 0)
			return FALSE;

	return TRUE;
}

int CKnightsManager::GetKnightsIndex( int nation )
{
	int knightindex = 0;

	if (nation == ELMORAD)	knightindex = 15000;

	foreach_stlmap (itr, g_pMain->m_KnightsArray)
	{
		if (itr->second != NULL && 
			knightindex < itr->second->m_sIndex)
		{
			if (nation == KARUS && itr->second->m_sIndex >= 15000)
				continue;

			knightindex = itr->second->m_sIndex;
		}
	}

	knightindex++;
	if ((nation == KARUS && (knightindex >= 15000 || knightindex < 0))
		|| nation == ELMORAD && (knightindex < 15000 || knightindex > 30000)
		|| g_pMain->GetClanPtr(knightindex))
		return -1;

	return knightindex;
}

void CKnightsManager::JoinKnights(CUser *pUser, Packet & pkt)
{
	if (pUser == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS);
	uint8 bResult = 0;

	do
	{
		if (pUser->getZoneID() != pUser->getNation())
			bResult = 12;
		else if (!pUser->isClanLeader() && !pUser->isClanAssistant())
			bResult = 6;

		if (bResult != 0)
			break;

		uint16 sClanID = pUser->m_pUserData->m_bKnights;
		CKnights *pKnights = g_pMain->GetClanPtr(sClanID);
		if (pKnights == NULL)
		{
			bResult = 7;
			break;
		}

		CUser *pTUser = g_pMain->GetUserPtr(pkt.read<uint16>());
		if (pTUser == NULL)
			bResult = 2;
		else if (pTUser->isDead())
			bResult = 3;
		else if (pTUser->getNation() != pUser->getNation())
			bResult = 4;
		else if (pTUser->m_pUserData->m_bKnights > 0)
			bResult = 5;

		if (bResult != 0)
			break;

		result	<< uint8(KNIGHTS_JOIN_REQ) << uint8(1)
				<< pUser->GetSocketID() << sClanID << pKnights->m_strName;
		pTUser->Send(&result);
		return;
	} while (0);


	result << uint8(KNIGHTS_JOIN) << bResult;
	pUser->Send(&result);
}

void CKnightsManager::JoinKnightsReq(CUser *pUser, Packet & pkt)
{
	if (pUser == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_JOIN));
	uint8 bFlag, bResult = 0;
	uint16 sid, sClanID;
	pkt >> bFlag >> sid >> sClanID;
	CUser *pTUser = g_pMain->GetUserPtr(sid);
	if (pTUser == NULL)
		bResult = 2;
	else if (bFlag == 0)
		bResult = 11;
	else
	{
		CKnights *pKnights = g_pMain->GetClanPtr(sClanID);
		if (pKnights == NULL)
			bResult = 7;
		else if (pKnights->m_sMembers >= MAX_CLAN_USERS)
			bResult = 8;
	}

	if (bResult != 0)
	{
		result << bResult;
		pUser->Send(&result);
	}
	else
	{
		result << pUser->GetSocketID() << sClanID;
		g_pMain->m_LoggerSendQueue.PutData(&result);
	}
}

void CKnightsManager::WithdrawKnights(CUser *pUser, Packet & kt)
{
	if (pUser == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS);
	uint8 bResult = 0;
	do
	{
		if (!pUser->isInClan())
			bResult = 10;
		else if (pUser->isClanLeader() && pUser->getZoneID() != pUser->getNation())
			bResult = 12;

		if (bResult != 0)
			break;

		result	<< uint8(pUser->isClanLeader() ? KNIGHTS_DESTROY : KNIGHTS_WITHDRAW)
				<< pUser->GetSocketID() << pUser->m_pUserData->m_bKnights;
		g_pMain->m_LoggerSendQueue.PutData(&result);
		return;
	} while (0);

	result << bResult;
	pUser->Send(&result);
}

void CKnightsManager::DestroyKnights( CUser* pUser )
{
	if (pUser == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_DESTROY));
	uint8 bResult = 1;
	if (!pUser->isClanLeader())
		bResult = 0;
	else if (pUser->getZoneID() != pUser->getNation())
		bResult = 12;

	if (bResult == 1)
	{
		result << pUser->GetSocketID() << pUser->m_pUserData->m_bKnights;
		g_pMain->m_LoggerSendQueue.PutData(&result);
	}
	else
	{
		result << bResult;
		pUser->Send(&result);
	}
}

void CKnightsManager::ModifyKnightsMember(CUser *pUser, Packet & pkt, uint8 opcode)
{
	if (pUser == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS, opcode);
	uint8 bResult = 1, bRemoveFlag = 0;
	std::string strUserID;
	
	pkt >> strUserID;
	do
	{
		if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE)
			bResult = 2;
		else if (pUser->getZoneID() != pUser->getNation())
			bResult = 12;
		else if (_strnicmp(strUserID.c_str(), pUser->m_pUserData->m_id, strUserID.size()) == 0)
			bResult = 9;
		else if (((opcode == KNIGHTS_ADMIT || opcode == KNIGHTS_REJECT) && pUser->getFame() < OFFICER)
			|| (opcode == KNIGHTS_PUNISH && pUser->getFame() < VICECHIEF))
			bResult = 0;	
		else if (opcode != KNIGHTS_ADMIT && opcode != KNIGHTS_REJECT && opcode != KNIGHTS_PUNISH 
			&& !pUser->isClanLeader())
			bResult = 6;

		if (bResult != 1)
			break;

		CUser *pTUser = g_pMain->GetUserPtr(strUserID.c_str(), TYPE_CHARACTER);
		if (pTUser == NULL)
		{
			if (opcode != KNIGHTS_REMOVE)	
				bResult = 2;
		}
		else
		{
			if (pUser->getNation() != pTUser->getNation())
				bResult = 4;
			else if (pUser->m_pUserData->m_bKnights != pTUser->m_pUserData->m_bKnights)
				bResult = 5;

			if (bResult == 1 && opcode == KNIGHTS_VICECHIEF)
			{
				if (pTUser->isClanAssistant())
					bResult = 8;
				else if (!g_pMain->GetClanPtr(pUser->m_pUserData->m_bKnights))	
					bResult = 7;
			}

			bRemoveFlag = 1;
		}

		if (bResult != 1)
			break;

		result << pUser->GetSocketID() << pUser->m_pUserData->m_bKnights << strUserID << bRemoveFlag;
		g_pMain->m_LoggerSendQueue.PutData(&result);
		return;
	} while (0);

	result << bResult;
	pUser->Send(&result);
}

void CKnightsManager::AllKnightsList(CUser *pUser, Packet & pkt)
{
	if (pUser == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_ALLLIST_REQ));
	uint16 sPage = pkt.read<uint16>(), start = sPage * 10, count = 0;
	result << uint8(1) << sPage << count;

	foreach_stlmap (itr, g_pMain->m_KnightsArray)
	{
		CKnights* pKnights = itr->second;
		if (pKnights == NULL
			|| pKnights->m_byFlag != KNIGHTS_TYPE
			|| pKnights->m_byNation != pUser->getNation()
			|| count++ < start) 
			continue;

		result << uint16(pKnights->m_sIndex) << pKnights->m_strName << uint16(pKnights->m_sMembers) << pKnights->m_strChief << uint32(pKnights->m_nPoints);
		if (count >= start + 10)
			break;
	}
	
	count -= start;
	result.put(4, count);
	pUser->Send(&result);
}

void CKnightsManager::AllKnightsMember(CUser *pUser)
{
	if (pUser == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_MEMBER_REQ));
	uint8 bResult = 1;

	if (!pUser->isInClan())
		bResult = 2;
	else if (g_pMain->GetClanPtr(pUser->m_pUserData->m_bKnights) == NULL)
		bResult = 7;

	result << bResult;
	if (bResult == 1)
	{
		uint16 pktSize = 0, count = 0;
		result << pktSize << count << count << count; // placeholders
		pktSize = (uint16)result.size();
		count = g_pMain->GetKnightsAllMembers(pUser->m_pUserData->m_bKnights, result, pktSize, pUser->isClanLeader());
		if (count > MAX_CLAN_USERS) 
			return;

		pktSize = ((uint16)result.size() - pktSize) + 6;
		result.put(2, pktSize);
		result.put(4, count);
		result.put(6, count);
		result.put(8, count);
	}
	pUser->Send(&result);
}

void CKnightsManager::CurrentKnightsMember(CUser *pUser, Packet & pkt)
{
	if (pUser == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_CURRENT_REQ));
	CKnights *pKnights = NULL;
	if (!pUser->isInClan()
		|| (pKnights = g_pMain->GetClanPtr(pUser->m_pUserData->m_bKnights)) == NULL)
	{
		result << uint8(0); // failed
		result << "is this error still used?";
		pUser->Send(&result);
		return;
	}

	uint16 page = pkt.read<uint16>();
	uint16 start = page * 10;
	uint16 count = 0;

	result	<< uint8(1) // success
			<< pKnights->m_strChief
			<< page;

	size_t pos = result.wpos();
	result	<< count; // placeholder

	foreach_array (i, pKnights->m_arKnightsUser)
	{
		_KNIGHTS_USER *p = &pKnights->m_arKnightsUser[i];
		if (!p->byUsed || p->pSession == NULL
			|| count++ < start)
			continue;

		CUser *pTUser = p->pSession;
		result << pUser->m_pUserData->m_id << pUser->getFame() << pUser->getLevel() << pUser->m_pUserData->m_sClass;
		count++;
		if (count >= start + 10)
			break;
	}

	count -= start;
	result.put(pos, count);
	pUser->Send(&result);
}

void CKnightsManager::ReceiveKnightsProcess(CUser* pUser, Packet & pkt)
{
	uint8 command = pkt.read<uint8>();
	
	if (command != KNIGHTS_ALLLIST_REQ)
	{
		uint8 bResult = pkt.read<uint8>();
		if (bResult > 0) 
		{
			Packet result(WIZ_KNIGHTS_PROCESS, command);
			result << bResult << "Error";
			pUser->Send(&result);
			return;
		}	
	}

	switch (command)
	{
	case KNIGHTS_CREATE:
		RecvCreateKnights(pUser, pkt);
		break;
	case KNIGHTS_JOIN:
	case KNIGHTS_WITHDRAW:
		RecvJoinKnights(pUser, pkt, command);
		break;
	case KNIGHTS_REMOVE:
	case KNIGHTS_ADMIT:
	case KNIGHTS_REJECT:
	case KNIGHTS_CHIEF:
	case KNIGHTS_VICECHIEF:
	case KNIGHTS_OFFICER:
	case KNIGHTS_PUNISH:
		RecvModifyFame(pUser, pkt, command);
		break;
	case KNIGHTS_DESTROY:
		RecvDestroyKnights(pUser, pkt);
		break;
	case KNIGHTS_MEMBER_REQ:
		{
			CKnights* pKnights = g_pMain->GetClanPtr(pUser->m_pUserData->m_bKnights);
			if (pKnights == NULL)
				break;

			uint16 len = pkt.read<uint16>(), count = pkt.read<uint16>();
			if (count > MAX_CLAN_USERS) 
				break;

			Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_MEMBER_REQ));
			result << uint8(1) << len << count;
			result.append(pkt.contents() + pkt.rpos(), len);
			pUser->Send(&result);
		}
		break;
	case KNIGHTS_LIST_REQ:
		RecvKnightsList(pkt);
		break;
	case KNIGHTS_ALLLIST_REQ:
		RecvKnightsAllList(pkt);
		break;
	}
}

void CKnightsManager::RecvCreateKnights(CUser *pUser, Packet & pkt)
{
	if (pUser == NULL) 
		return;

	std::string clanName;
	uint16 sClanID;
	uint8 bFlag, bNation;
	pkt >> bFlag >> sClanID >> bNation >> clanName;

	CKnights *pKnights = new CKnights();
	pKnights->m_sIndex = sClanID;
	pKnights->m_byFlag = bFlag;
	pKnights->m_byNation = bNation;
	strcpy(pKnights->m_strName, clanName.c_str());
	strcpy(pKnights->m_strChief, pUser->m_pUserData->m_id);

	pUser->m_pUserData->m_iGold -= CLAN_COIN_REQUIREMENT;
	g_pMain->m_KnightsArray.PutData(pKnights->m_sIndex, pKnights);
	pKnights->AddUser(pUser);
	pUser->m_pUserData->m_bFame = CHIEF;

	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_CREATE));
	result	<< uint8(1) << pUser->GetSocketID() 
			<< sClanID << clanName
			<< pKnights->m_byGrade << pKnights->m_byRanking
			<< pUser->m_pUserData->m_iGold;

	pUser->SendToRegion(&result);

	result.Initialize(UDP_KNIGHTS_PROCESS);
	result	<< uint8(KNIGHTS_CREATE)
			<< pKnights->m_byFlag << sClanID 
			<< bNation << clanName << pUser->m_pUserData->m_id;
	g_pMain->Send_UDP_All(&result, g_pMain->m_nServerGroup == 0 ? 0 : 1);
}

void CKnightsManager::RecvJoinKnights(CUser *pUser, Packet & pkt, BYTE command)
{
	if (pUser == NULL) 
		return;

	uint16 sClanID = pkt.read<uint16>();
	CKnights *pKnights = g_pMain->GetClanPtr(sClanID);
	if (pKnights == NULL)
		return;

	if (command == KNIGHTS_JOIN)
		pKnights->AddUser(pUser);
	else
		pKnights->RemoveUser(pUser);

	Packet result(WIZ_KNIGHTS_PROCESS, command);
	result	<< uint8(1) << pUser->GetSocketID() << pUser->m_pUserData->m_bKnights << pUser->getFame();

	if (command == KNIGHTS_JOIN)
	{
		result << pKnights->m_byRanking << int16(pKnights->m_sMarkVersion) 
			<< uint16(pKnights->m_sCape) // cape ID
			<< uint8(0) << uint8(0) << uint8(0) // cape RGB
			<< pKnights->m_strName << pKnights->m_byGrade << pKnights->m_byRanking;
	}

	pUser->SendToRegion(&result);

	result.Initialize(UDP_KNIGHTS_PROCESS);
	result << command << sClanID << pUser->m_pUserData->m_id;
	g_pMain->Send_UDP_All(&result, (g_pMain->m_nServerGroup == 0 ? 0 : 1));
}

void CKnightsManager::RecvModifyFame(CUser *pUser, Packet & pkt, BYTE command)
{
	if (pUser == NULL) 
		return;

	CString clanNotice;
	std::string strUserID;
	uint16 sClanID;

	pkt >> sClanID >> strUserID;

	CUser *pTUser = g_pMain->GetUserPtr(strUserID.c_str(), TYPE_CHARACTER);
	CKnights *pKnights = g_pMain->GetClanPtr(sClanID);

	switch (command)
	{
	case KNIGHTS_REMOVE:
		if (pTUser != NULL)
		{
			clanNotice = g_pMain->GetServerResource(IDS_KNIGHTS_REMOVE);
			pKnights->RemoveUser(pTUser);
		}
		else
		{
			pKnights->RemoveUser(strUserID.c_str());
		}
		break;
	case KNIGHTS_ADMIT:
		if (pTUser != NULL)
			pTUser->m_pUserData->m_bFame = KNIGHT;
		break;
	case KNIGHTS_REJECT:
		if (pTUser != NULL)
		{
			pTUser->m_pUserData->m_bKnights = 0;
			pTUser->m_pUserData->m_bFame = 0;

			RemoveKnightsUser(sClanID, pTUser->m_pUserData->m_id);
		}
		break;
	case KNIGHTS_CHIEF:
		if (pTUser != NULL)
		{
			pTUser->m_pUserData->m_bFame = CHIEF;
			clanNotice = g_pMain->GetServerResource(IDS_KNIGHTS_CHIEF);
		}
		break;
	case KNIGHTS_VICECHIEF:
		if (pTUser != NULL)
		{
			pTUser->m_pUserData->m_bFame = VICECHIEF;
			clanNotice = g_pMain->GetServerResource(IDS_KNIGHTS_VICECHIEF);
		}
		break;
	case KNIGHTS_OFFICER:
		if (pTUser != NULL)
			pTUser->m_pUserData->m_bFame = OFFICER;
		break;
	case KNIGHTS_PUNISH:
		if (pTUser != NULL)
			pTUser->m_pUserData->m_bFame = PUNISH;
		break;
	}

	if (pTUser != NULL)
		pTUser->SendClanUserStatusUpdate(command == KNIGHTS_REMOVE);

	Packet result(UDP_KNIGHTS_PROCESS, command);
	result << sClanID << strUserID;
	g_pMain->Send_UDP_All(&result, (g_pMain->m_nServerGroup == 0 ? 0 : 1));

	if (clanNotice.GetLength() == 0)
		return;

	// Construct the clan system chat packet
	pKnights->ConstructChatPacket(result, clanNotice, pTUser != NULL ? pTUser->m_pUserData->m_id : strUserID.c_str()); 

	// If we've been removed from a clan, tell the user as well (since they're no longer in the clan)
	if (command == KNIGHTS_REMOVE && pTUser != NULL)
		pTUser->Send(&result);

	// Otherwise, since we're actually in the clan, we don't need to be explicitly told what happened.
	if (pKnights != NULL)
		pKnights->Send(&result);
}

void CKnightsManager::RecvDestroyKnights(CUser *pUser, Packet & pkt)
{
	if (pUser == NULL)
		return;
	
	uint16 sClanID = pkt.read<uint16>();
	CKnights *pKnights = g_pMain->GetClanPtr(sClanID);
	if (pKnights == NULL)
		return;

	pKnights->Disband(pUser);

	Packet result(UDP_KNIGHTS_PROCESS, uint8(KNIGHTS_DESTROY));
	result << sClanID;
	g_pMain->Send_UDP_All(&result, (g_pMain->m_nServerGroup == 0 ? 0 : 1));
}

void CKnightsManager::RecvKnightsList(Packet & pkt)
{
	std::string clanName;
	uint32 nPoints;
	uint16 sClanID, sMembers;
	uint8 bNation, bRank;
	pkt >> sClanID >> bNation >> clanName >> sMembers >> nPoints >> bRank;

	if (g_pMain->m_nServerNo != BATTLE)
		return;

	CKnights *pKnights = g_pMain->GetClanPtr(sClanID);
	if (pKnights == NULL)
	{
		pKnights = new CKnights();
		if (!g_pMain->m_KnightsArray.PutData(sClanID, pKnights))
		{
			delete pKnights;
			pKnights = NULL;
			return;
		}
	}

	pKnights->m_sIndex = sClanID;
	pKnights->m_byNation = bNation;
	strcpy(pKnights->m_strName, clanName.c_str());
	pKnights->m_sMembers = sMembers;
	pKnights->m_nPoints = nPoints;
	pKnights->m_byGrade = g_pMain->GetKnightsGrade(nPoints);
	pKnights->m_byRanking = bRank;
}

BOOL CKnightsManager::AddKnightsUser(int index, char* UserName)
{
	CKnights *pKnights = g_pMain->GetClanPtr(index);
	if (pKnights == NULL)
		return FALSE;

	return pKnights->AddUser(UserName);
}

BOOL CKnightsManager::RemoveKnightsUser(int index, char* UserName)
{
	CKnights *pKnights = g_pMain->GetClanPtr(index);
	if (pKnights == NULL)
		return FALSE;

	return pKnights->RemoveUser(UserName);
}

/**
 * This method seems to be useless. Leaving it just in case.
 **/
void CKnightsManager::SetKnightsUser(int index, char* UserName)
{
	CKnights *pKnights = g_pMain->GetClanPtr(index);
	if (pKnights == NULL)
		return;

	for (int i = 0; i < MAX_CLAN_USERS; i++)
	{
		if (pKnights->m_arKnightsUser[i].byUsed == 0)
			continue;
		
		if (!_strcmpi(pKnights->m_arKnightsUser[i].strUserName, UserName))
			return;
	}

	pKnights->AddUser(UserName);
}

void CKnightsManager::RecvKnightsAllList(Packet & pkt)
{
	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_ALLLIST_REQ));
	uint8 count = pkt.read<uint8>(), send_count = 0;
	result << send_count; // placeholder for count
	for (int i = 0; i < count; i++)
	{
		uint32 nPoints; uint16 sClanID; uint8 bRank;
		pkt >> sClanID >> nPoints >> bRank;

		CKnights *pKnights = g_pMain->GetClanPtr(sClanID);
		if (pKnights == NULL)
			continue;

		if (pKnights->m_nPoints != nPoints
			|| pKnights->m_byRanking != bRank)
		{
			pKnights->m_nPoints = nPoints;
			pKnights->m_byRanking = bRank;
			pKnights->m_byGrade = g_pMain->GetKnightsGrade(nPoints);

			result << sClanID << pKnights->m_byGrade << pKnights->m_byRanking;
			send_count++;
		}
	}

	result.put(1, send_count);
	g_pMain->Send_All(&result);
}

void CKnightsManager::ListTop10Clans(CUser *pUser)
{
	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_TOP10));
	result << uint16(0);

	// TO-DO: List top 10 clans of each nation
	for (int i = 0; i < 5; i++)
	{
		result	<< int16(-1)	// Clan ID
				<< ""			// Clan name
				<< int16(-1)	// Symbol version
				<< int16(i);	// Rank
	}

	for (int i = 0; i < 5; i++)
	{
		result	<< int16(-1)	// Clan ID
				<< ""			// Clan name
				<< int16(-1)	// Symbol version
				<< int16(i);	// Rank
	}

	pUser->Send(&result);
}