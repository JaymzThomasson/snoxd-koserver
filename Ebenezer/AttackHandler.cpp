#include "StdAfx.h"

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

	_ITEM_TABLE *pTable = g_pMain->GetItemPtr(m_pUserData->m_sItemArray[RIGHTHAND].nNum);
	if (pTable == NULL && m_pUserData->m_sItemArray[RIGHTHAND].nNum != 0) 
		return;
	
	// If you're holding a weapon, do a client-based (ugh, do not trust!) delay check.
	if (pTable 
		&& (delaytime < pTable->m_sDelay
			|| distance > pTable->m_sRange))
		return;	
	// Empty handed.
	else if (delaytime < 100)
		return;			

	// We're attacking a player...
	if (tid < NPC_BAND)
	{
		pTUser = g_pMain->GetUserPtr(tid);
 
		if (pTUser == NULL || pTUser->isDead() || pTUser->isBlinking()
				|| (pTUser->GetNation() == GetNation() && GetZoneID() != 48 /* TO-DO: implement better checks */)
				|| !isAttackZone()) 
			bResult = 0;
		else 
		{
			damage = GetDamage(tid, 0);
			if (GetZoneID() == ZONE_SNOW_BATTLE && g_pMain->m_byBattleOpen == SNOW_BATTLE)
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
		if (g_pMain->m_bPointCheckFlag == FALSE)	
			return;	

		CNpc *pNpc = g_pMain->m_arNpcArray.GetData(tid);		
		if (pNpc != NULL && pNpc->isAlive() 
			&& (pNpc->GetNation() == 0 || pNpc->GetNation() == GetNation()))
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
			g_pMain->Send_AIServer(&result);	
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
		DEBUG_LOG("*** User Attack Dead, id=%s, result=%d, type=%d, HP=%d", pTUser->m_pUserData->m_id, bResult, pTUser->m_bResHpType, pTUser->m_pUserData->m_sHp);
	}
}

short CUser::GetDamage(short tid, int magicid)
{
	short damage = 0;
	int random = 0;
	short common_damage = 0, temp_hit = 0, temp_ac = 0, temp_hit_B = 0;
	BYTE result;

	_MAGIC_TABLE* pTable = NULL;
	_MAGIC_TYPE1* pType1 = NULL; 
	_MAGIC_TYPE2* pType2 = NULL;

	if( tid < 0 || tid >= MAX_USER) return -1;     // Check if target id is valid.

	CUser* pTUser = g_pMain->GetUserPtr(tid);
	if (!pTUser || pTUser->isDead()) 
		return -1;

	temp_ac = pTUser->m_sTotalAc + pTUser->m_sACAmount;    // g??   
	temp_hit_B = (int)( (m_sTotalHit* m_bAttackAmount * 200 / 100) / (temp_ac + 240) ) ;   // g??

	if (magicid > 0) {    // Skill/Arrow hit.    
		pTable = g_pMain->m_MagictableArray.GetData( magicid );     // Get main magic table.
		if( !pTable ) return -1; 
		
		if (pTable->bType[0] == 1) {	// SKILL HIT!			                                
			pType1 = g_pMain->m_Magictype1Array.GetData( magicid );	    // Get magic skill table type 1.
			if( !pType1 ) return -1;     	                                

			if(pType1->bHitType) {    // Non-relative hit.
				random = myrand(0,100) ;       
				if (pType1->sHitRate <= random)
					result = FAIL;
				else 
					result = SUCCESS;
			}
			else {     // Relative hit.
				result = GetHitRate( (m_sTotalHitrate / pTUser->m_sTotalEvasionrate) * (pType1->sHitRate / 100.0f) );			
			}
			temp_hit = (short)(temp_hit_B * (pType1->sHit / 100.0f));
		}
		else if (pTable->bType[0] == 2) {   // ARROW HIT!
			pType2 = g_pMain->m_Magictype2Array.GetData( magicid );	    // Get magic skill table type 1.
			if( !pType2 ) return -1; 
			
			if(pType2->bHitType == 1 || pType2->bHitType == 2 ) {    // Non-relative/Penetration hit.
				random = myrand(0,100) ; 
				
				if (pType2->sHitRate <= random)
					result = FAIL;
				else 
					result = SUCCESS;
			}
			else     // Relative hit/Arc hit.
				result = GetHitRate( (m_sTotalHitrate / pTUser->m_sTotalEvasionrate) * (pType2->sHitRate / 100.0f) );
			
			if(pType2->bHitType == 1 /* || pType2->bHitType == 2 */)  {
				temp_hit = (short)(m_sTotalHit * m_bAttackAmount * (pType2->sAddDamage / 100.0f) / 100);
			}
			else {
				temp_hit = (short)(temp_hit_B * (pType2->sAddDamage / 100.0f));
			}
		}
	}
	else {    // Normal Hit.     
		temp_hit = m_sTotalHit * m_bAttackAmount / 100 ;	// g??
		result = GetHitRate( m_sTotalHitrate / pTUser->m_sTotalEvasionrate ); 
	}
	
	switch(result) {						// 1. Magical item damage....
		case GREAT_SUCCESS:
		case SUCCESS:
		case NORMAL:
			if( magicid > 0 ) {	 // Skill Hit.
				damage = (short)temp_hit;
				random = myrand(0, damage);
				if (pTable->bType[0] == 1) {
					damage = (short)((temp_hit + 0.3f * random) + 0.99f);
				}
				else {
					damage = (short)(((temp_hit * 0.6f) + 1.0f * random) + 0.99f);
				}
			}
			else {	// Normal Hit.	
				damage = (short)temp_hit_B;
				random = myrand(0, damage);
				damage = (short)((0.85f * temp_hit_B) + 0.3f * random);
			}		
			
			break;
		case FAIL:
			damage = 0;
			break;
	}	

	damage = GetMagicDamage(damage, tid);	// 2. Magical item damage....	
	damage = GetACDamage(damage, tid);		// 3. Additional AC calculation....	
//	damage = damage / 2;	// ?????? ??? ??û!!!!
	damage = damage / 3;	// ?????? ??? ??û!!!!  

	return damage;	  
}

