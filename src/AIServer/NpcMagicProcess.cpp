#include "stdafx.h"
#include "NpcMagicProcess.h"
#include "ServerDlg.h"

void CNpcMagicProcess::MagicPacket(uint8 opcode, uint32 nSkillID, int16 sCasterID, int16 sTargetID, int16 sData1, int16 sData2, int16 sData3)
{
	Packet result(AG_MAGIC_ATTACK_REQ, opcode);
	result << nSkillID << sCasterID << sTargetID << sData1 << sData2 << sData3;
	g_pMain->Send(&result);

	// NOTE: Client controls skill hits.
	// Since we block these, we need to handle this ourselves.
	// For now, we'll just block the NPC's thread... 
	// this WILL need to be rewritten once spawns are though, as it is HORRIBLE.
	if (opcode != MAGIC_EFFECTING)
	{
		_MAGIC_TABLE * pSkill = g_pMain->m_MagictableArray.GetData(nSkillID);
		if (pSkill == nullptr)
			return;

		sleep(pSkill->bCastTime * SECOND);
		result.put(0, uint8(MAGIC_EFFECTING));
		g_pMain->Send(&result);
	}
}
