#include "stdafx.h"
#include "EbenezerDlg.h"
#include "MagicProcess.h"
#include "MagicInstance.h"
#include "User.h" // need to move UserRegionCheck() to get rid of this

void CMagicProcess::MagicPacket(Packet & pkt, Unit * pCaster /*= nullptr*/, bool isRecastingSavedMagic /*= false*/)
{
	MagicInstance instance;
	pkt >> instance.bOpcode >> instance.nSkillID;

	instance.pSkill = g_pMain->m_MagictableArray.GetData(instance.nSkillID);
	if (instance.pSkill == nullptr)
	{
		TRACE("[%s] Used skill %d but it does not exist.\n", pCaster->GetName().c_str(), instance.nSkillID);
		return;
	}

	pkt >> instance.sCasterID >> instance.sTargetID
		>> instance.sData[0] >> instance.sData[1] >> instance.sData[2] >> instance.sData[3]
		>> instance.sData[4] >> instance.sData[5] >> instance.sData[6] >> instance.sData[7];

	instance.pSkillCaster = g_pMain->GetUnit(instance.sCasterID);
	instance.pSkillTarget = g_pMain->GetUnit(instance.sTargetID);

	// Prevent users from faking other players or NPCs.
	if (pCaster != nullptr // if it's nullptr, it's from AI.
		&& (instance.pSkillCaster == nullptr 
			|| instance.pSkillCaster->isNPC()
			|| instance.pSkillCaster != pCaster))
		return;

	instance.bIsRecastingSavedMagic = isRecastingSavedMagic;
	instance.Run();
}

// TO-DO: Clean this up (even using unit code...)
bool CMagicProcess::UserRegionCheck(Unit * pSkillCaster, Unit * pSkillTarget, _MAGIC_TABLE * pSkill, int radius, short mousex /*= 0*/, short mousez /*= 0*/)
{
	if (pSkillCaster->isDead()
		|| pSkillTarget == nullptr)
		return false;

	switch (pSkill->bMoral)
	{
		case MORAL_PARTY_ALL:		// Check that it's your party.
			// NPCs cannot be in parties.
			if (pSkillCaster->isNPC()
				|| pSkillTarget->isNPC())
				return false;

			if (!TO_USER(pSkillTarget)->isInParty())
				return (pSkillTarget == pSkillCaster);

			if (TO_USER(pSkillTarget)->GetPartyID() == TO_USER(pSkillCaster)->GetPartyID()
				&& pSkill->bType[0] != 8)
				goto final_test;
			else if (TO_USER(pSkillTarget)->GetPartyID() == TO_USER(pSkillCaster)->GetPartyID() 
				&& pSkill->bType[0] == 8)
			{
				if (pSkillTarget->GetZoneID() == ZONE_BATTLE && (UNIXTIME - TO_USER(pSkillTarget)->m_tLastRegeneTime < CLAN_SUMMON_TIME))
					return false;

				goto final_test;	
			}

			break;

		case MORAL_SELF_AREA:
		case MORAL_AREA_ENEMY:
			if (pSkillTarget->GetNation() != pSkillCaster->GetNation())
				goto final_test;
			break;

		case MORAL_AREA_FRIEND:
			if (pSkillTarget->GetNation() == pSkillCaster->GetNation())
				goto final_test;
			break;

		case MORAL_CLAN_ALL:
			// NPCs cannot be in clans.
			if (pSkillCaster->isNPC()
				|| pSkillTarget->isNPC())
				return false;

			if (!TO_USER(pSkillTarget)->isInClan())
				return (pSkillTarget == pSkillCaster);

			if (TO_USER(pSkillTarget)->GetClanID() == TO_USER(pSkillCaster)->GetClanID() 
				&& pSkill->bType[0] != 8)
				goto final_test;
			else if (TO_USER(pSkillTarget)->GetClanID() == TO_USER(pSkillCaster)->GetClanID() 
				&& pSkill->bType[0] == 8)
			{
				if (pSkillTarget->GetZoneID() == ZONE_BATTLE && (UNIXTIME - TO_USER(pSkillTarget)->m_tLastRegeneTime < CLAN_SUMMON_TIME))
					return false;
				goto final_test;	
			}
			break;
	}
	return false;	

final_test:
	if (pSkillTarget->GetRegion() != pSkillCaster->GetRegion())
		return false;

	if (radius != 0)
	{
		float temp_x = pSkillTarget->GetX() - mousex;
		float temp_z = pSkillTarget->GetZ() - mousez;
		float distance = pow(temp_x, 2.0f) + pow(temp_z, 2.0f);
		if (distance > pow((float)radius, 2.0f)) 
			return false;
	}
	return true;	// Target is in the area.
}

