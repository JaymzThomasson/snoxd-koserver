// User.cpp: implementation of the CUser class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EbenezerDlg.h"
#include "User.h"
#include "AiPacket.h"
#include "Map.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// Cryption
extern T_KEY g_private_key;

CUser::CUser()
{
}

CUser::~CUser()
{
}

void CUser::Initialize()
{
	m_pMain = (CEbenezerDlg*)AfxGetApp()->GetMainWnd();

	// Cryption
	Make_public_key();
	jct.SetPublicKey(m_Public_key);
	jct.SetPrivateKey(g_private_key);
	jct.Init();

	m_CryptionFlag = 0;
	m_Sen_val = 0;
	m_Rec_val = 0;
	///~

	m_bSelectedCharacter = false;
	m_bStoreOpen = false;

	m_MagicProcess.m_pMain = m_pMain;
	m_MagicProcess.m_pSrcUser = this;

	m_RegionX = -1;
	m_RegionZ = -1;

	m_sBodyAc = 0;
	m_sTotalHit = 0;
	m_sTotalAc = 0;
	m_sTotalHitrate = 0;
	m_sTotalEvasionrate = 0;
	
	m_sItemMaxHp = 0;
	m_sItemMaxMp = 0;
	m_sItemWeight = 0;
	m_sItemHit = 0;
	m_sItemAc = 0;
	m_sItemStr = 0;
	m_sItemSta = 0;
	m_sItemDex = 0;
	m_sItemIntel = 0;
	m_sItemCham = 0;
	m_sItemHitrate = 100;
	m_sItemEvasionrate = 100;

	m_sSpeed = 0;

	m_iMaxHp = 0;
	m_iMaxMp = 1;
	m_iMaxExp = 0;
	m_sMaxWeight = 0;

	m_bFireR = 0;
	m_bColdR = 0;
	m_bLightningR = 0;
	m_bMagicR = 0;
	m_bDiseaseR = 0;
	m_bPoisonR = 0;

	m_sDaggerR = 0;			
	m_sSwordR = 0;			
	m_sAxeR = 0;						
	m_sMaceR = 0;						
	m_sSpearR = 0;					
	m_sBowR = 0;		
	
	m_bMagicTypeLeftHand = 0;		// For weapons and shields with special power.		
	m_bMagicTypeRightHand = 0;		
	m_sMagicAmountLeftHand = 0;        
	m_sMagicAmountRightHand = 0;       

	m_pMap = NULL;	
	m_bResHpType = USER_STANDING;
	m_bWarp = 0x00;

	m_sPartyIndex = -1;		
	m_sExchangeUser = -1;
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

	InitType3();	 // Initialize durational type 3 stuff :)
	InitType4();	 // Initialize durational type 4 stuff :)

	m_fSpeedHackClientTime = 0.0f;
	m_fSpeedHackServerTime = 0.0f;
	m_bSpeedHackCheck = 0;

	m_fBlinkStartTime = 0.0f;

	m_sAliveCount = 0;

	m_bAbnormalType = 1;	// User starts out in normal size.

	m_sWhoKilledMe = -1;
	m_iLostExp = 0;

	m_fLastTrapAreaTime = 0.0f;

	memset( m_strAccountID, NULL, MAX_ID_SIZE+1 );

	for (int i = 0 ; i < MAX_MESSAGE_EVENT ; i++) {
		m_iSelMsgEvent[i] = -1;
	}
	
	m_sEventNid = -1;

	m_bZoneChangeFlag = FALSE;

	m_bRegeneType = 0;

	m_fLastRegeneTime = 0.0f;

	m_bZoneChangeSameZone = FALSE;

	memset(m_strCouponId, NULL, MAX_COUPON_ID_LENGTH);
	m_iEditBoxEvent = -1;

	for (int j = 0 ; j < MAX_CURRENT_EVENT ; j++) {
		m_sEvent[j] = -1;
	}

	while( !m_arUserEvent.empty() )
	m_arUserEvent.pop_back();

	CIOCPSocket2::Initialize();
}

// Cryption
void CUser::Make_public_key()
{
	BYTE rand1,rand2,rand3,rand4,rand5,rand6,rand7,rand8;
	
	int out_flag = 0;
	do
	{
		rand1 = rand();
		rand2 = rand();
		rand3 = rand();
		rand4 = rand();
		rand5 = rand();
		rand6 = rand();
		rand7 = rand();
		rand8 = rand();

		m_Public_key = rand1;
		m_Public_key <<= 8;

		m_Public_key |= rand2;
		m_Public_key <<= 8;

		m_Public_key |= rand3;
		m_Public_key <<= 8;

		m_Public_key |= rand4;
		m_Public_key <<= 8;

		m_Public_key |= rand5;
		m_Public_key <<= 8;

		m_Public_key |= rand6;
		m_Public_key <<= 8;

		m_Public_key |= rand7;
		m_Public_key <<= 8;

		m_Public_key |= rand8;

		if(m_Public_key != 0)
			out_flag = 1;

	} while( !out_flag );
}
///~

void CUser::CloseProcess()
{
	if (GetState() == STATE_GAMESTART)
	{
		UserInOut( USER_OUT );

		if (isInParty())
			PartyRemove(m_Sid);

		if (m_sExchangeUser != -1 )
			ExchangeCancel();

		LogOut();
	}
	Initialize();
	CIOCPSocket2::CloseProcess();
}

void CUser::Parsing(int len, char *pData)
{
	int index = 0;
	float	currenttime = FLT_MIN;

	BYTE command = GetByte(pData, index);
	TRACE("Packet: %X (len=%d)\n", command, len);
	// If crypto's not been enabled yet, force the version packet to be sent.
	if (!m_CryptionFlag)
	{
		if (command == WIZ_VERSION_CHECK) {
			VersionCheck(pData+index); return; }

		return;
	}
	// If we're not authed yet, forced us to before we can do anything else.
	else if (m_strAccountID[0] == 0)
	{
		if (command == WIZ_LOGIN)
			LoginProcess(pData+index);

		return;
	}
	// If we haven't logged in yet, don't let us hit in-game packets.
	// TO-DO: Make sure we support all packets in the loading stage (and rewrite this logic considerably better).
	else if (!m_bSelectedCharacter)
	{
		switch( command )
		{
		case WIZ_SEL_NATION:
			SelNationToAgent( pData+index );
			break;
		case WIZ_NEW_CHAR:
			NewCharToAgent( pData+index );
			break;
		case WIZ_DEL_CHAR:
			DelCharToAgent( pData+index );
			break;
		case WIZ_SEL_CHAR:
			SelCharToAgent( pData+index );
			break;
		case WIZ_ALLCHAR_INFO_REQ:
			AllCharInfoToAgent();
			break;
		}
		return;
	}

	// Otherwise, assume we're authed & in-game.
	switch (command)
	{
	case WIZ_GAMESTART:
		if (GetState() == STATE_GAMESTART)
			break;

		GameStart(pData+index);
		break;
	case WIZ_SERVER_INDEX:
		SendServerIndex();
		break;
	case WIZ_RENTAL:
		RentalSystem(pData+index);
		break;
	case WIZ_SKILLDATA:
		SkillDataProcess(pData+index);
		break;
	case WIZ_MOVE:
		MoveProcess( pData+index );
		break;
	case WIZ_ROTATE:
		Rotate( pData+index );
		break;
	case WIZ_ATTACK:
		Attack( pData+index );
		break;
	case WIZ_CHAT:
		Chat( pData+index );
		break;
	case WIZ_CHAT_TARGET:
		ChatTargetSelect( pData+index );
		break;
	case WIZ_REGENE:	
		InitType3();	// Init Type 3.....
		InitType4();	// Init Type 4.....
		Regene( pData+index );
//		InitType3();	// Init Type 3.....
//		InitType4();	// Init Type 4.....
		break;
	case WIZ_REQ_USERIN:
		RequestUserIn( pData+index );
		break;
	case WIZ_REQ_NPCIN:
		RequestNpcIn( pData+index );
		break;
	case WIZ_WARP:
		if( m_pUserData->m_bAuthority == 0 ) {
			Warp( pData+index );
		}
		break;
	case WIZ_ITEM_MOVE:
		ItemMove( pData+index );
		break;
	case WIZ_NPC_EVENT:
		NpcEvent( pData+index );
		break;
	case WIZ_ITEM_TRADE:
		ItemTrade( pData+index );
		break;
	case WIZ_TARGET_HP:
		{
			int uid = GetShort( pData, index );
			BYTE echo = GetByte( pData, index );
			SendTargetHP(echo, uid);
		}
		break;
	case WIZ_BUNDLE_OPEN_REQ:
		BundleOpenReq( pData+index );
		break;
	case WIZ_ITEM_GET:
		ItemGet( pData+index );
		break;
	case WIZ_ZONE_CHANGE:
		//UserInOut( USER_IN );
		UserInOut( USER_REGENE );
		m_pMain->RegionUserInOutForMe(this);
		m_pMain->RegionNpcInfoForMe(this);
		m_bWarp = 0x00;
		break;
	case WIZ_POINT_CHANGE:
		PointChange( pData+index );
		break;
	case WIZ_STATE_CHANGE:
		StateChange( pData+index );
		break;
	//case WIZ_SPEEDHACK_USED:
	//	SpeedHackUser();
	//	break;
	case WIZ_PARTY:
		PartyProcess( pData+index );
		break;
	case WIZ_EXCHANGE:
		ExchangeProcess( pData+index );
		break;
	case WIZ_MAGIC_PROCESS:
		m_MagicProcess.MagicPacket( pData+index, len );
		break;
	case WIZ_MERCHANT:
		MerchantProcess(pData+index);
		break;
	case WIZ_SKILLPT_CHANGE:
		SkillPointChange( pData+index );
		break;
	case WIZ_OBJECT_EVENT:
		ObjectEvent( pData+index );
		break;
	case WIZ_WEATHER:
	case WIZ_TIME:
		UpdateGameWeather( pData+index, command );
		break;
	case WIZ_CLASS_CHANGE:
		ClassChange( pData+index );
		break;
	case WIZ_CONCURRENTUSER:
		CountConcurrentUser();
		break;
	case WIZ_DATASAVE:
		UserDataSaveToAgent();
		break;
	case WIZ_ITEM_REPAIR:
		ItemRepair( pData+index );
		break;
	case WIZ_KNIGHTS_PROCESS:
		m_pMain->m_KnightsManager.PacketProcess( this, pData+index );
		break;
	case WIZ_ITEM_REMOVE:
		ItemRemove( pData+index );
		break;
	case WIZ_OPERATOR:
		OperatorCommand( pData+index );
		break;
	case WIZ_SPEEDHACK_CHECK:
		SpeedHackTime( pData+index );
		m_sAliveCount = 0;
		break;
	case WIZ_SERVER_CHECK:
		ServerStatusCheck();
		break;
	case WIZ_WAREHOUSE:
		WarehouseProcess( pData+index );
		break;
	case WIZ_HOME:
		Home();
		break; 
	case WIZ_FRIEND_PROCESS:
		FriendProcess(pData+index);
		break;
	case WIZ_WARP_LIST:
		SelectWarpList( pData+index );
		break;
	case WIZ_ZONE_CONCURRENT:
		ZoneConCurrentUsers( pData+index );
		break;
	case WIZ_VIRTUAL_SERVER:
		ServerChangeOk( pData+index );
		break;
	case WIZ_PARTY_BBS:
		PartyBBS( pData+index );
		break;
	case WIZ_KICKOUT:
		KickOut( pData+index );
		break;
	case WIZ_CLIENT_EVENT:
		ClientEvent( pData+index );
		break;
	case WIZ_SELECT_MSG:
		RecvSelectMsg( pData+index );
		break;
	case WIZ_EDIT_BOX:
		RecvEditBox( pData+index );
		break;	
	case WIZ_SHOPPING_MALL: // letter system's used in here too
		ShoppingMall(pData+index);
		break;
	}

	currenttime = TimeGet();

	if( command == WIZ_GAMESTART ) {
		m_fHPLastTimeNormal = currenttime;

		for (int h = 0 ; h < MAX_TYPE3_REPEAT ; h++) {
			m_fHPLastTime[h] = currenttime;
		}
	}	

	if( m_fHPLastTimeNormal != 0.0f && (currenttime - m_fHPLastTimeNormal) > m_bHPIntervalNormal && m_bAbnormalType != ABNORMAL_BLINKING) {
		HPTimeChange( currenttime );	// For Sitdown/Standup HP restoration.
	}

	if (m_bType3Flag) {     // For Type 3 HP Duration.
		for (int i = 0 ; i < MAX_TYPE3_REPEAT ; i++) {	
			if( m_fHPLastTime[i] != 0.0f && (currenttime - m_fHPLastTime[i]) > m_bHPInterval[i] ) {
				HPTimeChangeType3(currenttime);	
				break;
			}
		}
	} 

	if (m_bType4Flag) {		// For Type 4 Stat Duration.
		Type4Duration(currenttime);
	}
		
	if (m_bAbnormalType == ABNORMAL_BLINKING) {		// Should you stop blinking?
		BlinkTimeCheck(currenttime);
	}
}

void CUser::SendServerIndex()
{
	char send_buff[5];
	int send_index = 0;
	
	SetByte(send_buff, WIZ_SERVER_INDEX, send_index);
	SetShort(send_buff, 1, send_index); // success
	SetShort(send_buff, m_pMain->m_nServerNo, send_index);

	Send(send_buff, send_index);
}

void CUser::SkillDataProcess(char *pData)
{
	int index = 0;
	BYTE opcode = GetByte(pData, index);

	switch (opcode)
	{
	case SKILL_DATA_SAVE:
		SkillDataSave(pData+index);
		break;

	case SKILL_DATA_LOAD:
		SkillDataLoad(pData+index);
		break;
	}
}

void CUser::SkillDataSave(char *pData)
{
	char send_buff[512];
	int index = 0, send_index = 0, sCount = 0;

	sCount = GetShort(pData, index);
	if (sCount <= 0 || sCount > 64)
		return;

	SetByte(send_buff, WIZ_SKILLDATA, send_index);
	SetShort(send_buff, GetSocketID(), send_index);
	SetByte(send_buff, SKILL_DATA_SAVE, send_index);
	SetShort(send_buff, sCount, send_index);

	for (int i = 0; i < sCount; i++)
	{
		int nItemID = GetDWORD(pData, index);
		SetDWORD(send_buff, nItemID, send_index);
	}
	
	int result = m_pMain->m_LoggerSendQueue.PutData(send_buff, send_index);
	if (result >= SMQ_FULL)
		DEBUG_LOG("Failed to send skillbar save packet : %d", result);
}

void CUser::SkillDataLoad(char *pData)
{
	char send_buff[512];
	int	send_index = 0;

	SetByte(send_buff, WIZ_SKILLDATA, send_index);
	SetShort(send_buff, GetSocketID(), send_index);
	SetByte(send_buff, SKILL_DATA_LOAD, send_index);

	int result = m_pMain->m_LoggerSendQueue.PutData(send_buff, send_index);
	if (result >= SMQ_FULL)
		DEBUG_LOG("Failed to send skillbar load packet : %d", result);
}

void CUser::RecvSkillDataLoad(char *pData)
{
	char send_buff[512];
	int index = 0, send_index = 0, sCount = 0;

	BYTE result = GetByte(pData, index);
	if (!result)
	{
		sCount = 0;
	}
	else
	{
		sCount = GetShort(pData, index);
		if (sCount < 0 || sCount > 64)
			sCount = 0;
	}

	SetByte(send_buff, WIZ_SKILLDATA, send_index);
	SetByte(send_buff, SKILL_DATA_LOAD, send_index);
	SetShort(send_buff, sCount, send_index);

	for (int i = 0; i < sCount; i++) 
	{
		int nItemID = GetDWORD(pData, index);
		SetDWORD(send_buff, nItemID, send_index);
	}

	Send(send_buff,send_index);
}

void CUser::UserDataSaveToAgent()
{
	int send_index = 0, retvalue = 0;
	char send_buff[256];

	if (m_pUserData->m_id[0] == 0 || m_pUserData->m_Accountid[0] == 0)
		return;

	SetByte( send_buff, WIZ_DATASAVE, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetKOString(send_buff, m_pUserData->m_Accountid, send_index);
	SetKOString(send_buff, m_pUserData->m_id, send_index);

	retvalue = m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );
	if (retvalue >= SMQ_FULL)
		DEBUG_LOG("DataSave Send Fail : %d", retvalue);
}

void CUser::LogOut()
{
	int index = 0, idlen = 0, idindex = 0, send_index = 0, count = 0;
	CUser* pUser = NULL;
	char send_buf[256]; 

	CTime t = CTime::GetCurrentTime();
	m_pMain->WriteLog("[%s : %s Logout : %d:%d:%d]\r\n", m_pUserData->m_Accountid, m_pUserData->m_id, t.GetHour(), t.GetMinute(), t.GetSecond());

	pUser = m_pMain->GetUserPtr(m_pUserData->m_Accountid, TYPE_ACCOUNT);
	if( pUser && (pUser->GetSocketID() != GetSocketID()) ) 
	{
		TRACE("%s : %s Logout: Sid ?? ??? ???...\n", m_pUserData->m_Accountid, m_pUserData->m_id);
		return;
	}
	else 
	{
		if (m_pUserData->m_id[0] == 0) 
			return; 

		SetByte( send_buf, WIZ_LOGOUT, send_index );
		SetShort( send_buf, m_Sid, send_index );
		SetKOString( send_buf, m_pUserData->m_Accountid, send_index);
		SetKOString( send_buf, m_pUserData->m_id, send_index);

		do {
			if( m_pMain->m_LoggerSendQueue.PutData( send_buf, send_index ) == 1 )
				break;
			else
				count++;
		} while( count < 30 );
		if( count > 29 ) {
			m_pMain->AddToList("Logout Send Fail : acname=%s, charid=%s ", m_pUserData->m_Accountid, m_pUserData->m_id);
		}

		SetByte( send_buf, AG_USER_LOG_OUT, index );
		m_pMain->Send_AIServer(send_buf, send_index);
	}

//	if( m_pUserData->m_bKnights > 0 )	{
//		m_pMain->m_KnightsManager.ModifyKnightsUser( m_pUserData->m_bKnights, m_pUserData->m_id, m_pUserData->m_bFame, m_pUserData->m_bLevel, m_pUserData->m_sClass, 0);
//	}
//	TRACE("Send Logout Result - %s\n", m_pUserData->m_id);
	return; 
}

