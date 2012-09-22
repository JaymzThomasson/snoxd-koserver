// Knights.cpp: implementation of the CKnights class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Knights.h"
#include "User.h"
#include "GameDefine.h"
#include "EbenezerDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CKnights::CKnights()
{
	InitializeValue();
}

CKnights::~CKnights()
{

}

void CKnights::InitializeValue()
{
	m_sIndex = 0;
	m_byFlag = 0;			// 1 : Clan, 2 : 기사단
	m_byNation = 0;		// nation
	m_byGrade = 0;			// clan 등급 (1 ~ 5등급)
	m_byRanking = 0;		// clan 등급 (1 ~ 5등)
	m_sMembers = 1;
	m_strName = m_strChief = m_strViceChief_1 = m_strViceChief_2 = m_strViceChief_3 = _T("");
	memset( m_Image, 0x00, MAX_KNIGHTS_MARK );
	m_nMoney = 0;
	m_sDomination = 0;
	m_nPoints = 0;
	
	for( int i=0; i<MAX_CLAN; i++ )	{
		m_arKnightsUser[i].byUsed = 0;
		m_arKnightsUser[i].strUserName = _T("");
	}
}
