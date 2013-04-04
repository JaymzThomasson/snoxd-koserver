#include "StdAfx.h"
#include "KingSystem.h"
#include "EbenezerDlg.h"
#include "User.h"
#include "resource.h"
#include "../shared/DateTime.h"

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

	m_bSentFirstMessage = false;
}

/**
 * @brief	Handles timed events related to the King system.
 */
void CKingSystem::CheckKingTimer()
{
	// Get the current time.
	uint8	bCurMonth = g_localTime.tm_mon + 1,
			bCurDay = g_localTime.tm_mday,
			bCurHour = g_localTime.tm_hour,
			bCurMinute = g_localTime.tm_min;

	// If there's an ongoing coin or XP event...
	if (m_byNoahEvent || m_byExpEvent)
		CheckSpecialEvent();

	switch (m_byType)
	{
	case 0:
	case 7:
		{
			if (bCurMonth == m_byMonth
				&& bCurDay == m_byDay
				&& bCurHour == m_byHour
				&& bCurMinute == m_byMinute)
			{
				m_byType = 1;
				g_pMain->SendFormattedResource(IDS_KING_RECOMMEND_TIME, m_byNation, false);

				// KingNotifyMessage(1, m_byNation, WAR_SYSTEM_CHAT);
				// SendUDP_ElectionStatus(m_byType);
				LoadRecommendList();
			}
		} break;

	case 1:
		{
			if (bCurMonth == m_byMonth
				&& bCurDay == m_byDay
				&& bCurHour == m_byHour
				&& bCurMinute == m_byMinute)
			{
				m_byType = 2;
				g_pMain->SendFormattedResource(IDS_KING_RECOMMEND_FINISH_TIME, m_byNation, false);

				// CheckRecommendList();
				// KingNotifyMessage(2, m_byNation, WAR_SYSTEM_CHAT);
				// SendUDP_ElectionStatus(m_byType);
			}

			if ((bCurMinute % 30) && !m_bSentFirstMessage)
			{
				m_bSentFirstMessage = true;
				g_pMain->SendFormattedResource(IDS_KING_PERIOD_OF_RECOMMEND_MESSAGE, m_byNation, true);

				// KingNotifyMessage(28, m_byNation, PUBLIC_CHAT);
				break; // awkward, but official behaviour.
			}

			m_bSentFirstMessage = false;
		} break;

	case 2:
		{
			if (bCurMonth == m_byMonth
				&& bCurDay == m_byDay
				&& bCurHour == m_byHour
				&& bCurMinute == m_byMinute)
			{
				m_byType = 3;
				g_pMain->SendFormattedResource(IDS_KING_ELECTION_TIME, m_byNation, false);

				// KingNotifyMessage(3, m_byNation, WAR_SYSTEM_CHAT);
				// SendUDP_ElectionStatus(m_byType);
			}
		} break;

	case 3:
		{
			if (bCurMonth == m_byMonth
				&& bCurDay == m_byDay
				&& bCurHour == m_byHour
				&& bCurMinute == m_byMinute)
			{
				m_byType = 6;
				// GetElectionResult();
				return;
			}

			if ((bCurMinute % 30) && !m_bSentFirstMessage)
			{
				m_bSentFirstMessage = true;
				g_pMain->SendFormattedResource(IDS_KING_PERIOD_OF_ELECTION_MESSAGE, m_byNation, true);

				// KingNotifyMessage(29, m_byNation, PUBLIC_CHAT);
				break; // awkward, but official behaviour.
			}

			m_bSentFirstMessage = false;
		} break;
	}

	switch (m_byImType)
	{
	case 1: // 47 hours after the impeachment time, call GetImpeachmentRequestResult()
		{
			DateTime dt(m_sImYear, m_byImMonth, m_byImDay, m_byImHour, m_byImMinute);
			dt.AddHours(47);
			if (bCurMonth == dt.GetMonth()
				&& bCurDay == dt.GetDay()
				&& bCurHour == dt.GetHour()
				&& bCurMinute == dt.GetMinute())
			{
				// GetImpeachmentRequestResult();
			}
		} break;

	case 2: // 2 days (48 hours) after the impeachment time, set the impeachment type to 3 
			// and send IDS_KING_IMPEACHMENT_ELECTION_MESSAGE as WAR_SYSTEM_CHAT
		{
			DateTime dt(m_sImYear, m_byImMonth, m_byImDay, m_byImHour, m_byImMinute);
			dt.AddDays(2);
			if (bCurMonth == dt.GetMonth()
				&& bCurDay == dt.GetDay()
				&& bCurHour == dt.GetHour()
				&& bCurMinute == dt.GetMinute())
			{
				m_byImType = 3;
				g_pMain->SendFormattedResource(IDS_KING_IMPEACHMENT_ELECTION_MESSAGE, m_byNation, false);
			}
		} break;

	case 3: // 3 days (72 hours) after the impeachment time, set the impeachment type to 4 
			// and call GetImpeachmentElectionResult()
		{
			DateTime dt(m_sImYear, m_byImMonth, m_byImDay, m_byImHour, m_byImMinute);
			dt.AddDays(3);
			if (bCurMonth == dt.GetMonth()
				&& bCurDay == dt.GetDay()
				&& bCurHour == dt.GetHour()
				&& bCurMinute == dt.GetMinute())
			{
				m_byImType = 4;
				// GetImpeachmentElectionResult();
			}
		} break;
	}
}