void CUser::SendMyInfo()
{
	C3DMap* pMap = GetMap();
	CKnights* pKnights = NULL;

	int  send_index = 0, i=0, iLength = 0;
	char send_buff[2048];

	short x = 0, z = 0;
//	int map_size = (pMap->m_nMapSize - 1) * pMap->m_fUnitDist ;		// Are you within the map limits?
//	if (m_pUserData->m_curx >= map_size || m_pUserData->m_curz >= map_size) {

	if( !pMap->IsValidPosition( m_pUserData->m_curx, m_pUserData->m_curz, 0.0f ) ) {
		GetStartPosition(x, z); 

		m_pUserData->m_curx = (float)x;
		m_pUserData->m_curz = (float)z;
	}

	// Unlock skill data (level 70 skill quest).
	SetByte(send_buff, WIZ_QUEST, send_index);
	SetByte(send_buff, 2, send_index);
	SetShort(send_buff, 0, send_index); // if 50+baseclass quest ID is completed
	SetByte(send_buff, 0, send_index);
	Send(send_buff, send_index);

	send_index = 0;

	SetByte( send_buff, WIZ_MYINFO, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetKOString(send_buff, m_pUserData->m_id, send_index, 1);

	SetShort( send_buff, (WORD)m_pUserData->m_curx*10, send_index );
	SetShort( send_buff, (WORD)m_pUserData->m_curz*10, send_index );
	SetShort( send_buff, (short)m_pUserData->m_cury*10, send_index );

	SetByte( send_buff, m_pUserData->m_bNation, send_index );
	SetByte( send_buff, m_pUserData->m_bRace, send_index );
	SetShort( send_buff, m_pUserData->m_sClass, send_index );
	SetByte( send_buff, m_pUserData->m_bFace, send_index );

	SetDWORD( send_buff, m_pUserData->m_nHair, send_index );

	SetByte( send_buff, m_pUserData->m_bRank, send_index );
	SetByte( send_buff, m_pUserData->m_bTitle, send_index );
	SetByte( send_buff, m_pUserData->m_bLevel, send_index );
	SetShort( send_buff, m_pUserData->m_sPoints, send_index );
	SetInt64( send_buff, m_iMaxExp, send_index );
	SetInt64( send_buff, m_pUserData->m_iExp, send_index );
	SetDWORD( send_buff, m_pUserData->m_iLoyalty, send_index );
	SetDWORD( send_buff, m_pUserData->m_iLoyaltyMonthly, send_index );

	SetShort( send_buff, m_pUserData->m_bKnights, send_index );
	SetShort( send_buff, m_pUserData->m_bFame, send_index );
	SetByte( send_buff, m_pUserData->m_bCity, send_index );

	if (m_pUserData->m_bKnights != 0)
		pKnights = m_pMain->m_KnightsArray.GetData(m_pUserData->m_bKnights);

	if (pKnights == NULL)
	{
		SetByte(send_buff, 0, send_index);   // grade type
		SetByte(send_buff, 0, send_index);   // name len
		SetByte(send_buff, 0, send_index);   // grade
		SetByte(send_buff, 0, send_index );  // rank
		SetShort(send_buff, 0, send_index);  // symbol/mark version

		SetShort(send_buff, -1, send_index );// cape num
		SetByte(send_buff, 0, send_index);   // cape r
		SetByte(send_buff, 0, send_index);   // cape g
		SetByte(send_buff, 0, send_index);   // cape b
	}
	else 
	{
		SetByte(send_buff, 0, send_index);  // grade type
		SetCString(send_buff, pKnights->m_strName, send_index, 1);
		SetByte(send_buff, pKnights->m_byGrade, send_index );
		SetByte(send_buff, pKnights->m_byRanking, send_index );
		SetShort(send_buff, 0, send_index); // symbol/mark version
		SetShort(send_buff, -1, send_index);// cape num
		SetByte(send_buff, 0, send_index);  //cape r
		SetByte(send_buff, 0, send_index);  //cape g
		SetByte(send_buff, 0, send_index);  //cape b
	}

	SetByte(send_buff, 0, send_index); // unknown
	SetByte(send_buff, 2, send_index);
	SetByte(send_buff, 3, send_index);
	SetByte(send_buff, 4, send_index);
	SetByte(send_buff, 5, send_index);

	SetShort(send_buff, m_iMaxHp, send_index);
	SetShort(send_buff, m_pUserData->m_sHp, send_index);

	SetShort(send_buff, m_iMaxMp, send_index);
	SetShort(send_buff, m_pUserData->m_sMp, send_index);

	SetDWORD(send_buff, m_sMaxWeight, send_index);
	SetDWORD(send_buff, m_sItemWeight, send_index);

	SetByte(send_buff, m_pUserData->m_bStr, send_index);
	SetByte(send_buff, (BYTE)m_sItemStr, send_index);
	SetByte(send_buff, m_pUserData->m_bSta, send_index);
	SetByte(send_buff, (BYTE)m_sItemSta, send_index);
	SetByte(send_buff, m_pUserData->m_bDex, send_index);
	SetByte(send_buff, (BYTE)m_sItemDex, send_index);
	SetByte(send_buff, m_pUserData->m_bIntel, send_index);
	SetByte(send_buff, (BYTE)m_sItemIntel, send_index);
	SetByte(send_buff, m_pUserData->m_bCha, send_index);
	SetByte(send_buff, (BYTE)m_sItemCham, send_index);	

	SetShort(send_buff, m_sTotalHit, send_index);
	SetShort(send_buff, m_sTotalAc, send_index);

	SetByte(send_buff, m_bFireR, send_index);
	SetByte(send_buff, m_bColdR, send_index);
	SetByte(send_buff, m_bLightningR, send_index);
	SetByte(send_buff, m_bMagicR, send_index);
	SetByte(send_buff, m_bDiseaseR, send_index);
	SetByte(send_buff, m_bPoisonR, send_index);

	SetDWORD(send_buff, m_pUserData->m_iGold, send_index);
	SetByte(send_buff, m_pUserData->m_bAuthority, send_index);

	SetByte(send_buff, -1, send_index); // national rank
	SetByte(send_buff, -1, send_index); // leader rank

	SetString(send_buff, (char *)m_pUserData->m_bstrSkill, 9, send_index);

	for (i=0; i < HAVE_MAX + SLOT_MAX + COSP_MAX + MBAG_MAX; i++)
	{
		SetDWORD(send_buff, m_pUserData->m_sItemArray[i].nNum, send_index);
		SetShort(send_buff, m_pUserData->m_sItemArray[i].sDuration, send_index);
		SetShort(send_buff, m_pUserData->m_sItemArray[i].sCount, send_index);
		SetByte(send_buff, 0, send_index);  // item type flag (e.g. rented)
		SetShort(send_buff, 0, send_index); // remaining time
		SetDWORD(send_buff, 0, send_index); // unknown
		SetDWORD(send_buff, 0, send_index); // expiration date
	}

	SetByte(send_buff, 0, send_index); // never worked out what this was for: possible values 0/1/2/3
	SetByte(send_buff, 0, send_index); // premium type
	SetShort(send_buff, 0, send_index); // premium time

	SetByte(send_buff, 0, send_index); // chicken flag

	SetDWORD(send_buff, m_pUserData->m_iMannerPoint, send_index);
	Send(send_buff, send_index);

	SendPremiumInfo();
	SetZoneAbilityChange(getZoneID());

	send_index = 0;

	SetByte( send_buff, AG_USER_INFO, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetKOString(send_buff, m_pUserData->m_id, send_index);
	SetByte( send_buff, m_pUserData->m_bZone, send_index );
	SetByte( send_buff, m_pUserData->m_bNation, send_index );
	SetByte( send_buff, m_pUserData->m_bLevel, send_index );
	SetShort( send_buff, m_pUserData->m_sHp, send_index );
	SetShort( send_buff, m_pUserData->m_sMp, send_index );
	SetShort( send_buff, m_sTotalHit * m_bAttackAmount / 100, send_index );  // g??
	SetShort( send_buff, m_sTotalAc + m_sACAmount, send_index );  // g??
	Setfloat( send_buff, m_sTotalHitrate, send_index );
	Setfloat( send_buff, m_sTotalEvasionrate, send_index );

// Yookozuna
	SetShort( send_buff, m_sItemAc, send_index);
	SetByte( send_buff, m_bMagicTypeLeftHand, send_index);
	SetByte( send_buff, m_bMagicTypeRightHand, send_index);
	SetShort( send_buff, m_sMagicAmountLeftHand, send_index);
	SetShort( send_buff, m_sMagicAmountRightHand, send_index);
	SetByte( send_buff, m_pUserData->m_bAuthority, send_index );
//
	m_pMain->Send_AIServer(send_buff, send_index);

//	if( m_pUserData->m_bKnights > 0 )	{
//		m_pMain->m_KnightsManager.ModifyKnightsUser( m_pUserData->m_bKnights, m_pUserData->m_id, m_pUserData->m_bFame, m_pUserData->m_bLevel, m_pUserData->m_sClass, 1);
//	}
}

void CUser::SetMaxHp(int iFlag)
{
	_CLASS_COEFFICIENT* p_TableCoefficient = NULL;
	p_TableCoefficient = m_pMain->m_CoefficientArray.GetData( m_pUserData->m_sClass );
	if( !p_TableCoefficient ) return;

	int temp_sta = 0;
	temp_sta = m_pUserData->m_bSta + m_sItemSta + m_bStaAmount;
//	if( temp_sta > 255 ) temp_sta = 255;

	if( m_pUserData->m_bZone == ZONE_SNOW_BATTLE && iFlag == 0 )	{
		m_iMaxHp = 100;
		//TRACE("--> SetMaxHp - name=%s, max=%d, hp=%d\n", m_pUserData->m_id, m_iMaxHp, m_pUserData->m_sHp);
	}
	else	{
		m_iMaxHp = (short)(((p_TableCoefficient->HP * m_pUserData->m_bLevel * m_pUserData->m_bLevel * temp_sta ) 
		      + (0.1 * m_pUserData->m_bLevel * temp_sta ) + (temp_sta / 5)) + m_sMaxHPAmount + m_sItemMaxHp);
		if( iFlag == 1 )	m_pUserData->m_sHp = m_iMaxHp + 20;		// ??? ?? hp?? ??? ?????? hpchange()??? ?????,, ???^^*
		else if( iFlag == 2 )	m_iMaxHp = 100;
		//TRACE("<-- SetMaxHp - name=%s, max=%d, hp=%d\n", m_pUserData->m_id, m_iMaxHp, m_pUserData->m_sHp);
	}

	if(m_iMaxHp < m_pUserData->m_sHp) {
		m_pUserData->m_sHp = m_iMaxHp;
		HpChange( m_pUserData->m_sHp );
	}
	if( m_pUserData->m_sHp < 5 )
		m_pUserData->m_sHp = 5;
}

void CUser::SetMaxMp()
{
	_CLASS_COEFFICIENT* p_TableCoefficient = NULL;
	p_TableCoefficient = m_pMain->m_CoefficientArray.GetData( m_pUserData->m_sClass );
	if( !p_TableCoefficient ) return;

	int temp_intel = 0, temp_sta = 0;
	temp_intel = m_pUserData->m_bIntel + m_sItemIntel + m_bIntelAmount + 30;
//	if( temp_intel > 255 ) temp_intel = 255;
	temp_sta = m_pUserData->m_bSta + m_sItemSta + m_bStaAmount;
//	if( temp_sta > 255 ) temp_sta = 255;

	if( p_TableCoefficient->MP != 0)
	{
		m_iMaxMp = (short)((p_TableCoefficient->MP * m_pUserData->m_bLevel * m_pUserData->m_bLevel * temp_intel)
				  + (0.1f * m_pUserData->m_bLevel * 2 * temp_intel) + (temp_intel / 5));
		m_iMaxMp += m_sItemMaxMp;		
		m_iMaxMp += 20;		 // ?????? ??�
	}
	else if( p_TableCoefficient->SP != 0)
	{
		m_iMaxMp = (short)((p_TableCoefficient->SP * m_pUserData->m_bLevel * m_pUserData->m_bLevel * temp_sta )
			  + (0.1f * m_pUserData->m_bLevel * temp_sta) + (temp_sta / 5));
		m_iMaxMp += m_sItemMaxMp;
	}

	if(m_iMaxMp < m_pUserData->m_sMp) {
		m_pUserData->m_sMp = m_iMaxMp;
		MSpChange( m_pUserData->m_sMp );
	}
}

void CUser::SendTimeStatus()
{
	char	send_buff[256];
	int send_index = 0;

	SetByte( send_buff, WIZ_TIME, send_index );
	SetShort( send_buff, m_pMain->m_nYear, send_index );
	SetShort( send_buff, m_pMain->m_nMonth, send_index );
	SetShort( send_buff, m_pMain->m_nDate, send_index );
	SetShort( send_buff, m_pMain->m_nHour, send_index );
	SetShort( send_buff, m_pMain->m_nMin, send_index );
	Send( send_buff, send_index );

	send_index = 0;
	SetByte( send_buff, WIZ_WEATHER, send_index );
	SetByte( send_buff, (BYTE)m_pMain->m_nWeather, send_index );
	SetShort( send_buff, m_pMain->m_nAmount, send_index );
	Send( send_buff, send_index );
}

void CUser::SetZoneAbilityChange(BYTE zone)
{
	char send_buff[6]; 
	int send_index = 0;

	SetByte(send_buff, WIZ_ZONEABILITY, send_index);
	SetByte(send_buff, 1, send_index);

	// Moradon or temples (but NOT FT).
	if (zone == 21
		|| ((zone / 10) == 5 && zone != 54))
	{
		SetByte(send_buff, 1, send_index);
		SetByte(send_buff, 0, send_index);
		SetByte(send_buff, 1, send_index);

		if (zone == 21)
			SetShort(send_buff, 20, send_index); // moradon tariff
		else
			SetShort(send_buff, 10, send_index); // tariff for all other 5* zones (temples), except FT
	}
	// Arena
	else if (zone == 48)
	{
		SetByte(send_buff, 0, send_index);
		SetByte(send_buff, 0, send_index);
		SetByte(send_buff, 1, send_index);

		SetShort(send_buff, 10, send_index);
	}
	// Now we handle FT
	else if (zone == 54)
	{
		SetByte(send_buff, 0, send_index);
		SetByte(send_buff, 7, send_index);
		SetByte(send_buff, 1, send_index);

		SetShort(send_buff, 10, send_index);
	}
	// desperation abyss & hell abyss
	else if (zone == 32 || zone == 33)
	{
		SetByte(send_buff, 0, send_index);
		SetByte(send_buff, 8, send_index);
		SetByte(send_buff, 1, send_index);

		SetShort(send_buff, 10, send_index); // tariff 
	}
	// colony zone
	else if (zone == 201)
	{
		SetByte(send_buff, 0, send_index);
		SetByte(send_buff, 1, send_index);
		SetByte(send_buff, 0, send_index);

		SetShort(send_buff, 20, send_index);
	}
	// delos
	else if (zone == 31)
	{
		// to-do
	}
	else if (zone == 1 || zone == 11)
	{
		SetByte(send_buff, 0, send_index);
		SetByte(send_buff, 1, send_index);
		SetByte(send_buff, 0, send_index);

		SetShort(send_buff, 10, send_index); // orc nation
	}
	else if (zone == 2 || zone == 12)
	{
		SetByte(send_buff, 0, send_index);
		SetByte(send_buff, 1, send_index);
		SetByte(send_buff, 0, send_index);

		SetShort(send_buff, 10, send_index); // human nation
	}
	else
		return;

	Send(send_buff, send_index);
}

void CUser::SendPremiumInfo()
{
	char send_buff[7]; int send_index = 0;

	SetByte(send_buff, WIZ_PREMIUM, send_index);
	SetByte(send_buff, 1, send_index);
	SetByte(send_buff, 0, send_index); // premium type
	SetDWORD(send_buff, 0, send_index); // premium time

	Send(send_buff, send_index);
}

void CUser::SetDetailData()
{
	C3DMap* pMap = NULL;

	SetSlotItemValue();
	SetUserAbility();

	if (getLevel() >= MAX_LEVEL) 
	{
		CloseProcess();
		return;
	}

	m_iMaxExp = m_pMain->GetExpByLevel(getLevel());
	m_sMaxWeight = (m_pUserData->m_bStr + m_sItemStr) * 50;

	m_pMap = m_pMain->GetZoneByID(m_pUserData->m_bZone);
	if (m_pMap == NULL) 
	{
		CloseProcess();
		return;
	}

	m_fWill_x = m_pUserData->m_curx;
	m_fWill_z = m_pUserData->m_curz;
	m_fWill_y = m_pUserData->m_cury;

	m_RegionX = (int)(m_pUserData->m_curx / VIEW_DISTANCE);
	m_RegionZ = (int)(m_pUserData->m_curz / VIEW_DISTANCE);
}

void CUser::RegisterRegion()
{
	int iRegX = 0, iRegZ = 0, old_region_x = 0, old_region_z = 0;
	iRegX = (int)(m_pUserData->m_curx / VIEW_DISTANCE);
	iRegZ = (int)(m_pUserData->m_curz / VIEW_DISTANCE);

	if( m_RegionX != iRegX || m_RegionZ != iRegZ)
	{
		C3DMap* pMap = GetMap();
		if( !pMap )
			return;
		
		old_region_x = m_RegionX;	old_region_z = m_RegionZ;
		pMap->RegionUserRemove(m_RegionX, m_RegionZ, m_Sid);
		m_RegionX = iRegX;		m_RegionZ = iRegZ;
		pMap->RegionUserAdd(m_RegionX, m_RegionZ, m_Sid);

		if( m_State == STATE_GAMESTART ) {
			RemoveRegion( old_region_x - m_RegionX, old_region_z - m_RegionZ );
			InsertRegion( m_RegionX - old_region_x, m_RegionZ - old_region_z );	
			m_pMain->RegionNpcInfoForMe(this);
			m_pMain->RegionUserInOutForMe(this);
		}
	}
}

void CUser::RemoveRegion(int del_x, int del_z)
{
	int send_index = 0;
	int region_x = -1, region_z = -1;
	char buff[256];
	C3DMap* pMap = GetMap();

	if (!pMap)
		return;

	SetByte( buff, WIZ_USER_INOUT, send_index );
	SetByte( buff, USER_OUT, send_index );
	SetShort( buff, GetSocketID(), send_index );

	if( del_x != 0 ) {
		m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX+del_x*2, m_RegionZ+del_z-1 );
		m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX+del_x*2, m_RegionZ+del_z );
		m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX+del_x*2, m_RegionZ+del_z+1 );
	}
	if( del_z != 0 ) {
		m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX+del_x, m_RegionZ+del_z*2 );
		if( del_x < 0 )
			m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX+del_x+1, m_RegionZ+del_z*2 );
		else if( del_x > 0 )
			m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX+del_x-1, m_RegionZ+del_z*2 );
		else {
			m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX+del_x-1, m_RegionZ+del_z*2 );
			m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX+del_x+1, m_RegionZ+del_z*2 );
		}
	}
}

void CUser::InsertRegion(int del_x, int del_z)
{
	int send_index = 0;
	char buff[256];
	C3DMap* pMap = GetMap();

	if (pMap == NULL)
		return;

	SetByte( buff, WIZ_USER_INOUT, send_index );
	SetByte( buff, USER_IN, send_index );
	SetShort( buff, m_Sid, send_index );

	GetUserInfo(buff, send_index);

	if (del_x != 0)
	{
		m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX+del_x, m_RegionZ-1 );
		m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX+del_x, m_RegionZ );
		m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX+del_x, m_RegionZ+1 );
	}

	if( del_z != 0 ) 
	{
		m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX, m_RegionZ+del_z );
		
		if( del_x < 0 )	
			m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX+1, m_RegionZ+del_z );		
		else if( del_x > 0 )
			m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX-1, m_RegionZ+del_z );
		else {
			m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX-1, m_RegionZ+del_z );
			m_pMain->Send_UnitRegion( buff, send_index, pMap, m_RegionX+1, m_RegionZ+del_z );
		}
	}
}

