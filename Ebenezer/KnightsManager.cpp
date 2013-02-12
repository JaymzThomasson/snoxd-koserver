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
	m_pMain = NULL;
}

CKnightsManager::~CKnightsManager()
{

}

// TO-DO: Move this to the CUser class.
void CKnightsManager::PacketProcess(CUser *pUser, Packet & pkt)
{
	uint8 opcode = pkt.read<uint8>();
	TRACE("Clan packet: %X\n", opcode); 
	if( !pUser ) return;

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
		ModifyKnightsMember(pUser, pkt);
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
	else if (m_pMain->m_nServerGroup == 2)
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
			m_pMain->m_LoggerSendQueue.PutData(&result);
			return;
		}
		ret_value = 6;
	}

	result << ret_value;
	pUser->Send(&result);
}

BOOL CKnightsManager::IsAvailableName( const char *strname)
{
	foreach_stlmap (itr, m_pMain->m_KnightsArray)
		if (_strnicmp(itr->second->m_strName, strname, MAX_ID_SIZE) == 0)
			return FALSE;

	return TRUE;
}

int CKnightsManager::GetKnightsIndex( int nation )
{
	//TRACE("GetKnightsIndex = nation=%d\n", nation);
	int knightindex = 0;

	if (nation == ELMORAD)	knightindex = 15000;

	foreach_stlmap (itr, m_pMain->m_KnightsArray)
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
		|| m_pMain->m_KnightsArray.GetData(knightindex))
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
		else if (pUser->getFame() != CHIEF && pUser->getFame() != VICECHIEF)
			bResult = 6;

		if (bResult != 0)
			break;

		uint16 sClanID = pUser->m_pUserData->m_bKnights;
		CKnights *pKnights = m_pMain->m_KnightsArray.GetData(sClanID);
		if (pKnights == NULL)
		{
			bResult = 7;
			break;
		}

		CUser *pTUser = m_pMain->GetUserPtr(pkt.read<uint16>());
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
	CUser *pTUser = m_pMain->GetUserPtr(sid);
	if (pTUser == NULL)
		bResult = 2;
	else if (bFlag == 0)
		bResult = 11;
	else
	{
		CKnights *pKnights = m_pMain->m_KnightsArray.GetData(sClanID);
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
		m_pMain->m_LoggerSendQueue.PutData(&result);
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
		else if (pUser->getFame() == CHIEF && pUser->getZoneID() != pUser->getNation())
			bResult = 12;

		if (bResult != 0)
			break;

		result	<< uint8(pUser->getFame() == CHIEF ? KNIGHTS_DESTROY : KNIGHTS_WITHDRAW)
				<< pUser->GetSocketID() << pUser->m_pUserData->m_bKnights;
		m_pMain->m_LoggerSendQueue.PutData(&result);
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
	if (pUser->getFame() != CHIEF)
		bResult = 0;
	else if (pUser->getZoneID() != pUser->getNation())
		bResult = 12;

	if (bResult == 1)
	{
		result << pUser->GetSocketID() << pUser->m_pUserData->m_bKnights;
		m_pMain->m_LoggerSendQueue.PutData(&result);
	}
	else
	{
		result << bResult;
		pUser->Send(&result);
	}
}

