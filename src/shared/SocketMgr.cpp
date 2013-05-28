#include "stdafx.h"
#include "SocketMgr.h"

bool SocketMgr::s_bRunningCleanupThread = true;
FastMutex SocketMgr::s_disconnectionQueueLock;
std::queue<Socket *> SocketMgr::s_disconnectionQueue;

Thread SocketMgr::s_cleanupThread; 

#ifdef USE_STD_ATOMIC
std::atomic_ulong SocketMgr::s_refCounter;
#else
volatile long SocketMgr::s_refCounter = 0;
#endif

uint32 THREADCALL SocketCleanupThread(void * lpParam)
{
	while (SocketMgr::s_bRunningCleanupThread)
	{
		SocketMgr::s_disconnectionQueueLock.Acquire();
		while (!SocketMgr::s_disconnectionQueue.empty())
		{
			Socket *pSock = SocketMgr::s_disconnectionQueue.front();
			if (pSock->GetSocketMgr())
				pSock->GetSocketMgr()->DisconnectCallback(pSock);
			SocketMgr::s_disconnectionQueue.pop();
		}
		SocketMgr::s_disconnectionQueueLock.Release();
		sleep(100);
	}

	return 0;
}

SocketMgr::SocketMgr() : m_threadCount(0), 
#ifdef CONFIG_USE_IOCP
	m_completionPort(nullptr), 
#endif
	m_bWorkerThreadsActive(false),
	m_bShutdown(false)
{
	IncRef();
}

void SocketMgr::SpawnWorkerThreads()
{
	if (m_bWorkerThreadsActive)
		return;

	m_threadCount = GetPreferredThreadCount(); // IOCP is better off multithreaded, epoll and kqueue are not
	m_bWorkerThreadsActive = true;

	TRACE("SocketMgr - spawning %u worker threads.\n", m_threadCount);

	for (long x = 0; x < m_threadCount; x++)
		m_threads.push_back(new Thread(SocketWorkerThread, static_cast<void *>(this)));

	if (!s_cleanupThread.isStarted())
		s_cleanupThread.start(SocketCleanupThread);
}

void HandleReadComplete(Socket * s, uint32 len)
{
	if (s->IsDeleted())
		return;

	s->m_readEvent.Unmark();
	if (len)
	{
		s->GetReadBuffer().IncrementWritten(len);
		s->OnRead();
		s->SetupReadEvent();
	}
	else
	{
		// s->Delete();	  // Queue deletion.
		s->Disconnect();
	}
}

void HandleWriteComplete(Socket * s, uint32 len)
{
	if (s->IsDeleted())
		return;

	s->m_writeEvent.Unmark();
	s->BurstBegin();					// Lock
	s->GetWriteBuffer().Remove(len);
	if( s->GetWriteBuffer().GetContiguousBytes() > 0 )
		s->WriteCallback();
	else
		s->DecSendLock();
	s->BurstEnd();					  // Unlock
}

void HandleShutdown(Socket * s, uint32 len) {}
void SocketMgr::OnConnect(Socket *pSock) {}
void SocketMgr::DisconnectCallback(Socket *pSock) {}
void SocketMgr::OnDisconnect(Socket *pSock) 
{
	FastGuard lock(s_disconnectionQueueLock);
	s_disconnectionQueue.push(pSock);
}

void SocketMgr::ShutdownThreads()
{
#ifdef CONFIG_USE_IOCP
	for (long i = 0; i < m_threadCount; i++)
	{
		OverlappedStruct * ov = new OverlappedStruct(SOCKET_IO_THREAD_SHUTDOWN);
		PostQueuedCompletionStatus(m_completionPort, 0, (ULONG_PTR)0, &ov->m_overlap);
	}
#endif

	m_bWorkerThreadsActive = false;

	foreach (itr, m_threads)
	{
		(*itr)->waitForExit();
		delete (*itr);
	}
}

void SocketMgr::Shutdown()
{
	if (m_bShutdown)
		return;

	ShutdownThreads();

	DecRef();
	m_bShutdown = true;
}

void SocketMgr::SetupSockets()
{
#ifdef CONFIG_USE_IOCP
	SetupWinsock();
#endif
}

void SocketMgr::CleanupSockets()
{
	if (s_cleanupThread.isStarted())
	{
		s_bRunningCleanupThread = false;
		s_cleanupThread.waitForExit();
	}

#ifdef CONFIG_USE_IOCP
	CleanupWinsock();
#endif
}

SocketMgr::~SocketMgr()
{
	Shutdown();
}
