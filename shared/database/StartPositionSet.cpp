// StartPositionSet.cpp : implementation file
//

#include "stdafx.h"
#include "StartPositionSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStartPositionSet

IMPLEMENT_DYNAMIC(CStartPositionSet, CRecordset)

CStartPositionSet::CStartPositionSet(CDatabase* pdb)
	: CRecordset(pdb)
{
	//{{AFX_FIELD_INIT(CStartPositionSet)
	m_sZoneID = 0;
	m_sKarusX = 0;
	m_sKarusZ = 0;
	m_sElmoradX = 0;
	m_sElmoradZ = 0;
	m_sKarusGateX = 0;
	m_sKarusGateZ = 0;
	m_sElmoradGateX = 0;
	m_sElmoradGateZ = 0;
	m_bRangeX = 0;
	m_bRangeZ = 0;

	m_nFields = 11;
	//}}AFX_FIELD_INIT
	m_nDefaultType = snapshot;
}


CString CStartPositionSet::GetDefaultConnect()
{
	return _T("ODBC;DSN=KN_Online;UID=knight;PWD=knight");
}

CString CStartPositionSet::GetDefaultSQL()
{
	return _T("[dbo].[START_POSITION]");
}

void CStartPositionSet::DoFieldExchange(CFieldExchange* pFX)
{
	//{{AFX_FIELD_MAP(CStartPositionSet)
	pFX->SetFieldType(CFieldExchange::outputColumn);

	RFX_Int(pFX, _T("[ZoneID]"), m_sZoneID);
	RFX_Int(pFX, _T("[sKarusX]"), m_sKarusX);
	RFX_Int(pFX, _T("[sKarusZ]"), m_sKarusZ);
	RFX_Int(pFX, _T("[sElmoradX]"), m_sElmoradX);
	RFX_Int(pFX, _T("[sElmoradZ]"), m_sElmoradZ);
	RFX_Int(pFX, _T("[sKarusGateX]"), m_sKarusGateX);
	RFX_Int(pFX, _T("[sKarusGateZ]"), m_sKarusGateZ);
	RFX_Int(pFX, _T("[sElmoGateX]"), m_sElmoradGateX);
	RFX_Int(pFX, _T("[sElmoGateZ]"), m_sElmoradGateZ);
	RFX_Byte(pFX, _T("[bRangeX]"), m_bRangeX);
	RFX_Byte(pFX, _T("[bRangeZ]"), m_bRangeZ);
	//}}AFX_FIELD_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CStartPositionSet diagnostics

#ifdef _DEBUG
void CStartPositionSet::AssertValid() const
{
	CRecordset::AssertValid();
}

void CStartPositionSet::Dump(CDumpContext& dc) const
{
	CRecordset::Dump(dc);
}
#endif //_DEBUG
