#pragma once

class CSetItemTableSet : public OdbcRecordset
{
public:
	CSetItemTableSet(OdbcConnection * dbConnection, SetItemArray * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetTableName() { return _T("SET_ITEM"); }
	virtual tstring GetColumns() { return _T("SetIndex, ACBonus, HPBonus, MPBonus, StrengthBonus, StaminaBonus, DexterityBonus, IntelBonus, CharismaBonus, FlameResistance, GlacierResistance, LightningResistance, PoisonResistance, MagicResistance, CurseResistance"); }

	virtual bool Fetch()
	{
		_SET_ITEM *pData = new _SET_ITEM;
		int i = 1;

		_dbCommand->FetchUInt32(i++, pData->SetIndex);
		_dbCommand->FetchUInt16(i++, pData->ACBonus);
		_dbCommand->FetchUInt16(i++, pData->HPBonus);
		_dbCommand->FetchUInt16(i++, pData->MPBonus);
		_dbCommand->FetchUInt16(i++, pData->StrengthBonus);
		_dbCommand->FetchUInt16(i++, pData->StaminaBonus);
		_dbCommand->FetchUInt16(i++, pData->DexterityBonus);
		_dbCommand->FetchUInt16(i++, pData->IntelBonus);
		_dbCommand->FetchUInt16(i++, pData->CharismaBonus);
		_dbCommand->FetchUInt16(i++, pData->FlameResistance);
		_dbCommand->FetchUInt16(i++, pData->GlacierResistance);
		_dbCommand->FetchUInt16(i++, pData->LightningResistance);
		_dbCommand->FetchUInt16(i++, pData->PoisonResistance);
		_dbCommand->FetchUInt16(i++, pData->MagicResistance);
		_dbCommand->FetchUInt16(i++, pData->CurseResistance);
		
		if (!m_pMap->PutData(pData->SetIndex, pData))
			delete pData;

		return true;
	}

	SetItemArray *m_pMap;
};