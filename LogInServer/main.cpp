#include "stdafx.h"

#define STR(str) #str
#define STRINGIFY(str) STR(str)

LoginServer * g_pMain;
static HANDLE s_hEvent;
BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType);

int main()
{
	LoginServer pMain;
	SetConsoleTitle("Login server for Knight Online v" STRINGIFY(__VERSION));

	// Override the console handler
	SetConsoleCtrlHandler(_ConsoleHandler, TRUE);

	g_pMain = &pMain;

	// Startup server
	if (!g_pMain->Startup())
	{
		system("pause"); // most users won't be running this via command prompt
		return 1;
	}

	printf("\nServer started up successfully!\n");

	// Create handle, wait until console's signaled as closing
	s_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	WaitForSingleObject(s_hEvent, INFINITE);

	return 0;
}

BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType)
{
	SetEvent(s_hEvent);
	Sleep(10000); // Win7 onwards allows 10 seconds before it'll forcibly terminate
	return TRUE;
}