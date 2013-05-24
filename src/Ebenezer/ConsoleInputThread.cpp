#include "StdAfx.h"
#include "EbenezerDlg.h"
#include "ConsoleInputThread.h"

#ifdef USE_STD_THREAD
static std::thread s_hConsoleInputThread;
#else
static HANDLE s_hConsoleInputThread = nullptr;
#endif

void StartConsoleInputThread()
{
#ifdef USE_STD_THREAD
	s_hConsoleInputThread = std::thread(ConsoleInputThread, static_cast<void *>(nullptr));
#else
	DWORD dwThread;
	s_hConsoleInputThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)&ConsoleInputThread, nullptr, 0, &dwThread);
#endif
}

void CleanupConsoleInputThread()
{
	// The thread is still pretty primitive; there's no way to signal the thread to end
	// as it's blocking on fgets(). Need to fix this up so that we can wait for the thread.
	// Currently we close the thread when a read error occurs (ctrl-c causes a read error, exiting does not).
#ifdef USE_STD_THREAD
	s_hConsoleInputThread.join();
#else
	TerminateThread(s_hConsoleInputThread, 0);
	WaitForSingleObject(s_hConsoleInputThread, INFINITE);
#endif
}

unsigned int __stdcall ConsoleInputThread(void * lpParam)
{
	size_t i = 0;
	size_t len;
	char cmd[300];

	while (g_bRunning)
	{
		// Read in single line from stdin
		memset(cmd, 0, sizeof(cmd)); 
		if (fgets(cmd, sizeof(cmd), stdin) == nullptr)
		{
			printf("Console input thread closing...\n");
			break;
		}

		if (!g_bRunning)
			break;

		len = strlen(cmd);
		for (i = 0; i < len; i++)
		{
			if (cmd[i] == '\n' || cmd[i] == '\r')
				cmd[i] = '\0';
		}

		g_pMain->HandleConsoleCommand(cmd);
	}

	return 0;
}