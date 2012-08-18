#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "Ebenezer.h"
#include "EbenezerDlg.h"
#include "User.h"
#include "AIPacket.h"
#include "PacketDefine.h"

extern BYTE g_serverdown_flag;

void CUser::MoveProcess(char *pBuf )
{
	if( m_bWarp ) return;
		
	int index = 0, send_index = 0, region = 0;
	WORD will_x, will_z;
	short will_y, speed=0;
	float real_x, real_z, real_y;
	BYTE echo;
	char send_buf[1024];
	memset( send_buf, 0x00, 1024 );

	will_x = GetShort( pBuf, index );
	will_z = GetShort( pBuf, index );
	will_y = GetShort( pBuf, index );

	speed = GetShort( pBuf, index );
	echo = GetByte( pBuf, index );

	real_x = will_x/10.0f; real_z = will_z/10.0f; real_y = will_y/10.0f;
	if( m_iZoneIndex < 0 || m_iZoneIndex >= m_pMain->m_ZoneArray.size() ) return;
	if( m_pMain->m_ZoneArray[m_iZoneIndex]->IsValidPosition( real_x, real_z, real_y ) == FALSE ) return;

//	real_y = m_pMain->m_ZoneArray[m_iZoneIndex]->GetHeight(	real_x, real_y, real_z );

//	if( speed > 60 ) {	// client ???? ???g???? u?? ???????? ???...
//		if( m_bSpeedAmount == 100 )
//			speed = 0;
//	}

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

	SetByte( send_buf, WIZ_MOVE, send_index );
	SetShort( send_buf, m_Sid, send_index );
	SetShort( send_buf, will_x, send_index );
	SetShort( send_buf, will_z, send_index );
	SetShort( send_buf, will_y, send_index );
	SetShort( send_buf, speed, send_index );
	SetByte( send_buf, echo, send_index );

	RegisterRegion();
	m_pMain->Send_Region( send_buf, send_index, (int)m_pUserData->m_bZone, m_RegionX, m_RegionZ, NULL, false );

	if( m_iZoneIndex >= 0 && m_iZoneIndex < m_pMain->m_ZoneArray.size() ) 
		m_pMain->m_ZoneArray[m_iZoneIndex]->CheckEvent( real_x, real_z, this );

	int  ai_send_index = 0;
	char ai_send_buff[256];
	memset( ai_send_buff, NULL, 256);

	SetByte( ai_send_buff, AG_USER_MOVE, ai_send_index );
	SetShort( ai_send_buff, m_Sid, ai_send_index );
	Setfloat( ai_send_buff, m_fWill_x, ai_send_index );
	Setfloat( ai_send_buff, m_fWill_z, ai_send_index );
	Setfloat( ai_send_buff, m_fWill_y, ai_send_index );
	SetShort( ai_send_buff, speed, ai_send_index );
	
	m_pMain->Send_AIServer(m_pUserData->m_bZone, ai_send_buff, ai_send_index);
}