void CMagicProcess::CheckExpiredType6Skills(Unit * pTarget)
{
	if (!pTarget->isPlayer()
		|| !pTarget->isTransformed()
		|| (UNIXTIME - pTarget->m_tTransformationStartTime) < pTarget->m_sTransformationDuration)
		return;

	MagicInstance instance;
	instance.pSkillCaster = pTarget;
	instance.Type6Cancel();
}

void CMagicProcess::CheckExpiredType9Skills(Unit * pTarget, bool bForceExpiration /*= false*/)
{
	if (!pTarget->isPlayer())
		return;

	FastGuard lock(pTarget->m_buffLock);
	Type9BuffMap & buffMap = pTarget->m_type9BuffMap;

	MagicInstance instance;
	instance.pSkillCaster = pTarget;

	for (auto itr = buffMap.begin(); itr != buffMap.end();)
	{
		if (bForceExpiration || UNIXTIME >= itr->second.tEndTime) 
		{
			// Cancel the skill, but don't remove it from the map. We'll do that.
			instance.nSkillID = itr->second.nSkillID;
			instance.Type9Cancel(false); 
			itr = buffMap.erase(itr);
		}
		else 
		{
			++itr;
		}
	}
}

void CMagicProcess::RemoveStealth(Unit * pTarget, InvisibilityType bInvisibilityType)
{
	if (bInvisibilityType != INVIS_DISPEL_ON_MOVE
		&& bInvisibilityType != INVIS_DISPEL_ON_ATTACK)
		return;

	FastGuard lock(pTarget->m_buffLock);
	Type9BuffMap & buffMap = pTarget->m_type9BuffMap;
	MagicInstance instance;

	// Buff type is 1 for 'dispel on move' skills and 2 for 'dispel on attack' skills.
	uint8 bType = (bInvisibilityType == INVIS_DISPEL_ON_MOVE ? 1 : 2);

	// If there's no such skill active on the user, no reason to remove it.
	auto itr = buffMap.find(bType);
	if (itr == buffMap.end())
		return;

	instance.pSkillCaster = pTarget;
	instance.nSkillID = itr->second.nSkillID;
	instance.Type9Cancel();
}

