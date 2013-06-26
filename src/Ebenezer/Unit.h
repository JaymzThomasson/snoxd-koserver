#pragma once

#if defined(AI_SERVER)
#	define KOMap MAP
#else
#	define KOMap C3DMap
#endif

#include "Define.h"
#include "../shared/ReferenceObject.h"
#include <map>

// Maximum range allowed between a player and an NPC.
#define MAX_NPC_RANGE		(121.0f) // pow(11.0f, 2.0f), to save having to calculate it in the code.

// Maximum range allowed between a unit and an object
#define MAX_OBJECT_RANGE	(100.0f) // pow(10.0f, 20.0f)

// Maximum range allowed between a player & their loot.
#define MAX_LOOT_RANGE		(121.0f) // pow(11.0f, 2.0f)

struct _MAGIC_TABLE;
struct _BUFF_TYPE4_INFO;
struct _BUFF_TYPE9_INFO;
class CRegion;
class KOMap;
class Packet;

typedef std::map<uint8, _BUFF_TYPE4_INFO> Type4BuffMap;
typedef std::map<uint8, _BUFF_TYPE9_INFO> Type9BuffMap;

/**
 * This class is a bridge between the CNpc & CUser classes
 **/
class Unit : public ReferenceObject
{
public:
	Unit(bool bPlayer = false);

	virtual void Initialize();

	INLINE bool isPlayer() { return m_bPlayer; }
	INLINE bool isNPC() { return !isPlayer(); }

	INLINE KOMap * GetMap() { return m_pMap; }

	virtual uint16 GetID() = 0;
	INLINE uint8 GetZoneID() { return m_bZone; }

	INLINE float GetX() { return m_curx; }
	INLINE float GetY() { return m_cury; }
	INLINE float GetZ() { return m_curz; }

	INLINE void SetPosition(float fx, float fy, float fz)
	{
		m_curx = fx;
		m_curz = fz;
		m_cury = fy;
	}

	INLINE uint16 GetSPosX() { return uint16(GetX() * 10); };
	INLINE uint16 GetSPosY() { return uint16(GetY() * 10); };
	INLINE uint16 GetSPosZ() { return uint16(GetZ() * 10); };

	INLINE uint16 GetRegionX() { return m_sRegionX; }
	INLINE uint16 GetRegionZ() { return m_sRegionZ; }

	INLINE uint16 GetNewRegionX() { return (uint16)(GetX()) / VIEW_DISTANCE; }
	INLINE uint16 GetNewRegionZ() { return (uint16)(GetZ()) / VIEW_DISTANCE; }

	INLINE CRegion * GetRegion() { return m_pRegion; }
	void SetRegion(uint16 x = -1, uint16 z = -1);

	virtual std::string & GetName() = 0; // this is especially fun...

	INLINE uint8 GetNation() { return m_bNation; }
	INLINE uint8 GetLevel() { return m_bLevel; }

	virtual int32 GetHealth() = 0;
	virtual int32 GetMaxHealth() = 0;
	virtual int32 GetMana() = 0;
	virtual int32 GetMaxMana() = 0;

	INLINE bool isIncapacitated() { return isDead() || isBlinded() || isBlinking(); }
	INLINE bool isTransformed() { return m_bIsTransformed; }
	INLINE bool isBlinded() { return m_bIsBlinded; }

	INLINE bool isBuffed()
	{
		FastGuard lock(m_buffLock);
		return !m_buffMap.empty();
	}

	INLINE bool canInstantCast() { return m_bInstantCast; }
	INLINE bool canStealth()	{ return m_bCanStealth; }

	virtual bool isBlinking() { return false; }

	virtual bool isDead() = 0;
	virtual bool isAlive() { return !isDead(); }

	float GetDistance(float fx, float fz);
	float GetDistance(Unit * pTarget);
	float GetDistanceSqrt(Unit * pTarget);

	bool isInRange(Unit * pTarget, float fSquaredRange);
	bool isInRange(float fx, float fz, float fSquaredRange);
	bool isInRangeSlow(Unit * pTarget, float fNonSquaredRange);
	bool isInRangeSlow(float fx, float fz, float fNonSquaredRange);

	static float GetDistance(float fStartX, float fStartZ, float fEndX, float fEndZ);
	static bool isInRange(float fStartX, float fStartZ, float fEndX, float fEndZ, float fSquaredRange);
	static bool isInRangeSlow(float fStartX, float fStartZ, float fEndX, float fEndZ, float fNonSquaredRange);

	virtual void GetInOut(Packet & result, uint8 bType) = 0;

	bool RegisterRegion();
	virtual void AddToRegion(int16 new_region_x, int16 new_region_z) = 0;
	void RemoveRegion(int16 del_x, int16 del_z);
	void InsertRegion(int16 insert_x, int16 insert_z);

