#include "stdafx.h"
#include "Map.h"
#include "EbenezerDlg.h"
#include "../shared/KOSocketMgr.h"
#include "User.h"
#include "MagicProcess.h"
#include "MagicInstance.h"

using std::string;
using std::vector;

void MagicInstance::Run()
{
	if (pSkill == nullptr)
		pSkill = g_pMain->m_MagictableArray.GetData(nSkillID);

	if (pSkill == nullptr
		|| pSkillCaster == nullptr
		|| !UserCanCast())
	{
		SendSkillFailed();
		return;
	}

#if 0
	// If the target is a mob/NPC *or* we're casting an AOE, tell the AI to handle it.
	if (sTargetID >= NPC_BAND
		|| (sTargetID == -1 && 
			(pSkill->bMoral == MORAL_AREA_ENEMY 
				|| pSkill->bMoral == MORAL_AREA_ALL 
				|| pSkill->bMoral == MORAL_SELF_AREA)))
	{
		SendSkillToAI();

		// If the target is specifically a mob, stop here. AI's got this one.
		// Otherwise, it's an AOE -- which means it might affect players too, so we need to handle it too.
		if (sTargetID >= NPC_BAND)
			return;
	}
#endif

	bool bInitialResult;
	switch (bOpcode)
	{
		case MAGIC_CASTING:
			SendSkill(true); // send this to the region
			break;

		case MAGIC_FLYING:
		{
			// Handle arrow & mana checking/removals.
			if (pSkillCaster->isPlayer())
			{
				CUser * pCaster = TO_USER(pSkillCaster);
				_MAGIC_TYPE2 * pType = g_pMain->m_Magictype2Array.GetData(nSkillID);

				// NOTE: Not all skills that use MAGIC_FLYING are type 2 skills.
				// Some type 3 skills use it (such as "Vampiric Fire"). 
				// For these we should really apply the same flying logic, but for now we'll just ignore it.
				if (pType != nullptr)
				{
					// Throwing knives are differentiated by the fact "NeedArrow" is set to 0.
					// We still need to check for & take 1 throwing knife in this case however.
					uint8 bCount = pType->bNeedArrow;
					if (!bCount)
						bCount = 1;

					if (pType == nullptr
						// The user does not have enough arrows! We should point them in the right direction. ;)
						|| (!pCaster->CheckExistItem(pSkill->iUseItem, bCount))
						// Ensure user has enough mana for this skill
						|| pSkill->sMsp > pSkillCaster->GetMana())
					{
						SendSkillFailed();
						return;
					}

					// Add all flying arrow instances to the user's list for hit detection
					FastGuard lock(pCaster->m_arrowLock);
					for (size_t i = 0; i < bCount; i++)
						pCaster->m_flyingArrows.push_back(Arrow(pType->iNum, UNIXTIME));

					// Remove the arrows
					pCaster->RobItem(pSkill->iUseItem, bCount);
				}
				// for non-type 2 skills, ensure we check the user's mana.
				else if (pSkill->sMsp > pSkillCaster->GetMana())
				{
					SendSkillFailed();
					return;
				}

				// Take the required mana for this skill
				pCaster->MSpChange(-(pSkill->sMsp));
			}

			SendSkill(true); // send this to the region
		} break;

		case MAGIC_FAIL:
			SendSkill(false); // don't send this to the region
			break;

		case MAGIC_EFFECTING:
			// Hacky check for a transformation item (Disguise Totem, Disguise Scroll)
			// These apply when first type's set to 0, second type's set and obviously, there's a consumable item.
			// Need to find a better way of handling this.
			if (!bIsRecastingSavedMagic
				&& (pSkill->bType[0] == 0 && pSkill->bType[1] != 0 && pSkill->iUseItem != 0
				&& (pSkillCaster->isPlayer() && TO_USER(pSkillCaster)->CheckExistItem(pSkill->iUseItem, 1))))
			{
				SendTransformationList();
				return;
			}

			bInitialResult = ExecuteSkill(pSkill->bType[0]);

			// NOTE: Some ROFD skills require a THIRD type.
			if (bInitialResult)
				ExecuteSkill(pSkill->bType[1]);
			break;

		case MAGIC_TYPE3_END: //This is also MAGIC_TYPE4_END
			break;

		case MAGIC_CANCEL:
		case MAGIC_CANCEL2:
			Type3Cancel();	//Damage over Time skills.
			Type4Cancel();	//Buffs
			Type6Cancel();	//Transformations
			Type9Cancel();	//Stealth, lupine etc.
			break;

		case MAGIC_TYPE4_EXTEND:
			Type4Extend();
			break;
	}
}

bool MagicInstance::UserCanCast()
{
	if (pSkill == nullptr)
		return false;
	
	// We don't need to check anything as we're just canceling our buffs.
	if (bOpcode == MAGIC_CANCEL || bOpcode == MAGIC_CANCEL2) 
		return true;

	if (bIsRecastingSavedMagic)
		return true;

	if (pSkillCaster->isBlinded())
		return false;

	// Users who are blinking cannot use skills.
	// Additionally, unless it's resurrection-related, dead players cannot use skills.
	if (pSkillCaster->isBlinking()
		|| (pSkillCaster->isDead() && pSkill->bType[0] != 5)) 
		return false;

	// If we're using an AOE, and the target is specified... something's not right.
	if ((pSkill->bMoral >= MORAL_AREA_ENEMY
			&& pSkill->bMoral <= MORAL_SELF_AREA)
		&& sTargetID != -1)
		return false;

	// NPCs do not need further checks.
	// NOTE: The source check's implemented in the main packet wrapper.
	if (pSkillCaster->isNPC())
		return true;

	if (pSkill->sSkill != 0
		&& (TO_USER(pSkillCaster)->m_sClass != (pSkill->sSkill / 10)
			|| pSkillCaster->GetLevel() < pSkill->sSkillLevel))
		return false;

	if ((pSkillCaster->GetMana() - pSkill->sMsp) < 0)
		return false;

	// If we're in a snow war, we're only ever allowed to use the snowball skill.
	if (pSkillCaster->GetZoneID() == ZONE_SNOW_BATTLE && g_pMain->m_byBattleOpen == SNOW_BATTLE 
		&& nSkillID != SNOW_EVENT_SKILL)
		return false;

	// If a target is specified, and we're using an attack skill, determine if the caster can attack the target.
	// NOTE: This disregards whether we're trying/able to attack ourselves (which may be skill induced?).
	if (pSkillTarget != nullptr
		&& (pSkill->bMoral == MORAL_ENEMY || pSkill->bMoral == MORAL_NPC || pSkill->bMoral == MORAL_ALL)
		&& !pSkillCaster->CanAttack(pSkillTarget))
		return false;

	// Archer & transformation skills will handle item checking themselves
	if ((pSkill->bType[0] != 2 && pSkill->bType[0] != 6) 
		// The user does not meet the item's requirements or does not have any of said item.
		&& (pSkill->iUseItem != 0
			&& !TO_USER(pSkillCaster)->CanUseItem(pSkill->iUseItem, 1))) 
		return false;

	// We cannot use CSW transformations outside of Delos (or when CSW is not enabled.)
	if (pSkill->bType[0] == 6
		&& (nSkillID / 10000) == 45
		&& pSkillCaster->GetZoneID() != ZONE_DELOS)
		return false;

	if ((bOpcode == MAGIC_EFFECTING || bOpcode == MAGIC_CASTING) 
		&& !IsAvailable())
		return false;

	// Instant casting affects the next cast skill only, and is then removed.
	if (pSkillCaster->canInstantCast())
		CMagicProcess::RemoveType4Buff(BUFF_TYPE_INSTANT_MAGIC, pSkillCaster);

	//Incase we made it to here, we can cast! Hurray!
	return true;
}

/**
 * @brief	Checks primary type 3 skill prerequisites before executing the skill.
 *
 * @return	true if it succeeds, false if it fails.
 */
bool MagicInstance::CheckType3Prerequisites()
{
	_MAGIC_TYPE3 * pType = g_pMain->m_Magictype3Array.GetData(nSkillID);
	if (pType == nullptr)
		return false;

	// Handle AOE prerequisites
	if (sTargetID == -1)
	{
		// No need to handle any prerequisite logic for NPCs/mobs casting AOEs.
		if (!pSkillCaster->isPlayer())
			return true;

		if (pSkill->bMoral == MORAL_PARTY_ALL
			&& pType->sTimeDamage > 0)
		{
			// Players may not cast group healing spells whilst transformed
			// into a monster (skills with IDs of 45###). 
			if (pSkillCaster->isTransformed()
				&& (TO_USER(pSkillCaster)->m_bAbnormalType / 10000 == 45))
			{
				SendSkillFailed();
				return false;
			}

			// Official behaviour means we cannot cast a group healing spell
			// if we currently have an active restoration spells on us.
			// This behaviour seems fairly illogical, but it's how it works.
			for (int i = 0; i < MAX_TYPE3_REPEAT; i++)
			{
				if (pSkillCaster->m_durationalSkills[i].m_sHPAmount > 0)
				{
					SendSkillFailed();
					return false;
				}
			}
		}

		// No other reason to reject AOE spells.
		return true;
	}
	// Handle prerequisites for skills cast on NPCs.
	else if (sTargetID >= NPC_BAND)
	{
		if (pSkillTarget == nullptr)
			return false;
		
		// Unless the zone is Delos, or it's a healing skill, we can continue on our merry way.
		if (pSkillCaster->GetZoneID() != 30
			|| (pType->sFirstDamage <= 0 && pType->sTimeDamage <= 0))
			return true;

		// We cannot heal gates! That would be bad, very bad.
		if (TO_NPC(pSkillTarget)->GetType() == NPC_GATE) // note: official only checks byType 50
		{
			SendSkillFailed();
			return false;
		}

		// Otherwise, officially there's no reason we can't heal NPCs (more specific logic later).
		return true;
	}
	// Handle prerequisites for skills cast on players.
	else
	{
		// We only care about friendly non-AOE spells.
		if (pSkill->bMoral > MORAL_PARTY)
			return true;

		if (pSkillTarget == nullptr 
			|| !pSkillTarget->isPlayer()
			|| pSkillTarget->isDead())
			return false;

		// If the spell is a healing/restoration spell...
		if (pType->sTimeDamage > 0)
		{
			// Official behaviour means we cannot cast a restoration spell
			// if the target currently has an active restoration spell on them.
			for (int i = 0; i < MAX_TYPE3_REPEAT; i++)
			{
				if (pSkillTarget->m_durationalSkills[i].m_sHPAmount > 0)
				{
					SendSkillFailed();
					return false;
				}
			}
		}

		// It appears that the server should reject any attacks or heals
		// on players that have transformed into monsters.
		if (pSkillTarget->isTransformed()
			&& (TO_USER(pSkillTarget)->m_bAbnormalType / 10000 == 45)
			&& !pSkillCaster->CanAttack(pSkillTarget))
		{
			SendSkillFailed();
			return false;
		}

		return true;
	}
}

