#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "Ebenezer.h"
#include "EbenezerDlg.h"
#include "User.h"

void CUser::NewCharToAgent(char *pBuf)
{
	int index = 0, idlen = 0, send_index = 0, retvalue = 0;
	int charindex = 0, race = 0, Class = 0, hair = 0, face = 0, str = 0, sta = 0, dex = 0, intel = 0, cha = 0;
	char charid[MAX_ID_SIZE+1];
	memset( charid, NULL, MAX_ID_SIZE+1 );
	char send_buff[256];
	memset( send_buff, NULL, 256);
	BYTE result;
	int sum = 0;
	_CLASS_COEFFICIENT* p_TableCoefficient = NULL;

	charindex = GetByte( pBuf, index );
	if (!GetKOString(pBuf, charid, index, MAX_ID_SIZE))
	{
		result = 0x05;
		goto fail_return;
	}

	race = GetByte( pBuf, index );
	Class = GetShort( pBuf, index );
	face = GetByte( pBuf, index );

	hair = GetDWORD( pBuf, index );

	str = GetByte( pBuf, index );
	sta = GetByte( pBuf, index );
	dex = GetByte( pBuf, index );
	intel = GetByte( pBuf, index );
	cha = GetByte( pBuf, index );

	if( charindex > 4 || charindex < 0 ) {
		result = 0x01;
		goto fail_return;
	}

	if( !IsValidName( charid ) ) {
		result = 0x05;
		goto fail_return;
	}

	p_TableCoefficient = m_pMain->m_CoefficientArray.GetData( Class );
	if( !p_TableCoefficient ) {
		result = 0x02;
		goto fail_return;
	}

	sum = str + sta + dex + intel + cha;
	if( sum > 300 ) {
		result = 0x02;
		goto fail_return;
	}

	if (str < 50 || sta < 50 || dex < 50 || intel < 50 || cha < 50) {
		result = 0x11;
		goto fail_return;		
	}

	SetByte( send_buff, WIZ_NEW_CHAR, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetKOString( send_buff, m_strAccountID, send_index );
	SetByte( send_buff, charindex, send_index );
	SetKOString(send_buff, charid, send_index);
	SetByte( send_buff, race, send_index );
	SetShort( send_buff, Class, send_index );
	SetByte( send_buff, face, send_index );
	SetDWORD( send_buff, hair, send_index );
	SetByte( send_buff, str, send_index );
	SetByte( send_buff, sta, send_index );
	SetByte( send_buff, dex, send_index );
	SetByte( send_buff, intel, send_index );
	SetByte( send_buff, cha, send_index );
	
	retvalue = m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );
	if (retvalue < SMQ_FULL)
		return;

	DEBUG_LOG("NewChar Send Fail : %d", retvalue);

fail_return:
	send_index = 0;
	SetByte( send_buff, WIZ_NEW_CHAR, send_index );
	SetByte( send_buff, result, send_index );
	Send( send_buff, send_index );
}

void CUser::DelCharToAgent(char *pBuf)
{
	int index = 0, idlen = 0, send_index = 0, retvalue = 0;
	int charindex = 0, soclen = 0;
	char charid[MAX_ID_SIZE+1];
	char socno[15];
	memset( charid, NULL, MAX_ID_SIZE+1 );
	memset( socno, NULL, 15 );
	char send_buff[256];
	memset( send_buff, NULL, 256);

	charindex = GetByte( pBuf, index );
	if( charindex > 4 )	goto fail_return;
	idlen = GetShort( pBuf, index );
	if( idlen > MAX_ID_SIZE || idlen <= 0 )	goto fail_return;
	GetString( charid, pBuf, idlen, index );
	soclen = GetShort( pBuf, index );
	// sungyong tw
	//if( soclen != 14 ) goto fail_return;
	if( soclen > 14 || soclen <= 0 ) goto fail_return;
	// ~sungyong tw
	GetString( socno, pBuf, soclen, index );

	if( m_pUserData->m_bKnights > 0 && m_pUserData->m_bFame == CHIEF)	goto fail_return;	

	SetByte( send_buff, WIZ_DEL_CHAR, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetKOString( send_buff, m_strAccountID, send_index );
	SetByte( send_buff, charindex, send_index );
	SetKOString(send_buff, charid, send_index);
	SetKOString(send_buff, socno, send_index);

	retvalue = m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );
	if (retvalue < SMQ_FULL)
		return;

	DEBUG_LOG("DelChar Send Fail : %d", retvalue);

fail_return:
	send_index = 0;
	SetByte( send_buff, WIZ_DEL_CHAR, send_index );
	SetByte( send_buff, 0x00, send_index );
	SetByte( send_buff, 0xFF, send_index );
	Send( send_buff, send_index );
}



