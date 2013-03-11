#pragma once

class CGameEvent
{
public:
	CGameEvent();
	void RunEvent(CUser* pUser = NULL);

	uint16	m_sIndex;
	uint8	m_bType;

	int32	m_iCond[5], m_iExec[5];
};