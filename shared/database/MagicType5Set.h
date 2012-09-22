#pragma once

#define T		_MAGIC_TYPE5
#define MapType	Magictype5Array

class CMagicType5Set : public CMyRecordSet<T>
{
public:
	CMagicType5Set(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 4;
	}

	DECLARE_DYNAMIC(CMagicType5Set)
	virtual CString GetDefaultSQL() { return _T("[dbo].[MAGIC_TYPE5]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Long(pFX, _T("[iNum]"), m_data.iNum);
		RFX_Byte(pFX, _T("[Type]"), m_data.bType);
		RFX_Byte(pFX, _T("[ExpRecover]"), m_data.bExpRecover);
		RFX_Int(pFX, _T("[NeedStone]"), m_data.sNeedStone);
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
IMPLEMENT_DYNAMIC(CMagicType5Set, CRecordset)