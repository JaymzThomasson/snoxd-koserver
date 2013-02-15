#include "StdAfx.h"

extern BYTE g_serverdown_flag;

ServerCommandTable CEbenezerDlg::s_commandTable;
ChatCommandTable CUser::s_commandTable;

void CEbenezerDlg::InitServerCommands()
{
	static Command<CEbenezerDlg> commandTable[] = 
	{
		// Command				Handler											Help message
		{ "kill",				&CEbenezerDlg::HandleKillUserCommand,			"Disconnects the specified player" },
		{ "open1",				&CEbenezerDlg::HandleWar1OpenCommand,			"Opens war zone 1" },
		{ "open2",				&CEbenezerDlg::HandleWar2OpenCommand,			"Opens war zone 2" },
		{ "open3",				&CEbenezerDlg::HandleWar3OpenCommand,			"Opens war zone 3" },
		{ "snowopen",			&CEbenezerDlg::HandleSnowWarOpenCommand,		"Opens the snow war zone" },
		{ "close",				&CEbenezerDlg::HandleWarCloseCommand,			"Closes the active war zone" },
		{ "down",				&CEbenezerDlg::HandleShutdownCommand,			"Shuts down the server" },
		{ "pause",				&CEbenezerDlg::HandlePauseCommand,				"Prevents users from connecting to the server" },
		{ "resume",				&CEbenezerDlg::HandleResumeCommand,				"Allows users to resume connecting to the server" },
		{ "discount",			&CEbenezerDlg::HandleDiscountCommand,			"Enables server discounts for the winning nation of the last war" },
		{ "alldiscount",		&CEbenezerDlg::HandleGlobalDiscountCommand,		"Enables server discounts for everyone" },
		{ "offdiscount",		&CEbenezerDlg::HandleDiscountOffCommand,		"Disables server discounts" },
		{ "captain",			&CEbenezerDlg::HandleCaptainCommand,			"Sets the captains/commanders for the war" },
		{ "santa",				&CEbenezerDlg::HandleSantaCommand,				"Enables a flying Santa Claus." },
		{ "offsanta",			&CEbenezerDlg::HandleSantaOffCommand,			"Disables a flying Santa Claus." },
		{ "permanent",			&CEbenezerDlg::HandlePermanentChatCommand,		"Sets the permanent chat bar to the specified text." },
		{ "offpermanent",		&CEbenezerDlg::HandlePermanentChatOffCommand,	"Resets the permanent chat bar text." },
	};

	init_command_table(CEbenezerDlg, commandTable, s_commandTable);
}

void CEbenezerDlg::CleanupServerCommands() { free_command_table(s_commandTable); }

void CUser::InitChatCommands()
{
	static Command<CUser> commandTable[] = 
	{
		// Command				Handler											Help message
		{ "give_item",			&CUser::HandleGiveItemCommand,					"Gives a player an item. Arguments: character name | item ID | [optional stack size]" },
		{ "zonechange",			&CUser::HandleZoneChangeCommand,				"Teleports you to the specified zone. Arguments: zone ID" },

	};

	init_command_table(CUser, commandTable, s_commandTable);
}

void CUser::CleanupChatCommands() { free_command_table(s_commandTable); }

