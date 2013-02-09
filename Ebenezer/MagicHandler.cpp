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

enum effect_type
{
	ATTACK_SKILL = 1,
	FLYING_SKILL = 2,
	ATTACK_SKILL_BONUS = 3,
	BUFF_SKILL = 4,
	CURING_SKILL = 5,
	TRANSFORMATION_SKILL = 6,
	MONSTER_STATUS_CHANGE_SKILL = 7,
	TELEPORT_SKILL = 8,
	MAGIC_EFFECT_9 = 9
};

void CUser::MagicSystem( Packet & pkt )
{
	uint8 command, subcommand;
	uint32 magicid;
	time_t skill_received_time;
	uint16 sid, tid, data1, data2, data3, data4, data5, data6, data7;
	CUser *pTargetUser = NULL;
	CNpc *pMon = NULL;

	skill_received_time = GetTickCount(); //Retrieve the time at which the Magic packet is going for internal processing.

	command = pkt.GetOpcode(); //This is actually WIZ_MAGIC_PROCESS
	pkt >> subcommand >> magicid >> sid >> tid;

	if( sid < 0 || tid < 0 || tid > INVALID_BAND || sid != (uint16)GetSocketID()) //Return if there's an invalid source or target received.
		return;

	if( sid < MAX_USER && isDead() )
		return;

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
	Do ALL required pre-liminary checks here, wrapped into CanCast()
	*/
	if(!CanCast(magicid, sid, tid))
		return;

	//if(!CheckSkillCooldown(magicid, skill_received_time)) //Check if the skill is off-cooldown.
	//	return;

	//LogSkillCooldown(magicid, skill_received_time); Use this <if> the skill is successfully casted!

	pkt >> data1 >> data2 >> data3 >> data4 >> data5 >> data6 >> data7;

	switch(subcommand)
	{
	case MAGIC_CASTING:
		goto echo;
		break;
	case MAGIC_FLYING:
		break;
	case MAGIC_EFFECTING:
		MagicType(1, 1, magicid, sid, tid, data1, data2, data3, data4, data5, data6, data7); //First byte : subcommand, second byte : subtype
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
		m_pMain->Send_Region( &result, GetMap(), m_RegionX, m_RegionZ );
	}
	else if ( sid >= NPC_BAND)
	{ 
		m_pMain->Send_Region( &result, pMon->GetMap(), pMon->m_sRegion_X, pMon->m_sRegion_Z );
	}
}

bool CUser::CheckSkillCooldown(uint32 magicid, time_t skill_received_time)
{
	std::map<uint32, time_t>::iterator it;
	_MAGIC_TABLE* pMagic = m_pMain->m_MagictableArray.GetData( magicid );
	if( !pMagic ) //Return before processing anything as there is no skill with this ID. (When the all-wrapping check is created this can be removed)
		return false;

	if( m_CoolDownList.empty() )
		return true;

	it = m_CoolDownList.find(magicid);
	if( it == m_CoolDownList.end() ) // Incase there is no such entry in the cooldown map the skill will be off-cooldown, thus allow it to cast.
		return true;

	if((skill_received_time - m_CoolDownList.find(magicid)->second) < ((pMagic->bReCastTime + pMagic->bCastTime) * 100) )
	{
		return false;
	}
	return true;
}

void CUser::LogSkillCooldown(uint32 magicid, time_t skill_received_time)
{
	std::map<uint32, time_t>::iterator it;
	it = m_CoolDownList.find(magicid);
	
	if( it == m_CoolDownList.end() ) //find() returns a pointer to end() incase it didn't find anything that matches the magicid, thus requiring a new entry.
		m_CoolDownList.insert(std::pair<uint32, time_t>(magicid, skill_received_time));
	else //in this case it did find one, thus update the last time the skill was used.
		m_CoolDownList.find(magicid)->second = skill_received_time;	
}

