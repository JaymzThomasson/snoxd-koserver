#pragma once

class CUserPersonalRankSet : public OdbcRecordset
{
public:
	CUserPersonalRankSet(OdbcConnection * dbConnection, UserRankMap * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetTableName() { return _T("USER_PERSONAL_RANK"); }
	virtual tstring GetColumns() { return _T("nRank, nSalary, strElmoUserID, strKarusUserID"); }

	virtual bool Fetch()
	{
		_USER_RANK *pData = new _USER_RANK;

		_dbCommand->FetchUInt16(1, pData->nRank);
		_dbCommand->FetchUInt32(2, pData->nSalary);
		_dbCommand->FetchString(3, pData->strElmoUserID, sizeof(pData->strElmoUserID));
		_dbCommand->FetchString(4, pData->strKarusUserID, sizeof(pData->strKarusUserID));

		// Trim first
		TRIM_RIGHT(pData->strElmoUserID);
		TRIM_RIGHT(pData->strKarusUserID);
		
		// Convert keys to uppercase for case insensitive lookups
		std::string strElmoUserID = pData->strElmoUserID;
		std::string strKarusUserID = pData->strKarusUserID;

		STRTOUPPER(strElmoUserID);
		STRTOUPPER(strKarusUserID);

		// We're not going to insert either of them, so ignore this row and avoid a mem leak.
		if (strElmoUserID.empty() && strKarusUserID.empty())
		{
			delete pData;
			return true; // this is normal.
		}

		if (!strElmoUserID.empty())
			m_pMap->insert(make_pair(strElmoUserID, pData));

		if (!strKarusUserID.empty())
			m_pMap->insert(make_pair(strKarusUserID, pData));

		return true;
	}

	UserRankMap *m_pMap;
};