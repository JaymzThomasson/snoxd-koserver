// User.cpp: implementation of the CUser class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EbenezerDlg.h"
#include "User.h"
#include "AiPacket.h"
#include "Map.h"
#include <set>

using namespace std;

CUser::CUser(uint16 socketID, SocketMgr *mgr) : KOSocket(socketID, mgr, -1, 16384, 3172), Unit(true)
{	
	m_MagicProcess.m_pSrcUser = this;
}

void CUser::OnConnect()
{
	KOSocket::OnConnect();
	Initialize();
}

void CUser::Initialize()
{
	Unit::Initialize();

	memset(&m_pUserData, 0x00, sizeof(_USER_DATA));
	m_pUserData.m_bAuthority = 1;
	m_pUserData.m_sBind = -1;

	m_state = GAME_STATE_CONNECTED;

	m_strAccountID.clear();
	m_bSelectedCharacter = false;
	m_bStoreOpen = false;
	m_bPartyLeader = false;
	m_bIsTransformed = false;
	m_bIsChicken = false;
	m_bIsHidingHelmet = false;
	m_bPremiumMerchant = false;

	m_bMerchantState = MERCHANT_STATE_NONE;
	m_bInvisibilityType = INVIS_NONE;

	m_sDirection = 0;

	m_sItemMaxHp = 0;
	m_sItemMaxMp = 0;
	m_sItemWeight = 0;
	m_sItemHit = 0;
	m_sItemAc = 0;

	memset(m_sStatItemBonuses, 0, sizeof(uint16) * STAT_COUNT);
	memset(m_bStatBuffs, 0, sizeof(uint8) * STAT_COUNT);

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
	m_sPrivateChatUser = -1;
	m_bNeedParty = 0x01;

	m_fHPLastTimeNormal = 0.0f;		// For Automatic HP recovery. 
	m_fHPStartTimeNormal = 0.0f;
	m_bHPAmountNormal = 0;
	m_bHPDurationNormal = 0;
	m_bHPIntervalNormal = 5;

	m_fAreaLastTime = 0.0f;		// For Area Damage spells Type 3.
	m_fAreaStartTime = 0.0f;
	m_bAreaInterval = 5;
	m_iAreaMagicID = 0;

	m_fSpeedHackClientTime = 0.0f;
	m_fSpeedHackServerTime = 0.0f;
	m_bSpeedHackCheck = 0;

	m_fBlinkStartTime = 0.0f;

	m_sAliveCount = 0;
	m_bAbnormalType = ABNORMAL_NORMAL;	// User starts out in normal size.

	m_sWhoKilledMe = -1;
	m_iLostExp = 0;

	m_fLastTrapAreaTime = 0.0f;

	memset(m_iSelMsgEvent, -1,  MAX_MESSAGE_EVENT);

	m_sEventNid = -1;
	m_bZoneChangeFlag = FALSE;
	m_bRegeneType = 0;
	m_fLastRegeneTime = 0.0f;
	m_bZoneChangeSameZone = FALSE;

	m_nTransformationItem = 0;
	m_fTransformationStartTime = 0.0f;
	m_sTransformationDuration = 0;

	while( !m_arUserEvent.empty() )
	m_arUserEvent.pop_back();
}

void CUser::OnDisconnect()
{
	KOSocket::OnDisconnect();

	g_pMain->RemoveSessionNames(this);

	if (!isInGame())
		return;

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
	LogOut();
}

bool CUser::HandlePacket(Packet & pkt)
{
	int index = 0;
	float	currenttime = FLT_MIN;

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
	else if (m_pUserData.m_Accountid[0] == '\0')
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
		RecvRegene(pkt);
		break;
	case WIZ_REQ_USERIN:
		RequestUserIn(pkt);
		//Request merchant characters too.
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
		QuestDataRequest(pkt);
		break;
	case WIZ_MERCHANT:
		MerchantProcess(pkt);
		break;
	case WIZ_MAGIC_PROCESS:
		m_MagicProcess.MagicPacket(pkt);
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
		g_pMain->m_KnightsManager.PacketProcess(this, pkt);
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
		ClientEvent(pkt);
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
	case WIZ_HELMET:
		HandleHelmet(pkt);
		break;
	case WIZ_CAPE:
		HandleCapeChange(pkt);
		break;
	case WIZ_CHALLENGE:
		HandleChallenge(pkt);
		break;

	default:
		TRACE("[SID=%d] Unknown packet %X\n", GetSocketID(), command);
		return false;
	}

	currenttime = TimeGet();

	if (command == WIZ_GAMESTART)
	{
		m_fHPLastTimeNormal = currenttime;
		fill_n(m_fHPLastTime, MAX_TYPE3_REPEAT, currenttime);
	}	

	if (!isBlinking() && m_fHPLastTimeNormal != 0.0f && (currenttime - m_fHPLastTimeNormal) > m_bHPIntervalNormal)
		HPTimeChange( currenttime );	// For Sitdown/Standup HP restoration.

	if (m_bType3Flag) {     // For Type 3 HP Duration.
		for (int i = 0 ; i < MAX_TYPE3_REPEAT ; i++) {	
			if( m_fHPLastTime[i] != 0.0f && (currenttime - m_fHPLastTime[i]) > m_bHPInterval[i] ) {
				HPTimeChangeType3(currenttime);	
				break;
			}
		}
	} 

	if (m_bType4Flag)		// For Type 4 Stat Duration.
		Type4Duration(currenttime);
	
	if (m_bIsTransformed && (currenttime - m_fTransformationStartTime) > m_sTransformationDuration)
		m_MagicProcess.Type6Cancel();

	if (isBlinking())		// Should you stop blinking?
		BlinkTimeCheck(currenttime);
	return true;
}

void CUser::SendLoyaltyChange(int32 nChangeAmount /*= 0*/)
{
	Packet result(WIZ_LOYALTY_CHANGE, uint8(1));

	m_pUserData.m_iLoyalty += nChangeAmount;
	m_pUserData.m_iLoyaltyMonthly += nChangeAmount;

	if (m_pUserData.m_iLoyalty < 0)
		m_pUserData.m_iLoyalty = 0;
	if (m_pUserData.m_iLoyaltyMonthly < 0)
		m_pUserData.m_iLoyaltyMonthly = 0;

	result	<< m_pUserData.m_iLoyalty << m_pUserData.m_iLoyaltyMonthly
			<< uint32(0) // Clan donations(? Donations made by this user? For the clan overall?)
			<< uint32(0); // Premium NP(? Additional NP gained?)

	Send(&result);
}

void CUser::ChangeFame(uint8 bFame)
{
	Packet result(WIZ_AUTHORITY_CHANGE, uint8(COMMAND_AUTHORITY));

	m_pUserData.m_bFame = bFame;
	result << GetSocketID() << getFame();
	SendToRegion(&result);
}

void CUser::SendServerIndex()
{
	Packet result(WIZ_SERVER_INDEX);
	result << uint16(1) << uint16(g_pMain->m_nServerNo);
	Send(&result);
}

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

void CUser::SkillDataLoad()
{
	Packet result(WIZ_SKILLDATA, uint8(SKILL_DATA_LOAD));
	g_pMain->AddDatabaseRequest(result, this);
}

void CUser::RecvSkillDataLoad(Packet & pkt)
{
	uint16 sCount = 0;
	uint8 bSuccess = pkt.read<uint8>();
	if (!bSuccess)
	{
		sCount = 0;
	}
	else
	{
		pkt >> sCount;
		if (sCount < 0 || sCount > 64)
			sCount = 0;
	}

	Packet result(WIZ_SKILLDATA, uint8(SKILL_DATA_LOAD));
	result << sCount;

	for (int i = 0; i < sCount; i++) 
		result << pkt.read<uint32>();

	Send(&result);
}

void CUser::UserDataSaveToAgent()
{
	if (!isInGame())
		return;

	Packet result(WIZ_DATASAVE);
	result << m_pUserData.m_Accountid << m_pUserData.m_id;
	g_pMain->AddDatabaseRequest(result, this);
}

void CUser::LogOut()
{
	CTime t = CTime::GetCurrentTime();
	g_pMain->WriteLog("[%s : %s Logout : %d:%d:%d]\r\n", m_pUserData.m_Accountid, m_pUserData.m_id, t.GetHour(), t.GetMinute(), t.GetSecond());

	CUser *pUser = g_pMain->GetUserPtr(m_pUserData.m_Accountid, TYPE_ACCOUNT);
	if (pUser && (pUser->GetSocketID() != GetSocketID()))
	{
		TRACE("[SID=%D] %s : %s logged out\n", GetSocketID(), m_pUserData.m_Accountid, m_pUserData.m_id);
		return;
	}

	if (m_pUserData.m_id[0] == 0) 
		return; 

	Packet result(AG_USER_LOG_OUT);
	result << m_strAccountID << GetName();
	g_pMain->Send_AIServer(&result);

	// synchronous request
	ReqUserLogOut();
}