bool MagicInstance::ExecuteSkill(uint8 bType)
{
	// Implement player-specific logic before skills are executed.
	if (pSkillCaster->isPlayer())
	{
		// Handle prerequisite checks for primary skills.
		// This must occur before stealth is removed; if a skill fails here, 
		// it should NOT unstealth users.
		if (pSkill->bType[0] == bType)
		{
			switch (bType)
			{
			case 3: 
				if (!CheckType3Prerequisites())
					return false;
			}
		}

		// If a player is stealthed, and they are casting a type 1/2/3/7 skill
		// it is classed as an attack, so they should be unstealthed.
		if (TO_USER(pSkillCaster)->m_bInvisibilityType != INVIS_NONE
			&& ((bType >= 1 && bType <= 3) || (bType == 7)))
		{
			CMagicProcess::RemoveStealth(pSkillCaster, INVIS_DISPEL_ON_MOVE);
			CMagicProcess::RemoveStealth(pSkillCaster, INVIS_DISPEL_ON_ATTACK);
		}
	}

	switch (bType)
	{
		case 1: return ExecuteType1();
		case 2: return ExecuteType2();
		case 3: return ExecuteType3();
		case 4: return ExecuteType4();
		case 5: return ExecuteType5();
		case 6: return ExecuteType6();
		case 7: return ExecuteType7();
		case 8: return ExecuteType8();
		case 9: return ExecuteType9();
	}

	return false;
}

void MagicInstance::SendTransformationList()
{
	if (!pSkillCaster->isPlayer())
		return;

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TRANSFORM_LIST));
	result << nSkillID;
	pSkillCaster->m_nTransformationItem = pSkill->iUseItem;
	TO_USER(pSkillCaster)->Send(&result);
}

/**
 * @brief	Sends the skill failed packet to the caster.
 *
 * @param	sTargetID	Target ID to override with, as some use cases 
 * 						require.
 */
void MagicInstance::SendSkillFailed(int16 sTargetID /*= -1*/)
{
	if (pSkillCaster == nullptr
		|| !pSkillCaster->isPlayer())
		return;

	Packet result;
	BuildSkillPacket(result, sCasterID, sTargetID == -1 ? this->sTargetID : sTargetID, MAGIC_FAIL, nSkillID, sData);
	TO_USER(pSkillCaster)->Send(&result);
}

/**
 * @brief	Builds skill packet using the specified data.
 *
 * @param	result			Buffer to store the packet in.
 * @param	sSkillCaster	Skill caster's ID.
 * @param	sSkillTarget	Skill target's ID.
 * @param	opcode			Skill system opcode.
 * @param	nSkillID		Identifier for the skill.
 * @param	sData			Array of additional misc skill data.
 */
void MagicInstance::BuildSkillPacket(Packet & result, int16 sSkillCaster, int16 sSkillTarget, int8 opcode, 
									 uint32 nSkillID, int16 sData[8])
{
	result.Initialize(WIZ_MAGIC_PROCESS);
	result	<< opcode << nSkillID << sSkillCaster << sSkillTarget
			<< sData[0] << sData[1] << sData[2] << sData[3]
			<< sData[4] << sData[5] << sData[6] << sData[7];
}

/**
 * @brief	Builds and sends skill packet using the specified data to pUnit.
 *
 * @param	pUnit			The unit to send the packet to. If an NPC is specified, 
 * 							bSendToRegion is assumed true.
 * @param	bSendToRegion	true to send the packet to pUser's region.
 * @param	sSkillCaster	Skill caster's ID.
 * @param	sSkillTarget	Skill target's ID.
 * @param	opcode			Skill system opcode.
 * @param	nSkillID		Identifier for the skill.
 * @param	sData			Array of additional misc skill data.
 */
void MagicInstance::BuildAndSendSkillPacket(Unit * pUnit, bool bSendToRegion, int16 sSkillCaster, int16 sSkillTarget, int8 opcode, uint32 nSkillID, int16 sData[8])
{
	Packet result;
	BuildSkillPacket(result, sSkillCaster, sSkillTarget, opcode, nSkillID, sData);

	if (bSendToRegion || pUnit->isNPC())
		pUnit->SendToRegion(&result);
	else
		TO_USER(pUnit)->Send(&result);
}

/**
 * @brief	Sends the skill data in the current context to pUnit. 
 * 			If pUnit is nullptr, it will assume the caster.
 *
 * @param	bSendToRegion	true to send the packet to pUnit's region. 
 * 							If pUnit is an NPC, this is assumed true.
 * @param	pUser		 	The unit to send the packet to.
 * 							If pUnit is an NPC, it will send to pUnit's region regardless.
 */
void MagicInstance::SendSkill(bool bSendToRegion /*= true*/, Unit * pUnit /*= nullptr*/)
{
	// If pUnit is nullptr, it will assume the caster.
	if (pUnit == nullptr)
		pUnit = pSkillCaster;

	// Build the skill packet using the skill data in the current context.
	BuildAndSendSkillPacket(pUnit, bSendToRegion, 
		this->sCasterID, this->sTargetID, this->bOpcode, this->nSkillID, this->sData);
}

