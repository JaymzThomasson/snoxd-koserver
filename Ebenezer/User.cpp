CUser::CUser(uint16 socketID, SocketMgr *mgr) : KOSocket(socketID, mgr, -1, 16384, 3172), Unit(true)
{
}

/**
 * @brief	Executes the connect action.
 */
void CUser::OnConnect()
{
	KOSocket::OnConnect();
	Initialize();
}

/**
 * @brief	Initializes this object.
 */
void CUser::Initialize()
{
	Unit::Initialize();

	// memset(&m_pUserData, 0x00, sizeof(_USER_DATA));
	m_strUserID.clear();
	m_strAccountID.clear();
	m_bLogout = 0;

	m_bAuthority = 1;
	m_sBind = -1;

	m_state = GAME_STATE_CONNECTED;

	m_bSelectedCharacter = false;
	m_bStoreOpen = false;
	m_bPartyLeader = false;
	m_bIsChicken = false;
	m_bIsHidingHelmet = false;
	m_bMining = false;
	m_bPremiumMerchant = false;
	m_bIsBlinded = false;
	m_bInstantCast = false;

	m_tLastMiningAttempt = 0;

	m_bMerchantState = MERCHANT_STATE_NONE;
	m_bInvisibilityType = INVIS_NONE;

	m_sDirection = 0;

	m_sItemMaxHp = 0;
	m_sItemMaxMp = 0;
	m_sItemWeight = 0;
	m_sItemHit = 0;
	m_sItemAc = 0;

	memset(&m_sStatItemBonuses, 0, sizeof(m_sStatItemBonuses));
	memset(&m_bStatBuffs, 0, sizeof(m_bStatBuffs));

	m_sItemHitrate = 100;
	m_sItemEvasionrate = 100;

	m_sSpeed = 0;

	m_iMaxHp = 0;
	m_iMaxMp = 1;
	m_iMaxExp = 0;
	m_sMaxWeight = 0;

	m_bResHpType = USER_STANDING;
	m_bWarp = 0x00;

	m_sMerchantsSocketID = -1;
	m_sChallengeUser = -1;
	m_sPartyIndex = -1;		
	m_sExchangeUser = -1;
	m_bRequestingChallenge = 0;
	m_bChallengeRequested = 0;
	m_bExchangeOK = 0x00;
	m_bBlockPrivateChat = false;
	m_sPrivateChatUser = -1;
	m_bNeedParty = 0x01;

	m_tHPLastTimeNormal = 0;		// For Automatic HP recovery. 
	m_tHPStartTimeNormal = 0;
	m_bHPAmountNormal = 0;
	m_bHPDurationNormal = 0;
	m_bHPIntervalNormal = 5;

	m_tAreaLastTime = 0;		// For Area Damage spells Type 3.
	m_tAreaStartTime = 0;
	m_bAreaInterval = 5;
	m_iAreaMagicID = 0;

	m_fSpeedHackClientTime = 0;
	m_fSpeedHackServerTime = 0;
	m_bSpeedHackCheck = 0;

	m_tBlinkExpiryTime = 0;

	m_sAliveCount = 0;
	m_bAbnormalType = ABNORMAL_NORMAL;	// User starts out in normal size.

	m_sWhoKilledMe = -1;
	m_iLostExp = 0;

	m_tLastTrapAreaTime = 0;

	memset(m_iSelMsgEvent, -1,  MAX_MESSAGE_EVENT);

	m_sEventNid = m_sEventSid = -1;
	m_nQuestHelperID = 0;
	m_bZoneChangeFlag = false;
	m_bRegeneType = 0;
	m_tLastRegeneTime = 0;
	m_bZoneChangeSameZone = false;

	m_nTransformationItem = 0;
	m_tTransformationStartTime = 0;
	m_sTransformationDuration = 0;

	memset(&m_bKillCounts, 0, sizeof(m_bKillCounts));
	m_sEventDataIndex = 0;
}

/**
 * @brief	Executes the disconnect action.
 */
void CUser::OnDisconnect()
{
	KOSocket::OnDisconnect();

	g_pMain->RemoveSessionNames(this);

	if (isInGame())
	{
		UserInOut(INOUT_OUT);

		if (isInParty())
			PartyRemove(GetSocketID());

		if (isInClan())
		{
			CKnights *pKnights = g_pMain->GetClanPtr(GetClanID());
			if (pKnights != NULL)
				pKnights->OnLogout(this);
		}

		ResetWindows();
	}
	LogOut();
}

/**
 * @brief	Handles an incoming user packet.
 *
 * @param	pkt	The packet.
 *
 * @return	true if it succeeds, false if it fails.
 */
bool CUser::HandlePacket(Packet & pkt)
{
	uint8 command = pkt.GetOpcode();
	TRACE("[SID=%d] Packet: %X (len=%d)\n", GetSocketID(), command, pkt.size());
	// If crypto's not been enabled yet, force the version packet to be sent.
	if (!isCryptoEnabled())
	{
		if (command == WIZ_VERSION_CHECK)
			VersionCheck(pkt);

		return true;
	}
	// If we're not authed yet, forced us to before we can do anything else.
	// NOTE: We're checking the account ID store here because it's only set on successful auth,
	// at which time the other account ID will be cleared out (yes, it's messy -- need to clean it up).
	else if (m_strAccountID.empty())
	{
		if (command == WIZ_LOGIN)
			LoginProcess(pkt);

		return true;
	}
	// If we haven't logged in yet, don't let us hit in-game packets.
	// TO-DO: Make sure we support all packets in the loading stage (and rewrite this logic considerably better).
	else if (!m_bSelectedCharacter)
	{
		switch (command)
		{
		case WIZ_SEL_NATION:
			SelNationToAgent(pkt);
			break;
		case WIZ_ALLCHAR_INFO_REQ:
			AllCharInfoToAgent();
			break;
		case WIZ_CHANGE_HAIR:
			ChangeHair(pkt);
			break;
		case WIZ_NEW_CHAR:
			NewCharToAgent(pkt);
			break;
		case WIZ_DEL_CHAR:
			DelCharToAgent(pkt);
			break;
		case WIZ_SEL_CHAR:
			SelCharToAgent(pkt);
			break;
		case WIZ_SPEEDHACK_CHECK:
			SpeedHackTime(pkt);
			break;
		default:
			TRACE("[SID=%d] Unhandled packet (%X) prior to selecting character\n", GetSocketID(), command);
			break;
		}
		return true;
	}

	// Otherwise, assume we're authed & in-game.
	switch (command)
	{
	case WIZ_GAMESTART:
		GameStart(pkt);
		break;
	case WIZ_SERVER_INDEX:
		SendServerIndex();
		break;
	case WIZ_RENTAL:
		RentalSystem(pkt);
		break;
	case WIZ_SKILLDATA:
		SkillDataProcess(pkt);
		break;
	case WIZ_MOVE:
		MoveProcess(pkt);
		break;
	case WIZ_ROTATE:
		Rotate(pkt);
		break;
	case WIZ_ATTACK:
		Attack(pkt);
		break;
	case WIZ_CHAT:
		Chat(pkt);
		break;
	case WIZ_CHAT_TARGET:
		ChatTargetSelect(pkt);
		break;
	case WIZ_REGENE:	
		Regene(pkt.read<uint8>()); // respawn type
		break;
	case WIZ_REQ_USERIN:
		RequestUserIn(pkt);
		break;
	case WIZ_REQ_NPCIN:
		RequestNpcIn(pkt);
		break;
	case WIZ_WARP:
		if (isGM())
			RecvWarp(pkt);
		break;
	case WIZ_ITEM_MOVE:
		ItemMove(pkt);
		break;
	case WIZ_NPC_EVENT:
		NpcEvent(pkt);
		break;
	case WIZ_ITEM_TRADE:
		ItemTrade(pkt);
		break;
	case WIZ_TARGET_HP:
		{
			uint16 uid = pkt.read<uint16>();
			uint8 echo = pkt.read<uint8>();
			SendTargetHP(echo, uid);
		}
		break;
	case WIZ_BUNDLE_OPEN_REQ:
		BundleOpenReq(pkt);
		break;
	case WIZ_ITEM_GET:
		ItemGet(pkt);
		break;
	case WIZ_ZONE_CHANGE:
		RecvZoneChange(pkt);
		break;
	case WIZ_POINT_CHANGE:
		PointChange(pkt);
		break;
	case WIZ_STATE_CHANGE:
		StateChange(pkt);
		break;
	case WIZ_PARTY:
		PartyProcess(pkt);
		break;
	case WIZ_EXCHANGE:
		ExchangeProcess(pkt);
		break;
	case WIZ_QUEST:
		QuestV2PacketProcess(pkt);
		break;
	case WIZ_MERCHANT:
		MerchantProcess(pkt);
		break;
	case WIZ_MAGIC_PROCESS:
		CMagicProcess::MagicPacket(pkt, this);
		break;
	case WIZ_SKILLPT_CHANGE:
		SkillPointChange(pkt);
		break;
	case WIZ_OBJECT_EVENT:
		ObjectEvent(pkt);
		break;
	case WIZ_WEATHER:
	case WIZ_TIME:
		UpdateGameWeather(pkt);
		break;
	case WIZ_CLASS_CHANGE:
		ClassChange(pkt);
		break;
	case WIZ_CONCURRENTUSER:
		CountConcurrentUser();
		break;
	case WIZ_DATASAVE:
		UserDataSaveToAgent();
		break;
	case WIZ_ITEM_REPAIR:
		ItemRepair(pkt);
		break;
	case WIZ_KNIGHTS_PROCESS:
		CKnightsManager::PacketProcess(this, pkt);
		break;
	case WIZ_ITEM_REMOVE:
		ItemRemove(pkt);
		break;
	case WIZ_OPERATOR:
		OperatorCommand(pkt);
		break;
	case WIZ_SPEEDHACK_CHECK:
		SpeedHackTime(pkt);
		m_sAliveCount = 0;
		break;
	case WIZ_WAREHOUSE:
		WarehouseProcess(pkt);
		break;
	case WIZ_HOME:
		Home();
		break; 
	case WIZ_FRIEND_PROCESS:
		FriendProcess(pkt);
		break;
	case WIZ_WARP_LIST:
		SelectWarpList(pkt);
		break;
	case WIZ_VIRTUAL_SERVER:
		ServerChangeOk(pkt);
		break;
	case WIZ_PARTY_BBS:
		PartyBBS(pkt);
		break;
	case WIZ_CLIENT_EVENT:
		ClientEvent(pkt.read<uint16>());
		break;
	case WIZ_SELECT_MSG:
		RecvSelectMsg(pkt);
		break;
	case WIZ_ITEM_UPGRADE:
		ItemUpgradeProcess(pkt);
		break;
	case WIZ_SHOPPING_MALL: // letter system's used in here too
		ShoppingMall(pkt);
		break;
	case WIZ_KING:
		CKingSystem::PacketProcess(this, pkt);
		break;
	case WIZ_HELMET:
		HandleHelmet(pkt);
		break;
	case WIZ_CAPE:
		HandleCapeChange(pkt);
		break;
	case WIZ_CHALLENGE:
		HandleChallenge(pkt);
		break;
	case WIZ_RANK:
		HandlePlayerRankings(pkt);
		break;
	case WIZ_MINING:
		HandleMiningSystem(pkt);
		break;

	default:
		TRACE("[SID=%d] Unknown packet %X\n", GetSocketID(), command);
		return false;
	}

	if (command == WIZ_GAMESTART)
	{
		m_tHPLastTimeNormal = UNIXTIME;
		fill_n(m_tHPLastTime, MAX_TYPE3_REPEAT, UNIXTIME);
	}	

	if (!isBlinking() && m_tHPLastTimeNormal != 0 && (UNIXTIME - m_tHPLastTimeNormal) > m_bHPIntervalNormal)
		HPTimeChange();	// For Sitdown/Standup HP restoration.

	if (m_bType3Flag) {     // For Type 3 HP Duration.
		for (int i = 0 ; i < MAX_TYPE3_REPEAT ; i++) {	
			if (m_tHPLastTime[i] != 0 && (UNIXTIME - m_tHPLastTime[i]) > m_bHPInterval[i])
			{
				HPTimeChangeType3();	
				break;
			}
		}
	} 

	if (!m_buffMap.empty())		// For Type 4 Stat Duration.
		Type4Duration();

	// Expire any timed out saved skills.
	CheckSavedMagic();
		
	if (isTransformed())
		CMagicProcess::CheckExpiredType6Skills(this);

	if (isBlinking())		// Should you stop blinking?
		BlinkTimeCheck();

	return true;
}

/**
 * @brief	Adjusts a player's loyalty (NP) and sends the loyalty 
 * 			change packet.
 *
 * @param	nChangeAmount	The amount to adjust the loyalty points by.
 */
void CUser::SendLoyaltyChange(int32 nChangeAmount /*= 0*/)
{
	Packet result(WIZ_LOYALTY_CHANGE, uint8(1));

	m_iLoyalty += nChangeAmount;
	m_iLoyaltyMonthly += nChangeAmount;

	if (m_iLoyalty < 0)
		m_iLoyalty = 0;
	if (m_iLoyaltyMonthly < 0)
		m_iLoyaltyMonthly = 0;

	result	<< m_iLoyalty << m_iLoyaltyMonthly
			<< uint32(0) // Clan donations(? Donations made by this user? For the clan overall?)
			<< uint32(0); // Premium NP(? Additional NP gained?)

	Send(&result);
}

/**
 * @brief	Changes a player's fame.
 *
 * @param	bFame	The fame.
 */
void CUser::ChangeFame(uint8 bFame)
{
	Packet result(WIZ_AUTHORITY_CHANGE, uint8(COMMAND_AUTHORITY));

	m_bFame = bFame;
	result << GetSocketID() << getFame();
	SendToRegion(&result);
}

/**
 * @brief	Sends the server index.
 */
void CUser::SendServerIndex()
{
	Packet result(WIZ_SERVER_INDEX);
	result << uint16(1) << uint16(g_pMain->m_nServerNo);
	Send(&result);
}

/**
 * @brief	Packet handler for skillbar requests.
 *
 * @param	pkt	The packet.
 */
void CUser::SkillDataProcess(Packet & pkt)
{
	uint8 opcode = pkt.read<uint8>();
	switch (opcode)
	{
	case SKILL_DATA_SAVE:
		SkillDataSave(pkt);
		break;

	case SKILL_DATA_LOAD:
		SkillDataLoad();
		break;
	}
}

/**
 * @brief	Packet handler for saving a skillbar.
 *
 * @param	pkt	The packet.
 */
void CUser::SkillDataSave(Packet & pkt)
{
	Packet result(WIZ_SKILLDATA, uint8(SKILL_DATA_SAVE));
	uint16 sCount = pkt.read<uint16>();
	if (sCount == 0 || sCount > 64)
		return;

	result	<< sCount;
	for (int i = 0; i < sCount; i++)
		result << pkt.read<uint32>();
	
	g_pMain->AddDatabaseRequest(result, this);
}

/**
 * @brief	Packet handler for loading a skillbar.
 */
void CUser::SkillDataLoad()
{
	Packet result(WIZ_SKILLDATA, uint8(SKILL_DATA_LOAD));
	g_pMain->AddDatabaseRequest(result, this);
}

/**
 * @brief	Initiates a database request to save the character's information.
 */
void CUser::UserDataSaveToAgent()
{
	if (!isInGame())
		return;

	Packet result(WIZ_DATASAVE);
	result << GetAccountName() << GetName();
	g_pMain->AddDatabaseRequest(result, this);
}

/**
 * @brief	Logs a player out.
 */
void CUser::LogOut()
{
	if (m_strUserID.empty()) 
		return; 

	Packet result(AG_USER_LOG_OUT);
	result << GetID() << GetName();
	Send_AIServer(&result);

	result.Initialize(WIZ_LOGOUT);
	m_deleted = true; // make this session unusable until the logout is complete
	g_pMain->AddDatabaseRequest(result, this);
}

/**
 * @brief	Sends the player's information on initial login.
 */
