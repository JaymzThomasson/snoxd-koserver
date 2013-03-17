#include "stdafx.h"

#define STR(str) #str
#define STRINGIFY(str) STR(str)

CEbenezerDlg g_pMain;
static bool s_bRunning = true;
BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType);

int main()
{
	SetConsoleTitle("Game server for Knight Online v" STRINGIFY(__VERSION));

	// Override the console handler
	SetConsoleCtrlHandler(_ConsoleHandler, TRUE);

	// Startup server
	if (!g_pMain.Startup())
	{
		system("pause"); // most users won't be running this via command prompt
		return 1;
	}

	printf("\nServer started up successfully!\n");

	// for OnTimer() (which we won't need soon enough)
	// we need to remember to dispatch messages
    MSG msg;

	// Standard mesage pump purely for OnTimer()'s sake
	while (s_bRunning)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}

BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType)
{
	s_bRunning = false;
	Sleep(10000); // Win7 onwards allows 10 seconds before it'll forcibly terminate
	return TRUE;
}