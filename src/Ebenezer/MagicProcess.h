#pragma once

class Packet;
class Unit;
struct _MAGIC_TABLE;
class CMagicProcess  
{
public:
	static void MagicPacket(Packet & pkt, Unit * pCaster = nullptr, bool isRecastingSavedMagic = false);
	static void CheckExpiredType6Skills(Unit * pTarget);
	static void CheckExpiredType9Skills(Unit * pTarget);
	static bool UserRegionCheck(Unit * pSkillCaster, Unit * pSkillTarget, _MAGIC_TABLE * pSkill, int radius, short mousex = 0, short mousez = 0);
	static bool GrantType4Buff(_MAGIC_TABLE * pSkill, _MAGIC_TYPE4 *pType, Unit * pCaster, Unit *pTarget);
	static bool RemoveType4Buff(uint8 byBuffType, Unit *pTarget);
};