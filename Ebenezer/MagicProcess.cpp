#include "stdafx.h"
#include "EbenezerDlg.h"
#include "MagicProcess.h"
#include "MagicInstance.h"
#include "User.h" // need to move UserRegionCheck() to get rid of this

void CMagicProcess::MagicPacket(Packet & pkt, Unit * pCaster /*= NULL*/, bool isRecastingSavedMagic /*= false*/)
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

	// Prevent users from faking other players or NPCs.
	if (pCaster != NULL // if it's NULL, it's from AI.
		&& (instance.pSkillCaster == NULL 
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
		|| pSkillTarget == NULL)
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

			if (TO_USER(pSkillTarget)->m_sPartyIndex == TO_USER(pSkillCaster)->m_sPartyIndex 
				&& pSkill->bType[0] != 8)
				goto final_test;
			else if (TO_USER(pSkillTarget)->m_sPartyIndex == TO_USER(pSkillCaster)->m_sPartyIndex 
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
