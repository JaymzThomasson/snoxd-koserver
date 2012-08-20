#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "Ebenezer.h"
#include "EbenezerDlg.h"
#include "User.h"

void CUser::Chat(char *pBuf)
{
	int index = 0, send_index = 0;
	BYTE type;
	CUser* pUser = NULL;
	char chatstr[1024]; memset( chatstr, NULL, 1024 );
	char finalstr[1024]; memset( finalstr, NULL, 1024 );
	char send_buff[1024]; memset( send_buff, NULL, 1024 );

	std::string buff;

	if (isMuted())
		return;	

	type = GetByte(pBuf, index);
	if (!GetKOString(pBuf, chatstr, index, 512))
		return;

	SetByte(send_buff, WIZ_CHAT, send_index);

	if (type != PUBLIC_CHAT)
	{
		SetByte(send_buff, type, send_index);
		SetByte(send_buff, getNation(), send_index);
		SetShort(send_buff, GetSocketID(), send_index);
		SetKOString(send_buff, m_pUserData->m_id, send_index, 1);
		SetKOString(send_buff, chatstr, send_index);
	}

	switch (type) 
	{
	case GENERAL_CHAT:
		m_pMain->Send_NearRegion(send_buff, send_index, GetMap(), m_RegionX, m_RegionZ, m_pUserData->m_curx, m_pUserData->m_curz);
		break;

	case PRIVATE_CHAT:
		if (m_sPrivateChatUser == GetSocketID()) 
			break;

		pUser = m_pMain->GetUserPtr(m_sPrivateChatUser);
		if (pUser == NULL || pUser->GetState() != STATE_GAMESTART) 
			break;

		pUser->Send(send_buff, send_index);
		// Send(send_buff, send_index);
		break;

	case PARTY_CHAT:
		if (isInParty())
			m_pMain->Send_PartyMember(m_sPartyIndex, send_buff, send_index );
		break;

	case SHOUT_CHAT:
		if (m_pUserData->m_sMp < (m_iMaxMp / 5))
			break;

		MSpChange(-(m_iMaxMp / 5));
		m_pMain->Send_Region(send_buff, send_index, GetMap(), m_RegionX, m_RegionZ, NULL, false);
		break;

	case KNIGHTS_CHAT:
		if (isInClan())
			m_pMain->Send_KnightsMember(m_pUserData->m_bKnights, send_buff, send_index, m_pUserData->m_bZone);
		break;
	case PUBLIC_CHAT:
		if (isGM())
			m_pMain->Send_All( send_buff, send_index );
		break;
	case COMMAND_CHAT:
		if( m_pUserData->m_bFame == COMMAND_CAPTAIN )
			m_pMain->Send_CommandChat( send_buff, send_index, m_pUserData->m_bNation, this );
		break;
	//case WAR_SYSTEM_CHAT:
	//	m_pMain->Send_All( send_buff, send_index );
	//	break;
	}
}

void CUser::ChatTargetSelect(char *pBuf)
{
	int index = 0, send_index = 0, idlen = 0, i = 0;
	CUser* pUser = NULL;
	char chatid[MAX_ID_SIZE+1]; memset( chatid, 0x00, MAX_ID_SIZE+1 );
	char send_buff[128]; memset( send_buff, 0x00, 128 );

	idlen = GetShort( pBuf, index );
	if( idlen > MAX_ID_SIZE || idlen < 0 ) return;
	GetString( chatid, pBuf, idlen, index );

	m_pMain->GetUserPtr(chatid, TYPE_CHARACTER);
	for (i = 0; i < MAX_USER; i++)
	{
		pUser = m_pMain->GetUnsafeUserPtr(i); 
		if (pUser && pUser->GetState() == STATE_GAMESTART)
		{
			if (_strnicmp(chatid, pUser->m_pUserData->m_id, MAX_ID_SIZE) == 0)
			{
				m_sPrivateChatUser = i;
				break;
			}
		}
	}

	SetByte( send_buff, WIZ_CHAT_TARGET, send_index );
	if( i == MAX_USER )
		SetShort( send_buff, 0, send_index );
	else {
		SetShort( send_buff, strlen( chatid ), send_index );
		SetString( send_buff, chatid, strlen( chatid ), send_index );
	}
	Send( send_buff, send_index );
}
