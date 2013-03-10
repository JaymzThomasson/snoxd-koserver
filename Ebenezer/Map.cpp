// Map.cpp: implementation of the CMap class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Map.h"
#include "Region.h"
#include "Define.h"
#include "User.h"
#include "../shared/database/MyRecordSet.h"
#include "../shared/database/EventSet.h"
#include "EbenezerDlg.h"

using namespace std;

extern CRITICAL_SECTION g_region_critical;

SMDFile::SMDMap SMDFile::s_loadedMaps;

SMDFile::SMDFile() : m_ref(0), m_ppnEvent(NULL), m_fHeight(NULL),
	m_nXRegion(0), m_nZRegion(0), m_nMapSize(0), m_fUnitDist(0.0f)
{
}

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

SMDFile *SMDFile::Load(string mapName)
{
#ifdef WIN32 // case insensitive filenames, allowing for database inconsistency...
	STRTOLOWER(mapName);
#endif

	// Look to see if that SMD file has been loaded already
	SMDMap::iterator itr = s_loadedMaps.find(mapName);

	// If it's been loaded already, we don't need to do anything.
	if (itr != s_loadedMaps.end())
		return itr->second;

	// Map hasn't already been loaded
	string filename = ".\\MAP\\" + mapName;

	// Does this file exist/can it be opened?
	FILE *fp = fopen(filename.c_str(), "rb");
	if (fp == NULL)
		return NULL;

	// Try to load the file now.
	SMDFile *smd = new SMDFile();
	if (!smd->LoadMap(fp))
	{
		// Problem? Make sure we clean up after ourselves.
		delete smd;
		smd = NULL;
	}
	else
	{
		// Loaded fine, so now add it to the map.
		s_loadedMaps.insert(make_pair(mapName, smd));
	}

	fclose(fp);
	return smd;
}

bool SMDFile::LoadMap(FILE *fp)
{
	LoadTerrain(fp);

	m_N3ShapeMgr.Create((m_nMapSize - 1)*m_fUnitDist, (m_nMapSize-1)*m_fUnitDist);
	if (!m_N3ShapeMgr.LoadCollisionData(fp)
		|| (m_nMapSize - 1) * m_fUnitDist != m_N3ShapeMgr.Width() 
		|| (m_nMapSize - 1) * m_fUnitDist != m_N3ShapeMgr.Height() )
		return false;

	int mapwidth = (int)m_N3ShapeMgr.Width();

	m_nXRegion = (int)(mapwidth / VIEW_DISTANCE) + 1;
	m_nZRegion = (int)(mapwidth / VIEW_DISTANCE) + 1;

	LoadObjectEvent(fp);
	LoadMapTile(fp);
	LoadRegeneEvent(fp);	
	LoadWarpList(fp);

	return true;
}

void SMDFile::LoadTerrain(FILE *fp)
{
	fread(&m_nMapSize, sizeof(m_nMapSize), 1, fp);
	fread(&m_fUnitDist, sizeof(m_fUnitDist), 1, fp);

	m_fHeight = new float[m_nMapSize * m_nMapSize];
	fread(m_fHeight, sizeof(float) * m_nMapSize * m_nMapSize, 1, fp);
}

void SMDFile::LoadObjectEvent(FILE *fp)
{
	int iEventObjectCount = 0;
	fread(&iEventObjectCount, sizeof(int), 1, fp);
	for (int i = 0; i < iEventObjectCount; i++)
	{
		_OBJECT_EVENT* pEvent = new _OBJECT_EVENT;

		fread(pEvent, sizeof(_OBJECT_EVENT) - sizeof(pEvent->byLife), 1, fp);
		pEvent->byLife = 1;

		if (pEvent->sIndex <= 0
			|| !m_ObjectEventArray.PutData(pEvent->sIndex, pEvent))
			delete pEvent;
	}
}

void SMDFile::LoadMapTile(FILE *fp)
{
	m_ppnEvent = new short[m_nMapSize * m_nMapSize];
	fread(m_ppnEvent, sizeof(short) * m_nMapSize * m_nMapSize, 1, fp);
}

