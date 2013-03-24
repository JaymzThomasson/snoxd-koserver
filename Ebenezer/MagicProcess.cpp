#include "stdafx.h"

using namespace std;

#define MORAL_SELF				1
#define MORAL_FRIEND_WITHME		2
#define MORAL_FRIEND_EXCEPTME	3
#define MORAL_PARTY				4
#define MORAL_NPC				5
#define MORAL_PARTY_ALL			6
#define MORAL_ENEMY				7
#define MORAL_ALL				8
#define MORAL_AREA_ENEMY		10
#define MORAL_AREA_FRIEND		11
#define MORAL_AREA_ALL			12
#define MORAL_SELF_AREA			13
#define MORAL_CLAN				14
#define MORAL_CLAN_ALL			15

#define MORAL_UNDEAD			16		// Undead Monster
#define MORAL_PET_WITHME		17      // My Pet
#define MORAL_PET_ENEMY			18		// Enemy's Pet
#define MORAL_ANIMAL1			19		// Animal #1
#define MORAL_ANIMAL2			20		// Animal #2
#define MORAL_ANIMAL3			21		// Animal #3
#define MORAL_ANGEL				22		// Angel
#define MORAL_DRAGON			23		// Dragon
#define MORAL_CORPSE_FRIEND     25		// A Corpse of the same race.
#define MORAL_CORPSE_ENEMY      26		// A Corpse of a different race.

#define WARP_RESURRECTION		1		// To the resurrection point.

#define REMOVE_TYPE3			1
#define REMOVE_TYPE4			2
#define RESURRECTION			3
#define	RESURRECTION_SELF		4
#define REMOVE_BLESS			5

CMagicProcess::CMagicProcess()
{
	m_pSrcUser = NULL;
}

CMagicProcess::~CMagicProcess()
{
}

void CMagicProcess::MagicPacket(Packet & pkt, bool isRecastingSavedMagic)
{
	MagicInstance instance;
	pkt >> instance.bOpcode >> instance.nSkillID;

	_MAGIC_TABLE * pMagic = instance.pSkill = g_pMain.m_MagictableArray.GetData(instance.nSkillID);
	if (instance.pSkill == NULL)
		return;

	pkt >> instance.sCasterID >> instance.sTargetID
		>> instance.sData1 >> instance.sData2 >> instance.sData3 >> instance.sData4
		>> instance.sData5 >> instance.sData6 >> instance.sData7 >> instance.sData8;

	instance.pSkillCaster = g_pMain.GetUnit(instance.sCasterID);
	instance.pSkillTarget = g_pMain.GetUnit(instance.sTargetID);

	instance.bIsRecastingSavedMagic = isRecastingSavedMagic;

	HandleMagic(&instance);
}

void CMagicProcess::HandleMagic(MagicInstance * pInstance)
{
	if (pInstance->pSkill == NULL)
		pInstance->pSkill = g_pMain.m_MagictableArray.GetData(pInstance->nSkillID);

	if (pInstance->pSkill == NULL
		|| pInstance->pSkillCaster == NULL
		|| (m_pSrcUser && !UserCanCast(pInstance)))
	{
		SendSkillFailed(pInstance);
		return;
	}

	// If the target is a mob/NPC *or* we're casting an AOE, tell the AI to handle it.
	if (pInstance->sTargetID >= NPC_BAND
		|| (pInstance->sTargetID == -1 && 
			(pInstance->pSkill->bMoral == MORAL_AREA_ENEMY 
				|| pInstance->pSkill->bMoral == MORAL_AREA_ALL 
				|| pInstance->pSkill->bMoral == MORAL_SELF_AREA)))
	{
		SendSkillToAI(pInstance);

		// If the target is specifically a mob, stop here. AI's got this one.
		// Otherwise, it's an AOE -- which means it might affect players too, so we need to handle it too.
		if (pInstance->sTargetID >= NPC_BAND)
			return;
	}

	bool bInitialResult;
	switch (pInstance->bOpcode)
	{
		case MAGIC_CASTING:
			SendSkill(pInstance);
			break;

		case MAGIC_EFFECTING:
			// Hacky check for a transformation item (Disguise Totem, Disguise Scroll)
			// These apply when first type's set to 0, second type's set and obviously, there's a consumable item.
			// Need to find a better way of handling this.
			if (pInstance->pSkill->bType[0] == 0 && pInstance->pSkill->bType[1] != 0
				&& pInstance->pSkill->iUseItem != 0
				&& (m_pSrcUser != NULL 
					&& m_pSrcUser->CheckExistItem(pInstance->pSkill->iUseItem, 1)))
			{
				SendTransformationList(pInstance);
				return;
			}

			bInitialResult = ExecuteSkill(pInstance, pInstance->pSkill->bType[0]);

			// NOTE: Some ROFD skills require a THIRD type.
			if (bInitialResult)
				ExecuteSkill(pInstance, pInstance->pSkill->bType[1]);
			break;
		case MAGIC_FLYING:
		case MAGIC_FAIL:
			SendSkill(pInstance);
			break;
		case MAGIC_TYPE3_END: //This is also MAGIC_TYPE4_END
			break;
		case MAGIC_CANCEL:
		case MAGIC_CANCEL2:
			Type3Cancel(pInstance);	//Damage over Time skills.
			Type4Cancel(pInstance);	//Buffs
			Type6Cancel();	//Transformations
			Type9Cancel(pInstance);	//Stealth, lupine etc.
			break;
		case MAGIC_TYPE4_EXTEND:
			Type4Extend(pInstance);
			break;
	}
}

bool CMagicProcess::UserCanCast(MagicInstance * pInstance)
{
	if (pInstance->pSkill == NULL)
		return false;

	// We don't want the source ever being anyone other than us.
	// Ever. Even in the case of NPCs, it's BAD. BAD!
	// We're better than that -- we don't need to have the client tell NPCs what to do.
	if (m_pSrcUser != NULL && pInstance->pSkillCaster != m_pSrcUser) 
		return false;

	// We don't need to check anything as we're just canceling our buffs.
	if (pInstance->bOpcode == MAGIC_CANCEL || pInstance->bOpcode == MAGIC_CANCEL2) 
		return true;

	if (pInstance->bIsRecastingSavedMagic)
		return true;

	// Users who are blinking cannot use skills.
	// Additionally, unless it's resurrection-related, dead players cannot use skills.
	if (m_pSrcUser->isBlinking()
		|| (m_pSrcUser->isDead() && pInstance->pSkill->bType[0] != 5)) 
		return false;

	// If we're using an AOE, and the target is specified... something's not right.
	if ((pInstance->pSkill->bMoral >= MORAL_AREA_ENEMY
			&& pInstance->pSkill->bMoral <= MORAL_SELF_AREA)
		&& pInstance->sTargetID != -1)
		return false;

	if (pInstance->pSkill->sSkill != 0
		&& (m_pSrcUser->m_sClass != (pInstance->pSkill->sSkill / 10)
			|| m_pSrcUser->m_bLevel < pInstance->pSkill->sSkillLevel))
		return false;

	if ((m_pSrcUser->m_sMp - pInstance->pSkill->sMsp) < 0)
		return false;

	// If we're in a snow war, we're only ever allowed to use the snowball skill.
	if (m_pSrcUser->GetZoneID() == ZONE_SNOW_BATTLE && g_pMain.m_byBattleOpen == SNOW_BATTLE 
		&& pInstance->nSkillID != SNOW_EVENT_SKILL)
		return false;

	if (pInstance->pSkillTarget != NULL)
	{
		// Players require a little more rigorous checking
		if (pInstance->pSkillTarget->isPlayer())
		{
			if (pInstance->pSkillTarget != m_pSrcUser)
			{
				if (pInstance->pSkillTarget->GetZoneID() != pInstance->pSkillCaster->GetZoneID()
					|| !TO_USER(pInstance->pSkillTarget)->isAttackZone()
					// Will have to add support for the outside battlefield
					|| (TO_USER(pInstance->pSkillTarget)->isAttackZone() 
						&& pInstance->pSkillTarget->GetNation() == pInstance->pSkillCaster->GetNation()))
					return false;
			}
		}
		// NPCs are simp-uhl.
		else 
		{
			if (pInstance->pSkillTarget->GetZoneID() != m_pSrcUser->GetZoneID()
				|| pInstance->pSkillTarget->GetNation() == m_pSrcUser->GetNation())
				return false;
		}
	}

	if ((pInstance->pSkill->bType[0] != 2 //Archer skills will handle item checking in ExecuteType2()
		&& pInstance->pSkill->bType[0] != 6) //So will transformations
		&& (pInstance->pSkill->iUseItem != 0
		&& !m_pSrcUser->CanUseItem(pInstance->pSkill->iUseItem, 1))) //The user does not meet the item's requirements or does not have any of said item.
		return false;

	if ((pInstance->bOpcode == MAGIC_EFFECTING || pInstance->bOpcode == MAGIC_CASTING) 
		&& !IsAvailable(pInstance))
		return false;

	//TO-DO : Add skill cooldown checks.

	//Incase we made it to here, we can cast! Hurray!
	return true;
}

void CMagicProcess::SendSkillToAI(MagicInstance * pInstance)
{
	if (pInstance->pSkill == NULL)
		return;

	if (pInstance->sTargetID >= NPC_BAND 
		|| (pInstance->sTargetID == -1 && (pInstance->pSkill->bMoral == MORAL_AREA_ENEMY || pInstance->pSkill->bMoral == MORAL_SELF_AREA)))
	{		
		int total_magic_damage = 0;

		Packet result(AG_MAGIC_ATTACK_REQ); // this is the order it was in.
		result	<< pInstance->sCasterID << pInstance->bOpcode << pInstance->sTargetID << pInstance->nSkillID 
				<< pInstance->sData1 << pInstance->sData2 << pInstance->sData3 
				<< pInstance->sData4 << pInstance->sData5 << pInstance->sData6
				<< m_pSrcUser->getStatWithItemBonus(STAT_CHA);


		_ITEM_TABLE* pRightHand = m_pSrcUser->GetItemPrototype(RIGHTHAND);
		if (pRightHand != NULL && pRightHand->isStaff()
			&& m_pSrcUser->GetItemPrototype(LEFTHAND) == NULL)
		{
			if (pInstance->pSkill->bType[0] == 3) {
				total_magic_damage += (int)((pRightHand->m_sDamage * 0.8f)+ (pRightHand->m_sDamage * m_pSrcUser->GetLevel()) / 60);

				_MAGIC_TYPE3 *pType3 = g_pMain.m_Magictype3Array.GetData(pInstance->nSkillID);
				if (pType3 == NULL)
					return;
				if (m_pSrcUser->m_bMagicTypeRightHand == pType3->bAttribute )					
					total_magic_damage += (int)((pRightHand->m_sDamage * 0.8f) + (pRightHand->m_sDamage * m_pSrcUser->GetLevel()) / 30);
				
				//TO-DO : Divide damage by 3 if duration = 0

				if (pType3->bAttribute == 4)
					total_magic_damage = 0;
			}
		}
		result << uint16(total_magic_damage);
		g_pMain.Send_AIServer(&result);		
	}
}

