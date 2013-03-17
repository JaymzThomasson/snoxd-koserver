#include "stdafx.h"
#include "NpcThread.h"
#include "ServerDlg.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define DELAY				250

DWORD	g_dwLastTimeCount	= GetTickCount();
DWORD	g_dwCurrTimeCount	= GetTickCount();

//////////////////////////////////////////////////////////////////////
// NPC Thread Callback Function
//
UINT NpcThreadProc(LPVOID pParam /* NPC_THREAD_INFO ptr */)
{
	NPC_THREAD_INFO*	pInfo	= (NPC_THREAD_INFO*)pParam;
	CNpc*				pNpc	= NULL;

	int					i			= 0;
	DWORD				dwDiffTime	= 0;
	DWORD				dwTickTime  = 0;
	srand( (unsigned)time( NULL ) );
	myrand( 1, 10000 ); myrand( 1, 10000 );

	float  fTime2 = 0.0f;
	float  fTime3 = 0.0f;
	int    duration_damage=0;

	if(!pInfo) return 0;

	while(!g_bNpcExit)
	{
		fTime2 = TimeGet();

		for(i = 0; i < NPC_NUM; i++)
		{
			pNpc = pInfo->pNpc[i];
			if( !pNpc ) continue;
			//if((pNpc->m_proto->m_tNpcType == NPCTYPE_DOOR || pNpc->m_proto->m_tNpcType == NPCTYPE_ARTIFACT || pNpc->m_proto->m_tNpcType == NPCTYPE_PHOENIX_GATE || pNpc->m_proto->m_tNpcType == NPCTYPE_GATE_LEVER) && !pNpc->m_bFirstLive) continue;
			//if( pNpc->m_bFirstLive ) continue;
			if( pNpc->m_sNid < 0 ) continue;		// 잘못된 몬스터 (임시코드 2002.03.24)

			fTime3 = fTime2 - pNpc->m_fDelayTime;
			dwTickTime = (DWORD)(fTime3 * 1000);

			//if(i==0)
			//TRACE("thread time = %.2f, %.2f, %.2f, delay=%d, state=%d, nid=%d\n", pNpc->m_fDelayTime, fTime2, fTime3, dwTickTime, pNpc->m_NpcState, pNpc->m_sNid+NPC_BAND);

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
			
			fTime3 = fTime2 - pNpc->m_fHPChangeTime;
			dwTickTime = (DWORD)(fTime3 * 1000);

			if( 10000 < dwTickTime )	{	// 10초마다 HP를 회복 시켜준다
				pNpc->HpChange();
			}

			pNpc->DurationMagic_4(fTime2);		// 마법 처리...
			pNpc->DurationMagic_3(fTime2);		// 지속마법..

			switch(pNpc->m_NpcState)
			{
			case NPC_LIVE:					// 방금 살아난 경우
				pNpc->NpcLive();
				break;

			case NPC_STANDING:						// 하는 일 없이 서있는 경우
				pNpc->NpcStanding();
				break;
			
			case NPC_MOVING:
				pNpc->NpcMoving();
				break;

			case NPC_ATTACKING:
				pNpc->NpcAttacking();
				break;

			case NPC_TRACING:
				pNpc->NpcTracing();
				break;

			case NPC_FIGHTING:
				pNpc->NpcFighting();
				break;

			case NPC_BACK:
				pNpc->NpcBack();
				break;

			case NPC_STRATEGY:
				break;

			case NPC_DEAD:
				pNpc->m_NpcState = NPC_LIVE;
				break;
			case NPC_SLEEPING:
				pNpc->NpcSleeping();
				break;
			case NPC_FAINTING:
				pNpc->NpcFainting(fTime2);
				break;
			case NPC_HEALING:
				pNpc->NpcHealing();
				break;

			default:
				break;
			}
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
	float  fCurrentTime = 0.0f;
	int j=0;

	while (!g_bNpcExit)
	{
		fCurrentTime = TimeGet();
		foreach_stlmap (itr, g_pMain.g_arZone)
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
				pRoom->MainRoom( fCurrentTime );
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

	memset(&m_pNpc, 0, sizeof(m_pNpc));
}

CNpcThread::~CNpcThread()
{
/*	for( int i = 0; i < NPC_NUM; i++ )
	{
		if(m_pNpc[i])
		{
			delete m_pNpc[i];
			m_pNpc[i] = NULL;
		}
	}	*/
}

void CNpcThread::InitThreadInfo(HWND hwnd)
{
	m_ThreadInfo.hWndMsg	= hwnd;
}