bool CMagicProcess::GrantType4Buff(_MAGIC_TABLE * pSkill, _MAGIC_TYPE4 *pType, Unit * pCaster, Unit *pTarget)
{
	// Buff mustn't already be added at this point.
	FastGuard lock(pTarget->m_buffLock);
	if (pTarget->m_buffMap.find(pType->bBuffType) != pTarget->m_buffMap.end())
		return false;

	switch (pType->bBuffType)
	{
	case BUFF_TYPE_HP_MP:
		if (pType->sMaxHP == 0 && pType->sMaxHPPct > 0)
			pTarget->m_sMaxHPAmount = (pType->sMaxHPPct - 100) * (pTarget->GetMaxHealth() - pTarget->m_sMaxHPAmount) / 100;
		else
			pTarget->m_sMaxHPAmount = pType->sMaxHP;

		if (pType->sMaxMP == 0 && pType->sMaxMPPct > 0)
			pTarget->m_sMaxMPAmount = (pType->sMaxMPPct - 100) * (pTarget->GetMaxMana() - pTarget->m_sMaxMPAmount) / 100;
		else
			pTarget->m_sMaxMPAmount = pType->sMaxMP;
		break;

	case BUFF_TYPE_AC:
		if (pType->sAC == 0 && pType->sACPct > 0)
			pTarget->m_sACAmount = pTarget->m_sTotalAc * (pType->sACPct - 100) / 100;
		else
			pTarget->m_sACAmount = pType->sAC;
		break;

	case BUFF_TYPE_SIZE:
		if (pCaster->isPlayer())
		{
			// Unfortunately there's no way to differentiate which does what.
			// Officially it also resorts to checking the skill ID.
			uint8 bEffect = ABNORMAL_NORMAL;
			switch (pSkill->iNum)
			{
			case 490034: // Bezoar
			case 490401: // Maximize Scroll
				bEffect = ABNORMAL_GIANT;
				break;

			case 490035: // Rice cake
			case 490100: // unknown, possibly intended to be "Minimize Scroll"
				bEffect = ABNORMAL_DWARF;
				break;
			}

			if (bEffect != ABNORMAL_NORMAL)
				TO_USER(pTarget)->StateChangeServerDirect(3, bEffect);
		}
		break;

	case BUFF_TYPE_DAMAGE:
		pTarget->m_bAttackAmount = pType->bAttack;
		break;

	case BUFF_TYPE_ATTACK_SPEED:
		pTarget->m_bAttackSpeedAmount = pType->bAttackSpeed;
		break;

	case BUFF_TYPE_SPEED:
		pTarget->m_bSpeedAmount = pType->bSpeed;
		break;

	case BUFF_TYPE_STATS:
		if (pTarget->isPlayer())
		{
			TO_USER(pTarget)->SetStatBuff(STAT_STR, pType->bStr);
			TO_USER(pTarget)->SetStatBuff(STAT_STA, pType->bSta);
			TO_USER(pTarget)->SetStatBuff(STAT_DEX, pType->bDex);
			TO_USER(pTarget)->SetStatBuff(STAT_INT, pType->bIntel);
			TO_USER(pTarget)->SetStatBuff(STAT_CHA, pType->bCha);	
		}
		break;

	case BUFF_TYPE_RESISTANCES:
		pTarget->m_bFireRAmount = pType->bFireR;
		pTarget->m_bColdRAmount = pType->bColdR;
		pTarget->m_bLightningRAmount = pType->bLightningR;
		pTarget->m_bMagicRAmount = pType->bMagicR;
		pTarget->m_bDiseaseRAmount = pType->bDiseaseR;
		pTarget->m_bPoisonRAmount = pType->bPoisonR;
		break;

	case BUFF_TYPE_ACCURACY:
		pTarget->m_bHitRateAmount = pType->bHitRate;
		pTarget->m_sAvoidRateAmount = pType->sAvoidRate;
		break;	

	case BUFF_TYPE_MAGIC_POWER:
		if (pTarget->isPlayer())
			pTarget->m_sMagicAttackAmount = (pType->bMagicAttack - 100) * TO_USER(pTarget)->GetStat(STAT_CHA) / 100;
		break;

	case BUFF_TYPE_EXPERIENCE:
		if (pTarget->isPlayer())
			TO_USER(pTarget)->m_bExpGainAmount = pType->bExpPct;
		break;

	case BUFF_TYPE_WEIGHT:
		if (pTarget->isPlayer())
			TO_USER(pTarget)->m_bMaxWeightAmount = pType->bExpPct;
		break;

	case BUFF_TYPE_WEAPON_DAMAGE:
		// uses pType->Attack
		break;

	case BUFF_TYPE_WEAPON_AC:
		if (pType->sAC == 0 && pType->sACPct > 0)
			pTarget->m_sACAmount = pTarget->m_sTotalAc * (pType->sACPct - 100) / 100;
		else
			pTarget->m_sACAmount = pType->sAC;
		break;

	case BUFF_TYPE_LOYALTY:
		// uses pType->ExpPct
		break;

	case BUFF_TYPE_NOAH_BONUS:
		break;

	case BUFF_TYPE_PREMIUM_MERCHANT:
		if (pTarget->isPlayer())
			TO_USER(pTarget)->m_bPremiumMerchant = true;
		break;

	case BUFF_TYPE_ATTACK_SPEED_ARMOR:
		pTarget->m_sACAmount += pType->sAC;

		// Prevent total defense from ever going below 0
		if ((pTarget->m_sACAmount + pTarget->m_sTotalAc) < 0)
			pTarget->m_sACAmount = -(pTarget->m_sTotalAc);

		pTarget->m_bAttackAmount = pType->bAttack;
		break;

	case BUFF_TYPE_DAMAGE_DOUBLE:
		pTarget->m_bAttackAmount = pType->bAttack;
		break;

	case BUFF_TYPE_DISABLE_TARGETING:
		pTarget->m_bIsBlinded = true;
		break;

	case BUFF_TYPE_BLIND:
		pTarget->m_bIsBlinded = true;
		if (pTarget->isPlayer())
			TO_USER(pTarget)->SendUserStatusUpdate(USER_STATUS_BLIND, USER_STATUS_INFLICT);
		break;

	case BUFF_TYPE_FREEZE:
		// Proportional to the target user's current HP.
		pTarget->m_bSpeedAmount = pType->bSpeed;
		break;

	case BUFF_TYPE_INSTANT_MAGIC:
		pTarget->m_bInstantCast = true;
		break;

	case BUFF_TYPE_DECREASE_RESIST:
		pTarget->m_bFireRAmount			= -(pType->bFireR / 100)	*	(pTarget->m_bFireR		- pTarget->m_bFireRAmount);
		pTarget->m_bColdRAmount			= -(pType->bColdR / 100)	*	(pTarget->m_bColdR		- pTarget->m_bColdRAmount);
		pTarget->m_bLightningRAmount	= -(pType->bLightningR / 100) * (pTarget->m_bLightningR - pTarget->m_bLightningRAmount);
		pTarget->m_bMagicRAmount		= -(pType->bMagicR / 100)	*	(pTarget->m_bMagicR		- pTarget->m_bMagicRAmount);
		pTarget->m_bDiseaseRAmount		= -(pType->bDiseaseR / 100) *	(pTarget->m_bDiseaseR	- pTarget->m_bDiseaseRAmount);
		pTarget->m_bPoisonRAmount		= -(pType->bPoisonR / 100)	*	(pTarget->m_bPoisonR	- pTarget->m_bPoisonRAmount);
		break;

	case BUFF_TYPE_MAGE_ARMOR:
		pTarget->m_bReflectArmorType = (pSkill->sSkill % 100);
		break;

	case BUFF_TYPE_PROHIBIT_INVIS:
		pTarget->m_bCanStealth = true;
		break;

	case BUFF_TYPE_RESIS_AND_MAGIC_DMG: // Elysian Web
		// Increases your magic resistance to block an additional 30% magic damage.
		break;

	case BUFF_TYPE_TRIPLEAC_HALFSPEED:	// Wall of Iron
		pTarget->m_sACAmount += pTarget->m_sTotalAc * 2; // TO-DO: Store this value independently for restoration
		pTarget->m_bSpeedAmount = pTarget->m_bSpeedAmount / 2;
		if (pTarget->m_bSpeedAmount = 0)
			pTarget->m_bSpeedAmount = 1;
		break;

	case BUFF_TYPE_BLOCK_CURSE:			// Counter Curse
		// Blocks all curses.
		break;

	case BUFF_TYPE_BLOCK_CURSE_REFLECT:	// Curse Refraction
		// Blocks all curses and has a chance to reflect the curse back upon the caster.
		break;

	case BUFF_TYPE_MANA_ABSORB:		// Outrage / Frenzy / Mana Shield
		// Uses mana to receive damage (for mana shield its multiplied by 4, 100 damage = 400 mana used)
		break;

	case BUFF_TYPE_IGNORE_WEAPON:		// Weapon cancellation
		// Disarms the opponent. (rendering them unable to attack)
		break;

	case BUFF_TYPE_PASSION_OF_SOUL:		// Passion of the Soul
		// Increase pet's HP by 120
		break;

	case BUFF_TYPE_FIRM_DETERMINATION:	// Firm Determination
		// Increase pet's AC by 20
		break;

	case BUFF_TYPE_SPEED2:				// Cold Wave
		pTarget->m_bSpeedAmount = (pTarget->m_bSpeedAmount / 100 * 65);
		break;

	case BUFF_TYPE_ATTACK_RANGE_ARMOR:	// Inevitable Murderous
		pTarget->m_sACAmount += 100;
		// Increase attack range by 1 meter.
		break;

	case BUFF_TYPE_MIRROR_DAMAGE_PARTY: // Minak's Thorn
		// Spreads damage received across party members and mirror's part of the damage.
		break;

	default:
		return false;
	}
	return true;
}

