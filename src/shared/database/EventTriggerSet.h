#pragma once

class CEventTriggerSet : public OdbcRecordset
{
public:
	CEventTriggerSet(OdbcConnection * dbConnection, EventTriggerArray * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetTableName() { return _T("EVENT_TRIGGER"); }
	virtual tstring GetColumns() { return _T("nIndex, sNpcID, nTriggerNum"); }

	virtual bool Fetch()
	{
		uint32 nIndex; // trap number, so can only have a maximum value of a byte anyway.
		uint32 nTriggerNum;
		uint16 sNpcID;

		_dbCommand->FetchUInt32(1, nIndex);
		_dbCommand->FetchUInt16(2, sNpcID);
		_dbCommand->FetchUInt32(3, nTriggerNum);

		NpcTrapPair key((uint8)nIndex, sNpcID);

		m_pMap->insert(std::make_pair(key, nTriggerNum));
		return true;
	}

	EventTriggerArray *m_pMap;
};