void CUser::RecvDeleteChar( char* pBuf )
{
	int nResult = 0, nLen = 0, index = 0, send_index = 0, char_index = 0, nKnights = 0;
	char strCharID[MAX_ID_SIZE+1];	memset( strCharID, 0x00, MAX_ID_SIZE+1 );
	char send_buff[256];			memset( send_buff, 0x00, 256 );

	nResult = GetByte( pBuf, index );
	char_index = GetByte( pBuf, index );
	nKnights = GetShort( pBuf, index );
	nLen = GetShort( pBuf, index );
	GetString( strCharID, pBuf, nLen, index );

	if( nResult == 1 && nKnights != 0 )	{
		m_pMain->m_KnightsManager.RemoveKnightsUser( nKnights, strCharID );
		TRACE("RecvDeleteChar ==> name=%s, knights=%d\n", strCharID, nKnights );

		memset( send_buff, 0x00, 128 );		send_index = 0;
		SetByte( send_buff, UDP_KNIGHTS_PROCESS, send_index );
		SetByte( send_buff, KNIGHTS_WITHDRAW, send_index );
		SetShort( send_buff, nKnights, send_index );
		SetKOString(send_buff, strCharID, send_index);
		if( m_pMain->m_nServerGroup == 0 )
			m_pMain->Send_UDP_All( send_buff, send_index );
		else
			m_pMain->Send_UDP_All( send_buff, send_index, 1 );
	}

	memset( send_buff, 0x00, 128 );		send_index = 0;
	SetByte( send_buff, WIZ_DEL_CHAR, send_index );
	SetByte( send_buff, nResult, send_index );	
	SetByte( send_buff, char_index, send_index );

	Send( send_buff, send_index );
}

void CUser::SelNationToAgent(char *pBuf)
{
	int index = 0, send_index = 0, retvalue = 0;
	int nation = 0;
	char send_buff[256];
	memset( send_buff, NULL, 256);

	nation = GetByte( pBuf, index );
	if( nation > 2 )
		goto fail_return;

	SetByte( send_buff, WIZ_SEL_NATION, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetKOString(send_buff, m_strAccountID, send_index);
	SetByte( send_buff, nation, send_index );

	retvalue = m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );
	if (retvalue < SMQ_FULL)
		return;

	DEBUG_LOG("Nation Sel Send Fail : %d", retvalue);

fail_return:
	send_index = 0;
	SetByte( send_buff, WIZ_SEL_NATION, send_index );
	SetByte( send_buff, 0x00, send_index );
	Send( send_buff, send_index );
}

