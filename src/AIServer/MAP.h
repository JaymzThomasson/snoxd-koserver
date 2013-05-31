#pragma once

#include "../N3Base/N3ShapeMgr.h"
#include "../shared/STLMap.h"
#include "Extern.h"
#include "RoomEvent.h"

typedef CSTLMap <_OBJECT_EVENT>		ObjectEventArray;
typedef CSTLMap <CRoomEvent>		RoomEventArray;

class CRegion;
class CNpc;
class CUser;

// temporary
struct CSize
{
	CSize() : cx(0), cy(0) {}
	CSize(int cx, int cy) : cx(cx), cy(cy) {}
	int cx, cy;
};

class SMDFile;
class MAP  
{
public:
	// Passthru methods
	int GetMapSize();
	float GetUnitDistance();
	int GetXRegionMax();
	int GetZRegionMax();
	short * GetEventIDs();
	int GetEventID(int x, int z);

	CRegion**		m_ppRegion;				// 64미터의 타일정보..
	int m_nZoneNumber;						// zone number
	int	m_nServerNo;
	std::string m_MapName;
	float*		m_fHeight;
	uint8		m_byRoomType;		// 방의 초기화관련( 0:자동으로 초기화, 1:전쟁이벤트 관련(특정조건이 완료시 초기화)
	uint8		m_byRoomEvent;		// event room(0:empty, 1:use)
	uint8		m_byRoomStatus;		// room status(1:진행중, 2:방을 초기화중, 3:방초기화 완료)
	uint8		m_byInitRoomCount;	// room 초기화 시간을 제어(몬스터와 동기화를 맞추기 위해)
	ObjectEventArray m_ObjectEventArray;
	RoomEventArray	 m_arRoomEventArray;
	short	m_sKarusRoom;			// karus의 성갯수
	short	m_sElmoradRoom;			// elmorad의 성갯수

	FastMutex m_lock;

public:
	MAP();
	virtual ~MAP();

	bool Initialize(_ZONE_INFO *pZone);

	bool LoadRoomEvent();
	bool ObjectIntersect(float x1, float z1, float y1, float x2, float z2, float y2);
	float GetHeight( float x, float z );

	bool RegionNpcRemove( int rx, int rz, int nid );
	void RegionNpcAdd( int rx, int rz, int nid );
	bool RegionUserRemove( int rx, int rz, int uid );
	void RegionUserAdd( int rx, int rz, int uid );

	int IsRoomCheck(float fx, float fz);	// 던젼에서 사용, 유저의 현재위치가 던젼의 어느 위치에 있는지를 판단
	bool IsRoomStatusCheck();

	bool IsMovable(int dest_x, int dest_y);
	void InitializeRoom();

	CRoomEvent* SetRoomEvent( int number );

protected:
	void RemoveMapData();

	SMDFile * m_smdFile;
};