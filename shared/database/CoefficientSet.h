#pragma once

#define T		_CLASS_COEFFICIENT
#define MapType	CoefficientArray

class CCoefficientSet : public CMyRecordSet<T>
{
public:
	CCoefficientSet(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 15; 
	}

	DECLARE_DYNAMIC(CCoefficientSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[COEFFICIENT]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Int(pFX, _T("[sClass]"), m_data.sClassNum);
		RFX_Single(pFX, _T("[ShortSword]"), m_data.ShortSword);
		RFX_Single(pFX, _T("[Sword]"), m_data.Sword);
		RFX_Single(pFX, _T("[Axe]"), m_data.Axe);
		RFX_Single(pFX, _T("[Club]"), m_data.Club);
		RFX_Single(pFX, _T("[Spear]"), m_data.Spear);
		RFX_Single(pFX, _T("[Pole]"), m_data.Pole);
		RFX_Single(pFX, _T("[Staff]"), m_data.Staff);
		RFX_Single(pFX, _T("[Bow]"), m_data.Bow);
		RFX_Single(pFX, _T("[Hp]"), m_data.HP);
		RFX_Single(pFX, _T("[Mp]"), m_data.MP);
		RFX_Single(pFX, _T("[Sp]"), m_data.SP);
		RFX_Single(pFX, _T("[Ac]"), m_data.AC);
		RFX_Single(pFX, _T("[Hitrate]"), m_data.Hitrate);
		RFX_Single(pFX, _T("[Evasionrate]"), m_data.Evasionrate);
	};

	virtual void HandleRead()
	{
		T * data = COPY_ROW();
		if (!m_stlMap->PutData(data->sClassNum, data))
			delete data;
	};

private:
	MapType * m_stlMap;
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CCoefficientSet, CRecordset)