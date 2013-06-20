#include "stdafx.h"
#include "Map.h"
#include "EbenezerDlg.h"
#include "User.h"
#include <set>
#include "../shared/SMDFile.h"

/* passthru methods */
int C3DMap::GetXRegionMax() { return m_smdFile->GetXRegionMax(); }
int C3DMap::GetZRegionMax() { return m_smdFile->GetZRegionMax(); }
bool C3DMap::IsValidPosition(float x, float z, float y) { return m_smdFile->IsValidPosition(x, z, y); }
_OBJECT_EVENT * C3DMap::GetObjectEvent(int objectindex) { return m_smdFile->GetObjectEvent(objectindex); }
_REGENE_EVENT * C3DMap::GetRegeneEvent(int objectindex) { return m_smdFile->GetRegeneEvent(objectindex); }
_WARP_INFO * C3DMap::GetWarp(int warpID) { return m_smdFile->GetWarp(warpID); }
void C3DMap::GetWarpList(int warpGroup, std::set<_WARP_INFO *> & warpEntries) { m_smdFile->GetWarpList(warpGroup, warpEntries); }

C3DMap::C3DMap() : m_smdFile(nullptr), m_ppRegion(nullptr),
	m_nZoneNumber(0), m_sMaxUser(150), m_wBundle(1),
	m_bType(0), m_isAttackZone(false)
{
}

bool C3DMap::Initialize(_ZONE_INFO *pZone)
{
	m_nServerNo = pZone->m_nServerNo;
	m_nZoneNumber = pZone->m_nZoneNumber;
	m_fInitX = pZone->m_fInitX;
	m_fInitY = pZone->m_fInitY;
	m_fInitZ = pZone->m_fInitZ;
	m_bType = pZone->m_bType;
	m_isAttackZone = pZone->isAttackZone == 1;

	m_smdFile = SMDFile::Load(pZone->m_MapName, true /* load warps & regene events */);

	if (m_smdFile != nullptr)
	{
		m_ppRegion = new CRegion*[m_smdFile->m_nXRegion];
		for (int i = 0; i < m_smdFile->m_nXRegion; i++)
			m_ppRegion[i] = new CRegion[m_smdFile->m_nZRegion];
	}

	return (m_smdFile != nullptr);
}

CRegion * C3DMap::GetRegion(uint16 regionX, uint16 regionZ)
{
	if (regionX > GetXRegionMax()
		|| regionZ > GetZRegionMax())
		return nullptr;

	FastGuard lock(m_lock);
	return &m_ppRegion[regionX][regionZ];
}


bool C3DMap::RegionItemAdd(uint16 rx, uint16 rz, _LOOT_BUNDLE * pBundle)
{
	if (rx > GetXRegionMax() || rz > GetZRegionMax()
		|| pBundle == nullptr)
		return false;

	FastGuard lock(m_lock);

	pBundle->nBundleID = m_wBundle++;
	m_ppRegion[rx][rz].m_RegionItemArray.PutData(pBundle->nBundleID, pBundle);
	if (m_wBundle > ZONEITEM_MAX)
		m_wBundle = 1;

	return true;
}

/**
 * @brief	Removes an item from a region's bundle.
 * 			If the bundle's empty, the bundle is then 
 * 			removed from the region.
 *
 * @param	pRegion	The region.
 * @param	pBundle	The bundle.
 * @param	pItem  	The item being removed from the bundle.
 */
void C3DMap::RegionItemRemove(CRegion * pRegion, _LOOT_BUNDLE * pBundle, _LOOT_ITEM * pItem)
{
	if (pBundle == nullptr)
		return;

	// If the bundle exists, and the item matches what the user's removing
	// we can remove this item from the bundle.
	foreach (itr, pBundle->Items)
	{
		if (&(*itr) == pItem)
		{
			pBundle->Items.erase(itr);

			// If this was the last item in the bundle, remove the bundle from the region.
			if (!pBundle->Items.empty())
				return;

			pRegion->m_RegionItemArray.DeleteData(pBundle->nBundleID);
			return;
		}
	}
}

bool C3DMap::CheckEvent(float x, float z, CUser* pUser)
{
	int event_index = m_smdFile->GetEventID((int)(x / m_smdFile->GetUnitDistance()), (int)(z / m_smdFile->GetUnitDistance()));
	if( event_index < 2 )
		return false;

	CGameEvent *pEvent = m_EventArray.GetData( event_index );
	if (pEvent == nullptr)
		return false;

	if( pEvent->m_bType == 1 && pEvent->m_iExec[0]==ZONE_BATTLE && g_pMain->m_byBattleOpen != NATION_BATTLE ) return false;
	if( pEvent->m_bType == 1 && pEvent->m_iExec[0]==ZONE_SNOW_BATTLE && g_pMain->m_byBattleOpen != SNOW_BATTLE ) return false;
	if( pUser->m_bNation == KARUS && pEvent->m_iExec[0] == ZONE_BATTLE )	{
		if( g_pMain->m_sKarusCount > MAX_BATTLE_ZONE_USERS )	{
			TRACE("### BattleZone karus full users = %d, name=%s \n", g_pMain->m_sKarusCount, pUser->GetName().c_str());
			return false;
		}
	}
	else if( pUser->m_bNation == ELMORAD && pEvent->m_iExec[0] == ZONE_BATTLE )	{
		if( g_pMain->m_sElmoradCount > MAX_BATTLE_ZONE_USERS )	{
			TRACE("### BattleZone elmorad full users = %d, name=%s \n", g_pMain->m_sElmoradCount, pUser->GetName().c_str());
			return false;
		}
	}
	pEvent->RunEvent( pUser );
	return true;
}

C3DMap::~C3DMap()
{
	m_EventArray.DeleteAllData();

	if (m_ppRegion != nullptr)
	{
		for (int i = 0; i < GetXRegionMax(); i++)
			delete [] m_ppRegion[i];

		delete [] m_ppRegion;
		m_ppRegion = nullptr;
	}

	if (m_smdFile != nullptr)
		m_smdFile->DecRef();
}