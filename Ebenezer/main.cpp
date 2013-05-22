#include "stdafx.h"
#include "EbenezerDlg.h"
#include "ConsoleInputThread.h"

CEbenezerDlg * g_pMain;

#ifdef WIN32
BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType);
static DWORD s_dwMainThreadID;
#endif

bool g_bRunning = true;

int main()
{
	CEbenezerDlg pMain;
	MSG msg;

	SetConsoleTitle("Game server for Knight Online v" STRINGIFY(__VERSION));

#ifdef WIN32
	// Override the console handler
	s_dwMainThreadID = GetCurrentThreadId();
	SetConsoleCtrlHandler(_ConsoleHandler, TRUE);
#else
	/* TO-DO: Signals */
#endif

	// Start up the time updater thread
	StartTimeThread();

	// Start up the console input thread
	StartConsoleInputThread();

	g_pMain = &pMain;

	// Start up server
	if (!pMain.Startup())
	{
		system("pause"); // most users won't be running this via command prompt
		return 1;
	}

	printf("\nServer started up successfully!\n");

	// Standard mesage pump for OnTimer() (which we won't need soon enough)
	while (GetMessage(&msg, NULL, 0, 0)
		&& msg.message != WM_QUIT)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// This seems redundant, but it's not. 
	// We still have the destructor for the dialog instance, which allows time for threads to properly cleanup.
	g_bRunning = false; 

	return 0;
}

#ifdef WIN32
BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType)
{
	PostThreadMessage(s_dwMainThreadID, WM_QUIT, 0, 0);
	sleep(10000); // Win7 onwards allows 10 seconds before it'll forcibly terminate
	return TRUE;
}
#endif