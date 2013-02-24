// User.cpp: implementation of the CUser class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "versionmanagerdlg.h"

using namespace std;

LSPacketHandler PacketHandlers[NUM_LS_OPCODES];
void InitPacketHandlers(void)
{
	memset(&PacketHandlers, 0, sizeof(LSPacketHandler) * NUM_LS_OPCODES);
	PacketHandlers[LS_VERSION_REQ]			= &LoginSession::HandleVersion;
	PacketHandlers[LS_DOWNLOADINFO_REQ]		= &LoginSession::HandlePatches;
	PacketHandlers[LS_LOGIN_REQ]			= &LoginSession::HandleLogin;
	PacketHandlers[LS_SERVERLIST]			= &LoginSession::HandleServerlist;
	PacketHandlers[LS_NEWS]					= &LoginSession::HandleNews;
#if __VERSION >= 1453
	PacketHandlers[LS_CRYPTION]				= &LoginSession::HandleSetEncryptionPublicKey;
	PacketHandlers[LS_UNKF7]				= &LoginSession::HandleUnkF7;
#endif
}

LoginSession::LoginSession(uint16 socketID, SocketMgr *mgr) : KOSocket(socketID, mgr, -1, 2048, 64) {}

bool LoginSession::HandlePacket(Packet & pkt)
{
	uint8 opcode = pkt.GetOpcode();

	// Unknown packet
	if (opcode >= NUM_LS_OPCODES
		|| PacketHandlers[opcode] == NULL)
		return false;

	(this->*PacketHandlers[opcode])(pkt);
	return true;
}

void LoginSession::HandleVersion(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	result << g_pMain->GetVersion();
	Send(&result);
}

void LoginSession::HandlePatches(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	std::set<std::string> downloadset;
	uint16 version;
	pkt >> version;

	foreach (itr, (*g_pMain->GetPatchList())) 
	{
		auto pInfo = itr->second;
		if (pInfo->sVersion > version)
			downloadset.insert(pInfo->strFileName);
	}

	result << g_pMain->GetFTPUrl() << g_pMain->GetFTPPath();
	result << uint16(downloadset.size());
	
	foreach (itr, downloadset)
		result << (*itr);

	Send(&result);
}

void LoginSession::HandleLogin(Packet & pkt)
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
	uint16 resultCode = 0;
	string account, password;

	pkt >> account >> password;
	if (account.size() == 0 || account.size() > MAX_ID_SIZE 
		|| password.size() == 0 || password.size() > MAX_PW_SIZE)
		resultCode = AUTH_NOT_FOUND; 
	else
		resultCode = g_pMain->m_DBProcess.AccountLogin(account, password);

	result << uint8(resultCode);
	if (resultCode == AUTH_SUCCESS)
	{
		result << int8(-1) << int8(-1); // prem type, prem days/hours (what are we even with 19XX)
		result << account; // it uses this for the game server now.
	}
	Send(&result);
}

void LoginSession::HandleServerlist(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	g_pMain->m_DBProcess.LoadUserCountList();

#if __VERSION >= 1500
	uint16 echo;
	pkt >> echo;
	result << echo;
#endif

	result << uint8(g_pMain->GetServerList()->size());
	foreach (itr, (*g_pMain->GetServerList())) 
	{		
		_SERVER_INFO *pServer = *itr;

		result << pServer->strServerIP;
#if __VERSION >= 1888
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

void LoginSession::HandleNews(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	News *pNews = g_pMain->GetNews();

	if (pNews->Size)
	{
		result << "Login Notice" << uint16(pNews->Size);
		result.append(pNews->Content, pNews->Size);
	}
	else // dummy news, will skip past it
	{
		result << "Login Notice" << "<empty>";
	}
	Send(&result);
}

#if __VERSION >= 1453
void LoginSession::HandleSetEncryptionPublicKey(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	result << uint64(0); // set key of 0 to disable encryption
	Send(&result);
}

void LoginSession::HandleUnkF7(Packet & pkt)
{
	Packet result(pkt.GetOpcode());
	result << uint16(0);
	Send(&result);
}
#endif