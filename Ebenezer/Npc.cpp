// Npc.cpp: implementation of the CNpc class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Npc.h"
#include "EbenezerDlg.h"
#include "Map.h"
#include "Define.h"
#include "AIPacket.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

extern CRITICAL_SECTION g_region_critical;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNpc::CNpc()
{

}

CNpc::~CNpc()
{

}

void CNpc::Initialize()
{
	m_pMain = (CEbenezerDlg*)AfxGetApp()->GetMainWnd();

	m_sNid = -1;				// NPC (서버상의)일련번호
	m_sSid = 0;
	m_pMap = NULL;
	m_bCurZone = -1;			// Current Zone number
	m_fCurX = 0;			// Current X Pos;
	m_fCurY = 0;			// Current Y Pos;
	m_fCurZ = 0;			// Current Z Pos;
	m_sPid = 0;				// MONSTER(NPC) Picture ID
	m_sSize = 100;				// MONSTER(NPC) Size
	memset(m_strName, 0, MAX_NPC_SIZE+1);		// MONSTER(NPC) Name
	m_iMaxHP = 0;				// 최대 HP
	m_iHP = 0;					// 현재 HP
	//m_byState = 0;			// 몬스터 (NPC) 상태이상
	m_tNpcType = 0;				// NPC Type
								// 0 : Normal Monster
								// 1 : NPC
								// 2 : 각 입구,출구 NPC
								// 3 : 경비병
	m_byGroup = 0;
	m_byLevel = 0;
	m_iSellingGroup = 0;
//	m_dwStepDelay = 0;		

	m_sRegion_X = 0;			// region x position
	m_sRegion_Z = 0;			// region z position
	m_byDirection = 0;			// npc의 방향,,
	m_iWeapon_1 = 0;
	m_iWeapon_2 = 0;
	m_NpcState = NPC_LIVE;
	m_byGateOpen = 1;
	m_sHitRate = 0;
	m_byObjectType = NORMAL_OBJECT;

	m_byEvent = -1;				//  This is for the event.
}

void CNpc::MoveResult(float xpos, float ypos, float zpos, float speed)
{
	Packet result(WIZ_NPC_MOVE);

	m_fCurX = xpos;
	m_fCurZ = zpos;
	m_fCurY = ypos;

	RegisterRegion();

	result << GetID() << GetSPosX() << GetSPosZ() << GetSPosY() << uint16(speed * 10); 
	m_pMain->Send_Region(&result, GetMap(), m_sRegion_X, m_sRegion_Z);
}

void CNpc::NpcInOut(BYTE Type, float fx, float fz, float fy)
{
	C3DMap *pMap = m_pMain->GetZoneByID(getZoneID());
	if (pMap == NULL)
		return;

	m_pMap = pMap;

	if (Type == NPC_OUT)
	{
		pMap->RegionNpcRemove(m_sRegion_X, m_sRegion_Z, GetID());
	}
	else	
	{
		pMap->RegionNpcAdd(m_sRegion_X, m_sRegion_Z, GetID());
		m_fCurX = fx;	m_fCurZ = fz;	m_fCurY = fy;
	}

	Packet result(WIZ_NPC_INOUT, Type);
	result << GetID();

	if (Type == NPC_OUT)
	{
		m_pMain->Send_Region(&result, GetMap(), m_sRegion_X, m_sRegion_Z);
		return;
	}

	GetNpcInfo(result);
	m_pMain->Send_Region(&result, GetMap(), m_sRegion_X, m_sRegion_Z);
}

void CNpc::GetNpcInfo(Packet & pkt)
{
	pkt << GetEntryID()
		<< (getNation() == 0 ? 1 : 2) // Monster = 1, NPC = 2 (need to use a better flag)
		<< m_sPid
		<< m_tNpcType
		<< m_iSellingGroup
		<< m_sSize
		<< m_iWeapon_1 << m_iWeapon_2
		<< getNation()
		<< m_byLevel
		<< GetSPosX() << GetSPosZ() << GetSPosY()
		<< uint32(isGateOpen())
		<< m_byObjectType
		<< uint16(0) << uint16(0) // unknown
		<< int16(m_byDirection);
}

void CNpc::RegisterRegion()
{
	int iRegX = 0, iRegZ = 0, old_region_x = 0, old_region_z = 0;
	iRegX = (int)(m_fCurX / VIEW_DISTANCE);
	iRegZ = (int)(m_fCurZ / VIEW_DISTANCE);

	if (m_sRegion_X != iRegX || m_sRegion_Z != iRegZ)
	{
		C3DMap* pMap = GetMap();
		if (pMap == NULL)
			return;
		
		old_region_x = m_sRegion_X;	old_region_z = m_sRegion_Z;
		pMap->RegionNpcRemove(m_sRegion_X, m_sRegion_Z, m_sNid);
		m_sRegion_X = iRegX;		m_sRegion_Z = iRegZ;
		pMap->RegionNpcAdd(m_sRegion_X, m_sRegion_Z, m_sNid);

		RemoveRegion( old_region_x - m_sRegion_X, old_region_z - m_sRegion_Z );	// delete npc 는 계산 방향이 진행방향의 반대...
		InsertRegion( m_sRegion_X - old_region_x, m_sRegion_Z - old_region_z );	// add npc 는 계산 방향이 진행방향...
	}
}

