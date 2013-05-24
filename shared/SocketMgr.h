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
	virtual void Shutdown();
	virtual ~SocketMgr();

	static FastMutex SocketMgr::s_disconnectionQueueLock;
	static std::queue<Socket *> SocketMgr::s_disconnectionQueue;

protected:
	bool m_bShutdown;
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

#ifdef USE_STD_ATOMIC
	/* before we increment, first time it'll be 0 */
	INLINE void IncRef() { if (s_refCounter++ == 0) SetupWinsock(); }
	INLINE void DecRef() { if (--s_refCounter== 0) CleanupWinsock(); }

	// reference counter (one app can hold multiple socket manager instances)
	static std::atomic_ulong s_refCounter;
#else
	/* first increment sets it to 1 */
	INLINE void IncRef() { if (InterlockedIncrement(&s_refCounter) == 1) SetupWinsock(); }
	INLINE void DecRef() { if (InterlockedDecrement(&s_refCounter) == 0) CleanupWinsock(); }

	// reference counter (one app can hold multiple socket manager instances)
	volatile static long s_refCounter;
#endif
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
