#include "stdafx.h"

extern CRITICAL_SECTION g_region_critical;

CNpc::CNpc()
{
	Initialize();
}

CNpc::~CNpc()
{

}

void CNpc::Initialize()
{
	Unit::Initialize();

	m_sSid = 0;
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
	m_iSellingGroup = 0;
//	m_dwStepDelay = 0;		

	m_byDirection = 0;			// npc의 방향,,
	m_iWeapon_1 = 0;
	m_iWeapon_2 = 0;
	m_NpcState = NPC_LIVE;
	m_byGateOpen = 1;
	m_byObjectType = NORMAL_OBJECT;

	m_byTrapNumber = 0;
}

void CNpc::MoveResult(float xpos, float ypos, float zpos, float speed)
{
	Packet result(WIZ_NPC_MOVE);

	SetPosition(xpos, ypos, zpos);
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
		SetPosition(fx, fy, fz);
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
		<< GetLevel()
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
		g_pMain.Send_AIServer(&result);
	}
}

void CNpc::HpChange(int amount, Unit *pAttacker /*= NULL*/, bool bSendToAI /*= true*/) 
{
	// Glorious copypasta.
	if (amount < 0 && -amount > m_iHP)
		m_iHP = 0;
	else if (amount >= 0 && m_iHP + amount > m_iMaxHP)
		m_iHP = m_iMaxHP;
	else
		m_iHP += amount;

	if (bSendToAI)
	{
		// NOTE: This will handle the death notification/looting.
		Packet result(AG_USER_SET_HP);
		result << GetID() << m_iHP << pAttacker->GetID();
		g_pMain.Send_AIServer(&result);
	}
}

void CNpc::MSpChange(int amount)
{
#if 0 // TO-DO: Implement this
	// Glorious copypasta.
	// TO-DO: Make this behave unsigned.
	m_iMP += amount;
	if (m_iMP < 0)
		m_iMP = 0;
	else if (m_iMP > m_iMaxMP)
		m_iMP = m_iMaxMP;

	Packet result(AG_USER_SET_MP);
	result << GetID() << m_iMP;
	g_pMain.Send_AIServer(&result);
#endif
}

void CNpc::OnDeath(Unit *pKiller)
{
	if (m_NpcState == NPC_DEAD)
		return;

	ASSERT(GetMap() != NULL);
	ASSERT(GetRegion() != NULL);

	m_NpcState = NPC_DEAD;

	if (m_byObjectType == SPECIAL_OBJECT)
	{
		_OBJECT_EVENT *pEvent = GetMap()->GetObjectEvent(GetEntryID());
		if (pEvent != NULL)
			pEvent->byLife = 0;
	}

	Unit::OnDeath(pKiller);

	GetRegion()->Remove(TO_NPC(this));
	SetRegion();
}
