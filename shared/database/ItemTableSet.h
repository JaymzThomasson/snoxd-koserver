#pragma once

#define T		_ITEM_TABLE
#define MapType	ItemtableArray

class CItemTableSet : public CMyRecordSet<T>
{
public:
	CItemTableSet(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 57;
	}

	DECLARE_DYNAMIC(CItemTableSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[ITEM]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Long(pFX, _T("[Num]"), m_data.m_iNum);
		RFX_Byte(pFX, _T("[Kind]"), m_data.m_bKind);
		RFX_Byte(pFX, _T("[Slot]"), m_data.m_bSlot);
		RFX_Byte(pFX, _T("[Race]"), m_data.m_bRace);
		RFX_Byte(pFX, _T("[Class]"), m_data.m_bClass);
		RFX_Int(pFX, _T("[Damage]"), m_data.m_sDamage);
		RFX_Int(pFX, _T("[Delay]"), m_data.m_sDelay);
		RFX_Int(pFX, _T("[Range]"), m_data.m_sRange);
		RFX_Int(pFX, _T("[Weight]"), m_data.m_sWeight);
		RFX_Int(pFX, _T("[Duration]"), m_data.m_sDuration);
		RFX_Long(pFX, _T("[BuyPrice]"), m_data.m_iBuyPrice);
		RFX_Long(pFX, _T("[SellPrice]"), m_data.m_iSellPrice);
		RFX_Int(pFX, _T("[Ac]"), m_data.m_sAc);
		RFX_Byte(pFX, _T("[Countable]"), m_data.m_bCountable);
		RFX_Long(pFX, _T("[Effect1]"), m_data.m_iEffect1);
		RFX_Long(pFX, _T("[Effect2]"), m_data.m_iEffect2);
		RFX_Byte(pFX, _T("[ReqLevel]"), m_data.m_bReqLevel);
		RFX_Byte(pFX, _T("[ReqLevelMax]"), m_data.m_bReqLevelMax);
		RFX_Byte(pFX, _T("[ReqRank]"), m_data.m_bReqRank);
		RFX_Byte(pFX, _T("[ReqTitle]"), m_data.m_bReqTitle);
		RFX_Byte(pFX, _T("[ReqStr]"), m_data.m_bReqStr);
		RFX_Byte(pFX, _T("[ReqSta]"), m_data.m_bReqSta);
		RFX_Byte(pFX, _T("[ReqDex]"), m_data.m_bReqDex);
		RFX_Byte(pFX, _T("[ReqIntel]"), m_data.m_bReqIntel);
		RFX_Byte(pFX, _T("[ReqCha]"), m_data.m_bReqCha);
		RFX_Byte(pFX, _T("[SellingGroup]"), m_data.m_bSellingGroup);
		RFX_Byte(pFX, _T("[ItemType]"), m_data.m_ItemType);
		RFX_Int(pFX, _T("[Hitrate]"), m_data.m_sHitrate);
		RFX_Int(pFX, _T("[Evasionrate]"), m_data.m_sEvarate);
		RFX_Int(pFX, _T("[DaggerAc]"), m_data.m_sDaggerAc);
		RFX_Int(pFX, _T("[SwordAc]"), m_data.m_sSwordAc);
		RFX_Int(pFX, _T("[MaceAc]"), m_data.m_sMaceAc);
		RFX_Int(pFX, _T("[AxeAc]"), m_data.m_sAxeAc);
		RFX_Int(pFX, _T("[SpearAc]"), m_data.m_sSpearAc);
		RFX_Int(pFX, _T("[BowAc]"), m_data.m_sBowAc);
		RFX_Byte(pFX, _T("[FireDamage]"), m_data.m_bFireDamage);
		RFX_Byte(pFX, _T("[IceDamage]"), m_data.m_bIceDamage);
		RFX_Byte(pFX, _T("[LightningDamage]"), m_data.m_bLightningDamage);
		RFX_Byte(pFX, _T("[PoisonDamage]"), m_data.m_bPoisonDamage);
		RFX_Byte(pFX, _T("[HPDrain]"), m_data.m_bHPDrain);
		RFX_Byte(pFX, _T("[MPDamage]"), m_data.m_bMPDamage);
		RFX_Byte(pFX, _T("[MPDrain]"), m_data.m_bMPDrain);
		RFX_Byte(pFX, _T("[MirrorDamage]"), m_data.m_bMirrorDamage);
		RFX_Byte(pFX, _T("[Droprate]"), m_data.m_bDroprate);
		RFX_Int(pFX, _T("[StrB]"), m_data.m_bStrB);
		RFX_Int(pFX, _T("[StaB]"), m_data.m_bStaB);
		RFX_Int(pFX, _T("[DexB]"), m_data.m_bDexB);
		RFX_Int(pFX, _T("[IntelB]"), m_data.m_bIntelB);
		RFX_Int(pFX, _T("[ChaB]"), m_data.m_bChaB);
		RFX_Int(pFX, _T("[MaxHpB]"), m_data.m_MaxHpB);
		RFX_Int(pFX, _T("[MaxMpB]"), m_data.m_MaxMpB);
		RFX_Int(pFX, _T("[FireR]"), m_data.m_bFireR);
		RFX_Int(pFX, _T("[ColdR]"), m_data.m_bColdR);
		RFX_Int(pFX, _T("[LightningR]"), m_data.m_bLightningR);
		RFX_Int(pFX, _T("[MagicR]"), m_data.m_bMagicR);
		RFX_Int(pFX, _T("[PoisonR]"), m_data.m_bPoisonR);
		RFX_Int(pFX, _T("[CurseR]"), m_data.m_bCurseR);
	};

	virtual void HandleRead()
	{
		T * data = COPY_ROW();
		if (!m_stlMap->PutData(data->m_iNum, data))
			delete data;
	};

private:
	MapType * m_stlMap;
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CItemTableSet, CRecordset)