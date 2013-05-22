#pragma once

#include <queue>
#include <set>
#include "Socket.h"
#include "ListenSocket.h"

class SocketMgr
{
public:
	SocketMgr();

	INLINE HANDLE GetCompletionPort() { return m_completionPort; }
	INLINE void SetCompletionPort(HANDLE cp) { m_completionPort = cp; }

	void CreateCompletionPort();
	void SpawnWorkerThreads();
	void ShutdownThreads();

	virtual Socket *AssignSocket(SOCKET socket) = 0;
	virtual void OnConnect(Socket *pSock) {};
	virtual void OnDisconnect(Socket *pSock) 
	{
		FastGuard lock(s_disconnectionQueueLock);
		s_disconnectionQueue.push(pSock);
	}
	virtual void DisconnectCallback(Socket *pSock) {}

	virtual ~SocketMgr();

	static FastMutex SocketMgr::s_disconnectionQueueLock;
	static std::queue<Socket *> SocketMgr::s_disconnectionQueue;

protected:
	HANDLE m_completionPort;
#ifdef USE_STD_THREAD
	std::vector<std::thread> m_hThreads;
	static std::thread s_hCleanupThread;
#else
	std::vector<HANDLE> m_hThreads;
	static HANDLE s_hCleanupThread;
#endif

	long m_threadCount;

	static void SetupWinsock();
	static void CleanupWinsock();

	// TO-DO: Make this atomic.
	INLINE void IncRef() { if (s_refs++ == 0) SetupWinsock(); }
	INLINE void DecRef() { if (--s_refs == 0) CleanupWinsock(); }

	static uint32 s_refs; // reference counter (one app can hold multiple socket manager instances)
};

typedef void(*OperationHandler)(Socket * s, uint32 len);

uint32 __stdcall SocketWorkerThread(void * lp);

void HandleReadComplete(Socket * s, uint32 len);
void HandleWriteComplete(Socket * s, uint32 len);
void HandleShutdown(Socket * s, uint32 len);

static OperationHandler ophandlers[] =
{
	&HandleReadComplete,
	&HandleWriteComplete,
	&HandleShutdown
};