void CUser::SelCharToAgent(char *pBuf)
{
	int index = 0, send_index = 0, retvalue = 0;
	char userid[MAX_ID_SIZE+1], accountid[MAX_ID_SIZE+1];
	memset( userid, NULL, MAX_ID_SIZE+1 );
	memset( accountid, NULL, MAX_ID_SIZE+1 );
	char send_buff[256];
	memset( send_buff, NULL, 256);
	CUser* pUser = NULL;
	CTime t = CTime::GetCurrentTime();
	BYTE	bInit = 0x01;

	if (!GetKOString(pBuf, accountid, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, userid, index, MAX_ID_SIZE))
		goto fail_return;

	bInit = GetByte( pBuf, index );
	
	if( _strnicmp( accountid, m_strAccountID, MAX_ID_SIZE ) != 0 ) {
		Close();
		return;
	}

	pUser = m_pMain->GetUserPtr( userid, 0x02 );
	if( pUser && (pUser->GetSocketID() != GetSocketID()) ) {
		pUser->Close();
		goto fail_return;
	}

	SetByte( send_buff, WIZ_SEL_CHAR, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetKOString(send_buff, m_strAccountID, send_index);
	SetKOString(send_buff, userid, send_index);
	SetByte( send_buff, bInit, send_index );

	m_pMain->WriteLog("[SelCharToAgent : %d:%d:%d] - acname=%s, name=%s, TH: %lu, Rear : %d\r\n", t.GetHour(), t.GetMinute(), t.GetSecond(), m_strAccountID, userid, GetCurrentThreadId(), m_pMain->m_LoggerSendQueue.GetRearPointer());

	retvalue = m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );
	if (retvalue < SMQ_FULL)
		return;

	DEBUG_LOG("SelChar Send Fail : %d", retvalue);

fail_return:
	send_index = 0;
	SetByte( send_buff, WIZ_SEL_CHAR, send_index );
	SetByte( send_buff, 0x00, send_index );
	Send( send_buff, send_index );
}

