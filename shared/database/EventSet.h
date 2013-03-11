#pragma once

// TO-DO: This should load the entire table after maps are loaded and assign the events then.
class CEventSet : public OdbcRecordset
{
public:
	CEventSet(OdbcConnection * dbConnection, C3DMap *pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetTableName() { return _T("EVENT"); }
	virtual tstring GetColumns() { return _T("EventNum, Type, Cond1, Cond2, Cond3, Cond4, Cond5, Exec1, Exec2, Exec3, Exec4, Exec5"); }
	virtual tstring GetWhereClause() { return string_format(_T("ZoneNum = %d"), m_pMap->m_nZoneNumber); }

	virtual bool Fetch()
	{
		CGameEvent *pData = new CGameEvent();
		int i = 1;

		_dbCommand->FetchUInt16(i++, pData->m_sIndex);
		_dbCommand->FetchByte(i++, pData->m_bType);

		// TO-DO: Get rid of this (need to tweak the database to just use int fields)
		for (int j = 0; j < 5; j++)
		{
			char tmp[16];
			_dbCommand->FetchString(i++, tmp, sizeof(tmp));
			pData->m_iCond[j] = atoi(tmp);
		}

		for (int j = 0; j < 5; j++)
		{
			char tmp[16];
			_dbCommand->FetchString(i++, tmp, sizeof(tmp));
			pData->m_iExec[j] = atoi(tmp);
		}

		if (!m_pMap->m_EventArray.PutData(pData->m_sIndex, pData))
			delete pData;

		return true;
	}

	C3DMap * m_pMap;
};