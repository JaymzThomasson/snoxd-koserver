#include "stdafx.h"
#include "GameSocket.h"
#include "ServerDlg.h"
#include "User.h"
#include "Map.h"
#include "region.h"
#include "Party.h"
#include "../shared/globals.h"
#include "extern.h"

extern CRITICAL_SECTION g_region_critical;

CGameSocket::~CGameSocket() {}

void CGameSocket::OnConnect()
{
	KOSocket::OnConnect();
}

void CGameSocket::Initialize()
{
	m_Party.Initialize();
}

void CGameSocket::OnDisconnect()
{
	TRACE("*** CloseProcess - socketID=%d ... server=%s *** \n", GetSocketID(), GetRemoteIP().c_str());
	g_pMain.DeleteAllUserList(this);
	Initialize();
}

bool CGameSocket::HandlePacket(Packet & pkt)
{
	switch (pkt.GetOpcode())
	{
	case AI_SERVER_CONNECT:
		RecvServerConnect(pkt);
		break;
	case AG_USER_INFO:
		RecvUserInfo(pkt);
		break;
	case AG_USER_INOUT:
		RecvUserInOut(pkt);
		break;
	case AG_USER_MOVE:
		RecvUserMove(pkt);
		break;
	case AG_USER_MOVEEDGE:
		RecvUserMoveEdge(pkt);
		break;
	case AG_ATTACK_REQ:
		RecvAttackReq(pkt);
		break;
	case AG_USER_LOG_OUT:
		RecvUserLogOut(pkt);
		break;
	case AG_USER_REGENE:
		RecvUserRegene(pkt);
		break;
	case AG_USER_SET_HP:
		RecvUserSetHP(pkt);
		break;
	case AG_USER_UPDATE:
		RecvUserUpdate(pkt);
		break;
	case AG_ZONE_CHANGE:
		RecvZoneChange(pkt);
		break;
	case AG_USER_PARTY:
		m_Party.PartyProcess(pkt);
		break;
	case AG_MAGIC_ATTACK_REQ:
		RecvMagicAttackReq(pkt);
		break;
	case AG_USER_INFO_ALL:
		RecvUserInfoAllData(pkt);
		break;
	case AG_PARTY_INFO_ALL:
		RecvPartyInfoAllData(pkt);
		break;
	case AG_CHECK_ALIVE_REQ:
		RecvCheckAlive(pkt);
		break;
	case AG_HEAL_MAGIC:
		RecvHealMagic(pkt);
		break;
	case AG_TIME_WEATHER:
		RecvTimeAndWeather(pkt);
		break;
	case AG_USER_FAIL:
		RecvUserFail(pkt);
		break;
	case AG_BATTLE_EVENT:
		RecvBattleEvent(pkt);
		break;
	case AG_NPC_GATE_OPEN:
		RecvGateOpen(pkt);
		break;
	case AG_USER_VISIBILITY:
		RecvUserVisibility(pkt);
		break;
	}
	return true;
}

void CGameSocket::RecvServerConnect(Packet & pkt)
{
	uint8 byReconnect = pkt.read<uint8>();
	printf("[GameServer connected - %s]\n", GetRemoteIP().c_str());

	Packet result(AI_SERVER_CONNECT, byReconnect);
	Send(&result);

	if (byReconnect == 1)
		TRACE("**** ReConnect - server=%s,  socket = %d ****\n ", GetRemoteIP().c_str(), GetSocketID());
	else
		TRACE("**** Connect - server=%s,  socket = %d ****\n ", GetRemoteIP().c_str(), GetSocketID());

	g_pMain.m_bFirstServerFlag = TRUE;
	g_pMain.AllNpcInfo();
}