void CUser::UserInOut(BYTE Type)
{
	int send_index = 0, iLength = 0;
	char buff[256];
	memset( buff, 0x00, 256 );
	if( m_iZoneIndex < 0 || m_iZoneIndex >= m_pMain->m_ZoneArray.size() ) return;
	C3DMap *pMap = m_pMain->m_ZoneArray[m_iZoneIndex];
	CKnights* pKnights = NULL;
	pKnights = m_pMain->m_KnightsArray.GetData( m_pUserData->m_bKnights );
	if( !pMap )
		return;

	if( Type == USER_OUT )
		pMap->RegionUserRemove( m_RegionX, m_RegionZ, m_Sid );
	else
		pMap->RegionUserAdd( m_RegionX, m_RegionZ, m_Sid );

	memset( buff, 0x00, 256 );		send_index = 0;
	SetByte( buff, WIZ_USER_INOUT, send_index );
	SetByte( buff, Type, send_index );
	SetShort( buff, m_Sid, send_index );
	if( Type == USER_OUT ) {
		m_pMain->Send_Region( buff, send_index, (int)m_pUserData->m_bZone, m_RegionX, m_RegionZ, this );

		// AI Server????? ??? ???..
		send_index=0;
		memset( buff, 0x00, 256 );
		SetByte( buff, AG_USER_INOUT, send_index );
		SetByte( buff, Type, send_index );
		SetShort( buff, m_Sid, send_index );
		SetShort( buff, strlen(m_pUserData->m_id), send_index );
		SetString( buff, m_pUserData->m_id, strlen(m_pUserData->m_id), send_index );
		Setfloat( buff, m_pUserData->m_curx, send_index );
		Setfloat( buff, m_pUserData->m_curz, send_index );
		m_pMain->Send_AIServer( m_pUserData->m_bZone, buff, send_index);
		return;
	}
	SetShort( buff, strlen(m_pUserData->m_id), send_index );
	SetString( buff, m_pUserData->m_id, strlen(m_pUserData->m_id), send_index );
	SetByte( buff, m_pUserData->m_bNation, send_index );
	SetShort( buff, m_pUserData->m_bKnights, send_index );
	SetByte( buff, m_pUserData->m_bFame, send_index );
	if( m_pUserData->m_bKnights == 0 )	{
		SetShort( buff, 0, send_index );
		SetByte( buff, 0, send_index );
		SetByte( buff, 0, send_index );
	}
	else {
		//pKnights = m_pMain->m_KnightsArray.GetData( m_pUserData->m_bKnights );
		if( pKnights )	{
			iLength = strlen( pKnights->m_strName );
			SetShort( buff, (short)iLength, send_index );
			SetString( buff, pKnights->m_strName, iLength, send_index );
			SetByte( buff, pKnights->m_byGrade, send_index );  // knights grade
			SetByte( buff, pKnights->m_byRanking, send_index );  // knights grade
			//TRACE("userinout knights index = %d, kname=%s, name=%s\n" , iLength, pKnights->strName, m_pUserData->m_id);
		}
		else	{
			SetShort( buff, 0, send_index );
			SetByte( buff, 0, send_index );
			SetByte( buff, 0, send_index );
		}
	}	
	SetByte( buff, m_pUserData->m_bLevel, send_index );
	SetByte( buff, m_pUserData->m_bRace, send_index );
	SetShort( buff, m_pUserData->m_sClass, send_index );
	SetShort( buff, (WORD)m_pUserData->m_curx*10, send_index );
	SetShort( buff, (WORD)m_pUserData->m_curz*10, send_index );
	SetShort( buff, (short)m_pUserData->m_cury*10, send_index );
	SetByte( buff, m_pUserData->m_bFace, send_index );
	SetByte( buff, m_pUserData->m_bHairColor, send_index );
	SetByte( buff, m_bResHpType, send_index );
// ????? ???...
	SetByte( buff, m_bAbnormalType, send_index );
//
	SetByte( buff, m_bNeedParty, send_index );
	SetByte( buff, m_pUserData->m_bAuthority, send_index );
	SetDWORD( buff, m_pUserData->m_sItemArray[BREAST].nNum, send_index );
	SetShort( buff, m_pUserData->m_sItemArray[BREAST].sDuration, send_index );
	SetDWORD( buff, m_pUserData->m_sItemArray[LEG].nNum, send_index );
	SetShort( buff, m_pUserData->m_sItemArray[LEG].sDuration, send_index );
	SetDWORD( buff, m_pUserData->m_sItemArray[HEAD].nNum, send_index );
	SetShort( buff, m_pUserData->m_sItemArray[HEAD].sDuration, send_index );
	SetDWORD( buff, m_pUserData->m_sItemArray[GLOVE].nNum, send_index );
	SetShort( buff, m_pUserData->m_sItemArray[GLOVE].sDuration, send_index );
	SetDWORD( buff, m_pUserData->m_sItemArray[FOOT].nNum, send_index );
	SetShort( buff, m_pUserData->m_sItemArray[FOOT].sDuration, send_index );
	SetDWORD( buff, m_pUserData->m_sItemArray[SHOULDER].nNum, send_index );
	SetShort( buff, m_pUserData->m_sItemArray[SHOULDER].sDuration, send_index );
	SetDWORD( buff, m_pUserData->m_sItemArray[RIGHTHAND].nNum, send_index );
	SetShort( buff, m_pUserData->m_sItemArray[RIGHTHAND].sDuration, send_index );
	SetDWORD( buff, m_pUserData->m_sItemArray[LEFTHAND].nNum, send_index );
	SetShort( buff, m_pUserData->m_sItemArray[LEFTHAND].sDuration, send_index );

//	TRACE("USERINOUT - %d, %s\n", m_Sid, m_pUserData->m_id);
	m_pMain->Send_Region( buff, send_index, (int)m_pUserData->m_bZone, m_RegionX, m_RegionZ, this );

	// AI Server????? ??? ???..
// ??? ???? ?'? ??????? ?????? ??.??
	if (m_bAbnormalType != ABNORMAL_BLINKING) {
		send_index=0;
		memset( buff, 0x00, 256 );
		SetByte( buff, AG_USER_INOUT, send_index );
		SetByte( buff, Type, send_index );
		SetShort( buff, m_Sid, send_index );
		SetShort( buff, strlen(m_pUserData->m_id), send_index );
		SetString( buff, m_pUserData->m_id, strlen(m_pUserData->m_id), send_index );
		Setfloat( buff, m_pUserData->m_curx, send_index );
		Setfloat( buff, m_pUserData->m_curz, send_index );
		m_pMain->Send_AIServer( m_pUserData->m_bZone, buff, send_index);
	}
//
}

void CUser::Rotate( char* pBuf )
{
	int index = 0, send_index = 0;
	int uid = -1;
	BYTE type = 0x00;
	char buff[256];
	memset( buff, NULL, 256 );
	short dir;

	dir = GetShort( pBuf, index );

	SetByte( buff, WIZ_ROTATE, send_index );
	SetShort( buff, m_Sid, send_index );
	SetShort( buff, dir, send_index );

	m_pMain->Send_Region( buff, send_index, (int)m_pUserData->m_bZone, m_RegionX, m_RegionZ, NULL, false );
}