/**
 * @brief	Checks to see if a special (coin/XP) event should end.
 */
void CKingSystem::CheckSpecialEvent()
{
	// Get the current time.
	uint8	bCurDay = g_localTime.tm_mday,
			bCurHour = g_localTime.tm_hour,
			bCurMinute = g_localTime.tm_min;

	int16 sEventExpiry;

	// If there's an exp event ongoing...
	if (m_byExpEvent)
	{
		if (bCurDay == m_byExpEvent_Day)
			sEventExpiry = bCurMinute + 60 * (bCurHour - m_byExpEvent_Hour) - m_byExpEvent_Minute;
		else
			sEventExpiry = bCurMinute + 60 * (bCurHour - m_byExpEvent_Hour + 24) - m_byExpEvent_Minute;

		if (sEventExpiry > m_sExpEvent_Duration)
		{
			m_byExpEvent = 0;
			m_byExpEvent_Day = 0;
			m_byExpEvent_Hour = 0;
			m_byExpEvent_Minute = 0;
			m_sExpEvent_Duration = 0;

			// TO-DO: Tell other servers that the event expired (i.e. via UDP)
			// TO-DO: Update the bonuses on the AI server's side (which we don't have implemented). 
			// TO-DO: Update the KING_SYSTEM table to reset the stored event data there too.

			g_pMain->SendFormattedResource(IDS_KING_EXP_BONUS_EVENT_STOP, m_byNation, false);

			// KingNotifyMessage(IDS_KING_EXP_BONUS_EVENT_STOP, m_byNation, WAR_SYSTEM_CHAT); 
			// 31 translates to a resource ID of 230, other args: 0, 0, 0, 0
		}
	}

	// If there's a coin event ongoing...
	if (m_byNoahEvent)
	{
		if (bCurDay == m_byNoahEvent_Day)
			sEventExpiry = bCurMinute + 60 * (bCurHour - m_byNoahEvent_Hour) - m_byNoahEvent_Minute;
		else
			sEventExpiry = bCurMinute + 60 * (bCurHour - m_byNoahEvent_Hour + 24) - m_byNoahEvent_Minute;

		if (sEventExpiry > m_sNoahEvent_Duration)
		{
			m_byNoahEvent = 0;
			m_byNoahEvent_Day = 0;
			m_byNoahEvent_Hour = 0;
			m_byNoahEvent_Minute = 0;
			m_sNoahEvent_Duration = 0;

			// TO-DO: Tell other servers that the event expired (i.e. via UDP)
			// TO-DO: Update the bonuses on the AI server's side (which we don't have implemented). 
			// TO-DO: Update the KING_SYSTEM table to reset the stored event data there too.
			g_pMain->SendFormattedResource(IDS_KING_NOAH_BONUS_EVENT_STOP, m_byNation, false);

			// KingNotifyMessage(IDS_KING_NOAH_BONUS_EVENT_STOP, m_byNation, WAR_SYSTEM_CHAT);
			// 32 translates to a resource ID of 231, other args: 0, 0, 0, 0
		}
	}
}

