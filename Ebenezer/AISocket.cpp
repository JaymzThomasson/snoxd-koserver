// AISocket.cpp: implementation of the CAISocket class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AISocket.h"
#include "EbenezerDlg.h"
#include "define.h"
#include "AiPacket.h"
#include "Npc.h"
#include "user.h"
#include "Map.h"
#include "../shared/lzf.h"
#include "../shared/crc32.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern CRITICAL_SECTION g_LogFile_critical;

CAISocket::CAISocket(int zonenum)
{
	m_iZoneNum = zonenum;
}

CAISocket::~CAISocket()
{

}

void CAISocket::Initialize()
{
	m_pMain = (CEbenezerDlg*)AfxGetApp()->GetMainWnd();
	m_MagicProcess.m_pMain = m_pMain;
}

void CAISocket::HandlePacket(char *pBuf, int len)
{
	Parsing(len, pBuf);
}

void CAISocket::Parsing( int len, char* pData )
{
	int index = 0;

	BYTE command = GetByte(pData, index);

	//TRACE("CAISocket::Parsing - command=%d, length = %d\n", command, len);

	switch( command )
	{
		case AG_CHECK_ALIVE_REQ:
			RecvCheckAlive(pData+index);
			break;
		case AI_SERVER_CONNECT:
			LoginProcess(pData+index);
			break;
		case AG_SERVER_INFO:
			RecvServerInfo(pData+index);
			break;
		case NPC_INFO_ALL:
			RecvNpcInfoAll(pData+index);
			break;
		case MOVE_RESULT:
			RecvNpcMoveResult(pData+index);
			break;
		case MOVE_END_RESULT:
			break;
		case AG_ATTACK_RESULT:
			RecvNpcAttack(pData+index);
			break;
		case AG_MAGIC_ATTACK_RESULT:
			RecvMagicAttackResult(pData+index);
			break;
		case AG_NPC_INFO:
			RecvNpcInfo(pData+index);
			break;
		case AG_USER_SET_HP:
			RecvUserHP(pData+index);
			break;
		case AG_USER_EXP:
			RecvUserExp(pData+index);
			break;
		case AG_SYSTEM_MSG:
			RecvSystemMsg(pData+index);
			break;
		case AG_NPC_GIVE_ITEM:
			RecvNpcGiveItem(pData+index);
			break;
		case AG_USER_FAIL:
			RecvUserFail(pData+index);
			break;
		case AG_NPC_GATE_DESTORY:
			RecvGateDestory(pData+index);
			break;
		case AG_DEAD:
			RecvNpcDead(pData+index);
			break;
		case AG_NPC_INOUT:
			RecvNpcInOut(pData+index);
			break;
		case AG_BATTLE_EVENT:
			RecvBattleEvent(pData+index);
			break;
		case AG_NPC_EVENT_ITEM:
			RecvNpcEventItem(pData+index);
			break;
		case AG_NPC_GATE_OPEN:
			RecvGateOpen(pData+index);
			break;
	}
}

void CAISocket::CloseProcess()
{
	CString logstr;
	CTime time = CTime::GetCurrentTime();
	logstr.Format("*** CloseProcess - socketID=%d...  ***  %d-%d-%d, %d:%d]\r\n", m_Sid, time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute() );
	LogFileWrite( logstr );

	Initialize();

	CIOCPSocket2::CloseProcess();
}

// sungyong 2002.05.23
void CAISocket::LoginProcess( char* pBuf )
{
	int index = 0;
	float fReConnectEndTime = 0.0f;
	BYTE ver = GetByte(pBuf, index);
	BYTE byReConnect = GetByte(pBuf, index);	// 0 : ó������, 1 : ������

	if(ver == -1)	// zone Ʋ���� ���� 
	{
		AfxMessageBox(_T("AI Server Version Fail!!"));
	}
	else			// Ʋ���� ���� 
	{
		DEBUG_LOG("AI Server Connect Success!! - %d", ver);
		if( byReConnect == 0 )	{
			m_pMain->m_sSocketCount++;
			if(m_pMain->m_sSocketCount == MAX_AI_SOCKET)	{
				m_pMain->m_bServerCheckFlag = TRUE;
				m_pMain->m_sSocketCount = 0;
				TRACE("*** ������ ������ ���� �غ��ܰ� ****\n");
				m_pMain->SendAllUserInfo();
			}
		}
		else if( byReConnect == 1 )	{
			if(m_pMain->m_sReSocketCount == 0)
			m_pMain->m_fReConnectStart = TimeGet();
			m_pMain->m_sReSocketCount++;
			TRACE("**** ReConnect - zone=%d,  socket = %d ****\n ", ver, m_pMain->m_sReSocketCount);
			fReConnectEndTime = TimeGet();
			if(fReConnectEndTime > m_pMain->m_fReConnectStart+120)	{	// 2�оȿ� ���� ������ �����ƴٸ�...
				TRACE("**** ReConnect - �ܼ��� ����... socket = %d ****\n ", m_pMain->m_sReSocketCount);
				m_pMain->m_sReSocketCount = 0;
				m_pMain->m_fReConnectStart = 0.0f;
			}

			if(m_pMain->m_sReSocketCount == MAX_AI_SOCKET)	{
				fReConnectEndTime = TimeGet();
				if(fReConnectEndTime < m_pMain->m_fReConnectStart+60)	{	// 1�оȿ� ���� ������ �����ƴٸ�...
					TRACE("**** ReConnect - ���� ���� �ʱ�ȭ �Ϸ� socket = %d ****\n ", m_pMain->m_sReSocketCount);
					m_pMain->m_bServerCheckFlag = TRUE;
					m_pMain->m_sReSocketCount = 0;
					TRACE("*** ������ ������ ���� �غ��ܰ� ****\n");
					m_pMain->SendAllUserInfo();
				}
				else	{								// �ϳ��� ������ �����̶���...
					m_pMain->m_sReSocketCount = 0;
					m_pMain->m_fReConnectStart = 0.0f;
				}
			}
		}
	}
}