bool MagicInstance::IsAvailable()
{
	CUser* pParty = nullptr;   // When the target is a party....
	int modulator = 0, Class = 0, moral = 0, skill_mod = 0 ;
	bool isNPC = (sCasterID >= NPC_BAND);		// Identifies source : true means source is NPC.

	if (pSkill == nullptr)
		goto fail_return;

	if (sTargetID >= 0 && sTargetID < MAX_USER) 
		moral = pSkillTarget->GetNation();
	else if (sTargetID >= NPC_BAND)     // Target existence check routine for NPC.          	
	{
		if (pSkillTarget == nullptr || pSkillTarget->isDead())
			goto fail_return;	//... Assuming NPCs can't be resurrected.

		moral = pSkillTarget->GetNation();
	}
	else if (sTargetID == -1)  // AOE/Party Moral check routine.
	{
		if (isNPC)
		{
			moral = 1;
		}
		else
		{
			if (pSkill->bMoral == MORAL_AREA_ENEMY)
				moral = pSkillCaster->GetNation() == KARUS ? ELMORAD : KARUS;
			else 
				moral = pSkillCaster->GetNation();	
		}
	}
	else 
		moral = pSkillCaster->GetNation();

	switch( pSkill->bMoral ) {		// Compare morals between source and target character.
		case MORAL_SELF:   // #1         // ( to see if spell is cast on the right target or not )
			if (pSkillCaster != pSkillTarget)
				goto fail_return;
			break;
		case MORAL_FRIEND_WITHME:	// #2
			if (moral != 0 && 
				pSkillCaster->GetNation() != moral)
				goto fail_return;
			break;
		case MORAL_FRIEND_EXCEPTME:	   // #3
			if (pSkillCaster->GetNation() != moral
				|| pSkillCaster == pSkillTarget)
				goto fail_return;
			break;
		case MORAL_PARTY:	 // #4
		{
			// NPCs can't *be* in parties.
			if (pSkillCaster->isNPC()
				|| (pSkillTarget != nullptr && pSkillTarget->isNPC()))
				goto fail_return;

			// We're definitely a user, so...
			CUser *pCaster = TO_USER(pSkillCaster);

			// If the caster's not in a party, make sure the target's not someone other than themselves.
			if ((!pCaster->isInParty() && pSkillCaster != pSkillTarget)
				// Verify that the nation matches the intended moral
				|| pCaster->GetNation() != moral
				// and that if there is a target, they're in the same party.
				|| (pSkillTarget != nullptr && 
					TO_USER(pSkillTarget)->m_sPartyIndex != pCaster->m_sPartyIndex))
				goto fail_return;
		} break;
		case MORAL_NPC:		// #5
			if (pSkillTarget == nullptr
				|| !pSkillTarget->isNPC()
				|| pSkillTarget->GetNation() != moral)
				goto fail_return;
			break;
		case MORAL_PARTY_ALL:     // #6
//			if ( !m_pSrcUser->isInParty() ) goto fail_return;		
//			if ( !m_pSrcUser->isInParty() && sid != tid) goto fail_return;					

			break;
		case MORAL_ENEMY:	// #7
			// Nation alone cannot dictate whether a unit can attack another.
			// As such, we must check behaviour specific to these entities.
			// For example: same nation players attacking each other in an arena.
			if (!pSkillCaster->CanAttack(pSkillTarget))
				goto fail_return;
			break;	
		case MORAL_ALL:	 // #8
			// N/A
			break;
		case MORAL_AREA_ENEMY:		// #10
			// N/A
			break;
		case MORAL_AREA_FRIEND:		// #11
			if (pSkillCaster->GetNation() != moral)
				goto fail_return;
			break;
		case MORAL_AREA_ALL:	// #12
			// N/A
			break;
		case MORAL_SELF_AREA:     // #13
			// Remeber, EVERYONE in the area is affected by this one. No moral check!!!
			break;
		case MORAL_CORPSE_FRIEND:		// #25
			if (pSkillCaster->GetNation() != moral
				// We need to revive *something*.
				|| pSkillTarget == nullptr
				// We cannot revive ourselves.
				|| pSkillCaster == pSkillTarget
				// We can't revive living targets.
				|| pSkillTarget->isAlive())
				goto fail_return;
			break;
		case MORAL_CLAN:		// #14
		{
			// NPCs cannot be in clans.
			if (pSkillCaster->isNPC()
				|| (pSkillTarget != nullptr && pSkillTarget->isNPC()))
				goto fail_return;

			// We're definitely a user, so....
			CUser * pCaster = TO_USER(pSkillCaster);

			// If the caster's not in a clan, make sure the target's not someone other than themselves.
			if ((!pCaster->isInClan() && pSkillCaster != pSkillTarget)
				// Verify the intended moral
				|| pCaster->GetNation() != moral
				// If we're targeting someone, that target must be in our clan.
				|| (pSkillTarget != nullptr 
					&& TO_USER(pSkillTarget)->GetClanID() != pCaster->GetClanID()))
				goto fail_return;
		} break;

		case MORAL_CLAN_ALL:	// #15
			break;
//
	}

	// This only applies to users casting skills. NPCs are fine and dandy (we can trust THEM at least).
	if (pSkillCaster->isPlayer())
	{
		modulator = pSkill->sSkill % 10;     // Hacking prevention!
		if( modulator != 0 ) {	
			Class = pSkill->sSkill / 10;
			if( Class != TO_USER(pSkillCaster)->GetClass() ) goto fail_return;
			if( pSkill->sSkillLevel > TO_USER(pSkillCaster)->m_bstrSkill[modulator] ) goto fail_return;
		}
		else if (pSkill->sSkillLevel > pSkillCaster->GetLevel()) goto fail_return;

		if (pSkill->bType[0] == 1) {	// Weapons verification in case of COMBO attack (another hacking prevention).
			if (pSkill->sSkill == 1055 || pSkill->sSkill == 2055) {		// Weapons verification in case of dual wielding attacks !		
				_ITEM_TABLE *pLeftHand = TO_USER(pSkillCaster)->GetItemPrototype(LEFTHAND),
							*pRightHand = TO_USER(pSkillCaster)->GetItemPrototype(RIGHTHAND);

				if ((pLeftHand != nullptr && !pLeftHand->isSword() && !pLeftHand->isAxe() && !pLeftHand->isMace())
					|| (pRightHand != nullptr && !pRightHand->isSword() && !pRightHand->isAxe() && !pRightHand->isMace()))
					return false;
			}
			else if (pSkill->sSkill == 1056 || pSkill->sSkill == 2056) {	// Weapons verification in case of 2 handed attacks !
				_ITEM_TABLE	*pRightHand = TO_USER(pSkillCaster)->GetItemPrototype(RIGHTHAND);

				if (TO_USER(pSkillCaster)->GetItem(LEFTHAND)->nNum != 0
					|| (pRightHand != nullptr 
						&& !pRightHand->isSword() && !pRightHand->isAxe() 
						&& !pRightHand->isMace() && !pRightHand->isSpear()))
					return false;
			}
		}

		// Handle MP/HP/item loss.
		if (bOpcode == MAGIC_EFFECTING) 
		{
			int total_hit = pSkillCaster->m_sTotalHit;

			if (pSkill->bType[0] == 2 && pSkill->bFlyingEffect != 0) // Type 2 related...
				return true;		// Do not reduce MP/SP when flying effect is not 0.

#if 0 // dodgy check preventing legitimate behaviour
			if (pSkill->bType[0] == 1 && sData[0] > 1)
				return true;		// Do not reduce MP/SP when combo number is higher than 0.
#endif
 
			if (pSkill->sMsp > pSkillCaster->GetMana())
				goto fail_return;

			// If the PLAYER uses an item to cast a spell.
 			if (pSkillCaster->isPlayer()
				&& (pSkill->bType[0] == 3 || pSkill->bType[0] == 4))
			{
				if (pSkill->iUseItem != 0) {
					_ITEM_TABLE* pItem = nullptr;				// This checks if such an item exists.
					pItem = g_pMain->GetItemPtr(pSkill->iUseItem);
					if( !pItem ) return false;

					if ((pItem->m_bClass != 0 && !TO_USER(pSkillCaster)->JobGroupCheck(pItem->m_bClass))
						|| (pItem->m_bReqLevel != 0 && TO_USER(pSkillCaster)->GetLevel() < pItem->m_bReqLevel)
						|| (!TO_USER(pSkillCaster)->RobItem(pSkill->iUseItem, 1)))	
						return false;
				}
			}
			if (pSkill->bType[0] != 4 || (pSkill->bType[0] == 4 && sTargetID == -1))
				pSkillCaster->MSpChange(-(pSkill->sMsp));

			if (pSkill->sHP > 0 && pSkill->sMsp == 0) {			// DEDUCTION OF HPs in Magic/Skill using HPs.
				if (pSkill->sHP > pSkillCaster->GetHealth()) goto fail_return;
				pSkillCaster->HpChange(-pSkill->sHP);
			}
		}
	}

	return true;      // Magic was successful! 

fail_return:    // In case of failure, send a packet(!)
	if (!isNPC)
		SendSkillFailed();

	return false;     // Magic was a failure!
}

bool MagicInstance::ExecuteType1()
{	
	if (pSkill == nullptr)
		return false;

	int damage = 0;
	bool bResult = false;

	_MAGIC_TYPE1* pType = g_pMain->m_Magictype1Array.GetData(nSkillID);
	if (pType == nullptr)
		return false;

	if (pSkillTarget != nullptr && !pSkillTarget->isDead())
	{
		bResult = 1;
		damage = pSkillCaster->GetDamage(pSkillTarget, pSkill);
		pSkillTarget->HpChange(-damage, pSkillCaster);

		if (pSkillTarget->m_bReflectArmorType != 0 && pSkillCaster != pSkillTarget)
			ReflectDamage(damage);
	}

	// This should only be sent once. I don't think there's reason to enforce this, as no skills behave otherwise
	sData[3] = (damage == 0 ? -104 : 0);

	// Send the skill data in the current context to the caster's region
	SendSkill();

	return bResult;
}

bool MagicInstance::ExecuteType2()
{
	/*
		NOTE: 
			Archery skills work differently to most other skills.
			
			When an archery skill is used, the client sends MAGIC_FLYING (instead of MAGIC_CASTING) 
			to show the arrows flying in the air to their targets.
			
			The client chooses the target(s) to be hit by the arrows.

			When an arrow hits a target, it will send MAGIC_EFFECTING which triggers this handler.
			An arrow sent may not necessarily hit a target.

			As such, for archery skills that use multiple arrows, not all n arrows will necessarily
			hit their target, and thus they will not necessarily call this handler all n times.

			What this means is, we must remove all n arrows from the user in MAGIC_FLYING, otherwise
			it is not guaranteed all arrows will be hit and thus removed.
			(and we can't just go and take all n items each time an arrow hits, that could potentially 
			mean 25 arrows are removed [5 each hit] when using "Arrow Shower"!)

			However, via the use of hacks, this MAGIC_FLYING step can be skipped -- so we must also check 
			to ensure that there arrows are indeed flying, to prevent users from spamming the skill
			without using arrows.
	 */
	if (pSkill == nullptr)
		return false;

	int damage = 0;
	bool bResult = false;
	float total_range = 0.0f;	// These variables are used for range verification!
	int sx, sz, tx, tz;

	_MAGIC_TYPE2 *pType = g_pMain->m_Magictype2Array.GetData(nSkillID);
	if (pType == nullptr)
		return false;

	int range = 0;

	// If we need arrows, then we require a bow.
	// This check is needed to allow for throwing knives (the sole exception at this time.
	// In this case, 'NeedArrow' is set to 0 -- we do not need a bow to use throwing knives, obviously.
	if (pType->bNeedArrow > 0)
	{
		_ITEM_TABLE * pTable = nullptr;
		if (pSkillCaster->isPlayer())
		{
			// Not wearing a left-handed bow
			pTable = TO_USER(pSkillCaster)->GetItemPrototype(LEFTHAND);
			if (pTable == nullptr || !pTable->isBow())
			{
				pTable = TO_USER(pSkillCaster)->GetItemPrototype(RIGHTHAND);

				// Not wearing a right-handed (2h) bow either!
				if (pTable == nullptr || !pTable->isBow())
					return false;
			}
		}
		else 
		{
			// TO-DO: Verify this. It's more a placeholder than anything. 
			pTable = g_pMain->GetItemPtr(TO_NPC(pSkillCaster)->m_iWeapon_1);
			if (pTable == nullptr)
				return false; 
		}
		
		// For arrow skills, we require a bow & its range.
		range = pTable->m_sRange;
	}
	else
	{
		// For non-arrow skills (i.e. throwing knives) we should use the skill's range.
		range = pSkill->sRange;
	}

	// is this checked already?
	if (pSkillTarget == nullptr || pSkillTarget->isDead())
		goto packet_send;
	
	total_range = pow(((pType->sAddRange * range) / 100.0f), 2.0f) ;     // Range verification procedure.
	sx = (int)pSkillCaster->GetX(); tx = (int)pSkillTarget->GetX();
	sz = (int)pSkillCaster->GetZ(); tz = (int)pSkillTarget->GetZ();
	
	if ((pow((float)(sx - tx), 2.0f) + pow((float)(sz - tz), 2.0f)) > total_range)	   // Target is out of range, exit.
		goto packet_send;
	
	if (pSkillCaster->isPlayer())
	{
		CUser * pUser = TO_USER(pSkillCaster);
		FastGuard lock(pUser->m_arrowLock);

		// No arrows currently flying.
		if (pUser->m_flyingArrows.empty())
			goto packet_send;

		ArrowList::iterator arrowItr;
		bool bFoundArrow = false;
		for (auto itr = pUser->m_flyingArrows.begin(); itr != pUser->m_flyingArrows.end();)
		{
			if (UNIXTIME >= itr->tFlyingTime + ARROW_EXPIRATION_TIME)
			{
				itr = pUser->m_flyingArrows.erase(itr);
			}
			else
			{
				if (itr->nSkillID == nSkillID)
				{
					arrowItr = itr; /* don't break out here to ensure we remove all expired arrows */
					bFoundArrow = true;
				}

				++itr;
			}
		}

		// No flying arrow matching this skill's criteria was found.
		// User's probably cheating.
		if (!bFoundArrow)
			goto packet_send;

		// Remove this instance's arrow now that we've found it.
		pUser->m_flyingArrows.erase(arrowItr);
	}

	damage = pSkillCaster->GetDamage(pSkillTarget, pSkill);  // Get damage points of enemy.	

	pSkillTarget->HpChange(-damage, pSkillCaster);     // Reduce target health point.
	if (pSkillTarget->m_bReflectArmorType != 0 && pSkillCaster != pSkillTarget)
		ReflectDamage(damage);

	bResult = true;

packet_send:
	// This should only be sent once. I don't think there's reason to enforce this, as no skills behave otherwise
	sData[3] = (damage == 0 ? -104 : 0);

	// Send the skill data in the current context to the caster's region
	SendSkill();

	return bResult;
}