bool CMagicProcess::ExecuteSkill(MagicInstance * pInstance, uint8 bType)
{
	switch (bType)
	{
		case 1: return ExecuteType1(pInstance);
		case 2: return ExecuteType2(pInstance);
		case 3: return ExecuteType3(pInstance);
		case 4: return ExecuteType4(pInstance);
		case 5: return ExecuteType5(pInstance);
		case 6: return ExecuteType6(pInstance);
		case 7: return ExecuteType7(pInstance);
		case 8: return ExecuteType8(pInstance);
		case 9: return ExecuteType9(pInstance);
	}

	return false;
}

void CMagicProcess::SendTransformationList(MagicInstance * pInstance)
{
	if (m_pSrcUser == NULL)
		return;

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TRANSFORM_LIST));
	result << pInstance->nSkillID;
	m_pSrcUser->m_nTransformationItem = pInstance->pSkill->iUseItem;
	m_pSrcUser->Send(&result);
}

void CMagicProcess::SendSkillFailed(MagicInstance * pInstance)
{
	SendSkill(pInstance, pInstance->sCasterID, pInstance->sTargetID, MAGIC_FAIL, 
				pInstance->nSkillID, pInstance->sData1, pInstance->sData2, pInstance->sData3, pInstance->sData4, 
				pInstance->sData5, pInstance->sData6, pInstance->sData7, pInstance->sData8);
}

void CMagicProcess::SendSkill(MagicInstance * pInstance, int16 pSkillCaster /* = -1 */, int16 pSkillTarget /* = -1 */,	
								int8 opcode /* = -1 */, uint32 nSkillID /* = 0 */, 
								int16 sData1 /* = -999 */, int16 sData2 /* = -999 */, int16 sData3 /* = -999 */, int16 sData4 /* = -999 */, 
								int16 sData5 /* = -999 */, int16 sData6 /* = -999 */, int16 sData7 /* = -999 */, int16 sData8 /* = -999 */)
{
	Packet result(WIZ_MAGIC_PROCESS);
	int16 sid = 0, tid = 0;

	// Yes, these are all default value overrides.
	// This is completely horrible, but will suffice for now...

	if (opcode == -1)
		opcode = pInstance->bOpcode;

	if (opcode == MAGIC_FAIL
		&& pSkillCaster >= NPC_BAND)
		return;

	if (pSkillCaster  == -1)
		pSkillCaster = pInstance->sCasterID;

	if (pSkillTarget  == -1)
		pSkillTarget = pInstance->sTargetID;

	if (nSkillID == 0)
		nSkillID = pInstance->nSkillID;

	if (sData1 == -999)
		sData1 = pInstance->sData1;
	if (sData2 == -999)
		sData2 = pInstance->sData2;
	if (sData3 == -999)
		sData3 = pInstance->sData3;
	if (sData4 == -999)
		sData4 = pInstance->sData4;
	if (sData5 == -999)
		sData5 = pInstance->sData5;
	if (sData6 == -999)
		sData6 = pInstance->sData6;
	if (sData7 == -999)
		sData7 = pInstance->sData7;
	if (sData8 == -999)
		sData8 = pInstance->sData8;

	result	<< opcode << nSkillID << pSkillCaster << pSkillTarget 
			<< sData1 << sData2 << sData3 << sData4
			<< sData5 << sData6 << sData7 << sData8;

	if (m_pSrcUser == NULL)
	{
		CNpc *pNpc = g_pMain.m_arNpcArray.GetData(pSkillCaster);
		if (pNpc == NULL)
			return;

		pNpc->SendToRegion(&result);
	}
	else
	{
		CUser *pUser = NULL;
		if ((pSkillCaster < 0 || pSkillCaster >= MAX_USER)
			&& (pSkillTarget >= 0 && pSkillTarget < MAX_USER))
			pUser = g_pMain.GetUserPtr(pSkillTarget);
		else
			pUser = m_pSrcUser;

		if (pUser == NULL)
			return;

		if (opcode == MAGIC_FAIL)
			m_pSrcUser->Send(&result);
		else
			pUser->SendToRegion(&result);
	}
}

bool CMagicProcess::IsAvailable(MagicInstance * pInstance)
{
	CUser* pParty = NULL;   // When the target is a party....
	bool isNPC = (pInstance->sCasterID >= NPC_BAND);		// Identifies source : TRUE means source is NPC.

	if (pInstance->pSkill == NULL)
		goto fail_return;

	int modulator = 0, Class = 0, moral = 0, skill_mod = 0 ;

	if (pInstance->sTargetID >= 0 && pInstance->sTargetID < MAX_USER) 
		moral = pInstance->pSkillTarget->GetNation();
	else if (pInstance->sTargetID >= NPC_BAND)     // Target existence check routine for NPC.          	
	{
		if (pInstance->pSkillTarget == NULL || pInstance->pSkillTarget->isDead())
			goto fail_return;	//... Assuming NPCs can't be resurrected.

		moral = pInstance->pSkillTarget->GetNation();
	}
	else if (pInstance->sTargetID == -1)  // AOE/Party Moral check routine.
	{
		if (isNPC)
		{
			moral = 1;
		}
		else
		{
			if (pInstance->pSkill->bMoral == MORAL_AREA_ENEMY)
				moral = pInstance->pSkillCaster->GetNation() == KARUS ? ELMORAD : KARUS;
			else 
				moral = pInstance->pSkillCaster->GetNation();	
		}
	}
	else 
		moral = m_pSrcUser->GetNation();

	switch( pInstance->pSkill->bMoral ) {		// Compare morals between source and target character.
		case MORAL_SELF:   // #1         // ( to see if spell is cast on the right target or not )
			if (pInstance->pSkillCaster != pInstance->pSkillTarget)
				goto fail_return;
			break;
		case MORAL_FRIEND_WITHME:	// #2
			if (pInstance->pSkillCaster->GetNation() != moral)
				goto fail_return;
			break;
		case MORAL_FRIEND_EXCEPTME:	   // #3
			if (pInstance->pSkillCaster->GetNation() != moral
				|| pInstance->pSkillCaster == pInstance->pSkillTarget)
				goto fail_return;
			break;
		case MORAL_PARTY:	 // #4
		{
			// NPCs can't *be* in parties.
			if (pInstance->pSkillCaster->isNPC()
				|| (pInstance->pSkillTarget != NULL && pInstance->pSkillTarget->isNPC()))
				goto fail_return;

			// We're definitely a user, so...
			CUser *pCaster = TO_USER(pInstance->pSkillCaster);

			// If the caster's not in a party, make sure the target's not someone other than themselves.
			if ((!pCaster->isInParty() && pInstance->pSkillCaster != pInstance->pSkillTarget)
				// Verify that the nation matches the intended moral
				|| pCaster->GetNation() != moral
				// and that if there is a target, they're in the same party.
				|| (pInstance->pSkillTarget != NULL && 
					TO_USER(pInstance->pSkillTarget)->m_sPartyIndex != pCaster->m_sPartyIndex))
				goto fail_return;
		} break;
		case MORAL_NPC:		// #5
			if (pInstance->pSkillTarget == NULL
				|| !pInstance->pSkillTarget->isNPC()
				|| pInstance->pSkillTarget->GetNation() != moral)
				goto fail_return;
			break;
		case MORAL_PARTY_ALL:     // #6
//			if ( !m_pSrcUser->isInParty() ) goto fail_return;		
//			if ( !m_pSrcUser->isInParty() && sid != tid) goto fail_return;					

			break;
		case MORAL_ENEMY:	// #7
			if (pInstance->pSkillCaster->GetNation() == moral)
				goto fail_return;
			break;	
		case MORAL_ALL:	 // #8
			// N/A
			break;
		case MORAL_AREA_ENEMY:		// #10
			// N/A
			break;
		case MORAL_AREA_FRIEND:		// #11
			if (pInstance->pSkillCaster->GetNation() != moral)
				goto fail_return;
			break;
		case MORAL_AREA_ALL:	// #12
			// N/A
			break;
		case MORAL_SELF_AREA:     // #13
			// Remeber, EVERYONE in the area is affected by this one. No moral check!!!
			break;
		case MORAL_CORPSE_FRIEND:		// #25
			if (pInstance->pSkillCaster->GetNation() != moral
				// We need to revive *something*.
				|| pInstance->pSkillTarget == NULL
				// We cannot revive ourselves.
				|| pInstance->pSkillCaster == pInstance->pSkillTarget
				// We can't revive living targets.
				|| pInstance->pSkillTarget->isAlive())
				goto fail_return;
			break;
		case MORAL_CLAN:		// #14
		{
			// NPCs cannot be in clans.
			if (pInstance->pSkillCaster->isNPC()
				|| (pInstance->pSkillTarget != NULL && pInstance->pSkillTarget->isNPC()))
				goto fail_return;

			// We're definitely a user, so....
			CUser * pCaster = TO_USER(pInstance->pSkillCaster);

			// If the caster's not in a clan, make sure the target's not someone other than themselves.
			if ((!pCaster->isInClan() && pInstance->pSkillCaster != pInstance->pSkillTarget)
				// Verify the intended moral
				|| pCaster->GetNation() != moral
				// If we're targeting someone, that target must be in our clan.
				|| (pInstance->pSkillTarget != NULL 
					&& TO_USER(pInstance->pSkillTarget)->GetClanID() != pCaster->GetClanID()))
				goto fail_return;
		} break;

		case MORAL_CLAN_ALL:	// #15
			break;
//
	}

	// This only applies to users casting skills. NPCs are fine and dandy (we can trust THEM at least).
	if (m_pSrcUser != NULL)
	{
		modulator = pInstance->pSkill->sSkill % 10;     // Hacking prevention!
		if( modulator != 0 ) {	
			Class = pInstance->pSkill->sSkill / 10;
			if( Class != m_pSrcUser->m_sClass ) goto fail_return;
			if( pInstance->pSkill->sSkillLevel > m_pSrcUser->m_bstrSkill[modulator] ) goto fail_return;
		}
		else if (pInstance->pSkill->sSkillLevel > m_pSrcUser->GetLevel()) goto fail_return;

		if (pInstance->pSkill->bType[0] == 1) {	// Weapons verification in case of COMBO attack (another hacking prevention).
			if (pInstance->pSkill->sSkill == 1055 || pInstance->pSkill->sSkill == 2055) {		// Weapons verification in case of dual wielding attacks !		
				_ITEM_TABLE *pLeftHand = m_pSrcUser->GetItemPrototype(LEFTHAND),
							*pRightHand = m_pSrcUser->GetItemPrototype(RIGHTHAND);

				if ((pLeftHand != NULL && !pLeftHand->isSword() && !pLeftHand->isAxe() && !pLeftHand->isMace())
					|| (pRightHand != NULL && !pRightHand->isSword() && !pRightHand->isAxe() && !pRightHand->isMace()))
					return false;
			}
			else if (pInstance->pSkill->sSkill == 1056 || pInstance->pSkill->sSkill == 2056) {	// Weapons verification in case of 2 handed attacks !
				_ITEM_TABLE	*pRightHand = m_pSrcUser->GetItemPrototype(RIGHTHAND);

				if (m_pSrcUser->GetItem(LEFTHAND)->nNum != 0
					|| (pRightHand != NULL 
						&& !pRightHand->isSword() && !pRightHand->isAxe() 
						&& !pRightHand->isMace() && !pRightHand->isSpear()))
					return false;
			}
		}

		if (pInstance->bOpcode == MAGIC_EFFECTING) {    // MP/SP SUBTRACTION ROUTINE!!! ITEM AND HP TOO!!!	
			int total_hit = m_pSrcUser->m_sTotalHit ;
			int skill_mana = (pInstance->pSkill->sMsp * total_hit) / 100 ; 

			if (pInstance->pSkill->bType[0] == 2 && pInstance->pSkill->bFlyingEffect != 0) // Type 2 related...
				return true;		// Do not reduce MP/SP when flying effect is not 0.

			if (pInstance->pSkill->bType[0] == 1 && pInstance->sData1 > 1)
				return true;		// Do not reduce MP/SP when combo number is higher than 0.
 
			if (pInstance->pSkill->bType[0] == 1 || pInstance->pSkill->bType[0] == 2)
			{
				if (skill_mana > m_pSrcUser->m_sMp)
					goto fail_return;
			}
			else if (pInstance->pSkill->sMsp > m_pSrcUser->m_sMp)
				goto fail_return;

			if (pInstance->pSkill->bType[0] == 3 || pInstance->pSkill->bType[0] == 4) {   // If the PLAYER uses an item to cast a spell.
				if (pInstance->sCasterID >= 0 && pInstance->sCasterID < MAX_USER)
				{	
					if (pInstance->pSkill->iUseItem != 0) {
						_ITEM_TABLE* pItem = NULL;				// This checks if such an item exists.
						pItem = g_pMain.GetItemPtr(pInstance->pSkill->iUseItem);
						if( !pItem ) return false;

						if ((pItem->m_bRace != 0 && m_pSrcUser->m_bRace != pItem->m_bRace)
							|| (pItem->m_bClass != 0 && !m_pSrcUser->JobGroupCheck(pItem->m_bClass))
							|| (pItem->m_bReqLevel != 0 && m_pSrcUser->GetLevel() < pItem->m_bReqLevel)
							|| (!m_pSrcUser->RobItem(pInstance->pSkill->iUseItem, 1)))	
							return false;
					}
				}
			}
			if (pInstance->pSkill->bType[0] == 1 || pInstance->pSkill->bType[0] == 2)	// Actual deduction of Skill or Magic point.
				m_pSrcUser->MSpChange(-(skill_mana));
			else if (pInstance->pSkill->bType[0] != 4 || (pInstance->pSkill->bType[0] == 4 && pInstance->sTargetID == -1))
				m_pSrcUser->MSpChange(-(pInstance->pSkill->sMsp));

			if (pInstance->pSkill->sHP > 0 && pInstance->pSkill->sMsp == 0) {			// DEDUCTION OF HPs in Magic/Skill using HPs.
				if (pInstance->pSkill->sHP > m_pSrcUser->m_sHp) goto fail_return;
				m_pSrcUser->HpChange(-pInstance->pSkill->sHP);
			}
		}
	}

	return true;      // Magic was successful! 

fail_return:    // In case of failure, send a packet(!)
	if (!isNPC)
		SendSkillFailed(pInstance);

	return false;     // Magic was a failure!
}