void CUser::RequestUserIn(char *pBuf)
{
	int index = 0, uid = -1, user_count = 0, buff_index = 0, t_count = 0, i=0,j=0, iLength=0;
	CUser* pUser = NULL;
	CKnights* pKnights = NULL;
	char buff[40960];

	buff_index = 3;
	user_count = GetShort( pBuf, index );
	for( i=0; i<user_count; i++ ) {
		uid = GetShort( pBuf, index );
		if( uid < 0 || uid >= MAX_USER )
			continue;
		if( i > 1000 ) 
			break;
		pUser = m_pMain->GetUserPtr(uid);
		if (pUser != NULL && pUser->GetState() == STATE_GAMESTART)
		{
			SetShort( buff, pUser->GetSocketID(), buff_index );
			GetUserInfo(buff, buff_index);
			t_count++;
		}
	}

	int temp_index = 0;
	SetByte( buff, WIZ_REQ_USERIN, temp_index );
	SetShort( buff, t_count, temp_index );
	SendCompressingPacket( buff, buff_index );
}

void CUser::RequestNpcIn(char *pBuf)
{
	if( m_pMain->m_bPointCheckFlag == FALSE)	return;	// ?????? ?????? ???

	int index = 0, nid = -1, npc_count = 0, buff_index = 0, t_count = 0, i=0,j=0;
	CNpc* pNpc = NULL;
	char buff[20480];

	buff_index = 3;	
	npc_count = GetShort( pBuf, index );
	for( i=0; i<npc_count; i++ ) {
		nid = GetShort( pBuf, index );
		if( nid < 0 || nid > NPC_BAND+NPC_BAND)
			continue;
		if( i > 1000 ) break;
		pNpc = m_pMain->m_arNpcArray.GetData(nid);
		if (pNpc == NULL)
			continue;

		SetShort( buff, pNpc->m_sNid, buff_index );
		pNpc->GetNpcInfo(buff, buff_index);
		t_count++;
	}

	int temp_index = 0;
	SetByte( buff, WIZ_REQ_NPCIN, temp_index );
	SetShort( buff, t_count, temp_index );

	SendCompressingPacket( buff, buff_index );
}

void CUser::SetSlotItemValue()	// ????? ???????? ??(�???, ????, ??????)? ?????.
{
	_ITEM_TABLE* pTable = NULL;
	int item_hit = 0, item_ac = 0;

	m_sItemMaxHp = 0; m_sItemMaxMp = 0;
	m_sItemHit = 0; m_sItemAc = 0; m_sItemStr = 0; m_sItemSta = 0; m_sItemDex = 0; m_sItemIntel = 0;
	m_sItemCham = 0; m_sItemHitrate = 100; m_sItemEvasionrate = 100; m_sItemWeight = 0;	

	m_bFireR = 0; m_bColdR = 0; m_bLightningR = 0; m_bMagicR = 0; m_bDiseaseR = 0; m_bPoisonR = 0;
	
	m_sDaggerR = 0; m_sSwordR = 0; m_sAxeR = 0; m_sMaceR = 0; m_sSpearR = 0; m_sBowR = 0;
	m_bMagicTypeLeftHand = 0; m_bMagicTypeRightHand = 0; m_sMagicAmountLeftHand = 0; m_sMagicAmountRightHand = 0;       

	for(int i=0; i<SLOT_MAX; i++)  {
		if(m_pUserData->m_sItemArray[i].nNum <= 0)
			continue;
		pTable = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[i].nNum );
		if( !pTable )
			continue;
		if( m_pUserData->m_sItemArray[i].sDuration == 0 ) {
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
			if( ( m_pUserData->m_sClass == BERSERKER || m_pUserData->m_sClass == BLADE ) )
				m_sItemHit += (short)(item_hit * 0.5f);
		}

		m_sItemMaxHp += pTable->m_MaxHpB;
		m_sItemMaxMp += pTable->m_MaxMpB;
		m_sItemAc += item_ac;
		m_sItemStr += pTable->m_bStrB;
		m_sItemSta += pTable->m_bStaB;
		m_sItemDex += pTable->m_bDexB;
		m_sItemIntel += pTable->m_bIntelB;
		m_sItemCham += pTable->m_bChaB;
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
		if(m_pUserData->m_sItemArray[i].nNum <= 0) continue;

		pTable = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[i].nNum );
		if( !pTable ) continue;

		if (pTable->m_bCountable == 0) {	// Non-countable items.
			m_sItemWeight += pTable->m_sWeight;
		}
		else {	// Countable items.
			m_sItemWeight += pTable->m_sWeight * m_pUserData->m_sItemArray[i].sCount;
		}
	}
//	
	if( m_sItemHit < 3 )
		m_sItemHit = 3;

	// For magical items..... by Yookozuna 2002.7.10
	_ITEM_TABLE* pLeftHand = NULL;			// Get item info for left hand.
	pLeftHand = m_pMain->m_ItemtableArray.GetData(m_pUserData->m_sItemArray[LEFTHAND].nNum);
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
	pRightHand = m_pMain->m_ItemtableArray.GetData(m_pUserData->m_sItemArray[RIGHTHAND].nNum);
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

void CUser::ExpChange(__int64 iExp)
{	
	char buff[256];
	int send_index = 0;

	if( m_pUserData->m_bLevel < 6 && iExp < 0 )
		return;
//	
	if( m_pUserData->m_bZone == ZONE_BATTLE && iExp < 0 ) 
		return;
//

/*
	if( m_pUserData->m_bZone != m_pUserData->m_bNation && m_pUserData->m_bZone < 3 && iExp < 0 ) {
		iExp = iExp / 10;
	}
*/
	m_pUserData->m_iExp += iExp;

	if( m_pUserData->m_iExp < 0 ) {
		if( m_pUserData->m_bLevel > 5) {
			m_pUserData->m_bLevel--;
			m_pUserData->m_iExp = m_pMain->GetExpByLevel(m_pUserData->m_bLevel) + m_pUserData->m_iExp;
			LevelChange( m_pUserData->m_bLevel, FALSE );
			return;
		}
	}
	else if( m_pUserData->m_iExp >= m_iMaxExp ) {
		if( m_pUserData->m_bLevel >= MAX_LEVEL ) {
			m_pUserData->m_iExp = m_iMaxExp;
			return;
		}

		m_pUserData->m_iExp = m_pUserData->m_iExp - m_iMaxExp;
		m_pUserData->m_bLevel++;
		LevelChange( m_pUserData->m_bLevel );
		return;
	}

	SetByte( buff, WIZ_EXP_CHANGE, send_index );
	SetInt64( buff, m_pUserData->m_iExp, send_index );
	Send( buff, send_index);
//	
	if (iExp < 0) {
		m_iLostExp = -iExp;
	}
//
}

void CUser::LevelChange(short level, BYTE type )
{
	if( level < 1 || level > MAX_LEVEL )
		return;

	char buff[256];
	int send_index = 0;

	if( type ) {
		if( (m_pUserData->m_sPoints+m_pUserData->m_bSta+m_pUserData->m_bStr+m_pUserData->m_bDex+m_pUserData->m_bIntel+m_pUserData->m_bCha) < (300+3*(level-1)) )
			m_pUserData->m_sPoints += 3;
		if( level > 9 && (m_pUserData->m_bstrSkill[0]+m_pUserData->m_bstrSkill[1]+m_pUserData->m_bstrSkill[2]+m_pUserData->m_bstrSkill[3]+m_pUserData->m_bstrSkill[4]
			+m_pUserData->m_bstrSkill[5]+m_pUserData->m_bstrSkill[6]+m_pUserData->m_bstrSkill[7]+m_pUserData->m_bstrSkill[8]) < (2*(level-9)) )
			m_pUserData->m_bstrSkill[0] += 2;	// Skill Points up
	}

	m_iMaxExp = m_pMain->GetExpByLevel(level);
	
	SetSlotItemValue();
	SetUserAbility();

	m_pUserData->m_sMp = m_iMaxMp;
	HpChange( m_iMaxHp );

	Send2AI_UserUpdateInfo();

	send_index = 0;
	SetByte( buff, WIZ_LEVEL_CHANGE, send_index );
	SetShort( buff, m_Sid, send_index );
	SetByte( buff, m_pUserData->m_bLevel, send_index );
	SetShort( buff, m_pUserData->m_sPoints, send_index );
	SetByte( buff, m_pUserData->m_bstrSkill[0], send_index );
	SetInt64( buff, m_iMaxExp, send_index );
	SetInt64( buff, m_pUserData->m_iExp, send_index );
	SetShort( buff, m_iMaxHp, send_index );
	SetShort( buff, m_pUserData->m_sHp, send_index );
	SetShort( buff, m_iMaxMp, send_index );
	SetShort( buff, m_pUserData->m_sMp, send_index );
	SetShort( buff, m_sMaxWeight, send_index );	
	SetShort( buff, m_sItemWeight, send_index );
	m_pMain->Send_Region( buff, send_index, GetMap(), m_RegionX, m_RegionZ );
	if( m_sPartyIndex != -1 ) {
		send_index = 0;
		SetByte( buff, WIZ_PARTY, send_index );
		SetByte( buff, PARTY_LEVELCHANGE, send_index );
		SetShort( buff, m_Sid, send_index );
		SetByte( buff, m_pUserData->m_bLevel, send_index );
		m_pMain->Send_PartyMember(m_sPartyIndex, buff, send_index);
	}
}

void CUser::PointChange(char *pBuf)
{
	int index = 0, send_index = 0, value = 0;
	BYTE type = 0x00;
	char send_buff[128];

	type = GetByte( pBuf, index );
	value = GetShort( pBuf, index );
	if (type > 5 || value != 1
		|| m_pUserData->m_sPoints < 1) return;

	switch( type ) {
	case STR:
		if( m_pUserData->m_bStr == 0xFF ) return;
		break;
	case STA:
		if( m_pUserData->m_bSta == 0xFF ) return;
		break;
	case DEX:
		if( m_pUserData->m_bDex == 0xFF ) return;
		break;
	case INTEL:
		if( m_pUserData->m_bIntel == 0xFF ) return;
		break;
	case CHA:
		if( m_pUserData->m_bCha == 0xFF ) return;
		break;
	}

	m_pUserData->m_sPoints -= value;

	SetByte( send_buff, WIZ_POINT_CHANGE, send_index );
	SetByte( send_buff, type, send_index );
	switch( type ) {
	case STR:
		m_pUserData->m_bStr++;
		SetShort( send_buff, m_pUserData->m_bStr, send_index );
		SetUserAbility();
		break;
	case STA:
		m_pUserData->m_bSta++;
		SetShort( send_buff, m_pUserData->m_bSta, send_index );
		SetMaxHp();
		SetMaxMp();
		break;
	case DEX:
		m_pUserData->m_bDex++;
		SetShort( send_buff, m_pUserData->m_bDex, send_index );
		SetUserAbility();
		break;
	case INTEL:
		m_pUserData->m_bIntel++;
		SetShort( send_buff, m_pUserData->m_bIntel, send_index );
		SetMaxMp();
		break;
	case CHA:
		m_pUserData->m_bCha++;
		SetShort( send_buff, m_pUserData->m_bCha, send_index );
		break;
	}
	SetShort( send_buff, m_iMaxHp, send_index );
	SetShort( send_buff, m_iMaxMp, send_index );
	SetShort( send_buff, m_sTotalHit, send_index );
	SetShort( send_buff, m_sMaxWeight, send_index );
	Send( send_buff, send_index );
}

void CUser::HpChange(int amount, int type, bool attack)		// type : Received From AIServer -> 1, The Others -> 0
{															// attack : Direct Attack(true) or Other Case(false)
	char buff[256];
	int send_index = 0;

	m_pUserData->m_sHp += amount;
	if( m_pUserData->m_sHp < 0 )
		m_pUserData->m_sHp = 0;
	else if ( m_pUserData->m_sHp > m_iMaxHp )
		m_pUserData->m_sHp = m_iMaxHp;

	SetByte( buff, WIZ_HP_CHANGE, send_index );
	SetShort( buff, m_iMaxHp, send_index );
	SetShort( buff, m_pUserData->m_sHp, send_index );
	Send( buff, send_index);

	if(type == 0) {
		send_index = 0;

		SetByte( buff, AG_USER_SET_HP, send_index );
		SetShort( buff, m_Sid, send_index );
		SetDWORD( buff, m_pUserData->m_sHp, send_index );
		m_pMain->Send_AIServer(buff, send_index);
	}

	if( m_sPartyIndex != -1 ) {
		send_index = 0;

		SetByte( buff, WIZ_PARTY, send_index );
		SetByte( buff, PARTY_HPCHANGE, send_index );
		SetShort( buff, m_Sid, send_index );
		SetShort( buff, m_iMaxHp, send_index );
		SetShort( buff, m_pUserData->m_sHp, send_index );
		SetShort( buff, m_iMaxMp, send_index );
		SetShort( buff, m_pUserData->m_sMp, send_index );
		m_pMain->Send_PartyMember(m_sPartyIndex, buff, send_index);
	}
	
	if( m_pUserData->m_sHp == 0 && attack == false) // ??? ??????? ??�??? Dead Packet ???
		Dead();
}

void CUser::MSpChange(int amount)
{
	char buff[256];
	int send_index = 0;

	m_pUserData->m_sMp += amount;
	if( m_pUserData->m_sMp < 0 )
		m_pUserData->m_sMp = 0;
	else if ( m_pUserData->m_sMp > m_iMaxMp )
		m_pUserData->m_sMp = m_iMaxMp;
	
	SetByte( buff, WIZ_MSP_CHANGE, send_index );
	SetShort( buff, m_iMaxMp, send_index );
	SetShort( buff, m_pUserData->m_sMp, send_index );
	Send( buff, send_index);

	if( m_sPartyIndex != -1 ) {
		send_index = 0; 

		SetByte( buff, WIZ_PARTY, send_index );
		SetByte( buff, PARTY_HPCHANGE, send_index );
		SetShort( buff, m_Sid, send_index );
		SetShort( buff, m_iMaxHp, send_index );
		SetShort( buff, m_pUserData->m_sHp, send_index );
		SetShort( buff, m_iMaxMp, send_index );
		SetShort( buff, m_pUserData->m_sMp, send_index );
		m_pMain->Send_PartyMember(m_sPartyIndex, buff, send_index);
	}
}

void CUser::Send2AI_UserUpdateInfo()
{
	int  send_index = 0;
	char send_buf[1024];

	SetByte(send_buf, AG_USER_UPDATE, send_index );
	SetShort( send_buf, m_Sid, send_index );
	SetByte( send_buf, m_pUserData->m_bLevel, send_index );
	SetShort( send_buf, m_pUserData->m_sHp, send_index );
	SetShort( send_buf, m_pUserData->m_sMp, send_index );
	SetShort( send_buf, m_sTotalHit * m_bAttackAmount / 100 , send_index ); // g??
	SetShort( send_buf, m_sTotalAc + m_sACAmount, send_index );  // g??
	Setfloat( send_buf, m_sTotalHitrate, send_index );
	Setfloat( send_buf, m_sTotalEvasionrate, send_index );

//
	SetShort( send_buf, m_sItemAc, send_index );
	SetByte( send_buf, m_bMagicTypeLeftHand, send_index );
	SetByte( send_buf, m_bMagicTypeRightHand, send_index );
	SetShort( send_buf, m_sMagicAmountLeftHand, send_index );
	SetShort( send_buf, m_sMagicAmountRightHand, send_index );
//

	m_pMain->Send_AIServer(send_buf, send_index);
}

void CUser::SetUserAbility()
{
	_CLASS_COEFFICIENT* p_TableCoefficient = NULL;
	_ITEM_TABLE* pItem = NULL;
	BOOL bHaveBow = FALSE;
	p_TableCoefficient = m_pMain->m_CoefficientArray.GetData( m_pUserData->m_sClass );
	if( !p_TableCoefficient ) return;
	
	float hitcoefficient = 0.0f;
	if( m_pUserData->m_sItemArray[RIGHTHAND].nNum != 0 ) {
		pItem = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[RIGHTHAND].nNum );
		if( pItem ) {
			switch(pItem->m_bKind/10) {	// ???? �??....
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
				hitcoefficient = p_TableCoefficient->Club;
				break;
			case WEAPON_SPEAR:
				hitcoefficient = p_TableCoefficient->Spear;
				break;
			case WEAPON_SHIELD:
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
			case WEAPON_ARROW:
				break;
			case WEAPON_JAVELIN:
				break;
			case WEAPON_WORRIOR_AC:
				break;
			case WEAPON_LOG_AC:
				break;
			case WEAPON_WIZARD_AC:
				break;
			case WEAPON_PRIEST_AC:
				break;
			}
		}
	}
	if( m_pUserData->m_sItemArray[LEFTHAND].nNum != 0 && hitcoefficient == 0.0f ) {
		pItem = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[LEFTHAND].nNum );	// ??? ???? : ? ???? ???
		if( pItem ) {
			switch(pItem->m_bKind/10) {
			case WEAPON_BOW:
			case WEAPON_LONGBOW:
			case WEAPON_LAUNCHER:
				hitcoefficient = p_TableCoefficient->Bow;
				bHaveBow = TRUE;
				break;
			}
		}
	}

	int temp_str = 0, temp_dex = 0;

	temp_str = m_pUserData->m_bStr+m_bStrAmount+m_sItemStr;
//	if( temp_str > 255 ) temp_str = 255;

	temp_dex = m_pUserData->m_bDex+m_bDexAmount+m_sItemDex;
//	if( temp_dex > 255 ) temp_dex = 255;

	m_sBodyAc = m_pUserData->m_bLevel;
	m_sMaxWeight = (m_pUserData->m_bStr + m_sItemStr ) * 50;
	if( bHaveBow ) 
		m_sTotalHit = (short)((((0.005 * pItem->m_sDamage * (temp_dex + 40)) + ( hitcoefficient * pItem->m_sDamage * m_pUserData->m_bLevel * temp_dex )) + 3));
	else
		m_sTotalHit = (short)((((0.005f * m_sItemHit * (temp_str + 40)) + ( hitcoefficient * m_sItemHit * m_pUserData->m_bLevel * temp_str )) + 3)); 	

	m_sTotalAc = (short)(p_TableCoefficient->AC * (m_sBodyAc + m_sItemAc));
	m_sTotalHitrate = ((1 + p_TableCoefficient->Hitrate * m_pUserData->m_bLevel *  temp_dex ) * m_sItemHitrate/100 ) * (m_bHitRateAmount/100);

	m_sTotalEvasionrate = ((1 + p_TableCoefficient->Evasionrate * m_pUserData->m_bLevel * temp_dex ) * m_sItemEvasionrate/100) * (m_sAvoidRateAmount/100);

	SetMaxHp();
	SetMaxMp();
}

