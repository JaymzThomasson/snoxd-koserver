#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"

void CUser::Chat(char *pBuf)
{
	Packet result(WIZ_CHAT);
	int index = 0;
	uint8 type;
	CUser* pUser = NULL;
	char chatstr[1024], finalstr[1024]; 
	memset( finalstr, NULL, 1024 );
	std::string buff;

	if (isMuted())
		return;	

	type = GetByte(pBuf, index);
	if (!GetKOString(pBuf, chatstr, index, 512))
		return;


#if 0 // Removed this - all it seems to do is cause chat to break for GMs (is it 19xx+ only?)
	if( isGM() && type == GENERAL_CHAT)
		type = 0x14;
#endif

	uint8 bNation = getNation();
	int16 sessID = int16(GetSocketID());

	// Handle GM notice & announcement commands
	if (type == PUBLIC_CHAT || type == ANNOUNCEMENT_CHAT)
	{
		// Trying to use a GM command without authorisation? Bad player!
		if (!isGM())
			return;

		bNation = KARUS; // arbitrary nation
		sessID = -1;
	}

	result.SByte();
	result << type << bNation << sessID;
	if (type == PUBLIC_CHAT || type == ANNOUNCEMENT_CHAT)
	{
		result << uint8(0); // GM notice/announcements show no name (so specify length of 0)

		// This is horrible, but we'll live with it for now.
		// Pull the notice string (#### NOTICE : %s ####) from the database.
		CString noticeText = m_pMain->GetServerResource(IDP_ANNOUNCEMENT);
		
		// Format the chat string around it, so our chat data is within the notice
		sprintf_s(finalstr, sizeof(finalstr), noticeText, chatstr);
		result.DByte();
		result << finalstr; // now tack on the formatted message from the user
	}
	else
	{
		result << m_pUserData->m_id; // everything else provides a name
		result.DByte();
		result << chatstr; // now tack on the chat message from the user
	}

	switch (type) 
	{
	case GENERAL_CHAT:
		m_pMain->Send_NearRegion(&result, GetMap(), m_RegionX, m_RegionZ, m_pUserData->m_curx, m_pUserData->m_curz);
		break;

	case PRIVATE_CHAT:
		if (m_sPrivateChatUser == GetSocketID()) 
			break;

		pUser = m_pMain->GetUserPtr(m_sPrivateChatUser);
		if (pUser == NULL || pUser->GetState() != STATE_GAMESTART) 
			break;

		pUser->Send(&result);
		break;

	case PARTY_CHAT:
		if (isInParty())
			m_pMain->Send_PartyMember(m_sPartyIndex, &result);
		break;

	case SHOUT_CHAT:
		if (m_pUserData->m_sMp < (m_iMaxMp / 5))
			break;

		MSpChange(-(m_iMaxMp / 5));
		m_pMain->Send_Region(&result, GetMap(), m_RegionX, m_RegionZ);
		break;

	case KNIGHTS_CHAT:
		if (isInClan())
			m_pMain->Send_KnightsMember(m_pUserData->m_bKnights, &result);
		break;
	case PUBLIC_CHAT:
	case ANNOUNCEMENT_CHAT:
		if (isGM())
			m_pMain->Send_All(&result);
		break;
	case COMMAND_CHAT:
		if( m_pUserData->m_bFame == COMMAND_CAPTAIN )
			m_pMain->Send_CommandChat(&result, m_pUserData->m_bNation, this);
		break;
	case MERCHANT_CHAT:
		if (isMerchanting())
			m_pMain->Send_Region(&result, GetMap(), m_RegionX, m_RegionZ);
	break;
	//case WAR_SYSTEM_CHAT:
	//	m_pMain->Send_All(&result);
	//	break;
	}
}

void CUser::ChatTargetSelect(char *pBuf)
{
	int index = 0, send_index = 0, i = 0;
	CUser* pUser = NULL;
	char chatid[MAX_ID_SIZE+1];
	char send_buff[128];

	if (!GetKOString(pBuf, chatid, index, MAX_ID_SIZE))
		return;

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
	if (i == MAX_USER)
		SetKOString(send_buff, "", send_index);
	else
		SetKOString(send_buff, chatid, send_index);

	Send(send_buff, send_index);
}
