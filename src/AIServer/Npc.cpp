#include "stdafx.h"
#include "Region.h"
#include "Extern.h"
#include "Npc.h"
#include "User.h"
#include "NpcThread.h"

// 1m
//float surround_fx[8] = {0.0f, -0.7071f, -1.0f, -0.7083f,  0.0f,  0.7059f,  1.0000f, 0.7083f};
//float surround_fz[8] = {1.0f,  0.7071f,  0.0f, -0.7059f, -1.0f, -0.7083f, -0.0017f, 0.7059f};
// 2m
static float surround_fx[8] = {0.0f, -1.4142f, -2.0f, -1.4167f,  0.0f,  1.4117f,  2.0000f, 1.4167f};
static float surround_fz[8] = {2.0f,  1.4142f,  0.0f, -1.4167f, -2.0f, -1.4167f, -0.0035f, 1.4117f};

#define ATROCITY_ATTACK_TYPE 1				// 선공
#define TENDER_ATTACK_TYPE	 0				// 후공	

// 행동변경 관련 
#define NO_ACTION				0
#define ATTACK_TO_TRACE			1				// 공격에서 추격
#define MONSTER_CHANGED			1 
#define LONG_ATTACK_RANGE		30				// 장거리 공격 유효거리
#define SHORT_ATTACK_RANGE		3				// 직접공격 유효거리

#define ARROW_MIN				391010000
#define ARROW_MAX				392010000

#define ATTACK_LIMIT_LEVEL		10
#define FAINTING_TIME			2 // in seconds

bool CNpc::RegisterRegion(float x, float z)
{
	MAP* pMap = GetMap();
	if (pMap == nullptr) 
	{
		TRACE("#### Npc-SetUid Zone Fail : [name=%s], zone=%d #####\n", GetName().c_str(), GetZoneID());
		return false;
	}

	int x1 = (int)x / TILE_SIZE;
	int z1 = (int)z / TILE_SIZE;
	int nRX = (int)x / VIEW_DIST;
	int nRY = (int)z / VIEW_DIST;

	if(x1 < 0 || z1 < 0 || x1 >= pMap->GetMapSize() || z1 >= pMap->GetMapSize())
	{
		TRACE("#### SetUid failed : [nid=%d, sid=%d, zone=%d], coordinates out of range of map x=%d, z=%d, map size=%d #####\n", 
			GetID(), m_proto->m_sSid, GetZoneID(), x1, z1, pMap->GetMapSize());
		return false;
	}

	// if(pMap->m_pMap[x1][z1].m_sEvent == 0) return false;
	if(nRX > pMap->GetXRegionMax() || nRY > pMap->GetZRegionMax() || nRX < 0 || nRY < 0)
	{
		TRACE("#### SetUid Fail : [nid=%d, sid=%d], nRX=%d, nRZ=%d #####\n", GetID(), m_proto->m_sSid, nRX, nRY);
		return false;
	}

	if (GetRegionX() != nRX || GetRegionZ() != nRY)
	{
		int nOld_RX = GetRegionX();
		int nOld_RZ = GetRegionZ();

		m_sRegionX = nRX;
		m_sRegionZ = nRY;

		pMap->RegionNpcAdd(GetRegionX(), GetRegionZ(), GetID());
		pMap->RegionNpcRemove(nOld_RX, nOld_RZ, GetID());
	}

	return true;
}

CNpc::CNpc() : Unit(), m_NpcState(NPC_LIVE), m_byGateOpen(0), m_byObjectType(NORMAL_OBJECT), m_byPathCount(0),
	m_byAttackPos(0), m_ItemUserLevel(0), m_Delay(0), 
	m_proto(nullptr), m_pZone(nullptr), m_pPath(nullptr)
{
	InitTarget();

	m_fDelayTime = getMSTime();

	m_tNpcAttType = ATROCITY_ATTACK_TYPE;		// 공격 성향
	m_tNpcOldAttType = ATROCITY_ATTACK_TYPE;		// 공격 성향
	m_tNpcLongType = 0;		// 원거리(1), 근거리(0)
	m_tNpcGroupType = 0;	// 도움을 주는냐(1), 안주는냐?(0)
	m_byNpcEndAttType = 1;
	m_byWhatAttackType = 0;
	m_byMoveType = 1;
	m_byInitMoveType  = 1;
	m_byRegenType = 0;
	m_byDungeonFamily = 0;
	m_bySpecialType = 0;
	m_byTrapNumber = 0;
	m_byChangeType = 0;
	m_byDeadType = 0;
	m_sChangeSid = 0;
	m_sControlSid = 0;
	m_sPathCount = 0;
	m_sMaxPathCount = 0;
	m_byMoneyType = 0;

	m_bFirstLive = true;

	m_fHPChangeTime = getMSTime();
	m_tFaintingTime = 0;

	m_nLimitMinX = m_nLimitMinZ = 0;
	m_nLimitMaxX = m_nLimitMaxZ = 0;
	m_lEventNpc = 0;
	m_fSecForRealMoveMetor = 0.0f;
	InitUserList();
	InitMagicValuable();

	m_bTracing = false;
	m_fTracingStartX = m_fTracingStartZ = 0.0f;

	for(int i=0; i<NPC_MAX_PATH_LIST; i++)	{
		m_PathList.pPattenPos[i].x = -1;
		m_PathList.pPattenPos[i].z = -1;
	}
	m_pPattenPos.x = m_pPattenPos.z = 0;

	m_bMonster = false;
}

CNpc::~CNpc()
{
	ClearPathFindData();
	InitUserList();
}

void CNpc::ClearPathFindData()
{
	m_bPathFlag = false;
	m_sStepCount = 0;
	m_iAniFrameCount = 0;
	m_iAniFrameIndex = 0;
	m_fAdd_x = m_fAdd_z = 0.0f;

	for (int i = 0; i < MAX_PATH_LINE; i++)
	{
		m_pPoint[i].byType = 0;
		m_pPoint[i].bySpeed = 0;
		m_pPoint[i].fXPos = -1.0f;
		m_pPoint[i].fZPos = -1.0f;
	}
}

void CNpc::InitUserList()
{
	m_sMaxDamageUserid = -1;
	m_TotalDamage = 0;

	for (int i = 0; i < NPC_HAVE_USER_LIST; i++)
	{
		m_DamagedUserList[i].bIs = false;
		m_DamagedUserList[i].iUid = -1;
		m_DamagedUserList[i].nDamage = 0;
		memset(&m_DamagedUserList[i].strUserID, 0, sizeof(m_DamagedUserList[i].strUserID));
	}
}

void CNpc::InitTarget()
{
	if (m_byAttackPos != 0)
	{
		if (hasTarget() && m_Target.id < NPC_BAND)
		{
			CUser* pUser = g_pMain->GetUserPtr(m_Target.id);
			if (pUser != nullptr
				&& m_byAttackPos > 0 && m_byAttackPos < 9)
				pUser->m_sSurroundNpcNumber[m_byAttackPos - 1] = -1;
		}
	}

	m_byAttackPos = 0;
	m_Target.id = -1;
	m_Target.bSet = false;
	m_Target.x = m_Target.y = m_Target.z = 0.0f;
	m_Target.failCount = 0;
	m_bTracing = false;
}

void CNpc::Init()
{
	Unit::Initialize();

	m_pZone = g_pMain->GetZoneByID(GetZoneID());
	m_Delay = 0;
	m_fDelayTime = getMSTime();

	if (GetMap() == nullptr) 
	{
		TRACE("#### Npc-Init Zone Fail : [name=%s], zone=%d #####\n", GetName().c_str(), GetZoneID());
		return;
	}
}

void CNpc::InitPos()
{
	static const float fDD = 1.5f;
	static const float fx[4][5] = 
	{														// battle pos 
		{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },					// 0
		{ 0.0f, -(fDD*2),  -(fDD*2), -(fDD*4),  -(fDD*4) },	// 1
		{ 0.0f,  0.0f, -(fDD*2), -(fDD*2), -(fDD*2) },		// 2
		{ 0.0f, -(fDD*2),  -(fDD*2), -(fDD*2), -(fDD*4) }	// 3
	};

	static const float fz[4][5] = 
	{														// battle pos 
		{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },					// 0
		{ 0.0f,  (fDD*1),  -(fDD*1),  (fDD*1),  -(fDD*1) },	// 1
		{ 0.0f, -(fDD*2), (fDD*1), (fDD*1), (fDD*3) },		// 2
		{ 0.0f,  (fDD*2),   0.0f,    -(fDD*2),  0.0f }		// 3
	};

	if (m_byBattlePos > 3)
		return;

	m_fBattlePos_x = fx[m_byBattlePos][m_byPathCount - 1];
	m_fBattlePos_z = fz[m_byBattlePos][m_byPathCount - 1];
}

void CNpc::InitMagicValuable()
{
	for (int i = 0; i < MAX_MAGIC_TYPE4; i++)
	{
		m_MagicType4[i].byAmount = 100;
		m_MagicType4[i].sDurationTime = 0;
		m_MagicType4[i].tStartTime = 0;
	}

	for (int i = 0; i< MAX_MAGIC_TYPE3; i++)	
	{
		m_MagicType3[i].sHPAttackUserID = -1;
		m_MagicType3[i].sHPAmount = 0;
		m_MagicType3[i].byHPDuration = 0;
		m_MagicType3[i].byHPInterval = 2;
		m_MagicType3[i].tStartTime = 0;
	}
}

void CNpc::Load(uint16 sNpcID, CNpcTable * proto, bool bMonster)
{
	m_sNid = sNpcID + NPC_BAND;
	m_proto = proto;

	m_bMonster = bMonster;

	m_sSize				= proto->m_sSize;
	m_iWeapon_1			= proto->m_iWeapon_1;
	m_iWeapon_2			= proto->m_iWeapon_2;
	m_bNation			= proto->m_byGroup;
	m_bLevel			= (uint8) proto->m_sLevel; // max level used that I know about is 250, no need for 2 bytes

	// Monsters cannot, by design, be friendly to everybody.
	if (isMonster() && GetNation() == Nation::ALL)
		m_bNation = Nation::NONE;

	m_byActType			= proto->m_byActType;
	m_byRank			= proto->m_byRank;
	m_byTitle			= proto->m_byTitle;
	m_iSellingGroup		= proto->m_iSellingGroup;
	m_iHP				= proto->m_iMaxHP;
	m_iMaxHP			= proto->m_iMaxHP;
	m_sMP				= proto->m_sMaxMP;
	m_sMaxMP			= proto->m_sMaxMP;
	m_sAttack			= proto->m_sAttack;
	m_sDefense			= proto->m_sDefense;
	m_sHitRate			= proto->m_sHitRate;
	m_sEvadeRate		= proto->m_sEvadeRate;
	m_sDamage			= proto->m_sDamage;
	m_sAttackDelay		= proto->m_sAttackDelay;
	m_sSpeed			= proto->m_sSpeed;

	// Object NPCs should have an effective speed of 1x (not that it should matter, mind)
	if (m_byObjectType == SPECIAL_OBJECT)
		m_sSpeed = 1000;

	m_fSpeed_1			= (float)(proto->m_bySpeed_1 * (m_sSpeed / 1000));
	m_fSpeed_2			= (float)(proto->m_bySpeed_2 * (m_sSpeed / 1000));
	m_fOldSpeed_1		= (float)(proto->m_bySpeed_1 * (m_sSpeed / 1000));
	m_fOldSpeed_2		= (float)(proto->m_bySpeed_2 * (m_sSpeed / 1000));

	m_fSecForMetor		= 4.0f;
	m_sStandTime		= proto->m_sStandTime;
	m_byFireR			= proto->m_byFireR;
	m_byColdR			= proto->m_byColdR;
	m_byLightningR		= proto->m_byLightningR;
	m_byMagicR			= proto->m_byMagicR;
	m_byDiseaseR		= proto->m_byDiseaseR;
	m_byPoisonR			= proto->m_byPoisonR;
	m_bySearchRange		= proto->m_bySearchRange;
	m_byAttackRange		= proto->m_byAttackRange;
	m_byTracingRange	= proto->m_byTracingRange;
	m_iMoney			= proto->m_iMoney;
	m_iItem				= proto->m_iItem;
	m_tNpcLongType		= proto->m_byDirectAttack;
	m_byWhatAttackType	= proto->m_byDirectAttack;


	m_sRegenTime		= 10000 * SECOND;
	m_sMaxPathCount		= 0;
	m_tItemPer			= proto->m_tItemPer;
	m_tDnPer			= proto->m_tDnPer;

	m_pZone = g_pMain->GetZoneByID(GetZoneID());
	m_bFirstLive = 1;
}

time_t CNpc::NpcLive()
{
	// Dungeon Work 
	if (m_byRegenType == 2 || (m_byRegenType == 1 && m_byChangeType == 100))
	{
		m_NpcState = NPC_LIVE;
		return m_sRegenTime;
	}

	if (m_byChangeType == 1)
	{
		m_byChangeType = 2;
		ChangeMonsterInfomation(1);
	}

	m_NpcState = SetLive() ? NPC_STANDING : NPC_LIVE;
	return m_sStandTime;
}

time_t CNpc::NpcFighting()
{
	if (isDead())
	{
		Dead();
		return -1;
	}
	
	return Attack();
}

time_t CNpc::NpcTracing()
{
	if (m_sStepCount != 0)
	{
		if (m_fPrevX < 0 || m_fPrevZ < 0)
		{
			TRACE("### Npc-NpcTracing  Fail : nid=(%d, %s), x=%.2f, z=%.2f\n", GetID(), GetName().c_str(), m_fPrevX, m_fPrevZ);
		}
		else	
		{
			m_curx = m_fPrevX;		
			m_curz = m_fPrevZ; 
		}
	}

	if (isNonAttackingObject())
	{
		InitTarget();
		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}	

	int nFlag = IsCloseTarget(m_byAttackRange, 1);
	if (nFlag == 1)
	{
		NpcMoveEnd();
		m_NpcState = NPC_FIGHTING;
		return 0;
	}
	else if (nFlag == -1)
	{
		InitTarget();
		NpcMoveEnd();
		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}
	else if (nFlag == 2 && m_tNpcLongType == 2)
	{
		NpcMoveEnd();
		m_NpcState = NPC_FIGHTING;
		return 0;
	}

	if (m_byActionFlag == ATTACK_TO_TRACE)
	{
		m_byActionFlag = NO_ACTION;
		m_byResetFlag = 1;

		// If we're not already following a user, define our start coords.
		if (!m_bTracing)
		{
			m_fTracingStartX = GetX();
			m_fTracingStartZ = GetZ();
			m_bTracing = true;
		}
	}

	if (m_byResetFlag == 1)
	{
		if (!ResetPath())// && !m_tNpcTraceType)
		{
			TRACE("##### NpcTracing Fail : 패스파인드 실패 , NPC_STANDING으로 ######\n");
			InitTarget();
			NpcMoveEnd();	// 이동 끝..
			m_NpcState = NPC_STANDING;
			return m_sStandTime;
		}
	}

	if (  (!m_bPathFlag && !StepMove())
		|| (m_bPathFlag && !StepNoPathMove()))
	{
		m_NpcState = NPC_STANDING;
		TRACE("### NpcTracing Fail : StepMove 실패, %s, %d ### \n", GetName().c_str(), GetID());
		return m_sStandTime;
	}

	Packet result(MOVE_RESULT, uint8(SUCCESS));
	result << GetID();
	if (IsMovingEnd())
		result	<< GetX() << GetZ() << GetY()
				<< float(0.0f);
	else
		result	<< m_fPrevX << m_fPrevZ << m_fPrevY 
				<< (float)(m_fSecForRealMoveMetor / ((double)m_sSpeed / 1000));
	g_pMain->Send(&result);

	if (nFlag == 2 && m_tNpcLongType == 0 && m_proto->m_tNpcType != NPC_HEALER)
	{
		if (!TracingAttack())
		{
			InitTarget();
			NpcMoveEnd();
			m_NpcState = NPC_STANDING;
			return m_sStandTime;
		}
	}	

	return m_sSpeed;	
}

time_t CNpc::NpcAttacking()
{
	if (isDead())
	{	
		Dead();
		return -1;
	}

	if (isNonAttackingObject())
	{
		m_NpcState = NPC_STANDING;
		return m_sStandTime/2;
	}
	
	int ret = IsCloseTarget(m_byAttackRange);
	if (ret == 1)
	{
		m_NpcState = NPC_FIGHTING;
		return 0;
	}

	int nValue = GetTargetPath();
	if (nValue == -1)
	{
		if (!RandomMove())
		{
			InitTarget();
			m_NpcState = NPC_STANDING;
			return m_sStandTime;
		}

		InitTarget();
		m_NpcState = NPC_MOVING;
		return m_sSpeed;
	}
	else if (nValue == 0)
	{
		m_fSecForMetor = m_fSpeed_2;
		IsNoPathFind(m_fSecForMetor);
	}

	m_NpcState = NPC_TRACING;
	return 0;
}

time_t CNpc::NpcMoving()
{
	if (isDead())
	{
		Dead();
		return -1;
	}

	if (m_sStepCount != 0)	
	{
		if (m_fPrevX < 0 || m_fPrevZ < 0)
		{
			TRACE("### Npc-Moving Fail : nid=(%d, %s), x=%.2f, z=%.2f\n", GetID(), GetName().c_str(), m_fPrevX, m_fPrevZ);
		}
		else	
		{
			m_curx = m_fPrevX;
			m_curz = m_fPrevZ; 
		}
	}

	if (FindEnemy())
	{
	/*	if (isGuard()) 
		{ 
			NpcMoveEnd();
			m_NpcState = NPC_FIGHTING; 
			return 0; 
		}
		else */
		{ 
			NpcMoveEnd();
			m_NpcState = NPC_ATTACKING;
			return m_sSpeed;
		}
	}	

	if (IsMovingEnd())
	{
		m_curx = m_fPrevX;		
		m_curz = m_fPrevZ; 
		if (GetX() < 0 || GetZ() < 0)	
			TRACE("Npc-NpcMoving-2 : nid=(%d, %s), x=%.2f, z=%.2f\n", GetID(), GetName().c_str(), GetX(), GetZ());

		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}

	if (  (!m_bPathFlag && !StepMove())
		|| (m_bPathFlag && !StepNoPathMove()))
	{
		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}

	Packet result(MOVE_RESULT, uint8(SUCCESS));
	result << GetID();
	if (IsMovingEnd())
		result	<< m_fPrevX << m_fPrevZ << m_fPrevY 
				<< float(0.0f);
	else
		result	<< m_fPrevX << m_fPrevZ << m_fPrevY 
				<< (float)(m_fSecForRealMoveMetor / ((double)m_sSpeed / 1000));
	g_pMain->Send(&result);

	return m_sSpeed;	
}

time_t CNpc::NpcStanding()
{
/*	if (g_pMain->m_byNight == 2)
	{
		m_NpcState = NPC_SLEEPING;
		return 0;
	}	*/

	MAP* pMap = GetMap();
	if (pMap == nullptr)	
	{
		TRACE("### NpcStanding Zone Index Error : nid=%d, name=%s, zone=%d ###\n", GetID(), GetName().c_str(), GetZoneID());
		return -1;
	}

	// dungeon work
	CRoomEvent* pRoom = pMap->m_arRoomEventArray.GetData(m_byDungeonFamily);
	if (pRoom != nullptr)
	{
		if (pRoom->m_byStatus == 1)	
		{
			m_NpcState = NPC_STANDING;
			return m_sStandTime;
		}
	}

	if (RandomMove())
	{
		m_iAniFrameCount = 0;
		m_NpcState = NPC_MOVING;
		return m_sStandTime;
	}	

	m_NpcState = NPC_STANDING;
	
	if (m_proto->m_tNpcType == NPC_SPECIAL_GATE && g_pMain->m_byBattleEvent == BATTLEZONE_OPEN)
	{
		m_byGateOpen = !m_byGateOpen;
		Packet result(AG_NPC_GATE_OPEN);
		result << GetID() << m_byGateOpen;
		g_pMain->Send(&result);
	}

	return m_sStandTime;
}

time_t CNpc::NpcBack()
{
	if (isDead())
	{
		Dead();
		return -1;
	}

	if (hasTarget())
	{
		if (m_Target.id < NPC_BAND)	
		{
			if (g_pMain->GetUserPtr(m_Target.id) == nullptr)	
			{
				m_NpcState = NPC_STANDING;
				return m_sSpeed;//STEP_DELAY;
			}
		}
		else
		{
			if (g_pMain->m_arNpc.GetData(m_Target.id) == nullptr)	
			{
				m_NpcState = NPC_STANDING;
				return m_sSpeed;
			}
		}
	}

	if (m_sStepCount != 0)	
	{
		if (m_fPrevX < 0 || m_fPrevZ < 0)	
		{
			TRACE("### Npc-NpcBack Fail-1 : nid=(%d, %s), x=%.2f, z=%.2f\n", GetID(), GetName().c_str(), m_fPrevX, m_fPrevZ);
		}
		else	
		{
			m_curx = m_fPrevX;		
			m_curz = m_fPrevZ; 
		}
	}
	
	if (IsMovingEnd())	
	{
		m_curx = m_fPrevX;		
		m_curz = m_fPrevZ; 

		if (GetX() < 0 || GetZ() < 0)
			TRACE("Npc-NpcBack-2 : nid=(%d, %s), x=%.2f, z=%.2f\n", GetID(), GetName().c_str(), GetX(), GetZ());

		Packet result(MOVE_RESULT, uint8(SUCCESS));
		result << GetID() << GetX() << GetZ() << GetY() << float(0.0f);
		g_pMain->Send(&result);

		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}

	if (  (!m_bPathFlag && !StepMove())
		|| (m_bPathFlag && !StepNoPathMove()))
	{
		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}

	Packet result(MOVE_RESULT, uint8(SUCCESS));
	result	<< GetID() << m_fPrevX << m_fPrevZ << m_fPrevY 
			<< (float)(m_fSecForRealMoveMetor / ((double)m_sSpeed / 1000));
	g_pMain->Send(&result);

	return m_sSpeed;
}

