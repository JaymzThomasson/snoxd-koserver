// Region.h: interface for the CRegion class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGION_H__F79F27E6_C1E6_4ED5_904A_934AA8163C88__INCLUDED_)
#define AFX_REGION_H__F79F27E6_C1E6_4ED5_904A_934AA8163C88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "STLMap.h"

class CUser;
class CNpc;


typedef CSTLMap <int>			ZoneUserArray;
typedef CSTLMap <int>			ZoneNpcArray;

class CRegion  
{
public:
	ZoneUserArray	m_RegionUserArray;
	ZoneNpcArray	m_RegionNpcArray;
	BYTE	m_byMoving;			// move : 1, not moving : 0
};

#endif // !defined(AFX_REGION_H__F79F27E6_C1E6_4ED5_904A_934AA8163C88__INCLUDED_)