bool CMagicProcess::ExecuteType1(MagicInstance * pInstance)
{	
	if (pInstance->pSkill == NULL)
		return false;

	int damage = 0;
	bool bResult = false;

	_MAGIC_TYPE1* pType = g_pMain.m_Magictype1Array.GetData(pInstance->nSkillID);
	if (pType == NULL)
		return false;

	if (pInstance->pSkillTarget != NULL && !pInstance->pSkillTarget->isDead())
	{
		bResult = 1;
		damage = pInstance->pSkillCaster->GetDamage(pInstance->pSkillTarget, pInstance->pSkill);
		pInstance->pSkillTarget->HpChange(-damage, pInstance->pSkillCaster);

		// This is more than a little ugly.
		if (pInstance->pSkillCaster->isPlayer())
			TO_USER(pInstance->pSkillCaster)->SendTargetHP(0, pInstance->sTargetID, -damage);
	}

	if (pInstance->pSkillCaster->isPlayer() && m_pSrcUser->m_bInvisibilityType != 0) //If we're allowing monsters to be stealthed too (it'd be cool) then this check needs to be changed.
		m_pSrcUser->StateChangeServerDirect(7, INVIS_NONE);

	// This should only be sent once. I don't think there's reason to enforce this, as no skills behave otherwise
	pInstance->sData4 = (damage == 0 ? -104 : 0);
	SendSkill(pInstance);	

	return bResult;
}

bool CMagicProcess::ExecuteType2(MagicInstance * pInstance)
{
	if (pInstance->pSkill == NULL)
		return false;

	int damage = 0;
	bool bResult = false;
	float total_range = 0.0f;	// These variables are used for range verification!
	int sx, sz, tx, tz;

	_MAGIC_TYPE2 *pType = g_pMain.m_Magictype2Array.GetData(pInstance->nSkillID);
	if (pType == NULL
		// The user does not have enough arrows! We should point them in the right direction. ;)
		|| !m_pSrcUser->CheckExistItem(pInstance->pSkill->iUseItem, pType->bNeedArrow))
		return false;

	// Not wearing a left-handed bow
	_ITEM_TABLE* pTable = m_pSrcUser->GetItemPrototype(LEFTHAND);
	if (pTable == NULL || !pTable->isBow())
	{
		pTable = m_pSrcUser->GetItemPrototype(RIGHTHAND);

		// Not wearing a right-handed (2h) bow either!
		if (pTable == NULL || !pTable->isBow())
			return false;
	}

	// is this checked already?
	if (pInstance->pSkillTarget == NULL || pInstance->pSkillTarget->isDead())
		goto packet_send;
	
	total_range = pow(((pType->sAddRange * pTable->m_sRange) / 100.0f), 2.0f) ;     // Range verification procedure.
	sx = (int)pInstance->pSkillCaster->GetX(); tx = (int)pInstance->pSkillTarget->GetX();
	sz = (int)pInstance->pSkillCaster->GetZ(); tz = (int)pInstance->pSkillTarget->GetZ();
	
	if ((pow((float)(sx - tx), 2.0f) + pow((float)(sz - tz), 2.0f)) > total_range)	   // Target is out of range, exit.
		goto packet_send;
	
	damage = pInstance->pSkillCaster->GetDamage(pInstance->pSkillTarget, pInstance->pSkill);  // Get damage points of enemy.	

	pInstance->pSkillTarget->HpChange(-damage, m_pSrcUser);     // Reduce target health point.

	// This is more than a little ugly.
	if (pInstance->pSkillCaster->isPlayer())
		TO_USER(pInstance->pSkillCaster)->SendTargetHP(0, pInstance->sTargetID, -damage);     // Change the HP of the target.

packet_send:
	if(pInstance->pSkillCaster->isPlayer() && m_pSrcUser->m_bInvisibilityType != 0) //If we're allowing monsters to be stealthed too (it'd be cool) then this check needs to be changed.
		m_pSrcUser->StateChangeServerDirect(7, INVIS_NONE);
	// This should only be sent once. I don't think there's reason to enforce this, as no skills behave otherwise
	pInstance->sData4 = (damage == 0 ? -104 : 0);
	SendSkill(pInstance);

	return bResult;
}

