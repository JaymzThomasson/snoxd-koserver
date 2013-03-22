#include "stdafx.h"
#include <time.h>

time_t UNIXTIME; // update this routinely to avoid the expensive time() syscall!
tm g_localTime;

void StartTimeThread()
{
	DWORD dwThreadId;

	UNIXTIME = time(NULL); // update it first, just to ensure it's set when we need to use it.
	g_localTime = *localtime(&UNIXTIME);

	CreateThread(NULL, NULL, &TimeThread, NULL, NULL, &dwThreadId);
}

DWORD WINAPI TimeThread(LPVOID lpParam)
{
	while (g_bRunning)
	{
		time_t t = time(NULL);
		if (UNIXTIME != t)
		{
			UNIXTIME = t;
			g_localTime = *localtime(&t);
		}

		Sleep(SECOND);	// might need to run it twice a second 
						// to be sure it does in fact update somewhat accurately.. depends on the use cases.
	}

	return 0;
}