void CUser::SendMyInfo()
{
	C3DMap* pMap = GetMap();
	CKnights* pKnights = NULL;

	if (!pMap->IsValidPosition( m_pUserData.m_curx, m_pUserData.m_curz, 0.0f))
	{
		short x = 0, z = 0;
		GetStartPosition(x, z); 

		m_pUserData.m_curx = (float)x;
		m_pUserData.m_curz = (float)z;
	}

	// Unlock skill data (level 70 skill quest).
	// NOTE: This is just temporary until we can load quest data.
	// At which time, we'll just send a list of quest IDs & their states (as is done here, just.. hardcoded)
	Packet result(WIZ_QUEST, uint8(1));
	uint16 Class = m_pUserData.m_sClass % 100;
	if (Class == 1 || Class == 5 || Class == 6)
		result << uint16(3) << uint16(51) << uint8(2) << uint16(510) << uint8(2) << uint16(511) << uint8(2); // if 50+baseclass quest ID is completed
	else if (Class == 2 || Class == 7 || Class == 8)
		result << uint16(4) << uint16(52) << uint8(2) << uint16(512) << uint8(2) << uint16(513) << uint8(2) << uint16(514) << uint8(2);
	else if (Class == 3 || Class == 9 || Class == 10)
		result << uint16(4) << uint16(53) << uint8(2) << uint16(515) << uint8(2) << uint16(516) << uint8(2) << uint16(517) << uint8(2);
	else if (Class == 4 || Class == 11 || Class == 12)
		result << uint16(7) << uint16(54) << uint8(2) << uint16(518) << uint8(2) << uint16(519) << uint8(2) << uint16(520) << uint8(2) << uint16(521) << uint8(2) << uint16(522) << uint8(2) << uint16(523) << uint8(2);
	Send(&result);

	result.Initialize(WIZ_MYINFO);

	// Load up our user rankings (for our NP symbols).
	g_pMain->GetUserRank(this);

	result.SByte(); // character name has a single byte length
	result	<< GetSocketID()
			<< m_pUserData.m_id
			<< GetSPosX() << GetSPosZ() << GetSPosY()
			<< GetNation() 
			<< m_pUserData.m_bRace << m_pUserData.m_sClass << m_pUserData.m_bFace
			<< m_pUserData.m_nHair
			<< m_pUserData.m_bRank << m_pUserData.m_bTitle
			<< GetLevel()
			<< m_pUserData.m_sPoints
			<< m_iMaxExp << m_pUserData.m_iExp
			<< m_pUserData.m_iLoyalty << m_pUserData.m_iLoyaltyMonthly
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
			<< m_iMaxHp << m_pUserData.m_sHp
			<< m_iMaxMp << m_pUserData.m_sMp
			<< m_sMaxWeight << m_sItemWeight
			<< getStat(STAT_STR) << uint8(getStatItemBonus(STAT_STR))
			<< getStat(STAT_STA) << uint8(getStatItemBonus(STAT_STA))
			<< getStat(STAT_DEX) << uint8(getStatItemBonus(STAT_DEX))
			<< getStat(STAT_INT) << uint8(getStatItemBonus(STAT_INT))
			<< getStat(STAT_CHA) << uint8(getStatItemBonus(STAT_CHA))
			<< m_sTotalHit << m_sTotalAc
			<< m_bFireR << m_bColdR << m_bLightningR << m_bMagicR << m_bDiseaseR << m_bPoisonR
			<< m_pUserData.m_iGold
			<< m_pUserData.m_bAuthority
			<< m_bPersonalRank << m_bKnightsRank; // national rank, leader rank

	result.append(m_pUserData.m_bstrSkill, 9);

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
	result	<< m_pUserData.m_bAccountStatus	// account status (0 = none, 1 = normal prem with expiry in hours, 2 = pc room)
			<< m_pUserData.m_bPremiumType		// premium type (7 = platinum premium)
			<< m_pUserData.m_sPremiumTime		// premium time
			<< m_bIsChicken						// chicken/beginner flag
			<< m_pUserData.m_iMannerPoint;

	Send(&result);

	g_pMain->AddCharacterName(this);

	SetZoneAbilityChange();
	Send2AI_UserUpdateInfo(true); 
}

void CUser::SetMaxHp(int iFlag)
{
	_CLASS_COEFFICIENT* p_TableCoefficient = NULL;
	p_TableCoefficient = g_pMain->m_CoefficientArray.GetData( m_pUserData.m_sClass );
	if( !p_TableCoefficient ) return;

	int temp_sta = getStatTotal(STAT_STA);
//	if( temp_sta > 255 ) temp_sta = 255;

	if( m_pUserData.m_bZone == ZONE_SNOW_BATTLE && iFlag == 0 )	{
		m_iMaxHp = 100;
	}
	else	{
		m_iMaxHp = (short)(((p_TableCoefficient->HP * GetLevel() * GetLevel() * temp_sta ) 
		      + 0.1 * (GetLevel() * temp_sta) + (temp_sta / 5)) + m_sMaxHPAmount + m_sItemMaxHp + 20);

		if( iFlag == 1 )	m_pUserData.m_sHp = m_iMaxHp;
		else if( iFlag == 2 )	m_iMaxHp = 100;
	}

	if(m_iMaxHp < m_pUserData.m_sHp) {
		m_pUserData.m_sHp = m_iMaxHp;
		HpChange( m_pUserData.m_sHp );
	}
}

void CUser::SetMaxMp()
{
	_CLASS_COEFFICIENT* p_TableCoefficient = NULL;
	p_TableCoefficient = g_pMain->m_CoefficientArray.GetData( m_pUserData.m_sClass );
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

	if(m_iMaxMp < m_pUserData.m_sMp) {
		m_pUserData.m_sMp = m_iMaxMp;
		MSpChange( m_pUserData.m_sMp );
	}
}

void CUser::SendTimeStatus()
{
	SendTime();
	SendWeather();
}

void CUser::SendTime()
{
	Packet result(WIZ_TIME);
	result	<< uint16(g_pMain->m_nYear) << uint16(g_pMain->m_nMonth) << uint16(g_pMain->m_nDate)
			<< uint16(g_pMain->m_nHour) << uint16(g_pMain->m_nMin);
	Send(&result);
}

void CUser::SendWeather()
{
	Packet result(WIZ_WEATHER);
	result << uint8(g_pMain->m_nWeather) << uint16(g_pMain->m_nAmount);
	Send(&result);
}

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
	else if (zone == 201)
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
		result	<< uint8(0) << uint8(1) << uint8(0)
				<< uint16(10); // orc-side tariff
	}
	else if (zone == 2 || zone == 12)
	{
		result	<< uint8(0) << uint8(1) << uint8(0)
				<< uint16(10); // human-side tariff
	}
	else 
		return;

	Send(&result);
}

void CUser::SendPremiumInfo()
{
	Packet result(WIZ_PREMIUM, m_pUserData.m_bAccountStatus);
	result << m_pUserData.m_bPremiumType << uint32(m_pUserData.m_sPremiumTime); 
	Send(&result);
}

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
	Send(&result); // NOTE: Compress
}

void CUser::RequestNpcIn(Packet & pkt)
{
	if (g_pMain->m_bPointCheckFlag == FALSE)
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
	Send(&result); // NOTE: Compress
}

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
		if(m_pUserData.m_sItemArray[i].nNum <= 0)
			continue;
		pTable = g_pMain->GetItemPtr( m_pUserData.m_sItemArray[i].nNum );
		if( !pTable )
			continue;
		if( m_pUserData.m_sItemArray[i].sDuration == 0 ) {
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
			if( ( m_pUserData.m_sClass == BERSERKER || m_pUserData.m_sClass == BLADE ) )
				m_sItemHit += (short)(item_hit * 0.5f);
		}

		m_sItemMaxHp += pTable->m_MaxHpB;
		m_sItemMaxMp += pTable->m_MaxMpB;
		m_sItemAc += item_ac;
		m_sStatItemBonuses[STAT_STR] += pTable->m_bStrB;
		m_sStatItemBonuses[STAT_STA] += pTable->m_bStaB;
		m_sStatItemBonuses[STAT_DEX] += pTable->m_bDexB;
		m_sStatItemBonuses[STAT_INT] += pTable->m_bIntelB;
		m_sStatItemBonuses[STAT_CHA] += pTable->m_bChaB;
		m_sItemHitrate += pTable->m_sHitrate;
		m_sItemEvasionrate += pTable->m_sEvarate;
//		m_sItemWeight += pTable->m_sWeight;

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

// Also add the weight of items in the inventory....
	for(int i=0 ; i < HAVE_MAX+SLOT_MAX ; i++)  {
		if(m_pUserData.m_sItemArray[i].nNum <= 0) continue;

		pTable = g_pMain->GetItemPtr( m_pUserData.m_sItemArray[i].nNum );
		if( !pTable ) continue;

		if (pTable->m_bCountable == 0) {	// Non-countable items.
			m_sItemWeight += pTable->m_sWeight;
		}
		else {	// Countable items.
			m_sItemWeight += pTable->m_sWeight * m_pUserData.m_sItemArray[i].sCount;
		}
	}
//	
	if( m_sItemHit < 3 )
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
	pRightHand = g_pMain->GetItemPtr(m_pUserData.m_sItemArray[RIGHTHAND].nNum);
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

