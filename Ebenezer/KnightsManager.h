// KnightsManager.h: interface for the CKnightsManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KNIGHTSMANAGER_H__B3BA0329_28DF_4E7F_BC19_101D7A69E896__INCLUDED_)
#define AFX_KNIGHTSMANAGER_H__B3BA0329_28DF_4E7F_BC19_101D7A69E896__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CUser;
class CEbenezerDlg;
class Packet;

class CKnightsManager  
{
public:

	void CreateKnights(CUser* pUser, Packet & pkt);
	void JoinKnights(CUser* pUser, Packet & pkt);
	void WithdrawKnights(CUser* pUser, Packet & pkt);
	void ModifyKnightsMember(CUser* pUser, Packet & pkt);
	void DestroyKnights(CUser* pUser);
	void AllKnightsList(CUser* pUser, Packet & pkt);
	void AllKnightsMember(CUser* pUser);
	void CurrentKnightsMember(CUser* pUser, Packet & pkt);
	void JoinKnightsReq(CUser* pUser, Packet & pkt);
	void ListTop10Clans(CUser *pUser);

	void RecvKnightsAllList( char* pBuf );
	BOOL AddKnightsUser(int index, char* UserName);
	void SetKnightsUser( int index, char* UserName );
	BOOL RemoveKnightsUser( int index, char* UserName );
	void RecvKnightsList( char* pBuf );
	BOOL LoadKnightsIndex(int index);
	BOOL LoadAllKnights();
	void RecvDestroyKnights( CUser* pUser, char* pBuf );
	void RecvModifyFame( CUser* pUser, char* pBuf, BYTE command );
	void RecvJoinKnights( CUser* pUser, char* pBuf, BYTE command );
	void RecvCreateKnights( CUser* pUser, char* pBuf );
	void ReceiveKnightsProcess( CUser* pUser, char* pBuf);

	int GetKnightsIndex( int nation );
	BOOL IsAvailableName( const char* strname);	
	void PacketProcess(CUser* pUser, Packet & pkt);	

	CKnightsManager();
	virtual ~CKnightsManager();

	CEbenezerDlg* m_pMain;
//	CDatabase	m_KnightsDB;
private:

};

#endif // !defined(AFX_KNIGHTSMANAGER_H__B3BA0329_28DF_4E7F_BC19_101D7A69E896__INCLUDED_)
