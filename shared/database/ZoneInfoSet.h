#pragma once

typedef std::map<uint16, _ZONE_INFO *> ZoneInfoMap;
	
class CZoneInfoSet : public OdbcRecordset
{
public:
	CZoneInfoSet(OdbcConnection * dbConnection, ZoneInfoMap * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

#if defined(AI_SERVER)
	virtual tstring GetSQL() { return _T("SELECT ServerNo, ZoneNo, strZoneName, RoomEvent FROM ZONE_INFO"); }
#else
	virtual tstring GetSQL() { return _T("SELECT ServerNo, ZoneNo, strZoneName, InitX, InitZ, InitY, Type, isAttackZome FROM ZONE_INFO"); }
#endif

	virtual void Fetch()
	{
		_ZONE_INFO * pData = new _ZONE_INFO;
		
		int i = 1;
		_dbCommand->FetchUInt16(i++, pData->m_nServerNo);
		_dbCommand->FetchUInt16(i++, pData->m_nZoneNumber);
		_dbCommand->FetchString(i++, pData->m_MapName, sizeof(pData->m_MapName));

#ifdef AI_SERVER
		_dbCommand->FetchByte(i++, pData->m_byRoomEvent);
#else
		uint32 iX = 0, iY = 0, iZ = 0;
		_dbCommand->FetchUInt32(i++, iX);
		_dbCommand->FetchUInt32(i++, iZ);
		_dbCommand->FetchUInt32(i++, iY);
		_dbCommand->FetchByte(i++, pData->m_bType);
		_dbCommand->FetchByte(i++, pData->isAttackZone);

		pData->m_fInitX = (float)(iX / 100.0f);
		pData->m_fInitY = (float)(iY / 100.0f);
		pData->m_fInitZ = (float)(iZ / 100.0f);
#endif

		if (m_pMap->find(pData->m_nZoneNumber) != m_pMap->end())
			delete pData;
		else
			m_pMap->insert(std::make_pair(pData->m_nZoneNumber, pData));
	}

	ZoneInfoMap * m_pMap;
};