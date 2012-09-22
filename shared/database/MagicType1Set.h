#pragma once

#define T		_MAGIC_TYPE1
#define MapType	Magictype1Array

class CMagicType1Set : public CMyRecordSet<T>
{
public:
	CMagicType1Set(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 9;
	}

	DECLARE_DYNAMIC(CMagicType1Set)
	virtual CString GetDefaultSQL() { return _T("[dbo].[MAGIC_TYPE1]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Long(pFX, _T("[iNum]"), m_data.iNum);
		RFX_Byte(pFX, _T("[Type]"), m_data.bHitType);
		RFX_Int(pFX, _T("[HitRate]"), m_data.sHitRate);
		RFX_Int(pFX, _T("[Hit]"), m_data.sHit);
		RFX_Byte(pFX, _T("[Delay]"), m_data.bDelay);
		RFX_Byte(pFX, _T("[ComboType]"), m_data.bComboType);
		RFX_Byte(pFX, _T("[ComboCount]"), m_data.bComboCount);
		RFX_Int(pFX, _T("[ComboDamage]"), m_data.sComboDamage);
		RFX_Int(pFX, _T("[Range]"), m_data.sRange);
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
IMPLEMENT_DYNAMIC(CMagicType1Set, CRecordset)