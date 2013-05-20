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

	HANDLE			m_hThread;
	short m_sThreadNumber;					// thread number ,, test

};
