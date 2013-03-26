#include "stdafx.h"
#include "Map.h"
#include "EbenezerDlg.h"
#include "User.h"
#include <set>
#include "../shared/SMDFile.h"

extern CRITICAL_SECTION g_region_critical;

/* passthru methods */
int C3DMap::GetXRegionMax() { return m_smdFile->GetXRegionMax(); }
int C3DMap::GetZRegionMax() { return m_smdFile->GetZRegionMax(); }
bool C3DMap::IsValidPosition(float x, float z, float y) { return m_smdFile->IsValidPosition(x, z, y); }
_OBJECT_EVENT * C3DMap::GetObjectEvent(int objectindex) { return m_smdFile->GetObjectEvent(objectindex); }
_REGENE_EVENT * C3DMap::GetRegeneEvent(int objectindex) { return m_smdFile->GetRegeneEvent(objectindex); }
_WARP_INFO * C3DMap::GetWarp(int warpID) { return m_smdFile->GetWarp(warpID); }
void C3DMap::GetWarpList(int warpGroup, std::set<_WARP_INFO *> & warpEntries) { m_smdFile->GetWarpList(warpGroup, warpEntries); }

C3DMap::C3DMap() : m_smdFile(NULL), m_ppRegion(NULL),
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

	m_smdFile = SMDFile::Load(pZone->m_MapName);

	if (m_smdFile != NULL)
	{
		m_ppRegion = new CRegion*[m_smdFile->m_nXRegion];
		for (int i = 0; i < m_smdFile->m_nXRegion; i++)
			m_ppRegion[i] = new CRegion[m_smdFile->m_nZRegion];

		m_smdFile->IncRef();
	}

	return (m_smdFile != NULL);
}

CRegion * C3DMap::GetRegion(uint16 regionX, uint16 regionZ)
{
	CRegion *pRegion = NULL;

	if (regionX >= GetXRegionMax()
		|| regionZ >= GetZRegionMax())
		return pRegion;

	EnterCriticalSection(&g_region_critical);
	pRegion = &m_ppRegion[regionX][regionZ];
	LeaveCriticalSection(&g_region_critical);

	return pRegion;
}


bool C3DMap::RegionItemAdd(uint16 rx, uint16 rz, _ZONE_ITEM * pItem)
{
	if (rx >= GetXRegionMax() || rz >= GetZRegionMax()
		|| pItem == NULL)
		return false;

	EnterCriticalSection( &g_region_critical );

	pItem->nBundleID = m_wBundle++;
	m_ppRegion[rx][rz].m_RegionItemArray.PutData(pItem->nBundleID, pItem);
	if (m_wBundle > ZONEITEM_MAX)
		m_wBundle = 1;

	LeaveCriticalSection( &g_region_critical );

	return true;
}

bool C3DMap::RegionItemRemove(uint16 rx, uint16 rz, int bundle_index, int itemid, int count)
{
	if (rx >= GetXRegionMax() || rz >= GetZRegionMax())
		return false;
	
	_ZONE_ITEM* pItem = NULL;
	CRegion	*region = NULL;
	bool bFind = false;
	short t_count = 0;
	
	EnterCriticalSection( &g_region_critical );
	
	region = &m_ppRegion[rx][rz];
	pItem = region->m_RegionItemArray.GetData( bundle_index );
	if( pItem ) {
		for(int j=0; j < LOOT_ITEMS; j++) {
			if( pItem->nItemID[j] == itemid && pItem->sCount[j] == count ) {
				pItem->nItemID[j] = 0; pItem->sCount[j] = 0;
				bFind = true;
				break;
			}
		}
		if( bFind ) {
			for( int j=0; j < LOOT_ITEMS; j++) {
				if( pItem->nItemID[j] != 0 )
					t_count++;
			}
			if( !t_count )
				region->m_RegionItemArray.DeleteData( bundle_index );
		}
	}

	LeaveCriticalSection( &g_region_critical );

	return bFind;
}

bool C3DMap::CheckEvent(float x, float z, CUser* pUser)
{
	int event_index = m_smdFile->GetEventID((int)x, (int)z);
	if( event_index < 2 )
		return false;

	CGameEvent *pEvent = m_EventArray.GetData( event_index );
	if (pEvent == NULL)
		return false;

	if( pEvent->m_bType == 1 && pEvent->m_iExec[0]==ZONE_BATTLE && g_pMain.m_byBattleOpen != NATION_BATTLE ) return false;
	if( pEvent->m_bType == 1 && pEvent->m_iExec[0]==ZONE_SNOW_BATTLE && g_pMain.m_byBattleOpen != SNOW_BATTLE ) return false;
	if( pUser->m_bNation == KARUS && pEvent->m_iExec[0] == ZONE_BATTLE )	{
		if( g_pMain.m_sKarusCount > MAX_BATTLE_ZONE_USERS )	{
			TRACE("### BattleZone karus full users = %d, name=%s \n", g_pMain.m_sKarusCount, pUser->GetName());
			return false;
		}
	}
	else if( pUser->m_bNation == ELMORAD && pEvent->m_iExec[0] == ZONE_BATTLE )	{
		if( g_pMain.m_sElmoradCount > MAX_BATTLE_ZONE_USERS )	{
			TRACE("### BattleZone elmorad full users = %d, name=%s \n", g_pMain.m_sElmoradCount, pUser->GetName());
			return false;
		}
	}
	pEvent->RunEvent( pUser );
	return true;
}

C3DMap::~C3DMap()
{
	m_EventArray.DeleteAllData();

	if (m_ppRegion != NULL)
	{
		for (int i = 0; i <= GetXRegionMax(); i++)
			delete [] m_ppRegion[i];

		delete [] m_ppRegion;
		m_ppRegion = NULL;
	}

	if (m_smdFile != NULL)
		m_smdFile->DecRef();
}