void CUser::ExpChange(int64 iExp)
{	
	// Stop players level 5 or under from losing XP on death.
	if ((GetLevel() < 6 && iExp < 0)
		// Stop players in the war zone (TO-DO: Add other war zones) from losing XP on death.
		|| (m_pUserData.m_bZone == ZONE_BATTLE && iExp < 0))
		return;

	// Despite being signed, we don't want m_iExp ever going below 0.
	// If this happens, we need to investigate why -- not sweep it under the rug.
	ASSERT(m_pUserData.m_iExp >= 0);

	// Adjust the exp gained based on the percent set by the buff
	if (iExp > 0)
		iExp *= m_bExpGainAmount / 100;

	bool bLevel = true;
	if (iExp < 0 
		&& (m_pUserData.m_iExp + iExp) < 0)
		bLevel = false;
	else
		m_pUserData.m_iExp += iExp;

	// If we need to delevel...
	if (!bLevel)
	{
		// Drop us back a level.
		m_pUserData.m_bLevel--;

		// Get the excess XP (i.e. below 0), so that we can take it off the max XP of the previous level
		// Remember: we're deleveling, not necessarily starting from scratch at the previous level
		int64 diffXP = m_pUserData.m_iExp + iExp;

		// Now reset our XP to max for the former level.
		m_pUserData.m_iExp = g_pMain->GetExpByLevel(GetLevel());

		// Get new stats etc.
		LevelChange(GetLevel(), FALSE);

		// Take the remainder of the XP off (and delevel again if necessary).
		ExpChange(diffXP);
		return;
	}
	// If we've exceeded our XP requirement, we've leveled.
	else if (m_pUserData.m_iExp >= m_iMaxExp)
	{
		if (GetLevel() < MAX_LEVEL)
		{
			// Reset our XP to 0, level us up.
			m_pUserData.m_iExp = 0;
			LevelChange(++m_pUserData.m_bLevel);
			return;
		}

		// Hit the max level? Can't level any further. Cap the XP.
		m_pUserData.m_iExp = m_iMaxExp;
	}

	// Tell the client our new XP
	Packet result(WIZ_EXP_CHANGE);
	result << uint8(0) << m_pUserData.m_iExp; // NOTE: Use proper flag
	Send(&result);

	// If we've lost XP, save it for possible refund later.
	if (iExp < 0)
		m_iLostExp = -iExp;
}

/*
	This method name is something of a misnomer: 
	it's called after the level has changed (so that stats can be applied, etc), it does not change the level 
*/
void CUser::LevelChange(short level, BYTE type )
{
	if( level < 1 || level > MAX_LEVEL )
		return;

	if( type ) {
		if ((m_pUserData.m_sPoints + getStatTotal()) < uint32(300 + 3 * (level - 1)))
			m_pUserData.m_sPoints += 3;
		if( level > 9 && (m_pUserData.m_bstrSkill[0]+m_pUserData.m_bstrSkill[1]+m_pUserData.m_bstrSkill[2]+m_pUserData.m_bstrSkill[3]+m_pUserData.m_bstrSkill[4]
			+m_pUserData.m_bstrSkill[5]+m_pUserData.m_bstrSkill[6]+m_pUserData.m_bstrSkill[7]+m_pUserData.m_bstrSkill[8]) < (2*(level-9)) )
			m_pUserData.m_bstrSkill[0] += 2;	// Skill Points up
	}

	m_iMaxExp = g_pMain->GetExpByLevel(level);
	
	SetSlotItemValue();
	SetUserAbility();

	m_pUserData.m_sMp = m_iMaxMp;
	HpChange( m_iMaxHp );

	Send2AI_UserUpdateInfo();

	Packet result(WIZ_LEVEL_CHANGE);
	result	<< GetSocketID()
			<< GetLevel() << m_pUserData.m_sPoints << m_pUserData.m_bstrSkill[0]
			<< m_iMaxExp << m_pUserData.m_iExp
			<< m_iMaxHp << m_pUserData.m_sHp 
			<< m_iMaxMp << m_pUserData.m_sMp
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

void CUser::PointChange(Packet & pkt)
{
	uint8 type = pkt.read<uint8>();
	StatType statType = (StatType)(type - 1);

	if (statType < STAT_STR || statType >= STAT_COUNT 
		|| m_pUserData.m_sPoints < 1
		|| getStat(statType) == STAT_MAX) 
		return;

	Packet result(WIZ_POINT_CHANGE, type);

	m_pUserData.m_sPoints--; // remove a free point
	result << uint16(++m_pUserData.m_bStats[statType]); // assign the free point to a stat
	SetUserAbility();
	result << m_iMaxHp << m_iMaxMp << m_sTotalHit << m_sMaxWeight;
	Send(&result);
}

void CUser::HpChange(int amount, Unit *pAttacker /*= NULL*/, bool bSendToAI /*= true*/) 
{
	Packet result(WIZ_HP_CHANGE);

	if (amount < 0 && -amount > m_pUserData.m_sHp)
		m_pUserData.m_sHp = 0;
	else if (amount >= 0 && m_pUserData.m_sHp + amount > m_iMaxHp)
		m_pUserData.m_sHp = m_iMaxHp;
	else
		m_pUserData.m_sHp += amount;

	uint16 tid = (pAttacker != NULL ? pAttacker->GetID() : -1);
	result << m_iMaxHp << m_pUserData.m_sHp << tid;
	Send(&result);

	if (bSendToAI)
	{
		result.Initialize(AG_USER_SET_HP);
		result << GetSocketID() << uint32(m_pUserData.m_sHp);
		g_pMain->Send_AIServer(&result);
	}

	if (isInParty())
		SendPartyHPUpdate();

	if (m_pUserData.m_sHp == 0)
		OnDeath(pAttacker);
}

void CUser::MSpChange(int amount)
{
	Packet result(WIZ_MSP_CHANGE);

	// TO-DO: Make this behave unsigned.
	m_pUserData.m_sMp += amount;
	if (m_pUserData.m_sMp < 0)
		m_pUserData.m_sMp = 0;
	else if (m_pUserData.m_sMp > m_iMaxMp)
		m_pUserData.m_sMp = m_iMaxMp;

	result << m_iMaxMp << m_pUserData.m_sMp;
	Send(&result);

	if (isInParty())
		SendPartyHPUpdate(); // handles MP too
}

void CUser::SendPartyHPUpdate()
{
	Packet result(WIZ_PARTY);
	result	<< uint8(PARTY_HPCHANGE)
			<< GetSocketID()
			<< m_iMaxHp << m_pUserData.m_sHp
			<< m_iMaxMp << m_pUserData.m_sMp;
	g_pMain->Send_PartyMember(m_sPartyIndex, &result);
}

void CUser::Send2AI_UserUpdateInfo(bool initialInfo /*= false*/)
{
	Packet result(initialInfo ? AG_USER_INFO : AG_USER_UPDATE);

	result.SByte();
	result	<< GetSocketID()
			<< m_pUserData.m_id
			<< GetZoneID() << GetNation() << GetLevel()
			<< m_pUserData.m_sHp << m_pUserData.m_sMp
			<< uint16(m_sTotalHit * m_bAttackAmount / 100)
			<< uint16(m_sTotalAc + m_sACAmount)
			<< m_sTotalHitrate << m_sTotalEvasionrate
			<< m_sItemAc
			<< m_bMagicTypeLeftHand << m_bMagicTypeRightHand
			<< m_sMagicAmountLeftHand << m_sMagicAmountRightHand
			<< m_pUserData.m_bAuthority << m_bInvisibilityType;

	g_pMain->Send_AIServer(&result);
}

void CUser::SetUserAbility(bool bSendPacket /*= true*/)
{
	BOOL bHaveBow = FALSE;
	_CLASS_COEFFICIENT* p_TableCoefficient = g_pMain->m_CoefficientArray.GetData(m_pUserData.m_sClass);
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
			bHaveBow = TRUE;
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
			bHaveBow = TRUE;
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
		if (m_pUserData.m_sHp < 30 * m_iMaxHp / 100)
		{
			m_sTotalAc += 20 * m_sTotalAc / 100;
			// m_sTotalAcUnk += 20 * m_sTotalAcUnk / 100;
		}
	}
	else if (isRogue())
	{
		// Valor: [Passive]Increase your resistance by 50 when your HP is down to 30% or below.
		if (m_pUserData.m_sHp < 30 * m_iMaxHp / 100)
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

void CUser::SendTargetHP( BYTE echo, int tid, int damage )
{
	int hp = 0, maxhp = 0;

	if (tid >= NPC_BAND)
	{
		if (g_pMain->m_bPointCheckFlag == FALSE) return;
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

		hp = pUser->m_pUserData.m_sHp;	
		maxhp = pUser->m_iMaxHp;
	}

	Packet result(WIZ_TARGET_HP);
	result << uint16(tid) << echo << maxhp << hp << uint16(damage);
	Send(&result);
}

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

	_ZONE_ITEM *pItem = GetRegion()->m_RegionItemArray.GetData(bundle_index);
	if (pItem == NULL)
		return;

	for (int i = 0; i < 6; i++)
		result << pItem->itemid[i] << pItem->count[i];

	Send(&result);
}

BOOL CUser::IsValidName(const char *name)
{
	CString upperName = name;
	if (upperName.GetLength() == 0
		|| upperName.GetLength() > MAX_ID_SIZE)
		return FALSE;

	upperName.MakeUpper();
	foreach (itr, g_pMain->m_BlockNameArray)
		if (strstr(upperName, *itr))
			return FALSE;

	return TRUE;
}

void CUser::ItemGet(Packet & pkt)
{
	Packet result(WIZ_ITEM_GET);
	uint32 bundle_index = pkt.read<uint32>(), itemid = pkt.read<uint32>(), usercount = 0, money = 0, levelsum = 0, i = 0;
	BYTE pos;
	_ITEM_TABLE* pTable = NULL;
	_ZONE_ITEM* pItem = NULL;
	C3DMap *pMap = GetMap();
	CRegion* pRegion = GetRegion();
	CUser* pGetUser = NULL;

	ASSERT(pMap != NULL);
	ASSERT(pRegion != NULL);

	if (bundle_index < 1
		|| isTrading()
		|| pRegion == NULL
		|| isDead()) // yeah, we know people abuse this. We do not care!
		goto fail_return;

	pItem = (_ZONE_ITEM*)pRegion->m_RegionItemArray.GetData( bundle_index );
	if (!pItem) 
		goto fail_return;

	for (i = 0; i < 6; i++)
	{
		if (pItem->itemid[i] == itemid)
			break;
	}

	if (i == 6)
		goto fail_return;

	// Copy the item so we can still use it after it's freed
	// TO-DO: Clean this up (but it works for now)
	_ZONE_ITEM pItem2;
	memcpy(&pItem2, pItem, sizeof(pItem2)); 

	if (!pMap->RegionItemRemove(GetRegionX(), GetRegionZ(), bundle_index, pItem->itemid[i], pItem->count[i]))
		goto fail_return;

	// Save us from having to tweak the rest of the method (tacky, but again - works for now)
	pItem = &pItem2; 

	short count = pItem->count[i];

	pTable = g_pMain->GetItemPtr( itemid );
	if (pTable == NULL)
		goto fail_return;

	if( isInParty() && itemid != ITEM_GOLD ) 
		pGetUser = GetItemRoutingUser(itemid, count);
	else
		pGetUser = this;
		
	if (pGetUser == NULL) 
		goto fail_return;

	if (itemid == ITEM_GOLD)
	{
		if (count == 0 || count >= SHRT_MAX)
			return;

		if (!isInParty())
		{
			m_pUserData.m_iGold += count;
			result << uint8(1) << bundle_index << uint8(-1) << itemid << count << m_pUserData.m_iGold;
			Send(&result);
			return;
		}

		_PARTY_GROUP *pParty = g_pMain->m_PartyArray.GetData(m_sPartyIndex);
		if (!pParty)
			goto fail_return;

		for (i = 0; i < MAX_PARTY_USERS; i++)
		{
			CUser *pUser = g_pMain->GetUserPtr(pParty->uid[i]);
			if (pUser == NULL)
				continue;

			usercount++;
			levelsum += pUser->GetLevel();
		}
		if( usercount == 0 ) goto fail_return;

		for (i = 0; i < MAX_PARTY_USERS; i++)
		{
			CUser *pUser = g_pMain->GetUserPtr(pParty->uid[i]);
			if (pUser == NULL) 
				continue;

			money = (int)(count * (float)(pUser->GetLevel() / (float)levelsum));    
			pUser->m_pUserData.m_iGold += money;

			result.clear();
			result << uint8(2) << bundle_index << uint8(-1) << itemid << pUser->m_pUserData.m_iGold;
			pUser->Send(&result);
		}
		return;
	}

	pos = pGetUser->GetEmptySlot(itemid, pTable->m_bCountable);
	if (pos < 0) 
		goto fail_return;

	if (!pGetUser->CheckWeight(itemid, count))
	{
		result << uint8(6);
		pGetUser->Send(&result);
		return;
	}

	pGetUser->m_pUserData.m_sItemArray[SLOT_MAX+pos].nNum = itemid;	// Add item to inventory. 
	if (pTable->m_bCountable)
	{
		pGetUser->m_pUserData.m_sItemArray[SLOT_MAX+pos].sCount += count;
		if (pGetUser->m_pUserData.m_sItemArray[SLOT_MAX+pos].sCount > MAX_ITEM_COUNT)
			pGetUser->m_pUserData.m_sItemArray[SLOT_MAX+pos].sCount = MAX_ITEM_COUNT;
	}
	else
	{
		pGetUser->m_pUserData.m_sItemArray[SLOT_MAX+pos].sCount = 1;
		pGetUser->m_pUserData.m_sItemArray[SLOT_MAX+pos].nSerialNum = g_pMain->GenerateItemSerial();
	}

	pGetUser->SendItemWeight();
	pGetUser->m_pUserData.m_sItemArray[SLOT_MAX+pos].sDuration = pTable->m_sDuration;
	
	// 1 = self, 5 = party
	// Tell the user who got the item that they actually got it.
	result	<< uint8(pGetUser == this ? 1 : 5)
			<< bundle_index
			<< pos << itemid << pGetUser->m_pUserData.m_sItemArray[SLOT_MAX+pos].sCount
			<< pGetUser->m_pUserData.m_iGold;
	pGetUser->Send(&result);

	if (isInParty())
	{
		// Tell our party the item was looted
		result.clear();
		result << uint8(3) << bundle_index << itemid << pGetUser->m_pUserData.m_id;
		g_pMain->Send_PartyMember(m_sPartyIndex, &result);

		// Let us know the other user got the item
		if (pGetUser != this)
		{
			result.clear();
			result << uint8(4);
			Send(&result);
		}
	} 

	return;

fail_return:
	result << uint8(0);
	Send(&result);
}

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
				GetSocketID(), m_pUserData.m_id, bType, buff, nBuff);
			break;
		}
		break;

	case 5:
		if (!isGM())
			return;
		break;

	default:
		TRACE("[SID=%d] StateChange: %s tripped (bType=%d, buff=%d, nBuff=%d) somehow, HOW!?\n", 
			GetSocketID(), m_pUserData.m_id, bType, buff, nBuff);
		return;
	}

	StateChangeServerDirect(bType, nBuff);
}

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