bool CMagicProcess::ExecuteType3(MagicInstance * pInstance)  // Applied when a magical attack, healing, and mana restoration is done.
{	
	int damage = 0, duration_damage = 0;
	vector<Unit *> casted_member;

	_MAGIC_TYPE3* pType = g_pMain.m_Magictype3Array.GetData(pInstance->nSkillID);
	if (pType == NULL) 
		return false;

	// If the target's a group of people...
	if (pInstance->sTargetID == -1)
	{
		// TO-DO: Make this not completely and utterly suck (i.e. kill that loop!).
		SessionMap & sessMap = g_pMain.s_socketMgr.GetActiveSessionMap();
		foreach (itr, sessMap)
		{		
			CUser* pTUser = TO_USER(itr->second);
			if (!pTUser->isDead() && !pTUser->isBlinking()
				&& UserRegionCheck(pInstance->sCasterID, pTUser->GetSocketID(), pInstance->nSkillID, pType->bRadius, pInstance->sData1, pInstance->sData3))
				casted_member.push_back(pTUser);
		}
		g_pMain.s_socketMgr.ReleaseLock();

		if (casted_member.empty())
		{
			SendSkillFailed(pInstance);
			return false;			
		}
	}
	else
	{	// If the target was a single unit.
		if (pInstance->pSkillTarget == NULL 
			|| pInstance->pSkillTarget->isDead() 
			|| (pInstance->pSkillTarget->isPlayer() 
				&& TO_USER(pInstance->pSkillTarget)->isBlinking())) 
			return false;
		
		casted_member.push_back(pInstance->pSkillTarget);
	}
	
	foreach (itr, casted_member)
	{
		// assume player for now
		CUser* pTUser = TO_USER(*itr); // it's checked above, not much need to check it again
		if ((pType->sFirstDamage < 0) && (pType->bDirectType == 1) && (pInstance->nSkillID < 400000))	// If you are casting an attack spell.
			damage = GetMagicDamage(pInstance, pTUser, pType->sFirstDamage, pType->bAttribute) ;	// Get Magical damage point.
		else 
			damage = pType->sFirstDamage;

		if( m_pSrcUser )	{
			if( m_pSrcUser->m_bZone == ZONE_SNOW_BATTLE && g_pMain.m_byBattleOpen == SNOW_BATTLE )
				damage = -10;		
		}

		if (pType->bDuration == 0) 
		{     // Non-Durational Spells.
			if (pType->bDirectType == 1)     // Health Point related !
			{			
				pTUser->HpChange(damage, m_pSrcUser);     // Reduce target health point.
				m_pSrcUser->SendTargetHP( 0, (*itr)->GetID(), damage );     // Change the HP of the target.			
			}
			else if ( pType->bDirectType == 2 || pType->bDirectType == 3 )    // Magic or Skill Point related !
				pTUser->MSpChange(damage);     // Change the SP or the MP of the target.		
			else if( pType->bDirectType == 4 )     // Armor Durability related.
				pTUser->ItemWoreOut( DEFENCE, -damage);     // Reduce Slot Item Durability
			else if( pType->bDirectType == 8 ) //
				continue;
				//Need to absorb HP from the target user to the source user
			else if( pType->bDirectType == 9 ) //Damage based on percentage of target's max HP
			{
				if (pType->sFirstDamage < 100)
					damage = (pType->sFirstDamage * pTUser->m_sHp) / -100;
				else
					damage = (pTUser->m_iMaxHp * (pType->sFirstDamage - 100)) / 100;

				pTUser->HpChange(damage, m_pSrcUser);
				m_pSrcUser->SendTargetHP( 0, (*itr)->GetID(), damage );
			}
			if (pInstance->sTargetID != -1)
				pInstance->sData2 = 1;
		}
		else if (pType->bDuration != 0) {    // Durational Spells! Remember, durational spells only involve HPs.
			if (damage != 0) {		// In case there was first damage......
				pTUser->HpChange(damage, m_pSrcUser);			// Initial damage!!!
				m_pSrcUser->SendTargetHP(0, pTUser->GetSocketID(), damage );     // Change the HP of the target. 
			}

			if (pTUser->m_bResHpType != USER_DEAD) {	// ���⵵ ��ȣ �ڵ� �߽�...
				if (pType->sTimeDamage < 0) {
					duration_damage = GetMagicDamage(pInstance, pTUser, pType->sTimeDamage, pType->bAttribute) ;
				}
				else duration_damage = pType->sTimeDamage ;

				for (int k = 0 ; k < MAX_TYPE3_REPEAT ; k++) {	// For continuous damages...
					if (pTUser->m_bHPInterval[k] == 5) {
						pTUser->m_tHPStartTime[k] = pTUser->m_tHPLastTime[k] = UNIXTIME;     // The durational magic routine.
						pTUser->m_bHPDuration[k] = pType->bDuration;
						pTUser->m_bHPInterval[k] = 2;		
						pTUser->m_bHPAmount[k] = duration_damage / ( pTUser->m_bHPDuration[k] / pTUser->m_bHPInterval[k] ) ;
						pTUser->m_sSourceID[k] = pInstance->sCasterID;
						break;
					}
				}

				pTUser->m_bType3Flag = TRUE;
			}

			if (pTUser->isInParty() && pType->sTimeDamage < 0)
				pTUser->SendPartyStatusUpdate(1, 1);

			pTUser->SendUserStatusUpdate(pType->bAttribute == POISON_R ? USER_STATUS_POISON : USER_STATUS_DOT, USER_STATUS_INFLICT);
		}

		if (pInstance->pSkillCaster->isPlayer() //If we're allowing monsters to be stealthed too (it'd be cool) then this check needs to be changed.
			&& m_pSrcUser->m_bInvisibilityType != 0 
			&& damage < 0) //To allow for minor healing (as rogues)
			m_pSrcUser->StateChangeServerDirect(7, INVIS_NONE);
	
		if ( pInstance->pSkill->bType[1] == 0 || pInstance->pSkill->bType[1] == 3 )
			SendSkill(pInstance, pInstance->sCasterID, pTUser->GetSocketID());

		// Tell the AI server we're healing someone (so that they can choose to pick on the healer!)
		if (pType->bDirectType == 1 && damage > 0
			&& pInstance->sCasterID != pInstance->sTargetID)
		{
			Packet result(AG_HEAL_MAGIC);
			result << pInstance->sCasterID;
			g_pMain.Send_AIServer(&result);
		}
	}

	return true;
}

bool CMagicProcess::ExecuteType4(MagicInstance * pInstance)
{
	int damage = 0;

	vector<CUser *> casted_member;
	if (pInstance->pSkill == NULL)
		return false;

	_MAGIC_TYPE4* pType = g_pMain.m_Magictype4Array.GetData(pInstance->nSkillID);
	if (pType == NULL)
		return false;

	if (!pInstance->bIsRecastingSavedMagic && pInstance->pSkillTarget->HasSavedMagic(pInstance->nSkillID))
		return false;

	if (pInstance->sTargetID == -1)
	{
		// TO-DO: Localise this. This is horribly unnecessary.
		SessionMap & sessMap = g_pMain.s_socketMgr.GetActiveSessionMap();
		foreach (itr, sessMap)
		{		
			CUser* pTUser = TO_USER(itr->second);
			if (!pTUser->isDead() && !pTUser->isBlinking()
				&& UserRegionCheck(pInstance->sCasterID, pTUser->GetSocketID(), pInstance->nSkillID, pType->bRadius, pInstance->sData1, pInstance->sData3))
				casted_member.push_back(pTUser);
		}
		g_pMain.s_socketMgr.ReleaseLock();

		if (casted_member.empty())
		{		
			if (pInstance->sCasterID >= 0 && pInstance->sCasterID < MAX_USER) 
				SendSkillFailed(pInstance);

			return false;
		}
	}
	else 
	{
		// If the target was another single player.
		CUser* pTUser = g_pMain.GetUserPtr(pInstance->sTargetID);
		if (pTUser == NULL 
			|| pTUser->isDead() || (pTUser->isBlinking() && !pInstance->bIsRecastingSavedMagic)) 
			return false;

		casted_member.push_back(pTUser);
	}

	foreach (itr, casted_member)
	{
		uint8 bResult = 1;
		CUser* pTUser = *itr;

		if (pTUser->m_bType4Buff[pType->bBuffType - 1] == 2 && pInstance->sTargetID == -1) {		// Is this buff-type already casted on the player?
			bResult = 0;
			goto fail_return ;					
		}

		if ( pInstance->nSkillID > 500000 && pInstance->pSkillTarget->isPlayer() )
			pInstance->pSkillTarget->InsertSavedMagic(pInstance->nSkillID, pType->sDuration);

		switch (pType->bBuffType)
		{
			case BUFF_TYPE_HP_MP:
				if (pType->sMaxHP == 0 && pType->sMaxHPPct > 0)
					pTUser->m_sMaxHPAmount = (pType->sMaxHPPct - 100) * (pTUser->m_iMaxHp - pTUser->m_sMaxHPAmount) / 100;
				else
					pTUser->m_sMaxHPAmount = pType->sMaxHP;

				if (pType->sMaxMP == 0 && pType->sMaxMPPct > 0)
					pTUser->m_sMaxMPAmount = (pType->sMaxMPPct - 100) * (pTUser->m_iMaxMp - pTUser->m_sMaxMPAmount) / 100;
				else
					pTUser->m_sMaxMPAmount = pType->sMaxMP;
				break;

			case BUFF_TYPE_AC:
				if (pType->sAC == 0)
					pTUser->m_sACAmount = pTUser->m_sTotalAc * (pType->sACPct - 100) / 100;
				else
					pTUser->m_sACAmount = pType->sAC;
				break;

			case BUFF_TYPE_SIZE:
				// These really shouldn't be hardcoded, but with mgame's implementation it doesn't seem we have a choice (as is).
				if (pInstance->nSkillID == 490034)	// Bezoar!!!
					pTUser->StateChangeServerDirect(3, ABNORMAL_GIANT); 
				else if (pInstance->nSkillID == 490035)	// Rice Cake!!!
					pTUser->StateChangeServerDirect(3, ABNORMAL_DWARF); 
				break;

			case BUFF_TYPE_DAMAGE:
				pTUser->m_bAttackAmount = pType->bAttack;
				break;

			case BUFF_TYPE_ATTACK_SPEED:
				pTUser->m_bAttackSpeedAmount = pType->bAttackSpeed;
				break;

			case BUFF_TYPE_SPEED:
				pTUser->m_bSpeedAmount = pType->bSpeed;
				break;

			case BUFF_TYPE_STATS:
				pTUser->setStatBuff(STAT_STR, pType->bStr);
				pTUser->setStatBuff(STAT_STA, pType->bSta);
				pTUser->setStatBuff(STAT_DEX, pType->bDex);
				pTUser->setStatBuff(STAT_INT, pType->bIntel);
				pTUser->setStatBuff(STAT_CHA, pType->bCha);	
				break;

			case BUFF_TYPE_RESISTANCES:
				pTUser->m_bFireRAmount = pType->bFireR;
				pTUser->m_bColdRAmount = pType->bColdR;
				pTUser->m_bLightningRAmount = pType->bLightningR;
				pTUser->m_bMagicRAmount = pType->bMagicR;
				pTUser->m_bDiseaseRAmount = pType->bDiseaseR;
				pTUser->m_bPoisonRAmount = pType->bPoisonR;
				break;

			case BUFF_TYPE_ACCURACY:
				pTUser->m_bHitRateAmount = pType->bHitRate;
				pTUser->m_sAvoidRateAmount = pType->sAvoidRate;
				break;	

			case BUFF_TYPE_MAGIC_POWER:
				pTUser->m_sMagicAttackAmount = (pType->bMagicAttack - 100) * pTUser->getStat(STAT_CHA) / 100;
				break;

			case BUFF_TYPE_EXPERIENCE:
				pTUser->m_bExpGainAmount = pType->bExpPct;
				break;

			case BUFF_TYPE_WEIGHT:
				pTUser->m_bMaxWeightAmount = pType->bExpPct;
				break;

			case BUFF_TYPE_WEAPON_DAMAGE:
				// uses pType->Attack
				break;

			case BUFF_TYPE_WEAPON_AC:
				// uses pType->AC if set, otherwise pType->ACPct
				break;

			case BUFF_TYPE_LOYALTY:
				// uses pType->ExpPct
				break;

			case BUFF_TYPE_NOAH_BONUS:
				break;

			case BUFF_TYPE_PREMIUM_MERCHANT:
				pTUser->m_bPremiumMerchant = true;
				break;

			case BUFF_TYPE_ATTACK_SPEED_ARMOR:
				// 
				break;

			case BUFF_TYPE_DAMAGE_DOUBLE:
				// Double damage output
				break;

			case BUFF_TYPE_DISABLE_TARGETING:
				// Set a variable to disable casting of skills
				break;

			case BUFF_TYPE_BLIND:
				// Set a variable to disable casting of skills (as we are blinded!)
				pTUser->SendUserStatusUpdate(USER_STATUS_BLIND, USER_STATUS_INFLICT);
				break;

			default:
				bResult = 0;
				goto fail_return;
		}


		pTUser->m_sDuration[pType->bBuffType - 1] = pType->sDuration;
		pTUser->m_tStartTime[pType->bBuffType - 1] = UNIXTIME;

		if (pInstance->sTargetID != -1 && pInstance->pSkill->bType[0] == 4)
		{
			if (pInstance->sCasterID >= 0 && pInstance->sCasterID < MAX_USER)
				m_pSrcUser->MSpChange( -(pInstance->pSkill->sMsp) );
		}

		if (pInstance->sCasterID >= 0 && pInstance->sCasterID < MAX_USER) 
			pTUser->m_bType4Buff[pType->bBuffType - 1] = (m_pSrcUser->GetNation() == pTUser->GetNation() ? 2 : 1);
		else
			pTUser->m_bType4Buff[pType->bBuffType - 1] = 1;

		pTUser->m_bType4Flag = TRUE;

		pTUser->SetSlotItemValue();				// Update character stats.
		pTUser->SetUserAbility();

		if(pInstance->sCasterID == pInstance->sTargetID)
			pTUser->SendItemMove(2);

		if (pTUser->isInParty() && pTUser->m_bType4Buff[pType->bBuffType - 1] == 1)
			pTUser->SendPartyStatusUpdate(2, 1);

		pTUser->Send2AI_UserUpdateInfo();

	fail_return:
		if (pInstance->pSkill->bType[1] == 0 || pInstance->pSkill->bType[1] == 4)
		{
			CUser *pUser = (pInstance->sCasterID >= 0 && pInstance->sCasterID < MAX_USER ? m_pSrcUser : pTUser);

			pInstance->sTargetID = (*itr)->GetSocketID();
			pInstance->sData2 = bResult;
			if (!pInstance->bIsRecastingSavedMagic)
				pInstance->sData4 = (bResult == 1 || pInstance->sData4 == 0 ? pType->sDuration : 0);
			pInstance->sData6 = pType->bSpeed;
			pUser->m_MagicProcess.SendSkill(pInstance);

			if (pInstance->pSkill->bMoral >= MORAL_ENEMY)
				pTUser->SendUserStatusUpdate(pType->bBuffType == BUFF_TYPE_SPEED ? USER_STATUS_SPEED : USER_STATUS_POISON, USER_STATUS_INFLICT);
		}
		
		if (bResult == 0
			&& pInstance->sCasterID >= 0 && pInstance->sCasterID < MAX_USER)
			SendSkill(pInstance, pInstance->sCasterID, (*itr)->GetSocketID(), MAGIC_FAIL);

		continue;
	}
	return true;
}

