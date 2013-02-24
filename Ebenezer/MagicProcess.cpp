// MagicProcess.cpp: implementation of the CMagicProcess class.
//
//////////////////////////////////////////////////////////////////////

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

#define MORAL_SELF				1		// �� �ڽ�..
#define MORAL_FRIEND_WITHME		2		// ���� ������ �츮��(����) �� �ϳ� ..
#define MORAL_FRIEND_EXCEPTME	3		// ���� �� �츮�� �� �ϳ� 
#define MORAL_PARTY				4		// ���� ������ �츮��Ƽ �� �ϳ�..
#define MORAL_NPC				5		// NPC�� �ϳ�.
#define MORAL_PARTY_ALL			6		// ���� ȣ���� ��Ƽ ����..
#define MORAL_ENEMY				7		// ������ ������ ���� ���� �ϳ�(NPC����)
#define MORAL_ALL				8		// �׻��� �����ϴ� ���� ���� �ϳ�.
#define MORAL_AREA_ENEMY		10		// ������ ���Ե� ��
#define MORAL_AREA_FRIEND		11		// ������ ���Ե� �츮��
#define MORAL_AREA_ALL			12		// ������ ���Ե� ����
#define MORAL_SELF_AREA			13		// ���� �߽����� �� ����
// �񷯸ӱ� Ŭ����ȯ
#define MORAL_CLAN				14		// Ŭ�� �ɹ� �� �Ѹ�...
#define MORAL_CLAN_ALL			15		// ���� ������ Ŭ�� �ɹ� ��...
//

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

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMagicProcess::CMagicProcess()
{
	m_pSrcUser = NULL;
	m_pTargetMon = NULL;
	m_pTargetUser = NULL;
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

	pkt >> m_pSkillCaster >> m_pSkillTarget
		>> m_sData1 >> m_sData2 >> m_sData3 >> m_sData4
		>> m_sData5 >> m_sData6 >> m_sData7 >> m_sData8;

	if (m_pSrcUser && !UserCanCast(pMagic, m_opcode))
			SendSkillFailed();

	uint8 bInitialResult = 0;

	switch(m_opcode)
	{
		case MAGIC_CASTING:
			SendSkill();
			break;
		case MAGIC_EFFECTING:
			bInitialResult = ExecuteSkill(pMagic, pMagic->bType[0], m_opcode);
			break;
		case MAGIC_FLYING:
		case MAGIC_FAIL:
			SendSkill();
			break;
		case MAGIC_TYPE3_END: //This is also MAGIC_TYPE4_END
			break;
		case MAGIC_CANCEL:
		case MAGIC_CANCEL2:
			Type3Cancel(m_nSkillID, m_pSrcUser->GetSocketID());
			Type4Cancel(m_nSkillID, m_pSrcUser->GetSocketID());
			//Type6Cancel(m_nSkillID, m_pSrcUser->GetSocketID());   // Scrolls etc.
			//Type9Cancel(m_nSkillID, m_pSrcUser->GetSocketID());   // Stealth lupine etc.
			break;
	}

	// NOTE: Some ROFD skills require a THIRD type.
	if (bInitialResult != 0)
		ExecuteSkill(pMagic, pMagic->bType[1], m_opcode);
}

bool CMagicProcess::UserCanCast(_MAGIC_TABLE *pSkill, uint8 m_opcode)
{
	// We don't want the source ever being anyone other than us.
	// Ever. Even in the case of NPCs, it's BAD. BAD!
	// We're better than that -- we don't need to have the client tell NPCs what to do.
	if (m_pSkillCaster != m_pSrcUser->GetSocketID()) 
		return false;

	if(m_opcode == MAGIC_CANCEL || m_opcode == MAGIC_CANCEL2) // We don't need to check anything as we're just canceling our buffs.
		return true;

	if(m_pSrcUser->isDead() || m_pSrcUser->isBlinking() && pSkill->bType[0] != 5) //To allow for resurrect scrolls
		return false;

	// If we're using an AOE, and the target is specified... something's not right.
	if ((pSkill->bMoral >= MORAL_AREA_ENEMY
			&& pSkill->bMoral <= MORAL_SELF_AREA)
		&& m_pSkillTarget != -1)
		return false;

	if(m_pSrcUser->m_pUserData->m_sClass != (pSkill->sSkill / 10)
		|| m_pSrcUser->m_pUserData->m_bLevel < pSkill->sSkillLevel)
		return false;

	if((m_pSrcUser->m_pUserData->m_sMp - pSkill->sMsp) < 0)
		return false;

	if(m_pSkillTarget >= NPC_BAND)
		m_pTargetMon = g_pMain->m_arNpcArray.GetData(m_pSkillTarget);
	else
		m_pTargetUser = g_pMain->GetUserPtr(m_pSkillTarget);

	if(m_pTargetUser != m_pSrcUser && ((m_pTargetUser->getZoneID() != m_pSrcUser->getZoneID())
		|| !m_pTargetUser->isAttackZone()))
		return false;

	if(m_pTargetUser != m_pSrcUser && ((m_pTargetUser->isAttackZone() && (m_pTargetUser->getNation() == m_pSrcUser->getNation())))) //Will have to add support for the outside battlefield
		return false;

	// If we're in a snow war, we're only ever allowed to use the snowball skill.
	if (m_pSrcUser->getZoneID() == ZONE_SNOW_BATTLE && g_pMain->m_byBattleOpen == SNOW_BATTLE 
		&& m_nSkillID != SNOW_EVENT_SKILL)
		return false;

	if(m_pTargetMon && (m_pTargetMon->getZoneID() != m_pSrcUser->getZoneID()))
		return false;

	if(m_pTargetMon && (m_pTargetMon->getNation() == m_pSrcUser->getNation()))
		return false;

	if(pSkill->iUseItem != 0 && !m_pSrcUser->CanUseItem(pSkill->iUseItem, 1)) //The user does not meet the item's requirements or does not have any of said item.
		return false;

	if(m_opcode == MAGIC_EFFECTING || m_opcode == MAGIC_CASTING && !IsAvailable(pSkill))
		return false;

	//TO-DO : Add skill cooldown checks.

	//Incase we made it to here, we can cast! Hurray!
	return true;
}

uint8 CMagicProcess::ExecuteSkill(_MAGIC_TABLE *pSkill, uint8 bType, uint8 m_opcode)
{
	uint8 bResult = 0;

	switch (bType)
	{
		case 1:
			bResult = ExecuteType1(pSkill);
			break;

		case 2:
			bResult = ExecuteType2(pSkill);
			break;
	
		case 3:
			ExecuteType3(pSkill);
			break;

		case 4:
			ExecuteType4(pSkill);
			break;
	
		case 5:
			ExecuteType5(pSkill);
			break;
	
		case 6:
			ExecuteType6(pSkill);
			break;
	
		case 7:
			ExecuteType7(pSkill);
			break;
	
		case 8:
			ExecuteType8(pSkill);
			break;
	
		case 9:
			ExecuteType9(pSkill);
			break;
	}

	return bResult;
}

