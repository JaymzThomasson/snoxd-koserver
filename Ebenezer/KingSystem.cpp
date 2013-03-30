#include "StdAfx.h"
#include "KingSystem.h"
#include "EbenezerDlg.h"
#include "User.h"

CKingSystem::CKingSystem()
{
	m_byNation = 0;

	m_byType = 0;
	m_sYear = 0;
	m_byMonth = m_byDay = m_byHour = m_byMinute = 0;

	m_byImType = 0;
	m_sImYear = 0;
	m_byImMonth = m_byImDay = m_byImHour = m_byImMinute = 0;

	m_byNoahEvent = m_byNoahEvent_Day = m_byNoahEvent_Hour = m_byNoahEvent_Minute = 0;
	m_sNoahEvent_Duration = 0;

	m_byExpEvent = m_byExpEvent_Day = m_byExpEvent_Hour = m_byExpEvent_Minute;
	m_sExpEvent_Duration = 0;

	m_nTribute = 0;
	m_byTerritoryTariff = 0;
	m_nTerritoryTax = m_nNationalTreasury = 0;
}

// Wrapper to lookup the appropriate King system instance.
void CKingSystem::PacketProcess(CUser * pUser, Packet & pkt)
{
	if (pUser == NULL)
		return;

	// ... onwards, to official-like code.
	CKingSystem * pKingSystem = g_pMain->m_KingSystemArray.GetData(pUser->GetNation());
	if (pKingSystem != NULL)
		pKingSystem->KingPacketProcess(pUser, pkt);
}

// The real method to handle packets from the client.
void CKingSystem::KingPacketProcess(CUser * pUser, Packet & pkt)
{
	switch (pkt.read<uint8>())
	{
	case KING_ELECTION:
		ElectionSystem(pUser, pkt);
		break;

	case KING_IMPEACHMENT:
		ImpeachmentSystem(pUser, pkt);
		break;

	case KING_TAX:
		KingTaxSystem(pUser, pkt);
		break;

	case KING_EVENT:
		KingSpecialEvent(pUser, pkt);
	}
}

void CKingSystem::ElectionSystem(CUser * pUser, Packet & pkt)
{
	switch (pkt.read<uint8>())
	{
	case KING_ELECTION_SCHEDULE:
		ElectionScheduleConfirmation(pUser, pkt);
		break;

	case KING_ELECTION_NOMINATE:
		CandidacyRecommend(pUser, pkt);
		break;

	case KING_ELECTION_NOTICE_BOARD:
		CandidacyNoticeBoard(pUser, pkt);
		break;

	case KING_ELECTION_POLL:
		ElectionPoll(pUser, pkt);
		break;

	case KING_ELECTION_RESIGN:
		CandidacyResign(pUser, pkt);
		break;
	}
}

void CKingSystem::ElectionScheduleConfirmation(CUser * pUser, Packet & pkt) {}
void CKingSystem::CandidacyRecommend(CUser * pUser, Packet & pkt) {}
void CKingSystem::CandidacyNoticeBoard(CUser * pUser, Packet & pkt) {}
void CKingSystem::ElectionPoll(CUser * pUser, Packet & pkt) {}
void CKingSystem::CandidacyResign(CUser * pUser, Packet & pkt) {}

void CKingSystem::ImpeachmentSystem(CUser * pUser, Packet & pkt)
{
	switch (pkt.read<uint8>())
	{
	case KING_IMPEACHMENT_REQUEST:
		ImpeachmentRequest(pUser, pkt);
		break;

	case KING_IMPEACHMENT_REQUEST_ELECT:
		ImpeachmentRequestElect(pUser, pkt);
		break;

	case KING_IMPEACHMENT_LIST:
		ImpeachmentList(pUser, pkt);
		break;

	case KING_IMPEACHMENT_ELECT:
		ImpeachmentElect(pUser, pkt);
		break;

	case KING_IMPEACHMENT_REQUEST_UI_OPEN:
		ImpeachmentRequestUiOpen(pUser, pkt);
		break;

	case KING_IMPEACHMENT_ELECTION_UI_OPEN:
		ImpeachmentElectionUiOpen(pUser, pkt);
		break;
	}
}

void CKingSystem::ImpeachmentRequest(CUser * pUser, Packet & pkt) {}
void CKingSystem::ImpeachmentRequestElect(CUser * pUser, Packet & pkt) {}
void CKingSystem::ImpeachmentList(CUser * pUser, Packet & pkt) {}
void CKingSystem::ImpeachmentElect(CUser * pUser, Packet & pkt) {}
void CKingSystem::ImpeachmentRequestUiOpen(CUser * pUser, Packet & pkt) {}
void CKingSystem::ImpeachmentElectionUiOpen(CUser * pUser, Packet & pkt) {}

void CKingSystem::KingTaxSystem(CUser * pUser, Packet & pkt)
{
}

void CKingSystem::KingSpecialEvent(CUser * pUser, Packet & pkt)
{
}