bool CMagicProcess::RemoveType4Buff(uint8 byBuffType, Unit *pTarget)
{
	// Buff must be added at this point. If it doesn't exist, we can't remove it twice.
	FastGuard lock(pTarget->m_buffLock);
	auto itr = pTarget->m_buffMap.find(byBuffType);
	if (itr == pTarget->m_buffMap.end())
		return false;

	// If this buff persists across logout, it should be removed here too.
	if (pTarget->isPlayer()
		&& pTarget->HasSavedMagic(itr->second.m_nSkillID))
		TO_USER(pTarget)->RemoveSavedMagic(itr->second.m_nSkillID);

	pTarget->m_buffMap.erase(itr);

	switch (byBuffType)
	{
	case BUFF_TYPE_HP_MP:
		pTarget->m_sMaxHPAmount = 0;
		pTarget->m_sMaxMPAmount = 0;
		break;

	case BUFF_TYPE_AC:
		pTarget->m_sACAmount = 0;
		break;

	case BUFF_TYPE_SIZE:
		pTarget->StateChangeServerDirect(3, ABNORMAL_NORMAL);
		break;

	case BUFF_TYPE_DAMAGE:
		pTarget->m_bAttackAmount = 100;
		break;

	case BUFF_TYPE_ATTACK_SPEED:
		pTarget->m_bAttackSpeedAmount = 100;
		break;

	case BUFF_TYPE_SPEED:
		pTarget->m_bSpeedAmount = 100;
		break;

	case BUFF_TYPE_STATS:
		if (pTarget->isPlayer())
		{
			TO_USER(pTarget)->SetStatBuff(STAT_STR, 0);
			TO_USER(pTarget)->SetStatBuff(STAT_STA, 0);
			TO_USER(pTarget)->SetStatBuff(STAT_DEX, 0);
			TO_USER(pTarget)->SetStatBuff(STAT_INT, 0);
			TO_USER(pTarget)->SetStatBuff(STAT_CHA, 0);	
		}
		break;

	case BUFF_TYPE_RESISTANCES:
		pTarget->m_bFireRAmount = 0;
		pTarget->m_bColdRAmount = 0;
		pTarget->m_bLightningRAmount = 0;
		pTarget->m_bMagicRAmount = 0;
		pTarget->m_bDiseaseRAmount = 0;
		pTarget->m_bPoisonRAmount = 0;
		break;

	case BUFF_TYPE_ACCURACY:
		pTarget->m_bHitRateAmount = 100;
		pTarget->m_sAvoidRateAmount = 100;
		break;	

	case BUFF_TYPE_MAGIC_POWER:
		pTarget->m_sMagicAttackAmount = 0;
		break;

	case BUFF_TYPE_EXPERIENCE:
		if (pTarget->isPlayer())
			TO_USER(pTarget)->m_bExpGainAmount = 100;
		break;

	case BUFF_TYPE_WEIGHT:
		if (pTarget->isPlayer())
			TO_USER(pTarget)->m_bMaxWeightAmount = 100;
		break;

	case BUFF_TYPE_WEAPON_DAMAGE:
		// uses pType->Attack
		break;

	case BUFF_TYPE_WEAPON_AC:
		pTarget->m_sACAmount = 0;
		break;

	case BUFF_TYPE_LOYALTY:
		// uses pType->ExpPct
		break;

	case BUFF_TYPE_NOAH_BONUS:
		break;

	case BUFF_TYPE_PREMIUM_MERCHANT:
		if (pTarget->isPlayer())
			TO_USER(pTarget)->m_bPremiumMerchant = false;
		break;

	case BUFF_TYPE_ATTACK_SPEED_ARMOR:
		// should this revert a specific amount? if so, we need to store it so that we know how much.
		// most likely though, it's just recalculated.
		pTarget->m_sACAmount = 0;
		pTarget->m_bAttackAmount = 100;
		break;

	case BUFF_TYPE_DAMAGE_DOUBLE:
		pTarget->m_bAttackAmount = 100;
		break;

	case BUFF_TYPE_DISABLE_TARGETING:
		pTarget->m_bIsBlinded = false;
		break;

	case BUFF_TYPE_BLIND:
		pTarget->m_bIsBlinded = false;
		if (pTarget->isPlayer())
			TO_USER(pTarget)->SendUserStatusUpdate(USER_STATUS_BLIND, USER_STATUS_CURE);
		break;

	case BUFF_TYPE_FREEZE:
		// Proportional to the target user's current HP.
		pTarget->m_bSpeedAmount = 100;
		break;

	case BUFF_TYPE_INSTANT_MAGIC:
		pTarget->m_bInstantCast = false;
		break;

	case BUFF_TYPE_DECREASE_RESIST:
		pTarget->m_bFireRAmount			= 0;
		pTarget->m_bColdRAmount			= 0;
		pTarget->m_bLightningRAmount	= 0;
		pTarget->m_bMagicRAmount		= 0;
		pTarget->m_bDiseaseRAmount		= 0;
		pTarget->m_bPoisonRAmount		= 0;
		break;

	case BUFF_TYPE_MAGE_ARMOR:
		pTarget->m_bReflectArmorType = 0;
		break;

	case BUFF_TYPE_PROHIBIT_INVIS:
		pTarget->m_bCanStealth = false;
		break;

	case BUFF_TYPE_RESIS_AND_MAGIC_DMG: // Elysian Web
		// Increases your magic resistance to block an additional 30% magic damage.
		break;

	case BUFF_TYPE_TRIPLEAC_HALFSPEED:	// Wall of Iron
		pTarget->m_sACAmount -= pTarget->m_sTotalAc * 2; // TO-DO: Store this value independently for restoration
		pTarget->m_bSpeedAmount = 100;
		if (pTarget->m_bSpeedAmount = 0)
			pTarget->m_bSpeedAmount = 1;
		break;

	case BUFF_TYPE_BLOCK_CURSE:			// Counter Curse
		// Blocks all curses.
		break;

	case BUFF_TYPE_BLOCK_CURSE_REFLECT:	// Curse Refraction
		// Blocks all curses and has a chance to reflect the curse back upon the caster.
		break;

	case BUFF_TYPE_MANA_ABSORB:		// Outrage / Frenzy / Mana Shield
		// Uses mana to receive damage (for mana shield its multiplied by 4, 100 damage = 400 mana used)
		break;

	case BUFF_TYPE_IGNORE_WEAPON:		// Weapon cancellation
		// Disarms the opponent. (rendering them unable to attack)
		break;

	case BUFF_TYPE_PASSION_OF_SOUL:		// Passion of the Soul
		// Increase pet's HP by 120
		break;

	case BUFF_TYPE_FIRM_DETERMINATION:	// Firm Determination
		// Increase pet's AC by 20
		break;

	case BUFF_TYPE_SPEED2:				// Cold Wave
		pTarget->m_bSpeedAmount = 100;
		break;

	case BUFF_TYPE_ATTACK_RANGE_ARMOR:	// Inevitable Murderous
		pTarget->m_sACAmount -= 100;
		// Buff type increases attack range by 1 meter.
		break;

	case BUFF_TYPE_MIRROR_DAMAGE_PARTY: // Minak's Thorn
		// Spreads damage received across party members and mirror's part of the damage.
		break;

	default:
		return false;
	}

	if (pTarget->isPlayer())
	{
		TO_USER(pTarget)->SetSlotItemValue();
		TO_USER(pTarget)->SetUserAbility();
		TO_USER(pTarget)->Send2AI_UserUpdateInfo();

		Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_END));
		result << byBuffType;
		TO_USER(pTarget)->Send(&result);
	}

	return true;
}