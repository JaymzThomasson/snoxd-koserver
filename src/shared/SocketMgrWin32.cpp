/*
 * Multiplatform Async Network Library
 * Copyright (c) 2007 Burlex
 * Hacked up to no good end by twostars (if you're looking for someone to blame!).
 *
 * SocketMgr - iocp-based SocketMgr for windows.
 *
 */

#include "stdafx.h"
#ifdef CONFIG_USE_IOCP

uint32 THREADCALL SocketMgr::SocketWorkerThread(void * lpParam)
{
	SocketMgr *socketMgr = (SocketMgr *)lpParam;
	HANDLE cp = socketMgr->GetCompletionPort();
	DWORD len;
	Socket * s = nullptr;
	OverlappedStruct * ov = nullptr;
	LPOVERLAPPED ol_ptr;

	while (socketMgr->m_bWorkerThreadsActive)
	{
#ifndef _WIN64
		if (!GetQueuedCompletionStatus(cp, &len, (LPDWORD)&s, &ol_ptr, INFINITE))
#else
		if (!GetQueuedCompletionStatus(cp, &len, (PULONG_PTR)&s, &ol_ptr, INFINITE))
#endif
		{
			if (s != nullptr)
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

uint32 SocketMgr::GetPreferredThreadCount()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	return si.dwNumberOfProcessors * 2;
}

void SocketMgr::CreateCompletionPort()
{
	SetCompletionPort(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, (ULONG_PTR)0, 0));
}

void SocketMgr::SetupWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,0), &wsaData);
}

void SocketMgr::CleanupWinsock() 
{
	WSACleanup(); 
}

#endif