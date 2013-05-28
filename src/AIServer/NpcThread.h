#pragma once

uint32 THREADCALL NpcThreadProc(void * lpParam /* CNpcThread ptr */);
uint32 THREADCALL ZoneEventThreadProc(void * lpParam /* = nullptr */);

typedef std::set<CNpc *> NpcSet;

class CNpc;
class CNpcThread  
{
public:
	CNpcThread();
	virtual ~CNpcThread();

public:
	void InitThreadInfo(HWND hwnd);
	NpcSet m_pNpcs;

	HWND	hWndMsg;

	Thread m_thread;
};