/**
 * @brief	Generates a list of the top 10 clan leaders eligible to nominate a King.
 */
void CKingSystem::LoadRecommendList()
{
	FastGuard lock(m_lock);

	m_top10ClanSet.clear();
	for (int i = 1; i <= 10; i++)
	{
		// Lookup the clan ranking #i.
		_KNIGHTS_RATING * pRating = 
			g_pMain->m_KnightsRatingArray[m_byNation].GetData(i);
		CKnights * pKnights = NULL;

		// Ignore this entry if no such clan is ranked #i
		if (pRating == NULL
			// or for whatever reason the clan no longer exists...
			|| (pKnights = g_pMain->GetClanPtr(pRating->sClanID)) == NULL)
			continue;

		// add to our top 10 ranked clan set.
		m_top10ClanSet.insert(pRating->sClanID);
	}
}

/**
 * @brief	This sends the appropriate resource as a notice to the server (or to a particular
 * 			user)
 * 			Beyond initial reversing, this doesn't need to exist -- in fact, not even going to
 * 			use it. It's just a temporary point of reference.
 *
 * @param	nResourceID	Identifier for the resource found in the SERVER_RESOURCE table.
 * @param	byNation   	The nation to send the notice/announcement to.
 * @param	chatType   	The chat type (notice/announcement).
 */
void CKingSystem::KingNotifyMessage(uint32 nResourceID, int byNation, ChatType chatType)
{
	std::string result;
	switch (nResourceID)
	{
	//	Resource ID (SERVER_RESOURCE)						// ID used internally (officially)
	case IDS_KING_RECOMMEND_TIME:							// 1 (none)
	case IDS_KING_RECOMMEND_FINISH_TIME:					// 2 (none)
	case IDS_KING_ELECTION_TIME:							// 3 (none)
	case IDS_KING_IMPEACHMENT_REQUEST_TIME:					// 4 (none)
	case IDS_KING_IMPEACHMENT_ELECTION_TIME:				// 5 (none)
	case IDS_KING_REIGN_TIME:								// 7 (none)
	case IDS_KING_KARUS_PRIZE_EVENT_MESSAGE:				// 11 (awarded %s %d coins)
	case IDS_KING_ELMO_PRIZE_EVENT_MESSAGE:					// 12 (awarded %s %d coins)
	case IDS_KING_KARUS_FUGITIVE_EVENT_MESSAGE_1:			// 13 (awarded %s %d coins -- probably inaccurate though, see below)
	case IDS_KING_ELMO_FUGITIVE_EVENT_MESSAGE_1:			// 14 (awarded %s %d coins -- probably inaccurate though, see below)
	case IDS_KING_FUGITIVE_EVENT_MESSAGE_2:					// 15 (%s killed %s and received %d coins as a reward)
	case IDS_KING_KARUS_WEATHER_FINE_EVENT:					// 16 (none)
	case IDS_KING_KARUS_WEATHER_RAIN_EVENT:					// 17 (none)
	case IDS_KING_KARUS_WEATHER_SNOW_EVENT:					// 18 (none)
	case IDS_KING_ELMO_WEATHER_FINE_EVENT:					// 19 (none)
	case IDS_KING_ELMO_WEATHER_RAIN_EVENT:					// 20 (none)
	case IDS_KING_ELMO_WEATHER_SNOW_EVENT:					// 21 (none)
	case IDS_KING_KARUS_NOAH_BONUS_EVENT:					// 22 (%d%% increased coin rate)
	case IDS_KING_KARUS_EXP_BONUS_EVENT:					// 23 (%d%% increased XP rate)
	case IDS_KING_ELMO_NOAH_BONUS_EVENT:					// 24 (%d%% increased coin rate)
	case IDS_KING_ELMO_EXP_BONUS_EVENT:						// 25 (%d%% increased XP rate)
	case IDS_KING_RECOMMEND_REQUEST_MESSAGE:				// 26 (%s can nominate a King)
	case IDS_KING_CANDIDACY_RECOMMEND_MESSAGE:				// 27 (%s has nominated %s as a King)
	case IDS_KING_PERIOD_OF_RECOMMEND_MESSAGE:				// 28 (none)
	case IDS_KING_PERIOD_OF_ELECTION_MESSAGE:				// 29 (none)
	case IDS_KING_ELECTION_RESULT_MESSAGE:					// 30 (%d%% of the vote was won by %s)
	case IDS_KING_EXP_BONUS_EVENT_STOP:						// 31 (none)
	case IDS_KING_NOAH_BONUS_EVENT_STOP:					// 32 (none)
	case IDS_KING_IMPEACHMENT_REQUEST_MESSAGE:				// 33 (none)
	case IDS_KING_IMPEACHMENT_PASS_MESSAGE:					// 34 (none)
	case IDS_KING_IMPEACHMENT_REJECT_MESSAGE:				// 35 (none)
	case IDS_KING_IMPEACHMENT_ELECTION_MESSAGE:				// 36 (none)
	case IDS_KING_IMPEACHMENT_ELECTION_YES_RESULT_MESSAGE:	// 37 (none)
	case IDS_KING_IMPEACHMENT_ELECTION_NO_RESULT_MESSAGE:	// 38 (none)
		break;
	}
}

