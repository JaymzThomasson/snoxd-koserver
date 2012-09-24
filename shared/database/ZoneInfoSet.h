#pragma once

#define T _ZONE_INFO
#define MapType	map<int, _ZONE_INFO *>

class CZoneInfoSet : public CMyRecordSet<T>
{
public:
	CZoneInfoSet(MapType *pMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_map(pMap)
	{
#ifdef EBENEZER
		m_nFields = 7;
#else
		m_nFields = 4;
#endif
	}

	DECLARE_DYNAMIC(CZoneInfoSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[ZONE_INFO]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Int(pFX, _T("[ServerNo]"), m_data.m_nServerNo);
		RFX_Int(pFX, _T("[ZoneNo]"), m_data.m_nZoneNumber);
		RFX_Text(pFX, _T("[strZoneName]"), m_data.m_MapName, sizeof(m_data.m_MapName));

#ifdef EBENEZER
		RFX_Long(pFX, _T("[InitX]"), m_InitX);
		RFX_Long(pFX, _T("[InitZ]"), m_InitZ);
		RFX_Long(pFX, _T("[InitY]"), m_InitY);
		RFX_Byte(pFX, _T("[Type]"), m_data.m_bType);
#else
		RFX_Byte(pFX, _T("[RoomEvent]"), m_data.m_byRoomEvent);
#endif
	};

	virtual void HandleRead()
	{
		T * data = COPY_ROW();

#ifdef EBENEZER
		data->m_fInitX = (float)(m_InitX / 100.0f);
		data->m_fInitY = (float)(m_InitY / 100.0f);
		data->m_fInitZ = (float)(m_InitZ / 100.0f);
#endif

		if (!m_map->insert(make_pair(data->m_nZoneNumber, data)).second)
			delete data;
	};

private:
	MapType * m_map;
#ifdef EBENEZER
	long m_InitX, m_InitY, m_InitZ;
#endif
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CZoneInfoSet, CRecordset)