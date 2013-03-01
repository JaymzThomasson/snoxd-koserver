#pragma once

#define T		_USER_RANK
#define MapType	UserRankMap

class CUserPersonalRankSet : public CMyRecordSet<T>
{
public:
	CUserPersonalRankSet(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 4;
	}

	DECLARE_DYNAMIC(CUserPersonalRankSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[USER_PERSONAL_RANK]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Int(pFX, _T("[nRank]"), m_data.nRank);
		RFX_Long(pFX, _T("[nSalary]"), m_data.nSalary);
		RFX_Text(pFX, _T("[strElmoUserID]"), m_data.strElmoUserID, MAX_ID_SIZE + 1); 
		RFX_Text(pFX, _T("[strKarusUserID]"), m_data.strKarusUserID, MAX_ID_SIZE + 1);
	};

	virtual void HandleRead()
	{
		T * data = COPY_ROW();
		
		// Trim first
		TRIM_RIGHT(data->strElmoUserID);
		TRIM_RIGHT(data->strKarusUserID);
		
		std::string strElmoUserID = data->strElmoUserID;
		std::string strKarusUserID = data->strKarusUserID;

		// Convert to uppercase for case insensitive lookups
		STRTOUPPER(strElmoUserID);
		STRTOUPPER(strKarusUserID);

		// We're not going to insert either of them, so ignore this row and avoid a mem leak.
		if (strElmoUserID.empty() && strKarusUserID.empty())
		{
			delete data;
			return;
		}

		if (!strElmoUserID.empty())
			m_stlMap->insert(make_pair(strElmoUserID, data));

		if (!strKarusUserID.empty())
			m_stlMap->insert(make_pair(strKarusUserID, data));
	};

private:
	MapType * m_stlMap;
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CUserPersonalRankSet, CRecordset)