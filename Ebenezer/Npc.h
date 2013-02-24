// Npc.h: interface for the CNpc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NPC_H__1DE71CDD_4040_4479_828D_E8EE07BD7A67__INCLUDED_)
#define AFX_NPC_H__1DE71CDD_4040_4479_828D_E8EE07BD7A67__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "define.h"

class CEbenezerDlg;
class C3DMap;

/**
 * This class is a bridge between the CNpc & CUser classes
 * Currently it's excessively messier than it needs to be, 
 * because of Aujard and the whole _USER_DATA setup.
 *
 * This will be written out eventually, so we can do this properly.
 **/
class Unit
{
public:
	Unit() : m_id(-1), m_pMap(NULL), m_pRegion(NULL), m_sRegionX(0), m_sRegionZ(0) {}

	__forceinline uint16 GetID() { return m_id; }
	__forceinline void SetID(uint16 id) { m_id = id; }

	__forceinline C3DMap * GetMap() { return m_pMap; }

	virtual uint8 GetZoneID() = 0;

	virtual float GetX() = 0;
	virtual float GetY() = 0;
	virtual float GetZ() = 0;

	__forceinline uint16 GetSPosX() { return uint16(GetX() * 10); };
	__forceinline uint16 GetSPosY() { return uint16(GetY() * 10); };
	__forceinline uint16 GetSPosZ() { return uint16(GetZ() * 10); };

	__forceinline uint16 GetRegionX() { return m_sRegionX; }
	__forceinline uint16 GetRegionZ() { return m_sRegionZ; }

	__forceinline uint16 GetNewRegionX() { return (uint16)(GetX()) / VIEW_DISTANCE; }
	__forceinline uint16 GetNewRegionZ() { return (uint16)(GetY()) / VIEW_DISTANCE; }

	__forceinline CRegion * GetRegion() { return m_pRegion; }
	__forceinline void SetRegion(uint16 x, uint16 z) 
	{
		m_sRegionX = x; m_sRegionZ = z; 
		m_pRegion = m_pMap->GetRegion(x, z);
	}

	virtual const char * GetName() = 0; // this is especially fun...

	virtual uint8 GetNation() = 0;
	virtual uint8 GetLevel() = 0;

// public for the moment
// protected:
	uint16 m_id;
	C3DMap * m_pMap;
	CRegion * m_pRegion;

	uint16 m_sRegionX, m_sRegionZ; // this is probably redundant
};

class CNpc  : public Unit
{
public:
	virtual uint8 GetZoneID() { return m_bZoneID; }

	virtual float GetX() { return m_fCurX; }
	virtual float GetY() { return m_fCurY; }
	virtual float GetZ() { return m_fCurZ; }

	virtual const char * GetName() { return m_strName; }

	virtual uint8 GetLevel() { return m_byLevel; }

	uint8	m_bZoneID;

	uint16	m_sSid; // prototype ID

	float	m_fCurX, m_fCurY, m_fCurZ;
	short	m_sPid;				// MONSTER(NPC) Picture ID
	short	m_sSize;			// MONSTER(NPC) Size
	int		m_iWeapon_1;
	int		m_iWeapon_2;
	char	m_strName[MAX_NPC_SIZE+1];		// MONSTER(NPC) Name
	int		m_iMaxHP;			// 최대 HP
	int		m_iHP;				// 현재 HP
	BYTE	m_byState;			// 몬스터 (NPC) 상태
	BYTE	m_byGroup;			// 소속 집단
	BYTE	m_byLevel;			// 레벨
	BYTE	m_tNpcType;			// NPC Type
								// 0 : Normal Monster
								// 1 : NPC
								// 2 : 각 입구,출구 NPC
								// 3 : 경비병
	int   m_iSellingGroup;		// ItemGroup
//	DWORD	m_dwStepDelay;		

	BYTE	m_NpcState;			// NPC의 상태 - 살았다, 죽었다, 서있다 등등...
	BYTE	m_byGateOpen;		// Gate Npc Status -> 1 : open 0 : close
	short   m_sHitRate;			// 공격 성공률
	BYTE    m_byObjectType;     // 보통은 0, object타입(성문, 레버)은 1
	int16	m_byDirection;

	short   m_byEvent;		    // This is for the quest. 

public:
	CNpc();


	void Initialize();
	void MoveResult(float xpos, float ypos, float zpos, float speed);
	void NpcInOut(BYTE Type, float fx, float fz, float fy);
	void GetNpcInfo(Packet & pkt);
	void RegisterRegion();
	void RemoveRegion(int del_x, int del_z);
	void InsertRegion(int del_x, int del_z);

	void SendGateFlag(BYTE bFlag = -1, bool bSendAI = true);
	void SendToRegion(Packet *result);

	void OnDeath();
	void SendDeathAnimation();

	__forceinline bool isDead() { return m_NpcState == NPC_DEAD || m_iHP <= 0; };
	__forceinline bool isAlive() { return !isDead(); };

	__forceinline bool isGate() { return GetType() == NPC_GATE || GetType() == NPC_PHOENIX_GATE || GetType() == NPC_SPECIAL_GATE || GetType() == NPC_VICTORY_GATE; };
	__forceinline bool isGateOpen() { return m_byGateOpen == TRUE; };
	__forceinline bool isGateClosed() { return !isGateOpen(); };

	__forceinline short GetEntryID() { return m_sSid; };
	__forceinline BYTE GetType() { return m_tNpcType; };
	__forceinline BYTE GetNation() { return m_byGroup; }; // NOTE: Need some consistency with naming

	__forceinline BYTE GetState() { return m_byState; };
	__forceinline C3DMap * GetMap() { return m_pMap; };

	virtual ~CNpc();
};

#endif // !defined(AFX_NPC_H__1DE71CDD_4040_4479_828D_E8EE07BD7A67__INCLUDED_)
