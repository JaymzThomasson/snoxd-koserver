// AISocket.cpp: implementation of the CAISocket class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AISocket.h"
#include "EbenezerDlg.h"
#include "define.h"
#include "AiPacket.h"
#include "Npc.h"
#include "user.h"
#include "Map.h"
#include "../shared/lzf.h"
#include "../shared/crc32.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern CRITICAL_SECTION g_LogFile_critical;

bool CAISocket::HandlePacket(Packet & pkt)
{
	TRACE("CAISocket::Parsing() - %X (%d), len=%d\n", pkt.GetOpcode(), pkt.GetOpcode(), pkt.size()); 
	switch (pkt.GetOpcode())
	{
		case AG_CHECK_ALIVE_REQ:
			RecvCheckAlive(pkt);
			break;
		case AI_SERVER_CONNECT:
			LoginProcess(pkt);
			break;
		case AG_SERVER_INFO:
			RecvServerInfo(pkt);
			break;
		case NPC_INFO_ALL:
			RecvNpcInfoAll(pkt);
			break;
		case MOVE_RESULT:
			RecvNpcMoveResult(pkt);
			break;
		case MOVE_END_RESULT:
			break;
		case AG_ATTACK_RESULT:
			RecvNpcAttack(pkt);
			break;
		case AG_MAGIC_ATTACK_RESULT:
			RecvMagicAttackResult(pkt);
			break;
		case AG_NPC_INFO:
			RecvNpcInfo(pkt);
			break;
		case AG_USER_SET_HP:
			RecvUserHP(pkt);
			break;
		case AG_USER_EXP:
			RecvUserExp(pkt);
			break;
		case AG_SYSTEM_MSG:
			RecvSystemMsg(pkt);
			break;
		case AG_NPC_GIVE_ITEM:
			RecvNpcGiveItem(pkt);
			break;
		case AG_USER_FAIL:
			RecvUserFail(pkt);
			break;
		case AG_NPC_GATE_DESTORY:
			RecvGateDestory(pkt);
			break;
		case AG_DEAD:
			RecvNpcDead(pkt);
			break;
		case AG_NPC_INOUT:
			RecvNpcInOut(pkt);
			break;
		case AG_BATTLE_EVENT:
			RecvBattleEvent(pkt);
			break;
		case AG_NPC_EVENT_ITEM:
			RecvNpcEventItem(pkt);
			break;
		case AG_NPC_GATE_OPEN:
			RecvGateOpen(pkt);
			break;
		case AG_COMPRESSED:
			RecvCompressed(pkt);
			break;
	}

	return true;
}

void CAISocket::OnConnect()
{
	// Set a flag to indicate whether we've ever connected before or not
	// This is used accordingly by the AI server when we tell it what our status is.
	// Kind of messy, and needs looking into further.
	m_bHasConnected = true; 
}

void CAISocket::OnDisconnect()
{
	CTime time = CTime::GetCurrentTime();
	TRACE("*** CloseProcess - socketID=%d...  ***  %d-%d-%d, %d:%d]\r\n", GetSocketID(), time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute() );
}

void CAISocket::LoginProcess(Packet & pkt)
{
	uint8 bReconnect = pkt.read<uint8>();

	DEBUG_LOG("AI Server Connect Success!!");
	if (bReconnect == 1)
		TRACE("**** ReConnect - socket = %d ****\n ", GetSocketID());

	g_pMain->m_bServerCheckFlag = TRUE;
	g_pMain->SendAllUserInfo();
}

void CAISocket::RecvServerInfo(Packet & pkt)
{
	int size = g_pMain->m_ZoneArray.GetSize();
	uint16 sTotalMonster;
	uint8 bZone;

	pkt >> bZone >> sTotalMonster;

	g_pMain->m_sZoneCount++;

	TRACE("RecvServerInfo....%d, total=%d, socketcount=%d\n", bZone, sTotalMonster, g_pMain->m_sZoneCount);

	if (g_pMain->m_sZoneCount == size)
	{
#if 0
		if (g_pMain->m_bFirstServerFlag == FALSE)
			TRACE("+++ �������� ���� ������ �� �޾���, User AcceptThread Start ....%d, socketcount=%d\n", bZone, g_pMain->m_sZoneCount);
#endif

		g_pMain->m_sZoneCount = 0;
		g_pMain->m_bFirstServerFlag = TRUE;
		g_pMain->m_bPointCheckFlag = TRUE;

		g_pMain->AddToList("All spawn data loaded.");
	}
}

