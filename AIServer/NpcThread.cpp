#include "stdafx.h"
#include "NpcThread.h"
#include "Npc.h"
#define DELAY				250

//////////////////////////////////////////////////////////////////////
// NPC Thread Callback Function
//
UINT NpcThreadProc(LPVOID pParam /* CNpcThread ptr */)
{
	CNpcThread*	pInfo	= (CNpcThread *)pParam;
	CNpc*				pNpc	= NULL;

	int					i			= 0;
	DWORD				dwDiffTime	= 0;
	time_t				dwTickTime  = 0;
	srand((uint32)UNIXTIME);
	myrand( 1, 10000 ); myrand( 1, 10000 );

	time_t fTime2 = 0;
	int    duration_damage=0;

	if(!pInfo) return 0;

	while(!g_bNpcExit)
	{
		fTime2 = getMSTime();

		foreach (itr, pInfo->m_pNpcs)
		{
			pNpc = *itr;

			//if((pNpc->m_proto->m_tNpcType == NPCTYPE_DOOR || pNpc->m_proto->m_tNpcType == NPCTYPE_ARTIFACT || pNpc->m_proto->m_tNpcType == NPCTYPE_PHOENIX_GATE || pNpc->m_proto->m_tNpcType == NPCTYPE_GATE_LEVER) && !pNpc->m_bFirstLive) continue;
			//if( pNpc->m_bFirstLive ) continue;
			if( pNpc->m_sNid < 0 ) continue;		// 잘못된 몬스터 (임시코드 2002.03.24)

			dwTickTime = fTime2 - pNpc->m_fDelayTime;

			if(pNpc->m_Delay > (int)dwTickTime && !pNpc->m_bFirstLive && pNpc->m_Delay != 0) 
			{
				if(pNpc->m_Delay < 0) pNpc->m_Delay = 0;

				//적발견시... (2002. 04.23수정, 부하줄이기)
				if(pNpc->m_NpcState == NPC_STANDING && pNpc->CheckFindEnermy() )	{
					if( pNpc->FindEnemy() )	{
						pNpc->m_NpcState = NPC_ATTACKING;
						pNpc->m_Delay = 0;
					}
				}
				continue;
			}	
			
			dwTickTime = fTime2 - pNpc->m_fHPChangeTime;
			if( 10000 < dwTickTime )	{	// 10초마다 HP를 회복 시켜준다
				pNpc->HpChange();
			}

			pNpc->DurationMagic_4();		// 마법 처리...
			pNpc->DurationMagic_3();		// 지속마법..

			uint8 bState = pNpc->m_NpcState;
			time_t tDelay = -1;
			switch (bState)
			{
			case NPC_LIVE:					// 방금 살아난 경우
				tDelay = pNpc->NpcLive();
				break;

			case NPC_STANDING:						// 하는 일 없이 서있는 경우
				tDelay = pNpc->NpcStanding();
				break;
			
			case NPC_MOVING:
				tDelay = pNpc->NpcMoving();
				break;

			case NPC_ATTACKING:
				tDelay = pNpc->NpcAttacking();
				break;

			case NPC_TRACING:
				tDelay = pNpc->NpcTracing();
				break;

			case NPC_FIGHTING:
				tDelay = pNpc->NpcFighting();
				break;

			case NPC_BACK:
				tDelay = pNpc->NpcBack();
				break;

			case NPC_STRATEGY:
				break;

			case NPC_DEAD:
				pNpc->m_NpcState = NPC_LIVE;
				break;

			case NPC_SLEEPING:
				tDelay = pNpc->NpcSleeping();
				break;

			case NPC_FAINTING:
				tDelay = pNpc->NpcFainting();
				break;

			case NPC_HEALING:
				tDelay = pNpc->NpcHealing();
				break;
			}

			// This may not be necessary, but it keeps behaviour identical.
			if (bState != NPC_LIVE && bState != NPC_DEAD
				&& pNpc->m_NpcState != NPC_DEAD)
				pNpc->m_fDelayTime = getMSTime();

			if (tDelay >= 0)
				pNpc->m_Delay = tDelay;
		}	

		Sleep(100);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// NPC Thread Callback Function
//
UINT ZoneEventThreadProc(LPVOID pParam /* = NULL */)
{
	CServerDlg* m_pMain = (CServerDlg*) pParam;
	int j=0;

	while (!g_bNpcExit)
	{
		foreach_stlmap (itr, g_pMain->g_arZone)
		{
			MAP *pMap = itr->second;
			if (pMap == NULL
				|| pMap->m_byRoomEvent == 0
				|| pMap->IsRoomStatusCheck()) 
				continue;

			for (j = 1; j < pMap->m_arRoomEventArray.GetSize() + 1; j++)
			{
				CRoomEvent* pRoom = pMap->m_arRoomEventArray.GetData(j);
				if( !pRoom ) continue;
				if( pRoom->m_byStatus == 1 || pRoom->m_byStatus == 3 )   continue; // 1:init, 2:progress, 3:clear
				// 여기서 처리하는 로직...
				pRoom->MainRoom();
			}
		}
		Sleep(1000);	// 1초당 한번
	}

	return 0;
}

CNpcThread::CNpcThread()
{
	m_hThread = NULL;
	m_sThreadNumber = -1;
}

CNpcThread::~CNpcThread()
{
	foreach (itr, m_pNpcs)
		delete *itr;

	m_pNpcs.clear();
}

void CNpcThread::InitThreadInfo(HWND hwnd)
{
	hWndMsg	= hwnd;
}