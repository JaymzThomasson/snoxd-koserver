#include "stdafx.h"
#include "KnightsManager.h"
#include "User.h"
#include "GameDefine.h"
#include "EbenezerDlg.h"

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
	case KNIGHTS_MARK_REGISTER:
		RegisterClanSymbol(pUser, pkt);
		break;
	case KNIGHTS_MARK_VERSION_REQ:
		RequestClanSymbolVersion(pUser, pkt);
		break;
	case KNIGHTS_MARK_REGION_REQ:
		RequestClanSymbols(pUser, pkt);
		break;
	case KNIGHTS_MARK_REQ:
		GetClanSymbol(pUser, pkt.read<uint16>());
		break;
	case KNIGHTS_TOP10:
		ListTop10Clans(pUser);
		break;
	case KNIGHTS_DONATE_POINTS:
		break;
	case KNIGHTS_POINT_REQ:
		break;
	case KNIGHTS_ALLY_LIST:
		break;

	default:
		TRACE("Unhandled clan system opcode: %X\n", opcode);
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
	else if (pUser->GetClanID() != 0)
		ret_value = 5;
	else if (g_pMain.m_nServerGroup == 2)
		ret_value = 8;
	else if (pUser->GetLevel() < CLAN_LEVEL_REQUIREMENT)
		ret_value = 2;
	else if (pUser->m_iGold < CLAN_COIN_REQUIREMENT)
		ret_value = 4;

	if (ret_value == 0)
	{
		uint16 knightindex = GetKnightsIndex(pUser->m_bNation);
		if (knightindex >= 0)
		{	
			result	<< uint8(CLAN_TYPE) 
					<< knightindex << pUser->GetNation()
					<< idname << pUser->GetName();
			g_pMain.AddDatabaseRequest(result, pUser);
			return;
		}
		ret_value = 6;
	}

	result << ret_value;
	pUser->Send(&result);
}

BOOL CKnightsManager::IsAvailableName( const char *strname)
{
	foreach_stlmap (itr, g_pMain.m_KnightsArray)
		if (_strnicmp(itr->second->m_strName.c_str(), strname, MAX_ID_SIZE) == 0)
			return FALSE;

	return TRUE;
}

int CKnightsManager::GetKnightsIndex( int nation )
{
	int knightindex = 0;

	if (nation == ELMORAD)	knightindex = 15000;

	foreach_stlmap (itr, g_pMain.m_KnightsArray)
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
		|| g_pMain.GetClanPtr(knightindex))
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
		if (pUser->GetMap()->m_isAttackZone)
			bResult = 12;
		else if (!pUser->isClanLeader() && !pUser->isClanAssistant())
			bResult = 6;

		if (bResult != 0)
			break;

		uint16 sClanID = pUser->GetClanID();
		CKnights *pKnights = g_pMain.GetClanPtr(sClanID);
		if (pKnights == NULL)
		{
			bResult = 7;
			break;
		}

		CUser *pTUser = g_pMain.GetUserPtr(pkt.read<uint16>());
		if (pTUser == NULL)
			bResult = 2;
		else if (pTUser->isDead())
			bResult = 3;
		else if (pTUser->GetNation() != pUser->GetNation())
			bResult = 4;
		else if (pTUser->GetClanID() > 0)
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
	CUser *pTUser = g_pMain.GetUserPtr(sid);
	if (pTUser == NULL)
		bResult = 2;
	else if (bFlag == 0)
		bResult = 11;
	else
	{
		CKnights *pKnights = g_pMain.GetClanPtr(sClanID);
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
		result << sClanID;
		g_pMain.AddDatabaseRequest(result, pUser);
	}
}

