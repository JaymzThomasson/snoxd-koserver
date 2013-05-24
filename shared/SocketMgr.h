#pragma once

#include <queue>
#include <set>
#include "Socket.h"
#include "ListenSocket.h"

uint32 __stdcall SocketCleanupThread(void * lpParam);

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

	static FastMutex SocketMgr::s_disconnectionQueueLock;
	static std::queue<Socket *> SocketMgr::s_disconnectionQueue;

protected:
	bool m_bShutdown;

#ifdef USE_STD_THREAD
	std::vector<std::thread> m_hThreads;
	static std::thread s_hCleanupThread;
#else
	std::vector<HANDLE> m_hThreads;
	static HANDLE s_hCleanupThread;
#endif

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