void CAISocket::RecvServerInfo(char* pBuf)
{
	int index = 0;
	BYTE type = GetByte(pBuf, index);
	BYTE byZone = GetByte(pBuf, index);
	CString logstr;
	int size = m_pMain->m_ZoneArray.GetSize();

	if(type == SERVER_INFO_START)	{	
		TRACE("�������� ������ �ޱ� �����մϴ�..%d\n", byZone);
	}
	else if(type == SERVER_INFO_END)	{
		short sTotalMonster = 0;
		sTotalMonster = GetShort(pBuf, index);
		DEBUG_LOG("All Monster info Received!!");

		m_pMain->m_sZoneCount++;

		TRACE("�������� ������ �� �޾���....%d, total=%d, socketcount=%d\n", byZone, sTotalMonster, m_pMain->m_sZoneCount);

		if(m_pMain->m_sZoneCount == size)	{
			if(m_pMain->m_bFirstServerFlag == FALSE)	{
				m_pMain->UserAcceptThread();
				TRACE("+++ �������� ���� ������ �� �޾���, User AcceptThread Start ....%d, socketcount=%d\n", byZone, m_pMain->m_sZoneCount);
			}
			m_pMain->m_sZoneCount = 0;
			m_pMain->m_bFirstServerFlag = TRUE;
			m_pMain->m_bPointCheckFlag = TRUE;
			TRACE("�������� ���� ������ �� �޾���, User AcceptThread Start ....%d, socketcount=%d\n", byZone, m_pMain->m_sZoneCount);
			// ���⿡�� Event Monster�� �����͸� �̸� �Ҵ� �ϵ��� ����~~
			//InitEventMonster( sTotalMonster );
		}
	}
}

// ai server�� ó�� ���ӽ� npc�� ���� ������ �޾ƿ´�..
void CAISocket::RecvNpcInfoAll(char* pBuf)
{
	int index = 0;
	BYTE		byCount = 0;			// ������
	BYTE        byType;				// 0:ó���� �������� �ʴ� ������, 1:����
	short		nid;				// NPC index
	short		sid;				// NPC index
	BYTE		bZone;				// Current zone number
	short		sPid;				// NPC Picture Number
	short		sSize = 100;				// NPC Size
	int			iweapon_1;
	int			iweapon_2;
	char		szName[MAX_NPC_SIZE+1];		// NPC Name
	BYTE		byGroup;		// �Ҽ� ����
	BYTE		byLevel;		// level
	float		fPosX;			// X Position
	float		fPosZ;			// Z Position
	float		fPosY;			// Y Position
	BYTE		byDirection;	// 
	BYTE		tNpcType;		// 00	: Monster
								// 01	: NPC
	int		iSellingGroup;
  	int		nMaxHP;			// �ִ� HP
	int		nHP;			// ���� HP
	BYTE		byGateOpen;		// �����ϰ��� ������ ���� ����
	short		sHitRate;
	BYTE		byObjectType;	// ���� : 0, Ư�� : 1

	byCount = GetByte(pBuf, index);

	for(int i=0; i<byCount; i++)
	{
		byType = GetByte(pBuf, index);
		nid = GetShort(pBuf, index);
		sid = GetShort(pBuf, index);
		sPid = GetShort(pBuf, index);
		sSize = GetShort(pBuf, index);
		iweapon_1 = GetDWORD(pBuf, index);
		iweapon_2 = GetDWORD(pBuf, index);
		bZone = GetByte(pBuf, index);
		int nLength = GetVarString(szName, pBuf, sizeof(BYTE), index);
		byGroup = GetByte(pBuf, index);
		byLevel  = GetByte(pBuf, index);
		fPosX = Getfloat(pBuf, index);
		fPosZ = Getfloat(pBuf, index);
		fPosY = Getfloat(pBuf, index);
		byDirection = GetByte(pBuf, index);
		tNpcType = GetByte(pBuf, index);
		iSellingGroup = GetDWORD(pBuf, index);
		nMaxHP = GetDWORD(pBuf, index);
		nHP = GetDWORD(pBuf, index);
		byGateOpen = GetByte(pBuf, index);
		sHitRate = GetShort(pBuf, index);
		byObjectType = GetByte(pBuf, index);

		//TRACE("RecvNpcInfoAll  : nid=%d, szName=%s, count=%d\n", nid, szName, byCount);

		if(nLength < 0 || nLength > MAX_NPC_SIZE)	{
			TRACE("#### RecvNpcInfoAll Fail : szName=%s\n", szName);
			continue;		// �߸��� monster ���̵� 
		}

		//TRACE("Recv --> NpcUserInfo : uid = %d, x=%f, z=%f.. \n", nid, fPosX, fPosZ);

		CNpc* pNpc = NULL;
		pNpc = new CNpc;
		if(pNpc == NULL)	{ 
			TRACE("#### Recv --> NpcUserInfoAll POINT Fail: uid=%d, sid=%d, name=%s, zone=%d, x=%f, z=%f.. \n", nid, sPid, szName, bZone, fPosX, fPosZ);
			continue;
		}
		pNpc->Initialize();

		pNpc->m_sNid = nid;
		pNpc->m_sSid = sid;
		pNpc->m_sPid = sPid;
		pNpc->m_sSize = sSize;
		pNpc->m_iWeapon_1 = iweapon_1;
		pNpc->m_iWeapon_2 = iweapon_2;
		strcpy(pNpc->m_strName, szName);
		pNpc->m_byGroup = byGroup;
		pNpc->m_byLevel = byLevel;
		pNpc->m_bCurZone = bZone;
		pNpc->m_pMap = m_pMain->GetZoneByID(bZone);
		pNpc->m_fCurX = fPosX;
		pNpc->m_fCurZ = fPosZ;
		pNpc->m_fCurY = fPosY;
		pNpc->m_byDirection = byDirection;
		pNpc->m_NpcState = NPC_LIVE;
		pNpc->m_tNpcType = tNpcType;
		pNpc->m_iSellingGroup = iSellingGroup;
		pNpc->m_iMaxHP = nMaxHP;
		pNpc->m_iHP = nHP;
		pNpc->m_byGateOpen = byGateOpen;
		pNpc->m_sHitRate = sHitRate;
		pNpc->m_byObjectType = byObjectType;
		pNpc->m_NpcState = NPC_LIVE;

		int nRegX = (int)fPosX / VIEW_DISTANCE;
		int nRegZ = (int)fPosZ / VIEW_DISTANCE;

		pNpc->m_sRegion_X = nRegX;
		pNpc->m_sRegion_Z = nRegZ;

		if (pNpc->GetMap() == NULL)
		{
			delete pNpc;
			pNpc = NULL;
			continue;
		}

		_OBJECT_EVENT* pEvent = NULL;
		if (pNpc->m_byObjectType == SPECIAL_OBJECT)
		{
			pEvent = pNpc->GetMap()->GetObjectEvent(pNpc->m_sSid);
			if( pEvent )	pEvent->byLife = 1;
		}

	//	TRACE("Recv --> NpcUserInfoAll : uid=%d, sid=%d, name=%s, x=%f, z=%f. gate=%d, objecttype=%d \n", nid, sPid, szName, fPosX, fPosZ, byGateOpen, byObjectType);

		if( !m_pMain->m_arNpcArray.PutData( pNpc->m_sNid, pNpc) ) {
			TRACE("Npc PutData Fail - %d\n", pNpc->m_sNid);
			delete pNpc;
			pNpc = NULL;
			continue;
		}

		if( byType == 0 )	{
			TRACE("Recv --> NpcUserInfoAll : �����ϸ� �ȵſ�,, uid=%d, sid=%d, name=%s\n", nid, sPid, szName);
			continue;		// region�� �������� ����...
		}

		pNpc->GetMap()->RegionNpcAdd(pNpc->m_sRegion_X, pNpc->m_sRegion_Z, pNpc->m_sNid);
	}
}
// ~sungyong 2002.05.23

