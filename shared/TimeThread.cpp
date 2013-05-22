#include "StdAfx.h"
#include <time.h>
#include "TimeThread.h"

time_t UNIXTIME; // update this routinely to avoid the expensive time() syscall!
tm g_localTime;

#ifdef USE_STD_THREAD
static std::thread s_hTimeThread;
#else
static HANDLE s_hTimeThread = NULL;
#endif

void StartTimeThread()
{
	UNIXTIME = time(NULL); // update it first, just to ensure it's set when we need to use it.
	g_localTime = *localtime(&UNIXTIME);

#ifdef USE_STD_THREAD
	s_hTimeThread = std::thread(TimeThread, static_cast<void *>(NULL));
#else
	DWORD dwThreadId;
	s_hTimeThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)&TimeThread, NULL, NULL, &dwThreadId);
#endif
}

void CleanupTimeThread()
{
#ifdef USE_STD_THREAD
	s_hTimeThread.join();
#else
	WaitForSingleObject(s_hTimeThread, INFINITE);
#endif
}

uint32 __stdcall TimeThread(void * lpParam)
{
	while (g_bRunning)
	{
		time_t t = time(NULL);
		if (UNIXTIME != t)
		{
			UNIXTIME = t;
			g_localTime = *localtime(&t);
		}

		sleep(1000);	// might need to run it twice a second 
						// to be sure it does in fact update somewhat accurately.. depends on the use cases.
	}

	return 0;
}
