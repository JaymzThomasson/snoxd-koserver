#include "stdafx.h"
#include "MagicProcess.h"
#include "EbenezerDlg.h"
#include "User.h"
#include "Npc.h"
#include "AiPacket.h"

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

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

void CMagicProcess::MagicPacket(Packet & pkt)
{
	pkt >> m_opcode >> m_nSkillID;

	_MAGIC_TABLE *pMagic = g_pMain->m_MagictableArray.GetData(m_nSkillID);
	if (!pMagic)
		return;

	pkt >> m_sCasterID >> m_sTargetID
		>> m_sData1 >> m_sData2 >> m_sData3 >> m_sData4
		>> m_sData5 >> m_sData6 >> m_sData7 >> m_sData8;

	m_pSkillCaster = g_pMain->GetUnit(m_sCasterID);
	m_pSkillTarget = g_pMain->GetUnit(m_sTargetID);

	if (m_pSkillCaster == NULL
		|| (m_pSrcUser && !UserCanCast(pMagic)))
	{
		SendSkillFailed();
		return;
	}


	// If the target is a mob/NPC *or* we're casting an AOE, tell the AI to handle it.
	if (m_sTargetID >= NPC_BAND
		|| (m_sTargetID == -1 && 
			(pMagic->bMoral == MORAL_AREA_ENEMY 
				|| pMagic->bMoral == MORAL_AREA_ALL 
				|| pMagic->bMoral == MORAL_SELF_AREA)))
	{
		SendSkillToAI(pMagic);

		// If the target is specifically a mob, stop here. AI's got this one.
		// Otherwise, it's an AOE -- which means it might affect players too, so we need to handle it too.
		if (m_sTargetID >= NPC_BAND)
			return;
	}

	bool bInitialResult;
	switch (m_opcode)
	{
		case MAGIC_CASTING:
			SendSkill();
			break;
		case MAGIC_EFFECTING:
			// Hacky check for a transformation item (Disguise Totem, Disguise Scroll)
			// These apply when first type's set to 0, second type's set and obviously, there's a consumable item.
			// Need to find a better way of handling this.
			if (pMagic->bType[0] == 0 && pMagic->bType[1] != 0
				&& pMagic->iUseItem != 0
				&& (m_pSrcUser != NULL 
					&& m_pSrcUser->CheckExistItem(pMagic->iUseItem, 1)))
			{
				SendTransformationList(pMagic);
				return;
			}

			bInitialResult = ExecuteSkill(pMagic, pMagic->bType[0]);

			// NOTE: Some ROFD skills require a THIRD type.
			if (bInitialResult)
				ExecuteSkill(pMagic, pMagic->bType[1]);
			break;
		case MAGIC_FLYING:
		case MAGIC_FAIL:
			SendSkill();
			break;
		case MAGIC_TYPE3_END: //This is also MAGIC_TYPE4_END
			break;
		case MAGIC_CANCEL:
		case MAGIC_CANCEL2:
			Type3Cancel(pMagic);
			Type4Cancel(pMagic);
			Type6Cancel();
			break;
		case MAGIC_CANCEL_TYPE9:
			// Type9Cancel(m_nSkillID, m_pSrcUser->GetSocketID());   // Stealth lupine etc.
			break;
	}
}

bool CMagicProcess::UserCanCast(_MAGIC_TABLE *pSkill)
{
	// We don't want the source ever being anyone other than us.
	// Ever. Even in the case of NPCs, it's BAD. BAD!
	// We're better than that -- we don't need to have the client tell NPCs what to do.
	if (m_pSrcUser != NULL && m_pSkillCaster != m_pSrcUser) 
		return false;

	// We don't need to check anything as we're just canceling our buffs.
	if (m_opcode == MAGIC_CANCEL || m_opcode == MAGIC_CANCEL2) 
		return true;

	// Users who are blinking cannot use skills.
	// Additionally, unless it's resurrection-related, dead players cannot use skills.
	if (m_pSrcUser->isBlinking()
		|| (m_pSrcUser->isDead() && pSkill->bType[0] != 5)) 
		return false;

	// If we're using an AOE, and the target is specified... something's not right.
	if ((pSkill->bMoral >= MORAL_AREA_ENEMY
			&& pSkill->bMoral <= MORAL_SELF_AREA)
		&& m_sTargetID != -1)
		return false;

	if (pSkill->sSkill != 0
		&& (m_pSrcUser->m_pUserData->m_sClass != (pSkill->sSkill / 10)
			|| m_pSrcUser->m_pUserData->m_bLevel < pSkill->sSkillLevel))
		return false;

	if ((m_pSrcUser->m_pUserData->m_sMp - pSkill->sMsp) < 0)
		return false;

	// If we're in a snow war, we're only ever allowed to use the snowball skill.
	if (m_pSrcUser->GetZoneID() == ZONE_SNOW_BATTLE && g_pMain->m_byBattleOpen == SNOW_BATTLE 
		&& m_nSkillID != SNOW_EVENT_SKILL)
		return false;

	if (m_pSkillTarget != NULL)
	{
		// Players require a little more rigorous checking
		if (m_pSkillTarget->isPlayer())
		{
			if (m_pSkillTarget != m_pSrcUser)
			{
				if (m_pSkillTarget->GetZoneID() != m_pSkillCaster->GetZoneID()
					|| !static_cast<CUser *>(m_pSkillTarget)->isAttackZone()
					// Will have to add support for the outside battlefield
					|| (static_cast<CUser *>(m_pSkillTarget)->isAttackZone() 
						&& m_pSkillTarget->GetNation() == m_pSkillCaster->GetNation()))
					return false;
			}
		}
		// NPCs are simp-uhl.
		else 
		{
			if (m_pSkillTarget->GetZoneID() != m_pSrcUser->GetZoneID()
				|| m_pSkillTarget->GetNation() == m_pSrcUser->GetNation())
				return false;
		}
	}

	if ((pSkill->bType[0] != 2 //Archer skills will handle item checking in ExecuteType2()
		&& pSkill->bType[0] != 6) //So will transformations
		&& (pSkill->iUseItem != 0
		&& !m_pSrcUser->CanUseItem(pSkill->iUseItem, 1))) //The user does not meet the item's requirements or does not have any of said item.
		return false;

	if ((m_opcode == MAGIC_EFFECTING || m_opcode == MAGIC_CASTING) && !IsAvailable(pSkill))
		return false;

	//TO-DO : Add skill cooldown checks.

	//Incase we made it to here, we can cast! Hurray!
	return true;
}

void CMagicProcess::SendSkillToAI(_MAGIC_TABLE *pSkill)
{
	if (m_sTargetID >= NPC_BAND 
		|| (m_sTargetID == -1 && (pSkill->bMoral == MORAL_AREA_ENEMY || pSkill->bMoral == MORAL_SELF_AREA)))
	{		
		int total_magic_damage = 0;

		Packet result(AG_MAGIC_ATTACK_REQ); // this is the order it was in.
		result	<< m_sCasterID << m_opcode << m_sTargetID << m_nSkillID 
				<< m_sData1 << m_sData2 << m_sData3 
				<< m_sData4 << m_sData5 << m_sData6
				<< m_pSrcUser->getStatWithItemBonus(STAT_CHA);


		_ITEM_TABLE* pRightHand = m_pSrcUser->GetItemPrototype(RIGHTHAND);
		if (pRightHand != NULL && pRightHand->isStaff()
			&& m_pSrcUser->GetItemPrototype(LEFTHAND) == NULL)
		{
			if (pSkill->bType[0] == 3) {
				total_magic_damage += (int)((pRightHand->m_sDamage * 0.8f)+ (pRightHand->m_sDamage * m_pSrcUser->GetLevel()) / 60);

				_MAGIC_TYPE3 *pType3 = g_pMain->m_Magictype3Array.GetData(m_nSkillID);
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
		g_pMain->Send_AIServer(&result);		
	}
}

bool CMagicProcess::ExecuteSkill(_MAGIC_TABLE *pSkill, uint8 bType)
{
	switch (bType)
	{
		case 1: return ExecuteType1(pSkill);
		case 2: return ExecuteType2(pSkill);
		case 3: return ExecuteType3(pSkill);
		case 4: return ExecuteType4(pSkill);
		case 5: return ExecuteType5(pSkill);
		case 6: return ExecuteType6(pSkill);
		case 7: return ExecuteType7(pSkill);
		case 8: return ExecuteType8(pSkill);
		case 9: return ExecuteType9(pSkill);
	}

	return false;
}

void CMagicProcess::SendTransformationList(_MAGIC_TABLE *pSkill)
{
	if (m_pSrcUser == NULL)
		return;

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TRANSFORM_LIST));
	result << m_nSkillID;
	m_pSrcUser->m_nTransformationItem = pSkill->iUseItem;
	m_pSrcUser->Send(&result);
}

