#include "stdafx.h"

#define STR(str) #str
#define STRINGIFY(str) STR(str)

CEbenezerDlg g_pMain;
BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType);
static DWORD s_dwMainThreadID;

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

	s_dwMainThreadID = GetCurrentThreadId();

	// Standard mesage pump purely for OnTimer()'s sake
	while (GetMessage(&msg, NULL, 0, 0)
		&& msg.message != WM_QUIT)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType)
{
	PostThreadMessage(s_dwMainThreadID, WM_QUIT, 0, 0);
	Sleep(10000); // Win7 onwards allows 10 seconds before it'll forcibly terminate
	return TRUE;
}