void CKnightsManager::WithdrawKnights(CUser *pUser, Packet & pkt)
{
	if (pUser == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS);
	uint8 bResult = 0;
	do
	{
		if (!pUser->isInClan())
			bResult = 10;
		else if (pUser->isClanLeader() && pUser->GetZoneID() != pUser->GetNation())
			bResult = 12;

		if (bResult != 0)
			break;

		result	<< uint8(pUser->isClanLeader() ? KNIGHTS_DESTROY : KNIGHTS_WITHDRAW)
				<< pUser->GetClanID();
		g_pMain.AddDatabaseRequest(result, pUser);
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
	else if (pUser->GetZoneID() != pUser->GetNation())
		bResult = 12;

	if (bResult == 1)
	{
		result << pUser->GetClanID();
		g_pMain.AddDatabaseRequest(result, pUser);
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
		else if (pUser->GetZoneID() != pUser->GetNation())
			bResult = 12;
		else if (_strnicmp(strUserID.c_str(), pUser->GetName(), strUserID.size()) == 0)
			bResult = 9;
		else if (((opcode == KNIGHTS_ADMIT || opcode == KNIGHTS_REJECT) && pUser->getFame() < OFFICER)
			|| (opcode == KNIGHTS_PUNISH && pUser->getFame() < VICECHIEF))
			bResult = 0;	
		else if (opcode != KNIGHTS_ADMIT && opcode != KNIGHTS_REJECT && opcode != KNIGHTS_PUNISH 
			&& !pUser->isClanLeader())
			bResult = 6;

		if (bResult != 1)
			break;

		CUser *pTUser = g_pMain.GetUserPtr(strUserID.c_str(), TYPE_CHARACTER);
		if (pTUser == NULL)
		{
			if (opcode != KNIGHTS_REMOVE)	
				bResult = 2;
		}
		else
		{
			if (pUser->GetNation() != pTUser->GetNation())
				bResult = 4;
			else if (pUser->GetClanID() != pTUser->GetClanID())
				bResult = 5;

			if (bResult == 1 && opcode == KNIGHTS_VICECHIEF)
			{
				if (pTUser->isClanAssistant())
					bResult = 8;
				else if (!g_pMain.GetClanPtr(pUser->GetClanID()))	
					bResult = 7;
			}

			bRemoveFlag = 1;
		}

		if (bResult != 1)
			break;

		result << pUser->GetClanID() << strUserID << bRemoveFlag;
		g_pMain.AddDatabaseRequest(result, pUser);
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

	foreach_stlmap (itr, g_pMain.m_KnightsArray)
	{
		CKnights* pKnights = itr->second;
		if (pKnights == NULL
			|| pKnights->m_byFlag < KNIGHTS_TYPE
			|| pKnights->m_byNation != pUser->GetNation()
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
	else if (g_pMain.GetClanPtr(pUser->GetClanID()) == NULL)
		bResult = 7;

	result << bResult;
	if (bResult == 1)
	{
		uint16 pktSize = 0, count = 0;
		result << pktSize << count << count << count; // placeholders
		pktSize = (uint16)result.size();
		count = g_pMain.GetKnightsAllMembers(pUser->GetClanID(), result, pktSize, pUser->isClanLeader());
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
		|| (pKnights = g_pMain.GetClanPtr(pUser->GetClanID())) == NULL)
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
		result << pUser->GetName() << pUser->getFame() << pUser->GetLevel() << pUser->m_sClass;
		count++;
		if (count >= start + 10)
			break;
	}

	count -= start;
	result.put(pos, count);
	pUser->Send(&result);
}

void CKnightsManager::RecvUpdateKnights(CUser *pUser, Packet & pkt, BYTE command)
{
	if (pUser == NULL) 
		return;

	uint16 sClanID = pkt.read<uint16>();
	CKnights *pKnights = g_pMain.GetClanPtr(sClanID);
	if (pKnights == NULL)
		return;

	if (command == KNIGHTS_JOIN)
		pKnights->AddUser(pUser);
	else
		pKnights->RemoveUser(pUser);

	Packet result(WIZ_KNIGHTS_PROCESS, command);
	result	<< uint8(1) << pUser->GetSocketID() << pUser->GetClanID() << pUser->getFame();

	if (command == KNIGHTS_JOIN)
	{
		result << pKnights->m_byFlag
			<< uint16(pKnights->m_sAlliance)
			<< uint16(pKnights->m_sCape) 
			<< pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << uint8(0)
			<< int16(pKnights->m_sMarkVersion) 
			<< pKnights->m_strName << pKnights->m_byGrade << pKnights->m_byRanking;
	}

	pUser->SendToRegion(&result);

	result.Initialize(UDP_KNIGHTS_PROCESS);
	result << command << sClanID << pUser->GetName();
	g_pMain.Send_UDP_All(&result, (g_pMain.m_nServerGroup == 0 ? 0 : 1));
}

void CKnightsManager::RecvModifyFame(CUser *pUser, Packet & pkt, BYTE command)
{
	if (pUser == NULL) 
		return;

	std::string clanNotice;
	std::string strUserID;
	uint16 sClanID;

	pkt >> sClanID >> strUserID;

	CUser *pTUser = g_pMain.GetUserPtr(strUserID.c_str(), TYPE_CHARACTER);
	CKnights *pKnights = g_pMain.GetClanPtr(sClanID);

	switch (command)
	{
	case KNIGHTS_REMOVE:
		if (pTUser != NULL)
		{
			clanNotice = g_pMain.GetServerResource(IDS_KNIGHTS_REMOVE);
			pKnights->RemoveUser(pTUser);
		}
		else
		{
			pKnights->RemoveUser(strUserID.c_str());
		}
		break;
	case KNIGHTS_ADMIT:
		if (pTUser != NULL)
			pTUser->m_bFame = KNIGHT;
		break;
	case KNIGHTS_REJECT:
		if (pTUser != NULL)
		{
			pTUser->SetClanID(0);
			pTUser->m_bFame = 0;

			RemoveKnightsUser(sClanID, pTUser->GetName());
		}
		break;
	case KNIGHTS_CHIEF:
		if (pTUser != NULL)
		{
			pTUser->m_bFame = CHIEF;
			clanNotice = g_pMain.GetServerResource(IDS_KNIGHTS_CHIEF);
		}
		break;
	case KNIGHTS_VICECHIEF:
		if (pTUser != NULL)
		{
			pTUser->m_bFame = VICECHIEF;
			clanNotice = g_pMain.GetServerResource(IDS_KNIGHTS_VICECHIEF);
		}
		break;
	case KNIGHTS_OFFICER:
		if (pTUser != NULL)
			pTUser->m_bFame = OFFICER;
		break;
	case KNIGHTS_PUNISH:
		if (pTUser != NULL)
			pTUser->m_bFame = PUNISH;
		break;
	}

	if (pTUser != NULL)
		pTUser->SendClanUserStatusUpdate(command == KNIGHTS_REMOVE);

	Packet result(UDP_KNIGHTS_PROCESS, command);
	result << sClanID << strUserID;
	g_pMain.Send_UDP_All(&result, (g_pMain.m_nServerGroup == 0 ? 0 : 1));

	if (clanNotice.empty())
		return;

	// Construct the clan system chat packet
	pKnights->ConstructChatPacket(result, clanNotice.c_str(), 
		pTUser != NULL ? pTUser->GetName() : strUserID.c_str()); 

	// If we've been removed from a clan, tell the user as well (since they're no longer in the clan)
	if (command == KNIGHTS_REMOVE && pTUser != NULL)
		pTUser->Send(&result);

	// Otherwise, since we're actually in the clan, we don't need to be explicitly told what happened.
	if (pKnights != NULL)
		pKnights->Send(&result);
}

BOOL CKnightsManager::AddKnightsUser(int index, const char* UserName)
{
	CKnights *pKnights = g_pMain.GetClanPtr(index);
	if (pKnights == NULL)
		return FALSE;

	return pKnights->AddUser(UserName);
}

BOOL CKnightsManager::RemoveKnightsUser(int index, const char* UserName)
{
	CKnights *pKnights = g_pMain.GetClanPtr(index);
	if (pKnights == NULL)
		return FALSE;

	return pKnights->RemoveUser(UserName);
}

/**
 * This method seems to be useless. Leaving it just in case.
 **/
void CKnightsManager::SetKnightsUser(int index, const char* UserName)
{
	CKnights *pKnights = g_pMain.GetClanPtr(index);
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

		CKnights *pKnights = g_pMain.GetClanPtr(sClanID);
		if (pKnights == NULL)
			continue;

		if (pKnights->m_nPoints != nPoints
			|| pKnights->m_byRanking != bRank)
		{
			pKnights->m_nPoints = nPoints;
			pKnights->m_byRanking = bRank;
			pKnights->m_byGrade = g_pMain.GetKnightsGrade(nPoints);

			result << sClanID << pKnights->m_byGrade << pKnights->m_byRanking;
			send_count++;
		}
	}

	result.put(1, send_count);
	g_pMain.Send_All(&result);
}

void CKnightsManager::RegisterClanSymbol(CUser* pUser, Packet & pkt)
{
	if (pUser == NULL || !pUser->isInClan())
		return;

	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_MARK_REGISTER));
	CKnights *pKnights = NULL;
	char clanSymbol[MAX_KNIGHTS_MARK];
	uint16 sFailCode = 1, sSymbolSize = pkt.read<uint16>();

	// Are they even a clan leader?
	if (!pUser->isClanLeader())
		sFailCode = 11;
	// Invalid zone (only in home zones)
	else if (pUser->GetZoneID() != pUser->GetNation())
		sFailCode = 12;
	// Invalid symbol size (or invalid packet)
	else if (sSymbolSize == 0 
		|| sSymbolSize > MAX_KNIGHTS_MARK
		|| pkt.size() < sSymbolSize)
		sFailCode = 13;
	// User doesn't have enough coins
	else if (pUser->m_iGold < CLAN_SYMBOL_COST)
		sFailCode = 14;
	// Clan doesn't exist
	else if ((pKnights = g_pMain.GetClanPtr(pUser->GetClanID())) == NULL)
		sFailCode = 20;
	// Clan not promoted
	else if (pKnights->m_byFlag < KNIGHTS_TYPE)
		sFailCode = 11;

	// Uh oh, did we error?
	if (sFailCode != 1)
	{
		result << sFailCode;
		pUser->Send(&result);
		return;
	}

	// Read the clan symbol from the packet
	pkt.read(clanSymbol, sSymbolSize);

	// Nope? Let's update the clan symbol.
	result	<< pUser->GetClanID() << sSymbolSize;
	result.append(clanSymbol, sSymbolSize);
	g_pMain.AddDatabaseRequest(result, pUser);
}

