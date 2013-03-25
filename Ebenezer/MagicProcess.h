#pragma once

class Packet;
class Unit;
struct _MAGIC_TABLE;
class CMagicProcess  
{
public:
	static void MagicPacket(Packet & pkt, Unit * pCaster = NULL, bool isRecastingSavedMagic = false);
	static void SendType4BuffRemove(short tid, BYTE buff);
	static void CheckExpiredType6Skills(Unit * pTarget);
	static bool UserRegionCheck(Unit * pSkillCaster, Unit * pSkillTarget, _MAGIC_TABLE * pSkill, int radius, short mousex = 0, short mousez = 0);
};