void CUser::LoyaltyChange(short tid)
{
	short loyalty_source = 0, loyalty_target = 0;

	// TO-DO: Rewrite this out, it shouldn't handle all cases so generally like this
	if (m_pUserData.m_bZone == 48 || m_pUserData.m_bZone == 21) 
		return;

	CUser* pTUser = g_pMain->GetUserPtr(tid);  
	if (pTUser == NULL) 
		return;

	if (pTUser->GetNation() != GetNation()) 
	{
		if (pTUser->m_pUserData.m_iLoyalty <= 0) 
		{
			loyalty_source = 0;
			loyalty_target = 0;
		}
		// TO-DO: Rewrite this out, it'd be better to handle this in the database.
		// Colony Zone
		else if (pTUser->GetZoneID() == 71) 
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

	g_pMain->WriteLog("%s Speed Hack Used\r\n", m_pUserData.m_id);
	
	if( m_pUserData.m_bAuthority != 0 )
		m_pUserData.m_bAuthority = -1;

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
		|| m_pUserData.m_bstrSkill[0] < 1 
		// restrict skill points per category to your level
		|| m_pUserData.m_bstrSkill[type] + 1 > GetLevel()
		// we need our first job change to assign skill points
		|| (m_pUserData.m_sClass % 100) <= 4
		// to set points in the mastery category, we need to be mastered.
		|| (type == 8
			&& ((m_pUserData.m_sClass % 2) != 0 || (m_pUserData.m_sClass % 100) < 6))) 
	{
		result << m_pUserData.m_bstrSkill[type]; // only send the packet on failure
		Send(&result);
		return;
	}

	m_pUserData.m_bstrSkill[0] -= 1;
	m_pUserData.m_bstrSkill[type] += 1;
}