void CKnightsManager::ModifyKnightsMember(CUser *pUser, Packet & pkt)
{
	int send_index = 0, ret_value = 0, vicechief = 0, remove_flag = 0;
	char send_buff[128]; 
	std::string strUserID;
	CUser* pTUser = NULL;
	
	if( !pUser ) return;
	pkt >> strUserID;
	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE)
	{
		ret_value = 2;
		goto fail_return;
	}
	if( pUser->m_pUserData->m_bZone > 2 )	{	// 전쟁존에서는 기사단 처리가 안됨
		ret_value = 12;
		goto fail_return;
	}

	if( _strnicmp(strUserID.c_str(), pUser->m_pUserData->m_id, strUserID.size()) == 0 ) {	// 자신은 할 수 없습니다
		ret_value = 9;
		goto fail_return;
	}

	if( pkt.GetOpcode() == KNIGHTS_ADMIT || pkt.GetOpcode() == KNIGHTS_REJECT ) {	// 기사단, 멤버가입 및 멤버거절, 장교 이상이 할 수 있습니다
		if( pUser->m_pUserData->m_bFame < OFFICER ) {
			goto fail_return;
		}
	}
	else if( pkt.GetOpcode() == KNIGHTS_PUNISH ){							// 징계, 부기사단장 이상이 할 수 있습니다
		if( pUser->m_pUserData->m_bFame < VICECHIEF ) {
			goto fail_return;
		}
	}
	else {
		if( pUser->m_pUserData->m_bFame != CHIEF ) {				// 기사단장 만이 할 수 있습니다
			ret_value = 6;
			goto fail_return;
		}
	}

	pTUser = m_pMain->GetUserPtr(strUserID.c_str(), TYPE_CHARACTER);
	if( !pTUser )	{
		if( pkt.GetOpcode() == KNIGHTS_REMOVE )	{			// 게임상에 없는 유저 추방시 (100)
			remove_flag = 0;
			SetByte( send_buff, WIZ_KNIGHTS_PROCESS, send_index );
			SetByte( send_buff, pkt.GetOpcode(), send_index );
			SetShort( send_buff, pUser->GetSocketID(), send_index );
			SetShort( send_buff, pUser->m_pUserData->m_bKnights, send_index );
			SetKOString(send_buff, (char *)strUserID.c_str(), send_index);
			SetByte( send_buff, remove_flag, send_index );
			m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );
			return;
		}
		else	{
			ret_value = 2;
			goto fail_return;
		}
	}
	
	if( pUser->m_pUserData->m_bNation != pTUser->m_pUserData->m_bNation ) {
		ret_value = 4;
		goto fail_return;
	}
	if ( pUser->m_pUserData->m_bKnights != pTUser->m_pUserData->m_bKnights )	{
		ret_value = 5;
		goto fail_return;
	}

	// 부단장이 3명이 됐는지를 판단 (클랜인 경우이다,,)
	if( pkt.GetOpcode() == KNIGHTS_VICECHIEF ){							// 부단장 임명
		if( pTUser->m_pUserData->m_bFame == VICECHIEF )	{		// 이미 부단장인 경우
			ret_value = 8;
			goto fail_return;
		}

		CKnights* pKnights = NULL;
		pKnights = m_pMain->m_KnightsArray.GetData( pUser->m_pUserData->m_bKnights );
		if( !pKnights )		{
			ret_value = 7;
			goto fail_return;
		}

	/*	if( !strcmp( pKnights->strViceChief_1, "") )	vicechief = 1;
		else if( !strcmp( pKnights->strViceChief_2, "") )	vicechief = 2;
		else if( !strcmp( pKnights->strViceChief_3, "") )	vicechief = 3;
		else {
			ret_value = 8;
			goto fail_return;
		}	*/
	}
	
	remove_flag = 1;	// 게임상에 있는 유저 추방시 (1)
	SetByte( send_buff, WIZ_KNIGHTS_PROCESS, send_index );
	SetByte( send_buff, pkt.GetOpcode(), send_index );
	SetShort( send_buff, pUser->GetSocketID(), send_index );
	SetShort( send_buff, pUser->m_pUserData->m_bKnights, send_index );
	SetKOString( send_buff, (char *)strUserID.c_str(), send_index );
	SetByte( send_buff, remove_flag, send_index );						
	m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );
	return;

fail_return:
	SetByte( send_buff, WIZ_KNIGHTS_PROCESS, send_index );
	SetByte( send_buff, pkt.GetOpcode(), send_index );
	SetByte( send_buff, ret_value, send_index );
	//TRACE("## ModifyKnights Fail - command=%d, nid=%d, name=%s, error_code=%d ##\n", command, pUser->GetSocketID(), pUser->m_pUserData->m_id, ret_value);
	pUser->Send( send_buff, send_index );
}

