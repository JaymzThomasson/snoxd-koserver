#pragma once

#define T		_MAGIC_TABLE
#define MapType	MagictableArray

class CMagicTableSet : public CMyRecordSet<T>
{
public:
	CMagicTableSet(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 20;
	}

	DECLARE_DYNAMIC(CMagicTableSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[MAGIC]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Long(pFX, _T("[MagicNum]"), m_data.iNum);
		RFX_Byte(pFX, _T("[BeforeAction]"), m_data.bBeforeAction);
		RFX_Byte(pFX, _T("[TargetAction]"), m_data.bTargetAction);
		RFX_Byte(pFX, _T("[SelfEffect]"),  m_data.bSelfEffect);
		RFX_Byte(pFX, _T("[FlyingEffect]"), m_data.bFlyingEffect);
		RFX_Int(pFX, _T("[TargetEffect]"), m_data.iTargetEffect);
		RFX_Byte(pFX, _T("[Moral]"), m_data.bMoral);
		RFX_Int(pFX, _T("[SkillLevel]"), m_data.sSkillLevel);
		RFX_Int(pFX, _T("[Skill]"), m_data.sSkill);
		RFX_Int(pFX, _T("[Msp]"), m_data.sMsp);
		RFX_Int(pFX, _T("[HP]"), m_data.sHP);
		RFX_Byte(pFX, _T("[ItemGroup]"), m_data.bItemGroup);
		RFX_Long(pFX, _T("[UseItem]"), m_data.iUseItem);
		RFX_Byte(pFX, _T("[CastTime]"), m_data.bCastTime);
		RFX_Byte(pFX, _T("[ReCastTime]"), m_data.bReCastTime);
		RFX_Byte(pFX, _T("[SuccessRate]"), m_data.bSuccessRate);
		RFX_Byte(pFX, _T("[Type1]"), m_data.bType[0]);
		RFX_Byte(pFX, _T("[Type2]"), m_data.bType[1]);
		RFX_Int(pFX, _T("[Range]"), m_data.sRange);
		RFX_Byte(pFX, _T("[Etc]"), m_data.bEtc);
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
IMPLEMENT_DYNAMIC(CMagicTableSet, CRecordset)