void CUser::MagicType1(uint32 magicid, uint16 sid, uint16 tid, uint16 data1, uint16 data2, uint16 data3, uint16 data4, uint16 data5, uint16 data6, uint16 data7)
{

	int16 damage = 0;

	_MAGIC_TABLE* pMagic = m_pMain->m_MagictableArray.GetData( magicid ); //Checking if the skill exists has already happened.

	_MAGIC_TYPE1* pMagic_Type1 = m_pMain->m_Magictype1Array.GetData( magicid );
	if( !pMagic_Type1 ) //Shouldn't be necessary unless there's a mismatch in the database.
		return;

	CUser* pTUser = m_pMain->GetUserPtr(tid);     // Get target info.
	if (!pTUser || pTUser->isDead())
		return;

	damage = GetDamage(tid, magicid); //Get the amount of damage that will be inflicted.

	pTUser->HpChange( -damage );     // Reduce target health point.

	if( pTUser->isDead() ) {    // Check if the target is dead.
		pTUser->m_bResHpType = USER_DEAD;     // Target status is officially dead now.

		if(sid >= NPC_BAND)
			pTUser->ExpChange( -pTUser->m_iMaxExp/100 );     // Reduce target's experience if the source was an NPC.

		if( !isInParty() ) {    // If the user is not in a party allocate all the National Points to the user, ifnot, divide it between the party.
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
		MagicType(pMagic->bType2, 1, magicid, sid, tid, data1, data2, data3, data4, data5, data6, data7); //If the skill has a second effect, be sure to cast that one too. (DONT FORGET THE SUB_TYPE HERE!!)

packet_send:
	if (pMagic->bType2 == 0 || pMagic->bType2 == 1) {
		Packet result(WIZ_MAGIC_PROCESS);
		result << MAGIC_EFFECTING << magicid << sid << tid << data1 << data2 << data3;
		if (damage == 0)
			result << int16(-104);
		else
			result << uint16(0);

		m_pMain->Send_Region(&result, GetMap(), m_RegionX, m_RegionZ);
	}
	return;
}

void CUser::MagicType4(uint32 magicid, uint16 sid, uint16 tid, uint16 data1, uint16 data2, uint16 data3, uint16 data4)
{
	int damage = 0;
	Packet result(WIZ_MAGIC_PROCESS);

	vector<int> casted_member;

	_MAGIC_TABLE* pMagic = pMagic = m_pMain->m_MagictableArray.GetData( magicid );

	_MAGIC_TYPE4* pType = pType = m_pMain->m_Magictype4Array.GetData( magicid );
	if (!pType)
		return;

	if (tid == -1) { //Means the source is targetting his whole party.
		for (int i = 0 ; i < MAX_USER ; i++) { //This however, what the fuck? Definitely need to remember making this better when doing the party system!
			CUser* pTUser = (CUser*)m_pMain->m_Iocport.m_SockArray[i];
			if( !pTUser || pTUser->m_bResHpType == USER_DEAD || pTUser->m_bAbnormalType == ABNORMAL_BLINKING) continue ;

			//if (UserRegionCheck(sid, i, magicid, pType->bRadius, data1, data3)) 
			//	casted_member.push_back(i);
		}

		if (casted_member.empty()) {
			result << MAGIC_FAIL << magicid
				<< sid << tid
				<< uint16(0) << uint16(0) << uint16(0) << uint16(0) << uint16(0) << uint16(0);

			if (sid >= 0 && sid < MAX_USER) {
				m_pMain->Send_Region(&result, GetMap(), m_RegionX, m_RegionZ, NULL );
			}
			return;	
		}
	}
	else //Means the target is another user
	{
		CUser* pTUser = m_pMain->GetUserPtr(tid);
		if (pTUser == NULL)
			return;
		
		casted_member.push_back(tid);
	}

	foreach (itr, casted_member)
	{
		CUser* pTUser = m_pMain->GetUserPtr(*itr) ;     // Get target info.  
		if (!pTUser || pTUser->isDead()) continue;

		if (pTUser->m_bType4Buff[pType->bBuffType - 1] == 2 && tid == -1) {		// Is this buff-type already casted on the player?
			result = 0 ;				// If so, fail! 
			goto fail_return;					
		}

		//Will handle saving scrolls etc. differently - replace m_bType4Buff with list with extra doSave flag.

		switch (pType->bBuffType) {	// Depending on which buff-type it is.....
			case 1 :
				pTUser->m_sMaxHPAmount = pType->sMaxHP;		// Get the amount that will be added/multiplied.
				pTUser->m_sDuration1 = pType->sDuration;	// Get the duration time.
				pTUser->m_fStartTime1 = TimeGet();			// Get the time when Type 4 spell starts.	
				break;
			case 2 :
				pTUser->m_sACAmount = pType->sAC;
				pTUser->m_sDuration2 = pType->sDuration;
				pTUser->m_fStartTime2 = TimeGet();
				break;

			case 3 : 
				if (magicid == 490034) // Bezoar
					StateChangeServerDirect(3, ABNORMAL_GIANT);
				else if (magicid == 490035) // Rice cake
					StateChangeServerDirect(3, ABNORMAL_DWARF);

				pTUser->m_sDuration3 = pType->sDuration;
				pTUser->m_fStartTime3 = TimeGet();
				break;

			case 4 :
				pTUser->m_bAttackAmount = pType->bAttack;
				pTUser->m_sDuration4 = pType->sDuration;
				pTUser->m_fStartTime4 = TimeGet();					
				break;

			case 5 :
				pTUser->m_bAttackSpeedAmount = pType->bAttackSpeed;
				pTUser->m_sDuration5 = pType->sDuration;
				pTUser->m_fStartTime5 = TimeGet();
				break;

			case 6 :
				pTUser->m_bSpeedAmount = pType->bSpeed;
				pTUser->m_sDuration6 = pType->sDuration;
				pTUser->m_fStartTime6 = TimeGet();
				break;

			case 7 :
				pTUser->m_bStrAmount = pType->bStr;
				pTUser->m_bStaAmount = pType->bSta;
				pTUser->m_bDexAmount = pType->bDex;
				pTUser->m_bIntelAmount = pType->bIntel;
				pTUser->m_bChaAmount = pType->bCha;	
				pTUser->m_sDuration7 = pType->sDuration;
				pTUser->m_fStartTime7 = TimeGet();
				break;

			case 8 :
				pTUser->m_bFireRAmount = pType->bFireR;
				pTUser->m_bColdRAmount = pType->bColdR;
				pTUser->m_bLightningRAmount = pType->bLightningR;
				pTUser->m_bMagicRAmount = pType->bMagicR;
				pTUser->m_bDiseaseRAmount = pType->bDiseaseR;
				pTUser->m_bPoisonRAmount = pType->bPoisonR;
				pTUser->m_sDuration8 = pType->sDuration;
				pTUser->m_fStartTime8 = TimeGet();
				break;

			case 9 :
				pTUser->m_bHitRateAmount = pType->bHitRate;
				pTUser->m_sAvoidRateAmount = pType->sAvoidRate;
				pTUser->m_sDuration9 = pType->sDuration;
				pTUser->m_fStartTime9 = TimeGet();
				break;	

			default :
				result = 0 ;
				goto fail_return;
		}

		if ((tid != -1 && pMagic->bType1 == 4) && (sid >= 0 && sid < MAX_USER))
				MSpChange( -(pMagic->sMsp) );

		if (sid >= 0 && sid < MAX_USER) {
			if (m_pUserData->m_bNation == pTUser->m_pUserData->m_bNation)
				pTUser->m_bType4Buff[pType->bBuffType - 1] = 2;
			else
				pTUser->m_bType4Buff[pType->bBuffType - 1] = 1;
		}
		else
			pTUser->m_bType4Buff[pType->bBuffType - 1] = 1;

		pTUser->m_bType4Flag = TRUE;

		pTUser->SetSlotItemValue();
		pTUser->SetUserAbility();

		if (pTUser->m_sPartyIndex != -1 && pTUser->m_bType4Buff[pType->bBuffType - 1] == 1) {
			Packet partypacket(WIZ_PARTY);
			partypacket << PARTY_STATUSCHANGE << tid << uint8(2) << uint8(1);
			m_pMain->Send_PartyMember(pTUser->m_sPartyIndex, &partypacket);
		}
		pTUser->Send2AI_UserUpdateInfo();

		if ( pMagic->bType2 == 0 || pMagic->bType2 == 4 ) {
			result << MAGIC_EFFECTING << magicid << sid << uint16(*itr) << data1 << 1 //result
				<< data3 << uint16(pType->sDuration) << uint8(0) << uint16(pType->bSpeed);

			if (sid >=0 && sid < MAX_USER)
				m_pMain->Send_Region(&result, GetMap(), m_RegionX, m_RegionZ, NULL);
			else
				m_pMain->Send_Region(&result, pTUser->GetMap(), pTUser->m_RegionX, pTUser->m_RegionZ, NULL);
		}
		result = 1;	
		continue; 

fail_return:
		if ( pMagic->bType2 == 4 ) {
			result << MAGIC_EFFECTING << magicid << sid << uint16(*itr) << data1 << 0 //result(is always 0)
				<< data3;

			if( data4 != 0 )
				result << data4;
			else
				result << uint16(pType->sDuration);

			result << uint16(0) << uint16(pType->bSpeed);

			if (sid >= 0 && sid < MAX_USER)
				m_pMain->Send_Region(&result, GetMap(), m_RegionX, m_RegionZ, NULL);
			else
				m_pMain->Send_Region(&result, pTUser->GetMap(), pTUser->m_RegionX, pTUser->m_RegionZ, NULL);
		}
		
		if (sid >= 0 && sid < MAX_USER) {
			Packet result(WIZ_MAGIC_PROCESS);
			result << MAGIC_FAIL << magicid << sid << uint16(*itr) << uint16(0) << uint16(0) << uint16(0) << uint16(0) << uint16(0) << uint16(0);
			Send( &result );
		}

		result = 1;	
		continue;
	}
}

void CUser::MagicType(uint16 effect_type, uint8 sub_type, uint32 magicid, uint16 sid, uint16 tid, uint16 data1, uint16 data2, uint16 data3, uint16 data4, uint16 data5, uint16 data6, uint16 data7)
{
	switch(effect_type)
	{
	case ATTACK_SKILL:
		MagicType1(magicid, sid, tid, data1, data2, data3, data4, data5, data6, data7);
		break;
	case FLYING_SKILL:
		if(sub_type == MAGIC_FLYING) //Basically a check if the player has enough mana and arrows.
			break;					 //subcommand will be MAGIC_FAIL and it'll just echo if the player has insufficient of either.
		break;
	case ATTACK_SKILL_BONUS:
		if(sub_type == MAGIC_CANCEL || sub_type == MAGIC_CANCEL2) //Cancelling type 3 magic.
			break;
		break;
	case BUFF_SKILL:
		if(sub_type == MAGIC_CANCEL || sub_type == MAGIC_CANCEL2) //Cancelling type 4 magic.
			break;
		break;
	case CURING_SKILL:
		break;
	case TRANSFORMATION_SKILL:
		break;
	case MONSTER_STATUS_CHANGE_SKILL:
		break;
	case TELEPORT_SKILL:
		break;
	case MAGIC_EFFECT_9:
		break;
	}
}

bool CUser::CanCast(uint32 magicid, uint16 sid, uint16 tid)
{
	CUser *pTargetUser = NULL;
	CNpc *pMon = NULL;
	_MAGIC_TABLE *pMagic = NULL;

	pMagic = m_pMain->m_MagictableArray.GetData( magicid );
	if(!pMagic)
		return false;

	if(tid < MAX_USER)
		pTargetUser = m_pMain->GetUserPtr(tid);
	else if(tid >= NPC_BAND)
		pMon = m_pMain->m_arNpcArray.GetData(tid);

	if(pMagic->iUseItem != 0 && pMagic->bType1 != 5 && !CanUseItem(pMagic->iUseItem)) //The user does not meet the item's requirements or does not have any of said item.
			return false;

	if((this)->m_pUserData->m_sClass != (pMagic->sSkill % 10)    //Trying to use a skill that is not meant to be casted by this character, either not mastered or totally different class' skill.
		|| pMagic->sSkillLevel > (this)->m_pUserData->m_bLevel)  //Skill level check.
		return false;

	if(((this)->m_pUserData->m_sMp - pMagic->sMsp) < 0) //This user does not have enough mana for this skill.
		return false;

	if( (pTargetUser != NULL && (pTargetUser->getZoneID() != (this)->getZoneID())) //The source and target aren't even in the same zone, how could they possibly interact!
		|| (pTargetUser != NULL && !pTargetUser->isAttackZone())				   //The target is not in an attackable zone!
		|| (pMon != NULL && (pMon->getZoneID() != (this)->getZoneID()))) 
		return false;


	//Checks for type 3 / 4 - already casted upon target?

	//Checks for "moral" and corresponding targets etc.

	//Zone checks in general - if both target and source are players

	//Cooldown checks

	//Check source validity when player and NPC

	//Check target validity when player and NPC

	//Check nations, player vs player, player vs npc, npc vs player.
	
	//Class checks for the magic being casted.

	//Weapon checks incase of dualwield-only attacks, double-handed-only attacks, staff-only attacks etc.

	//
	return true;
}