// Applied when a magical attack, healing, and mana restoration is done.
bool MagicInstance::ExecuteType3()
{	
	int damage = 0, duration_damage = 0;
	vector<Unit *> casted_member;

	_MAGIC_TYPE3* pType = g_pMain->m_Magictype3Array.GetData(nSkillID);
	if (pType == nullptr) 
		return false;

	// If the target's a group of people...
	if (sTargetID == -1)
	{
		std::vector<uint16> unitList;
		g_pMain->GetUnitListFromSurroundingRegions(pSkillCaster, &unitList);
		foreach (itr, unitList)
		{		
			Unit * pTarget = g_pMain->GetUnit(*itr);
			if (pSkillCaster != pTarget
				&& !pTarget->isDead() && !pTarget->isBlinking()
				&& CMagicProcess::UserRegionCheck(pSkillCaster, pTarget, pSkill, pType->bRadius, sData[0], sData[2]))
				casted_member.push_back(pTarget);
		}

		if (casted_member.empty())
		{
			SendSkillFailed();
			return false;			
		}

		// Hack to allow for the showing of AOE skills under any circumstance.
		// Send the skill data in the current context to the caster's region
		if (sTargetID == -1)
			SendSkill();
	}
	else
	{	// If the target was a single unit.
		if (pSkillTarget == nullptr 
			|| pSkillTarget->isDead() 
			|| (pSkillTarget->isPlayer() 
				&& TO_USER(pSkillTarget)->isBlinking())) 
			return false;
		
		casted_member.push_back(pSkillTarget);
	}
	
	// Anger explosion requires the caster be a player
	// and a full anger gauge (which will be reset after use).
	if (pType->bDirectType == 18)
	{
		// Only players can cast this skill.
		if (!pSkillCaster->isPlayer()
			|| !TO_USER(pSkillCaster)->hasFullAngerGauge())
			return false;

		// Reset the anger gauge
		TO_USER(pSkillCaster)->UpdateAngerGauge(0);
	}

	foreach (itr, casted_member)
	{
		Unit * pTarget = *itr; // it's checked above, not much need to check it again
		if ((pType->sFirstDamage < 0) && (pType->bDirectType == 1 || pType->bDirectType == 8) && (nSkillID < 400000))	// If you are casting an attack spell.
			damage = GetMagicDamage(pTarget, pType->sFirstDamage, pType->bAttribute);	// Get Magical damage point.
		else 
			damage = pType->sFirstDamage;

		if (pSkillCaster->isPlayer())
		{
			if (pSkillCaster->GetZoneID() == ZONE_SNOW_BATTLE && g_pMain->m_byBattleOpen == SNOW_BATTLE)
				damage = -10;
		}

		// Non-durational spells.
		if (pType->bDuration == 0) 
		{
			// Affects target's HP
			if (pType->bDirectType == 1)
			{			
				pTarget->HpChange(damage, pSkillCaster);

				if (pTarget->m_bReflectArmorType != 0 && pTarget != pSkillCaster)
					ReflectDamage(damage);
			}
			// Affects target's MP
			else if (pType->bDirectType == 2 || pType->bDirectType == 3)
			{
				pTarget->MSpChange(damage);
			}
			// "Magic Hammer" repairs equipped items.
			else if (pType->bDirectType == 4)
			{
				if (pTarget->isPlayer())
					TO_USER(pTarget)->ItemWoreOut(DEFENCE, -damage);
			}
			// Need to absorb HP from the target user to the source user
			// NOTE: Must only affect players.
			else if (pType->bDirectType == 8)
			{
				continue;
			}
			// Damage based on percentage of target's max HP
			else if (pType->bDirectType == 9) //Damage based on percentage of target's max HP
			{
				if (pType->sFirstDamage < 100)
					damage = (pType->sFirstDamage * pTarget->GetHealth()) / -100;
				else
					damage = (pTarget->GetMaxHealth() * (pType->sFirstDamage - 100)) / 100;

				pTarget->HpChange(damage, pSkillCaster);
			}
			// Drains target's MP, gives half of it to the caster as HP
			// NOTE: Non-durational form (as in 1.8xx). This was made durational later (database configured).
			else if (pType->bDirectType == 16)
			{
				// Only apply this to players
				if (pTarget->isPlayer())
				{
					pTarget->MSpChange(pType->sFirstDamage);
					pSkillCaster->HpChange(-(pType->sFirstDamage) / 2);
				}
			}

		}
		// Durational spells! Durational spells only involve HP.
		else if (pType->bDuration != 0) 
		{
			if (damage != 0)		// In case there was first damage......
				pTarget->HpChange(damage, pSkillCaster);			// Initial damage!!!

			if (pTarget->isAlive()) 
			{
				if (pType->sTimeDamage < 0) 
					duration_damage = GetMagicDamage(pTarget, pType->sTimeDamage, pType->bAttribute);
				else 
					duration_damage = pType->sTimeDamage;

				// Setup DOT (damage over time)
				for (int k = 0; k < MAX_TYPE3_REPEAT; k++) 
				{
					Unit::MagicType3 * pEffect = &pTarget->m_durationalSkills[k];
					if (pEffect->m_byUsed)
						continue;

					pEffect->m_byUsed = true;
					pEffect->m_tHPLastTime = 0;
					pEffect->m_bHPInterval = 2;					// interval of 2s between each damage loss/HP gain 

					// number of ticks required at a rate of 2s per tick over the total duration of the skill

					// NOTE: This (and the calculated HP amount) are being ceil()'d temporarily to bring us 
					// closer to hitting the full amount, which is currently not possible as the client 
					// usually cancels the "expired" skills 1-2 ticks short.
					// The excess here is negligible, and more than likely official exhibits the same behaviour,
					// however we'll revise when we handle these ticks to speed things up, which should hopefully 
					// make this a lot less of an issue.
	
					float tickCount = ceil((float)pType->bDuration / (float)pEffect->m_bHPInterval);

					// amount of HP to be given/taken every tick at a rate of 2s per tick
					pEffect->m_sHPAmount = (int16)(ceil(duration_damage / tickCount));
																					
					pEffect->m_bTickCount = 0;
					pEffect->m_bTickLimit = (uint8)(tickCount);
					pEffect->m_sSourceID = sCasterID;
					break;
				}

				pTarget->m_bType3Flag = true;
			}

			// Send the status updates (i.e. DOT or position indicators) to the party/user
			if (pTarget->isPlayer()
				// Ignore healing spells, not sure if this will break any skills.
				&& pType->sTimeDamage < 0)
			{
				TO_USER(pTarget)->SendUserStatusUpdate(pType->bAttribute == POISON_R ? USER_STATUS_POISON : USER_STATUS_DOT, USER_STATUS_INFLICT);
			}

		}

		if (sTargetID != -1)
			sData[1] = 1;

		// Send the skill data in the current context to the caster's region, with the target explicitly set.
		// In the case of AOEs, this will mean the AOE will show the AOE's effect on the user (not show the AOE itself again).
		if (pSkill->bType[1] == 0 || pSkill->bType[1] == 3)
			BuildAndSendSkillPacket(pSkillCaster, true, sCasterID, pTarget->GetID(), bOpcode, nSkillID, sData);

		// Tell the AI server we're healing someone (so that they can choose to pick on the healer!)
		if (pType->bDirectType == 1 && damage > 0
			&& sCasterID != sTargetID)
		{
			Packet result(AG_HEAL_MAGIC);
			result << sCasterID;
			g_pMain->Send_AIServer(&result);
		}
	}

	return true;
}