void CUser::UpdateGameWeather(Packet & pkt)
{
	if (!isGM())	// is this user a GM?
		return;

	if (pkt.GetOpcode() == WIZ_WEATHER)
	{
		pkt >> g_pMain->m_nWeather >> g_pMain->m_nAmount;
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
			<< m_pUserData.m_id << GetZoneID() << GetNation() << GetLevel()
			<< m_pUserData.m_sHp << m_pUserData.m_sMp 
			<< uint16(m_sTotalHit * m_bAttackAmount / 100)
			<< uint16(m_sTotalAc + m_sACAmount)
			<< m_sTotalHitrate << m_sTotalEvasionrate
			<< m_sPartyIndex << m_pUserData.m_bAuthority
			<< m_bInvisibilityType;
}

void CUser::CountConcurrentUser()
{
	if (!isGM())
		return;

	uint16 count = 0;
	SessionMap & sessMap = g_pMain->s_socketMgr.GetActiveSessionMap();
	foreach (itr, sessMap)
	{
		if (TO_USER(itr->second)->isInGame())
			count++;
	}
	g_pMain->s_socketMgr.ReleaseLock();

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
		if (m_pUserData.m_bZone == ZONE_BATTLE) {
			if (pTUser->m_pUserData.m_bNation == KARUS) {
				g_pMain->m_sKarusDead++;
				//TRACE("++ LoyaltyDivide - ka=%d, el=%d\n", g_pMain->m_sKarusDead, g_pMain->m_sElmoradDead);
			}
			else if (pTUser->m_pUserData.m_bNation == ELMORAD) {
				g_pMain->m_sElmoradDead++;
				//TRACE("++ LoyaltyDivide - ka=%d, el=%d\n", g_pMain->m_sKarusDead, g_pMain->m_sElmoradDead);
			}
		}
	}
		
	if (pTUser->m_pUserData.m_bNation != m_pUserData.m_bNation) {		// Different nations!!!
		level_difference = pTUser->GetLevel() - average_level;	// Calculate difference!

		if (pTUser->m_pUserData.m_iLoyalty <= 0) {	   // No cheats allowed...
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
	if (m_pUserData.m_bZone != m_pUserData.m_bNation && m_pUserData.m_bZone < 3) { 
		loyalty_source  = 2 * loyalty_source;
	}
//
	for (int j = 0; j < MAX_PARTY_USERS; j++) {		// Distribute loyalty amongst party members.
		CUser *pUser = g_pMain->GetUserPtr(pParty->uid[j]);
		if (pUser == NULL)
			continue;

		//TRACE("LoyaltyDivide 333 - user1=%s, %d\n", pUser->m_pUserData.m_id, pUser->m_pUserData.m_iLoyalty);
		individualvalue = pUser->GetLevel() * loyalty_source / levelsum ;
		pUser->SendLoyaltyChange(individualvalue);
	}

	pTUser->SendLoyaltyChange(loyalty_target);
}

void CUser::ItemWoreOut(int type, int damage)
{
	_ITEM_TABLE* pTable = NULL;
	int worerate = (int)sqrt(damage / 10.0f);
	if( worerate == 0 ) return;

	if( type == ATTACK ) {
		if( m_pUserData.m_sItemArray[RIGHTHAND].nNum != 0 ) {
			pTable = g_pMain->GetItemPtr( m_pUserData.m_sItemArray[RIGHTHAND].nNum );
			if( pTable ) {
				if( pTable->m_bSlot != 2 )	{// 2 == DEFENCE ITEM
					if( m_pUserData.m_sItemArray[RIGHTHAND].sDuration != 0 ) {
						m_pUserData.m_sItemArray[RIGHTHAND].sDuration -= worerate;
						ItemDurationChange( RIGHTHAND, pTable->m_sDuration, m_pUserData.m_sItemArray[RIGHTHAND].sDuration, worerate );
					}
				}
			}
		}
		if( m_pUserData.m_sItemArray[LEFTHAND].nNum != 0 ) {
			pTable = g_pMain->GetItemPtr( m_pUserData.m_sItemArray[LEFTHAND].nNum );
			if( pTable ) {
				if( pTable->m_bSlot != 2 ) {
					if( m_pUserData.m_sItemArray[LEFTHAND].sDuration != 0 ) {
						m_pUserData.m_sItemArray[LEFTHAND].sDuration -= worerate;
						ItemDurationChange( LEFTHAND, pTable->m_sDuration, m_pUserData.m_sItemArray[LEFTHAND].sDuration, worerate );
					}
				}
			}
		}
	}
	else if ( type == DEFENCE ) {
		if( m_pUserData.m_sItemArray[HEAD].nNum != 0 ) {
			pTable = g_pMain->GetItemPtr( m_pUserData.m_sItemArray[HEAD].nNum );
			if( pTable ) {
				if( m_pUserData.m_sItemArray[HEAD].sDuration != 0 ) {
					m_pUserData.m_sItemArray[HEAD].sDuration -= worerate;
					ItemDurationChange( HEAD, pTable->m_sDuration, m_pUserData.m_sItemArray[HEAD].sDuration, worerate );
				}
			}
		}
		if( m_pUserData.m_sItemArray[BREAST].nNum != 0 ) {
			pTable = g_pMain->GetItemPtr( m_pUserData.m_sItemArray[BREAST].nNum );
			if( pTable ) {
				if( m_pUserData.m_sItemArray[BREAST].sDuration != 0 ) {
					m_pUserData.m_sItemArray[BREAST].sDuration -= worerate;
					ItemDurationChange( BREAST, pTable->m_sDuration, m_pUserData.m_sItemArray[BREAST].sDuration, worerate );
				}
			}
		}
		if( m_pUserData.m_sItemArray[LEG].nNum != 0 ) {
			pTable = g_pMain->GetItemPtr( m_pUserData.m_sItemArray[LEG].nNum );
			if( pTable ) {
				if( m_pUserData.m_sItemArray[LEG].sDuration != 0 ) {
					m_pUserData.m_sItemArray[LEG].sDuration -= worerate;
					ItemDurationChange( LEG, pTable->m_sDuration, m_pUserData.m_sItemArray[LEG].sDuration, worerate );
				}
			}
		}
		if( m_pUserData.m_sItemArray[GLOVE].nNum != 0 ) {
			pTable = g_pMain->GetItemPtr( m_pUserData.m_sItemArray[GLOVE].nNum );
			if( pTable ) {
				if( m_pUserData.m_sItemArray[GLOVE].sDuration != 0 ) {
					m_pUserData.m_sItemArray[GLOVE].sDuration -= worerate;
					ItemDurationChange( GLOVE, pTable->m_sDuration, m_pUserData.m_sItemArray[GLOVE].sDuration, worerate );
				}
			}
		}
		if( m_pUserData.m_sItemArray[FOOT].nNum != 0 ) {
			pTable = g_pMain->GetItemPtr( m_pUserData.m_sItemArray[FOOT].nNum );
			if( pTable ) {
				if( m_pUserData.m_sItemArray[FOOT].sDuration != 0 ) {
					m_pUserData.m_sItemArray[FOOT].sDuration -= worerate;
					ItemDurationChange( FOOT, pTable->m_sDuration, m_pUserData.m_sItemArray[FOOT].sDuration, worerate );
				}
			}
		}
		if( m_pUserData.m_sItemArray[RIGHTHAND].nNum != 0 ) {
			pTable = g_pMain->GetItemPtr( m_pUserData.m_sItemArray[RIGHTHAND].nNum );
			if( pTable ) {
				if( pTable->m_bSlot == 2 ) {	// ?????
					if( m_pUserData.m_sItemArray[RIGHTHAND].sDuration != 0 ) {
						m_pUserData.m_sItemArray[RIGHTHAND].sDuration -= worerate;
						ItemDurationChange( RIGHTHAND, pTable->m_sDuration, m_pUserData.m_sItemArray[RIGHTHAND].sDuration, worerate );
					}
				}
			}
		}
		if( m_pUserData.m_sItemArray[LEFTHAND].nNum != 0 ) {
			pTable = g_pMain->GetItemPtr( m_pUserData.m_sItemArray[LEFTHAND].nNum );
			if( pTable ) {
				if( pTable->m_bSlot == 2 ) {	// ?????
					if( m_pUserData.m_sItemArray[LEFTHAND].sDuration != 0 ) {
						m_pUserData.m_sItemArray[LEFTHAND].sDuration -= worerate;
						ItemDurationChange( LEFTHAND, pTable->m_sDuration, m_pUserData.m_sItemArray[LEFTHAND].sDuration, worerate );
					}
				}
			}
		}
	}
}

void CUser::ItemDurationChange(uint8 slot, uint16 maxValue, int16 curValue, uint16 amount)
{
	if (slot >= SLOT_MAX)
		return;

	int curpercent = 0, beforepercent = 0, curbasis = 0, beforebasis = 0;

	// If the durability's now less than 0, reset it to 0.
	if (m_pUserData.m_sItemArray[slot].sDuration <= 0)
	{
		m_pUserData.m_sItemArray[slot].sDuration = 0;
		SendDurability(slot, 0);
		
		SetSlotItemValue();
		SetUserAbility(false);
		SendItemMove(1);
		return;
	}

	curpercent = (int)((curValue / (double)maxValue) * 100);
	beforepercent = (int)(((curValue + amount) / (double)maxValue ) * 100);
	
	curbasis = curpercent / 5;
	beforebasis = beforepercent / 5;

	if (curbasis != beforebasis) 
	{
		SendDurability(slot, curValue);

		if (curpercent >= 65 && curpercent < 70
			|| curpercent >= 25 && curpercent < 30)
			UserLookChange( slot, m_pUserData.m_sItemArray[slot].nNum, curValue);
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

void CUser::HPTimeChange(float currenttime)
{
	BOOL bFlag = FALSE;

	m_fHPLastTimeNormal = currenttime;

	if( m_bResHpType == USER_DEAD ) return;

	if( m_pUserData.m_bZone == ZONE_SNOW_BATTLE && g_pMain->m_byBattleOpen == SNOW_BATTLE )	{
		if( m_pUserData.m_sHp < 1 ) return;
		HpChange( 5 );
		return;
	}

	if( m_bResHpType == USER_STANDING ) {
		if( m_pUserData.m_sHp < 1 ) return;
		if( m_iMaxHp != m_pUserData.m_sHp )
			HpChange( (int)((GetLevel()*(1+GetLevel()/60.0) + 1)*0.2)+3 );

		if( m_iMaxMp != m_pUserData.m_sMp )
			MSpChange( (int)((GetLevel()*(1+GetLevel()/60.0) + 1)*0.2)+3 );
	}
	else if ( m_bResHpType == USER_SITDOWN ) {
		if( m_pUserData.m_sHp < 1 ) return;
		if( m_iMaxHp != m_pUserData.m_sHp ) {
			HpChange( (int)(GetLevel()*(1+GetLevel()/30.0) ) + 3 );
		}
		if( m_iMaxMp != m_pUserData.m_sMp ) {
			MSpChange((int)((m_iMaxMp * 5) / ((GetLevel() - 1) + 30 )) + 3 ) ;
		}
	}
}

void CUser::HPTimeChangeType3(float currenttime)
{
	for (int g = 0 ; g < MAX_TYPE3_REPEAT ; g++) {	// Get the current time for all the last times...
		m_fHPLastTime[g] = currenttime;
	}

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
		if (m_pUserData.m_sHp == 0)
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
			if (isDead() || ((currenttime - m_fHPStartTime[i]) >= m_bHPDuration[i]) || isDead())
			{
				Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE3_END));

				if (m_bHPAmount[i] > 0)
					result << uint8(100);
				else
					result << uint8(200);

				Send(&result);

				m_fHPStartTime[i] = 0.0f;
				m_fHPLastTime[i] = 0.0f;
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
		m_bType3Flag = FALSE;

	if (isInParty() && bType3Test)
		SendPartyStatusUpdate(1, 0);
}

void CUser::Type4Duration(float currenttime)
{
	BYTE buff_type = 0;					

	for (int i = 0; i < MAX_TYPE4_BUFF; i++)
	{
		if (m_sDuration[i] == 0
			|| currenttime <= (m_fStartTime[i] + m_sDuration[i]))
			continue;

		m_sDuration[i] = 0;
		m_fStartTime[i] = 0;

		buff_type = i + 1;

		switch (buff_type)
		{
		case 1: 
			m_sMaxHPAmount = 0;
			break;

		case 2:
			m_sACAmount = 0;
			break;

		case 3:
			StateChangeServerDirect(3, ABNORMAL_NORMAL);
			break;

		case 4:
			m_bAttackAmount = 100;
			break;

		case 5:
			m_bAttackSpeedAmount = 100;
			break;

		case 6:
			m_bSpeedAmount = 100;
			break;

		case 7:
			memset(m_sStatItemBonuses, 0, sizeof(uint16) * STAT_COUNT);
			break;

		case 8:
			m_bFireRAmount = m_bColdRAmount = m_bLightningRAmount = 0;
			m_bMagicRAmount = m_bDiseaseRAmount = m_bPoisonRAmount = 0;
			break;

		case 9:
			m_bHitRateAmount = 100;
			m_sAvoidRateAmount = 100;
			break;
		}

		break; // only ever handle one at a time with the current logic
	}

	if (buff_type) {
		m_bType4Buff[buff_type - 1] = 0;

		SetSlotItemValue();
		SetUserAbility();
		Send2AI_UserUpdateInfo();

		Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_END));
		result << buff_type;
		Send(&result);
	}

	int buff_test = 0;
	for (int i = 0 ; i < MAX_TYPE4_BUFF ; i++) {
		buff_test += m_bType4Buff[i];
	}
	if (buff_test == 0) m_bType4Flag = FALSE;

	BOOL bType4Test = TRUE ;
	for (int j = 0 ; j < MAX_TYPE4_BUFF ; j++) {
		if (m_bType4Buff[j] == 1) {
			bType4Test = FALSE;
			break;
		}
	}

	if (isInParty() && bType4Test)
		SendPartyStatusUpdate(2);
}

void CUser::SendAllKnightsID()
{
	Packet result(WIZ_KNIGHTS_LIST, uint8(1));
	uint16 count = 0;
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

	CUser *pUser = g_pMain->GetUserPtr(strUserID.c_str(), TYPE_CHARACTER);
	if (pUser == NULL)
		return;

	switch (opcode)
	{
	case OPERATOR_ARREST:
		ZoneChange(pUser->GetZoneID(), pUser->m_pUserData.m_curx, pUser->m_pUserData.m_curz);
		break;
	case OPERATOR_SUMMON:
		pUser->ZoneChange(GetZoneID(), m_pUserData.m_curx, m_pUserData.m_curz);
		break;
	case OPERATOR_CUTOFF:
		pUser->Disconnect();
		break;
	case OPERATOR_BAN:
	case OPERATOR_BAN_ACCOUNT: // ban account is meant to call a proc to do so
		pUser->m_pUserData.m_bAuthority = AUTHORITY_BANNED;
		pUser->Disconnect();
		break;
	case OPERATOR_MUTE:
		pUser->m_pUserData.m_bAuthority = AUTHORITY_MUTED;
		break;
	case OPERATOR_DISABLE_ATTACK:
		pUser->m_pUserData.m_bAuthority = AUTHORITY_ATTACK_DISABLED;
		break;
	case OPERATOR_ENABLE_ATTACK:
	case OPERATOR_UNMUTE:
		pUser->m_pUserData.m_bAuthority = AUTHORITY_PLAYER;
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
			TRACE("%s SpeedHack User Checked By Server Time\n", m_pUserData.m_id);
			Close();
		}
		else if( client_gap - server_gap < 0.0f ) {
			m_fSpeedHackClientTime = clienttime;
			m_fSpeedHackServerTime = TimeGet();
		}
	}
#endif
}

