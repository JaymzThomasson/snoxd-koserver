#pragma once

class CKnightsCapeSet : public OdbcRecordset
{
public:
	CKnightsCapeSet(OdbcConnection * dbConnection, KnightsCapeArray * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetSQL() { return _T("SELECT sCapeIndex, nBuyPrice, byGrade, nBuyLoyalty, byRanking FROM KNIGHTS_CAPE"); }
	virtual void Fetch()
	{
		_KNIGHTS_CAPE *pData = new _KNIGHTS_CAPE;

		_dbCommand->FetchUInt16(1, pData->sCapeIndex);
		_dbCommand->FetchUInt32(2, pData->nReqCoins);
		_dbCommand->FetchByte(3, pData->byGrade);
		_dbCommand->FetchUInt32(4, pData->nReqClanPoints);
		_dbCommand->FetchByte(3, pData->byRanking);

		// Convert this from NP to clan points
		pData->nReqClanPoints /= MAX_CLAN_USERS;

		if (!m_pMap->PutData(pData->sCapeIndex, pData))
			delete pData;
	}

	KnightsCapeArray *m_pMap;
};