#include "SocketMgr.h"

DWORD WINAPI SocketWorkerThread(LPVOID lpParam)
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

uint32 SocketMgr::s_refs = 0;

SocketMgr::SocketMgr() : m_hThreads(NULL), m_threadCount(0), m_completionPort(NULL)
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
	if (m_hThreads != NULL)
		return;

	DWORD id;
	SYSTEM_INFO si;
	GetSystemInfo(&si);

//#ifdef _DEBUG
//	m_threadCount = 1;
//#else
	m_threadCount = si.dwNumberOfProcessors * 2;
//#endif

	TRACE("SocketMgr - spawning %u worker threads.\n", m_threadCount);
	m_hThreads = new HANDLE[m_threadCount];
	for (long x = 0; x < m_threadCount; x++)
		m_hThreads[x] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&SocketWorkerThread, (LPVOID)this, 0, &id);
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

SocketMgr::~SocketMgr()
{
	ShutdownThreads();

	if (m_hThreads != NULL)
		delete [] m_hThreads;

	DecRef();
}

void SocketMgr::CleanupWinsock()
{
	WSACleanup();
}