void CGameSocket::RecvUserInfo(Packet & pkt)
{
	CUser *pUser = new CUser();
	std::string strUserID;

	pUser->Initialize();

	pkt.SByte();
	pkt >> pUser->m_iUserId >> strUserID >> pUser->m_curZone >> pUser->m_bNation 
		>> pUser->m_bLevel >> pUser->m_sHP >> pUser->m_sMP >> pUser->m_sHitDamage
		>> pUser->m_sAC >> pUser->m_fHitrate >> pUser->m_fAvoidrate >> pUser->m_sItemAC 
		>> pUser->m_bMagicTypeLeftHand >> pUser->m_bMagicTypeRightHand
		>> pUser->m_sMagicAmountLeftHand >> pUser->m_sMagicAmountRightHand
		>> pUser->m_byIsOP >> pUser->m_bInvisibilityType;

	if (strUserID.empty() || strUserID.length() > MAX_ID_SIZE)
	{
		delete pUser;
		return;
	}

	strcpy(pUser->m_strUserID, strUserID.c_str());
	pUser->m_pMap = g_pMain.GetZoneByID(pUser->m_curZone);
	pUser->m_bLive = USER_LIVE;

	TRACE("****  RecvUserInfo()---> uid = %d, name=%s ******\n", 
		pUser->m_iUserId, pUser->m_strUserID);

	if (pUser->m_iUserId >= USER_BAND && pUser->m_iUserId < MAX_USER)
		g_pMain.m_pUser[pUser->m_iUserId] = pUser;
	else 
		delete pUser;
}

void CGameSocket::RecvUserInOut(Packet & pkt)
{
	std::string strUserID;
	uint8 bType;
	uint16 uid;
	float fX, fZ;
	pkt.SByte();
	pkt >> bType >> uid >> strUserID >> fX >> fZ;
	if (fX < 0 || fZ < 0)
	{
		TRACE("Error:: RecvUserInOut(),, uid = %d, fX=%.2f, fZ=%.2f\n", uid, fX, fZ);
		return;
	}

	int region_x = 0, region_z=0;
	int x1 = (int)fX / TILE_SIZE;
	int z1 = (int)fZ / TILE_SIZE;
	region_x = (int)fX / VIEW_DIST; 
	region_z = (int)fZ / VIEW_DIST;

	// 수정할것,,, : 지금 존 번호를 0으로 했는데.. 유저의 존 정보의 번호를 읽어야,, 함,,
	MAP* pMap = NULL;
	//g_pMain.g_arZone[pUser->m_curZone];

	CUser* pUser = g_pMain.GetUserPtr(uid);

//	TRACE("^^& RecvUserInOut( type=%d )-> User(%s, %d),, zone=%d, index=%d, region_x=%d, y=%d\n", bType, pUser->m_strUserID, pUser->m_iUserId, pUser->m_curZone, pUser->m_sZoneIndex, region_x, region_z);

	if(pUser != NULL)
	{
	//	TRACE("##### Fail : ^^& RecvUserInOut() [name = %s]. state=%d, hp=%d\n", pUser->m_strUserID, pUser->m_bLive, pUser->m_sHP);
		BOOL bFlag = FALSE;
		
		if(pUser->m_bLive == USER_DEAD || pUser->m_sHP <= 0)
		{
			if(pUser->m_sHP > 0)
			{
				pUser->m_bLive = TRUE;
				TRACE("##### CGameSocket-RecvUserInOut Fail : UserHeal  [id=%s, bLive=%d, hp=%d], fX=%.2f, fZ=%.2f ######\n", pUser->m_strUserID, pUser->m_bLive, pUser->m_sHP, fX, fZ);
			}
			else
			{
				TRACE("##### CGameSocket-RecvUserInOut Fail : UserDead  [id=%s, bLive=%d, hp=%d], fX=%.2f, fZ=%.2f ######\n", pUser->m_strUserID, pUser->m_bLive, pUser->m_sHP, fX, fZ);
				// 죽은 유저이므로 게임서버에 죽은 처리를 한다...
				//Send_UserError(uid);
				//return;
			}
		}

		pMap = pUser->GetMap();

		if(pMap == NULL)
		{
			TRACE("#### Fail : pMap == NULL ####\n");
			return;
		}

		if(x1 < 0 || z1 < 0 || x1 >= pMap->GetMapSize() || z1 >= pMap->GetMapSize())
		{
			TRACE("#### RecvUserInOut Fail : [name=%s], x1=%d, z1=%d #####\n", pUser->m_strUserID, region_x, region_z);
			return;
		}
		// map 이동이 불가능이면 User등록 실패..
		//if(pMap->m_pMap[x1][z1].m_sEvent == 0) return;
		if(region_x > pMap->GetXRegionMax() || region_z > pMap->GetZRegionMax())
		{
			TRACE("#### GameSocket-RecvUserInOut() Fail : [name=%s], nRX=%d, nRZ=%d #####\n", pUser->m_strUserID, region_x, region_z);
			return;
		}

		//strcpy(pUser->m_strUserID, strName);
		pUser->m_curx = pUser->m_fWill_x = fX;
		pUser->m_curz = pUser->m_fWill_z = fZ;

		if(bType == 2)	{		// region out
			// 기존의 region정보에서 User의 정보 삭제..
			pMap->RegionUserRemove(region_x, region_z, uid);
			//TRACE("^^& RecvUserInOut()-> User(%s, %d)를 Region에서 삭제..,, zone=%d, index=%d, region_x=%d, y=%d\n", pUser->m_strUserID, pUser->m_iUserId, pUser->m_curZone, pUser->m_sZoneIndex, region_x, region_z);
		}
		else	{				// region in
			if(pUser->m_sRegionX != region_x || pUser->m_sRegionZ != region_z)	{
				pUser->m_sRegionX = region_x;		pUser->m_sRegionZ = region_z;
				pMap->RegionUserAdd(region_x, region_z, uid);
				//TRACE("^^& RecvUserInOut()-> User(%s, %d)를 Region에 등록,, zone=%d, index=%d, region_x=%d, y=%d\n", pUser->m_strUserID, pUser->m_iUserId, pUser->m_curZone, pUser->m_sZoneIndex, region_x, region_z);
			}
		}
	}
}