bool MagicInstance::ExecuteType4()
{
	int damage = 0;

	vector<CUser *> casted_member;
	if (pSkill == nullptr)
		return false;

	_MAGIC_TYPE4* pType = g_pMain->m_Magictype4Array.GetData(nSkillID);
	if (pType == nullptr)
		return false;

	if (!bIsRecastingSavedMagic 
		&& sTargetID >= 0 
		&& pSkillTarget->HasSavedMagic(nSkillID))
		return false;

	if (sTargetID == -1)
	{
		// TO-DO: Localise this. This is horribly unnecessary.
		SessionMap & sessMap = g_pMain->m_socketMgr.GetActiveSessionMap();
		foreach (itr, sessMap)
		{		
			CUser* pTUser = TO_USER(itr->second);
			if (!pTUser->isDead() && !pTUser->isBlinking()
				&& CMagicProcess::UserRegionCheck(pSkillCaster, pTUser, pSkill, pType->bRadius, sData[0], sData[2]))
				casted_member.push_back(pTUser);
		}
		g_pMain->m_socketMgr.ReleaseLock();

		if (casted_member.empty())
		{		
			if (pSkillCaster->isPlayer())
				SendSkillFailed();

			return false;
		}
	}
	else 
	{
		// If the target was another single player.
		CUser* pTUser = g_pMain->GetUserPtr(sTargetID);
		if (pTUser == nullptr 
			|| pTUser->isDead() || (pTUser->isBlinking() && !bIsRecastingSavedMagic)) 
			return false;

		casted_member.push_back(pTUser);
	}

	foreach (itr, casted_member)
	{
		uint8 bResult = 1;
		CUser* pTUser = *itr;
		_BUFF_TYPE4_INFO pBuffInfo;

		pTUser->m_buffLock.Acquire();
		Type4BuffMap::iterator buffItr = pTUser->m_buffMap.find(pType->bBuffType);
		bool bFoundBuff = (buffItr != pTUser->m_buffMap.end());
		pTUser->m_buffLock.Release();

		// If this skill is a debuff, and the caster is in the crossfire, 
		// we should not bother debuffing them.
		if (!pType->bIsBuff
			&& pTUser == pSkillCaster)
			continue;

		// If the user already has this buff
		if (bFoundBuff 
			// or it's a curse (debuff), and we're blocking them 
			|| (!pType->bIsBuff && pTUser->m_bBlockCurse)
			// or we couldn't grant the (de)buff...
			|| !CMagicProcess::GrantType4Buff(pSkill, pType, pSkillCaster, pTUser, bIsRecastingSavedMagic))
		{
			// We should only error out if we cannot grant a targeted buff
			// or, if the *targeted* user is blocking curses (debuffs).
			if (sTargetID != -1
				&& (pType->bIsBuff || (!pType->bIsBuff && pTUser->m_bBlockCurse)))
			{
				bResult = 0;
				goto fail_return;
			}

			// Debuffs of any kind, or area buffs should be ignored and carry on.
			// Usually - debuffs specifically - they correspond with attack skills which should
			// not be reset on fail.
			continue;
		}

		if (nSkillID > 500000 && pTUser->isPlayer())
			pTUser->InsertSavedMagic(nSkillID, pType->sDuration);

		if (pSkillCaster->isPlayer()
			&& (sTargetID != -1 && pSkill->bType[0] == 4))
			pSkillCaster->MSpChange( -(pSkill->sMsp) );

		pBuffInfo.m_nSkillID = nSkillID;
		pBuffInfo.m_bIsBuff = pType->bIsBuff;

		pBuffInfo.m_bDurationExtended = false;
		pBuffInfo.m_tEndTime = UNIXTIME + pType->sDuration;

		// Add the buff into the buff map.
		pTUser->AddType4Buff(pType->bBuffType, pBuffInfo);

		// Update character stats.
		pTUser->SetUserAbility();

		if (pTUser->isInParty() && pBuffInfo.isDebuff())
			pTUser->SendPartyStatusUpdate(2, 1);

		pTUser->Send2AI_UserUpdateInfo();

	fail_return:
		if (pSkill->bType[1] == 0 || pSkill->bType[1] == 4)
		{
			CUser *pUser = (pSkillCaster->isPlayer() ? TO_USER(pSkillCaster) : pTUser);

			if (!bIsRecastingSavedMagic)
				sData[3] = (bResult == 1 || sData[3] == 0 ? pType->sDuration : 0);

			int16 sDataCopy[8] = 
			{
				sData[0], bResult, sData[2], sData[3],
				sData[4], pType->bSpeed, sData[6], sData[7]
			};

			BuildAndSendSkillPacket(pUser, true, sCasterID, (*itr)->GetID(), bOpcode, nSkillID, sDataCopy);

			if (pSkill->bMoral >= MORAL_ENEMY)
			{
				UserStatus status = USER_STATUS_POISON;
				if (pType->bBuffType == BUFF_TYPE_SPEED || pType->bBuffType == BUFF_TYPE_SPEED2)
					status = USER_STATUS_SPEED;

				pTUser->SendUserStatusUpdate(status, USER_STATUS_INFLICT);
			}
		}
		
		if (bResult == 0
			&& pSkillCaster->isPlayer())
			SendSkillFailed((*itr)->GetID());

		continue;
	}
	return true;
}

bool MagicInstance::ExecuteType5()
{
	// Disallow NPCs (for now?).
	if (pSkillCaster->isNPC()
		|| (pSkillTarget != nullptr && pSkillTarget->isNPC()))
		return false;

	int damage = 0;
	int buff_test = 0; bool bType3Test = true, bType4Test = true; 	

	if (pSkill == nullptr)
		return false;

	_MAGIC_TYPE5* pType = g_pMain->m_Magictype5Array.GetData(nSkillID);
	if (pType == nullptr
		|| pSkillTarget == nullptr 
		|| (pSkillTarget->isDead() && pType->bType != RESURRECTION) 
		|| (!pSkillTarget->isDead() && pType->bType == RESURRECTION)) 
		return false;

	Type4BuffMap::iterator buffIterator;

	switch (pType->bType)
	{
		// Remove all DOT skills
		case REMOVE_TYPE3:
			for (int i = 0; i < MAX_TYPE3_REPEAT; i++)
			{
				Unit::MagicType3 * pEffect = &pSkillTarget->m_durationalSkills[i];
				if (!pEffect->m_byUsed)
					continue;

				// Ignore healing-over-time skills
				if (pEffect->m_sHPAmount >= 0)
					continue;

				pEffect->Reset();
				if (pSkillTarget->isPlayer())
				{
					// TO-DO: Wrap this up (ugh, I feel so dirty)
					Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE3_END));
					result << uint8(200); // removes DOT skill
					TO_USER(pSkillTarget)->Send(&result); 
				}
			}

			buff_test = 0;
			for (int j = 0; j < MAX_TYPE3_REPEAT; j++)
			{
				if (pSkillTarget->m_durationalSkills[j].m_byUsed)
					buff_test++;
			}
			if (buff_test == 0) pSkillTarget->m_bType3Flag = false;	

			// Check for Type 3 Curses.
			for (int k = 0; k < MAX_TYPE3_REPEAT; k++)
			{
				if (pSkillTarget->m_durationalSkills[k].m_sHPAmount < 0)
				{
					bType3Test = false;
					break;
				}
			}
  
			if (pSkillTarget->isPlayer()
				&& TO_USER(pSkillTarget)->isInParty() && bType3Test)
				TO_USER(pSkillTarget)->SendPartyStatusUpdate(1);
			break;

		case REMOVE_TYPE4: // Remove type 4 debuffs
			pSkillTarget->m_buffLock.Acquire();
			foreach (itr, pSkillTarget->m_buffMap)
			{
				if (itr->second.isDebuff())
					CMagicProcess::RemoveType4Buff(itr->first, TO_USER(pSkillTarget));

				if (pSkillTarget->HasSavedMagic(itr->first))
					TO_USER(pSkillTarget)->RecastSavedMagic();
			}
			pSkillTarget->m_buffLock.Release();

			// NOTE: This originally checked to see if there were any active debuffs.
			// As this seems pointless (as we're removing all of them), it was removed
			// however leaving this note in, in case this behaviour in certain conditions
			// is required.
			if (pSkillTarget->isPlayer() && TO_USER(pSkillTarget)->isInParty())
				TO_USER(pSkillTarget)->SendPartyStatusUpdate(2, 0);
			break;
			
		case RESURRECTION:		// RESURRECT A DEAD PLAYER!!!
			if (pSkillTarget->isPlayer())
				TO_USER(pSkillTarget)->Regene(1, nSkillID);
			break;

		case REMOVE_BLESS:
			pSkillTarget->m_buffLock.Acquire();
			buffIterator = pSkillTarget->m_buffMap.find(BUFF_TYPE_HP_MP);
			if (buffIterator != pSkillTarget->m_buffMap.end() 
				&& buffIterator->second.isBuff()) 
			{
				CMagicProcess::RemoveType4Buff(BUFF_TYPE_HP_MP, TO_USER(pSkillTarget));

				bool bIsDebuffed = false;
				foreach (itr, pSkillTarget->m_buffMap)
				{
					if (itr->second.isDebuff())
					{
						bIsDebuffed = true;
						break;
					}
				}

				if (pSkillTarget->isPlayer() && TO_USER(pSkillTarget)->isInParty() && !bIsDebuffed) 
					TO_USER(pSkillTarget)->SendPartyStatusUpdate(2, 0);
			}
			pSkillTarget->m_buffLock.Release();
			break;
	}

	if (pSkill->bType[1] == 0 || pSkill->bType[1] == 5)
		SendSkill();

	return true;
}

