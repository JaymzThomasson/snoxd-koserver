#pragma once

#include "Define.h"
#include "../shared/ReferenceObject.h"

// Maximum range allowed between a player and an NPC.
#define MAX_NPC_RANGE		(121.0f) // pow(11.0f, 2.0f), to save having to calculate it in the code.

// Maximum range allowed between a unit and an object
#define MAX_OBJECT_RANGE	(100.0f) // pow(10.0f, 20.0f)

// Maximum range allowed between a player & their loot.
#define MAX_LOOT_RANGE		(121.0f) // pow(11.0f, 2.0f)

/**
 * This class is a bridge between the CNpc & CUser classes
 * Currently it's excessively messier than it needs to be, 
 * because of Aujard and the whole _USER_DATA setup 
 * (which has recently been removed)
 *
 * This will be written out eventually, so we can do this properly.
 **/
struct _MAGIC_TABLE;
class CRegion;
class C3DMap;
class Packet;
class Unit : public ReferenceObject
{
public:
	Unit(bool bPlayer = false);

	virtual void Initialize();

	__forceinline bool isPlayer() { return m_bPlayer; }
	__forceinline bool isNPC() { return !isPlayer(); }

	__forceinline C3DMap * GetMap() { return m_pMap; }

	virtual uint16 GetID() = 0;
	__forceinline uint8 GetZoneID() { return m_bZone; }

	__forceinline float GetX() { return m_curx; }
	__forceinline float GetY() { return m_cury; }
	__forceinline float GetZ() { return m_curz; }

	__forceinline void SetPosition(float fx, float fy, float fz)
	{
		m_curx = fx;
		m_curz = fz;
		m_cury = fy;
	}

	__forceinline uint16 GetSPosX() { return uint16(GetX() * 10); };
	__forceinline uint16 GetSPosY() { return uint16(GetY() * 10); };
	__forceinline uint16 GetSPosZ() { return uint16(GetZ() * 10); };

	__forceinline uint16 GetRegionX() { return m_sRegionX; }
	__forceinline uint16 GetRegionZ() { return m_sRegionZ; }

	__forceinline uint16 GetNewRegionX() { return (uint16)(GetX()) / VIEW_DISTANCE; }
	__forceinline uint16 GetNewRegionZ() { return (uint16)(GetZ()) / VIEW_DISTANCE; }

	__forceinline CRegion * GetRegion() { return m_pRegion; }
	void SetRegion(uint16 x = -1, uint16 z = -1);

	virtual const char * GetName() = 0; // this is especially fun...

	__forceinline uint8 GetNation() { return m_bNation; }
	__forceinline uint8 GetLevel() { return m_bLevel; }

	virtual int32 GetHealth() = 0;
	virtual int32 GetMaxHealth() = 0;
	virtual int32 GetMana() = 0;
	virtual int32 GetMaxMana() = 0;

	__forceinline bool isTransformed() { return m_bIsTransformed; }
	__forceinline bool isBlinded() { return m_bIsBlinded; }
	__forceinline bool canInstantCast() { return m_bInstantCast; }
	__forceinline bool canStealth()	{ return m_bCanStealth; }

	virtual bool isBlinking() { return false; }

	virtual bool isDead() = 0;
	virtual bool isAlive() { return !isDead(); }

	float GetDistance(float fx, float fz);
	float GetDistance(Unit * pTarget);
	bool isInRange(Unit * pTarget, float fSquaredRange);
	bool isInRange(float fx, float fz, float fSquaredRange);
	bool isInRangeSlow(Unit * pTarget, float fNonSquaredRange);
	bool isInRangeSlow(float fx, float fz, float fNonSquaredRange);

	virtual void GetInOut(Packet & result, uint8 bType) = 0;

	bool RegisterRegion();
	virtual void AddToRegion(int16 new_region_x, int16 new_region_z) = 0;
	void RemoveRegion(int16 del_x, int16 del_z);
	void InsertRegion(int16 insert_x, int16 insert_z);

	short GetDamage(Unit *pTarget, _MAGIC_TABLE *pSkill);
	short GetMagicDamage(int damage, Unit *pTarget);
	short GetACDamage(int damage, Unit *pTarget);
	uint8 GetHitRate(float rate);

