#include "StdAfx.h"

enum
{
	CHALLENGE_PVP_REQUEST	= 1,
	CHALLENGE_PVP_CANCEL	= 2,
	CHALLENGE_PVP_ACCEPT	= 3,
	CHALLENGE_PVP_REJECT	= 4,
	CHALLENGE_PVP_REQ_SENT	= 5,
	CHALLENGE_CVC_REQUEST	= 6,
	CHALLENGE_CVC_CANCEL	= 7,
	CHALLENGE_CVC_ACCEPT	= 8,
	CHALLENGE_CVC_REJECT	= 9,
	CHALLENGE_CVC_REQ_SENT	= 10,
	CHALLENGE_GENERIC_ERROR	= 11,
	CHALLENGE_ZONE_ERROR	= 12,
	CHALLENGE_CLAN_ERROR	= 13
};

// also known as CUser::BattleFields()
void CUser::HandleChallenge(Packet & pkt)
{
	if (isDead())
		return;

	uint8 opcode = pkt.read<uint8>();
	switch (opcode)
	{
	case CHALLENGE_PVP_REQUEST:
		HandleChallengeRequestPVP(pkt);
		break;

	case CHALLENGE_PVP_ACCEPT:
		HandleChallengeAcceptPVP(pkt);
		break;

	case CHALLENGE_CVC_REQUEST:
		HandleChallengeRequestCVC(pkt);
		break;

	case CHALLENGE_CVC_ACCEPT:
		HandleChallengeAcceptCVC(pkt);
		break;

	case CHALLENGE_PVP_CANCEL:
	case CHALLENGE_CVC_CANCEL:
		HandleChallengeCancelled(opcode);
		break;

	case CHALLENGE_PVP_REJECT:
	case CHALLENGE_CVC_REJECT:
		HandleChallengeRejected(opcode);
		break;
	}
}

// This is sent when the challenger requests a PVP duel with a challengee.
void CUser::HandleChallengeRequestPVP(Packet & pkt)
{
	Packet result(WIZ_CHALLENGE);
	string strUserID;
	uint8 bErrorCode = CHALLENGE_GENERIC_ERROR;

	if (m_bRequestingChallenge 
		|| m_bChallengeRequested
		|| GetZoneID() == 48
		|| isInParty()
		|| isTrading()
		|| isMerchanting()
		|| isStoreOpen())
		goto fail_return;

	if (GetZoneID() == 201 || GetZoneID() == 30 || (GetZoneID() / 100) == 1)
	{
		bErrorCode = CHALLENGE_ZONE_ERROR;
		goto fail_return;
	}

	pkt.SByte();
	result.SByte();

	pkt >> strUserID;

	CUser *pUser = g_pMain.GetUserPtr(strUserID.c_str(), TYPE_CHARACTER);
	if (pUser == NULL
		|| !pUser->isInGame()
		|| pUser->isInParty()
		|| pUser->isDead() 
		|| pUser->m_bRequestingChallenge
		|| pUser->m_bChallengeRequested
		|| pUser->isMerchanting()
		|| pUser->isTrading()
		|| pUser->isStoreOpen()
		|| pUser->GetZoneID() != GetZoneID())
		goto fail_return;

	m_bRequestingChallenge = CHALLENGE_PVP_CANCEL;
	m_sChallengeUser = pUser->GetID();
	pUser->m_bChallengeRequested = CHALLENGE_PVP_REJECT;
	pUser->m_sChallengeUser = GetID();

	result << uint8(CHALLENGE_PVP_REQUEST) << GetName();
	pUser->Send(&result);

	result.clear();
	result << uint8(CHALLENGE_PVP_REQ_SENT) << pUser->GetName();
	Send(&result);
	return;

fail_return:
	result << bErrorCode;
	Send(&result);
}

// This is sent when the challenger requests a clan vs clan battle with a challengee.
void CUser::HandleChallengeRequestCVC(Packet & pkt)
{
	Packet result(WIZ_CHALLENGE);
	string strUserID;
	uint8 bErrorCode = CHALLENGE_GENERIC_ERROR;

	if (m_bRequestingChallenge 
		|| m_bChallengeRequested
		|| GetZoneID() == 48
		|| isInParty()
		|| isTrading()
		|| isMerchanting()
		|| isStoreOpen())
		goto fail_return;

	// Are we entitled to speak for the clan?
	if (!isClanLeader())
	{
		bErrorCode = CHALLENGE_CLAN_ERROR;
		goto fail_return;
	}

	if (GetZoneID() == 201 || GetZoneID() == 30 || (GetZoneID() / 100) == 1)
	{
		bErrorCode = CHALLENGE_ZONE_ERROR;
		goto fail_return;
	}

	pkt.SByte();
	result.SByte();

	pkt >> strUserID;

	CUser *pUser = g_pMain.GetUserPtr(strUserID.c_str(), TYPE_CHARACTER);
	if (pUser == NULL
		|| !pUser->isInGame()
		|| pUser->isInParty()
		|| pUser->isDead() 
		|| pUser->m_bRequestingChallenge
		|| pUser->m_bChallengeRequested
		|| pUser->isMerchanting()
		|| pUser->isTrading()
		|| pUser->isStoreOpen()
		|| pUser->GetZoneID() != GetZoneID())
		goto fail_return;

	// Is the target entitled to speak for the clan?
	if (!pUser->isClanLeader())
	{
		bErrorCode = CHALLENGE_CLAN_ERROR; // might need to be set to CHALLENGE_ZONE_ERROR (it's set to that in 1.298)
		goto fail_return;
	}

	m_bRequestingChallenge = CHALLENGE_CVC_CANCEL;
	m_sChallengeUser = pUser->GetID();
	pUser->m_bChallengeRequested = CHALLENGE_CVC_REJECT;
	pUser->m_sChallengeUser = GetID();

	result << uint8(CHALLENGE_CVC_REQUEST) << GetName();
	pUser->Send(&result);

	result.clear();
	result << uint8(CHALLENGE_CVC_REQ_SENT) << pUser->GetName();
	Send(&result);
	return;

fail_return:
	result << bErrorCode;
	Send(&result);
}

