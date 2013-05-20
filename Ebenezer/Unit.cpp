#include "stdafx.h"
#include "Map.h"
#include "EbenezerDlg.h"
#include "User.h"

Unit::Unit(bool bPlayer /*= false*/) 
	: m_pMap(NULL), m_pRegion(NULL), m_sRegionX(0), m_sRegionZ(0), m_bPlayer(bPlayer)
{
	Initialize();
}

void Unit::Initialize()
{
	m_pMap = NULL;
	m_pRegion = NULL;

	SetPosition(0.0f, 0.0f, 0.0f);
	m_bLevel = 0;
	m_bNation = 0;

	m_sTotalHit = 0;
	m_sTotalAc = 0;
	m_sTotalHitrate = 0.0f;
	m_sTotalEvasionrate = 0.0f;

	m_bResistanceBonus = 0;
	m_bFireR = 0;
	m_bColdR = 0;
	m_bLightningR = 0;
	m_bMagicR = 0;
	m_bDiseaseR = 0;
	m_bPoisonR = 0;

	m_sDaggerR = 0;			
	m_sSwordR = 0;			
	m_sAxeR = 0;						
	m_sMaceR = 0;						
	m_sSpearR = 0;					
	m_sBowR = 0;		
	
	m_bMagicTypeLeftHand = 0;		// For weapons and shields with special power.		
	m_bMagicTypeRightHand = 0;		
	m_sMagicAmountLeftHand = 0;        
	m_sMagicAmountRightHand = 0;

	m_bCanStealth = true;
	m_bReflectArmorType = 0;
	m_bIsTransformed = false;
	m_bIsBlinded = false;
	m_bInstantCast = false;

	InitType3();	 // Initialize durational type 3 stuff :)
	InitType4();	 // Initialize durational type 4 stuff :)
}

/* 
	NOTE: Due to KO messiness, we can really only calculate a 2D distance/
	There are a lot of instances where the y (height level, in this case) coord isn't set,
	which understandably screws things up a lot.
*/
// Calculate the distance between 2 2D points.
float Unit::GetDistance(float fx, float fz)
{
	return pow(GetX() - fx, 2.0f) + pow(GetZ() - fz, 2.0f);
}

// Calculate the 2D distance between Units.
float Unit::GetDistance(Unit * pTarget)
{
	ASSERT(pTarget != NULL);
	if (GetZoneID() != pTarget->GetZoneID())
		return -FLT_MAX;

	return GetDistance(pTarget->GetX(), pTarget->GetZ());
}

// Check to see if the Unit is in 2D range of another Unit.
// Range MUST be squared already.
bool Unit::isInRange(Unit * pTarget, float fSquaredRange)
{
	return (GetDistance(pTarget) <= fSquaredRange);
}

// Check to see if we're in the 2D range of the specified coordinates.
// Range MUST be squared already.
bool Unit::isInRange(float fx, float fz, float fSquaredRange)
{
	return (GetDistance(fx, fz) <= fSquaredRange);
}

// Check to see if the Unit is in 2D range of another Unit.
// Range must NOT be squared already.
// This is less preferable to the more common precalculated range.
bool Unit::isInRangeSlow(Unit * pTarget, float fNonSquaredRange)
{
	return isInRange(pTarget, pow(fNonSquaredRange, 2.0f));
}

// Check to see if the Unit is in 2D range of the specified coordinates.
// Range must NOT be squared already.
// This is less preferable to the more common precalculated range.
bool Unit::isInRangeSlow(float fx, float fz, float fNonSquaredRange)
{
	return isInRange(fx, fz, pow(fNonSquaredRange, 2.0f));
}

void Unit::SetRegion(uint16 x /*= -1*/, uint16 z /*= -1*/) 
{
	m_sRegionX = x; m_sRegionZ = z; 
	m_pRegion = m_pMap->GetRegion(x, z); // TO-DO: Clean this up
}

