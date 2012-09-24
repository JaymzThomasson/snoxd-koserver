#pragma once

// TO-DO: Make this a simple int->CString map
#define T		_SERVER_RESOURCE
#define MapType	ServerResourceArray

class CServerResourceSet : public CMyRecordSet<T>
{
public:
	CServerResourceSet(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 2;
	}

	DECLARE_DYNAMIC(CServerResourceSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[SERVER_RESOURCE]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Int(pFX, _T("[nResourceID]"), m_data.nResourceID);
		RFX_Text(pFX, _T("[strResource]"), m_data.strResource, 100);
	};

	virtual void HandleRead()
	{
		T * data = COPY_ROW();
		if (!m_stlMap->PutData(data->nResourceID, data))
			delete data;
	};

private:
	MapType * m_stlMap;
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CServerResourceSet, CRecordset)