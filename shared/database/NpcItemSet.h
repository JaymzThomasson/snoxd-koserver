#pragma once

class CNpcItemSet : public OdbcRecordset
{
public:
	CNpcItemSet(OdbcConnection * dbConnection, NpcItemArray * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetSQL() { return _T("SELECT sIndex, iItem01, sPersent01, iItem02, sPersent02, iItem03, sPersent03, iItem04, sPersent04, iItem05, sPersent05 FROM K_MONSTER_ITEM"); }
	virtual void Fetch()
	{
		_K_MONSTER_ITEM *pData = new _K_MONSTER_ITEM;

		_dbCommand->FetchUInt16(1, pData->sIndex);
		for (int i = 0, j = 2; i < 5; i++)
		{
			_dbCommand->FetchUInt16(j++, pData->iItem[i]);
			_dbCommand->FetchUInt16(j++, pData->sPercent[i]);
		}

		if (!m_pMap->PutData(pData->sIndex, pData))
			delete pData;
	}

	NpcItemArray *m_pMap;
};