bool CMagicProcess::ExecuteType5(MagicInstance * pInstance)
{
	// Disallow NPCs (for now?).
	if (pInstance->pSkillCaster->isNPC()
		|| pInstance->pSkillTarget->isNPC())
		return false;

	int damage = 0;
	int buff_test = 0; BOOL bType3Test = TRUE, bType4Test = TRUE; 	

	if (pInstance->pSkill == NULL)
		return false;

	_MAGIC_TYPE5* pType = g_pMain.m_Magictype5Array.GetData(pInstance->nSkillID);
	if (pType == NULL
		|| pInstance->pSkillTarget == NULL 
		|| (pInstance->pSkillTarget->isDead() && pType->bType != RESURRECTION) 
		|| (!pInstance->pSkillTarget->isDead() && pType->bType == RESURRECTION)) 
		return false;

	switch (pType->bType) 
	{
		case REMOVE_TYPE3:		// REMOVE TYPE 3!!!
			for (int i = 0; i < MAX_TYPE3_REPEAT; i++)
			{
				if (pInstance->pSkillTarget->m_bHPAmount[i] < 0) {
					pInstance->pSkillTarget->m_tHPStartTime[i] = 0;
					pInstance->pSkillTarget->m_tHPLastTime[i] = 0;   
					pInstance->pSkillTarget->m_bHPAmount[i] = 0;
					pInstance->pSkillTarget->m_bHPDuration[i] = 0;				
					pInstance->pSkillTarget->m_bHPInterval[i] = 5;
					pInstance->pSkillTarget->m_sSourceID[i] = -1;

					if (pInstance->pSkillTarget->isPlayer())
					{
						// TO-DO: Wrap this up (ugh, I feel so dirty)
						Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE3_END));
						result << uint8(200); // removes all
						TO_USER(pInstance->pSkillTarget)->Send(&result); 
					}
				}
			}

			buff_test = 0;
			for (int j = 0; j < MAX_TYPE3_REPEAT; j++)
				buff_test += pInstance->pSkillTarget->m_bHPDuration[j];
			if (buff_test == 0) pInstance->pSkillTarget->m_bType3Flag = FALSE;	

			// Check for Type 3 Curses.
			for (int k = 0; k < MAX_TYPE3_REPEAT; k++)
			{
				if (pInstance->pSkillTarget->m_bHPAmount[k] < 0)
				{
					bType3Test = FALSE;
					break;
				}
			}
  
			if (pInstance->pSkillTarget->isPlayer()
				&& TO_USER(pInstance->pSkillTarget)->isInParty() && bType3Test)
				TO_USER(pInstance->pSkillTarget)->SendPartyStatusUpdate(1);
			break;

		case REMOVE_TYPE4:		// REMOVE TYPE 4!!!
			for (int i = 0; i < MAX_TYPE4_BUFF; i++)
			{
				if (pInstance->pSkillTarget->m_bType4Buff[i] == 0)
					continue;

				uint8 buff_type = i + 1;

				switch (buff_type)
				{
				case 1: 
					pInstance->pSkillTarget->m_sMaxHPAmount = 0;
					pInstance->pSkillTarget->m_sMaxMPAmount = 0;
					break;

				case 2:
					pInstance->pSkillTarget->m_sACAmount = 0;
					break;

				case 3:
					if (pInstance->pSkillTarget->isPlayer())
						TO_USER(pInstance->pSkillTarget)->StateChangeServerDirect(3, ABNORMAL_NORMAL);
					break;

				case 4:
					pInstance->pSkillTarget->m_bAttackAmount = 100;
					break;

				case 5:
					pInstance->pSkillTarget->m_bAttackSpeedAmount = 100;
					break;

				case 6:
					pInstance->pSkillTarget->m_bSpeedAmount = 100;
					break;

				case 7:
					if (pInstance->pSkillTarget->isPlayer())
						memset(TO_USER(pInstance->pSkillTarget)->m_sStatItemBonuses, 0, sizeof(uint16) * STAT_COUNT);
					break;

				case 8:
					pInstance->pSkillTarget->m_bFireRAmount = pInstance->pSkillTarget->m_bColdRAmount = pInstance->pSkillTarget->m_bLightningRAmount = 0;
					pInstance->pSkillTarget->m_bMagicRAmount = pInstance->pSkillTarget->m_bDiseaseRAmount = pInstance->pSkillTarget->m_bPoisonRAmount = 0;
					break;

				case 9:
					pInstance->pSkillTarget->m_bHitRateAmount = 100;
					pInstance->pSkillTarget->m_sAvoidRateAmount = 100;
					break;
				}

				pInstance->pSkillTarget->m_bType4Buff[i] = 0;
				SendType4BuffRemove(pInstance->sTargetID, buff_type);
			}

			if (pInstance->pSkillTarget->isPlayer())
			{
				TO_USER(pInstance->pSkillTarget)->SetSlotItemValue();
				TO_USER(pInstance->pSkillTarget)->SetUserAbility();
				TO_USER(pInstance->pSkillTarget)->Send2AI_UserUpdateInfo();
			}

			buff_test = 0;
			for (int i = 0; i < MAX_TYPE4_BUFF; i++)
				buff_test += pInstance->pSkillTarget->m_bType4Buff[i];
			if (buff_test == 0) pInstance->pSkillTarget->m_bType4Flag = FALSE;

			bType4Test = TRUE ;
			for (int j = 0; j < MAX_TYPE4_BUFF; j++)
			{
				if (pInstance->pSkillTarget->m_bType4Buff[j] == 1)
				{
					bType4Test = FALSE;
					break;
				}
			}

			if (pInstance->pSkillTarget->isPlayer() && TO_USER(pInstance->pSkillTarget)->isInParty() && bType4Test)
				TO_USER(pInstance->pSkillTarget)->SendPartyStatusUpdate(2, 0);
			break;
			
		case RESURRECTION:		// RESURRECT A DEAD PLAYER!!!
			if (pInstance->pSkillTarget->isPlayer())
				TO_USER(pInstance->pSkillTarget)->Regene(1, pInstance->nSkillID);
			break;

		case REMOVE_BLESS:
			if (pInstance->pSkillTarget->m_bType4Buff[0] == 2) {
				pInstance->pSkillTarget->m_sDuration[0] = 0;		
				pInstance->pSkillTarget->m_tStartTime[0] = 0;
				pInstance->pSkillTarget->m_sMaxHPAmount = 0;
				pInstance->pSkillTarget->m_sMaxMPAmount = 0;
				pInstance->pSkillTarget->m_bType4Buff[0] = 0;

				SendType4BuffRemove(pInstance->sTargetID, 1);

				if (pInstance->pSkillTarget->isPlayer())
				{
					TO_USER(pInstance->pSkillTarget)->SetSlotItemValue();
					TO_USER(pInstance->pSkillTarget)->SetUserAbility();
					TO_USER(pInstance->pSkillTarget)->Send2AI_UserUpdateInfo();
				}

				buff_test = 0;
				for (int i = 0; i < MAX_TYPE4_BUFF; i++)
					buff_test += pInstance->pSkillTarget->m_bType4Buff[i];
				if (buff_test == 0) pInstance->pSkillTarget->m_bType4Flag = FALSE;

				bType4Test = TRUE;
				for (int j = 0; j < MAX_TYPE4_BUFF; j++)
				{
					if (pInstance->pSkillTarget->m_bType4Buff[j] == 1)
					{
						bType4Test = FALSE;
						break;
					}
				}

				if (pInstance->pSkillTarget->isPlayer() && TO_USER(pInstance->pSkillTarget)->isInParty() && bType4Test) 
					TO_USER(pInstance->pSkillTarget)->SendPartyStatusUpdate(2, 0);
			}

			break;
	}

	if (pInstance->pSkill->bType[1] == 0 || pInstance->pSkill->bType[1] == 5)
		SendSkill(pInstance);

	return true;
}