void CAISocket::RecvNpcMoveResult(char *pBuf)
{
	int index = 0;
	BYTE		flag;			// 01(INFO_MODIFY)	: NPC ���� ����
								// 02(INFO_DELETE)	: NPC ���� ����
	short		nid;			// NPC index
	float		fPosX;			// X Position
	float		fPosZ;			// Z Position
	float		fPosY;			// Y Position
	float		fSecForMetor;	// Sec�� metor
	flag = GetByte(pBuf,index);
	nid = GetShort(pBuf,index);
	fPosX = Getfloat(pBuf, index);
	fPosZ = Getfloat(pBuf, index);
	fPosY = Getfloat(pBuf, index);
	fSecForMetor = Getfloat(pBuf, index);

	CNpc* pNpc = m_pMain->m_arNpcArray.GetData( nid );
	if(!pNpc)
		return;

	if (pNpc->isDead())
	{
		Packet result(AG_NPC_HP_REQ);
		result << nid << pNpc->m_iHP;
		Send(&result);
	}
	
	pNpc->MoveResult(fPosX, fPosY, fPosZ, fSecForMetor);
}

void CAISocket::RecvNpcAttack(char* pBuf)
{
	int index = 0, send_index = 0, sid = -1, tid = -1, nHP = 0, temp_damage = 0;
	BYTE type, bResult, byAttackType = 0;
	float fDir=0.0f;
	short damage = 0;
	CNpc* pNpc = NULL, *pMon = NULL;
	CUser* pUser = NULL;
	_OBJECT_EVENT* pEvent = NULL;

	type = GetByte(pBuf,index);
	bResult = GetByte(pBuf,index);
	sid = GetShort(pBuf,index);
	tid = GetShort(pBuf,index);
	damage = GetShort(pBuf,index);
	nHP = GetDWORD(pBuf,index);
	byAttackType = GetByte(pBuf, index);

	//TRACE("CAISocket-RecvNpcAttack : sid=%s, tid=%d, zone_num=%d\n", sid, tid, m_iZoneNum);

	if(type == 0x01)			// user attack -> npc
	{
		pNpc = m_pMain->m_arNpcArray.GetData(tid);
		if(!pNpc)	return;
		pNpc->m_iHP -= damage;
		if( pNpc->m_iHP < 0 )
			pNpc->m_iHP = 0;

		// NPC died
		if (bResult == 4)
			pNpc->OnDeath();
		else 
		{
			Packet result(WIZ_ATTACK, byAttackType);
			result << bResult << sid << tid;
			pNpc->SendToRegion(&result);
		}

		pUser = m_pMain->GetUserPtr(sid);
		if (pUser != NULL) 
		{
			pUser->SendTargetHP( 0, tid, -damage ); 
			if( byAttackType != MAGIC_ATTACK && byAttackType != DURATION_ATTACK) {
				pUser->ItemWoreOut(ATTACK, damage);

			// LEFT HAND!!! by Yookozuna
			temp_damage = damage * pUser->m_bMagicTypeLeftHand / 100 ;

			switch (pUser->m_bMagicTypeLeftHand) {	// LEFT HAND!!!
				case ITEM_TYPE_HP_DRAIN :	// HP Drain		
					pUser->HpChange(temp_damage, 0);	
					break;
				case ITEM_TYPE_MP_DRAIN :	// MP Drain		
					pUser->MSpChange(temp_damage);
					break;
				}				
			
			temp_damage = 0;	// reset data;

			// RIGHT HAND!!! by Yookozuna
			temp_damage = damage * pUser->m_bMagicTypeRightHand / 100 ;

			switch (pUser->m_bMagicTypeRightHand) {	// LEFT HAND!!!
				case ITEM_TYPE_HP_DRAIN :	// HP Drain		
					pUser->HpChange(temp_damage, 0);			
					break;
				case ITEM_TYPE_MP_DRAIN :	// MP Drain		
					pUser->MSpChange(temp_damage);
					break;
				}	
//		
			}
		}

		if (bResult == 2 || bResult== 4)		// npc dead
		{
			pNpc->GetMap()->RegionNpcRemove(pNpc->m_sRegion_X, pNpc->m_sRegion_Z, tid);
			
//			TRACE("--- Npc Dead : Npc�� Region���� ����ó��.. ,, region_x=%d, y=%d\n", pNpc->m_sRegion_X, pNpc->m_sRegion_Z);
			pNpc->m_sRegion_X = 0;		pNpc->m_sRegion_Z = 0;
			pNpc->m_NpcState = NPC_DEAD;
			if( pNpc->m_byObjectType == SPECIAL_OBJECT )	{
				pEvent = pNpc->GetMap()->GetObjectEvent( pNpc->m_sSid );
				if( pEvent )	pEvent->byLife = 0;
			}
			if (pNpc->m_tNpcType == 2 && pUser != NULL) // EXP 
				pUser->GiveItem(900001000, 1);	
		}
	}
	else if (type == 2)		// npc attack -> user
	{
		pNpc = m_pMain->m_arNpcArray.GetData(sid);
		if(!pNpc)	return;

		//TRACE("CAISocket-RecvNpcAttack 222 : sid=%s, tid=%d, zone_num=%d\n", sid, tid, m_iZoneNum);
		if( tid >= USER_BAND && tid < NPC_BAND)
		{
			pUser = m_pMain->GetUserPtr(tid);
			if(pUser == NULL)	
				return;

			pUser->HpChange(-damage, 1, true);
			pUser->ItemWoreOut(DEFENCE, damage);

			Packet result(WIZ_ATTACK, byAttackType);
			result	<< uint8(bResult == 3 ? 0 : bResult)
					<< sid << tid;
			pNpc->SendToRegion(&result);

			//TRACE("RecvNpcAttack ==> sid = %d, tid = %d, result = %d\n", sid, tid, result);

			// user dead
			if (bResult == 2) 
			{
				if (pUser->m_bResHpType == USER_DEAD)
					return;

				pUser->OnDeath();
				pUser->m_bResHpType = USER_DEAD;
				DEBUG_LOG("*** User Dead, id=%s, result=%d, AI_HP=%d, GM_HP=%d, x=%d, z=%d", pUser->m_pUserData->m_id, result, nHP, pUser->m_pUserData->m_sHp, (int)pUser->m_pUserData->m_curx, (int)pUser->m_pUserData->m_curz);

				send_index = 0;
				if( pUser->m_pUserData->m_bFame == COMMAND_CAPTAIN )	{	// ���ֱ����� �ִ� ������ �״´ٸ�,, ���� ���� ��Ż
					pUser->ChangeFame(CHIEF);

					TRACE("---> AISocket->RecvNpcAttack() Dead Captain Deprive - %s\n", pUser->m_pUserData->m_id);
					if (pUser->getNation() == KARUS)		m_pMain->Announcement( KARUS_CAPTAIN_DEPRIVE_NOTIFY, KARUS );
					else if (pUser->getNation() == ELMORAD)	m_pMain->Announcement( ELMORAD_CAPTAIN_DEPRIVE_NOTIFY, ELMORAD );

				}

				if(pNpc->m_tNpcType == NPC_PATROL_GUARD)	{	// ���񺴿��� �״� ��������..
					pUser->ExpChange( -pUser->m_iMaxExp/100 );
					//TRACE("RecvNpcAttack : ����ġ�� 1%���� id = %s\n", pUser->m_pUserData->m_id);
				}
				else {
//
					if( pUser->m_pUserData->m_bZone != pUser->m_pUserData->m_bNation && pUser->m_pUserData->m_bZone < 3) {
						pUser->ExpChange(-pUser->m_iMaxExp / 100);
						//TRACE("������ 1%�� �￴�ٴϱ��� ��.��");
					}
//				
					else {
						pUser->ExpChange( -pUser->m_iMaxExp/20 );
					}
					//TRACE("RecvNpcAttack : ����ġ�� 5%���� id = %s\n", pUser->m_pUserData->m_id);
				}
			}
		}
		else if(tid >= NPC_BAND)		// npc attack -> monster
		{
			pMon = m_pMain->m_arNpcArray.GetData(tid);
			if(!pMon)	return;
			pMon->m_iHP -= damage;
			if( pMon->m_iHP < 0 )
				pMon->m_iHP = 0;

			Packet result(WIZ_ATTACK, byAttackType);
			result << bResult << sid << tid;
			if (bResult == 2)	{		// npc dead
				pNpc->GetMap()->RegionNpcRemove(pMon->m_sRegion_X, pMon->m_sRegion_Z, tid);
//				TRACE("--- Npc Dead : Npc�� Region���� ����ó��.. ,, region_x=%d, y=%d\n", pMon->m_sRegion_X, pMon->m_sRegion_Z);
				pMon->m_sRegion_X = 0;		pMon->m_sRegion_Z = 0;
				pMon->m_NpcState = NPC_DEAD;
				if( pNpc->m_byObjectType == SPECIAL_OBJECT )	{
					pEvent = pNpc->GetMap()->GetObjectEvent( pMon->m_sSid );
					if( pEvent )	pEvent->byLife = 0;
				}
			}
			pNpc->SendToRegion(&result);
		}
	}
}