void CUser::SelectCharacter(char *pBuf)
{
	int index = 0, send_index = 0, zoneindex = -1, retvalue = 0;
	char send_buff[MAX_SEND_SIZE];
	memset(send_buff, NULL, sizeof(send_buff));
	BYTE result, bInit;
	C3DMap* pMap = NULL;
	_ZONE_SERVERINFO *pInfo	= NULL;
	CKnights* pKnights = NULL;

	result = GetByte( pBuf, index );
	bInit = GetByte( pBuf, index );

	if (result == 0 || !getZoneID()) goto fail_return;

	pMap = m_pMap = m_pMain->GetZoneByID(getZoneID());
	if (pMap == NULL)
		goto fail_return;

	if( m_pMain->m_nServerNo != pMap->m_nServerNo ) {
		pInfo = m_pMain->m_ServerArray.GetData( pMap->m_nServerNo );
		if( !pInfo ) 
			goto fail_return;

		SetByte( send_buff, WIZ_SERVER_CHANGE, send_index );
		SetShort( send_buff, strlen( pInfo->strServerIP ), send_index );
		SetKOString(send_buff, pInfo->strServerIP, send_index);
		SetByte( send_buff, bInit, send_index );
		SetByte( send_buff, m_pUserData->m_bZone, send_index );
		SetByte( send_buff, m_pMain->m_byOldVictory, send_index );
		Send( send_buff, send_index );
		return;
	}

	if( m_pUserData->m_bAuthority == 0xff ) {
		Close();
		return;
	}

	if( m_pMain->m_byBattleOpen == NO_BATTLE && m_pUserData->m_bFame == COMMAND_CAPTAIN )	{
		m_pUserData->m_bFame = CHIEF;
	}

	if(m_pUserData->m_bZone != m_pUserData->m_bNation && m_pUserData->m_bZone < 3 && !m_pMain->m_byBattleOpen) {
		NativeZoneReturn();
		Close();
		return;
	}

	if(m_pUserData->m_bZone == ZONE_BATTLE && ( m_pMain->m_byBattleOpen != NATION_BATTLE) ) {
		NativeZoneReturn();
		Close();
		return;
	}
	if(m_pUserData->m_bZone == ZONE_SNOW_BATTLE && ( m_pMain->m_byBattleOpen != SNOW_BATTLE) ) {
		NativeZoneReturn();
		Close();
		return;
	}
	
	if(m_pUserData->m_bZone == ZONE_FRONTIER && m_pMain->m_byBattleOpen) {
		NativeZoneReturn();
		Close();
		return;
	}
//
	SetLogInInfoToDB(bInit);	// Write User Login Info To DB for Kicking out or Billing

	SetByte( send_buff, WIZ_SEL_CHAR, send_index );
	SetByte( send_buff, result, send_index );
	SetByte( send_buff, getZoneID(), send_index );
	SetShort( send_buff, (WORD)m_pUserData->m_curx*10, send_index );
	SetShort( send_buff, (WORD)m_pUserData->m_curz*10, send_index );
	SetShort( send_buff, (short)m_pUserData->m_cury*10, send_index );
	SetByte( send_buff, m_pMain->m_byOldVictory, send_index );

	m_bSelectedCharacter = true;
	Send( send_buff, send_index );


	SetDetailData();

	//TRACE("SelectCharacter 111 - id=%s, knights=%d, fame=%d\n", m_pUserData->m_id, m_pUserData->m_bKnights, m_pUserData->m_bFame);

	if( m_pUserData->m_bZone > 2)	
	{
		if( m_pUserData->m_bKnights == -1)	{	// ???? ???
			m_pUserData->m_bKnights = 0;
			m_pUserData->m_bFame = 0;
			//TRACE("SelectCharacter - id=%s, knights=%d, fame=%d\n", m_pUserData->m_id, m_pUserData->m_bKnights, m_pUserData->m_bFame);
			return;
		}
		else if( m_pUserData->m_bKnights != 0 )	{
			pKnights = m_pMain->m_KnightsArray.GetData( m_pUserData->m_bKnights );
			if( pKnights )	{
				m_pMain->m_KnightsManager.SetKnightsUser( m_pUserData->m_bKnights, m_pUserData->m_id );
			}
			else	{
				//TRACE("SelectCharacter - ???? ????T ??û,, id=%s, knights=%d, fame=%d\n", m_pUserData->m_id, m_pUserData->m_bKnights, m_pUserData->m_bFame);
				memset( send_buff, 0x00, 256);	send_index = 0;
				SetByte( send_buff, WIZ_KNIGHTS_PROCESS, send_index );
				SetByte( send_buff, KNIGHTS_LIST_REQ+0x10, send_index );
				SetShort( send_buff, GetSocketID(), send_index );
				SetShort( send_buff, m_pUserData->m_bKnights, send_index );
				retvalue = m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );
				if (retvalue >= SMQ_FULL)
					DEBUG_LOG("KNIGHTS_LIST_REQ Packet Drop!!!");

				pKnights = m_pMain->m_KnightsArray.GetData( m_pUserData->m_bKnights );
				if( pKnights )	{
					//TRACE("SelectCharacter - ???? ????T ???,, id=%s, knights=%d, fame=%d\n", m_pUserData->m_id, m_pUserData->m_bKnights, m_pUserData->m_bFame);
					m_pMain->m_KnightsManager.SetKnightsUser( m_pUserData->m_bKnights, m_pUserData->m_id );
				}
			}
		}
	}
	else	{	
		if( m_pUserData->m_bKnights == -1)	{	// ???? ???
			m_pUserData->m_bKnights = 0;
			m_pUserData->m_bFame = 0;
			//TRACE("SelectCharacter - id=%s, knights=%d, fame=%d\n", m_pUserData->m_id, m_pUserData->m_bKnights, m_pUserData->m_bFame);
			return;
		}
		else if( m_pUserData->m_bKnights != 0 )	{
			pKnights = m_pMain->m_KnightsArray.GetData( m_pUserData->m_bKnights );
			if( pKnights )	{
				m_pMain->m_KnightsManager.SetKnightsUser( m_pUserData->m_bKnights, m_pUserData->m_id );
			}
			else {			// ?????? ?i???? ??????.. 
				m_pUserData->m_bKnights = 0;
				m_pUserData->m_bFame = 0;
			}
		}
	}

	//TRACE("SelectCharacter - id=%s, knights=%d, fame=%d\n", m_pUserData->m_id, m_pUserData->m_bKnights, m_pUserData->m_bFame);
	return;

