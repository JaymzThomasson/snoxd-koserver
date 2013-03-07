#pragma once

UINT NpcThreadProc(LPVOID pParam /* NPC_THREAD_INFO ptr */);
UINT ZoneEventThreadProc(LPVOID pParam /* = NULL */);

class CNpc;

typedef struct _NPC_THREAD_INFO
{
	CNpc*	pNpc[NPC_NUM];
	BYTE	m_byNpcUsed[NPC_NUM];
	HWND	hWndMsg;
} NPC_THREAD_INFO;


class CNpcThread  
{
public:
	CNpcThread();
	virtual ~CNpcThread();

public:
	void InitThreadInfo(HWND hwnd);
	CNpc*	m_pNpc[NPC_NUM];

	NPC_THREAD_INFO		m_ThreadInfo;
	CWinThread*			m_pThread;
	short m_sThreadNumber;					// thread number ,, test

};
