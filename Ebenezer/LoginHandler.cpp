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

	result	<< uint8(0) << uint16(__VERSION) << m_Public_key 
			<< uint8(0); // 0 = success, 1 = prem error
	Send(&result);

	// Enable encryption
	m_CryptionFlag = 1;
}

void CUser::LoginProcess(Packet & pkt)
{
	Packet result(WIZ_LOGIN);
	std::string strAccountID, strPasswd;
	pkt >> strAccountID >> strPasswd;
	if (strAccountID.empty() || strAccountID.size() > MAX_ID_SIZE
		|| strPasswd.empty() || strPasswd.size() > MAX_PW_SIZE)
		goto fail_return;

	CUser *pUser = m_pMain->GetUserPtr(strAccountID.c_str(), TYPE_ACCOUNT);
	if (pUser && (pUser->GetSocketID() != GetSocketID()))
	{
		pUser->UserDataSaveToAgent();
		pUser->CloseProcess();
		goto fail_return;
	}

	result << uint16(GetSocketID()) << strAccountID << strPasswd;
	m_pMain->m_LoggerSendQueue.PutData(&result);
	m_strAccountID = strAccountID;
	return;

fail_return:
	result << uint8(-1);
	Send(&result);
}