bool CMagicProcess::ExecuteType6(MagicInstance * pInstance)
{
	if (pInstance->pSkill == NULL)
		return false;

	_MAGIC_TYPE6 * pType = g_pMain.m_Magictype6Array.GetData(pInstance->nSkillID);
	uint32 iUseItem = 0;
	uint16 sDuration = 0;

	// We can ignore all these checks if we're just recasting on relog.
	if (!pInstance->bIsRecastingSavedMagic)
	{
		if (pInstance->pSkillTarget->HasSavedMagic(pInstance->nSkillID))
			return false;

		if (pType == NULL
			|| m_pSrcUser->isAttackZone()
			|| m_pSrcUser->isTransformed())
			return false;

		// Let's start by looking at the item that was used for the transformation.
		_ITEM_TABLE *pTable = g_pMain.GetItemPtr(m_pSrcUser->m_nTransformationItem);

		// Also, for the sake of specific skills that bypass the list, let's lookup the 
		// item attached to the skill.
		_ITEM_TABLE *pTable2 = g_pMain.GetItemPtr(pInstance->pSkill->iUseItem);

		// If neither of these items exist, we have a bit of a problem...
		if (pTable == NULL 
			&& pTable2 == NULL)
			return false;

		/*
			If it's a totem (which is apparently a ring), then we need to override it 
			with a gem (which are conveniently stored in the skill table!)

			The same is true for special items such as the Hera transformation scroll, 
			however we need to go by the item attached to the skill for this one as 
			these skills bypass the transformation list and thus do not set the flag.
		*/

		// Special items (e.g. Hera transformation scroll) use the scroll (tied to the skill)
		if ((pTable2 != NULL && pTable2->m_bKind == 255)
			// Totems (i.e. rings) take gems (tied to the skill)
			|| (pTable != NULL && pTable->m_bKind == 93)) 
			iUseItem = pInstance->pSkill->iUseItem;
		// If we're using a normal transformation scroll, we can leave the item as it is.
		else 
			iUseItem = m_pSrcUser->m_nTransformationItem;

		// Attempt to take the item (no further checks, so no harm in multipurposing)
		// If we add more checks, remember to change this check.
		if (!m_pSrcUser->RobItem(iUseItem, 1))
			return false;

		// User's casting a new skill. Use the full duration.
		sDuration = pType->sDuration;
	}
	else 
	{
		// Server's recasting the skill (kept on relog, zone change, etc.)
		int16 tmp = pInstance->pSkillTarget->GetSavedMagicDuration(pInstance->nSkillID);

		// Has it expired (or not been set?) -- just in case.
		if (tmp <= 0)
			return false;

		// it's positive, so no harm here in casting.
		sDuration = tmp;
	}

	// TO-DO : Save duration, and obviously end
	m_pSrcUser->m_tTransformationStartTime = UNIXTIME;
	m_pSrcUser->m_sTransformationDuration = sDuration;

	m_pSrcUser->StateChangeServerDirect(3, pInstance->nSkillID);
	m_pSrcUser->m_bIsTransformed = true;

	// TO-DO : Give the users ALL TEH BONUSES!!

	SendSkill(pInstance, pInstance->sCasterID, pInstance->sTargetID, pInstance->bOpcode,
		pInstance->nSkillID, pInstance->sData1, 1, pInstance->sData3, sDuration, 0, 0, 0, 0);

	pInstance->pSkillTarget->InsertSavedMagic(pInstance->nSkillID, sDuration);

	return true;
}


bool CMagicProcess::ExecuteType7(MagicInstance * pInstance)
{
	return true;
}

bool CMagicProcess::ExecuteType8(MagicInstance * pInstance)	// Warp, resurrection, and summon spells.
{	
	if (pInstance->pSkill == NULL)
		return false;

	vector<CUser *> casted_member;
	_MAGIC_TYPE8* pType = g_pMain.m_Magictype8Array.GetData(pInstance->nSkillID);
	if (pType == NULL)
		return false;

	if (pInstance->sTargetID == -1)
	{
		// TO-DO: Localise this loop to make it not suck (the life out of the server).
		SessionMap & sessMap = g_pMain.s_socketMgr.GetActiveSessionMap();
		foreach (itr, sessMap)
		{		
			CUser* pTUser = TO_USER(itr->second);
			if (UserRegionCheck(pInstance->sCasterID, pTUser->GetSocketID(), pInstance->nSkillID, pType->sRadius, pInstance->sData1, pInstance->sData3))
				casted_member.push_back(pTUser);
		}
		g_pMain.s_socketMgr.ReleaseLock();

		if (casted_member.empty()) 
			return false;	
	}
	else 
	{	// If the target was another single player.
		CUser* pTUser = g_pMain.GetUserPtr(pInstance->sTargetID);
		if (pTUser == NULL) 
			return false;
		
		casted_member.push_back(pTUser);
	}

	foreach (itr, casted_member)
	{
		uint8 bResult = 0;
		_OBJECT_EVENT* pEvent = NULL;
		float x = 0.0f, z = 0.0f;
		x = (float)(myrand( 0, 400 )/100.0f);	z = (float)(myrand( 0, 400 )/100.0f);
		if( x < 2.5f )	x = 1.5f + x;
		if( z < 2.5f )	z = 1.5f + z;

		CUser* pTUser = *itr;
		_HOME_INFO* pHomeInfo = g_pMain.m_HomeArray.GetData(pTUser->GetNation());
		if (pHomeInfo == NULL)
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
				pTUser->m_MagicProcess.SendSkill(pInstance, pInstance->sCasterID, (*itr)->GetSocketID(), 
					pInstance->bOpcode, pInstance->nSkillID, pInstance->sData1, 1, pInstance->sData3);

				if (pTUser->GetMap() == NULL)
					continue;

				pEvent = pTUser->GetMap()->GetObjectEvent(pTUser->m_sBind);

				if( pEvent ) {
					pTUser->Warp(uint16((pEvent->fPosX + x) * 10), uint16((pEvent->fPosZ + z) * 10));	
				}
				// TO-DO: Remove this hardcoded nonsense
				else if(pTUser->GetNation() != pTUser->GetZoneID() && pTUser->GetZoneID() <= ELMORAD) 
				{	 // User is in different zone.
					if (pTUser->GetNation() == KARUS) // Land of Karus
						pTUser->Warp(uint16((852 + x) * 10), uint16((164 + z) * 10));
					else	// Land of Elmorad
						pTUser->Warp(uint16((177 + x) * 10), uint16((923 + z) * 10));
				}
				else if (pTUser->m_bZone == ZONE_BATTLE)
					pTUser->Warp(uint16((pHomeInfo->BattleZoneX + x) * 10), uint16((pHomeInfo->BattleZoneZ + z) * 10));	
				else if (pTUser->m_bZone == ZONE_FRONTIER)
					pTUser->Warp(uint16((pHomeInfo->FreeZoneX + x) * 10), uint16((pHomeInfo->FreeZoneZ + z) * 10));
				else
					pTUser->Warp(uint16((pTUser->GetMap()->m_fInitX + x) * 10), uint16((pTUser->GetMap()->m_fInitZ + z) * 10));	
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
				pTUser->m_MagicProcess.SendSkill(pInstance, pInstance->sCasterID, (*itr)->GetSocketID(), 
					pInstance->bOpcode, pInstance->nSkillID, pInstance->sData1, 1, pInstance->sData3);

				pTUser->m_bResHpType = USER_STANDING;     // Target status is officially alive now.
				pTUser->HpChange(pTUser->m_iMaxHp, m_pSrcUser);	 // Refill HP.	
				pTUser->ExpChange( pType->sExpRecover/100 );     // Increase target experience.
				
				Packet result(AG_USER_REGENE);
				result << uint16((*itr)->GetSocketID()) << uint16(pTUser->GetZoneID());
				g_pMain.Send_AIServer(&result);
			} break;

			case 12:	// Summon a target within the zone.	
				if (m_pSrcUser->GetZoneID() != pTUser->GetZoneID())   // Same zone? 
					goto packet_send;

				pTUser->m_MagicProcess.SendSkill(pInstance, pInstance->sCasterID, (*itr)->GetSocketID(), 
					pInstance->bOpcode, pInstance->nSkillID, pInstance->sData1, 1, pInstance->sData3);
					
				pTUser->Warp(m_pSrcUser->GetSPosX(), m_pSrcUser->GetSPosZ());
				break;

			case 13:	// Summon a target outside the zone.			
				if (m_pSrcUser->GetZoneID() == pTUser->GetZoneID())	  // Different zone? 
					goto packet_send;

				pTUser->m_MagicProcess.SendSkill(pInstance, pInstance->sCasterID, (*itr)->GetSocketID(), 
					pInstance->bOpcode, pInstance->nSkillID, pInstance->sData1, 1, pInstance->sData3);

				pTUser->ZoneChange(m_pSrcUser->GetZoneID(), m_pSrcUser->m_curx, m_pSrcUser->m_curz) ;
				pTUser->UserInOut(INOUT_RESPAWN);
				//TRACE(" Summon ,, name=%s, x=%.2f, z=%.2f\n", pTUser->GetName(), pTUser->m_curx, pTUser->m_curz);
				break;

			case 20:	// Randomly teleport the source (within 20 meters)		
				pTUser->m_MagicProcess.SendSkill(pInstance, pInstance->sCasterID, (*itr)->GetSocketID(), 
					pInstance->bOpcode, pInstance->nSkillID, pInstance->sData1, 1, pInstance->sData3);

				float warp_x, warp_z;		// Variable Initialization.
				float temp_warp_x, temp_warp_z;

				warp_x = pTUser->m_curx;	// Get current locations.
				warp_z = pTUser->m_curz;

				temp_warp_x = (float)myrand(0, 20) ;	// Get random positions (within 20 meters)
				temp_warp_z = (float)myrand(0, 20) ;

				if (temp_warp_x > 10)	// Get new x-position.
					warp_x = warp_x + (temp_warp_x - 10 ) ;
				else
					warp_x = warp_x - temp_warp_x ;

				if (temp_warp_z > 10)	// Get new z-position.
					warp_z = warp_z + (temp_warp_z - 10 ) ;
				else
					warp_z = warp_z - temp_warp_z ;
				
				if (warp_x < 0.0f) warp_x = 0.0f ;		// Make sure all positions are within range.
				if (warp_x > 4096) warp_x = 4096 ;		// Change it if it isn't!!!
				if (warp_z < 0.0f) warp_z = 0.0f ;		// (Warp function does not check this!)
				if (warp_z > 4096) warp_z = 4096 ;

				pTUser->Warp(uint16(warp_x * 10), uint16(warp_z * 10));
				break;

			case 21:	// Summon a monster within a zone.
				// LATER!!! 
				break;
			case 25:
				//This is used by Wild Advent (70 rogue skill) and Descent, teleport the user to the target user (Moral check to distinguish between the 2 skills)
				float dest_x, dest_z = 0.0f;
				if (pTUser->GetZoneID() != m_pSrcUser->GetZoneID()) //If we're not even in the same zone, I can't teleport to you!
					return false;
				if (pInstance->pSkill->bMoral < MORAL_ENEMY && pTUser->GetNation() != m_pSrcUser->GetNation()) //I'm not the same nation as you are and thus can't t
					return false;
					
				dest_x = pTUser->m_curx;
				dest_z = pTUser->m_curz;

				m_pSrcUser->Warp(uint16(dest_x * 10), uint16(dest_z * 10));
				break;
		}

		bResult = 1;
		