void CUser::ZoneChange(int zone, float x, float z)
{
	m_bZoneChangeFlag = TRUE;

	int send_index = 0, zoneindex = 0;
	char send_buff[128];
	memset( send_buff, NULL, 128 );
	C3DMap* pMap = NULL;
	_ZONE_SERVERINFO *pInfo = NULL;

	if( g_serverdown_flag ) return;

	zoneindex = m_pMain->GetZoneIndex( zone );
	if( zoneindex < 0 || zoneindex >= m_pMain->m_ZoneArray.size() ) return;
	pMap = (C3DMap*)m_pMain->m_ZoneArray[zoneindex];
	if( !pMap ) return;

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
//	????? ??? ???? ????....
			int temp_index = 0;
			char temp_buff[128];
			memset( temp_buff, NULL, 128 );

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

	m_iZoneIndex = zoneindex;
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

		SetByte( send_buff, WIZ_SERVER_CHANGE, send_index );
		SetShort( send_buff, strlen( pInfo->strServerIP ), send_index );
		SetString( send_buff, pInfo->strServerIP, strlen( pInfo->strServerIP ), send_index );
		SetShort( send_buff, pInfo->sPort, send_index );
		SetByte( send_buff, 0x02, send_index );				// ????? ???? ???? ???...
		SetByte( send_buff, m_pUserData->m_bZone, send_index );
		SetByte( send_buff, m_pMain->m_byOldVictory, send_index );
		Send( send_buff, send_index );
		return;
	}
	
	m_pUserData->m_sBind = -1;		// Bind Point Clear...
	
	m_RegionX = (int)(m_pUserData->m_curx / VIEW_DISTANCE);
	m_RegionZ = (int)(m_pUserData->m_curz / VIEW_DISTANCE);

	SetByte( send_buff, WIZ_ZONE_CHANGE, send_index );
	SetByte( send_buff, m_pUserData->m_bZone, send_index );
	SetShort( send_buff, (WORD)m_pUserData->m_curx*10, send_index );
	SetShort( send_buff, (WORD)m_pUserData->m_curz*10, send_index );
	SetShort( send_buff, (short)m_pUserData->m_cury*10, send_index );
	SetByte( send_buff, m_pMain->m_byOldVictory, send_index );
	Send( send_buff, send_index );
// ????? ????? >.<
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
//
	int  ai_send_index = 0;
	char ai_send_buff[256];
	memset( ai_send_buff, NULL, 256);

	SetByte( ai_send_buff, AG_ZONE_CHANGE, ai_send_index );
	SetShort( ai_send_buff, m_Sid, ai_send_index );
	SetByte( ai_send_buff, (BYTE)m_iZoneIndex, ai_send_index );
	SetByte( ai_send_buff, m_pUserData->m_bZone, ai_send_index );

	m_pMain->Send_AIServer(m_pUserData->m_bZone, ai_send_buff, ai_send_index);

	m_bZoneChangeFlag = FALSE;
}

void CUser::Warp(char *pBuf)
{
	if( m_bWarp ) return;
//	if( m_pUserData->m_bAuthority != 0 ) return;

	int index = 0, send_index = 0;
	WORD warp_x, warp_z;
	float real_x, real_z;
	char	send_buff[128];
	memset( send_buff, NULL, 128 );
	C3DMap* pMap = NULL;

	warp_x = GetShort( pBuf, index );
	warp_z = GetShort( pBuf, index );

	if( m_iZoneIndex < 0 || m_iZoneIndex >= m_pMain->m_ZoneArray.size() ) return;
	pMap = (C3DMap*)m_pMain->m_ZoneArray[m_iZoneIndex];
	if( !pMap )	return;

	real_x = warp_x/10.0f; real_z = warp_z/10.0f;
	if( !pMap->IsValidPosition( real_x, real_z, 0.0f ) ) return;

	SetByte( send_buff, WIZ_WARP, send_index );
	SetShort( send_buff, warp_x, send_index );
	SetShort( send_buff, warp_z, send_index );
	Send( send_buff, send_index );

	UserInOut( USER_OUT );

	m_pUserData->m_curx = m_fWill_x = real_x;
	m_pUserData->m_curz = m_fWill_z = real_z;

	m_RegionX = (int)(m_pUserData->m_curx / VIEW_DISTANCE);
	m_RegionZ = (int)(m_pUserData->m_curz / VIEW_DISTANCE);

	//TRACE(" Warp ,, name=%s, x=%.2f, z=%.2f\n", m_pUserData->m_id, m_pUserData->m_curx, m_pUserData->m_curz);

	//UserInOut( USER_IN );
	UserInOut( USER_WARP );
	m_pMain->RegionUserInOutForMe(this);
	m_pMain->RegionNpcInfoForMe(this);
}