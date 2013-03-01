#pragma once

/**
 * This class is a bridge between the CNpc & CUser classes
 * Currently it's excessively messier than it needs to be, 
 * because of Aujard and the whole _USER_DATA setup.
 *
 * This will be written out eventually, so we can do this properly.
 **/
class Unit
{
public:
	Unit(bool bPlayer = false) : m_pMap(NULL), m_pRegion(NULL), m_sRegionX(0), m_sRegionZ(0), m_bPlayer(bPlayer) {}

	__forceinline bool isPlayer() { return m_bPlayer; }
	__forceinline bool isNPC() { return !isPlayer(); }

	__forceinline C3DMap * GetMap() { return m_pMap; }

	virtual uint16 GetID() = 0;
	virtual uint8 GetZoneID() = 0;
	virtual float GetX() = 0;
	virtual float GetY() = 0;
	virtual float GetZ() = 0;

	__forceinline uint16 GetSPosX() { return uint16(GetX() * 10); };
	__forceinline uint16 GetSPosY() { return uint16(GetY() * 10); };
	__forceinline uint16 GetSPosZ() { return uint16(GetZ() * 10); };

	__forceinline uint16 GetRegionX() { return m_sRegionX; }
	__forceinline uint16 GetRegionZ() { return m_sRegionZ; }

	__forceinline uint16 GetNewRegionX() { return (uint16)(GetX()) / VIEW_DISTANCE; }
	__forceinline uint16 GetNewRegionZ() { return (uint16)(GetZ()) / VIEW_DISTANCE; }

	__forceinline CRegion * GetRegion() { return m_pRegion; }
	__forceinline void SetRegion(uint16 x = -1, uint16 z = -1) 
	{
		m_sRegionX = x; m_sRegionZ = z; 
		m_pRegion = m_pMap->GetRegion(x, z); // TO-DO: Clean this up
	}

	virtual const char * GetName() = 0; // this is especially fun...

	virtual uint8 GetNation() = 0;
	virtual uint8 GetLevel() = 0;

	virtual void GetInOut(Packet & result, uint8 bType) = 0;

	bool RegisterRegion();
	void RemoveRegion(int16 del_x, int16 del_z);
	void InsertRegion(int16 insert_x, int16 insert_z);

	void SendToRegion(Packet *result);

	void OnDeath(Unit *pKiller);
	void SendDeathAnimation();

// public for the moment
// protected:
	C3DMap * m_pMap;
	CRegion * m_pRegion;

	uint16 m_sRegionX, m_sRegionZ; // this is probably redundant

	bool m_bPlayer;
};