fail_return:
	SetByte( send_buff, WIZ_SEL_CHAR, send_index );
	SetByte( send_buff, 0x00, send_index );
	Send( send_buff, send_index );
}

void CUser::AllCharInfoToAgent()
{
	int send_index = 0, retvalue = 0;
	char send_buff[256];
	memset( send_buff, NULL, 256);

	SetByte( send_buff, WIZ_ALLCHAR_INFO_REQ, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetKOString(send_buff, m_strAccountID, send_index);

	retvalue = m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );
	if( retvalue >= SMQ_FULL ) {
		memset( send_buff, NULL, 256); send_index = 0;
		SetByte( send_buff, WIZ_ALLCHAR_INFO_REQ, send_index );
		SetByte( send_buff, 0xFF, send_index );
		
		DEBUG_LOG("All CharInfo Send Fail : %d", retvalue);
	}
}

// happens on character selection
void CUser::SetLogInInfoToDB(BYTE bInit)
{
	int index = 0, send_index = 0, retvalue = 0, addrlen = 20;
	char send_buff[256], strClientIP[20];
	memset( send_buff, NULL, 256); memset( strClientIP, 0x00, 20 );
	_ZONE_SERVERINFO *pInfo	= NULL;
	struct sockaddr_in addr;

	pInfo = m_pMain->m_ServerArray.GetData( m_pMain->m_nServerNo );
	if( !pInfo ) {
		CString logstr;
		logstr.Format("%d Server Info Invalid User Closed...\r\n", m_pMain->m_nServerNo );
		LogFileWrite( logstr );
		Close();
	}

	getpeername(m_Socket, (struct sockaddr*)&addr, &addrlen );
	strcpy( strClientIP, inet_ntoa(addr.sin_addr) );

	SetByte(send_buff, WIZ_LOGIN_INFO, send_index);

	SetShort(send_buff, m_Sid, send_index);
	SetKOString(send_buff, m_strAccountID, send_index);
	SetKOString(send_buff, m_pUserData->m_id, send_index);
	SetKOString(send_buff, pInfo->strServerIP, send_index);
	SetShort(send_buff, pInfo->sPort, send_index);
	SetKOString(send_buff, strClientIP, send_index);
	SetByte(send_buff, bInit, send_index);

	retvalue = m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );
	if( retvalue >= SMQ_FULL ) {
		char logstr[256]; memset( logstr, 0x00, 256 );
		sprintf( logstr, "UserInfo Send Fail : %d", retvalue);
		m_pMain->m_StatusList.AddString(logstr);
	}
}

void CUser::GameStart(char *pBuf)
{
	int index = 0;
	BYTE opcode = GetByte(pBuf, index);

	if (opcode == 1)
	{
		m_pMain->UserInOutForMe(this);
		m_pMain->NpcInOutForMe(this);
		SendNotice();
		SendTimeStatus();

		// SendHackToolList();

		char send_buff[] = { WIZ_GAMESTART };
		Send(send_buff, sizeof(send_buff));
	}
	else if (opcode == 2)
	{
		m_State = STATE_GAMESTART;
		UserInOut(USER_REGENE);

		if (!m_pUserData->m_bCity && m_pUserData->m_sHp <= 0)
			m_pUserData->m_bCity = -1;

		if (m_pUserData->m_bCity > 0)
		{
			int level = m_pUserData->m_bLevel;
			if (m_pUserData->m_bCity <= 100)
				level--;

			// make sure we don't exceed bounds
			if (level > MAX_LEVEL)
				level = MAX_LEVEL;
			else if (level < 1)
				level = 1;

			m_iLostExp = (m_pMain->GetExpByLevel(level) * (m_pUserData->m_bCity % 10) / 100);
			if (((m_pUserData->m_bCity % 10) / 100) == 1)
				m_iLostExp /= 2;
		}
		else
		{
			m_iLostExp = 0;
		}

		SendMyInfo();
		BlinkStart();
		SetUserAbility();
		// rental
		// ItemMallMagicRecast();

	}
}