void CMagicProcess::SendSkillFailed()
{
	SendSkill(m_sCasterID, m_sTargetID, MAGIC_FAIL, 
				m_nSkillID, m_sData1, m_sData2, m_sData3, m_sData4, 
				m_sData5, m_sData6, m_sData7, m_sData8);
}

void CMagicProcess::SendSkill(int16 pSkillCaster /* = -1 */, int16 pSkillTarget /* = -1 */,	
								int8 opcode /* = -1 */, uint32 nSkillID /* = 0 */, 
								int16 sData1 /* = -999 */, int16 sData2 /* = -999 */, int16 sData3 /* = -999 */, int16 sData4 /* = -999 */, 
								int16 sData5 /* = -999 */, int16 sData6 /* = -999 */, int16 sData7 /* = -999 */, int16 sData8 /* = -999 */)
{
	Packet result(WIZ_MAGIC_PROCESS);
	int16 sid = 0, tid = 0;

	// Yes, these are all default value overrides.
	// This is completely horrible, but will suffice for now...

	if (opcode == -1)
		opcode = m_opcode;

	if (opcode == MAGIC_FAIL
		&& pSkillCaster >= NPC_BAND)
		return;

	if (pSkillCaster  == -1)
		pSkillCaster = m_sCasterID;

	if (pSkillTarget  == -1)
		pSkillTarget = m_sTargetID;

	if (nSkillID == 0)
		nSkillID = m_nSkillID;

	if (sData1 == -999)
		sData1 = m_sData1;
	if (sData2 == -999)
		sData2 = m_sData2;
	if (sData3 == -999)
		sData3 = m_sData3;
	if (sData4 == -999)
		sData4 = m_sData4;
	if (sData5 == -999)
		sData5 = m_sData5;
	if (sData6 == -999)
		sData6 = m_sData6;
	if (sData7 == -999)
		sData7 = m_sData7;
	if (sData8 == -999)
		sData8 = m_sData8;

	result	<< opcode << nSkillID << pSkillCaster << pSkillTarget 
			<< sData1 << sData2 << sData3 << sData4
			<< sData5 << sData6 << sData7 << sData8;

	if (m_pSrcUser == NULL)
	{
		CNpc *pNpc = g_pMain->m_arNpcArray.GetData(pSkillCaster);
		if (pNpc == NULL)
			return;

		pNpc->SendToRegion(&result);
	}
	else
	{
		CUser *pUser = NULL;
		if ((pSkillCaster < 0 || pSkillCaster >= MAX_USER)
			&& (pSkillTarget >= 0 && pSkillTarget < MAX_USER))
			pUser = g_pMain->GetUserPtr(pSkillTarget);
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

bool CMagicProcess::IsAvailable(_MAGIC_TABLE *pSkill)
{
	CUser* pParty = NULL;   // When the target is a party....
	bool isNPC = (m_sCasterID >= NPC_BAND);		// Identifies source : TRUE means source is NPC.

	int modulator = 0, Class = 0, moral = 0, skill_mod = 0 ;

	if (m_sTargetID >= 0 && m_sTargetID < MAX_USER) 
		moral = m_pSkillTarget->GetNation();
	else if (m_sTargetID >= NPC_BAND)     // Target existence check routine for NPC.          	
	{
		if (m_pSkillTarget == NULL || m_pSkillTarget->isDead())
			goto fail_return;	//... Assuming NPCs can't be resurrected.

		moral = m_pSkillTarget->GetNation();
	}
	else if (m_sTargetID == -1)  // AOE/Party Moral check routine.
	{
		if (isNPC)
		{
			moral = 1;
		}
		else
		{
			if (pSkill->bMoral == MORAL_AREA_ENEMY)
				moral = m_pSkillCaster->GetNation() == KARUS ? ELMORAD : KARUS;
			else 
				moral = m_pSkillCaster->GetNation();	
		}
	}
	else 
		moral = m_pSrcUser->GetNation();

	switch( pSkill->bMoral ) {		// Compare morals between source and target character.
		case MORAL_SELF:   // #1         // ( to see if spell is cast on the right target or not )
			if (m_pSkillCaster != m_pSkillTarget)
				goto fail_return;
			break;
		case MORAL_FRIEND_WITHME:	// #2
			if (m_pSkillCaster->GetNation() != moral)
				goto fail_return;
			break;
		case MORAL_FRIEND_EXCEPTME:	   // #3
			if (m_pSkillCaster->GetNation() != moral
				|| m_pSkillCaster == m_pSkillTarget)
				goto fail_return;
			break;
		case MORAL_PARTY:	 // #4
		{
			// NPCs can't *be* in parties.
			if (m_pSkillCaster->isNPC()
				|| (m_pSkillTarget != NULL && m_pSkillTarget->isNPC()))
				goto fail_return;

			// We're definitely a user, so...
			CUser *pCaster = static_cast<CUser *>(m_pSkillCaster);

			// If the caster's not in a party, make sure the target's not someone other than themselves.
			if ((!pCaster->isInParty() && m_pSkillCaster != m_pSkillTarget)
				// Verify that the nation matches the intended moral
				|| pCaster->GetNation() != moral
				// and that if there is a target, they're in the same party.
				|| (m_pSkillTarget != NULL && 
					static_cast<CUser *>(m_pSkillTarget)->m_sPartyIndex != pCaster->m_sPartyIndex))
				goto fail_return;
		} break;
		case MORAL_NPC:		// #5
			if (m_pSkillTarget == NULL
				|| !m_pSkillTarget->isNPC()
				|| m_pSkillTarget->GetNation() != moral)
				goto fail_return;
			break;
		case MORAL_PARTY_ALL:     // #6
//			if ( !m_pSrcUser->isInParty() ) goto fail_return;		
//			if ( !m_pSrcUser->isInParty() && sid != tid) goto fail_return;					

			break;
		case MORAL_ENEMY:	// #7
			if (m_pSkillCaster->GetNation() == moral)
				goto fail_return;
			break;	
		case MORAL_ALL:	 // #8
			// N/A
			break;
		case MORAL_AREA_ENEMY:		// #10
			// N/A
			break;
		case MORAL_AREA_FRIEND:		// #11
			if (m_pSkillCaster->GetNation() != moral)
				goto fail_return;
			break;
		case MORAL_AREA_ALL:	// #12
			// N/A
			break;
		case MORAL_SELF_AREA:     // #13
			// Remeber, EVERYONE in the area is affected by this one. No moral check!!!
			break;
		case MORAL_CORPSE_FRIEND:		// #25
			if (m_pSkillCaster->GetNation() != moral
				// We need to revive *something*.
				|| m_pSkillTarget == NULL
				// We cannot revive ourselves.
				|| m_pSkillCaster == m_pSkillTarget
				// We can't revive living targets.
				|| m_pSkillTarget->isAlive())
				goto fail_return;
			break;
		case MORAL_CLAN:		// #14
		{
			// NPCs cannot be in clans.
			if (m_pSkillCaster->isNPC()
				|| (m_pSkillTarget != NULL && m_pSkillTarget->isNPC()))
				goto fail_return;

			// We're definitely a user, so....
			CUser * pCaster = static_cast<CUser *>(m_pSkillCaster);

			// If the caster's not in a clan, make sure the target's not someone other than themselves.
			if ((!pCaster->isInClan() && m_pSkillCaster != m_pSkillTarget)
				// Verify the intended moral
				|| pCaster->GetNation() != moral
				// If we're targeting someone, that target must be in our clan.
				|| (m_pSkillTarget != NULL 
					&& static_cast<CUser *>(m_pSkillTarget)->m_pUserData->m_bKnights != pCaster->m_pUserData->m_bKnights))
				goto fail_return;
		} break;

		case MORAL_CLAN_ALL:	// #15
			break;
//
	}

	// This only applies to users casting skills. NPCs are fine and dandy (we can trust THEM at least).
	if (m_pSrcUser != NULL)
	{
		modulator = pSkill->sSkill % 10;     // Hacking prevention!
		if( modulator != 0 ) {	
			Class = pSkill->sSkill / 10;
			if( Class != m_pSrcUser->m_pUserData->m_sClass ) goto fail_return;
			if( pSkill->sSkillLevel > m_pSrcUser->m_pUserData->m_bstrSkill[modulator] ) goto fail_return;
		}
		else if (pSkill->sSkillLevel > m_pSrcUser->GetLevel()) goto fail_return;

		if (pSkill->bType[0] == 1) {	// Weapons verification in case of COMBO attack (another hacking prevention).
			if (pSkill->sSkill == 1055 || pSkill->sSkill == 2055) {		// Weapons verification in case of dual wielding attacks !		
				_ITEM_TABLE *pLeftHand = m_pSrcUser->GetItemPrototype(LEFTHAND),
							*pRightHand = m_pSrcUser->GetItemPrototype(RIGHTHAND);

				if ((pLeftHand != NULL && !pLeftHand->isSword() && !pLeftHand->isAxe() && !pLeftHand->isMace())
					|| (pRightHand != NULL && !pRightHand->isSword() && !pRightHand->isAxe() && !pRightHand->isMace()))
					return false;
			}
			else if (pSkill->sSkill == 1056 || pSkill->sSkill == 2056) {	// Weapons verification in case of 2 handed attacks !
				_ITEM_TABLE	*pRightHand = m_pSrcUser->GetItemPrototype(RIGHTHAND);

				if (m_pSrcUser->GetItem(LEFTHAND)->nNum != 0
					|| (pRightHand != NULL 
						&& !pRightHand->isSword() && !pRightHand->isAxe() 
						&& !pRightHand->isMace() && !pRightHand->isSpear()))
					return false;
			}
		}

		if (m_opcode == MAGIC_EFFECTING) {    // MP/SP SUBTRACTION ROUTINE!!! ITEM AND HP TOO!!!	
			int total_hit = m_pSrcUser->m_sTotalHit ;
			int skill_mana = (pSkill->sMsp * total_hit) / 100 ; 

			if (pSkill->bType[0] == 2 && pSkill->bFlyingEffect != 0) // Type 2 related...
				return true;		// Do not reduce MP/SP when flying effect is not 0.

			if (pSkill->bType[0] == 1 && m_sData1 > 1)
				return true;		// Do not reduce MP/SP when combo number is higher than 0.
 
			if (pSkill->bType[0] == 1 || pSkill->bType[0] == 2)
			{
				if (skill_mana > m_pSrcUser->m_pUserData->m_sMp)
					goto fail_return;
			}
			else if (pSkill->sMsp > m_pSrcUser->m_pUserData->m_sMp)
				goto fail_return;

			if (pSkill->bType[0] == 3 || pSkill->bType[0] == 4) {   // If the PLAYER uses an item to cast a spell.
				if (m_sCasterID >= 0 && m_sCasterID < MAX_USER)
				{	
					if (pSkill->iUseItem != 0) {
						_ITEM_TABLE* pItem = NULL;				// This checks if such an item exists.
						pItem = g_pMain->GetItemPtr(pSkill->iUseItem);
						if( !pItem ) return false;

						if ((pItem->m_bRace != 0 && m_pSrcUser->m_pUserData->m_bRace != pItem->m_bRace)
							|| (pItem->m_bClass != 0 && !m_pSrcUser->JobGroupCheck(pItem->m_bClass))
							|| (pItem->m_bReqLevel != 0 && m_pSrcUser->GetLevel() < pItem->m_bReqLevel)
							|| (!m_pSrcUser->RobItem(pSkill->iUseItem, 1)))	
							return false;
					}
				}
			}
			if (pSkill->bType[0] == 1 || pSkill->bType[0] == 2)	// Actual deduction of Skill or Magic point.
				m_pSrcUser->MSpChange(-(skill_mana));
			else if (pSkill->bType[0] != 4 || (pSkill->bType[0] == 4 && m_sTargetID == -1))
				m_pSrcUser->MSpChange(-(pSkill->sMsp));

			if (pSkill->sHP > 0 && pSkill->sMsp == 0) {			// DEDUCTION OF HPs in Magic/Skill using HPs.
				if (pSkill->sHP > m_pSrcUser->m_pUserData->m_sHp) goto fail_return;
				m_pSrcUser->HpChange(-pSkill->sHP);
			}
		}
	}

	return true;      // Magic was successful! 

fail_return:    // In case of failure, send a packet(!)
	if (!isNPC)
		SendSkillFailed();

	return false;     // Magic was a failure!
}

bool CMagicProcess::ExecuteType1(_MAGIC_TABLE *pSkill)
{	
	int damage = 0;
	bool bResult = false;

	_MAGIC_TYPE1* pType = g_pMain->m_Magictype1Array.GetData(pSkill->iNum);
	if (pType == NULL)
		return false;

	if (m_pSkillTarget != NULL && !m_pSkillTarget->isDead())
	{
		bResult = 1;
		damage = m_pSkillCaster->GetDamage(m_pSkillTarget, pSkill);
		m_pSkillTarget->HpChange(-damage, m_pSkillCaster);

		// This is more than a little ugly.
		if (m_pSkillCaster->isPlayer())
			static_cast<CUser *>(m_pSkillCaster)->SendTargetHP(0, m_sTargetID, -damage);
	}

	if(m_pSkillCaster->isPlayer() && m_pSrcUser->m_bInvisibilityType != 0) //If we're allowing monsters to be stealthed too (it'd be cool) then this check needs to be changed.
		m_pSrcUser->StateChangeServerDirect(7, INVIS_NONE);

	// This should only be sent once. I don't think there's reason to enforce this, as no skills behave otherwise
	m_sData4 = (damage == 0 ? -104 : 0);
	SendSkill();	

	return bResult;
}

bool CMagicProcess::ExecuteType2(_MAGIC_TABLE *pSkill)
{		
	int damage = 0;
	bool bResult = false;
	float total_range = 0.0f;	// These variables are used for range verification!
	int sx, sz, tx, tz;

	_MAGIC_TYPE2 *pType = g_pMain->m_Magictype2Array.GetData(pSkill->iNum);
	if (pType == NULL
		// The user does not have enough arrows! We should point them in the right direction. ;)
		|| !m_pSrcUser->CheckExistItem(pSkill->iUseItem, pType->bNeedArrow))
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
	if (m_pSkillTarget == NULL || m_pSkillTarget->isDead())
		goto packet_send;
	
	total_range = pow(((pType->sAddRange * pTable->m_sRange) / 100.0f), 2.0f) ;     // Range verification procedure.
	sx = (int)m_pSkillCaster->GetX(); tx = (int)m_pSkillTarget->GetX();
	sz = (int)m_pSkillCaster->GetZ(); tz = (int)m_pSkillTarget->GetZ();
	
	if ((pow((float)(sx - tx), 2.0f) + pow((float)(sz - tz), 2.0f)) > total_range)	   // Target is out of range, exit.
		goto packet_send;
	
	damage = m_pSkillCaster->GetDamage(m_pSkillTarget, pSkill);  // Get damage points of enemy.	

	m_pSkillTarget->HpChange(-damage, m_pSrcUser);     // Reduce target health point.

	// This is more than a little ugly.
	if (m_pSkillCaster->isPlayer())
		static_cast<CUser *>(m_pSkillCaster)->SendTargetHP(0, m_sTargetID, -damage);     // Change the HP of the target.

packet_send:
	if(m_pSkillCaster->isPlayer() && m_pSrcUser->m_bInvisibilityType != 0) //If we're allowing monsters to be stealthed too (it'd be cool) then this check needs to be changed.
		m_pSrcUser->StateChangeServerDirect(7, INVIS_NONE);
	// This should only be sent once. I don't think there's reason to enforce this, as no skills behave otherwise
	m_sData4 = (damage == 0 ? -104 : 0);
	SendSkill();

	return bResult;
}

bool CMagicProcess::ExecuteType3(_MAGIC_TABLE *pSkill)  // Applied when a magical attack, healing, and mana restoration is done.
{	
	int damage = 0, duration_damage = 0;
	vector<Unit *> casted_member;

	_MAGIC_TYPE3* pType = g_pMain->m_Magictype3Array.GetData(pSkill->iNum);
	if (pType == NULL) 
		return false;

	// If the target's a group of people...
	if (m_sTargetID == -1)
	{
		// TO-DO: Make this not completely and utterly suck (i.e. kill that loop!).
		SessionMap & sessMap = g_pMain->s_socketMgr.GetActiveSessionMap();
		foreach (itr, sessMap)
		{		
			CUser* pTUser = static_cast<CUser *>(itr->second);
			if (!pTUser->isDead() && !pTUser->isBlinking()
				&& UserRegionCheck(m_sCasterID, pTUser->GetSocketID(), pSkill->iNum, pType->bRadius, m_sData1, m_sData3))
				casted_member.push_back(pTUser);
		}
		g_pMain->s_socketMgr.ReleaseLock();

		if (casted_member.empty())
		{
			SendSkillFailed();
			return false;			
		}
	}
	else
	{	// If the target was a single unit.
		if (m_pSkillTarget == NULL 
			|| m_pSkillTarget->isDead() 
			|| (m_pSkillTarget->isPlayer() 
				&& static_cast<CUser *>(m_pSkillTarget)->isBlinking())) 
			return false;
		
		casted_member.push_back(m_pSkillTarget);
	}
	
	foreach (itr, casted_member)
	{
		// assume player for now
		CUser* pTUser = static_cast<CUser *>(*itr); // it's checked above, not much need to check it again
		if ((pType->sFirstDamage < 0) && (pType->bDirectType == 1) && (pSkill->iNum < 400000))	// If you are casting an attack spell.
			damage = GetMagicDamage(pTUser, pType->sFirstDamage, pType->bAttribute) ;	// Get Magical damage point.
		else 
			damage = pType->sFirstDamage;

		if( m_pSrcUser )	{
			if( m_pSrcUser->m_pUserData->m_bZone == ZONE_SNOW_BATTLE && g_pMain->m_byBattleOpen == SNOW_BATTLE )
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
					damage = (pType->sFirstDamage * pTUser->m_pUserData->m_sHp) / -100;
				else
					damage = (pTUser->m_iMaxHp * (pType->sFirstDamage - 100)) / 100;

				pTUser->HpChange(damage, m_pSrcUser);
				m_pSrcUser->SendTargetHP( 0, (*itr)->GetID(), damage );
			}
		}
		else if (pType->bDuration != 0) {    // Durational Spells! Remember, durational spells only involve HPs.
			if (damage != 0) {		// In case there was first damage......
				pTUser->HpChange(damage, m_pSrcUser);			// Initial damage!!!
				m_pSrcUser->SendTargetHP(0, pTUser->GetSocketID(), damage );     // Change the HP of the target. 
			}

			if (pTUser->m_bResHpType != USER_DEAD) {	// ���⵵ ��ȣ �ڵ� �߽�...
				if (pType->sTimeDamage < 0) {
					duration_damage = GetMagicDamage(pTUser, pType->sTimeDamage, pType->bAttribute) ;
				}
				else duration_damage = pType->sTimeDamage ;

				for (int k = 0 ; k < MAX_TYPE3_REPEAT ; k++) {	// For continuous damages...
					if (pTUser->m_bHPInterval[k] == 5) {
						pTUser->m_fHPStartTime[k] = pTUser->m_fHPLastTime[k] = TimeGet();     // The durational magic routine.
						pTUser->m_bHPDuration[k] = pType->bDuration;
						pTUser->m_bHPInterval[k] = 2;		
						pTUser->m_bHPAmount[k] = duration_damage / ( pTUser->m_bHPDuration[k] / pTUser->m_bHPInterval[k] ) ;
						pTUser->m_sSourceID[k] = m_sCasterID;
						break;
					}
				}

				pTUser->m_bType3Flag = TRUE;
			}

			if (pTUser->isInParty() && pType->sTimeDamage < 0)
				pTUser->SendPartyStatusUpdate(1, 1);

			pTUser->SendUserStatusUpdate(pType->bAttribute == POISON_R ? USER_STATUS_POISON : USER_STATUS_DOT, 1);
		}

		if (m_pSkillCaster->isPlayer() //If we're allowing monsters to be stealthed too (it'd be cool) then this check needs to be changed.
			&& m_pSrcUser->m_bInvisibilityType != 0 
			&& damage < 0) //To allow for minor healing (as rogues)
			m_pSrcUser->StateChangeServerDirect(7, INVIS_NONE);
	
		if ( pSkill->bType[1] == 0 || pSkill->bType[1] == 3 )
			SendSkill(m_sCasterID, pTUser->GetSocketID());
		
		// Tell the AI server we're healing someone (so that they can choose to pick on the healer!)
		if (pType->bDirectType == 1 && damage > 0
			&& m_sCasterID != m_sTargetID)
		{

			Packet result(AG_HEAL_MAGIC);
			result << m_sCasterID;
			g_pMain->Send_AIServer(&result);
		}
	}

	return true;
}

bool CMagicProcess::ExecuteType4(_MAGIC_TABLE *pSkill)
{
	int damage = 0;

	vector<CUser *> casted_member;
	if (pSkill == NULL)
		return false;

	_MAGIC_TYPE4* pType = g_pMain->m_Magictype4Array.GetData(pSkill->iNum);
	if (pType == NULL)
		return false;

	if (m_sTargetID == -1)
	{
		// TO-DO: Localise this. This is horribly unnecessary.
		SessionMap & sessMap = g_pMain->s_socketMgr.GetActiveSessionMap();
		foreach (itr, sessMap)
		{		
			CUser* pTUser = static_cast<CUser *>(itr->second);
			if (!pTUser->isDead() && !pTUser->isBlinking()
				&& UserRegionCheck(m_sCasterID, pTUser->GetSocketID(), pSkill->iNum, pType->bRadius, m_sData1, m_sData3))
				casted_member.push_back(pTUser);
		}
		g_pMain->s_socketMgr.ReleaseLock();

		if (casted_member.empty())
		{		
			if (m_sCasterID >= 0 && m_sCasterID < MAX_USER) 
				SendSkillFailed();

			return false;
		}
	}
	else 
	{
		// If the target was another single player.
		CUser* pTUser = g_pMain->GetUserPtr(m_sTargetID);
		if (pTUser == NULL 
			|| pTUser->isDead() || pTUser->isBlinking()) 
			return false;

		casted_member.push_back(pTUser);
	}

	foreach (itr, casted_member)
	{
		uint8 bResult = 1;
		CUser* pTUser = *itr;
//
		if (pTUser->m_bType4Buff[pType->bBuffType - 1] == 2 && m_sTargetID == -1) {		// Is this buff-type already casted on the player?
			bResult = 0;
			goto fail_return ;					
		}
//
		//if ( data4 == -1 ) //Need to create InsertSaved Magic before enabling this.
			//pTUser->InsertSavedMagic( magicid, pType->sDuration );

		switch (pType->bBuffType)
		{
			case BUFF_TYPE_HP_MP:
				if (pType->sMaxHP == 0)
					pTUser->m_sMaxHPAmount = (pType->sMaxHPPct - 100) * (pTUser->m_iMaxHp - pTUser->m_sMaxHPAmount) / 100;
				else
					pTUser->m_sMaxHPAmount = pType->sMaxHP;

				if (pType->sMaxMP == 0)
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
				if (pSkill->iNum == 490034)	// Bezoar!!!
					pTUser->StateChangeServerDirect(3, ABNORMAL_GIANT); 
				else if (pSkill->iNum == 490035)	// Rice Cake!!!
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
				pTUser->SendUserStatusUpdate(USER_STATUS_BLIND, 1);
				break;

			default:
				bResult = 0;
				goto fail_return;
		}


		pTUser->m_sDuration[pType->bBuffType - 1] = pType->sDuration;
		pTUser->m_fStartTime[pType->bBuffType - 1] = TimeGet();

		if (m_sTargetID != -1 && pSkill->bType[0] == 4)
		{
			if (m_sCasterID >= 0 && m_sCasterID < MAX_USER)
				m_pSrcUser->MSpChange( -(pSkill->sMsp) );
		}

		if (m_sCasterID >= 0 && m_sCasterID < MAX_USER) 
			pTUser->m_bType4Buff[pType->bBuffType - 1] = (m_pSrcUser->GetNation() == pTUser->GetNation() ? 2 : 1);
		else
			pTUser->m_bType4Buff[pType->bBuffType - 1] = 1;

		pTUser->m_bType4Flag = TRUE;

		pTUser->SetSlotItemValue();				// Update character stats.
		pTUser->SetUserAbility();

		if(m_sCasterID == m_sTargetID)
			pTUser->SendItemMove(2);

		if (pTUser->isInParty() && pTUser->m_bType4Buff[pType->bBuffType - 1] == 1)
			pTUser->SendPartyStatusUpdate(2, 1);

		pTUser->Send2AI_UserUpdateInfo();

	fail_return:
		if (pSkill->bType[1] == 0 || pSkill->bType[1] == 4)
		{
			CUser *pUser = (m_sCasterID >= 0 && m_sCasterID < MAX_USER ? m_pSrcUser : pTUser);

			m_sTargetID = (*itr)->GetSocketID();
			m_sData2 = bResult;
			m_sData4 = (bResult == 1|| m_sData4 == 0 ? pType->sDuration : 0);
			m_sData6 = pType->bSpeed;
			pUser->m_MagicProcess.SendSkill();

			if (pSkill->bMoral >= MORAL_ENEMY)
				pTUser->SendUserStatusUpdate(pType->bBuffType == BUFF_TYPE_SPEED ? USER_STATUS_SPEED : USER_STATUS_POISON, 1);
		}
		
		if (bResult == 0
			&& m_sCasterID >= 0 && m_sCasterID < MAX_USER)
			SendSkill(m_sCasterID, (*itr)->GetSocketID(), MAGIC_FAIL);

		continue;
	}
	return true;
}

bool CMagicProcess::ExecuteType5(_MAGIC_TABLE *pSkill)
{
	// Disallow NPCs (for now?).
	if (m_pSkillCaster->isNPC()
		|| m_pSkillTarget->isNPC())
		return false;

	int damage = 0;
	int buff_test = 0; BOOL bType3Test = TRUE, bType4Test = TRUE; 	

	if (pSkill == NULL)
		return false;

	_MAGIC_TYPE5* pType = g_pMain->m_Magictype5Array.GetData(pSkill->iNum);
	if (pType == NULL
		|| m_pSkillTarget == NULL 
		|| (m_pSkillTarget->isDead() && pType->bType != RESURRECTION) 
		|| (!m_pSkillTarget->isDead() && pType->bType == RESURRECTION)) 
		return false;

	switch (pType->bType) 
	{
		case REMOVE_TYPE3:		// REMOVE TYPE 3!!!
			for (int i = 0; i < MAX_TYPE3_REPEAT; i++)
			{
				if (m_pSkillTarget->m_bHPAmount[i] < 0) {
					m_pSkillTarget->m_fHPStartTime[i] = 0.0f;
					m_pSkillTarget->m_fHPLastTime[i] = 0.0f;   
					m_pSkillTarget->m_bHPAmount[i] = 0;
					m_pSkillTarget->m_bHPDuration[i] = 0;				
					m_pSkillTarget->m_bHPInterval[i] = 5;
					m_pSkillTarget->m_sSourceID[i] = -1;

					if (m_pSkillTarget->isPlayer())
					{
						// TO-DO: Wrap this up (ugh, I feel so dirty)
						Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE3_END));
						result << uint8(200); // removes all
						static_cast<CUser *>(m_pSkillTarget)->Send(&result); 
					}
				}
			}

			buff_test = 0;
			for (int j = 0; j < MAX_TYPE3_REPEAT; j++)
				buff_test += m_pSkillTarget->m_bHPDuration[j];
			if (buff_test == 0) m_pSkillTarget->m_bType3Flag = FALSE;	

			// Check for Type 3 Curses.
			for (int k = 0; k < MAX_TYPE3_REPEAT; k++)
			{
				if (m_pSkillTarget->m_bHPAmount[k] < 0)
				{
					bType3Test = FALSE;
					break;
				}
			}
  
			if (m_pSkillTarget->isPlayer()
				&& static_cast<CUser *>(m_pSkillTarget)->isInParty() && bType3Test)
				static_cast<CUser *>(m_pSkillTarget)->SendPartyStatusUpdate(1);
			break;

		case REMOVE_TYPE4:		// REMOVE TYPE 4!!!
			for (int i = 0; i < MAX_TYPE4_BUFF; i++)
			{
				if (m_pSkillTarget->m_bType4Buff[i] == 0)
					continue;

				uint8 buff_type = i + 1;

				switch (buff_type)
				{
				case 1: 
					m_pSkillTarget->m_sMaxHPAmount = 0;
					m_pSkillTarget->m_sMaxMPAmount = 0;
					break;

				case 2:
					m_pSkillTarget->m_sACAmount = 0;
					break;

				case 3:
					if (m_pSkillTarget->isPlayer())
						static_cast<CUser *>(m_pSkillTarget)->StateChangeServerDirect(3, ABNORMAL_NORMAL);
					break;

				case 4:
					m_pSkillTarget->m_bAttackAmount = 100;
					break;

				case 5:
					m_pSkillTarget->m_bAttackSpeedAmount = 100;
					break;

				case 6:
					m_pSkillTarget->m_bSpeedAmount = 100;
					break;

				case 7:
					if (m_pSkillTarget->isPlayer())
						memset(static_cast<CUser *>(m_pSkillTarget)->m_sStatItemBonuses, 0, sizeof(uint16) * STAT_COUNT);
					break;

				case 8:
					m_pSkillTarget->m_bFireRAmount = m_pSkillTarget->m_bColdRAmount = m_pSkillTarget->m_bLightningRAmount = 0;
					m_pSkillTarget->m_bMagicRAmount = m_pSkillTarget->m_bDiseaseRAmount = m_pSkillTarget->m_bPoisonRAmount = 0;
					break;

				case 9:
					m_pSkillTarget->m_bHitRateAmount = 100;
					m_pSkillTarget->m_sAvoidRateAmount = 100;
					break;
				}

				m_pSkillTarget->m_bType4Buff[i] = 0;
				SendType4BuffRemove(m_sTargetID, buff_type);
			}

			if (m_pSkillTarget->isPlayer())
			{
				static_cast<CUser *>(m_pSkillTarget)->SetSlotItemValue();
				static_cast<CUser *>(m_pSkillTarget)->SetUserAbility();
				static_cast<CUser *>(m_pSkillTarget)->Send2AI_UserUpdateInfo();
			}

			buff_test = 0;
			for (int i = 0; i < MAX_TYPE4_BUFF; i++)
				buff_test += m_pSkillTarget->m_bType4Buff[i];
			if (buff_test == 0) m_pSkillTarget->m_bType4Flag = FALSE;

			bType4Test = TRUE ;
			for (int j = 0; j < MAX_TYPE4_BUFF; j++)
			{
				if (m_pSkillTarget->m_bType4Buff[j] == 1)
				{
					bType4Test = FALSE;
					break;
				}
			}

			if (m_pSkillTarget->isPlayer() && static_cast<CUser *>(m_pSkillTarget)->isInParty() && bType4Test)
				static_cast<CUser *>(m_pSkillTarget)->SendPartyStatusUpdate(2, 0);
			break;
			
		case RESURRECTION:		// RESURRECT A DEAD PLAYER!!!
			if (m_pSkillTarget->isPlayer())
				static_cast<CUser *>(m_pSkillTarget)->Regene(1, m_nSkillID);
			break;

		case REMOVE_BLESS:
			if (m_pSkillTarget->m_bType4Buff[0] == 2) {
				m_pSkillTarget->m_sDuration[0] = 0;		
				m_pSkillTarget->m_fStartTime[0] = 0.0f;
				m_pSkillTarget->m_sMaxHPAmount = 0;
				m_pSkillTarget->m_sMaxMPAmount = 0;
				m_pSkillTarget->m_bType4Buff[0] = 0;

				SendType4BuffRemove(m_sTargetID, 1);

				if (m_pSkillTarget->isPlayer())
				{
					static_cast<CUser *>(m_pSkillTarget)->SetSlotItemValue();
					static_cast<CUser *>(m_pSkillTarget)->SetUserAbility();
					static_cast<CUser *>(m_pSkillTarget)->Send2AI_UserUpdateInfo();
				}

				buff_test = 0;
				for (int i = 0; i < MAX_TYPE4_BUFF; i++)
					buff_test += m_pSkillTarget->m_bType4Buff[i];
				if (buff_test == 0) m_pSkillTarget->m_bType4Flag = FALSE;

				bType4Test = TRUE;
				for (int j = 0; j < MAX_TYPE4_BUFF; j++)
				{
					if (m_pSkillTarget->m_bType4Buff[j] == 1)
					{
						bType4Test = FALSE;
						break;
					}
				}

				if (m_pSkillTarget->isPlayer() && static_cast<CUser *>(m_pSkillTarget)->isInParty() && bType4Test) 
					static_cast<CUser *>(m_pSkillTarget)->SendPartyStatusUpdate(2, 0);
			}

			break;
	}

	if (pSkill->bType[1] == 0 || pSkill->bType[1] == 5)
		SendSkill();

	return true;
}

