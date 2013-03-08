#pragma once

class CGameEvent
{
public:
	CGameEvent();
	void RunEvent(CUser* pUser = NULL);

	int  m_sIndex;
	BYTE m_bType;

	int	m_iCond[5], m_iExec[5];
};