short CUser::GetMagicDamage(int damage, short tid)
{
	short total_r = 0;
	short temp_damage = 0;

	CUser* pTUser = g_pMain->GetUserPtr(tid);
	if (!pTUser || pTUser->isDead())
		return damage;	

	// RIGHT HAND!!! by Yookozuna
	if (m_bMagicTypeRightHand > 4 && m_bMagicTypeRightHand < 8) {
		temp_damage = damage * m_sMagicAmountRightHand / 100;
	}

	switch (m_bMagicTypeRightHand) {	// RIGHT HAND!!!
		case ITEM_TYPE_FIRE :	// Fire Damage
			total_r = pTUser->m_bFireR + pTUser->m_bFireRAmount;
			break;
		case ITEM_TYPE_COLD :	// Ice Damage
			total_r = pTUser->m_bColdR + pTUser->m_bColdRAmount;
			break;
		case ITEM_TYPE_LIGHTNING :	// Lightning Damage
			total_r = pTUser->m_bLightningR + pTUser->m_bLightningRAmount;
			break;
		case ITEM_TYPE_POISON :	// Poison Damage
			total_r = pTUser->m_bPoisonR + pTUser->m_bPoisonRAmount;
			break;
		case ITEM_TYPE_HP_DRAIN :	// HP Drain		
			HpChange(temp_damage);			
			break;
		case ITEM_TYPE_MP_DAMAGE :	// MP Damage		
			pTUser->MSpChange(-temp_damage);
			break;
		case ITEM_TYPE_MP_DRAIN :	// MP Drain		
			MSpChange(temp_damage);
			break;
		case 0:
			break;
	}

	if (m_bMagicTypeRightHand > 0 && m_bMagicTypeRightHand < 5) {
		if (total_r > 200) total_r = 200;
		temp_damage = m_sMagicAmountRightHand - m_sMagicAmountRightHand * total_r / 200;
		damage = damage + temp_damage;
	}

	total_r = 0 ;		// Reset all temporary data.
	temp_damage = 0 ;

	// LEFT HAND!!! by Yookozuna
	if (m_bMagicTypeLeftHand > 4 && m_bMagicTypeLeftHand < 8) {
		temp_damage = damage * m_sMagicAmountLeftHand / 100;
	}

	switch (m_bMagicTypeLeftHand) {	// LEFT HAND!!!
		case ITEM_TYPE_FIRE :	// Fire Damage
			total_r = pTUser->m_bFireR + pTUser->m_bFireRAmount;
			break;
		case ITEM_TYPE_COLD :	// Ice Damage
			total_r = pTUser->m_bColdR + pTUser->m_bColdRAmount;
			break;
		case ITEM_TYPE_LIGHTNING :	// Lightning Damage
			total_r = pTUser->m_bLightningR + pTUser->m_bLightningRAmount;
			break;
		case ITEM_TYPE_POISON :	// Poison Damage
			total_r = pTUser->m_bPoisonR + pTUser->m_bPoisonRAmount;
			break;
		case ITEM_TYPE_HP_DRAIN :	// HP Drain		
			HpChange(temp_damage);			
			break;
		case ITEM_TYPE_MP_DAMAGE :	// MP Damage		
			pTUser->MSpChange(-temp_damage);
			break;
		case ITEM_TYPE_MP_DRAIN :	// MP Drain		
			MSpChange(temp_damage);
			break;
		case 0:
			break;
	}

	if (m_bMagicTypeLeftHand > 0 && m_bMagicTypeLeftHand < 5) {
		if (total_r > 200) total_r = 200;
		temp_damage = m_sMagicAmountLeftHand - m_sMagicAmountLeftHand * total_r / 200;
		damage = damage + temp_damage;
	}

	total_r = 0 ;		// Reset all temporary data.
	temp_damage = 0 ;

	// Mirror Attack Check routine.
	if (pTUser->m_bMagicTypeLeftHand == ITEM_TYPE_MIRROR_DAMAGE) {
		temp_damage = damage * pTUser->m_sMagicAmountLeftHand / 100;
		HpChange(-temp_damage);		// Reflective Hit.
	}

	return damage;
}

