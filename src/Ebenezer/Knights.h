#pragma once

#define MAX_CLAN_USERS		36

class CUser;
struct _KNIGHTS_USER
{
	uint8    byUsed;
	char	strUserName[MAX_ID_SIZE+1];
	CUser	*pSession;
	_KNIGHTS_USER()
	{
		byUsed = 0;
		memset(strUserName, 0, sizeof(strUserName));
		pSession = nullptr;
	};
};

enum ClanTypeFlag
{
	ClanTypeTraining	= 1,
	ClanTypePromoted	= 2,
	ClanTypeAccredited5	= 3,
	ClanTypeAccredited4	= 4,
	ClanTypeAccredited2	= 5,
	ClanTypeAccredited3	= 6,
	ClanTypeAccredited1	= 7,
	ClanTypeRoyal5		= 8,
	ClanTypeRoyal4		= 9,
	ClanTypeRoyal3		= 10,
	ClanTypeRoyal2		= 11,
	ClanTypeRoyal1		= 12
};

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

	void Disband(CUser *pLeader = nullptr);

	void SendChat(const char * format, ...);
	void Send(Packet *pkt);

	virtual ~CKnights();
};