void CUser::SendMyInfo()
{
	C3DMap* pMap = GetMap();
	CKnights* pKnights = NULL;

	if (!pMap->IsValidPosition( m_curx, m_curz, 0.0f))
	{
		short x = 0, z = 0;
		GetStartPosition(x, z); 

		m_curx = (float)x;
		m_curz = (float)z;
	}


	QuestDataRequest();
	Packet result(WIZ_MYINFO);

	// Load up our user rankings (for our NP symbols).
	g_pMain->GetUserRank(this);

	// Are we the King? Let's see, shall we?
	CKingSystem * pData = g_pMain->m_KingSystemArray.GetData(GetNation());
	if (pData != NULL
		&& _strcmpi(pData->m_strKingName.c_str(), m_strUserID.c_str()) == 0)
		m_bRank = 1; // We're da King, man.
	else
		m_bRank = 0; // totally not da King.

	result.SByte(); // character name has a single byte length
	result	<< GetSocketID()
			<< GetName()
			<< GetSPosX() << GetSPosZ() << GetSPosY()
			<< GetNation() 
			<< m_bRace << m_sClass << m_bFace
			<< m_nHair
			<< m_bRank << m_bTitle
			<< GetLevel()
			<< m_sPoints
			<< m_iMaxExp << m_iExp
			<< m_iLoyalty << m_iLoyaltyMonthly
			<< GetClanID() << getFame();

	if (isInClan())
		pKnights = g_pMain->GetClanPtr(GetClanID());

	if (pKnights == NULL)
	{
		result	<< uint64(0) << uint16(-1) << uint32(0);
	}
	else 
	{
		pKnights->OnLogin(this);

		// TO-DO: Figure all this out.
		result	<< uint16(pKnights->m_sAlliance)
				<< pKnights->m_byFlag
				<< pKnights->m_strName
				<< pKnights->m_byGrade << pKnights->m_byRanking
				<< uint16(pKnights->m_sMarkVersion)
				<< uint16(pKnights->m_sCape)
				<< pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << uint8(0);
	}

	result	<< uint8(2) << uint8(3) << uint8(4) << uint8(5) // unknown
			<< m_iMaxHp << m_sHp
			<< m_iMaxMp << m_sMp
			<< m_sMaxWeight << m_sItemWeight
			<< getStat(STAT_STR) << uint8(getStatItemBonus(STAT_STR))
			<< getStat(STAT_STA) << uint8(getStatItemBonus(STAT_STA))
			<< getStat(STAT_DEX) << uint8(getStatItemBonus(STAT_DEX))
			<< getStat(STAT_INT) << uint8(getStatItemBonus(STAT_INT))
			<< getStat(STAT_CHA) << uint8(getStatItemBonus(STAT_CHA))
			<< m_sTotalHit << m_sTotalAc
			<< m_bFireR << m_bColdR << m_bLightningR << m_bMagicR << m_bDiseaseR << m_bPoisonR
			<< m_iGold
			<< m_bAuthority
			<< m_bPersonalRank << m_bKnightsRank; // national rank, leader rank

	result.append(m_bstrSkill, 9);

	for (int i = 0; i < INVENTORY_TOTAL; i++)
	{
		_ITEM_DATA *pItem = GetItem(i);
		result	<< pItem->nNum
				<< pItem->sDuration << pItem->sCount
				<< pItem->bFlag	// item type flag (e.g. rented)
				<< pItem->sRemainingRentalTime	// remaining time
				<< uint32(0) // unknown
				<< pItem->nExpirationTime; // expiration date in unix time
	}

	m_bIsChicken = CheckExistEvent(50, 1);
	result	<< m_bAccountStatus	// account status (0 = none, 1 = normal prem with expiry in hours, 2 = pc room)
			<< m_bPremiumType		// premium type (7 = platinum premium)
			<< m_sPremiumTime		// premium time
			<< m_bIsChicken						// chicken/beginner flag
			<< m_iMannerPoint;

	Send(&result);

	g_pMain->AddCharacterName(this);

	SetZoneAbilityChange();
	Send2AI_UserUpdateInfo(true); 
}

/**
 * @brief	Calculates & sets a player's maximum HP.
 *
 * @param	iFlag	If set to 1, additionally resets the HP to max. 
 * 					If set to 2, additionally resets the max HP to 100 (i.e. Snow war). 
 */
void CUser::SetMaxHp(int iFlag)
{
	_CLASS_COEFFICIENT* p_TableCoefficient = NULL;
	p_TableCoefficient = g_pMain->m_CoefficientArray.GetData( m_sClass );
	if( !p_TableCoefficient ) return;

	int temp_sta = getStatTotal(STAT_STA);
//	if( temp_sta > 255 ) temp_sta = 255;

	if( m_bZone == ZONE_SNOW_BATTLE && iFlag == 0 )	{
		m_iMaxHp = 100;
	}
	else	{
		m_iMaxHp = (short)(((p_TableCoefficient->HP * GetLevel() * GetLevel() * temp_sta ) 
		      + 0.1 * (GetLevel() * temp_sta) + (temp_sta / 5)) + m_sMaxHPAmount + m_sItemMaxHp + 20);

		if( iFlag == 1 )	m_sHp = m_iMaxHp;
		else if( iFlag == 2 )	m_iMaxHp = 100;
	}

	if(m_iMaxHp < m_sHp) {
		m_sHp = m_iMaxHp;
		HpChange( m_sHp );
	}
}

/**
 * @brief	Calculates & sets a player's maximum MP.
 */
void CUser::SetMaxMp()
{
	_CLASS_COEFFICIENT* p_TableCoefficient = NULL;
	p_TableCoefficient = g_pMain->m_CoefficientArray.GetData( m_sClass );
	if( !p_TableCoefficient ) return;

	int temp_intel = 0, temp_sta = 0;
	temp_intel = getStatTotal(STAT_INT) + 30;
//	if( temp_intel > 255 ) temp_intel = 255;
	temp_sta = getStatTotal(STAT_STA);
//	if( temp_sta > 255 ) temp_sta = 255;

	if( p_TableCoefficient->MP != 0)
	{
		m_iMaxMp = (short)((p_TableCoefficient->MP * GetLevel() * GetLevel() * temp_intel)
			+ (0.1f * GetLevel() * 2 * temp_intel) + (temp_intel / 5) + m_sMaxMPAmount + m_sItemMaxMp + 20);
	}
	else if( p_TableCoefficient->SP != 0)
	{
		m_iMaxMp = (short)((p_TableCoefficient->SP * GetLevel() * GetLevel() * temp_sta )
			  + (0.1f * GetLevel() * temp_sta) + (temp_sta / 5) + m_sMaxMPAmount + m_sItemMaxMp);
	}

	if(m_iMaxMp < m_sMp) {
		m_sMp = m_iMaxMp;
		MSpChange( m_sMp );
	}
}

/**
 * @brief	Sends the server time.
 */
void CUser::SendTime()
{
	Packet result(WIZ_TIME);
	result	<< uint16(g_pMain->m_nYear) << uint16(g_pMain->m_nMonth) << uint16(g_pMain->m_nDate)
			<< uint16(g_pMain->m_nHour) << uint16(g_pMain->m_nMin);
	Send(&result);
}

/**
 * @brief	Sends the weather status.
 */
void CUser::SendWeather()
{
	Packet result(WIZ_WEATHER);
	result << g_pMain->m_byWeather << g_pMain->m_sWeatherAmount;
	Send(&result);
}

/**
 * @brief	Sets various zone flags to control how
 * 			the client handles other players/NPCs.
 * 			Also sends the zone's current tax rate.
 */
void CUser::SetZoneAbilityChange()
{
	Packet result(WIZ_ZONEABILITY, uint8(1));
	uint8 zone = GetZoneID();

	// Moradon or temples (but NOT FT).
	if (zone == 21
		|| ((zone / 10) == 5 && zone != 54))
	{
		result	<< uint8(1) << uint8(0) << uint8(1)
				<< uint16(zone == 21 ? 20 : 10); // zone tariff
	}
	// Arena
	else if (zone == 48)
	{
		result	<< uint8(0) << uint8(0) << uint8(1)
				<< uint16(10);
	}
	// Now we handle FT
	else if (zone == 54)
	{
		result	<< uint8(0) << uint8(7) << uint8(1)
				<< uint16(10);
	}
	// desperation abyss & hell abyss
	else if (zone == 32 || zone == 33)
	{
		result	<< uint8(0) << uint8(8) << uint8(1)
				<< uint16(10);
	}
	// colony zone
	else if (zone == ZONE_RONARK_LAND)
	{
		result	<< uint8(0) << uint8(1) << uint8(0)
				<< uint16(20);
	}
	// delos
	else if (zone == 31)
	{
		// to-do
	}
	else if (zone == 1 || zone == 11)
	{
		CKingSystem * pData = g_pMain->m_KingSystemArray.GetData(KARUS);
		uint16 sTariff = (pData == NULL ? 0 : pData->m_byTerritoryTariff);

		result	<< uint8(0) << uint8(1) << uint8(0)
				<< sTariff; // orc-side tariff
	}
	else if (zone == 2 || zone == 12)
	{
		CKingSystem * pData = g_pMain->m_KingSystemArray.GetData(ELMORAD);
		uint16 sTariff = (pData == NULL ? 0 : pData->m_byTerritoryTariff);

		result	<< uint8(0) << uint8(1) << uint8(0)
				<< sTariff; // human-side tariff
	}
	else 
		return;

	Send(&result);
}

/**
 * @brief	Sends the user's premium state.
 */
void CUser::SendPremiumInfo()
{
	Packet result(WIZ_PREMIUM, m_bAccountStatus);
	result << m_bPremiumType << uint32(m_sPremiumTime); 
	Send(&result);
}

/**
 * @brief	Requests user info for the specified session IDs.
 *
 * @param	pkt	The packet.
 */
void CUser::RequestUserIn(Packet & pkt)
{
	Packet result(WIZ_REQ_USERIN);
	short user_count = pkt.read<uint16>(), online_count = 0;
	if (user_count > 1000)
		user_count = 1000;

	result << uint16(0); // placeholder for user count

	for (int i = 0; i < user_count; i++)
	{
		CUser *pUser = g_pMain->GetUserPtr(pkt.read<uint16>());
		if (pUser == NULL || !pUser->isInGame())
			continue;

		result << uint8(0) << pUser->GetSocketID();
		pUser->GetUserInfo(result);

		online_count++;
	}

	result.put(0, online_count); // substitute count in
	SendCompressed(&result);
}

/**
 * @brief	Request NPC info for the specified NPC IDs.
 *
 * @param	pkt	The packet.
 */
void CUser::RequestNpcIn(Packet & pkt)
{
	if (g_pMain->m_bPointCheckFlag == false)
		return;

	Packet result(WIZ_REQ_NPCIN);
	uint16 npc_count = pkt.read<uint16>();
	if (npc_count > 1000)
		npc_count = 1000;

	result << uint16(0); // NPC count placeholder

	for (int i = 0; i < npc_count; i++)
	{
		uint16 nid = pkt.read<uint16>();
		if (nid < 0 || nid > NPC_BAND+NPC_BAND)
			continue;

		CNpc *pNpc = g_pMain->m_arNpcArray.GetData(nid);
		if (pNpc == NULL || pNpc->isDead())
			continue;

		result << pNpc->GetID();
		pNpc->GetNpcInfo(result);
	}

	result.put(0, npc_count);
	SendCompressed(&result);
}

/**
 * @brief	Calculates & resets item stats/bonuses.
 */
void CUser::SetSlotItemValue()
{
	_ITEM_TABLE* pTable = NULL;
	int item_hit = 0, item_ac = 0;

	m_sItemMaxHp = m_sItemMaxMp = 0;
	m_sItemHit = m_sItemAc = 0; 
	m_sItemWeight = 0;	
	m_sItemHitrate = m_sItemEvasionrate = 100; 
	
	memset(m_sStatItemBonuses, 0, sizeof(uint16) * STAT_COUNT);
	m_bFireR = 0; m_bColdR = 0; m_bLightningR = 0; m_bMagicR = 0; m_bDiseaseR = 0; m_bPoisonR = 0;
	
	m_sDaggerR = 0; m_sSwordR = 0; m_sAxeR = 0; m_sMaceR = 0; m_sSpearR = 0; m_sBowR = 0;
	m_bMagicTypeLeftHand = 0; m_bMagicTypeRightHand = 0; m_sMagicAmountLeftHand = 0; m_sMagicAmountRightHand = 0;       

	for(int i=0; i<SLOT_MAX; i++)  {
		if(m_sItemArray[i].nNum <= 0)
			continue;
		pTable = g_pMain->GetItemPtr( m_sItemArray[i].nNum );
		if( !pTable )
			continue;
		if( m_sItemArray[i].sDuration == 0 ) {
			item_hit = pTable->m_sDamage / 2;
			item_ac = pTable->m_sAc / 2;
		}
		else {
			item_hit = pTable->m_sDamage;
			item_ac = pTable->m_sAc;
		}
		if( i == RIGHTHAND ) 	// ItemHit Only Hands
			m_sItemHit += item_hit;
		if( i == LEFTHAND ) {
			if( ( m_sClass == BERSERKER || m_sClass == BLADE ) )
				m_sItemHit += (short)(item_hit * 0.5f);
		}

		m_sItemMaxHp += pTable->m_MaxHpB;
		m_sItemMaxMp += pTable->m_MaxMpB;
		m_sItemAc += item_ac;
		m_sStatItemBonuses[STAT_STR] += pTable->m_sStrB;
		m_sStatItemBonuses[STAT_STA] += pTable->m_sStaB;
		m_sStatItemBonuses[STAT_DEX] += pTable->m_sDexB;
		m_sStatItemBonuses[STAT_INT] += pTable->m_sIntelB;
		m_sStatItemBonuses[STAT_CHA] += pTable->m_sChaB;
		m_sItemHitrate += pTable->m_sHitrate;
		m_sItemEvasionrate += pTable->m_sEvarate;
		m_sItemWeight += pTable->m_sWeight;

		m_bFireR += pTable->m_bFireR;
		m_bColdR += pTable->m_bColdR;
		m_bLightningR += pTable->m_bLightningR;
		m_bMagicR += pTable->m_bMagicR;
		m_bDiseaseR += pTable->m_bCurseR;
		m_bPoisonR += pTable->m_bPoisonR;

		m_sDaggerR += pTable->m_sDaggerAc;
		m_sSwordR += pTable->m_sSwordAc;
		m_sAxeR += pTable->m_sAxeAc;
		m_sMaceR += pTable->m_sMaceAc;
		m_sSpearR += pTable->m_sSpearAc;
		m_sBowR += pTable->m_sBowAc;
	}

	// Also add the weight of items in the inventory
	// This will include magic bags. Should we be including those?
	for (int i = SLOT_MAX; i < INVENTORY_TOTAL; i++) 
	{
		pTable = GetItemPrototype(i);
		if (pTable == NULL)
			continue;

		// Non-stackable items should have a count of 1. If not, something's broken.
		m_sItemWeight += pTable->m_sWeight * m_sItemArray[i].sCount;
	}

	if (m_sItemHit < 3)
		m_sItemHit = 3;

	_ITEM_TABLE* pLeftHand = GetItemPrototype(LEFTHAND);
	if (pLeftHand) {
		if (pLeftHand->m_bFireDamage) {
			m_bMagicTypeLeftHand = 1;
			m_sMagicAmountLeftHand = pLeftHand->m_bFireDamage;
		}

		if (pLeftHand->m_bIceDamage) {
			m_bMagicTypeLeftHand = 2;
			m_sMagicAmountLeftHand = pLeftHand->m_bIceDamage;
		}

		if (pLeftHand->m_bLightningDamage) {
			m_bMagicTypeLeftHand = 3;
			m_sMagicAmountLeftHand = pLeftHand->m_bLightningDamage;
		}

		if (pLeftHand->m_bPoisonDamage) {
			m_bMagicTypeLeftHand = 4;
			m_sMagicAmountLeftHand = pLeftHand->m_bPoisonDamage;
		}

		if (pLeftHand->m_bHPDrain) {
			m_bMagicTypeLeftHand = 5;
			m_sMagicAmountLeftHand = pLeftHand->m_bHPDrain;
		}

		if (pLeftHand->m_bMPDamage) {
			m_bMagicTypeLeftHand = 6;
			m_sMagicAmountLeftHand = pLeftHand->m_bMPDamage;
		}

		if (pLeftHand->m_bMPDrain) {
			m_bMagicTypeLeftHand = 7;
			m_sMagicAmountLeftHand = pLeftHand->m_bMPDrain;
		}

		if (pLeftHand->m_bMirrorDamage)	{
			m_bMagicTypeLeftHand = 8;
			m_sMagicAmountLeftHand = pLeftHand->m_bMirrorDamage;	
		}
	}

	_ITEM_TABLE* pRightHand = NULL;			// Get item info for right hand.
	pRightHand = g_pMain->GetItemPtr(m_sItemArray[RIGHTHAND].nNum);
	if (pRightHand) {
		if (pRightHand->m_bFireDamage) {
			m_bMagicTypeRightHand = 1;
			m_sMagicAmountRightHand = pRightHand->m_bFireDamage;
		}

		if (pRightHand->m_bIceDamage) {
			m_bMagicTypeRightHand = 2;
			m_sMagicAmountRightHand = pRightHand->m_bIceDamage;
		}

		if (pRightHand->m_bLightningDamage) {
			m_bMagicTypeRightHand = 3;
			m_sMagicAmountRightHand = pRightHand->m_bLightningDamage;
		}

		if (pRightHand->m_bPoisonDamage) {
			m_bMagicTypeRightHand = 4;
			m_sMagicAmountRightHand = pRightHand->m_bPoisonDamage;
		}

		if (pRightHand->m_bHPDrain) {
			m_bMagicTypeRightHand = 5;
			m_sMagicAmountRightHand = pRightHand->m_bHPDrain;
		}

		if (pRightHand->m_bMPDamage) {
			m_bMagicTypeRightHand = 6;
			m_sMagicAmountRightHand = pRightHand->m_bMPDamage;
		}

		if (pRightHand->m_bMPDrain) {
			m_bMagicTypeRightHand = 7;
			m_sMagicAmountRightHand = pRightHand->m_bMPDrain;
		}

		if (pRightHand->m_bMirrorDamage) {
			m_bMagicTypeRightHand = 8;
			m_sMagicAmountRightHand = pRightHand->m_bMirrorDamage;	
		}		
	}
}

/**
 * @brief	Changes the player's experience points by iExp.
 *
 * @param	iExp	The amount of experience points to adjust by.
 */
