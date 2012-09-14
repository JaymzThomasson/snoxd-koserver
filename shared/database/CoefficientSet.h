#if !defined(AFX_COEFFICIENTSET_H__EA091A3F_1163_453C_BD7A_42FC0520C4FD__INCLUDED_)
#define AFX_COEFFICIENTSET_H__EA091A3F_1163_453C_BD7A_42FC0520C4FD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CoefficientSet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCoefficientSet recordset

class CCoefficientSet : public CRecordset
{
public:
	CCoefficientSet(CDatabase* pDatabase = NULL);
	DECLARE_DYNAMIC(CCoefficientSet)

// Field/Param Data
	//{{AFX_FIELD(CCoefficientSet, CRecordset)
	int		m_sClass;
	float	m_ShortSword;
	float	m_Sword;
	float	m_Axe;
	float	m_Club;
	float	m_Spear;
	float	m_Pole;
	float	m_Staff;
	float	m_Bow;
	float	m_Hp;
	float	m_Mp;
	float	m_Sp;
	float	m_Ac;
	float	m_Hitrate;
	float	m_Evasionrate;
	//}}AFX_FIELD


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCoefficientSet)
	public:
	virtual CString GetDefaultConnect();    // Default connection string
	virtual CString GetDefaultSQL();    // Default SQL for Recordset
	virtual void DoFieldExchange(CFieldExchange* pFX);  // RFX support
	//}}AFX_VIRTUAL

// Implementation
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COEFFICIENTSET_H__EA091A3F_1163_453C_BD7A_42FC0520C4FD__INCLUDED_)
