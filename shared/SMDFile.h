#pragma once

class CGameEvent;
class CUser;

typedef CSTLMap <CGameEvent>		EventArray;
typedef CSTLMap <_OBJECT_EVENT>		ObjectEventArray;
typedef CSTLMap <_REGENE_EVENT>		ObjectRegeneArray;
typedef	CSTLMap <_WARP_INFO>		WarpArray;

class SMDFile
{
public:
	SMDFile();

	static SMDFile *Load(std::string mapName);

	bool LoadMap(FILE *fp);
	void LoadTerrain(FILE *fp);
	void LoadObjectEvent(FILE *fp);
	void LoadMapTile(FILE *fp);
	void LoadRegeneEvent(FILE *fp);
	void LoadWarpList(FILE *fp);

	BOOL IsValidPosition(float x, float z, float y);
	BOOL CheckEvent( float x, float z, CUser* pUser = NULL );
	BOOL ObjectCollision(float x1, float z1, float y1, float x2, float z2, float y2);
	float GetHeight( float x, float y, float z );

	int GetEventID(int x, int z);

	__forceinline int GetMapSize() { return m_nMapSize - 1; }
	__forceinline float GetUnitDistance() { return m_fUnitDist; }
	__forceinline int GetXRegionMax() { return m_nXRegion - 1; }
	__forceinline int GetZRegionMax() { return m_nZRegion - 1; }

	__forceinline short * GetEventIDs() { return m_ppnEvent; }

	__forceinline void IncRef() { m_ref++; }
	__forceinline void DecRef() { if (--m_ref == 0) delete this; }

	__forceinline _OBJECT_EVENT * GetObjectEvent(int objectindex) { return m_ObjectEventArray.GetData(objectindex); }
	__forceinline _REGENE_EVENT * GetRegeneEvent(int objectindex) { return m_ObjectRegeneArray.GetData(objectindex); }
	__forceinline _WARP_INFO * GetWarp(int warpID) { return m_WarpArray.GetData(warpID); }

	void GetWarpList(int warpGroup, std::set<_WARP_INFO *> & warpEntries);

	virtual ~SMDFile();

private:
	int m_ref;

	short*		m_ppnEvent;
	WarpArray	m_WarpArray;

	ObjectEventArray	m_ObjectEventArray;
	ObjectRegeneArray	m_ObjectRegeneArray;

	CN3ShapeMgr m_N3ShapeMgr;

	float*		m_fHeight;

	int			m_nXRegion, m_nZRegion;

	int			m_nMapSize;		// Grid Unit ex) 4m
	float		m_fUnitDist;	// i Grid Distance

	typedef std::map<std::string, SMDFile *> SMDMap;
	static SMDMap s_loadedMaps;

#if defined(EBENEZER)
	friend class C3DMap;
#elif defined(AI_SERVER)
	friend class MAP;
#endif
};