void CUser::ExpChange(int64 iExp)
{	
	// Stop players level 5 or under from losing XP on death.
	if ((GetLevel() < 6 && iExp < 0)
		// Stop players in the war zone (TO-DO: Add other war zones) from losing XP on death.
		|| (m_bZone == ZONE_BATTLE && iExp < 0))
		return;

	// Despite being signed, we don't want m_iExp ever going below 0.
	// If this happens, we need to investigate why -- not sweep it under the rug.
	ASSERT(m_iExp >= 0);

	// Adjust the exp gained based on the percent set by the buff
	if (iExp > 0)
		iExp *= m_bExpGainAmount / 100;

	bool bLevel = true;
	if (iExp < 0 
		&& (m_iExp + iExp) < 0)
		bLevel = false;
	else
		m_iExp += iExp;

	// If we need to delevel...
	if (!bLevel)
	{
		// Drop us back a level.
		m_bLevel--;

		// Get the excess XP (i.e. below 0), so that we can take it off the max XP of the previous level
		// Remember: we're deleveling, not necessarily starting from scratch at the previous level
		int64 diffXP = m_iExp + iExp;

		// Now reset our XP to max for the former level.
		m_iExp = g_pMain->GetExpByLevel(GetLevel());

		// Get new stats etc.
		LevelChange(GetLevel(), false);

		// Take the remainder of the XP off (and delevel again if necessary).
		ExpChange(diffXP);
		return;
	}
	// If we've exceeded our XP requirement, we've leveled.
	else if (m_iExp >= m_iMaxExp)
	{
		if (GetLevel() < MAX_LEVEL)
		{
			// Reset our XP to 0, level us up.
			m_iExp = 0;
			LevelChange(++m_bLevel);
			return;
		}

		// Hit the max level? Can't level any further. Cap the XP.
		m_iExp = m_iMaxExp;
	}

	// Tell the client our new XP
	Packet result(WIZ_EXP_CHANGE);
	result << uint8(0) << m_iExp; // NOTE: Use proper flag
	Send(&result);

	// If we've lost XP, save it for possible refund later.
	if (iExp < 0)
		m_iLostExp = -iExp;
}

/**
 * @brief	Handles stat updates after a level change. 
 * 			It does not change the level.
 *
 * @param	level   	The level we've changed to.
 * @param	bLevelUp	true to level up, false for deleveling.
 */
void CUser::LevelChange(short level, bool bLevelUp /*= true*/)
{
	if( level < 1 || level > MAX_LEVEL )
		return;

	if (bLevelUp)
	{
		if ((m_sPoints + getStatTotal()) < int32(300 + 3 * (level - 1)))
			m_sPoints += 3;
		if (level >= 10 && GetTotalSkillPoints() < 2 * (level - 9))
			m_bstrSkill[SkillPointFree] += 2;
	}

	m_iMaxExp = g_pMain->GetExpByLevel(level);
	
	SetSlotItemValue();
	SetUserAbility();

	m_sMp = m_iMaxMp;
	HpChange( m_iMaxHp );

	Send2AI_UserUpdateInfo();

	Packet result(WIZ_LEVEL_CHANGE);
	result	<< GetSocketID()
			<< GetLevel() << m_sPoints << m_bstrSkill[SkillPointFree]
			<< m_iMaxExp << m_iExp
			<< m_iMaxHp << m_sHp 
			<< m_iMaxMp << m_sMp
			<< m_sMaxWeight << m_sItemWeight;

	g_pMain->Send_Region(&result, GetMap(), GetRegionX(), GetRegionZ());
	if (isInParty())
	{
		// TO-DO: Move this to party specific code
		result.Initialize(WIZ_PARTY);
		result << uint8(PARTY_LEVELCHANGE) << GetSocketID() << GetLevel();
		g_pMain->Send_PartyMember(m_sPartyIndex, &result);
	}
}

/**
 * @brief	Handles player stat assignment.
 *
 * @param	pkt	The packet.
 */
void CUser::PointChange(Packet & pkt)
{
	uint8 type = pkt.read<uint8>();
	StatType statType = (StatType)(type - 1);

	if (statType < STAT_STR || statType >= STAT_COUNT 
		|| m_sPoints < 1
		|| getStat(statType) == STAT_MAX) 
		return;

	Packet result(WIZ_POINT_CHANGE, type);

	m_sPoints--; // remove a free point
	result << uint16(++m_bStats[statType]); // assign the free point to a stat
	SetUserAbility();
	result << m_iMaxHp << m_iMaxMp << m_sTotalHit << m_sMaxWeight;
	Send(&result);
}

/**
 * @brief	Changes a user's HP.
 *
 * @param	amount   	The amount to change by.
 * @param	pAttacker	The attacker.
 * @param	bSendToAI	true to update the AI server.
 */
void CUser::HpChange(int amount, Unit *pAttacker /*= NULL*/, bool bSendToAI /*= true*/) 
{
	Packet result(WIZ_HP_CHANGE);
	uint16 tid = (pAttacker != NULL ? pAttacker->GetID() : -1);
	int16 oldHP = m_sHp;

	// If we're taking damage...
	if (amount < 0)
	{
		// Handle mastery passives
		if (isMastered())
		{
			// Matchless: [Passive]Decreases all damages received by 15%
			if (CheckSkillPoint(SkillPointMaster, 10, MAX_LEVEL))
				amount = (85 * amount) / 100;
			// Absoluteness: [Passive]Decrease 10 % demage of all attacks
			else if (CheckSkillPoint(SkillPointMaster, 5, 9))
				amount = (90 * amount) / 100;
		}
    }

	if (amount < 0 && -amount >= m_sHp)
		m_sHp = 0;
	else if (amount >= 0 && m_sHp + amount > m_iMaxHp)
		m_sHp = m_iMaxHp;
	else
		m_sHp += amount;

	result << m_iMaxHp << m_sHp << tid;

	if (GetHealth() > 0
		&& isMastered())
	{
		const uint16 hp30Percent = (30 * GetMaxHealth()) / 100;
		if ((oldHP >= hp30Percent && m_sHp < hp30Percent)
			|| (m_sHp > hp30Percent))
		{
			SetSlotItemValue();
			SetUserAbility();
			Send2AI_UserUpdateInfo();

			if (m_sHp < hp30Percent)
				ShowEffect(106800); // skill ID for "Boldness", shown when a player takes damage.
		}
	}

	Send(&result);

	if (bSendToAI)
	{
		result.Initialize(AG_USER_SET_HP);
		result << GetSocketID() << uint32(m_sHp);
		Send_AIServer(&result);
	}

	if (isInParty())
		SendPartyHPUpdate();

	if (m_sHp == 0)
		OnDeath(pAttacker);
}

/**
 * @brief	Changes a user's mana points.
 *
 * @param	amount	The amount to adjust by.
 */
void CUser::MSpChange(int amount)
{
	Packet result(WIZ_MSP_CHANGE);
	int16 oldMP = m_sMp;
	
	// TO-DO: Make this behave unsigned.
	m_sMp += amount;
	if (m_sMp < 0)
		m_sMp = 0;
	else if (m_sMp > m_iMaxMp)
		m_sMp = m_iMaxMp;

	if (isMasteredMage())
	{
		const uint16 mp30Percent = (30 * GetMaxMana()) / 100;
		if (oldMP >= mp30Percent
			&& GetMana() < mp30Percent)
			ShowEffect(106800); // skill ID for "Boldness", shown when a player loses mana.
	}

	result << m_iMaxMp << m_sMp;
	Send(&result);

	if (isInParty())
		SendPartyHPUpdate(); // handles MP too
}

/**
 * @brief	Sends a HP update to the user's party.
 */
void CUser::SendPartyHPUpdate()
{
	Packet result(WIZ_PARTY);
	result	<< uint8(PARTY_HPCHANGE)
			<< GetSocketID()
			<< m_iMaxHp << m_sHp
			<< m_iMaxMp << m_sMp;
	g_pMain->Send_PartyMember(m_sPartyIndex, &result);
}

/**
 * @brief	Shows the specified skill's effect 
 * 			to the surrounding regions.
 *
 * @param	nSkillID	Skill identifier.
 */
void CUser::ShowEffect(uint32 nSkillID)
{
	Packet result(WIZ_EFFECT);
	result << GetID() << nSkillID;
	SendToRegion(&result);
}

/**
 * @brief	Sends a player's base information to the AI server.
 *
 * @param	initialInfo	true when initially sending a player's information
 * 						to the server.
 */
void CUser::Send2AI_UserUpdateInfo(bool initialInfo /*= false*/)
{
	Packet result(initialInfo ? AG_USER_INFO : AG_USER_UPDATE);

	result.SByte();
	result	<< GetSocketID()
			<< GetName()
			<< GetZoneID() << GetNation() << GetLevel()
			<< m_sHp << m_sMp
			<< uint16(m_sTotalHit * m_bAttackAmount / 100)
			<< uint16(m_sTotalAc + m_sACAmount)
			<< m_sTotalHitrate << m_sTotalEvasionrate
			<< m_sItemAc
			<< m_bMagicTypeLeftHand << m_bMagicTypeRightHand
			<< m_sMagicAmountLeftHand << m_sMagicAmountRightHand
			<< m_bAuthority << m_bInvisibilityType;

	Send_AIServer(&result);
}

/**
 * @brief	Calculates and resets the player's stats/resistances.
 *
 * @param	bSendPacket	true to send a subsequent item movement packet
 * 						which is almost always required in addition to
 * 						using this method.
 */
void CUser::SetUserAbility(bool bSendPacket /*= true*/)
{
	bool bHaveBow = false;
	_CLASS_COEFFICIENT* p_TableCoefficient = g_pMain->m_CoefficientArray.GetData(m_sClass);
	uint16 sItemDamage = 0;
	if (p_TableCoefficient == NULL)
		return;
	
	float hitcoefficient = 0.0f;
	_ITEM_TABLE * pRightHand = GetItemPrototype(RIGHTHAND);
	if (pRightHand != NULL)
	{
		switch (pRightHand->m_bKind/10)
		{
		case WEAPON_DAGGER:
			hitcoefficient = p_TableCoefficient->ShortSword;
			break;
		case WEAPON_SWORD:
			hitcoefficient = p_TableCoefficient->Sword;
			break;
		case WEAPON_AXE:
			hitcoefficient = p_TableCoefficient->Axe;
			break;
		case WEAPON_MACE:
		case WEAPON_MACE2:
			hitcoefficient = p_TableCoefficient->Club;
			break;
		case WEAPON_SPEAR:
			hitcoefficient = p_TableCoefficient->Spear;
			break;
		case WEAPON_BOW:
		case WEAPON_LONGBOW:
		case WEAPON_LAUNCHER:
			hitcoefficient = p_TableCoefficient->Bow;
			bHaveBow = true;
			break;
		case WEAPON_STAFF:
			hitcoefficient = p_TableCoefficient->Staff;
			break;
		}

		if (hitcoefficient != 0.0f)
			sItemDamage = pRightHand->m_sDamage;
	}

	_ITEM_TABLE *pLeftHand = GetItemPrototype(LEFTHAND);
	if (pLeftHand != NULL)
	{
		if (pLeftHand->isBow())
		{
			hitcoefficient = p_TableCoefficient->Bow;
			bHaveBow = true;
			sItemDamage = pLeftHand->m_sDamage;
		}
		else
		{
			sItemDamage += pLeftHand->m_sDamage / 2;
		}
	}

	int temp_str = getStat(STAT_STR), temp_dex = getStatTotal(STAT_DEX);
//	if( temp_str > 255 ) temp_str = 255;
//	if( temp_dex > 255 ) temp_dex = 255;

	uint32 baseAP = 0;
	if (temp_str > 150)
		baseAP = temp_str - 150;

	if (temp_str == 160)
		baseAP--;

	temp_str += getStatBonusTotal(STAT_STR);

	m_sMaxWeight = ((getStatWithItemBonus(STAT_STR) + GetLevel()) * 50) * (m_bMaxWeightAmount / 100);
	if (isRogue() || bHaveBow)  // latter check's probably unnecessary
		m_sTotalHit = (short)((((0.005f * sItemDamage * (temp_dex + 40)) + ( hitcoefficient * sItemDamage * GetLevel() * temp_dex )) + 3) * (m_bAttackAmount / 100));
	else
		m_sTotalHit = (short)(((((0.005f * sItemDamage * (temp_str + 40)) + ( hitcoefficient * sItemDamage * GetLevel() * temp_str )) + 3) * (m_bAttackAmount / 100)) + baseAP);	

	m_sTotalAc = (short)(p_TableCoefficient->AC * (GetLevel() + m_sItemAc));
	m_sTotalHitrate = ((1 + p_TableCoefficient->Hitrate * GetLevel() *  temp_dex ) * m_sItemHitrate/100 ) * (m_bHitRateAmount/100);

	m_sTotalEvasionrate = ((1 + p_TableCoefficient->Evasionrate * GetLevel() * temp_dex ) * m_sItemEvasionrate/100) * (m_sAvoidRateAmount/100);

	SetMaxHp();
	SetMaxMp();

	uint8 bDefenseBonus = 0, bResistanceBonus = 0;

	// Reset resistance bonus
	m_bResistanceBonus = 0;

	// Apply passive skill bonuses
	// NOTE: This is how it's done officially (we should really clean this up)
	// Passive bonuses do NOT stack.
	if (isWarrior())
	{
		// NOTE: These may need updating (they're based on 1.298 stats)
		if (CheckSkillPoint(PRO_SKILL2, 5, 14))
			bDefenseBonus = 20;
		else if (CheckSkillPoint(PRO_SKILL2, 15, 34))
			bDefenseBonus = 30;
		else if (CheckSkillPoint(PRO_SKILL2, 35, 54))
			bDefenseBonus = 40;
		else if (CheckSkillPoint(PRO_SKILL2, 55, 69))
			bDefenseBonus = 50;
		else if (CheckSkillPoint(PRO_SKILL2, 70, MAX_LEVEL))
		{
			// Level 70 skill quest
			if (CheckExistEvent(51, 2))
				bDefenseBonus = 60;
			else
				bDefenseBonus = 50;
		}

		// Resist: [Passive]Increase all resistance by 30. If a shield is not equipped, the effect will decrease by half.
		if (CheckSkillPoint(PRO_SKILL2, 10, 19))
			bResistanceBonus = 30;
		// Endure: [Passive]Increase all resistance by 60. If a shield is not equipped, the effect will decrease by half.
		else if (CheckSkillPoint(PRO_SKILL2, 20, 39))
			bResistanceBonus = 60;
		// Immunity: [Passive]Increase all resistance by 90. If a shield is not equipped, the effect will decrease by half.
		else if (CheckSkillPoint(PRO_SKILL2, 40, MAX_LEVEL))
			bResistanceBonus = 90;

		// If a shield's not equipped, bonuses are decreased by half.
		_ITEM_TABLE *pLeftHand = GetItemPrototype(LEFTHAND);
		if (pLeftHand == NULL || pLeftHand->isShield())
		{
			bResistanceBonus /= 2;
			bDefenseBonus /= 2;
		}

		m_bResistanceBonus = bResistanceBonus;
		m_sTotalAc += bDefenseBonus * m_sTotalAc / 100;
		// m_sTotalAcUnk += bDefenseBonus * m_sTotalAcUnk / 100;
	}
	
	// Mastered warriors / mastered priests
	if (CheckClass(6, 12))
	{
		// Boldness/Daring: [Passive]Increase your defense by 20% when your HP is down to 30% or lower.
		if (m_sHp < 30 * m_iMaxHp / 100)
		{
			m_sTotalAc += 20 * m_sTotalAc / 100;
			// m_sTotalAcUnk += 20 * m_sTotalAcUnk / 100;
		}
	}
	else if (isRogue())
	{
		// Valor: [Passive]Increase your resistance by 50 when your HP is down to 30% or below.
		if (m_sHp < 30 * m_iMaxHp / 100)
			m_bResistanceBonus += 50;
	}

#if 0
    if (m_sAdditionalAttackDamage)
      ++m_sTotalHit;

	if (m_sAdditionalDefense > 0 || m_sAdditionalDefensePct > 100)
      ++m_sTotalAc;
#endif

	uint8 bSta = getStat(STAT_STA);
	if (bSta > 100)
	{
		m_sTotalAc += bSta - 100;
		// m_sTotalAcUnk += (bSta - 100) / 3;
	}

	uint8 bInt = getStat(STAT_INT);
	if (bInt > 100)
		m_bResistanceBonus += (bInt - 100) / 2;

	// TO-DO: Transformation stats need to be applied here

	if (bSendPacket)
		SendItemMove(2);
}

/**
 * @brief	Sends the target's HP to the player.
 *
 * @param	echo  	Client-based flag that we must echo back to the client. 
 * 					Set to 0 if not responding to the client.
 * @param	tid   	The target's ID.
 * @param	damage	The amount of damage taken on this request, 0 if it does not apply.
 */
void CUser::SendTargetHP( BYTE echo, int tid, int damage )
{
	int hp = 0, maxhp = 0;

	if (tid >= NPC_BAND)
	{
		if (g_pMain->m_bPointCheckFlag == false) return;
		CNpc *pNpc = g_pMain->m_arNpcArray.GetData(tid);
		if (pNpc == NULL)
			return;
		hp = pNpc->m_iHP;	
		maxhp = pNpc->m_iMaxHP;
	}
	else 
	{
		CUser *pUser = g_pMain->GetUserPtr(tid);
		if (pUser == NULL || pUser->isDead()) 
			return;

		hp = pUser->m_sHp;	
		maxhp = pUser->m_iMaxHp;
	}

	Packet result(WIZ_TARGET_HP);
	result << uint16(tid) << echo << maxhp << hp << uint16(damage);
	Send(&result);
}

/**
 * @brief	Handler for opening a loot box.
 *
 * @param	pkt	The packet.
 */
void CUser::BundleOpenReq(Packet & pkt)
{
	Packet result(WIZ_BUNDLE_OPEN_REQ);
	uint32 bundle_index = pkt.read<uint32>();
	C3DMap* pMap = GetMap();

	if (pMap == NULL
		|| bundle_index < 1 
		|| GetRegion() == NULL
		|| isDead()) // yeah, we know people abuse this. We do not care!
		return;

	_LOOT_BUNDLE *pBundle = GetRegion()->m_RegionItemArray.GetData(bundle_index);
	if (pBundle == NULL
		|| !isInRange(pBundle->x, pBundle->z, MAX_LOOT_RANGE))
		return;

	// Send all items in the bundle to the player
	foreach (itr, pBundle->Items)
		result << itr->nItemID << itr->sCount;

	// The client expects all n items, so if there's any excess...
	// send placeholder data for them.
	for (uint32 i = pBundle->Items.size(); i < LOOT_ITEMS; i++)
		result << uint32(0) << uint16(0);

	Send(&result);
}

