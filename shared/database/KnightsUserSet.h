#pragma once

class CKnightsUserSet : public OdbcRecordset
{
public:
	CKnightsUserSet(OdbcConnection * dbConnection, void * dummy) 
		: OdbcRecordset(dbConnection) {}

	virtual tstring GetTableName() { return _T("KNIGHTS_USER"); }
	virtual tstring GetColumns() { return _T("sIDNum, strUserID"); }

	virtual bool Fetch()
	{
		uint16 sIDNum;
		std::string strUserID;

		_dbCommand->FetchUInt16(1, sIDNum);
		_dbCommand->FetchString(2, strUserID);

		g_pMain.m_KnightsManager.AddKnightsUser(sIDNum, strUserID.c_str());
		return true;
	}
};