// Map.cpp: implementation of the CMap class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Map.h"
#include "Region.h"
#include "Define.h"
#include "User.h"
#include "../shared/database/EventSet.h"
#include "EbenezerDlg.h"

using namespace std;

extern CRITICAL_SECTION g_region_critical;

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

		if (!LoadEvent())
			return false;

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


BOOL C3DMap::RegionItemAdd(uint16 rx, uint16 rz, _ZONE_ITEM * pItem)
{
	if (rx >= GetXRegionMax() || rz >= GetZRegionMax()
		|| pItem == NULL)
		return FALSE;

	EnterCriticalSection( &g_region_critical );

	pItem->bundle_index = m_wBundle++;
	m_ppRegion[rx][rz].m_RegionItemArray.PutData(pItem->bundle_index, pItem);
	if (m_wBundle > ZONEITEM_MAX)
		m_wBundle = 1;

	LeaveCriticalSection( &g_region_critical );

	return TRUE;
}

BOOL C3DMap::RegionItemRemove(uint16 rx, uint16 rz, int bundle_index, int itemid, int count)
{
	if (rx >= GetXRegionMax() || rz >= GetZRegionMax())
		return FALSE;
	
	_ZONE_ITEM* pItem = NULL;
	CRegion	*region = NULL;
	BOOL bFind = FALSE;
	short t_count = 0;
	
	EnterCriticalSection( &g_region_critical );
	
	region = &m_ppRegion[rx][rz];
	pItem = (_ZONE_ITEM*)region->m_RegionItemArray.GetData( bundle_index );
	if( pItem ) {
		for(int j=0; j<6; j++) {
			if( pItem->itemid[j] == itemid && pItem->count[j] == count ) {
				pItem->itemid[j] = 0; pItem->count[j] = 0;
				bFind = TRUE;
				break;
			}
		}
		if( bFind ) {
			for( int j=0; j<6; j++) {
				if( pItem->itemid[j] != 0 )
					t_count++;
			}
			if( !t_count )
				region->m_RegionItemArray.DeleteData( bundle_index );
		}
	}

	LeaveCriticalSection( &g_region_critical );

	return bFind;
}

BOOL C3DMap::CheckEvent(float x, float z, CUser* pUser)
{
	int event_index = m_smdFile->GetEventID((int)x, (int)z);
	if( event_index < 2 )
		return FALSE;

	CGameEvent *pEvent = m_EventArray.GetData( event_index );
	if (pEvent == NULL)
		return FALSE;

	if( pEvent->m_bType == 1 && pEvent->m_iExec[0]==ZONE_BATTLE && g_pMain.m_byBattleOpen != NATION_BATTLE ) return FALSE;
	if( pEvent->m_bType == 1 && pEvent->m_iExec[0]==ZONE_SNOW_BATTLE && g_pMain.m_byBattleOpen != SNOW_BATTLE ) return FALSE;
	if( pUser->m_bNation == KARUS && pEvent->m_iExec[0] == ZONE_BATTLE )	{
		if( g_pMain.m_sKarusCount > MAX_BATTLE_ZONE_USERS )	{
			TRACE("### BattleZone karus full users = %d, name=%s \n", g_pMain.m_sKarusCount, pUser->GetName());
			return FALSE;
		}
	}
	else if( pUser->m_bNation == ELMORAD && pEvent->m_iExec[0] == ZONE_BATTLE )	{
		if( g_pMain.m_sElmoradCount > MAX_BATTLE_ZONE_USERS )	{
			TRACE("### BattleZone elmorad full users = %d, name=%s \n", g_pMain.m_sElmoradCount, pUser->GetName());
			return FALSE;
		}
	}
	pEvent->RunEvent( pUser );
	return TRUE;
}

BOOL C3DMap::LoadEvent()
{
	LOAD_TABLE(CEventSet, &g_DBAgent.m_GameDB, this, true);
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