void CUser::Type3AreaDuration(float currenttime)
{
	Packet result(WIZ_MAGIC_PROCESS);

	_MAGIC_TYPE3* pType = g_pMain->m_Magictype3Array.GetData(m_iAreaMagicID);
	if (pType == NULL)
		return;

	if (m_fAreaLastTime != 0.0f && (currenttime - m_fAreaLastTime) > m_bAreaInterval)
	{
		m_fAreaLastTime = currenttime;
		if (isDead())
			return;
		
		// TO-DO: Make this not suck (needs to be localised)
		SessionMap & sessMap = g_pMain->s_socketMgr.GetActiveSessionMap();
		set<uint16> sessionIDs;
		foreach (itr, sessMap)
		{
			if (m_MagicProcess.UserRegionCheck(GetSocketID(), itr->first, m_iAreaMagicID, pType->bRadius))
				sessionIDs.insert(itr->first);
		}
		g_pMain->s_socketMgr.ReleaseLock();

		foreach (itr, sessionIDs)
		{
			result.clear();
			result	<< uint8(MAGIC_EFFECTING) << m_iAreaMagicID
					<< GetSocketID() << (*itr)
					<< uint16(0) << uint16(0) << uint16(0) << uint16(0) << uint16(0);
			SendToRegion(&result);
		}


		if ( (( currenttime - m_fAreaStartTime) >= pType->bDuration))
		{ // Did area duration end? 			
			m_bAreaInterval = 5;
			m_fAreaStartTime = 0.0f;
			m_fAreaLastTime = 0.0f;
			m_iAreaMagicID = 0;
		}
	}	


	result.clear();
	result	<< uint8(MAGIC_EFFECTING) << m_iAreaMagicID
			<< GetSocketID() << GetSocketID()
			<< uint16(0) << uint16(0) << uint16(0) << uint16(0) << uint16(0);
	SendToRegion(&result);
}

