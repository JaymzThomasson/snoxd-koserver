#pragma once

#define T		_VERSION_INFO
#define MapType	VersionInfoList

#include "MyRecordSet.h"
class CVersionSet : public CMyRecordSet<T>
{
public:
	CVersionSet(MapType *pMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_map(pMap)
	{
		m_nFields = 3;
	}

	DECLARE_DYNAMIC(CVersionSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[VERSION]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Int(pFX, _T("[sVersion]"), m_data.sVersion);
		RFX_Int(pFX, _T("[sHistoryVersion]"), m_data.sHistoryVersion);
		RFX_Text(pFX, _T("[strFileName]"), m_data.strFileName, sizeof(m_data.strFileName));
	};

	virtual void HandleRead()
	{
		T * data = COPY_ROW();
		auto itr = m_map->insert(make_pair(data->strFileName, data));
		if (!itr.second)
			delete data;
	};

private:
	MapType * m_map;
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CVersionSet, CRecordset)