bool MagicInstance::ExecuteType6()
{
	if (pSkill == nullptr
		|| !pSkillCaster->isPlayer())
		return false;

	_MAGIC_TYPE6 * pType = g_pMain->m_Magictype6Array.GetData(nSkillID);
	uint32 iUseItem = 0;
	uint16 sDuration = 0;

	if (pType == nullptr
		// Allow NPC transformations in PVP zones
		|| (pType->bUserSkillUse != 3 && pSkillCaster->GetMap()->canAttackOtherNation())
		|| (!bIsRecastingSavedMagic && pSkillCaster->isTransformed())
		// All buffs must be removed before using transformation skills
		|| pSkillCaster->isBuffed())
	{
		// If we're recasting it, then it's either already cast on us (i.e. we're changing zones)
		// or we're relogging. We need to remove it now, as the client doesn't see us using it.
		if (bIsRecastingSavedMagic)
			Type6Cancel(true);

		return false;
	}

	// We can ignore all these checks if we're just recasting on relog.
	if (!bIsRecastingSavedMagic)
	{
		if (pSkillTarget->HasSavedMagic(nSkillID))
			return false;

		// Let's start by looking at the item that was used for the transformation.
		_ITEM_TABLE *pTable = g_pMain->GetItemPtr(pSkillCaster->m_nTransformationItem);

		// Also, for the sake of specific skills that bypass the list, let's lookup the 
		// item attached to the skill.
		_ITEM_TABLE *pTable2 = g_pMain->GetItemPtr(pSkill->iUseItem);

		// If neither of these items exist, we have a bit of a problem...
		if (pTable == nullptr 
			&& pTable2 == nullptr)
			return false;

		/*
			If it's a totem (which is apparently a ring), then we need to override it 
			with a gem (which are conveniently stored in the skill table!)

			The same is true for special items such as the Hera transformation scroll, 
			however we need to go by the item attached to the skill for this one as 
			these skills bypass the transformation list and thus do not set the flag.
		*/


		// NOTE: Should this make use of "NeedItem"? It could very well indicate which to use.

		// Special items (e.g. Hera transformation scroll) use the scroll (tied to the skill)
		if ((pTable2 != nullptr && (pTable2->m_bKind == 255 || pTable2->m_bKind == 97))
			// Totems (i.e. rings) take gems (tied to the skill)
			|| (pTable != nullptr && pTable->m_bKind == 93)) 
			iUseItem = pSkill->iUseItem;
		// If we're using a normal transformation scroll, we can leave the item as it is.
		else 
			iUseItem = pSkillCaster->m_nTransformationItem;

		// Attempt to take the item (no further checks, so no harm in multipurposing)
		// If we add more checks, remember to change this check.
		if (!TO_USER(pSkillCaster)->RobItem(iUseItem, 1))
			return false;

		// User's casting a new skill. Use the full duration.
		sDuration = pType->sDuration;
	}
	else 
	{
		// Server's recasting the skill (kept on relog, zone change, etc.)
		int16 tmp = pSkillCaster->GetSavedMagicDuration(nSkillID);

		// Has it expired (or not been set?) -- just in case.
		if (tmp <= 0)
			return false;

		// it's positive, so no harm here in casting.
		sDuration = tmp;
	}

	// TO-DO : Save duration, and obviously end
	pSkillCaster->m_tTransformationStartTime = UNIXTIME;
	pSkillCaster->m_sTransformationDuration = sDuration;

	pSkillCaster->StateChangeServerDirect(3, nSkillID);
	pSkillCaster->m_bIsTransformed = true;

	// TO-DO : Give the users ALL TEH BONUSES!!

	sData[1] = 1;
	sData[3] = sDuration;
	SendSkill();

	pSkillCaster->InsertSavedMagic(nSkillID, sDuration);
	return true;
}

bool MagicInstance::ExecuteType7()
{
	return true;
}

// Warp, resurrection, and summon spells.
bool MagicInstance::ExecuteType8()
{	
	if (pSkill == nullptr)
		return false;

	vector<CUser *> casted_member;
	_MAGIC_TYPE8* pType = g_pMain->m_Magictype8Array.GetData(nSkillID);
	if (pType == nullptr)
		return false;

	if (sTargetID == -1)
	{
		// TO-DO: Localise this loop to make it not suck (the life out of the server).
		SessionMap & sessMap = g_pMain->m_socketMgr.GetActiveSessionMap();
		foreach (itr, sessMap)
		{		
			CUser* pTUser = TO_USER(itr->second);
			if (CMagicProcess::UserRegionCheck(pSkillCaster, pTUser, pSkill, pType->sRadius, sData[0], sData[2]))
				casted_member.push_back(pTUser);
		}
		g_pMain->m_socketMgr.ReleaseLock();

		if (casted_member.empty()) 
			return false;	
	}
	else 
	{	// If the target was another single player.
		CUser* pTUser = g_pMain->GetUserPtr(sTargetID);
		if (pTUser == nullptr) 
			return false;
		
		casted_member.push_back(pTUser);
	}

	foreach (itr, casted_member)
	{
		uint8 bResult = 0;
		_OBJECT_EVENT* pEvent = nullptr;
		CUser* pTUser = *itr;
		// If we're in a home zone, we'll want the coordinates from there. Otherwise, assume our own home zone.
		_HOME_INFO* pHomeInfo = g_pMain->m_HomeArray.GetData(pTUser->GetZoneID() <= ZONE_ELMORAD ? pTUser->GetZoneID() : pTUser->GetNation());
		if (pHomeInfo == nullptr)
			return false;

		if (pType->bWarpType != 11) 
		{   // Warp or summon related: targets CANNOT be dead.
			if (pTUser->isDead())
				goto packet_send;
		}
		// Resurrection related: we're reviving DEAD targets.
		else if (!pTUser->isDead()) 
			goto packet_send;

		// Is target already warping?			
		if (pTUser->m_bWarp)
			goto packet_send;

		switch(pType->bWarpType) {	
			case 1:		// Send source to resurrection point.
				// Send the packet to the target.
				sData[1] = 1;
				BuildAndSendSkillPacket(*itr, true, sCasterID, (*itr)->GetID(), bOpcode, nSkillID, sData); 
				if (pTUser->GetMap() == nullptr)
					continue;

				pEvent = pTUser->GetMap()->GetObjectEvent(pTUser->m_sBind);

				if (pEvent != nullptr)
					pTUser->Warp(uint16(pEvent->fPosX * 10), uint16(pEvent->fPosZ * 10));	
				else if (pTUser->GetZoneID() <= ELMORAD) 
				{
					if (pTUser->GetNation() == KARUS)
						pTUser->Warp(uint16((pHomeInfo->KarusZoneX + myrand(0, pHomeInfo->KarusZoneLX)) * 10), uint16((pHomeInfo->KarusZoneZ + myrand(0, pHomeInfo->KarusZoneLZ)) * 10));
					else
						pTUser->Warp(uint16((pHomeInfo->ElmoZoneX + myrand(0, pHomeInfo->ElmoZoneLX)) * 10), uint16((pHomeInfo->ElmoZoneZ + + myrand(0, pHomeInfo->ElmoZoneLZ)) * 10));
				}
				else if (pTUser->GetMap()->isWarZone())
					pTUser->Warp(uint16((pHomeInfo->BattleZoneX + myrand(0, pHomeInfo->BattleZoneLX)) * 10), uint16((pHomeInfo->BattleZoneZ + myrand(0, pHomeInfo->BattleZoneLZ)) * 10));
				else if (pTUser->GetMap()->canAttackOtherNation())
					pTUser->Warp(uint16((pHomeInfo->FreeZoneX + myrand(0, pHomeInfo->FreeZoneLX)) * 10), uint16((pHomeInfo->FreeZoneZ + myrand(0, pHomeInfo->FreeZoneLZ)) * 10));
				else
					pTUser->Warp(uint16(pTUser->GetMap()->m_fInitX * 10), uint16(pTUser->GetMap()->m_fInitZ * 10));
				break;

			case 2:		// Send target to teleport point WITHIN the zone.
				// LATER!!!
				break;
			case 3:		// Send target to teleport point OUTSIDE the zone.
				// LATER!!!
				break;
			case 5:		// Send target to a hidden zone.
				// LATER!!!
				break;
			
			case 11:	// Resurrect a dead player.
			{
				// Send the packet to the target.
				sData[1] = 1;
				BuildAndSendSkillPacket(*itr, true, sCasterID, (*itr)->GetID(), bOpcode, nSkillID, sData); 

				pTUser->m_bResHpType = USER_STANDING;     // Target status is officially alive now.
				pTUser->HpChange(pTUser->m_iMaxHp, pSkillCaster);	 // Refill HP.	
				pTUser->ExpChange( pType->sExpRecover/100 );     // Increase target experience.
				
				Packet result(AG_USER_REGENE);
				result << uint16((*itr)->GetSocketID()) << uint16(pTUser->GetZoneID());
				g_pMain->Send_AIServer(&result);
			} break;

			case 12:	// Summon a target within the zone.	
				if (pSkillCaster->GetZoneID() != pTUser->GetZoneID())   // Same zone? 
					goto packet_send;

				// Send the packet to the target.
				sData[1] = 1;
				BuildAndSendSkillPacket(*itr, true, sCasterID, (*itr)->GetID(), bOpcode, nSkillID, sData); 
				
				pTUser->Warp(pSkillCaster->GetSPosX(), pSkillCaster->GetSPosZ());
				break;

			case 13:	// Summon a target outside the zone.			
				if (pSkillCaster->GetZoneID() == pTUser->GetZoneID())	  // Different zone? 
					goto packet_send;

				// Send the packet to the target.
				sData[1] = 1;
				BuildAndSendSkillPacket(*itr, true, sCasterID, (*itr)->GetID(), bOpcode, nSkillID, sData); 

				pTUser->ZoneChange(pSkillCaster->GetZoneID(), pSkillCaster->GetX(), pSkillCaster->GetZ()) ;
				pTUser->UserInOut(INOUT_RESPAWN);
				break;

			case 20:	// Teleport the source (radius) meters forward
			{
				// Calculate difference between where user is now and where they were previously
				// to figure out an orientation.
				// Should really use m_sDirection, but not sure what the value is exactly.
				float	warp_x = pTUser->GetX() - pTUser->m_oldx, 
						warp_z = pTUser->GetZ() - pTUser->m_oldz;

				// Unable to work out orientation, so we'll just fail (won't be necessary with m_sDirection).
				float	distance = sqrtf(warp_x*warp_x + warp_z*warp_z);
				if (distance == 0.0f)
					goto packet_send;

				warp_x /= distance; warp_z /= distance;
				warp_x *= pType->sRadius; warp_z *= pType->sRadius;
				warp_x += pTUser->m_oldx; warp_z += pTUser->m_oldz;

				sData[1] = 1;
				BuildAndSendSkillPacket(*itr, true, sCasterID, (*itr)->GetID(), bOpcode, nSkillID, sData); 
				pTUser->Warp(uint16(warp_x * 10), uint16(warp_z * 10));
			} break;

			case 21:	// Summon a monster within a zone.
				// LATER!!! 
				break;
			case 25:
				//This is used by Wild Advent (70 rogue skill) and Descent, teleport the user to the target user (Moral check to distinguish between the 2 skills)
				float dest_x, dest_z = 0.0f;
				if (pTUser->GetZoneID() != pSkillCaster->GetZoneID()) //If we're not even in the same zone, I can't teleport to you!
					return false;
				if (pSkill->bMoral < MORAL_ENEMY && pTUser->GetNation() != pSkillCaster->GetNation()) //I'm not the same nation as you are and thus can't t
					return false;
					
				dest_x = pTUser->GetX();
				dest_z = pTUser->GetZ();

				if (pSkillCaster->isPlayer())
					TO_USER(pSkillCaster)->Warp(uint16(dest_x * 10), uint16(dest_z * 10));
				break;
		}

		bResult = 1;
		
packet_send:
		// Send the packet to the caster.
		sData[1] = bResult;
		BuildAndSendSkillPacket(pSkillCaster, true, sCasterID, (*itr)->GetID(), bOpcode, nSkillID, sData); 
	}
	return true;
}

