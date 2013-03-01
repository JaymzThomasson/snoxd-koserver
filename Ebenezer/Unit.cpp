#include "stdafx.h"

bool Unit::RegisterRegion()
{
	uint16 
		new_region_x = GetNewRegionX(), new_region_z = GetNewRegionZ(), 
		old_region_x = GetRegionX(),	old_region_z = GetRegionZ();

	if (GetRegion() == NULL
		|| (old_region_x == new_region_x && old_region_z == new_region_z))
		return false;

	// TO-DO: Fix this up
	if (isPlayer())
	{
		GetRegion()->Remove(static_cast<CUser *>(this));
		SetRegion(new_region_x, new_region_z);
		GetRegion()->Add(static_cast<CUser *>(this));
	}
	else
	{
		GetRegion()->Remove(static_cast<CNpc *>(this));
		SetRegion(new_region_x, new_region_z);
		GetRegion()->Add(static_cast<CNpc *>(this));
	}

	RemoveRegion(old_region_x - new_region_x, old_region_z - new_region_z);
	InsertRegion(new_region_x - old_region_x, new_region_z - old_region_z);	

	return true;
}

void Unit::RemoveRegion(int16 del_x, int16 del_z)
{
	ASSERT(GetMap() != NULL);

	Packet result;
	GetInOut(result, INOUT_OUT);
	g_pMain->Send_OldRegions(&result, del_x, del_z, GetMap(), GetRegionX(), GetRegionZ());
}

void Unit::InsertRegion(int16 insert_x, int16 insert_z)
{
	ASSERT(GetMap() != NULL);

	Packet result;
	GetInOut(result, INOUT_IN);
	g_pMain->Send_NewRegions(&result, insert_x, insert_z, GetMap(), GetRegionX(), GetRegionZ());
}

void Unit::SendToRegion(Packet *result)
{
	g_pMain->Send_Region(result, GetMap(), GetRegionX(), GetRegionZ());
}

void Unit::OnDeath(Unit *pKiller)
{
	SendDeathAnimation();
}

void Unit::SendDeathAnimation()
{
	Packet result(WIZ_DEAD);
	result << GetID();
	SendToRegion(&result);
}