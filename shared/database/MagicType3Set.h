#pragma once

#define T		_MAGIC_TYPE3
#define MapType	Magictype3Array

class CMagicType3Set : public CMyRecordSet<T>
{
public:
	CMagicType3Set(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 9;
	}

	DECLARE_DYNAMIC(CMagicType3Set)
	virtual CString GetDefaultSQL() { return _T("[dbo].[MAGIC_TYPE3]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Long(pFX, _T("[iNum]"), m_data.iNum);
		RFX_Byte(pFX, _T("[Radius]"), m_data.bRadius);
		RFX_Int(pFX, _T("[Angle]"), m_data.sAngle);
		RFX_Byte(pFX, _T("[DirectType]"), m_data.bDirectType);
		RFX_Int(pFX, _T("[FirstDamage]"), m_data.sFirstDamage);
		RFX_Int(pFX, _T("[EndDamage]"), m_data.sEndDamage);
		RFX_Int(pFX, _T("[TimeDamage]"), m_data.sTimeDamage);
		RFX_Byte(pFX, _T("[Duration]"), m_data.bDuration);
		RFX_Byte(pFX, _T("[Attribute]"), m_data.bAttribute);
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
IMPLEMENT_DYNAMIC(CMagicType3Set, CRecordset)