bool CMagicProcess::ExecuteType6(_MAGIC_TABLE *pSkill)
{
	_MAGIC_TYPE6 * pType = g_pMain->m_Magictype6Array.GetData(pSkill->iNum);
	uint32 iUseItem = 0;

	if (pType == NULL
		|| m_pSrcUser->isAttackZone()
		|| m_pSrcUser->isTransformed())
		return false;

	// Let's start by looking at the item that was used for the transformation.
	_ITEM_TABLE *pTable = g_pMain->GetItemPtr(m_pSrcUser->m_nTransformationItem);

	// Also, for the sake of specific skills that bypass the list, let's lookup the 
	// item attached to the skill.
	_ITEM_TABLE *pTable2 = g_pMain->GetItemPtr(pSkill->iUseItem);

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
		iUseItem = pSkill->iUseItem;
	// If we're using a normal transformation scroll, we can leave the item as it is.
	else 
		iUseItem = m_pSrcUser->m_nTransformationItem;

	// Attempt to take the item (no further checks, so no harm in multipurposing)
	// If we add more checks, remember to change this check.
	if (!m_pSrcUser->RobItem(iUseItem, 1))
		return false;

	// TO-DO : Save duration, and obviously end
	m_pSrcUser->m_fTransformationStartTime = TimeGet();
	m_pSrcUser->m_sTransformationDuration = pType->sDuration;

	m_pSrcUser->StateChangeServerDirect(3, pSkill->iNum);
	m_pSrcUser->m_bIsTransformed = true;

	// TO-DO : Give the users ALL TEH BONUSES!!

	SendSkill(m_sCasterID, m_sTargetID, m_opcode,
		m_nSkillID, m_sData1, 1, m_sData3, 0, 0, 0, 0, 0);

	return true;
}


