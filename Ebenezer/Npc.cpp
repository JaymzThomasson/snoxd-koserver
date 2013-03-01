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
	Initialize();
}

CNpc::~CNpc()
{

}

void CNpc::Initialize()
{
	m_sSid = 0;
	m_pMap = NULL;
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

	m_byDirection = 0;			// npc의 방향,,
	m_iWeapon_1 = 0;
	m_iWeapon_2 = 0;
	m_NpcState = NPC_LIVE;
	m_byGateOpen = 1;
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
	SendToRegion(&result);
}

void CNpc::GetInOut(Packet & result, uint8 bType)
{
	result.Initialize(WIZ_NPC_INOUT);
	result << bType << GetID();
	if (bType != INOUT_OUT)
		GetNpcInfo(result);
}

void CNpc::SendInOut(uint8 bType, float fx, float fz, float fy)
{
	if (GetRegion() == NULL)
	{
		SetRegion(GetNewRegionX(), GetNewRegionZ());
		if (GetRegion() == NULL)
			return;
	}

	if (bType == INOUT_OUT)
	{
		GetRegion()->Remove(this);
	}
	else	
	{
		GetRegion()->Add(this);
		m_fCurX = fx;	m_fCurZ = fz;	m_fCurY = fy;
	}

	Packet result;
	GetInOut(result, bType);
	SendToRegion(&result);
}

void CNpc::GetNpcInfo(Packet & pkt)
{
	pkt << GetEntryID()
		<< uint8(GetNation() == 0 ? 1 : 2) // Monster = 1, NPC = 2 (need to use a better flag)
		<< m_sPid
		<< m_tNpcType
		<< m_iSellingGroup
		<< m_sSize
		<< m_iWeapon_1 << m_iWeapon_2
		<< GetNation()
		<< m_byLevel
		<< GetSPosX() << GetSPosZ() << GetSPosY()
		<< uint32(isGateOpen())
		<< m_byObjectType
		<< uint16(0) << uint16(0) // unknown
		<< int16(m_byDirection);
}

void CNpc::SendGateFlag(BYTE bFlag /*= -1*/, bool bSendAI /*= true*/)
{
	Packet result(WIZ_OBJECT_EVENT, uint8(OBJECT_FLAG_LEVER));

	// If there's a flag to set, set it now.
	if (bFlag >= 0)
		m_byGateOpen = bFlag;

	// Tell everyone nearby our new status.
	result << uint8(1) << GetID() << m_byGateOpen;
	SendToRegion(&result);

	// Tell the AI server our new status
	if (bSendAI)
	{
		result.Initialize(AG_NPC_GATE_OPEN);
		result << GetID() << m_byGateOpen;
		g_pMain->Send_AIServer(&result);
	}
}

void CNpc::OnDeath(Unit *pKiller)
{
	ASSERT(GetMap() != NULL && GetRegion() != NULL);

	m_NpcState = NPC_DEAD;

	if (m_byObjectType == SPECIAL_OBJECT)
	{
		_OBJECT_EVENT *pEvent = GetMap()->GetObjectEvent(GetEntryID());
		if (pEvent != NULL)
			pEvent->byLife = 0;
	}

	Unit::OnDeath(pKiller);

	GetRegion()->Remove(static_cast<CNpc *>(this));
	SetRegion();
}
