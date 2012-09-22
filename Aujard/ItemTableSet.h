#pragma once

#define T		_ITEM_TABLE
#define MapType	ItemtableArray

#include "../shared/database/MyRecordSet.h"
class CItemTableSet : public CMyRecordSet<T>
{
public:
	CItemTableSet(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 2;
	}

	DECLARE_DYNAMIC(CItemTableSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[ITEM]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Long(pFX, _T("[Num]"), m_data.m_iNum);
		RFX_Byte(pFX, _T("[Countable]"), m_data.m_bCountable);
	};

	virtual void HandleRead()
	{
		T * data = COPY_ROW();
		if (!m_stlMap->PutData(data->m_iNum, data))
			delete data;
	};

private:
	MapType * m_stlMap;
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CItemTableSet, CRecordset)