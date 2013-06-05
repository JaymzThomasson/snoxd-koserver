#include "stdafx.h"
#include "Map.h"
#include "EbenezerDlg.h"
#include "AISocket.h"
#include "User.h"
#include "MagicProcess.h"

using std::string;

bool CAISocket::HandlePacket(Packet & pkt)
{
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
		// The AI server should send magic system requests to us.
		// It shouldn't have to duplicate all the processing code.
		case AG_MAGIC_ATTACK_REQ:
			CMagicProcess::MagicPacket(pkt);
			break;
		// This will probably need to be removed eventually.
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
	KOSocket::OnConnect();

	// Set a flag to indicate whether we've ever connected before or not
	// This is used accordingly by the AI server when we tell it what our status is.
	// Kind of messy, and needs looking into further.
	m_bHasConnected = true; 
}

void CAISocket::OnDisconnect()
{
	TRACE("*** CloseProcess - socketID=%d...  ***\n", GetSocketID());
	g_pMain->DeleteAllNpcList();
	g_pMain->m_sErrorSocketCount = 3; // yup, we're definitely disconnected (magic number!)
}

void CAISocket::LoginProcess(Packet & pkt)
{
	uint8 bReconnect = pkt.read<uint8>();

	TRACE("Connected to the AI server\n");
	if (bReconnect == 1)
		TRACE("**** ReConnect - socket = %d ****\n ", GetSocketID());

	g_pMain->m_bServerCheckFlag = true;
	g_pMain->SendAllUserInfo();
}

void CAISocket::RecvServerInfo(Packet & pkt)
{
	int size = g_pMain->m_ZoneArray.GetSize();
	uint16 sTotalMonster;
	uint8 bZone;

	pkt >> bZone >> sTotalMonster;

	g_pMain->m_sZoneCount++;

	if (g_pMain->m_sZoneCount == size)
	{
#if 0
		if (g_pMain->m_bFirstServerFlag == false)
			TRACE("+++ �������� ���� ������ �� �޾���, User AcceptThread Start ....%d, socketcount=%d\n", bZone, g_pMain->m_sZoneCount);
#endif

		g_pMain->m_sZoneCount = 0;
		g_pMain->m_bFirstServerFlag = true;
		g_pMain->m_bPointCheckFlag = true;

		printf("All spawn data loaded.\n");
	}
}

void CAISocket::RecvNpcInfoAll(Packet & pkt)
{
	uint8 bCount = pkt.read<uint8>(); // max of 20

	pkt.SByte();
	for (int i = 0; i < bCount; i++)
	{
		uint8 bDirection;
		std::string strName;

		CNpc* pNpc = new CNpc();
		pNpc->Initialize();

		pkt >> pNpc->m_NpcState >> pNpc->m_sNid >> pNpc->m_sSid >> pNpc->m_sPid >> pNpc->m_sSize >> pNpc->m_iWeapon_1 >> pNpc->m_iWeapon_2
			>> pNpc->m_bZone >> strName >> pNpc->m_bNation >> pNpc->m_bLevel
			>> pNpc->m_curx >> pNpc->m_curz >> pNpc->m_cury >> bDirection >> pNpc->m_NpcState
			>> pNpc->m_tNpcType >> pNpc->m_iSellingGroup >> pNpc->m_iMaxHP >> pNpc->m_iHP >> pNpc->m_byGateOpen
			>> pNpc->m_sTotalHitrate >> pNpc->m_sTotalEvasionrate >> pNpc->m_sTotalAc >> pNpc->m_byObjectType
			>> pNpc->m_byTrapNumber >> pNpc->m_bMonster;

		if (strName.empty())
			strName = "<the spawn with no name>";

		if (strName.length() > MAX_NPC_SIZE)
		{
			pNpc->DecRef();
			continue;
		}

		pNpc->m_pMap = g_pMain->GetZoneByID(pNpc->GetZoneID());
		if (pNpc->GetMap() == nullptr)
		{
			pNpc->DecRef();
			continue;
		}

		//TRACE("Recv --> NpcUserInfo : uid = %d, x=%f, z=%f.. \n", nid, fPosX, fPosZ);
		pNpc->m_strName = strName;

		pNpc->m_byDirection = bDirection;
		pNpc->SetRegion(pNpc->GetNewRegionX(), pNpc->GetNewRegionZ());

		if (pNpc->m_byObjectType == SPECIAL_OBJECT)
		{
			_OBJECT_EVENT* pEvent = pNpc->GetMap()->GetObjectEvent(pNpc->m_sSid);
			if (pEvent != nullptr)
				pEvent->byLife = 1;
		}

	//	TRACE("Recv --> NpcUserInfoAll : uid=%d, sid=%d, name=%s, x=%f, z=%f. gate=%d, objecttype=%d \n", nid, sPid, szName, fPosX, fPosZ, byGateOpen, byObjectType);

		if (!g_pMain->m_arNpcArray.PutData(pNpc->GetID(), pNpc))
		{
			TRACE("Npc PutData Fail - %d\n", pNpc->GetID());
			pNpc->DecRef();
			continue;
		}

		if (pNpc->m_NpcState == NPC_DEAD)
		{
			TRACE("Recv --> NpcUserInfoAll : nid=%d, sid=%d, name=%s\n", pNpc->GetID(), pNpc->m_sSid, strName.c_str());
			continue;
		}

		pNpc->SendInOut(INOUT_IN, pNpc->GetX(), pNpc->GetZ(), pNpc->GetY());
	}
}

