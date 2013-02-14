#pragma once

#define T		_MAGIC_TYPE7
#define MapType	Magictype7Array

class CMagicType7Set : public CMyRecordSet<T>
{
public:
	CMagicType7Set(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 12;
	}

	DECLARE_DYNAMIC(CMagicType7Set)
	virtual CString GetDefaultSQL() { return _T("[dbo].[MAGIC_TYPE7]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Long(pFX, _T("[nIndex]"), m_data.iNum);
		RFX_Byte(pFX, _T("[byValidGroup]"), m_data.bValidGroup);
		RFX_Byte(pFX, _T("[byNatoinChange]"), m_data.bNationChange);
		RFX_Int(pFX, _T("[shMonsterNum]"), m_data.sMonsterNum);
		RFX_Byte(pFX, _T("[byTargetChange]"), m_data.bTargetChange);
		RFX_Byte(pFX, _T("[byStateChange]"), m_data.bStateChange);
		RFX_Byte(pFX, _T("[byRadius]"), m_data.bRadius);
		RFX_Int(pFX, _T("[shHitrate]"), m_data.sHitRate);
		RFX_Int(pFX, _T("[shDuration]"), m_data.sDuration);
		RFX_Int(pFX, _T("[shDamage]"), m_data.sDamage);
		RFX_Byte(pFX, _T("[byVisoin]"), m_data.bVision);
		RFX_Long(pFX, _T("[nNeedItem]"), m_data.nNeedItem);
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
IMPLEMENT_DYNAMIC(CMagicType7Set, CRecordset)