int CUser::GetEmptySlot(int itemid, int bCountable)
{
	for (int i = 0; i < HAVE_MAX; i++)
	{
		if (m_pUserData.m_sItemArray[SLOT_MAX+i].nNum == 0)
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
		KickOutZoneUser(TRUE);
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

CUser* CUser::GetItemRoutingUser(int itemid, short itemcount)
{
	if( !isInParty() ) return NULL;

	CUser* pUser = NULL;
	_PARTY_GROUP* pParty = NULL;
	int select_user = -1, count = 0;
 
	pParty = g_pMain->m_PartyArray.GetData( m_sPartyIndex );
	if( !pParty ) return NULL;
	if(	pParty->bItemRouting > 7 ) return NULL;
//
	_ITEM_TABLE* pTable = NULL;
	pTable = g_pMain->GetItemPtr( itemid );
	if( !pTable ) return NULL;
//
	while(count<8) {
		pUser = g_pMain->GetUserPtr(pParty->uid[pParty->bItemRouting]);
		if( pUser ) {
			if (pTable->m_bCountable) {	// Check weight of countable item.
				if ((pTable->m_sWeight * count + pUser->m_sItemWeight) <= pUser->m_sMaxWeight) {			
					pParty->bItemRouting++;
					if( pParty->bItemRouting > 6 )
						pParty->bItemRouting = 0;
					return pUser;
				}
			}
			else {	// Check weight of non-countable item.
				if ((pTable->m_sWeight + pUser->m_sItemWeight) <= pUser->m_sMaxWeight) {
					pParty->bItemRouting++;
					if( pParty->bItemRouting > 6 )
						pParty->bItemRouting = 0;
					return pUser;
				}
			}
		}
		if( pParty->bItemRouting > 6 )
			pParty->bItemRouting = 0;
		else
			pParty->bItemRouting++;
		count++;
	}

	return NULL;
}

void CUser::ClassChangeReq()
{
	Packet result(WIZ_CLASS_CHANGE, uint8(CLASS_CHANGE_RESULT));
	if (GetLevel() < 10) // if we haven't got our first job change
		result << uint8(2);
	else if ((m_pUserData.m_sClass % 100) > 4) // if we've already got our job change
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
		|| (g_pMain->m_sDiscount == 1 && g_pMain->m_byOldVictory == m_pUserData.m_bNation))
		temp_value /= 2;

	money = m_pUserData.m_iGold - temp_value;

	// Not enough money, or level too low.
	if (money < 0
		|| GetLevel() < 10)
		goto fail_return;

	// Get total skill points
	for (int i = 1; i < 9; i++)
		skill_point += m_pUserData.m_bstrSkill[i];

	// If we don't have any skill points, there's no point resetting now is there.
	if (skill_point <= 0)
	{
		type = 2;
		goto fail_return;
	}

	// Reset skill points.
	m_pUserData.m_bstrSkill[0] = (GetLevel() - 9) * 2;
	for (int i = 1; i < 9; i++)	
		m_pUserData.m_bstrSkill[i] = 0;

	// Take coins.
	m_pUserData.m_iGold = money;

	result << uint8(1) << m_pUserData.m_iGold << m_pUserData.m_bstrSkill[0];
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
	
	money = m_pUserData.m_iGold - temp_money;
	if(money < 0)	goto fail_return;

	for (int i = 0; i < SLOT_MAX; i++)
	{
		if (m_pUserData.m_sItemArray[i].nNum) {
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
	switch (m_pUserData.m_bRace)
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

	m_pUserData.m_sPoints = (GetLevel() - 1) * 3 + 10;
	ASSERT(getStatTotal() == 290);

	m_pUserData.m_iGold = money;

	SetUserAbility();
	Send2AI_UserUpdateInfo();

	result << uint8(1) // result (success)
		<< m_pUserData.m_iGold
		<< getStat(STAT_STR) << getStat(STAT_STA) << getStat(STAT_DEX) << getStat(STAT_INT) << getStat(STAT_CHA)
		<< m_iMaxHp << m_iMaxMp << m_sTotalHit << m_sMaxWeight << m_pUserData.m_sPoints;
	Send(&result);

fail_return:
	result << bResult << temp_money;
	Send(&result);
}

void CUser::GoldChange(short tid, int gold)
{
	if (m_pUserData.m_bZone < 3) return;	// Money only changes in Frontier zone and Battle zone!!!
	if (m_pUserData.m_bZone == ZONE_SNOW_BATTLE) return;

	CUser* pTUser = g_pMain->GetUserPtr(tid);
	if (pTUser == NULL || pTUser->m_pUserData.m_iGold <= 0)
		return;

	// Reward money in war zone
	if (gold == 0)
	{
		// If we're not in a party, we can distribute cleanly.
		if (!isInParty())
		{
			GoldGain((pTUser->m_pUserData.m_iGold * 4) / 10);
			pTUser->GoldLose(pTUser->m_pUserData.m_iGold / 2);
			return;
		}

		// Otherwise, if we're in a party, we need to divide it up.
		_PARTY_GROUP* pParty = g_pMain->m_PartyArray.GetData(m_sPartyIndex);
		if (pParty == NULL)
			return;			

		int userCount = 0, levelSum = 0, temp_gold = (pTUser->m_pUserData.m_iGold * 4) / 10;	
		pTUser->GoldLose(pTUser->m_pUserData.m_iGold / 2);		

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

	if (m_pUserData.m_bZone == pWarp->sZone) 
	{
		m_bZoneChangeSameZone = TRUE;

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

BOOL CUser::GetWarpList(int warp_group)
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
	return TRUE;
}

BOOL CUser::BindObjectEvent(_OBJECT_EVENT *pEvent)
{
	if (pEvent->sBelong != 0 && pEvent->sBelong != GetNation())
		return FALSE;

	Packet result(WIZ_OBJECT_EVENT, uint8(pEvent->sType));

	m_pUserData.m_sBind = pEvent->sIndex;

	result << uint8(1);
	Send(&result);
	return TRUE;
}

BOOL CUser::GateLeverObjectEvent(_OBJECT_EVENT *pEvent, int nid)
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
		return FALSE;

	// Move the lever (up/down).
	pNpc->SendGateFlag(!pNpc->m_byGateOpen);

	// Open/close the gate.
	pGateNpc->SendGateFlag(!pGateNpc->m_byGateOpen);
	return TRUE;
}

/***
 * Not sure what this is used for, so keeping logic the same just in case.
 ***/
BOOL CUser::FlagObjectEvent(_OBJECT_EVENT *pEvent, int nid)
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
		return FALSE;

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
	return TRUE;
}

BOOL CUser::WarpListObjectEvent(_OBJECT_EVENT *pEvent)
{
	// If the warp gate belongs to a nation, which isn't us...
	if (pEvent->sBelong != 0 && pEvent->sBelong != GetNation()
		// or we're in the opposing nation's zone...
		|| (GetZoneID() != GetNation() && GetZoneID() <= ELMORAD)
		// or we're unable to retrieve the warp list...
		|| !GetWarpList(pEvent->sControlNpcID)) 
		return FALSE;

	return TRUE;
}

void CUser::ObjectEvent(Packet & pkt)
{
	if (g_pMain->m_bPointCheckFlag == FALSE
		|| isDead())
		return;

	BOOL bSuccess = FALSE;
	uint16 objectindex, nid;
	pkt >> objectindex >> nid;

	_OBJECT_EVENT * pEvent = GetMap()->GetObjectEvent(objectindex);
	if (pEvent != NULL)
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

void CUser::SendAnvilRequest(int nid)
{
	Packet result(WIZ_ITEM_UPGRADE, uint8(1));
	result << nid;
	Send(&result);
}

void CUser::UpdateVisibility(InvisibilityType bNewType)
{
	Packet result(AG_USER_VISIBILITY);
	m_bInvisibilityType = (uint8)(bNewType);
	result << GetID() << m_bInvisibilityType;
	g_pMain->Send_AIServer(&result);
}

void CUser::BlinkStart()
{
	// Don't blink in these zones
	if (m_pUserData.m_bZone == 201 // colony zone
		|| (m_pUserData.m_bZone / 100) == 1) // war zone
		return;

	m_bAbnormalType = ABNORMAL_BLINKING;
	m_fBlinkStartTime = TimeGet();
	m_bRegeneType = REGENE_ZONECHANGE;
	
	UpdateVisibility(INVIS_NORMAL); // AI shouldn't see us
	m_bInvisibilityType = INVIS_NONE; // but players should. 

	StateChangeServerDirect(3, ABNORMAL_BLINKING);
}

void CUser::BlinkTimeCheck(float currenttime)
{
	if ((currenttime - m_fBlinkStartTime) < BLINK_TIME)
		return;

	m_fBlinkStartTime = 0.0f;
	m_bRegeneType = REGENE_NORMAL;

	StateChangeServerDirect(3, ABNORMAL_NORMAL);

	Packet result(AG_USER_REGENE);
	result	<< GetSocketID() << m_pUserData.m_sHp;
	g_pMain->Send_AIServer(&result);

	result.Initialize(AG_USER_INOUT);
	result.SByte(); // TO-DO: Remove this redundant uselessness that is mgame
	result	<< uint8(INOUT_RESPAWN) << GetSocketID()
			<< m_pUserData.m_id
			<< m_pUserData.m_curx << m_pUserData.m_curz;
	g_pMain->Send_AIServer(&result);

	UpdateVisibility(INVIS_NONE);
}

void CUser::GoldGain(int gold)	// 1 -> Get gold    2 -> Lose gold
{
	Packet result(WIZ_GOLD_CHANGE);
	
	m_pUserData.m_iGold += gold;

	result << uint8(1) << gold << m_pUserData.m_iGold;
	Send(&result);	
}

BOOL CUser::GoldLose(unsigned int gold)
{
	if (m_pUserData.m_iGold < gold) 
		return FALSE;
	
	Packet result(WIZ_GOLD_CHANGE);
	m_pUserData.m_iGold -= gold;
	result << uint8(2) << gold << m_pUserData.m_iGold;
	Send(&result);	
	return TRUE;
}

BOOL CUser::CheckSkillPoint(BYTE skillnum, BYTE min, BYTE max)
{
	if (skillnum < 5 || skillnum > 8) 
		return FALSE;

	return (m_pUserData.m_bstrSkill[skillnum] >= min && m_pUserData.m_bstrSkill[skillnum] <= max);
}

BOOL CUser::CheckClass(short class1, short class2, short class3, short class4, short class5, short class6)
{
	return (JobGroupCheck(class1) || JobGroupCheck(class2) || JobGroupCheck(class3) || JobGroupCheck(class4) || JobGroupCheck(class5) || JobGroupCheck(class6));
}

BOOL CUser::JobGroupCheck(short jobgroupid)
{
	if (jobgroupid > 100) 
		return m_pUserData.m_sClass == jobgroupid;

	int subClass = m_pUserData.m_sClass % 100;

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
	float currenttime = 0.0f;
	currenttime = TimeGet();

	if (ZONE_TRAP_INTERVAL < (currenttime - m_fLastTrapAreaTime)) {	// Time interval has passed :)
		HpChange( -ZONE_TRAP_DAMAGE );     // Reduce target health point.

		if (m_pUserData.m_sHp == 0)
			OnDeath(this);
	} 

	m_fLastTrapAreaTime = currenttime;		// Update Last Trap Area time :)
}

// TO-DO: This needs updating.
void CUser::KickOutZoneUser(BOOL home, int nZoneID /*= 21 */)
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
		if (m_pUserData.m_bNation == KARUS) {
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
	pHomeInfo = g_pMain->m_HomeArray.GetData(m_pUserData.m_bNation);
	if (!pHomeInfo) return;

	m_pUserData.m_bZone = m_pUserData.m_bNation;

	if (m_pUserData.m_bNation == KARUS) {
		m_pUserData.m_curx = (float)(pHomeInfo->KarusZoneX + myrand(0, pHomeInfo->KarusZoneLX));
		m_pUserData.m_curz = (float)(pHomeInfo->KarusZoneZ + myrand(0, pHomeInfo->KarusZoneLZ)); 
	}
	else {
		m_pUserData.m_curx = (float)(pHomeInfo->ElmoZoneX + myrand(0, pHomeInfo->ElmoZoneLX));
		m_pUserData.m_curz = (float)(pHomeInfo->ElmoZoneZ + myrand(0, pHomeInfo->ElmoZoneLZ)); 
	}
}

BOOL CUser::CheckRandom(short percent)
{
	if (percent < 0 || percent > 1000) 
		return FALSE;

	return (percent > myrand(0, 1000));
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

void CUser::HandleCapeChange(Packet & pkt)
{
	Packet result(WIZ_CAPE);
	CKnights *pKnights = NULL;
	_KNIGHTS_CAPE *pCape = NULL;
	int16 sErrorCode = 0, sCapeID;
	uint8 r, g, b;

	pkt >> sCapeID >> r >> g >> b;

	// If we're not a clan leader, what are we doing changing the cape?
	if (!isClanLeader()
		|| isDead())
	{
		sErrorCode = -1;
		goto fail_return;
	}

	// Does the clan exist?
	if ((pKnights = g_pMain->GetClanPtr(GetClanID())) == NULL)
	{
		sErrorCode = -2;
		goto fail_return;
	}

	// Make sure we're promoted
	if (pKnights->m_byFlag < KNIGHTS_TYPE
		// and that if we're in an alliance, we're the main alliance.
		|| (pKnights->m_sAlliance != 0 && pKnights->m_sAlliance != pKnights->m_sIndex))
	{
		sErrorCode = -1;
		goto fail_return;
	}

	long nReqClanPoints = 0, nReqCoins = 0;
	if (sCapeID >= 0)
	{
		// Does this cape type exist?
		if ((pCape = g_pMain->m_KnightsCapeArray.GetData(sCapeID)) == NULL)
		{
			sErrorCode = -5;
			goto fail_return;
		}

		// Is our clan allowed to use this cape?
		if ((pCape->byGrade && pKnights->m_byGrade > pCape->byGrade)
			// not sure if this should use another error, need to confirm
			|| pKnights->m_byFlag < pCape->byRanking)
		{
			sErrorCode = -6;
			goto fail_return;
		}

		// NOTE: Error code -8 is for nDuration
		// It applies if we do not have the required item ('nDuration', awful name).
		// Since no capes seem to use it, we'll ignore it...

		// Can we even afford this cape?
		if (m_pUserData.m_iGold < (uint32)pCape->nReqCoins)
		{
			sErrorCode = -7;
			goto fail_return;
		}

		nReqCoins = pCape->nReqCoins;
		nReqClanPoints = pCape->nReqClanPoints;
	}

	bool bApplyingPaint = false;

	// These are 0 when not used
	if (r != 0 || g != 0 || b != 0)
	{
		// To use paint, the clan needs to be accredited
		if (pKnights->m_byFlag <= KNIGHTS_TYPE)
		{
			sErrorCode = -1; // need to find the error code for this
			goto fail_return;
		}

		bApplyingPaint = true;
		nReqClanPoints += 1000; // does this need tweaking per clan rank?
	}

	// If this requires clan points, does our clan have enough?
	if (pKnights->m_nPoints < nReqClanPoints)
	{
		// this error may not be correct
		sErrorCode = -7;
		goto fail_return;
	}

	if (nReqCoins > 0)
		GoldLose(nReqCoins);

	if (nReqClanPoints)
		pKnights->m_nPoints -= nReqClanPoints;

	// Are we changing the cape?
	if (sCapeID >= 0)
		pKnights->m_sCape = sCapeID;

	// Are we applying paint?
	if (bApplyingPaint)
	{
		pKnights->m_bCapeR = r;
		pKnights->m_bCapeG = g;
		pKnights->m_bCapeB = b;
	}

	result	<< uint16(1) // success
			<< uint16(pKnights->m_sAlliance)
			<< uint16(pKnights->m_sIndex)
			<< uint16(pKnights->m_sCape) 
			<< pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB
			<< uint8(0);

	Send(&result);

	result.Initialize(WIZ_KNIGHTS_PROCESS);
	result	<< uint8(KNIGHTS_CAPE_UPDATE)
			<< GetClanID()
			<< pKnights->m_byFlag
			<< uint16(pKnights->m_sCape)
			<< pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB
			<< uint8(0);

	// TO-DO:
	// When we implement alliances, this should send to the alliance
	// if the clan is part of one. Also, their capes should be updated.
	pKnights->Send(&result);

	// TO-DO: Send to other servers via UDP.

	// Now tell Aujard to save (we don't particularly care whether it was able to do so or not).
	result.clear();
	result	<< uint16(pKnights->m_sIndex) << uint16(pKnights->m_sCape) 
			<< pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB;
	g_pMain->AddDatabaseRequest(result, this);
	return;

fail_return:
	result << sErrorCode;
	Send(&result);
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

bool CUser::CanUseItem(long itemid, uint16 count)
{
	_ITEM_TABLE* pItem = pItem = g_pMain->GetItemPtr(itemid);
	if(!pItem)
		return false;

	if(pItem->m_bClass != 0 && pItem->m_bClass != m_pUserData.m_sClass) //Class related item check
		return false;

	if(pItem->m_bReqLevel > m_pUserData.m_bLevel) //Level related item check
		return false;

	if(!CheckExistItem(itemid, count))
		return false;

	return true;
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

_ITEM_TABLE* CUser::GetItemPrototype(uint8 pos)
{
	if (pos >= INVENTORY_TOTAL)
		return NULL;

	_ITEM_DATA *pItem = GetItem(pos);
	return pItem->nNum == 0 ? NULL : g_pMain->GetItemPtr(pItem->nNum);
}

/* TO-DO: Move all these to their own handler file */
enum
{
	ITEM_UPGRADE_REQ		= 1,
	ITEM_UPGRADE			= 2,
	ITEM_ACCESSORIES		= 3,
	ITEM_BIFROST_EXCHANGE	= 5,
	ITEM_UPGRADE_REBIRTH	= 7,
	ITEM_SEAL				= 8,
	ITEM_CHARACTER_SEAL		= 9
};

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

void CUser::ItemUpgrade(Packet & pkt)
{
}

void CUser::ItemUpgradeAccessories(Packet & pkt)
{
}

void CUser::BifrostPieceProcess(Packet & pkt)
{
}

void CUser::ItemUpgradeRebirth(Packet & pkt)
{
}

void CUser::ItemSealProcess(Packet & pkt)
{
	#define ITEM_SEAL_PRICE 1000000
	enum
	{
		SEAL_TYPE_SEAL		= 1,
		SEAL_TYPE_UNSEAL	= 2,
		SEAL_TYPE_KROWAZ	= 3
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
			uint8 bSrcPos;
			pkt >> unk0 >> nItemID >> bSrcPos >> strPasswd;

			/* 
				Most of these checks are handled client-side, so we shouldn't need to provide error messages.
				Also, item sealing requires certain premium types (gold, platinum, etc) - need to double-check 
				these before implementing this check.
			*/

			// do we have enough coins?
			if (m_pUserData.m_iGold < ITEM_SEAL_PRICE 
				// is this a valid position? (need to check if it can be taken from new slots)
				|| bSrcPos >= HAVE_MAX 
				// is the password valid by client limits?
				|| strPasswd.empty() || strPasswd.length() > 8
				// does the item exist where the client says it does?
				|| GetItem(SLOT_MAX + bSrcPos)->nNum != nItemID) 
				return;

			// NOTE: Possible error codes are:
			// 2/6 - "This item cannot be sealed." 
			// 4   - "Invalid Citizen Registry Number" (i.e. password's wrong)
			// 5   - "Insufficient items to perform the seal."
			// 8   - "Please try again. You may not repeat this function instantly."
			// all else (we'll go with 3, but it's the default case): "Sealing the item failed."

			// TO-DO: Implement Aujard packet -> stored procedure for verification + addition to the table
		} break;

		// Used when unsealing an item.
		case SEAL_TYPE_UNSEAL:
		{
		} break;

		// Used when binding a Krowaz item (presumably to take it from bound -> sealed)
		case SEAL_TYPE_KROWAZ:
		{
		} break;
	}
}

void CUser::CharacterSealProcess(Packet & pkt)
{
}

void CUser::QuestDataRequest(Packet & pkt)
{
	uint8 opcode = pkt.read<uint8>();

	// NOTE: In 1.298 the client requested opcode 1 on login. Now it requests 3, so not sure why that was changed.
	// Other opcodes are unknown
	if (opcode == 3)
	{
		// Sending this now is probably wrong, but it's cleaner than it was before.
		Packet result(WIZ_QUEST, uint8(1));
		result << uint16(m_questMap.size());
		foreach (itr, m_questMap)
			result	<< itr->first << itr->second;
		Send(&result);
	}

	// NOTE: To activate Seed/Max (the kid that gets eaten by a worm), we need to send:
	// sub opcode  = 2
	// quest ID    = 500 (says something about beginner weapons), 
	// quest state = 1 (pending completion)
	// and additionally, a final byte to specify whether he's shown or not (1 - no, 2 - yes)
	// Before this is implemented I think it best to research this updated system further.

	/*	
	// Unlock skill data (level 70 skill quest).
	// NOTE: This is just temporary until we can load quest data.
	// At which time, we'll just send a list of quest IDs & their states (as is done here, just.. hardcoded)
	Packet result(WIZ_QUEST, uint8(1));
	uint16 Class = m_pUserData.m_sClass % 100;
	if (Class == 1 || Class == 5 || Class == 6)
		result << uint16(3) << uint16(51) << uint8(2) << uint16(510) << uint8(2) << uint16(511) << uint8(2); // if 50+baseclass quest ID is completed
	else if (Class == 2 || Class == 7 || Class == 8)
		result << uint16(4) << uint16(52) << uint8(2) << uint16(512) << uint8(2) << uint16(513) << uint8(2) << uint16(514) << uint8(2);
	else if (Class == 3 || Class == 9 || Class == 10)
		result << uint16(4) << uint16(53) << uint8(2) << uint16(515) << uint8(2) << uint16(516) << uint8(2) << uint16(517) << uint8(2);
	else if (Class == 4 || Class == 11 || Class == 12)
		result << uint16(7) << uint16(54) << uint8(2) << uint16(518) << uint8(2) << uint16(519) << uint8(2) << uint16(520) << uint8(2) << uint16(521) << uint8(2) << uint16(522) << uint8(2) << uint16(523) << uint8(2);
	Send(&result);
	*/
}

bool CUser::CheckExistEvent(uint16 sQuestID, uint8 bQuestState)
{
	// Attempt to find a quest with that ID in the map
	QuestMap::iterator itr = m_questMap.find(sQuestID);

	// If it doesn't exist, it doesn't exist. 
	// Unless of course, we wanted it to not exist, in which case we're right!
	// (this is pretty annoyingly dumb, but we'd have to change existing EVT logic to fix this)
	if (itr == m_questMap.end())
		return bQuestState == 0;

	return itr->second == bQuestState;
}
