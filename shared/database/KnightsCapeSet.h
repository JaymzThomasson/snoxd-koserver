#pragma once

#define T		_KNIGHTS_CAPE
#define MapType	KnightsCapeArray

class CKnightsCapeSet : public CMyRecordSet<T>
{
public:
	CKnightsCapeSet(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 5;
	}

	DECLARE_DYNAMIC(CKnightsCapeSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[KNIGHTS_CAPE]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Int(pFX, _T("[sCapeIndex]"), m_data.sCapeIndex);
		RFX_Long(pFX, _T("[nBuyPrice]"), m_data.nReqCoins);
		RFX_Long(pFX, _T("[nBuyLoyalty]"), m_data.nReqClanPoints); // this is in NP form (in the TBL)
		RFX_Byte(pFX, _T("[byGrade]"), m_data.byGrade);
		RFX_Byte(pFX, _T("[byRanking]"), m_data.byRanking);
	};

	virtual void HandleRead()
	{
		T * data = COPY_ROW();

		// Convert this from NP to clan points
		data->nReqClanPoints /= MAX_CLAN_USERS;

		if (!m_stlMap->PutData(data->sCapeIndex, data))
			delete data;
	};

private:
	MapType * m_stlMap;
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CKnightsCapeSet, CRecordset)