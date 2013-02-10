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

class CNpc  
{
public:
	CEbenezerDlg* m_pMain;

	short	m_sNid;				// NPC (서버상의)일련번호
	short	m_sSid;				// NPC 테이블 참조번호
	BYTE	m_bCurZone;			// Current Zone;

	C3DMap * m_pMap;

	float	m_fCurX;			// Current X Pos;
	float	m_fCurY;			// Current Y Pos;
	float	m_fCurZ;			// Current Z Pos;
	short	m_sPid;				// MONSTER(NPC) Picture ID
	short	m_sSize;			// MONSTER(NPC) Size
	int		m_iWeapon_1;
	int		m_iWeapon_2;
	TCHAR	m_strName[MAX_NPC_SIZE+1];		// MONSTER(NPC) Name
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

	short m_sRegion_X;			// region x position
	short m_sRegion_Z;			// region z position
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
	int GetRegionNpcList(int region_x, int region_z, char *buff, int &t_count);

	void SendGateFlag(BYTE bFlag = -1, bool bSendAI = true);
	void SendToRegion(Packet *result);

	void OnDeath();
	void SendDeathAnimation();

	__forceinline bool isDead() { return m_NpcState == NPC_DEAD || m_iHP <= 0; };
	__forceinline bool isAlive() { return !isDead(); };

	__forceinline bool isGate() { return GetType() == NPC_GATE || GetType() == NPC_PHOENIX_GATE || GetType() == NPC_SPECIAL_GATE; };
	__forceinline bool isGateOpen() { return m_byGateOpen == TRUE; };
	__forceinline bool isGateClosed() { return !isGateOpen(); };

	__forceinline short GetID() { return m_sNid; };
	__forceinline short GetEntryID() { return m_sSid; };
	__forceinline BYTE GetType() { return m_tNpcType; };
	__forceinline BYTE getNation() { return m_byGroup; }; // NOTE: Need some consistency with naming
	__forceinline BYTE getZoneID() { return m_bCurZone; };

	__forceinline BYTE GetState() { return m_byState; };
	__forceinline C3DMap * GetMap() { return m_pMap; };

	__forceinline uint16 GetSPosX() { return uint16(m_fCurX * 10); };
	__forceinline uint16 GetSPosY() { return uint16(m_fCurY * 10); };
	__forceinline uint16 GetSPosZ() { return uint16(m_fCurZ * 10); };

	virtual ~CNpc();
};

#endif // !defined(AFX_NPC_H__1DE71CDD_4040_4479_828D_E8EE07BD7A67__INCLUDED_)
