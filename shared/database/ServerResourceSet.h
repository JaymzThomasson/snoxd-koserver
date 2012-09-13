#if !defined(AFX_SERVERRESOURCESET_H__2F772D75_7255_43A8_869E_82FA34930974__INCLUDED_)
#define AFX_SERVERRESOURCESET_H__2F772D75_7255_43A8_869E_82FA34930974__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerResourceSet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServerResourceSet recordset

class CServerResourceSet : public CRecordset
{
public:
	CServerResourceSet(CDatabase* pDatabase = NULL);
	DECLARE_DYNAMIC(CServerResourceSet)

// Field/Param Data
	//{{AFX_FIELD(CServerResourceSet, CRecordset)
	int		m_nResourceID;
	CString	m_strResource;
	//}}AFX_FIELD


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerResourceSet)
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

#endif // !defined(AFX_SERVERRESOURCESET_H__2F772D75_7255_43A8_869E_82FA34930974__INCLUDED_)
