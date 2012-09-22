// Knights.h: interface for the CKnights class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KNIGHTS_H__741B63A3_F081_45B0_9918_012D2E88A8BC__INCLUDED_)
#define AFX_KNIGHTS_H__741B63A3_F081_45B0_9918_012D2E88A8BC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "define.h"
#include "gamedefine.h"

class CKnights  
{
public:
	int		m_sIndex;
	BYTE    m_byFlag;			// 1 : Clan, 2 : Knights
	BYTE	m_byNation;			// nation
	BYTE	m_byGrade;
	BYTE	m_byRanking;
	CString	m_strName;
	int		m_sMembers;
	CString	m_strChief;
	CString m_strViceChief_1;
	CString m_strViceChief_2;
	CString m_strViceChief_3;
	__int64	m_nMoney;
	int     m_sDomination;
	long	m_nPoints;
	BYTE	m_Image[MAX_KNIGHTS_MARK];
	_KNIGHTS_USER m_arKnightsUser[MAX_CLAN];		// 클랜원의 정보

	CKnights();
	virtual ~CKnights();

	void InitializeValue();
};

#endif // !defined(AFX_KNIGHTS_H__741B63A3_F081_45B0_9918_012D2E88A8BC__INCLUDED_)
