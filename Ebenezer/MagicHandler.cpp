// MagicHandler.cpp: implementation of the CMagicHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"
#include "AIPacket.h"

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

void CUser::MagicSystem( Packet & pkt )
{
	uint8 command, subcommand;
	uint32 magicid, skill_received_time;
	uint16 sid, tid, data1, data2, data3, data4, data5, data6, data7;
	CUser *pUser, *pTargetUser = NULL;
	CNpc *pMon = NULL;

	skill_received_time = TimeGetInt(); //Retrieve the time at which the Magic packet is going for internal processing.

	command = pkt.GetOpcode();
	pkt >> subcommand >> magicid >> sid >> tid;

	if( sid < 0 || tid < 0 || tid > INVALID_BAND || sid != (uint16)GetSocketID()) //Return if there's an invalid source or target received.
		return;

	if( sid < MAX_USER )
	{
		if (isDead())	
			return;
	}

	if(tid >= NPC_BAND)
	{
		pMon = m_pMain->m_arNpcArray.GetData(tid);
		if( !pMon || pMon->m_NpcState == NPC_DEAD ) 
			return;
	}
	else if( tid < MAX_USER )
	{
		pTargetUser = m_pMain->GetUserPtr(tid);
		if ( !pTargetUser )
			return;
	}

	/*
	Do ALL required pre-liminary checks here
	*/
	if(!CheckSkillCooldown(magicid, skill_received_time)) //Check if the skill is off-cooldown.
		return;

	//LogSkillCooldown(magicid, skill_received_time); Use this <if> the skill is successfully casted!

	pkt >> data1 >> data2 >> data3 >> data4 >> data5 >> data6 >> data7;

	switch(command)
	{
	case MAGIC_CASTING:
		break;
	case MAGIC_FLYING:
		break;
	case MAGIC_EFFECTING:
		break;
	case MAGIC_FAIL:
		goto echo;
		break;
	case MAGIC_TYPE3_END: //This is also MAGIC_TYPE4_END
		break;
	case MAGIC_CANCEL:
		break;
	case MAGIC_CANCEL2:
		break;
	}

echo :
	Packet result(WIZ_MAGIC_PROCESS);
	result << subcommand << magicid << sid << tid << data1 << data2 << data3 << data4 << data5 << data6 << data7;

	if (sid < MAX_USER)
	{
		m_pMain->Send_Region( &result, pUser->GetMap(), pUser->m_RegionX, pUser->m_RegionZ, NULL, false );
	}
	else if ( sid >= NPC_BAND)
	{ 
		m_pMain->Send_Region( &result, pMon->GetMap(), pMon->m_sRegion_X, pMon->m_sRegion_Z, NULL, false );
	}
}

BOOL CUser::CheckSkillCooldown(int32 magicid, int32 skill_received_time)
{
	int result = true;
	_MAGIC_TABLE* pMagic = m_pMain->m_MagictableArray.GetData( magicid );
	if( !pMagic ) //Return before processing anything as there is no skill with this ID.
		return false;

	if( m_CoolDownList.empty() )
		return result;

	foreach (itr, m_CoolDownList)
		{
			if ((*itr)->magicid == magicid && ( ( skill_received_time - (*itr)->last_used_time) < ( (int32)pMagic->bReCastTime + (int32)pMagic->bCastTime) ) )
			{
				result = false;
				break;
			}
		}
	return result;
}

void CUser::LogSkillCooldown(int32 magicid, int32 skill_received_time)
{	
	bool new_entry = true; //Flag to indicate a new entry requirement.
	foreach(itr, m_CoolDownList)
	{
		if((*itr)->magicid == magicid) 
		{
			(*itr)->last_used_time = skill_received_time;
			new_entry = false;
		}
	}

	if(new_entry = true) //If this magic id hasn't been casted before, add it to the cooldownlist
	{
		_SKILL_COOLDOWN* pCooldown;
		pCooldown = new _SKILL_COOLDOWN;
		pCooldown->magicid = magicid;
		pCooldown->last_used_time = skill_received_time;
		m_CoolDownList.push_front(pCooldown);
	}
}

void CUser::MagicType1(int32 magicid, int16 sid, int16 tid, uint16 data1, uint16 data2, uint16 data3, uint16 data4, uint16 data5, uint16 data6, uint16 data7)
{

	int16 damage = GetDamage(tid, magicid); //Get the amount of damage that will be inflicted.

	_MAGIC_TABLE* pMagic = m_pMain->m_MagictableArray.GetData( magicid ); //Checking if the skill exists has already happened.

	_MAGIC_TYPE1* pMagic_Type1 = m_pMain->m_Magictype1Array.GetData( magicid );
	if( !pMagic_Type1 ) //Shouldn't be necessary unless there's a mismatch in the database.
		return;

	CUser* pTUser = m_pMain->GetUserPtr(tid);     // Get target info.
	if (!pTUser || pTUser->isDead())
		return;

	pTUser->HpChange( -damage );     // Reduce target health point.

	if( pTUser->isDead() ) {    // Check if the target is dead.
		pTUser->m_bResHpType = USER_DEAD;     // Target status is officially dead now.

		if(sid >= NPC_BAND)
			pTUser->ExpChange( -pTUser->m_iMaxExp/100 );     // Reduce target's experience if the source was an NPC.

		if( m_sPartyIndex == -1 ) {    // If the user is not in a party allocate all the National Points to the user, ifnot, divide it between the party.
			LoyaltyChange(tid);
		}
		else {
			LoyaltyDivide(tid);
		}

		GoldChange(tid, 0); //Reward the killer with the money he deserves.

		pTUser->InitType3();	// Re-initialize buffs on the dead person
		pTUser->InitType4();	// Re-initialize buffs on the dead person

		pTUser->m_sWhoKilledMe = sid;
	} 
	SendTargetHP( 0, tid, -damage );     // Change the HP of the target.
	if(pMagic->bType2 > 0 && pMagic->bType2 != 1)
		MagicType(pMagic->bType2); //If the skill has a second effect, be sure to cast that one too.

packet_send:
	if (pMagic->bType2 == 0 || pMagic->bType2 == 1) {
		Packet result(WIZ_MAGIC_PROCESS);
		result << MAGIC_EFFECTING << magicid << sid << tid << data1 << data2 << data3;
		if (damage == 0) {
			result << int16(-104);
		}
		else {
			result << uint16(0);
		}

		m_pMain->Send_Region( &result, GetMap(), m_RegionX, m_RegionZ, NULL, false );
	}

	return;
}

void CUser::MagicType(uint16 effect_type)
{
	switch(effect_type)
	{
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	case 5:
		break;
	case 6:
		break;
	case 7:
		break;
	case 8:
		break;
	}
}