void CKnightsManager::AllKnightsList(CUser *pUser, Packet & pkt)
{
	int send_index = 0, buff_index = 0, count = 0, index = 0, start = 0;
	uint16 page = pkt.read<uint16>();
	char send_buff[4096], temp_buff[4096];

	if( !pUser ) return;

	start = page * 10;			// page : 0 ~

	foreach_stlmap (itr, m_pMain->m_KnightsArray)
	{
		CKnights* pKnights = itr->second;
		if (pKnights == NULL
			|| pKnights->m_byFlag != KNIGHTS_TYPE
			|| pKnights->m_byNation != pUser->m_pUserData->m_bNation
			|| count++ < start) 
			continue;

		SetShort( temp_buff, pKnights->m_sIndex, buff_index );
		SetKOString( temp_buff, pKnights->m_strName, buff_index );
		SetShort( temp_buff, pKnights->m_sMembers, buff_index );
		SetKOString( temp_buff, pKnights->m_strChief, buff_index );
		SetDWORD( temp_buff, pKnights->m_nPoints, buff_index );
		if (count >= start + 10)
			break;
	}

	SetByte( send_buff, WIZ_KNIGHTS_PROCESS, send_index );
	SetByte( send_buff, KNIGHTS_ALLLIST_REQ, send_index );
	SetByte( send_buff, 0x01, send_index );
	SetShort( send_buff, page, send_index );
	SetShort( send_buff, count-start, send_index );
	SetString( send_buff, temp_buff, buff_index, send_index );
	pUser->Send( send_buff, send_index );
}

void CKnightsManager::AllKnightsMember(CUser *pUser)
{
	int index = 0, send_index = 0, page = 0, ret_value = 0, temp_index = 0, count=0, pktsize = 0;
	char send_buff[4096], temp_buff[4096]; 
	CKnights* pKnights = NULL;

	if( !pUser ) return;
	if( pUser->m_pUserData->m_bKnights <= 0 ) {		// 기사단에 가입되어 있지 않습니다
		ret_value = 2;
		goto fail_return;
	}

	pKnights = m_pMain->m_KnightsArray.GetData( pUser->m_pUserData->m_bKnights );
	if( !pKnights )	{
		ret_value = 7;
		goto fail_return;
	}

	// 단장 
/*	if( pUser->m_pUserData->m_bFame == CHIEF )	{
		SetByte( send_buff, WIZ_KNIGHTS_PROCESS, send_index );
		SetByte( send_buff, KNIGHTS_MEMBER_REQ, send_index );
		SetShort( send_buff, pUser->GetSocketID(), send_index );
		SetShort( send_buff, pUser->m_pUserData->m_bKnights, send_index );
		//SetShort( send_buff, page, send_index );
		m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );
		return;
	}	*/

	// 직접.. 게임서버에서 유저정보를 참조해서 불러오는 방식 (단장이 아닌 모든 사람)
	if( pUser->m_pUserData->m_bFame == CHIEF )	{
		count = m_pMain->GetKnightsAllMembers( pUser->m_pUserData->m_bKnights, temp_buff, temp_index, 1 );
	}
	else	{
		count = m_pMain->GetKnightsAllMembers( pUser->m_pUserData->m_bKnights, temp_buff, temp_index, 0 );
	}

	pktsize = temp_index+4;
	if (count > MAX_CLAN_USERS) 
		return;

	SetByte( send_buff, WIZ_KNIGHTS_PROCESS, send_index );
	SetByte( send_buff, KNIGHTS_MEMBER_REQ, send_index );
	SetByte( send_buff, 0x01, send_index );
	SetShort( send_buff, pktsize, send_index );
	SetShort( send_buff, count, send_index );
	SetString( send_buff, temp_buff, temp_index, send_index );
	pUser->Send( send_buff, send_index );

/*	SetByte( send_buff, WIZ_KNIGHTS_PROCESS, send_index );
	SetByte( send_buff, KNIGHTS_MEMBER_REQ, send_index );
	SetShort( send_buff, pUser->GetSocketID(), send_index );
	SetShort( send_buff, pUser->m_pUserData->m_bKnights, send_index );
	//SetShort( send_buff, page, send_index );
	m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );	*/
	return;

fail_return:
	SetByte( send_buff, WIZ_KNIGHTS_PROCESS, send_index );
	SetByte( send_buff, KNIGHTS_MEMBER_REQ, send_index );
	SetByte( send_buff, ret_value, send_index );
	pUser->Send( send_buff, send_index );
}

