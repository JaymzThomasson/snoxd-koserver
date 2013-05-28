#pragma once

#include <queue>
#include <set>
#include "Socket.h"

uint32 THREADCALL SocketCleanupThread(void * lpParam);

class SocketMgr
{
public:
	SocketMgr();

	uint32 GetPreferredThreadCount();
	void SpawnWorkerThreads();
	void ShutdownThreads();

	static void SetupSockets();
	static void CleanupSockets();

#ifdef CONFIG_USE_IOCP
#	include "SocketMgrWin32.inl"
#endif

	virtual Socket *AssignSocket(SOCKET socket) = 0;
	virtual void OnConnect(Socket *pSock);
	virtual void OnDisconnect(Socket *pSock);
	virtual void DisconnectCallback(Socket *pSock);
	virtual void Shutdown();
	virtual ~SocketMgr();

	static FastMutex s_disconnectionQueueLock;
	static std::queue<Socket *> s_disconnectionQueue;

protected:
	bool m_bShutdown;

	std::vector<Thread *> m_threads;
	static Thread s_cleanupThread;

	long m_threadCount;
	bool m_bWorkerThreadsActive;

#ifdef USE_STD_ATOMIC
	/* before we increment, first time it'll be 0 */
	INLINE void IncRef() { if (s_refCounter++ == 0) SetupSockets(); }
	INLINE void DecRef() { if (--s_refCounter== 0) CleanupSockets(); }

	// reference counter (one app can hold multiple socket manager instances)
	static std::atomic_ulong s_refCounter;
#else
	/* first increment sets it to 1 */
	INLINE void IncRef() { if (InterlockedIncrement(&s_refCounter) == 1) SetupSockets(); }
	INLINE void DecRef() { if (InterlockedDecrement(&s_refCounter) == 0) CleanupSockets(); }

	// reference counter (one app can hold multiple socket manager instances)
	volatile static long s_refCounter;
#endif

public:
	static bool s_bRunningCleanupThread;
};