bool CMagicProcess::ExecuteType7(_MAGIC_TABLE *pSkill)
{
	return true;
}

bool CMagicProcess::ExecuteType8(_MAGIC_TABLE *pSkill)	// Warp, resurrection, and summon spells.
{	
	if (pSkill == NULL)
		return false;

	vector<CUser *> casted_member;
	_MAGIC_TYPE8* pType = g_pMain->m_Magictype8Array.GetData(pSkill->iNum);
	if (pType == NULL)
		return false;

	if (m_sTargetID == -1)
	{
		// TO-DO: Localise this loop to make it not suck (the life out of the server).
		SessionMap & sessMap = g_pMain->s_socketMgr.GetActiveSessionMap();
		foreach (itr, sessMap)
		{		
			CUser* pTUser = static_cast<CUser *>(itr->second);
			if (UserRegionCheck(m_sCasterID, pTUser->GetSocketID(), pSkill->iNum, pType->sRadius, m_sData1, m_sData3))
				casted_member.push_back(pTUser);
		}
		g_pMain->s_socketMgr.ReleaseLock();

		if (casted_member.empty()) 
			return false;	
	}
	else 
	{	// If the target was another single player.
		CUser* pTUser = g_pMain->GetUserPtr(m_sTargetID);
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
		_HOME_INFO* pHomeInfo = g_pMain->m_HomeArray.GetData(pTUser->GetNation());
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
				pTUser->m_MagicProcess.SendSkill(m_sCasterID, (*itr)->GetSocketID(), 
					m_opcode, m_nSkillID, m_sData1, 1, m_sData3);

				if (pTUser->GetMap() == NULL)
					continue;

				pEvent = pTUser->GetMap()->GetObjectEvent(pTUser->m_pUserData->m_sBind);

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
				else if (pTUser->m_pUserData->m_bZone == ZONE_BATTLE)
					pTUser->Warp(uint16((pHomeInfo->BattleZoneX + x) * 10), uint16((pHomeInfo->BattleZoneZ + z) * 10));	
				else if (pTUser->m_pUserData->m_bZone == ZONE_FRONTIER)
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
				pTUser->m_MagicProcess.SendSkill(m_sCasterID, (*itr)->GetSocketID(), 
					m_opcode, m_nSkillID, m_sData1, 1, m_sData3);

				pTUser->m_bResHpType = USER_STANDING;     // Target status is officially alive now.
				pTUser->HpChange(pTUser->m_iMaxHp, m_pSrcUser);	 // Refill HP.	
				pTUser->ExpChange( pType->sExpRecover/100 );     // Increase target experience.
				
				Packet result(AG_USER_REGENE);
				result << uint16((*itr)->GetSocketID()) << uint16(pTUser->GetZoneID());
				g_pMain->Send_AIServer(&result);
			} break;

			case 12:	// Summon a target within the zone.	
				if (m_pSrcUser->GetZoneID() != pTUser->GetZoneID())   // Same zone? 
					goto packet_send;

				pTUser->m_MagicProcess.SendSkill(m_sCasterID, (*itr)->GetSocketID(), 
					m_opcode, m_nSkillID, m_sData1, 1, m_sData3);
					
				pTUser->Warp(m_pSrcUser->GetSPosX(), m_pSrcUser->GetSPosZ());
				break;

			case 13:	// Summon a target outside the zone.			
				if (m_pSrcUser->GetZoneID() == pTUser->GetZoneID())	  // Different zone? 
					goto packet_send;

				pTUser->m_MagicProcess.SendSkill(m_sCasterID, (*itr)->GetSocketID(), 
					m_opcode, m_nSkillID, m_sData1, 1, m_sData3);

				pTUser->ZoneChange(m_pSrcUser->GetZoneID(), m_pSrcUser->m_pUserData->m_curx, m_pSrcUser->m_pUserData->m_curz) ;
				pTUser->UserInOut(INOUT_RESPAWN);
				//TRACE(" Summon ,, name=%s, x=%.2f, z=%.2f\n", pTUser->m_pUserData->m_id, pTUser->m_pUserData->m_curx, pTUser->m_pUserData->m_curz);
				break;

			case 20:	// Randomly teleport the source (within 20 meters)		
				pTUser->m_MagicProcess.SendSkill(m_sCasterID, (*itr)->GetSocketID(), 
					m_opcode, m_nSkillID, m_sData1, 1, m_sData3);

				float warp_x, warp_z;		// Variable Initialization.
				float temp_warp_x, temp_warp_z;

				warp_x = pTUser->m_pUserData->m_curx;	// Get current locations.
				warp_z = pTUser->m_pUserData->m_curz;

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
				if (pSkill->bMoral < MORAL_ENEMY && pTUser->GetNation() != m_pSrcUser->GetNation()) //I'm not the same nation as you are and thus can't t
					return false;
					
				dest_x = pTUser->m_pUserData->m_curx;
				dest_z = pTUser->m_pUserData->m_curz;

				m_pSrcUser->Warp(uint16(dest_x * 10), uint16(dest_z * 10));
				break;
		}

		bResult = 1;
		
