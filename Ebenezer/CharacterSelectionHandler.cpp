#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"

void CUser::NewCharToAgent(char *pBuf)
{
	Packet result(WIZ_NEW_CHAR);
	int index = 0, hair;
	uint16 Class;
	uint8 charindex, race, face, str, sta, dex, intel, cha, errorCode;
	char charid[MAX_ID_SIZE+1];

	charindex = GetByte( pBuf, index );
	if (!GetKOString(pBuf, charid, index, MAX_ID_SIZE))
	{
		errorCode = NEWCHAR_INVALID_NAME;
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

	if (charindex > 2)
	{
		errorCode = NEWCHAR_NO_MORE;
		goto fail_return;
	}

	if (!IsValidName(charid))
	{
		result = NEWCHAR_INVALID_NAME;
		goto fail_return;
	}

	_CLASS_COEFFICIENT* p_TableCoefficient = m_pMain->m_CoefficientArray.GetData(Class);
	if (p_TableCoefficient == NULL
		|| (str + sta + dex + intel + cha) > 300) 
	{
		errorCode = NEWCHAR_INVALID_DETAILS;
		goto fail_return;
	}

	if (str < 50 || sta < 50 || dex < 50 || intel < 50 || cha < 50) 
	{
		errorCode = NEWCHAR_STAT_TOO_LOW;
		goto fail_return;		
	}

	// Send packet to Aujard
	result	<< uint16(GetSocketID())
			<< m_strAccountID
			<< charindex << charid << race << Class << face << hair
			<< str << sta << dex << intel << cha;
	
	int retValue = m_pMain->m_LoggerSendQueue.PutData(&result);
	if (retValue < SMQ_FULL)
		return;

	DEBUG_LOG("NewChar Send Fail : %d", retValue);

fail_return:
	result << errorCode;
	Send(&result);
}

void CUser::DelCharToAgent(char *pBuf)
{
	Packet result(WIZ_DEL_CHAR);
	int index = 0;
	char charid[MAX_ID_SIZE+1], socno[15];

	uint8 charindex = GetByte( pBuf, index );
	if (charindex > 2
		|| !GetKOString(pBuf, charid, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, socno, index, sizeof(socno) - 1))
		goto fail_return;

	if (isClanLeader())	
		goto fail_return;	

	// Send packet to Aujard
	result	<< uint16(GetSocketID())
			<< m_strAccountID << charindex << charid << socno;

	int retValue = m_pMain->m_LoggerSendQueue.PutData(&result);
	if (retValue < SMQ_FULL)
		return;

	DEBUG_LOG("DelChar Send Fail : %d", retValue);

fail_return:
	result << uint8(0) << uint8(-1);
	Send(&result);
}

void CUser::RecvDeleteChar( char* pBuf )
{
	Packet result;
	int index = 0, sKnights = 0;
	char strCharID[MAX_ID_SIZE+1];
	uint8 bResult, bCharIndex;

	bResult = GetByte(pBuf, index);
	bCharIndex = GetByte(pBuf, index);
	sKnights = GetShort(pBuf, index);
	if (!GetKOString(pBuf, strCharID, index, MAX_ID_SIZE))
		return;

	if (bResult == 1 && sKnights != 0)
	{
		// TO-DO: Synchronise this system better. Much better. This is dumb.
		m_pMain->m_KnightsManager.RemoveKnightsUser(sKnights, strCharID);
		result.SetOpcode(UDP_KNIGHTS_PROCESS);
		result << uint8(KNIGHTS_WITHDRAW) << sKnights << strCharID;
		m_pMain->Send_UDP_All(&result, m_pMain->m_nServerGroup == 0 ? 0 : 1);
	}


	result.Initialize(WIZ_DEL_CHAR);
	result << bResult << bCharIndex;
	Send(&result);
}

void CUser::SelNationToAgent(char *pBuf)
{
	Packet result(WIZ_SEL_NATION);
	int index = 0;
	uint8 nation = GetByte(pBuf, index);
	if (nation != KARUS && nation != ELMORAD)
	{
		result << uint8(0);
		Send(&result);
		return;
	}

	result << uint16(GetSocketID()) << m_strAccountID << nation; 
	m_pMain->m_LoggerSendQueue.PutData(&result);
}

void CUser::SelCharToAgent(char *pBuf)
{
	Packet result(WIZ_SEL_CHAR);
	int index = 0;
	char userid[MAX_ID_SIZE+1], accountid[MAX_ID_SIZE+1];
	CTime t = CTime::GetCurrentTime();
	BYTE	bInit = 0x01;

	if (!GetKOString(pBuf, accountid, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, userid, index, MAX_ID_SIZE)
		|| _strnicmp( accountid, m_strAccountID, MAX_ID_SIZE) != 0)
	{
		Close();
		return;
	}

	bInit = GetByte(pBuf, index);

	// Disconnect any currently logged in sessions.
	CUser *pUser = m_pMain->GetUserPtr(userid, TYPE_CHARACTER);
	if (pUser && (pUser->GetSocketID() != GetSocketID()))
	{
		pUser->Close();

		// And reject the login attempt (otherwise we'll probably desync char data)
		result << uint8(0);
		Send(&result);
		return;
	}

	result << uint16(GetSocketID()) << m_strAccountID << userid << bInit;
	m_pMain->m_LoggerSendQueue.PutData(&result);
}

void CUser::SelectCharacter(char *pBuf)
{
	Packet result(WIZ_SEL_CHAR);
	int index = 0;
	
	BYTE bResult, bInit;
	C3DMap* pMap = NULL;
	_ZONE_SERVERINFO *pInfo	= NULL;

	bResult = GetByte( pBuf, index );
	bInit = GetByte( pBuf, index );
	result << bResult;
	if (bResult == 0 || !getZoneID()) 
		goto fail_return;

	pMap = m_pMap = m_pMain->GetZoneByID(getZoneID());
	if (pMap == NULL)
		goto fail_return;

	if (m_pMain->m_nServerNo != pMap->m_nServerNo)
	{
		pInfo = m_pMain->m_ServerArray.GetData( pMap->m_nServerNo );
		if (pInfo == NULL) 
			goto fail_return;

		SendServerChange(pInfo->strServerIP, bInit);
		return;
	}

	if (m_pUserData->m_bAuthority == 0xff)
	{
		Close();
		return;
	}

	if (m_pMain->m_byBattleOpen == NO_BATTLE && m_pUserData->m_bFame == COMMAND_CAPTAIN)
		m_pUserData->m_bFame = CHIEF;

	if ((getZoneID() != getNation() && getZoneID() < 3 && !m_pMain->m_byBattleOpen)
		|| (getZoneID() == ZONE_BATTLE && (m_pMain->m_byBattleOpen != NATION_BATTLE))
		|| (getZoneID() == ZONE_SNOW_BATTLE && (m_pMain->m_byBattleOpen != SNOW_BATTLE))
		|| (getZoneID() == ZONE_FRONTIER && m_pMain->m_byBattleOpen))
	{
		NativeZoneReturn();
		Close();
		return;
	}

	SetLogInInfoToDB(bInit);

	result << getZoneID() << GetSPosX() << GetSPosZ() << GetSPosY() << m_pMain->m_byOldVictory;
	m_bSelectedCharacter = true;
	Send(&result);

	SetDetailData();

	if (m_pUserData->m_bKnights == -1)
	{
		m_pUserData->m_bKnights = m_pUserData->m_bFame = 0;
		return;
	}
	else if (m_pUserData->m_bKnights != 0)
	{
		CKnights* pKnights = m_pMain->m_KnightsArray.GetData( m_pUserData->m_bKnights );
		if (pKnights != NULL)
		{
			m_pMain->m_KnightsManager.SetKnightsUser( m_pUserData->m_bKnights, m_pUserData->m_id );
		}
		else if (getZoneID() > 2)
		{
			result.Initialize(WIZ_KNIGHTS_PROCESS);
			result << uint8(KNIGHTS_LIST_REQ) << uint16(GetSocketID()) << m_pUserData->m_bKnights;
			m_pMain->m_LoggerSendQueue.PutData(&result);
		}
	}
	return;

fail_return:
	Send(&result);
}

void CUser::SendServerChange(char *ip, uint8 bInit)
{
	Packet result(WIZ_SERVER_CHANGE);
	result << ip << uint16(_LISTEN_PORT) << bInit << getZoneID() << m_pMain->m_byOldVictory;
	Send(&result);
}

void CUser::AllCharInfoToAgent()
{
	Packet result(WIZ_ALLCHAR_INFO_REQ);
	result << uint16(GetSocketID()) << m_strAccountID; 
	m_pMain->m_LoggerSendQueue.PutData(&result);
}

// happens on character selection
void CUser::SetLogInInfoToDB(BYTE bInit)
{
	int addrlen = 20;
	char strClientIP[20];
	struct sockaddr_in addr;

	_ZONE_SERVERINFO *pInfo = m_pMain->m_ServerArray.GetData(m_pMain->m_nServerNo);
	if (pInfo == NULL) 
	{
		Close();
		return;
	}

	getpeername(m_Socket, (struct sockaddr*)&addr, &addrlen );
	strcpy_s( strClientIP, sizeof(strClientIP),inet_ntoa(addr.sin_addr) );

	Packet result(WIZ_LOGIN_INFO);
	result	<< uint16(GetSocketID()) << m_strAccountID << m_pUserData->m_id 
			<< pInfo->strServerIP << uint16(_LISTEN_PORT) << strClientIP 
			<< bInit;
	m_pMain->m_LoggerSendQueue.PutData(&result);
}

// This packet actually contains the char name after the opcode
void CUser::GameStart(char *pBuf)
{
	int index = 0;
	BYTE opcode = GetByte(pBuf, index);

	if (opcode == 1)
	{
		SendMyInfo();
		m_pMain->UserInOutForMe(this);
		m_pMain->MerchantUserInOutForMe(this);
		m_pMain->NpcInOutForMe(this);
		SendNotice();
		SendTimeStatus();

		// SendHackToolList();

		Packet result(WIZ_GAMESTART);
		Send(&result);
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

		BlinkStart();
		SetUserAbility();
		// rental
		// ItemMallMagicRecast();

	}
}