void CUser::SendTargetHP( BYTE echo, int tid, int damage )
{
	int send_index = 0, hp = 0, maxhp = 0;
	char buff[256];
	CUser* pTUser = NULL;
	CNpc* pNpc = NULL;

	if (tid < 0)
		return;

	if (tid >= NPC_BAND)
	{
		if (m_pMain->m_bPointCheckFlag == FALSE) return;
		pNpc = m_pMain->m_arNpcArray.GetData(tid);
		if (!pNpc)
			return;
		hp = pNpc->m_iHP;	maxhp = pNpc->m_iMaxHP;
	}
	else 
	{
		pTUser = m_pMain->GetUserPtr(tid);
		if( pTUser == NULL || pTUser->isDead()) 
			return;
		hp = pTUser->m_pUserData->m_sHp;	maxhp = pTUser->m_iMaxHp;
	}

	SetByte( buff, WIZ_TARGET_HP, send_index );
	SetShort( buff, tid, send_index );
	SetByte( buff, echo, send_index );
	SetDWORD( buff, maxhp, send_index );
	SetDWORD( buff, hp, send_index );
	SetShort( buff, damage, send_index );
	Send( buff, send_index );
}

void CUser::BundleOpenReq(char *pBuf)
{
	int index = 0, send_index = 0, bundle_index = 0;
	char send_buff[256];
	_ZONE_ITEM* pItem = NULL;
	C3DMap* pMap = GetMap();
	CRegion* pRegion = NULL;

	bundle_index = GetDWORD( pBuf, index );
	if (pMap == NULL
		|| bundle_index < 1 
		|| m_RegionX < 0 || m_RegionZ < 0 
		|| m_RegionX > pMap->GetXRegionMax() || m_RegionZ > pMap->GetZRegionMax())
		return;

	pRegion = &(pMap->m_ppRegion[m_RegionX][m_RegionZ]);
	if (pRegion == NULL)
		return;

	pItem = (_ZONE_ITEM*)pRegion->m_RegionItemArray.GetData( bundle_index );
	if (pItem == NULL)
		return;

	SetByte(send_buff, WIZ_BUNDLE_OPEN_REQ, send_index);
	for (int i = 0; i < 6; i++)
	{
		SetDWORD(send_buff, pItem->itemid[i], send_index);
		SetShort(send_buff, pItem->count[i], send_index);
	}
	Send(send_buff, send_index);
}

BOOL CUser::IsValidName(char *name)
{
	CString upperName = name;
	upperName.MakeUpper();

	foreach (itr, m_pMain->m_BlockNameArray)
		if (strstr(upperName, *itr))
			return FALSE;

	return TRUE;
}

void CUser::ItemGet(char *pBuf)
{
	int index = 0, send_index = 0, bundle_index = 0, itemid = 0, count = 0, i=0;
	BYTE pos;
	_ITEM_TABLE* pTable = NULL;
	char send_buff[256];
	_ZONE_ITEM* pItem = NULL;
	C3DMap* pMap = GetMap();
	CRegion* pRegion = NULL;
	CUser* pUser = NULL;
	CUser* pGetUser = NULL;
	_PARTY_GROUP* pParty = NULL;

	ASSERT(pMap != NULL);

	bundle_index = GetDWORD(pBuf, index);
	if (bundle_index < 1
		|| m_sExchangeUser != -1)
		goto fail_return;
	
	if (m_RegionX < 0 || m_RegionZ < 0 
		|| m_RegionX > pMap->GetXRegionMax() || m_RegionZ > pMap->GetZRegionMax())
		goto fail_return;

	pRegion = &(pMap->m_ppRegion[m_RegionX][m_RegionZ]);
	if (!pRegion)	goto fail_return;
	pItem = (_ZONE_ITEM*)pRegion->m_RegionItemArray.GetData( bundle_index );
	if(!pItem) goto fail_return;

	itemid = GetDWORD( pBuf, index );

	for(i=0; i<6; i++) {
		if( pItem->itemid[i] == itemid )
			break;
	}
	if(i == 6 ) goto fail_return;
	count = pItem->count[i];

	if( pMap->RegionItemRemove( m_RegionX, m_RegionZ, bundle_index, pItem->itemid[i], pItem->count[i] ) == FALSE )
		goto fail_return;
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
	if( !pTable ) goto fail_return;

	if( m_sPartyIndex != -1 && itemid != ITEM_GOLD ) 
		pGetUser = GetItemRoutingUser(itemid, count);
	else
		pGetUser = this;
		
	if( !pGetUser ) goto fail_return;
	pos = pGetUser->GetEmptySlot( itemid, pTable->m_bCountable );

	if( pos != 0xFF ) {	// Common Item
		if( pos >= HAVE_MAX ) goto fail_return;

		if( pGetUser->m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum != 0 ) {	
			if( pTable->m_bCountable != 1 ) goto fail_return;	
			else if( pGetUser->m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum != itemid ) goto fail_return;
		}
//
		if (pTable->m_bCountable) {	// Check weight of countable item.
			if ((pTable->m_sWeight * count + pGetUser->m_sItemWeight) > pGetUser->m_sMaxWeight) {			
				send_index = 0; 
				SetByte( send_buff, WIZ_ITEM_GET, send_index );
				SetByte( send_buff, 0x06, send_index );
				pGetUser->Send( send_buff, send_index );
				return;
			}
		}
		else {	// Check weight of non-countable item.
			if ((pTable->m_sWeight + pGetUser->m_sItemWeight) > pGetUser->m_sMaxWeight) {
				send_index = 0; 
				SetByte( send_buff, WIZ_ITEM_GET, send_index );
				SetByte( send_buff, 0x06, send_index );
				pGetUser->Send( send_buff, send_index );			
				return;
			}
		}
//
		pGetUser->m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum = itemid;	// Add item to inventory. 

		if( pTable->m_bCountable ) {	// Apply number of item.		
			pGetUser->m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount += count;
			if( pGetUser->m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount > MAX_ITEM_COUNT ) {
				pGetUser->m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount = MAX_ITEM_COUNT;
			}
		}
		else {
			pGetUser->m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount = 1;
			pGetUser->m_pUserData->m_sItemArray[SLOT_MAX+pos].nSerialNum = m_pMain->GenerateItemSerial();
		}

		pGetUser->SendItemWeight();
		pGetUser->m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration = pTable->m_sDuration;
		pGetUser->ItemLogToAgent( pGetUser->m_pUserData->m_id, "MONSTER", ITEM_MONSTER_GET, pGetUser->m_pUserData->m_sItemArray[SLOT_MAX+pos].nSerialNum, itemid, count, pTable->m_sDuration );
	}
	else {	// Gold
		if( itemid != ITEM_GOLD ) goto fail_return;
		if( count > 0 && count < 32767 ) {
			if( m_sPartyIndex == -1 ) {
				m_pUserData->m_iGold += count;
				SetByte( send_buff, WIZ_ITEM_GET, send_index );
				SetByte( send_buff, 0x01, send_index );
				SetByte( send_buff, pos, send_index );
				SetDWORD( send_buff, itemid, send_index );
				SetShort( send_buff, count, send_index );
				SetDWORD( send_buff, m_pUserData->m_iGold, send_index );
				Send( send_buff, send_index );
			}
			else {
				pParty = m_pMain->m_PartyArray.GetData( m_sPartyIndex );
				if( !pParty ) goto fail_return;
				int usercount = 0, money = 0, levelsum = 0;
				for( i=0; i<8; i++ ) {
					if( pParty->uid[i] != -1 ) {
						usercount++;
						levelsum += pParty->bLevel[i];
					}
				}
				if( usercount == 0 ) goto fail_return;
				for( i=0; i<8; i++ ) {
					if( pParty->uid[i] != -1 ) {
						pUser = m_pMain->GetUserPtr(pParty->uid[i]);
						if (pUser == NULL) continue;

						money = (int)(count * (float)(pUser->m_pUserData->m_bLevel / (float)levelsum));    
						pUser->m_pUserData->m_iGold += money;

						send_index = 0; 
						SetByte( send_buff, WIZ_ITEM_GET, send_index );
						SetByte( send_buff, 0x02, send_index );
						SetByte( send_buff, 0xff, send_index );			// gold -> pos : 0xff
						SetDWORD( send_buff, itemid, send_index );
						SetDWORD( send_buff, pUser->m_pUserData->m_iGold, send_index );
						pUser->Send( send_buff, send_index );
					}
				}
			}
		}
		return;
	}
	
	SetByte( send_buff, WIZ_ITEM_GET, send_index );
	if( pGetUser == this )
		SetByte( send_buff, 0x01, send_index );
	else
		SetByte( send_buff, 0x05, send_index );
	SetByte( send_buff, pos, send_index );
	SetDWORD( send_buff, itemid, send_index );
	SetShort( send_buff, pGetUser->m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount, send_index );
	SetDWORD( send_buff, pGetUser->m_pUserData->m_iGold, send_index );
	pGetUser->Send( send_buff, send_index );

	if( m_sPartyIndex != -1 ) {
		send_index = 0;
		SetByte( send_buff, WIZ_ITEM_GET, send_index );
		SetByte( send_buff, 0x03, send_index );
		SetDWORD( send_buff, itemid, send_index );
		SetKOString(send_buff, pGetUser->m_pUserData->m_id, send_index);
		m_pMain->Send_PartyMember( m_sPartyIndex, send_buff, send_index );
		if( pGetUser != this ) {
			send_index = 0;
			SetByte( send_buff, WIZ_ITEM_GET, send_index );
			SetByte( send_buff, 0x04, send_index );
			Send( send_buff, send_index );
		}
	} 

	return;

fail_return:
	send_index = 0;
	SetByte( send_buff, WIZ_ITEM_GET, send_index );
	SetByte( send_buff, 0x00, send_index );
	Send( send_buff, send_index );
}

