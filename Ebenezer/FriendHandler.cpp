#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"

// From the client
void CUser::FriendProcess(char *pBuf)
{
	int index = 0;
	BYTE subcommand = GetByte( pBuf, index );

	switch (subcommand)
	{
		case FRIEND_REQUEST:
			FriendRequest(pBuf+index);
			break;
		case FRIEND_REPORT:
			FriendReport(pBuf+index);
			break;
		case FRIEND_ADD:
		case FRIEND_REMOVE:
			FriendModify(pBuf);
			break;
	}
}

// Request friend list.
void CUser::FriendRequest(char *pBuf)
{
	Packet result(WIZ_FRIEND_PROCESS, uint8(FRIEND_REQUEST));
	result << uint16(GetSocketID());
	m_pMain->m_LoggerSendQueue.PutData(&result);
}

// Add or remove a friend from your list.
void CUser::FriendModify(char *pBuf)
{
	CUser *pUser = NULL;
	char charName[MAX_ID_SIZE+1];
	int index = 0, send_index = 0;
	char send_buff[6];
	BYTE opcode = GetByte(pBuf, index);

	if (!GetKOString(pBuf, charName, index, MAX_ID_SIZE)
		|| (opcode == FRIEND_ADD && (pUser = m_pMain->GetUserPtr(charName, TYPE_CHARACTER)) == NULL))
		return;

	SetByte(send_buff, WIZ_FRIEND_PROCESS, send_index);
	SetByte(send_buff, opcode, send_index);
	SetShort(send_buff, GetSocketID(), send_index);
	if (opcode == FRIEND_ADD)
		SetShort(send_buff, pUser->GetSocketID(), send_index);
	SetKOString(send_buff, charName, send_index, sizeof(BYTE));

	int result = m_pMain->m_LoggerSendQueue.PutData(send_buff, send_index);
	if (result >= SMQ_FULL)
		DEBUG_LOG("Failed to send friend list modify packet : %d", result);
}

// Refresh the status of your friends.
void CUser::FriendReport(char *pBuf)
{
	Packet result(WIZ_FRIEND_PROCESS, uint8(FRIEND_REPORT));
	int index = 0;
	short usercount = 0, sid;
	char charName[MAX_ID_SIZE+1];

	usercount = GetShort(pBuf, index);
	if (usercount > MAX_FRIEND_COUNT || usercount < 0) 
		return;

	result << usercount;
	for (int i = 0; i < usercount; i++) 
	{
		GetKOString(pBuf, charName, index, MAX_ID_SIZE);
		uint8 status = GetFriendStatus(charName, sid);
		result << charName << sid << status;
	}

	Send(&result);
}

// Retrieves the status (and socket ID) of a character.
BYTE CUser::GetFriendStatus(char * charName, short & sid)
{
	CUser *pUser;
	if (charName[0] == 0
		|| (pUser = m_pMain->GetUserPtr(charName, TYPE_CHARACTER)) == NULL)
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
			FriendReport(pBuf+index); // Hmm, old method was redundant. Need to check this.
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

	short sid = -1;
	if (opcode == FRIEND_ADD)
		sid = GetShort(pBuf, index);

	if (!GetKOString(pBuf, charName, index, MAX_ID_SIZE, sizeof(BYTE)))
		return;

	uint8 status = GetFriendStatus(charName, sid);
	result << opcode << bResult << charName << sid << status;
	Send(&result);
}