void CGameSocket::RecvUserMove(Packet & pkt)
{
	uint16 uid, speed;
	float fX, fZ, fY;
	pkt >> uid >> fX >> fZ >> fY >> speed;
	SetUid(fX, fZ, uid, speed);
}

void CGameSocket::RecvUserMoveEdge(Packet & pkt)
{
	uint16 uid;
	float fX, fZ, fY;
	pkt >> uid >> fX >> fZ >> fY;
	SetUid(fX, fZ, uid, 0);
}

BOOL CGameSocket::SetUid(float x, float z, int id, int speed)
{
	int x1 = (int)x / TILE_SIZE;
	int z1 = (int)z / TILE_SIZE;
	int nRX = (int)x / VIEW_DIST;
	int nRZ = (int)z / VIEW_DIST;

	CUser* pUser = g_pMain.GetUserPtr(id);
	if(pUser == NULL) 
	{
		TRACE("#### User등록 실패 sid = %d ####\n", id);
		return FALSE;
	}

	MAP* pMap = pUser->GetMap();
	if (pMap == NULL)
	{
		TRACE("#### User not in valid zone, sid = %d ####\n", id);
		return FALSE;
	}
	
	if(x1 < 0 || z1 < 0 || x1 >= pMap->GetMapSize() || z1 >= pMap->GetMapSize())
	{
		TRACE("#### GameSocket ,, SetUid Fail : [nid=%d, name=%s], x1=%d, z1=%d #####\n", id, pUser->m_strUserID, x1, z1);
		return FALSE;
	}
	if(nRX > pMap->GetXRegionMax() || nRZ > pMap->GetZRegionMax())
	{
		TRACE("#### GameSocket , SetUid Fail : [nid=%d, name=%s], nRX=%d, nRZ=%d #####\n", id, pUser->m_strUserID, nRX, nRZ);
		return FALSE;
	}
	// map 이동이 불가능이면 User등록 실패..
	// if(pMap->m_pMap[x1][z1].m_sEvent == 0) return FALSE;

	if(pUser != NULL)
	{
		if(pUser->m_bLive == USER_DEAD || pUser->m_sHP <= 0)
		{
			if(pUser->m_sHP > 0)
			{
				pUser->m_bLive = USER_LIVE;
				TRACE("##### CGameSocket-SetUid Fail : User가 Heal된 경우.. [id=%s, bLive=%d, hp=%d] ######\n", pUser->m_strUserID, pUser->m_bLive, pUser->m_sHP);
			}
			else
			{
				TRACE("##### CGameSocket-SetUid Fail : UserDead  [id=%s, bLive=%d, hp=%d] ######\n", pUser->m_strUserID, pUser->m_bLive, pUser->m_sHP);
				//Send_UserError(id);
				return FALSE;
			}
		}
		
		///// attack ~ 
		if( speed != 0 )	{
			pUser->m_curx = pUser->m_fWill_x;
			pUser->m_curz = pUser->m_fWill_z;
			pUser->m_fWill_x = x;
			pUser->m_fWill_z = z;
		}
		else	{
			pUser->m_curx = pUser->m_fWill_x = x;
			pUser->m_curz = pUser->m_fWill_z = z;
		}
		/////~ attack 

		//TRACE("GameSocket : SetUid()--> uid = %d, x=%f, z=%f \n", id, x, z);
		if(pUser->m_sRegionX != nRX || pUser->m_sRegionZ != nRZ)
		{
			//TRACE("*** SetUid()-> User(%s, %d)를 Region에 삭제,, zone=%d, index=%d, region_x=%d, y=%d\n", pUser->m_strUserID, pUser->m_iUserId, pUser->m_curZone, pUser->m_sZoneIndex, pUser->m_sRegionX, pUser->m_sRegionZ);
			pMap->RegionUserRemove(pUser->m_sRegionX, pUser->m_sRegionZ, id);
			pUser->m_sRegionX = nRX;		pUser->m_sRegionZ = nRZ;
			pMap->RegionUserAdd(pUser->m_sRegionX, pUser->m_sRegionZ, id);
			//TRACE("*** SetUid()-> User(%s, %d)를 Region에 등록,, zone=%d, index=%d, region_x=%d, y=%d\n", pUser->m_strUserID, pUser->m_iUserId, pUser->m_curZone, pUser->m_sZoneIndex, nRX, nRZ);
		}
	}

	// dungeon work
	// if( pUser->m_curZone == 던젼 ) 
	int room = pMap->IsRoomCheck( x, z );

	return TRUE;
}

