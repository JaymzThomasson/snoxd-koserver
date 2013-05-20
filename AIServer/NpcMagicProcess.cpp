#include "stdafx.h"
#include "NpcMagicProcess.h"
#include "ServerDlg.h"

void CNpcMagicProcess::MagicPacket(uint8 opcode, uint32 nSkillID, int16 sCasterID, int16 sTargetID, int16 sData1, int16 sData2, int16 sData3)
{
	Packet result(AG_MAGIC_ATTACK_REQ, opcode);
	result << nSkillID << sCasterID << sTargetID << sData1 << sData2 << sData3;
	g_pMain->Send(&result);
}
