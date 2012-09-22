#pragma once

#define T		_HOME_INFO
#define MapType	HomeArray

class CHomeSet : public CMyRecordSet<T>
{
public:
	CHomeSet(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 17;
	}

	DECLARE_DYNAMIC(CHomeSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[HOME]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Byte(pFX, _T("[Nation]"), m_data.bNation);
		RFX_Long(pFX, _T("[ElmoZoneX]"), m_data.ElmoZoneX);
		RFX_Long(pFX, _T("[ElmoZoneZ]"), m_data.ElmoZoneZ);
		RFX_Byte(pFX, _T("[ElmoZoneLX]"), m_data.ElmoZoneLX);
		RFX_Byte(pFX, _T("[ElmoZoneLZ]"), m_data.ElmoZoneLZ);
		RFX_Long(pFX, _T("[KarusZoneX]"), m_data.KarusZoneX);
		RFX_Long(pFX, _T("[KarusZoneZ]"), m_data.KarusZoneZ);
		RFX_Byte(pFX, _T("[KarusZoneLX]"), m_data.KarusZoneLX);
		RFX_Byte(pFX, _T("[KarusZoneLZ]"), m_data.KarusZoneLZ);
		RFX_Long(pFX, _T("[FreeZoneX]"), m_data.FreeZoneX);
		RFX_Long(pFX, _T("[FreeZoneZ]"), m_data.FreeZoneZ);
		RFX_Byte(pFX, _T("[FreeZoneLX]"), m_data.FreeZoneLX);
		RFX_Byte(pFX, _T("[FreeZoneLZ]"), m_data.FreeZoneLZ);
		RFX_Long(pFX, _T("[BattleZoneX]"), m_data.BattleZoneX);
		RFX_Long(pFX, _T("[BattleZoneZ]"), m_data.BattleZoneZ);
		RFX_Byte(pFX, _T("[BattleZoneLX]"), m_data.BattleZoneLX);
		RFX_Byte(pFX, _T("[BattleZoneLZ]"), m_data.BattleZoneLZ);
	};

	virtual void HandleRead()
	{
		T * data = COPY_ROW();
		if (!m_stlMap->PutData(data->bNation, data))
			delete data;
	};

private:
	MapType * m_stlMap;
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CHomeSet, CRecordset)