void CAISocket::RecvNpcInfoAll(Packet & pkt)
{
	uint8 bCount = pkt.read<uint8>(); // max of 20

	pkt.SByte();
	for (int i = 0; i < bCount; i++)
	{
		uint8 bType, bDirection;
		std::string strName;

		CNpc* pNpc = new CNpc();
		pNpc->Initialize();

		pkt >> bType >> pNpc->m_sNid >> pNpc->m_sSid >> pNpc->m_sPid >> pNpc->m_sSize >> pNpc->m_iWeapon_1 >> pNpc->m_iWeapon_2
			>> pNpc->m_bCurZone >> strName >> pNpc->m_byGroup >> pNpc->m_byLevel >> pNpc->m_fCurX >> pNpc->m_fCurZ >> pNpc->m_fCurY
			>> bDirection >> pNpc->m_tNpcType >> pNpc->m_iSellingGroup >> pNpc->m_iMaxHP >> pNpc->m_iHP
			>> pNpc->m_byGateOpen >> pNpc->m_sHitRate >> pNpc->m_byObjectType;

		if (strName.empty() || strName.length() > MAX_NPC_SIZE)
		{
			delete pNpc;
			continue;
		}

		//TRACE("Recv --> NpcUserInfo : uid = %d, x=%f, z=%f.. \n", nid, fPosX, fPosZ);
		strcpy(pNpc->m_strName, strName.c_str());

		pNpc->m_pMap = g_pMain->GetZoneByID(pNpc->getZoneID());
		pNpc->m_NpcState = NPC_LIVE;
		pNpc->m_byDirection = bDirection;
		pNpc->m_sRegion_X = (int)pNpc->m_fCurX / VIEW_DISTANCE;
		pNpc->m_sRegion_Z = (int)pNpc->m_fCurZ / VIEW_DISTANCE;

		if (pNpc->GetMap() == NULL)
		{
			delete pNpc;
			pNpc = NULL;
			continue;
		}

		if (pNpc->m_byObjectType == SPECIAL_OBJECT)
		{
			_OBJECT_EVENT* pEvent = pNpc->GetMap()->GetObjectEvent(pNpc->m_sSid);
			if (pEvent != NULL)
				pEvent->byLife = 1;
		}

	//	TRACE("Recv --> NpcUserInfoAll : uid=%d, sid=%d, name=%s, x=%f, z=%f. gate=%d, objecttype=%d \n", nid, sPid, szName, fPosX, fPosZ, byGateOpen, byObjectType);

		if (!g_pMain->m_arNpcArray.PutData(pNpc->m_sNid, pNpc))
		{
			TRACE("Npc PutData Fail - %d\n", pNpc->m_sNid);
			delete pNpc;
			continue;
		}

		if (bType == 0)
		{
			TRACE("Recv --> NpcUserInfoAll : nid=%d, sid=%d, name=%s\n", pNpc->m_sNid, pNpc->m_sSid, strName.c_str());
			continue;
		}

		pNpc->GetMap()->RegionNpcAdd(pNpc->m_sRegion_X, pNpc->m_sRegion_Z, pNpc->m_sNid);
	}
}

void CAISocket::RecvNpcMoveResult(Packet & pkt)
{
	uint8 flag;			// 01(INFO_MODIFY), 02(INFO_DELETE)	
	uint16 sNid;
	float fX, fY, fZ, fSecForMetor;
	pkt >> flag >> sNid >> fX >> fZ >> fY >> fSecForMetor;

	CNpc * pNpc = g_pMain->m_arNpcArray.GetData(sNid);
	if (pNpc == NULL)
		return;

	if (pNpc->isDead())
	{
		Packet result(AG_NPC_HP_REQ);
		result << sNid << pNpc->m_iHP;
		Send(&result);
	}
	
	pNpc->MoveResult(fX, fY, fZ, fSecForMetor);
}

