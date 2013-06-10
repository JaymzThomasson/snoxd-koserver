#include "stdafx.h"
#include "GameSocket.h"
#include "User.h"
#include "MAP.h"
#include "Region.h"
#include "../shared/globals.h"
#include "Npc.h"

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
	g_pMain->DeleteAllUserList(this);
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
	case AG_NPC_HP_CHANGE:
		RecvNpcHpChange(pkt);
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

	g_pMain->m_bFirstServerFlag = true;
	g_pMain->AllNpcInfo();
}

void CGameSocket::RecvUserInfo(Packet & pkt)
{
	CUser *pUser = new CUser();

	pUser->Initialize();

	pkt.SByte();
	pkt >> pUser->m_iUserId >> pUser->m_strUserID >> pUser->m_curZone >> pUser->m_bNation 
		>> pUser->m_bLevel >> pUser->m_sHP >> pUser->m_sMP >> pUser->m_sHitDamage
		>> pUser->m_sAC >> pUser->m_fHitrate >> pUser->m_fAvoidrate >> pUser->m_sItemAC 
		>> pUser->m_bMagicTypeLeftHand >> pUser->m_bMagicTypeRightHand
		>> pUser->m_sMagicAmountLeftHand >> pUser->m_sMagicAmountRightHand
		>> pUser->m_byIsOP >> pUser->m_bInvisibilityType;

	if (pUser->GetName().empty() || pUser->GetName().length() > MAX_ID_SIZE)
	{
		delete pUser;
		return;
	}

	pUser->m_pMap = g_pMain->GetZoneByID(pUser->m_curZone);
	pUser->m_bLive = AI_USER_LIVE;

	TRACE("****  RecvUserInfo()---> uid = %d, name=%s ******\n", 
		pUser->m_iUserId, pUser->GetName().c_str());

	if (pUser->m_iUserId < MAX_USER)
		g_pMain->m_pUser[pUser->m_iUserId] = pUser;
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
	MAP* pMap = nullptr;
	//g_pMain->g_arZone[pUser->m_curZone];

	CUser* pUser = g_pMain->GetUserPtr(uid);

//	TRACE("^^& RecvUserInOut( type=%d )-> User(%s, %d),, zone=%d, index=%d, region_x=%d, y=%d\n", bType, pUser->GetName().c_str(), pUser->m_iUserId, pUser->m_curZone, pUser->m_sZoneIndex, region_x, region_z);

	if(pUser != nullptr)
	{
	//	TRACE("##### Fail : ^^& RecvUserInOut() [name = %s]. state=%d, hp=%d\n", pUser->GetName().c_str(), pUser->m_bLive, pUser->m_sHP);
		
		if(pUser->m_bLive == AI_USER_DEAD || pUser->m_sHP <= 0)
		{
			if(pUser->m_sHP > 0)
			{
				pUser->m_bLive = true;
				TRACE("##### CGameSocket-RecvUserInOut Fail : UserHeal  [id=%s, bLive=%d, hp=%d], fX=%.2f, fZ=%.2f ######\n", pUser->GetName().c_str(), pUser->m_bLive, pUser->m_sHP, fX, fZ);
			}
			else
			{
				TRACE("##### CGameSocket-RecvUserInOut Fail : UserDead  [id=%s, bLive=%d, hp=%d], fX=%.2f, fZ=%.2f ######\n", pUser->GetName().c_str(), pUser->m_bLive, pUser->m_sHP, fX, fZ);
				// 죽은 유저이므로 게임서버에 죽은 처리를 한다...
				//Send_UserError(uid);
				//return;
			}
		}

		pMap = pUser->GetMap();

		if(pMap == nullptr)
		{
			TRACE("#### Fail : pMap == nullptr ####\n");
			return;
		}

		if(x1 < 0 || z1 < 0 || x1 >= pMap->GetMapSize() || z1 >= pMap->GetMapSize())
		{
			TRACE("#### RecvUserInOut Fail : [name=%s], x1=%d, z1=%d #####\n", pUser->GetName().c_str(), region_x, region_z);
			return;
		}

		//if(pMap->m_pMap[x1][z1].m_sEvent == 0) return;
		if(region_x > pMap->GetXRegionMax() || region_z > pMap->GetZRegionMax())
		{
			TRACE("#### GameSocket-RecvUserInOut() Fail : [name=%s], nRX=%d, nRZ=%d #####\n", pUser->GetName().c_str(), region_x, region_z);
			return;
		}

		pUser->m_curx = pUser->m_fWill_x = fX;
		pUser->m_curz = pUser->m_fWill_z = fZ;

		// leaving a region
		if (bType == 2)	
		{
			pMap->RegionUserRemove(region_x, region_z, uid);
		}
		// entering a region
		else if (pUser->m_sRegionX != region_x || pUser->m_sRegionZ != region_z)	
		{
			pUser->m_sRegionX = region_x;		
			pUser->m_sRegionZ = region_z;

			pMap->RegionUserAdd(region_x, region_z, uid);
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

bool CGameSocket::SetUid(float x, float z, int id, int speed)
{
	int x1 = (int)x / TILE_SIZE;
	int z1 = (int)z / TILE_SIZE;
	int nRX = (int)x / VIEW_DIST;
	int nRZ = (int)z / VIEW_DIST;

	CUser* pUser = g_pMain->GetUserPtr(id);
	if(pUser == nullptr) 
	{
		TRACE("#### User등록 실패 sid = %d ####\n", id);
		return false;
	}

	MAP* pMap = pUser->GetMap();
	if (pMap == nullptr)
	{
		TRACE("#### User not in valid zone, sid = %d ####\n", id);
		return false;
	}
	
	if(x1 < 0 || z1 < 0 || x1 >= pMap->GetMapSize() || z1 >= pMap->GetMapSize())
	{
		TRACE("#### GameSocket ,, SetUid Fail : [nid=%d, name=%s], x1=%d, z1=%d #####\n", id, pUser->GetName().c_str(), x1, z1);
		return false;
	}
	if(nRX > pMap->GetXRegionMax() || nRZ > pMap->GetZRegionMax())
	{
		TRACE("#### GameSocket , SetUid Fail : [nid=%d, name=%s], nRX=%d, nRZ=%d #####\n", id, pUser->GetName().c_str(), nRX, nRZ);
		return false;
	}

	// if(pMap->m_pMap[x1][z1].m_sEvent == 0) return false;

	if (pUser != nullptr)
	{
		if (pUser->m_bLive == AI_USER_DEAD || pUser->m_sHP <= 0)
			return false;
		
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

		if(pUser->m_sRegionX != nRX || pUser->m_sRegionZ != nRZ)
		{
			pMap->RegionUserRemove(pUser->m_sRegionX, pUser->m_sRegionZ, id);
			pUser->m_sRegionX = nRX;		pUser->m_sRegionZ = nRZ;
			pMap->RegionUserAdd(pUser->m_sRegionX, pUser->m_sRegionZ, id);
		}
	}

	// dungeon work
	// if( pUser->m_curZone == 던젼 ) 
	int room = pMap->IsRoomCheck( x, z );

	return true;
}

void CGameSocket::RecvAttackReq(Packet & pkt)
{
	uint16 sid, tid;
	float rx=0.0f, ry=0.0f, rz=0.0f, fDir = 0.0f, fHitAgi, fAvoidAgi;
	short sDamage, sAC, sItemAC, sAmountLeft, sAmountRight;
	uint8 type, result, bTypeLeft, bTypeRight;

	pkt >> type >> result >> sid >> tid >> sDamage >> sAC >> fHitAgi >> fAvoidAgi
		>> sItemAC >> bTypeLeft >> bTypeRight >> sAmountLeft >> sAmountRight;

	CUser* pUser = g_pMain->GetUserPtr(sid);
	if (pUser == nullptr
		|| pUser->m_bLive == AI_USER_DEAD
		|| pUser->m_sHP <= 0)
		return;

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

	g_pMain->DeleteUserList(uid);
	TRACE("**** User LogOut -- uid = %d, name = %s\n", uid, strUserID.c_str());
}

void CGameSocket::RecvUserRegene(Packet & pkt)
{
	uint16 uid, sHP;
	pkt >> uid >> sHP;

	CUser* pUser = g_pMain->GetUserPtr(uid);
	if(pUser == nullptr)	
		return;

	pUser->m_bLive = AI_USER_LIVE;
	pUser->m_sHP = sHP;

	TRACE("**** RecvUserRegene -- uid = (%s,%d), HP = %d\n", pUser->GetName().c_str(), pUser->m_iUserId, pUser->m_sHP);
}

void CGameSocket::RecvUserSetHP(Packet & pkt)
{
	uint16 uid = pkt.read<uint16>();
	uint32 nHP = pkt.read<uint32>();

	CUser* pUser = g_pMain->GetUserPtr(uid);
	if(pUser == nullptr)	return;

	if(pUser->m_sHP != nHP)	{
		pUser->m_sHP = nHP;
		if(pUser->m_sHP <= 0)	{
			pUser->Dead(-100, 0);
		}
	}
}

void CGameSocket::RecvNpcHpChange(Packet & pkt)
{
	int16 nid, sAttackerID;
	int32 nHP, nAmount;
	pkt >> nid >> sAttackerID >> nHP >> nAmount;
	CNpc * pNpc = g_pMain->m_arNpc.GetData(nid);
	if (pNpc == nullptr)
		return;

	if (nAmount < 0)
	{
		pNpc->SetDamage(0, -nAmount, sAttackerID, 1);
	}
	else
	{		
		pNpc->m_iHP += nAmount;
		if (pNpc->m_iHP > pNpc->m_iMaxHP)
			pNpc->m_iHP = pNpc->m_iMaxHP;
	}
}

void CGameSocket::RecvUserUpdate(Packet & pkt)
{
	uint16 uid = pkt.read<uint16>();
	std::string strUserID;

	CUser* pUser = g_pMain->GetUserPtr(uid);
	if (pUser == nullptr)
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

	CUser* pUser = g_pMain->GetUserPtr(uid);
	if (pUser == nullptr)	
		return;

	pUser->m_pMap = g_pMain->GetZoneByID(byZoneNumber);
	pUser->m_curZone = byZoneNumber;

	TRACE("**** RecvZoneChange -- user(%s, %d), cur_zone = %d\n", pUser->GetName().c_str(), pUser->m_iUserId, byZoneNumber);
}

void CGameSocket::RecvUserInfoAllData(Packet & pkt)
{
	uint8 byCount = pkt.read<uint8>();
	pkt.SByte();
	for (int i = 0; i < byCount; i++)
	{
		CUser* pUser = new CUser();

		pUser->Initialize();

		pkt >> pUser->m_iUserId >> pUser->m_strUserID >> pUser->m_curZone
			>> pUser->m_bNation >> pUser->m_bLevel 
			>> pUser->m_sHP >> pUser->m_sMP
			>> pUser->m_sHitDamage >> pUser->m_sAC
			>> pUser->m_fHitrate >> pUser->m_fAvoidrate
			>> pUser->m_sPartyNumber >> pUser->m_byIsOP
			>> pUser->m_bInvisibilityType;

		if (pUser->GetName().empty() || pUser->GetName().length() > MAX_ID_SIZE)
		{
			TRACE("###  RecvUserInfoAllData() Fail ---> uid = %d, name=%s, len=%d  ### \n", 
				pUser->m_iUserId, pUser->GetName().c_str(), pUser->GetName().length());

			delete pUser;
			continue;
		}

		pUser->m_pMap = g_pMain->GetZoneByID(pUser->m_curZone);
		pUser->m_bLive = AI_USER_LIVE;
		if (pUser->m_sPartyNumber != -1)
			pUser->m_byNowParty = 1;

		TRACE("****  RecvUserInfoAllData()---> uid = %d, %s, party_number=%d  ******\n", 
			pUser->m_iUserId, pUser->GetName().c_str(), pUser->m_sPartyNumber);

		if (pUser->m_iUserId >= USER_BAND && pUser->m_iUserId < MAX_USER)
		{
			// Does a user already exist? Free them (I know, tacky...)
			if (g_pMain->m_pUser[pUser->m_iUserId] != nullptr)
				delete g_pMain->m_pUser[pUser->m_iUserId];

			g_pMain->m_pUser[pUser->m_iUserId] = pUser;
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

	CNpc* pNpc = g_pMain->m_arNpc.GetData(nid);
	if(pNpc == nullptr)		return;

	if (!pNpc->isGate()) 
	{
		TRACE("####   RecvGateOpen()  NpcType Fail --> type = %d  ####\n", pNpc->m_proto->m_tNpcType);
		return;
	}

	if (byGateOpen < 0 || byGateOpen < 2)
	{
		TRACE("####   RecvGateOpen()  byGateOpen Fail --> byGateOpen = %d  ####\n", byGateOpen);
		return;
	}

	pNpc->m_byGateOpen = byGateOpen;

	TRACE("****  RecvGateOpen()---> nid = %d, byGateOpen = %d  ******\n", nid, byGateOpen);
}

void CGameSocket::RecvUserVisibility(Packet & pkt)
{
	uint16 sid;
	uint8 bIsInvisible;
	pkt >> sid >> bIsInvisible;

	CUser *pUser = g_pMain->GetUserPtr(sid);
	if (pUser == nullptr)
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

	if (g_pMain->m_arParty.PutData(pParty->wIndex, pParty))
		TRACE("****  RecvPartyInfoAllData()---> PartyIndex = %d  ******\n", sPartyIndex);
}

void CGameSocket::RecvCheckAlive(Packet & pkt)
{
//	TRACE("CAISocket-RecvCheckAlive : zone_num=%d\n", m_iZoneNum);
}

void CGameSocket::RecvHealMagic(Packet & pkt)
{
	uint16 sid = pkt.read<uint16>();
	CUser* pUser = g_pMain->GetUserPtr(sid);

	if (pUser == nullptr
		|| pUser->m_sHP <= 0
		|| pUser->m_bLive == AI_USER_DEAD) 
		return;

	pUser->HealMagic();
}

void CGameSocket::RecvTimeAndWeather(Packet & pkt)
{
	pkt >> g_pMain->m_iYear >> g_pMain->m_iMonth >> g_pMain->m_iDate >> g_pMain->m_iHour >> g_pMain->m_iMin >> g_pMain->m_iWeather >> g_pMain->m_iAmount;

	if (g_pMain->m_iHour >=5 && g_pMain->m_iHour < 21)	g_pMain->m_byNight = 1;
	else												g_pMain->m_byNight = 2;
}

void CGameSocket::RecvUserFail(Packet & pkt)
{
	uint16 sid, tid, sHP;
	pkt >> sid >> tid >> sHP;
	CUser* pUser = g_pMain->GetUserPtr(sid);
	if (pUser == nullptr) 
		return;

	pUser->m_bLive = AI_USER_LIVE;
	pUser->m_sHP = sHP;
}

void CGameSocket::RecvBattleEvent(Packet & pkt)
{
	uint8 bType = pkt.read<uint8>(), bEvent = pkt.read<uint8>();

	if (bEvent == BATTLEZONE_OPEN || bEvent == BATTLEZONE_CLOSE)
	{
		g_pMain->m_sKillKarusNpc = g_pMain->m_sKillElmoNpc = 0;
		g_pMain->m_byBattleEvent = bEvent;
		if (bEvent == BATTLEZONE_CLOSE)
			g_pMain->ResetBattleZone();
	}

	foreach_stlmap (itr, g_pMain->m_arNpc)
	{
		CNpc *pNpc = itr->second;
		if (pNpc == nullptr)
			continue;

		if (pNpc->m_proto->m_tNpcType > 10 && (pNpc->GetNation() == KARUS || pNpc->GetNation() == ELMORAD))
		{
			if (bEvent == BATTLEZONE_OPEN || bEvent == BATTLEZONE_CLOSE)
				pNpc->ChangeAbility(bEvent);
		}
	}
}