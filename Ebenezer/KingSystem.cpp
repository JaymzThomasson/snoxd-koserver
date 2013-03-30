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

// "Check election day" button at the election NPC
void CKingSystem::ElectionScheduleConfirmation(CUser * pUser, Packet & pkt)
{
	Packet result(WIZ_KING, uint8(KING_ELECTION));
	result << uint8(KING_ELECTION_SCHEDULE);

	switch (m_byImType)
	{
		// No impeachment, send election date.
		case 0:
		{
			// Client expects month as 1,12 (tm_mon is 0,11)
			uint8 byElectionMonth = g_localTime.tm_mon + 1;

			/* When's the next election? */
			// If we've passed the election date, we need next month's election.
			// (NOTE: this is official behaviour; it disregards the month set in the table)
			if (g_localTime.tm_mday > m_byDay)
			{
				// Next month is January? Make it so.
				++byElectionMonth;
				while (byElectionMonth > 12)
					byElectionMonth -= 12;
			}

			result	<< uint8(1) // election type
					<< byElectionMonth 
					<< m_byDay << m_byHour << m_byMinute;
		} break;

		// Last scheduled impeachment?
		case 1:
		{
			result	<< uint8(3)
					<< m_byImMonth 
					<< m_byImDay << m_byImHour << m_byImMinute;
		} break;

		// Next impeachment?
		case 3:
		{
			// This should not be necessary, but will leave.
			uint8 byImpeachmentMonth = m_byImMonth;
			while (byImpeachmentMonth > 12)
				m_byImMonth -= 12;

			result	<< uint8(2)
					<< byImpeachmentMonth
					<< m_byImDay << m_byImHour << m_byImMinute;
		} break;

		default:
			return;
	}

	pUser->Send(&result);
}

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
	Packet result(WIZ_KING, uint8(KING_TAX));
	uint8 bOpcode = pkt.read<uint8>();
	result << bOpcode;

	// If you're not a King, you shouldn't have access to this command.
	if (!pUser->isKing())
	{
		result << int16(-1);
		pUser->Send(&result);
		return;
	}

	switch (bOpcode)
	{
		// Collect King's fund
		case 2:
			break;

		// Lookup the tariff
		case 3:
		{
			result << int16(1) << m_byTerritoryTariff;
			pUser->Send(&result);
		} break;

		// Update the tariff
		case 4:
		{
			uint8 byTerritoryTariff = pkt.read<uint8>();

			// Invalid tariff amount
			if (byTerritoryTariff > 10)
			{
				result << int16(-2);
				pUser->Send(&result);
				return;
			}

			// Update the tariff
			m_byTerritoryTariff = byTerritoryTariff;

			// Let all users in your nation know.
			result << int16(1) << byTerritoryTariff << m_byNation;
			g_pMain->Send_All(&result, NULL, m_byNation);

			// Update the database (TO-DO: Implement the request)
			DatabaseThread::AddRequest(&result);
		} break;

		// King's sceptre
		case 7:
			break;
	}

}

void CKingSystem::KingSpecialEvent(CUser * pUser, Packet & pkt)
{
}