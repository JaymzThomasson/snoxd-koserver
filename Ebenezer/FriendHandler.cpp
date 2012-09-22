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
	int send_index = 0;
	char send_buff[4];

	SetByte(send_buff, WIZ_FRIEND_PROCESS, send_index);
	SetByte(send_buff, FRIEND_REQUEST, send_index);
	SetShort(send_buff, m_Sid, send_index);

	int result = m_pMain->m_LoggerSendQueue.PutData(send_buff, send_index);
	if (result >= SMQ_FULL)
		DEBUG_LOG("Failed to send friend list request packet : %d", result);
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
	int index = 0, send_index = 0;
	short usercount = 0, idlen = 0;
	char send_buff[640];

	usercount = GetShort(pBuf, index);	// Get usercount packet.
	if (usercount > MAX_FRIEND_COUNT || usercount < 0) return;
	
	SetByte(send_buff, WIZ_FRIEND_PROCESS, send_index);
	SetByte(send_buff, FRIEND_REPORT, send_index);
	SetShort(send_buff, usercount, send_index);

	for (int i = 0; i < usercount; i++) 
	{
		char charName[MAX_ID_SIZE+1] = "";
		short sid;

		GetKOString(pBuf, charName, index, MAX_ID_SIZE);
		SetKOString(send_buff, charName, send_index);

		BYTE result = GetFriendStatus(charName, sid);
		SetShort(send_buff, sid, send_index);
		SetShort(send_buff, result, send_index);
	}

	Send(send_buff, send_index);
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
	if (pUser->m_sPartyIndex != -1) // user in party
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
			RecvFriendRequest(pBuf+index);
			break;
		case FRIEND_ADD:
		case FRIEND_REMOVE:
			RecvFriendModify(pBuf);
			break;
	}
}

void CUser::RecvFriendRequest(char *pBuf)
{
	char send_buff[(MAX_ID_SIZE * MAX_FRIEND_COUNT + 3) + 2 + 2];
	int index = 0, send_index = 0;
	BYTE count = GetByte(pBuf, index);

	SetByte(send_buff, WIZ_FRIEND_PROCESS, send_index);
	SetByte(send_buff, FRIEND_REQUEST, send_index);
	SetShort(send_buff, count, send_index);

	for (int i = 0; i < count; i++)
	{
		char charName[MAX_ID_SIZE+1] = "";
		if (!GetKOString(pBuf, charName, index, MAX_ID_SIZE, sizeof(BYTE)))
			continue;

		short sid;
		BYTE status = GetFriendStatus(charName, sid);

		SetKOString(send_buff, charName, send_index);
		SetShort(send_buff, status, send_index);
		SetByte(send_buff, status, send_index);
	}
	Send(send_buff, send_index);
}

void CUser::RecvFriendModify(char *pBuf)
{
	char send_buff[7+MAX_ID_SIZE], charName[MAX_ID_SIZE+1] = "";
	int index = 0, send_index = 0;
	BYTE opcode = GetByte(pBuf, index),
		 result = GetByte(pBuf, index);
	short sid = -1;
	if (opcode == FRIEND_ADD)
		sid = GetShort(pBuf, index);

	if (!GetKOString(pBuf, charName, index, MAX_ID_SIZE, sizeof(BYTE)))
		return;

	SetByte(send_buff, WIZ_FRIEND_PROCESS, send_index);
	SetByte(send_buff, opcode, send_index);
	SetByte(send_buff, result, send_index);
	SetKOString(send_buff, charName, send_index);

	BYTE status = GetFriendStatus(charName, sid);
	SetShort(send_buff, sid, send_index);
	SetByte(send_buff, status, send_index);
	
	Send(send_buff, send_index);
}