/**
 * @brief	Handler for looting an item from a loot box.
 *
 * @param	pkt	The packet.
 */
void CUser::ItemGet(Packet & pkt)
{
	enum LootErrorCodes
	{
		LootError					= 0,
		LootSolo					= 1,
		LootPartyCoinDistribution	= 2,
		LootPartyNotification		= 3,
		LootPartyItemGivenAway		= 4,
		LootPartyItemGivenToUs		= 5,
		LootNoRoom					= 6
	};

	Packet result(WIZ_ITEM_GET);
	uint32 nBundleID = pkt.read<uint32>(), nItemID = pkt.read<uint32>();
	_LOOT_BUNDLE * pBundle = NULL;
	_LOOT_ITEM * pItem = NULL;
	CRegion* pRegion = GetRegion();
	CUser * pReceiver = NULL;

	// Lock the array while we process this request
	// to prevent any race conditions between getting/removing the items...
	FastGuard lock(pRegion->m_RegionItemArray.m_lock);

	// Are we in any region?
	if (pRegion == NULL
		|| isTrading()
		|| isDead()
		// Does the bundle exist in this region's list?
		|| (pBundle = pRegion->m_RegionItemArray.GetData(nBundleID)) == NULL
		// Are we close enough to the bundle?
		|| !isInRange(pBundle->x, pBundle->z, MAX_LOOT_RANGE))
		goto fail_return;

	// Does the specified item we're looting exist in the bundle?
	foreach (itr, pBundle->Items)
	{
		if (itr->nItemID == nItemID)
		{
			pItem = &(*itr);
			break;
		}
	}

	// Attempt to loot the specified item.
	// If looting is possible, we can then give the receiver the item.
	if (pItem == NULL
		|| pItem->sCount == 0
		|| (pReceiver = GetLootUser(pBundle, pItem)) == NULL)
		goto fail_return;

	// If we're dealing with coins, either:
	//  - we're not in a party, in which case the coins go to us. 
	//  - we're in a party, in which case we need to distribute the coins (proportionately, by their level). 
	// Error handling should already have occurred in GetLootUser().
	if (nItemID == ITEM_GOLD)
	{
		_PARTY_GROUP * pParty;
		// Not in a party, so all the coins go to us.
		if (!isInParty()
			|| (pParty = g_pMain->m_PartyArray.GetData(m_sPartyIndex)) == NULL)
		{
			// NOTE: Coins have been checked already.
			m_iGold += pItem->sCount;
			result << uint8(LootSolo) << nBundleID << int8(-1) << nItemID << pItem->sCount << GetCoins();
			pReceiver->Send(&result);
		}
		// In a party, so distribute the coins relative to their level.
		else
		{
			uint16 sumOfLevels = 0;
			vector<CUser *> partyUsers;
			for (int i = 0; i < MAX_PARTY_USERS; i++)
			{
				CUser * pUser = g_pMain->GetUserPtr(pParty->uid[i]);
				if (pUser == NULL)
					continue;

				sumOfLevels += pUser->GetLevel();
				partyUsers.push_back(pUser);
			}

			if (partyUsers.empty())
				goto fail_return;

			foreach (itr, partyUsers)
			{
				// Calculate the number of coins to give the player
				int coins = (int)(pItem->sCount * (float)((*itr)->GetLevel() / (float)sumOfLevels));

				// Give each party member coins relative to their level.
				// Ensure we don't overflow the coins (no need to error out like solo player looting)
				if (((*itr)->GetCoins() + coins) > COIN_MAX)
					(*itr)->m_iGold = COIN_MAX;
				else
					(*itr)->m_iGold += coins;

				// Let each player know they received coins.
				result.clear();
				result << uint8(LootPartyCoinDistribution) << nBundleID << uint8(-1) << nItemID << (*itr)->GetCoins();
				(*itr)->Send(&result);
			}
		}
	} // end of coin distribution
	// If the item selected is actually an item...
	else
	{
		// Retrieve the position for this item.
		int8 bDstPos = pReceiver->FindSlotForItem(pItem->nItemID, pItem->sCount);

		// This should NOT happen unless their inventory changed after the check.
		// The item won't be removed until after processing's complete, so it's OK to error out here.
		if (bDstPos < 0)
			goto fail_return;

		// Ensure there's enough room in this user's inventory.
		if (!pReceiver->CheckWeight(pItem->nItemID, pItem->sCount))
		{
			result << uint8(LootNoRoom);
			pReceiver->Send(&result);
			return; // don't need to remove the item, so stop here.
		}

		// Add item to receiver's inventory
		_ITEM_TABLE * pTable = g_pMain->GetItemPtr(nItemID); // note: already looked up in GetLootUser() so it definitely exists
		_ITEM_DATA * pDstItem = &pReceiver->m_sItemArray[bDstPos];

		pDstItem->nNum    = pItem->nItemID;
		pDstItem->sCount += pItem->sCount;

		if (pDstItem->sCount == pItem->sCount)
		{
			pDstItem->nSerialNum = g_pMain->GenerateItemSerial();

			// NOTE: Obscure special items that act as if their durations are their stack sizes
			// will be broken here, but usual cases are typically only given in the PUS.
			// Will need to revise this logic (rather, shift it out into its own method).
			pDstItem->sDuration  = pTable->m_sDuration; 
		}

		if (pDstItem->sCount > MAX_ITEM_COUNT)
			pDstItem->sCount = MAX_ITEM_COUNT;

		pReceiver->SendItemWeight();

		result	<< uint8(pReceiver == this ? LootSolo : LootPartyItemGivenToUs)
				<< nBundleID 
				<< uint8(bDstPos - SLOT_MAX) 
				<< pDstItem->nNum << pDstItem->sCount
				<< pReceiver->GetCoins();

		pReceiver->Send(&result);

		// Now notify the party that we've looted, if applicable.
		if (isInParty())
		{
			result.clear();
			result << uint8(LootPartyNotification) << nBundleID << nItemID << pReceiver->GetName();
			g_pMain->Send_PartyMember(m_sPartyIndex, &result);

			// If we're not the receiver, i.e. round-robin gave it to someone else
			// we should let us know that this was done (otherwise we'll be like, "GM!!? WHERE'S MY ITEM?!?")
			if (pReceiver != this)
			{
				result.clear();
				result << uint8(LootPartyItemGivenAway);
				Send(&result);
			}
		}
	}

	// Everything is OK, we have a target. Now remove the item from the bundle.
	// If there's nothing else in the bundle, remove the bundle from the region.
	GetMap()->RegionItemRemove(pRegion, pBundle, pItem);
	return;

fail_return:
	// Generic error
	result << uint8(LootError);
	Send(&result);
}

/**
 * @brief	Gets the user to give the loot to.
 *
 * @param	pBundle	The loot bundle.
 * @param	pItem  	The item being looted.
 *
 * @return	null if it fails, else the loot user.
 */
CUser * CUser::GetLootUser(_LOOT_BUNDLE * pBundle, _LOOT_ITEM * pItem)
{
	CUser * pReceiver = NULL;

	if (pBundle == NULL
		|| pItem == NULL)
		return NULL;

	// If we're dealing with coins, either:
	//  - we're in a party, in which case we need to distribute the coins (proportionately, by their level). 
	//	  No checks are necessary here (the coins will be miniscule, so if there's no room we can safely ignore them)
	//  - we're not in a party, in which case the coins go to us. 
	//	  In this case, we MUST check to be sure we have room for the coins.
	if (pItem->nItemID == ITEM_GOLD)
	{
		// NOTE: No checks are necessary if we're in a party.
		if (!isInParty())
		{
			// We're not in a party, so we must check to be 
			// sure we have enough room for the coins.
			if ((GetCoins() + pItem->sCount) > COIN_MAX)
				return NULL;
		}

		// The caller will perform the distribution.
		return this; 
	}

	// If we're dealing with items:
	//	- if we're in a party: 
	//		distribute the item to the next player in the party in round-robin fashion, 
	//		whilst ensuring that user can actually hold the item.
	//  - if not in a party: 
	//		simply ensure that we can hold the item.
	if (isInParty())
	{
		// This ensures the user can hold the item.
		return GetItemRoutingUser(pItem->nItemID, pItem->sCount);
	}
	else
	{
		// NOTE: We check to see if they can hold this item in the caller.
		pReceiver = this;
	}

	return pReceiver;
}

/**
 * @brief	Packet handler for various player state changes.
 *
 * @param	pkt	The packet.
 */
void CUser::StateChange(Packet & pkt)
{
	if (isDead())
		return;

	uint8 bType = pkt.read<uint8>(), buff;
	uint32 nBuff = pkt.read<uint32>();
	buff = *(uint8 *)&nBuff; // don't ask

	switch (bType)
	{
	case 1:
		if (buff != USER_STANDING || buff != USER_SITDOWN)
			return;
		break;

	case 3:
		// /unview | /view
		if ((buff == 1 || buff == 5)
			&& !isGM())
			return;
		break;

	case 4: // emotions
		switch (buff)
		{
		case 1: // Greeting 1-3
		case 2:
		case 3:
		case 11: // Provoke 1-3
		case 12:
		case 13:
		case 14: // additional animations randomly used when hitting spacebar
		case 15:
			break; // don't do anything with them (this can be handled neater, but just for testing purposes), just make sure they're allowed

		default:
			TRACE("[SID=%d] StateChange: %s tripped (bType=%d, buff=%d, nBuff=%d) somehow, HOW!?\n", 
				GetSocketID(), GetName(), bType, buff, nBuff);
			break;
		}
		break;

	case 5:
		if (!isGM())
			return;
		break;

	case 7: // invisibility flag, we don't want users overriding server behaviour.
		return;

	default:
		TRACE("[SID=%d] StateChange: %s tripped (bType=%d, buff=%d, nBuff=%d) somehow, HOW!?\n", 
			GetSocketID(), GetName(), bType, buff, nBuff);
		return;
	}

	StateChangeServerDirect(bType, nBuff);
}

/**
 * @brief	Changes a player's state directly from the server
 * 			without any checks.
 *
 * @param	bType	State type.
 * @param	nBuff	The buff/flag (depending on the state type).
 */
void CUser::StateChangeServerDirect(BYTE bType, uint32 nBuff)
{
	uint8 buff = *(uint8 *)&nBuff; // don't ask
	switch (bType)
	{
	case 1:
		m_bResHpType = buff;
		break;

	case 2:
		m_bNeedParty = buff;
		break;

	case 3:
		m_bAbnormalType = nBuff;
		break;

	case 6:
		nBuff = m_bPartyLeader; // we don't set this here.
		break;

	case 7:
		UpdateVisibility((InvisibilityType)buff);
		break;

	case 8: // beginner quest
		break;
	}

	Packet result(WIZ_STATE_CHANGE);
	result << GetSocketID() << bType << nBuff; 
	SendToRegion(&result);
}

/**
 * @brief	Takes a target's loyalty points (NP)
 * 			and rewards some/all to the killer (current user).
 *
 * @param	tid	The target's ID.
 */
void CUser::LoyaltyChange(short tid)
{
	short loyalty_source = 0, loyalty_target = 0;

	// TO-DO: Rewrite this out, it shouldn't handle all cases so generally like this
	if (m_bZone == 48 || m_bZone == 21) 
		return;

	CUser* pTUser = g_pMain->GetUserPtr(tid);  
	if (pTUser == NULL) 
		return;

	if (pTUser->GetNation() != GetNation()) 
	{
		if (pTUser->m_iLoyalty <= 0) 
		{
			loyalty_source = 0;
			loyalty_target = 0;
		}
		// TO-DO: Rewrite this out, it'd be better to handle this in the database.
		// Colony Zone
		else if (pTUser->GetZoneID() == ZONE_RONARK_LAND) 
		{
			loyalty_source = 64;
			loyalty_target = -50;

			// Handle CZ rank
			//	m_zColonyZoneLoyalty += loyalty_source;
			//	g_pMain->UpdateColonyZoneRankInfo();
		}
		// Ardream
		else if (pTUser->GetZoneID() == 72)
		{
			loyalty_source =  25; 
			loyalty_target = -25;
		}
		// Other zones
		else 
		{
			loyalty_source =  50;
			loyalty_target = -50;
		}
	}

	SendLoyaltyChange(loyalty_source);
	pTUser->SendLoyaltyChange(loyalty_target);

	// TO-DO: Move this to a better place (death handler, preferrably)
	// If a war's running, and we died/killed in a war zone... (this method should NOT be so tied up in specifics( 
	if (g_pMain->m_byBattleOpen && GetZoneID() / 100 == 1) 
	{
		// Update the casualty count
		if (pTUser->GetNation() == KARUS)
			g_pMain->m_sKarusDead++;
		else 
			g_pMain->m_sElmoradDead++;
	}
}

/**
 * @brief	Change's a player's loyalty points (NP).
 *
 * @param	sAmount			  	The amount.
 * @param	bDistributeToParty	true to distribute to party.
 */
void CUser::ChangeNP(short sAmount, bool bDistributeToParty /*= true*/)
{
	if (bDistributeToParty && isInParty()) 
		; /* TO-DO: Cut out all the specifics from LoyaltyDivide() and implement the core of it as its own method */
	else // Otherwise, we just give NP to the player (which this does, implicitly)
		SendLoyaltyChange(sAmount); 
}

void CUser::SpeedHackUser()
{
	if (!isInGame())
		return;

	if( m_bAuthority != 0 )
		m_bAuthority = -1;

	Disconnect();
}

void CUser::UserLookChange(int pos, int itemid, int durability)
{
	if (pos >= SLOT_MAX) // let's leave it at this for the moment, the updated check needs considerable reworking
		return;

	Packet result(WIZ_USERLOOK_CHANGE);
	result << GetSocketID() << uint8(pos) << itemid << uint16(durability);
	SendToRegion(&result, this);
}

void CUser::SendNotice()
{
	Packet result(WIZ_NOTICE);
	uint8 count = 0;

#if __VERSION < 1453 // NOTE: This is actually still supported if we wanted to use it.
	result << count; // placeholder the count
	result.SByte(); // only old-style notices use single byte lengths
	for (count = 0; count < 20; count++)
	{
		if (g_pMain->m_ppNotice[count][0] == 0)
			continue;

		result << g_pMain->m_ppNotice[count];
	}
	result.put(0, count); // replace the placeholdered line count
#else
	result << uint8(2); // new-style notices (top-right of screen)
	result << count; // placeholder the count

	// Use first line for header, 2nd line for data, 3rd line for header... etc.
	// It's most likely what they do officially (as usual, | is their line separator)
	for (int i = 0; i < 10; i += 2)
	{
		if (g_pMain->m_ppNotice[i][0] == 0)
			continue;

		// header | data
		result << g_pMain->m_ppNotice[i] << g_pMain->m_ppNotice[i + 1];
		count++;
	}
	result.put(1, count); // replace the placeholdered line count
#endif
	
	Send(&result);
}

void CUser::SkillPointChange(Packet & pkt)
{
	uint8 type = pkt.read<uint8>();
	Packet result(WIZ_SKILLPT_CHANGE, type);
	// invalid type
	if (type < 5 || type > 8 
		// not enough free skill points to allocate
		|| m_bstrSkill[0] < 1 
		// restrict skill points per category to your level
		|| m_bstrSkill[type] + 1 > GetLevel()
		// we need our first job change to assign skill points
		|| (m_sClass % 100) <= 4
		// to set points in the mastery category, we need to be mastered.
		|| (type == 8
			&& ((m_sClass % 2) != 0 || (m_sClass % 100) < 6))) 
	{
		result << m_bstrSkill[type]; // only send the packet on failure
		Send(&result);
		return;
	}

	m_bstrSkill[0] -= 1;
	m_bstrSkill[type] += 1;
}

void CUser::UpdateGameWeather(Packet & pkt)
{
	if (!isGM())	// is this user a GM?
		return;

	if (pkt.GetOpcode() == WIZ_WEATHER)
	{
		pkt >> g_pMain->m_byWeather >> g_pMain->m_sWeatherAmount;
	}
	else
	{
		uint16 y, m, d;
		pkt >> y >> m >> d >> g_pMain->m_nHour >> g_pMain->m_nMin;
	}
	Send(&pkt); // pass the packet straight on
}

void CUser::GetUserInfoForAI(Packet & result)
{
	result.SByte(); 
	result	<< GetSocketID()
			<< GetName() << GetZoneID() << GetNation() << GetLevel()
			<< m_sHp << m_sMp 
			<< uint16(m_sTotalHit * m_bAttackAmount / 100)
			<< uint16(m_sTotalAc + m_sACAmount)
			<< m_sTotalHitrate << m_sTotalEvasionrate
			<< m_sPartyIndex << m_bAuthority
			<< m_bInvisibilityType;
}

void CUser::CountConcurrentUser()
{
	if (!isGM())
		return;

	uint16 count = 0;
	SessionMap & sessMap = g_socketMgr.GetActiveSessionMap();
	foreach (itr, sessMap)
	{
		if (TO_USER(itr->second)->isInGame())
			count++;
	}
	g_socketMgr.ReleaseLock();

	Packet result(WIZ_CONCURRENTUSER);
	result << count;
	Send(&result);
}