short CUser::GetACDamage(int damage, short tid)
{
	_ITEM_TABLE* pLeftHand = NULL;
	_ITEM_TABLE* pRightHand = NULL;

	CUser* pTUser = g_pMain->GetUserPtr(tid);
	if (pTUser == NULL || pTUser->isDead())
		return damage;	

	if( m_pUserData->m_sItemArray[RIGHTHAND].nNum != 0 ) {
		pRightHand = g_pMain->GetItemPtr( m_pUserData->m_sItemArray[RIGHTHAND].nNum );
		if( pRightHand ) {
			switch(pRightHand->m_bKind/10) {		// Weapon Type Right Hand....
				case WEAPON_DAGGER:		
					damage = damage - damage * pTUser->m_sDaggerR / 200 ;
					break;
				case WEAPON_SWORD:
					damage = damage - damage * pTUser->m_sSwordR / 200 ;				
					break;
				case WEAPON_AXE:
					damage = damage - damage * pTUser->m_sAxeR / 200 ;				
					break;
				case WEAPON_MACE:
					damage = damage - damage * pTUser->m_sMaceR / 200 ;				
					break;
				case WEAPON_SPEAR:
					damage = damage - damage * pTUser->m_sSpearR / 200 ;			
					break;
				case WEAPON_BOW:
					damage = damage - damage * pTUser->m_sBowR / 200 ;			
					break;
			}
		}
	}

	if( m_pUserData->m_sItemArray[LEFTHAND].nNum != 0) {
		pLeftHand = g_pMain->GetItemPtr( m_pUserData->m_sItemArray[LEFTHAND].nNum );	
		if( pLeftHand ) {
			switch(pLeftHand->m_bKind/10) {			// Weapon Type Right Hand....
				case WEAPON_DAGGER:		
					damage = damage - damage * pTUser->m_sDaggerR / 200 ;
					break;
				case WEAPON_SWORD:
					damage = damage - damage * pTUser->m_sSwordR / 200 ;				
					break;
				case WEAPON_AXE:
					damage = damage - damage * pTUser->m_sAxeR / 200 ;				
					break;
				case WEAPON_MACE:
					damage = damage - damage * pTUser->m_sMaceR / 200 ;				
					break;
				case WEAPON_SPEAR:
					damage = damage - damage * pTUser->m_sSpearR / 200 ;			
					break;
				case WEAPON_BOW:
					damage = damage - damage * pTUser->m_sBowR / 200 ;			
					break;
			}
		}
	}

	return damage;
}

