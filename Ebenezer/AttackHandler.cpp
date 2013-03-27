#include "StdAfx.h"
#include "Map.h"
#include "EbenezerDlg.h"
#include "User.h"

void CUser::Attack(Packet & pkt)
{
	Packet result;
	int16 sid = -1, tid = -1, damage, delaytime, distance;	
	uint8 bType, bResult;	
	
	CUser* pTUser = NULL;

	pkt >> bType >> bResult >> tid >> delaytime >> distance;

//	delaytime = delaytime / 100.0f;
//	distance = distance / 10.0f;

	if (isBlinking()
		|| isDead())
		return;

	// If you're holding a weapon, do a client-based (ugh, do not trust!) delay check.
	_ITEM_TABLE *pTable = GetItemPrototype(RIGHTHAND);
	if (pTable != NULL) 
	{
		if (delaytime < pTable->m_sDelay
			|| distance > pTable->m_sRange)
			return;	
	}
	// Empty handed.
	else if (delaytime < 100)
		return;			

	// We're attacking a player...
	if (tid < MAX_USER)
	{
		pTUser = g_pMain.GetUserPtr(tid);
 
		if (pTUser == NULL || pTUser->isDead() || pTUser->isBlinking()
				|| (pTUser->GetNation() == GetNation() && GetZoneID() != 48 /* TO-DO: implement better checks */)
				|| !isAttackZone()) 
			bResult = 0;
		else 
		{
			damage = GetDamage(pTUser, NULL);
			if (GetZoneID() == ZONE_SNOW_BATTLE && g_pMain.m_byBattleOpen == SNOW_BATTLE)
				damage = 0;		

			if (damage <= 0)
				bResult = 0;
			else 
			{
				// TO-DO: Move all this redundant code into appropriate event-based methods so that all the other cases don't have to copypasta (and forget stuff).
				pTUser->HpChange(-damage, this);
				if (pTUser->isDead())
					bResult = 2;

				ItemWoreOut(ATTACK, damage);
				pTUser->ItemWoreOut(DEFENCE, damage);
				SendTargetHP(0, tid, -damage);
			}
		}
	}
	// We're attacking an NPC...
	else if (tid >= NPC_BAND)
	{
		// AI hasn't loaded yet
		if (g_pMain.m_bPointCheckFlag == false)	
			return;	

		CNpc *pNpc = g_pMain.m_arNpcArray.GetData(tid);		
		if (pNpc != NULL && pNpc->isAlive() 
			&& (pNpc->GetNation() == 0
				|| pNpc->GetNation() != GetNation()))
		{
			result.SetOpcode(AG_ATTACK_REQ);
			result	<< bType << bResult
					<< GetSocketID() << tid
					<< uint16(m_sTotalHit * m_bAttackAmount / 100)
					<< uint16(m_sTotalAc + m_sACAmount)
					<< m_sTotalHitrate /* this is actually a float. screwed up naming... */
					<< m_sTotalEvasionrate /* also a float */
					<< m_sItemAc
					<< m_bMagicTypeLeftHand << m_bMagicTypeRightHand
					<< m_sMagicAmountLeftHand << m_sMagicAmountRightHand;
			Send_AIServer(&result);	
			return;
		}
	}

	result.SetOpcode(WIZ_ATTACK);
	result << bType << bResult << GetSocketID() << tid;
	SendToRegion(&result);

	if (tid < NPC_BAND
		&& bResult == 2 // 2 means a player died.
		&& pTUser) 
	{
		pTUser->Send(&result);
		TRACE("*** User Attack Dead, id=%s, result=%d, type=%d, HP=%d\n", pTUser->GetName(), bResult, pTUser->m_bResHpType, pTUser->m_sHp);
	}
}

void CUser::RecvRegene(Packet & pkt)
{
	uint8 regene_type = pkt.read<uint8>();
	Regene(regene_type);
}

