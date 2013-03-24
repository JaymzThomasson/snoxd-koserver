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

struct MagicInstance
{
	uint8	bOpcode;
	uint32	nSkillID;
	_MAGIC_TABLE * pSkill;
	int16	sCasterID, sTargetID; 
	Unit	*pSkillCaster, *pSkillTarget;
	uint16	sData1, sData2, sData3, sData4, 
			sData5, sData6, sData7, sData8;
	bool	bIsRecastingSavedMagic;
};

enum MagicDamageType
{
	FIRE_DAMAGE			= 5,
	ICE_DAMAGE			= 6,
	LIGHTNING_DAMAGE	= 7
};

class CMagicProcess  
{
public:
	short GetWeatherDamage(short damage, short attribute);
	void SendType4BuffRemove(short tid, BYTE buff);
	void Type3Cancel(MagicInstance * pInstance);
	void Type4Cancel(MagicInstance * pInstance);
	void Type6Cancel();
	void Type9Cancel(MagicInstance * pInstance);
	void Type4Extend(MagicInstance * pInstance);

	BOOL UserRegionCheck(int sid, int tid, int magicid, int radius, short mousex = 0, short mousez = 0);
	short GetMagicDamage(MagicInstance * pInstance, Unit *pTarget, int total_hit, int attribute);

	bool ExecuteType1(MagicInstance * pInstance);	
	bool ExecuteType2(MagicInstance * pInstance);
	bool ExecuteType3(MagicInstance * pInstance);
	bool ExecuteType4(MagicInstance * pInstance);
	bool ExecuteType5(MagicInstance * pInstance);
	bool ExecuteType6(MagicInstance * pInstance);
	bool ExecuteType7(MagicInstance * pInstance);
	bool ExecuteType8(MagicInstance * pInstance);
	bool ExecuteType9(MagicInstance * pInstance);

	bool IsAvailable(MagicInstance * pInstance);
	bool UserCanCast(MagicInstance * pInstance);
	void SendSkillToAI(MagicInstance * pInstance);
	void MagicPacket(Packet & pkt, bool isRecastingSavedMagic = false);
	void HandleMagic(MagicInstance * pInstance);
	void ReflectDamage(MagicInstance * pInstance, int32 damage);

	bool ExecuteSkill(MagicInstance * pInstance, uint8 bType);

	void SendTransformationList(MagicInstance * pInstance);
	void SendSkillFailed(MagicInstance * pInstance);
	void SendSkill(MagicInstance * pInstance, int16 pSkillCaster = -1, int16 pSkillTarget = -1, 
					int8 opcode = -1, uint32 nSkillID = 0, 
					int16 sData1 = -999, int16 sData2 = -999, int16 sData3 = -999, int16 sData4 = -999, 
					int16 sData5 = -999, int16 sData6 = -999, int16 sData7 = -999, int16 sData8 = -999);

	CMagicProcess();
	virtual ~CMagicProcess();

	CUser*			m_pSrcUser;
};