void CAISocket::RecvMagicAttackResult(char* pBuf)
{
	Packet result(WIZ_MAGIC_PROCESS, uint8(*pBuf));
	int index = 0, sid = -1, tid = -1, magicid=0;
	BYTE byCommand; 
	short data0, data1, data2, data3, data4, data5;

	/* 
		This is all so redundant...
		When everything's switched over to pass in Packets
		we can just pass it through directly!
		As it is now.. we still need a length (which we can hardcode, but meh)
	*/
	byCommand = GetByte(pBuf, index);			// magic type ( 1:casting, 2:flying, 3:effecting, 4:fail )
	magicid = GetDWORD(pBuf, index);
	sid = GetShort(pBuf, index);
	tid = GetShort(pBuf, index);
	data0 = GetShort(pBuf, index);
	data1 = GetShort(pBuf, index);
	data2 = GetShort(pBuf, index);
	data3 = GetShort(pBuf, index);
	data4 = GetShort(pBuf, index);
	data5 = GetShort(pBuf, index);

	result	<< byCommand << magicid << sid << tid 
			<< data0 << data1 << data2 << data3 << data4 << data5;

	if (byCommand == MAGIC_CASTING
		|| (byCommand == MAGIC_EFFECTING && sid >= NPC_BAND && tid >= NPC_BAND))
	{
		CNpc *pNpc = m_pMain->m_arNpcArray.GetData(sid);
		if (!pNpc)
			return;

		m_pMain->Send_Region(&result, pNpc->GetMap(), pNpc->m_sRegion_X, pNpc->m_sRegion_Z);
	}
	else if (byCommand == MAGIC_EFFECTING)
	{
		if (sid >= USER_BAND && sid < NPC_BAND)
		{
			CUser *pUser = m_pMain->GetUserPtr(sid);
			if (pUser == NULL || pUser->isDead())
				return;

			m_pMain->Send_Region(&result, pUser->GetMap(), pUser->m_RegionX, pUser->m_RegionZ);
			return;
		}

		// If we're an NPC, casting a skill (rather, it's finished casting) on a player...
		m_MagicProcess.MagicPacket(result);
	}
	
}

