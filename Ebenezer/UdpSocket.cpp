// UdpSocket.cpp: implementation of the CUdpSocket class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "define.h"
#include "UdpSocket.h"
#include "EbenezerDlg.h"
#include "AiPacket.h"
#include "Knights.h"
#include "User.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

DWORD WINAPI RecvUDPThread( LPVOID lp )
{
	CUdpSocket *pUdp = (CUdpSocket*)lp;
	int ret = 0, addrlen = sizeof(pUdp->m_ReplyAddress);

	while(1) {
		ret = recvfrom( pUdp->m_hUDPSocket, pUdp->m_pRecvBuf, 1024, 0, (LPSOCKADDR)&pUdp->m_ReplyAddress, &addrlen );

		if(ret == SOCKET_ERROR) {
			int err = WSAGetLastError(); 
			getpeername(pUdp->m_hUDPSocket, (SOCKADDR*)&pUdp->m_ReplyAddress, &addrlen);
			TRACE("recvfrom() error : %d IP : %s\n", err, inet_ntoa(pUdp->m_ReplyAddress.sin_addr));

			// 재전송 루틴...

			Sleep(10);
			continue;
		}

		if( ret ) {
			if( pUdp->PacketProcess(ret) == false ) {
				// broken packet...
			}
		}

		Sleep(10);
	}

	return 1;
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUdpSocket::CUdpSocket(CEbenezerDlg* pMain)
{
	m_hUDPSocket = INVALID_SOCKET;
	memset( m_pRecvBuf, 0x00, 8192 );
	m_pMain = pMain;
}

CUdpSocket::~CUdpSocket()
{

}

bool CUdpSocket::CreateSocket()
{
	m_hUDPSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_hUDPSocket == INVALID_SOCKET) {
	    TRACE("udp socket create fail...\n");
        return false;
    }

	int sock_buf_size = 32768;
    setsockopt(m_hUDPSocket, SOL_SOCKET, SO_RCVBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
    setsockopt(m_hUDPSocket, SOL_SOCKET, SO_SNDBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));

	int optlen = sizeof(sock_buf_size);
    int r = getsockopt(m_hUDPSocket, SOL_SOCKET, SO_RCVBUF, (char *)&sock_buf_size, &optlen);
	if( r == SOCKET_ERROR ) {
		TRACE("buffer size set fail...\n");
		return false;
	}

    memset((char*)&m_SocketAddress, 0x00, sizeof(m_SocketAddress));
    m_SocketAddress.sin_family = AF_INET;
    m_SocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_SocketAddress.sin_port = htons(_UDP_PORT);

    if( bind(m_hUDPSocket, (LPSOCKADDR)&m_SocketAddress, sizeof(m_SocketAddress)) == SOCKET_ERROR ) {
	    TRACE("UDP bind() Error...\n");
		closesocket(m_hUDPSocket);
        return false;
    }

	DWORD id;
	m_hUdpThread = ::CreateThread( NULL, 0, RecvUDPThread, (LPVOID)this, 0, &id);
#ifndef DEBUG
	::SetThreadPriority(m_hUdpThread,THREAD_PRIORITY_ABOVE_NORMAL);
#endif

	TRACE("UDP Socket Create Success...\n");
	return true;
}

int CUdpSocket::SendUDPPacket(char* strAddress, char* pBuf, int len)
{
	int s_size = 0, index = 0;
	
	BYTE pTBuf[2048];

	if( len > 2048 || len <= 0 )
		return 0;

	pTBuf[index++] = (BYTE)PACKET_START1;
	pTBuf[index++] = (BYTE)PACKET_START2;
	memcpy( pTBuf+index, &len, 2 );
	index += 2;
	memcpy( pTBuf+index, pBuf, len );
	index += len;
	pTBuf[index++] = (BYTE)PACKET_END1;
	pTBuf[index++] = (BYTE)PACKET_END2;

    m_SocketAddress.sin_addr.s_addr = inet_addr(strAddress);

	s_size = sendto(m_hUDPSocket, (char*)pTBuf, index, 0, (LPSOCKADDR)&m_SocketAddress, sizeof(m_SocketAddress));

	return s_size;
}

