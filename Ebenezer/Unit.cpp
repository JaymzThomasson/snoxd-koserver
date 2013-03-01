#include "stdafx.h"

Unit::Unit(bool bPlayer /*= false*/) 
	: m_pMap(NULL), m_pRegion(NULL), m_sRegionX(0), m_sRegionZ(0), m_bPlayer(bPlayer)
{
	InitType3();
	InitType4();
}

bool Unit::RegisterRegion()
{
	uint16 
		new_region_x = GetNewRegionX(), new_region_z = GetNewRegionZ(), 
		old_region_x = GetRegionX(),	old_region_z = GetRegionZ();

	if (GetRegion() == NULL
		|| (old_region_x == new_region_x && old_region_z == new_region_z))
		return false;

	// TO-DO: Fix this up
	if (isPlayer())
	{
		GetRegion()->Remove(static_cast<CUser *>(this));
		SetRegion(new_region_x, new_region_z);
		GetRegion()->Add(static_cast<CUser *>(this));
	}
	else
	{
		GetRegion()->Remove(static_cast<CNpc *>(this));
		SetRegion(new_region_x, new_region_z);
		GetRegion()->Add(static_cast<CNpc *>(this));
	}

	RemoveRegion(old_region_x - new_region_x, old_region_z - new_region_z);
	InsertRegion(new_region_x - old_region_x, new_region_z - old_region_z);	

	return true;
}

void Unit::RemoveRegion(int16 del_x, int16 del_z)
{
	ASSERT(GetMap() != NULL);

	Packet result;
	GetInOut(result, INOUT_OUT);
	g_pMain->Send_OldRegions(&result, del_x, del_z, GetMap(), GetRegionX(), GetRegionZ());
}

void Unit::InsertRegion(int16 insert_x, int16 insert_z)
{
	ASSERT(GetMap() != NULL);

	Packet result;
	GetInOut(result, INOUT_IN);
	g_pMain->Send_NewRegions(&result, insert_x, insert_z, GetMap(), GetRegionX(), GetRegionZ());
}

