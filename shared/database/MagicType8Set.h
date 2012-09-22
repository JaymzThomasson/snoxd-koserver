#pragma once

#define T		_MAGIC_TYPE8
#define MapType	Magictype8Array

class CMagicType8Set : public CMyRecordSet<T>
{
public:
	CMagicType8Set(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 5;
	}

	DECLARE_DYNAMIC(CMagicType8Set)
	virtual CString GetDefaultSQL() { return _T("[dbo].[MAGIC_TYPE8]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Long(pFX, _T("[iNum]"), m_data.iNum);
		RFX_Byte(pFX, _T("[Target]"), m_data.bTarget);
		RFX_Int(pFX, _T("[Radius]"), m_data.sRadius);
		RFX_Byte(pFX, _T("[WarpType]"), m_data.bWarpType);
		RFX_Int(pFX, _T("[ExpRecover]"), m_data.sExpRecover);
	};

	virtual void HandleRead()
	{
		T * data = COPY_ROW();
		if (!m_stlMap->PutData(data->iNum, data))
			delete data;
	};

private:
	MapType * m_stlMap;
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CMagicType8Set, CRecordset)