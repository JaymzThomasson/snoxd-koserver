#pragma once

#define T DummyStorage
class CBattleSet : public CMyRecordSet<T>
{
public:
	CBattleSet(BYTE *byOldVictory, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_byOldVictory(byOldVictory)
	{
		m_nFields = 1; 
	}

	DECLARE_DYNAMIC(CBattleSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[BATTLE]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		//{{AFX_FIELD_MAP(CBattleSet)
		pFX->SetFieldType(CFieldExchange::outputColumn);
		RFX_Byte(pFX, _T("[byNation]"), *m_byOldVictory);	
		//}}AFX_FIELD_MAP
	};

private:
	BYTE * m_byOldVictory;
};
#undef T
IMPLEMENT_DYNAMIC(CBattleSet, CRecordset)