bool MagicInstance::ExecuteType9()
{
	if (pSkill == nullptr
		// Only players can use these skills.
		|| !pSkillCaster->isPlayer())
		return false;

	_MAGIC_TYPE9* pType = g_pMain->m_Magictype9Array.GetData(nSkillID);
	if (pType == nullptr)
		return false;

	sData[1] = 1;

	CUser * pCaster = TO_USER(pSkillCaster);
	FastGuard lock(pCaster->m_buffLock);
	Type9BuffMap & buffMap = pCaster->m_type9BuffMap;

	// Ensure this type of skill isn't already in use.
	if (buffMap.find(pType->bStateChange) != buffMap.end())
	{
		sData[1] = 0;
		SendSkillFailed();
		return false;
	}
	
	if (pType->bStateChange <= 2 
		&& pCaster->canStealth())
	{
		// Cannot stealth when already stealthed.
		// This prevents us from using both types 1 and 2 (i.e. dispel on move & dispel on attack)
		// at the same time.
		if (pCaster->m_bInvisibilityType != INVIS_NONE)
		{
			sData[1] = 0;
			SendSkillFailed();
			return false;
		}

		// Invisibility perk does NOT apply when using these skills on monsters.
		if (pSkillTarget->isPlayer())
		{
			pCaster->StateChangeServerDirect(7, pType->bStateChange); // Update the client to be invisible
			buffMap.insert(std::make_pair(pType->bStateChange, _BUFF_TYPE9_INFO(nSkillID, UNIXTIME + pType->sDuration)));
		}
	}
	else if (pType->bStateChange >= 3 && pType->bStateChange <= 4)
	{
		Packet stealth(WIZ_STEALTH, uint8(1));
		stealth << uint16(pType->sRadius);

		// If the player's in a party, apply this skill to all members of the party.
		if (pCaster->isInParty())
		{
			_PARTY_GROUP * pParty = g_pMain->m_PartyArray.GetData(pCaster->GetPartyID());
			if (pParty == nullptr)
				return false;

			for (int i = 0; i < MAX_PARTY_USERS; i++)
			{
				CUser *pUser = g_pMain->GetUserPtr(pParty->uid[i]);
				if (pUser == nullptr)
					continue;

				FastGuard buffLock(pUser->m_buffLock);

				// If this user already has this skill active, we don't need to reapply it.
				if (pUser->m_type9BuffMap.find(pType->bStateChange) 
					!= pUser->m_type9BuffMap.end())
					continue;

				pUser->m_type9BuffMap.insert(std::make_pair(pType->bStateChange, _BUFF_TYPE9_INFO(nSkillID, UNIXTIME + pType->sDuration)));
				pUser->Send(&stealth);
			}
		}
		else // not in a party, so just apply this skill to us.
		{
			buffMap.insert(std::make_pair(pType->bStateChange, _BUFF_TYPE9_INFO(nSkillID, UNIXTIME + pType->sDuration)));
			pCaster->Send(&stealth);
		}
	}

	Packet result;
	BuildSkillPacket(result, sCasterID, sTargetID, bOpcode, nSkillID, sData);
	if (pCaster->isInParty() && pType->bStateChange == 4)
		g_pMain->Send_PartyMember(pCaster->GetPartyID(), &result);
	else
		pCaster->Send(&result);

	return true;
}

short MagicInstance::GetMagicDamage(Unit *pTarget, int total_hit, int attribute)
{	
	short damage = 0, temp_hit = 0, righthand_damage = 0, attribute_damage = 0 ; 
	int random = 0, total_r = 0 ;
	uint8 result; 

	if (pTarget == nullptr
		|| pSkillCaster == nullptr
		|| pTarget->isDead()
		|| pSkillCaster->isDead())
		return 0;

	int16 sMagicAmount = 0;
	if (pSkillCaster->isNPC())
	{
		result = pSkillCaster->GetHitRate(pTarget->m_fTotalHitrate / pSkillCaster->m_fTotalEvasionrate); 
	}
	else
	{
		CUser *pUser = TO_USER(pSkillCaster);
		uint8 bCha = pUser->GetStat(STAT_CHA);
		if (bCha > 86)
			sMagicAmount = bCha - 86;

		sMagicAmount += pUser->m_sMagicAttackAmount;
		total_hit *= bCha / 186;
		result = SUCCESS;
	}
		
	if (result != FAIL) 
	{
		// In case of SUCCESS.... 
		switch (attribute)
		{
			case FIRE_R: 
				total_r = pTarget->m_sFireR + pTarget->m_bFireRAmount;
				break;
			case COLD_R :
				total_r = pTarget->m_sColdR + pTarget->m_bColdRAmount;
				break;
			case LIGHTNING_R :
				total_r = pTarget->m_sLightningR + pTarget->m_bLightningRAmount; 
				break;
			case MAGIC_R :
				total_r = pTarget->m_sMagicR + pTarget->m_bMagicRAmount;
				break;
			case DISEASE_R :
				total_r = pTarget->m_sDiseaseR + pTarget->m_bDiseaseRAmount;
				break;
			case POISON_R :			
				total_r = pTarget->m_sPoisonR + pTarget->m_bPoisonRAmount;
				break;
		}
		
		if (pSkillCaster->isPlayer()) 
		{
			CUser *pUser = TO_USER(pSkillCaster);

			// double the staff's damage when using a skill of the same attribute as the staff
			_ITEM_TABLE *pRightHand = pUser->GetItemPrototype(RIGHTHAND);
			if (pRightHand != nullptr && pRightHand->isStaff()
				&& pUser->GetItemPrototype(LEFTHAND) == nullptr)
			{
				FastGuard lock(pSkillCaster->m_equippedItemBonusLock);
				righthand_damage = pRightHand->m_sDamage;
				auto itr = pSkillCaster->m_equippedItemBonuses.find(RIGHTHAND);
				if (itr != pSkillCaster->m_equippedItemBonuses.end())
				{
					auto bonusItr = itr->second.find(attribute);
					if (bonusItr != itr->second.end()) 
						attribute_damage = pRightHand->m_sDamage; 
				}
			}
			else 
			{
				righthand_damage = 0;
			}

			// Add on any elemental skill damage
			FastGuard lock(pSkillCaster->m_equippedItemBonusLock);
			foreach (itr, pSkillCaster->m_equippedItemBonuses)
			{
				uint8 bSlot = itr->first;
				foreach (bonusItr, itr->second)
				{
					uint8 bType = bonusItr->first; 
					int16 sAmount = bonusItr->second;
					int16 sTempResist = 0;

					switch (bType)
					{
						case ITEM_TYPE_FIRE :	// Fire Damage
							sTempResist = pTarget->m_sFireR + pTarget->m_bFireRAmount;
							break;
						case ITEM_TYPE_COLD :	// Ice Damage
							sTempResist = pTarget->m_sColdR + pTarget->m_bColdRAmount;
							break;
						case ITEM_TYPE_LIGHTNING :	// Lightning Damage
							sTempResist = pTarget->m_sLightningR + pTarget->m_bLightningRAmount;
							break;
						case ITEM_TYPE_POISON :	// Poison Damage
							sTempResist = pTarget->m_sPoisonR + pTarget->m_bPoisonRAmount;
							break;
					}

					sTempResist += pTarget->m_bResistanceBonus;
					if (bType >= ITEM_TYPE_FIRE 
						&& bType <= ITEM_TYPE_POISON)
					{
						if (sTempResist > 200) 
							sTempResist = 200;

						// add attribute damage amount to right-hand damage instead of attribute
						// so it doesn't bother taking into account caster level (as it would with the staff attributes).
						righthand_damage += sAmount - sAmount * sTempResist / 200;
					}
				}
			}

		}

		damage = (short)(230 * total_hit / (total_r + 250));
		random = myrand(0, damage);
		damage = (short)(random * 0.3f + (damage * 0.85f)) - sMagicAmount;

		if (pSkillCaster->isNPC())
			damage -= ((3 * righthand_damage) + (3 * attribute_damage));
		else if (attribute != MAGIC_R)	// Only if the staff has an attribute.
			damage -= (short)(((righthand_damage * 0.8f) + (righthand_damage * pSkillCaster->GetLevel()) / 60) + ((attribute_damage * 0.8f) + (attribute_damage * pSkillCaster->GetLevel()) / 30));
		if(pTarget->m_bMagicDamageReduction < 100)
			damage = damage * pTarget->m_bMagicDamageReduction / 100;
	}

	// Apply boost for skills matching weather type.
	// This isn't actually used officially, but I think it's neat...
	GetWeatherDamage(damage, attribute);
	return damage / 3;		
}