void CGameSocket::RecvAttackReq(Packet & pkt)
{
	uint16 sid, tid;
	float rx=0.0f, ry=0.0f, rz=0.0f, fDir = 0.0f, fHitAgi, fAvoidAgi;
	short sDamage, sAC, sItemAC, sAmountLeft, sAmountRight;
	BYTE type, result, bTypeLeft, bTypeRight;

	pkt >> type >> result >> sid >> tid >> sDamage >> sAC >> fHitAgi >> fAvoidAgi
		>> sItemAC >> bTypeLeft >> bTypeRight >> sAmountLeft >> sAmountRight;

	CUser* pUser = g_pMain.GetUserPtr(sid);
	if(pUser == NULL) return;

	if(pUser->m_bLive == USER_DEAD || pUser->m_sHP <= 0)
	{
		if(pUser->m_sHP > 0)
		{
			pUser->m_bLive = USER_LIVE;
			TRACE("##### CGameSocket-Attack Fail : User가 Heal된 경우.. [id=%d, %s, bLive=%d, hp=%d] ######\n", pUser->m_iUserId, pUser->m_strUserID, pUser->m_bLive, pUser->m_sHP);
		}		
		else
		{
			TRACE("##### CGameSocket-Attack Fail : UserDead  [id=%d, %s, bLive=%d, hp=%d] ######\n", pUser->m_iUserId, pUser->m_strUserID, pUser->m_bLive, pUser->m_sHP);
			// 죽은 유저이므로 게임서버에 죽은 처리를 한다...
			Send_UserError(sid, tid);
			return;
		}
	}
	pUser->m_sHitDamage = sDamage;
	pUser->m_fHitrate = fHitAgi;
	pUser->m_fAvoidrate = fAvoidAgi;
	pUser->m_sAC = sAC;
	pUser->m_sItemAC = sItemAC;
	pUser->m_bMagicTypeLeftHand = bTypeLeft;
	pUser->m_bMagicTypeRightHand = bTypeRight;
	pUser->m_sMagicAmountLeftHand = sAmountLeft;
	pUser->m_sMagicAmountRightHand = sAmountRight;

	pUser->Attack(sid, tid);
}

