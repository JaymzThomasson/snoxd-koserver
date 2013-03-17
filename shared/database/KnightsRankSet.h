#pragma once

class CKnightsRankSet : public OdbcRecordset
{
public:
	CKnightsRankSet(OdbcConnection * dbConnection, void * dummy) 
		: OdbcRecordset(dbConnection), nKarusCount(0), nElmoCount(0) 
	{
		memset(&strKarusCaptain, 0, sizeof(strKarusCaptain));
		memset(&strElmoCaptain, 0, sizeof(strElmoCaptain));
	}

	virtual tstring GetTableName() { return _T("KNIGHTS_RATING"); }
	virtual tstring GetColumns() { return _T("nRank, shIndex, nPoints"); }
#if 0
	RFX_Long(pFX, _T("[nRank]"), m_nRank);
	RFX_Int(pFX, _T("[shIndex]"), m_shIndex);
	RFX_Text(pFX, _T("[strName]"), m_strName);
	RFX_Long(pFX, _T("[nPoints]"), m_nPoints);
#endif

	virtual bool Fetch()
	{
		uint32 nRank, nPoints;
		uint16 sClanID;
		string strKnightsName;

		_dbCommand->FetchUInt32(1, nRank);
		_dbCommand->FetchUInt16(2, sClanID);
		_dbCommand->FetchUInt32(3, nPoints);

		CKnights *pKnights = g_pMain.GetClanPtr(sClanID);
		if (pKnights == NULL)
			return true;

		if (pKnights->m_byNation == KARUS)
		{
			if (nKarusCount == 5)
				return true;
			
			CUser *pUser = g_pMain.GetUserPtr(pKnights->m_strChief, TYPE_CHARACTER);
			if (pUser == NULL || pUser->GetZoneID() != ZONE_BATTLE)
				return true;

			if (pUser->GetClanID() == sClanID)
			{
				sprintf_s(strKarusCaptain[nKarusCount++], 50, "[%s][%s]", strKnightsName, pUser->GetName());
				pUser->ChangeFame(COMMAND_CAPTAIN);
			}
		}
		else if (pKnights->m_byNation == ELMORAD)
		{
			if (nElmoCount == 5)
				return true;

			CUser *pUser = g_pMain.GetUserPtr(pKnights->m_strChief, TYPE_CHARACTER);
			if (pUser == NULL || pUser->GetZoneID() != ZONE_BATTLE)
				return true;
			if (pUser->GetClanID() == sClanID)
			{
				sprintf_s(strElmoCaptain[nElmoCount++], 50, "[%s][%s]", strKnightsName, pUser->GetName());
				pUser->ChangeFame(COMMAND_CAPTAIN);
			}
		}

		return true;
	}


	char strKarusCaptain[5][50], strElmoCaptain[5][50];	
	uint32 nKarusCount, nElmoCount;
};