void CMagicProcess::SendSkillFailed()
{
	SendSkill(m_pSkillCaster, m_pSkillTarget, MAGIC_FAIL, 
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
		pSkillCaster = m_pSkillCaster;

	if (pSkillTarget  == -1)
		pSkillTarget = m_pSkillTarget;

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
	CUser* pUser = NULL;	// When the target is a player....
	CUser* pParty = NULL;   // When the target is a party....
	CNpc* pNpc = NULL;		// When the monster is the target....
	CNpc* pMon = NULL;		// When the monster is the source....
	bool isNPC = (m_pSkillCaster >= NPC_BAND);		// Identifies source : TRUE means source is NPC.
	_MAGIC_TYPE5* pType = NULL;		// Only for type 5 magic!

	int modulator = 0, Class = 0, moral = 0, skill_mod = 0 ;

	if (m_pSkillTarget >= 0 && m_pSkillTarget < MAX_USER) {		// Target existence check routine for player.          
		pUser = g_pMain->GetUserPtr(m_pSkillTarget);
		if (pUser == NULL)
			goto fail_return;

		if (pSkill->bType[0] != 5) // If not a Warp/Resurrection spell...
		{
			if (pUser->isDead() || pUser->isBlinking())
				goto fail_return;
		}
		else // skill type 5 (warp/resurrection)
		{
			pType = g_pMain->m_Magictype5Array.GetData(pSkill->iNum);    
			if (pType == NULL
				|| pUser->isBlinking()
				|| (pUser->isDead() && pType->sNeedStone == 0 && pType->bExpRecover == 0))
				goto fail_return;					
		}

		moral = pUser->getNation();
	}
	else if (m_pSkillTarget >= NPC_BAND)     // Target existence check routine for NPC.          	
	{
		if (!g_pMain->m_bPointCheckFlag)
			goto fail_return;	
		pNpc = g_pMain->m_arNpcArray.GetData(m_pSkillTarget);
		if (pNpc == NULL || pNpc->isDead())
			goto fail_return;	//... Assuming NPCs can't be resurrected.

		moral = pNpc->m_byGroup;
	}
	else if (m_pSkillTarget == -1)  // AOE/Party Moral check routine.
	{
		if (isNPC)
		{
			moral = 1;
		}
		else
		{
			if (pSkill->bMoral == MORAL_AREA_ENEMY)
				moral = m_pSrcUser->getNation() == KARUS ? ELMORAD : KARUS;
			else 
				moral = m_pSrcUser->getNation();	
		}
	}
	else 
		moral = m_pSrcUser->getNation();

	switch( pSkill->bMoral ) {		// Compare morals between source and target character.
		case MORAL_SELF:   // #1         // ( to see if spell is cast on the right target or not )
			if(isNPC) {
				if( m_pSkillTarget != pMon->m_sNid ) goto fail_return;
			}
			else {
				if( m_pSkillTarget != m_pSrcUser->GetSocketID() ) goto fail_return;
			}
			break;
		case MORAL_FRIEND_WITHME:	// #2
			if ((isNPC && pMon->m_byGroup != moral)
				|| (!isNPC && m_pSrcUser->getNation() != moral))
				goto fail_return;
			break;
		case MORAL_FRIEND_EXCEPTME:	   // #3
			if(isNPC) {
				if( pMon->m_byGroup != moral ) goto fail_return;
				if( m_pSkillTarget == pMon->m_sNid ) goto fail_return;				
			}
			else {
				if( m_pSrcUser->m_pUserData->m_bNation != moral ) goto fail_return;
				if( m_pSkillTarget == m_pSrcUser->GetSocketID() ) goto fail_return;
			}
			break;
		case MORAL_PARTY:	 // #4
			if( !m_pSrcUser->isInParty() && m_pSkillCaster != m_pSkillTarget) goto fail_return;
			if (m_pSrcUser->m_pUserData->m_bNation != moral) goto fail_return;
			if( pUser )
				if( pUser->m_sPartyIndex != m_pSrcUser->m_sPartyIndex ) goto fail_return;
			break;
		case MORAL_NPC:		// #5
			if( !pNpc ) goto fail_return;
			if( pNpc->m_byGroup != moral ) goto fail_return;
			break;
		case MORAL_PARTY_ALL:     // #6
//			if ( !m_pSrcUser->isInParty() ) goto fail_return;		
//			if ( !m_pSrcUser->isInParty() && sid != tid) goto fail_return;					

			break;
		case MORAL_ENEMY:	// #7	
			if(isNPC) {
				if( pMon->m_byGroup == moral ) goto fail_return;		
			}
			else {
				if( m_pSrcUser->getNation() == moral ) goto fail_return;	
			}
			break;	
		case MORAL_ALL:	 // #8
			// N/A
			break;
		case MORAL_AREA_ENEMY:		// #10
			// N/A
			break;
		case MORAL_AREA_FRIEND:		// #11
			if( m_pSrcUser->m_pUserData->m_bNation != moral ) goto fail_return;
			break;
		case MORAL_AREA_ALL:	// #12
			// N/A
			break;
		case MORAL_SELF_AREA:     // #13
			// Remeber, EVERYONE in the area is affected by this one. No moral check!!!
			break;
		case MORAL_CORPSE_FRIEND:		// #25
			if(isNPC) {
				if( pMon->m_byGroup != moral ) goto fail_return;
				if( m_pSkillTarget == pMon->m_sNid ) goto fail_return;				
			}
			else {
				if( m_pSrcUser->getNation() != moral ) goto fail_return;
				if( m_pSkillTarget == m_pSrcUser->GetSocketID() ) goto fail_return;
				if(pUser->m_bResHpType != USER_DEAD) goto fail_return; 
			}
			break;
//
		case MORAL_CLAN:		// #14
			if( m_pSrcUser->m_pUserData->m_bKnights == -1 && m_pSkillCaster != m_pSkillTarget ) goto fail_return;
			if( m_pSrcUser->m_pUserData->m_bNation != moral ) goto fail_return;
			if( pUser ) {
				if( pUser->m_pUserData->m_bKnights != m_pSrcUser->m_pUserData->m_bKnights ) goto fail_return;
			}
			break;

		case MORAL_CLAN_ALL:	// #15
			break;
//
	}

	if (!isNPC) {	// If the user cast the spell (and not the NPC).....
		modulator = pSkill->sSkill % 10;     // Hacking prevention!
		if( modulator != 0 ) {	
			Class = pSkill->sSkill / 10;
			if( Class != m_pSrcUser->m_pUserData->m_sClass ) goto fail_return;
			if( pSkill->sSkillLevel > m_pSrcUser->m_pUserData->m_bstrSkill[modulator] ) goto fail_return;
		}
		else if (pSkill->sSkillLevel > m_pSrcUser->getLevel()) goto fail_return;

		if (pSkill->bType[0] == 1) {	// Weapons verification in case of COMBO attack (another hacking prevention).
			if (pSkill->sSkill == 1055 || pSkill->sSkill == 2055) {		// Weapons verification in case of DUAL ATTACK (type 1)!		
				_ITEM_TABLE* pLeftHand = g_pMain->GetItemPtr(m_pSrcUser->m_pUserData->m_sItemArray[LEFTHAND].nNum);
				if (!pLeftHand) return false;

				_ITEM_TABLE* pRightHand = g_pMain->GetItemPtr(m_pSrcUser->m_pUserData->m_sItemArray[RIGHTHAND].nNum);
				if (!pRightHand) return false;

				int left_index = pLeftHand->m_bKind / 10 ;
				int right_index = pRightHand->m_bKind / 10 ;

				if ((left_index != WEAPON_SWORD && left_index != WEAPON_AXE && left_index != WEAPON_MACE) 
					&& (right_index != WEAPON_SWORD  && right_index != WEAPON_AXE && right_index != WEAPON_MACE)) 
					return false;
			}
			else if (pSkill->sSkill == 1056 || pSkill->sSkill == 2056) {	// Weapons verification in case of DOUBLE ATTACK !
				_ITEM_TABLE* pRightHand = g_pMain->GetItemPtr(m_pSrcUser->m_pUserData->m_sItemArray[RIGHTHAND].nNum);
				if (!pRightHand) return false;

				int right_index = pRightHand->m_bKind / 10 ;

				if(	(right_index != WEAPON_SWORD && right_index != WEAPON_AXE && right_index != WEAPON_MACE
						&& right_index != WEAPON_SPEAR) || m_pSrcUser->m_pUserData->m_sItemArray[LEFTHAND].nNum != 0 )
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
				if (m_pSkillCaster >= 0 && m_pSkillCaster < MAX_USER)
				{	
					if (pSkill->iUseItem != 0) {
						_ITEM_TABLE* pItem = NULL;				// This checks if such an item exists.
						pItem = g_pMain->GetItemPtr(pSkill->iUseItem);
						if( !pItem ) return false;

						if ((pItem->m_bRace != 0 && m_pSrcUser->m_pUserData->m_bRace != pItem->m_bRace)
							|| (pItem->m_bClass != 0 && !m_pSrcUser->JobGroupCheck(pItem->m_bClass))
							|| (pItem->m_bReqLevel != 0 && m_pSrcUser->getLevel() < pItem->m_bReqLevel)
							|| (!m_pSrcUser->RobItem(pSkill->iUseItem, 1)))	
							return false;
					}
				}
			}

			if (pSkill->bType[0] == 5) {
				if (m_pSkillTarget >= 0 && m_pSkillTarget < MAX_USER)	{	
					if (pSkill->iUseItem != 0) {					
						pType = g_pMain->m_Magictype5Array.GetData(pSkill->iNum);    
						if (!pType) goto fail_return;

						CUser* pTUser = g_pMain->GetUserPtr(m_pSkillTarget);    
						if (!pTUser) goto fail_return;

						// No resurrections for low level users
						if (pType->bType == 3 && pTUser->getLevel() <= 5)
							return false;

						if (pType->bType == 3) {
							if (!pTUser->RobItem(pSkill->iUseItem, pType->sNeedStone))
								return false;
						}
						else if (!m_pSrcUser->RobItem(pSkill->iUseItem, pType->sNeedStone))
							return false;
					}
				}
			}
//
			if (pSkill->bType[0] == 1 || pSkill->bType[0] == 2)	// Actual deduction of Skill or Magic point.
				m_pSrcUser->MSpChange(-(skill_mana));
			else if (pSkill->bType[0] != 4 || (pSkill->bType[0] == 4 && m_pSkillTarget == -1))
				m_pSrcUser->MSpChange(-(pSkill->sMsp));

			if (pSkill->sHP > 0 && pSkill->sMsp == 0) {			// DEDUCTION OF HPs in Magic/Skill using HPs.
				if (pSkill->sHP > m_pSrcUser->m_pUserData->m_sHp) goto fail_return;
				m_pSrcUser->HpChange(-pSkill->sHP);
			}
//
		}
	}

	return true;      // Magic was successful! 

fail_return:    // In case of failure, send a packet(!)
	if (!isNPC)
		SendSkillFailed();

	return false;     // Magic was a failure!
}

BYTE CMagicProcess::ExecuteType1(_MAGIC_TABLE *pSkill)
{	
	int damage = 0;
	uint8 bResult = 0;

	if (pSkill == NULL)
		return 0; 

	_MAGIC_TYPE1* pType = g_pMain->m_Magictype1Array.GetData(pSkill->iNum);
	if (pType == NULL)
		return 0;

	damage = m_pSrcUser->GetDamage(m_pSkillTarget, pSkill->iNum);  // Get damage points of enemy.

	// TO-DO: See if this has already be checked?
	CUser* pTUser = g_pMain->GetUserPtr(m_pSkillTarget);     // Get target info.  
	if (pTUser == NULL || pTUser->isDead())
		goto packet_send;

	bResult = 1;

	// TO-DO: Replace all of this with more abstract attack/death code 
	// so that when we actually want to tweak behaviour, we don't have to tweak 9999999 cases where things die. Just the one...
	pTUser->HpChange( -damage, 0, false, m_pSrcUser->GetSocketID());     // Reduce target health point.

	if( pTUser->m_pUserData->m_sHp == 0) {    // Check if the target is dead.
		pTUser->m_bResHpType = USER_DEAD;     // Target status is officially dead now.

		if( !m_pSrcUser->isInParty() ) {    // Something regarding loyalty points.
			m_pSrcUser->LoyaltyChange(m_pSkillTarget);
		}
		else {
			m_pSrcUser->LoyaltyDivide(m_pSkillTarget);
		}

		m_pSrcUser->GoldChange(m_pSkillTarget, 0);

		pTUser->InitType3();	// Init Type 3.....
		pTUser->InitType4();	// Init Type 4.....

		if( pTUser->m_pUserData->m_bZone != pTUser->m_pUserData->m_bNation && pTUser->m_pUserData->m_bZone < 3) {
			pTUser->ExpChange(-pTUser->m_iMaxExp / 100);
		}
		pTUser->m_sWhoKilledMe = m_pSkillCaster;		// Who the hell killed me?
	} 
	m_pSrcUser->SendTargetHP( 0, m_pSkillTarget, -damage );     // Change the HP of the target.

packet_send:

	// This should only be sent once. I don't think there's reason to enforce this, as no skills behave otherwise
	m_sData4 = (damage == 0 ? -104 : 0);
	SendSkill();	

	return bResult;
}

BYTE CMagicProcess::ExecuteType2(_MAGIC_TABLE *pSkill)
{		
	int damage = 0;
	uint8 bResult = 0;
	float total_range = 0.0f;	// These variables are used for range verification!
	int sx, sz, tx, tz;

	if (pSkill == NULL)
		return 0;

	_ITEM_TABLE* pTable = NULL;		// Get item info.
	if (m_pSrcUser->m_pUserData->m_sItemArray[LEFTHAND].nNum)
		pTable = g_pMain->GetItemPtr(m_pSrcUser->m_pUserData->m_sItemArray[LEFTHAND].nNum);
	else
		pTable = g_pMain->GetItemPtr(m_pSrcUser->m_pUserData->m_sItemArray[RIGHTHAND].nNum);

	if (pTable == NULL) 
		return 0;

	_MAGIC_TYPE2* pType = g_pMain->m_Magictype2Array.GetData(pSkill->iNum);     // Get magic skill table type 2.
	if (pType == NULL)
		return 0;

	CUser* pTUser = g_pMain->GetUserPtr(m_pSkillTarget);
	// is this checked already?
	if (pTUser == NULL || pTUser->isDead())
		goto packet_send;
	
	total_range = pow(((pType->sAddRange * pTable->m_sRange) / 100.0f), 2.0f) ;     // Range verification procedure.
	sx = (int)m_pSrcUser->m_pUserData->m_curx; tx = (int)pTUser->m_pUserData->m_curx;
	sz = (int)m_pSrcUser->m_pUserData->m_curz; tz = (int)pTUser->m_pUserData->m_curz;
	
	if ((pow((float)(sx - tx), 2.0f) + pow((float)(sz - tz), 2.0f)) > total_range)	   // Target is out of range, exit.
		goto packet_send;
	
	damage = m_pSrcUser->GetDamage(m_pSkillTarget, pSkill->iNum);  // Get damage points of enemy.	

	// TO-DO: Replace all of this with more abstract attack/death code 
	// so that when we actually want to tweak behaviour, we don't have to tweak 9999999 cases where things die. Just the one...

	pTUser->HpChange(-damage, 0, false, m_pSrcUser->GetSocketID());     // Reduce target health point.

	if( pTUser->m_pUserData->m_sHp == 0){     // Check if the target is dead.    
		pTUser->m_bResHpType = USER_DEAD;     // Target status is officially dead now.

		if( !m_pSrcUser->isInParty() ) {    // Something regarding loyalty points.
			m_pSrcUser->LoyaltyChange(m_pSkillTarget);
		}
		else {
			m_pSrcUser->LoyaltyDivide(m_pSkillTarget);
		}

		m_pSrcUser->GoldChange(m_pSkillTarget, 0);

		pTUser->InitType3();	// Init Type 3.....
		pTUser->InitType4();	// Init Type 4.....

		if( pTUser->m_pUserData->m_bZone != pTUser->m_pUserData->m_bNation && pTUser->m_pUserData->m_bZone < 3) {
			pTUser->ExpChange(-pTUser->m_iMaxExp / 100);
		}
		pTUser->m_sWhoKilledMe = m_pSkillCaster;		// Who the hell killed me?
	} 

	m_pSrcUser->SendTargetHP( 0, m_pSkillTarget, -damage );     // Change the HP of the target.

packet_send:
	// This should only be sent once. I don't think there's reason to enforce this, as no skills behave otherwise
	m_sData4 = (damage == 0 ? -104 : 0);
	SendSkill();

	return bResult;
}

void CMagicProcess::ExecuteType3(_MAGIC_TABLE *pSkill)  // Applied when a magical attack, healing, and mana restoration is done.
{	
	int damage = 0, duration_damage = 0;
	bool isNPC = false;
	CNpc* pMon = NULL;

	vector<CUser *> casted_member;

	if (pSkill == NULL) 
		return;

	_MAGIC_TYPE3* pType = g_pMain->m_Magictype3Array.GetData(pSkill->iNum);
	if (pType == NULL) 
		return;

	if (m_pSkillCaster >= NPC_BAND)
	{
		isNPC = true;
		pMon = g_pMain->m_arNpcArray.GetData(m_pSkillCaster);
		if (pMon == NULL || pMon->isDead())
			return;
	}

	if (m_pSkillTarget == -1) {		// If the target was the source's party....
		// TO-DO: Make this not completely and utterly suck (i.e. kill that loop!).
		SessionMap & sessMap = g_pMain->s_socketMgr.GetActiveSessionMap();
		foreach (itr, sessMap)
		{		
			CUser* pTUser = static_cast<CUser *>(itr->second);
			if (!pTUser->isDead() && !pTUser->isBlinking()
				&& UserRegionCheck(m_pSkillCaster, pTUser->GetSocketID(), pSkill->iNum, pType->bRadius, m_sData1, m_sData3))
				casted_member.push_back(pTUser);
		}
		g_pMain->s_socketMgr.ReleaseLock();

		if (casted_member.empty())
		{
			SendSkillFailed();
			return;			
		}
	}
	else
	{	// If the target was another single player.
		CUser* pTUser = g_pMain->GetUserPtr(m_pSkillTarget);
		if (pTUser == NULL || pTUser->isDead() || pTUser->isBlinking()) 
			return;		
		
		casted_member.push_back(pTUser);
	}
	
	foreach (itr, casted_member)
	{
		CUser* pTUser = *itr; // it's checked above, not much need to check it again
		if ((pType->sFirstDamage < 0) && (pType->bDirectType == 1) && (pSkill->iNum < 400000)) {		// If you are casting an attack spell.
			damage = GetMagicDamage(m_pSkillCaster, pTUser->GetSocketID(), pType->sFirstDamage, pType->bAttribute) ;	// Get Magical damage point.
		}
		else damage = pType->sFirstDamage;

		if( m_pSrcUser )	{
			if( m_pSrcUser->m_pUserData->m_bZone == ZONE_SNOW_BATTLE && g_pMain->m_byBattleOpen == SNOW_BATTLE )	{
				damage = -10;		
			}
		}

		if (pType->bDuration == 0) 
		{     // Non-Durational Spells.
			if (pType->bDirectType == 1)     // Health Point related !
			{			
				pTUser->HpChange(damage, 0, false, m_pSrcUser->GetSocketID());     // Reduce target health point.
				
				if( pTUser->m_pUserData->m_sHp == 0) {     // Check if the target is dead.		
					pTUser->m_bResHpType = USER_DEAD;

					if (isNPC) {	// Killed by monster/NPC.
//
						if( pTUser->m_pUserData->m_bZone != pTUser->m_pUserData->m_bNation && pTUser->m_pUserData->m_bZone < 3) {
							pTUser->ExpChange(-pTUser->m_iMaxExp / 100);
							//TRACE("������ 1%�� �￴�ٴϱ��� ��.��\r\n");
						}
						else {
//
							pTUser->ExpChange( -pTUser->m_iMaxExp/20 ); 					
//
						}
//
					}
					else 
					{	// Killed by another player.
						if( m_pSrcUser->m_pUserData->m_bZone == ZONE_SNOW_BATTLE && g_pMain->m_byBattleOpen == SNOW_BATTLE )	{
							m_pSrcUser->GoldGain( SNOW_EVENT_MONEY );	// 10000���Ƹ� �ִ� �κ�,,,,,

							if( m_pSrcUser->m_pUserData->m_bZone == ZONE_SNOW_BATTLE && g_pMain->m_byBattleOpen == SNOW_BATTLE )	{
								if (pTUser->m_pUserData->m_bNation == KARUS) {
									g_pMain->m_sKarusDead++;
									//TRACE("++ ExecuteType3 - ka=%d, el=%d\n", g_pMain->m_sKarusDead, g_pMain->m_sElmoradDead);
								}
								else if (pTUser->m_pUserData->m_bNation == ELMORAD) {
									g_pMain->m_sElmoradDead++;
									//TRACE("++ ExecuteType3 - ka=%d, el=%d\n", g_pMain->m_sKarusDead, g_pMain->m_sElmoradDead);
								}
							}

						}
						else	{
							if( !m_pSrcUser->isInParty() ) {    // Something regarding loyalty points.
								m_pSrcUser->LoyaltyChange(pTUser->GetSocketID());
							}
							else {
								m_pSrcUser->LoyaltyDivide(pTUser->GetSocketID());
							}

							m_pSrcUser->GoldChange(pTUser->GetSocketID(), 0);
						}
					}

					pTUser->InitType3();	// Init Type 3.....
					pTUser->InitType4();	// Init Type 4.....

					if (m_pSkillCaster >= 0 && m_pSkillCaster < MAX_USER) {
						if( pTUser->m_pUserData->m_bZone != pTUser->m_pUserData->m_bNation && pTUser->m_pUserData->m_bZone < 3) {
							pTUser->ExpChange(-pTUser->m_iMaxExp / 100);
						}
						pTUser->m_sWhoKilledMe = m_pSkillCaster;	// Who the hell killed me?
					}
				}

				if (!isNPC) m_pSrcUser->SendTargetHP( 0, (*itr)->GetSocketID(), damage ) ;     // Change the HP of the target.			
			}
			else if ( pType->bDirectType == 2 || pType->bDirectType == 3 )    // Magic or Skill Point related !
				pTUser->MSpChange(damage);     // Change the SP or the MP of the target.		
			else if( pType->bDirectType == 4 )     // Armor Durability related.
				pTUser->ItemWoreOut( DEFENCE, -damage);     // Reduce Slot Item Durability
		}
		else if (pType->bDuration != 0) {    // Durational Spells! Remember, durational spells only involve HPs.
			if (damage != 0) {		// In case there was first damage......
				pTUser->HpChange(damage, 0, false, m_pSrcUser->GetSocketID());			// Initial damage!!!
		
				if( pTUser->m_pUserData->m_sHp == 0) {     // Check if the target is dead.	
					pTUser->m_bResHpType = USER_DEAD;

					if (isNPC) {	// Killed by monster/NPC.
//
						if( pTUser->m_pUserData->m_bZone != pTUser->m_pUserData->m_bNation && pTUser->m_pUserData->m_bZone < 3) {
							pTUser->ExpChange(-pTUser->m_iMaxExp / 100);
							//TRACE("������ 1%�� �￴�ٴϱ��� ��.��\r\n");
						}
						else {
//
							pTUser->ExpChange( -pTUser->m_iMaxExp / 20 );
//
						}
//
					}
					else 
					{	// Killed by another player.
						if( !m_pSrcUser->isInParty() ) {    // Something regarding loyalty points.
							m_pSrcUser->LoyaltyChange(pTUser->GetSocketID());
						}
						else {
							m_pSrcUser->LoyaltyDivide(pTUser->GetSocketID());
						}

						m_pSrcUser->GoldChange(pTUser->GetSocketID(), 0);
					}

					pTUser->InitType3();	// Init Type 3.....
					pTUser->InitType4();	// Init Type 4..... 

					if (m_pSkillCaster >= 0 && m_pSkillCaster < MAX_USER) {
						if( pTUser->m_pUserData->m_bZone != pTUser->m_pUserData->m_bNation && pTUser->m_pUserData->m_bZone < 3) {
							pTUser->ExpChange(-pTUser->m_iMaxExp / 100);
						}
						pTUser->m_sWhoKilledMe = m_pSkillCaster;	// Who the hell killed me?
					}
				}
				if (!isNPC) m_pSrcUser->SendTargetHP( 0, pTUser->GetSocketID(), damage );     // Change the HP of the target. 
			}

			if (pTUser->m_bResHpType != USER_DEAD) {	// ���⵵ ��ȣ �ڵ� �߽�...
				if (pType->sTimeDamage < 0) {
					duration_damage = GetMagicDamage(m_pSkillCaster, pTUser->GetSocketID(), pType->sTimeDamage, pType->bAttribute) ;
				}
				else duration_damage = pType->sTimeDamage ;

				for (int k = 0 ; k < MAX_TYPE3_REPEAT ; k++) {	// For continuous damages...
					if (pTUser->m_bHPInterval[k] == 5) {
						pTUser->m_fHPStartTime[k] = pTUser->m_fHPLastTime[k] = TimeGet();     // The durational magic routine.
						pTUser->m_bHPDuration[k] = pType->bDuration;
						pTUser->m_bHPInterval[k] = 2;		
						pTUser->m_bHPAmount[k] = duration_damage / ( pTUser->m_bHPDuration[k] / pTUser->m_bHPInterval[k] ) ;
						pTUser->m_sSourceID[k] = m_pSkillCaster;
						break;
					}
				}

				pTUser->m_bType3Flag = TRUE;
			}

			if (pTUser->isInParty() && pType->sTimeDamage < 0)
				pTUser->SendPartyStatusUpdate(1, 1);
		} 
	
		if ( pSkill->bType[1] == 0 || pSkill->bType[1] == 3 )
			SendSkill(m_pSkillCaster, pTUser->GetSocketID());
		
		// Tell the AI server we're healing someone (so that they can choose to pick on the healer!)
		if (pType->bDirectType == 1 && damage > 0
			&& !isNPC
			&& m_pSkillCaster != m_pSkillTarget)
		{

			Packet result(AG_HEAL_MAGIC);
			result << m_pSkillCaster;
			g_pMain->Send_AIServer(&result);
		}
	}
}

void CMagicProcess::ExecuteType4(_MAGIC_TABLE *pSkill)
{
	int damage = 0;

	vector<CUser *> casted_member;
	if (pSkill == NULL)
		return;

	_MAGIC_TYPE4* pType = g_pMain->m_Magictype4Array.GetData(pSkill->iNum);
	if (pType == NULL)
		return;

	if (m_pSkillTarget == -1)
	{
		// TO-DO: Localise this. This is horribly unnecessary.
		SessionMap & sessMap = g_pMain->s_socketMgr.GetActiveSessionMap();
		foreach (itr, sessMap)
		{		
			CUser* pTUser = static_cast<CUser *>(itr->second);
			if (!pTUser->isDead() && !pTUser->isBlinking()
				&& UserRegionCheck(m_pSkillCaster, pTUser->GetSocketID(), pSkill->iNum, pType->bRadius, m_sData1, m_sData3))
				casted_member.push_back(pTUser);
		}
		g_pMain->s_socketMgr.ReleaseLock();

		if (casted_member.empty())
		{		
			if (m_pSkillCaster >= 0 && m_pSkillCaster < MAX_USER) 
				SendSkillFailed();
			return;	
		}
	}
	else 
	{
		// If the target was another single player.
		CUser* pTUser = g_pMain->GetUserPtr(m_pSkillTarget);
		if (pTUser == NULL 
			|| pTUser->isDead() || pTUser->isBlinking()) 
			return;
		casted_member.push_back(pTUser);
	}

	foreach (itr, casted_member)
	{
		uint8 bResult = 1;
		CUser* pTUser = *itr;
//
		if (pTUser->m_bType4Buff[pType->bBuffType - 1] == 2 && m_pSkillTarget == -1) {		// Is this buff-type already casted on the player?
			bResult = 0;
			goto fail_return ;					
		}
//
		//if ( data4 == -1 ) //Need to create InsertSaved Magic before enabling this.
			//pTUser->InsertSavedMagic( magicid, pType->sDuration );

		switch (pType->bBuffType) {	// Depending on which buff-type it is.....
			case 1:
				pTUser->m_sMaxHPAmount = pType->sMaxHP;		// Get the amount that will be added/multiplied.
				pTUser->m_sDuration1 = pType->sDuration;	// Get the duration time.
				pTUser->m_fStartTime1 = TimeGet();			// Get the time when Type 4 spell starts.	
				break;

			case 2:
				pTUser->m_sACAmount = pType->sAC;
				pTUser->m_sDuration2 = pType->sDuration;
				pTUser->m_fStartTime2 = TimeGet();
				break;

			case 3:
				// These really shouldn't be hardcoded, but with mgame's implementation it doesn't seem we have a choice (as is).
				if (pSkill->iNum == 490034)	// Bezoar!!!
					pTUser->StateChangeServerDirect(3, ABNORMAL_GIANT); 
				else if (pSkill->iNum == 490035)	// Rice Cake!!!
					pTUser->StateChangeServerDirect(3, ABNORMAL_DWARF); 

				pTUser->m_sDuration3 = pType->sDuration;
				pTUser->m_fStartTime3 = TimeGet();
				break;

			case 4:
				pTUser->m_bAttackAmount = pType->bAttack;
				pTUser->m_sDuration4 = pType->sDuration;
				pTUser->m_fStartTime4 = TimeGet();					
				break;

			case 5:
				pTUser->m_bAttackSpeedAmount = pType->bAttackSpeed;
				pTUser->m_sDuration5 = pType->sDuration;
				pTUser->m_fStartTime5 = TimeGet();
				break;

			case 6:
				pTUser->m_bSpeedAmount = pType->bSpeed;
				pTUser->m_sDuration6 = pType->sDuration;
				pTUser->m_fStartTime6 = TimeGet();
				break;

			case 7:
				pTUser->setStatBuff(STAT_STR, pType->bStr);
				pTUser->setStatBuff(STAT_STA, pType->bSta);
				pTUser->setStatBuff(STAT_DEX, pType->bDex);
				pTUser->setStatBuff(STAT_INT, pType->bIntel);
				pTUser->setStatBuff(STAT_CHA, pType->bCha);	
				pTUser->m_sDuration7 = pType->sDuration;
				pTUser->m_fStartTime7 = TimeGet();
				break;

			case 8:
				pTUser->m_bFireRAmount = pType->bFireR;
				pTUser->m_bColdRAmount = pType->bColdR;
				pTUser->m_bLightningRAmount = pType->bLightningR;
				pTUser->m_bMagicRAmount = pType->bMagicR;
				pTUser->m_bDiseaseRAmount = pType->bDiseaseR;
				pTUser->m_bPoisonRAmount = pType->bPoisonR;
				pTUser->m_sDuration8 = pType->sDuration;
				pTUser->m_fStartTime8 = TimeGet();
				break;

			case 9:
				pTUser->m_bHitRateAmount = pType->bHitRate;
				pTUser->m_sAvoidRateAmount = pType->sAvoidRate;
				pTUser->m_sDuration9 = pType->sDuration;
				pTUser->m_fStartTime9 = TimeGet();
				break;	

			default:
				bResult = 0;
				goto fail_return;
		}

		if (m_pSkillTarget != -1 && pSkill->bType[0] == 4)
		{
			if (m_pSkillCaster >= 0 && m_pSkillCaster < MAX_USER)
				m_pSrcUser->MSpChange( -(pSkill->sMsp) );
		}

		if (m_pSkillCaster >= 0 && m_pSkillCaster < MAX_USER) 
			pTUser->m_bType4Buff[pType->bBuffType - 1] = (m_pSrcUser->getNation() == pTUser->getNation() ? 2 : 1);
		else
			pTUser->m_bType4Buff[pType->bBuffType - 1] = 1;

		pTUser->m_bType4Flag = TRUE;

		pTUser->SetSlotItemValue();				// Update character stats.
		pTUser->SetUserAbility();

		if(m_pSkillCaster == m_pSkillTarget)
			pTUser->SendItemMove(2);

		if (pTUser->isInParty() && pTUser->m_bType4Buff[pType->bBuffType - 1] == 1)
			pTUser->SendPartyStatusUpdate(2, 1);

		pTUser->Send2AI_UserUpdateInfo();

	fail_return:
		if (pSkill->bType[1] == 0 || pSkill->bType[1] == 4)
		{
			CUser *pUser = (m_pSkillCaster >= 0 && m_pSkillCaster < MAX_USER ? m_pSrcUser : pTUser);

			pUser->m_MagicProcess.SendSkill(m_pSkillCaster, (*itr)->GetSocketID(),
				m_opcode, m_nSkillID,
				m_sData1, bResult, m_sData3,
				(bResult == 1 || m_sData4 == 0 ? pType->sDuration : 0),
				0, pType->bSpeed);
		}
		
		if (bResult == 0
			&& m_pSkillCaster >= 0 && m_pSkillCaster < MAX_USER)
			SendSkill(m_pSkillCaster, (*itr)->GetSocketID(), MAGIC_FAIL);
		continue;
	}
}

void CMagicProcess::ExecuteType5(_MAGIC_TABLE *pSkill)
{
	int damage = 0;
	int i = 0, j = 0, k =0; int buff_test = 0; BOOL bType3Test = TRUE, bType4Test = TRUE; 	

	if (pSkill == NULL)
		return;

	_MAGIC_TYPE5* pType = g_pMain->m_Magictype5Array.GetData(pSkill->iNum);
	if (pType == NULL)
		return;

	CUser* pTUser = g_pMain->GetUserPtr(m_pSkillTarget);
	if (pTUser == NULL 
		|| (pTUser->isDead() && pType->bType != RESURRECTION) 
		|| (!pTUser->isDead() && pType->bType == RESURRECTION)) 
		return;

	switch(pType->bType) {
		case REMOVE_TYPE3:		// REMOVE TYPE 3!!!
			for (i = 0 ; i < MAX_TYPE3_REPEAT ; i++) {
				if (pTUser->m_bHPAmount[i] < 0) {
					pTUser->m_fHPStartTime[i] = 0.0f;
					pTUser->m_fHPLastTime[i] = 0.0f;   
					pTUser->m_bHPAmount[i] = 0;
					pTUser->m_bHPDuration[i] = 0;				
					pTUser->m_bHPInterval[i] = 5;
					pTUser->m_sSourceID[i] = -1;

					// TO-DO: Wrap this up (ugh, I feel so dirty)
					Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE3_END));
					result << uint8(200); // removes all
					pTUser->Send(&result); 
				}
			}

			buff_test = 0;
			for (j = 0; j < MAX_TYPE3_REPEAT; j++)
				buff_test += pTUser->m_bHPDuration[j];
			if (buff_test == 0) pTUser->m_bType3Flag = FALSE;	

			// Check for Type 3 Curses.
			for (k = 0 ; k < MAX_TYPE3_REPEAT ; k++) {
				if (pTUser->m_bHPAmount[k] < 0) {
					bType3Test = FALSE;
					break;
				}
			}
  
			if (pTUser->isInParty() && bType3Test)
				pTUser->SendPartyStatusUpdate(1);
			break;

		case REMOVE_TYPE4:		// REMOVE TYPE 4!!!
			if (pTUser->m_bType4Buff[0] == 1) {
				pTUser->m_sDuration1 = 0;		
				pTUser->m_fStartTime1 = 0.0f;
				pTUser->m_sMaxHPAmount = 0;
				pTUser->m_bType4Buff[0] = 0;

				SendType4BuffRemove(m_pSkillTarget, 1);
			}

			if (pTUser->m_bType4Buff[1] == 1) {
				pTUser->m_sDuration2 = 0;		
				pTUser->m_fStartTime2 = 0.0f;
				pTUser->m_sACAmount = 0;
				pTUser->m_bType4Buff[1] = 0;
				
				SendType4BuffRemove(m_pSkillTarget, 2);
			}

			if (pTUser->m_bType4Buff[3] == 1) {
				pTUser->m_sDuration4 = 0;		
				pTUser->m_fStartTime4 = 0.0f;
				pTUser->m_bAttackAmount = 100;
				pTUser->m_bType4Buff[3] = 0;

				SendType4BuffRemove(m_pSkillTarget, 4);
			}

			if (pTUser->m_bType4Buff[4] == 1) {
				pTUser->m_sDuration5 = 0;		
				pTUser->m_fStartTime5 = 0.0f;
				pTUser->m_bAttackSpeedAmount = 100;	
				pTUser->m_bType4Buff[4] = 0;

				SendType4BuffRemove(m_pSkillTarget, 5);
			}

			if (pTUser->m_bType4Buff[5] == 1) {
				pTUser->m_sDuration6 = 0;		
				pTUser->m_fStartTime6 = 0.0f;
				pTUser->m_bSpeedAmount = 100;
				pTUser->m_bType4Buff[5] = 0;				

				SendType4BuffRemove(m_pSkillTarget, 6);
			}

			if (pTUser->m_bType4Buff[6] == 1) {
				pTUser->m_sDuration7 = 0;		
				pTUser->m_fStartTime7 = 0.0f;
				// TO-DO: Implement reset.
				memset(pTUser->m_bStatBuffs, 0, sizeof(uint8) * STAT_COUNT);
				pTUser->m_bType4Buff[6] = 0;			
				SendType4BuffRemove(m_pSkillTarget, 7);
			}

			if (pTUser->m_bType4Buff[7] == 1) {
				pTUser->m_sDuration8 = 0;		
				pTUser->m_fStartTime8 = 0.0f;
				pTUser->m_bFireRAmount = 0;
				pTUser->m_bColdRAmount = 0;
				pTUser->m_bLightningRAmount = 0;
				pTUser->m_bMagicRAmount = 0;
				pTUser->m_bDiseaseRAmount = 0;
				pTUser->m_bPoisonRAmount = 0;
				pTUser->m_bType4Buff[7] = 0;			

				SendType4BuffRemove(m_pSkillTarget, 8);
			}

			if (pTUser->m_bType4Buff[8] == 1) {
				pTUser->m_sDuration9 = 0;		
				pTUser->m_fStartTime9 = 0.0f;
				pTUser->m_bHitRateAmount = 100;
				pTUser->m_sAvoidRateAmount = 100;
				pTUser->m_bType4Buff[8] = 0;			

				SendType4BuffRemove(m_pSkillTarget, 9);
			}

			pTUser->SetSlotItemValue();
			pTUser->SetUserAbility();
			pTUser->Send2AI_UserUpdateInfo();

			buff_test = 0;
			for (i = 0 ; i < MAX_TYPE4_BUFF; i++)
				buff_test += pTUser->m_bType4Buff[i];
			if (buff_test == 0) pTUser->m_bType4Flag = FALSE;

			bType4Test = TRUE ;
			for (j = 0; j < MAX_TYPE4_BUFF; j++) {
				if (pTUser->m_bType4Buff[j] == 1) {
					bType4Test = FALSE;
					break;
				}
			}

			if (pTUser->isInParty() && bType4Test)
				pTUser->SendPartyStatusUpdate(2, 0);
			break;
			
		case RESURRECTION:		// RESURRECT A DEAD PLAYER!!!
			pTUser->Regene(1, m_nSkillID);
			break;

		case REMOVE_BLESS:
			if (pTUser->m_bType4Buff[0] == 2) {
				pTUser->m_sDuration1 = 0;		
				pTUser->m_fStartTime1 = 0.0f;
				pTUser->m_sMaxHPAmount = 0;
				pTUser->m_bType4Buff[0] = 0;

				SendType4BuffRemove(m_pSkillTarget, 1);

				pTUser->SetSlotItemValue();
				pTUser->SetUserAbility();
				pTUser->Send2AI_UserUpdateInfo();
			
				buff_test = 0;
				for (i = 0 ; i < MAX_TYPE4_BUFF ; i++) {
					buff_test += pTUser->m_bType4Buff[i];
				}
				if (buff_test == 0) pTUser->m_bType4Flag = FALSE;

				bType4Test = TRUE ;
				for (j = 0 ; j < MAX_TYPE4_BUFF ; j++) {
					if (pTUser->m_bType4Buff[j] == 1) {
						bType4Test = FALSE;
						break;
					}
				}

				if (pTUser->isInParty() && bType4Test) 
					pTUser->SendPartyStatusUpdate(2, 0);
			}

			break;
	}

	if (pSkill->bType[1] == 0 || pSkill->bType[1] == 5)
		SendSkill();
}

