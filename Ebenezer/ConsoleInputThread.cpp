#include "StdAfx.h"
#include "EbenezerDlg.h"
#include "ConsoleInputThread.h"

void StartConsoleInputThread()
{
	DWORD dwThread;
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)&ConsoleInputThread, NULL, NULL, &dwThread);
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