packet_send:
		pInstance->sData2 = bResult;
		SendSkill(pInstance, pInstance->sCasterID, (*itr)->GetSocketID());
	}
	return true;
}

bool CMagicProcess::ExecuteType9(MagicInstance * pInstance)
{
	if (pInstance->pSkill == NULL)
		return false;

	_MAGIC_TYPE9* pType = g_pMain.m_Magictype9Array.GetData(pInstance->nSkillID);
	if (pType == NULL)
		return false;

	pInstance->sData2 = 1;
	
	if (pInstance->pSkillTarget->HasSavedMagic(pInstance->nSkillID))
	{
		pInstance->sData2 = 0;
		SendSkillFailed(pInstance);
		return false;
	}
	
	if (pType->bStateChange <= 2)
	{
		m_pSrcUser->StateChangeServerDirect(7, pType->bStateChange); // Update the client to be invisible
		pInstance->pSkillTarget->InsertSavedMagic(pInstance->nSkillID, pType->sDuration);
	}
	else if (pType->bStateChange >= 3 && pType->bStateChange <= 4)
	{
		Packet stealth(WIZ_STEALTH);
		stealth << uint8(1) << uint16(pType->sRadius);
		if(m_pSrcUser->isInParty())
		{
			_PARTY_GROUP* pParty = g_pMain.m_PartyArray.GetData(m_pSrcUser->m_sPartyIndex);
			if (pParty == NULL)
				return false;

			for (int i = 0; i < MAX_PARTY_USERS; i++)
			{
				CUser *pUser = g_pMain.GetUserPtr(pParty->uid[i]);
				if (pUser == NULL)
					continue;
				//To-do : save the skill for all the party members
				pUser->Send(&stealth);
			}
		}
		else
			m_pSrcUser->Send(&stealth);
		//TO-DO : Save the skill in the saved skill list
	}

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_EFFECTING));
	result	<< pInstance->nSkillID << pInstance->sCasterID << pInstance->sTargetID
			<< pInstance->sData1 << pInstance->sData2 << pInstance->sData3 << pInstance->sData4 << pInstance->sData5 << pInstance->sData6 << pInstance->sData7 << pInstance->sData8;
	if(m_pSrcUser->isInParty() && pType->bStateChange == 4)
		g_pMain.Send_PartyMember(m_pSrcUser->m_sPartyIndex, &result);
	else
		m_pSrcUser->Send(&result);

	return true;
}

short CMagicProcess::GetMagicDamage(MagicInstance * pInstance, Unit *pTarget, int total_hit, int attribute)
{	
	short damage = 0, temp_hit = 0, righthand_damage = 0, attribute_damage = 0 ; 
	int random = 0, total_r = 0 ;
	BYTE result; 

	if (pTarget == NULL
		|| pInstance->pSkillCaster == NULL
		|| pTarget->isDead()
		|| pInstance->pSkillCaster->isDead())
		return 0;

	uint16 sMagicAmount = 0;
	if (pInstance->pSkillCaster->isNPC())
	{
		result = pInstance->pSkillCaster->GetHitRate(pTarget->m_sTotalHitrate / pInstance->pSkillCaster->m_sTotalEvasionrate); 
	}
	else
	{
		CUser *pUser = TO_USER(pInstance->pSkillCaster);
		uint8 bCha = pUser->getStat(STAT_CHA);
		if (bCha > 86)
			sMagicAmount = pUser->m_sMagicAttackAmount - (bCha - 86);

		total_hit *= pUser->getStat(STAT_CHA) / 186;
		result = SUCCESS;
	}
		
	if (result != FAIL) 
	{
		// In case of SUCCESS.... 
		switch (attribute)
		{
			case FIRE_R: 
				total_r = pTarget->m_bFireR + pTarget->m_bFireRAmount;
				break;
			case COLD_R :
				total_r = pTarget->m_bColdR + pTarget->m_bColdRAmount;
				break;
			case LIGHTNING_R :
				total_r = pTarget->m_bLightningR + pTarget->m_bLightningRAmount; 
				break;
			case MAGIC_R :
				total_r = pTarget->m_bMagicR + pTarget->m_bMagicRAmount;
				break;
			case DISEASE_R :
				total_r = pTarget->m_bDiseaseR + pTarget->m_bDiseaseRAmount;
				break;
			case POISON_R :			
				total_r = pTarget->m_bPoisonR + pTarget->m_bPoisonRAmount;
				break;
		}
		
		if (pInstance->pSkillCaster->isPlayer()) 
		{
			CUser *pUser = TO_USER(pInstance->pSkillCaster);
			_ITEM_TABLE *pRightHand = pUser->GetItemPrototype(RIGHTHAND);
			if (pRightHand != NULL && pRightHand->isStaff()
				&& pUser->GetItemPrototype(LEFTHAND) == NULL)
			{
				righthand_damage = pRightHand->m_sDamage;
					
				if (m_pSrcUser->m_bMagicTypeRightHand == attribute)
					attribute_damage = pRightHand->m_sDamage;
			}
			else 
			{
				righthand_damage = 0;
			}
		}

		damage = (short)(230 * total_hit / (total_r + 250));
		random = myrand(0, damage);
		damage = (short)(random * 0.3f + (damage * 0.85f)) - sMagicAmount;

		if (pInstance->pSkillCaster->isNPC())
			damage -= ((3 * righthand_damage) + (3 * attribute_damage));
		else if (attribute != 4)	// Only if the staff has an attribute.
			damage -= (short)(((righthand_damage * 0.8f) + (righthand_damage * pInstance->pSkillCaster->GetLevel()) / 60) + ((attribute_damage * 0.8f) + (attribute_damage * pInstance->pSkillCaster->GetLevel()) / 30));
	}

	return damage / 3;		
}

// TO-DO: Clean this up (even using unit code...)
BOOL CMagicProcess::UserRegionCheck(int sid, int tid, int magicid, int radius, short mousex, short mousez)
{
	CNpc* pMon = NULL;

	uint32 currenttime = 0;
	BOOL bFlag = FALSE;

	CUser* pTUser = g_pMain.GetUserPtr(tid);  
	if (pTUser == NULL) return FALSE;
	
	if (sid >= NPC_BAND) {					
		pMon = g_pMain.m_arNpcArray.GetData(sid);
		if( !pMon || pMon->m_NpcState == NPC_DEAD ) return FALSE;
		bFlag = TRUE;
	}

	_MAGIC_TABLE* pMagic = g_pMain.m_MagictableArray.GetData( magicid );   // Get main magic table.
	if( !pMagic ) return FALSE;

	switch (pMagic->bMoral) {
		case MORAL_PARTY_ALL :		// Check that it's your party.
/*
			if( !pTUser->isInParty()) {
				if (sid == tid) {
					return TRUE; 
				}
				else {
					return FALSE; 
				}
			}			

			if ( pTUser->m_sPartyIndex == m_pSrcUser->m_sPartyIndex) 
				goto final_test;

			break;
*/

			if (!pTUser->isInParty())
				return (sid == tid);

			if ( pTUser->m_sPartyIndex == m_pSrcUser->m_sPartyIndex && pMagic->bType[0] != 8 ) {
				goto final_test;
			}
			else if (pTUser->m_sPartyIndex == m_pSrcUser->m_sPartyIndex && pMagic->bType[0] == 8) {
				if (pTUser->m_bZone == ZONE_BATTLE && (UNIXTIME - pTUser->m_tLastRegeneTime < CLAN_SUMMON_TIME)) {
					return FALSE;
				}
				else {
					goto final_test;	
				}
			}

			break;
//
		case MORAL_SELF_AREA :
		case MORAL_AREA_ENEMY :
			if (!bFlag) {
				if (pTUser->GetNation() != m_pSrcUser->GetNation())		// Check that it's your enemy.
					goto final_test;
			}
			else {
				if (pTUser->GetNation() != pMon->GetNation())
					goto final_test;
			}
			break;

		case MORAL_AREA_FRIEND :				
			if (pTUser->m_bNation == m_pSrcUser->m_bNation) 		// Check that it's your ally.
				goto final_test;
			break;
// �񷯸ӱ� Ŭ�� ��ȯ!!!
		case MORAL_CLAN_ALL :
			if( pTUser->GetClanID() == -1) {
				if (sid == tid) {
					return TRUE; 
				}
				else {
					return FALSE; 
				}
			}			
/*
			if ( pTUser->GetClanID() == m_pSrcUser->GetClanID()) 
				goto final_test;
*/
			if ( pTUser->GetClanID() == m_pSrcUser->GetClanID() && pMagic->bType[0] != 8 ) {
				goto final_test;
			}
			else if (pTUser->GetClanID() == m_pSrcUser->GetClanID() && pMagic->bType[0] == 8) {
				if (pTUser->m_bZone == ZONE_BATTLE && (UNIXTIME - pTUser->m_tLastRegeneTime < CLAN_SUMMON_TIME)) {
					return FALSE;
				}
				else {
					goto final_test;	
				}
			}

			break;
//
	}

	return FALSE;	

final_test :
	if (!bFlag) {	// When players attack...
		if (pTUser->GetRegion() == m_pSrcUser->GetRegion()) { // Region Check!
			if (radius !=0) { 	// Radius check! ( ...in case there is one :(  )
				float temp_x = pTUser->GetX() - mousex;
				float temp_z = pTUser->GetZ() - mousez;
				float distance = pow(temp_x, 2.0f) + pow(temp_z, 2.0f);
				if ( distance > pow((float)radius, 2.0f) ) return FALSE ;
			}		
			return TRUE;	// Target is in the area.
		}
	}
	else {			// When monsters attack...
		if (pTUser->GetRegion() == pMon->GetRegion()) { // Region Check!
			if (radius !=0) { 	// Radius check! ( ...in case there is one :(  )
				float temp_x = pTUser->GetX() - pMon->GetX();
				float temp_z = pTUser->GetZ() - pMon->GetZ();
				float distance = pow(temp_x, 2.0f) + pow(temp_z, 2.0f);	
				if ( distance > pow((float)radius, 2.0f) ) return FALSE ;
			}		
			return TRUE;	// Target is in the area.
		}
	}

	return FALSE;
}

void CMagicProcess::Type6Cancel()
{
	if (m_pSrcUser == NULL
		|| !m_pSrcUser->m_bIsTransformed)
		return;

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_CANCEL_TYPE6));
	uint32 nSkillID = m_pSrcUser->m_bAbnormalType;

	// TO-DO: Reset stat changes, recalculate stats.
	m_pSrcUser->m_bIsTransformed = false;
	m_pSrcUser->Send(&result);

	m_pSrcUser->StateChangeServerDirect(3, ABNORMAL_NORMAL);
	m_pSrcUser->m_savedMagicMap.erase(nSkillID);
}

