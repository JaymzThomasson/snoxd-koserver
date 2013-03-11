#pragma once

class CMagicType3Set : public OdbcRecordset
{
public:
	CMagicType3Set(OdbcConnection * dbConnection, Magictype3Array * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetSQL() { return _T("SELECT iNum, DirectType, FirstDamage, EndDamage, TimeDamage, Duration, Attribute, Radius, Angle FROM MAGIC_TYPE3"); }
	virtual void Fetch()
	{
		_MAGIC_TYPE3 *pData = new _MAGIC_TYPE3;

		_dbCommand->FetchUInt32(1, pData->iNum);
		_dbCommand->FetchByte(2, pData->bDirectType);
		_dbCommand->FetchUInt16(3, pData->sFirstDamage);
		_dbCommand->FetchUInt16(4, pData->sEndDamage);
		_dbCommand->FetchUInt16(5, pData->sTimeDamage);
		_dbCommand->FetchByte(6, pData->bRadius);
		_dbCommand->FetchByte(7, pData->bDuration);
		_dbCommand->FetchByte(8, pData->bAttribute);

		if (!m_pMap->PutData(pData->iNum, pData))
			delete pData;
	}

	Magictype3Array *m_pMap;
};