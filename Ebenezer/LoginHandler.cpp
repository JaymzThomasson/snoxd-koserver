#include "StdAfx.h"

void CUser::VersionCheck(Packet & pkt)
{
	Packet result(WIZ_VERSION_CHECK);
	/*
	string strAccountID;
	int16 unk = pkt.read<int16>(); // -1
	pkt >> strAccountID;
	if (strAccountID.empty() || strAccountID.size() > MAX_ID_SIZE)
		return;
	*/

	// because of their sucky encryption method, 0 means it effectively won't be encrypted. 
	// We don't want that happening...
	uint64 publicKey = 0;
	do
	{
		publicKey = (uint64)rand() << 32 | rand();
	} while (publicKey == 0); 

	m_crypto.SetPublicKey(publicKey);
	result	<< uint8(0) << uint16(__VERSION) << m_crypto.GetPublicKey()
			<< uint8(0); // 0 = success, 1 = prem error
	Send(&result);

	// Enable encryption
	EnableCrypto(m_crypto.GetPublicKey());
}

void CUser::LoginProcess(Packet & pkt)
{
	Packet result(WIZ_LOGIN);
	std::string strAccountID, strPasswd;
	pkt >> strAccountID >> strPasswd;
	if (strAccountID.empty() || strAccountID.size() > MAX_ID_SIZE
		|| strPasswd.empty() || strPasswd.size() > MAX_PW_SIZE)
		goto fail_return;

	CUser *pUser = g_pMain->GetUserPtr(strAccountID.c_str(), TYPE_ACCOUNT);
	if (pUser && (pUser->GetSocketID() != GetSocketID()))
	{
		pUser->UserDataSaveToAgent();
		pUser->Disconnect();
		goto fail_return;
	}

	result << GetSocketID() << strAccountID << strPasswd;
	g_pMain->m_LoggerSendQueue.PutData(&result);
	m_strAccountID = strAccountID;
	return;

fail_return:
	result << uint8(-1);
	Send(&result);
}

void CUser::RecvLoginProcess(Packet & pkt)
{
	Packet result(WIZ_LOGIN);
	int8 bResult = pkt.read<int8>();

	// Error? Reset the account ID.
	if (bResult < 0)
		m_strAccountID = "";

	result << bResult;
	Send(&result);
}