// IOCPort.cpp: implementation of the CIOCPort class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IOCPort.h"
#include "IOCPSocket2.h"
#include "Define.h"
#include <algorithm>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CRITICAL_SECTION g_critical;

DWORD WINAPI AcceptThread(LPVOID lp);
DWORD WINAPI ReceiveWorkerThread(LPVOID lp);
DWORD WINAPI ClientWorkerThread(LPVOID lp);

DWORD WINAPI AcceptThread(LPVOID lp)
{
	CIOCPort* pIocport = (CIOCPort*) lp;

	WSANETWORKEVENTS	network_event;
	DWORD				wait_return;
	int					sid;
	CIOCPSocket2*		pSocket = NULL;

	struct sockaddr_in  addr;
	int					len;

	while (1)
	{
		wait_return = WaitForSingleObject(pIocport->m_hListenEvent, INFINITE);
		if (wait_return == WAIT_FAILED)
		{
			DEBUG_LOG_FILE("WAit failed error: %d", GetLastError());
			return 1;
		}

		WSAEnumNetworkEvents( pIocport->m_ListenSocket, pIocport->m_hListenEvent, &network_event);
		if (network_event.lNetworkEvents & FD_ACCEPT)
		{
			if (network_event.iErrorCode[FD_ACCEPT_BIT] == 0 ) 
			{
				sid = pIocport->GetNewSid();
				if (sid < 0
					|| (pSocket = pIocport->GetIOCPSocket(sid)) == NULL) 
				{
					DEBUG_LOG_FILE("Cannot accept anymore sockets.");

					// make sure we don't hold up the queue
					closesocket(accept(pIocport->m_ListenSocket, (struct sockaddr *)&addr, &len));
					goto loop_pass_accept;
				}

				len = sizeof(addr);
				if (!pSocket->Accept( pIocport->m_ListenSocket, (struct sockaddr *)&addr, &len))
				{
					DEBUG_LOG_FILE("Accept fail: %d", sid);
					EnterCriticalSection(&g_critical);
					pIocport->RidIOCPSocket( sid, pSocket );
					pIocport->PutOldSid( sid );
					LeaveCriticalSection(&g_critical);
					goto loop_pass_accept;
				}

				pSocket->InitSocket( pIocport );

				if (!pIocport->Associate(pSocket, pIocport->m_hServerIOCPort))
				{
					DEBUG_LOG("Unable to associate socket with session: %d", sid);
					pSocket->CloseProcess();
					EnterCriticalSection(&g_critical);
					pIocport->RidIOCPSocket( sid, pSocket );
					pIocport->PutOldSid( sid );
					LeaveCriticalSection(&g_critical);
					goto loop_pass_accept;
				}

				pSocket->Receive();
				TRACE("Accepted socket: %d\n", sid);
			}

loop_pass_accept:
			continue;
		}
	}
	
	return 1;
}

DWORD WINAPI ReceiveWorkerThread(LPVOID lp)
{
	CIOCPort* pIocport = (CIOCPort*) lp;

	DWORD			WorkIndex;	
	LPOVERLAPPED	pOvl;
	DWORD			nbytes;
	DWORD			dwFlag = 0;
	CIOCPSocket2*	pSocket = NULL;

	while (1)
	{
		BOOL b = GetQueuedCompletionStatus(pIocport->m_hServerIOCPort, &nbytes, &WorkIndex, &pOvl, INFINITE);

		if ((!b && !pOvl)
			|| WorkIndex > (DWORD)pIocport->m_SocketArraySize)
			continue;

		pSocket = (CIOCPSocket2 *)pIocport->m_SockArray[WorkIndex];
		if (!pSocket)
			continue;

		if (b)
		{
			switch (pOvl->Offset)
			{
			case	OVL_RECEIVE:
				if (!nbytes)
				{
					TRACE("Socket %d dropped (0 bytes received)\n", WorkIndex);
					pSocket->CloseProcess();
					EnterCriticalSection(&g_critical);
					pIocport->RidIOCPSocket(pSocket->GetSocketID(), pSocket);
					pIocport->PutOldSid(pSocket->GetSocketID());
					LeaveCriticalSection(&g_critical);
					break;
				}

				pSocket->m_nPending = pSocket->m_nWouldblock = 0;
				pSocket->ReceivedData((int)nbytes);
				pSocket->Receive();
			break;

			case	OVL_SEND:
				pSocket->m_nPending = pSocket->m_nWouldblock = 0;
			break;

			case	OVL_CLOSE:
				TRACE("Socket %d disconnected by server\n", WorkIndex);
				pSocket->CloseProcess();
				EnterCriticalSection(&g_critical);
				pIocport->RidIOCPSocket(pSocket->GetSocketID(), pSocket);
				pIocport->PutOldSid(pSocket->GetSocketID());
				LeaveCriticalSection(&g_critical);
			break;
			}
		}
		else
		{
			pSocket->CloseProcess();

			EnterCriticalSection( &g_critical );
			pIocport->RidIOCPSocket( pSocket->GetSocketID(), pSocket );
			pIocport->PutOldSid( pSocket->GetSocketID() );
			LeaveCriticalSection( &g_critical );

			if (pOvl)
				TRACE("Socket %d disconnected abnormally\n", WorkIndex);
			else 
				TRACE("Socket %d disconnected by IOCP error[%d] - %d\n", GetLastError(), WorkIndex);
		}
	}

	return 1;
}

