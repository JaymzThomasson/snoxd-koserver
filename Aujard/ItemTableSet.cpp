// ItemTableSet.cpp : implementation file
//

#include "stdafx.h"
#include "aujard.h"
#include "ItemTableSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CItemTableSet

IMPLEMENT_DYNAMIC(CItemTableSet, CRecordset)

CItemTableSet::CItemTableSet(CDatabase* pdb)
	: CRecordset(pdb)
{
	//{{AFX_FIELD_INIT(CItemTableSet)
	m_Num = 0;
	m_Countable = 0;
	m_nFields = 2;
	//}}AFX_FIELD_INIT
	m_nDefaultType = snapshot;
}


CString CItemTableSet::GetDefaultConnect()
{
	return _T("ODBC;DSN=KN_Online;UID=knight;PWD=knight");
}

CString CItemTableSet::GetDefaultSQL()
{
	return _T("[dbo].[ITEM]");
}

void CItemTableSet::DoFieldExchange(CFieldExchange* pFX)
{
	//{{AFX_FIELD_MAP(CItemTableSet)
	pFX->SetFieldType(CFieldExchange::outputColumn);
	RFX_Long(pFX, _T("[Num]"), m_Num);
	RFX_Byte(pFX, _T("[Countable]"), m_Countable);
	//}}AFX_FIELD_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CItemTableSet diagnostics

#ifdef _DEBUG
void CItemTableSet::AssertValid() const
{
	CRecordset::AssertValid();
}

void CItemTableSet::Dump(CDumpContext& dc) const
{
	CRecordset::Dump(dc);
}
#endif //_DEBUG
