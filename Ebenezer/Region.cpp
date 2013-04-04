#include "StdAfx.h"
#include "Region.h"
#include "User.h"
#include "Npc.h"

// this should be replaced
extern CRITICAL_SECTION g_region_critical;

/**
 * @brief	Adds user instance to the region.
 *
 * @param	pUser	The user to add.
 */
void CRegion::Add(CUser * pUser)
{
	EnterCriticalSection(&g_region_critical);
	m_RegionUserArray.insert(pUser->GetID());
	LeaveCriticalSection(&g_region_critical);
}

/**
 * @brief	Removes the given user instance from the region.
 *
 * @param	pUser	The user to remove.
 */
void CRegion::Remove(CUser * pUser)
{
	EnterCriticalSection(&g_region_critical);
	m_RegionUserArray.erase(pUser->GetID());
	LeaveCriticalSection(&g_region_critical);
}

/**
 * @brief	Adds the given NPC to the region.
 *
 * @param	pNpc	The NPC to add.
 */
void CRegion::Add(CNpc * pNpc)
{
	EnterCriticalSection(&g_region_critical);
	m_RegionNpcArray.insert(pNpc->GetID());
	LeaveCriticalSection(&g_region_critical);
}

/**
 * @brief	Removes the given NPC from the region.
 *
 * @param	pNpc	The NPC to remove.
 */
void CRegion::Remove(CNpc * pNpc)
{
	EnterCriticalSection(&g_region_critical);
	m_RegionNpcArray.erase(pNpc->GetID());
	LeaveCriticalSection(&g_region_critical);
}