void CAISocket::RecvNpcAttack(Packet & pkt)
{
	int index = 0, send_index = 0, sid = -1, tid = -1, nHP = 0, temp_damage = 0;
	BYTE type, bResult, byAttackType = 0;
	float fDir=0.0f;
	short damage = 0;
	CNpc* pNpc = NULL, *pMon = NULL;
	CUser* pUser = NULL;
	_OBJECT_EVENT* pEvent = NULL;

	pkt >> type >> bResult >> sid >> tid >> damage >> nHP >> byAttackType;

	//TRACE("CAISocket-RecvNpcAttack : sid=%s, tid=%d, zone_num=%d\n", sid, tid, m_iZoneNum);

	if(type == 0x01)			// user attack -> npc
	{
		pNpc = g_pMain->m_arNpcArray.GetData(tid);
		if(!pNpc)	return;
		pNpc->m_iHP -= damage;
		if( pNpc->m_iHP < 0 )
			pNpc->m_iHP = 0;

		// NPC died
		if (bResult == 4)
			pNpc->OnDeath();
		else 
		{
			Packet result(WIZ_ATTACK, byAttackType);
			result << bResult << sid << tid;
			pNpc->SendToRegion(&result);
		}

		pUser = g_pMain->GetUserPtr(sid);
		if (pUser != NULL) 
		{
			pUser->SendTargetHP( 0, tid, -damage ); 
			if( byAttackType != MAGIC_ATTACK && byAttackType != DURATION_ATTACK) {
				pUser->ItemWoreOut(ATTACK, damage);

			// LEFT HAND!!! by Yookozuna
			temp_damage = damage * pUser->m_bMagicTypeLeftHand / 100 ;

			switch (pUser->m_bMagicTypeLeftHand) {	// LEFT HAND!!!
				case ITEM_TYPE_HP_DRAIN :	// HP Drain		
					pUser->HpChange(temp_damage, 0);	
					break;
				case ITEM_TYPE_MP_DRAIN :	// MP Drain		
					pUser->MSpChange(temp_damage);
					break;
				}				
			
			temp_damage = 0;	// reset data;

			// RIGHT HAND!!! by Yookozuna
			temp_damage = damage * pUser->m_bMagicTypeRightHand / 100 ;

			switch (pUser->m_bMagicTypeRightHand) {	// LEFT HAND!!!
				case ITEM_TYPE_HP_DRAIN :	// HP Drain		
					pUser->HpChange(temp_damage, 0);			
					break;
				case ITEM_TYPE_MP_DRAIN :	// MP Drain		
					pUser->MSpChange(temp_damage);
					break;
				}	
//		
			}
		}

		if (bResult == 2 || bResult== 4)		// npc dead
		{
			pNpc->GetMap()->RegionNpcRemove(pNpc->m_sRegion_X, pNpc->m_sRegion_Z, tid);
			
//			TRACE("--- Npc Dead : Npc�� Region���� ����ó��.. ,, region_x=%d, y=%d\n", pNpc->m_sRegion_X, pNpc->m_sRegion_Z);
			pNpc->m_sRegion_X = 0;		pNpc->m_sRegion_Z = 0;
			pNpc->m_NpcState = NPC_DEAD;
			if( pNpc->m_byObjectType == SPECIAL_OBJECT )	{
				pEvent = pNpc->GetMap()->GetObjectEvent( pNpc->m_sSid );
				if( pEvent )	pEvent->byLife = 0;
			}
			if (pNpc->m_tNpcType == 2 && pUser != NULL) // EXP 
				pUser->GiveItem(900001000, 1);	
		}
	}
	else if (type == 2)		// npc attack -> user
	{
		pNpc = g_pMain->m_arNpcArray.GetData(sid);
		if(!pNpc)	return;

		//TRACE("CAISocket-RecvNpcAttack 222 : sid=%s, tid=%d, zone_num=%d\n", sid, tid, m_iZoneNum);
		if( tid >= USER_BAND && tid < NPC_BAND)
		{
			pUser = g_pMain->GetUserPtr(tid);
			if(pUser == NULL)	
				return;

			pUser->HpChange(-damage, 1, true);
			pUser->ItemWoreOut(DEFENCE, damage);

			Packet result(WIZ_ATTACK, byAttackType);
			result	<< uint8(bResult == 3 ? 0 : bResult)
					<< sid << tid;
			pNpc->SendToRegion(&result);

			//TRACE("RecvNpcAttack ==> sid = %d, tid = %d, result = %d\n", sid, tid, result);

			// user dead
			if (bResult == 2) 
			{
				if (pUser->m_bResHpType == USER_DEAD)
					return;

				pUser->OnDeath();
				pUser->m_bResHpType = USER_DEAD;
				DEBUG_LOG("*** User Dead, id=%s, result=%d, AI_HP=%d, GM_HP=%d, x=%d, z=%d", pUser->m_pUserData->m_id, result, nHP, pUser->m_pUserData->m_sHp, (int)pUser->m_pUserData->m_curx, (int)pUser->m_pUserData->m_curz);

				send_index = 0;
				if( pUser->m_pUserData->m_bFame == COMMAND_CAPTAIN )	{	// ���ֱ����� �ִ� ������ �״´ٸ�,, ���� ���� ��Ż
					pUser->ChangeFame(CHIEF);

					TRACE("---> AISocket->RecvNpcAttack() Dead Captain Deprive - %s\n", pUser->m_pUserData->m_id);
					if (pUser->getNation() == KARUS)		g_pMain->Announcement( KARUS_CAPTAIN_DEPRIVE_NOTIFY, KARUS );
					else if (pUser->getNation() == ELMORAD)	g_pMain->Announcement( ELMORAD_CAPTAIN_DEPRIVE_NOTIFY, ELMORAD );

				}

				if(pNpc->m_tNpcType == NPC_PATROL_GUARD)	{	// ���񺴿��� �״� ��������..
					pUser->ExpChange( -pUser->m_iMaxExp/100 );
					//TRACE("RecvNpcAttack : ����ġ�� 1%���� id = %s\n", pUser->m_pUserData->m_id);
				}
				else {
//
					if( pUser->m_pUserData->m_bZone != pUser->m_pUserData->m_bNation && pUser->m_pUserData->m_bZone < 3) {
						pUser->ExpChange(-pUser->m_iMaxExp / 100);
						//TRACE("������ 1%�� �￴�ٴϱ��� ��.��");
					}
//				
					else {
						pUser->ExpChange( -pUser->m_iMaxExp/20 );
					}
					//TRACE("RecvNpcAttack : ����ġ�� 5%���� id = %s\n", pUser->m_pUserData->m_id);
				}
			}
		}
		else if(tid >= NPC_BAND)		// npc attack -> monster
		{
			pMon = g_pMain->m_arNpcArray.GetData(tid);
			if(!pMon)	return;
			pMon->m_iHP -= damage;
			if( pMon->m_iHP < 0 )
				pMon->m_iHP = 0;

			Packet result(WIZ_ATTACK, byAttackType);
			result << bResult << sid << tid;
			if (bResult == 2)	{		// npc dead
				pNpc->GetMap()->RegionNpcRemove(pMon->m_sRegion_X, pMon->m_sRegion_Z, tid);
//				TRACE("--- Npc Dead : Npc�� Region���� ����ó��.. ,, region_x=%d, y=%d\n", pMon->m_sRegion_X, pMon->m_sRegion_Z);
				pMon->m_sRegion_X = 0;		pMon->m_sRegion_Z = 0;
				pMon->m_NpcState = NPC_DEAD;
				if( pNpc->m_byObjectType == SPECIAL_OBJECT )	{
					pEvent = pNpc->GetMap()->GetObjectEvent( pMon->m_sSid );
					if( pEvent )	pEvent->byLife = 0;
				}
			}
			pNpc->SendToRegion(&result);
		}
	}
}

