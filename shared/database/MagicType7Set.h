#pragma once

class CMagicType7Set : public OdbcRecordset
{
public:
	CMagicType7Set(OdbcConnection * dbConnection, Magictype7Array * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetSQL() { return _T("SELECT nIndex, byValidGroup, byNatoinChange, shMonsterNum, byTargetChange, byStateChange, byRadius, shHitrate, shDuration, shDamage, byVisoin, nNeedItem FROM MAGIC_TYPE7"); }
	virtual void Fetch()
	{
		_MAGIC_TYPE7 *pData = new _MAGIC_TYPE7;

		_dbCommand->FetchUInt32(1, pData->iNum);
		_dbCommand->FetchByte(2, pData->bValidGroup);
		_dbCommand->FetchByte(3, pData->bNationChange);
		_dbCommand->FetchUInt16(4, pData->sMonsterNum);
		_dbCommand->FetchByte(5, pData->bTargetChange);
		_dbCommand->FetchByte(6, pData->bStateChange);
		_dbCommand->FetchByte(7, pData->bRadius);
		_dbCommand->FetchUInt16(8, pData->sHitRate);
		_dbCommand->FetchUInt16(9, pData->sDuration);
		_dbCommand->FetchUInt16(10, pData->sDamage);
		_dbCommand->FetchByte(11, pData->bVision);
		_dbCommand->FetchUInt32(12, pData->nNeedItem);

		if (!m_pMap->PutData(pData->iNum, pData))
			delete pData;
	}

	Magictype7Array *m_pMap;
};