void CUser::Regene(uint8 regene_type, uint32 magicid /*= 0*/)
{
	ASSERT(GetMap() != NULL);

	CUser* pUser = NULL;
	_OBJECT_EVENT* pEvent = NULL;
	_HOME_INFO* pHomeInfo = NULL;
	_MAGIC_TYPE5* pType = NULL;

	if (!isDead())
		return;

	InitType3();
	InitType4();

	if (regene_type != 1 && regene_type != 2) {
		regene_type = 1;
	}

	if (regene_type == 2) {
		magicid = 490041;	// The Stone of Ressurection magic ID

		if (!RobItem(379006000, 3 * GetLevel())) {
			return;	// Subtract resurrection stones.
		}

		if (GetLevel() <= 5) {
			return;	// 5 level minimum.
		}
	}

	pHomeInfo = g_pMain.m_HomeArray.GetData(m_bNation);
	if (!pHomeInfo) return;

	UserInOut(INOUT_OUT);

	float x = 0.0f, z = 0.0f;
	x = (float)(myrand( 0, 400 )/100.0f);	z = (float)(myrand( 0, 400 )/100.0f);
	if( x < 2.5f )	x = 1.5f + x;
	if( z < 2.5f )	z = 1.5f + z;

	pEvent = GetMap()->GetObjectEvent(m_sBind);	

	// TO-DO: Clean this entire thing up. Wow.
	if (magicid == 0) {
		if( pEvent && pEvent->byLife == 1 ) {		// Bind Point
			m_curx = pEvent->fPosX + x;
			m_curz = pEvent->fPosZ + z;
			m_cury = 0;
		}
		else if( m_bNation != m_bZone) {	// Free Zone or Opposite Zone
			if(m_bZone > 200) {		// Frontier Zone...
				x = (float)(pHomeInfo->FreeZoneX + myrand(0, pHomeInfo->FreeZoneLX));
				z = (float)(pHomeInfo->FreeZoneZ + myrand(0, pHomeInfo->FreeZoneLZ));
			}
//
			else if(m_bZone > 100 && m_bZone < 200) {		// Battle Zone...
/*
				m_bResHpType = USER_STANDING;
				HpChange( m_iMaxHp );
				KickOutZoneUser();	// Go back to your own zone!
				return;
*/
				x = (float)(pHomeInfo->BattleZoneX + myrand(0, pHomeInfo->BattleZoneLX));
				z = (float)(pHomeInfo->BattleZoneZ + myrand(0, pHomeInfo->BattleZoneLZ));
				if (m_bZone == ZONE_SNOW_BATTLE) {
					x = (float)(pHomeInfo->FreeZoneX + myrand(0, pHomeInfo->FreeZoneLX));
					z = (float)(pHomeInfo->FreeZoneZ + myrand(0, pHomeInfo->FreeZoneLZ));					
				}
			}
			else if (m_bZone > 10 && m_bZone < 20) {
				x = (float)(527 + myrand(0, 10));
				z = (float)(543 + myrand(0, 10));
			}
			else if (m_bZone < 3) {	// Specific Lands...
				if (m_bNation == KARUS) {
					x = (float)(pHomeInfo->ElmoZoneX + myrand(0, pHomeInfo->ElmoZoneLX));
					z = (float)(pHomeInfo->ElmoZoneZ + myrand(0, pHomeInfo->ElmoZoneLZ));			
				}
				else if (m_bNation == ELMORAD) {
					x = (float)(pHomeInfo->KarusZoneX + myrand(0, pHomeInfo->KarusZoneLX));
					z = (float)(pHomeInfo->KarusZoneZ + myrand(0, pHomeInfo->KarusZoneLZ));	
				}		
				else return;
			}
			else
			{
				short sx, sz;
				GetStartPosition(sx, sz);

				x = sx;
				z = sz;
			}

			m_curx = x;
			m_curz = z;
		}
		else {	
			if (m_bNation == KARUS) {
				x = (float)(pHomeInfo->KarusZoneX + myrand(0, pHomeInfo->KarusZoneLX));
				z = (float)(pHomeInfo->KarusZoneZ + myrand(0, pHomeInfo->KarusZoneLZ));			
			}
			else if (m_bNation == ELMORAD) {			
				x = (float)(pHomeInfo->ElmoZoneX + myrand(0, pHomeInfo->ElmoZoneLX));
				z = (float)(pHomeInfo->ElmoZoneZ + myrand(0, pHomeInfo->ElmoZoneLZ));
			}		
			else return;		

			m_curx = x;
			m_curz = z;
		}
	}

	Packet result(WIZ_REGENE);
	result << GetSPosX() << GetSPosZ() << GetSPosY();
	Send(&result);
	
	if (magicid > 0) {	// Clerical Resurrection.
		pType = g_pMain.m_Magictype5Array.GetData(magicid);     
		if ( !pType ) return;

		m_bResHpType = USER_STANDING;
		MSpChange(-m_iMaxMp);					// Empty out MP.

		if (m_sWhoKilledMe == -1 && regene_type == 1) {		
			ExpChange((m_iLostExp * pType->bExpRecover) / 100);		// Restore Target Experience.
		}

		m_bRegeneType = REGENE_MAGIC;
	}
	else {		// Normal Regene.
//
		m_bAbnormalType = ABNORMAL_BLINKING;
//
		m_bResHpType = USER_STANDING;	
		m_bRegeneType = REGENE_NORMAL;
	}

	HpChange(m_iMaxHp);

	m_tLastRegeneTime = UNIXTIME;
	m_sWhoKilledMe = -1;
	m_iLostExp = 0;

	if (!isBlinking())
	{
		result.Initialize(AG_USER_REGENE);
		result << GetSocketID() << m_sHp;
		Send_AIServer(&result);
	}

	SetRegion(GetNewRegionX(), GetNewRegionZ());

	UserInOut(INOUT_RESPAWN);		

	g_pMain.RegionUserInOutForMe(this);
	g_pMain.RegionNpcInfoForMe(this);

	BlinkStart();

	if (isInParty())
	{
		// TO-DO: Wrap these up into Party-specific methods (nothing for that yet)
		// UPDATE: Sticking them in the CUser class for the moment. Need to have them make sense, though.
		if (!m_bType3Flag)
			SendPartyStatusUpdate(1);
 
		if (!m_bType4Flag)
			SendPartyStatusUpdate(2);
	}
}