void CKnightsManager::CurrentKnightsMember(CUser *pUser, Packet & pkt)
{
	int index = 0, send_index = 0, buff_index = 0, count = 0, i=0, start = 0;
	char send_buff[128], temp_buff[4096], errormsg[1024];
	CUser* pTUser = NULL;
	CKnights* pKnights = NULL;

	if( !pUser ) return;
	if( pUser->m_pUserData->m_bKnights <= 0 ) goto fail_return;
	pKnights = m_pMain->m_KnightsArray.GetData( pUser->m_pUserData->m_bKnights);
	if( !pKnights ) goto fail_return;

	_snprintf(errormsg, sizeof(errormsg), m_pMain->GetServerResource(IDP_KNIGHT_NOT_REGISTERED));
	
	uint16 page = pkt.read<uint16>();
	start = page * 10;			// page : 0 ~

	for (int i = 0; i < MAX_USER; i++)
	{
		pTUser = m_pMain->GetUnsafeUserPtr(i);
		if( !pTUser ) continue;
		if( pTUser->m_pUserData->m_bKnights != pUser->m_pUserData->m_bKnights ) continue;
		if( count < start ) {
			count++;
			continue;
		}
		SetKOString(temp_buff, pUser->m_pUserData->m_id, buff_index);
		SetByte( temp_buff, pUser->m_pUserData->m_bFame, buff_index );
		SetByte( temp_buff, pUser->getLevel(), buff_index );
		SetShort( temp_buff, pUser->m_pUserData->m_sClass, buff_index );
		count++;

		if (count >= start + 10)
			break;
	}

	SetByte( send_buff, WIZ_KNIGHTS_PROCESS, send_index );
	SetByte( send_buff, KNIGHTS_CURRENT_REQ, send_index );
	SetByte( send_buff, 0x01, send_index );
	SetKOString( send_buff, pKnights->m_strChief, send_index );
	SetShort( send_buff, page, send_index );
	SetShort( send_buff, count-start, send_index );
	SetString( send_buff, temp_buff, buff_index, send_index );
	pUser->Send( send_buff, send_index );

fail_return:
	SetByte( send_buff, WIZ_KNIGHTS_PROCESS, send_index );
	SetByte( send_buff, KNIGHTS_CURRENT_REQ, send_index );
	SetByte( send_buff, 0x00, send_index );

	SetKOString( send_buff, errormsg, send_index ); // is this even still usead anymore
	pUser->Send( send_buff, send_index );
}