/**
 * @brief	Wrapper for the King system's packet handler.
 *
 * @param [in,out]	pUser	If non-null, the user sending the request.
 * @param [in,out]	pkt  	The packet.
 */
void CKingSystem::PacketProcess(CUser * pUser, Packet & pkt)
{
	if (pUser == NULL)
		return;

	// ... onwards, to official-like code.
	CKingSystem * pKingSystem = g_pMain->m_KingSystemArray.GetData(pUser->GetNation());
	if (pKingSystem != NULL)
		pKingSystem->KingPacketProcess(pUser, pkt);
}

/**
 * @brief	The real packet handler for the King system.
 *
 * @param [in,out]	pUser	If non-null, the user sending the packet.
 * @param [in,out]	pkt  	The packet.
 */
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
		break;

	case KING_NATION_INTRO:
		break;
	}
}

/**
 * @brief	Election system.
 *
 * @param [in,out]	pUser	If non-null, the user sending the packet.
 * @param [in,out]	pkt  	The packet.
 */
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

/**
 * @brief	"Check election day" button at the election NPC
 *
 * @param [in,out]	pUser	If non-null, the user sending the packet.
 * @param [in,out]	pkt  	The packet.
 */
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

/**
 * @brief	Handler for candidacy recommendations.
 *
 * @param [in,out]	pUser	If non-null, the user sending the packet.
 * @param [in,out]	pkt  	The packet.
 */
void CKingSystem::CandidacyRecommend(CUser * pUser, Packet & pkt) 
{
	Packet result(WIZ_KING, uint8(KING_ELECTION));
	std::string strUserID;
	pkt.SByte();
	pkt >> strUserID;
	if (strUserID.empty() || strUserID.length() > MAX_ID_SIZE)
		return;

	result << uint8(KING_ELECTION_NOMINATE);

	// Make sure it's nomination time.
	if (m_byType != 1)
	{
		result << int16(-2);
		pUser->Send(&result);
		return;
	}

	FastGuard lock(m_lock);

	// Make sure the user nominating a King is a clan leader
	if (!pUser->isClanLeader()
		// ... of a top 10 clan.
		|| m_top10ClanSet.find(pUser->GetClanID()) == m_top10ClanSet.end())
	{
		result << int16(-3);
		pUser->Send(&result);
		return;
	}

	// Send request to database.
	result << strUserID;
	g_pMain->AddDatabaseRequest(result, pUser);
}

/**
 * @brief	Candidacy notice board system.
 *
 * @param [in,out]	pUser	If non-null, the user sending the packet.
 * @param [in,out]	pkt  	The packet.
 */
