#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"

void CUser::VersionCheck(char *pBuf)
{
	Packet result(WIZ_VERSION_CHECK);
	/*
	char strAccountID[MAX_ID_SIZE+1];
	int index = 0;

	short unk = GetShort(pBuf, index); // -1
	if (!GetKOString(pBuf, strAccountID, MAX_ID_SIZE))
		return;
	*/

	result	<< uint8(0) << uint16(__VERSION) << m_Public_key 
			<< uint8(0); // 0 = success, 1 = prem error
	Send(&result);

	// Enable encryption
	m_CryptionFlag = 1;
}

void CUser::LoginProcess(char *pBuf)
{
	Packet result(WIZ_LOGIN);
	int index = 0;
	char accountid[MAX_ID_SIZE+1], password[MAX_PW_SIZE+1];

	if (!GetKOString(pBuf, accountid, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, password, index, MAX_PW_SIZE))
		goto fail_return;

	CUser *pUser = m_pMain->GetUserPtr(accountid, TYPE_ACCOUNT);
	if (pUser && (pUser->GetSocketID() != GetSocketID()))
	{
		pUser->UserDataSaveToAgent();
		pUser->CloseProcess();
		goto fail_return;
	}

	result << uint16(GetSocketID()) << accountid << password;
	m_pMain->m_LoggerSendQueue.PutData(&result);
	strcpy_s(m_strAccountID, sizeof(m_strAccountID), accountid);
	return;

fail_return:
	result << uint8(-1);
	Send(&result);
}