#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"
#include "AIPacket.h"

extern BYTE g_serverdown_flag;

void CUser::MoveProcess(char *pBuf )
{
	ASSERT(GetMap() != NULL);
	if( m_bWarp ) return;
		
	int index = 0, region = 0;
	WORD will_x, will_z;
	short will_y, speed=0;
	float real_x, real_z, real_y;
	BYTE echo;

	will_x = GetShort( pBuf, index );
	will_z = GetShort( pBuf, index );
	will_y = GetShort( pBuf, index );

	speed = GetShort( pBuf, index );
	echo = GetByte( pBuf, index );

	real_x = will_x/10.0f; real_z = will_z/10.0f; real_y = will_y/10.0f;

	if (GetMap()->IsValidPosition(real_x, real_z, real_y) == FALSE) 
		return;

	if (isDead() && speed != 0)
		TRACE("### MoveProcess Fail : name=%s(%d), m_bResHpType=%d, hp=%d, speed=%d, x=%d, z=%d ###\n", m_pUserData->m_id, m_Sid, m_bResHpType, m_pUserData->m_sHp, speed, (int)m_pUserData->m_curx, (int)m_pUserData->m_curz);

	if( speed != 0 ) {
		m_pUserData->m_curx = m_fWill_x;	// ????? ??? ?????g?? ??????g?? ????...
		m_pUserData->m_curz = m_fWill_z;
		m_pUserData->m_cury = m_fWill_y;

		m_fWill_x = will_x/10.0f;	// ?????g?? ???....
		m_fWill_z = will_z/10.0f;
		m_fWill_y = will_y/10.0f;
	}
	else {
		m_pUserData->m_curx = m_fWill_x = will_x/10.0f;	// ?????g == ???? ??g...
		m_pUserData->m_curz = m_fWill_z = will_z/10.0f;
		m_pUserData->m_cury = m_fWill_y = will_y/10.0f;
	}

	Packet result(WIZ_MOVE);
	result << uint16(m_Sid)
		<< will_x << will_z << will_y << speed
		<< echo;

	RegisterRegion();
	m_pMain->Send_Region( &result, GetMap(), m_RegionX, m_RegionZ );

	GetMap()->CheckEvent( real_x, real_z, this );

	result.Initialize(AG_USER_MOVE);
	result << uint16(m_Sid) << m_fWill_x << m_fWill_z << m_fWill_y << speed;
	m_pMain->Send_AIServer(&result);
}

void CUser::UserInOut(BYTE Type)
{
	if (GetMap() == NULL)
		return;

	Packet result(WIZ_USER_INOUT);
	result << Type << uint8(0) << uint16(GetSocketID());

	if (Type == USER_OUT)
		GetMap()->RegionUserRemove(m_RegionX, m_RegionZ, GetSocketID());
	else
		GetMap()->RegionUserAdd(m_RegionX, m_RegionZ, GetSocketID());

	if (Type != USER_OUT)
		GetUserInfo(result);

	m_pMain->Send_Region(&result, GetMap(), m_RegionX, m_RegionZ, this );

	if (Type == USER_OUT || m_bAbnormalType != ABNORMAL_BLINKING) 
	{
		result.Initialize(AG_USER_INOUT);
		result << Type << uint16(GetSocketID()) << m_pUserData->m_id << m_pUserData->m_curx << m_pUserData->m_curz;
		m_pMain->Send_AIServer(&result);
	}
}