void CAISocket::RecvMagicAttackResult(Packet & pkt)
{
	uint32 magicid;
	uint16 sid, tid;
	uint8 byCommand; 

	/* 
		This is all so redundant...
		When everything's switched over to pass in Packets
		we can just pass it through directly!
		As it is now.. we still need a length (which we can hardcode, but meh)
	*/
	pkt >> byCommand >> magicid >> sid >> tid;

	if (byCommand == MAGIC_CASTING
		|| (byCommand == MAGIC_EFFECTING && sid >= NPC_BAND && tid >= NPC_BAND))
	{
		CNpc *pNpc = g_pMain->m_arNpcArray.GetData(sid);
		if (!pNpc)
			return;

		g_pMain->Send_Region(&pkt, pNpc->GetMap(), pNpc->m_sRegion_X, pNpc->m_sRegion_Z);
	}
	else if (byCommand == MAGIC_EFFECTING)
	{
		if (sid >= USER_BAND && sid < NPC_BAND)
		{
			CUser *pUser = g_pMain->GetUserPtr(sid);
			if (pUser == NULL || pUser->isDead())
				return;

			g_pMain->Send_Region(&pkt, pUser->GetMap(), pUser->m_RegionX, pUser->m_RegionZ);
			return;
		}

		// If we're an NPC, casting a skill (rather, it's finished casting) on a player...
		pkt.rpos(0);
		m_MagicProcess.MagicPacket(pkt);
	}
	
}