void CKingSystem::CandidacyNoticeBoard(CUser * pUser, Packet & pkt)
{
	Packet result(WIZ_KING, uint8(KING_ELECTION));
	uint8 opcode = pkt.read<uint8>();
	bool bSuccess = false;

	result << uint8(KING_ELECTION_NOMINATE) << opcode;

	switch (opcode)
	{
	case 1:
		CandidacyNoticeBoard_Write(pUser, pkt);
		return;

	case 2:
		CandidacyNoticeBoard_Read(pUser, pkt);
		return;

	case 4:
		if (m_byType == 1 || m_byType == 2 || m_byType == 3)
		{
			// TO-DO: Find user in (candidate list?), if not found we can break out of here and error.
			if (1 == 2)
				break;

			// 
			bSuccess = true;
		}
		break;

	case 5:
		if (m_byType == 1 || m_byType == 2 || m_byType == 3)
			bSuccess = true;
		break;


	default: 
		return;
	}

	result << int16(bSuccess ? 1 : -1);
	if (opcode == 4)
		result << bSuccess;

	pUser->Send(&result);
}

void CKingSystem::CandidacyNoticeBoard_Write(CUser * pUser, Packet & pkt)
{
}

void CKingSystem::CandidacyNoticeBoard_Read(CUser * pUser, Packet & pkt)
{
}

void CKingSystem::ElectionPoll(CUser * pUser, Packet & pkt) {}
void CKingSystem::CandidacyResign(CUser * pUser, Packet & pkt) {}

/**
 * @brief	Impeachment system.
 *
 * @param [in,out]	pUser	If non-null, the user sending the packet.
 * @param [in,out]	pkt  	The packet.
 */
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

/**
 * @brief	King tax system.
 *
 * @param [in,out]	pUser	If non-null, the user sending the packet.
 * @param [in,out]	pkt  	The packet.
 */
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

/**
 * @brief	Handles commands accessible to the King.
 *
 * @param [in,out]	pUser	If non-null, the user sending the packet.
 * @param [in,out]	pkt  	The packet.
 */
