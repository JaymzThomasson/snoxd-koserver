#include "stdafx.h"
#include "User.h"
#include "Map.h"
#include "EbenezerDlg.h"

CNpc::CNpc()
{
	Initialize();
}


CNpc::~CNpc()
{
}

/**
 * @brief	Initializes this object.
 */
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

/**
 * @brief	Adds the NPC to the region.
 *
 * @param	new_region_x	The new region x coordinate.
 * @param	new_region_z	The new region z coordinate.
 */
void CNpc::AddToRegion(int16 new_region_x, int16 new_region_z)
{
	GetRegion()->Remove(this);
	SetRegion(new_region_x, new_region_z);
	GetRegion()->Add(this);
}

/**
 * @brief	Sends the movement packet for the NPC.
 *
 * @param	fPosX 	The position x coordinate.
 * @param	fPosY 	The position y coordinate.
 * @param	fPosZ 	The position z coordinate.
 * @param	fSpeed	The speed.
 */
void CNpc::MoveResult(float fPosX, float fPosY, float fPosZ, float fSpeed)
{
	Packet result(WIZ_NPC_MOVE);

	SetPosition(fPosX, fPosY, fPosZ);
	RegisterRegion();

	result << GetID() << GetSPosX() << GetSPosZ() << GetSPosY() << uint16(fSpeed * 10);
	SendToRegion(&result);
}

/**
 * @brief	Constructs an in/out packet for the NPC.
 *
 * @param	result	The packet buffer the constructed packet will be stored in.
 * @param	bType 	The type (in or out).
 */
void CNpc::GetInOut(Packet & result, uint8 bType)
{
	result.Initialize(WIZ_NPC_INOUT);
	result << bType << GetID();
	if (bType != INOUT_OUT)
		GetNpcInfo(result);
}

/**
 * @brief	Constructs and sends an in/out packet for the NPC.
 *
 * @param	bType	The type (in or out).
 * @param	fX   	The x coordinate.
 * @param	fZ   	The z coordinate.
 * @param	fY   	The y coordinate.
 */
void CNpc::SendInOut(uint8 bType, float fX, float fZ, float fY)
{
	if (GetRegion() == nullptr)
	{
		SetRegion(GetNewRegionX(), GetNewRegionZ());
		if (GetRegion() == nullptr)
			return;
	}

	if (bType == INOUT_OUT)
	{
		GetRegion()->Remove(this);
	}
	else	
	{
		GetRegion()->Add(this);
		SetPosition(fX, fY, fZ);
	}

	Packet result;
	GetInOut(result, bType);
	SendToRegion(&result);
}

/**
 * @brief	Gets NPC information for use in various NPC packets.
 *
 * @param	pkt	The packet the information will be stored in.
 */
void CNpc::GetNpcInfo(Packet & pkt)
{
	pkt << GetEntryID()
		<< uint8(isMonster() ? 1 : 2) // Monster = 1, NPC = 2 (need to use a better flag)
		<< m_sPid
		<< m_tNpcType
		<< m_iSellingGroup
		<< m_sSize
		<< m_iWeapon_1 << m_iWeapon_2
		// Monsters require 0 regardless, otherwise they'll act as NPCs.
		<< uint8(isMonster() ? 0 : GetNation())
		<< GetLevel()
		<< GetSPosX() << GetSPosZ() << GetSPosY()
		<< uint32(isGateOpen())
		<< m_byObjectType
		<< uint16(0) << uint16(0) // unknown
		<< int16(m_byDirection);
}

/**
 * @brief	Sends a gate status.
 *
 * @param	bFlag  	The flag (open or shut).
 * @param	bSendAI	true to update the AI server.
 */
void CNpc::SendGateFlag(uint8 bFlag /*= -1*/, bool bSendAI /*= true*/)
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
		Send_AIServer(&result);
	}
}

/**
 * @brief	Changes an NPC's hitpoints.
 *
 * @param	amount   	The amount to adjust the HP by.
 * @param	pAttacker	The attacker.
 * @param	bSendToAI	true to update the AI server.
 */
void CNpc::HpChange(int amount, Unit *pAttacker /*= nullptr*/, bool bSendToAI /*= true*/) 
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
		Packet result(AG_NPC_SET_HP);
		result << GetID() << m_iHP << pAttacker->GetID();
		Send_AIServer(&result);
	}

	if (pAttacker != nullptr
		&& pAttacker->isPlayer())
		TO_USER(pAttacker)->SendTargetHP(0, GetID(), amount);
}

/**
 * @brief	Changes an NPC's mana.
 *
 * @param	amount	The amount to adjust the mana by.
 */
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
	Send_AIServer(&result);
#endif
}

/**
 * @brief	Determine if an NPC can attack the specified unit.
 *
 * @param	pTarget	The target we are attempting to attack.
 *
 * @return	true if we can attack, false if not.
 */
bool CNpc::CanAttack(Unit * pTarget)
{
	if (!Unit::CanAttack(pTarget))
		return false;
	
	// A nation of 0 indicates friendliness to all
	if (GetNation() == Nation::ALL)
		return false;

	// A nation of 3 indicates hostility to all (or friendliness to none)
	if (GetNation() == Nation::NONE)
		return true;

	// An NPC cannot attack a unit of the same nation
	return (GetNation() != pTarget->GetNation());
}

/**
 * @brief	Executes the death action.
 *
 * @param	pKiller	The killer.
 */
void CNpc::OnDeath(Unit *pKiller)
{
	if (m_NpcState == NPC_DEAD)
		return;

	ASSERT(GetMap() != nullptr);
	ASSERT(GetRegion() != nullptr);

	m_NpcState = NPC_DEAD;

	if (m_byObjectType == SPECIAL_OBJECT)
	{
		_OBJECT_EVENT *pEvent = GetMap()->GetObjectEvent(GetEntryID());
		if (pEvent != nullptr)
			pEvent->byLife = 0;
	}

	Unit::OnDeath(pKiller);

	GetRegion()->Remove(TO_NPC(this));
	SetRegion();
}