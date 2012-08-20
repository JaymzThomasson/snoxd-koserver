#if !defined(AFX_STARTPOSITIONSET_H__496F83FF_F4B8_4DBF_803B_BA8E741D9FDB__INCLUDED_)
#define AFX_STARTPOSITIONSET_H__496F83FF_F4B8_4DBF_803B_BA8E741D9FDB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StartPositionSet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStartPositionSet recordset

class CStartPositionSet : public CRecordset
{
public:
	CStartPositionSet(CDatabase* pDatabase = NULL);
	DECLARE_DYNAMIC(CStartPositionSet)

// Field/Param Data
	//{{AFX_FIELD(CStartPositionSet, CRecordset)
	int   m_sZoneID;
	int   m_sKarusX;
	int   m_sKarusZ;
	int   m_sElmoradX;
	int   m_sElmoradZ;
	int   m_sKarusGateX;
	int   m_sKarusGateZ;
	int   m_sElmoradGateX;
	int   m_sElmoradGateZ;
	BYTE  m_bRangeX;
	BYTE  m_bRangeZ;
	//}}AFX_FIELD


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStartPositionSet)
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

#endif // !defined(AFX_STARTPOSITIONSET_H__496F83FF_F4B8_4DBF_803B_BA8E741D9FDB__INCLUDED_)
