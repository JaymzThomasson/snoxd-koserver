#pragma once

class CKingSystemSet : public OdbcRecordset
{
public:
	CKingSystemSet(OdbcConnection * dbConnection, KingSystemArray * pMap) 
		: OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetTableName() { return _T("KING_SYSTEM"); }
	virtual tstring GetColumns() { return _T("byNation, byType, sYear, byMonth, byDay, byHour, byMinute, byImType, sImYear, byImMonth, byImDay, byImHour, byImMinute, byNoahEvent, byNoahEvent_Day, byNoahEvent_Hour, byNoahEvent_Minute, sNoahEvent_Duration, byExpEvent, byExpEvent_Day, byExpEvent_Hour, byExpEvent_Minute, sExpEvent_Duration, nTribute, byTerritoryTariff, nTerritoryTax, nNationalTreasury, strKingName, strImRequestID"); }

	virtual bool Fetch()
	{
		_KING_SYSTEM * pData;
		uint8 byNation;
		uint32 i = 1;

		// Pull the nation first so we can use it as an ID.
		_dbCommand->FetchByte(i++, byNation);

		// Hi, I'll take a map for 2 please.
		if (byNation != KARUS && byNation != ELMORAD)
			return true;

		// Do we have an entry for this nation already?
		pData = m_pMap->GetData(byNation);

		// We don't? Create one.
		if (pData == NULL)
		{
			pData = new _KING_SYSTEM;

			// We don't need to check if it exists, because if it did
			// we wouldn't be here...
			m_pMap->PutData(byNation, pData);
		}

		// Let's set the nation then read the rest of the data from the table.
		pData->byNation = byNation;

		/* Election */
		_dbCommand->FetchByte(i++, pData->byType);
		_dbCommand->FetchUInt16(i++, pData->sYear);
		_dbCommand->FetchByte(i++, pData->byMonth);
		_dbCommand->FetchByte(i++, pData->byDay);
		_dbCommand->FetchByte(i++, pData->byHour);
		_dbCommand->FetchByte(i++, pData->byMinute);

		/* Impeachment */
		_dbCommand->FetchByte(i++, pData->byImType);
		_dbCommand->FetchUInt16(i++, pData->sImYear);
		_dbCommand->FetchByte(i++, pData->byImMonth);
		_dbCommand->FetchByte(i++, pData->byImDay);
		_dbCommand->FetchByte(i++, pData->byImHour);
		_dbCommand->FetchByte(i++, pData->byImMinute);

		/* King events */

		// Noah
		_dbCommand->FetchByte(i++, pData->byNoahEvent);
		_dbCommand->FetchByte(i++, pData->byNoahEvent_Day);
		_dbCommand->FetchByte(i++, pData->byNoahEvent_Hour);
		_dbCommand->FetchByte(i++, pData->byNoahEvent_Minute);
		_dbCommand->FetchUInt16(i++, pData->sNoahEvent_Duration);

		// Experience
		_dbCommand->FetchByte(i++, pData->byExpEvent);
		_dbCommand->FetchByte(i++, pData->byExpEvent_Day);
		_dbCommand->FetchByte(i++, pData->byExpEvent_Hour);
		_dbCommand->FetchByte(i++, pData->byExpEvent_Minute);
		_dbCommand->FetchUInt16(i++, pData->sExpEvent_Duration);

		/* Money, money, money. */
		_dbCommand->FetchUInt32(i++, pData->nTribute);
		_dbCommand->FetchByte(i++, pData->byTerritoryTariff);
		_dbCommand->FetchUInt32(i++, pData->nTerritoryTax);
		_dbCommand->FetchUInt32(i++, pData->nNationalTreasury);

		/* Names are so hard to remember. */
		_dbCommand->FetchString(i++, pData->strKingName);
		_dbCommand->FetchString(i++, pData->strImRequestID);

		return true;
	}

	KingSystemArray * m_pMap;
};