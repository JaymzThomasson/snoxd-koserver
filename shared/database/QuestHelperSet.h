#pragma once

class CQuestHelperSet : public OdbcRecordset
{
public:
	CQuestHelperSet(OdbcConnection * dbConnection, QuestHelperArray * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetTableName() { return _T("QUEST_HELPER"); }
	virtual tstring GetColumns() { return _T("nIndex, bMessageType, bLevel, nExp, bClass, bNation, bQuestType, bZone, sNpcId, sEventDataIndex, bEventStatus, nEventTriggerIndex, nEventCompleteIndex, nExchangeIndex, nEventTalkIndex, strLuaFilename"); }

	virtual bool Fetch()
	{
		_QUEST_HELPER *pData = new _QUEST_HELPER;

		_dbCommand->FetchUInt32(1, pData->nIndex);
		_dbCommand->FetchByte  (2, pData->bMessageType);
		_dbCommand->FetchByte  (3, pData->bLevel);
		_dbCommand->FetchUInt32(4, pData->nExp);
		_dbCommand->FetchByte  (5, pData->bClass);
		_dbCommand->FetchByte  (6, pData->bNation);
		_dbCommand->FetchByte  (7, pData->bQuestType);
		_dbCommand->FetchByte  (8, pData->bZone);
		_dbCommand->FetchUInt16(9, pData->sNpcId);
		_dbCommand->FetchUInt16(10, pData->sEventDataIndex);
		_dbCommand->FetchByte  (11, pData->bEventStatus);
		_dbCommand->FetchUInt32(12, pData->nEventTriggerIndex);
		_dbCommand->FetchUInt32(13, pData->nEventCompleteIndex);
		_dbCommand->FetchUInt32(14, pData->nExchangeIndex);
		_dbCommand->FetchUInt32(15, pData->nEventTalkIndex);
		_dbCommand->FetchString(16, pData->strLuaFilename);

		if (!m_pMap->PutData(pData->nIndex, pData))
			delete pData;

		return true;
	}

	QuestHelperArray *m_pMap;
};