void CMagicProcess::ExecuteType6(_MAGIC_TABLE *pSkill)
{
}

void CMagicProcess::ExecuteType7(_MAGIC_TABLE *pSkill)
{
}

void CMagicProcess::ExecuteType8(_MAGIC_TABLE *pSkill)	// Warp, resurrection, and summon spells.
{	
	if (pSkill == NULL)
		return;

	vector<CUser *> casted_member;
	_MAGIC_TYPE8* pType = g_pMain->m_Magictype8Array.GetData(pSkill->iNum);
	if (pType == NULL)
		return;

	if (m_pSkillTarget == -1)
	{
		// TO-DO: Localise this loop to make it not suck (the life out of the server).
		SessionMap & sessMap = g_pMain->s_socketMgr.GetActiveSessionMap();
		foreach (itr, sessMap)
		{		
			CUser* pTUser = static_cast<CUser *>(itr->second);
			if (UserRegionCheck(m_pSkillCaster, pTUser->GetSocketID(), pSkill->iNum, pType->sRadius, m_sData1, m_sData3))
				casted_member.push_back(pTUser);
		}
		g_pMain->s_socketMgr.ReleaseLock();

		if (casted_member.empty()) 
			return;	
	}
	else 
	{	// If the target was another single player.
		CUser* pTUser = g_pMain->GetUserPtr(m_pSkillTarget);
		if (pTUser == NULL) 
			return;
		
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
		_HOME_INFO* pHomeInfo = g_pMain->m_HomeArray.GetData(pTUser->m_pUserData->m_bNation);
		if (pHomeInfo == NULL)
			return;

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
				pTUser->m_MagicProcess.SendSkill(m_pSkillCaster, (*itr)->GetSocketID(), 
					m_opcode, m_nSkillID, m_sData1, 1, m_sData3);

				if (pTUser->GetMap() == NULL)
					continue;

				pEvent = pTUser->GetMap()->GetObjectEvent(pTUser->m_pUserData->m_sBind);

				if( pEvent ) {
					pTUser->Warp(uint16((pEvent->fPosX + x) * 10), uint16((pEvent->fPosZ + z) * 10));	
				}
				// TO-DO: Remove this hardcoded nonsense
				else if(pTUser->getNation() != pTUser->getZoneID() && pTUser->getZoneID() <= ELMORAD) 
				{	 // User is in different zone.
					if (pTUser->getNation() == KARUS) // Land of Karus
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
				pTUser->m_MagicProcess.SendSkill(m_pSkillCaster, (*itr)->GetSocketID(), 
					m_opcode, m_nSkillID, m_sData1, 1, m_sData3);

				pTUser->m_bResHpType = USER_STANDING;     // Target status is officially alive now.
				pTUser->HpChange(pTUser->m_iMaxHp, 0, false, m_pSrcUser->GetSocketID());	 // Refill HP.	
				pTUser->ExpChange( pType->sExpRecover/100 );     // Increase target experience.
				
				Packet result(AG_USER_REGENE);
				result << uint16((*itr)->GetSocketID()) << uint16(pTUser->getZoneID());
				g_pMain->Send_AIServer(&result);
			} break;

			case 12:	// Summon a target within the zone.	
				if (m_pSrcUser->getZoneID() != pTUser->getZoneID())   // Same zone? 
					goto packet_send;

				pTUser->m_MagicProcess.SendSkill(m_pSkillCaster, (*itr)->GetSocketID(), 
					m_opcode, m_nSkillID, m_sData1, 1, m_sData3);
					
				pTUser->Warp(m_pSrcUser->GetSPosX(), m_pSrcUser->GetSPosZ());
				break;

			case 13:	// Summon a target outside the zone.			
				if (m_pSrcUser->getZoneID() == pTUser->getZoneID())	  // Different zone? 
					goto packet_send;

				pTUser->m_MagicProcess.SendSkill(m_pSkillCaster, (*itr)->GetSocketID(), 
					m_opcode, m_nSkillID, m_sData1, 1, m_sData3);

				pTUser->ZoneChange(m_pSrcUser->getZoneID(), m_pSrcUser->m_pUserData->m_curx, m_pSrcUser->m_pUserData->m_curz) ;
				pTUser->UserInOut( USER_REGENE );
				//TRACE(" Summon ,, name=%s, x=%.2f, z=%.2f\n", pTUser->m_pUserData->m_id, pTUser->m_pUserData->m_curx, pTUser->m_pUserData->m_curz);
				break;

			case 20:	// Randomly teleport the source (within 20 meters)		
				pTUser->m_MagicProcess.SendSkill(m_pSkillCaster, (*itr)->GetSocketID(), 
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
		}

		bResult = 1;
		
packet_send:
		m_sData2 = bResult;
		SendSkill(m_pSkillCaster, (*itr)->GetSocketID());
	}
}