bool Unit::RegisterRegion()
{
	uint16 
		new_region_x = GetNewRegionX(), new_region_z = GetNewRegionZ(), 
		old_region_x = GetRegionX(),	old_region_z = GetRegionZ();

	if (GetRegion() == NULL
		|| (old_region_x == new_region_x && old_region_z == new_region_z))
		return false;

	AddToRegion(new_region_x, new_region_z);

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

	temp_ac = pTarget->m_sTotalAc + pTarget->m_sACAmount;
	temp_hit_B = (int)((m_sTotalHit * m_bAttackAmount * 200 / 100) / (temp_ac + 240));

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
				result = GetHitRate((m_sTotalHitrate / pTarget->m_sTotalEvasionrate) * (pType1->sHitRate / 100.0f));			
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
				result = GetHitRate((m_sTotalHitrate / pTarget->m_sTotalEvasionrate) * (pType2->sHitRate / 100.0f));
			}

			if (pType2->bHitType == 1 /* || pType2->bHitType == 2 */) 
				temp_hit = (short)(m_sTotalHit * m_bAttackAmount * (pType2->sAddDamage / 100.0f) / 100);
			else
				temp_hit = (short)(temp_hit_B * (pType2->sAddDamage / 100.0f));
		}
	}
	// Normal hit (R attack)     
	else 
	{
		temp_hit = m_sTotalHit * m_bAttackAmount / 100;
		result = GetHitRate(m_sTotalHitrate / pTarget->m_sTotalEvasionrate);
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
	if (pTarget->isDead())
		return 0;

	if (m_bMagicTypeRightHand > 4 && m_bMagicTypeRightHand < 8)
		temp_damage = damage * m_sMagicAmountRightHand / 100;

	switch (m_bMagicTypeRightHand)
	{	// RIGHT HAND!!!
		case ITEM_TYPE_FIRE :	// Fire Damage
			total_r = pTarget->m_bFireR + pTarget->m_bFireRAmount;
			break;
		case ITEM_TYPE_COLD :	// Ice Damage
			total_r = pTarget->m_bColdR + pTarget->m_bColdRAmount;
			break;
		case ITEM_TYPE_LIGHTNING :	// Lightning Damage
			total_r = pTarget->m_bLightningR + pTarget->m_bLightningRAmount;
			break;
		case ITEM_TYPE_POISON :	// Poison Damage
			total_r = pTarget->m_bPoisonR + pTarget->m_bPoisonRAmount;
			break;
		case ITEM_TYPE_HP_DRAIN :	// HP Drain		
			HpChange(temp_damage);			
			break;
		case ITEM_TYPE_MP_DAMAGE :	// MP Damage		
			pTarget->MSpChange(-temp_damage);
			break;
		case ITEM_TYPE_MP_DRAIN :	// MP Drain		
			MSpChange(temp_damage);
			break;
	}

	total_r += pTarget->m_bResistanceBonus;

	if (m_bMagicTypeRightHand > 0 && m_bMagicTypeRightHand < 5)
	{
		if (total_r > 200) total_r = 200;
		temp_damage = m_sMagicAmountRightHand - m_sMagicAmountRightHand * total_r / 200;
		damage += temp_damage;
	}

	total_r = 0;		// Reset all temporary data.
	temp_damage = 0;

	if (m_bMagicTypeLeftHand > 4 && m_bMagicTypeLeftHand < 8)
		temp_damage = damage * m_sMagicAmountLeftHand / 100;

	switch (m_bMagicTypeLeftHand)
	{	// LEFT HAND!!!
		case ITEM_TYPE_FIRE :	// Fire Damage
			total_r = pTarget->m_bFireR + pTarget->m_bFireRAmount;
			break;
		case ITEM_TYPE_COLD :	// Ice Damage
			total_r = pTarget->m_bColdR + pTarget->m_bColdRAmount;
			break;
		case ITEM_TYPE_LIGHTNING :	// Lightning Damage
			total_r = pTarget->m_bLightningR + pTarget->m_bLightningRAmount;
			break;
		case ITEM_TYPE_POISON :	// Poison Damage
			total_r = pTarget->m_bPoisonR + pTarget->m_bPoisonRAmount;
			break;
		case ITEM_TYPE_HP_DRAIN :	// HP Drain		
			HpChange(temp_damage);			
			break;
		case ITEM_TYPE_MP_DAMAGE :	// MP Damage		
			pTarget->MSpChange(-temp_damage);
			break;
		case ITEM_TYPE_MP_DRAIN :	// MP Drain		
			MSpChange(temp_damage);
			break;
	}

	total_r += pTarget->m_bResistanceBonus;

	if (m_bMagicTypeLeftHand > 0 && m_bMagicTypeLeftHand < 5)
	{
		if (total_r > 200) total_r = 200;
		temp_damage = m_sMagicAmountLeftHand - m_sMagicAmountLeftHand * total_r / 200;
		damage += temp_damage;
	}

	total_r = 0;		// Reset all temporary data.
	temp_damage = 0;

	// Mirror Attack Check routine.
	if (pTarget->m_bMagicTypeLeftHand == ITEM_TYPE_MIRROR_DAMAGE)
	{
		temp_damage = damage * pTarget->m_sMagicAmountLeftHand / 100;
		HpChange(-temp_damage);		// Reflective Hit.
	}

	return damage;
}

