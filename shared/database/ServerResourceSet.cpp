// ServerResourceSet.cpp : implementation file
//

#include "stdafx.h"
#include "ServerResourceSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerResourceSet

IMPLEMENT_DYNAMIC(CServerResourceSet, CRecordset)

CServerResourceSet::CServerResourceSet(CDatabase* pdb)
	: CRecordset(pdb)
{
	//{{AFX_FIELD_INIT(CServerResourceSet)
	m_nResourceID = 0;
	m_strResource = _T("");
	// dungeon work
	m_nFields = 2;
	//}}AFX_FIELD_INIT
	m_nDefaultType = snapshot;
}

CString CServerResourceSet::GetDefaultConnect()
{
	return _T("ODBC;DSN=KN_Online;UID=knight;PWD=knight");
}

CString CServerResourceSet::GetDefaultSQL()
{
	return _T("[dbo].[SERVER_RESOURCE]");
}

void CServerResourceSet::DoFieldExchange(CFieldExchange* pFX)
{
	//{{AFX_FIELD_MAP(CServerResourceSet)
	pFX->SetFieldType(CFieldExchange::outputColumn);
	RFX_Int(pFX, _T("[nResourceID]"), m_nResourceID);
	RFX_Text(pFX, _T("[strResource]"), m_strResource, 255);
	//}}AFX_FIELD_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CServerResourceSet diagnostics

#ifdef _DEBUG
void CServerResourceSet::AssertValid() const
{
	CRecordset::AssertValid();
}

void CServerResourceSet::Dump(CDumpContext& dc) const
{
	CRecordset::Dump(dc);
}
#endif //_DEBUG
