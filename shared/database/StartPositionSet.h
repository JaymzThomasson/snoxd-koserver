#pragma once

#define T		_START_POSITION
#define MapType	StartPositionArray

class CStartPositionSet : public CMyRecordSet<T>
{
public:
	CStartPositionSet(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 11;
	}

	DECLARE_DYNAMIC(CStartPositionSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[START_POSITION]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Int(pFX, _T("[ZoneID]"), m_data.ZoneID);
		RFX_Int(pFX, _T("[sKarusX]"), m_data.sKarusX);
		RFX_Int(pFX, _T("[sKarusZ]"), m_data.sKarusZ);
		RFX_Int(pFX, _T("[sElmoradX]"), m_data.sElmoradX);
		RFX_Int(pFX, _T("[sElmoradZ]"), m_data.sElmoradZ);
		RFX_Int(pFX, _T("[sKarusGateX]"), m_data.sKarusGateX);
		RFX_Int(pFX, _T("[sKarusGateZ]"), m_data.sKarusGateZ);
		RFX_Int(pFX, _T("[sElmoGateX]"), m_data.sElmoradGateX);
		RFX_Int(pFX, _T("[sElmoGateZ]"), m_data.sElmoradGateZ);
		RFX_Byte(pFX, _T("[bRangeX]"), m_data.bRangeX);
		RFX_Byte(pFX, _T("[bRangeZ]"), m_data.bRangeZ);
	};

	virtual void HandleRead()
	{
		T * data = COPY_ROW();
		if (!m_stlMap->PutData(data->ZoneID, data))
			delete data;
	};

private:
	MapType * m_stlMap;
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CStartPositionSet, CRecordset)