bool CNpc::SetLive()
{
	int i = 0, j = 0;
	m_iHP = m_iMaxHP;
	m_sMP = m_sMaxMP;
	m_sPathCount = 0;
	m_iPattenFrame = 0;
	m_byResetFlag = 0;
	m_byActionFlag = NO_ACTION;
	m_byMaxDamagedNation = 0;

	m_sRegionX = m_sRegionZ = -1;
	m_fAdd_x = 0.0f;	m_fAdd_z = 0.0f;
	m_fStartPoint_X = 0.0f;		m_fStartPoint_Y = 0.0f; 
	m_fEndPoint_X = 0.0f;		m_fEndPoint_Y = 0.0f;
	m_min_x = m_min_y = m_max_x = m_max_y = 0;

	InitTarget();
	ClearPathFindData();
	InitUserList();
	//InitPos();

	CNpc* pNpc = nullptr;

	if (m_lEventNpc == 1 && !m_bFirstLive)
	{
#if 0
		NpcSet::iterator itr = g_pMain->m_arEventNpcThread[0]->m_pNpcs.find(this);
		if (itr != g_pMain->m_arEventNpcThread[0]->m_pNpcs.end())
		{
			m_lEventNpc = 0;
			g_pMain->m_arEventNpcThread[0]->m_pNpcs.erase(itr);
		}
#endif
		m_lEventNpc = 0;
		return true;
	}

	MAP* pMap = GetMap();
	if (pMap == nullptr)	
		return false;

	if (m_bFirstLive)
	{
		m_nInitX = m_fPrevX = GetX();
		m_nInitY = m_fPrevY = GetY();
		m_nInitZ = m_fPrevZ = GetZ();
	}

	if (GetX() < 0 || GetZ() < 0)
		TRACE("Npc-SetLive-1 : nid=(%d, %s), x=%.2f, z=%.2f\n", GetID(), GetName().c_str(), GetX(), GetZ());

	int dest_x = (int)m_nInitX / TILE_SIZE;
	int dest_z = (int)m_nInitZ / TILE_SIZE;

	bool bMove = pMap->IsMovable(dest_x, dest_z);

	if (m_proto->m_tNpcType != NPCTYPE_MONSTER || m_lEventNpc == 1)
	{
		m_curx = m_fPrevX = m_nInitX;
		m_cury = m_fPrevY = m_nInitY;
		m_curz = m_fPrevZ = m_nInitZ;
	}
	else	
	{
		int nX = 0;
		int nZ = 0;
		int nTileX = 0;
		int nTileZ = 0;
		int nRandom = 0;

		while (1)	
		{
			i++;
			nRandom = abs(m_nInitMinX - m_nInitMaxX);
			if (nRandom <= 1)	
				nX = m_nInitMinX;
			else	
			{
				if (m_nInitMinX < m_nInitMaxX)
					nX = myrand(m_nInitMinX, m_nInitMaxX);
				else
					nX = myrand(m_nInitMaxX, m_nInitMinX);
			}
			nRandom = abs(m_nInitMinY - m_nInitMaxY);
			if (nRandom <= 1)	
				nZ = m_nInitMinY;
			else	
			{
				if (m_nInitMinY < m_nInitMaxY)
					nZ = myrand(m_nInitMinY, m_nInitMaxY);
				else
					nZ = myrand(m_nInitMaxY, m_nInitMinY);
			}

			nTileX = nX / TILE_SIZE;
			nTileZ = nZ / TILE_SIZE;

			if (nTileX >= pMap->GetMapSize())
				nTileX = pMap->GetMapSize();
			if (nTileZ >= pMap->GetMapSize())
				nTileZ = pMap->GetMapSize();

			if (nTileX < 0 || nTileZ < 0)	
			{
				TRACE("#### Npc-SetLive() Fail : nTileX=%d, nTileZ=%d #####\n", nTileX, nTileZ);
				return false;
			}

			m_nInitX = m_fPrevX = m_curx = (float)nX;
			m_nInitZ = m_fPrevZ = m_curz = (float)nZ;

			if (GetX() < 0 || GetZ() < 0)
				TRACE("Npc-SetLive-2 : nid=(%d, %s), x=%.2f, z=%.2f\n", GetID(), GetName().c_str(), GetX(), GetZ());

			break;
		}	
	}

	m_fHPChangeTime = getMSTime();
	m_tFaintingTime = 0;
	InitMagicValuable();

	if (m_bFirstLive)	
	{
		NpcTypeParser();
		m_bFirstLive = false;

		if (g_pMain->m_CurrentNPC.increment() == g_pMain->m_TotalNPC)
		{
			printf("Monster All Init Success - %d\n", g_pMain->m_TotalNPC);
			g_pMain->GameServerAcceptThread();
		}
	}
	
	if (m_byMoveType == 3 && m_sMaxPathCount == 2)
	{
		__Vector3 vS, vE, vDir;
		float fDir;
		vS.Set((float)m_PathList.pPattenPos[0].x, 0, (float)m_PathList.pPattenPos[0].z);
		vE.Set((float)m_PathList.pPattenPos[1].x, 0, (float)m_PathList.pPattenPos[1].z);
		vDir = vE - vS;
		vDir.Normalize();
		Yaw2D(vDir.x, vDir.z, fDir);
		m_byDirection = (uint8)fDir;
	}

	if (m_bySpecialType == 5 && m_byChangeType == 0)
		return false;
	else if( m_bySpecialType == 5 && m_byChangeType == 3)	
	{
	//else if( m_byChangeType == 3)	{
		//char notify[50];
		//_snprintf( notify, sizeof(notify), "** 알림 : %s 몬스터가 출현하였습니다 **", GetName().c_str());
		//g_pMain->SendSystemMsg(notify, PUBLIC_CHAT);
	}

	RegisterRegion(GetX(), GetZ());
	m_byDeadType = 0;

	Packet result(AG_NPC_INFO);
	result.SByte();
	FillNpcInfo(result);
	g_pMain->Send(&result);

	return true;
}

bool CNpc::RandomMove()
{
	m_fSecForMetor = m_fSpeed_1;

	if (GetMap() == nullptr
		|| m_bySearchRange == 0
		|| m_byMoveType == 0
		|| !GetUserInView()
		// 4 means non-moving.
		|| m_byMoveType == 4)
		return false;

	float fDestX = -1.0f, fDestZ = -1.0f;
	int max_xx = GetMap()->GetMapSize();
	int max_zz = GetMap()->GetMapSize();
	int x = 0, y = 0;

	__Vector3 vStart, vEnd, vNewPos;
	float fDis = 0.0f;
 
	int nPathCount = 0;

	int random_x = 0, random_z = 0;
	bool bPeedBack = false;

	if (m_byMoveType == 1)	
	{
		bPeedBack = isInSpawnRange((int)GetX(), (int)GetZ());
		if (bPeedBack == false)
		{	
			//TRACE("초기위치를 벗어났군,,,  patten=%d \n", m_iPattenFrame);
		}

		if (m_iPattenFrame == 0)		
		{
			m_pPattenPos.x = (short)m_nInitX;
			m_pPattenPos.z = (short)m_nInitZ;
		}

		random_x = myrand(3, 7);
		random_z = myrand(3, 7);

		fDestX = GetX() + (float)random_x;
		fDestZ = GetZ() + (float)random_z;

		if (m_iPattenFrame == 2)	
		{
			fDestX = m_pPattenPos.x;
			fDestZ = m_pPattenPos.z;
			m_iPattenFrame = 0;
		}
		else	
		{
			m_iPattenFrame++;
		}

		vStart.Set(GetX(), GetY(), GetZ());
		vEnd.Set(fDestX, 0, fDestZ);
		fDis = GetDistance(vStart, vEnd);
		if (fDis > 50)	
		{
			GetVectorPosition(vStart, vEnd, 40, &vNewPos);
			fDestX = vNewPos.x;
			fDestZ = vNewPos.z;
			m_iPattenFrame = 2;
			bPeedBack = true;
		}
	}
	else if (m_byMoveType == 2)  
	{
		if (m_sPathCount == m_sMaxPathCount)		
			m_sPathCount = 0;

		if (m_sPathCount != 0 && IsInPathRange() == false)	
		{
			m_sPathCount--;
			nPathCount = GetNearPathPoint();

			if (nPathCount  == -1)	
			{
				TRACE("##### RandomMove Fail : [nid = %d, sid=%d], path = %d/%d, 이동할 수 있는 거리에서 너무 멀어졌당,, 어케해 #####\n", 
					GetID(), m_proto->m_sSid, m_sPathCount, m_sMaxPathCount);

				vStart.Set(GetX(), GetY(), GetZ());
				fDestX = (float)m_PathList.pPattenPos[0].x + m_fBattlePos_x;
				fDestZ = (float)m_PathList.pPattenPos[0].z + m_fBattlePos_z;
				vEnd.Set(fDestX, 0, fDestZ);
				GetVectorPosition(vStart, vEnd, 40, &vNewPos);
				fDestX = vNewPos.x;
				fDestZ = vNewPos.z;
			}
			else	
			{
				if(nPathCount < 0)	return false;
				fDestX = (float)m_PathList.pPattenPos[nPathCount].x + m_fBattlePos_x;
				fDestZ = (float)m_PathList.pPattenPos[nPathCount].z + m_fBattlePos_z;
				m_sPathCount = nPathCount;
			}
		}
		else	
		{
			if(m_sPathCount < 0)	return false;
			fDestX = (float)m_PathList.pPattenPos[m_sPathCount].x + m_fBattlePos_x;
			fDestZ = (float)m_PathList.pPattenPos[m_sPathCount].z + m_fBattlePos_z;
		}

		m_sPathCount++;
	}
	else if (m_byMoveType == 3)	
	{
		if (m_sPathCount == m_sMaxPathCount)	
		{
			m_byMoveType = 0;
			m_sPathCount = 0;
			return false;
		}

		if (m_sPathCount != 0 && IsInPathRange() == false)	
		{
			m_sPathCount--;
			nPathCount = GetNearPathPoint();
			if (nPathCount  == -1)	
			{
				TRACE("##### RandomMove Fail : [nid = %d, sid=%d], path = %d/%d, 이동할 수 있는 거리에서 너무 멀어졌당,, 어케해 #####\n", GetID(), m_proto->m_sSid, m_sPathCount, m_sMaxPathCount);
				vStart.Set(GetX(), GetY(), GetZ());
				fDestX = (float)m_PathList.pPattenPos[0].x + m_fBattlePos_x;
				fDestZ = (float)m_PathList.pPattenPos[0].z + m_fBattlePos_z;
				vEnd.Set(fDestX, 0, fDestZ);
				GetVectorPosition(vStart, vEnd, 40, &vNewPos);
				fDestX = vNewPos.x;
				fDestZ = vNewPos.z;
			}
			else	
			{
				if (nPathCount < 0)	
					return false;

				fDestX = (float)m_PathList.pPattenPos[nPathCount].x + m_fBattlePos_x;
				fDestZ = (float)m_PathList.pPattenPos[nPathCount].z + m_fBattlePos_x;
				m_sPathCount = nPathCount;
			}
		}
		else	
		{
			if (m_sPathCount < 0)	
				return false;

			fDestX = (float)m_PathList.pPattenPos[m_sPathCount].x + m_fBattlePos_x;
			fDestZ = (float)m_PathList.pPattenPos[m_sPathCount].z + m_fBattlePos_x;
		}

		m_sPathCount++;
	}

	vStart.Set(GetX(), 0, GetZ());
	vEnd.Set(fDestX, 0, fDestZ);

	if (GetX() < 0 || GetZ() < 0 || fDestX < 0 || fDestZ < 0)	
	{
		TRACE("##### RandomMove Fail : value is negative.. [nid = %d, name=%s], cur_x=%.2f, z=%.2f, dest_x=%.2f, dest_z=%.2f#####\n", 
			GetID(), GetName().c_str(), GetX(), GetZ(), fDestX, fDestZ);
		return false;
	}

	int mapWidth = (int)(max_xx * GetMap()->GetUnitDistance());

	if (GetX() >= mapWidth || GetZ() >= mapWidth || fDestX >= mapWidth || fDestZ >= mapWidth)	
	{
		TRACE("##### RandomMove Fail : value is overflow .. [nid = %d, name=%s], cur_x=%.2f, z=%.2f, dest_x=%.2f, dest_z=%.2f#####\n", 
			GetID(), GetName().c_str(), GetX(), GetZ(), fDestX, fDestZ);
		return false;
	}

    if (m_proto->m_tNpcType == NPC_DUNGEON_MONSTER)
	{
		if (!isInSpawnRange((int)fDestX, (int)fDestZ))
			return false;	
    }

	fDis = GetDistance(vStart, vEnd);
	if (fDis > NPC_MAX_MOVE_RANGE)
	{
		if (m_byMoveType == 2 || m_byMoveType == 3)	
		{
			if (--m_sPathCount <= 0) 
				m_sPathCount=0;
		}

		TRACE("##### RandomMove Fail : NPC_MAX_MOVE_RANGE overflow  .. [nid = %d, name=%s], cur_x=%.2f, z=%.2f, dest_x=%.2f, dest_z=%.2f, fDis=%.2f#####\n", 
			GetID(), GetName().c_str(), GetX(), GetZ(), fDestX, fDestZ, fDis);
		return false;
	}

	if (fDis <= m_fSecForMetor)		
	{
		ClearPathFindData();
		m_fStartPoint_X = GetX();		
		m_fStartPoint_Y = GetZ();
		m_fEndPoint_X = fDestX;			
		m_fEndPoint_Y = fDestZ;
		m_bPathFlag = true;
		m_iAniFrameIndex = 1;
		m_pPoint[0].fXPos = m_fEndPoint_X;
		m_pPoint[0].fZPos = m_fEndPoint_Y;
		return true;
	}

	float fTempRange = (float)fDis+2;
	int min_x = (int)(GetX() - fTempRange)/TILE_SIZE;	if(min_x < 0) min_x = 0;
	int min_z = (int)(GetZ() - fTempRange)/TILE_SIZE;	if(min_z < 0) min_z = 0;
	int max_x = (int)(GetX() + fTempRange)/TILE_SIZE;	if(max_x >= max_xx) max_x = max_xx - 1;
	int max_z = (int)(GetZ() + fTempRange)/TILE_SIZE;	if(min_z >= max_zz) min_z = max_zz - 1;

	CPoint start, end;
	start.x = (int)(GetX()/TILE_SIZE) - min_x;
	start.y = (int)(GetZ()/TILE_SIZE) - min_z;
	end.x = (int)(fDestX/TILE_SIZE) - min_x;
	end.y = (int)(fDestZ/TILE_SIZE) - min_z;

	if(start.x < 0 || start.y < 0 || end.x < 0 || end.y < 0)	return false;

	m_fStartPoint_X = GetX();		m_fStartPoint_Y = GetZ();
	m_fEndPoint_X = fDestX;	m_fEndPoint_Y = fDestZ;

	m_min_x = min_x;
	m_min_y = min_z;
	m_max_x = max_x;
	m_max_y = max_z;

	if (m_byMoveType == 2 || m_byMoveType == 3 || bPeedBack == true) 	
	{
		IsNoPathFind(m_fSecForMetor);
		return true;
	}

	return PathFind(start, end, m_fSecForMetor) == 1;
}

bool CNpc::RandomBackMove()
{
	m_fSecForMetor = m_fSpeed_2;

	if (m_bySearchRange == 0) return false;
	
	float fDestX = -1.0f, fDestZ = -1.0f;
	if (GetMap() == nullptr) 
	{
		TRACE("#### Npc-RandomBackMove Zone Fail : [name=%s], zone=%d #####\n", GetName().c_str(), GetZoneID());
		return false;
	}

	int max_xx = GetMap()->GetMapSize();
	int max_zz = GetMap()->GetMapSize();
	int x = 0, y = 0;
	float fTempRange = (float)m_bySearchRange*2;
	int min_x = (int)(GetX() - fTempRange)/TILE_SIZE;	if(min_x < 0) min_x = 0;
	int min_z = (int)(GetZ() - fTempRange)/TILE_SIZE;	if(min_z < 0) min_z = 0;
	int max_x = (int)(GetX() + fTempRange)/TILE_SIZE;	if(max_x > max_xx) max_x = max_xx;
	int max_z = (int)(GetZ() + fTempRange)/TILE_SIZE;	if(max_z > max_zz) max_z = max_zz;

	__Vector3 vStart, vEnd, vEnd22;
	float fDis = 0.0f;
	vStart.Set(GetX(), GetY(), GetZ());

	uint16 nID = m_Target.id;					// Target 을 구한다.
	CUser* pUser = nullptr;

	int iDir = 0;

	int iRandomX = 0, iRandomZ = 0, iRandomValue=0;
	iRandomValue = rand() % 2;

	// Head towards player
	if (nID < NPC_BAND)
	{
		pUser = g_pMain->GetUserPtr(nID);
		if (pUser == nullptr)	
			return false;

		if((int)pUser->GetX() != (int)GetX())  
		{
			iRandomX = myrand((int)m_bySearchRange, (int)(m_bySearchRange*1.5));
			iRandomZ = myrand(0, (int)m_bySearchRange);

			if((int)pUser->GetX() > (int)GetX())  
				iDir = 1;
			else
				iDir = 2;
		}
		else	// z축으로
		{
			iRandomZ = myrand((int)m_bySearchRange, (int)(m_bySearchRange*1.5));
			iRandomX = myrand(0, (int)m_bySearchRange);
			if((int)pUser->GetZ() > (int)GetZ())  
				iDir = 3;
			else
				iDir = 4;
		}

		switch(iDir)
		{
		case 1:
			fDestX = GetX() - iRandomX;
			if(iRandomValue == 0)
				fDestZ = GetZ() - iRandomX;
			else
				fDestZ = GetZ() + iRandomX;
			break;
		case 2:
			fDestX = GetX() + iRandomX;
			if(iRandomValue == 0)
				fDestZ = GetZ() - iRandomX;
			else
				fDestZ = GetZ() + iRandomX;
			break;
		case 3:
			fDestZ = GetZ() - iRandomX;
			if(iRandomValue == 0)
				fDestX = GetX() - iRandomX;
			else
				fDestX = GetX() + iRandomX;
			break;
		case 4:
			fDestZ = GetZ() - iRandomX;
			if(iRandomValue == 0)
				fDestX = GetX() - iRandomX;
			else
				fDestX = GetX() + iRandomX;
			break;
		}

		vEnd.Set(fDestX, 0, fDestZ);
		fDis = GetDistance(vStart, vEnd);
		if(fDis > 20)	// 20미터 이상이면 20미터로 맞춘다,,
		{
			GetVectorPosition(vStart, vEnd, 20, &vEnd22);
			fDestX = vEnd22.x;
			fDestZ = vEnd22.z;
		}
	}
	// Head towards monster/NPC
	else 
	{
	}

	CPoint start, end;
	start.x = (int)(GetX()/TILE_SIZE) - min_x;
	start.y = (int)(GetZ()/TILE_SIZE) - min_z;
	end.x = (int)(fDestX/TILE_SIZE) - min_x;
	end.y = (int)(fDestZ/TILE_SIZE) - min_z;

	if (start.x < 0 || start.y < 0 || end.x < 0 || end.y < 0)
		return false;

	m_fStartPoint_X = GetX();		m_fStartPoint_Y = GetZ();
	m_fEndPoint_X = fDestX;	m_fEndPoint_Y = fDestZ;

	m_min_x = min_x;
	m_min_y = min_z;
	m_max_x = max_x;
	m_max_y = max_z;

	int nValue = PathFind(start, end, m_fSecForMetor);
	if (nValue == 1)
		return true;

	return false;
}

bool CNpc::IsInPathRange()
{
	if (m_sPathCount < 0)	
		return false;

	static const float fPathRange = 40.0f;
	return isInRangeSlow((float)m_PathList.pPattenPos[m_sPathCount].x + m_fBattlePos_x, 
						(float)m_PathList.pPattenPos[m_sPathCount].z + m_fBattlePos_z,
						fPathRange + 1);
}

int CNpc::GetNearPathPoint()
{
	__Vector3 vStart, vEnd;
	float fMaxPathRange = (float)NPC_MAX_MOVE_RANGE;
	float fDis1 = 0.0f, fDis2 = 0.0f;
	int nRet = -1;
	vStart.Set(GetX(), GetY(), GetZ());
	vEnd.Set((float)m_PathList.pPattenPos[m_sPathCount].x + m_fBattlePos_x, 0, (float)m_PathList.pPattenPos[m_sPathCount].z + m_fBattlePos_z);
	fDis1 = GetDistance(vStart, vEnd);

	if(m_sPathCount+1 >= m_sMaxPathCount)	{
		if(m_sPathCount-1 > 0)	{
			vEnd.Set((float)m_PathList.pPattenPos[m_sPathCount-1].x + m_fBattlePos_x, 0, (float)m_PathList.pPattenPos[m_sPathCount-1].z + m_fBattlePos_z);
			fDis2 = GetDistance(vStart, vEnd);
		}
		else {
			vEnd.Set((float)m_PathList.pPattenPos[0].x + m_fBattlePos_x, 0, (float)m_PathList.pPattenPos[0].z + m_fBattlePos_z);
			fDis2 = GetDistance(vStart, vEnd);
		}
	}
	else
	{
		vEnd.Set((float)m_PathList.pPattenPos[m_sPathCount+1].x + m_fBattlePos_x, 0, (float)m_PathList.pPattenPos[m_sPathCount+1].z + m_fBattlePos_z);
		fDis2 = GetDistance(vStart, vEnd);
	}

	if(fDis1 <= fDis2)
	{
		if(fDis1 <= fMaxPathRange)
			nRet =  m_sPathCount;
	}
	else
	{
		if(fDis2 <= fMaxPathRange)
			nRet =  m_sPathCount+1;
	}

	return nRet;
}

bool CNpc::isInSpawnRange(int nX, int nZ)
{
	CRect r(m_nLimitMinX, m_nLimitMinZ, m_nLimitMaxX, m_nLimitMaxZ);
	return r.PtInRect(nX, nZ);
}

/////////////////////////////////////////////////////////////////////////////////////////
//	PathFind 를 수행한다.
//
int CNpc::PathFind(CPoint start, CPoint end, float fDistance)
{
	ClearPathFindData();

	if(start.x < 0 || start.y < 0 || end.x < 0 || end.y < 0)
	{
		return -1;
	}

	if(start.x == end.x && start.y == end.y)	// 같은 타일 안에서,, 조금 움직임이 있었다면,,
	{
		m_bPathFlag = true;
		m_iAniFrameIndex = 1;
		m_pPoint[0].fXPos = m_fEndPoint_X;
		m_pPoint[0].fZPos = m_fEndPoint_Y;
		return 1;
	}
	
	// 여기에서 패스파인드를 실행할건지.. 바로 목표점으로 갈건인지를 판단..
	if(IsPathFindCheck(fDistance) == true)
	{
		m_bPathFlag = true;
		return 1;
	}


	int i;
	int min_x, max_x;
	int min_y, max_y;

	min_x = m_min_x;
	min_y = m_min_y;
	max_x = m_max_x;
	max_y = m_max_y;

	m_vMapSize.cx = max_x - min_x + 1;		
	m_vMapSize.cy = max_y - min_y + 1;
	
	m_pPath = nullptr;

	m_vPathFind.SetMap(m_vMapSize.cx, m_vMapSize.cy, GetMap()->GetEventIDs(), GetMap()->GetMapSize(), min_x, min_y);
	m_pPath = m_vPathFind.FindPath(end.x, end.y, start.x, start.y);

	int count = 0;

	while(m_pPath)
	{
		m_pPath = m_pPath->Parent;
		if(m_pPath == nullptr)			break;
		m_pPoint[count].pPoint.x = m_pPath->x+min_x;		m_pPoint[count].pPoint.y = m_pPath->y+min_y;
		//m_pPath = m_pPath->Parent;
		count++;
	}	
	
	if(count <= 0 || count >= MAX_PATH_LINE)	{	
		return 0;
	}

	m_iAniFrameIndex = count-1;
	
	float x1 = 0.0f;
	float z1 = 0.0f;

	int nAdd = GetDir(m_fStartPoint_X, m_fStartPoint_Y, m_fEndPoint_X, m_fEndPoint_Y);

	for(i=0; i<count; i++)
	{
		if(i==(count-1))
		{
			m_pPoint[i].fXPos = m_fEndPoint_X;
			m_pPoint[i].fZPos = m_fEndPoint_Y;
		}
		else
		{
			x1 = (float)(m_pPoint[i].pPoint.x * TILE_SIZE + m_fAdd_x);
			z1 = (float)(m_pPoint[i].pPoint.y * TILE_SIZE + m_fAdd_z);
			m_pPoint[i].fXPos = x1;
			m_pPoint[i].fZPos = z1;
		}
	}

	return 1;
}


