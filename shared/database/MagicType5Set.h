#pragma once

class CMagicType5Set : public OdbcRecordset
{
public:
	CMagicType5Set(OdbcConnection * dbConnection, Magictype5Array * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetSQL() { return _T("SELECT iNum, Type, ExpRecover, NeedStone FROM MAGIC_TYPE5"); }
	virtual void Fetch()
	{
		_MAGIC_TYPE5 *pData = new _MAGIC_TYPE5;

		_dbCommand->FetchUInt32(1, pData->iNum);
		_dbCommand->FetchByte(2, pData->bType);
		_dbCommand->FetchByte(3, pData->bExpRecover);
		_dbCommand->FetchUInt16(4, pData->sNeedStone);

		if (!m_pMap->PutData(pData->iNum, pData))
			delete pData;
	}

	Magictype5Array *m_pMap;
};