void CKnightsManager::RequestClanSymbolVersion(CUser* pUser, Packet & pkt)
{
	if (pUser == NULL
		|| !pUser->isInClan())
		return;

	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_MARK_VERSION_REQ));
	int16 sFailCode = 1;

	CKnights *pKnights = g_pMain.GetClanPtr(pUser->GetClanID());
	if (pKnights == NULL || pKnights->m_byFlag < KNIGHTS_TYPE /* not promoted */ || !pUser->isClanLeader())
		sFailCode = 11;
	else if (pUser->GetZoneID() != pUser->GetNation())
		sFailCode = 12;
	
	result << sFailCode;

	if (sFailCode == 1)
		result << uint16(pKnights->m_sMarkVersion);

	pUser->Send(&result);
}

/**
 * The clan member (leader only?) tells groups of users to update the clan symbols
 * they have for this clan. This is a horrible, horrible idea.
 **/
void CKnightsManager::RequestClanSymbols(CUser* pUser, Packet & pkt)
{
	// Should we force them to be a clan leader too? 
	// Need to check if *any* clan member can trigger this, or if it's just leaders.
	if (pUser == NULL
		|| !pUser->isInClan())
		return;

	uint16 sCount = pkt.read<uint16>();
	if (sCount > 100)
		sCount = 100;

	for (int i = 0; i < sCount; i++)
	{
		uint16 sid = pkt.read<uint16>();
		CUser *pTUser = g_pMain.GetUserPtr(sid);
		if (pTUser == NULL
			|| !pTUser->isInGame())
			continue;

		// This is really quite scary that users can send directly to specific players like this.
		// Quite possibly we should replace this with a completely server-side implementation.
		GetClanSymbol(pTUser, pUser->GetClanID());
	}
}

void CKnightsManager::GetClanSymbol(CUser* pUser, uint16 sClanID)
{
	if (pUser == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS);
	CKnights *pKnights = g_pMain.GetClanPtr(sClanID);

	// Dose that clan exist?
	if (pKnights == NULL 
		// Are they promoted?
		|| pKnights->m_byFlag < KNIGHTS_TYPE
		// Is their symbol version set?
		|| pKnights->m_sMarkVersion == 0
		// The clan symbol is more than 0 bytes, right?
		|| pKnights->m_sMarkLen <= 0)
		return;

	result	<< uint8(KNIGHTS_MARK_REQ) << uint16(1); // success
	result	<< uint16(pKnights->m_byNation) << sClanID
			<< uint16(pKnights->m_sMarkVersion) << uint16(pKnights->m_sMarkLen);
	result.append(pKnights->m_Image, pKnights->m_sMarkLen);
	pUser->SendCompressed(&result);
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