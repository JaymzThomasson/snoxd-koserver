#include "StdAfx.h"
#include "EbenezerDlg.h"
#include "ConsoleInputThread.h"

#ifdef USE_STD_THREAD
static std::thread s_hConsoleInputThread;
#else
static HANDLE s_hConsoleInputThread = NULL;
#endif

void StartConsoleInputThread()
{
#ifdef USE_STD_THREAD
	s_hConsoleInputThread = std::thread(ConsoleInputThread, static_cast<void *>(NULL));
#else
	DWORD dwThread;
	s_hConsoleInputThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)&ConsoleInputThread, NULL, NULL, &dwThread);
#endif
}

void CleanupConsoleInputThread()
{
#ifdef USE_STD_THREAD
	s_hConsoleInputThread.join();
#else
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
		if (fgets(cmd, sizeof(cmd), stdin) == NULL)
			continue;

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