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

	BOOL AddKnightsUser(int index, const char* UserName);
	void SetKnightsUser(int index, const char* UserName);
	BOOL RemoveKnightsUser(int index, const char* UserName);
	BOOL LoadKnightsIndex(int index);
	BOOL LoadAllKnights();

	// database requests go here
	void ReqKnightsPacket(CUser* pUser, Packet & pkt);
	void ReqCreateKnights(CUser* pUser, Packet & pkt);
	void ReqJoinKnights(CUser* pUser, Packet & pkt);
	void ReqWithdrawKnights(CUser* pUser, Packet & pkt);
	void ReqModifyKnightsMember(CUser* pUser, Packet & pkt, uint8 command);
	void ReqDestroyKnights(CUser* pUser, Packet & pkt);
	void ReqAllKnightsMember(CUser *pUser, Packet & pkt);
	void ReqKnightsList(CUser *pUser, Packet & pkt);
	void ReqRegisterClanSymbol(CUser *pUser, Packet & pkt);

	// received from database request (this will go)
	void RecvCreateKnights(CUser* pUser, Packet & pkt);
	void RecvJoinKnights(CUser* pUser, Packet & pkt, BYTE command);
	void RecvModifyFame(CUser* pUser, Packet & pkt, BYTE command);
	void RecvDestroyKnights( CUser* pUser, Packet & pkt);
	void RecvKnightsList(Packet & pkt);
	void RecvKnightsAllList(Packet & pkt);
	void RecvRegisterClanSymbol(CUser* pUser, Packet & pkt);

	void ReceiveKnightsProcess(CUser* pUser, Packet & pkt);

	int GetKnightsIndex( int nation );
	BOOL IsAvailableName( const char* strname);	
	void PacketProcess(CUser* pUser, Packet & pkt);	

	CKnightsManager();
	virtual ~CKnightsManager();

//	CDatabase	m_KnightsDB;
private:

};