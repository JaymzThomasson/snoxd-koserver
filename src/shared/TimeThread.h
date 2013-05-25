#pragma once

void StartTimeThread();
void CleanupTimeThread();
	
uint32 THREADCALL TimeThread(void * lpParam);

extern bool g_bRunning;
extern time_t UNIXTIME;
extern tm g_localTime;