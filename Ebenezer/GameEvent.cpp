#include "stdafx.h"
#include "User.h"
#include "GameEvent.h"

CGameEvent::CGameEvent() : m_bType(0)
{
	memset(&m_iCond, 0, sizeof(m_iCond));
	memset(&m_iExec, 0, sizeof(m_iExec));
}

void CGameEvent::RunEvent(CUser *pUser)
{
	switch (m_bType)
	{
		case ZONE_CHANGE:
			if (pUser->m_bWarp)
				break;
			pUser->ZoneChange( m_iExec[0], (float)m_iExec[1], (float)m_iExec[2] );
			break;
		case ZONE_TRAP_DEAD:
		//	TRACE("&&& User - zone trap dead ,, name=%s\n", pUser->m_pUserData.m_id);
		//	pUser->Dead();
			break;	
		case ZONE_TRAP_AREA:
			pUser->TrapProcess();
			break;
	}
}