void CKingSystem::KingSpecialEvent(CUser * pUser, Packet & pkt)
{
	Packet result(WIZ_KING, uint8(KING_EVENT));
	uint8 opcode = pkt.read<uint8>();
	result << opcode;

	if (!pUser->isKing())
	{
		result << int16(-1);
		pUser->Send(&result);
		return;
	}

	switch (opcode)
	{
	case KING_EVENT_NOAH: // Noah event
		{
			FastGuard lock(m_lock);

			uint8 bAmount = pkt.read<uint8>();
			if (bAmount < 1 || bAmount > 3)
				return;

			uint32 nCost = 50000000 * bAmount;
			if (nCost > m_nNationalTreasury)
			{
				result << int16(-3);
				pUser->Send(&result);
				return;
			}

			m_nNationalTreasury -= nCost;

			m_byNoahEvent = bAmount;
			m_byNoahEvent_Day = g_localTime.tm_mday;
			m_byNoahEvent_Hour = g_localTime.tm_hour;
			m_byNoahEvent_Minute = g_localTime.tm_min;

			m_sNoahEvent_Duration = 30; // event expires in 30 minutes

			// %d%% increased coin rate 
			g_pMain->SendFormattedResource(m_byNation == KARUS ? IDS_KING_KARUS_NOAH_BONUS_EVENT : IDS_KING_ELMO_NOAH_BONUS_EVENT,
				m_byNation, false, bAmount);

			// TO-DO: Update other servers via UDP
			// TO-DO: Update the AI server

			// Update the database
			result << m_byNation << bAmount << m_byNoahEvent_Day << m_byNoahEvent_Hour << m_byNoahEvent_Minute << m_sNoahEvent_Duration;
			g_pMain->AddDatabaseRequest(result);
		} break;
		
	case KING_EVENT_EXP: // EXP event
		{
			FastGuard lock(m_lock);

			uint8 bAmount = pkt.read<uint8>();
			if (bAmount != 10 && bAmount != 30 && bAmount != 50)
				return;

			uint32 nCost = 30000000 * bAmount;
			if (nCost > m_nNationalTreasury)
			{
				result << int16(-3);
				pUser->Send(&result);
				return;
			}

			m_nNationalTreasury -= nCost;

			m_byExpEvent = bAmount;
			m_byExpEvent_Day = g_localTime.tm_mday;
			m_byExpEvent_Hour = g_localTime.tm_hour;
			m_byExpEvent_Minute = g_localTime.tm_min;

			m_sExpEvent_Duration = 30; // event expires in 30 minutes

			// %d%% increased coin rate 
			g_pMain->SendFormattedResource(m_byNation == KARUS ? IDS_KING_KARUS_EXP_BONUS_EVENT : IDS_KING_ELMO_EXP_BONUS_EVENT,
				m_byNation, false, bAmount);

			// TO-DO: Update other servers via UDP
			// TO-DO: Update the AI server

			// Update the database
			result << m_byNation << bAmount << m_byExpEvent_Day << m_byExpEvent_Hour << m_byExpEvent_Minute << m_sExpEvent_Duration;
			g_pMain->AddDatabaseRequest(result);
		} break;

	case KING_EVENT_PRIZE:
		{
			FastGuard lock(m_lock);

			uint32 nCoins;
			std::string strUserID;
			pkt.SByte();
			pkt >> nCoins >> strUserID;
			
			// If the user submitted invalid input, chances are 
			// the coins will end up 0. We can safely ignore it.
			if (nCoins == 0)
				return;

			CUser * pTUser = g_pMain->GetUserPtr(strUserID, TYPE_CHARACTER);
			if (pTUser == NULL	// this session check isn't official behaviour
								// as they try to handle offline users -
								// note the 'try' (it doesn't work properly)...
				|| strUserID.empty() || strUserID.length() > MAX_ID_SIZE)
			{
				result << int16(-2);
				pUser->Send(&result);
				return;
			}

			if (nCoins > m_nNationalTreasury)
			{
				result << int16(-4);
				pUser->Send(&result);
				return;
			}

			m_nNationalTreasury -= nCoins;
			pTUser->GoldGain(nCoins);

			// (awarded %s %d coins)
			g_pMain->SendFormattedResource(m_byNation == KARUS ? IDS_KING_KARUS_PRIZE_EVENT_MESSAGE : IDS_KING_ELMO_PRIZE_EVENT_MESSAGE,
				m_byNation, false, pTUser->m_strUserID.c_str(), nCoins);
				
			// TO-DO: Update other servers via UDP

			// Update the database
			result << m_byNation << nCoins << strUserID;
			g_pMain->AddDatabaseRequest(result);

		} break;

	case KING_EVENT_FUGITIVE: // not sure what this is exactly, but it seems to work pretty much the same as the /prize command
		break;

	case KING_EVENT_WEATHER: // Weather
		{
			FastGuard lock(m_lock);

			uint8 bType, bAmount;
			pkt >> bType >> bAmount;
			if (bAmount == 0 || bAmount > 100
				|| bType == 0 || bType > WEATHER_SNOW)
				return;

			if (m_nNationalTreasury < 100000)
			{
				result << int16(-3);
				pUser->Send(&result);
				return;
			}

			m_nNationalTreasury -= 100000;

			g_pMain->m_byKingWeatherEvent = 1;
			g_pMain->m_byKingWeatherEvent_Day = g_localTime.tm_mday;
			g_pMain->m_byKingWeatherEvent_Hour = g_localTime.tm_hour;
			g_pMain->m_byKingWeatherEvent_Minute = g_localTime.tm_min;

			g_pMain->m_nWeather = bType;
			g_pMain->m_nAmount = bAmount;

			g_pMain->UpdateWeather();

			// TO-DO: Update other servers via UDP

			// Get the resource ID, which differs per nation and weather type.
			// This works because they're sequential.
			uint32 nResourceID = 
				(m_byNation == KARUS 
					? IDS_KING_KARUS_WEATHER_FINE_EVENT + (bType-1) 
					: IDS_KING_ELMO_WEATHER_FINE_EVENT  + (bType-1));

			g_pMain->SendFormattedResource(nResourceID, m_byNation, false);
		} break;

	case KING_EVENT_NOTICE: // /royalorder command (King chat)
		{
			std::string strMessage;
			pkt.SByte();
			pkt >> strMessage;
			if (strMessage.empty() || strMessage.length() > 256)
				return;

			result.SByte();
			result << int16(1) << strMessage;
			g_pMain->Send_All(&result, NULL, m_byNation);
		} break;
	}
}