//	NPC 사망처리
void CNpc::Dead(int iDeadType)
{
	MAP* pMap = GetMap();
	if(pMap == nullptr)	return;

	m_iHP = 0;
	m_NpcState = NPC_DEAD;
	m_Delay = m_sRegenTime;
	m_bFirstLive = false;
	m_byDeadType = 100;		// 전쟁이벤트중에서 죽는 경우

	if (GetRegionX() > pMap->GetXRegionMax() || GetRegionZ() > pMap->GetZRegionMax())
	{
		TRACE("#### Npc-Dead() Fail : [nid=%d, sid=%d], nRX=%d, nRZ=%d #####\n", 
			GetID(), m_proto->m_sSid, GetRegionX(), GetRegionZ());
		return;
	}

	pMap->RegionNpcRemove(GetRegionX(), GetRegionZ(), GetID());

	if (iDeadType == 1)
	{
		Packet result(AG_DEAD);
		result << GetID();
		g_pMain->Send(&result);
	}

	// Dungeon Work : 변하는 몬스터의 경우 변하게 처리..
	if( m_bySpecialType == 1 || m_bySpecialType == 4)	{
		if( m_byChangeType == 0 )	{
			m_byChangeType = 1;
			//ChangeMonsterInfomation( 1 );
		}
		else if( m_byChangeType == 2 )	{
			if( m_byDungeonFamily < 0 || m_byDungeonFamily >= MAX_DUNGEON_BOSS_MONSTER )	{
				TRACE("#### Npc-Dead() m_byDungeonFamily Fail : [nid=%d, name=%s], m_byDungeonFamily=%d #####\n", GetID(), GetName().c_str(), m_byDungeonFamily);
				return;
			}
//			pMap->m_arDungeonBossMonster[m_byDungeonFamily] = 0;
		}
	}
	else	{
		m_byChangeType = 100;
	}

	
/*
	if( m_byDungeonFamily < 0 || m_byDungeonFamily >= MAX_DUNGEON_BOSS_MONSTER )	{
		TRACE("#### Npc-Dead() m_byDungeonFamily Fail : [nid=%d, name=%s], m_byDungeonFamily=%d #####\n", GetID(), GetName().c_str(), m_byDungeonFamily);
		return;
	}
	if( pMap->m_arDungeonBossMonster[m_byDungeonFamily] == 0 )	{
		m_byRegenType = 2;				// 리젠이 안되도록.. 
	}	*/
}

//	NPC 주변의 적을 찾는다.
bool CNpc::FindEnemy()
{
	if (isNonAttackingObject())
		return false;

	bool bIsGuard = isGuard();
	bool bIsNeutralZone = (GetZoneID() == 21 || GetZoneID() == 48); // Moradon/Arena

	// Disable AI enemy finding (of users) in neutral zones.
	// Guards and monsters are, however, allowed.
	if (!isMonster()
		&& !bIsGuard 
		&& bIsNeutralZone)
		return false;

	// Healer Npc
	int iMonsterNid = 0;
	if( m_proto->m_tNpcType == NPC_HEALER )	{		// Heal
		iMonsterNid = FindFriend( 2 );
		if( iMonsterNid != 0 )	return true;
	}

	MAP* pMap = GetMap();
	if (pMap == nullptr)	return false;
	CUser *pUser = nullptr;
	CNpc *pNpc = nullptr;

	int target_uid = 0;
	__Vector3 vUser, vNpc;
	float fDis = 0.0f;
	float fCompareDis = 0.0f;
	vNpc.Set(GetX(), GetY(), GetZ());

	float fSearchRange = (float)m_bySearchRange;

	int iExpand = FindEnemyRegion();

	if (GetRegionX() > pMap->GetXRegionMax() 
		|| GetRegionZ() > pMap->GetZRegionMax())
		return false;

	/*** Only find user enemies in non-neutral zones unless we're a monster ***/
	if ((isMonster() || !bIsNeutralZone)
		&& GetNation() != Nation::ALL)
	{
		fCompareDis = FindEnemyExpand(GetRegionX(), GetRegionZ(), fCompareDis, 1);

		int x=0, y=0;

		// 이웃해 있는 Region을 검색해서,,  몬의 위치와 제일 가까운 User을 향해.. 이동..
		for(int l=0; l<4; l++)	{
			if(m_iFind_X[l] == 0 && m_iFind_Y[l] == 0)		continue;

			x = GetRegionX() + (m_iFind_X[l]);
			y = GetRegionZ() + (m_iFind_Y[l]);

			// 이부분 수정요망,,
			if (x < 0 || y < 0 || x > pMap->GetXRegionMax() || y > pMap->GetZRegionMax())		continue;

			fCompareDis = FindEnemyExpand(x, y, fCompareDis, 1);
		}

		if (hasTarget() && (fCompareDis <= fSearchRange))		
			return true;

		fCompareDis = 0.0f;
	}

	/*** Only find NPC/mob enemies if we are a guard ***/
	if (bIsGuard) // || m_proto->m_tNpcType == NPCTYPE_MONSTER)	
	{
		fCompareDis = FindEnemyExpand(GetRegionX(), GetRegionZ(), fCompareDis, 2);

		int x=0, y=0;

		// 이웃해 있는 Region을 검색해서,,  몬의 위치와 제일 가까운 User을 향해.. 이동..
		for(int l=0; l<4; l++)	{
			if(m_iFind_X[l] == 0 && m_iFind_Y[l] == 0)			continue;

			x = GetRegionX() + (m_iFind_X[l]);
			y = GetRegionZ() + (m_iFind_Y[l]);

			// 이부분 수정요망,,
			if(x < 0 || y < 0 || x > pMap->GetXRegionMax() || y > pMap->GetZRegionMax())	continue;

			fCompareDis = FindEnemyExpand(x, y, fCompareDis, 2);
		}

		if (hasTarget() && (fCompareDis <= fSearchRange))	return true;
	}

	// 아무도 없으므로 리스트에 관리하는 유저를 초기화한다.
	InitUserList();		
	InitTarget();
	return false;
}

// Npc가 유저를 검색할때 어느 Region까지 검색해야 하는지를 판단..
int CNpc::FindEnemyRegion()
{
	/*
        1	2	3
		4	0	5
		6	7	8
	*/
	int iRetValue = 0;
	int  iSX = GetRegionX() * VIEW_DIST;
	int  iSZ = GetRegionZ() * VIEW_DIST;
	int  iEX = (GetRegionX()+1) * VIEW_DIST;
	int  iEZ = (GetRegionZ()+1) * VIEW_DIST;
	int  iSearchRange = m_bySearchRange;
	int iCurSX = (int)GetX() - m_bySearchRange;
	int iCurSY = (int)GetX() - m_bySearchRange;
	int iCurEX = (int)GetX() + m_bySearchRange;
	int iCurEY = (int)GetX() + m_bySearchRange;
	
	int iMyPos = GetMyField();

	switch(iMyPos)
	{
	case 1:
		if((iCurSX < iSX) && (iCurSY < iSZ))
			iRetValue = 1;
		else if((iCurSX > iSX) && (iCurSY < iSZ))
			iRetValue = 2;
		else if((iCurSX < iSX) && (iCurSY > iSZ))
			iRetValue = 4;
		else if((iCurSX >= iSX) && (iCurSY >= iSZ))
			iRetValue = 0;
		break;
	case 2:
		if((iCurEX < iEX) && (iCurSY < iSZ))
			iRetValue = 2;
		else if((iCurEX > iEX) && (iCurSY < iSZ))
			iRetValue = 3;
		else if((iCurEX <= iEX) && (iCurSY >= iSZ))
			iRetValue = 0;
		else if((iCurEX > iEX) && (iCurSY > iSZ))
			iRetValue = 5;
		break;
	case 3:
		if((iCurSX < iSX) && (iCurEY < iEZ))
			iRetValue = 4;
		else if((iCurSX >= iSX) && (iCurEY <= iEZ))
			iRetValue = 0;
		else if((iCurSX < iSX) && (iCurEY > iEZ))
			iRetValue = 6;
		else if((iCurSX > iSX) && (iCurEY > iEZ))
			iRetValue = 7;
		break;
	case 4:
		if((iCurEX <= iEX) && (iCurEY <= iEZ))
			iRetValue = 0;
		else if((iCurEX > iEX) && (iCurEY < iEZ))
			iRetValue = 5;
		else if((iCurEX < iEX) && (iCurEY > iEZ))
			iRetValue = 7;
		else if((iCurEX > iEX) && (iCurEY > iEZ))
			iRetValue = 8;
		break;
	}

	if(iRetValue <= 0) // 임시로 보정(문제시),, 하기 위한것..
		iRetValue = 0;

	switch(iRetValue)
	{
	case 0:
		m_iFind_X[0] = 0;  m_iFind_Y[0] = 0;
		m_iFind_X[1] = 0;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = 0;  m_iFind_Y[2] = 0;
		m_iFind_X[3] = 0;  m_iFind_Y[3] = 0;
		break;
	case 1:
		m_iFind_X[0] = -1;  m_iFind_Y[0] = -1;
		m_iFind_X[1] = 0;  m_iFind_Y[1] = -1;
		m_iFind_X[2] = -1;  m_iFind_Y[2] = 0;
		m_iFind_X[3] = 0;  m_iFind_Y[3] = 0;
		break;
	case 2:
		m_iFind_X[0] = 0;  m_iFind_Y[0] = -1;
		m_iFind_X[1] = 0;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = 0;  m_iFind_Y[2] = 0;
		m_iFind_X[3] = 0;  m_iFind_Y[3] = 0;
		break;
	case 3:
		m_iFind_X[0] = 0;  m_iFind_Y[0] = 0;
		m_iFind_X[1] = 1;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = 0;  m_iFind_Y[2] = 1;
		m_iFind_X[3] = 1;  m_iFind_Y[3] = 1;
		break;
	case 4:
		m_iFind_X[0] = -1;  m_iFind_Y[0] = 0;
		m_iFind_X[1] = 0;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = 0;  m_iFind_Y[2] = 0;
		m_iFind_X[3] = 0;  m_iFind_Y[3] = 0;
		break;
	case 5:
		m_iFind_X[0] = 0;  m_iFind_Y[0] = 0;
		m_iFind_X[1] = 1;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = 0;  m_iFind_Y[2] = 0;
		m_iFind_X[3] = 0;  m_iFind_Y[3] = 0;
		break;
	case 6:
		m_iFind_X[0] = -1;  m_iFind_Y[0] = 0;
		m_iFind_X[1] = 0;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = -1;  m_iFind_Y[2] = 1;
		m_iFind_X[3] = 0;  m_iFind_Y[3] = 1;
		break;
	case 7:
		m_iFind_X[0] = 0;  m_iFind_Y[0] = 0;
		m_iFind_X[1] = 0;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = 0;  m_iFind_Y[2] = 1;
		m_iFind_X[3] = 0;  m_iFind_Y[3] = 0;
		break;
	case 8:
		m_iFind_X[0] = 0;  m_iFind_Y[0] = 0;
		m_iFind_X[1] = 1;  m_iFind_Y[1] = 0;
		m_iFind_X[2] = 0;  m_iFind_Y[2] = 1;
		m_iFind_X[3] = 1;  m_iFind_Y[3] = 1;
		break;
	}

	return iRetValue;
}

float CNpc::FindEnemyExpand(int nRX, int nRZ, float fCompDis, int nType)
{
	MAP* pMap = GetMap();
	float fDis = 0.0f;
	if(pMap == nullptr)	return fDis;
	float fComp = fCompDis;
	float fSearchRange = (float)m_bySearchRange;
	uint16 target_uid;
	__Vector3 vUser, vNpc, vMon;
	vNpc.Set(GetX(), GetY(), GetZ());
	int iLevelComprison = 0;
	
	// Finding players
	if (nType == 1)	
	{
		FastGuard lock(pMap->m_lock);
		CRegion *pRegion = &pMap->m_ppRegion[nRX][nRZ];

		//TRACE("FindEnemyExpand type1,, region_x=%d, region_z=%d, user=%d, mon=%d\n", nRX, nRZ, nUser, nMonster);
		if (pRegion->m_RegionUserArray.IsEmpty())
			return 0.0f;

		foreach_stlmap (itr, pRegion->m_RegionUserArray)
		{
			CUser *pUser = g_pMain->GetUserPtr(*itr->second);
			if (pUser == nullptr 
				|| pUser->isDead()
				|| GetNation() == pUser->GetNation()
				|| pUser->m_bInvisibilityType
				|| pUser->m_byIsOP == MANAGER_USER)
				continue;

			float fDis = Unit::GetDistance(pUser);
			if (fDis > pow(fSearchRange, 2.0f)
				|| fDis < pow(fComp, 2.0f))
				continue;

			target_uid = pUser->m_iUserId;
			fComp = fDis;

			//후공몹...
			if(!m_tNpcAttType)	{	// 날 공격한 놈을 찾는다.
				if(IsDamagedUserList(pUser) || (m_tNpcGroupType && m_Target.id == target_uid))	{
					m_Target.id	= target_uid;
					m_Target.bSet = true;
					m_Target.failCount = 0;
					m_Target.x	= pUser->GetX();
					m_Target.y	= pUser->GetY();
					m_Target.z	= pUser->GetZ();
				}
			}
			else	{	// 선공몹...
				iLevelComprison = pUser->m_bLevel - m_proto->m_sLevel;
				// 작업할 것 : 타입에 따른 공격성향으로..
				//if(iLevelComprison > ATTACK_LIMIT_LEVEL)	continue;

				m_Target.id	= target_uid;
				m_Target.bSet = true;
				m_Target.failCount = 0;
				m_Target.x	= pUser->GetX();
				m_Target.y	= pUser->GetY();
				m_Target.z	= pUser->GetZ();
				//TRACE("Npc-FindEnemyExpand - target_x = %.2f, z=%.2f\n", m_Target.x, m_Target.z);
			}
		}
	}
	// Finding NPCs/monsters
	else if (nType == 2)		
	{
		FastGuard lock(pMap->m_lock);
		CRegion *pRegion = &pMap->m_ppRegion[nRX][nRZ];

		if (pRegion->m_RegionNpcArray.IsEmpty())
			return 0.0f;

		foreach_stlmap (itr, pRegion->m_RegionNpcArray)
		{
			int nNpcid = *itr->second;
			if( nNpcid < NPC_BAND )	continue;
			CNpc *pNpc = g_pMain->m_arNpc.GetData(nNpcid);

			if (GetID() == pNpc->GetID())	continue;

			if (pNpc != nullptr && pNpc->m_NpcState != NPC_DEAD && pNpc->GetID() != GetID())	
			{
				if (GetNation() == pNpc->GetNation())
					continue;

				vMon.Set(pNpc->GetX(), pNpc->GetY(), pNpc->GetZ()); 
				fDis = GetDistance(vMon, vNpc);

				if(fDis <= fSearchRange)	{
					if(fDis >= fComp)	{	// 
						target_uid = nNpcid;
						fComp = fDis;
						m_Target.id	= target_uid;
						m_Target.bSet = true;
						m_Target.failCount = 0;
						m_Target.x	= pNpc->GetX();
						m_Target.y	= pNpc->GetY();
						m_Target.z	= pNpc->GetZ();
					//	TRACE("Npc-IsCloseTarget - target_x = %.2f, z=%.2f\n", m_Target.x, m_Target.z);
					}
				}	
				else continue;
			}
		}
	}

	return fComp;
}

// region을 4등분해서 몬스터의 현재 위치가 region의 어느 부분에 들어가는지를 판단
int CNpc::GetMyField()
{
	int iRet = 0;
	int iX = GetRegionX() * VIEW_DIST;
	int iZ = GetRegionZ() * VIEW_DIST;
	int iAdd = VIEW_DIST / 2;
	int iCurX = (int)GetX();	// monster current position_x
	int iCurZ = (int)GetZ();
	if(COMPARE(iCurX, iX, iX+iAdd) && COMPARE(iCurZ, iZ, iZ+iAdd))
		iRet = 1;
	else if(COMPARE(iCurX, iX+iAdd, iX+VIEW_DIST) && COMPARE(iCurZ, iZ, iZ+iAdd))
		iRet = 2;
	else if(COMPARE(iCurX, iX, iX+iAdd) && COMPARE(iCurZ, iZ+iAdd, iZ+VIEW_DIST))
		iRet = 3;
	else if(COMPARE(iCurX, iX+iAdd, iX+VIEW_DIST) && COMPARE(iCurZ, iZ+iAdd, iZ+VIEW_DIST))
		iRet = 4;

	return iRet;
}

bool CNpc::IsDamagedUserList(CUser *pUser)
{
	if (pUser == nullptr) 
		return false;

	for (int i = 0; i < NPC_HAVE_USER_LIST; i++)
	{
		if (STRCASECMP(m_DamagedUserList[i].strUserID, pUser->GetName().c_str()) == 0) 
			return true;
	}

	return false;
}

int CNpc::IsSurround(CUser* pUser)
{
	if(m_tNpcLongType) return 0;		//원거리는 통과

	if(pUser == nullptr)	return -2;		// User가 없으므로 타겟지정 실패..
	int nDir = pUser->IsSurroundCheck(GetX(), 0.0f, GetZ(), GetID());
	if(nDir != 0)
	{
		m_byAttackPos = nDir;
		return nDir;
	}
	return -1;					// 타겟이 둘러 쌓여 있음...
}

//	x, y 가 움직일 수 있는 좌표인지 판단
bool CNpc::IsMovable(float x, float z)
{
	MAP* pMap = GetMap();
	if (pMap == nullptr
		|| x < 0 || z < 0
		|| x >= pMap->GetMapSize() || z >= pMap->GetMapSize()
		|| pMap->GetEventID((int)x, (int)z) == 0)
		return false;

	return true;
}

//	Path Find 로 찾은길을 다 이동 했는지 판단
bool CNpc::IsMovingEnd()
{
	if(m_fPrevX == m_fEndPoint_X && m_fPrevZ == m_fEndPoint_Y) 
	{
		//m_sStepCount = 0;
		m_iAniFrameCount = 0;
		return true;
	}

	return false;
}

//	Step 수 만큼 타켓을 향해 이동한다.
bool CNpc::StepMove()
{
	if(m_NpcState != NPC_MOVING && m_NpcState != NPC_TRACING && m_NpcState != NPC_BACK) return false;
	
	__Vector3 vStart, vEnd, vDis;
	float fDis;

	float fOldCurX = 0.0f, fOldCurZ = 0.0f;

	if(m_sStepCount == 0)	{
		fOldCurX = GetX();  fOldCurZ = GetZ();
	}
	else	{
		fOldCurX = m_fPrevX; fOldCurZ = m_fPrevZ;
	}

	vStart.Set(fOldCurX, 0, fOldCurZ);
	vEnd.Set(m_pPoint[m_iAniFrameCount].fXPos, 0, m_pPoint[m_iAniFrameCount].fZPos);

	// 안전 코드..
	if(m_pPoint[m_iAniFrameCount].fXPos < 0 || m_pPoint[m_iAniFrameCount].fZPos < 0)
	{
		m_fPrevX = m_fEndPoint_X;
		m_fPrevZ = m_fEndPoint_Y;
		TRACE("##### Step Move Fail : [nid = %d,%s] m_iAniFrameCount=%d/%d ######\n", GetID(), GetName().c_str(), m_iAniFrameCount, m_iAniFrameIndex);
		RegisterRegion(m_fPrevX, m_fPrevZ);
		return false;	
	}

	fDis = GetDistance(vStart, vEnd);

	// For as long as speeds are broken, this check's going to cause problems
	// It's disabled for now, but note that the removal of this check is the reason 
	// why mobs are going to have weird looking bursts of speed.
	// Without, they'll just get stuck going back and forth in position. Compromises.
//#if 0
	if(fDis >= m_fSecForMetor)
	{
		GetVectorPosition(vStart, vEnd, m_fSecForMetor, &vDis);
		m_fPrevX = vDis.x;
		m_fPrevZ = vDis.z;
	}
	else
//#endif
	{
		m_iAniFrameCount++;
		if(m_iAniFrameCount == m_iAniFrameIndex)
		{
			vEnd.Set(m_pPoint[m_iAniFrameCount].fXPos, 0, m_pPoint[m_iAniFrameCount].fZPos);
			fDis = GetDistance(vStart, vEnd);
			// 마지막 좌표는 m_fSecForMetor ~ m_fSecForMetor+1 사이도 가능하게 이동
			if(fDis > m_fSecForMetor)
			{
				GetVectorPosition(vStart, vEnd, m_fSecForMetor, &vDis);
				m_fPrevX = vDis.x;
				m_fPrevZ = vDis.z;
				m_iAniFrameCount--;
			}
			else
			{
				m_fPrevX = m_fEndPoint_X;
				m_fPrevZ = m_fEndPoint_Y;
			}
		}
		else
		{
			vEnd.Set(m_pPoint[m_iAniFrameCount].fXPos, 0, m_pPoint[m_iAniFrameCount].fZPos);
			fDis = GetDistance(vStart, vEnd);
			if(fDis >= m_fSecForMetor)
			{
				GetVectorPosition(vStart, vEnd, m_fSecForMetor, &vDis);
				m_fPrevX = vDis.x;
				m_fPrevZ = vDis.z;
			}
			else
			{
				m_fPrevX = m_fEndPoint_X;
				m_fPrevZ = m_fEndPoint_Y;
			}
		}
	}

	vStart.Set(fOldCurX, 0, fOldCurZ);
	vEnd.Set(m_fPrevX, 0, m_fPrevZ);

	m_fSecForRealMoveMetor = GetDistance(vStart, vEnd);

	if(m_fSecForRealMoveMetor > m_fSecForMetor+1)
	{
		TRACE("#### move fail : [nid = %d], m_fSecForMetor = %.2f\n", GetID(), m_fSecForRealMoveMetor);
	}

	if (m_sStepCount++ > 0)
	{
		m_curx = fOldCurX;		 m_curz = fOldCurZ;
		if(GetX() < 0 || GetZ() < 0)
			TRACE("Npc-StepMove : nid=(%d, %s), x=%.2f, z=%.2f\n", GetID(), GetName().c_str(), GetX(), GetZ());
		
		return RegisterRegion(GetX(), GetZ());
	}

	return true;
}