void CKnightsManager::ReceiveKnightsProcess(CUser* pUser, Packet & pkt)
{
	uint8 command, bResult;

	pkt >> command >> bResult;

	// Surely this isn't still used...
	if (bResult > 0) 
	{
		Packet result(WIZ_KNIGHTS_PROCESS, command);
		result << bResult << "Error";
		pUser->Send(&result);
		return;
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
			CKnights* pKnights = m_pMain->m_KnightsArray.GetData(pUser->m_pUserData->m_bKnights);
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
	m_pMain->m_KnightsArray.PutData(pKnights->m_sIndex, pKnights);
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
	m_pMain->Send_UDP_All(&result, m_pMain->m_nServerGroup == 0 ? 0 : 1);
}

void CKnightsManager::RecvJoinKnights(CUser *pUser, Packet & pkt, BYTE command)
{
	if (pUser == NULL) 
		return;

	uint16 sClanID = pkt.read<uint16>();
	CKnights *pKnights = m_pMain->m_KnightsArray.GetData(sClanID);

	if (command == KNIGHTS_JOIN)
		pKnights->AddUser(pUser);
	else
		pKnights->RemoveUser(pUser);

	Packet result(WIZ_KNIGHTS_PROCESS, command);
	result	<< uint8(1) << pUser->GetSocketID()
			<< pUser->m_pUserData->m_bKnights << pUser->m_pUserData->m_bFame;

	if (pKnights != NULL)
		result << pKnights->m_strName << pKnights->m_byGrade << pKnights->m_byRanking;

	pUser->SendToRegion(&result);

	result.Initialize(UDP_KNIGHTS_PROCESS);
	result << command << sClanID << pUser->m_pUserData->m_id;
	m_pMain->Send_UDP_All(&result, (m_pMain->m_nServerGroup == 0 ? 0 : 1));
}

void CKnightsManager::RecvModifyFame(CUser *pUser, Packet & pkt, BYTE command)
{
	if (pUser == NULL) 
		return;

	CString clanNotice;
	std::string strUserID;
	uint16 sClanID;

	pkt >> sClanID >> strUserID;

	CUser *pTUser = m_pMain->GetUserPtr(strUserID.c_str(), TYPE_CHARACTER);
	CKnights *pKnights = m_pMain->m_KnightsArray.GetData(sClanID);

	switch (command)
	{
	case KNIGHTS_REMOVE:
		if (pTUser != NULL)
		{
			clanNotice = m_pMain->GetServerResource(IDS_KNIGHTS_REMOVE);
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
			clanNotice = m_pMain->GetServerResource(IDS_KNIGHTS_CHIEF);
		}
		break;
	case KNIGHTS_VICECHIEF:
		if (pTUser != NULL)
		{
			pTUser->m_pUserData->m_bFame = VICECHIEF;
			clanNotice = m_pMain->GetServerResource(IDS_KNIGHTS_VICECHIEF);
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
	m_pMain->Send_UDP_All(&result, (m_pMain->m_nServerGroup == 0 ? 0 : 1));

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
	CKnights *pKnights = m_pMain->m_KnightsArray.GetData(sClanID);
	if (pKnights == NULL)
		return;

	pKnights->Disband(pUser);

	Packet result(UDP_KNIGHTS_PROCESS, uint8(KNIGHTS_DESTROY));
	result << sClanID;
	m_pMain->Send_UDP_All(&result, (m_pMain->m_nServerGroup == 0 ? 0 : 1));
}

void CKnightsManager::RecvKnightsList(Packet & pkt)
{
	std::string clanName;
	uint32 nPoints;
	uint16 sClanID, sMembers;
	uint8 bNation, bRank;
	pkt >> sClanID >> bNation >> clanName >> sMembers >> nPoints >> bRank;

	if (m_pMain->m_nServerNo != BATTLE)
		return;

	CKnights *pKnights = m_pMain->m_KnightsArray.GetData(sClanID);
	if (pKnights == NULL)
	{
		pKnights = new CKnights();
		if (!m_pMain->m_KnightsArray.PutData(sClanID, pKnights))
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
	pKnights->m_byGrade = m_pMain->GetKnightsGrade(nPoints);
	pKnights->m_byRanking = bRank;
}

BOOL CKnightsManager::AddKnightsUser(int index, char* UserName)
{
	CKnights *pKnights = m_pMain->m_KnightsArray.GetData(index);
	if (pKnights == NULL)
		return FALSE;

	return pKnights->AddUser(UserName);
}

BOOL CKnightsManager::RemoveKnightsUser(int index, char* UserName)
{
	CKnights *pKnights = m_pMain->m_KnightsArray.GetData(index);
	if (pKnights == NULL)
		return FALSE;

	return pKnights->RemoveUser(UserName);
}

/**
 * This method seems to be useless. Leaving it just in case.
 **/
void CKnightsManager::SetKnightsUser(int index, char* UserName)
{
	CKnights *pKnights = m_pMain->m_KnightsArray.GetData(index);
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

		CKnights *pKnights = m_pMain->m_KnightsArray.GetData(sClanID);
		if (pKnights == NULL)
			continue;

		if (pKnights->m_nPoints != nPoints
			|| pKnights->m_byRanking != bRank)
		{
			pKnights->m_nPoints = nPoints;
			pKnights->m_byRanking = bRank;
			pKnights->m_byGrade = m_pMain->GetKnightsGrade(nPoints);

			result << sClanID << pKnights->m_byGrade << pKnights->m_byRanking;
			send_count++;
		}
	}

	result.put(1, send_count);
	m_pMain->Send_All(&result);
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