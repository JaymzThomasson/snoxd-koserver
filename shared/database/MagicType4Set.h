#pragma once

#define T		_MAGIC_TYPE4
#define MapType	Magictype4Array

class CMagicType4Set : public CMyRecordSet<T>
{
public:
	CMagicType4Set(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 28;
	}

	DECLARE_DYNAMIC(CMagicType4Set)
	virtual CString GetDefaultSQL() { return _T("[dbo].[MAGIC_TYPE4]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Long(pFX, _T("[iNum]"), m_data.iNum);
		RFX_Byte(pFX, _T("[BuffType]"), m_data.bBuffType);
		RFX_Byte(pFX, _T("[Radius]"), m_data.bRadius);
		RFX_Int(pFX, _T("[Duration]"), m_data.sDuration);
		RFX_Byte(pFX, _T("[AttackSpeed]"), m_data.bAttackSpeed);
		RFX_Byte(pFX, _T("[Speed]"), m_data.bSpeed);
		RFX_Int(pFX, _T("[AC]"), m_data.sAC);
		RFX_Int(pFX, _T("[ACPct]"), m_data.sACPct);
		RFX_Byte(pFX, _T("[Attack]"), m_data.bAttack);
		RFX_Byte(pFX, _T("[MagicAttack]"), m_data.bMagicAttack);
		RFX_Int(pFX, _T("[MaxHP]"), m_data.sMaxHP);
		RFX_Int(pFX, _T("[MaxHPPct]"), m_data.sMaxHPPct);
		RFX_Int(pFX, _T("[MaxMP]"), m_data.sMaxMP);
		RFX_Int(pFX, _T("[MaxMPPct]"), m_data.sMaxMPPct);
		RFX_Byte(pFX, _T("[HitRate]"), m_data.bHitRate);
		RFX_Int(pFX, _T("[AvoidRate]"), m_data.sAvoidRate);
		RFX_Byte(pFX, _T("[Str]"), m_data.bStr);
		RFX_Byte(pFX, _T("[Sta]"), m_data.bSta);
		RFX_Byte(pFX, _T("[Dex]"), m_data.bDex);
		RFX_Byte(pFX, _T("[Intel]"), m_data.bIntel);
		RFX_Byte(pFX, _T("[Cha]"), m_data.bCha);
		RFX_Byte(pFX, _T("[FireR]"), m_data.bFireR);
		RFX_Byte(pFX, _T("[ColdR]"), m_data.bColdR);
		RFX_Byte(pFX, _T("[LightningR]"), m_data.bLightningR);
		RFX_Byte(pFX, _T("[MagicR]"), m_data.bMagicR);
		RFX_Byte(pFX, _T("[DiseaseR]"), m_data.bDiseaseR);
		RFX_Byte(pFX, _T("[PoisonR]"), m_data.bPoisonR);
		RFX_Byte(pFX, _T("[ExpPct]"), m_data.bExpPct);
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
IMPLEMENT_DYNAMIC(CMagicType4Set, CRecordset)