int CUdpSocket::SendUDPPacket(char* strAddress, Packet *pkt)
{
	uint16 len = (uint16)pkt->size() + 1;
	if (len == 0)
		return 0;

	ByteBuffer buff(len + 6);
	buff	<< uint8(PACKET_START1) << uint8(PACKET_START2)
			<< len << pkt->GetOpcode() << *pkt
			<< uint8(PACKET_END1) << uint8(PACKET_END2);

    m_SocketAddress.sin_addr.s_addr = inet_addr(strAddress);
	return sendto(m_hUDPSocket, (const char *)buff.contents(), buff.size(), 0, (LPSOCKADDR)&m_SocketAddress, sizeof(m_SocketAddress));
}

bool CUdpSocket::PacketProcess(int len)
{
	BYTE		*pTmp;
	bool		foundCore;
	MYSHORT		slen;
	int length;

	if( len <= 0 ) return false;

	pTmp = new BYTE[len+1];

	memcpy( pTmp, m_pRecvBuf, len );

	foundCore = FALSE;

	int	sPos=0, ePos = 0;

	for (int i = 0; i < len && !foundCore; i++)
	{
		if (i+2 >= len) break;

		if (pTmp[i] == PACKET_START1 && pTmp[i+1] == PACKET_START2)
		{
			sPos = i+2;

			slen.b[0] = pTmp[sPos];
			slen.b[1] = pTmp[sPos + 1];

			length = slen.w;

			if( length <= 0 ) goto cancelRoutine;
			if( length > len ) goto cancelRoutine;

			ePos = sPos+length + 2;

			if( (ePos + 2) > len ) goto cancelRoutine;

			if (pTmp[ePos] == PACKET_END1 && pTmp[ePos+1] == PACKET_END2)
			{
				Parsing( (char*)(pTmp+sPos+2), length );
				foundCore = TRUE;
				break;
			}
			else 
				goto cancelRoutine;
		}
	}

	delete[] pTmp;

	return foundCore;

cancelRoutine:	
	delete[] pTmp;
	return foundCore;
}

void CUdpSocket::Parsing(char *pBuf, int len)
{
	BYTE command;
	int index = 0;

	command = GetByte( pBuf, index );

	switch( command ) {
	case STS_CHAT:
		ServerChat( pBuf+index );
		break;
	case UDP_BATTLE_EVENT_PACKET:
		RecvBattleEvent( pBuf+index );
		break;
	case UDP_KNIGHTS_PROCESS:
		ReceiveKnightsProcess( pBuf+index );
		break;
	case UDP_BATTLEZONE_CURRENT_USERS:
		RecvBattleZoneCurrentUsers( pBuf+index );
		break;
	}
}

void CUdpSocket::ServerChat(char *pBuf)
{
	int index = 0;
	char chatstr[512];

	if (!GetKOString(pBuf, chatstr, index, sizeof(chatstr)))
		return;

	DEBUG_LOG(chatstr);
}

