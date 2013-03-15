#pragma once

#include <queue>

class DatabaseThread
{
public:
	// Startup the database threads
	static void Startup(DWORD dwThreads);

	// Add to the queue and notify threads of activity.
	static void AddRequest(Packet * pkt);

	// Main thread procedure
	static BOOL WINAPI ThreadProc(LPVOID lpParam);

	// Shutdown threads.
	static void Shutdown();

	static std::queue<Packet *> _queue;
	static FastMutex _lock;
	static bool _running;
	static HANDLE s_hEvent;
	static HANDLE *s_hThreads;
	static DWORD s_dwThreads;
};