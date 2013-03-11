#pragma once

class CKnightsUserSet : public OdbcRecordset
{
public:
	CKnightsUserSet(OdbcConnection * dbConnection, void * dummy) 
		: OdbcRecordset(dbConnection) {}

	virtual tstring GetSQL() { return _T("SELECT sIDNum, strUserID FROM KNIGHTS_USER"); }
	virtual void Fetch()
	{
		uint16 sIDNum;
		char strUserID[MAX_ID_SIZE+1];

		_dbCommand->FetchUInt16(1, sIDNum);
		_dbCommand->FetchString(2, strUserID, sizeof(strUserID));

		TRIM_RIGHT(strUserID);

		g_pMain->m_KnightsManager.AddKnightsUser(sIDNum, strUserID);
	}
};