void CUdpSocket::RecvBattleEvent(char *pBuf)
{
	int index = 0, send_index = 0, udp_index = 0;
	int nType = 0, nResult = 0, nLen = 0, nKillKarus = 0, nElmoKill = 0;
	char strMaxUserName[MAX_ID_SIZE+1], strKnightsName[MAX_ID_SIZE+1];
	char finalstr[1024], send_buff[1024];

	std::string buff;

	nType = GetByte( pBuf, index );
	nResult = GetByte(pBuf, index);

	if( nType == BATTLE_EVENT_OPEN )	{
	}
	else if( nType == BATTLE_MAP_EVENT_RESULT )	{
		if( m_pMain->m_byBattleOpen == NO_BATTLE )	{
			TRACE("#### UDP RecvBattleEvent Fail : battleopen = %d, type = %d\n", m_pMain->m_byBattleOpen, nType);
			return;
		}
		if( nResult == KARUS )	{
			//TRACE("--> UDP RecvBattleEvent : 카루스 땅으로 넘어갈 수 있어\n");
			m_pMain->m_byKarusOpenFlag = 1;		// 카루스 땅으로 넘어갈 수 있어
		}
		else if( nResult == ELMORAD )	{
			//TRACE("--> UDP  RecvBattleEvent : 엘모 땅으로 넘어갈 수 있어\n");
			m_pMain->m_byElmoradOpenFlag = 1;	// 엘모 땅으로 넘어갈 수 있어
		}
	}
	else if( nType == BATTLE_EVENT_RESULT )	{
		if( m_pMain->m_byBattleOpen == NO_BATTLE )	{
			TRACE("####  UDP  RecvBattleEvent Fail : battleopen = %d, type=%d\n", m_pMain->m_byBattleOpen, nType);
			return;
		}
		if( nResult == KARUS )	{
			//TRACE("-->  UDP RecvBattleEvent : 카루스가 승리하였습니다.\n");
		}
		else if( nResult == ELMORAD )	{
			//TRACE("-->  UDP RecvBattleEvent : 엘모라드가 승리하였습니다.\n");
		}

		m_pMain->m_bVictory = nResult;
		m_pMain->m_byOldVictory = nResult;
		m_pMain->m_byKarusOpenFlag = 0;		// 카루스 땅으로 넘어갈 수 없도록
		m_pMain->m_byElmoradOpenFlag = 0;	// 엘모 땅으로 넘어갈 수 없도록
		m_pMain->m_byBanishFlag = 1;
	}
	else if( nType == BATTLE_EVENT_MAX_USER )	{
		nLen = GetByte(pBuf, index);
		if (!GetKOString(pBuf, strKnightsName, index, MAX_ID_SIZE)
			|| GetKOString(pBuf, strMaxUserName, index, MAX_ID_SIZE))
			return;

		int nResourceID = 0;
		switch (nResult)
		{
		case 1: // captain
			nResourceID = IDS_KILL_CAPTAIN;
			break;
		case 2: // keeper

		case 7: // warders?
		case 8:
			nResourceID = IDS_KILL_GATEKEEPER;
			break;

		case 3: // Karus sentry
			nResourceID = IDS_KILL_KARUS_GUARD1;
			break;
		case 4: // Karus sentry
			nResourceID = IDS_KILL_KARUS_GUARD2;
			break;
		case 5: // El Morad sentry
			nResourceID = IDS_KILL_ELMO_GUARD1;
			break;
		case 6: // El Morad sentry
			nResourceID = IDS_KILL_ELMO_GUARD2;
			break;
		}

		if (nResourceID == 0)
		{
			TRACE("RecvBattleEvent: could not establish resource for result %d", nResult);
			return;
		}

		_snprintf(finalstr, sizeof(finalstr), m_pMain->GetServerResource(nResourceID), strKnightsName, strMaxUserName);

		SetByte( send_buff, WIZ_CHAT, send_index );
		SetByte( send_buff, WAR_SYSTEM_CHAT, send_index );
		SetByte( send_buff, 1, send_index );
		SetShort( send_buff, -1, send_index );
		SetKOString( send_buff, finalstr, send_index );
		m_pMain->Send_All( send_buff, send_index );

		send_index = 0;
		SetByte( send_buff, WIZ_CHAT, send_index );
		SetByte( send_buff, PUBLIC_CHAT, send_index );
		SetByte( send_buff, 1, send_index );
		SetShort( send_buff, -1, send_index );
		SetKOString( send_buff, finalstr, send_index );
		m_pMain->Send_All( send_buff, send_index );
	}
	else if( nType == BATTLE_EVENT_KILL_USER )	{
		if( nResult == 1 )	{
			nKillKarus = GetShort( pBuf, index );
			nElmoKill = GetShort( pBuf, index );
			m_pMain->m_sKarusDead = m_pMain->m_sKarusDead + nKillKarus;
			m_pMain->m_sElmoradDead = m_pMain->m_sElmoradDead + nElmoKill;

			//TRACE("-->  UDP RecvBattleEvent type = 1 : 적국 유저 죽인수 : karus=%d->%d, elmo=%d->%d\n", nKillKarus, m_pMain->m_sKarusDead, nElmoKill, m_pMain->m_sElmoradDead);
			Packet result(UDP_BATTLE_EVENT_PACKET, uint8(BATTLE_EVENT_KILL_USER));
			result << uint8(2) << m_pMain->m_sKarusDead << m_pMain->m_sElmoradDead;
			m_pMain->Send_UDP_All(&result);
		}
		else if( nResult == 2 )	{
			nKillKarus = GetShort( pBuf, index );
			nElmoKill = GetShort( pBuf, index );

			//TRACE("-->  UDP RecvBattleEvent type = 2 : 적국 유저 죽인수 : karus=%d->%d, elmo=%d->%d\n", m_pMain->m_sKarusDead, nKillKarus, m_pMain->m_sElmoradDead, nElmoKill);

			m_pMain->m_sKarusDead = nKillKarus;
			m_pMain->m_sElmoradDead = nElmoKill;
		}
	}

}

