#pragma once

#define NONE_R				0	
#define	FIRE_R				1
#define	COLD_R				2
#define LIGHTNING_R			3
#define MAGIC_R				4
#define DISEASE_R			5
#define POISON_R			6
#define LIGHT_R				7
#define DARKNESS_R			8

class Packet;
class Unit;
struct _MAGIC_TABLE;

class CMagicProcess  
{
public:
	short GetWeatherDamage(short damage, short attribute);
	void SendType4BuffRemove(short tid, BYTE buff);
	void Type3Cancel(_MAGIC_TABLE *pSkill);
	void Type4Cancel(_MAGIC_TABLE *pSkill);
	void Type6Cancel();

	BOOL UserRegionCheck(int sid, int tid, int magicid, int radius, short mousex = 0, short mousez = 0);
	short GetMagicDamage(Unit *pTarget, int total_hit, int attribute);

	bool ExecuteType1(_MAGIC_TABLE *pSkill);	
	bool ExecuteType2(_MAGIC_TABLE *pSkill);
	bool ExecuteType3(_MAGIC_TABLE *pSkill);
	bool ExecuteType4(_MAGIC_TABLE *pSkill);
	bool ExecuteType5(_MAGIC_TABLE *pSkill);
	bool ExecuteType6(_MAGIC_TABLE *pSkill);
	bool ExecuteType7(_MAGIC_TABLE *pSkill);
	bool ExecuteType8(_MAGIC_TABLE *pSkill);
	bool ExecuteType9(_MAGIC_TABLE *pSkill);

	bool IsAvailable(_MAGIC_TABLE *pSkill);
	bool UserCanCast(_MAGIC_TABLE *pSkill);
	void SendSkillToAI(_MAGIC_TABLE *pSkill);
	void MagicPacket(Packet & pkt);

	bool ExecuteSkill(_MAGIC_TABLE *pSkill, uint8 bType);

	void SendTransformationList(_MAGIC_TABLE *pSkill);
	void SendSkillFailed();
	void SendSkill(int16 pSkillCaster = -1, int16 pSkillTarget = -1, 
					int8 opcode = -1, uint32 nSkillID = 0, 
					int16 sData1 = -999, int16 sData2 = -999, int16 sData3 = -999, int16 sData4 = -999, 
					int16 sData5 = -999, int16 sData6 = -999, int16 sData7 = -999, int16 sData8 = -999);

	CMagicProcess();
	virtual ~CMagicProcess();

	CUser*			m_pSrcUser;

	// Need to make sure this data's not going to change during skill handling
	// (i.e. during multiple concurrent packets)
	// This cannot happen with the existing system, but it's a potential later worry.
	uint8	m_opcode;
	uint32	m_nSkillID;
	int16	m_sCasterID, m_sTargetID; 
	Unit	*m_pSkillCaster, *m_pSkillTarget;
	uint16	m_sData1, m_sData2, m_sData3, m_sData4, 
			m_sData5, m_sData6, m_sData7, m_sData8;
};