void CUser::LoyaltyDivide(short tid)
{
	int levelsum = 0, individualvalue = 0;
	short temp_loyalty = 0, level_difference = 0, loyalty_source = 0, loyalty_target = 0, average_level = 0; 
	BYTE total_member = 0;

	if (!isInParty())
		return;

	_PARTY_GROUP *pParty = g_pMain->m_PartyArray.GetData( m_sPartyIndex );
	if (pParty == NULL)
		return;

	CUser* pTUser = g_pMain->GetUserPtr(tid);
	if (pTUser == NULL) 
		return;

	for (int i = 0; i < MAX_PARTY_USERS; i++)
	{
		CUser *pUser = g_pMain->GetUserPtr(pParty->uid[i]);
		if (pUser == NULL)
			continue;
		levelsum += pUser->GetLevel();
		total_member++;
	}

	if (levelsum <= 0) return;		// Protection codes.
	if (total_member <= 0) return;

	average_level = levelsum / total_member;	// Calculate average level.

	//	This is for the Event Battle on Wednesday :(
	if (g_pMain->m_byBattleOpen) {
		if (m_bZone == ZONE_BATTLE) {
			if (pTUser->m_bNation == KARUS) {
				g_pMain->m_sKarusDead++;
				//TRACE("++ LoyaltyDivide - ka=%d, el=%d\n", g_pMain->m_sKarusDead, g_pMain->m_sElmoradDead);
			}
			else if (pTUser->m_bNation == ELMORAD) {
				g_pMain->m_sElmoradDead++;
				//TRACE("++ LoyaltyDivide - ka=%d, el=%d\n", g_pMain->m_sKarusDead, g_pMain->m_sElmoradDead);
			}
		}
	}
		
	if (pTUser->m_bNation != m_bNation) {		// Different nations!!!
		level_difference = pTUser->GetLevel() - average_level;	// Calculate difference!

		if (pTUser->m_iLoyalty <= 0) {	   // No cheats allowed...
			loyalty_source = 0;
			loyalty_target = 0;
		}
		else if (level_difference > 5) {	// At least six levels higher...
			loyalty_source  = 50;
			loyalty_target = -25;
		}
		else if (level_difference < -5) {	// At least six levels lower...
			loyalty_source  = 10; 
			loyalty_target = -5;
		}
		else {		// Within the 5 and -5 range...
			loyalty_source  =  30;
			loyalty_target = -15;
		}
	}
	else {		// Same Nation!!! 
		individualvalue = -1000 ;

		for (int j = 0; j < MAX_PARTY_USERS; j++) {		// Distribute loyalty amongst party members.
			CUser *pUser = g_pMain->GetUserPtr(pParty->uid[j]);
			if (pUser == NULL)
				continue;

			pUser->SendLoyaltyChange(individualvalue);
		}
		
		return;
	}
//
	if (m_bZone != m_bNation && m_bZone < 3) { 
		loyalty_source  = 2 * loyalty_source;
	}
//
	for (int j = 0; j < MAX_PARTY_USERS; j++) {		// Distribute loyalty amongst party members.
		CUser *pUser = g_pMain->GetUserPtr(pParty->uid[j]);
		if (pUser == NULL)
			continue;

		//TRACE("LoyaltyDivide 333 - user1=%s, %d\n", pUser->GetName(), pUser->m_iLoyalty);
		individualvalue = pUser->GetLevel() * loyalty_source / levelsum ;
		pUser->SendLoyaltyChange(individualvalue);
	}

	pTUser->SendLoyaltyChange(loyalty_target);
}

void CUser::ItemWoreOut(int type, int damage)
{
	static uint8 armourTypes[] = { RIGHTHAND, LEFTHAND, HEAD, BREAST, LEG, GLOVE, FOOT };
	uint8 totalSlots;

	int worerate = (int)sqrt(damage / 10.0f);
	if (worerate == 0) return;

	ASSERT(type == ATTACK || type == DEFENCE);

	// Inflict damage on equipped weapons.
	if (type == ATTACK)
		totalSlots = 2; // use only the first 2 slots (should be RIGHTHAND & LEFTHAND).
	// Inflict damage on equipped armour.
	else if (type == DEFENCE)
		totalSlots = sizeof(armourTypes) / sizeof(*armourTypes); // use all the slots.

	for (uint8 i = 0, slot = armourTypes[i]; i < totalSlots; i++)
	{
		_ITEM_DATA * pItem = GetItem(slot);
		_ITEM_TABLE * pTable = NULL;

		// Is a non-broken item equipped?
		if (pItem == NULL || pItem->sDuration <= 0
			// Does the item exist?
			|| (pTable = g_pMain->GetItemPtr(pItem->nNum)) == NULL
			// If it's in the left or righthand slot, is it a shield? (this doesn't apply to weapons)
			|| ((slot == LEFTHAND || slot == RIGHTHAND)
					&& pTable->m_bSlot != 2))
			continue;

		int beforepercent = (int)((pItem->sDuration / (double)pTable->m_sDuration) * 100);
		int curpercent;

		if (worerate > pItem->sDuration)
			pItem->sDuration = 0;
		else 
			pItem->sDuration -= worerate;

		if (m_sItemArray[slot].sDuration == 0)
		{
			SendDurability(slot, 0);
		
			SetSlotItemValue();
			SetUserAbility(false);
			SendItemMove(1);
			continue;
		}

		curpercent = (int)((pItem->sDuration / (double)pTable->m_sDuration) * 100);

		if ((curpercent / 5) != (beforepercent / 5)) 
		{
			SendDurability(slot, pItem->sDuration);

			if (curpercent >= 65 && curpercent < 70
				|| curpercent >= 25 && curpercent < 30)
				UserLookChange(slot, pItem->nNum, pItem->sDuration);
		}
	}
}

void CUser::SendDurability(uint8 slot, uint16 durability)
{
	Packet result(WIZ_DURATION, slot);
	result << durability;
	Send(&result);
}

void CUser::SendItemMove(uint8 subcommand)
{
	Packet result(WIZ_ITEM_MOVE, subcommand);

	// If the subcommand is not error, send the stats.
	if (subcommand != 0)
	{
		result	<< m_sTotalHit << uint16(m_sTotalAc + m_sACAmount)
				<< m_sMaxWeight
				<< m_iMaxHp << m_iMaxMp
				<< getStatBonusTotal(STAT_STR) << getStatBonusTotal(STAT_STA)
				<< getStatBonusTotal(STAT_DEX) << getStatBonusTotal(STAT_INT)
				<< getStatBonusTotal(STAT_CHA)
				<< uint16(m_bFireR + m_bResistanceBonus) << uint16(m_bColdR + m_bResistanceBonus) << uint16(m_bLightningR + m_bResistanceBonus) 
				<< uint16(m_bMagicR + m_bResistanceBonus) << uint16(m_bDiseaseR + m_bResistanceBonus) << uint16(m_bPoisonR + m_bResistanceBonus);
	}
	Send(&result);
}

void CUser::HPTimeChange()
{
	m_tHPLastTimeNormal = UNIXTIME;

	if (m_bResHpType == USER_DEAD
		|| m_sHp < 1)
		return;

	int mpPercent = 100;

	if (GetZoneID() == ZONE_SNOW_BATTLE 
		&& g_pMain->m_byBattleOpen == SNOW_BATTLE)
	{
		HpChange(5);
		return;
	}

	// For mages with under 30% of HP
	if (CheckClass(110, 210)
		&& m_sMp < (30 * m_iMaxMp / 100))
		mpPercent = 120;

	if (m_bResHpType == USER_STANDING
		/* // unknown types
		|| m_bResHpType == 4 
		|| m_bResHpType == 5*/)
	{
		if (isTransformed())
		{
			// TO-DO: Apply transformation rates
		}
		else if (m_iMaxMp != m_sMp)
		{
			MSpChange((int)(((GetLevel() * (1 + GetLevel() / 60.0) + 1) * 0.2) + 3) * mpPercent / 100);
		}
	}
	else if (m_bResHpType == USER_SITDOWN)
	{
		if (m_iMaxHp != m_sHp)
			HpChange((int)(GetLevel() * (1 + GetLevel() / 30.0)) + 3);

		if (m_iMaxMp != m_sMp)
			MSpChange((int)(((m_iMaxMp * 5) / ((GetLevel() - 1) + 30 )) + 3) * mpPercent / 100);
	}
}

void CUser::HPTimeChangeType3()
{
	fill_n(m_tHPLastTime, MAX_TYPE3_REPEAT, UNIXTIME);

	if (isDead())
		return;

	for (int h = 0; h < MAX_TYPE3_REPEAT; h++)
	{
		// Yikes. This will need cleaning up.
		CUser *pUser = NULL;
		CNpc *pNpc = NULL;
		Unit *pUnit = NULL;

		if (m_sSourceID[h] < MAX_USER) 
		{
			pUser = g_pMain->GetUserPtr(m_sSourceID[h]);
			if (pUser != NULL)
				pUser->SendTargetHP(0, GetSocketID(), m_bHPAmount[h]);

			pUnit = pUser;
		}
		else
		{
			pNpc = g_pMain->m_arNpcArray.GetData(m_sSourceID[h]);
			pUnit = pNpc;
		}

		// Reduce the HP 
		HpChange(m_bHPAmount[h]); // do we need to specify the source of the DOT?

		// Aw, did we die? :(
		if (m_sHp == 0)
		{
			OnDeath(pUnit);
			break;
		}
	}

	// Type 3 cancellation process.
	// This probably shouldn't be here.
	for (int i = 0; i < MAX_TYPE3_REPEAT; i++)
	{
		if (m_bHPDuration[i] > 0)
		{
			if ((UNIXTIME - m_tHPStartTime[i]) >= m_bHPDuration[i])
			{
				Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE3_END));

				if (m_bHPAmount[i] > 0)
					result << uint8(100);
				else
					result << uint8(200);

				Send(&result);

				m_tHPStartTime[i] = 0;
				m_tHPLastTime[i] = 0;
				m_bHPAmount[i] = 0;
				m_bHPDuration[i] = 0;				
				m_bHPInterval[i] = 5;
				m_sSourceID[i] = -1; 
			}
		}
	}

	int buff_test = 0;
	bool bType3Test = true;
	foreach_array (j, m_bHPDuration)
	{
		buff_test += m_bHPDuration[j];
		if (m_bHPAmount[j] < 0)
			bType3Test = false;
	}

	if (buff_test == 0)
		m_bType3Flag = false;

	if (isInParty() && bType3Test)
		SendPartyStatusUpdate(1, 0);
}

void CUser::Type4Duration()
{
	foreach (itr, m_buffMap)
	{
		if (itr->second.m_tEndTime <= UNIXTIME)
			continue;

		CMagicProcess::RemoveType4Buff(itr->first, this);
		break; // only ever handle one at a time with the current logic
	}

	bool bIsDebuffed = false;
	foreach (itr, m_buffMap)
	{
		if (itr->second.isDebuff())
		{
			bIsDebuffed = true;
			break;
		}
	}

	if (isInParty() && !bIsDebuffed)
		SendPartyStatusUpdate(2);
}

void CUser::SendAllKnightsID()
{
	Packet result(WIZ_KNIGHTS_LIST, uint8(1));
	uint16 count = 0;

	FastGuard lock(g_pMain->m_KnightsArray.m_lock);
	foreach_stlmap (itr, g_pMain->m_KnightsArray)
	{
		CKnights *pKnights = itr->second;
		if (pKnights == NULL)
			continue;
		result << pKnights->m_sIndex << pKnights->m_strName;
		count++;
	}

	result.put(0, count);
	SendCompressed(&result);
}

void CUser::OperatorCommand(Packet & pkt)
{
	if (!isGM())
		return;

	std::string strUserID;
	uint8 opcode;
	pkt >> opcode >> strUserID;

	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE)
		return;

	CUser *pUser = g_pMain->GetUserPtr(strUserID, TYPE_CHARACTER);
	if (pUser == NULL)
		return;

	switch (opcode)
	{
	case OPERATOR_ARREST:
		ZoneChange(pUser->GetZoneID(), pUser->m_curx, pUser->m_curz);
		break;
	case OPERATOR_SUMMON:
		pUser->ZoneChange(GetZoneID(), m_curx, m_curz);
		break;
	case OPERATOR_CUTOFF:
		pUser->Disconnect();
		break;
	case OPERATOR_BAN:
	case OPERATOR_BAN_ACCOUNT: // ban account is meant to call a proc to do so
		pUser->m_bAuthority = AUTHORITY_BANNED;
		pUser->Disconnect();
		break;
	case OPERATOR_MUTE:
		pUser->m_bAuthority = AUTHORITY_MUTED;
		break;
	case OPERATOR_DISABLE_ATTACK:
		pUser->m_bAuthority = AUTHORITY_ATTACK_DISABLED;
		break;
	case OPERATOR_ENABLE_ATTACK:
	case OPERATOR_UNMUTE:
		pUser->m_bAuthority = AUTHORITY_PLAYER;
		break;
	}
}

void CUser::SpeedHackTime(Packet & pkt)
{
#if 0 // temporarily disabled
	BYTE b_first;
	float servertime = 0.0f, clienttime = 0.0f, client_gap = 0.0f, server_gap = 0.0f;

	pkt >> b_first >> clienttime;

	if( b_first ) {
		m_fSpeedHackClientTime = clienttime;
		m_fSpeedHackServerTime = TimeGet();
	}
	else {
		servertime = TimeGet();

		server_gap = servertime - m_fSpeedHackServerTime;
		client_gap = clienttime - m_fSpeedHackClientTime;

		if( client_gap - server_gap > 10.0f ) {
			TRACE("%s SpeedHack User Checked By Server Time\n", m_id);
			Close();
		}
		else if( client_gap - server_gap < 0.0f ) {
			m_fSpeedHackClientTime = clienttime;
			m_fSpeedHackServerTime = TimeGet();
		}
	}
#endif
}

void CUser::Type3AreaDuration()
{
	Packet result(WIZ_MAGIC_PROCESS);

	_MAGIC_TABLE * pSkill = g_pMain->m_MagictableArray.GetData(m_iAreaMagicID);
	if (pSkill == NULL)
		return;

	_MAGIC_TYPE3 * pType = g_pMain->m_Magictype3Array.GetData(m_iAreaMagicID);
	if (pType == NULL)
		return;

	if (m_tAreaLastTime != 0 && (UNIXTIME - m_tAreaLastTime) > m_bAreaInterval)
	{
		m_tAreaLastTime = UNIXTIME;
		if (isDead())
			return;
		
		// TO-DO: Make this not suck (needs to be localised)
		SessionMap & sessMap = g_socketMgr.GetActiveSessionMap();
		set<uint16> sessionIDs;
		foreach (itr, sessMap)
		{
			if (CMagicProcess::UserRegionCheck(this, TO_USER(itr->second), pSkill, pType->bRadius))
				sessionIDs.insert(itr->first);
		}
		g_socketMgr.ReleaseLock();

		foreach (itr, sessionIDs)
		{
			result.clear();
			result	<< uint8(MAGIC_EFFECTING) << m_iAreaMagicID
					<< GetSocketID() << (*itr)
					<< uint16(0) << uint16(0) << uint16(0) << uint16(0) << uint16(0);
			SendToRegion(&result);
		}


		if (UNIXTIME - m_tAreaStartTime >= pType->bDuration)
		{ // Did area duration end? 			
			m_bAreaInterval = 5;
			m_tAreaStartTime = 0;
			m_tAreaLastTime = 0;
			m_iAreaMagicID = 0;
		}
	}	


	result.clear();
	result	<< uint8(MAGIC_EFFECTING) << m_iAreaMagicID
			<< GetSocketID() << GetSocketID()
			<< uint16(0) << uint16(0) << uint16(0) << uint16(0) << uint16(0);
	SendToRegion(&result);
}

int CUser::FindSlotForItem(uint32 nItemID, uint16 sCount)
{
	int result = -1;
	_ITEM_TABLE *pTable = g_pMain->GetItemPtr(nItemID);
	if (pTable == NULL)
		return result;

	// If the item's stackable, try to find it a home.
	// We could do this in the same logic, but I'd prefer one initial check
	// over the additional logic hit each loop iteration.
	if (pTable->m_bCountable)
	{
		for (int i = SLOT_MAX; i < SLOT_MAX+HAVE_MAX; i++)
		{
			_ITEM_DATA *pItem = GetItem(i);

			// If it's the item we're after, and there will be room to store it...
			if (pItem->nNum == nItemID
				&& pItem->sCount + sCount <= ITEMCOUNT_MAX)
				return i;

			// Found a free slot, we'd prefer to stack it though
			// so store the first free slot, and ignore it.
			if (pItem->nNum == 0
				&& result < 0)
				result = i;
		}

		// If we didn't find a slot countaining our stackable item, it's possible we found
		// an empty slot. So return that (or -1 if it none was found; no point searching again).
		return result;
	}

	// If it's not stackable, don't need any additional logic.
	// Just find the first free slot.
	return GetEmptySlot();
}

int CUser::GetEmptySlot()
{
	for (int i = SLOT_MAX; i < SLOT_MAX+HAVE_MAX; i++)
	{
		_ITEM_DATA *pItem = GetItem(i);
		if (pItem->nNum == 0)
			return i;
	}

	return -1;
}

void CUser::Home()
{
	if (isDead())
		return;

	// The point where you will be warped to.
	short x = 0, z = 0;

	// Forgotten Temple
	if (GetZoneID() == 55)
	{
		KickOutZoneUser(true);
		return;
	}
	// Prevent /town'ing in quest arenas
	else if ((GetZoneID() / 10) == 5
		|| !GetStartPosition(x, z))
		return;

	Warp(x * 10, z * 10);
}

bool CUser::GetStartPosition(short & x, short & z, BYTE bZone /*= 0 */)
{
	// Get start position data for current zone (unless we specified a zone).
	int nZoneID = (bZone == 0 ? GetZoneID() : bZone);
	_START_POSITION *pData = g_pMain->GetStartPosition(nZoneID);
	if (pData == NULL)
		return false;

	// TO-DO: Allow for Delos/CSW.

	// NOTE: This is how mgame does it.
	// This only allows for positive randomisation; we should really allow for the full range...
	if (GetNation() == KARUS)
	{
		x = pData->sKarusX + myrand(0, pData->bRangeX);
		z = pData->sKarusZ + myrand(0, pData->bRangeZ);
	}
	else
	{
		x = pData->sElmoradX + myrand(0, pData->bRangeX);
		z = pData->sElmoradZ + myrand(0, pData->bRangeZ);
	}

	return true;
}