void CMagicProcess::Type9Cancel(MagicInstance * pInstance)
{
	if (pInstance->pSkill == NULL)
		return;

	_MAGIC_TYPE9 *pType = g_pMain.m_Magictype9Array.GetData(pInstance->nSkillID);
	if (pType == NULL)
		return;

	uint8 bResponse = 0;
	
	if (pType->bStateChange <= 2 || pType->bStateChange >= 5 && pType->bStateChange < 7) //Stealths
	{
		TO_USER(pInstance->pSkillCaster)->StateChangeServerDirect(7, INVIS_NONE);
		TO_USER(pInstance->pSkillCaster)->m_savedMagicMap.erase(pInstance->nSkillID);
		bResponse = 91;
	}
	else if (pType->bStateChange >= 3 && pType->bStateChange <= 4) //Lupine etc.
	{
		Packet stealth(WIZ_STEALTH);
		stealth << uint16(0) << uint8(0);
		TO_USER(pInstance->pSkillCaster)->Send(&stealth);
		bResponse = 92;
	}
	else if (pType->bStateChange == 7) //Guardian pet related
	{
		Packet pet(WIZ_PET, uint8(1));
		pet << uint16(1) << uint16(6);
		TO_USER(pInstance->pSkillCaster)->Send(&pet);
		bResponse = 93;
	}


	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_END));
		result << bResponse;
	TO_USER(pInstance->pSkillCaster)->Send(&result);
}

void CMagicProcess::Type4Cancel(MagicInstance * pInstance)
{
	if (pInstance->pSkill == NULL)
		return;

	if (pInstance->pSkillTarget != pInstance->pSkillCaster)
		return;

	_MAGIC_TYPE4* pType = g_pMain.m_Magictype4Array.GetData(pInstance->nSkillID);
	if (pType == NULL)
		return;

	BOOL buff = FALSE;
	switch (pType->bBuffType)
	{
		case BUFF_TYPE_HP_MP:
			if (pInstance->pSkillCaster->m_sMaxHPAmount > 0
				|| pInstance->pSkillCaster->m_sMaxMPAmount > 0)
			{
				pInstance->pSkillCaster->m_sMaxHPAmount = 0;
				pInstance->pSkillCaster->m_sMaxMPAmount = 0;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_AC:
			if (pInstance->pSkillCaster->m_sACAmount > 0) 
			{
				pInstance->pSkillCaster->m_sACAmount = 0;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_SIZE:
			if (pInstance->pSkillCaster->isPlayer())
			{
				TO_USER(pInstance->pSkillCaster)->StateChangeServerDirect(3, ABNORMAL_NORMAL);
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_DAMAGE:
			if (pInstance->pSkillCaster->m_bAttackAmount > 100) 
			{
				pInstance->pSkillCaster->m_bAttackAmount = 100;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_ATTACK_SPEED:
			if (pInstance->pSkillCaster->m_bAttackSpeedAmount > 100) 
			{
				pInstance->pSkillCaster->m_bAttackSpeedAmount = 100;	
				buff = TRUE;
			}
			break;	

		case BUFF_TYPE_SPEED:
			if (pInstance->pSkillCaster->m_bSpeedAmount > 100)
			{
				pInstance->pSkillCaster->m_bSpeedAmount = 100;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_STATS:
			if (pInstance->pSkillCaster->isPlayer()
				&& TO_USER(pInstance->pSkillCaster)->getStatBuffTotal() > 0) {
				// TO-DO: Implement reset
				memset(TO_USER(pInstance->pSkillCaster)->m_bStatBuffs, 0, sizeof(uint8) * STAT_COUNT);
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_RESISTANCES:
			if ((pInstance->pSkillCaster->m_bFireRAmount + pInstance->pSkillCaster->m_bColdRAmount + pInstance->pSkillCaster->m_bLightningRAmount +
				pInstance->pSkillCaster->m_bMagicRAmount + pInstance->pSkillCaster->m_bDiseaseRAmount + pInstance->pSkillCaster->m_bPoisonRAmount) > 0) {
				pInstance->pSkillCaster->m_bFireRAmount = 0;
				pInstance->pSkillCaster->m_bColdRAmount = 0;
				pInstance->pSkillCaster->m_bLightningRAmount = 0;
				pInstance->pSkillCaster->m_bMagicRAmount = 0;
				pInstance->pSkillCaster->m_bDiseaseRAmount = 0;
				pInstance->pSkillCaster->m_bPoisonRAmount = 0;
				buff = TRUE;
			}
			break;	

		case BUFF_TYPE_ACCURACY:
			if ((pInstance->pSkillCaster->m_bHitRateAmount + pInstance->pSkillCaster->m_sAvoidRateAmount) > 200)
			{
				pInstance->pSkillCaster->m_bHitRateAmount = 100;
				pInstance->pSkillCaster->m_sAvoidRateAmount = 100;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_MAGIC_POWER:
			if (pInstance->pSkillCaster->m_sMagicAttackAmount > 0)
			{
				pInstance->pSkillCaster->m_sMagicAttackAmount = 0;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_EXPERIENCE:
			if (pInstance->pSkillCaster->m_bExpGainAmount > 100)
			{
				pInstance->pSkillCaster->m_bExpGainAmount = 100;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_WEIGHT:
			if (pInstance->pSkillCaster->m_bMaxWeightAmount > 100)
			{
				pInstance->pSkillCaster->m_bMaxWeightAmount = 100;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_WEAPON_DAMAGE:
		case BUFF_TYPE_WEAPON_AC:
		case BUFF_TYPE_LOYALTY:
			// TO-DO
			break;
	}
	
	if (buff)
	{
		pInstance->pSkillCaster->m_sDuration[pType->bBuffType - 1] = 0;
		pInstance->pSkillCaster->m_tStartTime[pType->bBuffType - 1] = 0;
		pInstance->pSkillCaster->m_bType4Buff[pType->bBuffType - 1] = 0;

		if (pInstance->pSkillCaster->isPlayer())
		{
			TO_USER(pInstance->pSkillCaster)->SetSlotItemValue();
			TO_USER(pInstance->pSkillCaster)->SetUserAbility();
			TO_USER(pInstance->pSkillCaster)->SendItemMove(2);
			TO_USER(pInstance->pSkillCaster)->Send2AI_UserUpdateInfo();

			Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_END));
			result << pType->bBuffType;
			TO_USER(pInstance->pSkillCaster)->Send(&result);
		}
	}

	int buff_test = 0;
	for (int i = 0; i < MAX_TYPE4_BUFF; i++)
		buff_test += pInstance->pSkillCaster->m_bType4Buff[i];
	if (buff_test == 0) pInstance->pSkillCaster->m_bType4Flag = FALSE;	

	if (pInstance->pSkillCaster->isPlayer() && !pInstance->pSkillCaster->m_bType4Flag
		&& TO_USER(pInstance->pSkillCaster)->isInParty())
		TO_USER(pInstance->pSkillCaster)->SendPartyStatusUpdate(2);

	if (TO_USER(pInstance->pSkillCaster)->m_savedMagicMap.find(pInstance->nSkillID) != TO_USER(pInstance->pSkillCaster)->m_savedMagicMap.end())
		TO_USER(pInstance->pSkillCaster)->m_savedMagicMap.erase(pInstance->nSkillID);
}

void CMagicProcess::Type3Cancel(MagicInstance * pInstance)
{
	if (pInstance->pSkill == NULL
		|| pInstance->pSkillCaster != pInstance->pSkillTarget)
		return;

	// Should this take only the specified skill? I'm thinking so.
	_MAGIC_TYPE3* pType = g_pMain.m_Magictype3Array.GetData(pInstance->nSkillID);
	if (pType == NULL)
		return;

	for (int i = 0; i < MAX_TYPE3_REPEAT; i++)
	{
		if (pInstance->pSkillCaster->m_bHPAmount[i] > 0)
		{
			pInstance->pSkillCaster->m_tHPStartTime[i] = 0;
			pInstance->pSkillCaster->m_tHPLastTime[i] = 0;   
			pInstance->pSkillCaster->m_bHPAmount[i] = 0;
			pInstance->pSkillCaster->m_bHPDuration[i] = 0;				
			pInstance->pSkillCaster->m_bHPInterval[i] = 5;
			pInstance->pSkillCaster->m_sSourceID[i] = -1;
			break;
		}
	}

	if (pInstance->pSkillCaster->isPlayer())
	{
		Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE3_END));
		result << uint8(100);
		TO_USER(pInstance->pSkillCaster)->Send(&result); 
	}

	int buff_test = 0;
	for (int j = 0; j < MAX_TYPE3_REPEAT; j++)
		buff_test += pInstance->pSkillCaster->m_bHPDuration[j];
	if (buff_test == 0) pInstance->pSkillCaster->m_bType3Flag = FALSE;	

	if (pInstance->pSkillCaster->isPlayer() && !pInstance->pSkillCaster->m_bType3Flag
		&& TO_USER(pInstance->pSkillCaster)->isInParty())
		TO_USER(pInstance->pSkillCaster)->SendPartyStatusUpdate(1, 0);
}

void CMagicProcess::SendType4BuffRemove(short tid, BYTE buff)
{
	CUser* pTUser = g_pMain.GetUserPtr(tid);  
	if (pTUser == NULL) 
		return;

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_END));
	result << buff;
	pTUser->Send(&result);
}

short CMagicProcess::GetWeatherDamage(short damage, short attribute)
{
	// Give a 10% damage output boost based on weather (and skill's elemental attribute)
	if ((g_pMain.m_nWeather == WEATHER_FINE && attribute == ATTRIBUTE_FIRE)
		|| (g_pMain.m_nWeather == WEATHER_RAIN && attribute == ATTRIBUTE_LIGHTNING)
		|| (g_pMain.m_nWeather == WEATHER_SNOW && attribute == ATTRIBUTE_ICE))
		damage = (damage * 110) / 100;

	return damage;
}

void CMagicProcess::Type4Extend(MagicInstance * pInstance)
{
	if (pInstance->pSkill == NULL)
		return;

	_MAGIC_TYPE4 *pType = g_pMain.m_Magictype4Array.GetData(pInstance->nSkillID);
	if (pType == NULL)
		return;

	CUser* pTUser = g_pMain.GetUserPtr(pInstance->sTargetID);
	if (pTUser == NULL) 
		return;

	if (pInstance->pSkillCaster->isPlayer() && pTUser->RobItem(800022000, 1))
	{
		pTUser->m_sDuration[pType->bBuffType -1] *= 2;
		Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_EXTEND));
		result << uint32(pInstance->nSkillID);
		pTUser->Send(&result);
	}	
}