void CGameSocket::RecvUserLogOut(Packet & pkt)
{
	short uid;
	std::string strUserID;

	pkt >> uid >> strUserID; // double byte string for once

	g_pMain.DeleteUserList(uid);
	TRACE("**** User LogOut -- uid = %d, name = %s\n", uid, strUserID.c_str());
}

void CGameSocket::RecvUserRegene(Packet & pkt)
{
	uint16 uid, sHP;
	pkt >> uid >> sHP;

	CUser* pUser = g_pMain.GetUserPtr(uid);
	if(pUser == NULL)	
		return;

	pUser->m_bLive = USER_LIVE;
	pUser->m_sHP = sHP;

	TRACE("**** RecvUserRegene -- uid = (%s,%d), HP = %d\n", pUser->m_strUserID, pUser->m_iUserId, pUser->m_sHP);
}

void CGameSocket::RecvUserSetHP(Packet & pkt)
{
	uint16 uid = pkt.read<uint16>();
	uint32 nHP = pkt.read<uint32>();

	CUser* pUser = g_pMain.GetUserPtr(uid);
	if(pUser == NULL)	return;

	if(pUser->m_sHP != nHP)	{
		pUser->m_sHP = nHP;
		if(pUser->m_sHP <= 0)	{
			pUser->Dead(-100, 0);
		}
	}
}

void CGameSocket::RecvUserUpdate(Packet & pkt)
{
	uint16 uid = pkt.read<uint16>();
	std::string strUserID;

	CUser* pUser = g_pMain.GetUserPtr(uid);
	if (pUser == NULL)
		return;

	uint8 bOldLevel = pUser->m_bLevel;

	pkt.SByte();
	pkt >> strUserID >> pUser->m_bLevel >> pUser->m_sHP >> pUser->m_sMP
		>> pUser->m_sHitDamage >> pUser->m_sAC >> pUser->m_fHitrate >> pUser->m_fAvoidrate
		>> pUser->m_sItemAC >> pUser->m_bMagicTypeLeftHand >> pUser->m_bMagicTypeRightHand
		>> pUser->m_sMagicAmountLeftHand >> pUser->m_sMagicAmountRightHand
		>> pUser->m_bInvisibilityType;
}

void CGameSocket::Send_UserError(short uid, short tid)
{
	Packet result(AG_USER_FAIL);
	result << uid << tid;
	Send(&result);
}

void CGameSocket::RecvZoneChange(Packet & pkt)
{
	uint16 uid = pkt.read<uint16>();
	uint8 byZoneNumber = pkt.read<uint8>();

	CUser* pUser = g_pMain.GetUserPtr(uid);
	if (pUser == NULL)	
		return;

	pUser->m_pMap = g_pMain.GetZoneByID(byZoneNumber);
	pUser->m_curZone = byZoneNumber;

	TRACE("**** RecvZoneChange -- user(%s, %d), cur_zone = %d\n", pUser->m_strUserID, pUser->m_iUserId, byZoneNumber);
}

void CGameSocket::RecvMagicAttackReq(Packet & pkt)
{
	uint16 sid = pkt.read<uint16>();
	CUser* pUser = g_pMain.GetUserPtr(sid);
	if (pUser == NULL) return;
	if(pUser->m_bLive == USER_DEAD || pUser->m_sHP <= 0)
	{
		if(pUser->m_sHP > 0)
		{
			pUser->m_bLive = USER_LIVE;
			TRACE("##### CGameSocket-Magic Attack Fail : User가 Heal된 경우.. [id=%s, bLive=%d, hp=%d] ######\n", pUser->m_strUserID, pUser->m_bLive, pUser->m_sHP);
		}		
		else
		{
			TRACE("##### CGameSocket-Magic Attack Fail : UserDead  [id=%s, bLive=%d, hp=%d] ######\n", pUser->m_strUserID, pUser->m_bLive, pUser->m_sHP);
			Send_UserError(sid, -1);
			return;
		}
	}

	pUser->m_MagicProcess.MagicPacket(pkt);
}

