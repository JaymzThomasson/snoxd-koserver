#if !defined(AFX_ITEMTABLESET_H__9C214CAF_6316_4E33_84A4_8DEBC3D83176__INCLUDED_)
#define AFX_ITEMTABLESET_H__9C214CAF_6316_4E33_84A4_8DEBC3D83176__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ItemTableSet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CItemTableSet recordset

class CItemTableSet : public CRecordset
{
public:
	CItemTableSet(CDatabase* pDatabase = NULL);
	DECLARE_DYNAMIC(CItemTableSet)

// Field/Param Data
	//{{AFX_FIELD(CItemTableSet, CRecordset)
	long	m_Num;
	BYTE	m_Countable;
	//}}AFX_FIELD


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CItemTableSet)
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

#endif // !defined(AFX_ITEMTABLESET_H__9C214CAF_6316_4E33_84A4_8DEBC3D83176__INCLUDED_)
