#pragma once

class CUser;
class CEbenezerDlg;
class Packet;

class CKnightsManager  
{
public:
	void CreateKnights(CUser* pUser, Packet & pkt);
	void JoinKnights(CUser* pUser, Packet & pkt);
	void WithdrawKnights(CUser* pUser, Packet & pkt);
	void ModifyKnightsMember(CUser* pUser, Packet & pkt, uint8 opcode);
	void DestroyKnights(CUser* pUser);
	void AllKnightsList(CUser* pUser, Packet & pkt);
	void AllKnightsMember(CUser* pUser);
	void CurrentKnightsMember(CUser* pUser, Packet & pkt);
	void JoinKnightsReq(CUser* pUser, Packet & pkt);
	void RegisterClanSymbol(CUser* pUser, Packet & pkt);
	void RequestClanSymbolVersion(CUser* pUser, Packet & pkt);
	void RequestClanSymbols(CUser* pUser, Packet & pkt);
	void GetClanSymbol(CUser* pUser, uint16 sClanID);
	void ListTop10Clans(CUser *pUser);

	bool AddKnightsUser(int index, const char* UserName);
	void SetKnightsUser(int index, const char* UserName);
	bool RemoveKnightsUser(int index, const char* UserName);
	bool LoadKnightsIndex(int index);
	bool LoadAllKnights();

	// database requests go here
	void ReqKnightsPacket(CUser* pUser, Packet & pkt);
	void ReqCreateKnights(CUser* pUser, Packet & pkt);
	void ReqUpdateKnights(CUser* pUser, Packet & pkt, uint8 opcode);
	void ReqModifyKnightsMember(CUser* pUser, Packet & pkt, uint8 command);
	void ReqDestroyKnights(CUser* pUser, Packet & pkt);
	void ReqAllKnightsMember(CUser *pUser, Packet & pkt);
	void ReqKnightsList(Packet & pkt);
	void ReqRegisterClanSymbol(CUser *pUser, Packet & pkt);

	void RecvUpdateKnights(CUser* pUser, Packet & pkt, BYTE command);
	void RecvModifyFame(CUser* pUser, Packet & pkt, BYTE command);
	void RecvKnightsAllList(Packet & pkt);

	int GetKnightsIndex( int nation );
	bool IsAvailableName( const char* strname);	
	void PacketProcess(CUser* pUser, Packet & pkt);	

	CKnightsManager();
	virtual ~CKnightsManager();
};