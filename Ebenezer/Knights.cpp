#include "stdafx.h"

#include "Knights.h"
#include "EbenezerDlg.h"
#include "User.h"

using std::string;

CKnights::CKnights()
{
	m_sIndex = 0;
	m_byFlag = 0;			// 1 : Clan, 2 : 기사단
	m_byNation = 0;		// nation
	m_byGrade = 5;			// clan 등급 (1 ~ 5등급)
	m_byRanking = 0;		// clan 등급 (1 ~ 5등)
	m_sMembers = 1;
	memset(&m_Image, 0, sizeof(m_Image));
	m_nMoney = 0;
	m_sDomination = 0;
	m_nPoints = 0;
	m_sCape = -1;
	m_sAlliance = 0;
	m_sMarkLen = 0;
	m_sMarkVersion = 0;
	m_bCapeR = m_bCapeG = m_bCapeB = 0;
}

void CKnights::OnLogin(CUser *pUser)
{
	// TO-DO: Implement login notice here

	// Set the active session for this user
	foreach_array (i, m_arKnightsUser)
	{
		if (!m_arKnightsUser[i].byUsed
			|| _strcmpi(m_arKnightsUser[i].strUserName, pUser->GetName()))
			continue;

		m_arKnightsUser[i].pSession = pUser;
		break;
	}
}

void CKnights::OnLogout(CUser *pUser)
{
	// TO-DO: Implement logout notice here

	// Unset the active session for this user
	foreach_array (i, m_arKnightsUser)
	{
		if (!m_arKnightsUser[i].byUsed
			|| _strcmpi(m_arKnightsUser[i].strUserName, pUser->GetName()))
			continue;

		m_arKnightsUser[i].pSession = NULL;
		break;
	}
}

bool CKnights::AddUser(const char *strUserID)
{
	for (int i = 0; i < MAX_CLAN_USERS; i++)
	{
		if (m_arKnightsUser[i].byUsed == 0)
		{
			m_arKnightsUser[i].byUsed = 1;
			strcpy(m_arKnightsUser[i].strUserName, strUserID);
			m_arKnightsUser[i].pSession = g_pMain->GetUserPtr(strUserID, TYPE_CHARACTER);
			return true;
		}
	}

	return false;
}

bool CKnights::AddUser(CUser *pUser)
{
	if (pUser == NULL
		|| !AddUser(pUser->GetName()))
		return false;

	pUser->SetClanID(m_sIndex);
	pUser->m_bFame = TRAINEE;
	return true;
}

bool CKnights::RemoveUser(const char *strUserID)
{
	for (int i = 0; i < MAX_CLAN_USERS; i++)
	{
		if (m_arKnightsUser[i].byUsed == 0)
			continue;

		if (!_strcmpi(m_arKnightsUser[i].strUserName, strUserID))
		{
			m_arKnightsUser[i].byUsed = 0;
			strcpy(m_arKnightsUser[i].strUserName, "");
			m_arKnightsUser[i].pSession = NULL;
			return true;
		}
	}

	return false;
}

bool CKnights::RemoveUser(CUser *pUser)
{
	if (pUser == NULL)
		return false;

	bool result = RemoveUser(pUser->GetName());
	pUser->SetClanID(0);
	pUser->m_bFame = 0;
	if (!pUser->isClanLeader())
		pUser->SendClanUserStatusUpdate();
	return result;
}

void CKnights::Disband(CUser *pLeader /*= NULL*/)
{
	string clanNotice;
	g_pMain->GetServerResource(m_byFlag == CLAN_TYPE ? IDS_CLAN_DESTROY : IDS_KNIGHTS_DESTROY, 
		&clanNotice, m_strName.c_str());
	SendChat(clanNotice.c_str());

	foreach_array (i, m_arKnightsUser)
	{
		_KNIGHTS_USER *p = &m_arKnightsUser[i];
		if (p->byUsed && p->pSession != NULL)
			RemoveUser(p->pSession);
	}
	g_pMain->m_KnightsArray.DeleteData(m_sIndex);

	if (pLeader == NULL)
		return;

	Packet result(WIZ_KNIGHTS_PROCESS, uint8(KNIGHTS_DESTROY));
	result << uint8(1);
	pLeader->Send(&result);
}

void CKnights::SendChat(const char * format, ...)
{
	char buffer[128];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buffer, 128, format, ap);
	va_end(ap);

	Packet result;
	ChatPacket::Construct(&result, KNIGHTS_CHAT, buffer);
	Send(&result);
}

void CKnights::Send(Packet *pkt)
{
	foreach_array (i, m_arKnightsUser)
	{
		_KNIGHTS_USER *p = &m_arKnightsUser[i];
		if (p->byUsed && p->pSession != NULL)
			p->pSession->Send(pkt);
	}
}

CKnights::~CKnights()
{
}
