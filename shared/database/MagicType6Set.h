#pragma once

#define T		_MAGIC_TYPE6
#define MapType	Magictype6Array

class CMagicType6Set : public CMyRecordSet<T>
{
public:
	CMagicType6Set(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 23;
	}

	DECLARE_DYNAMIC(CMagicType6Set)
	virtual CString GetDefaultSQL() { return _T("[dbo].[MAGIC_TYPE6]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Long(pFX, _T("[iNum]"), m_data.iNum);
		RFX_Int(pFX, _T("[Size]"), m_data.sSize);
		RFX_Int(pFX, _T("[TransformID]"), m_data.sTransformID);
		RFX_Int(pFX, _T("[Duration]"), m_data.sDuration);
		RFX_Int(pFX, _T("[MaxHp]"), m_data.sMaxHp);
		RFX_Int(pFX, _T("[MaxMp]"), m_data.sMaxMp);
		RFX_Byte(pFX, _T("[Speed]"), m_data.bSpeed);
		RFX_Int(pFX, _T("[AttackSpeed]"), m_data.sAttackSpeed);
		RFX_Int(pFX, _T("[TotalHit]"), m_data.sTotalHit);
		RFX_Int(pFX, _T("[TotalAc]"), m_data.sTotalAc);
		RFX_Int(pFX, _T("[TotalHitRate]"), m_data.sTotalHitRate);
		RFX_Int(pFX, _T("[TotalEvasionRate]"), m_data.sTotalEvasionRate);
		RFX_Int(pFX, _T("[TotalFireR]"), m_data.sTotalFireR);
		RFX_Int(pFX, _T("[TotalColdR]"), m_data.sTotalColdR);
		RFX_Int(pFX, _T("[TotalLightningR]"), m_data.sTotalLightningR);
		RFX_Int(pFX, _T("[TotalMagicR]"), m_data.sTotalMagicR);
		RFX_Int(pFX, _T("[TotalDiseaseR]"), m_data.sTotalDiseaseR);
		RFX_Int(pFX, _T("[TotalPoisonR]"), m_data.sTotalPoisonR);
		RFX_Int(pFX, _T("[Class]"), m_data.sClass);
		RFX_Byte(pFX, _T("[UserSkillUse]"), m_data.bUserSkillUse);
		RFX_Byte(pFX, _T("[NeedItem]"), m_data.bNeedItem);
		RFX_Byte(pFX, _T("[SkillSuccessRate]"), m_data.bSkillSuccessRate);
		RFX_Byte(pFX, _T("[MonsterFriendly]"), m_data.bMonsterFriendly);
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
IMPLEMENT_DYNAMIC(CMagicType6Set, CRecordset)