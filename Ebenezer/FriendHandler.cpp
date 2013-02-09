#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"

// From the client
void CUser::FriendProcess(Packet & pkt)
{
	uint8 opcode = pkt.read<uint8>();
	switch (opcode)
	{
		case FRIEND_REQUEST:
			FriendRequest();
			break;
		case FRIEND_REPORT:
			FriendReport(pkt);
			break;
		case FRIEND_ADD:
		case FRIEND_REMOVE:
			FriendModify(pkt);
			break;
	}
}

// Request friend list.
void CUser::FriendRequest()
{
	Packet result(WIZ_FRIEND_PROCESS, uint8(FRIEND_REQUEST));
	result << uint16(GetSocketID());
	m_pMain->m_LoggerSendQueue.PutData(&result);
}

// Add or remove a friend from your list.
void CUser::FriendModify(Packet & pkt)
{
	uint8 opcode;
	std::string strUserID;
	CUser *pUser;
	pkt >> opcode >> strUserID;

	if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE
		|| (opcode == FRIEND_ADD && (pUser = m_pMain->GetUserPtr(strUserID.c_str(), TYPE_CHARACTER)) == NULL))
		return;

	Packet result(WIZ_FRIEND_PROCESS, opcode);
	result << uint16(GetSocketID());
	if (opcode == FRIEND_ADD)
		result << uint16(pUser->GetSocketID());

	result.SByte();
	result << strUserID;
	m_pMain->m_LoggerSendQueue.PutData(&result);
}

// Refresh the status of your friends.
void CUser::FriendReport(Packet & pkt)
{
	Packet result(WIZ_FRIEND_PROCESS, uint8(FRIEND_REPORT));
	uint16 usercount = pkt.read<uint16>();

	if (usercount > MAX_FRIEND_COUNT) 
		return;

	result << usercount;
	for (int i = 0; i < usercount; i++) 
	{
		std::string strUserID;
		int16 sid;

		pkt >> strUserID;
		if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE)
			return; // malformed packet, just ignore it.

		uint8 status = GetFriendStatus(strUserID, sid);
		result << strUserID << sid << status;
	}

	Send(&result);
}

// Retrieves the status (and socket ID) of a character.
BYTE CUser::GetFriendStatus(std::string & charName, int16 & sid)
{
	CUser *pUser;
	if (charName.empty()
		|| (pUser = m_pMain->GetUserPtr(charName.c_str(), TYPE_CHARACTER)) == NULL)
	{
		sid = -1;
		return 0; // user not found
	}

	sid = pUser->GetSocketID();
	if (pUser->isInParty()) // user in party
		return 3;

	return 1; // user not in party
}

// From Aujard
void CUser::RecvFriendProcess(char *pBuf)
{
	int index = 0;
	BYTE subcommand = GetByte(pBuf, index);

	switch (subcommand)
	{
		case FRIEND_REQUEST:
			//FriendReport(pkt); // Hmm, old method was redundant. Need to check this.
			break;
		case FRIEND_ADD:
		case FRIEND_REMOVE:
			RecvFriendModify(pBuf);
			break;
	}
}

void CUser::RecvFriendModify(char *pBuf)
{
	Packet result(WIZ_FRIEND_PROCESS);
	char charName[MAX_ID_SIZE+1];
	int index = 0;
	uint8 opcode = GetByte(pBuf, index),
		 bResult = GetByte(pBuf, index);

	int16 sid = -1;
	if (opcode == FRIEND_ADD)
		sid = GetShort(pBuf, index);

	if (!GetKOString(pBuf, charName, index, MAX_ID_SIZE, sizeof(BYTE)))
		return;

	std::string tmp = charName; // this entire method will be updated soon
	uint8 status = GetFriendStatus(tmp, sid);
	result << opcode << bResult << charName << sid << status;
	Send(&result);
}