#pragma once

class CBattleSet : public OdbcRecordset
{
public:
	CBattleSet(OdbcConnection * dbConnection, uint8 * byOldVictory) 
		: OdbcRecordset(dbConnection), m_byOldVictory(byOldVictory) {}

	virtual tstring GetSQL() { return _T("SELECT byNation FROM BATTLE"); }
	virtual void Fetch() { _dbCommand->FetchByte(1, *m_byOldVictory); }

	uint8 *m_byOldVictory;
};