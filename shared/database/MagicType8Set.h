#pragma once

class CMagicType8Set : public OdbcRecordset
{
public:
	CMagicType8Set(OdbcConnection * dbConnection, Magictype8Array * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetTableName() { return _T("MAGIC_TYPE8"); }
	virtual tstring GetColumns() { return _T("iNum, Target, Radius, WarpType, ExpRecover"); }

	virtual void Fetch()
	{
		_MAGIC_TYPE8 *pData = new _MAGIC_TYPE8;

		_dbCommand->FetchUInt32(1, pData->iNum);
		_dbCommand->FetchByte(2, pData->bTarget);
		_dbCommand->FetchUInt16(2, pData->sRadius);
		_dbCommand->FetchByte(4, pData->bWarpType);
		_dbCommand->FetchUInt16(5, pData->sExpRecover);

		if (!m_pMap->PutData(pData->iNum, pData))
			delete pData;
	}

	Magictype8Array *m_pMap;
};