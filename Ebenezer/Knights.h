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

class CEbenezerDlg;
class CUser;

class CKnights  
{
public:
	CEbenezerDlg *m_pMain; // TEMPORARY. Only used for clan lookups.

	int		m_sIndex;
	BYTE    m_byFlag;			// 1 : Clan, 2 : Knights
	BYTE	m_byNation;			// nation
	BYTE	m_byGrade;
	BYTE	m_byRanking;
	char	m_strName[MAX_ID_SIZE+1];
	int		m_sMembers;
	char	m_strChief[MAX_ID_SIZE+1];
	char	m_strViceChief_1[MAX_ID_SIZE+1];
	char	m_strViceChief_2[MAX_ID_SIZE+1];
	char	m_strViceChief_3[MAX_ID_SIZE+1];
	__int64	m_nMoney;
	int     m_sDomination;
	long	m_nPoints;
	BYTE	m_Image[MAX_KNIGHTS_MARK];
	_KNIGHTS_USER m_arKnightsUser[MAX_CLAN_USERS];		// 클랜원의 정보

	CKnights();
	void InitializeValue();

	bool AddUser(char *strUserID);
	bool AddUser(CUser *pUser);
	bool RemoveUser(char *strUserID);
	bool RemoveUser(CUser *pUser);

	void Disband(CUser *pLeader = NULL);

	void ConstructChatPacket(Packet & data, const char * format, ...);
	void SendChat(const char * format, ...);
	void Send(Packet *pkt);

	virtual ~CKnights();
};

#endif // !defined(AFX_KNIGHTS_H__741B63A3_F081_45B0_9918_012D2E88A8BC__INCLUDED_)