void CAISocket::RecvNpcInfo(Packet & pkt)
{
	std::string strName;
	uint8 Mode;
	uint16 sNid;

	pkt.SByte();
	pkt >> Mode >> sNid;

	CNpc *pNpc = g_pMain->m_arNpcArray.GetData(sNid);
	if (pNpc == NULL)
		return;

	pkt >> pNpc->m_sSid >> pNpc->m_sPid >> pNpc->m_sSize >> pNpc->m_iWeapon_1 >> pNpc->m_iWeapon_2
		>> pNpc->m_bCurZone >> strName >> pNpc->m_byGroup >> pNpc->m_byLevel 
		>> pNpc->m_fCurX >> pNpc->m_fCurZ >> pNpc->m_fCurY >> pNpc->m_byDirection >> pNpc->m_NpcState
		>> pNpc->m_tNpcType >> pNpc->m_iSellingGroup >> pNpc->m_iMaxHP >> pNpc->m_iHP >> pNpc->m_byGateOpen
		>> pNpc->m_sHitRate >> pNpc->m_byObjectType;


	if (strName.empty() || strName.length() > MAX_NPC_SIZE)
	{
		delete pNpc;
		return;
	}

	strcpy(pNpc->m_strName, strName.c_str());

	// Bug? Test?
	// pNpc->m_NpcState = NPC_DEAD;

	if (pNpc->m_NpcState == NPC_LIVE)
	{
		char strLog[256]; 
		CTime t = CTime::GetCurrentTime();
		sprintf_s(strLog, sizeof(strLog), "## time(%d:%d-%d) npc regen check(%d) : nid=%d, name=%s, x=%d, z=%d, rx=%d, rz=%d ## \r\n", t.GetHour(), t.GetMinute(), t.GetSecond(), 
			pNpc->m_NpcState, pNpc->m_sNid, pNpc->m_strName, (int)pNpc->m_fCurX, (int)pNpc->m_fCurZ, pNpc->m_sRegion_X, pNpc->m_sRegion_Z);
		EnterCriticalSection( &g_LogFile_critical );
		g_pMain->m_RegionLogFile.Write( strLog, strlen(strLog) );
		LeaveCriticalSection( &g_LogFile_critical );
		TRACE(strLog);
		// to-do: replace with g_pMain->WriteRegionLog(...);
	}


	if (pNpc->GetMap() == NULL)
		return;

	pNpc->m_sRegion_X = (int)pNpc->m_fCurX / VIEW_DISTANCE;
	pNpc->m_sRegion_Z = (int)pNpc->m_fCurZ / VIEW_DISTANCE;

	if (pNpc->m_byObjectType == SPECIAL_OBJECT)
	{
		_OBJECT_EVENT *pEvent = pNpc->GetMap()->GetObjectEvent( pNpc->m_sSid );
		if (pEvent != NULL)
			pEvent->byLife = 1;
	}

	if (Mode == 0)
	{
		TRACE("RecvNpcInfo - dead monster nid=%d, name=%s\n", pNpc->m_sNid, pNpc->m_strName);
		return;
	}

	pNpc->NpcInOut(NPC_IN, pNpc->m_fCurX, pNpc->m_fCurZ, pNpc->m_fCurY);
}

void CAISocket::RecvUserHP(Packet & pkt)
{
	uint16 sNid;
	uint32 nHP, nMaxHP;

	pkt >> sNid >> nHP >> nMaxHP;

	// Is this such a good idea?? 
	// It should be better to let Ebenezer handle the reason for the HP change
	// that way the packet can be sent to the client etc...
	if (sNid >= USER_BAND && sNid < NPC_BAND)
	{
		CUser * pUser = g_pMain->GetUserPtr(sNid);
		if (pUser == NULL)
			return;

		pUser->m_pUserData->m_sHp = nHP;
	}
	else if (sNid >= NPC_BAND)
	{
		CNpc * pNpc = g_pMain->m_arNpcArray.GetData(sNid);
		if (pNpc == NULL)
			return;

		pNpc->m_iHP = nHP;
		pNpc->m_iMaxHP = nMaxHP;
	}
}

void CAISocket::RecvUserExp(Packet & pkt)
{
	uint16 tid, sExp, sLoyalty;
	pkt >> tid >> sExp >> sLoyalty;

	CUser* pUser = g_pMain->GetUserPtr(tid);
	if (pUser == NULL)
		return;

	if (sExp > 0)
		pUser->ExpChange(sExp);

	if (sLoyalty > 0)
		pUser->SendLoyaltyChange(sLoyalty);
}