short Unit::GetDamage(Unit *pTarget, _MAGIC_TABLE *pSkill)
{
	short damage = 0;
	int random = 0;
	short common_damage = 0, temp_hit = 0, temp_ac = 0, temp_hit_B = 0;
	uint8 result;

	if (pTarget == NULL || pTarget->isDead())
		return -1;

	// For now, we assume it's a user.
	// This will slowly be merged to handle both users and NPCs.
	CUser *pTUser = static_cast<CUser *>(pTarget);
	CUser *pUser = static_cast<CUser *>(this);

	temp_ac = pTUser->m_sTotalAc + pTUser->m_sACAmount; // CNpc::m_sDefense
	temp_hit_B = (int)((pUser->m_sTotalHit * pUser->m_bAttackAmount * 200 / 100) / (temp_ac + 240) ) ;

    // Skill/arrow hit.    
	if (pSkill != NULL)
	{
		// SKILL HIT! YEAH!	                                
		if (pSkill->bType[0] == 1)
		{
			_MAGIC_TYPE1 *pType1 = g_pMain->m_Magictype1Array.GetData(pSkill->iNum);
			if (pType1 == NULL)
				return -1;     	                                

			// Non-relative hit.
			if (pType1->bHitType)
			{
				result = (pType1->sHitRate <= myrand(0, 100) ? FAIL : SUCCESS);
			}
			// Relative hit.
			else 
			{
				result = GetHitRate((pUser->m_sTotalHitrate / pTUser->m_sTotalEvasionrate) * (pType1->sHitRate / 100.0f));			
			}

			temp_hit = (short)(temp_hit_B * (pType1->sHit / 100.0f));
		}
		// ARROW HIT! YEAH!
		else if (pSkill->bType[0] == 2)
		{
			_MAGIC_TYPE2 *pType2 = g_pMain->m_Magictype2Array.GetData(pSkill->iNum);
			if (pType2 == NULL)
				return -1; 
			
			// Non-relative/Penetration hit.
			if (pType2->bHitType == 1 || pType2->bHitType == 2)
			{
				result = (pType2->sHitRate <= myrand(0, 100) ? FAIL : SUCCESS);
			}
			// Relative hit/Arc hit.
			else   
			{
				result = GetHitRate((pUser->m_sTotalHitrate / pTUser->m_sTotalEvasionrate) * (pType2->sHitRate / 100.0f));
			}

			if (pType2->bHitType == 1 /* || pType2->bHitType == 2 */) 
				temp_hit = (short)(pUser->m_sTotalHit * pUser->m_bAttackAmount * (pType2->sAddDamage / 100.0f) / 100);
			else
				temp_hit = (short)(temp_hit_B * (pType2->sAddDamage / 100.0f));
		}
	}
	// Normal hit (R attack)     
	else 
	{
		temp_hit = pUser->m_sTotalHit * pUser->m_bAttackAmount / 100;
		result = GetHitRate(pUser->m_sTotalHitrate / pTUser->m_sTotalEvasionrate);
	}
	
	switch (result)
	{						// 1. Magical item damage....
		case GREAT_SUCCESS:
		case SUCCESS:
		case NORMAL:
			if (pSkill != NULL)
			{	 // Skill Hit.
				damage = (short)temp_hit;
				random = myrand(0, damage);
				if (pSkill->bType[0] == 1)
					damage = (short)((temp_hit + 0.3f * random) + 0.99f);
				else
					damage = (short)(((temp_hit * 0.6f) + 1.0f * random) + 0.99f);
			}
			else
			{	// Normal Hit.	
				damage = (short)temp_hit_B;
				random = myrand(0, damage);
				damage = (short)((0.85f * temp_hit_B) + 0.3f * random);
			}		
			
			break;
		case FAIL:
			damage = 0;
			break;
	}	

	damage = GetMagicDamage(damage, pTarget);	// 2. Magical item damage....	

	// These two only apply to players
	if (isPlayer() && pTarget->isPlayer())
	{
		damage = GetACDamage(damage, pTarget);		// 3. Additional AC calculation....	
		damage = damage / 3;
	}

	return damage;	  
}

short Unit::GetMagicDamage(int damage, Unit *pTarget)
{
	short total_r, temp_damage = 0;

	// temporarily assume it's player->player
	ASSERT(pTarget->isPlayer());
	if (pTarget->isNPC() || pTarget->isDead())
		return 0;

	CUser	* pTUser = static_cast<CUser *>(pTarget),
			* pUser  = static_cast<CUser *>(this);

	// RIGHT HAND!!! by Yookozuna
	if (pUser->m_bMagicTypeRightHand > 4 && pUser->m_bMagicTypeRightHand < 8)
		temp_damage = damage * pUser->m_sMagicAmountRightHand / 100;

	switch (pUser->m_bMagicTypeRightHand)
	{	// RIGHT HAND!!!
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
			pUser->HpChange(temp_damage);			
			break;
		case ITEM_TYPE_MP_DAMAGE :	// MP Damage		
			pTUser->MSpChange(-temp_damage);
			break;
		case ITEM_TYPE_MP_DRAIN :	// MP Drain		
			pUser->MSpChange(temp_damage);
			break;
	}

	if (pUser->m_bMagicTypeRightHand > 0 && pUser->m_bMagicTypeRightHand < 5)
	{
		if (total_r > 200) total_r = 200;
		temp_damage = pUser->m_sMagicAmountRightHand - pUser->m_sMagicAmountRightHand * total_r / 200;
		damage = damage + temp_damage;
	}

	total_r = 0 ;		// Reset all temporary data.
	temp_damage = 0 ;

	// LEFT HAND!!! by Yookozuna
	if (pUser->m_bMagicTypeLeftHand > 4 && pUser->m_bMagicTypeLeftHand < 8)
		temp_damage = damage * pUser->m_sMagicAmountLeftHand / 100;

	switch (pUser->m_bMagicTypeLeftHand)
	{	// LEFT HAND!!!
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
			pUser->HpChange(temp_damage);			
			break;
		case ITEM_TYPE_MP_DAMAGE :	// MP Damage		
			pTUser->MSpChange(-temp_damage);
			break;
		case ITEM_TYPE_MP_DRAIN :	// MP Drain		
			pUser->MSpChange(temp_damage);
			break;
	}

	if (pUser->m_bMagicTypeLeftHand > 0 && pUser->m_bMagicTypeLeftHand < 5)
	{
		if (total_r > 200) total_r = 200;
		temp_damage = pUser->m_sMagicAmountLeftHand - pUser->m_sMagicAmountLeftHand * total_r / 200;
		damage = damage + temp_damage;
	}

	total_r = 0;		// Reset all temporary data.
	temp_damage = 0;

	// Mirror Attack Check routine.
	if (pTUser->m_bMagicTypeLeftHand == ITEM_TYPE_MIRROR_DAMAGE) {
		temp_damage = damage * pTUser->m_sMagicAmountLeftHand / 100;
		pUser->HpChange(-temp_damage);		// Reflective Hit.
	}

	return damage;
}