void CUser::ResetWindows()
{
	if (isTrading())
		ExchangeCancel();

	if (m_bRequestingChallenge)
		HandleChallengeCancelled(m_bRequestingChallenge);

	if (m_bChallengeRequested)
		HandleChallengeRejected(m_bChallengeRequested);

	// If we're a vendor, close the stall
	if (isMerchanting())
		MerchantClose();

	// If we're just browsing, free up our spot so others can browse the vendor.
	if (m_sMerchantsSocketID >= 0)
		CancelMerchant();

/*	if (isUsingBuyingMerchant())
		BuyingMerchantClose();

	if (isUsingStore())
		m_bStoreOpen = false;*/
}

CUser * CUser::GetItemRoutingUser(uint32 nItemID, uint16 sCount)
{
	if (!isInParty())
		return this;

	_ITEM_TABLE * pTable;
	_PARTY_GROUP * pParty = g_pMain->m_PartyArray.GetData(m_sPartyIndex);
	if (pParty == NULL
		|| (pTable = g_pMain->GetItemPtr(nItemID)) == NULL
		|| pParty->bItemRouting >= MAX_PARTY_USERS)
		return NULL;

	for (int i = 0; i < MAX_PARTY_USERS; i++)
	{
		CUser * pUser = g_pMain->GetUserPtr(pParty->uid[pParty->bItemRouting]);

		if (pParty->bItemRouting > 6)
			pParty->bItemRouting = 0;
		else
			pParty->bItemRouting++;

		if (pUser != NULL 
			&& pUser->CheckWeight(pTable, nItemID, sCount))
			return pUser;
	}

	return NULL;
}

void CUser::ClassChangeReq()
{
	Packet result(WIZ_CLASS_CHANGE, uint8(CLASS_CHANGE_RESULT));
	if (GetLevel() < 10) // if we haven't got our first job change
		result << uint8(2);
	else if ((m_sClass % 100) > 4) // if we've already got our job change
		result << uint8(3);
	else // otherwise
		result << uint8(1);
	Send(&result);
}

void CUser::AllSkillPointChange()
{
	Packet result(WIZ_CLASS_CHANGE, uint8(ALL_SKILLPT_CHANGE));
	int index = 0, skill_point = 0, money = 0, temp_value = 0, old_money = 0;
	uint8 type = 0;

	temp_value = (int)pow((GetLevel() * 2.0f), 3.4f);
	if (GetLevel() < 30)		
		temp_value = (int)(temp_value * 0.4f);
	else if (GetLevel() >= 60)
		temp_value = (int)(temp_value * 1.5f);

	temp_value = (int)(temp_value * 1.5f);

	// If global discounts are enabled 
	if (g_pMain->m_sDiscount == 2 // or war discounts are enabled
		|| (g_pMain->m_sDiscount == 1 && g_pMain->m_byOldVictory == m_bNation))
		temp_value /= 2;

	money = m_iGold - temp_value;

	// Not enough money, or level too low.
	if (money < 0
		|| GetLevel() < 10)
		goto fail_return;

	// Get total skill points
	for (int i = 1; i < 9; i++)
		skill_point += m_bstrSkill[i];

	// If we don't have any skill points, there's no point resetting now is there.
	if (skill_point <= 0)
	{
		type = 2;
		goto fail_return;
	}

	// Reset skill points.
	m_bstrSkill[0] = (GetLevel() - 9) * 2;
	for (int i = 1; i < 9; i++)	
		m_bstrSkill[i] = 0;

	// Take coins.
	m_iGold = money;

	result << uint8(1) << m_iGold << m_bstrSkill[0];
	Send(&result);
	return;

fail_return:
	result << type << temp_value;
	Send(&result);
}

void CUser::AllPointChange()
{
	Packet result(WIZ_CLASS_CHANGE, uint8(ALL_POINT_CHANGE));
	int money, temp_money;
	uint8 bResult = 0;

	if (GetLevel() > MAX_LEVEL)
		goto fail_return;

	temp_money = (int)pow((GetLevel() * 2.0f ), 3.4f);
	if (GetLevel() < 30)
		temp_money = (int)(temp_money * 0.4f);
	else if (GetLevel() >= 60) 
		temp_money = (int)(temp_money * 1.5f);

	if ((g_pMain->m_sDiscount == 1 && g_pMain->m_byOldVictory == GetNation())
		|| g_pMain->m_sDiscount == 2)
		temp_money /= 2;
	
	money = m_iGold - temp_money;
	if(money < 0)	goto fail_return;

	for (int i = 0; i < SLOT_MAX; i++)
	{
		if (m_sItemArray[i].nNum) {
			bResult = 4;
			goto fail_return;
		}
	}
	
	// It's 300-10 for clarity (the 10 being the stat points assigned on char creation)
	if (getStatTotal() == 290)
	{
		bResult = 2; // don't need to reallocate stats, it has been done already...
		goto fail_return;
	}

	// TO-DO: Pull this from the database.
	switch (m_bRace)
	{
	case KARUS_BIG:	
		setStat(STAT_STR, 65);
		setStat(STAT_STA, 65);
		setStat(STAT_DEX, 60);
		setStat(STAT_INT, 50);
		setStat(STAT_CHA, 50);
		break;
	case KARUS_MIDDLE:
		setStat(STAT_STR, 65);
		setStat(STAT_STA, 65);
		setStat(STAT_DEX, 60);
		setStat(STAT_INT, 50);
		setStat(STAT_CHA, 50);
		break;
	case KARUS_SMALL:
		setStat(STAT_STR, 50);
		setStat(STAT_STA, 50);
		setStat(STAT_DEX, 70);
		setStat(STAT_INT, 70);
		setStat(STAT_CHA, 50);
		break;
	case KARUS_WOMAN:
		setStat(STAT_STR, 50);
		setStat(STAT_STA, 60);
		setStat(STAT_DEX, 60);
		setStat(STAT_INT, 60);
		setStat(STAT_CHA, 50);
		break;
	case BABARIAN:
		setStat(STAT_STR, 65);
		setStat(STAT_STA, 65);
		setStat(STAT_DEX, 60);
		setStat(STAT_INT, 50);
		setStat(STAT_CHA, 50);
		break;
	case ELMORAD_MAN:
		setStat(STAT_STR, 60);
		setStat(STAT_STA, 60);
		setStat(STAT_DEX, 70);
		setStat(STAT_INT, 50);
		setStat(STAT_CHA, 50);
		break;
	case ELMORAD_WOMAN:
		setStat(STAT_STR, 50);
		setStat(STAT_STA, 50);
		setStat(STAT_DEX, 70);
		setStat(STAT_INT, 70);
		setStat(STAT_CHA, 50);
		break;
	}

	m_sPoints = (GetLevel() - 1) * 3 + 10;
	ASSERT(getStatTotal() == 290);

	m_iGold = money;

	SetUserAbility();
	Send2AI_UserUpdateInfo();

	result << uint8(1) // result (success)
		<< m_iGold
		<< getStat(STAT_STR) << getStat(STAT_STA) << getStat(STAT_DEX) << getStat(STAT_INT) << getStat(STAT_CHA)
		<< m_iMaxHp << m_iMaxMp << m_sTotalHit << m_sMaxWeight << m_sPoints;
	Send(&result);

fail_return:
	result << bResult << temp_money;
	Send(&result);
}

void CUser::GoldChange(short tid, int gold)
{
	if (m_bZone < 3) return;	// Money only changes in Frontier zone and Battle zone!!!
	if (m_bZone == ZONE_SNOW_BATTLE) return;

	CUser* pTUser = g_pMain->GetUserPtr(tid);
	if (pTUser == NULL || pTUser->m_iGold <= 0)
		return;

	// Reward money in war zone
	if (gold == 0)
	{
		// If we're not in a party, we can distribute cleanly.
		if (!isInParty())
		{
			GoldGain((pTUser->m_iGold * 4) / 10);
			pTUser->GoldLose(pTUser->m_iGold / 2);
			return;
		}

		// Otherwise, if we're in a party, we need to divide it up.
		_PARTY_GROUP* pParty = g_pMain->m_PartyArray.GetData(m_sPartyIndex);
		if (pParty == NULL)
			return;			

		int userCount = 0, levelSum = 0, temp_gold = (pTUser->m_iGold * 4) / 10;	
		pTUser->GoldLose(pTUser->m_iGold / 2);		

		// TO-DO: Clean up the party system. 
		for (int i = 0; i < MAX_PARTY_USERS; i++)
		{
			CUser *pUser = g_pMain->GetUserPtr(pParty->uid[i]);
			if (pUser == NULL)
				continue;

			userCount++;
			levelSum += pUser->GetLevel();
		}

		// No users (this should never happen! Needs to be cleaned up...), don't bother with the below loop.
		if (userCount == 0) 
			return;

		for (int i = 0; i < MAX_PARTY_USERS; i++)
		{		
			CUser * pUser = g_pMain->GetUserPtr(pParty->uid[i]);
			if (pUser == NULL)
				continue;

			pUser->GoldGain((int)(temp_gold * (float)(pUser->GetLevel() / (float)levelSum)));
		}			
		return;
	}

	// Otherwise, use the coin amount provided.

	// Source gains money
	if (gold > 0)
	{
		GoldGain(gold);
		pTUser->GoldLose(gold);
	}
	// Source loses money
	else
	{
		GoldLose(gold);
		pTUser->GoldGain(gold);
	}
}

void CUser::SelectWarpList(Packet & pkt)
{
	if (isDead())
		return;

	uint16 npcid, warpid;
	pkt >> npcid >> warpid;

	_WARP_INFO *pWarp = GetMap()->GetWarp(warpid);
	if (pWarp == NULL
		|| (pWarp->sNation != 0 && pWarp->sNation != GetNation()))
		return;

	C3DMap *pMap = g_pMain->GetZoneByID(pWarp->sZone);
	if (pMap == NULL)
		return;

	_ZONE_SERVERINFO *pInfo = g_pMain->m_ServerArray.GetData(pMap->m_nServerNo);
	if (pInfo == NULL)
		return;

	float rx = 0.0f, rz = 0.0f;
	rx = (float)myrand( 0, (int)pWarp->fR*2 );
	if( rx < pWarp->fR ) rx = -rx;
	rz = (float)myrand( 0, (int)pWarp->fR*2 );
	if( rz < pWarp->fR ) rz = -rz;

	if (m_bZone == pWarp->sZone) 
	{
		m_bZoneChangeSameZone = true;

		Packet result(WIZ_WARP_LIST, uint8(2));
		result << uint8(1);
		Send(&result);
	}

	ZoneChange(pWarp->sZone, pWarp->fX + rx, pWarp->fZ + rz);
}

void CUser::ServerChangeOk(Packet & pkt)
{
	uint16 warpid = pkt.read<uint16>();
	C3DMap* pMap = GetMap();
	float rx = 0.0f, rz = 0.0f;
	if (pMap == NULL)
		return;

	_WARP_INFO* pWarp = pMap->GetWarp(warpid);
	if (pWarp == NULL)
		return;

	rx = (float)myrand(0, (int)pWarp->fR * 2);
	if (rx < pWarp->fR) rx = -rx;
	rz = (float)myrand(0, (int)pWarp->fR * 2);
	if (rz < pWarp->fR) rz = -rz;

	ZoneChange(pWarp->sZone, pWarp->fX + rx, pWarp->fZ + rz);
}

bool CUser::GetWarpList(int warp_group)
{
	Packet result(WIZ_WARP_LIST, uint8(1));
	C3DMap* pMap = GetMap();
	set<_WARP_INFO*> warpList;

	pMap->GetWarpList(warp_group, warpList);

	result << uint16(warpList.size());
	foreach (itr, warpList)
	{
		C3DMap *pDstMap = g_pMain->GetZoneByID((*itr)->sZone);
		if (pDstMap == NULL)
			continue;

		result	<< (*itr)->sWarpID 
				<< (*itr)->strWarpName << (*itr)->strAnnounce
				<< (*itr)->sZone
				<< pDstMap->m_sMaxUser
				<< uint32((*itr)->dwPay);
	}

	Send(&result);
	return true;
}

bool CUser::BindObjectEvent(_OBJECT_EVENT *pEvent)
{
	if (pEvent->sBelong != 0 && pEvent->sBelong != GetNation())
		return false;

	Packet result(WIZ_OBJECT_EVENT, uint8(pEvent->sType));

	m_sBind = pEvent->sIndex;

	result << uint8(1);
	Send(&result);
	return true;
}

bool CUser::GateLeverObjectEvent(_OBJECT_EVENT *pEvent, int nid)
{
	_OBJECT_EVENT *pGateEvent;
	CNpc* pNpc, *pGateNpc;

		// Does the lever (object) NPC exist?
	if ((pNpc = g_pMain->m_arNpcArray.GetData(nid)) == NULL
		// Does the corresponding gate object event exist?
		|| (pGateEvent = GetMap()->GetObjectEvent(pEvent->sControlNpcID)) == NULL
		// Does the corresponding gate (object) NPC exist?
		|| (pGateNpc = g_pMain->m_arNpcArray.GetData(pEvent->sControlNpcID)) == NULL
		// Is it even a gate?
		|| !pGateNpc->isGate()
		// If the gate's closed (i.e. the lever is down), we can't open it unless the lever isn't nation-specific
		// or we're the correct nation. Seems the other nation cannot close them.
		|| (pNpc->isGateClosed() && pNpc->GetNation() != 0 && pNpc->GetNation() != GetNation()))
		return false;

	// Move the lever (up/down).
	pNpc->SendGateFlag(!pNpc->m_byGateOpen);

	// Open/close the gate.
	pGateNpc->SendGateFlag(!pGateNpc->m_byGateOpen);
	return true;
}

/***
 * Not sure what this is used for, so keeping logic the same just in case.
 ***/
bool CUser::FlagObjectEvent(_OBJECT_EVENT *pEvent, int nid)
{
	_OBJECT_EVENT *pFlagEvent;
	CNpc *pNpc, *pFlagNpc;

	// Does the flag object NPC exist?
	if ((pNpc = g_pMain->m_arNpcArray.GetData(nid)) == NULL
		// Does the corresponding flag event exist?
		|| (pFlagEvent = GetMap()->GetObjectEvent(pEvent->sControlNpcID)) == NULL
		// Does the corresponding flag object NPC exist?
		|| (pFlagNpc = g_pMain->GetNpcPtr(pEvent->sControlNpcID, GetZoneID())) == NULL
		// Is this marked a gate? (i.e. can control)
		|| !pFlagNpc->isGate()
		// Is the war over or the gate closed?
		|| g_pMain->m_bVictory > 0 || pNpc->isGateClosed())
		return false;

	// Reset objects
	pNpc->SendGateFlag(0);
	pFlagNpc->SendGateFlag(0);

	// Add flag score (not sure what this is, is this even used anymore?)
	if (GetNation() == KARUS) 
		g_pMain->m_bKarusFlag++;
	else
		g_pMain->m_bElmoradFlag++;

	// Did one of the teams win?
	g_pMain->BattleZoneVictoryCheck();	
	return true;
}

bool CUser::WarpListObjectEvent(_OBJECT_EVENT *pEvent)
{
	// If the warp gate belongs to a nation, which isn't us...
	if (pEvent->sBelong != 0 && pEvent->sBelong != GetNation()
		// or we're in the opposing nation's zone...
		|| (GetZoneID() != GetNation() && GetZoneID() <= ELMORAD)
		// or we're unable to retrieve the warp list...
		|| !GetWarpList(pEvent->sControlNpcID)) 
		return false;

	return true;
}

void CUser::ObjectEvent(Packet & pkt)
{
	if (g_pMain->m_bPointCheckFlag == false
		|| isDead())
		return;

	bool bSuccess = false;
	uint16 objectindex, nid;
	pkt >> objectindex >> nid;

	_OBJECT_EVENT * pEvent = GetMap()->GetObjectEvent(objectindex);
	if (pEvent != NULL
		|| !isInRange(pEvent->fPosX, pEvent->fPosZ, MAX_OBJECT_RANGE))
	{
		switch (pEvent->sType)
		{
		case OBJECT_BIND:
			case OBJECT_REMOVE_BIND:
				bSuccess = BindObjectEvent(pEvent);
				break;

			case OBJECT_GATE_LEVER:
				bSuccess = GateLeverObjectEvent(pEvent, nid);
				break;

			case OBJECT_FLAG_LEVER:
				bSuccess = FlagObjectEvent(pEvent, nid);
				break;

			case OBJECT_WARP_GATE:
				bSuccess = WarpListObjectEvent(pEvent);
				if (bSuccess)
					return;
				break;

			case OBJECT_ANVIL:
				SendAnvilRequest(nid);
				return;
		}

	}

	if (!bSuccess)
	{
		Packet result(WIZ_OBJECT_EVENT, uint8(pEvent == NULL ? 0 : pEvent->sType));
		result << uint8(0);
		Send(&result);
	}
}

void CUser::SendAnvilRequest(uint16 sNpcID, uint8 bType /*= ITEM_UPGRADE_REQ*/)
{
	Packet result(WIZ_ITEM_UPGRADE, uint8(bType));
	result << sNpcID;
	Send(&result);
}

void CUser::UpdateVisibility(InvisibilityType bNewType)
{
	Packet result(AG_USER_VISIBILITY);
	m_bInvisibilityType = (uint8)(bNewType);
	result << GetID() << m_bInvisibilityType;
	Send_AIServer(&result);
}