void CNpc::RemoveRegion(int del_x, int del_z)
{
	ASSERT(GetMap() != NULL);

	Packet result(WIZ_NPC_INOUT, uint8(NPC_OUT));
	result << GetID();

	if (del_x != 0)
	{
		m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X+del_x*2, m_sRegion_Z+del_z-1);
		m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X+del_x*2, m_sRegion_Z+del_z);
		m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X+del_x*2, m_sRegion_Z+del_z+1);
	}

	if (del_z != 0)
	{	
		m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X+del_x, m_sRegion_Z+del_z*2);
		if (del_x < 0) 
			m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X+del_x+1, m_sRegion_Z+del_z*2);
		else if (del_x > 0)
			m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X+del_x-1, m_sRegion_Z+del_z*2);
		else
		{
			m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X+del_x-1, m_sRegion_Z+del_z*2);
			m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X+del_x+1, m_sRegion_Z+del_z*2);
		}
	}
}

void CNpc::InsertRegion(int del_x, int del_z)
{
	ASSERT(GetMap() != NULL);

	Packet result(WIZ_NPC_INOUT, uint8(NPC_IN));
	result << GetID();
	GetNpcInfo(result);

	if (del_x != 0)
	{
		m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X + del_x, m_sRegion_Z - 1);
		m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X + del_x, m_sRegion_Z);
		m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X + del_x, m_sRegion_Z + 1);
	}

	if (del_z != 0)
	{
		m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X, m_sRegion_Z + del_z);
		
		if (del_x < 0)
			m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X + 1, m_sRegion_Z + del_z);
		else if (del_x > 0)
			m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X - 1, m_sRegion_Z + del_z);
		else 
		{
			m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X - 1, m_sRegion_Z + del_z);
			m_pMain->Send_UnitRegion(&result, GetMap(), m_sRegion_X + 1, m_sRegion_Z + del_z);
		}
	}
}

int CNpc::GetRegionNpcList(int region_x, int region_z, char *buff, int &t_count)
{
	if( m_pMain->m_bPointCheckFlag == FALSE)	return 0;	// 포인터 참조하면 안됨

	int buff_index = 0;
	C3DMap* pMap = GetMap();
	CNpc* pNpc = NULL;

	if (pMap == NULL
		|| region_x < 0 || region_z < 0 
		|| region_x > pMap->GetXRegionMax() 
		|| region_z > pMap->GetZRegionMax() )
		return 0;

	EnterCriticalSection( &g_region_critical );

	CRegion *pRegion = &pMap->m_ppRegion[region_x][region_z];
	foreach_stlmap (itr, pRegion->m_RegionNpcArray)
	{
		CNpc *pNpc = m_pMain->m_arNpcArray.GetData(*itr->second);
		if (pNpc == NULL)
			continue;
		SetShort(buff, pNpc->m_sNid, buff_index);
		t_count++;
	}

	LeaveCriticalSection( &g_region_critical );

	return buff_index;
}

void CNpc::SendGateFlag(BYTE bFlag /*= -1*/, bool bSendAI /*= true*/)
{
	Packet result(WIZ_OBJECT_EVENT, uint8(OBJECT_FLAG_LEVER));

	// If there's a flag to set, set it now.
	if (bFlag >= 0)
		m_byGateOpen = bFlag;

	// Tell everyone nearby our new status.
	result << uint8(1) << GetID() << m_byGateOpen;
	m_pMain->Send_Region(&result, GetMap(), m_sRegion_X, m_sRegion_Z);

	// Tell the AI server our new status
	if (bSendAI)
	{
		result.Initialize(AG_NPC_GATE_OPEN);
		result << GetID() << m_byGateOpen;
		m_pMain->Send_AIServer(&result);
	}
}

/* NOTE: This code onwards needs to be merged with user code */
void CNpc::SendToRegion(Packet *result)
{
	m_pMain->Send_Region(result, GetMap(), m_sRegion_X, m_sRegion_Z);
}

void CNpc::OnDeath()
{
	SendDeathAnimation();
}

void CNpc::SendDeathAnimation()
{
	Packet result(WIZ_DEAD);
	result << GetID();
	SendToRegion(&result);
}