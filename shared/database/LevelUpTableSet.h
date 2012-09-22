#pragma once

typedef pair<BYTE, long>	LevelUpPair;
#define T		LevelUpPair
#define MapType	LevelUpArray

class CLevelUpTableSet : public CMyRecordSet<T>
{
public:
	CLevelUpTableSet(MapType *pMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_map(pMap)
	{
		m_nFields = 2;
	}

	DECLARE_DYNAMIC(CLevelUpTableSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[LEVEL_UP]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Byte(pFX, _T("[Level]"), m_data.first);
		RFX_Long(pFX, _T("[Exp]"), m_data.second);
	};

	virtual void HandleRead()
	{
		m_map->insert(m_data);
	};

private:
	MapType * m_map;
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CLevelUpTableSet, CRecordset)