#pragma once

void StartTimeThread();
void CleanupTimeThread();
	
uint32 __stdcall TimeThread(void * lpParam);

extern bool g_bRunning;
extern time_t UNIXTIME;
extern tm g_localTime;