void CMagicProcess::ExecuteType9(_MAGIC_TABLE *pSkill)
{
}

short CMagicProcess::GetMagicDamage(int sid, int tid, int total_hit, int attribute)
{	
	CNpc* pMon;

	short damage = 0, temp_hit = 0, righthand_damage = 0, attribute_damage = 0 ; 
	int random = 0, total_r = 0 ;
	BYTE result; 

	CUser* pTUser = g_pMain->GetUserPtr(tid);  
	if (pTUser == NULL || pTUser->isDead()) return 0;

	if (sid >= NPC_BAND) {	// If the source is a monster.
		pMon = g_pMain->m_arNpcArray.GetData(sid);
		if( !pMon || pMon->m_NpcState == NPC_DEAD ) return 0;

		result = m_pSrcUser->GetHitRate( pMon->m_sHitRate / pTUser->m_sTotalEvasionrate ); 		
	}
	else {	// If the source is another player.
		total_hit = total_hit * m_pSrcUser->getStat(STAT_CHA) / 170;
		result = SUCCESS ;
	}
		
	if (result != FAIL) {		// In case of SUCCESS.... 
		switch (attribute) {
			case NONE_R :
				total_r = 0;		
				break;
			case FIRE_R	:
				total_r = pTUser->m_bFireR + pTUser->m_bFireRAmount ;
				break;
			case COLD_R :
				total_r = pTUser->m_bColdR + pTUser->m_bColdRAmount ;
				break;
			case LIGHTNING_R :
				total_r = pTUser->m_bLightningR + pTUser->m_bLightningRAmount ; 
				break;
			case MAGIC_R :
				total_r = pTUser->m_bMagicR + pTUser->m_bMagicRAmount ;
				break;
			case DISEASE_R :
				total_r = pTUser->m_bDiseaseR + pTUser->m_bDiseaseRAmount ;
				break;
			case POISON_R :			
				total_r = pTUser->m_bPoisonR + pTUser->m_bPoisonRAmount ;
				break;
			case LIGHT_R :
				// LATER !!!
				break;
			case DARKNESS_R	:
				// LATER !!!
				break;
		}
		
		if ( sid >= 0 && sid < MAX_USER) {
			if( m_pSrcUser->m_pUserData->m_sItemArray[RIGHTHAND].nNum != 0 ) {	// Does the magic user have a staff?
				_ITEM_TABLE* pRightHand = NULL;
				pRightHand = g_pMain->GetItemPtr(m_pSrcUser->m_pUserData->m_sItemArray[RIGHTHAND].nNum);

				if( pRightHand && m_pSrcUser->m_pUserData->m_sItemArray[LEFTHAND].nNum == 0 && pRightHand->m_bKind / 10 == WEAPON_STAFF) {				
					righthand_damage = pRightHand->m_sDamage ;
					
					if (m_pSrcUser->m_bMagicTypeRightHand == attribute) {
						attribute_damage = pRightHand->m_sDamage ;	
					}
				}
				else {
					righthand_damage = 0 ;
				}
			}
		}

		damage = (short)(total_hit - ((0.7 * total_hit * total_r) / 200)) ;
		random = myrand (0, damage);
		damage = (short)((0.7 * (total_hit - ((0.9 * total_hit * total_r) / 200))) + 0.2 * random);
//	
		if (sid >= NPC_BAND) {
			damage -= (3 * righthand_damage) - (3 * attribute_damage);
		}
		else{
			if (attribute != 4) {	// Only if the staff has an attribute.
				damage -= (short)(((righthand_damage * 0.8f) + (righthand_damage * m_pSrcUser->getLevel()) / 60) - ((attribute_damage * 0.8f) + (attribute_damage * m_pSrcUser->getLevel()) / 30));
			}


		}
//
	}

	damage = damage / 3 ;	// ������ ��û 

	return damage ;		
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

// �񷯸ӱ� ������ ��Ƽ ��ȯ >.<
			if( !pTUser->isInParty()) {
				if (sid == tid) {
					return TRUE; 
				}
				else {
					return FALSE; 
				}
			}			

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
		if ( pTUser->m_pUserData->m_bZone == m_pSrcUser->m_pUserData->m_bZone ) {		// Zone Check!
			if ( (pTUser->m_RegionX == m_pSrcUser->m_RegionX) && (pTUser->m_RegionZ == m_pSrcUser->m_RegionZ) ) { // Region Check!
				if (radius !=0) { 	// Radius check! ( ...in case there is one :(  )
					float temp_x = pTUser->m_pUserData->m_curx - mousex;
					float temp_z = pTUser->m_pUserData->m_curz - mousez;
					float distance = pow(temp_x, 2.0f) + pow(temp_z, 2.0f);
					if ( distance > pow((float)radius, 2.0f) ) return FALSE ;
				}		
				return TRUE;	// Target is in the area.
			}
		}	
	}
	else {			// When monsters attack...
		if ( pTUser->getZoneID() == pMon->getZoneID() ) {		// Zone Check!
			if ( (pTUser->m_RegionX == pMon->m_sRegion_X) && (pTUser->m_RegionZ == pMon->m_sRegion_Z) ) { // Region Check!
				if (radius !=0) { 	// Radius check! ( ...in case there is one :(  )
					float temp_x = pTUser->m_pUserData->m_curx - pMon->m_fCurX;
					float temp_z = pTUser->m_pUserData->m_curz - pMon->m_fCurZ;
					float distance = pow(temp_x, 2.0f) + pow(temp_z, 2.0f);	
					if ( distance > pow((float)radius, 2.0f) ) return FALSE ;
				}		
				return TRUE;	// Target is in the area.
			}
		}	
	}

	return FALSE;
}

