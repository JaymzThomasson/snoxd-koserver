#pragma once

#define T		_MAGIC_TYPE2
#define MapType	Magictype2Array

class CMagicType2Set : public CMyRecordSet<T>
{
public:
	CMagicType2Set(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 6;
	}

	DECLARE_DYNAMIC(CMagicType2Set)
	virtual CString GetDefaultSQL() { return _T("[dbo].[MAGIC_TYPE2]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Long(pFX, _T("[iNum]"), m_data.iNum);
		RFX_Byte(pFX, _T("[HitType]"), m_data.bHitType);
		RFX_Int(pFX, _T("[HitRate]"), m_data.sHitRate);
		RFX_Int(pFX, _T("[AddDamage]"), m_data.sAddDamage);
		RFX_Int(pFX, _T("[AddRange]"), m_data.sAddRange);
		RFX_Byte(pFX, _T("[NeedArrow]"), m_data.bNeedArrow);
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
IMPLEMENT_DYNAMIC(CMagicType2Set, CRecordset)