void CUser::BlinkStart()
{
	// Don't blink in these zones
	if (GetZoneID() == ZONE_RONARK_LAND // colony zone
		|| (GetZoneID() / 100) == 1) // war zone
		return;

	m_bAbnormalType = ABNORMAL_BLINKING;
	m_tBlinkExpiryTime = UNIXTIME + BLINK_TIME;
	m_bRegeneType = REGENE_ZONECHANGE;
	
	UpdateVisibility(INVIS_NORMAL); // AI shouldn't see us
	m_bInvisibilityType = INVIS_NONE; // but players should. 

	StateChangeServerDirect(3, ABNORMAL_BLINKING);
}

void CUser::BlinkTimeCheck()
{
	if (UNIXTIME < m_tBlinkExpiryTime)
		return;

	m_bRegeneType = REGENE_NORMAL;

	StateChangeServerDirect(3, ABNORMAL_NORMAL);

	Packet result(AG_USER_REGENE);
	result	<< GetSocketID() << m_sHp;
	Send_AIServer(&result);

	result.Initialize(AG_USER_INOUT);
	result.SByte(); // TO-DO: Remove this redundant uselessness that is mgame
	result	<< uint8(INOUT_RESPAWN) << GetSocketID()
			<< GetName()
			<< m_curx << m_curz;
	Send_AIServer(&result);

	UpdateVisibility(INVIS_NONE);
}

void CUser::GoldGain(int gold)	// 1 -> Get gold    2 -> Lose gold
{
	Packet result(WIZ_GOLD_CHANGE);
	
	m_iGold += gold;

	result << uint8(1) << gold << m_iGold;
	Send(&result);	
}

bool CUser::GoldLose(unsigned int gold)
{
	if (m_iGold < gold) 
		return false;
	
	Packet result(WIZ_GOLD_CHANGE);
	m_iGold -= gold;
	result << uint8(2) << gold << m_iGold;
	Send(&result);	
	return true;
}

bool CUser::CheckSkillPoint(BYTE skillnum, BYTE min, BYTE max)
{
	if (skillnum < 5 || skillnum > 8) 
		return false;

	return (m_bstrSkill[skillnum] >= min && m_bstrSkill[skillnum] <= max);
}

bool CUser::CheckClass(short class1, short class2, short class3, short class4, short class5, short class6)
{
	return (JobGroupCheck(class1) || JobGroupCheck(class2) || JobGroupCheck(class3) || JobGroupCheck(class4) || JobGroupCheck(class5) || JobGroupCheck(class6));
}

bool CUser::JobGroupCheck(short jobgroupid)
{
	if (jobgroupid > 100) 
		return m_sClass == jobgroupid;

	int subClass = m_sClass % 100;

	switch (jobgroupid) 
	{
		case GROUP_WARRIOR:
			return (subClass == 1 || subClass == 5 || subClass == 6);

		case GROUP_ROGUE:
			return (subClass == 2 || subClass == 7 || subClass == 8);

		case GROUP_MAGE:
			return (subClass == 3 || subClass == 9 || subClass == 10);

		case GROUP_CLERIC:	
			return (subClass == 4 || subClass == 11 || subClass == 12);
	}

	return (subClass == jobgroupid);
}

void CUser::TrapProcess()
{
	// If the time interval has passed
	if ((UNIXTIME - m_tLastTrapAreaTime) >= ZONE_TRAP_INTERVAL)
	{
		HpChange(-ZONE_TRAP_DAMAGE, this);
		m_tLastTrapAreaTime = UNIXTIME;
	}
}

// TO-DO: This needs updating.
void CUser::KickOutZoneUser(bool home, int nZoneID /*= 21 */)
{
	int yourmama=0, random = 0;
	_REGENE_EVENT* pRegene = NULL;
	C3DMap* pMap = g_pMain->GetZoneByID(nZoneID);
	if (pMap == NULL) return;

	if (home)
	{
		int random = myrand(0, 9000) ;
		if( random >= 0 && random < 3000 )			yourmama = 0;
		else if( random >= 3000 && random < 6000 )	yourmama = 1;
		else if( random >= 6000 && random < 9001 )	yourmama = 2;

		pRegene = pMap->GetRegeneEvent(yourmama) ;	
		if (pRegene == NULL) 
		{
			KickOutZoneUser();
			return;
		}

		float x = pRegene->fRegenePosX + (float)myrand(0, (int)pRegene->fRegeneAreaX);
		float y = pRegene->fRegenePosZ + (float)myrand(0, (int)pRegene->fRegeneAreaZ);

		ZoneChange(pMap->m_nZoneNumber, x, y);			
	}
	else {
		if (m_bNation == KARUS) {
			ZoneChange( pMap->m_nZoneNumber, 1335, 83);	// Move user to native zone.
		}
		else {
			ZoneChange( pMap->m_nZoneNumber, 445, 1950 );	// Move user to native zone.
		}
	}
}

void CUser::NativeZoneReturn()
{
	_HOME_INFO* pHomeInfo = NULL;	// Send user back home in case it was the battlezone.
	pHomeInfo = g_pMain->m_HomeArray.GetData(m_bNation);
	if (!pHomeInfo) return;

	m_bZone = m_bNation;

	if (m_bNation == KARUS) {
		m_curx = (float)(pHomeInfo->KarusZoneX + myrand(0, pHomeInfo->KarusZoneLX));
		m_curz = (float)(pHomeInfo->KarusZoneZ + myrand(0, pHomeInfo->KarusZoneLZ)); 
	}
	else {
		m_curx = (float)(pHomeInfo->ElmoZoneX + myrand(0, pHomeInfo->ElmoZoneLX));
		m_curz = (float)(pHomeInfo->ElmoZoneZ + myrand(0, pHomeInfo->ElmoZoneLZ)); 
	}
}

void CUser::SendToRegion(Packet *pkt, CUser *pExceptUser /*= NULL*/)
{
	g_pMain->Send_Region(pkt, GetMap(), GetRegionX(), GetRegionZ(), pExceptUser);
}

void CUser::OnDeath(Unit *pKiller)
{
	if (m_bResHpType == USER_DEAD)
		return;

	m_bResHpType = USER_DEAD;

	if (getFame() == COMMAND_CAPTAIN)
	{
		ChangeFame(CHIEF);
		if (GetNation() == KARUS)
			g_pMain->Announcement(KARUS_CAPTAIN_DEPRIVE_NOTIFY, KARUS);
		else
			g_pMain->Announcement(ELMORAD_CAPTAIN_DEPRIVE_NOTIFY, ELMORAD);
	}

	InitType3();
	InitType4();

	if (pKiller != NULL)
	{
		if (pKiller->isNPC())
		{
			CNpc *pNpc = TO_NPC(pKiller);
			if (pNpc->GetType() == NPC_PATROL_GUARD
				|| (GetZoneID() != GetNation() && GetZoneID() <= ELMORAD))
				ExpChange(-m_iMaxExp / 100);
			else
				ExpChange(-m_iMaxExp / 20);
		}
		else
		{
			CUser *pUser = TO_USER(pKiller);

			// Looks like we died of "natural causes!" (probably residual DOT)
			if (pUser == this)
			{
				m_sWhoKilledMe = -1;
			}
			// Someone else killed us? Need to clean this up.
			else
			{
				// Did we get killed in the snow war? Handle appropriately.
				if (GetZoneID() == ZONE_SNOW_BATTLE 
					&& g_pMain->m_byBattleOpen == SNOW_BATTLE)
				{
					pUser->GoldGain(SNOW_EVENT_MONEY);

					if (GetNation() == KARUS)
						g_pMain->m_sKarusDead++;
					else 
						g_pMain->m_sElmoradDead++;
				}
				// Otherwise...
				else
				{
					if (!pUser->isInParty())
						pUser->LoyaltyChange(GetID());
					else
						pUser->LoyaltyDivide(GetID());

					pUser->GoldChange(GetID(), 0);

					if (GetZoneID() != GetNation() && GetZoneID() <= ELMORAD)
						ExpChange(-(m_iMaxExp / 100));
				}
			
				m_sWhoKilledMe = pUser->GetID();
			}
		}
	}

	Unit::OnDeath(pKiller);
}

// We have no clan handler, we probably won't need to implement it (but we'll see).
void CUser::SendClanUserStatusUpdate(bool bToRegion /*= true*/)
{
	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_MODIFY_FAME));
	result	<< uint8(1) << GetSocketID() 
			<< GetClanID() << getFame();

	// TO-DO: Make this region code user-specific to perform faster.
	if (bToRegion)
		SendToRegion(&result);
	else
		Send(&result);
}

void CUser::SendPartyStatusUpdate(uint8 bStatus, uint8 bResult /*= 0*/)
{
	if (!isInParty())
		return;

	Packet result(WIZ_PARTY, uint8(PARTY_STATUSCHANGE));
	result << GetSocketID() << bStatus << bResult;
	g_pMain->Send_PartyMember(m_sPartyIndex, &result);
}

void CUser::HandleHelmet(Packet & pkt)
{
	if (isDead())
		return;

	Packet result(WIZ_HELMET);
	pkt >> m_bIsHidingHelmet;
#if __VERSION >= 1900
	// pkt >> cospre flag
#endif
	result	<< m_bIsHidingHelmet 
#if __VERSION >= 1900
//			<< cospre flag
#endif
			<< uint32(GetSocketID());
	SendToRegion(&result);
}

bool CUser::isAttackZone()
{
	// this needs to be handled more generically (i.e. bounds loaded from the database, or their existing SMD method)
	if (GetZoneID() == 21 
		&& ((GetX() < 735.0f && GetX() > 684.0f) 
		&& ((GetZ() < 491.0f && GetZ() > 440.0f) || (GetZ() < 411.0f && GetZ() > 360.0f)))
		|| ((GetZoneID() == 1  && g_pMain->m_byKarusOpenFlag) || (GetZoneID() == 2 && g_pMain->m_byElmoradOpenFlag)) )  //Taking into account invasions
	return true;

	return GetMap()->isAttackZone();
}

bool CUser::CanUseItem(uint32 itemid, uint16 count)
{
	_ITEM_TABLE* pItem = pItem = g_pMain->GetItemPtr(itemid);
	return (pItem != NULL
		// Check the item's class requirement
		|| (pItem->m_bClass == 0 || JobGroupCheck(pItem->m_bClass))
		// Check the item's level requirement
		|| (pItem->m_bReqLevel <= GetLevel() && pItem->m_bReqLevelMax >= GetLevel())
		|| CheckExistItem(itemid, count));
}

void CUser::SendUserStatusUpdate(UserStatus type, UserStatusBehaviour status)
{
	Packet result(WIZ_ZONEABILITY, uint8(2));
	result << uint8(type) << uint8(status);
	/*
			  1				, 1 = Damage over time
			  1				, 2 = Cure damage over time
			  2				, 1 = poison (purple)
			  2				, 2 = Cure poison
			  3				, 1 = disease (green)
			  3				, 2 = Cure disease
			  4				, 1 = blind
			  5				, 1 = HP is grey (not sure what this is)
			  5				, 2 = Cure grey HP
	*/
	Send(&result);
}

/**
 * @brief	Gets an item's prototype from a slot in a player's inventory.
 *
 * @param	pos	The position of the item in the player's inventory.
 * @returns	NULL if an invalid position is specified, or if there's no item at that position.
 * 			The item's prototype (_ITEM_TABLE *) otherwise.
 */
_ITEM_TABLE* CUser::GetItemPrototype(uint8 pos, _ITEM_DATA *& pItem)
{
	if (pos >= INVENTORY_TOTAL)
		return NULL;

	pItem = GetItem(pos);
	return pItem->nNum == 0 ? NULL : g_pMain->GetItemPtr(pItem->nNum);
}

/* TO-DO: Move all these to their own handler file */

/**
 * @brief	Packet handler for the assorted systems that
 * 			were deemed to come under the 'upgrade' system.
 *
 * @param	pkt	The packet.
 */
void CUser::ItemUpgradeProcess(Packet & pkt)
{
	uint8 opcode = pkt.read<uint8>();
	switch (opcode)
	{
	case ITEM_UPGRADE:
		ItemUpgrade(pkt);
		break;

	case ITEM_ACCESSORIES:
		ItemUpgradeAccessories(pkt);
		break;

	case ITEM_BIFROST_EXCHANGE:
		BifrostPieceProcess(pkt);
		break;

	case ITEM_UPGRADE_REBIRTH:
		ItemUpgradeRebirth(pkt);
		break;

	case ITEM_SEAL:
		ItemSealProcess(pkt);
		break;

	case ITEM_CHARACTER_SEAL:
		CharacterSealProcess(pkt);
		break;
	}
}

// Some item ID definitions for moving (and using) later
#define MIN_ITEM_ID 100000000
#define MAX_ITEM_ID 999999999

#define MAGE_EARRING 310310004
#define WARRIOR_EARRING 310310005
#define ROGUE_EARRING 310310006
#define PRIEST_EARRING 310310007

#define UPGRADE_RUBY_EARRING_MIN 310110005
#define UPGRADE_RUBY_EARRING_MAX 310110007

#define UPGRADE_PEARL_EARRING_MIN 310150005
#define UPGRADE_PEARL_EARRING_MAX 310150007

/**
 * @brief	Packet handler for the standard item upgrade system.
 *
 * @param	pkt	The packet.
 */
void CUser::ItemUpgrade(Packet & pkt)
{
	enum UpgradeErrorCodes
	{
		UpgradeFailed		= 0,
		UpgradeSucceeded	= 1,
		UpgradeTrading		= 2,
		UpgradeNeedCoins	= 3,
		UpgradeNoMatch		= 4,
		UpgradeRental		= 5
	};

	Packet result(WIZ_ITEM_UPGRADE, uint8(ITEM_UPGRADE));
	int32 nItemID[10]; int8 bPos[10];
	uint16 sNpcID;
	int8 bResult = UpgradeNoMatch;

	if (isTrading() || isMerchanting())
	{
		bResult = UpgradeTrading;
		goto fail_return;
	}

	pkt >> sNpcID;
	for (int i = 0; i < 10; i++)
		pkt >> nItemID[i] >> bPos[i];

	if (bPos[0] >= HAVE_MAX)
		return;

	_ITEM_DATA  * pOriginItem = GetItem(SLOT_MAX + bPos[0]);
	_ITEM_TABLE * proto;
	if (pOriginItem->nNum != nItemID[0]
		|| (proto = g_pMain->GetItemPtr(nItemID[0])) == NULL)
		goto fail_return; // error with error code UpgradeNoMatch ("Items required for upgrade do not match")
	else if (pOriginItem->isRented() 
		|| pOriginItem->isSealed()) // unsure if there's another error code for sealed items
	{
		bResult = UpgradeRental;
		goto fail_return;
	}

	// Check the first 5 (+1, first is technically the item we're upgrading) slots. 
	for (int x = 0; x < 5; x++)
	{
		if (bPos[x+1] >= HAVE_MAX 
			&& nItemID[x+1] != GetItem(bPos[x+1])->nNum)
			goto fail_return;
	}
	
	{ // scoped lock to prevent race conditions
		FastGuard lock(g_pMain->m_ItemUpgradeArray.m_lock);
		int nReqOriginItem = nItemID[0] % 1000;
		_ITEM_UPGRADE * pUpgrade = NULL;
		foreach_stlmap (itr, g_pMain->m_ItemUpgradeArray)
		{
			pUpgrade = itr->second;
			if (pUpgrade->nOriginItem != nReqOriginItem)
				continue;

			if ((nItemID[0] / MIN_ITEM_ID) != pUpgrade->nIndex / 100000
				&& pUpgrade->nIndex < 300000) 
				continue;

			if (pUpgrade->bOriginType != -1 
				&& pUpgrade->nIndex < 200000 && pUpgrade->nIndex >= 100000)
			{
				switch (pUpgrade->bOriginType)
				{
					case 0:
						if (!proto->isStaff()) 
							continue;
						break;

					case 1:
						if (proto->m_bKind != 21)
							continue;
						break;

					case 2:
						if (proto->m_bKind != 22)
							continue;
						break;

					case 3:
						if (proto->m_bKind != 31) 
							continue;
						break;

					case 4:
						if (proto->m_bKind != 32) 
							continue;
						break;

					case 5:
						if (proto->m_bKind != 41) 
							continue;
						break;

					case 6:
						if (proto->m_bKind != 42) 
							continue;
						break;

					case 7:
						if (proto->m_bKind != 51) 
							continue;
						break;

					case 8:
						if (proto->m_bKind != 52) 
							continue;
						break;

					case 9:
						if (proto->m_bKind != 70 && proto->m_bKind != 71) 
							continue;
						break;

					case 10:
						if (proto->m_bKind != 110) 
							continue;
						break;

					case 11:
						if ((nItemID[0] / 10000000) != 19) 
							continue;
						break;

					case 12:
						if (proto->m_bKind != 60) 
							continue;
						break;
				}
			}
			else
			{
				if (((pUpgrade->nIndex >= 300000 || pUpgrade->nIndex < 200000)
					&& (pUpgrade->nIndex >= 400000 || pUpgrade->nIndex < 300000 || proto->m_bSlot - proto->m_bKind == 73))
					|| (proto->m_bKind - proto->m_bSlot) == 8)
					continue;

				break;
			}

			if ((nItemID[0] / MIN_ITEM_ID) != (pUpgrade->nIndex / 100000) 
				&& ((pUpgrade->nIndex / 100000) == 1 
					|| (pUpgrade->nIndex / 100000) == 2))
				continue;

			for (int x = 0; x < 5; x++)
			{
				if (bPos[x+1] >= HAVE_MAX 
					&& nItemID[x+1] != GetItem(bPos[x+1])->nNum 
					&& nItemID[x+1] != pUpgrade->nReqItem[x])
					goto fail_return;

				if (pUpgrade->nReqItem[x+1] != 0)
					continue;

				if (pUpgrade->nReqItem[6] == 255 
					|| pUpgrade->nReqItem[6] == proto->m_ItemType)
				{
					if (!hasCoins(pUpgrade->nReqNoah))
					{
						bResult = UpgradeNeedCoins;
						goto fail_return;
					}

					bResult = UpgradeSucceeded;
					break;
				}
			}
		}

		// If we ran out of upgrades to search through, it failed.
		if (bResult != UpgradeSucceeded
			|| pUpgrade == NULL)
			goto fail_return;

		// Take the required coins
		GoldLose(pUpgrade->nReqNoah);

		// Generate a random number, test if the item burned.
		int rand = myrand(9000, 10000);
		if (pUpgrade->sGenRate <= rand)
		{
			bResult = UpgradeFailed;
			RobItem(nItemID[0], 1); // remove the item
		}
		else
		{
			// Generate the new item ID
			int nNewItemID = pOriginItem->nNum + pUpgrade->nGiveItem;

			// Does this new item exist?
			_ITEM_TABLE * newProto = g_pMain->GetItemPtr(nNewItemID);
			if (newProto == NULL)
			{ // if not, just say it doesn't match. No point removing the item anyway (like official :/).
				bResult = UpgradeNoMatch;
				goto fail_return;
			}

			// Update the user's item in their inventory with the new item
			pOriginItem->nNum = nNewItemID;

			// Reset the durability also, to the new cap.
			pOriginItem->sDuration = newProto->m_sDuration;

			// Remove all required items, if applicable.
			for (int i = 0; i < 10; i++)
			{
				if (bPos[i] >= HAVE_MAX)
					continue;

				_ITEM_DATA * pItem = GetItem(bPos[i]);
				if (pItem->nNum == 0 
					|| pItem->sCount == 0)
					continue;

				pItem->sCount--;
				if (pItem->sCount == 0)
					memset(pItem, 0, sizeof(pItem));
			}

			// Replace the item ID in the list for the packet
			nItemID[0] = nNewItemID;
		}
	} // end of scoped lock

	result << bResult;
	foreach_array (i, nItemID)
		result << nItemID[i] << bPos[i];
	Send(&result);

	// Send the result to everyone in the area
	// (i.e. make the anvil do its burned/upgraded indicator thing)
	result.Initialize(WIZ_OBJECT_EVENT);
	result << uint8(OBJECT_ANVIL) << bResult << sNpcID;
	SendToRegion(&result);

	return;
fail_return:
	result << bResult;

	// The item data's only sent when not experiencing a general error
	if (bResult != 2)
	{
		foreach_array (i, nItemID)
			result << nItemID[i] << bPos[i];
	}
	Send(&result);
}

