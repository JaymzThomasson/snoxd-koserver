// VersionSet.cpp : implementation file
//

#include "stdafx.h"
#include "VersionSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVersionSet

IMPLEMENT_DYNAMIC(CVersionSet, CRecordset)

CVersionSet::CVersionSet(CDatabase* pdb)
	: CRecordset(pdb)
{
	//{{AFX_FIELD_INIT(CVersionSet)
	m_sVersion = 0;
	m_sHistoryVersion = 0;
	m_nFields = 2;
	//}}AFX_FIELD_INIT
	m_nDefaultType = snapshot;
}


CString CVersionSet::GetDefaultConnect()
{
	return _T("ODBC;DSN=KN_online;UID=knight;PWD=knight");
}

CString CVersionSet::GetDefaultSQL()
{
	return _T("[dbo].[VERSION]");
}

void CVersionSet::DoFieldExchange(CFieldExchange* pFX)
{
	//{{AFX_FIELD_MAP(CVersionSet)
	pFX->SetFieldType(CFieldExchange::outputColumn);
	RFX_Int(pFX, _T("[sVersion]"), m_sVersion);
	RFX_Int(pFX, _T("[sHistoryVersion]"), m_sHistoryVersion);
	//}}AFX_FIELD_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CVersionSet diagnostics

#ifdef _DEBUG
void CVersionSet::AssertValid() const
{
	CRecordset::AssertValid();
}

void CVersionSet::Dump(CDumpContext& dc) const
{
	CRecordset::Dump(dc);
}
#endif //_DEBUG