void CAISocket::RecvNpcInfo(char* pBuf)
{
	int index = 0;

	BYTE		Mode;			// 01(INFO_MODIFY)	: NPC ���� ����
								// 02(INFO_DELETE)	: NPC ���� ����
	short		nid;			// NPC index
	short		sid;			// NPC index
	short		sPid;			// NPC Picture Number
	short		sSize = 100;	// NPC Size
	int			iWeapon_1;		// ������ ����
	int			iWeapon_2;		// �޼�  ����
	BYTE        bZone;			// Current zone number
	char		szName[MAX_NPC_SIZE+1];		// NPC Name
	BYTE		byGroup;		// �Ҽ� ����
	BYTE		byLevel;			// level
	float		fPosX;			// X Position
	float		fPosZ;			// Z Position
	float		fPosY;			// Y Position
	BYTE		byDirection;	// ����
	BYTE		tState;			// NPC ����
								// 00	: NPC Dead
								// 01	: NPC Live
	BYTE		tNpcKind;		// 00	: Monster
								// 01	: NPC
	int		iSellingGroup;
  	int		nMaxHP;			// �ִ� HP
	int		nHP;			// ���� HP
	BYTE		byGateOpen;
	short		sHitRate;		// ���� ������
	BYTE		byObjectType;	// ���� : 0, Ư�� : 1

	Mode = GetByte(pBuf, index);
	nid = GetShort(pBuf, index);
	sid = GetShort(pBuf, index);
	sPid = GetShort(pBuf, index);
	sSize = GetShort(pBuf, index);
	iWeapon_1 = GetDWORD(pBuf, index);
	iWeapon_2 = GetDWORD(pBuf, index);
	bZone = GetByte(pBuf, index);
	int nLength = GetVarString(szName, pBuf, sizeof(BYTE), index);
	if(nLength < 0 || nLength > MAX_NPC_SIZE) return;		// �߸��� monster ���̵� 
	byGroup = GetByte(pBuf, index);
	byLevel  = GetByte(pBuf, index);
	fPosX = Getfloat(pBuf, index);
	fPosZ = Getfloat(pBuf, index);
	fPosY = Getfloat(pBuf, index);
	byDirection = GetByte(pBuf, index);
	tState = GetByte(pBuf, index);
	tNpcKind = GetByte(pBuf, index);
	iSellingGroup = GetDWORD(pBuf, index);
	nMaxHP = GetDWORD(pBuf, index);
	nHP = GetDWORD(pBuf, index);
	byGateOpen = GetByte(pBuf, index);
	sHitRate = GetShort(pBuf, index);
	byObjectType = GetByte(pBuf, index);

	CNpc* pNpc = NULL;

	pNpc = m_pMain->m_arNpcArray.GetData(nid);
	if(!pNpc)	return;

	pNpc->m_NpcState = NPC_DEAD;

	if( pNpc->m_NpcState == NPC_LIVE )	{	// ���� �ִµ� �� ������ �޴� ����
		char strLog[256]; 
		CTime t = CTime::GetCurrentTime();
		sprintf_s(strLog, sizeof(strLog), "## time(%d:%d-%d) npc regen check(%d) : nid=%d, name=%s, x=%d, z=%d, rx=%d, rz=%d ## \r\n", t.GetHour(), t.GetMinute(), t.GetSecond(), pNpc->m_NpcState, nid, szName, (int)pNpc->m_fCurX, (int)pNpc->m_fCurZ, pNpc->m_sRegion_X, pNpc->m_sRegion_Z);
		EnterCriticalSection( &g_LogFile_critical );
		m_pMain->m_RegionLogFile.Write( strLog, strlen(strLog) );
		LeaveCriticalSection( &g_LogFile_critical );
		TRACE(strLog);
		// to-do: replace with m_pMain->WriteRegionLog(...);
	}

	pNpc->m_NpcState = NPC_LIVE;
	
	pNpc->m_sNid = nid;
	pNpc->m_sSid = sid;
	pNpc->m_sPid = sPid;
	pNpc->m_sSize = sSize;
	pNpc->m_iWeapon_1 = iWeapon_1;
	pNpc->m_iWeapon_2 = iWeapon_2;
	strcpy(pNpc->m_strName, szName);
	pNpc->m_byGroup = byGroup;
	pNpc->m_byLevel = byLevel;
	pNpc->m_bCurZone = bZone;
	pNpc->m_pMap = m_pMain->GetZoneByID(bZone);
	pNpc->m_fCurX = fPosX;
	pNpc->m_fCurZ = fPosZ;
	pNpc->m_fCurY = fPosY;
	pNpc->m_byDirection = byDirection;
	pNpc->m_NpcState = tState;
	pNpc->m_tNpcType = tNpcKind;
	pNpc->m_iSellingGroup = iSellingGroup;
	pNpc->m_iMaxHP = nMaxHP;
	pNpc->m_iHP = nHP;
	pNpc->m_byGateOpen = byGateOpen;
	pNpc->m_sHitRate = sHitRate;
	pNpc->m_byObjectType = byObjectType;

	if (pNpc->GetMap() == NULL)
		return;

	int nRegX = (int)fPosX / VIEW_DISTANCE;
	int nRegZ = (int)fPosZ / VIEW_DISTANCE;

	pNpc->m_sRegion_X = nRegX;
	pNpc->m_sRegion_Z = nRegZ;

	_OBJECT_EVENT* pEvent = NULL;
	if( pNpc->m_byObjectType == SPECIAL_OBJECT )	{
		pEvent = pNpc->GetMap()->GetObjectEvent( pNpc->m_sSid );
		if( pEvent )	pEvent->byLife = 1;
	}

	if(Mode == 0)	{
		TRACE("RecvNpcInfo - dead monster nid=%d, name=%s\n", pNpc->m_sNid, pNpc->m_strName);
		return;
	}

	pNpc->NpcInOut(NPC_IN, fPosX, fPosZ, fPosY);
}

