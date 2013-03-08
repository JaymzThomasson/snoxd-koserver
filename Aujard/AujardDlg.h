#pragma once

#include "../shared/SharedMem.h"

struct OdbcError;
class CAujardDlg
{
public:
	CAujardDlg();

	bool Startup();

	BOOL InitializeMMF();
	void ReportSQLError(OdbcError *pError);
	void WriteLogFile(std::string & logMessage);

	void AccountLogIn(Packet & pkt, int16 uid);
	void SelectNation(Packet & pkt, int16 uid);
	void AllCharInfoReq(Packet & pkt, int16 uid);
	void ChangeHairReq(Packet & pkt, int16 uid);
	void CreateNewChar(Packet & pkt, int16 uid);
	void DeleteChar(Packet & pkt, int16 uid);
	void SelectCharacter(Packet & pkt, int16 uid);
	void UserLogOut(Packet & pkt, int16 uid);
	void UserDataSave(Packet & pkt, int16 uid);
	void KnightsPacket(Packet & pkt, int16 uid);
	void CreateKnights(Packet & pkt, int16 uid);
	void JoinKnights(Packet & pkt, int16 uid);
	void WithdrawKnights(Packet & pkt, int16 uid);
	void ModifyKnightsMember(Packet & pkt, uint8 command, int16 uid);
	void DestroyKnights(Packet & pkt, int16 uid);
	void AllKnightsMember(Packet & pkt, int16 uid);
	void KnightsList(Packet & pkt, int16 uid);
	void RegisterClanSymbol(Packet & pkt, int16 uid);
	void SetLogInInfo(Packet & pkt, int16 uid);
	void UserKickOut(Packet & pkt);
	void BattleEventResult(Packet & pkt);
	void ShoppingMall(Packet & pkt, int16 uid);
	void LoadWebItemMall(Packet & pkt, int16 uid);
	void SkillDataProcess(Packet & pkt, int16 uid);
	void SkillDataSave(Packet & pkt, int16 uid);
	void SkillDataLoad(Packet & pkt, int16 uid);
	void FriendProcess(Packet & pkt, int16 uid);
	void RequestFriendList(Packet & pkt, int16 uid);
	void AddFriend(Packet & pkt, int16 uid);
	void RemoveFriend(Packet & pkt, int16 uid);
	void ChangeCape(Packet & pkt);

	void SaveUserData();
	void ConCurrentUserCount();
	_USER_DATA* GetUserPtr(const char* struserid, short & uid);

	void OnTimer(UINT nIDEvent);
	void AllSaveRoutine();

	~CAujardDlg();

	CSharedMemQueue	m_LoggerSendQueue, m_LoggerRecvQueue;

	HANDLE	m_hReadQueueThread;
	HANDLE	m_hMMFile;
	char*	m_lpMMFile;

	CDBAgent	m_DBAgent;

	int	m_nServerNo, m_nZoneNo;
	char m_strGameDSN[32], m_strAccountDSN[32];
	char m_strGameUID[32], m_strAccountUID[32];
	char m_strGamePWD[32], m_strAccountPWD[32];

	FILE * m_fp;
	FastMutex m_lock;
};

extern CAujardDlg g_pMain;