short Unit::GetACDamage(int damage, Unit *pTarget)
{
	if (isNPC() || pTarget->isNPC()
		|| pTarget->isDead())
		return 0;

	CUser	* pUser  = static_cast<CUser *>(this),
			* pTUser = static_cast<CUser *>(pTarget);

	_ITEM_TABLE * pRightHand = pUser->GetItemPrototype(RIGHTHAND);
	if (pRightHand != NULL)
	{
		if (pRightHand->isDagger())
			damage -= damage * pTUser->m_sDaggerR / 200;
		else if (pRightHand->isSword())
			damage -= damage * pTUser->m_sSwordR / 200;
		else if (pRightHand->isAxe())
			damage -= damage * pTUser->m_sAxeR / 200;
		else if (pRightHand->isMace())
			damage -= damage * pTUser->m_sMaceR / 200;
		else if (pRightHand->isSpear())
			damage -= damage * pTUser->m_sSpearR / 200;
		else if (pRightHand->isBow())
			damage -= damage * pTUser->m_sBowR / 200;
	}

	_ITEM_TABLE * pLeftHand = pUser->GetItemPrototype(RIGHTHAND);
	if (pLeftHand != NULL)
	{
		if (pLeftHand->isDagger())
			damage -= damage * pTUser->m_sDaggerR / 200;
		else if (pLeftHand->isSword())
			damage -= damage * pTUser->m_sSwordR / 200;
		else if (pLeftHand->isAxe())
			damage -= damage * pTUser->m_sAxeR / 200;
		else if (pLeftHand->isMace())
			damage -= damage * pTUser->m_sMaceR / 200;
		else if (pLeftHand->isSpear())
			damage -= damage * pTUser->m_sSpearR / 200;
		else if (pLeftHand->isBow())
			damage -= damage * pTUser->m_sBowR / 200;
	}

	return damage;
}

