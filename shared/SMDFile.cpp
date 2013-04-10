#include "stdafx.h"
#include "../N3Base/N3ShapeMgr.h"
#include "STLMap.h"
#include "database/structs.h"
#include <set>
#include "SMDFile.h"

SMDFile::SMDMap SMDFile::s_loadedMaps;

SMDFile::SMDFile() : m_ppnEvent(NULL), m_fHeight(NULL),
	m_nXRegion(0), m_nZRegion(0), m_nMapSize(0), m_fUnitDist(0.0f),
	m_N3ShapeMgr(new CN3ShapeMgr())
{
}

SMDFile *SMDFile::Load(std::string mapName)
{
#ifdef WIN32 // case insensitive filenames, allowing for database inconsistency...
	STRTOLOWER(mapName);
#endif

	// Look to see if that SMD file has been loaded already
	SMDMap::iterator itr = s_loadedMaps.find(mapName);

	// If it's been loaded already, we don't need to do anything.
	if (itr != s_loadedMaps.end())
	{
		// Add another reference.
		itr->second->IncRef();
		return itr->second;
	}

	// Map hasn't already been loaded
	std::string filename = ".\\MAP\\" + mapName;

	// Does this file exist/can it be opened?
	FILE *fp = fopen(filename.c_str(), "rb");
	if (fp == NULL)
		return NULL;

	// Try to load the file now.
	SMDFile *smd = new SMDFile();
	if (!smd->LoadMap(fp))
	{
		// Problem? Make sure we clean up after ourselves.
		smd->DecRef(); // it's the only reference anyway
		smd = NULL;
	}
	else
	{
		// Loaded fine, so now add it to the map.
		s_loadedMaps.insert(std::make_pair(mapName, smd));
	}

	fclose(fp);
	return smd;
}

bool SMDFile::LoadMap(FILE *fp)
{
	LoadTerrain(fp);

	m_N3ShapeMgr->Create((m_nMapSize - 1)*m_fUnitDist, (m_nMapSize-1)*m_fUnitDist);
	if (!m_N3ShapeMgr->LoadCollisionData(fp)
		|| (m_nMapSize - 1) * m_fUnitDist != m_N3ShapeMgr->Width() 
		|| (m_nMapSize - 1) * m_fUnitDist != m_N3ShapeMgr->Height())
		return false;

	int mapwidth = (int)m_N3ShapeMgr->Width();

	m_nXRegion = (int)(mapwidth / VIEW_DISTANCE) + 1;
	m_nZRegion = (int)(mapwidth / VIEW_DISTANCE) + 1;

	LoadObjectEvent(fp);
	LoadMapTile(fp);

#if defined(EBENEZER)
	LoadRegeneEvent(fp);	
	LoadWarpList(fp);
#endif

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

bool SMDFile::IsValidPosition(float x, float z, float y)
{
	// TO-DO: Implement more thorough check
	return (x < m_N3ShapeMgr->Width() && z < m_N3ShapeMgr->Height());
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
	float fHeight = m_N3ShapeMgr->GetHeightNearstPos(vPos); // 가장 가까운 높이값을 돌려준다..
	if(-FLT_MAX != fHeight && fHeight > fYTerrain) return fHeight;
	else return fYTerrain;
}

int SMDFile::GetEventID(int x, int z)
{
	if (x < 0 || x >= m_nMapSize || z < 0 || z >= m_nMapSize)
		return 0;

	return m_ppnEvent[x * m_nMapSize + z];
}

bool SMDFile::ObjectCollision(float x1, float z1, float y1, float x2, float z2, float y2)
{
	__Vector3	vec1(x1, y1, z1), vec2(x2, y2, z2);
	__Vector3	vDir = vec2 - vec1;
	float fSpeed = 	vDir.Magnitude();
	vDir.Normalize();
	
	return m_N3ShapeMgr->CheckCollision(vec1, vDir, fSpeed);
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

	delete m_N3ShapeMgr;
}