void CAISocket::RecvUserHP(char* pBuf)
{
	int index = 0, nid = 0, nHP = 0, nMaxHP = 0;

	nid = GetShort(pBuf, index);
	nHP = GetDWORD(pBuf, index);
	nMaxHP = GetDWORD(pBuf, index);

	if( nid >= USER_BAND && nid < NPC_BAND)	{
		CUser* pUser = m_pMain->GetUserPtr(nid);
		if(pUser == NULL)		return;
		pUser->m_pUserData->m_sHp = nHP;
	}
	else if(nid >= NPC_BAND)	{
		CNpc* pNpc = m_pMain->m_arNpcArray.GetData(nid);
		if(!pNpc)	return;
		int nOldHP = pNpc->m_iHP;
		pNpc->m_iHP = nHP;
		pNpc->m_iMaxHP = nMaxHP;
//		TRACE("RecvNpcHP - (%d,%s), %d->%d\n", pNpc->m_sNid, pNpc->m_strName, nOldHP, pNpc->m_sHP);
	}
}

void CAISocket::RecvUserExp(char* pBuf)
{
	int index = 0;
	int nid = 0;
	short sExp = 0;
	short sLoyalty = 0;

	nid = GetShort(pBuf,index);
	sExp = GetShort(pBuf,index);
	sLoyalty = GetShort(pBuf,index);

	CUser* pUser = m_pMain->GetUserPtr(nid);
	if(pUser == NULL)
		return;
	if(sExp < 0 || sLoyalty < 0)	{
		TRACE("#### AISocket - RecvUserExp : exp=%d, loyalty=%d,, �߸��� ����ġ�� �´�,, ������!!\n", sExp, sLoyalty);
		return;
	}
	pUser->ExpChange(sExp);
	if (sLoyalty > 0)
		pUser->SendLoyaltyChange(sLoyalty);
}

void CAISocket::RecvSystemMsg(char* pBuf)
{
	Packet result(WIZ_CHAT);
	int index = 0;
	char strSysMsg[256];

	BYTE bType;
	short sWho;

	bType = GetByte(pBuf,index);
	sWho = GetShort(pBuf,index);
	if (!GetKOString(pBuf, strSysMsg, index, sizeof(strSysMsg) - 1))
		return;

	//TRACE("RecvSystemMsg - type=%d, who=%d, len=%d, msg=%s\n", bType, sWho, sLength, strSysMsg);

	switch(sWho)
	{
	case SEND_ALL:
		result << bType << uint8(1) << int16(-1) << strSysMsg; 
		m_pMain->Send_All(&result);
		break;
	}
	
}

void CAISocket::RecvNpcGiveItem(char* pBuf)
{
	Packet result(WIZ_ITEM_DROP);
	int index = 0;
	short sUid, sNid, regionx, regionz;
	float fX, fZ, fY;
	BYTE byCount, bZone;
	int nItemNumber[NPC_HAVE_ITEM_LIST];
	short sCount[NPC_HAVE_ITEM_LIST];
	_ZONE_ITEM* pItem = NULL;
	C3DMap* pMap = NULL;
	CUser* pUser = NULL;

	sUid = GetShort(pBuf,index);	// Item�� ������ ������ ���̵�... (�̰��� �����ؼ� �۾��ϼ�~)
	sNid = GetShort(pBuf,index);
	bZone = GetByte(pBuf, index);
	regionx = GetShort( pBuf, index );
	regionz = GetShort( pBuf, index );
	fX = Getfloat(pBuf,index);
	fZ = Getfloat(pBuf,index);
	fY = Getfloat(pBuf,index);
	byCount = GetByte(pBuf,index);
	for(int i=0; i<byCount; i++)
	{
		nItemNumber[i] = GetDWORD(pBuf, index);
		sCount[i] = GetShort(pBuf,index);
	}

	if( sUid < 0 || sUid >= MAX_USER ) return;
	pMap = m_pMain->GetZoneByID(bZone);
	if (pMap == NULL)
		return;

	pItem = new _ZONE_ITEM;
	for(int i=0; i<6; i++) {
		pItem->itemid[i] = 0;
		pItem->count[i] = 0;
	}
	pItem->bundle_index = pMap->m_wBundle;
	pItem->time = TimeGet();
	pItem->x = fX;
	pItem->z = fZ;
	pItem->y = fY;
	for(int i=0; i<byCount; i++) {
		if( m_pMain->m_ItemtableArray.GetData(nItemNumber[i]) ) {
			pItem->itemid[i] = nItemNumber[i];
			pItem->count[i] = sCount[i];
		}
	}

	if (!pMap->RegionItemAdd(regionx, regionz, pItem ))
	{
		delete pItem;
		return;
	}

	pUser = m_pMain->GetUserPtr(sUid);
	if (pUser == NULL) 
		return;

	result << sNid << uint32(pItem->bundle_index);
	if (!pUser->isInParty())
		pUser->Send(&result);
	else
		m_pMain->Send_PartyMember(pUser->m_sPartyIndex, &result);
}

void CAISocket::RecvUserFail(char* pBuf)
{
	short nid = 0, sid=0;
	int index = 0;

	nid = GetShort(pBuf,index);
	sid = GetShort(pBuf,index);

	CUser* pUser = m_pMain->GetUserPtr(nid);
	if (pUser == NULL)
		return;

	pUser->HpChange(-10000, 1);

	Packet result(WIZ_ATTACK, uint8(1));
	result << uint8(2) << sid << nid;
	pUser->SendToRegion(&result);

	TRACE("### AISocket - RecvUserFail : sid=%d, tid=%d, id=%s ####\n", sid, nid, pUser->m_pUserData->m_id);
}

