#pragma once

#define T		_KNIGHTS_CAPE
#define MapType	KnightsCapeArray

class CKnightsCapeSet : public CMyRecordSet<T>
{
public:
	CKnightsCapeSet(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 4;
	}

	DECLARE_DYNAMIC(CKnightsCapeSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[KNIGHTS_CAPE]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Int(pFX, _T("[sCapeIndex]"), m_data.sCapeIndex);
		RFX_Long(pFX, _T("[nBuyPrice]"), m_data.nBuyPrice);
		RFX_Long(pFX, _T("[nDuration]"), m_data.nDuration);
		RFX_Byte(pFX, _T("[byGrade]"), m_data.byGrade);
	};

	virtual void HandleRead()
	{
		T * data = COPY_ROW();
		if (!m_stlMap->PutData(data->sCapeIndex, data))
			delete data;
	};

private:
	MapType * m_stlMap;
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CKnightsCapeSet, CRecordset)