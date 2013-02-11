#include "StdAfx.h"

void CUser::Chat(Packet & pkt)
{
	Packet result(WIZ_CHAT);
	uint8 type = pkt.read<uint8>();
	char finalstr[1024] = ""; 
	std::string buff, chatstr;

	if (isMuted())
		return;	

	pkt >> chatstr;
	if (chatstr.empty() || chatstr.size() >= 128)
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
		sprintf_s(finalstr, sizeof(finalstr), noticeText, chatstr.c_str());
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
	{
		if (m_sPrivateChatUser == GetSocketID()) 
			break;

		CUser *pUser = m_pMain->GetUserPtr(m_sPrivateChatUser);
		if (pUser == NULL || pUser->GetState() != STATE_GAMESTART) 
			break;

		pUser->Send(&result);
	} break;

	case PARTY_CHAT:
		if (isInParty())
			m_pMain->Send_PartyMember(m_sPartyIndex, &result);
		break;

	case SHOUT_CHAT:
		if (m_pUserData->m_sMp < (m_iMaxMp / 5))
			break;

		// Characters under level 35 require 3,000 coins to shout.
		if (!isGM()
			&& getLevel() < 35
			&& !GoldLose(SHOUT_COIN_REQUIREMENT))
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

void CUser::ChatTargetSelect(Packet & pkt)
{
	uint8 type = pkt.read<uint8>();

	// TO-DO: Replace this with an enum
	// Attempt to find target player in-game
	if (type == 1)
	{
		Packet result(WIZ_CHAT_TARGET, type);
		std::string strUserID;
		if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE)
			return;

		CUser *pUser = m_pMain->GetUserPtr(strUserID.c_str(), TYPE_CHARACTER);
		if (pUser == NULL || pUser == this)
		{
			result << int16(0); 
		}
/*		TO-DO: Implement PM blocking
		else if (pUser->isBlockingPMs())
		{
			result << int16(-1);
		}
*/
		else
		{
			m_sPrivateChatUser = pUser->GetSocketID();
			result << int16(0) << pUser->m_pUserData->m_id;
		}
		Send(&result);
	}
	// Allow/block PMs
	else
	{
		// m_bAllowPrivateChat = GetByte(pBuf, index); 
	}
}
