#pragma once

class CMagicType1Set : public OdbcRecordset
{
public:
	CMagicType1Set(OdbcConnection * dbConnection, Magictype1Array * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetSQL() { return _T("SELECT iNum, Type, HitRate, Hit, AddDamage, Delay, ComboType, ComboCount, ComboDamage, Range FROM MAGIC_TYPE1"); }
	virtual void Fetch()
	{
		_MAGIC_TYPE1 *pData = new _MAGIC_TYPE1;

		_dbCommand->FetchUInt32(1, pData->iNum);
		_dbCommand->FetchByte(2, pData->bHitType);
		_dbCommand->FetchUInt16(3, pData->sHitRate);
		_dbCommand->FetchUInt16(4, pData->sHit);
		_dbCommand->FetchByte(5, pData->bDelay);
		_dbCommand->FetchByte(6, pData->bComboType);
		_dbCommand->FetchByte(7, pData->bComboCount);
		_dbCommand->FetchUInt16(8, pData->sComboDamage);
		_dbCommand->FetchUInt16(9, pData->sRange);

		if (!m_pMap->PutData(pData->iNum, pData))
			delete pData;
	}

	Magictype1Array *m_pMap;
};