// TO-DO: Update this. It's VERY dated.
void CUser::GetUserInfo(Packet & pkt)
{
	CKnights *pKnights = NULL;
	pkt.SByte();
	pkt		<< m_pUserData->m_id
			<< uint16(getNation()) << m_pUserData->m_bKnights << uint16(m_pUserData->m_bFame);

	if (isInClan())
		pKnights = m_pMain->m_KnightsArray.GetData(m_pUserData->m_bKnights);

	if (pKnights == NULL)
	{
		// should work out to be 11 bytes, 6-7 being cape ID.
		pkt	<< uint32(0) << uint16(0) << uint16(-1) << uint16(0) << uint8(0);
	}
	else 
	{
		pkt	<< uint8(0) // grade type
				<< pKnights->m_strName
				<< pKnights->m_byGrade << pKnights->m_byRanking
				<< uint16(0) // symbol/mark version
				<< uint16(-1) // cape ID
				<< uint8(0) << uint8(0) << uint8(0); // cape RGB
	}

	pkt	<< getLevel() << m_pUserData->m_bRace << m_pUserData->m_sClass
		<< GetSPosX() << GetSPosZ() << GetSPosY()
		<< m_pUserData->m_bFace << m_pUserData->m_nHair
		<< m_bResHpType << uint32(m_bAbnormalType)
		<< m_bNeedParty
		<< m_pUserData->m_bAuthority
		<< m_pUserData->m_sItemArray[BREAST].nNum << m_pUserData->m_sItemArray[BREAST].sDuration
		<< m_pUserData->m_sItemArray[LEG].nNum << m_pUserData->m_sItemArray[LEG].sDuration
		<< m_pUserData->m_sItemArray[BREAST].nNum << m_pUserData->m_sItemArray[BREAST].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[LEG].nNum << m_pUserData->m_sItemArray[LEG].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[HEAD].nNum << m_pUserData->m_sItemArray[HEAD].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[GLOVE].nNum << m_pUserData->m_sItemArray[GLOVE].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[FOOT].nNum << m_pUserData->m_sItemArray[FOOT].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[SHOULDER].nNum << m_pUserData->m_sItemArray[SHOULDER].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[RIGHTHAND].nNum << m_pUserData->m_sItemArray[RIGHTHAND].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[LEFTHAND].nNum << m_pUserData->m_sItemArray[LEFTHAND].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[CWING].nNum << m_pUserData->m_sItemArray[CWING].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[CTOP].nNum << m_pUserData->m_sItemArray[CTOP].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[CHELMET].nNum << m_pUserData->m_sItemArray[CHELMET].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[CRIGHT].nNum << m_pUserData->m_sItemArray[CRIGHT].sDuration << uint8(0)
		<< m_pUserData->m_sItemArray[CLEFT].nNum << m_pUserData->m_sItemArray[CLEFT].sDuration << uint8(0)
		<< getZoneID() << uint8(-1) << uint8(-1) << uint16(0) << uint16(0) << uint16(0);
}

void CUser::Rotate( char* pBuf )
{
	int index = 0, send_index = 0;
	int uid = -1;
	BYTE type = 0x00;
	char buff[256];
	short dir;

	dir = GetShort( pBuf, index );

	SetByte( buff, WIZ_ROTATE, send_index );
	SetShort( buff, m_Sid, send_index );
	SetShort( buff, dir, send_index );

	m_pMain->Send_Region( buff, send_index, GetMap(), m_RegionX, m_RegionZ );
}