void CAISocket::RecvSystemMsg(Packet & pkt)
{
	Packet result(WIZ_CHAT);

	std::string strSysMsg;
	uint8 bType;
	uint16 sWho;

	pkt >> bType >> sWho >> strSysMsg;

	//TRACE("RecvSystemMsg - type=%d, who=%d, len=%d, msg=%s\n", bType, sWho, sLength, strSysMsg);
	switch (sWho)
	{
	case SEND_ALL:
		result << bType << uint8(1) << int16(-1) << strSysMsg; 
		g_pMain->Send_All(&result);
		break;
	}
	
}

void CAISocket::RecvNpcGiveItem(Packet & pkt)
{
	Packet result(WIZ_ITEM_DROP);
	short sUid, sNid, regionx, regionz;
	float fX, fZ, fY;
	BYTE byCount, bZone;
	int nItemNumber[NPC_HAVE_ITEM_LIST];
	short sCount[NPC_HAVE_ITEM_LIST];
	CUser* pUser = NULL;

	pkt >> sUid >> sNid >> bZone >> regionx >> regionz >> fX >> fZ >> fY >> byCount;
	for (int i = 0; i < byCount; i++)
		pkt >> nItemNumber[i] >> sCount[i];

	if (sUid < 0 || sUid >= MAX_USER)
		return;

	C3DMap *pMap = g_pMain->GetZoneByID(bZone);
	if (pMap == NULL)
		return;

	_ZONE_ITEM *pItem = new _ZONE_ITEM;
	for(int i=0; i<6; i++) {
		pItem->itemid[i] = 0;
		pItem->count[i] = 0;
	}
	pItem->bundle_index = pMap->m_wBundle;
	pItem->time = TimeGet();
	pItem->x = fX;
	pItem->z = fZ;
	pItem->y = fY;
	for(int i=0; i<byCount; i++) {
		if( g_pMain->GetItemPtr(nItemNumber[i]) ) {
			pItem->itemid[i] = nItemNumber[i];
			pItem->count[i] = sCount[i];
		}
	}

	if (!pMap->RegionItemAdd(regionx, regionz, pItem ))
	{
		delete pItem;
		return;
	}

	pUser = g_pMain->GetUserPtr(sUid);
	if (pUser == NULL) 
		return;

	result << sNid << uint32(pItem->bundle_index);
	if (!pUser->isInParty())
		pUser->Send(&result);
	else
		g_pMain->Send_PartyMember(pUser->m_sPartyIndex, &result);
}

void CAISocket::RecvUserFail(Packet & pkt)
{
	short nid, sid;
	pkt >> nid >> sid;
	CUser* pUser = g_pMain->GetUserPtr(nid);
	if (pUser == NULL)
		return;

	pUser->HpChange(-10000, 1);

	Packet result(WIZ_ATTACK, uint8(1));
	result << uint8(2) << sid << nid;
	pUser->SendToRegion(&result);

	TRACE("### AISocket - RecvUserFail : sid=%d, tid=%d, id=%s ####\n", sid, nid, pUser->m_pUserData->m_id);
}

void CAISocket::InitEventMonster(int index)
{
	int count = index;
	if( count < 0 || count > NPC_BAND )	{
		TRACE("### InitEventMonster index Fail = %d ###\n", index);
		return;
	}

	int max_eventmop = count+EVENT_MONSTER;
	for( int i=count; i<max_eventmop; i++ )	{
		CNpc* pNpc = NULL;
		pNpc = new CNpc;
		if(pNpc == NULL) return;
		pNpc->Initialize();

		pNpc->m_sNid = i+NPC_BAND;
		//TRACE("InitEventMonster : uid = %d\n", pNpc->m_sNid);
		if( !g_pMain->m_arNpcArray.PutData( pNpc->m_sNid, pNpc) ) {
			TRACE("Npc PutData Fail - %d\n", pNpc->m_sNid);
			delete pNpc;
			pNpc = NULL;
		}	
	}

	count = g_pMain->m_arNpcArray.GetSize();
	TRACE("TotalMonster = %d\n", count);
}

void CAISocket::RecvCheckAlive(Packet & pkt)
{
	Packet result(AG_CHECK_ALIVE_REQ);		
	g_pMain->m_sErrorSocketCount = 0;
	Send(&result);
}

void CAISocket::RecvGateDestory(Packet & pkt)
{
	uint16 nid, sCurZone, rX, rZ;
	uint8 bGateStatus;
	pkt >> nid >> bGateStatus >> sCurZone >> rX >> rZ;

	CNpc* pNpc = g_pMain->m_arNpcArray.GetData(nid);
	if (pNpc == NULL)
		return;

	pNpc->m_byGateOpen = bGateStatus;
	TRACE("RecvGateDestory - (%d,%s), gate_status=%d\n", pNpc->m_sNid, pNpc->m_strName, pNpc->m_byGateOpen);
}