	virtual void InsertSavedMagic(uint32 nSkillID, uint16 sDuration) {}
	virtual bool HasSavedMagic(uint32 nSkillID) { return false; }
	virtual int16 GetSavedMagicDuration(uint32 nSkillID) { return -1; }

	virtual void HpChange(int amount, Unit *pAttacker = NULL, bool bSendToAI = true) = 0;
	virtual void MSpChange(int amount) = 0;

	void SendToRegion(Packet *result);
	void Send_AIServer(Packet *result);

	void InitType3();
	void InitType4();

	virtual void StateChangeServerDirect(uint8 bType, uint32 nBuff) {}

	void OnDeath(Unit *pKiller);
	void SendDeathAnimation();

// public for the moment
// protected:
	C3DMap  * m_pMap;
	CRegion * m_pRegion;

	uint8	m_bZone;
	float	m_curx, m_curz, m_cury;

	uint16	m_sRegionX, m_sRegionZ; // this is probably redundant

	bool	m_bPlayer;

	uint8	m_bLevel;
	uint8	m_bNation;

	short	m_sTotalHit;
	short	m_sTotalAc;
	float	m_sTotalHitrate;
	float	m_sTotalEvasionrate;

	short   m_sACAmount;
	uint8    m_bAttackAmount;
	short	m_sMagicAttackAmount;
	short	m_sMaxHPAmount, m_sMaxMPAmount;
	uint8	m_bHitRateAmount;
	short	m_sAvoidRateAmount;

	uint8	m_bAttackSpeedAmount;
	uint8   m_bSpeedAmount;

	// these are actually player-specific, but I don't see much of a problem with them being here.
	uint8	m_bExpGainAmount;
	uint8	m_bMaxWeightAmount; 

	uint8	m_bFireR;
	uint8	m_bFireRAmount;
	uint8	m_bColdR;
	uint8	m_bColdRAmount;
	uint8	m_bLightningR;
	uint8	m_bLightningRAmount;
	uint8	m_bMagicR;
	uint8	m_bMagicRAmount;
	uint8	m_bDiseaseR;
	uint8	m_bDiseaseRAmount;
	uint8	m_bPoisonR;
	uint8	m_bPoisonRAmount;	

	uint8	m_bResistanceBonus;

	uint8   m_bMagicTypeLeftHand;			// The type of magic item in user's left hand  
	uint8   m_bMagicTypeRightHand;			// The type of magic item in user's right hand
	short   m_sMagicAmountLeftHand;         // The amount of magic item in user's left hand
	short	m_sMagicAmountRightHand;        // The amount of magic item in user's left hand

	short   m_sDaggerR;						// Resistance to Dagger
	short   m_sSwordR;						// Resistance to Sword
	short	m_sAxeR;						// Resistance to Axe
	short	m_sMaceR;						// Resistance to Mace
	short	m_sSpearR;						// Resistance to Spear
	short	m_sBowR;						// Resistance to Bow		

	time_t	m_tHPLastTime[MAX_TYPE3_REPEAT];		// For Automatic HP recovery and Type 3 durational HP recovery.
	time_t	m_tHPStartTime[MAX_TYPE3_REPEAT];
	int16	m_bHPAmount[MAX_TYPE3_REPEAT];
	uint8	m_bHPDuration[MAX_TYPE3_REPEAT];
	uint8	m_bHPInterval[MAX_TYPE3_REPEAT];
	short	m_sSourceID[MAX_TYPE3_REPEAT];
	bool	m_bType3Flag;

	short   m_sDuration[MAX_TYPE4_BUFF];
	time_t	m_tStartTime[MAX_TYPE4_BUFF];

	uint8	m_bType4Buff[MAX_TYPE4_BUFF];
	bool	m_bType4Flag;

	bool	m_bIsTransformed; // Is the unit in a transformed state?

	uint32	m_nTransformationItem; // item used for transforming (e.g. disguise scroll, totem..)
	time_t	m_tTransformationStartTime;
	uint16	m_sTransformationDuration;

	bool	m_bIsBlinded;
	bool	m_bCanStealth;
	bool	m_bInstantCast;

	uint8	m_bReflectArmorType;
};
