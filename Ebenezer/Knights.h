#pragma once

#include "define.h"
#include "gamedefine.h"

class CUser;
class CKnights  
{
public:
	uint16	m_sIndex;
	uint8	m_byFlag;			// 1 : Clan, 2 : Knights
	uint8	m_byNation;			// nation
	uint8	m_byGrade;
	uint8	m_byRanking;
	uint16	m_sMembers;

	std::string m_strName;
	std::string m_strChief;
	std::string m_strViceChief_1;
	std::string m_strViceChief_2;
	std::string m_strViceChief_3;

	uint64	m_nMoney;
	uint16	m_sDomination;
	uint32	m_nPoints;
	uint16	m_sMarkVersion, m_sMarkLen;
	char	m_Image[MAX_KNIGHTS_MARK];
	uint16	m_sCape;
	uint8	m_bCapeR, m_bCapeG, m_bCapeB;
	uint16	m_sAlliance;

	_KNIGHTS_USER m_arKnightsUser[MAX_CLAN_USERS];

	CKnights();

	// Attach our session to the clan's list & tell clannies we logged in.
	void OnLogin(CUser *pUser);

	// Detach our session from the clan's list & tell clannies we logged off.
	void OnLogout(CUser *pUser);

	bool AddUser(const char *strUserID);
	bool AddUser(CUser *pUser);
	bool RemoveUser(const char *strUserID);
	bool RemoveUser(CUser *pUser);

	void Disband(CUser *pLeader = NULL);

	void ConstructChatPacket(Packet & data, const char * format, ...);
	void SendChat(const char * format, ...);
	void Send(Packet *pkt);

	virtual ~CKnights();
};
