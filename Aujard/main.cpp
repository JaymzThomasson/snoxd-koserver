#include "stdafx.h"

#define STR(str) #str
#define STRINGIFY(str) STR(str)

CAujardDlg g_pMain;
static bool s_bRunning = true;
BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType);

int main()
{
	SetConsoleTitle("Aujard for Knight Online v" STRINGIFY(__VERSION));

	// Override the console handle
	SetConsoleCtrlHandler(_ConsoleHandler, TRUE);

	if (!g_pMain.Startup())
	{
		system("pause"); // most users won't be running this via command prompt
		return 1;
	}

	printf("\nStarted up successfully!\n");

	// for OnTimer() (which we won't need soon enough)
	// we need to remember to dispatch messages
    MSG msg;

	// Standard mesage pump purely for OnTimer()'s sake
	while (s_bRunning && GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType)
{
	s_bRunning = false;
	Sleep(10000); // Win7 onwards allows 10 seconds before it'll forcibly terminate
	return TRUE;
}