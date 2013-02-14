#pragma once

#define T		_MAGIC_TYPE9
#define MapType	Magictype9Array

class CMagicType9Set : public CMyRecordSet<T>
{
public:
	CMagicType9Set(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 12;
	}

	DECLARE_DYNAMIC(CMagicType9Set)
	virtual CString GetDefaultSQL() { return _T("[dbo].[MAGIC_TYPE9]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Long(pFX, _T("[iNum]"), m_data.iNum);
		RFX_Byte(pFX, _T("[ValidGroup]"), m_data.bValidGroup);
		RFX_Byte(pFX, _T("[NationChange]"), m_data.bNationChange);
		RFX_Int(pFX, _T("[MonsterNum]"), m_data.sMonsterNum);
		RFX_Byte(pFX, _T("[TargetChange]"), m_data.bTargetChange);
		RFX_Byte(pFX, _T("[StateChange]"), m_data.bStateChange);
		RFX_Int(pFX, _T("[Radius]"), m_data.sRadius);
		RFX_Int(pFX, _T("[Hitrate]"), m_data.sHitRate);
		RFX_Int(pFX, _T("[Duration]"), m_data.sDuration);
		RFX_Int(pFX, _T("[AddDamage]"), m_data.sDamage);
		RFX_Int(pFX, _T("[Vision]"), m_data.sVision);
		RFX_Long(pFX, _T("[NeedItem]"), m_data.nNeedItem);
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
IMPLEMENT_DYNAMIC(CMagicType9Set, CRecordset)