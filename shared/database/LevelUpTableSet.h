#pragma once

class CLevelUpTableSet : public OdbcRecordset
{
public:
	CLevelUpTableSet(OdbcConnection * dbConnection, LevelUpArray * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetTableName() { return _T("LEVEL_UP"); }
	virtual tstring GetColumns() { return _T("[Level], [Exp]"); }

	virtual void Fetch()
	{
		// TO-DO: This needs to be increased to support bigint
		std::pair<uint8, uint32> pData;

		_dbCommand->FetchByte(1, pData.first);
		_dbCommand->FetchUInt32(2, pData.second);

		m_pMap->insert(pData);
	}

	LevelUpArray *m_pMap;
};