void CUser::ZoneChange(int zone, float x, float z)
{
	m_bZoneChangeFlag = TRUE;

	int send_index = 0, zoneindex = 0;
	char send_buff[128];
	C3DMap* pMap = NULL;
	_ZONE_SERVERINFO *pInfo = NULL;

	if( g_serverdown_flag ) return;

	pMap = m_pMain->GetZoneByID(zone);
	if (!pMap) 
		return;

	m_pMap = pMap;
	if( pMap->m_bType == 2 ) {	// If Target zone is frontier zone.
		if( m_pUserData->m_bLevel < 20 && m_pMain->m_byBattleOpen != SNOW_BATTLE)
			return;
	}

	if( m_pMain->m_byBattleOpen == NATION_BATTLE )	{		// Battle zone open
		if( m_pUserData->m_bZone == BATTLE_ZONE )	{
			if( pMap->m_bType == 1 && m_pUserData->m_bNation != zone )	{	// ???? ?????? ???? ????..
				if( m_pUserData->m_bNation == KARUS && !m_pMain->m_byElmoradOpenFlag )	{
					TRACE("#### ZoneChange Fail ,,, id=%s, nation=%d, flag=%d\n", m_pUserData->m_id, m_pUserData->m_bNation, m_pMain->m_byElmoradOpenFlag);
					return;
				}
				else if( m_pUserData->m_bNation == ELMORAD && !m_pMain->m_byKarusOpenFlag )	{
					TRACE("#### ZoneChange Fail ,,, id=%s, nation=%d, flag=%d\n", m_pUserData->m_id, m_pUserData->m_bNation, m_pMain->m_byKarusOpenFlag);
					return;
				}
			}
		}
		else if( pMap->m_bType == 1 && m_pUserData->m_bNation != zone ) {		// ???? ?????? ???? ????..
			return;
		}
//
		else if( pMap->m_bType == 2 && zone == ZONE_FRONTIER ) {	 // You can't go to frontier zone when Battlezone is open.
			int temp_index = 0;
			char temp_buff[3];

			SetByte( temp_buff, WIZ_WARP_LIST, temp_index );
			SetByte( temp_buff, 2, temp_index );
			SetByte( temp_buff,0, temp_index );
			Send(temp_buff, temp_index);
//
			return;
		}
//
	}
	else if( m_pMain->m_byBattleOpen == SNOW_BATTLE )	{					// Snow Battle zone open
		if( pMap->m_bType == 1 && m_pUserData->m_bNation != zone ) {		// ???? ?????? ???? ????..
			return;
		}
		else if( pMap->m_bType == 2 && (zone == ZONE_FRONTIER || zone == ZONE_BATTLE ) ) {			// You can't go to frontier zone when Battlezone is open.
			return;
		}
	}
	else	{					// Battle zone close
		if( pMap->m_bType == 1 && m_pUserData->m_bNation != zone && (zone < 10 || zone > 20))		// ???? ?????? ???? ????..
			return;
	}

	m_bWarp = 0x01;

	UserInOut( USER_OUT );

	if( m_pUserData->m_bZone == ZONE_SNOW_BATTLE )	{
		//TRACE("ZoneChange - name=%s\n", m_pUserData->m_id);
		SetMaxHp( 1 );
	}

	m_pUserData->m_bZone = zone;
	m_pUserData->m_curx = m_fWill_x = x;
	m_pUserData->m_curz = m_fWill_z = z;

	if( m_pUserData->m_bZone == ZONE_SNOW_BATTLE )	{
		//TRACE("ZoneChange - name=%s\n", m_pUserData->m_id);
		SetMaxHp();
	}

	PartyRemove(m_Sid);	// ??????? Z?????? ó??

	//TRACE("ZoneChange ,,, id=%s, nation=%d, zone=%d, x=%.2f, z=%.2f\n", m_pUserData->m_id, m_pUserData->m_bNation, zone, x, z);
	
	if( m_pMain->m_nServerNo != pMap->m_nServerNo ) {
		pInfo = m_pMain->m_ServerArray.GetData( pMap->m_nServerNo );
		if( !pInfo ) 
			return;

		UserDataSaveToAgent();
		
		CTime t = CTime::GetCurrentTime();
		m_pMain->WriteLog("[ZoneChange : %d-%d-%d] - sid=%d, acname=%s, name=%s, zone=%d, x=%d, z=%d \r\n", t.GetHour(), t.GetMinute(), t.GetSecond(), m_Sid, m_strAccountID, m_pUserData->m_id, zone, (int)x, (int)z);

		m_pUserData->m_bLogout = 2;	// server change flag
		SendServerChange(pInfo->strServerIP, 2);
		return;
	}
	
	m_pUserData->m_sBind = -1;		// Bind Point Clear...
	
	m_RegionX = (int)(m_pUserData->m_curx / VIEW_DISTANCE);
	m_RegionZ = (int)(m_pUserData->m_curz / VIEW_DISTANCE);

	SetByte( send_buff, WIZ_ZONE_CHANGE, send_index );
	SetByte( send_buff, 0x03, send_index );
	SetByte( send_buff, m_pUserData->m_bZone, send_index );
	SetShort( send_buff, (WORD)m_pUserData->m_curx*10, send_index );
	SetShort( send_buff, (WORD)m_pUserData->m_curz*10, send_index );
	SetShort( send_buff, (short)m_pUserData->m_cury*10, send_index );
	SetByte( send_buff, m_pMain->m_byOldVictory, send_index );
	Send( send_buff, send_index );

	if (!m_bZoneChangeSameZone) {
		m_sWhoKilledMe = -1;
		m_iLostExp = 0;
		m_bRegeneType = 0;
		m_fLastRegeneTime = 0.0f;
		m_pUserData->m_sBind = -1;
		InitType3();
		InitType4();
	}	

	if (m_bZoneChangeSameZone) {
		m_bZoneChangeSameZone = FALSE;
	}

	send_index = 0;
	SetByte( send_buff, AG_ZONE_CHANGE, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetByte( send_buff, getZoneID(), send_index );

	m_pMain->Send_AIServer(send_buff, send_index);

	m_bZoneChangeFlag = FALSE;
}

void CUser::Warp(char *pBuf)
{
	ASSERT(GetMap() != NULL);
	if( m_bWarp ) return;

	int index = 0, send_index = 0;
	WORD warp_x, warp_z;
	float real_x, real_z;
	char	send_buff[128];

	warp_x = GetShort( pBuf, index );
	warp_z = GetShort( pBuf, index );

	real_x = warp_x/10.0f; real_z = warp_z/10.0f;
	if (!GetMap()->IsValidPosition(real_x, real_z, 0.0f)) 
	{
		TRACE("Invalid position %f,%f\n", real_x, real_z);
		return;
	}

	SetByte(send_buff, WIZ_WARP, send_index);
	SetShort(send_buff, warp_x, send_index);
	SetShort(send_buff, warp_z, send_index);
	Send(send_buff, send_index);

	UserInOut(USER_OUT);

	m_pUserData->m_curx = m_fWill_x = real_x;
	m_pUserData->m_curz = m_fWill_z = real_z;

	m_RegionX = (int)(m_pUserData->m_curx / VIEW_DISTANCE);
	m_RegionZ = (int)(m_pUserData->m_curz / VIEW_DISTANCE);

	//TRACE(" Warp ,, name=%s, x=%.2f, z=%.2f\n", m_pUserData->m_id, m_pUserData->m_curx, m_pUserData->m_curz);

	UserInOut(USER_WARP);
	m_pMain->UserInOutForMe(this);
	m_pMain->NpcInOutForMe(this);
	m_pMain->MerchantUserInOutForMe(this);

	ResetWindows();
}