/**
 * @brief	Packet handler for the accessory upgrade system.
 *
 * @param	pkt	The packet.
 */
void CUser::ItemUpgradeAccessories(Packet & pkt)
{
}

/**
 * @brief	Packet handler for the Chaotic Generator system
 * 			which is used to exchange Bifrost pieces/fragments.
 *
 * @param	pkt	The packet.
 */
void CUser::BifrostPieceProcess(Packet & pkt)
{
}

/**
 * @brief	Packet handler for the upgrading of 'rebirthed' items.
 *
 * @param	pkt	The packet.
 */
void CUser::ItemUpgradeRebirth(Packet & pkt)
{
}

/**
 * @brief	Packet handler for the item sealing system.
 *
 * @param	pkt	The packet.
 */
void CUser::ItemSealProcess(Packet & pkt)
{
	#define ITEM_SEAL_PRICE 1000000
	enum
	{
		SEAL_TYPE_SEAL		= 1,
		SEAL_TYPE_UNSEAL	= 2,
		SEAL_TYPE_KROWAZ	= 3
	};

	enum SealErrorCodes
	{
		SealErrorNone			= 0, // no error, success!
		SealErrorFailed			= 2, // "Seal Failed."
		SealErrorNeedCoins		= 3, // "Not enough coins."
		SealErrorInvalidCode	= 4, // "Invalid Citizen Registry Number" (i.e. invalid code/password)
		SealErrorPremiumOnly	= 5, // "Only available to premium users"
		SealErrorFailed2		= 6, // "Seal Failed."
		SealErrorTooSoon		= 7, // "Please try again. You may not repeat this function instantly."
	};

	// Seal type
	uint8 opcode = pkt.read<uint8>();

	Packet result(WIZ_ITEM_UPGRADE, uint8(ITEM_SEAL));
	result << opcode;

	switch (opcode)
	{
		// Used when sealing an item.
		case SEAL_TYPE_SEAL:
		{
			string strPasswd;
			uint32 nItemID; 
			int16 unk0; // set to -1 in this case
			uint8 bSrcPos, bResponse = SealErrorNone;
			pkt >> unk0 >> nItemID >> bSrcPos >> strPasswd;

			/* 
				Most of these checks are handled client-side, so we shouldn't need to provide error messages.
				Also, item sealing requires certain premium types (gold, platinum, etc) - need to double-check 
				these before implementing this check.
			*/

			// is this a valid position? (need to check if it can be taken from new slots)
			if (bSrcPos >= HAVE_MAX 
				// does the item exist where the client says it does?
				|| GetItem(SLOT_MAX + bSrcPos)->nNum != nItemID
				// i ain't be allowin' no stealth items to be sealed!
				|| GetItem(SLOT_MAX + bSrcPos)->nSerialNum == 0)
				bResponse = SealErrorFailed;
			// is the password valid by client limits?
			else if (strPasswd.empty() || strPasswd.length() > 8)
				bResponse = SealErrorInvalidCode;
			// do we have enough coins?
			else if (m_iGold < ITEM_SEAL_PRICE)
				bResponse = SealErrorNeedCoins;

			_ITEM_TABLE* pItem = g_pMain->m_ItemtableArray.GetData(nItemID);

#if 0 // this doesn't look right
			// If the item's not equippable we not be lettin' you seal no moar!
			if (pItem->m_bSlot >= SLOT_MAX)
				bResponse = SealErrorFailed;
#endif

			// If no error, pass it along to the database.
			if (bResponse == SealErrorNone)
			{
				g_pMain->AddDatabaseRequest(result, this);
			}
			// If there's an error, tell the client.
			// From memory though, there was no need -- it handled all of these conditions itself
			// so there was no need to differentiate (just ignore the packet). Need to check this.
			else 
			{
				result << bResponse;
				Send(&result);
			}
		} break;

		// Used when unsealing an item.
		case SEAL_TYPE_UNSEAL:
		{
			string strPasswd;
			uint32 nItemID; 
			int16 unk0; // set to -1 in this case
			uint8 bSrcPos, bResponse = SealErrorNone;
			pkt >> unk0 >> nItemID >> bSrcPos >> strPasswd;

			if (GetItem(SLOT_MAX+bSrcPos)->bFlag != ITEM_FLAG_SEALED
				|| GetItem(SLOT_MAX+bSrcPos)->nNum != nItemID)
				bResponse = SealErrorFailed;
			else if (strPasswd.empty() || strPasswd.length() > 8)
				bResponse = SealErrorInvalidCode;

			// If no error, pass it along to the database.
			if (bResponse == SealErrorNone)
			{
				result << nItemID << bSrcPos << strPasswd << bResponse << false;
				g_pMain->AddDatabaseRequest(result, this);
			}
			// If there's an error, tell the client.
			// From memory though, there was no need -- it handled all of these conditions itself
			// so there was no need to differentiate (just ignore the packet). Need to check this.
			else
			{
				result << bResponse;
				Send(&result);
			}
		} break;

		// Used when binding a Krowaz item (used to take it from not bound -> bound)
		case SEAL_TYPE_KROWAZ:
		{
			string strPasswd = "1";
			uint32 nItemID;
			uint8 bSrcPos = 0 , unk3, bResponse = 1;
			uint16 unk1, unk2;
			pkt >> unk1 >> nItemID >> bSrcPos >> unk3 >> unk2;

			Packet result(WIZ_ITEM_UPGRADE, uint8(ITEM_SEAL));
			result << uint8(3) << uint8(1) << nItemID << bSrcPos;
			Send(&result);
		} break;
	}
}

/**
 * @brief	Packet handler for the character sealing system.
 *
 * @param	pkt	The packet.
 */
void CUser::CharacterSealProcess(Packet & pkt)
{
}

/**
 * @brief	Checks & removes any expired skills from
 * 			the saved magic list.
 */
void CUser::CheckSavedMagic()
{
	FastGuard lock(m_savedMagicLock);
	if (m_savedMagicMap.empty())
		return;

	set<uint32> deleteSet;
	foreach (itr, m_savedMagicMap)
	{
		if (itr->second <= UNIXTIME)
			deleteSet.insert(itr->first);
	}
	foreach (itr, deleteSet)
		m_savedMagicMap.erase(*itr);
}

/**
 * @brief	Inserts a skill to the saved magic list
 * 			to persist across zone changes/logouts.
 *
 * @param	nSkillID 	Identifier for the skill.
 * @param	sDuration	The duration.
 */
void CUser::InsertSavedMagic(uint32 nSkillID, uint16 sDuration)
{
	FastGuard lock(m_savedMagicLock);
	UserSavedMagicMap::iterator itr = m_savedMagicMap.find(nSkillID);

	// If the buff is already in the savedBuffMap there's no need to add it again!
	if (itr != m_savedMagicMap.end())
		return;
	
	m_savedMagicMap.insert(make_pair(nSkillID, UNIXTIME + sDuration));
}

/**
 * @brief	Checks if the given skill ID is already in our 
 * 			saved magic list.
 *
 * @param	nSkillID	Identifier for the skill.
 *
 * @return	true if the skill exists in the saved magic list, false if not.
 */
bool CUser::HasSavedMagic(uint32 nSkillID)
{
	FastGuard lock(m_savedMagicLock);
	return m_savedMagicMap.find(nSkillID) != m_savedMagicMap.end();
}

/**
 * @brief	Gets the duration for a saved skill, 
 * 			if applicable.
 *
 * @param	nSkillID	Identifier for the skill.
 *
 * @return	The saved magic duration.
 */
int16 CUser::GetSavedMagicDuration(uint32 nSkillID)
{
	FastGuard lock(m_savedMagicLock);
	auto itr = m_savedMagicMap.find(nSkillID);
	if (itr == m_savedMagicMap.end())
		return 0;

	return int16(itr->second - UNIXTIME);
}

/**
 * @brief	Recasts any saved skills on login/zone change.
 */
void CUser::RecastSavedMagic()
{
	FastGuard lock(m_savedMagicLock);
	UserSavedMagicMap castSet;
	foreach (itr, m_savedMagicMap)
	{
		if (itr->first != 0)
			castSet.insert(make_pair(itr->first, itr->second));
	}

	if (castSet.empty())
		return;

	foreach (itr, castSet)
	{
		_MAGIC_TABLE *pSkill = g_pMain->m_MagictableArray.GetData(itr->first);
		Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_EFFECTING));
		result << pSkill->iNum << GetSocketID() << GetSocketID() << uint16(0) << uint16(1) << uint16(0) << uint16(itr->second - UNIXTIME) << uint16(0) << uint16(0);
		switch (pSkill->bType[0])
		{
			case 6:
				// Not allowing transformations in PvP zones!
				if (isAttackZone())
				{
					m_savedMagicMap.erase(itr->first);
					return;
				}

				StateChangeServerDirect(3, ABNORMAL_NORMAL);
				UpdateVisibility(INVIS_NONE);
				break;

			case 9:
				//To-do : Add support for Guards, until then we don't need this line.
				//_MAGIC_TYPE9 *pType = g_pMain->m_Magictype9Array.GetData(pSkill->iNum);
				break;
		}
		CMagicProcess::MagicPacket(result, this, true);
	}

}

/**
 * @brief	Displays the player rankings board in PK zones, 
 * 			when left-ALT is held.
 *
 * @param	pkt	The packet.
 */
void CUser::HandlePlayerRankings(Packet & pkt)
{
	/* 
		NOTE: This is a mockup. 
		It should not be used in its current state for anything
		other than testing.
	*/
	Packet result(WIZ_RANK, uint8(1));

	CKnights * pKnights = g_pMain->GetClanPtr(GetClanID());
	uint16 sClanID = 0;
	uint16 sMarkVersion = 0;
	std::string strClanName;

	// Just testing with the active clan, 
	// as we don't currently store the clan ID in the rankings tables.
	if (pKnights != NULL)
	{
		sClanID = GetClanID();
		sMarkVersion = pKnights->m_sMarkVersion;
		strClanName = pKnights->m_strName;
	}

	FastGuard lock(g_pMain->m_userRankingsLock);

	// List top 10 rankings for each nation
	for (int nation = KARUS_ARRAY; nation <= ELMORAD_ARRAY; nation++)
	{
		uint16 sCount = 0;
		size_t wpos = result.wpos();
		result << sCount; // placeholder

		foreach (itr, g_pMain->m_playerRankings[nation])
		{
			if (itr->first > 10)
				break;

			result	<< itr->second->strUserID[nation]
					<< true // seems to be 0 or 1, not sure what it does though
					<< sClanID // clan ID
					<< sMarkVersion // mark/symbol version
					<< strClanName // clan name
					<< itr->second->nLoyalty[nation]
					<< uint16(123); // bonus from prem NP

			sCount++;
		}

		result.put(wpos, sCount);
		wpos = result.wpos();
	}

	result	<< uint16(1) // I don't know what this is, changing it to 0 has no effect...
			// player's own stats
			<< GetLoyalty()
			<< uint16(123);

	Send(&result);
}

/**
 * @brief	Handles packets related to the mining system.
 * 			Also handles soccer-related packets (yuck).
 *
 * @param	pkt	The packet.
 */
void CUser::HandleMiningSystem(Packet & pkt)
{
	uint8 opcode;
	pkt >> opcode;
	
	switch (opcode)
	{
	case MiningStart:
		HandleMiningStart(pkt);
		break;

	case MiningAttempt:
		HandleMiningAttempt(pkt);
		break;

	case MiningStop:
		HandleMiningStop(pkt);
		break;

	case MiningSoccer:
		HandleSoccer(pkt);
		break;
	}
}

/**
 * @brief	Handles users requesting to start mining.
 * 			NOTE: This is a mock-up, so be warned that it does not 
 * 			handle checks such as identifying if the user is allowed
 * 			to mine in this area.
 *
 * @param	pkt	The packet.
 */
void CUser::HandleMiningStart(Packet & pkt)
{
	Packet result(WIZ_MINING, uint8(MiningStart));
	uint16 resultCode = MiningResultSuccess;

	// Are we mining already?
	if (isMining())
		resultCode = MiningResultMiningAlready;

	// Do we have a pickaxe? Is it worn?
	_ITEM_DATA * pItem;
	_ITEM_TABLE * pTable = GetItemPrototype(RIGHTHAND, pItem);
	if (pItem == NULL || pTable == NULL
		|| pItem->sDuration <= 0
		|| !pTable->isPickaxe())
		resultCode = MiningResultNotPickaxe;

	result << resultCode;

	// If nothing went wrong, allow the user to start mining.
	// Be sure to let everyone know we're mining.
	if (resultCode == MiningResultSuccess)
	{
		m_bMining = true;
		result << GetID();
		SendToRegion(&result);
	}
	else
	{
		Send(&result);
	}
}

/**
 * @brief	Handles a user's mining attempt by finding a random reward (or none at all).
 * 			This is sent automatically by the client every MINING_DELAY (5) seconds.
 *
 * @param	pkt	The packet.
 */
void CUser::HandleMiningAttempt(Packet & pkt)
{
	if (!isMining())
		return;

	Packet result(WIZ_MINING, uint8(MiningAttempt));
	uint16 resultCode = MiningResultSuccess;

	// Do we have a pickaxe? Is it worn?
	_ITEM_DATA * pItem;
	_ITEM_TABLE * pTable = GetItemPrototype(RIGHTHAND, pItem);
	if (pItem == NULL || pTable == NULL
		|| pItem->sDuration <= 0 // are we supposed to wear the pickaxe on use? Need to verify.
		|| !pTable->isPickaxe())
		resultCode = MiningResultNotPickaxe;

	// Check to make sure we're not spamming the packet...
	if ((UNIXTIME - m_tLastMiningAttempt) < MINING_DELAY)
		resultCode = MiningResultMiningAlready; // as close an error as we're going to get...

	// Effect to show to clients
	uint16 sEffect = 0;

	// This is just a mock-up based on another codebase's implementation.
	// Need to log official data to get a proper idea of how it behaves, rate-wise,
	// so that we can then implement it more dynamically.
	if (resultCode == MiningResultSuccess)
	{
		int rate = myrand(1, 100), random = myrand(1, 10000);
		if (rate <= 50 && random <= 5000)
		{
			ExpChange(1);
			sEffect = 13082; // "XP" effect
		}
		else if (rate >= 50 && rate <= 75 && random <= 7500)
		{ 
			GiveItem(389043000); // Sling
			sEffect = 13081; // "Item" effect
		}
		else if (rate >= 75 && rate <= 100 && random <= 10000)
		{
			GiveItem(399210000); // Mysterious Ore
			sEffect = 13081; // "Item" effect
		}
		else
		{
			resultCode = MiningResultNothingFound;
		}

		m_tLastMiningAttempt = UNIXTIME;
	}

	result << resultCode << GetID() << sEffect;
	if (resultCode != MiningResultSuccess
		&& resultCode != MiningResultNothingFound)
	{
		// Tell us the error first
		Send(&result);

		// and then tell the client to stop mining
		HandleMiningStop(pkt);
		return;
	}

	SendToRegion(&result);
}

/**
 * @brief	Handles when a user stops mining.
 *
 * @param	pkt	The packet.
 */
void CUser::HandleMiningStop(Packet & pkt)
{
	if (!isMining())
		return;

	Packet result(WIZ_MINING, uint8(MiningStop));
	result << uint16(1) << GetID();
	m_bMining = false;
	SendToRegion(&result);
}

void CUser::HandleSoccer(Packet & pkt)
{
}