uint8 Unit::GetHitRate(float rate)
{
	int random = myrand(1, 10000);
	if (rate >= 5.0f)
	{
		if (random >= 1 && random <= 3500)
			return GREAT_SUCCESS;
		else if (random >= 3501 && random <= 7500)
			return SUCCESS;
		else if (random >= 7501 && random <= 9800)
			return NORMAL;
	}
	else if (rate < 5.0f && rate >= 3.0f)
	{
		if (random >= 1 && random <= 2500)
			return GREAT_SUCCESS;
		else if (random >= 2501 && random <= 6000)
			return SUCCESS;
		else if (random >= 6001 && random <= 9600)
			return NORMAL;
	}
	else if (rate < 3.0f && rate >= 2.0f)
	{
		if (random >= 1 && random <= 2000)
			return GREAT_SUCCESS;
		else if (random >= 2001 && random <= 5000)
			return SUCCESS;
		else if (random >= 5001 && random <= 9400)
			return NORMAL;
	}
	else if (rate < 2.0f && rate >= 1.25f)
	{
		if (random >= 1 && random <= 1500)
			return GREAT_SUCCESS;
		else if (random >= 1501 && random <= 4000)
			return SUCCESS;
		else if (random >= 4001 && random <= 9200)
			return NORMAL;
	}
	else if (rate < 1.25f && rate >= 0.8f)
	{
		if (random >= 1 && random <= 1000)
			return GREAT_SUCCESS;
		else if (random >= 1001 && random <= 3000)
			return SUCCESS;
		else if (random >= 3001 && random <= 9000)
			return NORMAL;
	}	
	else if (rate < 0.8f && rate >= 0.5f)
	{
		if (random >= 1 && random <= 800)
			return GREAT_SUCCESS;
		else if (random >= 801 && random <= 2500)
			return SUCCESS;
		else if (random >= 2501 && random <= 8000)
			return NORMAL;
	}
	else if (rate < 0.5f && rate >= 0.33f)
	{
		if (random >= 1 && random <= 600)
			return GREAT_SUCCESS;
		else if (random >= 601 && random <= 2000)
			return SUCCESS;
		else if (random >= 2001 && random <= 7000)
			return NORMAL;
	}
	else if (rate < 0.33f && rate >= 0.2f)
	{
		if (random >= 1 && random <= 400)
			return GREAT_SUCCESS;
		else if (random >= 401 && random <= 1500)
			return SUCCESS;
		else if (random >= 1501 && random <= 6000)
			return NORMAL;
	}
	else
	{
		if (random >= 1 && random <= 200)
			return GREAT_SUCCESS;
		else if (random >= 201 && random <= 1000)
			return SUCCESS;
		else if (random >= 1001 && random <= 5000)
			return NORMAL;
	}
	
	return FAIL;
}

void Unit::SendToRegion(Packet *result)
{
	g_pMain->Send_Region(result, GetMap(), GetRegionX(), GetRegionZ());
}

void Unit::InitType3()
{
	for (int i = 0 ; i < MAX_TYPE3_REPEAT; i++)
	{
		m_fHPStartTime[i] = 0.0f;		
		m_fHPLastTime[i] = 0.0f;
		m_bHPAmount[i] = 0;
		m_bHPDuration[i] = 0;
		m_bHPInterval[i] = 5;
		m_sSourceID[i] = -1;
	}

	m_bType3Flag = FALSE;
}

void Unit::InitType4()
{
	m_bAttackSpeedAmount = 100;		// this is for the duration spells Type 4
    m_bSpeedAmount = 100;
    m_sACAmount = 0;
    m_bAttackAmount = 100;
	m_sMaxHPAmount = 0;
	m_bHitRateAmount = 100;
	m_sAvoidRateAmount = 100;
	m_bFireRAmount = 0;
	m_bColdRAmount = 0;
	m_bLightningRAmount = 0;
	m_bMagicRAmount = 0;
	m_bDiseaseRAmount = 0;
	m_bPoisonRAmount = 0;		
	memset(m_sDuration, 0, sizeof(uint16) * MAX_TYPE4_BUFF);
	memset(m_fStartTime, 0, sizeof(float) * MAX_TYPE4_BUFF);
	memset(m_bType4Buff, 0, sizeof(*m_bType4Buff) * MAX_TYPE4_BUFF);
	m_bType4Flag = FALSE;

	// this is going to need cleaning up
	if (isPlayer())
		static_cast<CUser *>(this)->StateChangeServerDirect(3, ABNORMAL_NORMAL);
}

void Unit::OnDeath(Unit *pKiller)
{
	SendDeathAnimation();
}

void Unit::SendDeathAnimation()
{
	Packet result(WIZ_DEAD);
	result << GetID();
	SendToRegion(&result);
}