#pragma once

#include "define.h"
#include "Unit.h"

class CEbenezerDlg;
class CNpc  : public Unit
{
public:
	virtual uint16 GetID() { return m_sNid; }
	virtual uint8 GetZoneID() { return m_bZoneID; }

	virtual float GetX() { return m_fCurX; }
	virtual float GetY() { return m_fCurY; }
	virtual float GetZ() { return m_fCurZ; }

	virtual const char * GetName() { return m_strName; }

	virtual uint8 GetLevel() { return m_byLevel; }

	uint8	m_bZoneID;

	uint16	m_sNid; // NPC ID
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

	BYTE    m_byObjectType;     // 보통은 0, object타입(성문, 레버)은 1
	int16	m_byDirection;

	short   m_byEvent;		    // This is for the quest. 

public:
	CNpc();


	virtual void Initialize();
	void MoveResult(float xpos, float ypos, float zpos, float speed);
	virtual void GetInOut(Packet & result, uint8 bType);
	void SendInOut(uint8 bType, float fx, float fz, float fy);
	void GetNpcInfo(Packet & pkt);

	void SendGateFlag(BYTE bFlag = -1, bool bSendAI = true);

	virtual void OnDeath(Unit *pKiller);

	virtual bool isDead() { return m_NpcState == NPC_DEAD || m_iHP <= 0; };

	__forceinline bool isGate() { return GetType() == NPC_GATE || GetType() == NPC_PHOENIX_GATE || GetType() == NPC_SPECIAL_GATE || GetType() == NPC_VICTORY_GATE; };
	__forceinline bool isGateOpen() { return m_byGateOpen == TRUE; };
	__forceinline bool isGateClosed() { return !isGateOpen(); };

	__forceinline short GetEntryID() { return m_sSid; };
	__forceinline BYTE GetType() { return m_tNpcType; };
	__forceinline BYTE GetNation() { return m_byGroup; }; // NOTE: Need some consistency with naming

	__forceinline BYTE GetState() { return m_byState; };

	virtual ~CNpc();
};