/***
 * Another server has informed us of a clan event.
 ***/
void CUdpSocket::ReceiveKnightsProcess(char* pBuf)
{
	int index = 0, command = 0, pktsize = 0, count = 0;

	command = GetByte( pBuf, index );
	//TRACE("UDP - ReceiveKnightsProcess - command=%d\n", command);

	switch(command) {
	case KNIGHTS_CREATE:
		RecvCreateKnights( pBuf+index );
		break;
	case KNIGHTS_JOIN:
	case KNIGHTS_WITHDRAW:
		RecvJoinKnights( pBuf+index, command );
		break;
	case KNIGHTS_REMOVE:
	case KNIGHTS_ADMIT:
	case KNIGHTS_REJECT:
	case KNIGHTS_CHIEF:
	case KNIGHTS_VICECHIEF:
	case KNIGHTS_OFFICER:
	case KNIGHTS_PUNISH:
		RecvModifyFame( pBuf+index, command );
		break;
	case KNIGHTS_DESTROY:
		RecvDestroyKnights( pBuf+index );
		break;
	}
}

/***
 * We've been told from another server that a clan has been created. 
 ***/
void CUdpSocket::RecvCreateKnights(char* pBuf)
{
	int index = 0, send_index = 0, knightsindex = 0, nation = 0, community = 0;
	char knightsname[MAX_ID_SIZE+1], chiefname[MAX_ID_SIZE+1];
	CKnights* pKnights = NULL;

	community = GetByte( pBuf, index );
	knightsindex = GetShort( pBuf, index );
	nation = GetByte( pBuf, index );
	if (!GetKOString(pBuf, knightsname, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, chiefname, index, MAX_ID_SIZE))
		return;

	pKnights = new CKnights();

	pKnights->m_sIndex = knightsindex;
	pKnights->m_byFlag = community;
	pKnights->m_byNation = nation;
	strcpy(pKnights->m_strName, knightsname);
	strcpy(pKnights->m_strChief, chiefname);
	pKnights->AddUser(chiefname);

	m_pMain->m_KnightsArray.PutData( pKnights->m_sIndex, pKnights );

	//TRACE("UDP - RecvCreateKnights - knname=%s, name=%s, index=%d\n", knightsname, chiefname, knightsindex);
}

/***
 * We've been told from another server that someone has joined (or rejected joining) a clan.
 ***/
void CUdpSocket::RecvJoinKnights(char* pBuf, BYTE command)
{
	int knightsindex = 0, index = 0;
	char charid[MAX_ID_SIZE+1];
	CString clanNotice;

	knightsindex = GetShort( pBuf, index );
	if (!GetKOString(pBuf, charid, index, MAX_ID_SIZE))
		return;

	CKnights *pKnights = m_pMain->GetClanPtr(knightsindex);
	if (pKnights == NULL)
		return;

	if (command == KNIGHTS_JOIN)
	{	
		clanNotice = m_pMain->GetServerResource(IDS_KNIGHTS_JOIN);
		pKnights->AddUser(charid);
		TRACE("UDP - RecvJoinKnights - name=%s, index=%d\n", charid, knightsindex);
	}
	else 
	{
		clanNotice = m_pMain->GetServerResource(IDS_KNIGHTS_WITHDRAW);
		pKnights->RemoveUser(charid);
		TRACE("UDP - RecvJoinKnights - name=%s, index=%d\n", charid, knightsindex );
	}

	pKnights->SendChat(clanNotice, charid);
}

