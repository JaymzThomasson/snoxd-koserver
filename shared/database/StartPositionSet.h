#pragma once

#if 0
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
#endif

	
class CStartPositionSet : public OdbcRecordset
{
public:
	CStartPositionSet(OdbcConnection * dbConnection, StartPositionArray * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetSQL() { return _T("SELECT ZoneID, sKarusX, sKarusZ, sElmoradX, sElmoradZ, sKarusGateX, sKarusGateZ, sElmoGateX, sElmoGateZ, bRangeX, bRangeZ FROM START_POSITION"); }
	virtual void Fetch()
	{
		_START_POSITION *pData = new _START_POSITION;

		_dbCommand->FetchUInt16(1, pData->ZoneID);
		_dbCommand->FetchUInt16(2, pData->sKarusX);
		_dbCommand->FetchUInt16(3, pData->sKarusZ);
		_dbCommand->FetchUInt16(4, pData->sElmoradX);
		_dbCommand->FetchUInt16(5, pData->sElmoradZ);
		_dbCommand->FetchUInt16(6, pData->sKarusGateX);
		_dbCommand->FetchUInt16(7, pData->sKarusGateZ);
		_dbCommand->FetchUInt16(8, pData->sElmoradGateX);
		_dbCommand->FetchUInt16(9, pData->sElmoradGateZ);
		_dbCommand->FetchByte (10, pData->bRangeX);
		_dbCommand->FetchByte (11, pData->bRangeZ);

		if (!m_pMap->PutData(pData->ZoneID, pData))
			delete pData;
	}

	StartPositionArray *m_pMap;
};