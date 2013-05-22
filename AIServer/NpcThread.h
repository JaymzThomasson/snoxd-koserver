#pragma once

UINT NpcThreadProc(LPVOID pParam /* CNpcThread ptr */);
UINT ZoneEventThreadProc(LPVOID pParam /* = NULL */);

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

#ifdef USE_STD_THREAD
	std::thread m_hThread;
#else
	HANDLE m_hThread;
#endif

	short m_sThreadNumber;					// thread number ,, test
};