void CAISocket::RecvNpcDead(Packet & pkt)
{
	uint16 nid = pkt.read<uint16>();
	CNpc* pNpc = g_pMain->m_arNpcArray.GetData(nid);
	if (pNpc == NULL)
		return;

	C3DMap* pMap = pNpc->GetMap();
	if (pMap == NULL)
		return;

	if (pNpc->m_byObjectType == SPECIAL_OBJECT)
	{
		_OBJECT_EVENT *pEvent = pMap->GetObjectEvent(pNpc->m_sSid);
		if (pEvent)
			pEvent->byLife = 0;
	}

	pMap->RegionNpcRemove(pNpc->m_sRegion_X, pNpc->m_sRegion_Z, nid);

	Packet result(WIZ_DEAD);
	result << nid;
	g_pMain->Send_Region(&result, pNpc->GetMap(), pNpc->m_sRegion_X, pNpc->m_sRegion_Z);

	pNpc->m_sRegion_X = pNpc->m_sRegion_Z = 0;
}

void CAISocket::RecvNpcInOut(Packet & pkt)
{
	uint8 bType;
	uint16 sNid;
	float fX, fZ, fY;

	pkt >> bType >> sNid >> fX >> fZ >> fY;
	CNpc * pNpc = g_pMain->m_arNpcArray.GetData(sNid);
	if (pNpc)
		pNpc->NpcInOut(bType, fX, fZ, fY);
}

void CAISocket::RecvBattleEvent(Packet & pkt)
{
	int retvalue = 0;
	std::string strMaxUserName;
	char strKnightsName[MAX_ID_SIZE+1];
	char chatstr[1024];
	CUser* pUser = NULL;
	CKnights* pKnights = NULL;

	uint8 bType, bResult;
	pkt >> bType >> bResult;

	if (bType == BATTLE_EVENT_OPEN)
	{
	}
	else if (bType == BATTLE_MAP_EVENT_RESULT)
	{
		if (g_pMain->m_byBattleOpen == NO_BATTLE)
		{
			TRACE("#### RecvBattleEvent Fail : battleopen = %d, type = %d\n", g_pMain->m_byBattleOpen, bType);
			return;
		}

		if (bResult == KARUS)
			g_pMain->m_byKarusOpenFlag = 1;	
		else if (bResult == ELMORAD)
			g_pMain->m_byElmoradOpenFlag = 1;

		Packet result(UDP_BATTLE_EVENT_PACKET, bType);
		result << bResult;
		g_pMain->Send_UDP_All(&result);
	}
	else if (bType == BATTLE_EVENT_RESULT)
	{
		if (g_pMain->m_byBattleOpen == NO_BATTLE)
		{
			TRACE("#### RecvBattleEvent Fail : battleopen = %d, type=%d\n", g_pMain->m_byBattleOpen, bType);
			return;
		}

		pkt.SByte();
		pkt >> strMaxUserName;

		if (!strMaxUserName.empty())
		{
			if (g_pMain->m_byBattleSave == 0)
			{
				Packet result(WIZ_BATTLE_EVENT, bType);
				result.SByte();
				result << bResult << strMaxUserName;

				g_pMain->m_LoggerSendQueue.PutData(&result);
				g_pMain->m_byBattleSave = 1;
			}
		}

		g_pMain->m_bVictory = bResult;
		g_pMain->m_byOldVictory = bResult;
		g_pMain->m_byKarusOpenFlag = 0;
		g_pMain->m_byElmoradOpenFlag = 0;
		g_pMain->m_byBanishFlag = 1;

		Packet result(UDP_BATTLE_EVENT_PACKET, bType);
		result << bResult;
		g_pMain->Send_UDP_All(&result);
	}
	else if (bType == BATTLE_EVENT_MAX_USER)
	{
		pkt.SByte();
		pkt >> strMaxUserName;

		if (!strMaxUserName.empty())
		{
			pUser = g_pMain->GetUserPtr(strMaxUserName.c_str(), TYPE_CHARACTER);
			if (pUser != NULL)
			{
				pKnights = g_pMain->GetClanPtr(pUser->m_pUserData->m_bKnights);
				if (pKnights)
					strcpy_s(strKnightsName, sizeof(strKnightsName), pKnights->m_strName);

				/* War rewards */
				// Warders
				if (bResult >= 3 && bResult <= 6)
					pUser->ChangeNP(500); /* TO-DO: Remove hardcoded values */
				// Keeper
				else if (bResult == 8)
					pUser->ChangeNP(1000);
			}
		}

		int nResourceID = 0;
		switch (bResult)
		{
		case 1: // captain
			nResourceID = IDS_KILL_CAPTAIN;
			break;
		case 3: // Karus warder 1
			nResourceID = IDS_KILL_KARUS_GUARD1;
			break;
		case 4: // Karus warder 2
			nResourceID = IDS_KILL_KARUS_GUARD2;
			break;
		case 5: // El Morad warder 1
			nResourceID = IDS_KILL_ELMO_GUARD1;
			break;
		case 6: // El Morad warder 2
			nResourceID = IDS_KILL_ELMO_GUARD2;
			break;
		case 8: // Keeper
			nResourceID = IDS_KILL_GATEKEEPER;
			break;
		}

		if (nResourceID == 0)
		{
			TRACE("RecvBattleEvent: could not establish resource for result %d", bResult);
			return;
		}

		_snprintf(chatstr, sizeof(chatstr), g_pMain->GetServerResource(nResourceID), strKnightsName, strMaxUserName);

#if 0
		send_index = 0;
		sprintf( finalstr, g_pMain->GetServerResource(IDP_ANNOUNCEMENT), chatstr );
		SetByte( send_buff, WIZ_CHAT, send_index );
		SetByte( send_buff, WAR_SYSTEM_CHAT, send_index );
		SetByte( send_buff, 1, send_index );
		SetShort( send_buff, -1, send_index );
		SetKOString( send_buff, finalstr, send_index );
		g_pMain->Send_All( send_buff, send_index );

		send_index = 0;
		SetByte( send_buff, WIZ_CHAT, send_index );
		SetByte( send_buff, PUBLIC_CHAT, send_index );
		SetByte( send_buff, 1, send_index );
		SetShort( send_buff, -1, send_index );
		SetKOString( send_buff, finalstr, send_index );
		g_pMain->Send_All( send_buff, send_index );
#endif

		Packet result(UDP_BATTLE_EVENT_PACKET, bType);
		result.SByte();
		result << bResult << strKnightsName << strMaxUserName;
		g_pMain->Send_UDP_All(&result);
	}
}


