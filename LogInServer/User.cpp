// User.cpp: implementation of the CUser class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "versionmanagerdlg.h"
#include "User.h"

#include <set>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

LSPacketHandler PacketHandlers[NUM_LS_OPCODES];
void InitPacketHandlers(void)
{
	memset(&PacketHandlers, 0, sizeof(LSPacketHandler) * NUM_LS_OPCODES);
	PacketHandlers[LS_VERSION_REQ]			= &CUser::HandleVersion;
	PacketHandlers[LS_DOWNLOADINFO_REQ]		= &CUser::HandlePatches;
	PacketHandlers[LS_LOGIN_REQ]			= &CUser::HandleLogin;
	PacketHandlers[LS_SERVERLIST]			= &CUser::HandleServerlist;
	PacketHandlers[LS_NEWS]					= &CUser::HandleNews;
#if __VERSION >= 1453
	PacketHandlers[LS_CRYPTION]				= &CUser::HandleSetEncryptionPublicKey;
	PacketHandlers[LS_UNKF7]				= &CUser::HandleUnkF7;
#endif
}

void CUser::Initialize()
{
	m_pMain = (CVersionManagerDlg*)AfxGetApp()->GetMainWnd();
	CIOCPSocket2::Initialize();
}

void CUser::Parsing(Packet & pkt)
{
	uint8 opcode = pkt.GetOpcode();

	// Unknown packet
	if (opcode >= NUM_LS_OPCODES
		|| PacketHandlers[opcode] == NULL)
	{
		Close();
		return;
	}

	(this->*PacketHandlers[opcode])(pkt);
}

void CUser::HandleVersion(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	result << uint16(m_pMain->m_nLastVersion);
	Send(&result);
}

void CUser::HandlePatches(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	set<CString> downloadset;

	uint16 version;
	pkt >> version;

	foreach (itr, m_pMain->m_VersionList) 
	{
		_VERSION_INFO *pInfo = itr->second;
		if (pInfo->sVersion > version)
			downloadset.insert(pInfo->strFileName);
	}

	result << m_pMain->m_strFtpUrl << m_pMain->m_strFilePath;
	result << uint16(downloadset.size());
	
	foreach (itr, downloadset)
		result << (*itr);

	Send(&result);
}

void CUser::HandleLogin(Packet & pkt)
{
	enum LoginErrorCode
	{
		AUTH_SUCCESS	= 0x01,
		AUTH_NOT_FOUND	= 0x02,
		AUTH_INVALID	= 0x03,
		AUTH_BANNED		= 0x04,
		AUTH_IN_USE		= 0x05,
		AUTH_ERROR		= 0x06,
		AUTH_FAILED		= 0xFF
	};

	Packet result(pkt.GetOpcode());
	BYTE resultCode = 0;
	string account, password;

	pkt >> account >> password;
	if (account.size() == 0 || account.size() > MAX_ID_SIZE 
		|| password.size() == 0 || password.size() > MAX_PW_SIZE)
		resultCode = uint8(AUTH_NOT_FOUND); 
	else
		resultCode = m_pMain->m_DBProcess.AccountLogin(account.c_str(), password.c_str());

	result << resultCode;
	if (resultCode == AUTH_SUCCESS)
	{
		result << int8(-1) << int8(-1); // prem type, prem days/hours (what are we even with 19XX)
		result << account; // it uses this for the game server now.
	}
	Send(&result);
}

void CUser::HandleServerlist(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	m_pMain->m_DBProcess.LoadUserCountList();

#if __VERSION >= 1500
	uint16 echo;
	pkt >> echo;
	result << echo;
#endif

	result << uint8(m_pMain->m_nServerCount);
	foreach (itr, m_pMain->m_ServerList) 
	{		
		_SERVER_INFO *pServer = *itr;

		result << pServer->strServerIP;
#if __VERSION >= 1890
		result << pServer->strLanIP;
#endif
		result << pServer->strServerName;

		if (pServer->sUserCount <= pServer->sPlayerCap)
			result << pServer->sUserCount;
		else
			result << int16(-1);
#if __VERSION >= 1453
		result << pServer->sServerID << pServer->sGroupID;
		result << pServer->sPlayerCap << pServer->sFreePlayerCap;

#if __VERSION < 1600
		result << uint8(1); // unknown, 1 in 15XX samples, 0 in 18XX+
#else
		result << uint8(0); 
#endif

		// we read all this stuff from ini, TO-DO: make this more versatile.
		result	<< pServer->strKarusKingName << pServer->strKarusNotice 
				<< pServer->strElMoradKingName << pServer->strElMoradNotice;
#endif
	}
	Send(&result);
}

void CUser::HandleNews(Packet & pkt)
{
	Packet result(pkt.GetOpcode());

	if (m_pMain->m_news.Size)
	{
		result << "Login Notice" << uint16(m_pMain->m_news.Size);
		result.append(m_pMain->m_news.Content, m_pMain->m_news.Size);
	}
	else // dummy news, will skip past it
	{
		result << "Login Notice" << "<empty>";
	}
	Send(&result);
}

#if __VERSION >= 1453
void CUser::HandleSetEncryptionPublicKey(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	result << uint64(0); // set key of 0 to disable encryption
	Send(&result);
}

void CUser::HandleUnkF7(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	result << uint16(0);
	Send(&result);
}
#endif