DWORD WINAPI ClientWorkerThread(LPVOID lp)
{
	CIOCPort* pIocport = (CIOCPort*) lp;

	DWORD			WorkIndex;	
	BOOL			b;
	LPOVERLAPPED	pOvl;
	DWORD			nbytes;
	DWORD			dwFlag = 0;
	CIOCPSocket2*	pSocket = NULL;

	while (1)
	{
		b = GetQueuedCompletionStatus( 
									  pIocport->m_hClientIOCPort,
									  &nbytes,
									  &WorkIndex,
									  &pOvl,
									  INFINITE);
		if ((!b && !pOvl)
			|| WorkIndex > (DWORD)pIocport->m_ClientSockSize) 
			continue;

		pSocket = (CIOCPSocket2 *)pIocport->m_ClientSockArray[WorkIndex];
		if (pSocket == NULL)
			continue;

		if (b)
		{
			switch (pOvl->Offset)
			{
			case	OVL_RECEIVE:
				if (!nbytes) 
				{
					OutputDebugString("AISocket Closed By 0 Byte Notify\n" );
					pSocket->CloseProcess();
					EnterCriticalSection( &g_critical ); // AI server doesn't need the sid system
					pIocport->RidIOCPSocket( pSocket->GetSocketID(), pSocket );
					LeaveCriticalSection( &g_critical );
					break;
				}

				pSocket->m_nPending = pSocket->m_nWouldblock = 0;
				pSocket->ReceivedData((int)nbytes);
				pSocket->Receive();
				break;
			case	OVL_SEND:
				pSocket->m_nPending = pSocket->m_nWouldblock = 0;
				break;
			case	OVL_CLOSE:
					
				OutputDebugString("AISocket Closed By Close()\n" );
				pSocket->CloseProcess();
				EnterCriticalSection( &g_critical ); // AI server doesn't need the sid system
				pIocport->RidIOCPSocket( pSocket->GetSocketID(), pSocket );
				LeaveCriticalSection( &g_critical );
				break;
			}
		}
		else 
		{
			OutputDebugString("AISocket Closed By Abnormal Termination\n" );
			pSocket->CloseProcess();
			EnterCriticalSection( &g_critical );
			pIocport->RidIOCPSocket( pSocket->GetSocketID(), pSocket );				
			LeaveCriticalSection( &g_critical );
		}

	}

	return 1;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIOCPort::CIOCPort()
{
	m_SockArray = NULL;
	m_SockArrayInActive = NULL;
	m_ClientSockArray = NULL;

	m_SocketArraySize = 0;
	m_ClientSockSize = 0;

	m_dwConcurrency = 1;
	m_hAcceptThread = NULL;

	m_hReceiveWorkerThreads = NULL;

	for (int i = 0; i < 10; i++)
		m_hClientWorkerThreads[i] = NULL;
}

void CIOCPort::Init(int serversocksize, int clientsocksize, int workernum)
{
	m_SocketArraySize = serversocksize;
	m_ClientSockSize = clientsocksize;
	
	m_SockArray = new CIOCPSocket2* [serversocksize];
	for(int i = 0; i<serversocksize; i++ ) {
		m_SockArray[i] = NULL;
	}

	m_SockArrayInActive = new CIOCPSocket2* [serversocksize];
	for(int i = 0; i<serversocksize; i++ ) {
		m_SockArrayInActive[i] = NULL;
	}

	m_ClientSockArray = new CIOCPSocket2* [clientsocksize];		// 해당 서버가 클라이언트로서 다른 컴터에 붙는 소켓수
	for(int i=0; i<clientsocksize; i++ ) {
		m_ClientSockArray[i] = NULL;
	}

	for(int i = 0; i<serversocksize; i++)
		m_SidList.push_back(i);

	InitializeCriticalSection( &g_critical );

	CreateReceiveWorkerThread(workernum);
	CreateClientWorkerThread();

	m_PostOverlapped.hEvent = NULL;

	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD( 2, 2 );
	err = WSAStartup( wVersionRequested, &wsaData ); 
}

BOOL CIOCPort::Listen(int port)
{
	int opt;
	struct sockaddr_in addr;
	struct linger lingerOpt;
	
	// Open a TCP socket (an Internet stream socket).
	//
	if ( ( m_ListenSocket = socket(AF_INET, SOCK_STREAM, 0) ) < 0 ) 
	{
		TRACE("Can't open stream socket\n");
		return FALSE;
	}

	// Bind our local address so that the client can send to us. 
	//
	memset((void *)&addr, 0, sizeof(addr));
	addr.sin_family			= AF_INET;
	addr.sin_addr.s_addr	= htonl(INADDR_ANY);
	addr.sin_port			= htons(port);
	
	// added in an attempt to allow rebinding to the port 
	//
	opt = 1;
	setsockopt( m_ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

	opt = 1;
	setsockopt( m_ListenSocket, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, sizeof(opt));

	// Linger off -> close socket immediately regardless of existance of data 
	//
	lingerOpt.l_onoff = 1;
	lingerOpt.l_linger = 0;

	setsockopt(m_ListenSocket, SOL_SOCKET, SO_LINGER, (char *)&lingerOpt, sizeof(lingerOpt));
	
	if ( bind(m_ListenSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0 )
	{
		TRACE("Can't bind local address\n");
		return FALSE;
	}

	int socklen, len, err;

	socklen = SOCKET_BUFF_SIZE*4;
	setsockopt( m_ListenSocket, SOL_SOCKET, SO_RCVBUF, (char*)&socklen, sizeof(socklen));
	
	len = sizeof(socklen);
	err = getsockopt( m_ListenSocket, SOL_SOCKET, SO_RCVBUF, (char*)&socklen, &len);
	if (err == SOCKET_ERROR)
	{
		TRACE("FAIL : Set Socket RecvBuf of port(%d) as %d\n", port, socklen);
		return FALSE;
	}

	socklen = SOCKET_BUFF_SIZE*4;
	setsockopt( m_ListenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&socklen, sizeof(socklen));
	len = sizeof(socklen);
	err = getsockopt( m_ListenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&socklen, &len);

	if (err == SOCKET_ERROR)
	{
		TRACE("FAIL: Set Socket SendBuf of port(%d) as %d\n", port, socklen);
		return FALSE;
	}
	
	listen(m_ListenSocket, 5);

	m_hListenEvent = WSACreateEvent();
	if( m_hListenEvent == WSA_INVALID_EVENT ) {
		err = WSAGetLastError();
		TRACE("Listen Event Create Fail!! %d \n", err);
		return FALSE;
	}
	WSAEventSelect( m_ListenSocket, m_hListenEvent, FD_ACCEPT);

	TRACE("Port (%05d) initialzed\n", port);

	CreateAcceptThread();

	return TRUE;
}

BOOL CIOCPort::Associate(CIOCPSocket2 *pIocpSock, HANDLE hPort)
{
	if (!hPort) {
		TRACE("ERROR : No Completion Port\n");
		return FALSE;
	}
	
	HANDLE hTemp;
	hTemp = CreateIoCompletionPort( pIocpSock->GetSocketHandle(), hPort, (DWORD)pIocpSock->GetSocketID(), m_dwConcurrency);
	
	return (hTemp == hPort);
}

int CIOCPort::GetNewSid()
{
	int ret = -1;

	EnterCriticalSection(&g_critical);
	if (m_SidList.empty())
	{
		TRACE("SID List Is Empty !!\n");
	}
	else
	{
		ret = m_SidList.front();
		m_SidList.pop_front();
	}
	LeaveCriticalSection(&g_critical);

	return ret;
}

void CIOCPort::PutOldSid(int sid)
{
	if (sid < 0 || sid > m_SocketArraySize)
	{
		TRACE("recycle sid invalid value : %d\n", sid);
		return;
	}

	auto itr = find(m_SidList.begin(), m_SidList.end(), sid);
	if (itr == m_SidList.end())
		m_SidList.push_back(sid);
}

void CIOCPort::CreateAcceptThread()
{
	DWORD id;

	m_hAcceptThread = ::CreateThread( NULL, 0, AcceptThread, (LPVOID)this, CREATE_SUSPENDED, &id);

#ifndef DEBUG
	::SetThreadPriority(m_hAcceptThread,THREAD_PRIORITY_ABOVE_NORMAL);
#endif
}

void CIOCPort::CreateReceiveWorkerThread(int workernum)
{
	SYSTEM_INFO		SystemInfo;

	//
	// try to get timing more accurate... Avoid context
	// switch that could occur when threads are released
	//
#ifndef DEBUG
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#endif

	//
	// Figure out how many processors we have to size the minimum
	// number of worker threads and concurrency
	//
	GetSystemInfo (&SystemInfo);
	
	if (!workernum)
		m_dwNumberOfWorkers = 2 * SystemInfo.dwNumberOfProcessors;
	else
		m_dwNumberOfWorkers = workernum;

	m_dwConcurrency = SystemInfo.dwNumberOfProcessors;
	m_hServerIOCPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, m_dwNumberOfWorkers);
	
	m_hReceiveWorkerThreads = new HANDLE[m_dwNumberOfWorkers];
	for (DWORD i = 0; i < m_dwNumberOfWorkers; i++)
	{
		DWORD workerId;
		m_hReceiveWorkerThreads[i] = CreateThread(NULL, 0, ReceiveWorkerThread, (LPVOID)this, 0, &workerId);
	}
}

void CIOCPort::CreateClientWorkerThread()
{
	m_hClientIOCPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 10);
	for (int i = 0; i< 10; i++) 
	{
		DWORD workerId;
		m_hClientWorkerThreads[i] = ::CreateThread(NULL, 0, ClientWorkerThread, (LPVOID)this, 0, &workerId);
	}
}

CIOCPSocket2* CIOCPort::GetIOCPSocket(int index)
{
	CIOCPSocket2 *pIOCPSock = NULL;

	if( index > m_SocketArraySize ) {
		TRACE("InActiveSocket Array Overflow[%d]\n", index );
		return NULL;
	}
	if ( !m_SockArrayInActive[index] ) {
		TRACE("InActiveSocket Array Invalid[%d]\n", index );
		return NULL;
	}
	else
		pIOCPSock = (CIOCPSocket2 *)m_SockArrayInActive[index];

	m_SockArray[index] = pIOCPSock;
	m_SockArrayInActive[index] = NULL;

	pIOCPSock->SetSocketID( index );

	return pIOCPSock;
}

void CIOCPort::RidIOCPSocket(int index, CIOCPSocket2 *pSock)
{
	if( index < 0 || (pSock->GetSockType() == TYPE_ACCEPT && index >= m_SocketArraySize) || (pSock->GetSockType() == TYPE_CONNECT && index >= m_ClientSockSize) ) {
		TRACE("Invalid Sock index - RidIOCPSocket\n");
		return;
	}
	if( pSock->GetSockType() == TYPE_ACCEPT ) {
		m_SockArray[index] = NULL;
		m_SockArrayInActive[index] = pSock;
	}
	else if( pSock->GetSockType() == TYPE_CONNECT ){
		m_ClientSockArray[index] = NULL;
	}
}

int CIOCPort::GetClientSid()
{
	for(int i=0; i<m_ClientSockSize; i++) {
		if( m_ClientSockArray[i] == NULL ) {
			return i;
		}
	}
		
	return -1;
}

void CIOCPort::DeleteAllArray()
{
	EnterCriticalSection( &g_critical );
	for( int i=0; i<m_SocketArraySize; i++ ) {
		if ( m_SockArray[i] != NULL ) {
			delete m_SockArray[i];
			m_SockArray[i] = NULL;
		}
	}
	delete[] m_SockArray;

	for (int i=0; i < m_SocketArraySize; i++ ) {
		if ( m_SockArrayInActive[i] != NULL ) {
			delete m_SockArrayInActive[i];
			m_SockArrayInActive[i] = NULL;
		}
	}
	delete[] m_SockArrayInActive;

	for (int i=0; i < m_ClientSockSize; i++ ) {
		if ( m_ClientSockArray[i] != NULL ) {
			delete m_ClientSockArray[i];
			m_ClientSockArray[i] = NULL;
		}
	}
	delete[] m_ClientSockArray;

	while( !m_SidList.empty() )
		m_SidList.pop_back();

	LeaveCriticalSection( &g_critical );
}

CIOCPort::~CIOCPort()
{
	if (m_hAcceptThread != NULL)
		TerminateThread(m_hAcceptThread, 0);

	if (m_hReceiveWorkerThreads != NULL)
	{
		for (DWORD i = 0; i < m_dwNumberOfWorkers; i++)
		{
			if (m_hReceiveWorkerThreads[i] == NULL)
				continue;

			TerminateThread(m_hReceiveWorkerThreads[i], 0);
			m_hReceiveWorkerThreads[i] = NULL;
		}

		delete [] m_hReceiveWorkerThreads;
	}

	for (int i = 0; i < 10; i++)
	{
			if (m_hClientWorkerThreads[i] == NULL)
				continue;

		TerminateThread(m_hClientWorkerThreads[i], 0);
		m_hClientWorkerThreads[i] = NULL;
	}

	DeleteAllArray();
	DeleteCriticalSection( &g_critical );

	WSACleanup();
}