void CAISocket::RecvNpcEventItem(Packet & pkt)
{
	uint16 sUid, sNid;
	uint32 nItemID, nCount;

	pkt >> sUid >> sNid >> nItemID >> nCount;

	CUser *pUser = g_pMain->GetUserPtr(sUid);
	if (pUser == NULL)
		return;

	pUser->EventMoneyItemGet(nItemID, nCount);
}

void CAISocket::RecvGateOpen(Packet & pkt)
{
	uint16 sNid, sEventID; 
	uint8 bFlag;

	pkt >> sNid >> sEventID >> bFlag;

	CNpc *pNpc = g_pMain->m_arNpcArray.GetData(sNid);
	if (pNpc == NULL)	
	{
		TRACE("#### RecvGateOpen Npc Pointer null : nid=%d ####\n", sNid);
		return;
	}

	pNpc->m_byGateOpen = bFlag; // possibly not needed (we'll do it below), but need to make sure.

	_OBJECT_EVENT *pEvent = pNpc->GetMap()->GetObjectEvent(sEventID);
	if (pEvent == NULL)	
	{
		TRACE("#### RecvGateOpen Npc Object fail : nid=%d, sid=%d ####\n", sNid, sEventID);
		return;
	}

	if (pNpc->isGate())
		pNpc->SendGateFlag(bFlag, false);
}

void CAISocket::RecvCompressed(Packet & pkt)
{
#if __VERSION >= 1900 // 32-bit
	uint32 compressedLength, originalLength;
#else
	uint16 compressedLength, originalLength;
#endif
	uint32 crc;
	pkt >> compressedLength >> originalLength >> crc;

	char *decompressedBuffer = new char[originalLength];

	// Does the length match what it's supposed to be
	uint32 result = lzf_decompress(pkt.contents() + pkt.rpos(), compressedLength, decompressedBuffer, originalLength);
	if (result
		!= originalLength)
	{
		return;
	}

	pkt.Initialize(*decompressedBuffer);
	if (originalLength > 1)
		pkt.append(decompressedBuffer + 1, originalLength - 1);

	HandlePacket(pkt);
}
