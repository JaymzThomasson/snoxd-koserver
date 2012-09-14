// CoefficientSet.cpp : implementation file
//

#include "stdafx.h"
#include "CoefficientSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCoefficientSet

IMPLEMENT_DYNAMIC(CCoefficientSet, CRecordset)

CCoefficientSet::CCoefficientSet(CDatabase* pdb)
	: CRecordset(pdb)
{
	//{{AFX_FIELD_INIT(CCoefficientSet)
	m_sClass = 0;
	m_ShortSword = 0.0f;
	m_Sword = 0.0f;
	m_Axe = 0.0f;
	m_Club = 0.0f;
	m_Spear = 0.0f;
	m_Pole = 0.0f;
	m_Staff = 0.0f;
	m_Bow = 0.0f;
	m_Hp = 0.0f;
	m_Mp = 0.0f;
	m_Sp = 0.0f;
	m_Ac = 0.0f;
	m_Hitrate = 0.0f;
	m_Evasionrate = 0.0f;
	m_nFields = 15;
	//}}AFX_FIELD_INIT
	m_nDefaultType = snapshot;
}


CString CCoefficientSet::GetDefaultConnect()
{
	return _T("ODBC;DSN=KN_Online;UID=knight;PWD=knight");
}

CString CCoefficientSet::GetDefaultSQL()
{
	return _T("[dbo].[COEFFICIENT]");
}

void CCoefficientSet::DoFieldExchange(CFieldExchange* pFX)
{
	//{{AFX_FIELD_MAP(CCoefficientSet)
	pFX->SetFieldType(CFieldExchange::outputColumn);
	RFX_Int(pFX, _T("[sClass]"), m_sClass);
	RFX_Single(pFX, _T("[ShortSword]"), m_ShortSword);
	RFX_Single(pFX, _T("[Sword]"), m_Sword);
	RFX_Single(pFX, _T("[Axe]"), m_Axe);
	RFX_Single(pFX, _T("[Club]"), m_Club);
	RFX_Single(pFX, _T("[Spear]"), m_Spear);
	RFX_Single(pFX, _T("[Pole]"), m_Pole);
	RFX_Single(pFX, _T("[Staff]"), m_Staff);
	RFX_Single(pFX, _T("[Bow]"), m_Bow);
	RFX_Single(pFX, _T("[Hp]"), m_Hp);
	RFX_Single(pFX, _T("[Mp]"), m_Mp);
	RFX_Single(pFX, _T("[Sp]"), m_Sp);
	RFX_Single(pFX, _T("[Ac]"), m_Ac);
	RFX_Single(pFX, _T("[Hitrate]"), m_Hitrate);
	RFX_Single(pFX, _T("[Evasionrate]"), m_Evasionrate);
	//}}AFX_FIELD_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CCoefficientSet diagnostics

#ifdef _DEBUG
void CCoefficientSet::AssertValid() const
{
	CRecordset::AssertValid();
}

void CCoefficientSet::Dump(CDumpContext& dc) const
{
	CRecordset::Dump(dc);
}
#endif //_DEBUG