packet_send:
		m_sData2 = bResult;
		SendSkill(m_sCasterID, (*itr)->GetSocketID());
	}
	return true;
}

bool CMagicProcess::ExecuteType9(_MAGIC_TABLE *pSkill)
{
	_MAGIC_TYPE9* pType = g_pMain->m_Magictype9Array.GetData(pSkill->iNum);
	if (pType == NULL)
		return false;

	m_sData2 = 1;
	/*
	if(!InTheSavedSkillList)
	{
		m_sData2 = 1;
	}
	else
	{
		m_sData2 = 0;
		SendSkillFailed();
		return false;
	}
	*/

	if (pType->bStateChange <= 2)
		m_pSrcUser->StateChangeServerDirect(7, pType->bStateChange); //Update the client to be invisible
	else if (pType->bStateChange == 3)
		m_pSrcUser->m_bCanSeeStealth = true;
	else if (pType->bStateChange == 4)
	{
		if(m_pSrcUser->isInParty())
		{
			_PARTY_GROUP* pParty = g_pMain->m_PartyArray.GetData(m_pSrcUser->m_sPartyIndex);
			if (pParty == NULL)
				return false;

			for (int i = 0; i < MAX_PARTY_USERS; i++)
			{
				CUser *pUser = g_pMain->GetUserPtr(pParty->uid[i]);
				if (pUser == NULL)
					continue;

				pUser->m_bCanSeeStealth = true;
			}
		}
	}

	//TO-DO : Save the skill in the saved skill list

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_EFFECTING));
	result	<< m_nSkillID << m_sCasterID << m_sTargetID
			<< m_sData1 << m_sData2 << m_sData3 << m_sData4 << m_sData5 << m_sData6 << m_sData7 << m_sData8;
	if(m_pSrcUser->isInParty() && pType->bStateChange == 4)
		g_pMain->Send_PartyMember(m_pSrcUser->m_sPartyIndex, &result);
	else
		m_pSrcUser->Send(&result);

	return true;
}

