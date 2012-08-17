#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "Ebenezer.h"
#include "EbenezerDlg.h"
#include "User.h"
#include "PacketDefine.h"

void CUser::VersionCheck(char *pBuf)
{
	int index = 0, send_index = 0;
	char send_buff[128];
	memset( send_buff, NULL, 128 );

	/*
	char strAccountID[MAX_ID_SIZE+1];
	memset(strAccountID, 0x00, MAX_ID_SIZE+1);

	short unk = GetShort(pBuf, index); // -1
	if (!GetKOString(pBuf, strAccountID, MAX_ID_SIZE))
		return;
	*/

	SetByte( send_buff, WIZ_VERSION_CHECK, send_index );
#if __VERSION >= 1800
	SetByte(send_buff, 0, send_index); // unknown
#endif
	SetShort(send_buff, __VERSION, send_index );

	// Cryption
	SetInt64(send_buff, m_Public_key, send_index);
	///~
	SetByte(send_buff, 0, send_index); // 0 = success, 1 = prem error
#if __VERSION < 1700
	SendCompressingPacket(send_buff, send_index);
#else // doesn't seem to bother "compressing" it anymore
	Send(send_buff, send_index);
#endif

	// Cryption
	m_CryptionFlag = 1;
	///~
}

void CUser::LoginProcess(char *pBuf)
{
	int index = 0, send_index = 0, retvalue = 0;

	char accountid[MAX_ID_SIZE+1];
	memset( accountid, NULL, MAX_ID_SIZE+1 );

	char password[MAX_PW_SIZE+1];
	memset( password, NULL, MAX_PW_SIZE+1 );

	char send_buff[256];
	memset( send_buff, NULL, 256);
	CUser* pUser = NULL;
	CTime t = CTime::GetCurrentTime();

	if (!GetKOString(pBuf, accountid, index, MAX_ID_SIZE)
		|| !GetKOString(pBuf, password, index, MAX_PW_SIZE))
		goto fail_return;

	pUser = m_pMain->GetUserPtr( accountid, 0x01 );
	if( pUser && (pUser->GetSocketID() != GetSocketID()) ) {
		pUser->UserDataSaveToAgent();
		pUser->Close();
		goto fail_return;
	}
	
	SetByte( send_buff, WIZ_LOGIN, send_index);
	SetShort( send_buff, m_Sid, send_index);
	SetKOString(send_buff, accountid, send_index);
	SetKOString(send_buff, password, send_index);

	retvalue = m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );
	if (retvalue >= SMQ_FULL) 
	{
		DEBUG_LOG("Login Send Fail : %d", retvalue);
		goto fail_return;
	}

	strcpy( m_strAccountID, accountid );
	return;

fail_return:
	send_index = 0;
	SetByte( send_buff, WIZ_LOGIN, send_index );
	SetByte( send_buff, 0xFF, send_index );		 // 성공시 국가 정보... FF 실패
	Send( send_buff, send_index );
}