bool CNpc::StepNoPathMove()
{
	if(m_NpcState != NPC_MOVING && m_NpcState != NPC_TRACING && m_NpcState != NPC_BACK) return false;
	
	__Vector3 vStart, vEnd;
	float fOldCurX = 0.0f, fOldCurZ = 0.0f;

	if(m_sStepCount == 0)	{
		fOldCurX = GetX(); fOldCurZ = GetZ();
	}
	else	{
		fOldCurX = m_fPrevX; fOldCurZ = m_fPrevZ;
	}

	
	if(m_sStepCount < 0 || m_sStepCount >= m_iAniFrameIndex)	{	
		TRACE("#### IsNoPtahfind Fail : nid=%d,%s, count=%d/%d ####\n", GetID(), GetName().c_str(), m_sStepCount, m_iAniFrameIndex);
		return false;
	}

	vStart.Set(fOldCurX, 0, fOldCurZ);
	m_fPrevX = m_pPoint[m_sStepCount].fXPos;
	m_fPrevZ = m_pPoint[m_sStepCount].fZPos;
	vEnd.Set(m_fPrevX, 0, m_fPrevZ);

	if(m_fPrevX == -1 || m_fPrevZ == -1)	{
		TRACE("##### StepNoPath Fail : nid=%d,%s, x=%.2f, z=%.2f #####\n", GetID(), GetName().c_str(), m_fPrevX, m_fPrevZ);
		return false;
	}

	m_fSecForRealMoveMetor = GetDistance(vStart, vEnd);
	
	if (m_sStepCount++ > 0)
	{
		if(fOldCurX < 0 || fOldCurZ < 0)	{
			TRACE("#### Npc-StepNoPathMove Fail : nid=(%d, %s), x=%.2f, z=%.2f\n", GetID(), GetName().c_str(), fOldCurX, fOldCurZ);
			return false;
		}
		else	
		{
			m_curx = fOldCurX;	 m_curz = fOldCurZ;
		}
		
		return RegisterRegion(GetX(), GetZ());
	}

	return true;
}

//	NPC와 Target 과의 거리가 지정 범위보다 작은지 판단
int CNpc::IsCloseTarget(int nRange, int Flag)
{
	if (!hasTarget())
		return -1;

	CUser * pUser = nullptr;
	CNpc * pNpc = nullptr;
	__Vector3 vUser, vWillUser, vNpc, vDistance;
	float fDis = 0.0f, fWillDis = 0.0f, fX = 0.0f, fZ = 0.0f;
	bool  bUserType = false;	// 타겟이 유저이면 true
	vNpc.Set(GetX(), GetY(), GetZ());

	if (m_Target.id < NPC_BAND)	
	{
		pUser = g_pMain->GetUserPtr(m_Target.id);
		if (pUser == nullptr)	
		{
			InitTarget();
			return -1;
		}
		vUser.Set(pUser->GetX(), pUser->GetY(), pUser->GetZ()); 
		vWillUser.Set(pUser->m_fWill_x, pUser->m_fWill_y, pUser->m_fWill_z); 
		fX = pUser->GetX();		
		fZ = pUser->GetZ();

		vDistance = vWillUser - vNpc;
		fWillDis = vDistance.Magnitude();	
		fWillDis = fWillDis - m_proto->m_fBulk;
		bUserType = true;
	}
	else 
	{
		pNpc = g_pMain->m_arNpc.GetData(m_Target.id);
		if(pNpc == nullptr) 
		{
			InitTarget();
			return -1;
		}
		vUser.Set(pNpc->GetX(), pNpc->GetY(), pNpc->GetZ()); 
		fX = pNpc->GetX();
		fZ = pNpc->GetZ();
	}
	
	vDistance = vUser - vNpc;
	fDis = vDistance.Magnitude();	

	fDis = fDis - m_proto->m_fBulk;

	// 작업할것 :	 던젼 몬스터의 경우 일정영역을 벗어나지 못하도록 체크하는 루틴 	
    if ( m_proto->m_tNpcType == NPC_DUNGEON_MONSTER )	{
		if( isInSpawnRange( (int)vUser.x, (int)vUser.z ) == false)
			return -1;	
    }		

	if(Flag == 1)	{
		m_byResetFlag = 1;
		if(pUser)	{
			if(m_Target.x == pUser->GetX() && m_Target.z == pUser->GetZ())
				m_byResetFlag = 0;
		}
		//TRACE("NpcTracing-IsCloseTarget - target_x = %.2f, z=%.2f, dis=%.2f, Flag=%d\n", m_Target.x, m_Target.z, fDis, m_byResetFlag);
	}
	
	if((int)fDis > nRange)	{
		if(Flag == 2)			{
			//TRACE("NpcFighting-IsCloseTarget - target_x = %.2f, z=%.2f, dis=%.2f\n", m_Target.x, m_Target.z, fDis);
			m_byResetFlag = 1;
			m_Target.x = fX;
			m_Target.z = fZ;
		}
		return 0; 
	}

	/* 타겟의 좌표를 최신 것으로 수정하고, 마지막 포인터 좌표를 수정한다,, */
	m_fEndPoint_X = GetX();
	m_fEndPoint_Y = GetZ();
	m_Target.x = fX;
	m_Target.z = fZ;

	//if( m_tNpcLongType && m_proto->m_tNpcType != NPC_BOSS_MONSTER)	{		// 장거리 공격이 가능한것은 공격거리로 판단..
	if( m_tNpcLongType == 1 )	{		// 장거리 공격이 가능한것은 공격거리로 판단..
		if(fDis < LONG_ATTACK_RANGE)	return 1;
		else if(fDis > LONG_ATTACK_RANGE && fDis <= nRange) return 2;
	}
	else	{					// 단거리(직접공격)
		if( Flag == 1 )		{	// 몬스터의 이동하면서이 거리체크시
			if(fDis < (SHORT_ATTACK_RANGE+m_proto->m_fBulk) )	return 1;
			if(fDis > (SHORT_ATTACK_RANGE+m_proto->m_fBulk) && fDis <= nRange) return 2;				// 유저의 현재좌표를 기준으로
			if( bUserType == true )	{	// 유저일때만,, Will좌표를 기준으로 한다
				if(fWillDis > (SHORT_ATTACK_RANGE+m_proto->m_fBulk) && fWillDis <= nRange) return 2;		// 유저의 Will돠표를 기준으로
			}
		}
		else {
			if(fDis < (SHORT_ATTACK_RANGE+m_proto->m_fBulk) )	return 1;
			else if(fDis > (SHORT_ATTACK_RANGE+m_proto->m_fBulk) && fDis <= nRange) return 2;
		}
	}


	//TRACE("Npc-IsCloseTarget - target_x = %.2f, z=%.2f\n", m_Target.x, m_Target.z);
	return 0;
}

//	Target 과 NPC 간 Path Finding을 수행한다.
int CNpc::GetTargetPath(int option)
{
	int nInitType = m_byInitMoveType;
	if(m_byInitMoveType >= 100)	{
		nInitType = m_byInitMoveType - 100;
	}
	// 행동 타입 수정
	if(m_proto->m_tNpcType != 0)	{
		//if(m_byMoveType != m_byInitMoveType)
		//	m_byMoveType = m_byInitMoveType;	// 자기 자리로 돌아갈 수 있도록..
		if(m_byMoveType != nInitType)	m_byMoveType = nInitType;	// 자기 자리로 돌아갈 수 있도록..
	}

	// 추격할때는 뛰는 속도로 맞추어준다...
	m_fSecForMetor = m_fSpeed_2;
	CUser* pUser = nullptr;
	CNpc* pNpc = nullptr;
	float iTempRange = 0.0f;
	__Vector3 vUser, vNpc, vDistance, vEnd22;
	float fDis = 0.0f;
	float fDegree = 0.0f, fTargetDistance = 0.0f;
	float fSurX = 0.0f, fSurZ = 0.0f;

	// Player
	if (m_Target.id < NPC_BAND)	
	{
		pUser = g_pMain->GetUserPtr(m_Target.id);
		if(pUser == nullptr
			|| pUser->isDead()
			|| pUser->GetZoneID() != GetZoneID())
		{
			InitTarget();
			return -1;
		}

		if(option == 1)	{	// magic이나 활등으로 공격 당했다면...
			vNpc.Set(GetX(), GetY(), GetZ());
			vUser.Set(pUser->GetX(), pUser->GetY(), pUser->GetZ()); 
			fDis = GetDistance(vNpc, vUser);
			if(fDis >= NPC_MAX_MOVE_RANGE)		return -1;	// 너무 거리가 멀어서,, 추적이 안되게..
			iTempRange = fDis + 10;
		}
		else	{
			iTempRange = (float)m_bySearchRange;				// 일시적으로 보정한다.
			if(IsDamagedUserList(pUser)) iTempRange = (float)m_byTracingRange;	// 공격받은 상태면 찾을 범위 증가.
			else iTempRange += 2;
		}

		if (m_bTracing
			&& !isInRangeSlow(m_fTracingStartX, m_fTracingStartZ, iTempRange))
		{
			InitTarget();
			return -1;
		}
	}
	// NPC
	else if(m_Target.id >= NPC_BAND && m_Target.id < INVALID_BAND)	{	// Target 이 mon 인 경우
		pNpc = g_pMain->m_arNpc.GetData(m_Target.id);
		if(pNpc == nullptr) {
			InitTarget();
			return false;
		}
		if(pNpc->m_iHP <= 0 || pNpc->m_NpcState == NPC_DEAD)	{
			InitTarget();
			return -1;
		}

		iTempRange = (float)m_byTracingRange;				// 일시적으로 보정한다.
	}

	MAP* pMap = GetMap();
	if (pMap == nullptr) 
		return -1;

	int max_xx = pMap->GetMapSize();
	int max_zz = pMap->GetMapSize();

	int min_x = (int)(GetX() - iTempRange)/TILE_SIZE;	if(min_x < 0) min_x = 0;
	int min_z = (int)(GetZ() - iTempRange)/TILE_SIZE;	if(min_z < 0) min_z = 0;
	int max_x = (int)(GetX() + iTempRange)/TILE_SIZE;	if(max_x > max_xx) max_x = max_xx;
	int max_z = (int)(GetZ() + iTempRange)/TILE_SIZE;	if(min_z > max_zz) min_z = max_zz;

	// Targeting player
	if (m_Target.id < NPC_BAND)	
	{
		if (pUser == nullptr)
			return -1;

		CRect r = CRect(min_x, min_z, max_x+1, max_z+1);
		if (!r.PtInRect((int)pUser->GetX()/TILE_SIZE, (int)pUser->GetZ()/TILE_SIZE))
		{
			TRACE("### Npc-GetTargetPath() User Fail return -1: [nid=%d] t_Name=%s, AttackPos=%d ###\n", GetID(), pUser->GetName().c_str(), m_byAttackPos);
			return -1;
		}

		m_fStartPoint_X = GetX();		m_fStartPoint_Y = GetZ();
	
		vNpc.Set(GetX(), GetY(), GetZ());
		vUser.Set(pUser->GetX(), pUser->GetY(), pUser->GetZ()); 
		
		IsSurround(pUser);

		if(m_byAttackPos > 0 && m_byAttackPos < 9)	{
			fDegree = (float)((m_byAttackPos-1)*45);
			fTargetDistance = 2.0f+m_proto->m_fBulk;
			ComputeDestPos(vUser, 0.0f, fDegree, fTargetDistance, &vEnd22);
			fSurX = vEnd22.x - vUser.x;			fSurZ = vEnd22.z - vUser.z;
			m_fEndPoint_X = vUser.x + fSurX;	m_fEndPoint_Y = vUser.z + fSurZ;
		}
		else
		{
			CalcAdaptivePosition(vNpc, vUser, 2.0f+m_proto->m_fBulk, &vEnd22);
			m_fEndPoint_X = vEnd22.x;	m_fEndPoint_Y = vEnd22.z;
		}
	}
	else 
	{
		if (pNpc == nullptr)
			return -1;

		CRect r = CRect(min_x, min_z, max_x+1, max_z+1);
		if (!r.PtInRect((int)pNpc->GetX()/TILE_SIZE, (int)pNpc->GetZ()/TILE_SIZE))
		{
			TRACE("### Npc-GetTargetPath() Npc Fail return -1: [nid=%d] t_Name=%s, AttackPos=%d ###\n", GetID(), pNpc->GetName().c_str(), m_byAttackPos);
			return -1;
		}

		m_fStartPoint_X = GetX();		m_fStartPoint_Y = GetZ();
	
		vNpc.Set(GetX(), GetY(), GetZ());
		vUser.Set(pNpc->GetX(), pNpc->GetY(), pNpc->GetZ()); 
		
		CalcAdaptivePosition(vNpc, vUser, 2.0f+m_proto->m_fBulk, &vEnd22);
		m_fEndPoint_X = vEnd22.x;	m_fEndPoint_Y = vEnd22.z;
	}

	vDistance = vEnd22 - vNpc;
	fDis = vDistance.Magnitude();

	if(fDis <= m_fSecForMetor)	{
		ClearPathFindData();
		m_bPathFlag = true;
		m_iAniFrameIndex = 1;
		m_pPoint[0].fXPos = m_fEndPoint_X;
		m_pPoint[0].fZPos = m_fEndPoint_Y;
		return true;
	}
	
	if((int)fDis > iTempRange)	{
		TRACE("Npc-GetTargetPath() searchrange over Fail return -1: [nid=%d,%s]\n", GetID(), GetName().c_str());
		return -1; 
	}

	
	if (m_proto->m_tNpcType != NPC_DUNGEON_MONSTER && hasTarget())	
		return 0;	

	CPoint start, end;
	start.x = (int)(GetX()/TILE_SIZE) - min_x;
	start.y = (int)(GetZ()/TILE_SIZE) - min_z;
	end.x = (int)(vEnd22.x/TILE_SIZE) - min_x;
	end.y = (int)(vEnd22.z/TILE_SIZE) - min_z;

	// 작업할것 :	 던젼 몬스터의 경우 일정영역을 벗어나지 못하도록 체크하는 루틴 	
    if ( m_proto->m_tNpcType == NPC_DUNGEON_MONSTER )	{
		if( isInSpawnRange( (int)vEnd22.x, (int)vEnd22.z ) == false)
			return -1;	
    }

	m_min_x = min_x;
	m_min_y = min_z;
	m_max_x = max_x;
	m_max_y = max_z;

	// Run Path Find ---------------------------------------------//
	return PathFind(start, end, m_fSecForMetor);
}