void SMDFile::LoadRegeneEvent(FILE *fp)	
{
	int iEventObjectCount = 0;
	fread(&iEventObjectCount, sizeof(iEventObjectCount), 1, fp);
	for (int i = 0; i < iEventObjectCount; i++)
	{
		_REGENE_EVENT *pEvent = new _REGENE_EVENT;
		fread(pEvent, sizeof(_REGENE_EVENT) - sizeof(pEvent->sRegenePoint), 1, fp);
		pEvent->sRegenePoint = i;

		if (pEvent->sRegenePoint < 0
			|| !m_ObjectRegeneArray.PutData(pEvent->sRegenePoint, pEvent))
			delete pEvent;
	}	
}

void SMDFile::LoadWarpList(FILE *fp)
{
	int WarpCount = 0;

	fread(&WarpCount, sizeof(WarpCount), 1, fp);
	for (int i = 0; i < WarpCount; i++)
	{
		_WARP_INFO *pWarp = new _WARP_INFO;
		fread(pWarp, sizeof(_WARP_INFO), 1, fp);

		if (pWarp->sWarpID == 0
			|| !m_WarpArray.PutData(pWarp->sWarpID, pWarp))
			delete pWarp;
	}
}

void SMDFile::GetWarpList(int warpGroup, std::set<_WARP_INFO *> & warpEntries)
{
	foreach_stlmap (itr, m_WarpArray)
	{
		_WARP_INFO *pWarp = itr->second;
		if (pWarp == NULL || (pWarp->sWarpID / 10) != warpGroup)
			continue;
		
		warpEntries.insert(pWarp);
	}
}

BOOL SMDFile::IsValidPosition(float x, float z, float y)
{
	// TO-DO: Implement more thorough check
	return (x < m_N3ShapeMgr.Width() && z < m_N3ShapeMgr.Height());
}

float SMDFile::GetHeight(float x, float y, float z)
{
	int iX, iZ;
	iX = (int)(x/m_fUnitDist);
	iZ = (int)(z/m_fUnitDist);
	//_ASSERT( iX, iZ가 범위내에 있는 값인지 체크하기);

	float fYTerrain;
	float h1, h2, h3;
	float dX, dZ;
	dX = (x - iX*m_fUnitDist)/m_fUnitDist;
	dZ = (z - iZ*m_fUnitDist)/m_fUnitDist;

//	_ASSERT(dX>=0.0f && dZ>=0.0f && dX<1.0f && dZ<1.0f);
	if( !(dX>=0.0f && dZ>=0.0f && dX<1.0f && dZ<1.0f) )
		return FLT_MIN;

	if ((iX+iZ)%2==1)
	{
		if ((dX+dZ) < 1.0f)
		{
			h1 = m_fHeight[iX * m_nMapSize + iZ+1];
			h2 = m_fHeight[iX+1 * m_nMapSize + iZ];
			h3 = m_fHeight[iX * m_nMapSize + iZ];

			//if (dX == 1.0f) return h2;

			float h12 = h1+(h2-h1)*dX;	// h1과 h2사이의 높이값
			float h32 = h3+(h2-h3)*dX;	// h3과 h2사이의 높이값
			fYTerrain = h32 + (h12-h32)*((dZ)/(1.0f-dX));	// 찾고자 하는 높이값
		}
		else
		{
			h1 = m_fHeight[iX * m_nMapSize + iZ+1];
			h2 = m_fHeight[iX+1 * m_nMapSize + iZ];
			h3 = m_fHeight[iX+1 * m_nMapSize + iZ+1];

			if (dX == 0.0f) return h1;

			float h12 = h1+(h2-h1)*dX;	// h1과 h2사이의 높이값
			float h13 = h1+(h3-h1)*dX;	// h1과 h3사이의 높이값
			fYTerrain = h13 + (h12-h13)*((1.0f-dZ)/(dX));	// 찾고자 하는 높이값
		}
	}
	else
	{
		if (dZ > dX)
		{
			h1 = m_fHeight[iX * m_nMapSize + iZ+1];
			h2 = m_fHeight[iX+1 * m_nMapSize + iZ+1];
			h3 = m_fHeight[iX * m_nMapSize + iZ];

			//if (dX == 1.0f) return h2;

			float h12 = h1+(h2-h1)*dX;	// h1과 h2사이의 높이값
			float h32 = h3+(h2-h3)*dX;	// h3과 h2사이의 높이값
			fYTerrain = h12 + (h32-h12)*((1.0f-dZ)/(1.0f-dX));	// 찾고자 하는 높이값
		}
		else
		{
			h1 = m_fHeight[iX * m_nMapSize + iZ];
			h2 = m_fHeight[iX+1 * m_nMapSize + iZ];
			h3 = m_fHeight[iX+1 * m_nMapSize + iZ+1];

			if (dX == 0.0f) return h1;

			float h12 = h1+(h2-h1)*dX;	// h1과 h2사이의 높이값
			float h13 = h1+(h3-h1)*dX;	// h1과 h3사이의 높이값
			fYTerrain = h12 + (h13-h12)*((dZ)/(dX));	// 찾고자 하는 높이값
		}
	}

	__Vector3 vPos(x, y, z);
	float fHeight = m_N3ShapeMgr.GetHeightNearstPos(vPos); // 가장 가까운 높이값을 돌려준다..
	if(-FLT_MAX != fHeight && fHeight > fYTerrain) return fHeight;
	else return fYTerrain;
}