void CUser::StateChange(char *pBuf)
{
	int index = 0, send_index = 0;

	BYTE type = 0x00;
	BYTE buff = 0x00;

	char send_buff[128];

	type = GetByte( pBuf, index );
	buff = GetByte( pBuf, index );

	if( type > 5 ) return;
	if( type == 5 && m_pUserData->m_bAuthority != 0) return;	//  Operators only!!!
	
	if (type == 1) {
		m_bResHpType = buff;
	}
	else if (type == 2) {
		m_bNeedParty = buff;
	}
	else if (type == 3) {
		m_bAbnormalType = buff;
	}

	SetByte( send_buff, WIZ_STATE_CHANGE, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetByte( send_buff, type, send_index );
	if (type == 1) {
		SetByte( send_buff, m_bResHpType, send_index );
	}
	else if (type == 2) {
		SetByte( send_buff, m_bNeedParty, send_index );
	}
//
	else if (type == 3) {
		SetDWORD( send_buff, m_bAbnormalType, send_index ); // NOTE: This packet needs to be explicitly updated.
	}	
//
	else {		// Just plain echo :)
		SetByte( send_buff, buff, send_index );
//		N3_SP_STATE_CHANGE_ACTION = 0x04,			// 1 - ???, 11 - ????
//		N3_SP_STATE_CHANGE_VISIBLE = 0x05 };		// ??? 0 ~ 255
	}

	m_pMain->Send_Region( send_buff, send_index, GetMap(), m_RegionX, m_RegionZ );
}

void CUser::StateChangeServerDirect(BYTE bType, int nValue)
{
	char buff[5];
	int index = 0;

	SetByte(buff, bType, index);
	SetDWORD(buff, nValue, index);
	StateChange(buff);
}

void CUser::LoyaltyChange(short tid)
{
	int send_index = 0; char send_buff[256];
	short level_difference = 0, loyalty_source = 0, loyalty_target = 0;

	CUser* pTUser = m_pMain->GetUserPtr(tid);     // Get target info.  
	if (pTUser == NULL) return;									  // Check if target exists and not already dead.
	
	if (pTUser->getNation() != getNation()) {		// Different nations!!!
		level_difference = pTUser->m_pUserData->m_bLevel - m_pUserData->m_bLevel ;	// Calculate difference!

		if (pTUser->m_pUserData->m_iLoyalty <= 0) {		// No cheats allowed...
			loyalty_source = 0;
			loyalty_target = 0;
		}
		else if (level_difference > 5) {	// Target at least six levels lower...
			loyalty_source = 50;
			loyalty_target = -25;
		}
		else if (level_difference < -5) {	// Target at least six levels higher...
			loyalty_source =  10; 
			loyalty_target = -5;
		}
		else {								// Target within the 5 and -5 range...
			loyalty_source =  30;
			loyalty_target = -15;
		}
	}
	else {		// Same Nations!!!
		if (pTUser->m_pUserData->m_iLoyalty >= 0) {		
			loyalty_source  = -1000;
			loyalty_target = -15;			
		}
		else {
			loyalty_source  =  100;
			loyalty_target = -15;
		}
	}
//
	if (m_pUserData->m_bZone != m_pUserData->m_bNation && m_pUserData->m_bZone < 3) { 
		loyalty_source  = 2 * loyalty_source;
	}
//

	//TRACE("LoyaltyChange 222 - user1=%s, %d,, user2=%s, %d\n", m_pUserData->m_id,  m_pUserData->m_iLoyalty, pTUser->m_pUserData->m_id, pTUser->m_pUserData->m_iLoyalty);
	
	m_pUserData->m_iLoyalty += loyalty_source;			// Recalculations of Loyalty...
	pTUser->m_pUserData->m_iLoyalty += loyalty_target;  

	if (m_pUserData->m_iLoyalty < 0) m_pUserData->m_iLoyalty = 0;	// Cannot be less than zero.
	if (pTUser->m_pUserData->m_iLoyalty < 0) pTUser->m_pUserData->m_iLoyalty = 0;

	//TRACE("LoyaltyChange 222 - user1=%s, %d,, user2=%s, %d\n", m_pUserData->m_id,  m_pUserData->m_iLoyalty, pTUser->m_pUserData->m_id, pTUser->m_pUserData->m_iLoyalty);

	SetByte( send_buff, WIZ_LOYALTY_CHANGE, send_index );	// Send result to source.
	SetDWORD( send_buff, m_pUserData->m_iLoyalty, send_index );
	Send( send_buff, send_index );

	send_index = 0;		// Send result to target.
	SetByte( send_buff, WIZ_LOYALTY_CHANGE, send_index );
	SetDWORD( send_buff, pTUser->m_pUserData->m_iLoyalty, send_index );
	pTUser->Send( send_buff, send_index );

//	This is for the Event Battle on Wednesday :(
	if (m_pMain->m_byBattleOpen) {
		if (m_pUserData->m_bZone == ZONE_BATTLE ) { // || m_pUserData->m_bZone == ZONE_SNOW_BATTLE) {
			if (pTUser->m_pUserData->m_bNation == KARUS) {
				m_pMain->m_sKarusDead++;
				//TRACE("++ LoyaltyChange - ka=%d, el=%d\n", m_pMain->m_sKarusDead, m_pMain->m_sElmoradDead);
			}
			else if (pTUser->m_pUserData->m_bNation == ELMORAD) {
				m_pMain->m_sElmoradDead++;
				//TRACE("++ LoyaltyChange - ka=%d, el=%d\n", m_pMain->m_sKarusDead, m_pMain->m_sElmoradDead);
			}
		}
	}
//
}

void CUser::SpeedHackUser()
{
	if (GetState() != STATE_GAMESTART)
		return;

	m_pMain->WriteLog("%s Speed Hack Used\r\n", m_pUserData->m_id);
	
	if( m_pUserData->m_bAuthority != 0 )
		m_pUserData->m_bAuthority = -1;

	Close();
}

void CUser::UserLookChange(int pos, int itemid, int durability)
{
	int send_index = 0;
	char send_buff[256];

	if( pos >= SLOT_MAX )
		return;

	SetByte( send_buff, WIZ_USERLOOK_CHANGE, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetByte( send_buff, (BYTE)pos, send_index );
	SetDWORD( send_buff, itemid, send_index );
	SetShort( send_buff, durability, send_index );
	m_pMain->Send_Region( send_buff, send_index, GetMap(), m_RegionX, m_RegionZ, this );
}

void CUser::SendNotice()
{
	int send_index = 0, count = 0;
	char send_buff[2048];

	SetByte( send_buff, WIZ_NOTICE, send_index );
#if __VERSION < 1453
	char buff[1024]; int buff_index = 0;
	for( int i=0; i<20; i++ ) {
		if (m_pMain->m_ppNotice[i][0] == 0)
			continue;

		SetKOString(buff, m_pMain->m_ppNotice[i], buff_index, 1);
		count++;
	}
	SetByte( send_buff, count, send_index );
	SetString( send_buff, buff, buff_index, send_index );*/
#else
	SetByte(send_buff, 2, send_index); // type 2 = new-style notices

	// hardcoded temporarily
	SetByte(send_buff, 3, send_index); // 3 boxes

	SetKOString(send_buff, "Header 1", send_index);
	SetKOString(send_buff, "Data in header 1", send_index);

	SetKOString(send_buff, "Header 2", send_index);
	SetKOString(send_buff, "Data in header 2", send_index);

	SetKOString(send_buff, "Header 3", send_index);
	SetKOString(send_buff, "Data in header 3", send_index);
#endif
	
	Send( send_buff, send_index );
}

void CUser::SkillPointChange(char *pBuf)
{
	int index = 0, send_index = 0, value = 0;
	BYTE type = 0x00;
	char send_buff[128];

	type = GetByte( pBuf, index );
	if( type > 0x08 ) goto fail_return;
	if( m_pUserData->m_bstrSkill[0] < 1 ) goto fail_return;
	if( (m_pUserData->m_bstrSkill[type]+1) > m_pUserData->m_bLevel ) goto fail_return;

	m_pUserData->m_bstrSkill[0] -= 1;
	m_pUserData->m_bstrSkill[type] += 1;

	return;

fail_return:
	SetByte( send_buff, WIZ_SKILLPT_CHANGE, send_index );
	SetByte( send_buff, type, send_index );
	SetByte( send_buff, m_pUserData->m_bstrSkill[type], send_index );
	Send( send_buff, send_index );
}

void CUser::UpdateGameWeather(char *pBuf, BYTE type)
{
	int index = 0, send_index = 0, year = 0, month = 0, date = 0;
	char send_buff[128];

	if( m_pUserData->m_bAuthority != 0 )	// is this user administrator?
		return;

	if( type == WIZ_WEATHER ) {
		m_pMain->m_nWeather = GetByte( pBuf, index );
		m_pMain->m_nAmount = GetShort( pBuf, index );

		SetByte( send_buff, WIZ_WEATHER, send_index );
		SetByte( send_buff, (BYTE)m_pMain->m_nWeather, send_index );
		SetShort( send_buff, m_pMain->m_nAmount, send_index );
		m_pMain->Send_All( send_buff, send_index );
	}
	else if( type == WIZ_TIME ) {
		year = GetShort( pBuf, index );
		month = GetShort( pBuf, index );
		date = GetShort( pBuf, index );
		m_pMain->m_nHour = GetShort( pBuf, index );
		m_pMain->m_nMin = GetShort( pBuf, index );

		SetByte( send_buff, WIZ_TIME, send_index );
		SetShort( send_buff, year, send_index );
		SetShort( send_buff, month, send_index );
		SetShort( send_buff, date, send_index );
		SetShort( send_buff, m_pMain->m_nHour, send_index );
		SetShort( send_buff, m_pMain->m_nMin, send_index );
		m_pMain->Send_All( send_buff, send_index );
	}
}

void CUser::SendUserInfo(char *temp_send, int &index)
{
	SetShort( temp_send, m_Sid, index );
	SetKOString(temp_send, m_pUserData->m_id, index);
	SetByte( temp_send, m_pUserData->m_bZone, index );
	SetByte( temp_send, m_pUserData->m_bNation, index );
	SetByte( temp_send, m_pUserData->m_bLevel, index );
	SetShort( temp_send, m_pUserData->m_sHp, index );
	SetShort( temp_send, m_pUserData->m_sMp, index );
	SetShort( temp_send, m_sTotalHit * m_bAttackAmount / 100, index );    // g??
	SetShort( temp_send, m_sTotalAc + m_sACAmount , index );	// g??
	Setfloat( temp_send, m_sTotalHitrate, index );
	Setfloat( temp_send, m_sTotalEvasionrate, index );
	SetShort( temp_send, m_sPartyIndex, index );
	SetByte( temp_send, m_pUserData->m_bAuthority, index );
}

void CUser::CountConcurrentUser()
{
	if( m_pUserData->m_bAuthority != 0 )
		return;
	int usercount = 0, send_index = 0;
	char send_buff[128];
	CUser* pUser = NULL;

	for(int i=0; i<MAX_USER; i++ ) {
		pUser = m_pMain->GetUnsafeUserPtr(i);
		if (pUser != NULL && pUser->GetState() == STATE_GAMESTART)
			usercount++;
	}

	SetByte( send_buff, WIZ_CONCURRENTUSER, send_index );
	SetShort( send_buff, usercount, send_index );
	Send( send_buff, send_index );
}

void CUser::LoyaltyDivide(short tid)
{
	int send_index = 0, levelsum = 0, individualvalue = 0;
	char send_buff[256];
	short temp_loyalty = 0, level_difference = 0, loyalty_source = 0, loyalty_target = 0, average_level = 0; 
	BYTE total_member = 0;

	CUser* pUser = NULL;

	_PARTY_GROUP* pParty = NULL;		// Party Pointer Initialization!
	if( m_sPartyIndex < 0 ) return;
	pParty = m_pMain->m_PartyArray.GetData( m_sPartyIndex );
	if( !pParty ) return;

	CUser* pTUser = NULL ;									  // Target Pointer initialization!		
	pTUser = m_pMain->GetUserPtr(tid);
	if (pTUser == NULL) return;									  // Check if target exists and not already dead.		

	for( int i = 0; i < 8; i++ ) {		// Get total level and number of members in party.
		if( pParty->uid[i] != -1 ) {
			levelsum += pParty->bLevel[i];
			total_member ++;			
		}
	}

	if (levelsum <= 0) return;		// Protection codes.
	if (total_member <= 0) return;

	average_level = levelsum / total_member;	// Calculate average level.

	//	This is for the Event Battle on Wednesday :(
	if (m_pMain->m_byBattleOpen) {
		if (m_pUserData->m_bZone == ZONE_BATTLE) {
			if (pTUser->m_pUserData->m_bNation == KARUS) {
				m_pMain->m_sKarusDead++;
				//TRACE("++ LoyaltyDivide - ka=%d, el=%d\n", m_pMain->m_sKarusDead, m_pMain->m_sElmoradDead);
			}
			else if (pTUser->m_pUserData->m_bNation == ELMORAD) {
				m_pMain->m_sElmoradDead++;
				//TRACE("++ LoyaltyDivide - ka=%d, el=%d\n", m_pMain->m_sKarusDead, m_pMain->m_sElmoradDead);
			}
		}
	}
		
	if (pTUser->m_pUserData->m_bNation != m_pUserData->m_bNation) {		// Different nations!!!
		level_difference = pTUser->m_pUserData->m_bLevel - average_level;	// Calculate difference!

		if (pTUser->m_pUserData->m_iLoyalty <= 0) {	   // No cheats allowed...
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

		for (int j = 0 ; j < 8 ; j++) {		// Distribute loyalty amongst party members.
			if( pParty->uid[j] != -1 || pParty->uid[j] >= MAX_USER ) {
				pUser = m_pMain->GetUserPtr(pParty->uid[j]);
				if (pUser == NULL) continue;

				//TRACE("LoyaltyDivide 111 - user1=%s, %d\n", pUser->m_pUserData->m_id, pUser->m_pUserData->m_iLoyalty);
			
				pUser->m_pUserData->m_iLoyalty += individualvalue;	
				if (pUser->m_pUserData->m_iLoyalty < 0) pUser->m_pUserData->m_iLoyalty = 0;	// Cannot be less than zero.

				//TRACE("LoyaltyDivide 222 - user1=%s, %d\n", pUser->m_pUserData->m_id, pUser->m_pUserData->m_iLoyalty);

				send_index = 0;	
				SetByte( send_buff, WIZ_LOYALTY_CHANGE, send_index );	// Send result to source.
				SetDWORD( send_buff, pUser->m_pUserData->m_iLoyalty, send_index );
				pUser->Send( send_buff, send_index );			
			}
		}		
		
		return;
	}
//
	if (m_pUserData->m_bZone != m_pUserData->m_bNation && m_pUserData->m_bZone < 3) { 
		loyalty_source  = 2 * loyalty_source;
	}
//
	for (int j = 0 ; j < 8 ; j++) {		// Distribute loyalty amongst party members.
		if( pParty->uid[j] != -1 || pParty->uid[j] >= MAX_USER ) {
			pUser = m_pMain->GetUserPtr(pParty->uid[j]);
			if (pUser == NULL) continue;

			//TRACE("LoyaltyDivide 333 - user1=%s, %d\n", pUser->m_pUserData->m_id, pUser->m_pUserData->m_iLoyalty);
			individualvalue = pUser->m_pUserData->m_bLevel * loyalty_source / levelsum ;
			pUser->m_pUserData->m_iLoyalty += individualvalue;	
			if (pUser->m_pUserData->m_iLoyalty < 0) pUser->m_pUserData->m_iLoyalty = 0;

			//TRACE("LoyaltyDivide 444 - user1=%s, %d\n", pUser->m_pUserData->m_id, pUser->m_pUserData->m_iLoyalty);

			send_index = 0;	
			SetByte( send_buff, WIZ_LOYALTY_CHANGE, send_index );	// Send result to source.
			SetDWORD( send_buff, pUser->m_pUserData->m_iLoyalty, send_index );
			pUser->Send( send_buff, send_index );

			individualvalue = 0;
		}
	}

	pTUser->m_pUserData->m_iLoyalty += loyalty_target;	// Recalculate target loyalty.
	if (pTUser->m_pUserData->m_iLoyalty < 0) pTUser->m_pUserData->m_iLoyalty = 0;

	//TRACE("LoyaltyDivide 555 - user1=%s, %d\n", pTUser->m_pUserData->m_id, pTUser->m_pUserData->m_iLoyalty);
	
	send_index = 0;		// Send result to target.
	SetByte( send_buff, WIZ_LOYALTY_CHANGE, send_index );
	SetDWORD( send_buff, pTUser->m_pUserData->m_iLoyalty, send_index );
	pTUser->Send( send_buff, send_index );
}

void CUser::Dead()
{
	int send_index = 0;
	char chatstr[1024], finalstr[1024], send_buff[1024], strKnightsName[MAX_ID_SIZE+1];	
	CKnights* pKnights = NULL;

	SetByte( send_buff, WIZ_DEAD, send_index );
	SetShort( send_buff, m_Sid, send_index );
	m_pMain->Send_Region( send_buff, send_index, GetMap(), m_RegionX, m_RegionZ );

	m_bResHpType = USER_DEAD;

	Send( send_buff, send_index );		// ????? ??? ???? ??Y? ????... (?? ?? ?? ????, ???? ???? ????)

	DEBUG_LOG("----> User Dead ,, nid=%d, name=%s, type=%d, x=%d, z=%d ******", m_Sid, m_pUserData->m_id, m_bResHpType, (int)m_pUserData->m_curx, (int)m_pUserData->m_curz);

	send_index = 0;
	if( m_pUserData->m_bFame == COMMAND_CAPTAIN )	{	// ????????? ??? ??? ??�??,, ???? ???? ??Z
		m_pUserData->m_bFame = CHIEF;
		SetByte( send_buff, WIZ_AUTHORITY_CHANGE, send_index );
		SetByte( send_buff, COMMAND_AUTHORITY, send_index );
		SetShort( send_buff, GetSocketID(), send_index );
		SetByte( send_buff, m_pUserData->m_bFame, send_index );
		m_pMain->Send_Region( send_buff, send_index, GetMap(), m_RegionX, m_RegionZ );
		Send( send_buff, send_index );

		pKnights = m_pMain->m_KnightsArray.GetData( m_pUserData->m_bKnights );
		if( pKnights )		strcpy_s( strKnightsName, sizeof(strKnightsName), pKnights->m_strName );
		else				strcpy_s( strKnightsName, sizeof(strKnightsName), "*" );
		//TRACE("---> Dead Captain Deprive - %s\n", m_pUserData->m_id);
		if( m_pUserData->m_bNation == KARUS )	{
			sprintf( chatstr, m_pMain->GetServerResource(IDS_KARUS_CAPTAIN_DEPRIVE), strKnightsName, m_pUserData->m_id );
		}
		else if( m_pUserData->m_bNation == ELMORAD )	{
			sprintf( chatstr, m_pMain->GetServerResource(IDS_ELMO_CAPTAIN_DEPRIVE), strKnightsName, m_pUserData->m_id );
		}

		send_index = 0;
		sprintf( finalstr, m_pMain->GetServerResource(IDP_ANNOUNCEMENT), chatstr );
		SetByte( send_buff, WIZ_CHAT, send_index );
		SetByte( send_buff, WAR_SYSTEM_CHAT, send_index );
		SetByte( send_buff, 1, send_index );
		SetShort( send_buff, -1, send_index );
		SetKOString(send_buff, finalstr, send_index);
		m_pMain->Send_All( send_buff, send_index, NULL, m_pUserData->m_bNation );
	}
}

void CUser::ItemWoreOut(int type, int damage)
{
	_ITEM_TABLE* pTable = NULL;
	int worerate = (int)sqrt(damage / 10.0f);
	if( worerate == 0 ) return;

	if( type == ATTACK ) {
		if( m_pUserData->m_sItemArray[RIGHTHAND].nNum != 0 ) {
			pTable = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[RIGHTHAND].nNum );
			if( pTable ) {
				if( pTable->m_bSlot != 2 )	{// 2 == DEFENCE ITEM
					if( m_pUserData->m_sItemArray[RIGHTHAND].sDuration != 0 ) {
						m_pUserData->m_sItemArray[RIGHTHAND].sDuration -= worerate;
						ItemDurationChange( RIGHTHAND, pTable->m_sDuration, m_pUserData->m_sItemArray[RIGHTHAND].sDuration, worerate );
					}
				}
			}
		}
		if( m_pUserData->m_sItemArray[LEFTHAND].nNum != 0 ) {
			pTable = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[LEFTHAND].nNum );
			if( pTable ) {
				if( pTable->m_bSlot != 2 ) {
					if( m_pUserData->m_sItemArray[LEFTHAND].sDuration != 0 ) {
						m_pUserData->m_sItemArray[LEFTHAND].sDuration -= worerate;
						ItemDurationChange( LEFTHAND, pTable->m_sDuration, m_pUserData->m_sItemArray[LEFTHAND].sDuration, worerate );
					}
				}
			}
		}
	}
	else if ( type == DEFENCE ) {
		if( m_pUserData->m_sItemArray[HEAD].nNum != 0 ) {
			pTable = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[HEAD].nNum );
			if( pTable ) {
				if( m_pUserData->m_sItemArray[HEAD].sDuration != 0 ) {
					m_pUserData->m_sItemArray[HEAD].sDuration -= worerate;
					ItemDurationChange( HEAD, pTable->m_sDuration, m_pUserData->m_sItemArray[HEAD].sDuration, worerate );
				}
			}
		}
		if( m_pUserData->m_sItemArray[BREAST].nNum != 0 ) {
			pTable = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[BREAST].nNum );
			if( pTable ) {
				if( m_pUserData->m_sItemArray[BREAST].sDuration != 0 ) {
					m_pUserData->m_sItemArray[BREAST].sDuration -= worerate;
					ItemDurationChange( BREAST, pTable->m_sDuration, m_pUserData->m_sItemArray[BREAST].sDuration, worerate );
				}
			}
		}
		if( m_pUserData->m_sItemArray[LEG].nNum != 0 ) {
			pTable = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[LEG].nNum );
			if( pTable ) {
				if( m_pUserData->m_sItemArray[LEG].sDuration != 0 ) {
					m_pUserData->m_sItemArray[LEG].sDuration -= worerate;
					ItemDurationChange( LEG, pTable->m_sDuration, m_pUserData->m_sItemArray[LEG].sDuration, worerate );
				}
			}
		}
		if( m_pUserData->m_sItemArray[GLOVE].nNum != 0 ) {
			pTable = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[GLOVE].nNum );
			if( pTable ) {
				if( m_pUserData->m_sItemArray[GLOVE].sDuration != 0 ) {
					m_pUserData->m_sItemArray[GLOVE].sDuration -= worerate;
					ItemDurationChange( GLOVE, pTable->m_sDuration, m_pUserData->m_sItemArray[GLOVE].sDuration, worerate );
				}
			}
		}
		if( m_pUserData->m_sItemArray[FOOT].nNum != 0 ) {
			pTable = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[FOOT].nNum );
			if( pTable ) {
				if( m_pUserData->m_sItemArray[FOOT].sDuration != 0 ) {
					m_pUserData->m_sItemArray[FOOT].sDuration -= worerate;
					ItemDurationChange( FOOT, pTable->m_sDuration, m_pUserData->m_sItemArray[FOOT].sDuration, worerate );
				}
			}
		}
		if( m_pUserData->m_sItemArray[RIGHTHAND].nNum != 0 ) {
			pTable = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[RIGHTHAND].nNum );
			if( pTable ) {
				if( pTable->m_bSlot == 2 ) {	// ?????
					if( m_pUserData->m_sItemArray[RIGHTHAND].sDuration != 0 ) {
						m_pUserData->m_sItemArray[RIGHTHAND].sDuration -= worerate;
						ItemDurationChange( RIGHTHAND, pTable->m_sDuration, m_pUserData->m_sItemArray[RIGHTHAND].sDuration, worerate );
					}
				}
			}
		}
		if( m_pUserData->m_sItemArray[LEFTHAND].nNum != 0 ) {
			pTable = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[LEFTHAND].nNum );
			if( pTable ) {
				if( pTable->m_bSlot == 2 ) {	// ?????
					if( m_pUserData->m_sItemArray[LEFTHAND].sDuration != 0 ) {
						m_pUserData->m_sItemArray[LEFTHAND].sDuration -= worerate;
						ItemDurationChange( LEFTHAND, pTable->m_sDuration, m_pUserData->m_sItemArray[LEFTHAND].sDuration, worerate );
					}
				}
			}
		}
	}
}

void CUser::ItemDurationChange(int slot, int maxvalue, int curvalue, int amount)
{
	if( maxvalue <= 0 ) return;
	if( slot < 0 || slot > SLOT_MAX ) return;
	
	int curpercent = 0, beforepercent = 0, curbasis = 0, beforebasis = 0;
	int send_index = 0;
	char send_buff[128];
	
	if( m_pUserData->m_sItemArray[slot].sDuration <= 0 ) {
		m_pUserData->m_sItemArray[slot].sDuration = 0;
		SetByte( send_buff, WIZ_DURATION, send_index );
		SetByte( send_buff, slot, send_index );
		SetShort( send_buff, 0, send_index );
		Send( send_buff, send_index );
		
		SetSlotItemValue();
		SetUserAbility();

		send_index = 0;
		SetByte( send_buff, WIZ_ITEM_MOVE, send_index );
		SetByte( send_buff, 0x01, send_index );
		SetShort( send_buff, m_sTotalHit, send_index );
		SetShort( send_buff, m_sTotalAc, send_index );
		SetShort( send_buff, m_sItemWeight, send_index );
		SetShort( send_buff, m_iMaxHp, send_index );
		SetShort( send_buff, m_iMaxMp, send_index );
		SetByte( send_buff, (BYTE)m_sItemStr, send_index );
		SetByte( send_buff, (BYTE)m_sItemSta, send_index );
		SetByte( send_buff, (BYTE)m_sItemDex, send_index );
		SetByte( send_buff, (BYTE)m_sItemIntel, send_index );
		SetByte( send_buff, (BYTE)m_sItemCham, send_index );
		SetByte( send_buff, m_bFireR, send_index );
		SetByte( send_buff, m_bColdR, send_index );
		SetByte( send_buff, m_bLightningR, send_index );
		SetByte( send_buff, m_bMagicR, send_index );
		SetByte( send_buff, m_bDiseaseR, send_index );
		SetByte( send_buff, m_bPoisonR, send_index );
		Send( send_buff, send_index );
		return;
	}
	curpercent = (int)((curvalue / (double)maxvalue) * 100);
	beforepercent = (int)(((curvalue+amount) / (double)maxvalue ) * 100);
	
	curbasis = curpercent / 5;
	beforebasis = beforepercent / 5;

	if( curbasis != beforebasis ) {
		SetByte( send_buff, WIZ_DURATION, send_index );
		SetByte( send_buff, slot, send_index );
		SetShort( send_buff, curvalue, send_index );
		Send( send_buff, send_index );

		if( curpercent >= 65 && curpercent < 70 )
			UserLookChange( slot, m_pUserData->m_sItemArray[slot].nNum, curvalue );
		if( curpercent >= 25 && curpercent < 30 )
			UserLookChange( slot, m_pUserData->m_sItemArray[slot].nNum, curvalue );
	}
}

void CUser::HPTimeChange(float currenttime)
{
	BOOL bFlag = FALSE;

	m_fHPLastTimeNormal = currenttime;

	if( m_bResHpType == USER_DEAD ) return;

	if( m_pUserData->m_bZone == ZONE_SNOW_BATTLE && m_pMain->m_byBattleOpen == SNOW_BATTLE )	{
		if( m_pUserData->m_sHp < 1 ) return;
		HpChange( 5 );
		return;
	}

	if( m_bResHpType == USER_STANDING ) {
		if( m_pUserData->m_sHp < 1 ) return;
		if( m_iMaxHp != m_pUserData->m_sHp )
			HpChange( (int)((m_pUserData->m_bLevel*(1+m_pUserData->m_bLevel/60.0) + 1)*0.2)+3 );

		if( m_iMaxMp != m_pUserData->m_sMp )
			MSpChange( (int)((m_pUserData->m_bLevel*(1+m_pUserData->m_bLevel/60.0) + 1)*0.2)+3 );
	}
	else if ( m_bResHpType == USER_SITDOWN ) {
		if( m_pUserData->m_sHp < 1 ) return;
		if( m_iMaxHp != m_pUserData->m_sHp ) {
			HpChange( (int)(m_pUserData->m_bLevel*(1+m_pUserData->m_bLevel/30.0) ) + 3 );
		}
		if( m_iMaxMp != m_pUserData->m_sMp ) {
			MSpChange((int)((m_iMaxMp * 5) / ((m_pUserData->m_bLevel - 1) + 30 )) + 3 ) ;
		}
	}
}

