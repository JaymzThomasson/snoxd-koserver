#pragma once

void StartTimeThread();
DWORD WINAPI TimeThread(LPVOID lpParam);

extern bool g_bRunning;
extern time_t UNIXTIME;
extern tm g_localTime;