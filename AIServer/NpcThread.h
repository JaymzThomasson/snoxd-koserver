#pragma once

UINT NpcThreadProc(LPVOID pParam /* CNpcThread ptr */);
UINT ZoneEventThreadProc(LPVOID pParam /* = NULL */);

class CNpc;
class CNpcThread  
{
public:
	CNpcThread();
	virtual ~CNpcThread();

public:
	void InitThreadInfo(HWND hwnd);
	CNpc*	m_pNpc[NPC_NUM];

	CNpc*	pNpc[NPC_NUM];
	BYTE	m_byNpcUsed[NPC_NUM];
	HWND	hWndMsg;

	HANDLE			m_hThread;
	short m_sThreadNumber;					// thread number ,, test

};