void CUser::Chat(Packet & pkt)
{
	Packet result(WIZ_CHAT);
	uint8 type = pkt.read<uint8>();
	char finalstr[1024] = ""; 
	std::string buff, chatstr;

	if (isMuted())
		return;	

	pkt >> chatstr;
	if (chatstr.empty() || chatstr.size() > 128)
		return;

	// Process GM commands
	if (isGM() && ProcessChatCommand(chatstr))
		return;

#if 0 // Removed this - all it seems to do is cause chat to break for GMs (is it 19xx+ only?)
	if( isGM() && type == GENERAL_CHAT)
		type = 0x14;
#endif

	uint8 bNation = getNation();
	uint16 sessID = GetSocketID();

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
		if (getFame() == COMMAND_CAPTAIN)
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
		pkt >> strUserID;
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

bool CUser::ProcessChatCommand(std::string & message)
{
	// Commands require at least 2 characters
	if (message.size() <= 1
		// If the prefix isn't correct
		|| message[0] != CHAT_COMMAND_PREFIX
		// or if we're saying, say, ++++ (faster than looking for the command in the map)
		|| message[1] == CHAT_COMMAND_PREFIX)
		// we're not a command.
		return false;

	// Split up the command by spaces
	CommandArgs vargs = StrSplit(message, " ");
	std::string command = vargs.front(); // grab the first word (the command)
	vargs.pop_front(); // remove the command from the argument list

	// Make the command lowercase, for 'case-insensitive' checking.
	STRTOLOWER(command);

	// Command doesn't exist
	ChatCommandTable::iterator itr = s_commandTable.find(command.c_str() + 1); // skip the prefix character
	if (itr == s_commandTable.end())
		return false;

	// Run the command
	return (this->*(itr->second->Handler))(vargs, message.c_str() + command.size() + 1, itr->second->Help);
}

COMMAND_HANDLER(CUser::HandleGiveItemCommand)
{
	// Char name | item ID | [stack size]
	if (vargs.size() < 2)
	{
		// send description
		return true;
	}

	std::string strUserID = vargs.front();
	vargs.pop_front();

	CUser *pUser = m_pMain->GetUserPtr(strUserID.c_str(), TYPE_CHARACTER);
	if (pUser == NULL)
	{
		// send error message saying the character does not exist or is not online
		return true;
	}

	uint32 nItemID = atoi(vargs.front().c_str());
	vargs.pop_front();
	_ITEM_TABLE *pItem = m_pMain->GetItemPtr(nItemID);
	if (pItem == NULL)
	{
		// send error message saying the item does not exist
		return true;
	}

	uint16 sCount = 1;
	if (!vargs.empty())
		sCount = atoi(vargs.front().c_str());


	if (!pUser->GiveItem(nItemID, sCount))
	{
		// send error message saying the item couldn't be added
	}

	return true;
}

COMMAND_HANDLER(CUser::HandleZoneChangeCommand)
{
	if (vargs.empty())
	{
		// send description
		return true;
	}

	// Behave as in official (we'll fix this later)
	int nZoneID = atoi(vargs.front().c_str());
	ZoneChange(nZoneID, m_pUserData->m_curx, m_pUserData->m_curz);
	return true;
}

bool CEbenezerDlg::ProcessServerCommand(std::string & message)
{
	// Commands require at least 2 characters
	if (message.size() <= 1
		// If the prefix isn't correct
		|| message[0] != SERVER_COMMAND_PREFIX)
		// we're not a command.
		return false;

	// Split up the command by spaces
	CommandArgs vargs = StrSplit(message, " ");
	std::string command = vargs.front(); // grab the first word (the command)
	vargs.pop_front(); // remove the command from the argument list

	// Make the command lowercase, for 'case-insensitive' checking.
	STRTOLOWER(command);

	// Command doesn't exist
	ServerCommandTable::iterator itr = s_commandTable.find(command.c_str() + 1); // skip the prefix character
	if (itr == s_commandTable.end())
		return false;

	// Run the command
	return (this->*(itr->second->Handler))(vargs, message.c_str() + command.size() + 1, itr->second->Help);
}

COMMAND_HANDLER(CEbenezerDlg::HandleKillUserCommand)
{
	if (vargs.empty())
	{
		// send error saying we need another argument
		return true;
	}

	std::string strUserID = vargs.front();
	CUser *pUser = GetUserPtr(strUserID.c_str(), TYPE_CHARACTER);
	if (pUser == NULL)
	{
		// send error saying that user was not found
		return true;
	}
	
	// Disconnect the player
	pUser->CloseProcess();

	// send a message saying the player was disconnected
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandleWar1OpenCommand)
{
	BattleZoneOpen(BATTLEZONE_OPEN, 1);
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandleWar2OpenCommand)
{
	BattleZoneOpen(BATTLEZONE_OPEN, 2);
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandleWar3OpenCommand)
{
	BattleZoneOpen(BATTLEZONE_OPEN, 3);
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandleSnowWarOpenCommand)
{
	BattleZoneOpen(SNOW_BATTLEZONE_OPEN);
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandleWarCloseCommand)
{
	m_byBanishFlag = 1;
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandleShutdownCommand)
{
	g_serverdown_flag = TRUE;
	SuspendThread(m_Iocport.m_hAcceptThread);
	AddToList("Server shutdown, %d users kicked out.", KickOutAllUsers());
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandlePauseCommand)
{
	SuspendThread(m_Iocport.m_hAcceptThread);
	AddToList("Server no longer accepting connections.");
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandleResumeCommand)
{
	ResumeThread(m_Iocport.m_hAcceptThread);
	AddToList("Server accepting connections.");
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandleDiscountCommand)
{
	m_sDiscount = 1;
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandleGlobalDiscountCommand)
{
	m_sDiscount = 2;
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandleDiscountOffCommand)
{
	m_sDiscount = 0;
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandleCaptainCommand)
{
	LoadKnightsRankTable();
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandleSantaCommand)
{
	m_bSanta = TRUE;
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandleSantaOffCommand)
{
	m_bSanta = FALSE;
	return true;
}

COMMAND_HANDLER(CEbenezerDlg::HandlePermanentChatCommand)
{
	if (vargs.empty())
	{
		// send error saying we need args (unlike the previous implementation of this command)
		return true;
	}

	SetPermanentMessage("%s", args);
	return true;
}

void CEbenezerDlg::GetPermanentMessage(Packet & result)
{
	result  << uint8(PERMANENT_CHAT)	 // chat type 
			<< uint8(1)					 // nation
			<< int16(-1)				 // session ID
			<< uint8(0)					 // character name length
			<< m_strPermanentChat;		 // chat message
}

void CEbenezerDlg::SetPermanentMessage(const char * format, ...)
{
	Packet data(WIZ_CHAT);
	char buffer[128];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buffer, 128, format, ap);
	va_end(ap);

	m_bPermanentChatMode = TRUE;
	m_strPermanentChat = buffer;

	GetPermanentMessage(data); 
	Send_All(&data);
}

COMMAND_HANDLER(CEbenezerDlg::HandlePermanentChatOffCommand)
{
	Packet data(WIZ_CHAT, uint8(END_PERMANENT_CHAT));

	data  << uint8(1)				// nation
		  << int16(-1)				// session ID
		  << uint8(0)				// character name length
		  << uint16(0);				// chat message

	m_bPermanentChatMode = FALSE;
	Send_All(&data);
	return true;
}