void CGameSocket::RecvUserInfoAllData(Packet & pkt)
{
	uint8 byCount = pkt.read<uint8>();
	pkt.SByte();
	for (int i = 0; i < byCount; i++)
	{
		CUser* pUser = new CUser();
		std::string strUserID;

		pUser->Initialize();

		pkt >> pUser->m_iUserId >> strUserID >> pUser->m_curZone
			>> pUser->m_bNation >> pUser->m_bLevel 
			>> pUser->m_sHP >> pUser->m_sMP
			>> pUser->m_sHitDamage >> pUser->m_sAC
			>> pUser->m_fHitrate >> pUser->m_fAvoidrate
			>> pUser->m_sPartyNumber >> pUser->m_byIsOP
			>> pUser->m_bInvisibilityType;

		if (strUserID.empty() || strUserID.length() > MAX_ID_SIZE)
		{
			TRACE("###  RecvUserInfoAllData() Fail ---> uid = %d, name=%s, len=%d  ### \n", 
				pUser->m_iUserId, strUserID.c_str(), strUserID.length());

			delete pUser;
			continue;
		}

		strcpy(pUser->m_strUserID, strUserID.c_str());
		pUser->m_pMap = g_pMain.GetZoneByID(pUser->m_curZone);
		pUser->m_bLive = USER_LIVE;
		if (pUser->m_sPartyNumber != -1)
			pUser->m_byNowParty = 1;

		TRACE("****  RecvUserInfoAllData()---> uid = %d, %s, party_number=%d  ******\n", 
			pUser->m_iUserId, pUser->m_strUserID, pUser->m_sPartyNumber);

		if (pUser->m_iUserId >= USER_BAND && pUser->m_iUserId < MAX_USER)
		{
			// Does a user already exist? Free them (I know, tacky...)
			if (g_pMain.m_pUser[pUser->m_iUserId] != NULL)
				delete g_pMain.m_pUser[pUser->m_iUserId];

			g_pMain.m_pUser[pUser->m_iUserId] = pUser;
		}
		else
		{
			delete pUser;
		}
	}
}

void CGameSocket::RecvGateOpen(Packet & pkt)
{
	uint16 nid;
	uint8 byGateOpen;

	pkt >> nid >> byGateOpen;

	if(nid < NPC_BAND || nid < INVALID_BAND)	{
		TRACE("####   RecvGateOpen()  nid Fail --> nid = %d  ####\n", nid);
		return;
	}

	CNpc* pNpc = NULL;
	pNpc = g_pMain.m_arNpc.GetData(nid);
	if(pNpc == NULL)		return;

	if(pNpc->m_proto->m_tNpcType == NPC_DOOR || pNpc->m_proto->m_tNpcType == NPC_GATE_LEVER || pNpc->m_proto->m_tNpcType == NPC_PHOENIX_GATE ) 	{
		if(byGateOpen < 0 || byGateOpen < 2) 	{
			TRACE("####   RecvGateOpen()  byGateOpen Fail --> byGateOpen = %d  ####\n", byGateOpen);
			return;
		}

		pNpc->m_byGateOpen = byGateOpen;

		TRACE("****  RecvGateOpen()---> nid = %d, byGateOpen = %d  ******\n", nid, byGateOpen);
	}
	else	{
		TRACE("####   RecvGateOpen()  NpcType Fail --> type = %d  ####\n", pNpc->m_proto->m_tNpcType);
		return;
	}
	
}

void CGameSocket::RecvUserVisibility(Packet & pkt)
{
	uint16 sid;
	uint8 bIsInvisible;
	pkt >> sid >> bIsInvisible;

	CUser *pUser = g_pMain.GetUserPtr(sid);
	if (pUser == NULL)
		return;

	pUser->m_bInvisibilityType = bIsInvisible;
}

