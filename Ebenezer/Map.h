// Map.h: interface for the CMap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_3DMAP_H__986E02B6_E5A3_43CF_B1D7_A7135551933D__INCLUDED_)
#define AFX_3DMAP_H__986E02B6_E5A3_43CF_B1D7_A7135551933D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\N3Base\N3ShapeMgr.h"
#include "Region.h"
#include "GameEvent.h"
#include "../shared/STLMap.h"
#include <set>

struct _ZONE_INFO
{
	int m_nServerNo;
	int m_nZoneNumber;
	char m_MapName[_MAX_PATH];
	float m_fInitX, m_fInitY, m_fInitZ;
	BYTE m_bType, isAttackZone;

	_ZONE_INFO()
	{
		memset(m_MapName, 0x00, sizeof(m_MapName));
	};
};


typedef CSTLMap <CGameEvent>		EventArray;
typedef CSTLMap <_OBJECT_EVENT>		ObjectEventArray;
typedef CSTLMap <_REGENE_EVENT>		ObjectRegeneArray;
typedef	CSTLMap <_WARP_INFO>		WarpArray;

class CUser;
class CEbenezerDlg;

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

	int GetEventID(float x, float z);

	__forceinline int GetXRegionMax() { return m_nXRegion - 1; }
	__forceinline int GetZRegionMax() { return m_nZRegion - 1; }

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

	friend class C3DMap;
};

class C3DMap
{
public:
	// Passthru methods
	__forceinline int GetXRegionMax() { return m_smdFile->GetXRegionMax(); }
	__forceinline int GetZRegionMax() { return m_smdFile->GetZRegionMax(); }

	__forceinline BOOL IsValidPosition(float x, float z, float y) { return m_smdFile->IsValidPosition(x, z, y); }
	
	__forceinline _OBJECT_EVENT * GetObjectEvent(int objectindex) { return m_smdFile->GetObjectEvent(objectindex); }
	__forceinline _REGENE_EVENT * GetRegeneEvent(int objectindex) { return m_smdFile->GetRegeneEvent(objectindex); }
	__forceinline _WARP_INFO * GetWarp(int warpID) { return m_smdFile->GetWarp(warpID); }
	__forceinline void GetWarpList(int warpGroup, std::set<_WARP_INFO *> & warpEntries) { m_smdFile->GetWarpList(warpGroup, warpEntries); }
	
	C3DMap();
	bool Initialize(_ZONE_INFO *pZone);
	BOOL LoadEvent();
	BOOL CheckEvent( float x, float z, CUser* pUser = NULL );
	BOOL RegionNpcRemove( int rx, int rz, int nid );
	void RegionNpcAdd( int rx, int rz, int nid );
	BOOL RegionUserRemove( int rx, int rz, int uid );
	void RegionUserAdd( int rx, int rz, int uid );
	BOOL RegionItemRemove( int rx, int rz, int bundle_index, int itemid, int count );
	BOOL RegionItemAdd( int rx, int rz, _ZONE_ITEM* pItem );
	BOOL ObjectCollision(float x1, float z1, float y1, float x2, float z2, float y2);
	float GetHeight( float x, float y, float z );
	virtual ~C3DMap();

	EventArray	m_EventArray;

	int	m_nServerNo, m_nZoneNumber;
	float m_fInitX, m_fInitZ, m_fInitY;
	BYTE	m_bType;		// Zone Type : 1 -> common zone,  2 -> battle zone, 3 -> 24 hour open battle zone
	short	m_sMaxUser;
	uint8 m_isAttackZone;

	CRegion**	m_ppRegion;

	DWORD m_wBundle;	// Zone Item Max Count

	SMDFile *m_smdFile;
	CEbenezerDlg* m_pMain;
};

#endif // !defined(AFX_3DMAP_H__986E02B6_E5A3_43CF_B1D7_A7135551933D__INCLUDED_)
