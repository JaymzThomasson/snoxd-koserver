#pragma once

class CUser;
class CEbenezerDlg;
class Packet;

class CKnightsManager  
{
public:
	static void CreateKnights(CUser* pUser, Packet & pkt);
	static void JoinKnights(CUser* pUser, Packet & pkt);
	static void WithdrawKnights(CUser* pUser, Packet & pkt);
	static void ModifyKnightsMember(CUser* pUser, Packet & pkt, uint8 opcode);
	static void DestroyKnights(CUser* pUser);
	static void AllKnightsList(CUser* pUser, Packet & pkt);
	static void AllKnightsMember(CUser* pUser);
	static void CurrentKnightsMember(CUser* pUser, Packet & pkt);
	static void JoinKnightsReq(CUser* pUser, Packet & pkt);
	static void RegisterClanSymbol(CUser* pUser, Packet & pkt);
	static void RequestClanSymbolVersion(CUser* pUser, Packet & pkt);
	static void RequestClanSymbols(CUser* pUser, Packet & pkt);
	static void GetClanSymbol(CUser* pUser, uint16 sClanID);
	static void ListTop10Clans(CUser *pUser);
	static void DonateNP(CUser *pUser, Packet & pkt);

	static bool AddKnightsUser(int index, const char* UserName);
	static void SetKnightsUser(int index, const char* UserName);
	static void AddUserDonatedNP(int index, std::string & strUserID, uint32 nDonatedNP);
	static bool RemoveKnightsUser(int index, const char* UserName);
	static bool LoadKnightsIndex(int index);
	static bool LoadAllKnights();

	// database requests go here
	static void ReqKnightsPacket(CUser* pUser, Packet & pkt);
	static void ReqCreateKnights(CUser* pUser, Packet & pkt);
	static void ReqUpdateKnights(CUser* pUser, Packet & pkt, uint8 opcode);
	static void ReqModifyKnightsMember(CUser* pUser, Packet & pkt, uint8 command);
	static void ReqDestroyKnights(CUser* pUser, Packet & pkt);
	static void ReqAllKnightsMember(CUser *pUser, Packet & pkt);
	static void ReqKnightsList(Packet & pkt);
	static void ReqRegisterClanSymbol(CUser *pUser, Packet & pkt);
	static void ReqDonateNP(CUser *pUser, Packet & pkt);
	static void ReqRefundNP(Packet & pkt);

	static void RecvUpdateKnights(CUser* pUser, Packet & pkt, uint8 command);
	static void RecvModifyFame(CUser* pUser, Packet & pkt, uint8 command);
	static void RecvKnightsAllList(Packet & pkt);

	static int GetKnightsIndex( int nation );
	static bool IsAvailableName( const char* strname);	
	static void PacketProcess(CUser* pUser, Packet & pkt);	
};