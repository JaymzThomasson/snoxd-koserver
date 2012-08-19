// IOCPort.h: interface for the CIOCPort class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IOCPORT_H__1555441D_142E_4C26_B889_D9DCFC5E54E8__INCLUDED_)
#define AFX_IOCPORT_H__1555441D_142E_4C26_B889_D9DCFC5E54E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning(disable : 4786)

#include <list>
using namespace std;

typedef  list<int>  SidList;

class CIOCPSocket2;

class CIOCPort  
{
public:
	CIOCPort();
	void CreateSendWorkerThread();
	int GetClientSid();
	void CreateAcceptThread();
	void CreateClientWorkerThread();
	void RidIOCPSocket(int index, CIOCPSocket2 *pSock);
	CIOCPSocket2* GetIOCPSocket(int index);
	void CreateReceiveWorkerThread(int workernum);
	void PutOldSid(int sid);
	int GetNewSid();
	BOOL Associate(CIOCPSocket2 *pIocpSock, HANDLE hPort);
	BOOL Listen( int port );
	void Init( int serversocksize, int clientsocksize, int workernum = 0 );
	void DeleteAllArray();
	virtual ~CIOCPort();

	SOCKET m_ListenSocket;
	HANDLE m_hListenEvent;
	HANDLE m_hServerIOCPort;
	HANDLE m_hClientIOCPort;
	HANDLE m_hSendIOCPort;		// Compress Packet Send Exclusively
	HANDLE m_hAcceptThread;

	OVERLAPPED		m_PostOverlapped;

	int m_SocketArraySize;
	int m_ClientSockSize;

	SidList m_SidList;

	HANDLE * m_hReceiveWorkerThreads;
	HANDLE * m_hSendWorkerThreads;
	HANDLE   m_hClientWorkerThreads[10]; // AI

	CIOCPSocket2 **m_SockArray;
	CIOCPSocket2 **m_SockArrayInActive;

	CIOCPSocket2 **m_ClientSockArray;		// AI server

protected:
	DWORD m_dwNumberOfWorkers;
	DWORD m_dwConcurrency;
};

#endif // !defined(AFX_IOCPORT_H__1555441D_142E_4C26_B889_D9DCFC5E54E8__INCLUDED_)