/***
 * We've been told from another server that a user's status in the clan has changed.
 ***/
void CUdpSocket::RecvModifyFame(char* pBuf, BYTE command)
{
	CString clanNotice;
	int index = 0, knightsindex = 0, vicechief = 0;
	char userid[MAX_ID_SIZE+1];

	knightsindex = GetShort(pBuf, index);
	if (!GetKOString(pBuf, userid, index, MAX_ID_SIZE))
		return;

	CUser *pTUser = m_pMain->GetUserPtr(userid, TYPE_CHARACTER);
	CKnights *pKnights = m_pMain->GetClanPtr(knightsindex);
	if (pKnights == NULL)
		return;

	switch (command)
	{
	case KNIGHTS_REMOVE:
	case KNIGHTS_REJECT:
		if (pTUser)
		{
			pTUser->m_pUserData->m_bKnights = 0;
			pTUser->m_pUserData->m_bFame = 0;

			if (command == KNIGHTS_REMOVE)
				clanNotice = m_pMain->GetServerResource(IDS_KNIGHTS_REMOVE);
		}

		m_pMain->m_KnightsManager.RemoveKnightsUser(knightsindex, userid);
		break;

	case KNIGHTS_ADMIT:
		if (pTUser)
			pTUser->m_pUserData->m_bFame = KNIGHT;
		break;

	case KNIGHTS_CHIEF+0x10:
		if (pTUser)
		{
			pTUser->m_pUserData->m_bFame = CHIEF;
			clanNotice = m_pMain->GetServerResource(IDS_KNIGHTS_CHIEF);
		}
		break;

	case KNIGHTS_VICECHIEF+0x10:
		if (pTUser)
		{
			pTUser->m_pUserData->m_bFame = VICECHIEF;
			clanNotice = m_pMain->GetServerResource(IDS_KNIGHTS_VICECHIEF);
		}
		break;

	case KNIGHTS_OFFICER+0x10:
		if (pTUser)
			pTUser->m_pUserData->m_bFame = OFFICER;
		break;

	case KNIGHTS_PUNISH+0x10:
		if (pTUser)
			pTUser->m_pUserData->m_bFame = PUNISH;
		break;
	}

	if (pTUser != NULL)
		pTUser->SendClanUserStatusUpdate(command == KNIGHTS_REMOVE);

	if (clanNotice.GetLength() == 0)
		return;

	Packet result;

	// Construct the clan system chat packet
	pKnights->ConstructChatPacket(result, clanNotice, pTUser != NULL ? pTUser->m_pUserData->m_id : userid); 

	// If we've been removed from a clan, tell the user as well (since they're no longer in the clan)
	if (command == KNIGHTS_REMOVE && pTUser != NULL)
		pTUser->Send(&result);

	// Otherwise, since we're actually in the clan, we don't need to be explicitly told what happened.
	if (pKnights != NULL)
		pKnights->Send(&result);
}

/***
 * We've been told from another server that the clan is disbanding 
 ***/
void CUdpSocket::RecvDestroyKnights(char* pBuf)
{
	int index = 0;
	int16 knightsindex = GetShort(pBuf, index);
	CKnights *pKnights = m_pMain->GetClanPtr(knightsindex);
	if (pKnights == NULL)
	{
		TRACE("UDP - ### RecvDestoryKnights  Fail == index = %d ###\n", knightsindex);
		return;
	}

	pKnights->Disband();
}

/***
 * Another server has updated our war player counts.
 ***/
void CUdpSocket::RecvBattleZoneCurrentUsers( char* pBuf )
{
	int nKarusMan = 0, nElmoradMan = 0, index = 0;

	nKarusMan = GetShort( pBuf, index );
	nElmoradMan = GetShort( pBuf, index );

	m_pMain->m_sKarusCount = nKarusMan;
	m_pMain->m_sElmoradCount = nElmoradMan;
	//TRACE("UDP - RecvBattleZoneCurrentUsers - karus=%d, elmorad=%d\n", nKarusMan, nElmoradMan);
}