void CAISocket::InitEventMonster(int index)
{
	int count = index;
	if( count < 0 || count > NPC_BAND )	{
		TRACE("### InitEventMonster index Fail = %d ###\n", index);
		return;
	}

	int max_eventmop = count+EVENT_MONSTER;
	for( int i=count; i<max_eventmop; i++ )	{
		CNpc* pNpc = NULL;
		pNpc = new CNpc;
		if(pNpc == NULL) return;
		pNpc->Initialize();

		pNpc->m_sNid = i+NPC_BAND;
		//TRACE("InitEventMonster : uid = %d\n", pNpc->m_sNid);
		if( !m_pMain->m_arNpcArray.PutData( pNpc->m_sNid, pNpc) ) {
			TRACE("Npc PutData Fail - %d\n", pNpc->m_sNid);
			delete pNpc;
			pNpc = NULL;
		}	
	}

	count = m_pMain->m_arNpcArray.GetSize();
	TRACE("TotalMonster = %d\n", count);
}

void CAISocket::RecvCheckAlive(char* pBuf)
{
	Packet result(AG_CHECK_ALIVE_REQ);		
	m_pMain->m_sErrorSocketCount = 0;
	Send(&result);
}

void CAISocket::RecvGateDestory(char* pBuf)
{
	int index = 0, cur_zone=0, rx=0, rz=0;
	int nid = 0, gate_status = 0;

	nid = GetShort(pBuf,index);
	gate_status = GetByte(pBuf,index);
	cur_zone = GetShort(pBuf,index);
	rx = GetShort(pBuf,index);
	rz = GetShort(pBuf,index);

	if(nid >= NPC_BAND)		{
		CNpc* pNpc = m_pMain->m_arNpcArray.GetData(nid);
		if(!pNpc)	return;
		pNpc->m_byGateOpen = gate_status;
		TRACE("RecvGateDestory - (%d,%s), gate_status=%d\n", pNpc->m_sNid, pNpc->m_strName, pNpc->m_byGateOpen);
	}
}

void CAISocket::RecvNpcDead(char* pBuf)
{
	int index = 0;
	short nid = GetShort(pBuf, index);
	if (nid < NPC_BAND)	
		return;

	CNpc* pNpc = m_pMain->m_arNpcArray.GetData(nid);
	if (!pNpc)
		return;

	C3DMap* pMap = pNpc->GetMap();
	if (pMap == NULL)
		return;

	if (pNpc->m_byObjectType == SPECIAL_OBJECT)
	{
		_OBJECT_EVENT *pEvent = pMap->GetObjectEvent(pNpc->m_sSid);
		if (pEvent)
			pEvent->byLife = 0;
	}

	pMap->RegionNpcRemove(pNpc->m_sRegion_X, pNpc->m_sRegion_Z, nid);

	Packet result(WIZ_DEAD);
	result << nid;
	m_pMain->Send_Region(&result, pNpc->GetMap(), pNpc->m_sRegion_X, pNpc->m_sRegion_Z);

	pNpc->m_sRegion_X = pNpc->m_sRegion_Z = 0;
}

void CAISocket::RecvNpcInOut(char* pBuf)
{
	int index = 0, nid = 0, nType = 0;
	float fx = 0.0f, fz=0.0f, fy=0.0f;

	nType = GetByte(pBuf, index);
	nid = GetShort(pBuf, index);
	fx = Getfloat(pBuf, index);
	fz = Getfloat(pBuf, index);
	fy = Getfloat(pBuf, index);

	if (nid < NPC_BAND)
		return;

	CNpc* pNpc = m_pMain->m_arNpcArray.GetData(nid);
	if (pNpc)
		pNpc->NpcInOut(nType, fx, fz, fy);
}