void CMagicProcess::Type4Cancel(int magicid, short tid)
{
	CUser* pTUser = g_pMain->GetUserPtr(tid);  
	if (pTUser == NULL) 
		return;

	_MAGIC_TYPE4* pType = g_pMain->m_Magictype4Array.GetData(magicid);
	if (pType == NULL)
		return;

	BOOL buff = FALSE;
	BYTE buff_type = pType->bBuffType; 

	switch (buff_type) {		
		case 1:
			if (pTUser->m_sMaxHPAmount > 0) {
				pTUser->m_sDuration1 = 0;		
				pTUser->m_fStartTime1 = 0.0f;
				pTUser->m_sMaxHPAmount = 0;		
				buff = TRUE;
			}
			break;

		case 2:
			if (pTUser->m_sACAmount > 0) {
				pTUser->m_sDuration2 = 0;		
				pTUser->m_fStartTime2 = 0.0f;
				pTUser->m_sACAmount = 0;
				buff = TRUE;
			}
			break;
// 
		case 3:
			pTUser->m_sDuration3 = 0;		
			pTUser->m_fStartTime3 = 0.0f;
			pTUser->StateChangeServerDirect(3, ABNORMAL_NORMAL);
			buff = TRUE;	
			break;
//  
		case 4:
			if (pTUser->m_bAttackAmount > 100) {
				pTUser->m_sDuration4 = 0;		
				pTUser->m_fStartTime4 = 0.0f;
				pTUser->m_bAttackAmount = 100;
				buff = TRUE;
			}
			break;

		case 5:
			if (pTUser->m_bAttackSpeedAmount > 100) {
				pTUser->m_sDuration5 = 0;		
				pTUser->m_fStartTime5 = 0.0f;
				pTUser->m_bAttackSpeedAmount = 100;	
				buff = TRUE;
			}
			break;	

		case 6:	
			if (pTUser->m_bSpeedAmount > 100) {
				pTUser->m_sDuration6 = 0;		
				pTUser->m_fStartTime6 = 0.0f;
				pTUser->m_bSpeedAmount = 100;
				buff = TRUE;
			}
			break;

		case 7:	
			if (pTUser->getStatBuffTotal() > 0) {
				pTUser->m_sDuration7 = 0;		
				pTUser->m_fStartTime7 = 0.0f;
				// TO-DO: Implement reset
				memset(pTUser->m_bStatBuffs, 0, sizeof(uint8) * STAT_COUNT);
				buff = TRUE;
			}
			break;

		case 8:	
			if ((pTUser->m_bFireRAmount + pTUser->m_bColdRAmount + pTUser->m_bLightningRAmount +
				pTUser->m_bMagicRAmount + pTUser->m_bDiseaseRAmount + pTUser->m_bPoisonRAmount) > 0) {
				pTUser->m_sDuration8 = 0;		
				pTUser->m_fStartTime8 = 0.0f;
				pTUser->m_bFireRAmount = 0;
				pTUser->m_bColdRAmount = 0;
				pTUser->m_bLightningRAmount = 0;
				pTUser->m_bMagicRAmount = 0;
				pTUser->m_bDiseaseRAmount = 0;
				pTUser->m_bPoisonRAmount = 0;
				buff = TRUE;
			}
			break;	

		case 9:	
			if ((pTUser->m_bHitRateAmount + pTUser->m_sAvoidRateAmount) > 200) {
				pTUser->m_sDuration9 = 0;		
				pTUser->m_fStartTime9 = 0.0f;
				pTUser->m_bHitRateAmount = 100;
				pTUser->m_sAvoidRateAmount = 100;
				buff = TRUE;
			}
			break;
	}
	
	if (buff)
	{
		pTUser->m_bType4Buff[buff_type - 1] = 0;
		pTUser->SetSlotItemValue();
		pTUser->SetUserAbility();
		pTUser->SendItemMove(2);
		pTUser->Send2AI_UserUpdateInfo();

		Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE4_END));
		result << buff_type;
		pTUser->Send(&result);
	}

	int buff_test = 0;
	for (int i = 0; i < MAX_TYPE4_BUFF; i++)
		buff_test += pTUser->m_bType4Buff[i];
	if (buff_test == 0) pTUser->m_bType4Flag = FALSE;	

	if (pTUser->isInParty() && !pTUser->m_bType4Flag)
		pTUser->SendPartyStatusUpdate(2);
}