// This is sent when the challengee accepts a challenger's PVP request
void CUser::HandleChallengeAcceptPVP(Packet & pkt)
{
	if (!m_bChallengeRequested)
		return;

	CUser *pUser = g_pMain.GetUserPtr(m_sChallengeUser);

	m_sChallengeUser = -1;
	m_bChallengeRequested = 0;

	if (pUser == NULL)
	{
		Packet result(WIZ_CHALLENGE);
		result << uint8(CHALLENGE_GENERIC_ERROR);
		Send(&result);
		return;
	}

	pUser->m_sChallengeUser = -1;
	pUser->m_bRequestingChallenge = 0;

	// Glorious magic numbers!
	ZoneChange(48, 135.0f, 115.0f);
	pUser->ZoneChange(48, 120.0f, 115.0f);
}

// This is sent when the challengee accepts a challenger's clan vs clan request
void CUser::HandleChallengeAcceptCVC(Packet & pkt)
{
	if (!m_bChallengeRequested)
		return;

	Packet result(WIZ_CHALLENGE, uint8(CHALLENGE_GENERIC_ERROR));
	CUser *pUser = g_pMain.GetUserPtr(m_sChallengeUser);

	m_sChallengeUser = -1;
	m_bChallengeRequested = 0;

	if (pUser == NULL)
	{
		Send(&result);
		return;
	}
	
	pUser->m_sChallengeUser = -1;
	pUser->m_bRequestingChallenge = 0;

	CKnights *pClan1 = g_pMain.GetClanPtr(GetClanID());
	CKnights *pClan2 = g_pMain.GetClanPtr(pUser->GetClanID());

	if (pClan1 == NULL || pClan2 == NULL)
	{
		Send(&result);
		pUser->Send(&result);
		return;
	}

	foreach_array (i, pClan1->m_arKnightsUser)
	{
		_KNIGHTS_USER *p = &pClan1->m_arKnightsUser[i];
		if (!p->byUsed || p->pSession == NULL)
			continue;

		CUser *pClanMember = p->pSession;
		if (pClanMember->GetZoneID() == GetZoneID()
			&& !pClanMember->isStoreOpen()
			&& !pClanMember->isMerchanting()
			&& !pClanMember->isTrading()
			&& !pClanMember->isInParty()
			&& !pClanMember->m_bRequestingChallenge
			&& pClanMember->m_sChallengeUser < 0)
			pClanMember->ZoneChange(48, 128.0f, 125.0f);
	}

	foreach_array (i, pClan2->m_arKnightsUser)
	{
		_KNIGHTS_USER *p = &pClan2->m_arKnightsUser[i];
		if (!p->byUsed || p->pSession == NULL)
			continue;

		CUser *pClanMember = p->pSession;
		if (pClanMember->GetZoneID() == GetZoneID()
			&& !pClanMember->isStoreOpen()
			&& !pClanMember->isMerchanting()
			&& !pClanMember->isTrading()
			&& !pClanMember->isInParty()
			&& !pClanMember->m_bRequestingChallenge
			&& pClanMember->m_sChallengeUser < 0)
			pClanMember->ZoneChange(48, 135.0f, 120.0f);
	}
}

// This is sent when the challenger cancels their request to the challengee.
void CUser::HandleChallengeCancelled(uint8 opcode)
{
	if (!m_bRequestingChallenge)
		return;

	Packet result(WIZ_CHALLENGE);

	CUser *pUser = g_pMain.GetUserPtr(m_sChallengeUser);
	if (pUser == NULL || pUser->m_sChallengeUser != GetID())
	{
		result << uint8(CHALLENGE_GENERIC_ERROR);
		Send(&result);
	}
	else
	{
		pUser->m_sChallengeUser = -1;
		pUser->m_bChallengeRequested = 0;
		result << opcode;
		pUser->Send(&result);
	}

	m_sChallengeUser = -1;
	m_bRequestingChallenge = 0;
}

// This is sent when the challengee rejects the challenger's request.
void CUser::HandleChallengeRejected(uint8 opcode)
{
	if (!m_bChallengeRequested)
		return;

	Packet result(WIZ_CHALLENGE);

	CUser *pUser = g_pMain.GetUserPtr(m_sChallengeUser);
	if (pUser == NULL || pUser->m_sChallengeUser != GetID())
	{
		result << uint8(CHALLENGE_GENERIC_ERROR);
	}
	else
	{
		pUser->m_sChallengeUser = -1;
		pUser->m_bRequestingChallenge = 0;
		result << opcode;
		pUser->Send(&result);
	}

	m_sChallengeUser = -1;
	m_bChallengeRequested = 0;
	Send(&result);
}