#pragma once

class CUserKnightsRankSet : public CUserPersonalRankSet
{
public:
	CUserKnightsRankSet(OdbcConnection * dbConnection, UserRankMap * pMap) 
		: CUserPersonalRankSet(dbConnection, pMap) {}

	virtual tstring GetTableName() { return _T("USER_KNIGHTS_RANK"); }
	virtual tstring GetColumns() { return _T("shIndex, nMoney, strElmoUserID, strKarusUserID"); }
};