void CMagicProcess::Type3Cancel(int magicid, short tid)
{
	CUser* pTUser = g_pMain->GetUserPtr(tid);  
	if (pTUser == NULL) 
		return;

	_MAGIC_TYPE3* pType = g_pMain->m_Magictype3Array.GetData(magicid);
	if (pType == NULL)
		return;

	for (int i = 0 ; i < MAX_TYPE3_REPEAT ; i++) {
		if (pTUser->m_bHPAmount[i] > 0) {
			pTUser->m_fHPStartTime[i] = 0.0f;
			pTUser->m_fHPLastTime[i] = 0.0f;   
			pTUser->m_bHPAmount[i] = 0;
			pTUser->m_bHPDuration[i] = 0;				
			pTUser->m_bHPInterval[i] = 5;
			pTUser->m_sSourceID[i] = -1;
			break;
		}
	}

	Packet result(WIZ_MAGIC_PROCESS, uint8(MAGIC_TYPE3_END));
	result << uint8(100);
	pTUser->Send(&result); 

	int buff_test = 0;
	for (int j = 0; j < MAX_TYPE3_REPEAT; j++)
		buff_test += pTUser->m_bHPDuration[j];
	if (buff_test == 0) pTUser->m_bType3Flag = FALSE;	

	if (pTUser->isInParty() && !pTUser->m_bType3Flag)
		pTUser->SendPartyStatusUpdate(1, 0);
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
