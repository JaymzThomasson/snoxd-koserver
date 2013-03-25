#include "StdAfx.h"
#include "Region.h"
#include "User.h"
#include "Npc.h"

// this should be replaced
extern CRITICAL_SECTION g_region_critical;

void CRegion::Add(CUser * pUser)
{
	EnterCriticalSection(&g_region_critical);
	m_RegionUserArray.insert(pUser->GetID());
	LeaveCriticalSection(&g_region_critical);
}

void CRegion::Remove(CUser * pUser)
{
	EnterCriticalSection(&g_region_critical);
	m_RegionUserArray.erase(pUser->GetID());
	LeaveCriticalSection(&g_region_critical);
}

void CRegion::Add(CNpc * pNpc)
{
	EnterCriticalSection(&g_region_critical);
	m_RegionNpcArray.insert(pNpc->GetID());
	LeaveCriticalSection(&g_region_critical);
}

void CRegion::Remove(CNpc * pNpc)
{
	EnterCriticalSection(&g_region_critical);
	m_RegionNpcArray.erase(pNpc->GetID());
	LeaveCriticalSection(&g_region_critical);
}