short CMagicProcess::GetMagicDamage(Unit *pTarget, int total_hit, int attribute)
{	
	short damage = 0, temp_hit = 0, righthand_damage = 0, attribute_damage = 0 ; 
	int random = 0, total_r = 0 ;
	BYTE result; 

	if (pTarget->isDead()
		|| m_pSkillCaster->isDead())
		return 0;

	uint16 sMagicAmount = 0;
	if (m_pSkillCaster->isNPC())
	{
		result = m_pSkillCaster->GetHitRate(pTarget->m_sTotalHitrate / m_pSkillCaster->m_sTotalEvasionrate); 
	}
	else
	{
		CUser *pUser = static_cast<CUser *>(m_pSkillCaster);
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
		
		if (m_pSkillCaster->isPlayer()) 
		{
			CUser *pUser = static_cast<CUser *>(m_pSkillCaster);
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

		if (m_pSkillCaster->isNPC())
			damage -= ((3 * righthand_damage) + (3 * attribute_damage));
		else if (attribute != 4)	// Only if the staff has an attribute.
			damage -= (short)(((righthand_damage * 0.8f) + (righthand_damage * m_pSkillCaster->GetLevel()) / 60) + ((attribute_damage * 0.8f) + (attribute_damage * m_pSkillCaster->GetLevel()) / 30));
	}

	return damage / 3;		
}

BOOL CMagicProcess::UserRegionCheck(int sid, int tid, int magicid, int radius, short mousex, short mousez)
{
	CNpc* pMon = NULL;

	float currenttime = 0.0f;
	BOOL bFlag = FALSE;

	CUser* pTUser = g_pMain->GetUserPtr(tid);  
	if (pTUser == NULL) return FALSE;
	
	if (sid >= NPC_BAND) {					
		pMon = g_pMain->m_arNpcArray.GetData(sid);
		if( !pMon || pMon->m_NpcState == NPC_DEAD ) return FALSE;
		bFlag = TRUE;
	}

	_MAGIC_TABLE* pMagic = NULL;
	pMagic = g_pMain->m_MagictableArray.GetData( magicid );   // Get main magic table.
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
				currenttime = TimeGet();
				if (pTUser->m_pUserData->m_bZone == ZONE_BATTLE && (currenttime - pTUser->m_fLastRegeneTime < CLAN_SUMMON_TIME)) {
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
				if (pTUser->m_pUserData->m_bNation != m_pSrcUser->m_pUserData->m_bNation)		// Check that it's your enemy.
					goto final_test;
			}
			else {
				if (pTUser->m_pUserData->m_bNation != pMon->m_byGroup)
					goto final_test;
			}
			break;

		case MORAL_AREA_FRIEND :				
			if (pTUser->m_pUserData->m_bNation == m_pSrcUser->m_pUserData->m_bNation) 		// Check that it's your ally.
				goto final_test;
			break;
// �񷯸ӱ� Ŭ�� ��ȯ!!!
		case MORAL_CLAN_ALL :
			if( pTUser->m_pUserData->m_bKnights == -1) {
				if (sid == tid) {
					return TRUE; 
				}
				else {
					return FALSE; 
				}
			}			
/*
			if ( pTUser->m_pUserData->m_bKnights == m_pSrcUser->m_pUserData->m_bKnights) 
				goto final_test;
*/
			if ( pTUser->m_pUserData->m_bKnights == m_pSrcUser->m_pUserData->m_bKnights && pMagic->bType[0] != 8 ) {
				goto final_test;
			}
			else if (pTUser->m_pUserData->m_bKnights == m_pSrcUser->m_pUserData->m_bKnights && pMagic->bType[0] == 8) {
				currenttime = TimeGet();
				if (pTUser->m_pUserData->m_bZone == ZONE_BATTLE && (currenttime - pTUser->m_fLastRegeneTime < CLAN_SUMMON_TIME)) {
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
	// TO-DO: Reset stat changes, recalculate stats.
	m_pSrcUser->m_bIsTransformed = false;
	m_pSrcUser->Send(&result);
	m_pSrcUser->StateChangeServerDirect(3, ABNORMAL_NORMAL); 
}

void CMagicProcess::Type4Cancel(_MAGIC_TABLE * pSkill)
{
	if (m_pSkillTarget != m_pSkillCaster)
		return;

	_MAGIC_TYPE4* pType = g_pMain->m_Magictype4Array.GetData(pSkill->iNum);
	if (pType == NULL)
		return;

	BOOL buff = FALSE;
	switch (pType->bBuffType)
	{
		case BUFF_TYPE_HP_MP:
			if (m_pSkillCaster->m_sMaxHPAmount > 0
				|| m_pSkillCaster->m_sMaxMPAmount > 0)
			{
				m_pSkillCaster->m_sMaxHPAmount = 0;
				m_pSkillCaster->m_sMaxMPAmount = 0;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_AC:
			if (m_pSkillCaster->m_sACAmount > 0) 
			{
				m_pSkillCaster->m_sACAmount = 0;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_SIZE:
			if (m_pSkillCaster->isPlayer())
			{
				static_cast<CUser *>(m_pSkillCaster)->StateChangeServerDirect(3, ABNORMAL_NORMAL);
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_DAMAGE:
			if (m_pSkillCaster->m_bAttackAmount > 100) 
			{
				m_pSkillCaster->m_bAttackAmount = 100;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_ATTACK_SPEED:
			if (m_pSkillCaster->m_bAttackSpeedAmount > 100) 
			{
				m_pSkillCaster->m_bAttackSpeedAmount = 100;	
				buff = TRUE;
			}
			break;	

		case BUFF_TYPE_SPEED:
			if (m_pSkillCaster->m_bSpeedAmount > 100)
			{
				m_pSkillCaster->m_bSpeedAmount = 100;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_STATS:
			if (m_pSkillCaster->isPlayer()
				&& static_cast<CUser *>(m_pSkillCaster)->getStatBuffTotal() > 0) {
				// TO-DO: Implement reset
				memset(static_cast<CUser *>(m_pSkillCaster)->m_bStatBuffs, 0, sizeof(uint8) * STAT_COUNT);
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_RESISTANCES:
			if ((m_pSkillCaster->m_bFireRAmount + m_pSkillCaster->m_bColdRAmount + m_pSkillCaster->m_bLightningRAmount +
				m_pSkillCaster->m_bMagicRAmount + m_pSkillCaster->m_bDiseaseRAmount + m_pSkillCaster->m_bPoisonRAmount) > 0) {
				m_pSkillCaster->m_bFireRAmount = 0;
				m_pSkillCaster->m_bColdRAmount = 0;
				m_pSkillCaster->m_bLightningRAmount = 0;
				m_pSkillCaster->m_bMagicRAmount = 0;
				m_pSkillCaster->m_bDiseaseRAmount = 0;
				m_pSkillCaster->m_bPoisonRAmount = 0;
				buff = TRUE;
			}
			break;	

		case BUFF_TYPE_ACCURACY:
			if ((m_pSkillCaster->m_bHitRateAmount + m_pSkillCaster->m_sAvoidRateAmount) > 200)
			{
				m_pSkillCaster->m_bHitRateAmount = 100;
				m_pSkillCaster->m_sAvoidRateAmount = 100;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_MAGIC_POWER:
			if (m_pSkillCaster->m_sMagicAttackAmount > 0)
			{
				m_pSkillCaster->m_sMagicAttackAmount = 0;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_EXPERIENCE:
			if (m_pSkillCaster->m_bExpGainAmount > 100)
			{
				m_pSkillCaster->m_bExpGainAmount = 100;
				buff = TRUE;
			}
			break;

		case BUFF_TYPE_WEIGHT:
			if (m_pSkillCaster->m_bMaxWeightAmount > 100)
			{
				m_pSkillCaster->m_bMaxWeightAmount = 100;
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
		m_pSkillCaster->m_sDuration[pType->bBuffType - 1] = 0;
		m_pSkillCaster->m_fStartTime[pType->bBuffType - 1] = 0.0f;
		m_pSkillCaster->m_bType4Buff[pType->bBuffType - 1] = 0;

		if (m_pSkillCaster->isPlayer())
		{
			static_cast<CUser *>(m_pSkillCaster)->SetSlotItemValue();
			static_cast<CUser *>(m_pSkillCaster)->SetUserAbility();
			static_cast<CUser *>(m_pSkillCaster)->SendItemMove(2);
			static_cast<CUser *>(m_pSkillCaster)->Send2AI_UserUpdateInfo();

			Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_END));
			result << pType->bBuffType;
			static_cast<CUser *>(m_pSkillCaster)->Send(&result);
		}
	}

	int buff_test = 0;
	for (int i = 0; i < MAX_TYPE4_BUFF; i++)
		buff_test += m_pSkillCaster->m_bType4Buff[i];
	if (buff_test == 0) m_pSkillCaster->m_bType4Flag = FALSE;	

	if (m_pSkillCaster->isPlayer() && !m_pSkillCaster->m_bType4Flag
		&& static_cast<CUser *>(m_pSkillCaster)->isInParty())
		static_cast<CUser *>(m_pSkillCaster)->SendPartyStatusUpdate(2);
}

void CMagicProcess::Type3Cancel(_MAGIC_TABLE *pSkill)
{
	if (m_pSkillCaster != m_pSkillTarget)
		return;

	// Should this take only the specified skill? I'm thinking so.
	_MAGIC_TYPE3* pType = g_pMain->m_Magictype3Array.GetData(pSkill->iNum);
	if (pType == NULL)
		return;

	for (int i = 0; i < MAX_TYPE3_REPEAT; i++)
	{
		if (m_pSkillCaster->m_bHPAmount[i] > 0)
		{
			m_pSkillCaster->m_fHPStartTime[i] = 0.0f;
			m_pSkillCaster->m_fHPLastTime[i] = 0.0f;   
			m_pSkillCaster->m_bHPAmount[i] = 0;
			m_pSkillCaster->m_bHPDuration[i] = 0;				
			m_pSkillCaster->m_bHPInterval[i] = 5;
			m_pSkillCaster->m_sSourceID[i] = -1;
			break;
		}
	}

	if (m_pSkillCaster->isPlayer())
	{
		Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE3_END));
		result << uint8(100);
		static_cast<CUser *>(m_pSkillCaster)->Send(&result); 
	}

	int buff_test = 0;
	for (int j = 0; j < MAX_TYPE3_REPEAT; j++)
		buff_test += m_pSkillCaster->m_bHPDuration[j];
	if (buff_test == 0) m_pSkillCaster->m_bType3Flag = FALSE;	

	if (m_pSkillCaster->isPlayer() && !m_pSkillCaster->m_bType3Flag
		&& static_cast<CUser *>(m_pSkillCaster)->isInParty())
		static_cast<CUser *>(m_pSkillCaster)->SendPartyStatusUpdate(1, 0);
}

void CMagicProcess::SendType4BuffRemove(short tid, BYTE buff)
{
	CUser* pTUser = g_pMain->GetUserPtr(tid);  
	if (pTUser == NULL) 
		return;

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_END));
	result << buff;
	pTUser->Send(&result);
}

short CMagicProcess::GetWeatherDamage(short damage, short attribute)
{
	// Give a 10% damage output boost based on weather (and skill's elemental attribute)
	if ((g_pMain->m_nWeather == WEATHER_FINE && attribute == ATTRIBUTE_FIRE)
		|| (g_pMain->m_nWeather == WEATHER_RAIN && attribute == ATTRIBUTE_LIGHTNING)
		|| (g_pMain->m_nWeather == WEATHER_SNOW && attribute == ATTRIBUTE_ICE))
		damage = (damage * 110) / 100;

	return damage;
}