void CAISocket::RecvNpcMoveResult(Packet & pkt)
{
	uint8 flag;			// 01(INFO_MODIFY), 02(INFO_DELETE)	
	uint16 sNid;
	float fX, fY, fZ, fSecForMetor;
	pkt >> flag >> sNid >> fX >> fZ >> fY >> fSecForMetor;

	CNpc * pNpc = g_pMain->m_arNpcArray.GetData(sNid);
	if (pNpc == nullptr)
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
	int nHP = 0, temp_damage = 0;
	int16 sid, tid;
	uint8 type, bResult, byAttackType = 0;
	float fDir=0.0f;
	short damage = 0;
	CNpc* pNpc = nullptr, *pMon = nullptr;
	CUser* pUser = nullptr;
	_OBJECT_EVENT* pEvent = nullptr;

	pkt >> type >> bResult >> sid >> tid >> damage >> nHP >> byAttackType;

	//TRACE("CAISocket-RecvNpcAttack : sid=%s, tid=%d, zone_num=%d\n", sid, tid, m_iZoneNum);

	if(type == 0x01)			// user attack -> npc
	{
		pNpc = g_pMain->m_arNpcArray.GetData(tid);
		if(!pNpc)	return;
		pNpc->m_iHP -= damage;
		if( pNpc->m_iHP < 0 )
			pNpc->m_iHP = 0;

		pUser = g_pMain->GetUserPtr(sid);

		// NPC died
		if (bResult == 2 || bResult == 4)
			pNpc->OnDeath(pUser);
		else 
		{
			Packet result(WIZ_ATTACK, byAttackType);
			result << bResult << sid << tid;
			pNpc->SendToRegion(&result);
		}

		if (pUser != nullptr) 
		{
			if( byAttackType != MAGIC_ATTACK && byAttackType != DURATION_ATTACK) {
				pUser->ItemWoreOut(ATTACK, damage);

			temp_damage = damage * pUser->m_bMagicTypeLeftHand / 100 ;

			switch (pUser->m_bMagicTypeLeftHand) {	// LEFT HAND!!!
				case ITEM_TYPE_HP_DRAIN :	// HP Drain		
					pUser->HpChange(temp_damage);	
					break;
				case ITEM_TYPE_MP_DRAIN :	// MP Drain		
					pUser->MSpChange(temp_damage);
					break;
				}				
			
			temp_damage = 0;	// reset data;

			temp_damage = damage * pUser->m_bMagicTypeRightHand / 100 ;

			switch (pUser->m_bMagicTypeRightHand) {	// LEFT HAND!!!
				case ITEM_TYPE_HP_DRAIN :	// HP Drain		
					pUser->HpChange(temp_damage);			
					break;
				case ITEM_TYPE_MP_DRAIN :	// MP Drain		
					pUser->MSpChange(temp_damage);
					break;
				}	
//		
			}
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
			if(pUser == nullptr)	
				return;

			pUser->HpChange(-damage, pNpc, false);
			pUser->ItemWoreOut(DEFENCE, damage);

			Packet result(WIZ_ATTACK, byAttackType);
			result	<< uint8(bResult == 3 ? 0 : bResult)
					<< sid << tid;
			pNpc->SendToRegion(&result);

			//TRACE("RecvNpcAttack ==> sid = %d, tid = %d, result = %d\n", sid, tid, result);
		}
		else if (tid >= NPC_BAND)		// npc attack -> monster
		{
			pMon = g_pMain->m_arNpcArray.GetData(tid);
			if(!pMon)	return;
			pMon->m_iHP -= damage;
			if( pMon->m_iHP < 0 )
				pMon->m_iHP = 0;

			Packet result(WIZ_ATTACK, byAttackType);
			result << bResult << sid << tid;
			pNpc->SendToRegion(&result);
			if (bResult == 2)	{		// npc dead
				pMon->OnDeath(pNpc);
			}
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

	pkt.SetOpcode(WIZ_MAGIC_PROCESS);
	if (byCommand == MAGIC_CASTING
		|| (byCommand == MAGIC_EFFECTING && sid >= NPC_BAND && tid >= NPC_BAND))
	{
		CNpc *pNpc = g_pMain->m_arNpcArray.GetData(sid);
		if (!pNpc)
			return;

		pNpc->SendToRegion(&pkt);
	}
	else if (byCommand == MAGIC_EFFECTING)
	{
		if (sid >= USER_BAND && sid < NPC_BAND)
		{
			CUser *pUser = g_pMain->GetUserPtr(sid);
			if (pUser == nullptr || pUser->isDead())
				return;

			pUser->SendToRegion(&pkt);
			return;
		}

		// If we're an NPC, casting a skill (rather, it's finished casting) on a player...
		pkt.rpos(0);
		CMagicProcess::MagicPacket(pkt, false);
	}
	
}

void CAISocket::RecvNpcInfo(Packet & pkt)
{
	std::string strName;
	uint8 Mode, byDirection;
	uint16 sNid;

	pkt.SByte();
	pkt >> Mode >> sNid;

	CNpc *pNpc = g_pMain->m_arNpcArray.GetData(sNid);
	if (pNpc == nullptr)
		return;

	pkt >> pNpc->m_sSid >> pNpc->m_sPid >> pNpc->m_sSize >> pNpc->m_iWeapon_1 >> pNpc->m_iWeapon_2
		>> pNpc->m_bZone >> strName >> pNpc->m_bNation >> pNpc->m_bLevel
		>> pNpc->m_curx >> pNpc->m_curz >> pNpc->m_cury >> byDirection >> pNpc->m_NpcState
		>> pNpc->m_tNpcType >> pNpc->m_iSellingGroup >> pNpc->m_iMaxHP >> pNpc->m_iHP >> pNpc->m_byGateOpen
		>> pNpc->m_sTotalHitrate >> pNpc->m_sTotalEvasionrate >> pNpc->m_sTotalAc >> pNpc->m_byObjectType
		>> pNpc->m_byTrapNumber >> pNpc->m_bMonster;

	if (strName.empty() || strName.length() > MAX_NPC_SIZE)
	{
		pNpc->DecRef();
		return;
	}

	pNpc->m_byDirection = byDirection;
	pNpc->m_strName = strName;

	if (pNpc->GetMap() == nullptr)
		return;

	pNpc->InsertRegion(pNpc->GetNewRegionX(), pNpc->GetNewRegionZ());
	pNpc->SetRegion(pNpc->GetNewRegionX(), pNpc->GetNewRegionZ());

	if (pNpc->m_byObjectType == SPECIAL_OBJECT)
	{
		_OBJECT_EVENT *pEvent = pNpc->GetMap()->GetObjectEvent( pNpc->m_sSid );
		if (pEvent != nullptr)
			pEvent->byLife = 1;
	}

	if (Mode == 0)
	{
		TRACE("RecvNpcInfo - dead monster nid=%d, name=%s\n", pNpc->GetID(), pNpc->m_strName);
		return;
	}

	pNpc->SendInOut(INOUT_IN, pNpc->GetX(), pNpc->GetZ(), pNpc->GetY());
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
		if (pUser == nullptr)
			return;

		pUser->m_sHp = nHP;
	}
	else if (sNid >= NPC_BAND)
	{
		CNpc * pNpc = g_pMain->m_arNpcArray.GetData(sNid);
		if (pNpc == nullptr)
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
	if (pUser == nullptr)
		return;

	if (sExp > 0)
		pUser->ExpChange(sExp);

	if (sLoyalty > 0)
		pUser->SendLoyaltyChange(sLoyalty);
}

void CAISocket::RecvSystemMsg(Packet & pkt)
{
	Packet result;
	std::string strSysMsg;
	uint8 bType;

	pkt >> bType >> strSysMsg;

	ChatPacket::Construct(&result, bType, &strSysMsg);
	g_pMain->Send_All(&result);
}

void CAISocket::RecvNpcGiveItem(Packet & pkt)
{
	Packet result(WIZ_ITEM_DROP);
	short sUid, sNid, regionx, regionz;
	float fX, fZ, fY;
	uint8 byCount, bZone;
	int nItemNumber[NPC_HAVE_ITEM_LIST];
	short sCount[NPC_HAVE_ITEM_LIST];
	CUser* pUser = nullptr;
	pkt >> sUid >> sNid >> bZone >> regionx >> regionz >> fX >> fZ >> fY >> byCount;

	for (int i = 0; i < byCount; i++)
		pkt >> nItemNumber[i] >> sCount[i];

	if (sUid < 0 || sUid >= MAX_USER)
		return;

	C3DMap *pMap = g_pMain->GetZoneByID(bZone);
	if (pMap == nullptr)
		return;

	_LOOT_BUNDLE * pBundle = new _LOOT_BUNDLE;

	pBundle->tDropTime = UNIXTIME;
	pBundle->x = fX;
	pBundle->z = fZ;
	pBundle->y = fY;

	for (int i = 0; i < byCount; i++)
	{
		if (g_pMain->GetItemPtr(nItemNumber[i]))
		{
			_LOOT_ITEM pItem(nItemNumber[i], sCount[i]);
			pBundle->Items.push_back(pItem); // emplace_back() would be so much more useful here, but requires C++11.
		}
	}

	if (!pMap->RegionItemAdd(regionx, regionz, pBundle))
	{
		delete pBundle;
		return;
	}

	pUser = g_pMain->GetUserPtr(sUid);
	if (pUser == nullptr) 
		return;

	result << sNid << pBundle->nBundleID;
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
	if (pUser == nullptr)
		return;

	// wtf is this I don't even
	pUser->HpChange(-10000, nullptr, false);

	Packet result(WIZ_ATTACK, uint8(1));
	result << uint8(2) << sid << nid;
	pUser->SendToRegion(&result);

	TRACE("### AISocket - RecvUserFail : sid=%d, tid=%d, id=%s ####\n", sid, nid, pUser->GetName().c_str());
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
		CNpc* pNpc = new CNpc();
		pNpc->m_sNid = i+NPC_BAND;
		//TRACE("InitEventMonster : uid = %d\n", pNpc->m_sNid);
		if (!g_pMain->m_arNpcArray.PutData(pNpc->GetID(), pNpc))
		{
			TRACE("Npc PutData Fail - %d\n", pNpc->GetID());
			pNpc->DecRef();
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
	if (pNpc == nullptr)
		return;

	pNpc->m_byGateOpen = bGateStatus;
	TRACE("RecvGateDestory - (%d,%s), gate_status=%d\n", pNpc->GetID(), pNpc->m_strName, pNpc->m_byGateOpen);
}

// TO-DO: Remove this. NPCs don't just randomly die, it would make sense to do this as a result of the cause, not just because.
void CAISocket::RecvNpcDead(Packet & pkt)
{
	uint16 nid = pkt.read<uint16>();
	CNpc* pNpc = g_pMain->m_arNpcArray.GetData(nid);
	if (pNpc == nullptr)
		return;

	C3DMap* pMap = pNpc->GetMap();
	if (pMap == nullptr)
		return;

	pNpc->OnDeath(nullptr);
}

void CAISocket::RecvNpcInOut(Packet & pkt)
{
	uint8 bType;
	uint16 sNid;
	float fX, fZ, fY;

	pkt >> bType >> sNid >> fX >> fZ >> fY;
	CNpc * pNpc = g_pMain->m_arNpcArray.GetData(sNid);
	if (pNpc)
		pNpc->SendInOut(bType, fX, fZ, fY);
}

void CAISocket::RecvBattleEvent(Packet & pkt)
{
	string chatstr, strMaxUserName, strKnightsName;
	CUser* pUser = nullptr;
	CKnights* pKnights = nullptr;

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

				g_pMain->AddDatabaseRequest(result);
				g_pMain->m_byBattleSave = 1;
			}
		}

		g_pMain->m_bVictory = bResult;
		g_pMain->m_byOldVictory = bResult;
		g_pMain->m_byKarusOpenFlag = 0;
		g_pMain->m_byElmoradOpenFlag = 0;
		g_pMain->m_byBanishFlag = 1;
	}
	else if (bType == BATTLE_EVENT_MAX_USER)
	{
		pkt.SByte();
		pkt >> strMaxUserName;

		if (!strMaxUserName.empty())
		{
			pUser = g_pMain->GetUserPtr(strMaxUserName, TYPE_CHARACTER);
			if (pUser != nullptr)
			{
				pKnights = g_pMain->GetClanPtr(pUser->GetClanID());
				if (pKnights)
					strKnightsName = pKnights->m_strName;

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

		g_pMain->GetServerResource(nResourceID, &strKnightsName, strMaxUserName.c_str());

		Packet result;
		string finalstr;

		g_pMain->GetServerResource(IDP_ANNOUNCEMENT, &finalstr, chatstr.c_str());
		ChatPacket::Construct(&result, WAR_SYSTEM_CHAT, &finalstr);
		g_pMain->Send_All(&result);

		ChatPacket::Construct(&result, PUBLIC_CHAT, &finalstr);
		g_pMain->Send_All(&result);
	}
}


void CAISocket::RecvNpcEventItem(Packet & pkt)
{
	uint16 sUid, sNid;
	uint32 nItemID, nCount;

	pkt >> sUid >> sNid >> nItemID >> nCount;

	CUser *pUser = g_pMain->GetUserPtr(sUid);
	if (pUser == nullptr)
		return;

	pUser->GiveItem(nItemID, nCount);
}

void CAISocket::RecvGateOpen(Packet & pkt)
{
	uint16 sNid, sEventID; 
	uint8 bFlag;

	pkt >> sNid >> sEventID >> bFlag;

	CNpc *pNpc = g_pMain->m_arNpcArray.GetData(sNid);
	if (pNpc == nullptr)	
	{
		TRACE("#### RecvGateOpen Npc Pointer null : nid=%d ####\n", sNid);
		return;
	}

	pNpc->m_byGateOpen = bFlag; // possibly not needed (we'll do it below), but need to make sure.

	_OBJECT_EVENT *pEvent = pNpc->GetMap()->GetObjectEvent(sEventID);
	if (pEvent == nullptr)	
	{
		TRACE("#### RecvGateOpen Npc Object fail : nid=%d, sid=%d ####\n", sNid, sEventID);
		return;
	}

	if (pNpc->isGate())
		pNpc->SendGateFlag(bFlag, false);
}

void CAISocket::RecvCompressed(Packet & pkt)
{
#if __VERSION >= 1800 // 32-bit
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
		delete [] decompressedBuffer;
		return;
	}

	pkt.Initialize(*decompressedBuffer);
	if (originalLength > 1)
		pkt.append(decompressedBuffer + 1, originalLength - 1);

	delete [] decompressedBuffer;

	HandlePacket(pkt);
}
