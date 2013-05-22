#include "stdafx.h"
#include "SocketMgr.h"

uint32 __stdcall SocketWorkerThread(void * lpParam)
{
	SocketMgr *socketMgr = (SocketMgr *)lpParam;
	HANDLE cp = socketMgr->GetCompletionPort();
	DWORD len;
	Socket * s;
	OverlappedStruct * ov;
	LPOVERLAPPED ol_ptr;

	while (true)
	{
#ifndef _WIN64
		if (!GetQueuedCompletionStatus(cp, &len, (LPDWORD)&s, &ol_ptr, INFINITE))
#else
		if (!GetQueuedCompletionStatus(cp, &len, (PULONG_PTR)&s, &ol_ptr, INFINITE))
#endif
		{
			if (s != NULL)
				s->Disconnect();
			continue;
		}

		ov = CONTAINING_RECORD(ol_ptr, OverlappedStruct, m_overlap);

		if (ov->m_event == SOCKET_IO_THREAD_SHUTDOWN)
		{
			delete ov;
			return 0;
		}

		if (ov->m_event < NUM_SOCKET_IO_EVENTS)
			ophandlers[ov->m_event](s, len);
	}

	return 0;
}

static bool s_bRunningCleanupThread = true;
unsigned int __stdcall SocketCleanupThread(void * lpParam)
{
	while (s_bRunningCleanupThread)
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

uint32 SocketMgr::s_refs = 0;
FastMutex SocketMgr::s_disconnectionQueueLock;
std::queue<Socket *> SocketMgr::s_disconnectionQueue;

#ifdef USE_STD_THREAD
std::thread SocketMgr::s_hCleanupThread; 
#else
HANDLE SocketMgr::s_hCleanupThread = NULL; 
#endif

SocketMgr::SocketMgr() : m_threadCount(0), m_completionPort(NULL), m_bShutdown(false)
{
	IncRef();
}

void SocketMgr::CreateCompletionPort()
{
	SetCompletionPort(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (ULONG_PTR)0, 0));
}

void SocketMgr::SetupWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,0), &wsaData);
}

void SocketMgr::SpawnWorkerThreads()
{
	if (!m_hThreads.empty())
		return;

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	m_threadCount = si.dwNumberOfProcessors * 2;

	TRACE("SocketMgr - spawning %u worker threads.\n", m_threadCount);
#ifdef USE_STD_THREAD
	for (long x = 0; x < m_threadCount; x++)
		m_hThreads.push_back(std::thread(SocketWorkerThread, this));

	if (!s_hCleanupThread.joinable())
		s_hCleanupThread = std::thread(SocketCleanupThread, static_cast<void *>(NULL));
#else
	DWORD id;
	for (long x = 0; x < m_threadCount; x++)
		m_hThreads.push_back(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&SocketWorkerThread, (LPVOID)this, 0, &id));

	if (s_hCleanupThread == NULL)
		s_hCleanupThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&SocketCleanupThread, NULL, 0, &id);
#endif
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

void HandleShutdown(Socket * s, uint32 len)
{

}

void SocketMgr::ShutdownThreads()
{
	for (long i = 0; i < m_threadCount; i++)
	{
		OverlappedStruct * ov = new OverlappedStruct(SOCKET_IO_THREAD_SHUTDOWN);
		PostQueuedCompletionStatus(m_completionPort, 0, (ULONG_PTR)0, &ov->m_overlap);
	}
}

void SocketMgr::Shutdown()
{
	if (m_bShutdown)
		return;

	ShutdownThreads();

#ifdef USE_STD_THREAD
	foreach (itr, m_hThreads)
		(*itr).join();
#else
	foreach (itr, m_hThreads)
		WaitForSingleObject(*itr, INFINITE);
#endif

	DecRef();
	m_bShutdown = true;
}

SocketMgr::~SocketMgr()
{
	Shutdown();
}

void SocketMgr::CleanupWinsock()
{
#ifdef USE_STD_THREAD
	if (s_hCleanupThread.joinable())
	{
		s_bRunningCleanupThread = false;
		s_hCleanupThread.join();
	}
#else
	if (s_hCleanupThread != NULL)
	{
		s_bRunningCleanupThread = false;
		s_hCleanupThread = NULL;
	}
#endif

	WSACleanup();
}