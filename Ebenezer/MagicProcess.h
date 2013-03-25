#pragma once

enum ResistanceTypes
{
	NONE_R		= 0,
	FIRE_R		= 1,
	COLD_R		= 2,
	LIGHTNING_R	= 3,
	MAGIC_R		= 4,
	DISEASE_R	= 5,
	POISON_R	= 6,
	LIGHT_R		= 7,
	DARKNESS_R	= 8
};

class Packet;
class Unit;
struct _MAGIC_TABLE;

enum MagicDamageType
{
	FIRE_DAMAGE			= 5,
	ICE_DAMAGE			= 6,
	LIGHTNING_DAMAGE	= 7
};

class CMagicProcess  
{
public:
	static void SendType4BuffRemove(short tid, BYTE buff);
	static bool UserRegionCheck(Unit * pSkillCaster, Unit * pSkillTarget, _MAGIC_TABLE * pSkill, int radius, short mousex = 0, short mousez = 0);

	void MagicPacket(Packet & pkt, bool isRecastingSavedMagic = false);
	void CheckExpiredType6Skills();

	CMagicProcess();
	virtual ~CMagicProcess();

	CUser*			m_pSrcUser;
};

class MagicInstance
{
public:
	uint8	bOpcode;
	uint32	nSkillID;
	_MAGIC_TABLE * pSkill;
	int16	sCasterID, sTargetID; 
	Unit	*pSkillCaster, *pSkillTarget;
	uint16	sData1, sData2, sData3, sData4, 
			sData5, sData6, sData7, sData8;
	bool	bIsRecastingSavedMagic;

	void Run();

	bool IsAvailable();
	bool UserCanCast();

	bool ExecuteSkill(uint8 bType);
	bool ExecuteType1();	
	bool ExecuteType2();
	bool ExecuteType3();
	bool ExecuteType4();
	bool ExecuteType5();
	bool ExecuteType6();
	bool ExecuteType7();
	bool ExecuteType8();
	bool ExecuteType9();

	void Type3Cancel();
	void Type4Cancel();
	void Type6Cancel();
	void Type9Cancel();
	void Type4Extend();

	short GetMagicDamage(Unit *pTarget, int total_hit, int attribute);
	short GetWeatherDamage(short damage, int attribute);
	void ReflectDamage(int32 damage);

	void SendSkillToAI();
	void SendSkill(int16 pSkillCaster = -1, int16 pSkillTarget = -1, 
					int8 opcode = -1, uint32 nSkillID = 0, 
					int16 sData1 = -999, int16 sData2 = -999, int16 sData3 = -999, int16 sData4 = -999, 
					int16 sData5 = -999, int16 sData6 = -999, int16 sData7 = -999, int16 sData8 = -999);
	void SendSkillFailed();
	void SendTransformationList();
};