void CUser::HPTimeChangeType3(float currenttime)
{
	int send_index = 0;
	char send_buff[128];

	for (int g = 0 ; g < MAX_TYPE3_REPEAT ; g++) {	// Get the current time for all the last times...
		m_fHPLastTime[g] = currenttime;
	}

	if( m_bResHpType != USER_DEAD ) {	// Make sure the user is not dead first!!!
		for (int h = 0 ; h < MAX_TYPE3_REPEAT ; h++) {
			HpChange(m_bHPAmount[h]);	// Reduce HP...
			CUser* pUser = NULL;

			if (m_sSourceID[h] >= 0 && m_sSourceID[h] < MAX_USER) {	// Send report to the source...
				pUser = m_pMain->GetUserPtr(m_sSourceID[h]);
				if (pUser) {
					pUser->SendTargetHP( 0, m_Sid, m_bHPAmount[h] );
				}
			}	// ...End of : Send report to the source...

			if ( m_pUserData->m_sHp == 0) {     // Check if the target is dead.	
				// sungyong work : loyalty
				m_bResHpType = USER_DEAD;	// Officially declare the user DEAD!!!!!

				if (m_sSourceID[h] >= NPC_BAND) {	// If the killer was a NPC
//
					if( m_pUserData->m_bZone != m_pUserData->m_bNation && m_pUserData->m_bZone < 3) {
						ExpChange(-m_iMaxExp / 100);
						//TRACE("????? 1%?? ??????? ??.??\r\n");
					}
					else {
//
						ExpChange(-m_iMaxExp/20 );     // Reduce target experience.		
//
					}
//
				}
				else {	// You got killed by another player
					if (pUser) {	// (No more pointer mistakes....)
						if( pUser->m_sPartyIndex == -1 ) {     // Something regarding loyalty points.
							pUser->LoyaltyChange(m_Sid);
						}
						else {
							pUser->LoyaltyDivide(m_Sid);
						}
						
						pUser->GoldChange(m_Sid, 0);
					}
				}				
				// ??????? ????? ??? ???!!!
				InitType3();	// Init Type 3.....
				InitType4();	// Init Type 4.....
				
				if (m_sSourceID[h] >= 0 && m_sSourceID[h] < MAX_USER) {
					m_sWhoKilledMe = m_sSourceID[h];	// Who the hell killed me?
//
					if( m_pUserData->m_bZone != m_pUserData->m_bNation && m_pUserData->m_bZone < 3) {
						ExpChange(-m_iMaxExp / 100);
						//TRACE("????? 1%?? ??????? ??.??\r\n");
					}
//
				}
				
				break;	// Exit the for loop :)
			}	// ...End of : Check if the target is dead.
		}
	}
	else return;

	for (int i = 0 ; i < MAX_TYPE3_REPEAT ; i++) {	// Type 3 Cancellation Process.
		if( m_bHPDuration[i] > 0 ) {
			if( ((currenttime - m_fHPStartTime[i]) >= m_bHPDuration[i]) || m_bResHpType == USER_DEAD) {
				/*	Send Party Packet.....
				if (m_sPartyIndex != -1) {
					SetByte( send_buff, WIZ_PARTY, send_index );
					SetByte( send_buff, PARTY_STATUSCHANGE, send_index );
					SetShort( send_buff, m_Sid, send_index );
					SetByte( send_buff, 1, send_index );
					SetByte( send_buff, 0x00, send_index);
					m_pMain->Send_PartyMember(m_sPartyIndex, send_buff, send_index);
					send_index = 0 ;
				}
				//  end of Send Party Packet.....*/ 

				SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
				SetByte( send_buff, MAGIC_TYPE3_END, send_index );	

				if (m_bHPAmount[i] > 0) {
					SetByte( send_buff, 100, send_index );
				}
				else {
					SetByte( send_buff, 200, send_index );
				}

				Send( send_buff, send_index ); 
				send_index = 0;
				
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
	for (int j = 0 ; j < MAX_TYPE3_REPEAT ; j++) {
		buff_test += m_bHPDuration[j];
	}
	if (buff_test == 0) m_bType3Flag = FALSE;
//
	BOOL bType3Test = TRUE;
	for (int k = 0 ; k < MAX_TYPE3_REPEAT ; k++) {
		if (m_bHPAmount[k] < 0) {
			bType3Test = FALSE;
			break;
		}
	}

	// Send Party Packet.....
	if (m_sPartyIndex != -1 && bType3Test) {
		send_index = 0;
		SetByte( send_buff, WIZ_PARTY, send_index );
		SetByte( send_buff, PARTY_STATUSCHANGE, send_index );
		SetShort( send_buff, m_Sid, send_index );
		SetByte( send_buff, 1, send_index );
		SetByte( send_buff, 0x00, send_index);
		m_pMain->Send_PartyMember(m_sPartyIndex, send_buff, send_index);
	}
	//  end of Send Party Packet.....  //
//
}

void CUser::Type4Duration(float currenttime)
{
	int send_index = 0;
	char send_buff[128];
	BYTE buff_type = 0;					

	if (m_sDuration1 && buff_type == 0) {
		if (currenttime > (m_fStartTime1 + m_sDuration1)) {
			m_sDuration1 = 0;		
			m_fStartTime1 = 0.0f;
			m_sMaxHPAmount = 0;
			buff_type = 1 ;			
		}
	}

	if (m_sDuration2 && buff_type == 0) {
		if (currenttime > (m_fStartTime2 + m_sDuration2)) {
			m_sDuration2 = 0;		
			m_fStartTime2 = 0.0f;
			m_sACAmount = 0;
			buff_type = 2 ;
		}
	}
//
	if (m_sDuration3 && buff_type == 0) {
		if (currenttime > (m_fStartTime3 + m_sDuration3)) {
			m_sDuration3 = 0;		
			m_fStartTime3 = 0.0f;
			buff_type = 3 ;
			
			send_index = 0 ;
			SetByte(send_buff, 3, send_index);	// You are now normal again!!!
			SetByte(send_buff, ABNORMAL_NORMAL, send_index);
			StateChange(send_buff);					
			send_index = 0 ;
		}
	}
//
	if (m_sDuration4 && buff_type == 0) {
		if (currenttime > (m_fStartTime4 + m_sDuration4)){
			m_sDuration4 = 0;		
			m_fStartTime4 = 0.0f;
			m_bAttackAmount = 100;
			buff_type = 4 ;
		}
	}

	if (m_sDuration5 && buff_type == 0) {
		if (currenttime > (m_fStartTime5 + m_sDuration5)){
			m_sDuration5 = 0;		
			m_fStartTime5 = 0.0f;
			m_bAttackSpeedAmount = 100;	
			buff_type = 5 ;
		}
	}

	if (m_sDuration6 && buff_type == 0) {
		if (currenttime > (m_fStartTime6 + m_sDuration6)){
			m_sDuration6 = 0;		
			m_fStartTime6 = 0.0f;
			m_bSpeedAmount = 100;
			buff_type = 6 ;
		}
	}

	if (m_sDuration7 && buff_type == 0) {
		if (currenttime > (m_fStartTime7 + m_sDuration7)){
			m_sDuration7 = 0;		
			m_fStartTime7 = 0.0f;
			m_bStrAmount = 0;
			m_bStaAmount = 0;
			m_bDexAmount = 0;
			m_bIntelAmount = 0;
			m_bChaAmount = 0;
			buff_type = 7 ;
		}
	}

	if (m_sDuration8 && buff_type == 0) {
		if (currenttime > (m_fStartTime8 + m_sDuration8)){
			m_sDuration8 = 0;		
			m_fStartTime8 = 0.0f;
			m_bFireRAmount = 0;
			m_bColdRAmount = 0;
			m_bLightningRAmount = 0;
			m_bMagicRAmount = 0;
			m_bDiseaseRAmount = 0;
			m_bPoisonRAmount = 0;
			buff_type = 8 ;
		}
	}

	if (m_sDuration9 && buff_type == 0) {
		if (currenttime > (m_fStartTime9 + m_sDuration9)){
			m_sDuration9 = 0;		
			m_fStartTime9 = 0.0f;
			m_bHitRateAmount = 100;
			m_sAvoidRateAmount = 100;
			buff_type = 9 ;
		}
	}

	if (buff_type) {
		m_bType4Buff[buff_type - 1] = 0;

		SetSlotItemValue();
		SetUserAbility();
		Send2AI_UserUpdateInfo();	// AI Server?? ??? ????� ???....		

		/*	Send Party Packet.....
		if (m_sPartyIndex != -1) {
			SetByte( send_buff, WIZ_PARTY, send_index );
			SetByte( send_buff, PARTY_STATUSCHANGE, send_index );
			SetShort( send_buff, m_Sid, send_index );
//			if (buff_type != 5 && buff_type != 6) {
//				SetByte( send_buff, 3, send_index );
//			}
//			else {
			SetByte( send_buff, 2, send_index );
//			}
			SetByte( send_buff, 0x00, send_index);
			m_pMain->Send_PartyMember(m_sPartyIndex, send_buff, send_index);
			send_index = 0 ;
		}
		//  end of Send Party Packet.....  */

		SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );
		SetByte( send_buff, MAGIC_TYPE4_END, send_index );	
		SetByte( send_buff, buff_type, send_index ); 
		Send( send_buff, send_index ); 
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
//
	// Send Party Packet.....
	if (m_sPartyIndex != -1 && bType4Test) {
		send_index = 0 ;
		SetByte( send_buff, WIZ_PARTY, send_index );
		SetByte( send_buff, PARTY_STATUSCHANGE, send_index );
		SetShort( send_buff, m_Sid, send_index );
		SetByte( send_buff, 2, send_index );
		SetByte( send_buff, 0x00, send_index);
		m_pMain->Send_PartyMember(m_sPartyIndex, send_buff, send_index);
	}
	//  end of Send Party Packet.....  //
//
}

BYTE CUser::ItemCountChange(int itemid, int type, int amount)	// 0 : Requested item not available.
{																// 1 : Amount is greater than current item count.
	int send_index = 0, result = 0, slot = -1 ;					// 2 : Success. Current item count updated or deleted.
	char send_buff[128];

	_ITEM_TABLE* pTable = NULL;				// This checks if such an item exists.
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
	if( !pTable ) {
		result = 0;
		return result;	
	}

	for ( int i = SLOT_MAX * type ; i < SLOT_MAX + HAVE_MAX * type ; i++ ) {
		if( m_pUserData->m_sItemArray[i].nNum == itemid ) {		
// 
			if (pTable->m_bReqDex > (m_pUserData->m_bDex + m_sItemDex + m_bDexAmount) && pTable->m_bReqDex != 0) {
				return result;
			}

			if (pTable->m_bReqStr > (m_pUserData->m_bStr + m_sItemStr + m_bStrAmount) && pTable->m_bReqStr != 0) {
				return result;
			}

			if (pTable->m_bReqSta > (m_pUserData->m_bSta + m_sItemSta + m_bStaAmount) && pTable->m_bReqSta != 0) {
				return result;
			}

			if (pTable->m_bReqIntel > (m_pUserData->m_bIntel + m_sItemIntel + m_bIntelAmount) && pTable->m_bReqIntel != 0) {
				return result;
			}

			if (pTable->m_bReqCha > (m_pUserData->m_bCha + m_sItemCham + m_bChaAmount) && pTable->m_bReqCha !=0) {
				return result;
			}
//
			if (!pTable->m_bCountable) {	// This checks if the user actually has that item.
				result = 2;
				return result;
			}

			slot = i ;
			m_pUserData->m_sItemArray[i].sCount -= amount ;

			if (m_pUserData->m_sItemArray[i].sCount == 0) {
				m_pUserData->m_sItemArray[i].nNum = 0 ;
				result = 2 ;	// You have just ran out of items.
				break;
			}
			else if (m_pUserData->m_sItemArray[i].sCount < 0) {
				m_pUserData->m_sItemArray[i].sCount += amount ;
				result = 1 ;	// Insufficient number of items.
				break;
			}

			result = 2 ;		// No error detected....
		}		
	}

	if ( result < 2 ) return result ;	// Something happened :(

	SendItemWeight();

	SetByte( send_buff, WIZ_ITEM_COUNT_CHANGE, send_index );	
	SetShort( send_buff, 1, send_index );	// The number of for-loops
	SetByte( send_buff, type, send_index );
	SetByte( send_buff, slot - type * SLOT_MAX , send_index );
	SetDWORD( send_buff, itemid, send_index );	// The ID of item.
	SetDWORD( send_buff, m_pUserData->m_sItemArray[slot].sCount, send_index );

	Send( send_buff, send_index );

	return result ;		// Success :) 
}

void CUser::SendAllKnightsID()
{
	int send_index = 0, count = 0, buff_index = 0;
	char send_buff[4096], temp_buff[4096];

	foreach_stlmap (itr, m_pMain->m_KnightsArray)
	{
		CKnights *pKnights = itr->second;
		if( !pKnights ) continue;
		//if( pKnights->bFlag != KNIGHTS_TYPE ) continue;
		SetShort( temp_buff, pKnights->m_sIndex, buff_index );
		SetCString(temp_buff, pKnights->m_strName, buff_index);
		count++;
	}

	SetByte( send_buff, WIZ_KNIGHTS_LIST, send_index );
	SetByte( send_buff, 0x01, send_index );					// All List 
	SetShort( send_buff, count, send_index );
	SetString( send_buff, temp_buff, buff_index, send_index );
	SendCompressingPacket( send_buff, send_index );
}

void CUser::ItemRemove(char *pBuf)
{
	int index = 0, send_index = 0, slot = 0, pos = 0, itemid = 0, count = 0, durability = 0;
	__int64 serial = 0;
	char send_buff[128];

	slot = GetByte( pBuf, index );
	pos = GetByte( pBuf, index );
	itemid = GetDWORD( pBuf, index );

	if( slot == 1 ) {
		if( pos > SLOT_MAX ) goto fail_return;
		if( m_pUserData->m_sItemArray[pos].nNum != itemid ) goto fail_return;
		count = m_pUserData->m_sItemArray[pos].sCount;
		durability = m_pUserData->m_sItemArray[pos].sDuration;
		serial = m_pUserData->m_sItemArray[pos].nSerialNum;
		m_pUserData->m_sItemArray[pos].nNum = 0;
		m_pUserData->m_sItemArray[pos].sCount = 0;
		m_pUserData->m_sItemArray[pos].sDuration = 0;
		m_pUserData->m_sItemArray[pos].nSerialNum = 0;
	}
	else {
		if( pos > HAVE_MAX ) goto fail_return;
		if( m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum != itemid ) goto fail_return;
		count = m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount;
		durability = m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration;
		serial = m_pUserData->m_sItemArray[SLOT_MAX+pos].nSerialNum;
		m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum = 0;
		m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount = 0;
		m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration = 0;
		m_pUserData->m_sItemArray[SLOT_MAX+pos].nSerialNum = 0;
	}

	SendItemWeight();

	SetByte( send_buff, WIZ_ITEM_REMOVE, send_index );
	SetByte( send_buff, 0x01, send_index );
	Send( send_buff, send_index );

	ItemLogToAgent( m_pUserData->m_id, "DESTROY", ITEM_DESTROY, serial, itemid, count, durability );
	return;
fail_return:
	SetByte( send_buff, WIZ_ITEM_REMOVE, send_index );
	SetByte( send_buff, 0x00, send_index );
	Send( send_buff, send_index );
}

void CUser::OperatorCommand(char *pBuf)
{
	int index = 0;
	char userid[MAX_ID_SIZE+1];
	CUser* pUser = NULL;

	if( m_pUserData->m_bAuthority != 0 ) return;	// Is this user`s authority operator?
	
	BYTE command = GetByte( pBuf, index );
	if (!GetKOString(pBuf, userid, index, MAX_ID_SIZE))
		return;

	pUser = m_pMain->GetUserPtr(userid, TYPE_CHARACTER);
	if( !pUser ) return;

	switch( command ) {
	case OPERATOR_ARREST:
		ZoneChange( pUser->m_pUserData->m_bZone, pUser->m_pUserData->m_curx, pUser->m_pUserData->m_curz );
		break;
	case OPERATOR_KILL:
		pUser->m_pUserData->m_bAuthority = 255;
		pUser->Close();
		break;
	case OPERATOR_NOTCHAT:
		pUser->m_pUserData->m_bAuthority = 2;
		break;
	case OPERATOR_CHAT:
		pUser->m_pUserData->m_bAuthority = 1;
		break;
	}
}

void CUser::SpeedHackTime(char* pBuf)
{
	BYTE b_first = 0x00;
	int index = 0;
	float servertime = 0.0f, clienttime = 0.0f, client_gap = 0.0f, server_gap = 0.0f;

	b_first = GetByte( pBuf, index );
	clienttime = Getfloat( pBuf, index );

	if( b_first ) {
		m_fSpeedHackClientTime = clienttime;
		m_fSpeedHackServerTime = TimeGet();
	}
	else {
		servertime = TimeGet();

		server_gap = servertime - m_fSpeedHackServerTime;
		client_gap = clienttime - m_fSpeedHackClientTime;

		if( client_gap - server_gap > 10.0f ) {
			char logstr[256];
			sprintf_s( logstr, sizeof(logstr), "%s SpeedHack User Checked By Server Time\r\n", m_pUserData->m_id);
			LogFileWrite( logstr );

			Close();
		}
		else if( client_gap - server_gap < 0.0f ) {
			m_fSpeedHackClientTime = clienttime;
			m_fSpeedHackServerTime = TimeGet();
		}
	}
}

void CUser::ServerStatusCheck()
{
	int send_index = 0;
	char send_buff[3];
	SetByte(send_buff, WIZ_SERVER_CHECK, send_index );
	SetShort(send_buff, m_pMain->m_sErrorSocketCount, send_index);
	Send( send_buff, send_index );
}

void CUser::Type3AreaDuration(float currenttime)
{
	int send_index = 0;
	char send_buff[128];

	_MAGIC_TYPE3* pType = NULL;
	pType = m_pMain->m_Magictype3Array.GetData(m_iAreaMagicID);      // Get magic skill table type 3.
	if( !pType ) return;

	if ( m_fAreaLastTime != 0.0f && (currenttime - m_fAreaLastTime) > m_bAreaInterval) {    // Did one second pass?
		m_fAreaLastTime = currenttime ;
		if( m_bResHpType == USER_DEAD ) return;
		
		for (int i = 0 ; i < MAX_USER ; i++) {	// Actual damage procedure.
			if (m_MagicProcess.UserRegionCheck(m_Sid, i, m_iAreaMagicID, pType->bRadius)) {	// Region check.
				CUser* pTUser = NULL ;     		
				pTUser = m_pMain->GetUnsafeUserPtr(i);
				if (pTUser == NULL)
					continue;

				SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );	// Set packet.
				SetByte( send_buff, MAGIC_EFFECTING, send_index );
				SetDWORD( send_buff, m_iAreaMagicID, send_index );
				SetShort( send_buff, m_Sid, send_index );
				SetShort( send_buff, i, send_index );
				SetShort( send_buff, 0, send_index );	
				SetShort( send_buff, 0, send_index );	
				SetShort( send_buff, 0, send_index );	
				m_pMain->Send_Region( send_buff, send_index, GetMap(), m_RegionX, m_RegionZ, NULL, false );
			}
		}

		if ( (( currenttime - m_fAreaStartTime) >= pType->bDuration) || m_bResHpType == USER_DEAD) { // Did area duration end? 			
			m_bAreaInterval = 5;
			m_fAreaStartTime = 0.0f;
			m_fAreaLastTime = 0.0f;
			m_iAreaMagicID = 0;
		}
	}	


	SetByte( send_buff, WIZ_MAGIC_PROCESS, send_index );	// Set packet.
	SetByte( send_buff, MAGIC_EFFECTING, send_index );
	SetDWORD( send_buff, m_iAreaMagicID, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetShort( send_buff, 0, send_index );	
	SetShort( send_buff, 0, send_index );	
	SetShort( send_buff, 0, send_index );	
	m_pMain->Send_Region( send_buff, send_index, GetMap(), m_RegionX, m_RegionZ, NULL, false );	// Send packet to region.
}

void CUser::InitType4()
{
	m_bAttackSpeedAmount = 100;		// this is for the duration spells Type 4
    m_bSpeedAmount = 100;
    m_sACAmount = 0;
    m_bAttackAmount = 100;
	m_sMaxHPAmount = 0;
	m_bHitRateAmount = 100;
	m_sAvoidRateAmount = 100;
	m_bStrAmount = 0;
	m_bStaAmount = 0;
	m_bDexAmount = 0;
	m_bIntelAmount = 0;
	m_bChaAmount = 0;
	m_bFireRAmount = 0;
	m_bColdRAmount = 0;
	m_bLightningRAmount = 0;
	m_bMagicRAmount = 0;
	m_bDiseaseRAmount = 0;
	m_bPoisonRAmount = 0;		
// ????? ???
	m_bAbnormalType = 1;
//
	m_sDuration1 = 0 ;  m_fStartTime1 = 0.0f ;		// Used for Type 4 Durational Spells.
	m_sDuration2 = 0 ;  m_fStartTime2 = 0.0f ;
	m_sDuration3 = 0 ;  m_fStartTime3 = 0.0f ;
	m_sDuration4 = 0 ;  m_fStartTime4 = 0.0f ;
	m_sDuration5 = 0 ;  m_fStartTime5 = 0.0f ;
	m_sDuration6 = 0 ;  m_fStartTime6 = 0.0f ;
	m_sDuration7 = 0 ;  m_fStartTime7 = 0.0f ;
	m_sDuration8 = 0 ;  m_fStartTime8 = 0.0f ;
	m_sDuration9 = 0 ;  m_fStartTime9 = 0.0f ;

	for (int h = 0 ; h < MAX_TYPE4_BUFF ; h++) {
		m_bType4Buff[h] = 0 ;
	}

	m_bType4Flag = FALSE;
}

int CUser::GetEmptySlot(int itemid, int bCountable)	// item ????? ?????? ????? �????...
{
	int pos = 255, i=0;
	
	_ITEM_TABLE* pTable = NULL;

	if (bCountable == -1) {
		pTable = m_pMain->m_ItemtableArray.GetData( itemid );
		if (!pTable) return pos;
		
		bCountable = pTable->m_bCountable;
	}

	if( itemid == ITEM_GOLD ) return pos;
	for(i=0; i<HAVE_MAX; i++) {
		if( m_pUserData->m_sItemArray[SLOT_MAX+i].nNum != 0 )
			continue;
		else {
			pos = i;
			break;
		}
	}
	if( bCountable ){
		for(i=0; i<HAVE_MAX; i++) {
			if( m_pUserData->m_sItemArray[SLOT_MAX+i].nNum == itemid )
				return i;
		}
		if( i == HAVE_MAX )
			return pos;
	}
	return pos;
}

void CUser::Home()
{
	// The point where you will be warped to.
	short x = 0, z = 0;

	// Forgotten Temple
	if (getZoneID() == 55)
	{
		KickOutZoneUser(TRUE);
		return;
	}
	// Prevent /town'ing in quest arenas
	else if ((getZoneID() / 10) == 5
		|| !GetStartPosition(x, z))
		return;

	char send_buff[4]; int send_index = 0;
	SetShort(send_buff, (WORD)(x * 10), send_index);	
	SetShort(send_buff, (WORD)(z * 10), send_index);
	Warp(send_buff);
}

bool CUser::GetStartPosition(short & x, short & z, BYTE bZone /*= 0 */)
{
	// Get start position data for current zone (unless we specified a zone).
	int nZoneID = (bZone == 0 ? getZoneID() : bZone);
	_START_POSITION *pData = m_pMain->GetStartPosition(nZoneID);
	if (pData == NULL)
		return false;

	// TO-DO: Allow for Delos/CSW.

	// NOTE: This is how mgame does it.
	// This only allows for positive randomisation; we should really allow for the full range...
	if (getNation() == KARUS)
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
/*	if (isTrading())
		ExchangeCancel();

	if (isUsingMerchant())
		MerchantClose();

	if (isUsingBuyingMerchant())
		BuyingMerchantClose();

	if (isUsingStore())
		m_bStoreOpen = false;*/
}

CUser* CUser::GetItemRoutingUser(int itemid, short itemcount)
{
	if( m_sPartyIndex == -1 ) return NULL;

	CUser* pUser = NULL;
	_PARTY_GROUP* pParty = NULL;
	int select_user = -1, count = 0;
 
	pParty = m_pMain->m_PartyArray.GetData( m_sPartyIndex );
	if( !pParty ) return NULL;
	if(	pParty->bItemRouting > 7 ) return NULL;
//
	_ITEM_TABLE* pTable = NULL;
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
	if( !pTable ) return NULL;
//
	while(count<8) {
		pUser = m_pMain->GetUserPtr(pParty->uid[pParty->bItemRouting]);
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
	char send_buff[128];
	int send_index = 0;

	SetByte( send_buff, WIZ_CLASS_CHANGE, send_index );
	SetByte( send_buff, CLASS_CHANGE_RESULT, send_index );
	if( m_pUserData->m_bLevel < 10 )
		SetByte( send_buff, 0x02, send_index );
	else if ( (m_pUserData->m_sClass % 100) > 4 )
		SetByte( send_buff, 0x03, send_index );
	else
		SetByte( send_buff, 0x01, send_index );
	Send( send_buff, send_index );
}

void CUser::AllSkillPointChange()
{
	int index = 0, send_index = 0, skill_point = 0, money = 0, i=0, j=0, temp_value = 0, old_money = 0;
	BYTE type = 0x00;    // 0:???? ???, 1:????, 2:?????? ??u?? ?????..
	char send_buff[128];

	temp_value = (int)pow(( m_pUserData->m_bLevel * 2.0f ), 3.4f);
	temp_value = ( temp_value / 100 )*100;
	if( m_pUserData->m_bLevel < 30)		temp_value = (int)(temp_value * 0.4f);
	else if( m_pUserData->m_bLevel >= 60 && m_pUserData->m_bLevel <= 90 ) temp_value = (int)(temp_value * 1.5f);

	temp_value = (int)(temp_value * 1.5);

	if( m_pMain->m_sDiscount == 1 && m_pMain->m_byOldVictory == m_pUserData->m_bNation )		{	// ????????? ?�???????
		old_money = temp_value;
		temp_value = (int)(temp_value * 0.5f);
		//TRACE("^^ AllSkillPointChange - Discount ,, money=%d->%d\n", old_money, temp_value);
	}

	if( m_pMain->m_sDiscount == 2  )		{	
		old_money = temp_value;
		temp_value = (int)(temp_value * 0.5f);
		//TRACE("^^ AllSkillPointChange - Discount ,, money=%d->%d\n", old_money, temp_value);
	}

	money = m_pUserData->m_iGold - temp_value;
	//money = m_pUserData->m_iGold - 100;

	if(money < 0)	goto fail_return;
	if( m_pUserData->m_bLevel < 10 )	goto fail_return;

	for(i=1; i<9; i++)	{
		skill_point += m_pUserData->m_bstrSkill[i];
	}

	if(skill_point <= 0) {
		type = 2;
		goto fail_return;
	}

	// ????? ???? ??? : ????u?? 255? ??? ????? ????? ???? ???..
	//m_pUserData->m_bstrSkill[0] += skill_point;		
	m_pUserData->m_bstrSkill[0] = (m_pUserData->m_bLevel - 9) * 2;
	for(j=1; j<9; j++)	
		m_pUserData->m_bstrSkill[j] = 0;
	m_pUserData->m_iGold = money;
	type = 1;

	SetByte( send_buff, WIZ_CLASS_CHANGE, send_index );
	SetByte( send_buff, ALL_SKILLPT_CHANGE, send_index );
	SetByte( send_buff, type, send_index );
	SetDWORD( send_buff, m_pUserData->m_iGold, send_index );
	SetByte( send_buff, m_pUserData->m_bstrSkill[0], send_index );
	Send( send_buff, send_index );

	return;

fail_return:
	SetByte( send_buff, WIZ_CLASS_CHANGE, send_index );
	SetByte( send_buff, ALL_SKILLPT_CHANGE, send_index );
	SetByte( send_buff, type, send_index );
	SetDWORD( send_buff, temp_value, send_index );
	Send( send_buff, send_index );
}

void CUser::AllPointChange()
{
	int index = 0, send_index = 0, total_point = 0, money = 0, classcode=0, temp_money = 0, old_money=0;
	double dwMoney = 0;
	BYTE type = 0x00;
	char send_buff[128];

	if( m_pUserData->m_bLevel > 80 ) goto fail_return;

	temp_money = (int)pow(( m_pUserData->m_bLevel * 2.0f ), 3.4f);
	temp_money = (temp_money/100)*100;
	if( m_pUserData->m_bLevel < 30)		temp_money = (int)(temp_money * 0.4f);
	else if( m_pUserData->m_bLevel >= 60 && m_pUserData->m_bLevel <= 90 ) temp_money = (int)(temp_money * 1.5f);

	if( m_pMain->m_sDiscount == 1 && m_pMain->m_byOldVictory == m_pUserData->m_bNation )		{	// ????????? ?�???????
		temp_money = (int)(temp_money * 0.5f);
		//TRACE("^^ AllPointChange - Discount ,, money=%d->%d\n", old_money, temp_money);
	}

	if( m_pMain->m_sDiscount == 2  )		{	
		temp_money = (int)(temp_money * 0.5f);
	}

	money = m_pUserData->m_iGold - temp_money;
	if(money < 0)	goto fail_return;

	for (int i = 0; i < SLOT_MAX; i++)
	{
		if (m_pUserData->m_sItemArray[i].nNum) {
			type = 0x04;
			goto fail_return;
		}
	}
	
	switch( m_pUserData->m_bRace ) {
	case KARUS_BIG:	
		if( m_pUserData->m_bStr == 65 && m_pUserData->m_bSta == 65 && m_pUserData->m_bDex == 60 && m_pUserData->m_bIntel == 50 && m_pUserData->m_bCha == 50 )	{
			type = 0x02;	
			goto fail_return;
		}
		m_pUserData->m_bStr = 65;
		m_pUserData->m_bSta = 65;
		m_pUserData->m_bDex = 60;
		m_pUserData->m_bIntel = 50;
		m_pUserData->m_bCha = 50;
		break;
	case KARUS_MIDDLE:
		if( m_pUserData->m_bStr == 65 && m_pUserData->m_bSta == 65 && m_pUserData->m_bDex == 60 && m_pUserData->m_bIntel == 50 && m_pUserData->m_bCha == 50 )	{
			type = 0x02;	
			goto fail_return;
		}
		m_pUserData->m_bStr = 65;
		m_pUserData->m_bSta = 65;
		m_pUserData->m_bDex = 60;
		m_pUserData->m_bIntel = 50;
		m_pUserData->m_bCha = 50;
		break;
	case KARUS_SMALL:
		if( m_pUserData->m_bStr == 50 && m_pUserData->m_bSta == 50 && m_pUserData->m_bDex == 70 && m_pUserData->m_bIntel == 70 && m_pUserData->m_bCha == 50 )	{
			type = 0x02;	
			goto fail_return;
		}
		m_pUserData->m_bStr = 50;
		m_pUserData->m_bSta = 50;
		m_pUserData->m_bDex = 70;
		m_pUserData->m_bIntel = 70;
		m_pUserData->m_bCha = 50;
		break;
	case KARUS_WOMAN:
		if( m_pUserData->m_bStr == 50 && m_pUserData->m_bSta == 60 && m_pUserData->m_bDex == 60 && m_pUserData->m_bIntel == 70 && m_pUserData->m_bCha == 50 )	{
			type = 0x02;	
			goto fail_return;
		}
		m_pUserData->m_bStr = 50;
		m_pUserData->m_bSta = 60;
		m_pUserData->m_bDex = 60;
		m_pUserData->m_bIntel = 70;
		m_pUserData->m_bCha = 50;
		break;
	case BABARIAN:
		if( m_pUserData->m_bStr == 65 && m_pUserData->m_bSta == 65 && m_pUserData->m_bDex == 60 && m_pUserData->m_bIntel == 50 && m_pUserData->m_bCha == 50 )	{
			type = 0x02;	
			goto fail_return;
		}
		m_pUserData->m_bStr = 65;
		m_pUserData->m_bSta = 65;
		m_pUserData->m_bDex = 60;
		m_pUserData->m_bIntel = 50;
		m_pUserData->m_bCha = 50;
		break;
	case ELMORAD_MAN:
		if( m_pUserData->m_bStr == 60 && m_pUserData->m_bSta == 60 && m_pUserData->m_bDex == 70 && m_pUserData->m_bIntel == 50 && m_pUserData->m_bCha == 50 )	{
			type = 0x02;	
			goto fail_return;
		}
		m_pUserData->m_bStr = 60;
		m_pUserData->m_bSta = 60;
		m_pUserData->m_bDex = 70;
		m_pUserData->m_bIntel = 50;
		m_pUserData->m_bCha = 50;
		break;
	case ELMORAD_WOMAN:
		if( m_pUserData->m_bStr == 50 && m_pUserData->m_bSta == 50 && m_pUserData->m_bDex == 70 && m_pUserData->m_bIntel == 70 && m_pUserData->m_bCha == 50 )	{
			type = 0x02;	
			goto fail_return;
		}
		m_pUserData->m_bStr = 50;
		m_pUserData->m_bSta = 50;
		m_pUserData->m_bDex = 70;
		m_pUserData->m_bIntel = 70;
		m_pUserData->m_bCha = 50;
		break;
	}

	m_pUserData->m_sPoints = (m_pUserData->m_bLevel-1) * 3 + 10;
	m_pUserData->m_iGold = money;

	SetUserAbility();
	Send2AI_UserUpdateInfo();

	type = 0x01;
	SetByte( send_buff, WIZ_CLASS_CHANGE, send_index );
	SetByte( send_buff, ALL_POINT_CHANGE, send_index );
	SetByte( send_buff, type, send_index );
	SetDWORD( send_buff, m_pUserData->m_iGold, send_index );
	SetShort( send_buff, m_pUserData->m_bStr, send_index );
	SetShort( send_buff, m_pUserData->m_bSta, send_index );
	SetShort( send_buff, m_pUserData->m_bDex, send_index );
	SetShort( send_buff, m_pUserData->m_bIntel, send_index );
	SetShort( send_buff, m_pUserData->m_bCha, send_index );
	SetShort( send_buff, m_iMaxHp, send_index );
	SetShort( send_buff, m_iMaxMp, send_index );
	SetShort( send_buff, m_sTotalHit, send_index );
	SetShort( send_buff, m_sMaxWeight, send_index );
	SetShort( send_buff, m_pUserData->m_sPoints, send_index );
	Send( send_buff, send_index );


fail_return:
	SetByte( send_buff, WIZ_CLASS_CHANGE, send_index );
	SetByte( send_buff, ALL_POINT_CHANGE, send_index );
	SetByte( send_buff, type, send_index );
	SetDWORD( send_buff, temp_money, send_index );
	Send( send_buff, send_index );

}

void CUser::GoldChange(short tid, int gold)
{
	if (m_pUserData->m_bZone < 3) return;	// Money only changes in Frontier zone and Battle zone!!!
	if (m_pUserData->m_bZone == ZONE_SNOW_BATTLE) return;

	int s_temp_gold = 0; int t_temp_gold = 0, i = 0, send_index = 0;
	BYTE s_type = 0; BYTE t_type = 0;    // 1 -> Get gold    2 -> Lose gold

	char send_buff[256];

	CUser* pTUser = m_pMain->GetUserPtr(tid);
	if (pTUser == NULL)
		return;
	
	if ( pTUser->m_pUserData->m_iGold <= 0 ) return;

	if (gold == 0) {		// Reward money in battle zone!!!
		if (m_sPartyIndex == -1) {	// Source is NOT in a party.
			s_type = 1 ; t_type = 2 ;

			s_temp_gold = (pTUser->m_pUserData->m_iGold * 4) / 10;
			t_temp_gold = pTUser->m_pUserData->m_iGold / 2;

			m_pUserData->m_iGold += s_temp_gold;
			pTUser->m_pUserData->m_iGold -= t_temp_gold;			
		}
		else {	// When the source is in a party.			
			_PARTY_GROUP* pParty = NULL;
			pParty = m_pMain->m_PartyArray.GetData( m_sPartyIndex );
			if( !pParty ) return;			

			s_type = 1 ; t_type = 2 ;

			s_temp_gold = (pTUser->m_pUserData->m_iGold * 4) / 10;
			t_temp_gold = pTUser->m_pUserData->m_iGold / 2;
			
			pTUser->m_pUserData->m_iGold -= t_temp_gold;		

			SetByte( send_buff, WIZ_GOLD_CHANGE, send_index );	// First the victim...
			SetByte( send_buff, t_type, send_index );
			SetDWORD( send_buff, t_temp_gold, send_index );
			SetDWORD( send_buff, pTUser->m_pUserData->m_iGold, send_index );
			pTUser->Send( send_buff, send_index );	

			int usercount = 0, money = 0, levelsum = 0, count = 0;		// For the loot sharing procedure...
			count = s_temp_gold ;

			for( i=0; i<8; i++ ) {
				if( pParty->uid[i] != -1 ) {
					usercount++;
					levelsum += pParty->bLevel[i];
				}
			}

			if( usercount == 0 ) return;

			for( i=0; i<8; i++ ) {		
				if( pParty->uid[i] != -1 || pParty->uid[i] >= MAX_USER ) {
					CUser * pUser = m_pMain->GetUserPtr(pParty->uid[i]);
					if (pUser == NULL)
						continue;

					money = (int)(count * (float)(pUser->m_pUserData->m_bLevel / (float)levelsum));
					pUser->m_pUserData->m_iGold += money;

					send_index = 0;
					SetByte( send_buff, WIZ_GOLD_CHANGE, send_index );		// Now the party members...
					SetByte( send_buff, 1, send_index );
					SetDWORD( send_buff, money, send_index );
					SetDWORD( send_buff, pUser->m_pUserData->m_iGold, send_index );
					pUser->Send( send_buff, send_index );
				}
			}			
			return ;
		}
	}
	else {	// When actual values are provided.
		if (gold > 0 ) {	// Source gains money.
			s_type = 1; t_type = 2;
			s_temp_gold = gold; t_temp_gold = gold;

			m_pUserData->m_iGold += s_temp_gold;
			pTUser->m_pUserData->m_iGold -= t_temp_gold;
		}
		else {	// Source loses money.
			s_type = 2; t_type = 1;
			s_temp_gold = gold; t_temp_gold = gold;

			m_pUserData->m_iGold -= s_temp_gold;
			pTUser->m_pUserData->m_iGold += t_temp_gold;
		}
	}

	SetByte( send_buff, WIZ_GOLD_CHANGE, send_index );	// First the source...
	SetByte( send_buff, s_type, send_index );
	SetDWORD( send_buff, s_temp_gold, send_index );
	SetDWORD( send_buff, m_pUserData->m_iGold, send_index );
	Send( send_buff, send_index );	

	send_index = 0;

	SetByte( send_buff, WIZ_GOLD_CHANGE, send_index );	// Now the target
	SetByte( send_buff, t_type, send_index );
	SetDWORD( send_buff, t_temp_gold, send_index );
	SetDWORD( send_buff, pTUser->m_pUserData->m_iGold, send_index );
	pTUser->Send( send_buff, send_index );	
}

void CUser::SelectWarpList(char *pBuf)
{
	int index = 0, send_index = 0, warpid = 0, zoneindex = 0;
	_WARP_INFO* pWarp = NULL;
	_ZONE_SERVERINFO *pInfo	= NULL;
	C3DMap* pMap = NULL;
	char send_buff[128]; 

	BYTE type = 2 ;
	warpid = GetShort(pBuf, index);

	pWarp = GetMap()->m_WarpArray.GetData(warpid);
	if (pWarp == NULL)
		return;

	pMap = m_pMain->GetZoneByID(pWarp->sZone);
	if (pMap == NULL)
		return;

	pInfo = m_pMain->m_ServerArray.GetData(pMap->m_nServerNo);
	if (pInfo == NULL)
		return;

	float rx = 0.0f, rz = 0.0f;
	rx = (float)myrand( 0, (int)pWarp->fR*2 );
	if( rx < pWarp->fR ) rx = -rx;
	rz = (float)myrand( 0, (int)pWarp->fR*2 );
	if( rz < pWarp->fR ) rz = -rz;

	if (m_pUserData->m_bZone == pWarp->sZone) 
	{
		m_bZoneChangeSameZone = TRUE;

		SetByte(send_buff, WIZ_WARP_LIST, send_index);
		SetByte(send_buff, type, send_index);
		SetByte(send_buff, 1, send_index);
		Send(send_buff, send_index);
	}

	ZoneChange(pWarp->sZone, pWarp->fX + rx, pWarp->fZ + rz);
}

void CUser::ZoneConCurrentUsers(char *pBuf)
{
	int index = 0, send_index = 0, zone = 0, usercount = 0, nation = 0;
	char send_buff[128];
	CUser* pUser = NULL;

	zone = GetShort( pBuf, index );
	nation = GetByte( pBuf, index );

	for (int i = 0; i < MAX_USER; i++)
	{
		pUser = m_pMain->GetUnsafeUserPtr(i);
		if (pUser != NULL && pUser->m_pUserData->m_bZone == zone && pUser->getNation() == nation)
			usercount++;
	}

	SetByte( send_buff, WIZ_ZONE_CONCURRENT, send_index );
	SetShort( send_buff, usercount, send_index );
	Send( send_buff, send_index );
}

void CUser::ServerChangeOk(char *pBuf)
{
	int index = 0, warpid = 0;
	C3DMap* pMap = GetMap();
	float rx = 0.0f, rz = 0.0f;
	warpid = GetShort(pBuf, index);

	if (pMap == NULL)
		return;

	_WARP_INFO* pWarp = pMap->m_WarpArray.GetData(warpid);
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
	C3DMap* pMap = GetMap();
	int warpid = 0, send_index = 0;
	int zoneindex = -1, temp_index = 0, count = 0;
	char buff[8192], send_buff[8192]; 

	BYTE type = 1;
	foreach_stlmap (itr, pMap->m_WarpArray)
	{
		_WARP_INFO *pWarp = itr->second;
		if (pWarp == NULL || (pWarp->sWarpID / 10) != warp_group)
			continue;
		
		SetShort( buff, pWarp->sWarpID, send_index );

		SetKOString(buff, pWarp->strWarpName, send_index);
		SetKOString(buff, pWarp->strAnnounce, send_index);
		SetShort(buff, pWarp->sZone, send_index);

		C3DMap *pDstMap = m_pMain->GetZoneByID(pWarp->sZone);
		SetShort(buff, pDstMap != NULL ? pDstMap->m_sMaxUser : 0, send_index);

		SetDWORD( buff, pWarp->dwPay, send_index );
		SetShort( buff, (short)(pWarp->fX*10), send_index );
		SetShort( buff, (short)(pWarp->fZ*10), send_index );
		SetShort( buff, (short)(pWarp->fY*10), send_index );
		count++;
	}

	SetByte( send_buff, WIZ_WARP_LIST, temp_index );
	SetByte( send_buff, type, temp_index );
	SetShort( send_buff, count, temp_index );
	SetString( send_buff, buff, send_index, temp_index );
	Send( send_buff, temp_index );
	
	return TRUE;
}

void CUser::InitType3()
{
	for (int i = 0 ; i < MAX_TYPE3_REPEAT ; i++) {     // This is for the duration spells Type 3.
		m_fHPStartTime[i] = 0.0f;		
		m_fHPLastTime[i] = 0.0f;
		m_bHPAmount[i] = 0;
		m_bHPDuration[i] = 0;
		m_bHPInterval[i] = 5;
		m_sSourceID[i] = -1;
	}

	m_bType3Flag = FALSE;
}

BOOL CUser::BindObjectEvent(_OBJECT_EVENT *pEvent)
{
	int  send_index = 0;
	char send_buff[3];

	if (pEvent->sBelong != 0 && pEvent->sBelong != getNation())
		return FALSE;

	m_pUserData->m_sBind = pEvent->sIndex;

	SetByte(send_buff, WIZ_OBJECT_EVENT, send_index);
	SetByte(send_buff, (BYTE)pEvent->sType, send_index);
	SetByte(send_buff, 1, send_index);
	Send(send_buff, send_index);

	return TRUE;
}

BOOL CUser::GateLeverObjectEvent(_OBJECT_EVENT *pEvent, int nid)
{
	_OBJECT_EVENT *pGateEvent;
	CNpc* pNpc, *pGateNpc;

		// Does the lever (object) NPC exist?
	if ((pNpc = m_pMain->m_arNpcArray.GetData(nid)) == NULL
		// Does the corresponding gate object event exist?
		|| (pGateEvent = GetMap()->GetObjectEvent(pEvent->sControlNpcID)) == NULL
		// Does the corresponding gate (object) NPC exist?
		|| (pGateNpc = m_pMain->m_arNpcArray.GetData(pEvent->sControlNpcID)) == NULL
		// Is it even a gate?
		|| !pGateNpc->isGate()
		// If the gate's closed (i.e. the lever is down), we can't open it unless the lever isn't nation-specific
		// or we're the correct nation. Seems the other nation cannot close them.
		|| (pNpc->isGateClosed() && pNpc->getNation() != 0 && pNpc->getNation() != getNation()))
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
	if ((pNpc = m_pMain->m_arNpcArray.GetData(nid)) == NULL
		// Does the corresponding flag event exist?
		|| (pFlagEvent = GetMap()->GetObjectEvent(pEvent->sControlNpcID)) == NULL
		// Does the corresponding flag object NPC exist?
		|| (pFlagNpc = m_pMain->GetNpcPtr(pEvent->sControlNpcID, getZoneID())) == NULL
		// Is this marked a gate? (i.e. can control)
		|| !pFlagNpc->isGate()
		// Is the war over or the gate closed?
		|| m_pMain->m_bVictory > 0 || pNpc->isGateClosed())
		return FALSE;

	// Reset objects
	pNpc->SendGateFlag(0);
	pFlagNpc->SendGateFlag(0);

	// Add flag score (not sure what this is, is this even used anymore?)
	if (getNation() == KARUS) 
		m_pMain->m_bKarusFlag++;
	else
		m_pMain->m_bElmoradFlag++;

	// Did one of the teams win?
	m_pMain->BattleZoneVictoryCheck();	
	return TRUE;
}

BOOL CUser::WarpListObjectEvent(_OBJECT_EVENT *pEvent)
{
	if (!GetWarpList(pEvent->sControlNpcID)) 
		return FALSE;

	return TRUE;
}

void CUser::ObjectEvent(char *pBuf)
{
	int index = 0, objectindex = 0, send_index = 0, nid = 0;
	char send_buff[6];
	BOOL bSuccess = FALSE;

	if (m_pMain->m_bPointCheckFlag == FALSE)
		return;

	objectindex = GetShort(pBuf, index);
	nid = GetShort(pBuf, index);
	
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
		SetByte(send_buff, WIZ_OBJECT_EVENT, send_index);
		SetByte(send_buff, pEvent == NULL ? 0 : pEvent->sType, send_index);
		SetByte(send_buff, 0, send_index);
	}
}

void CUser::SendAnvilRequest(int nid)
{
	char send_buff[4]; int send_index = 0;
	SetByte(send_buff, WIZ_ITEM_UPGRADE, send_index);
	SetByte(send_buff, 1, send_index);
	SetShort(send_buff, nid, send_index);
	Send(send_buff, send_index);
}

void CUser::BlinkStart()
{
	// Don't blink in these zones
	if (m_pUserData->m_bZone == 201 // colony zone
		|| (m_pUserData->m_bZone / 100) == 1) // war zone
		return;

	m_bAbnormalType = ABNORMAL_BLINKING;
	m_fBlinkStartTime = TimeGet();
	m_bRegeneType = REGENE_ZONECHANGE;
	
	// TO-DO: Tell the AI server that mobs shouldn't see/attack us

	StateChangeServerDirect(3, ABNORMAL_BLINKING);
}

void CUser::BlinkTimeCheck(float currenttime)
{
	int send_index = 0;				
	char send_buff[256];

	if (BLINK_TIME < (currenttime - m_fBlinkStartTime)) {
		m_fBlinkStartTime = 0.0f;
		
		m_bAbnormalType = ABNORMAL_NORMAL;
//
		if (m_bRegeneType == REGENE_MAGIC) {
			HpChange(m_iMaxHp / 2);					// Refill HP.	
		}
		else {
			HpChange(m_iMaxHp);	
		}

		m_bRegeneType = REGENE_NORMAL;
//
		SetByte(send_buff, 3, send_index);
		SetDWORD(send_buff, m_bAbnormalType, send_index); // NOTE: this packet needs to explicitly be updated.
		StateChange(send_buff); 

		//TRACE("?? BlinkTimeCheck : name=%s(%d), type=%d ??\n", m_pUserData->m_id, m_Sid, m_bAbnormalType);
//
		// AI_server?? regene??? ???...	
		send_index=0;
		SetByte( send_buff, AG_USER_REGENE, send_index );
		SetShort( send_buff, m_Sid, send_index );
		SetShort( send_buff, m_pUserData->m_sHp, send_index );
		m_pMain->Send_AIServer(send_buff, send_index);
//
//
		send_index=0;		// To AI Server....
		SetByte( send_buff, AG_USER_INOUT, send_index );
		SetByte( send_buff, USER_REGENE, send_index );
		SetShort( send_buff, m_Sid, send_index );
		SetKOString(send_buff, m_pUserData->m_id, send_index);
		Setfloat( send_buff, m_pUserData->m_curx, send_index );
		Setfloat( send_buff, m_pUserData->m_curz, send_index );
		m_pMain->Send_AIServer(send_buff, send_index);
//
	}
}

void CUser::KickOut(char *pBuf)
{
	int index = 0, send_index = 0;
	char accountid[MAX_ID_SIZE+1], send_buff[256];
	CUser* pUser = NULL;
	if (!GetKOString(pBuf, accountid, index, MAX_ID_SIZE))
		return;

	pUser = m_pMain->GetUserPtr(accountid, TYPE_ACCOUNT);
	if( pUser ) {
		pUser->UserDataSaveToAgent();
		pUser->Close();
	}
	else {
		SetByte( send_buff, WIZ_KICKOUT, send_index );
		SetKOString( send_buff, accountid, send_index );
		m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );
	}
}

void CUser::GoldGain(int gold)	// 1 -> Get gold    2 -> Lose gold
{
	int send_index = 0;
	char send_buff[256];
	
	m_pUserData->m_iGold += gold;	// Add gold.

	SetByte( send_buff, WIZ_GOLD_CHANGE, send_index );	// First the source...
	SetByte( send_buff, 0x01, send_index );
	SetDWORD( send_buff, gold, send_index );
	SetDWORD( send_buff, m_pUserData->m_iGold, send_index );
	Send( send_buff, send_index );	
}

BOOL CUser::GoldLose(unsigned int gold)
{
	int send_index = 0;
	char send_buff[256];
	
	if (m_pUserData->m_iGold < gold) return FALSE;	// Insufficient gold!
	
	m_pUserData->m_iGold -= gold;	// Subtract gold.

	SetByte( send_buff, WIZ_GOLD_CHANGE, send_index );	// First the source...
	SetByte( send_buff, 0x02, send_index );
	SetDWORD( send_buff, gold, send_index );
	SetDWORD( send_buff, m_pUserData->m_iGold, send_index );
	Send( send_buff, send_index );	

	return TRUE;
}

BOOL CUser::CheckSkillPoint(BYTE skillnum, BYTE min, BYTE max)
{
	if (skillnum < 5 || skillnum > 8) return FALSE;

	if (m_pUserData->m_bstrSkill[skillnum] >= min && m_pUserData->m_bstrSkill[skillnum] <= max) {
		return TRUE;
	}
	else {
		return FALSE;
	}		

	return FALSE;
}

BOOL CUser::CheckClass(short class1, short class2, short class3, short class4, short class5, short class6)
{

	if (JobGroupCheck(class1)) return TRUE;
	if (JobGroupCheck(class2)) return TRUE;
	if (JobGroupCheck(class3)) return TRUE;
	if (JobGroupCheck(class4)) return TRUE;
	if (JobGroupCheck(class5)) return TRUE;
	if (JobGroupCheck(class6)) return TRUE;
	return FALSE;
}

BOOL CUser::JobGroupCheck(short jobgroupid)
{
	if (jobgroupid > 100) 
		return m_pUserData->m_sClass == jobgroupid;

	int subClass = m_pUserData->m_sClass % 100;

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

		default: // for all others
			return (subClass == jobgroupid);
	}

	return FALSE; // this will never hit
}

void CUser::TrapProcess()
{
	float currenttime = 0.0f;
	currenttime = TimeGet();

	if (ZONE_TRAP_INTERVAL < (currenttime - m_fLastTrapAreaTime)) {	// Time interval has passed :)
		HpChange( -ZONE_TRAP_DAMAGE );     // Reduce target health point.

		if( m_pUserData->m_sHp == 0) {    // Check if the target is dead.
			m_bResHpType = USER_DEAD;     // Target status is officially dead now.
		
			InitType3();	// Init Type 3.....
			InitType4();	// Init Type 4.....

			m_sWhoKilledMe = -1;		// Who the hell killed me?
		}
	} 

	m_fLastTrapAreaTime = currenttime;		// Update Last Trap Area time :)
}

// TO-DO: This needs updating.
void CUser::KickOutZoneUser(BOOL home, int nZoneID /*= 21 */)
{
	int yourmama=0, random = 0;
	_REGENE_EVENT* pRegene = NULL;
	C3DMap* pMap = m_pMain->GetZoneByID(nZoneID);
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
		if (m_pUserData->m_bNation == KARUS) {
			ZoneChange( pMap->m_nZoneNumber, 1335, 83);	// Move user to native zone.
		}
		else {
			ZoneChange( pMap->m_nZoneNumber, 445, 1950 );	// Move user to native zone.
		}
	}
}