void CAISocket::RecvBattleEvent(char* pBuf)
{
	int index = 0, send_index = 0, retvalue = 0;
	int nType = 0, nResult = 0, nLen = 0;
	char strMaxUserName[MAX_ID_SIZE+1], strKnightsName[MAX_ID_SIZE+1];
	char chatstr[1024], finalstr[1024], send_buff[1024];
	CUser* pUser = NULL;
	CKnights* pKnights = NULL;

	nType = GetByte( pBuf, index );
	nResult = GetByte(pBuf, index);

	if( nType == BATTLE_EVENT_OPEN )	{
	}
	else if( nType == BATTLE_MAP_EVENT_RESULT )	{
		if( m_pMain->m_byBattleOpen == NO_BATTLE )	{
			TRACE("#### RecvBattleEvent Fail : battleopen = %d, type = %d\n", m_pMain->m_byBattleOpen, nType);
			return;
		}
		if( nResult == KARUS )	{
			//TRACE("--> RecvBattleEvent : ī�罺 ������ �Ѿ �� �־�\n");
			m_pMain->m_byKarusOpenFlag = 1;		// ī�罺 ������ �Ѿ �� �־�
		}
		else if( nResult == ELMORAD )	{
			//TRACE("--> RecvBattleEvent : ���� ������ �Ѿ �� �־�\n");
			m_pMain->m_byElmoradOpenFlag = 1;	// ���� ������ �Ѿ �� �־�
		}

		Packet result(UDP_BATTLE_EVENT_PACKET, uint8(nType));
		result << uint8(nResult);
		m_pMain->Send_UDP_All(&result);
	}
	else if( nType == BATTLE_EVENT_RESULT )	{
		if( m_pMain->m_byBattleOpen == NO_BATTLE )	{
			TRACE("#### RecvBattleEvent Fail : battleopen = %d, type=%d\n", m_pMain->m_byBattleOpen, nType);
			return;
		}
		if( nResult == KARUS )	{
			//TRACE("--> RecvBattleEvent : ī�罺�� �¸��Ͽ����ϴ�.\n");
		}
		else if( nResult == ELMORAD )	{
			//TRACE("--> RecvBattleEvent : �������尡 �¸��Ͽ����ϴ�.\n");
		}

		nLen = GetByte(pBuf, index);

		if( nLen > 0 && nLen < MAX_ID_SIZE+1 )	{
			GetString( strMaxUserName, pBuf, nLen, index );
			if( m_pMain->m_byBattleSave == 0 )	{
				send_index = 0;			// �¸������� sql�� ����
				SetByte( send_buff, WIZ_BATTLE_EVENT, send_index );
				SetByte( send_buff, nType, send_index );
				SetByte( send_buff, nResult, send_index );
				SetByte( send_buff, nLen, send_index );
				SetString( send_buff, strMaxUserName, nLen, send_index );
				retvalue = m_pMain->m_LoggerSendQueue.PutData( send_buff, send_index );
				if (retvalue >= SMQ_FULL)
					DEBUG_LOG("WIZ_BATTLE_EVENT Send Fail : %d, %d", retvalue, nType);
				m_pMain->m_byBattleSave = 1;
			}
		}

		m_pMain->m_bVictory = nResult;
		m_pMain->m_byOldVictory = nResult;
		m_pMain->m_byKarusOpenFlag = 0;		// ī�罺 ������ �Ѿ �� ������
		m_pMain->m_byElmoradOpenFlag = 0;	// ���� ������ �Ѿ �� ������
		m_pMain->m_byBanishFlag = 1;

		Packet result(UDP_BATTLE_EVENT_PACKET, uint8(nType));
		result << uint8(nResult);
		m_pMain->Send_UDP_All(&result);
	}
	else if( nType == BATTLE_EVENT_MAX_USER )	{
		if (GetKOString(pBuf, strMaxUserName, index, MAX_ID_SIZE, sizeof(BYTE)))
		{
			pUser = m_pMain->GetUserPtr(strMaxUserName, TYPE_CHARACTER);
			if (pUser != NULL)
			{
				pKnights = m_pMain->GetClanPtr(pUser->m_pUserData->m_bKnights);
				if (pKnights)
					strcpy_s(strKnightsName, sizeof(strKnightsName), pKnights->m_strName);

				/* War rewards */
				// Warders
				if (nResult >= 3 && nResult <= 6)
					pUser->ChangeNP(500); /* TO-DO: Remove hardcoded values */
				// Keeper
				else if (nResult == 8)
					pUser->ChangeNP(1000);
			}

			int nResourceID = 0;
			switch (nResult)
			{
			case 1: // captain
				nResourceID = IDS_KILL_CAPTAIN;
				break;
			case 3: // Karus warder 1
				nResourceID = IDS_KILL_KARUS_GUARD1;
				break;
			case 4: // Karus warder 2
				nResourceID = IDS_KILL_KARUS_GUARD2;
				break;
			case 5: // El Morad warder 1
				nResourceID = IDS_KILL_ELMO_GUARD1;
				break;
			case 6: // El Morad warder 2
				nResourceID = IDS_KILL_ELMO_GUARD2;
				break;
			case 8: // Keeper
				nResourceID = IDS_KILL_GATEKEEPER;
				break;
			}

			if (nResourceID == 0)
			{
				TRACE("RecvBattleEvent: could not establish resource for result %d", nResult);
				return;
			}

			_snprintf(chatstr, sizeof(chatstr), m_pMain->GetServerResource(nResourceID), strKnightsName, strMaxUserName);

			send_index = 0;
			sprintf( finalstr, m_pMain->GetServerResource(IDP_ANNOUNCEMENT), chatstr );
			SetByte( send_buff, WIZ_CHAT, send_index );
			SetByte( send_buff, WAR_SYSTEM_CHAT, send_index );
			SetByte( send_buff, 1, send_index );
			SetShort( send_buff, -1, send_index );
			SetKOString( send_buff, finalstr, send_index );
			m_pMain->Send_All( send_buff, send_index );

			send_index = 0;
			SetByte( send_buff, WIZ_CHAT, send_index );
			SetByte( send_buff, PUBLIC_CHAT, send_index );
			SetByte( send_buff, 1, send_index );
			SetShort( send_buff, -1, send_index );
			SetKOString( send_buff, finalstr, send_index );
			m_pMain->Send_All( send_buff, send_index );

			Packet result(UDP_BATTLE_EVENT_PACKET, uint8(nType));
			result << uint8(nResult) << strKnightsName << strMaxUserName;
			m_pMain->Send_UDP_All(&result);
		}
	}
}


void CAISocket::RecvNpcEventItem( char* pBuf )
{
	int index = 0, zoneindex = -1, nItemNumber = 0, nCount = 0;
	short sUid = 0, sNid = 0;
	CUser* pUser = NULL;

	sUid = GetShort(pBuf,index);	// Item�� ������ ������ ���̵�... (�̰��� �����ؼ� �۾��ϼ�~)
	sNid = GetShort(pBuf,index);
	nItemNumber = GetDWORD(pBuf, index);
	nCount = GetDWORD(pBuf,index);

	pUser = m_pMain->GetUserPtr(sUid);
	if (pUser == NULL) return;
	pUser->EventMoneyItemGet( nItemNumber, nCount );
}

void CAISocket::RecvGateOpen( char* pBuf )
{
	int index = 0, nNid = 0, nSid = 0, nGateFlag = 0;
	CNpc* pNpc = NULL;
	_OBJECT_EVENT* pEvent = NULL;

	nNid = GetShort(pBuf, index);
	nSid = GetShort(pBuf, index);
	nGateFlag = GetByte(pBuf, index);

	pNpc = m_pMain->m_arNpcArray.GetData(nNid);
	if (pNpc == NULL)	
	{
		TRACE("#### RecvGateOpen Npc Pointer null : nid=%d ####\n", nNid);
		return;
	}

	pNpc->m_byGateOpen = nGateFlag; // possibly not needed (we'll do it below), but need to make sure.

	pEvent = pNpc->GetMap()->GetObjectEvent(nSid);
	if (pEvent == NULL)	
	{
		TRACE("#### RecvGateOpen Npc Object fail : nid=%d, sid=%d ####\n", nNid, nSid);
		return;
	}

	//TRACE("---> RecvGateOpen Npc Object fail : nid=%d, sid=%d, nGateFlag = %d ####\n", nNid, nSid, nGateFlag);
	if (pNpc->isGate())
		pNpc->SendGateFlag(nGateFlag, false);
}