int SMDFile::GetEventID(float x, float z)
{
	int iX = (int)(x / m_fUnitDist);
	int iZ = (int)(z / m_fUnitDist);
	if (iX < 0 || iX >= m_nMapSize || iZ < 0 || iZ >= m_nMapSize)
		return 0;

	return m_ppnEvent[iX * m_nMapSize + iZ];
}

BOOL SMDFile::ObjectCollision(float x1, float z1, float y1, float x2, float z2, float y2)
{
	__Vector3	vec1(x1, y1, z1), vec2(x2, y2, z2);
	__Vector3	vDir = vec2 - vec1;
	float fSpeed = 	vDir.Magnitude();
	vDir.Normalize();
	
	return m_N3ShapeMgr.CheckCollision(vec1, vDir, fSpeed);
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
	int event_index = m_smdFile->GetEventID(x, z);
	if( event_index < 2 )
		return FALSE;

	CGameEvent *pEvent = m_EventArray.GetData( event_index );
	if (pEvent == NULL)
		return FALSE;

	if( pEvent->m_bType == 1 && pEvent->m_iExec[0]==ZONE_BATTLE && g_pMain->m_byBattleOpen != NATION_BATTLE ) return FALSE;
	if( pEvent->m_bType == 1 && pEvent->m_iExec[0]==ZONE_SNOW_BATTLE && g_pMain->m_byBattleOpen != SNOW_BATTLE ) return FALSE;
	if( pUser->m_bNation == KARUS && pEvent->m_iExec[0] == ZONE_BATTLE )	{
		if( g_pMain->m_sKarusCount > MAX_BATTLE_ZONE_USERS )	{
			TRACE("### BattleZone karus full users = %d, name=%s \n", g_pMain->m_sKarusCount, pUser->m_id);
			return FALSE;
		}
	}
	else if( pUser->m_bNation == ELMORAD && pEvent->m_iExec[0] == ZONE_BATTLE )	{
		if( g_pMain->m_sElmoradCount > MAX_BATTLE_ZONE_USERS )	{
			TRACE("### BattleZone elmorad full users = %d, name=%s \n", g_pMain->m_sElmoradCount, pUser->m_id);
			return FALSE;
		}
	}
	pEvent->RunEvent( pUser );
	return TRUE;
}

BOOL C3DMap::LoadEvent()
{
	CEventSet EventSet(&m_EventArray, (BYTE)m_nZoneNumber, &g_pMain->m_GameDB);
	return EventSet.Read();
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

SMDFile::~SMDFile()
{
	if (m_ppnEvent != NULL)
	{
		delete [] m_ppnEvent;
		m_ppnEvent = NULL;
	}

	if (m_fHeight != NULL)
	{
		delete[] m_fHeight;
		m_fHeight = NULL;
	}
}
