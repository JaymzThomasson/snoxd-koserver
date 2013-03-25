#pragma once

#include "LuaEngine.h"
#include "Unit.h"

class CNpc  : public Unit
{
public:
	virtual uint16 GetID() { return m_sNid; }
	virtual const char * GetName() { return m_strName; }

	uint16	m_sNid; // NPC ID
	uint16	m_sSid; // prototype ID

	short	m_sPid;				// MONSTER(NPC) Picture ID
	short	m_sSize;			// MONSTER(NPC) Size
	int		m_iWeapon_1;
	int		m_iWeapon_2;
	char	m_strName[MAX_NPC_SIZE+1];		// MONSTER(NPC) Name
	int		m_iMaxHP;			// 최대 HP
	int		m_iHP;				// 현재 HP
	BYTE	m_byState;			// 몬스터 (NPC) 상태
	BYTE	m_tNpcType;			// NPC Type
								// 0 : Normal Monster
								// 1 : NPC
								// 2 : 각 입구,출구 NPC
								// 3 : 경비병
	int   m_iSellingGroup;		// ItemGroup
//	DWORD	m_dwStepDelay;		

	BYTE	m_NpcState;			// NPC의 상태 - 살았다, 죽었다, 서있다 등등...
	BYTE	m_byGateOpen;		// Gate Npc Status -> 1 : open 0 : close

	BYTE    m_byObjectType;     // 보통은 0, object타입(성문, 레버)은 1
	int16	m_byDirection;

	uint8   m_byTrapNumber;

public:
	CNpc();


	virtual void Initialize();
	virtual void AddToRegion(int16 new_region_x, int16 new_region_z);

	void MoveResult(float xpos, float ypos, float zpos, float speed);
	virtual void GetInOut(Packet & result, uint8 bType);
	void SendInOut(uint8 bType, float fx, float fz, float fy);
	void GetNpcInfo(Packet & pkt);

	void SendGateFlag(BYTE bFlag = -1, bool bSendAI = true);

	virtual void HpChange(int amount, Unit *pAttacker = NULL, bool bSendToAI = true); 
	virtual void MSpChange(int amount);

	virtual void OnDeath(Unit *pKiller);

	virtual bool isDead() { return m_NpcState == NPC_DEAD || m_iHP <= 0; };

	__forceinline bool isGate() { return GetType() == NPC_GATE || GetType() == NPC_PHOENIX_GATE || GetType() == NPC_SPECIAL_GATE || GetType() == NPC_VICTORY_GATE; };
	__forceinline bool isGateOpen() { return m_byGateOpen == 1; };
	__forceinline bool isGateClosed() { return !isGateOpen(); };

	__forceinline short GetEntryID() { return m_sSid; };
	__forceinline BYTE GetType() { return m_tNpcType; };
	__forceinline BYTE GetState() { return m_byState; };

	virtual ~CNpc();

	DECLARE_LUA_CLASS(CNpc);

	DECLARE_LUA_GETTER(GetID);
	DECLARE_LUA_GETTER(GetEntryID);
	DECLARE_LUA_GETTER(GetName);
	DECLARE_LUA_GETTER(GetNation);
	DECLARE_LUA_GETTER(GetType);
	DECLARE_LUA_GETTER(GetZoneID);
	DECLARE_LUA_GETTER(GetX);
	DECLARE_LUA_GETTER(GetY);
	DECLARE_LUA_GETTER(GetZ);
};