int CNpc::Attack()
{
	// 텔레포트 가능하게,, (렌덤으로,, )
	int nRandom = 0, nPercent=1000;
	bool bTeleport = false;

	if (isNonAttackingObject())
	{
		m_NpcState = NPC_STANDING;
		InitTarget();
		return 0;
	}	

	if( m_tNpcLongType == 1 )	{		// 장거리 공격이 가능한것은 공격거리로 판단..
		return LongAndMagicAttack();
	}
		
	int ret = 0;
	int nStandingTime = m_sStandTime;

	ret = IsCloseTarget(m_byAttackRange, 2);

	if(ret == 0)   {
		m_sStepCount = 0;
		m_byActionFlag = ATTACK_TO_TRACE;
		m_NpcState = NPC_TRACING;			// 공격하고 도망가는 유저를 따라 잡기위해(반응을 좀더 빠르게) 
		return 0;							// IsCloseTarget()에 유저 x, y값을 갱신하고 Delay = 0으로 줌
	}	
	else if( ret == 2 )	{
		//if(m_proto->m_tNpcType == NPC_BOSS_MONSTER)	{		// 대장 몬스터이면.....
		if(m_tNpcLongType == 2)	{		// 직접, 간접(롱)공격이 가능한 몬스터 이므로 장거리 공격을 할 수 있다.
			return LongAndMagicAttack();
		}
		else	{
			m_sStepCount = 0;
			m_byActionFlag = ATTACK_TO_TRACE;
			m_NpcState = NPC_TRACING;			// 공격하고 도망가는 유저를 따라 잡기위해(반응을 좀더 빠르게) 
			return 0;							// IsCloseTarget()에 유저 x, y값을 갱신하고 Delay = 0으로 줌
		}
	}
	else if( ret == -1 )	{
		m_NpcState = NPC_STANDING;
		InitTarget();
		return 0;
	}

	int		nDamage		= 0;
	uint16 nID = m_Target.id;					// Target 을 구한다.

	// Targeting player
	if (nID < NPC_BAND)	
	{
		CUser * pUser = g_pMain->GetUserPtr(nID);
		if (pUser == nullptr)	
		{
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if(pUser->m_bLive == AI_USER_DEAD)	{		// User 가 이미 죽은경우
			//SendAttackSuccess(ATTACK_TARGET_DEAD_OK, pUser->m_iUserId, 0, pUser->m_iHP);
			SendAttackSuccess(ATTACK_TARGET_DEAD_OK, pUser->m_iUserId, 0, 0);
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if (pUser->m_bInvisibilityType
			/*|| pUser->m_state == STATE_DISCONNECTED*/)
		{
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if(pUser->m_byIsOP == MANAGER_USER)	{	// 운영자는 공격을 안하게..
			InitTarget();
			m_NpcState = NPC_MOVING;
			return nStandingTime;
		}
		// Npc와 유저와의 HP를 비교하여.. 도망을 갈 것인지를 판단...
	/*	if(m_byNpcEndAttType)	{
			if(IsCompStatus(pUser) == true)	{
				m_NpcState = NPC_BACK;
				return 0;
			}	
		}	*/

		if(m_byWhatAttackType == 4 || m_byWhatAttackType == 5)	
		{
			nRandom = myrand(1, 10000);
			if(nRandom < nPercent)	
			{
				CNpcMagicProcess::MagicPacket(MAGIC_EFFECTING, m_proto->m_iMagic2, GetID(), -1, int16(GetX()), int16(GetY()), int16(GetZ()));
				//TRACE("++++ AreaMagicAttack --- sid=%d, magicid=%d\n", GetID(), m_proto->m_iMagic2);
				return m_sAttackDelay + 1000;
			}
		}
		else	{
			if(m_byWhatAttackType == 2)	{
				nRandom = myrand(1, 10000);

				if (nRandom < nPercent)	
				{
					Packet result(AG_MAGIC_ATTACK_RESULT, uint8(MAGIC_EFFECTING));
					result	<< m_proto->m_iMagic1 << GetID() << pUser->m_iUserId
							<< uint16(0) << uint16(0) << uint16(0) << uint16(0) << uint16(0) << uint16(0);
					g_pMain->Send(&result);

					//TRACE("LongAndMagicAttack --- sid=%d, tid=%d\n", GetID(), pUser->m_iUserId);
					return m_sAttackDelay;
				}
			}
		}

		nDamage = GetFinalDamage(pUser);	// 최종 대미지
		
		if(nDamage > 0) {
			pUser->SetDamage(nDamage, GetID());
			if(pUser->m_bLive != AI_USER_DEAD)	{
				SendAttackSuccess(ATTACK_SUCCESS, pUser->m_iUserId, nDamage, pUser->m_sHP);
			}
		}
		else
			SendAttackSuccess(ATTACK_FAIL, pUser->m_iUserId, nDamage, pUser->m_sHP);
	}
	else // Targeting NPC
	{
		CNpc * pNpc = g_pMain->m_arNpc.GetData(nID);
		if (pNpc == nullptr)	
		{
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		// healer이면서 같은국가의 NPC인경우에는 힐
		if (m_proto->m_tNpcType == NPC_HEALER && pNpc->GetNation() == GetNation())
		{
			m_NpcState = NPC_HEALING;
			return 0;
		}

		if (pNpc->isDead())
		{
			SendAttackSuccess(ATTACK_TARGET_DEAD, pNpc->GetID(), 0, 0);
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		// Npc와 유저와의 HP를 비교하여.. 도망을 갈 것인지를 판단...
	/*	if(IsCompStatus(pUser) == true)	{
			m_NpcState = NPC_BACK;
			return 0;
		}	*/

		// MoveAttack
		//MoveAttack();

		// 명중이면 //Damage 처리 ----------------------------------------------------------------//
		nDamage = GetNFinalDamage(pNpc);	// 최종 대미지

		if(nDamage > 0)	{
			pNpc->SetDamage(0, nDamage, GetID());
			//if(pNpc->m_iHP > 0)
			SendAttackSuccess(ATTACK_SUCCESS, pNpc->GetID(), nDamage, pNpc->m_iHP);
		}
		else
			SendAttackSuccess(ATTACK_FAIL, pNpc->GetID(), nDamage, pNpc->m_iHP);
	}

	return m_sAttackDelay;
}

int CNpc::LongAndMagicAttack()
{
	int ret = 0;
	int nStandingTime = m_sStandTime;

	ret = IsCloseTarget(m_byAttackRange, 2);

	if(ret == 0)	{
		m_sStepCount = 0;
		m_byActionFlag = ATTACK_TO_TRACE;
		m_NpcState = NPC_TRACING;			// 공격하고 도망가는 유저를 따라 잡기위해(반응을 좀더 빠르게) 
		return 0;							// IsCloseTarget()에 유저 x, y값을 갱신하고 Delay = 0으로 줌
	}	
	else if( ret == 2 )	{
		//if(m_proto->m_tNpcType != NPC_BOSS_MONSTER)	{		// 대장 몬스터이면.....
		if(m_tNpcLongType == 1)	{		// 장거리 몬스터이면.....
			m_sStepCount = 0;
			m_byActionFlag = ATTACK_TO_TRACE;
			m_NpcState = NPC_TRACING;			// 공격하고 도망가는 유저를 따라 잡기위해(반응을 좀더 빠르게) 
			return 0;							// IsCloseTarget()에 유저 x, y값을 갱신하고 Delay = 0으로 줌
		}
	}
	if( ret == -1 )	{
		m_NpcState = NPC_STANDING;
		InitTarget();
		return 0;
	}

	CNpc*	pNpc		= nullptr;	
	CUser*	pUser		= nullptr;
	int		nDamage		= 0;
	uint16 nID = m_Target.id;

	if (nID < NPC_BAND)	
	{
		pUser = g_pMain->GetUserPtr(nID);
		if (pUser == nullptr)	
		{
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if(pUser->m_bLive == AI_USER_DEAD)	{		// User 가 이미 죽은경우
			SendAttackSuccess(ATTACK_TARGET_DEAD_OK, pUser->m_iUserId, 0, 0);
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if (pUser->m_bInvisibilityType
			/*|| pUser->m_state == STATE_DISCONNECTED*/)
		{
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if(pUser->m_byIsOP == MANAGER_USER)	{		// 운영자는 공격을 안하게..
			InitTarget();
			m_NpcState = NPC_MOVING;
			return nStandingTime;
		}
		// Npc와 유저와의 HP를 비교하여.. 도망을 갈 것인지를 판단...
	/*	if(m_byNpcEndAttType)
		{
			if(IsCompStatus(pUser) == true)
			{
				m_NpcState = NPC_BACK;
				return 0;
			}	
		}	*/

		CNpcMagicProcess::MagicPacket(MAGIC_CASTING, m_proto->m_iMagic1, GetID(), pUser->m_iUserId);
		//TRACE("**** LongAndMagicAttack --- sid=%d, tid=%d\n", GetID(), pUser->m_iUserId);
	}
	else // Target monster/NPC 
	{
		CNpc * pNpc = g_pMain->m_arNpc.GetData(nID);
		if (pNpc == nullptr)	
		{
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		if(pNpc->m_iHP <= 0 || pNpc->m_NpcState == NPC_DEAD)	{
			SendAttackSuccess(ATTACK_TARGET_DEAD, pNpc->GetID(), 0, 0);
			InitTarget();
			m_NpcState = NPC_STANDING;
			return nStandingTime;
		}

		// Npc와 유저와의 HP를 비교하여.. 도망을 갈 것인지를 판단...
	/*	if(IsCompStatus(pUser) == true)
		{
			m_NpcState = NPC_BACK;
			return 0;
		}	*/
	}

	return m_sAttackDelay;
}

bool CNpc::TracingAttack()
{
	int nDamage = 0;
	uint16 nID = m_Target.id;

	if (nID < NPC_BAND)	// Target is a player
	{
		CUser * pUser = g_pMain->GetUserPtr(nID);
		if (pUser == nullptr)
			return false;

		if (pUser->m_bLive == AI_USER_DEAD)		
		{
			SendAttackSuccess(ATTACK_TARGET_DEAD_OK, pUser->m_iUserId, 0, 0);
			return false;
		}

		if (pUser->m_bInvisibilityType
			/*|| pUser->m_state == STATE_DISCONNECTED*/
			|| pUser->m_byIsOP == MANAGER_USER)
			return false;

		nDamage = GetFinalDamage(pUser);
		
		if (nDamage > 0)		
		{
			pUser->SetDamage(nDamage, GetID());
			if (pUser->m_bLive != AI_USER_DEAD)
				SendAttackSuccess(ATTACK_SUCCESS, pUser->m_iUserId, nDamage, pUser->m_sHP);
		}
		else
			SendAttackSuccess(ATTACK_FAIL, pUser->m_iUserId, nDamage, pUser->m_sHP);
	}
	else // Target is an NPC/monster
	{
		CNpc * pNpc = g_pMain->m_arNpc.GetData(nID);
		if (pNpc == nullptr)
			return false;

		if (pNpc->isDead())
		{
			SendAttackSuccess(ATTACK_TARGET_DEAD, pNpc->GetID(), 0, 0);
			return false;
		}

		nDamage = GetNFinalDamage(pNpc);

		if (nDamage <= 0)
		{
			SendAttackSuccess(ATTACK_FAIL, pNpc->GetID(), nDamage, pNpc->m_iHP);
		}
		else
		{
			if (!pNpc->SetDamage(0, nDamage, GetID())) 
			{
				// SendAttackSuccess(ATTACK_SUCCESS, pNpc->GetID(), nDamage, pNpc->m_iHP);
				SendAttackSuccess(ATTACK_TARGET_DEAD, pNpc->GetID(), nDamage, pNpc->m_iHP);
				return false;
			}

			SendAttackSuccess(ATTACK_SUCCESS, pNpc->GetID(), nDamage, pNpc->m_iHP);
		}
	}

	return true;
}

void CNpc::MoveAttack()
{
	if (!hasTarget())
		return;

	__Vector3 vUser, vNpc;
	__Vector3 vDistance, vEnd22;

	float fDis = 0.0f;
	float fX = 0.0f, fZ = 0.0f;

	vNpc.Set(GetX(), GetY(), GetZ());

	if (m_Target.id < NPC_BAND)	// Target is a player
	{
		__Vector3 vUser;
		CUser * pUser = g_pMain->GetUserPtr(m_Target.id);
		if (pUser == nullptr) 
		{
			InitTarget();
			return;
		}
		vUser.Set(pUser->GetX(), pUser->GetY(), pUser->GetZ()); 

		CalcAdaptivePosition(vNpc, vUser, 2, &vEnd22);

		if(m_byAttackPos > 0 && m_byAttackPos < 9)
		{
			fX = vUser.x + surround_fx[m_byAttackPos-1];	fZ = vUser.z + surround_fz[m_byAttackPos-1];
			vEnd22.Set(fX, 0, fZ);
		}
		else
		{
			fX = vEnd22.x;	fZ = vEnd22.z;
		}
	}
	else // Target is an NPC/monster
	{
		CNpc * pNpc = g_pMain->m_arNpc.GetData(m_Target.id);		
		if (pNpc == nullptr) 
		{
			InitTarget();
			return;
		}
		vUser.Set(pNpc->GetX(), pNpc->GetY(), pNpc->GetZ()); 

		CalcAdaptivePosition(vNpc, vUser, 2, &vEnd22);
		fX = vEnd22.x;	fZ = vEnd22.z;
	}
	
	vDistance = vUser - vNpc;
	fDis = vDistance.Magnitude();	
	
	if ((int)fDis < 3) return;

	vDistance = vEnd22 - vNpc;
	fDis = vDistance.Magnitude();	
	m_curx = vEnd22.x;
	m_curz = vEnd22.z;

	if (GetX() < 0 || GetZ() < 0)
	{
		TRACE("Npc-MoveAttack : nid=(%d, %s), x=%.2f, z=%.2f\n", 
			GetID(), GetName().c_str(), GetX(), GetZ());
	}


	Packet result(MOVE_RESULT, uint8(SUCCESS));
	result	<< GetID()
			<< GetX() << GetZ() << GetY();

	// This seems really dumb, but for reasons unbeknownst to me
	// we're sending the packet twice -- first with the distance/speed,
	// second without. 
	int wpos = result.wpos(); 
	result << fDis;
	g_pMain->Send(&result); // send the first packet

	result.put(wpos, 0.0f); // replace the distance/speed with 0
	g_pMain->Send(&result); // send it again

	RegisterRegion(GetX(), GetZ());
	
	m_fEndPoint_X = GetX();
	m_fEndPoint_Y = GetZ();
}

int CNpc::GetNFinalDamage(CNpc *pNpc)
{
	short damage = 0;
	float Attack = 0;
	float Avoid = 0;
	short Hit = 0;
	short Ac = 0;
	int random = 0;
	uint8 result;

	if(pNpc == nullptr)	return damage;

	// 공격민첩
	Attack = (float)m_sHitRate;

	// 방어민첩
	Avoid = (float)pNpc->m_sEvadeRate;

	//공격자 Hit 
	Hit = m_sDamage;
	
	// 방어자 Ac 
	Ac = (short)pNpc->m_sDefense;

	// 타격비 구하기
	result = GetHitRate(Attack/Avoid);		

	switch(result)
	{
//	case GREAT_SUCCESS:
//		damage = (short)(0.6 * (2 * Hit));
//		if(damage <= 0){
//			damage = 0;
//			break;
//		}
//		damage = myrand(0, damage);
//		damage += (short)(0.7 * (2 * Hit));
//		break;
	case GREAT_SUCCESS:
		damage = (short)(0.6 * Hit);
		if(damage <= 0){
			damage = 0;
			break;
		}
		damage = myrand(0, damage);
		damage += (short)(0.7 * Hit);
		break;
	case SUCCESS:
	case NORMAL:
		if(Hit - Ac > 0)
		{
			damage = (short)(0.6 * (Hit - Ac));
			if(damage <= 0){
				damage = 0;
			break;
			}
			damage = myrand(0, damage);
			damage += (short)(0.7 * (Hit - Ac));
		}
		else
			damage = 0;
		break;
	case FAIL:
		damage = 0;
		break;
	}
	
	return damage;	
}

bool CNpc::IsCompStatus(CUser* pUser)
{
	
	if(IsHPCheck(pUser->m_sHP) == true)
	{
		if(RandomBackMove() == false)
			return false;
		else 
			return true;
	}

	return false;
}

//	Target 의 위치가 다시 길찾기를 할 정도로 변했는지 판단
bool CNpc::IsChangePath()
{
	// 패스파인드의 마지막 좌표를 가지고,, Target이 내 공격거리에 있는지를 판단,,
//	if(!m_pPath) return true;

	float fCurX=0.0f, fCurZ=0.0f;
	GetTargetPos(fCurX, fCurZ);

	__Vector3 vStart, vEnd;
	vStart.Set(m_fEndPoint_X, 0, m_fEndPoint_Y);
	vEnd.Set(fCurX, 0, fCurZ);
	
	float fDis = GetDistance(vStart, vEnd);
	float fCompDis = 3.0f;

	if(fDis < fCompDis)
		return false;

	return true;
}

bool CNpc::GetTargetPos(float& x, float& z)
{
	if (!hasTarget())
		return false;

	if (m_Target.id < NPC_BAND)
	{
		CUser* pUser = g_pMain->GetUserPtr(m_Target.id);
		if (pUser == nullptr)
			return false;

		x = pUser->GetX();
		z = pUser->GetZ();
	}
	else 
	{
		CNpc* pNpc = g_pMain->m_arNpc.GetData(m_Target.id);
		if (pNpc == nullptr)
			return false;

		x = pNpc->GetX();
		z = pNpc->GetZ();
	}

	return true;
}

//	Target 과 NPC 간에 길찾기를 다시한다.
bool CNpc::ResetPath()
{
	float cur_x, cur_z;
	GetTargetPos(cur_x, cur_z);

//	TRACE("ResetPath : user pos ,, x=%.2f, z=%.2f\n", cur_x, cur_z);

	m_Target.x = cur_x;
	m_Target.z = cur_z;

	int nValue = GetTargetPath();
	if(nValue == -1)		// 타겟이 없어지거나,, 멀어졌음으로...
	{
		TRACE("Npc-ResetPath Fail - target_x = %.2f, z=%.2f, value=%d\n", m_Target.x, m_Target.z, nValue);
		return false;
	}
	else if(nValue == 0)	// 타겟 방향으로 바로 간다..
	{
		m_fSecForMetor = m_fSpeed_2;	// 공격일때는 뛰는 속도로... 
		IsNoPathFind(m_fSecForMetor);
	}

	//TRACE("Npc-ResetPath - target_x = %.2f, z=%.2f, value=%d\n", m_Target.x, m_Target.z, nValue);

	return true;	
}

int CNpc::GetFinalDamage(CUser *pUser, int type)
{
	short damage = 0;
	float Attack = 0;
	float Avoid = 0;
	short Hit = 0;
	short Ac = 0;
	short HitB = 0;
	int random = 0;
	uint8 result;

	if(pUser == nullptr)	return damage;
	
	Attack = (float)m_sHitRate;		// 공격민첩
	Avoid = (float)pUser->m_fAvoidrate;		// 방어민첩	
	Hit = m_sDamage;	// 공격자 Hit 		
	Ac = (short)pUser->m_sItemAC + (short)pUser->GetLevel() + (short)(pUser->m_sAC - pUser->GetLevel() - pUser->m_sItemAC);
	HitB = (int)((Hit * 200) / (Ac + 240)) ;

	int nMaxDamage = (int)(2.6 * m_sDamage);

	// 타격비 구하기
	result = GetHitRate(Attack/Avoid);	
	
//	TRACE("Hitrate : %d     %f/%f\n", result, Attack, Avoid);

	switch(result)
	{
	case GREAT_SUCCESS:
/*
		damage = (short)(0.6 * (2 * Hit));
		if(damage <= 0){
			damage = 0;
			break;
		}
		damage = myrand(0, damage);
		damage += (short)(0.7 * (2 * Hit));
		break;
*/	
//		damage = 0;
//		break;

		damage = (short)HitB;
		if(damage <= 0){
			damage = 0;
			break;
		}

		damage = (int)(0.3f * myrand(0, damage));
		damage += (short)(0.85f * (float)HitB);
//		damage = damage * 2;
		damage = (damage * 3) / 2;
		break;

	case SUCCESS:
/*
		damage = (short)(0.6f * Hit);
		if(damage <= 0){
			damage = 0;
			break;
		}
		damage = myrand(0, damage);
		damage += (short)(0.7f * Hit);
		break;
*/
/*
		damage = (short)(0.3f * (float)HitB);
		if(damage <= 0){
			damage = 0;
			break;
		}
		damage = myrand(0, damage);
		damage += (short)(0.85f * (float)HitB);
*/
/*
		damage = (short)HitB;
		if(damage <= 0){
			damage = 0;
			break;
		}
		damage = (int)(0.3f * myrand(0, damage));
		damage += (short)(0.85f * (float)HitB);
		damage = damage * 2;

		break;
*/
	case NORMAL:
		/*
		if(Hit - Ac > 0){
			damage = (short)(0.6f * (Hit - Ac));
			if(damage <= 0){
				damage = 0;
				break;
			}
			damage = myrand(0, damage);
			damage += (short)(0.7f * (Hit - Ac));

		}
		else
			damage = 0;
		*/
/*
		damage = (short)(0.3f * (float)HitB);
		if(damage <= 0){
			damage = 0;
			break;
		}
		damage = myrand(0, damage);
		damage += (short)(0.85f * (float)HitB);
*/
		damage = (short)HitB;
		if(damage <= 0){
			damage = 0;
			break;
		}
		damage = (int)(0.3f * myrand(0, damage));
		damage += (short)(0.85f * (float)HitB);

		break;

	case FAIL:
		damage = 0;

		break;
	}
	
	if(damage > nMaxDamage)	{
		TRACE("#### Npc-GetFinalDamage Fail : nid=%d, result=%d, damage=%d, maxdamage=%d\n", GetID(), result, damage, nMaxDamage);
		damage = nMaxDamage;
	}

	return damage;	
}

//	나를 공격한 유저를 타겟으로 삼는다.(기준 : 렙과 HP를 기준으로 선정)
void CNpc::ChangeTarget(int nAttackType, CUser *pUser)
{
	int preDamage, lastDamage;
	__Vector3 vUser, vNpc;
	float fDistance1 = 0.0f, fDistance2 = 0.0f;
	int iRandom = myrand(0, 100);

	if (pUser == nullptr
		|| pUser->m_bLive == AI_USER_DEAD
		|| pUser->GetNation() == GetNation()
		|| pUser->m_bInvisibilityType
		|| pUser->m_byIsOP == MANAGER_USER
		|| m_NpcState == NPC_FAINTING
		|| isNonAttackingObject())
		return;

	CUser *preUser = nullptr;
	if (hasTarget() && m_Target.id < NPC_BAND)	
		preUser = g_pMain->GetUserPtr(m_Target.id);

	if (pUser == preUser)	
	{
		if (m_tNpcGroupType)	 {			// 가족타입이면 시야안에 같은 타입에게 목표 지정
			m_Target.failCount = 0;
			if(m_proto->m_tNpcType == NPC_BOSS)	FindFriend(1);
			else		FindFriend();
		}
		else	{
			if(m_proto->m_tNpcType == NPC_BOSS)	{
				m_Target.failCount = 0;
				FindFriend(1);
			}
		}
		return;
	}
	if(preUser != nullptr/* && preUser->m_state == GAME_STATE_INGAME */)
	{
		preDamage = 0; lastDamage = 0;

		if(iRandom >= 0 && iRandom < 50)	{			// 몬스터 자신을 가장 강하게 타격한 유저
			preDamage = preUser->GetDamage(GetID());
			lastDamage = pUser->GetDamage(GetID());
			//TRACE("Npc-changeTarget 111 - iRandom=%d, pre=%d, last=%d\n", iRandom, preDamage, lastDamage);
			if(preDamage > lastDamage) return;
		}
		else if(iRandom >= 50 && iRandom < 80)	{		// 가장 가까운 플레이어
			vNpc.Set(GetX(), GetY(), GetZ());
			vUser.Set(preUser->GetX(), 0, preUser->GetZ());
			fDistance1 = GetDistance(vNpc, vUser);
			vUser.Set(pUser->GetX(), 0, pUser->GetZ());
			fDistance2 = GetDistance(vNpc, vUser);
			//TRACE("Npc-changeTarget 222 - iRandom=%d, preDis=%.2f, lastDis=%.2f\n", iRandom, fDistance1, fDistance2);
			if(fDistance2 > fDistance1)	return;
		}
		if(iRandom >= 80 && iRandom < 95)		{		// 몬스터가 유저에게 가장 많이 타격을 줄 수 있는 유저
			preDamage = GetFinalDamage(preUser, 0);
			lastDamage = GetFinalDamage(pUser, 0); 
			//TRACE("Npc-changeTarget 333 - iRandom=%d, pre=%d, last=%d\n", iRandom, preDamage, lastDamage);
			if(preDamage > lastDamage) return;
		}
		if(iRandom >= 95 && iRandom < 101)		{		// Heal Magic을 사용한 유저
		}
	}
	else if(preUser == nullptr && nAttackType == 1004)		return;		// Heal magic에 반응하지 않도록..
		
	m_Target.id	= pUser->m_iUserId;
	m_Target.bSet = true;
	m_Target.x	= pUser->GetX();
	m_Target.y	= pUser->GetY();
	m_Target.z  = pUser->GetZ();

	//TRACE("Npc-changeTarget - target_x = %.2f, z=%.2f\n", m_Target.x, m_Target.z);

	int nValue = 0;
	// 어슬렁 거리는데 공격하면 바로 반격
	if(m_NpcState == NPC_STANDING || m_NpcState == NPC_MOVING || m_NpcState == NPC_SLEEPING)
	{									// 가까이 있으면 반격으로 이어지구
		if(IsCloseTarget(pUser, m_byAttackRange) == true)
		{
			m_NpcState = NPC_FIGHTING;
			m_Delay = 0;
		}
		else							// 바로 도망가면 좌표를 갱신하고 추적	
		{
			nValue = GetTargetPath(1);
			if(nValue == 1)	// 반격 동작후 약간의 딜레이 시간이 있음	
			{
				m_NpcState = NPC_TRACING;
				m_Delay = 0;
			}
			else if(nValue == -1)
			{
				m_NpcState = NPC_STANDING;
				m_Delay = 0;
			}
			else if(nValue == 0)
			{
				m_fSecForMetor = m_fSpeed_2;	// 공격일때는 뛰는 속도로... 
				IsNoPathFind(m_fSecForMetor);
				m_NpcState = NPC_TRACING;
				m_Delay = 0;
			}
		}
	}
//	else m_NpcState = NPC_ATTACKING;	// 한참 공격하는데 누가 방해하면 목표를 바꿈

	if(m_tNpcGroupType)	 {			// 가족타입이면 시야안에 같은 타입에게 목표 지정
		m_Target.failCount = 0;
		if(m_proto->m_tNpcType == NPC_BOSS)	FindFriend(1);
		else		FindFriend();
	}
	else	{
		if(m_proto->m_tNpcType == NPC_BOSS)	{
			m_Target.failCount = 0;
			FindFriend(1);
		}
	}
}

//	나를 공격한 Npc를 타겟으로 삼는다.(기준 : 렙과 HP를 기준으로 선정)
void CNpc::ChangeNTarget(CNpc *pNpc)
{
	int preDamage, lastDamage;
	__Vector3 vMonster, vNpc;
	float fDist = 0.0f;

	if (pNpc == nullptr
		|| pNpc->m_NpcState == NPC_DEAD
		|| !hasTarget()
		|| m_Target.id < NPC_BAND)
		return;

	CNpc *preNpc = g_pMain->m_arNpc.GetData(m_Target.id);
	if (preNpc == nullptr
		|| pNpc == preNpc) return;

	preDamage = 0; lastDamage = 0;
	preDamage = GetNFinalDamage(preNpc);
	lastDamage = GetNFinalDamage(pNpc); 

	vNpc.Set(GetX(), GetY(), GetZ());
	vMonster.Set(preNpc->GetX(), 0, preNpc->GetZ());
	fDist = GetDistance(vNpc, vMonster);
	preDamage = (int)((double)preDamage/fDist + 0.5);
	vMonster.Set(pNpc->GetX(), 0, pNpc->GetZ());
	fDist = GetDistance(vNpc, vMonster);
	lastDamage = (int)((double)lastDamage/fDist + 0.5);

	if(preDamage > lastDamage) return;
		
	m_Target.id	= pNpc->GetID();
	m_Target.bSet = true;
	m_Target.x	= pNpc->GetX();
	m_Target.y	= pNpc->GetZ();
	m_Target.z  = pNpc->GetZ();

	int nValue = 0;
	if (m_NpcState == NPC_STANDING || m_NpcState == NPC_MOVING || m_NpcState == NPC_SLEEPING)
	{
		if (IsCloseTarget(m_byAttackRange) == 1)
		{
			m_NpcState = NPC_FIGHTING;
			m_Delay = 0;
		}
		else
		{
			nValue = GetTargetPath();
			if (nValue == 1)
			{
				m_NpcState = NPC_TRACING;
				m_Delay = 0;
			}
			else if(nValue == -1)
			{
				m_NpcState = NPC_STANDING;
				m_Delay = 0;
			}
			else if(nValue == 0)
			{
				m_fSecForMetor = m_fSpeed_2;
				IsNoPathFind(m_fSecForMetor);
				m_NpcState = NPC_TRACING;
				m_Delay = 0;
			}
		}
	}
//	else m_NpcState = NPC_ATTACKING;	// 한참 공격하는데 누가 방해하면 목표를 바꿈

	if(m_tNpcGroupType)					// 가족타입이면 시야안에 같은 타입에게 목표 지정
	{
		m_Target.failCount = 0;
		FindFriend();
	}
}


void CNpc::ToTargetMove(CUser* pUser)
{
	TRACE("### ToTargetMove() 유저 길찾기 실패 ### \n");
}

//	NPC 의 방어력을 얻어온다.
int CNpc::GetDefense()
{
	return m_sDefense;
}

bool CNpc::SetDamage(int nAttackType, int nDamage, uint16 uid, int iDeadType /*= 0*/)
{
	bool bIsDurationDamage = (nAttackType < 0);
	int i=0, len=0;
	int userDamage = 0;
	bool bFlag = false;
	_ExpUserList *tempUser = nullptr;

	if(m_NpcState == NPC_DEAD) return true;
	if(m_iHP <= 0) return true;
	if(nDamage < 0) return true;

	CUser* pUser = nullptr;
	CNpc* pNpc = nullptr;
	char strDurationID[MAX_ID_SIZE+1];

	const char * id = nullptr;

	if (uid < NPC_BAND)	{	// Target 이 User 인 경우
		pUser = g_pMain->GetUserPtr(uid);	// 해당 사용자인지 인증
		if(pUser == nullptr) return true;
		id = pUser->GetName().c_str();
	}
	else if (uid >= NPC_BAND && uid < INVALID_BAND)	{	// Target 이 mon 인 경우
		pNpc = g_pMain->m_arNpc.GetData(uid);
		if(pNpc == nullptr) return true;
		userDamage = nDamage;
		id = pNpc->GetName().c_str();
		goto go_result;
	}

	userDamage = nDamage;		
													// 잉여 데미지는 소용없다.		
	if( (m_iHP - nDamage) < 0 ) userDamage = m_iHP;

	for(i = 0; i < NPC_HAVE_USER_LIST; i++)	{
		if(m_DamagedUserList[i].iUid == uid)	{
			if (bIsDurationDamage)
			{
				bFlag = true;
				strncpy(strDurationID, pUser->GetName().c_str(), sizeof(strDurationID));
				if (STRCASECMP(m_DamagedUserList[i].strUserID, strDurationID) == 0)	{
					m_DamagedUserList[i].nDamage += userDamage; 
					goto go_result;
				}
			}
			else if (STRCASECMP(m_DamagedUserList[i].strUserID, id) == 0) 
			{ 
				m_DamagedUserList[i].nDamage += userDamage; 
				goto go_result;
			}
		}
	}

	for(i = 0; i < NPC_HAVE_USER_LIST; i++)				// 인원 제한이 최종 대미지에 영향을 미치나?
	{
		if(m_DamagedUserList[i].iUid == -1)
		{
			if(m_DamagedUserList[i].nDamage <= 0)
			{
				len = strlen(id);
				if( len > MAX_ID_SIZE || len <= 0 ) {
					TRACE("###  Npc SerDamage Fail ---> uid = %d, name=%s, len=%d, id=%s  ### \n", GetID(), GetName().c_str(), len, id);
					continue;
				}
				if(bFlag == true)	strncpy(m_DamagedUserList[i].strUserID, strDurationID, sizeof(m_DamagedUserList[i].strUserID));
				else	{
					if (STRCASECMP("**duration**", id) == 0) {
						strcpy(m_DamagedUserList[i].strUserID, pUser->GetName().c_str());
					}
					else strcpy(m_DamagedUserList[i].strUserID, id);
				}
				m_DamagedUserList[i].iUid = uid;
				m_DamagedUserList[i].nDamage = userDamage;
				m_DamagedUserList[i].bIs = false;
				break;
			}
		}
	}

go_result:
	m_TotalDamage += userDamage;
	m_iHP -= nDamage;	

	if( m_iHP <= 0 )
	{
		m_iHP = 0;
		Dead(iDeadType);
		SendExpToUserList();
		GiveNpcHaveItem();
		return false;
	}

	int iRandom = myrand(1, 100);
	int iLightningR = 0;

	if (uid < NPC_BAND)	// Target 이 User 인 경우
	{
		if(nAttackType == 3 && m_NpcState != NPC_FAINTING)	{			// 기절 시키는 스킬을 사용했다면..
			// 확률 계산..
			iLightningR = (int)(10 + (40 - 40 * ( (double)m_byLightningR / 80)));
			if( COMPARE(iRandom, 0, iLightningR) )	{
				m_NpcState = NPC_FAINTING;
				m_Delay = 0;
				m_tFaintingTime = UNIXTIME;
			}
			else	ChangeTarget(nAttackType, pUser);
		}
		else	{
			ChangeTarget(nAttackType, pUser);
		}
	}
	else // Target 이 mon 인 경우
	{
		ChangeNTarget(pNpc);
	}

	return true;
}

bool CNpc::SetHMagicDamage(int nDamage)
{
	if (isDead()
		|| nDamage <= 0)
		return false;

	int oldHP = 0;

	oldHP = m_iHP;
	m_iHP += nDamage;
	if( m_iHP < 0 )
		m_iHP = 0;
	else if ( m_iHP > m_iMaxHP )
		m_iHP = m_iMaxHP;

	TRACE("Npc - SetHMagicDamage(), nid=%d,%s, oldHP=%d -> curHP=%d\n", GetID(), GetName().c_str(), oldHP, m_iHP);

	Packet result(AG_USER_SET_HP);
	result << GetID() << m_iHP;
	g_pMain->Send(&result);

	return true;
}

//	NPC 사망처리시 경험치 분배를 계산한다.(일반 유저와 버디 사용자구분)
void CNpc::SendExpToUserList()
{
	int i=0;
	int nExp = 0;
	int nPartyExp = 0;
	int nLoyalty = 0;
	int nPartyLoyalty = 0;
	double totalDamage = 0;
	double CompDamage = 0;
	double TempValue = 0;
	int nPartyNumber = -1;
	int nUid = -1;
	CUser* pUser = nullptr;
	CUser* pPartyUser = nullptr;
	CUser* pMaxDamageUser = nullptr;
	_PARTY_GROUP* pParty = nullptr;
	char strMaxDamageUser[MAX_ID_SIZE+1];
	MAP* pMap = GetMap();
	if (pMap == nullptr) return;

	IsUserInSight();	// 시야권내에 있는 유저 셋팅..
				
	for(i = 0; i < NPC_HAVE_USER_LIST; i++)				// 일단 리스트를 검색한다.
	{
		if(m_DamagedUserList[i].iUid < 0 || m_DamagedUserList[i].nDamage<= 0) continue;
		if(m_DamagedUserList[i].bIs == true) pUser = g_pMain->GetUserPtr(m_DamagedUserList[i].iUid);
		if(pUser == nullptr) continue;
		
		if(pUser->m_byNowParty == 1)			// 파티 소속
		{	
			totalDamage = GetPartyDamage(pUser->m_sPartyNumber);
			if(totalDamage == 0 || m_TotalDamage == 0)
				nPartyExp = 0;
			else	{
				if( CompDamage < totalDamage )	{	// 
					CompDamage = totalDamage;
					m_sMaxDamageUserid = m_DamagedUserList[i].iUid;
					pMaxDamageUser = g_pMain->GetUserPtr(m_DamagedUserList[i].iUid);
					if(pMaxDamageUser == nullptr)	{
						m_byMaxDamagedNation = pUser->m_bNation;
						strncpy( strMaxDamageUser, pUser->GetName().c_str(), sizeof(strMaxDamageUser) );
					}
					else	{
						m_byMaxDamagedNation = pMaxDamageUser->m_bNation;
						strncpy( strMaxDamageUser, pMaxDamageUser->GetName().c_str(), sizeof(strMaxDamageUser) );
					}
				}

				TempValue = m_proto->m_iExp * (totalDamage / m_TotalDamage);
				nPartyExp = (int)TempValue;
				if(TempValue > nPartyExp)	nPartyExp = nPartyExp + 1;
			}
			if(m_proto->m_iLoyalty == 0 || totalDamage == 0 || m_TotalDamage == 0)
				nPartyLoyalty = 0;
			else	{
				TempValue = m_proto->m_iLoyalty * (totalDamage / m_TotalDamage);
				nPartyLoyalty = (int)TempValue;
				if(TempValue > nPartyLoyalty)	nPartyLoyalty = nPartyLoyalty + 1;
			}
			// 파티원 전체를 돌면서 경험치 분배
			if(i != 0)
			{
				bool bFlag = false;
				int count = 0;
				for(int j=0; j<i; j++)
				{
					if(m_DamagedUserList[j].iUid < 0 || m_DamagedUserList[j].nDamage<= 0) continue;
					if(m_DamagedUserList[j].bIs == true) pPartyUser = g_pMain->GetUserPtr(m_DamagedUserList[j].iUid);
					if(pPartyUser == nullptr) continue;
					if(pUser->m_sPartyNumber == pPartyUser->m_sPartyNumber)	continue;
					count++;
				}

				if(count == i)	bFlag = true;

				// 여기에서 또 작업...
				if(bFlag == true)	{
					int uid = 0;
					pParty = g_pMain->m_arParty.GetData( pUser->m_sPartyNumber );
					if( pParty ) {	
						int nTotalMan = 0;
						int nTotalLevel = 0;
						for(int j=0; j<8; j++)	{
							uid = pParty->uid[j];
							pPartyUser = g_pMain->GetUserPtr(uid);
							if(pPartyUser)	{
								nTotalMan++;
								nTotalLevel += pPartyUser->m_bLevel;
							}
						}

						nPartyExp = GetPartyExp( nTotalLevel, nTotalMan, nPartyExp );
						//TRACE("* PartyUser GetPartyExp total_level=%d, total_man = %d, exp=%d *\n", nTotalLevel, nTotalMan, nPartyExp);

						for(int k=0; k<8; k++)	{
							uid = pParty->uid[k];
							pPartyUser = g_pMain->GetUserPtr(uid);
							if(pPartyUser)
							{
								// monster와 거리를 판단
								if( IsInExpRange(pPartyUser) == true)
								{
									TempValue = ( nPartyExp * ( 1+0.3*( nTotalMan-1 ) ) ) * (double)pPartyUser->m_bLevel / (double)nTotalLevel;
									//TempValue = ( nPartyExp * ( 1+0.3*( nTotalMan-1 ) ) );
									nExp = (int)TempValue;
									if(TempValue > nExp)	nExp = nExp + 1;
									if(nPartyLoyalty <= 0)
										nLoyalty = 0;
									else	{
										TempValue = ( nPartyLoyalty * ( 1+0.2*( nTotalMan-1 ) ) ) * (double)pPartyUser->m_bLevel / (double)nTotalLevel;
										nLoyalty = (int)TempValue;
										if(TempValue > nLoyalty)	nLoyalty = nLoyalty + 1;
									}
									pPartyUser->SetPartyExp(nExp, nLoyalty, nTotalLevel, nTotalMan);
								}
							}
						}	
					}
				}
			}
			else if(i==0)
			{
				int uid = 0;
				pParty = g_pMain->m_arParty.GetData( pUser->m_sPartyNumber );
				if( pParty ) {	
					int nTotalMan = 0;
					int nTotalLevel = 0;
					for(int j=0; j<8; j++)	{
						uid = pParty->uid[j];
						pPartyUser = g_pMain->GetUserPtr(uid);
						if(pPartyUser)	{
							nTotalMan++;
							nTotalLevel += pPartyUser->m_bLevel;
						}
					}

					nPartyExp = GetPartyExp( nTotalLevel, nTotalMan, nPartyExp );
					//TRACE("* PartyUser GetPartyExp total_level=%d, total_man = %d, exp=%d *\n", nTotalLevel, nTotalMan, nPartyExp);

					for(int k=0; k<8; k++)	{
						uid = pParty->uid[k];
						pPartyUser = g_pMain->GetUserPtr(uid);
						if(pPartyUser)
						{
							// monster와 거리를 판단
							if( IsInExpRange(pPartyUser) == true)
							{
								TempValue = ( nPartyExp * ( 1+0.3*( nTotalMan-1 ) ) ) * (double)pPartyUser->m_bLevel / (double)nTotalLevel;
								//TempValue = ( nPartyExp * ( 1+0.3*( nTotalMan-1 ) ) );
								nExp = (int)TempValue;
								if(TempValue > nExp)	nExp = nExp + 1;
								if(nPartyLoyalty <= 0)
									nLoyalty = 0;
								else	{
									TempValue = ( nPartyLoyalty * ( 1+0.2*( nTotalMan-1 ) ) ) * (double)pPartyUser->m_bLevel / (double)nTotalLevel;
									nLoyalty = (int)TempValue;
									if(TempValue > nLoyalty)	nLoyalty = nLoyalty + 1;
								}
								pPartyUser->SetPartyExp(nExp, nLoyalty, nTotalLevel, nTotalMan);
							}
						}
					}	
				}
			}
			//nExp = 
		}	
		else if(pUser->m_byNowParty == 2)		// 부대 소속
		{	
			
		}	
		else									// 개인
		{
			totalDamage = m_DamagedUserList[i].nDamage;
			
			if(totalDamage == 0 || m_TotalDamage == 0)	{
				nExp = 0;
				nLoyalty = 0;
			}
			else	{

				if( CompDamage < totalDamage )	{	// 
					CompDamage = totalDamage;
					m_sMaxDamageUserid = m_DamagedUserList[i].iUid;
					pMaxDamageUser = g_pMain->GetUserPtr(m_DamagedUserList[i].iUid);
					if(pMaxDamageUser == nullptr)	{
						m_byMaxDamagedNation = pUser->m_bNation;
						strncpy( strMaxDamageUser, pUser->GetName().c_str(), sizeof(strMaxDamageUser) );
					}
					else	{
						m_byMaxDamagedNation = pMaxDamageUser->m_bNation;
						strncpy( strMaxDamageUser, pMaxDamageUser->GetName().c_str(), sizeof(strMaxDamageUser) );
					}
				}

				TempValue = m_proto->m_iExp * ( totalDamage / m_TotalDamage );
				nExp = (int)TempValue;
				if(TempValue > nExp)	nExp = nExp + 1;

				if(m_proto->m_iLoyalty == 0) nLoyalty = 0;
				else	{
					TempValue = m_proto->m_iLoyalty * ( totalDamage / m_TotalDamage );
					nLoyalty = (int)TempValue;
					if(TempValue > nLoyalty)	nLoyalty = nLoyalty + 1;
				}

				//TRACE("* User Exp id=%s, damage=%d, total=%d, exp=%d, loral=%d *\n", pUser->GetName().c_str(), (int)totalDamage, m_TotalDamage, nExp, nLoyalty);
				pUser->SetExp(nExp, nLoyalty, GetLevel());
			}
		}
	}

	if( g_pMain->m_byBattleEvent == BATTLEZONE_OPEN )	{	// 전쟁중
		if( m_bySpecialType >= 90 && m_bySpecialType <= 100 )	{					// 죽었을때 데미지를 많이 입힌 유저를 기록해 주세여
			if( strlen( strMaxDamageUser) != 0 )	{		// 몬스터에게 가장 데미지를 많이 입힌 유저의 이름을 전송
				Packet result(AG_BATTLE_EVENT, uint8(BATTLE_EVENT_MAX_USER));

				switch (m_bySpecialType)
				{
				case 100:	result << uint8(1); break;
				case 90:	result << uint8(3); break;
				case 91:	result << uint8(4); break;
				case 92:	result << uint8(5);	break;
				case 93:	result << uint8(6); break;
				case 98:	result << uint8(7); break;
				case 99:	result << uint8(8); break;
				}

				if (m_bySpecialType == 90 || m_bySpecialType == 91 || m_bySpecialType == 98)
					g_pMain->m_sKillKarusNpc++;
				else if (m_bySpecialType == 92 || m_bySpecialType == 93 || m_bySpecialType == 99)
					g_pMain->m_sKillElmoNpc++;

				result.SByte();
				result << strMaxDamageUser;
				g_pMain->Send(&result);

				bool	bKarusComplete = (g_pMain->m_sKillKarusNpc == pMap->m_sKarusRoom),
						bElMoradComplete = (g_pMain->m_sKillElmoNpc == pMap->m_sElmoradRoom);
				if (bKarusComplete || bElMoradComplete)
				{
					result.clear();
					result	<< uint8(BATTLE_EVENT_RESULT) 
							<< uint8(bKarusComplete ? KARUS : ELMORAD)
							<< strMaxDamageUser;
					g_pMain->Send(&result);
				}
			}
		}
	}
}

bool CNpc::IsCloseTarget(CUser *pUser, int nRange)
{
	if (pUser == nullptr
		|| pUser->isDead()
		|| !isInRangeSlow(pUser, nRange * 2.0f))
		return false; 

	m_Target.id = pUser->m_iUserId;
	m_Target.bSet = true;
	m_Target.x = pUser->GetX();
	m_Target.y = pUser->GetY();
	m_Target.z = pUser->GetZ();

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// 시야 범위내의 내동료를 찾는다.
// type = 0: 같은 그룹이면서 같은 패밀리 타입만 도움, 1:그룹이나 패밀리에 관계없이 도움, 
//        2:사제NPC가 같은 아군의 상태를 체크해서 치료할 목적으로,, 리턴으로 치료될 아군의 NID를 리턴한다
int CNpc::FindFriend(int type)
{
	CNpc* pNpc = nullptr;
	MAP* pMap = GetMap();
	if (pMap == nullptr) return 0;
	if (pMap == nullptr
		|| m_bySearchRange == 0
		|| (type != 2 && hasTarget()))
		return 0;

	int min_x = (int)(GetX() - m_bySearchRange)/VIEW_DIST;	if(min_x < 0) min_x = 0;
	int min_z = (int)(GetZ() - m_bySearchRange)/VIEW_DIST;	if(min_z < 0) min_z = 0;
	int max_x = (int)(GetX() + m_bySearchRange)/VIEW_DIST;	if(max_x > pMap->GetXRegionMax()) max_x = pMap->GetXRegionMax();
	int max_z = (int)(GetZ() + m_bySearchRange)/VIEW_DIST;	if(min_z > pMap->GetZRegionMax()) min_z = pMap->GetZRegionMax();

	int search_x = max_x - min_x + 1;		
	int search_z = max_z - min_z + 1;	
	
	int i, j, count = 0;
	_TargetHealer arHealer[9];
	for(i=0; i<9; i++)	{
		arHealer[i].sNID = -1;
		arHealer[i].sValue = 0;
	}

	for(i = 0; i < search_x; i++)	{
		for(j = 0; j < search_z; j++)	{
			FindFriendRegion(min_x+i, min_z+j, pMap, &arHealer[count], type);
			//FindFriendRegion(min_x+i, min_z+j, pMap, type);
		}
	}

	int iValue = 0, iMonsterNid = 0;
	for(i=0; i<9; i++)	{
		if(iValue < arHealer[i].sValue)	{
			iValue = arHealer[i].sValue;
			iMonsterNid = arHealer[i].sNID;
		}
	}

	if (iMonsterNid != 0)	
	{
		m_Target.id = iMonsterNid;
		m_Target.bSet = true;
		return iMonsterNid;
	}

	return 0;
}

void CNpc::FindFriendRegion(int x, int z, MAP* pMap, _TargetHealer* pHealer, int type)
{
	if (x < 0 || z < 0 || x > pMap->GetXRegionMax() || z > pMap->GetZRegionMax())
	{
		TRACE("#### Npc-FindFriendRegion() Fail : [nid=%d, sid=%d], nRX=%d, nRZ=%d #####\n", GetID(), m_proto->m_sSid, x, z);
		return;
	}

	FastGuard lock(pMap->m_lock);
	CRegion *pRegion = &pMap->m_ppRegion[x][z];
	__Vector3 vStart, vEnd;
	float fDis = 0.0f, 
		fSearchRange = (type == 2 ? (float)m_byAttackRange : (float)m_byTracingRange);
	int iValue = 0;

	vStart.Set(GetX(), GetY(), GetZ());

	foreach_stlmap (itr, pRegion->m_RegionNpcArray)
	{
		CNpc * pNpc = g_pMain->m_arNpc.GetData(itr->first);

		if (pNpc != nullptr && pNpc->m_NpcState != NPC_DEAD && pNpc->GetID() != GetID())
		{
			vEnd.Set(pNpc->GetX(), pNpc->GetY(), pNpc->GetZ()); 
			fDis = GetDistance(vStart, vEnd);

			if(fDis <= fSearchRange)	{
				if(type == 1)	{
					if(GetID() != pNpc->GetID())	{
						if (pNpc->hasTarget() && pNpc->m_NpcState == NPC_FIGHTING) continue;
						pNpc->m_Target.id = m_Target.id;
						pNpc->m_Target.bSet = true;
						pNpc->m_Target.x = m_Target.x;
						pNpc->m_Target.y = m_Target.y;
						pNpc->m_Target.z = m_Target.z;
						pNpc->m_Target.failCount = 0;
						pNpc->NpcStrategy(NPC_ATTACK_SHOUT);
					}
				}
				else if(type == 0)	{
					if(pNpc->m_tNpcGroupType && GetID() != pNpc->GetID() && pNpc->m_proto->m_byFamilyType == m_proto->m_byFamilyType)	{
						if (pNpc->hasTarget() && pNpc->m_NpcState == NPC_FIGHTING) continue;
						pNpc->m_Target.id = m_Target.id;
						pNpc->m_Target.bSet = true;
						pNpc->m_Target.x = m_Target.x;
						pNpc->m_Target.y = m_Target.y;
						pNpc->m_Target.z = m_Target.z;
						pNpc->m_Target.failCount = 0;
						pNpc->NpcStrategy(NPC_ATTACK_SHOUT);
					}
				}
				else if(type == 2)	{
					if(pHealer == nullptr) continue;

					int iHP = (int)(pNpc->m_iMaxHP * 0.9);
					if(pNpc->m_iHP <= iHP)	{		// HP 체크
						int iCompValue = (int)((pNpc->m_iMaxHP - pNpc->m_iHP) / (pNpc->m_iMaxHP * 0.01));
						if (iValue < iCompValue)		{
							iValue = iCompValue;
							pHealer->sNID = pNpc->GetID();
							pHealer->sValue = iValue;
						}
					}
				}
			}	
			else continue;
		}
	}
}

void CNpc::NpcStrategy(uint8 type)
{
	switch(type)
	{
	case NPC_ATTACK_SHOUT:
		m_NpcState = NPC_TRACING;
		m_Delay = m_sSpeed;//STEP_DELAY;
		m_fDelayTime = getMSTime();
		break;
	}
}

void CNpc::FillNpcInfo(Packet & result)
{
	if (m_bySpecialType == 5 && m_byChangeType == 0)	
		result << uint8(0);
	else
		result << uint8(1);

	result	<< GetID() << m_proto->m_sSid << m_proto->m_sPid
			<< m_sSize << m_iWeapon_1 << m_iWeapon_2
			<< GetZoneID() << GetName()
			<< GetNation() << GetLevel()
			<< GetX() << GetZ() << GetY() << m_byDirection
			<< bool(m_iHP > 0) // are we alive?
			<< m_proto->m_tNpcType
			<< m_iSellingGroup << m_iMaxHP << m_iHP
			<< m_byGateOpen 
			<< float(m_sHitRate) << float(m_sEvadeRate) << m_sDefense
			<< m_byObjectType << m_byTrapNumber 
			<< m_bMonster;
}

int CNpc::GetDir(float x1, float z1, float x2, float z2)
{
	int nDir;					//	3 4 5
								//	2 8 6
								//	1 0 7

	int nDirCount = 0;

	int x11 = (int)x1 / TILE_SIZE;
	int y11 = (int)z1 / TILE_SIZE; 
	int x22 = (int)x2 / TILE_SIZE;
	int y22 = (int)z2 / TILE_SIZE; 

	int deltax = x22 - x11;
	int deltay = y22 - y11;

	int fx = ((int)x1/TILE_SIZE) * TILE_SIZE;
	int fy = ((int)z1/TILE_SIZE) * TILE_SIZE;

	float add_x = x1 - fx;
	float add_y = z1 - fy;

	if (deltay==0) {
		if (x22>x11) nDir = DIR_RIGHT;		
		else nDir = DIR_LEFT;	
		goto result_value;
	}
	if (deltax==0) 
	{
		if (y22>y11) nDir = DIR_DOWN;	
		else nDir = DIR_UP;		
		goto result_value;
	}
	else 
	{
		if (y22>y11) 
		{
			if (x22>x11) 
			{
				nDir = DIR_DOWNRIGHT;		// -> 
			}
			else 
			{
				nDir = DIR_DOWNLEFT;		// -> 
			}
		}
		else
		{
			if (x22 > x11) 
			{
				nDir = DIR_UPRIGHT;		
			}
			else 
			{
				nDir = DIR_UPLEFT;
			}
		}
	}

result_value:

	switch(nDir)
	{
	case DIR_DOWN:
		m_fAdd_x = add_x;
		m_fAdd_z = 3;
		break;
	case DIR_DOWNLEFT:
		m_fAdd_x = 1;
		m_fAdd_z = 3;
		break;
	case DIR_LEFT:
		m_fAdd_x = 1;
		m_fAdd_z = add_y;
		break;
	case DIR_UPLEFT:
		m_fAdd_x = 1;
		m_fAdd_z = 1;
		break;
	case DIR_UP:
		m_fAdd_x = add_x;
		m_fAdd_z = 1;
		break;
	case DIR_UPRIGHT:
		m_fAdd_x = 3;
		m_fAdd_z = 1;
		break;
	case DIR_RIGHT:
		m_fAdd_x = 3;
		m_fAdd_z = add_y;
		break;
	case DIR_DOWNRIGHT:
		m_fAdd_x = 3;
		m_fAdd_z = 3;
		break;
	}

	return nDir;
}

float CNpc::RandomGenf(float max, float min)
{
	if ( max == min ) return max;
	if ( min > max ) { float b = min; min = max; max = b; }
	int k = rand()%(int)((max*100-min*100));	

	return (float)((float)(k*0.01f)+min);
}

void CNpc::NpcMoveEnd()
{
	RegisterRegion(GetX(), GetZ());

	Packet result(MOVE_RESULT, uint8(SUCCESS));
	result	<< GetID()
			<< GetX() << GetZ() << GetY() << float(0.0f);

	g_pMain->Send(&result);
}

void CNpc::GetVectorPosition(__Vector3 & vOrig, __Vector3 & vDest, float fDis, __Vector3 * vResult)
{
	*vResult = vDest - vOrig;
	vResult->Magnitude();
	vResult->Normalize();
	*vResult *= fDis;
	*vResult += vOrig;
}

float CNpc::GetDistance(__Vector3 & vOrig, __Vector3 & vDest)
{
	return (vOrig - vDest).Magnitude();
}

bool CNpc::GetUserInView()
{
	MAP* pMap = GetMap();
	if (pMap == nullptr)	return false;
	//if( m_ZoneIndex > 5 || m_ZoneIndex < 0) return false;		// 임시코드 ( 2002.03.24 )
	int min_x = (int)(GetX() - NPC_VIEW_RANGE)/VIEW_DIST;	if(min_x < 0) min_x = 0;
	int min_z = (int)(GetZ() - NPC_VIEW_RANGE)/VIEW_DIST;	if(min_z < 0) min_z = 0;
	int max_x = (int)(GetX() + NPC_VIEW_RANGE)/VIEW_DIST;	if(max_x > pMap->GetXRegionMax()) max_x = pMap->GetXRegionMax();
	int max_z = (int)(GetZ() + NPC_VIEW_RANGE)/VIEW_DIST;	if(max_z > pMap->GetZRegionMax()) max_z = pMap->GetZRegionMax();

	int search_x = max_x - min_x + 1;		
	int search_z = max_z - min_z + 1;	
	
	bool bFlag = false;
	int i, j;

	for(i = 0; i < search_x; i++)	{
		for(j = 0; j < search_z; j++)	{
			bFlag = GetUserInViewRange(min_x+i, min_z+j);
			if(bFlag == true)	return true;
		}
	}

	return false;
}

bool CNpc::GetUserInViewRange(int x, int z)
{
	MAP* pMap = GetMap();
	if (pMap == nullptr || x < 0 || z < 0 || x > pMap->GetXRegionMax() || z > pMap->GetZRegionMax())
	{
		TRACE("#### Npc-GetUserInViewRange() Fail : [nid=%d, sid=%d], x1=%d, z1=%d #####\n", GetID(), m_proto->m_sSid, x, z);
		return false;
	}

	FastGuard lock(pMap->m_lock);
	CRegion * pRegion = &pMap->m_ppRegion[x][z];
	float fDis = 0.0f; 

	foreach_stlmap (itr, pRegion->m_RegionUserArray)
	{
		CUser *pUser = g_pMain->GetUserPtr(*itr->second);
		if (pUser == nullptr)
			continue;

		if (isInRange(pUser, NPC_VIEW_RANGE))
			return true;
	}
	
	return false;
}

void CNpc::SendAttackSuccess(uint8 byResult, int tuid, short sDamage, int nHP, uint8 byFlag, short sAttack_type)
{
	uint16 sid, tid;
	uint8 type;

	if (byFlag == 0)
	{
		type = 2;
		sid = GetID();
		tid = tuid;
	}
	else	
	{
		type = 1;
		sid = tuid;
		tid = GetID();
	}

	Packet result(AG_ATTACK_RESULT, type);
	result << byResult << sid << tid << sDamage << nHP << uint8(sAttack_type);
	g_pMain->Send(&result);
}

void CNpc::CalcAdaptivePosition(__Vector3 & vPosOrig, __Vector3 & vPosDest, float fAttackDistance, __Vector3 * vResult)
{
	*vResult = vPosOrig - vPosDest;	
	vResult->Normalize();	
	*vResult *= fAttackDistance;
	*vResult += vPosDest;
}

void CNpc::IsUserInSight()
{
	for (int j = 0; j < NPC_HAVE_USER_LIST; j++)
		m_DamagedUserList[j].bIs = false;

	for (int i = 0; i < NPC_HAVE_USER_LIST; i++)
	{
		CUser * pUser = g_pMain->GetUserPtr(m_DamagedUserList[i].iUid);
		if (pUser == nullptr
			|| !isInRangeSlow(pUser, NPC_EXP_RANGE))
			continue;

		if (m_DamagedUserList[i].iUid == pUser->m_iUserId)
		{
			if (STRCASECMP(m_DamagedUserList[i].strUserID, pUser->GetName().c_str()) == 0) 
				m_DamagedUserList[i].bIs = true;
		}
	}
}

bool CNpc::IsLevelCheck(int iLevel)
{
	if (iLevel <= GetLevel())
		return false;

	return (iLevel - GetLevel() >= 8);
}

bool CNpc::IsHPCheck(int iHP)
{
	return (m_iHP < (m_iMaxHP*0.2));
}

// 패스 파인드를 할것인지를 체크하는 루틴..
bool CNpc::IsPathFindCheck(float fDistance)
{
	int nX = 0, nZ = 0;
	__Vector3 vStart, vEnd, vDis, vOldDis;
	float fDis = 0.0f;
	vStart.Set(m_fStartPoint_X, 0, m_fStartPoint_Y);
	vEnd.Set(m_fEndPoint_X, 0, m_fEndPoint_Y);
	vDis.Set(m_fStartPoint_X, 0, m_fStartPoint_Y);
	int count = 0;
	int nError = 0;

	MAP* pMap = GetMap();

	nX = (int)(vStart.x / TILE_SIZE);
	nZ = (int)(vStart.z / TILE_SIZE);
	if(pMap->IsMovable(nX, nZ) == true)
	{
		nError = -1;
		return false;
	}
	nX = (int)(vEnd.x / TILE_SIZE);
	nZ = (int)(vEnd.z / TILE_SIZE);
	if(pMap->IsMovable(nX, nZ) == true)
	{
		nError = -1;
		return false;
	}

	do
	{
		vOldDis.Set(vDis.x, 0, vDis.z);
		GetVectorPosition(vDis, vEnd, fDistance, &vDis);
		fDis = GetDistance(vOldDis, vEnd);

		if(fDis > NPC_MAX_MOVE_RANGE)
		{
			nError = -1;
			break;
		}
		
		nX = (int)(vDis.x / TILE_SIZE);
		nZ = (int)(vDis.z / TILE_SIZE);

		if(pMap->IsMovable(nX, nZ) == true
			|| count >= MAX_PATH_LINE)
		{
			nError = -1;
			break;
		}

		m_pPoint[count].fXPos = vEnd.x;
		m_pPoint[count++].fZPos = vEnd.z;

	} while (fDis <= fDistance);

	m_iAniFrameIndex = count;

	if(nError == -1)
		return false;

	return true;
}

// 패스 파인드를 하지 않고 공격대상으로 가는 루틴..
void CNpc::IsNoPathFind(float fDistance)
{
	ClearPathFindData();
	m_bPathFlag = true;

	int nX = 0, nZ = 0;
	__Vector3 vStart, vEnd, vDis, vOldDis;
	float fDis = 0.0f;
	vStart.Set(m_fStartPoint_X, 0, m_fStartPoint_Y);
	vEnd.Set(m_fEndPoint_X, 0, m_fEndPoint_Y);
	vDis.Set(m_fStartPoint_X, 0, m_fStartPoint_Y);
	int count = 0;
	int nError = 0;

	fDis = GetDistance(vStart, vEnd);	
	if(fDis > NPC_MAX_MOVE_RANGE)	{						// 100미터 보다 넓으면 스탠딩상태로..
		ClearPathFindData();
		TRACE("#### Npc-IsNoPathFind Fail : NPC_MAX_MOVE_RANGE overflow  .. [nid = %d, name=%s], cur_x=%.2f, z=%.2f, dest_x=%.2f, dest_z=%.2f, fDis=%.2f#####\n", 
			GetID(), GetName().c_str(), m_fStartPoint_X, m_fStartPoint_Y, m_fEndPoint_X, m_fEndPoint_Y, fDis);
		return;
	}

	if (GetMap() == nullptr)
	{
		ClearPathFindData();
		TRACE("#### Npc-IsNoPathFind No map : [nid=%d, name=%s], zone=%d #####\n", GetID(), GetName().c_str(), GetZoneID());
		return;
	}
	MAP* pMap = GetMap();

	do
	{
		vOldDis.Set(vDis.x, 0, vDis.z);
		GetVectorPosition(vDis, vEnd, fDistance, &vDis);
		fDis = GetDistance(vOldDis, vEnd);
		
		nX = (int)(vDis.x / TILE_SIZE);
		nZ = (int)(vDis.z / TILE_SIZE);
		if(count < 0 || count >= MAX_PATH_LINE)	{	
			ClearPathFindData();
			TRACE("#### Npc-IsNoPathFind index overflow Fail 1 :  count=%d ####\n", count);
			return;
		}	

		m_pPoint[count].fXPos = vEnd.x;
		m_pPoint[count++].fZPos = vEnd.z;
	} while (fDis <= fDistance);

	if(count <= 0 || count >= MAX_PATH_LINE)	{	
		ClearPathFindData();
		TRACE("#### IsNoPtahfind Fail : nid=%d,%s, count=%d ####\n", GetID(), GetName().c_str(), count);
		return;
	}
	m_iAniFrameIndex = count;

}

//	NPC 가 가진 아이템을 떨군다.
void CNpc::GiveNpcHaveItem()
{
	int temp = 0, iPer = 0, iMakeItemCode = 0, iMoney = 0, iRandom, nCount = 1, i =0;

/*	if( m_byMoneyType == 1 )	{
		SetByte(pBuf, AG_NPC_EVENT_ITEM, index);
		SetShort(pBuf, m_sMaxDamageUserid, index);	
		SetShort(pBuf, GetID(), index);
		SetDWORD(pBuf, TYPE_MONEY_SID, index);
		SetDWORD(pBuf, m_iMoney, index);
		return;
	}	*/

	iRandom = myrand(70, 100);
	iMoney = m_iMoney * iRandom / 100;
	//m_iMoney, m_iItem;
	_NpcGiveItem m_GiveItemList[NPC_HAVE_ITEM_LIST];			// Npc의 ItemList
	if( iMoney <= 0 )	{
		nCount = 0;
	}
	else	{
		m_GiveItemList[0].sSid = TYPE_MONEY_SID;
		if( iMoney >= SHRT_MAX ) {
			iMoney = 32000;	
			m_GiveItemList[0].count = iMoney;
		}
		else	m_GiveItemList[0].count = iMoney;
	}
	
	_K_MONSTER_ITEM * pItem = g_pMain->m_NpcItemArray.GetData(m_iItem);
	if (pItem != nullptr)
	{
		// j = iItem
		for (int j = 0; j < 5; j++)
		{
			if (pItem->iItem[j] == 0
				|| pItem->sPercent[j] == 0)
				continue;

			iRandom = myrand(1, 10000);
			iPer = pItem->sPercent[j];
			if (iRandom > iPer)
				continue;

			if (j < 2)
			{
				if (pItem->iItem[j] < 100)
				{
					iMakeItemCode = ItemProdution(pItem->iItem[j]);
				}
				else 
				{
					_MAKE_ITEM_GROUP * pGroup = g_pMain->m_MakeItemGroupArray.GetData(pItem->iItem[j]);
					if (pGroup == nullptr
						|| pGroup->iItems.size() != 30)
						continue;

					iMakeItemCode = pGroup->iItems[myrand(1, 30) - 1];
				}

				if (iMakeItemCode == 0) 
					continue;

				m_GiveItemList[nCount].sSid = iMakeItemCode;
				m_GiveItemList[nCount].count = 1;
			}
			else	
			{
				m_GiveItemList[nCount].sSid = pItem->iItem[j];
				if (COMPARE(m_GiveItemList[nCount].sSid, ARROW_MIN, ARROW_MAX))
					m_GiveItemList[nCount].count = 20;
				else	
					m_GiveItemList[nCount].count = 1;
			}
			nCount++;
		}
	}

	if( m_sMaxDamageUserid < 0 || m_sMaxDamageUserid > MAX_USER )	{
		return;
	}

	Packet result(AG_NPC_GIVE_ITEM);
	result	<< m_sMaxDamageUserid << GetID()
			<< GetZoneID() << GetRegionX() << GetRegionZ()
			<< GetX() << GetZ() << GetY()
			<< uint8(nCount);

	for (i = 0; i < nCount; i++)
		result << m_GiveItemList[i].sSid << m_GiveItemList[i].count;

	g_pMain->Send(&result);
}


void CNpc::Yaw2D(float fDirX, float fDirZ, float& fYawResult)
{
	if ( fDirX >= 0.0f ) 
	{ 
		if ( fDirZ >= 0.0f ) 
			fYawResult = (float)(asin(fDirX)); 
		else 
			fYawResult = D3DXToRadian(90.0f) + (float)(acos(fDirX)); 
	}
	else 
	{ 
		if ( fDirZ >= 0.0f ) 
			fYawResult = D3DXToRadian(270.0f) + (float)(acos(-fDirX)); 
		else 
			fYawResult = D3DXToRadian(180.0f) + (float)(asin(-fDirX)); 
	}
}
  
void CNpc::ComputeDestPos(__Vector3 & vCur, float fDegree, float fDegreeOffset, float fDistance, __Vector3 * vResult)
{
	__Matrix44 mtxRot; 
	vResult->Zero();
	mtxRot.RotationY(D3DXToRadian(fDegree+fDegreeOffset));
	*vResult *= mtxRot;
	*vResult *= fDistance;
	*vResult += vCur;
}

int	CNpc::GetPartyDamage(int iNumber)
{
	int i=0;
	int nDamage = 0;
	CUser* pUser = nullptr;
	for(i = 0; i < NPC_HAVE_USER_LIST; i++)				// 일단 리스트를 검색한다.
	{
		if(m_DamagedUserList[i].iUid < 0 || m_DamagedUserList[i].nDamage<= 0) continue;
		if(m_DamagedUserList[i].bIs == true) pUser = g_pMain->GetUserPtr(m_DamagedUserList[i].iUid);
		if(pUser == nullptr) continue;
		
		if(pUser->m_sPartyNumber != iNumber)	continue;

		nDamage += m_DamagedUserList[i].nDamage;
	}

	return nDamage;
}

void CNpc::NpcTypeParser()
{
	// 선공인지 후공인지를 결정한다
	switch(m_byActType)
	{
	case 1:
		m_tNpcAttType = m_tNpcOldAttType = 0;
		break;
	case 2:
		m_tNpcAttType = m_tNpcOldAttType = 0;
		m_byNpcEndAttType = 0;			
		break;
	case 3:
		m_tNpcGroupType = 1;
		m_tNpcAttType = m_tNpcOldAttType = 0;
		break;
	case 4:
		m_tNpcGroupType = 1;
		m_tNpcAttType = m_tNpcOldAttType = 0;
		m_byNpcEndAttType = 0;			
		break;
	case 6:
		m_byNpcEndAttType = 0;			
		break;
	case 5:
	case 7:
		m_tNpcAttType = m_tNpcOldAttType = 1;
		break;
	default :
		m_tNpcAttType = m_tNpcOldAttType = 1;
	}
}

void CNpc::HpChange()
{
	m_fHPChangeTime = getMSTime();

	//if(m_NpcState == NPC_FIGHTING || m_NpcState == NPC_DEAD)	return;
	if(m_NpcState == NPC_DEAD)	return;
	if( m_iHP < 1 )	return;	// 죽기직전일때는 회복 안됨...
	if( m_iHP == m_iMaxHP)  return;	// HP가 만빵이기 때문에.. 
	
	//int amount =  (int)(m_sLevel*(1+m_sLevel/60.0) + 1) ;
	int amount =  (int)(m_iMaxHP / 20) ;

	m_iHP += amount;
	if( m_iHP < 0 )
		m_iHP = 0;
	else if ( m_iHP > m_iMaxHP )
		m_iHP = m_iMaxHP;

	Packet result(AG_USER_SET_HP);
	result << GetID() << m_iHP << m_iMaxHP;
	g_pMain->Send(&result);
}

bool CNpc::IsInExpRange(CUser* pUser)
{
	if (GetZoneID() != pUser->GetZoneID())
		return false;

	return isInRangeSlow(pUser->GetX(), pUser->GetZ(), NPC_EXP_RANGE);
}

bool CNpc::CheckFindEnemy()
{
	if (isGuard())
		return true;

	MAP* pMap = GetMap();

	if (pMap == nullptr
		|| GetRegionX() > pMap->GetXRegionMax() 
		|| GetRegionZ() > pMap->GetZRegionMax())
	{
		TRACE("#### CheckFindEnemy Fail : [nid=%d, sid=%d], nRX=%d, nRZ=%d #####\n", GetID(), m_proto->m_sSid, GetRegionX(), GetRegionZ());
		return false;
	}

	FastGuard lock(pMap->m_lock);
	if (pMap->m_ppRegion[GetRegionX()][GetRegionZ()].m_byMoving == 1)
		return true;

	return false;
}

void CNpc::MSpChange(int type, int amount)
{
	if( type == 2 ) {
		m_sMP += amount;
		if( m_sMP < 0 )
			m_sMP = 0;
		else if ( m_sMP > m_sMaxMP )
			m_sMP = m_sMaxMP;
	}
	else if( type == 3 ) {	// monster는 SP가 없음..
	}
}

int	CNpc::ItemProdution(int item_number)							// 아이템 제작
{
	int iItemNumber = 0, iRandom = 0, i=0, iItemGrade = 0, iItemLevel = 0;
	int iDefault = 0, iItemCode=0, iItemKey=0, iRand2=0, iRand3=0, iRand4=0, iRand5=0;
	int iTemp1 = 0, iTemp2 = 0, iTemp3 = 0;

	iRandom = myrand(1, 10000);

	iItemGrade = GetItemGrade(item_number);
	if(iItemGrade == 0)		return 0;
	iItemLevel = GetLevel() / 5;

	if( COMPARE( iRandom, 1, 4001) )	{			// 무기구 아이템
		iDefault = 100000000;
		iRandom = myrand( 1, 10000 );				// 무기의 종류를 결정(단검, 검, 도끼,,,,)
		if( COMPARE ( iRandom, 1, 701 ) )			iRand2 = 10000000;
		else if( COMPARE ( iRandom, 701, 1401 ) )	iRand2 = 20000000;
		else if( COMPARE ( iRandom, 1401, 2101 ) )	iRand2 = 30000000;
		else if( COMPARE ( iRandom, 2101, 2801 ) )	iRand2 = 40000000;
		else if( COMPARE ( iRandom, 2801, 3501 ) )	iRand2 = 50000000;
		else if( COMPARE ( iRandom, 3501, 5501 ) )	iRand2 = 60000000;
		else if( COMPARE ( iRandom, 5501, 6501 ) )	iRand2 = 70000000;
		else if( COMPARE ( iRandom, 6501, 8501 ) )	iRand2 = 80000000;
		else if( COMPARE ( iRandom, 8501, 10001 ) )	iRand2 = 90000000;

		iTemp1 = GetWeaponItemCodeNumber( 1 );
		//TRACE("ItemProdution : GetWeaponItemCodeNumber() = %d, iRand2=%d\n", iTemp1, iRand2);
		if( iTemp1 == 0 )	return 0;
		iItemCode = iTemp1 * 100000;	// 루팅분포표 참조

		iRand3 = myrand(1, 10000);					// 종족(엘모, 카루스)
		if( COMPARE( iRand3, 1, 5000) )	iRand3 = 10000;
		else	iRand3 = 50000;
		iRand4 = myrand(1, 10000);					// 한손, 양손무기인지를 결정
		if( COMPARE( iRand4, 1, 5000) )	iRand4 = 0;
		else	iRand4 = 5000000;
		
		iRandom = GetItemCodeNumber(iItemLevel, 1);	// 레이매직표 적용
		//TRACE("ItemProdution : GetItemCodeNumber() = %d, iRand2=%d, iRand3=%d, iRand4=%d\n", iRandom, iRand2, iRand3, iRand4);
		if(iRandom == -1)	{						// 잘못된 아이템 생성실패
			return 0;
		}
		iRand5 = iRandom * 10;
		iItemNumber = iDefault + iItemCode + iRand2 + iRand3 + iRand4 + iRand5 + iItemGrade;

		//TRACE("ItemProdution : Weapon Success item_number = %d, default=%d, itemcode=%d, iRand2=%d, iRand3=%d, iRand4=%d, iRand5, iItemGrade=%d\n", iItemNumber, iDefault, iItemCode, iRand2, iRand3, iRand4, iRand5, iItemGrade);
	}
	else if( COMPARE( iRandom, 4001, 8001) )	{		// 방어구 아이템
		iDefault = 200000000;			
		
		iTemp1 = GetWeaponItemCodeNumber( 2 );
		//TRACE("ItemProdution : GetWeaponItemCodeNumber() = %d\n", iTemp1 );
		if( iTemp1 == 0 )	return 0;
		iItemCode = iTemp1 * 1000000;		// 루팅분포표 참조

		if( m_byMaxDamagedNation == KARUS )	{		// 종족
			iRandom = myrand(0, 10000);					// 직업의 갑옷을 결정		
			if( COMPARE( iRandom, 0, 2000) )	{		
				iRand2 = 0;	
				iRand3 = 10000;							// 전사갑옷은 아크투아렉만 가지도록
			}
			else if( COMPARE( iRandom, 2000, 4000) )	{
				iRand2 = 40000000;
				iRand3 = 20000;							// 로그갑옷은 투아렉만 가지도록
			}
			else if( COMPARE( iRandom, 4000, 6000) )	{
				iRand2 = 60000000;
				iRand3 = 30000;							// 마법사갑옷은 링클 투아렉만 가지도록
			}
			else if( COMPARE( iRandom, 6000, 10001) )	{
				iRand2 = 80000000;
				iRandom = myrand(0, 10000);
				if( COMPARE( iRandom, 0, 5000) )	iRand3 = 20000;	// 사제갑옷은 투아렉
				else								iRand3 = 40000;	// 사제갑옷은 퓨리투아렉
			}
		}
		else if( m_byMaxDamagedNation == ELMORAD )	{
			iRandom = myrand(0, 10000);					// 직업의 갑옷을 결정		
			if( COMPARE( iRandom, 0, 3300) )	{		
				iRand2 = 0;	
				iItemKey = myrand(0, 10000);			// 전사갑옷은 모든 종족이 가짐
				if( COMPARE( iItemKey, 0, 3333) )			iRand3 = 110000;
				else if( COMPARE( iItemKey, 3333, 6666) )	iRand3 = 120000;
				else if( COMPARE( iItemKey, 6666, 10001) )	iRand3 = 130000;
			}
			else if( COMPARE( iRandom, 3300, 5600) )	{
				iRand2 = 40000000;
				iItemKey = myrand(0, 10000);			// 로그갑옷은 남자와 여자만 가짐
				if( COMPARE( iItemKey, 0, 5000) )	iRand3 = 120000;
				else								iRand3 = 130000;
			}
			else if( COMPARE( iRandom, 5600, 7800) )	{
				iRand2 = 60000000;
				iItemKey = myrand(0, 10000);			// 마법사갑옷은 남자와 여자만 가짐
				if( COMPARE( iItemKey, 0, 5000) )	iRand3 = 120000;
				else								iRand3 = 130000;
			}
			else if( COMPARE( iRandom, 7800, 10001) )	{
				iRand2 = 80000000;
				iItemKey = myrand(0, 10000);			// 사제갑옷은 남자와 여자만 가짐
				if( COMPARE( iItemKey, 0, 5000) )	iRand3 = 120000;
				else								iRand3 = 130000;
			}
			
		}
		
		iTemp2 = myrand(0, 10000);					// 몸의 부위 아이템 결정
		if( COMPARE( iTemp2, 0, 2000) )				iRand4 = 1000;
		else if( COMPARE( iTemp2, 2000, 4000) )		iRand4 = 2000;
		else if( COMPARE( iTemp2, 4000, 6000) )		iRand4 = 3000;
		else if( COMPARE( iTemp2, 6000, 8000) )		iRand4 = 4000;
		else if( COMPARE( iTemp2, 8000, 10001) )	iRand4 = 5000;
		iRandom = GetItemCodeNumber(iItemLevel, 2);				// 레이매직표 적용
		if(iRandom == -1)	{		// 잘못된 아이템 생성실패
			return 0;
		}
		iRand5 = iRandom * 10;
		iItemNumber = iDefault + iRand2 + iItemCode + iRand3 + iRand4 + iRand5 + iItemGrade;	// iItemGrade : 아이템 등급생성표 적용
		//TRACE("ItemProdution : Defensive Success item_number = %d, default=%d, iRand2=%d, itemcode=%d, iRand3=%d, iRand4=%d, iRand5, iItemGrade=%d\n", iItemNumber, iDefault, iRand2, iItemCode, iRand3, iRand4, iRand5, iItemGrade);
	}
	else if( COMPARE( iRandom, 8001, 10001) )	{       // 악세사리 아이템
		iDefault = 300000000;
		iRandom = myrand(0, 10000);					// 악세사리 종류결정(귀고리, 목걸이, 반지, 벨트)
		if( COMPARE( iRandom, 0, 2500) )			iRand2 = 10000000;
		else if( COMPARE( iRandom, 2500, 5000) )	iRand2 = 20000000;
		else if( COMPARE( iRandom, 5000, 7500) )	iRand2 = 30000000;
		else if( COMPARE( iRandom, 7500, 10001) )	iRand2 = 40000000;
		iRand3 = myrand(1, 10000);					// 종족(엘모라드, 카루스)
		if( COMPARE( iRand3, 1, 5000) )	iRand3 = 110000;
		else	iRand3 = 150000;
		iRandom = GetItemCodeNumber(iItemLevel, 3);	// 레이매직표 적용
		//TRACE("ItemProdution : GetItemCodeNumber() = %d\n", iRandom);
		if(iRandom == -1)	{		// 잘못된 아이템 생성실패
			return 0;
		}
		iRand4 = iRandom * 10;
		iItemNumber = iDefault + iRand2 + iRand3 + iRand4 + iItemGrade;
		//TRACE("ItemProdution : Accessary Success item_number = %d, default=%d, iRand2=%d, iRand3=%d, iRand4=%d, iItemGrade=%d\n", iItemNumber, iDefault, iRand2, iRand3, iRand4, iItemGrade);
	}
	
	return iItemNumber;
}

int  CNpc::GetItemGrade(int item_grade)
{
	int iPercent = 0, iRandom = 0, i=0;
	_MAKE_ITEM_GRADE_CODE* pItemData = nullptr;

	iRandom = myrand(1, 1000);
	pItemData = g_pMain->m_MakeGradeItemArray.GetData(item_grade); 
	if(pItemData == nullptr)	return 0;


	for(i=0; i<9; i++)	{
		if(i == 0)	{
			if(pItemData->sGrade[i] == 0)	{
				iPercent += pItemData->sGrade[i];
				continue;
			}
			if( COMPARE( iRandom, 0, pItemData->sGrade[i]) )	return i+1;
			else	{
				iPercent += pItemData->sGrade[i];
				continue;
			}
		}
		else	{
			if(pItemData->sGrade[i] == 0)	{
				iPercent += pItemData->sGrade[i];
				continue;
			}

			if( COMPARE( iRandom, iPercent, iPercent+pItemData->sGrade[i]) )	return i+1;
			else	{
				iPercent += pItemData->sGrade[i];
				continue;
			}
		}
		
	}

	return 0;
}

int  CNpc::GetWeaponItemCodeNumber(int item_type)
{
	int iPercent = 0, iRandom = 0, i=0, iItem_level = 0;
	_MAKE_WEAPON* pItemData = nullptr;

	iRandom = myrand(0, 1000);
	if( item_type == 1 )	{		// 무기구
		iItem_level = GetLevel() / 10;
		pItemData = g_pMain->m_MakeWeaponItemArray.GetData(iItem_level); 
	}
	else if( item_type == 2 )	{	// 방어구
		iItem_level = GetLevel() / 10;
		pItemData = g_pMain->m_MakeDefensiveItemArray.GetData(iItem_level); 
	}

	if(pItemData == nullptr)	return 0;

	for(i=0; i<MAX_UPGRADE_WEAPON; i++)	{
		if(i == 0)	{
			if(pItemData->sClass[i] == 0)	{
				iPercent += pItemData->sClass[i];
				continue;
			}
			if( COMPARE( iRandom, 0, pItemData->sClass[i]) )	return i+1;
			else	{
				iPercent += pItemData->sClass[i];
				continue;
			}
		}
		else	{
			if(pItemData->sClass[i] == 0)	{
				iPercent += pItemData->sClass[i];
				continue;
			}

			if( COMPARE( iRandom, iPercent, iPercent+pItemData->sClass[i]) )	return i+1;
			else	{
				iPercent += pItemData->sClass[i];
				continue;
			}
		}
	}

	return 0;
}

int  CNpc::GetItemCodeNumber(int level, int item_type)
{
	int iItemCode = 0, iItemType = 0, iPercent = 0;

	_MAKE_ITEM_LARE_CODE * pItemData = g_pMain->m_MakeLareItemArray.GetData(level); 
	if (pItemData == nullptr)	
		return -1;

	int iItemPercent[] = { pItemData->sLareItem, pItemData->sMagicItem, pItemData->sGeneralItem };
	int iRandom = myrand(0, 1000);
	for (int i = 0; i < 3; i++)
	{
		if (i == 0)	
		{
			if (COMPARE(iRandom, 0, iItemPercent[i]))
			{
				iItemType = i+1;
				break;
			}
			else	
			{
				iPercent += iItemPercent[i];
				continue;
			}
		}
		else
		{
			if (COMPARE(iRandom, iPercent, iPercent+iItemPercent[i]))
			{
				iItemType = i+1;
				break;
			}
			else	{
				iPercent += iItemPercent[i];
				continue;
			}
		}
	}

	switch (iItemType)
	{
		case 1:						// lare item
			if (item_type == 1)
				iItemCode = myrand(16, 24);
			else if (item_type == 2)
				iItemCode = myrand(12, 24);
			else if (item_type == 3)
				iItemCode = myrand(0, 10);
			break;

		case 2:						// magic item
			if (item_type == 1)
				iItemCode = myrand(6, 15);
			else if (item_type == 2)
				iItemCode = myrand(6, 11);
			else if (item_type == 3)
				iItemCode = myrand(0, 10);
			break;

		case 3:						// general item
			if (item_type == 1
				|| item_type == 2)
				iItemCode = 5;
			else if (item_type == 3)
				iItemCode = myrand(0, 10);
			break;	
	}

	return iItemCode;
}

void CNpc::DurationMagic_4()
{
	MAP* pMap = GetMap();
	if (pMap == nullptr)	
		return;

	if (m_byDungeonFamily > 0)
	{
		CRoomEvent* pRoom = pMap->m_arRoomEventArray.GetData(m_byDungeonFamily);
		if (pRoom == nullptr)
		{
			// If it doesn't exist, there's no point continually assuming it exists. Just unset it.
			// We'll only throw the message once, so that the user knows they need to make sure the room event exists.
			m_byDungeonFamily = 0;
			TRACE("#### Npc-DurationMagic_4() failed: room event does not exist : [nid=%d, name=%s], m_byDungeonFamily(event)=%d #####\n", 
				GetID(), GetName().c_str(), m_byDungeonFamily);
		}
		else if (pRoom->m_byStatus == 3
				&& m_NpcState != NPC_DEAD
				&& m_byRegenType == 0)
		{
			m_byRegenType = 2;
			Dead(1);
			return;
		}
	}

	for (int i = 0; i < MAX_MAGIC_TYPE4; i++)	
	{
		if (!m_MagicType4[i].sDurationTime
			|| UNIXTIME < (m_MagicType4[i].tStartTime + m_MagicType4[i].sDurationTime))
			continue;

		// Remove buff state
		m_MagicType4[i].sDurationTime = 0;		
		m_MagicType4[i].tStartTime = 0;
		m_MagicType4[i].byAmount = 0;

		// Revert speed de/buff
		if (i == 5)	// BUFF_TYPE_SPEED
		{
			m_fSpeed_1 = m_fOldSpeed_1;
			m_fSpeed_2 = m_fOldSpeed_2;
		}
	}
}

void CNpc::ChangeMonsterInfomation(int iChangeType)
{
	if (m_sChangeSid == 0 || m_byChangeType == 0)
		return;	

	if (m_NpcState != NPC_DEAD)
		return;
	
	CNpcTable*	pNpcTable = GetProto();

	if (iChangeType == 1)
	{
		if (isMonster())	
			pNpcTable = g_pMain->m_arMonTable.GetData(m_sChangeSid);
		else
			pNpcTable = g_pMain->m_arNpcTable.GetData(m_sChangeSid);
	}

	if (pNpcTable == nullptr)
	{
		TRACE("##### ChangeMonsterInfomation Sid Fail -- Sid = %d #####\n", m_sChangeSid);
		return;
	}

	m_proto			= pNpcTable;
	m_sSize			= pNpcTable->m_sSize;
	m_iWeapon_1		= pNpcTable->m_iWeapon_1;
	m_iWeapon_2		= pNpcTable->m_iWeapon_2;
	m_bNation		= pNpcTable->m_byGroup;
	m_byActType		= pNpcTable->m_byActType;
	m_byRank		= pNpcTable->m_byRank;
	m_byTitle		= pNpcTable->m_byTitle;
	m_iSellingGroup = pNpcTable->m_iSellingGroup;
	m_iHP			= pNpcTable->m_iMaxHP;
	m_iMaxHP		= pNpcTable->m_iMaxHP;
	m_sMP			= pNpcTable->m_sMaxMP;
	m_sMaxMP		= pNpcTable->m_sMaxMP;
	m_sAttack		= pNpcTable->m_sAttack;	
	m_sDefense		= pNpcTable->m_sDefense;
	m_sHitRate		= pNpcTable->m_sHitRate;
	m_sEvadeRate	= pNpcTable->m_sEvadeRate;
	m_sDamage		= pNpcTable->m_sDamage;	
	m_sAttackDelay	= pNpcTable->m_sAttackDelay;
	m_sSpeed		= pNpcTable->m_sSpeed;
	m_fSpeed_1		= (float)pNpcTable->m_bySpeed_1;
	m_fSpeed_2		= (float)pNpcTable->m_bySpeed_2;
	m_fOldSpeed_1	= (float)pNpcTable->m_bySpeed_1;
	m_fOldSpeed_2	= (float)pNpcTable->m_bySpeed_2;
	m_sStandTime	= pNpcTable->m_sStandTime;
	m_byFireR		= pNpcTable->m_byFireR;
	m_byColdR		= pNpcTable->m_byColdR;
	m_byLightningR	= pNpcTable->m_byLightningR;
	m_byMagicR		= pNpcTable->m_byMagicR;
	m_byDiseaseR	= pNpcTable->m_byDiseaseR;
	m_byPoisonR		= pNpcTable->m_byPoisonR;
	m_bySearchRange	= pNpcTable->m_bySearchRange;
	m_byAttackRange	= pNpcTable->m_byAttackRange;
	m_byTracingRange= pNpcTable->m_byTracingRange;
	m_iMoney		= pNpcTable->m_iMoney;
	m_iItem			= pNpcTable->m_iItem;
	m_tNpcLongType	= pNpcTable->m_byDirectAttack;	
	m_byWhatAttackType = pNpcTable->m_byMagicAttack;
}

void CNpc::DurationMagic_3()
{
	int duration_damage = 0;

	for (int i = 0; i < MAX_MAGIC_TYPE3; i++)
	{
		if (!m_MagicType3[i].byHPDuration
			|| UNIXTIME < (m_MagicType3[i].tStartTime + m_MagicType3[i].byHPInterval))
			continue;

		m_MagicType3[i].byHPInterval += 2;

		// ignore healing, just apply DOT
		if (m_MagicType3[i].sHPAmount < 0)
		{
			duration_damage = abs(m_MagicType3[i].sHPAmount);

			// If DOT damage has killed the NPC....
			if (!SetDamage(-1, duration_damage, m_MagicType3[i].sHPAttackUserID))
			{
				SendAttackSuccess(MAGIC_ATTACK_TARGET_DEAD, m_MagicType3[i].sHPAttackUserID, duration_damage, m_iHP, 1, DURATION_ATTACK);

				m_MagicType3[i].tStartTime = 0;
				m_MagicType3[i].byHPDuration = 0;
				m_MagicType3[i].byHPInterval = 2;
				m_MagicType3[i].sHPAmount = 0;
				m_MagicType3[i].sHPAttackUserID = -1; 
				duration_damage = 0;
			}
			// Not dead yet.
			else
			{
				SendAttackSuccess(ATTACK_SUCCESS, m_MagicType3[i].sHPAttackUserID, duration_damage, m_iHP, 1, DURATION_ATTACK);	
			}
		}

		// Has DOT expired yet? If it's expired, we can remove it.
		if (UNIXTIME < (m_MagicType3[i].tStartTime + m_MagicType3[i].byHPDuration))
			continue;

		m_MagicType3[i].tStartTime = 0;
		m_MagicType3[i].byHPDuration = 0;
		m_MagicType3[i].byHPInterval = 2;
		m_MagicType3[i].sHPAmount = 0;
		m_MagicType3[i].sHPAttackUserID = -1;
		duration_damage = 0;
	}	
}

time_t CNpc::NpcSleeping()
{
	if (g_pMain->m_byNight == 1)	
	{
		m_NpcState = NPC_STANDING;
		return m_Delay;
	}

	m_NpcState = NPC_SLEEPING;
	return m_sStandTime;
}

time_t CNpc::NpcFainting()
{
	if (UNIXTIME < (m_tFaintingTime + FAINTING_TIME)) 
		return -1;

	m_NpcState = NPC_STANDING;
	m_tFaintingTime = 0;
	return 0;
}

time_t CNpc::NpcHealing()
{
	if (m_proto->m_tNpcType != NPC_HEALER)
	{
		InitTarget();
		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}

	int ret = IsCloseTarget(m_byAttackRange, 2);
	if (ret == 0)   
	{
		m_sStepCount = 0;
		m_byActionFlag = ATTACK_TO_TRACE;
		m_NpcState = NPC_TRACING;
		return 0;
	}	
	else if (ret == 2)
	{
		if (m_tNpcLongType == 2)	
			return LongAndMagicAttack();

		m_sStepCount = 0;
		m_byActionFlag = ATTACK_TO_TRACE;
		m_NpcState = NPC_TRACING;
		return 0;
	}
	else if (ret == -1)
	{
		m_NpcState = NPC_STANDING;
		InitTarget();
		return 0;
	}

	if (hasTarget()
		&& m_Target.id >= NPC_BAND)	
	{
		CNpc * pNpc = g_pMain->m_arNpc.GetData(m_Target.id);
		if (pNpc == nullptr
			|| pNpc->isDead())
		{
			InitTarget();
			return m_sStandTime;
		}

		int iHP = (int)(pNpc->GetMaxHealth() * 0.9);
		if (pNpc->GetHealth() >= iHP)	
		{
			InitTarget();
		}
		else	
		{
			CNpcMagicProcess::MagicPacket(MAGIC_EFFECTING, m_proto->m_iMagic3, GetID(), m_Target.id);
			return m_sAttackDelay;
		}
	}


	int iMonsterNid = FindFriend(2);
	if (iMonsterNid == 0)	
	{
		InitTarget();
		m_NpcState = NPC_STANDING;
		return m_sStandTime;
	}

	CNpcMagicProcess::MagicPacket(MAGIC_EFFECTING, m_proto->m_iMagic3, GetID(), iMonsterNid);
	return m_sAttackDelay;
}

int CNpc::GetPartyExp(int party_level, int man, int nNpcExp)
{
	int nPartyExp = 0;
	int nLevel = party_level / man;
	double TempValue = 0;
	nLevel = GetLevel() - nLevel;

	if (nLevel < 2)
	{
		nPartyExp = nNpcExp; // x1
	}
	else if (nLevel >= 2 && nLevel < 5)
	{
		TempValue = nNpcExp * 1.2; // x1.2
		nPartyExp = (int)TempValue;
		if (TempValue > nPartyExp)  
			nPartyExp++;
	}
	else if (nLevel >= 5 && nLevel < 8)	
	{
		TempValue = nNpcExp * 1.5; // x1.5
		nPartyExp = (int)TempValue;
		if (TempValue > nPartyExp)  
			nPartyExp++;
	}
	else if (nLevel >= 8)	
	{
		nPartyExp = nNpcExp * 2; // x2
	}	

	return nPartyExp;
}

void CNpc::ChangeAbility(int iChangeType)
{
	if (iChangeType > 2)
		return;

	int nHP = 0, nAC=0, nDamage=0, nMagicR=0, nDiseaseR=0, nPoisonR=0, nLightningR=0, nFireR=0, nColdR=0;
	CNpcTable*	pNpcTable = GetProto();

	if (iChangeType == BATTLEZONE_OPEN)
	{
		nHP = (int)(pNpcTable->m_iMaxHP / 2);
		nAC = (int)(pNpcTable->m_sDefense * 0.2);
		nDamage = (int)(pNpcTable->m_sDamage * 0.3);
		nMagicR = (int)(pNpcTable->m_byMagicR / 2);
		nDiseaseR = (int)(pNpcTable->m_byDiseaseR / 2);
		nPoisonR = (int)(pNpcTable->m_byPoisonR / 2);
		nLightningR = (int)(pNpcTable->m_byLightningR / 2);
		nFireR = (int)(pNpcTable->m_byFireR / 2);
		nColdR = (int)(pNpcTable->m_byColdR / 2);
		m_iMaxHP = nHP;

		if (GetHealth() > nHP)
			HpChange();

		m_sDefense = nAC;
		m_sDamage = nDamage;
		m_byFireR		= nFireR;
		m_byColdR		= nColdR;
		m_byLightningR	= nLightningR;
		m_byMagicR		= nMagicR;
		m_byDiseaseR	= nDiseaseR;
		m_byPoisonR		= nPoisonR;
	}
	else if (iChangeType == BATTLEZONE_CLOSE)
	{
		m_iMaxHP		= pNpcTable->m_iMaxHP;
		if (GetMaxHealth() > GetHealth())
		{
			m_iHP = GetMaxHealth() - 50;
			HpChange();
		}

		m_sDamage		= pNpcTable->m_sDamage;
		m_sDefense		= pNpcTable->m_sDefense;
		m_byFireR		= pNpcTable->m_byFireR;
		m_byColdR		= pNpcTable->m_byColdR;
		m_byLightningR	= pNpcTable->m_byLightningR;
		m_byMagicR		= pNpcTable->m_byMagicR;
		m_byDiseaseR	= pNpcTable->m_byDiseaseR;
		m_byPoisonR		= pNpcTable->m_byPoisonR;
	}
}

bool CNpc::Teleport()
{
	int nX=0, nZ=0, nTileX=0, nTileZ=0;
	MAP* pMap = GetMap();
	if (pMap == nullptr)	
		return false;

	while (1)	
	{
		nX = myrand(0, 10);
		nX = myrand(0, 10);
		nX = (int)GetX() + nX;
		nZ = (int)GetZ() + nZ;
		nTileX = nX / TILE_SIZE;
		nTileZ = nZ / TILE_SIZE;

		if (nTileX > pMap->GetMapSize())		
			nTileX = pMap->GetMapSize();

		if (nTileZ > pMap->GetMapSize())		
			nTileZ = pMap->GetMapSize();

		if(nTileX < 0 || nTileZ < 0)	
		{
			TRACE("#### Npc-SetLive() Fail : nTileX=%d, nTileZ=%d #####\n", nTileX, nTileZ);
			return false;
		}
		break;
	}	

	Packet result(AG_NPC_INOUT);
	result << uint8(NPC_OUT) << GetID() << GetX() << GetZ() << GetY();
	g_pMain->Send(&result);

	m_curx = (float)nX;	m_curz = (float)nZ;

	result.clear();
	result << uint8(NPC_IN) << GetID() << GetX() << GetZ() << float(0.0f);
	g_pMain->Send(&result);

	RegisterRegion(GetX(), GetZ());
	return true;
}