BYTE CUser::GetHitRate(float rate)
{
	BYTE result;
	int random = 0;
	random = myrand(1, 10000);

	if( rate >= 5.0 )
	{
		if( random >= 1 && random <= 3500)
			result = GREAT_SUCCESS;
		else if( random >= 3501 && random <= 7500)
			result = SUCCESS;
		else if( random >= 7501 && random <= 9800)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 5.0 && rate >= 3.0)
	{
		if( random >= 1 && random <= 2500)
			result = GREAT_SUCCESS;
		else if( random >= 2501 && random <= 6000)
			result = SUCCESS;
		else if( random >= 6001 && random <= 9600)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 3.0 && rate >= 2.0)
	{
		if( random >= 1 && random <= 2000)
			result = GREAT_SUCCESS;
		else if( random >= 2001 && random <= 5000)
			result = SUCCESS;
		else if( random >= 5001 && random <= 9400)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 2.0 && rate >= 1.25)
	{
		if( random >= 1 && random <= 1500)
			result = GREAT_SUCCESS;
		else if( random >= 1501 && random <= 4000)
			result = SUCCESS;
		else if( random >= 4001 && random <= 9200)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 1.25 && rate >= 0.8)
	{
		if( random >= 1 && random <= 1000)
			result = GREAT_SUCCESS;
		else if( random >= 1001 && random <= 3000)
			result = SUCCESS;
		else if( random >= 3001 && random <= 9000)
			result = NORMAL;
		else
			result = FAIL;
	}	
	else if ( rate < 0.8 && rate >= 0.5)
	{
		if( random >= 1 && random <= 800)
			result = GREAT_SUCCESS;
		else if( random >= 801 && random <= 2500)
			result = SUCCESS;
		else if( random >= 2501 && random <= 8000)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 0.5 && rate >= 0.33)
	{
		if( random >= 1 && random <= 600)
			result = GREAT_SUCCESS;
		else if( random >= 601 && random <= 2000)
			result = SUCCESS;
		else if( random >= 2001 && random <= 7000)
			result = NORMAL;
		else
			result = FAIL;
	}
	else if ( rate < 0.33 && rate >= 0.2)
	{
		if( random >= 1 && random <= 400)
			result = GREAT_SUCCESS;
		else if( random >= 401 && random <= 1500)
			result = SUCCESS;
		else if( random >= 1501 && random <= 6000)
			result = NORMAL;
		else
			result = FAIL;
	}
	else
	{
		if( random >= 1 && random <= 200)
			result = GREAT_SUCCESS;
		else if( random >= 201 && random <= 1000)
			result = SUCCESS;
		else if( random >= 1001 && random <= 5000)
			result = NORMAL;
		else
			result = FAIL;
	}
	
	return result;
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

	pHomeInfo = g_pMain->m_HomeArray.GetData(m_pUserData->m_bNation);
	if (!pHomeInfo) return;

	UserInOut(INOUT_OUT);

	float x = 0.0f, z = 0.0f;
	x = (float)(myrand( 0, 400 )/100.0f);	z = (float)(myrand( 0, 400 )/100.0f);
	if( x < 2.5f )	x = 1.5f + x;
	if( z < 2.5f )	z = 1.5f + z;

	pEvent = GetMap()->GetObjectEvent(m_pUserData->m_sBind);	

	// TO-DO: Clean this entire thing up. Wow.
	if (magicid == 0) {
		if( pEvent && pEvent->byLife == 1 ) {		// Bind Point
			m_pUserData->m_curx = pEvent->fPosX + x;
			m_pUserData->m_curz = pEvent->fPosZ + z;
			m_pUserData->m_cury = 0;
		}
		else if( m_pUserData->m_bNation != m_pUserData->m_bZone) {	// Free Zone or Opposite Zone
			if(m_pUserData->m_bZone > 200) {		// Frontier Zone...
				x = (float)(pHomeInfo->FreeZoneX + myrand(0, pHomeInfo->FreeZoneLX));
				z = (float)(pHomeInfo->FreeZoneZ + myrand(0, pHomeInfo->FreeZoneLZ));
			}
//
			else if(m_pUserData->m_bZone > 100 && m_pUserData->m_bZone < 200) {		// Battle Zone...
/*
				m_bResHpType = USER_STANDING;
				HpChange( m_iMaxHp );
				KickOutZoneUser();	// Go back to your own zone!
				return;
*/
				x = (float)(pHomeInfo->BattleZoneX + myrand(0, pHomeInfo->BattleZoneLX));
				z = (float)(pHomeInfo->BattleZoneZ + myrand(0, pHomeInfo->BattleZoneLZ));
				if (m_pUserData->m_bZone == ZONE_SNOW_BATTLE) {
					x = (float)(pHomeInfo->FreeZoneX + myrand(0, pHomeInfo->FreeZoneLX));
					z = (float)(pHomeInfo->FreeZoneZ + myrand(0, pHomeInfo->FreeZoneLZ));					
				}
			}
			else if (m_pUserData->m_bZone > 10 && m_pUserData->m_bZone < 20) {
				x = (float)(527 + myrand(0, 10));
				z = (float)(543 + myrand(0, 10));
			}
			else if (m_pUserData->m_bZone < 3) {	// Specific Lands...
				if (m_pUserData->m_bNation == KARUS) {
					x = (float)(pHomeInfo->ElmoZoneX + myrand(0, pHomeInfo->ElmoZoneLX));
					z = (float)(pHomeInfo->ElmoZoneZ + myrand(0, pHomeInfo->ElmoZoneLZ));			
				}
				else if (m_pUserData->m_bNation == ELMORAD) {
					x = (float)(pHomeInfo->KarusZoneX + myrand(0, pHomeInfo->KarusZoneLX));
					z = (float)(pHomeInfo->KarusZoneZ + myrand(0, pHomeInfo->KarusZoneLZ));	
				}		
				else return;
			}

			//m_pUserData->m_curx = x;
			//m_pUserData->m_curz = z;
		}
		else {	
			if (m_pUserData->m_bNation == KARUS) {
				x = (float)(pHomeInfo->KarusZoneX + myrand(0, pHomeInfo->KarusZoneLX));
				z = (float)(pHomeInfo->KarusZoneZ + myrand(0, pHomeInfo->KarusZoneLZ));			
			}
			else if (m_pUserData->m_bNation == ELMORAD) {			
				x = (float)(pHomeInfo->ElmoZoneX + myrand(0, pHomeInfo->ElmoZoneLX));
				z = (float)(pHomeInfo->ElmoZoneZ + myrand(0, pHomeInfo->ElmoZoneLZ));
			}		
			else return;		

			m_pUserData->m_curx = x;
			m_pUserData->m_curz = z;
		}
	}

	Packet result(WIZ_REGENE);
	result << GetSPosX() << GetSPosZ() << GetSPosY();
	Send(&result);
	
	if (magicid > 0) {	// Clerical Resurrection.
		pType = g_pMain->m_Magictype5Array.GetData(magicid);     
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

	m_fLastRegeneTime = TimeGet();
	m_sWhoKilledMe = -1;
	m_iLostExp = 0;

	if (!isBlinking())
	{
		result.Initialize(AG_USER_REGENE);
		result << GetSocketID() << m_pUserData->m_sHp;
		g_pMain->Send_AIServer(&result);
	}

	SetRegion(GetNewRegionX(), GetNewRegionZ());

	UserInOut(INOUT_RESPAWN);		

	g_pMain->RegionUserInOutForMe(this);
	g_pMain->RegionNpcInfoForMe(this);

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