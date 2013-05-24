#pragma once

void StartConsoleInputThread();
void CleanupConsoleInputThread();
unsigned int __stdcall ConsoleInputThread(void * lpParam);