short Unit::GetACDamage(int damage, Unit *pTarget)
{
	// This isn't applicable to NPCs.
	if (isNPC() || pTarget->isNPC())
		return damage;

	if (pTarget->isDead())
		return 0;

	CUser * pUser  = TO_USER(this);
	uint8 weaponSlots[] = { RIGHTHAND, LEFTHAND };

	foreach_array (slot, weaponSlots)
	{
		_ITEM_TABLE * pWeapon = pUser->GetItemPrototype(slot);
		if (pWeapon == NULL)
			continue;

		if (pWeapon->isDagger())
			damage -= damage * pTarget->m_sDaggerR / 200;
		else if (pWeapon->isSword())
			damage -= damage * pTarget->m_sSwordR / 200;
		else if (pWeapon->isAxe())
			damage -= damage * pTarget->m_sAxeR / 200;
		else if (pWeapon->isMace())
			damage -= damage * pTarget->m_sMaceR / 200;
		else if (pWeapon->isSpear())
			damage -= damage * pTarget->m_sSpearR / 200;
		else if (pWeapon->isBow())
			damage -= damage * pTarget->m_sBowR / 200;
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

// Handle it here so that we don't need to ref the class everywhere
void Unit::Send_AIServer(Packet *result)
{
	g_pMain->Send_AIServer(result);
}

void Unit::InitType3()
{
	for (int i = 0 ; i < MAX_TYPE3_REPEAT; i++)
	{
		m_tHPStartTime[i] = 0;		
		m_tHPLastTime[i] = 0;
		m_bHPAmount[i] = 0;
		m_bHPDuration[i] = 0;
		m_bHPInterval[i] = 5;
		m_sSourceID[i] = -1;
	}

	m_bType3Flag = false;
}

void Unit::InitType4()
{
	m_bAttackSpeedAmount = 100;		// this is for the duration spells Type 4
    m_bSpeedAmount = 100;
    m_sACAmount = 0;
    m_bAttackAmount = 100;
	m_sMagicAttackAmount = 0;
	m_sMaxHPAmount = 0;
	m_sMaxMPAmount = 0;
	m_bHitRateAmount = 100;
	m_sAvoidRateAmount = 100;
	m_bFireRAmount = 0;
	m_bColdRAmount = 0;
	m_bLightningRAmount = 0;
	m_bMagicRAmount = 0;
	m_bDiseaseRAmount = 0;
	m_bPoisonRAmount = 0;		

	StateChangeServerDirect(3, ABNORMAL_NORMAL);
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

void Unit::AddType4Buff(uint8 bBuffType, _BUFF_TYPE4_INFO & pBuffInfo)
{
	FastGuard lock(m_buffLock);
	m_buffMap.insert(std::make_pair(bBuffType, pBuffInfo));
}