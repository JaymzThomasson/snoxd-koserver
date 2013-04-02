#pragma once

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
};