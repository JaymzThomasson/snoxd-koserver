#pragma once

#include "Region.h"
#include "GameEvent.h"
#include "../shared/STLMap.h"

class CGameEvent;
typedef CSTLMap <CGameEvent>		EventArray;

class CUser;
class CEbenezerDlg;
class SMDFile;
class C3DMap
{
public:
	// Passthru methods
	int GetXRegionMax();
	int GetZRegionMax(); 
	bool IsValidPosition(float x, float z, float y);
	_OBJECT_EVENT * GetObjectEvent(int objectindex);
	_REGENE_EVENT * GetRegeneEvent(int objectindex);
	_WARP_INFO * GetWarp(int warpID);
	void GetWarpList(int warpGroup, std::set<_WARP_INFO *> & warpEntries);
	
	INLINE bool isAttackZone() { return m_isAttackZone; }

	C3DMap();
	bool Initialize(_ZONE_INFO *pZone);
	CRegion * GetRegion(uint16 regionX, uint16 regionZ);
	bool CheckEvent( float x, float z, CUser* pUser = nullptr );
	void RegionItemRemove(CRegion * pRegion, _LOOT_BUNDLE * pBundle, _LOOT_ITEM * pItem);
	bool RegionItemAdd(uint16 rx, uint16 rz, _LOOT_BUNDLE * pBundle);
	bool ObjectCollision(float x1, float z1, float y1, float x2, float z2, float y2);
	float GetHeight( float x, float y, float z );
	virtual ~C3DMap();

	EventArray	m_EventArray;

	int	m_nServerNo, m_nZoneNumber;
	float m_fInitX, m_fInitZ, m_fInitY;
	uint8	m_bType;		// Zone Type : 1 -> common zone,  2 -> battle zone, 3 -> 24 hour open battle zone
	short	m_sMaxUser;
	bool m_isAttackZone;

	CRegion**	m_ppRegion;

	uint32	m_wBundle;	// Zone Item Max Count

	SMDFile *m_smdFile;
	FastMutex m_lock;
};