void CUser::EventMoneyItemGet( int itemid, int count )
{
}

void CUser::NativeZoneReturn()
{
	_HOME_INFO* pHomeInfo = NULL;	// Send user back home in case it was the battlezone.
	pHomeInfo = m_pMain->m_HomeArray.GetData(m_pUserData->m_bNation);
	if (!pHomeInfo) return;

	m_pUserData->m_bZone = m_pUserData->m_bNation;

	if (m_pUserData->m_bNation == KARUS) {
		m_pUserData->m_curx = (float)(pHomeInfo->KarusZoneX + myrand(0, pHomeInfo->KarusZoneLX));
		m_pUserData->m_curz = (float)(pHomeInfo->KarusZoneZ + myrand(0, pHomeInfo->KarusZoneLZ)); 
	}
	else {
		m_pUserData->m_curx = (float)(pHomeInfo->ElmoZoneX + myrand(0, pHomeInfo->ElmoZoneLX));
		m_pUserData->m_curz = (float)(pHomeInfo->ElmoZoneZ + myrand(0, pHomeInfo->ElmoZoneLZ)); 
	}
}

BOOL CUser::CheckRandom(short percent)
{
	if (percent < 0 || percent > 1000) return FALSE;

	int rand = 0;
	rand = myrand(0, 1000);
	
	if (percent > rand) return TRUE;

	return FALSE;
}

void CUser::RecvEditBox(char *pBuf)
{
	EVENT* pEvent = NULL;
	EVENT_DATA* pEventData = NULL;

	int index = 0; int selevent = -1; 
	if (!GetKOString(pBuf, m_strCouponId, index, MAX_COUPON_ID_LENGTH))
		return;
	selevent = m_iEditBoxEvent;

	pEvent = m_pMain->m_Event.GetData(getZoneID());
	if(!pEvent)	goto fail_return;

	pEventData = pEvent->m_arEvent.GetData(selevent);
	if (pEventData == NULL
		|| CheckEventLogic(pEventData)
		|| !RunEvent(pEventData))
		goto fail_return;

	return;	

fail_return:
	m_iEditBoxEvent = -1;
	memset(m_strCouponId, NULL, MAX_COUPON_ID_LENGTH);
}
