#pragma once

#define T		CGameEvent
#define MapType	EventArray

class CEventSet : public CMyRecordSet<T>
{
public:
	CEventSet(MapType *stlMap, BYTE bZone, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 13; 
		/*
			TO-DO: Replace this system with a SINGLE list that's split up into each
				   zone, as opposed to loading the entire table for every zone.
		*/
		m_bZoneID = bZone;
	}

	DECLARE_DYNAMIC(CEventSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[EVENT]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);
	
		RFX_Byte(pFX, _T("[ZoneNum]"), m_bRowZoneID);
		RFX_Int(pFX, _T("[EventNum]"), m_data.m_sIndex);
		RFX_Byte(pFX, _T("[Type]"), m_data.m_bType);
		RFX_Text(pFX, _T("[Cond1]"), m_Cond[0]);
		RFX_Text(pFX, _T("[Cond2]"), m_Cond[1]);
		RFX_Text(pFX, _T("[Cond3]"), m_Cond[2]);
		RFX_Text(pFX, _T("[Cond4]"), m_Cond[3]);
		RFX_Text(pFX, _T("[Cond5]"), m_Cond[4]);
		RFX_Text(pFX, _T("[Exec1]"), m_Exec[0]);
		RFX_Text(pFX, _T("[Exec2]"), m_Exec[1]);
		RFX_Text(pFX, _T("[Exec3]"), m_Exec[2]);
		RFX_Text(pFX, _T("[Exec4]"), m_Exec[3]);
		RFX_Text(pFX, _T("[Exec5]"), m_Exec[4]);
	};

	virtual void HandleRead()
	{
		T * data = COPY_ROW();

		if (m_bRowZoneID != m_bZoneID
			|| !m_stlMap->PutData(data->m_sIndex, data))
		{
			delete data;
			return;
		}

		// TO-DO: Get rid of this (need to tweak the database to just use int fields)
		foreach_array(i, m_Cond)
		{
			data->m_iCond[i] = atoi(iValue); 
			data->m_iExec[i] = atoi(m_Exec[i]); 
		}
	};

private:
	MapType * m_stlMap;
	BYTE m_bZoneID, m_bRowZoneID;
	CString m_Cond[5], m_Exec[5]; // TO-DO: Tweak database to just use int fields... as above.
};
IMPLEMENT_DYNAMIC(CEventSet, CRecordset)
#undef MapType
#undef T