void CGameSocket::RecvPartyInfoAllData(Packet & pkt)
{
	uint16 sPartyIndex = pkt.read<uint16>();
	if (sPartyIndex >= SHRT_MAX)
	{
		TRACE("#### RecvPartyInfoAllData Index Fail -  index = %d ####\n", sPartyIndex);
		return;
	}

	_PARTY_GROUP *pParty = new _PARTY_GROUP;
	pParty->wIndex = sPartyIndex;

	for (int i = 0; i < 8; i++)
		pParty->uid[i] = pkt.read<uint16>();

	EnterCriticalSection( &g_region_critical );
	if (g_pMain.m_arParty.PutData(pParty->wIndex, pParty))
		TRACE("****  RecvPartyInfoAllData()---> PartyIndex = %d  ******\n", sPartyIndex);
	LeaveCriticalSection(&g_region_critical);
}

void CGameSocket::RecvCheckAlive(Packet & pkt)
{
//	TRACE("CAISocket-RecvCheckAlive : zone_num=%d\n", m_iZoneNum);
}

void CGameSocket::RecvHealMagic(Packet & pkt)
{
	uint16 sid = pkt.read<uint16>();

	//TRACE("RecvHealMagic : [sid=%d] \n", sid);

	CUser* pUser = NULL;
	pUser = g_pMain.GetUserPtr(sid);
	if(pUser == NULL) return;

	if(pUser->m_bLive == USER_DEAD || pUser->m_sHP <= 0)	{
		if(pUser->m_sHP > 0)	{
			pUser->m_bLive = USER_LIVE;
			TRACE("##### CGameSocket-RecvHealMagic Fail : User가 Heal된 경우.. [id=%d, %s, bLive=%d, hp=%d] ######\n", pUser->m_iUserId, pUser->m_strUserID, pUser->m_bLive, pUser->m_sHP);
		}		
		else
		{
			TRACE("##### CGameSocket-RecvHealMagic Fail : UserDead  [id=%d, %s, bLive=%d, hp=%d] ######\n", pUser->m_iUserId, pUser->m_strUserID, pUser->m_bLive, pUser->m_sHP);
			// 죽은 유저이므로 게임서버에 죽은 처리를 한다...
			//Send_UserError(sid, tid);
			return;
		}
	}

	pUser->HealMagic();
}

void CGameSocket::RecvTimeAndWeather(Packet & pkt)
{
	pkt >> g_pMain.m_iYear >> g_pMain.m_iMonth >> g_pMain.m_iDate >> g_pMain.m_iHour >> g_pMain.m_iMin >> g_pMain.m_iWeather >> g_pMain.m_iAmount;

	if (g_pMain.m_iHour >=5 && g_pMain.m_iHour < 21)	g_pMain.m_byNight = 1;
	else												g_pMain.m_byNight = 2;
}

void CGameSocket::RecvUserFail(Packet & pkt)
{
	uint16 sid, tid, sHP;
	pkt >> sid >> tid >> sHP;
	CUser* pUser = g_pMain.GetUserPtr(sid);
	if (pUser == NULL) 
		return;

	pUser->m_bLive = USER_LIVE;
	pUser->m_sHP = sHP;
}

void CGameSocket::RecvBattleEvent(Packet & pkt)
{
	uint8 bType = pkt.read<uint8>(), bEvent = pkt.read<uint8>();

	if (bEvent == BATTLEZONE_OPEN || bEvent == BATTLEZONE_CLOSE)
	{
		g_pMain.m_sKillKarusNpc = g_pMain.m_sKillElmoNpc = 0;
		g_pMain.m_byBattleEvent = bEvent;
		if (bEvent == BATTLEZONE_CLOSE)
			g_pMain.ResetBattleZone();
	}

	int nSize = g_pMain.m_arNpc.GetSize();
	for (int i = 0; i < nSize; i++)
	{
		CNpc *pNpc = g_pMain.m_arNpc.GetData(i);
		if (pNpc == NULL)
			continue;
		if (pNpc->m_proto->m_tNpcType > 10 && (pNpc->m_byGroup == KARUS_ZONE || pNpc->m_byGroup == ELMORAD_ZONE))
		{
			if (bEvent == BATTLEZONE_OPEN || bEvent == BATTLEZONE_CLOSE)
				pNpc->ChangeAbility(bEvent);
		}
	}
}