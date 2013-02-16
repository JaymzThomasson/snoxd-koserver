#pragma once

#define T		CKnights
#define MapType	KnightsArray

class CKnightsSet : public CMyRecordSet<T>
{
public:
	CKnightsSet(MapType *stlMap, CDatabase* pDatabase = NULL)
		: CMyRecordSet<T>(pDatabase), m_stlMap(stlMap)
	{
		m_nFields = 17;
	}

	DECLARE_DYNAMIC(CKnightsSet)
	virtual CString GetDefaultSQL() { return _T("[dbo].[KNIGHTS]"); };

	virtual void DoFieldExchange(CFieldExchange* pFX)
	{
		pFX->SetFieldType(CFieldExchange::outputColumn);

		RFX_Int(pFX, _T("[IDNum]"), m_data.m_sIndex);
		RFX_Byte(pFX, _T("[Flag]"), m_data.m_byFlag);
		RFX_Byte(pFX, _T("[Nation]"), m_data.m_byNation);
		RFX_Byte(pFX, _T("[Ranking]"), m_data.m_byRanking);
		RFX_Text(pFX, _T("[IDName]"), m_data.m_strName, sizeof(m_data.m_strName));
		RFX_Int(pFX, _T("[Members]"), m_data.m_sMembers);
		RFX_Text(pFX, _T("[Chief]"), m_data.m_strChief, sizeof(m_data.m_strChief));
		RFX_Text(pFX, _T("[ViceChief_1]"), m_data.m_strViceChief_1, sizeof(m_data.m_strViceChief_1));
		RFX_Text(pFX, _T("[ViceChief_2]"), m_data.m_strViceChief_2, sizeof(m_data.m_strViceChief_2));
		RFX_Text(pFX, _T("[ViceChief_3]"), m_data.m_strViceChief_3, sizeof(m_data.m_strViceChief_3));
		RFX_BigInt(pFX, _T("[Gold]"), m_data.m_nMoney);
		RFX_Int(pFX, _T("[Domination]"), m_data.m_sDomination);
		RFX_Long(pFX, _T("[Points]"), m_data.m_nPoints);
		RFX_Binary(pFX, _T("[Mark]"), m_Image, sizeof(m_data.m_Image));	
		RFX_Int(pFX, _T("[sMarkVersion]"), m_data.m_sMarkVersion);
		RFX_Int(pFX, _T("[sMarkLen]"), m_data.m_sMarkLen);
		RFX_Int(pFX, _T("[sCape]"), m_data.m_sCape);
	};


	virtual void HandleRead()
	{
		T * data = COPY_ROW();
		memcpy(data->m_Image, m_Image.GetData(), sizeof(data->m_Image));

		TRIM_RIGHT(data->m_strName);
		TRIM_RIGHT(data->m_strChief);
		TRIM_RIGHT(data->m_strViceChief_1);
		TRIM_RIGHT(data->m_strViceChief_2);
		TRIM_RIGHT(data->m_strViceChief_3);

		if (!m_stlMap->PutData(data->m_sIndex, data))
			delete data;
	};

private:
	MapType * m_stlMap;
	CByteArray m_Image;
};
#undef MapType
#undef T
IMPLEMENT_DYNAMIC(CKnightsSet, CRecordset)