short MagicInstance::GetWeatherDamage(short damage, int attribute)
{
	// Give a 10% damage output boost based on weather (and skill's elemental attribute)
	if ((g_pMain->m_byWeather == WEATHER_FINE && attribute == ATTRIBUTE_FIRE)
		|| (g_pMain->m_byWeather == WEATHER_RAIN && attribute == ATTRIBUTE_LIGHTNING)
		|| (g_pMain->m_byWeather == WEATHER_SNOW && attribute == ATTRIBUTE_ICE))
		damage = (damage * 110) / 100;

	return damage;
}

void MagicInstance::Type6Cancel(bool bForceRemoval /*= false*/)
{
	// NPCs cannot transform.
	if (!pSkillCaster->isPlayer()
		// Are we transformed? Note: if we're relogging, and we need to remove it, we should ignore this check.
		|| (!bForceRemoval && !pSkillCaster->isTransformed()))
		return;

	CUser * pUser = TO_USER(pSkillCaster);
	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_CANCEL_TYPE6));

	// TO-DO: Reset stat changes, recalculate stats.
	pUser->m_bIsTransformed = false;
	pUser->Send(&result);

	pUser->RemoveSavedMagic(pUser->m_bAbnormalType);
	pUser->StateChangeServerDirect(3, ABNORMAL_NORMAL);
}

void MagicInstance::Type9Cancel(bool bRemoveFromMap /*= true*/)
{
	if (pSkillCaster == nullptr
		|| !pSkillCaster->isPlayer())
		return;

	_MAGIC_TYPE9 * pType = g_pMain->m_Magictype9Array.GetData(nSkillID);
	if (pType == nullptr)
		return;

	uint8 bResponse = 0;
	CUser * pCaster = TO_USER(pSkillCaster);
	FastGuard lock(pCaster->m_buffLock);
	Type9BuffMap & buffMap = pCaster->m_type9BuffMap;

	// If this skill isn't already applied, there's no reason to remove it.
	if (buffMap.find(pType->bStateChange) == buffMap.end())
		return;
	
	// Remove the buff from the map
	if (bRemoveFromMap)
		buffMap.erase(pType->bStateChange);

	// Stealths
	if (pType->bStateChange <= 2 
		|| (pType->bStateChange >= 5 && pType->bStateChange < 7))
	{
		pCaster->StateChangeServerDirect(7, INVIS_NONE);
		bResponse = 91;
	}
	// Lupine etc.
	else if (pType->bStateChange >= 3 && pType->bStateChange <= 4) 
	{
		pCaster->InitializeStealth();
		bResponse = 92;
	}
	// Guardian pet related
	else if (pType->bStateChange == 7)
	{
		Packet pet(WIZ_PET, uint8(1));
		pet << uint16(1) << uint16(6);
		pCaster->Send(&pet);
		bResponse = 93;
	}

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_END));
	result << bResponse;
	pCaster->Send(&result);
}

void MagicInstance::Type4Cancel()
{
	if (pSkill == nullptr
		|| pSkillTarget != pSkillCaster)
		return;

	_MAGIC_TYPE4* pType = g_pMain->m_Magictype4Array.GetData(nSkillID);
	if (pType == nullptr)
		return;

	if (!CMagicProcess::RemoveType4Buff(pType->bBuffType, TO_USER(pSkillCaster)))
		return;

	if (pSkillCaster->isPlayer())
	{
		FastGuard bufffLock(pSkillCaster->m_buffLock);
		if (pSkillCaster->m_buffMap.empty()
			&& TO_USER(pSkillCaster)->isInParty())
			TO_USER(pSkillCaster)->SendPartyStatusUpdate(2);
	}

	TO_USER(pSkillCaster)->RemoveSavedMagic(nSkillID);
}

void MagicInstance::Type3Cancel()
{
	if (pSkill == nullptr
		|| pSkillCaster != pSkillTarget)
		return;

	// Should this take only the specified skill? I'm thinking so.
	_MAGIC_TYPE3* pType = g_pMain->m_Magictype3Array.GetData(nSkillID);
	if (pType == nullptr)
		return;

	for (int i = 0; i < MAX_TYPE3_REPEAT; i++)
	{
		Unit::MagicType3 * pEffect = &pSkillCaster->m_durationalSkills[i];
		if (!pEffect->m_byUsed
			// we can only cancel healing-over-time skills
			// damage-over-time skills must remain.
			|| pEffect->m_sHPAmount <= 0)
			continue;

		pEffect->Reset();
		break;	// we can only have one healing-over-time skill active.
				// since we've found it, no need to loop through the rest of the array.
	}

	if (pSkillCaster->isPlayer())
	{
		Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE3_END));
		result << uint8(100); // remove the healing-over-time skill.
		TO_USER(pSkillCaster)->Send(&result); 
	}

	int buff_test = 0;
	for (int j = 0; j < MAX_TYPE3_REPEAT; j++)
	{
		if (pSkillCaster->m_durationalSkills[j].m_byUsed)
			buff_test++;
	}
	if (buff_test == 0) pSkillCaster->m_bType3Flag = false;	

	if (pSkillCaster->isPlayer() && !pSkillCaster->m_bType3Flag
		&& TO_USER(pSkillCaster)->isInParty())
		TO_USER(pSkillCaster)->SendPartyStatusUpdate(1, 0);
}

void MagicInstance::Type4Extend()
{
	if (pSkill == nullptr)
		return;

	_MAGIC_TYPE4 *pType = g_pMain->m_Magictype4Array.GetData(nSkillID);
	if (pType == nullptr)
		return;

	FastGuard lock(pSkillTarget->m_buffLock);
	Type4BuffMap::iterator itr = pSkillTarget->m_buffMap.find(pType->bBuffType);

	// Can't extend a buff that hasn't been started.
	if (itr == pSkillTarget->m_buffMap.end()
		// Can't extend a buff that's already been extended.
		|| itr->second.m_bDurationExtended
		// Only players can extend buffs.
		|| !pSkillCaster->isPlayer() 
		// Require the "Duration Item" for buff duration extension.
		|| !TO_USER(pSkillTarget)->RobItem(800022000, 1))
		return;

	// Extend the duration of a buff.
	itr->second.m_tEndTime += pType->sDuration;

	// Mark the buff as extended (we cannot extend it more than once).
	itr->second.m_bDurationExtended = true;

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_EXTEND));
	result << uint32(nSkillID);
	TO_USER(pSkillTarget)->Send(&result);
}

void MagicInstance::ReflectDamage(int32 damage)
{
	if(damage < 0)
		damage *= -1;

	int16 total_resistance_caster = 0;
	int32 reflect_damage = 0;

	switch(pSkillTarget->m_bReflectArmorType)
	{
		case FIRE_DAMAGE:
			total_resistance_caster = pSkillCaster->m_sFireR + pSkillCaster->m_bFireRAmount;
			reflect_damage = ((230 * damage) / (total_resistance_caster + 250)) / 100 * 25;
			pSkillCaster->HpChange(-damage, pSkillTarget);
		break;
		
		case ICE_DAMAGE:
			total_resistance_caster = pSkillCaster->m_sColdR + pSkillCaster->m_bColdRAmount;
			reflect_damage = ((230 * damage) / (total_resistance_caster + 250)) / 100 * 25;
			pSkillCaster->HpChange(-damage, pSkillTarget);
		break;

		case LIGHTNING_DAMAGE:
			total_resistance_caster = pSkillCaster->m_sLightningR + pSkillCaster->m_bLightningRAmount;
			reflect_damage = ((230 * damage) / (total_resistance_caster + 250)) / 100 * 25;
			pSkillCaster->HpChange(-damage, pSkillTarget);
		break;
	}
}