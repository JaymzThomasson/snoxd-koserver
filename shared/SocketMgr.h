#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_MAXVER
#include <afxwin.h>
#endif

#include <set>
#include "Socket.h"
#include "ListenSocket.h"

class SocketMgr
{
public:
	SocketMgr();

	__forceinline HANDLE GetCompletionPort() { return m_completionPort; }
	__forceinline void SetCompletionPort(HANDLE cp) { m_completionPort = cp; }

	void CreateCompletionPort();
	void SpawnWorkerThreads();
	void ShutdownThreads();

	virtual Socket *AssignSocket(SOCKET socket) = 0;
	virtual void OnDisconnect(Socket *pSock) = 0;

	virtual ~SocketMgr();

private:
	HANDLE m_completionPort;
	HANDLE *m_hThreads;
	long m_threadCount;

	static void SetupWinsock();
	static void CleanupWinsock();

	__forceinline void IncRef() { if (s_refs++ == 0) SetupWinsock(); }
	__forceinline void DecRef() { if (--s_refs == 0) CleanupWinsock(); }

	static uint32 s_refs; // reference counter (one app can hold multiple socket manager instances)
};

typedef void(*OperationHandler)(Socket * s, uint32 len);

DWORD WINAPI SocketWorkerThread(LPVOID lp);

void HandleReadComplete(Socket * s, uint32 len);
void HandleWriteComplete(Socket * s, uint32 len);
void HandleShutdown(Socket * s, uint32 len);

static OperationHandler ophandlers[] =
{
	&HandleReadComplete,
	&HandleWriteComplete,
	&HandleShutdown
};