	virtual short GetDamage(Unit *pTarget, _MAGIC_TABLE *pSkill = nullptr, bool bPreviewOnly = false) = 0;
	short GetMagicDamage(int damage, Unit *pTarget, bool bPreviewOnly = false);
	short GetACDamage(int damage, Unit *pTarget);
	uint8 GetHitRate(float rate);

	virtual void InsertSavedMagic(uint32 nSkillID, uint16 sDuration) {}
	virtual bool HasSavedMagic(uint32 nSkillID) { return false; }
	virtual int16 GetSavedMagicDuration(uint32 nSkillID) { return -1; }

	virtual void HpChange(int amount, Unit *pAttacker = nullptr, bool bSendToAI = true) = 0;
	virtual void MSpChange(int amount) = 0;

	void SendToRegion(Packet *result);
	void Send_AIServer(Packet *result);

	virtual void InitType3();
	virtual void InitType4();
	void AddType4Buff(uint8 bBuffType, _BUFF_TYPE4_INFO & pBuffInfo);

	virtual void StateChangeServerDirect(uint8 bType, uint32 nBuff) {}
	virtual bool CanAttack(Unit * pTarget);

	void OnDeath(Unit *pKiller);
	void SendDeathAnimation();

// public for the moment
// protected:
	KOMap  * m_pMap;
	CRegion * m_pRegion;

	uint8	m_bZone;
	float	m_curx, m_curz, m_cury;

	uint16	m_sRegionX, m_sRegionZ; // this is probably redundant

	bool	m_bPlayer;

	uint8	m_bLevel;
	uint8	m_bNation;

	short	m_sTotalHit;
	short	m_sTotalAc;
	float	m_fTotalHitrate;
	float	m_fTotalEvasionrate;

	short   m_sACAmount;
	uint8   m_bAttackAmount;
	short	m_sMagicAttackAmount;
	short	m_sMaxHPAmount, m_sMaxMPAmount;
	uint8	m_bHitRateAmount;
	short	m_sAvoidRateAmount;

	uint8	m_bAttackSpeedAmount;
	uint8   m_bSpeedAmount;

	uint16	m_sFireR;
	uint8	m_bFireRAmount;
	uint16	m_sColdR;
	uint8	m_bColdRAmount;
	uint16	m_sLightningR;
	uint8	m_bLightningRAmount;
	uint16	m_sMagicR;
	uint8	m_bMagicRAmount;
	uint16	m_sDiseaseR;
	uint8	m_bDiseaseRAmount;
	uint16	m_sPoisonR;
	uint8	m_bPoisonRAmount;	
	uint8	m_bMagicDamageReduction;

	uint8	m_bResistanceBonus;

	// bonus type -> amount
	typedef std::map<uint8, int16> ItemBonusMap;

	// slot id -> bonus map
	typedef std::map<uint8, ItemBonusMap> EquippedItemBonuses;

	// This map is for applying item bonuses from equipped skills, i.e. resistances, drains, damage reflection, etc.
	// It is indexed by slot ID (this should really work with the item container), and contains a map of each bonus (indexed by type)
	// supported by this item (we support multiple bonuses, official most likely still overrides them).
	EquippedItemBonuses m_equippedItemBonuses;
	FastMutex m_equippedItemBonusLock;

	short   m_sDaggerR;						// Resistance to Dagger
	short   m_sSwordR;						// Resistance to Sword
	short	m_sAxeR;						// Resistance to Axe
	short	m_sMaceR;						// Resistance to Mace
	short	m_sSpearR;						// Resistance to Spear
	short	m_sBowR;						// Resistance to Bow		

	struct MagicType3
	{
		bool	m_byUsed;		// indicates whether this element is used
		time_t	m_tHPLastTime;	// time when the durational skill last affected the unit
		int16	m_sHPAmount;	// HP amount to affet the unit by (negative for damage, positive for HP recovery)
		uint8	m_bHPInterval;	// interval (in seconds) between each durational skill effect
		uint8	m_bTickCount;	// 
		uint8	m_bTickLimit;	// number of ticks required before the skill expires
		uint16	m_sSourceID;	// ID of the unit that used this skill on the unit

		MagicType3() { Reset(); }

		INLINE void Reset()
		{
			m_byUsed = false;
			m_tHPLastTime = 0;
			m_sHPAmount = 0;
			m_bHPInterval = 0;
			m_bTickCount = 0;
			m_bTickLimit = 0;
			m_sSourceID = -1;
		}
	};

	MagicType3 m_durationalSkills[MAX_TYPE3_REPEAT];
	bool	m_bType3Flag;

	Type4BuffMap m_buffMap;
	Type9BuffMap m_type9BuffMap;
	FastMutex	m_buffLock;

	bool	m_bIsTransformed; // Is the unit in a transformed state?

	uint32	m_nTransformationItem; // item used for transforming (e.g. disguise scroll, totem..)
	time_t	m_tTransformationStartTime;
	uint16	m_sTransformationDuration;

	bool	m_bIsBlinded;
	bool	m_bCanStealth;
	bool	m_bInstantCast;

	uint8	m_bReflectArmorType;
};
