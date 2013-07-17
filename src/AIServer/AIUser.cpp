#include "stdafx.h"
#include "User.h"
#include "Region.h"
#include "GameSocket.h"
#include "Npc.h"
#include "MAP.h"

#define MORAL_GOOD		0x01
#define MORAL_BAD		0x02
#define MORAL_NEUTRAL	0x03

static float surround_fx[8] = {0.0f, -0.7071f, -1.0f, -0.7083f,  0.0f,  0.7059f,  1.0000f, 0.7083f};
static float surround_fz[8] = {1.0f,  0.7071f,  0.0f, -0.7059f, -1.0f, -0.7083f, -0.0017f, 0.7059f};

CUser::CUser() : Unit(UnitPlayer) {}
CUser::~CUser() {}

void CUser::Initialize()
{
	Unit::Initialize();

	m_iUserId = -1;					// User의 번호
	m_bLive = AI_USER_DEAD;			// 죽었니? 살았니?
	m_sHP = 0;							// HP
	m_sMP = 0;							// MP
	m_sMaxHP = 0;							// MaxHP
	m_sMaxMP = 0;							// MaxMP
	m_state = 0;				// User의 상태
	m_sOldRegionX = 0;	
	m_sOldRegionZ = 0;	
	m_bResHp = 0;						// 회복량
	m_bResMp = 0;
	m_bResSta = 0;
	m_sItemAc = 0;
	m_byNowParty = 0;
	m_sPartyTotalLevel = 0;
	m_byPartyTotalMan = 0;
	m_sPartyNumber = -1;
	m_byIsOP = 0;
	m_bInvisibilityType = 0;
	InitNpcAttack();
}

void CUser::Attack(int sid, int tid)
{
	CNpc* pNpc = g_pMain->GetNpcPtr(tid);
	if (pNpc == nullptr
		|| pNpc->isDead())
		return;

/*	if (pNpc->isGuard())
	{
		pNpc->m_Target.id = GetID();
		pNpc->m_Target.bSet = true;
		pNpc->m_Target.x = GetX();
		pNpc->m_Target.y = GetY();
		pNpc->Attack();
	//	return;
	}	*/

	int nFinalDamage = GetDamage(pNpc);
	if (nFinalDamage <= 0)
		SendAttackSuccess(tid, ATTACK_FAIL, nFinalDamage, pNpc->m_iHP);
	else if (!pNpc->SetDamage(nFinalDamage, GetID()))
		SendAttackSuccess(tid, ATTACK_TARGET_DEAD, nFinalDamage, pNpc->m_iHP);
	else
		SendAttackSuccess(tid, ATTACK_SUCCESS, nFinalDamage, pNpc->m_iHP);
}

void CUser::SendAttackSuccess(short tid, uint8 bResult, short sDamage, int nHP, short sAttack_type, uint8 type /*= 1*/, short sid /*= -1*/)
{
	if (sid < 0)
		sid = GetID();

	Packet result(AG_ATTACK_RESULT, type);
	result << bResult << sid << tid << sDamage << nHP << uint8(sAttack_type);
	g_pMain->Send(&result);
}

/**
 * @brief	Applies damage to a user.
 *
 * @param	damage		The damage.
 * @param	attackerID  The attacker's ID.
 *
 * @return	false if the damage caused death, true if not.
 * 			NOTE: The somewhat backwards logic is for consistency with CNpc::SetDamage().
 */
bool CUser::SetDamage(int damage, int attackerID)
{
	if (damage <= 0 || isDead())
		return true;

	m_sHP -= (short)damage;

	if (m_sHP > 0)
		return true;

	m_sHP = 0;
	Dead(attackerID, damage);
	return false;
}

void CUser::Dead(int tid, int nDamage)
{
	if (m_bLive == AI_USER_DEAD)
		return;

	m_sHP = 0;
	m_bLive = AI_USER_DEAD;

	InitNpcAttack();

	MAP* pMap = GetMap();
	if (pMap == nullptr 
		|| m_sRegionX < 0 || m_sRegionZ < 0 
		|| m_sRegionX > pMap->GetXRegionMax() || m_sRegionZ > pMap->GetZRegionMax())
		return;

	pMap->RegionUserRemove(m_sRegionX, m_sRegionZ, GetID());

	TRACE("*** User Dead = %d, %s ***\n", GetID(), GetName().c_str());
	if (tid > 0)
		SendAttackSuccess(GetID(), ATTACK_TARGET_DEAD, nDamage, m_sHP, 1, 2, tid /*sid*/);
}

void CUser::SendHP()
{
	if (isDead())
		return;

	Packet result(AG_USER_SET_HP);
	result << GetID() << uint32(m_sHP);
	g_pMain->Send(&result);   
}

void CUser::SetExp(int iNpcExp, int iLoyalty, int iLevel)
{
	int nExp = 0;
	int nLoyalty = 0;
	int nLevel = 0;
	double TempValue = 0;
	nLevel = iLevel - m_bLevel;

	if(nLevel <= -14)	{
		TempValue = iNpcExp * 0.2;
		nExp = (int)TempValue;
		if(TempValue > nExp)  nExp=nExp+1;
		TempValue = iLoyalty * 0.2;
		nLoyalty = (int)TempValue;
		if(TempValue > nLoyalty)  nLoyalty=nLoyalty+1;
	}
	else if(nLevel <= -8 && nLevel >= -13)
	{
		TempValue = iNpcExp * 0.5;
		nExp = (int)TempValue;
		if(TempValue > nExp)  nExp=nExp+1;
		TempValue = iLoyalty * 0.5;
		nLoyalty = (int)TempValue;
		if(TempValue > nLoyalty)  nLoyalty=nLoyalty+1;
	}
	else if(nLevel <= -2 && nLevel >= -7)
	{
		TempValue = iNpcExp * 0.8;
		nExp = (int)TempValue;
		if(TempValue > nExp)  nExp=nExp+1;
		TempValue = iLoyalty * 0.8;
		nLoyalty = (int)TempValue;
		if(TempValue > nLoyalty)  nLoyalty=nLoyalty+1;
	}
	else if(nLevel >= -1 ) // && nLevel < 2)
	{
		nExp = iNpcExp * 1;
		nLoyalty = iLoyalty * 1;
	}

	SendExp(nExp, nLoyalty);
}

void CUser::SetPartyExp(int iNpcExp, int iLoyalty, int iPartyLevel, int iMan)
{
	int nExp = 0;
	int nLoyalty = 0;
	int nPercent = 0, nLevelPercent = 0, nExpPercent = 0;
	double TempValue = 0;

	TempValue = (double)iPartyLevel / 100.0;
	nExpPercent = (int)(iNpcExp * TempValue);

	SendExp(iNpcExp, iLoyalty);
}

void CUser::SendExp(int32 iExp, int32 iLoyalty, int tType)
{
	Packet result(AG_USER_EXP);
	result << GetID() << iExp << iLoyalty;
	g_pMain->Send(&result);   	
}

void CUser::InitNpcAttack()
{
	memset(&m_sSurroundNpcNumber, -1, sizeof(m_sSurroundNpcNumber));
}

int CUser::IsSurroundCheck(float fX, float fY, float fZ, int NpcID)
{
	int nDir = 0;
	__Vector3 vNpc, vUser, vDis;
	vNpc.Set(fX, fY, fZ);
	float fDX, fDZ;
	float fDis = 0.0f, fCurDis=1000.0f;
	bool bFlag = false;
	for(int i=0; i<8; i++)
	{
		//if(m_sSurroundNpcNumber[i] != -1) continue;
		if(m_sSurroundNpcNumber[i] == NpcID)
		{
			if (bFlag)
				m_sSurroundNpcNumber[i] = -1;
			else
			{
				m_sSurroundNpcNumber[i] = NpcID;
				nDir = i+1;
				bFlag = true;
			}
			//return nDir;
		}

		if(m_sSurroundNpcNumber[i] == -1 && bFlag==false)
		{
			fDX = GetX() + surround_fx[i]; 
			fDZ = GetZ() + surround_fz[i]; 
			vUser.Set(fDX, 0.0f, fDZ);
			vDis = vUser - vNpc;
			fDis = vDis.Magnitude();
			if(fDis < fCurDis)
			{
				nDir = i+1;
				fCurDis = fDis;
			}
		}
	}

	
/*	TRACE("User-Sur : [0=%d,1=%d,2=%d,3=%d,4=%d,5=%d,6=%d,7=%d]\n", m_sSurroundNpcNumber[0], 
		m_sSurroundNpcNumber[1], m_sSurroundNpcNumber[2], m_sSurroundNpcNumber[3], m_sSurroundNpcNumber[4],
		m_sSurroundNpcNumber[5],m_sSurroundNpcNumber[6], m_sSurroundNpcNumber[7]);
	*/
	if(nDir != 0)
	{
		m_sSurroundNpcNumber[nDir-1] = NpcID;
	}

	return nDir;
}

void CUser::HealMagic()
{
	int region_x = (int)(GetX() / VIEW_DIST);
	int region_z = (int)(GetZ() / VIEW_DIST);

	MAP* pMap = GetMap();
	if (pMap == nullptr) return;
	int min_x = region_x - 1;	if(min_x < 0) min_x = 0;
	int min_z = region_z - 1;	if(min_z < 0) min_z = 0;
	int max_x = region_x + 1;	if(max_x > pMap->GetXRegionMax()) max_x = pMap->GetXRegionMax();
	int max_z = region_z + 1;	if(min_z > pMap->GetZRegionMax()) min_z = pMap->GetZRegionMax();

	int search_x = max_x - min_x + 1;		
	int search_z = max_z - min_z + 1;	
	
	int i, j;

	for(i = 0; i < search_x; i++)	{
		for(j = 0; j < search_z; j++)	{
			HealAreaCheck( min_x+i, min_z+j );
		}
	}
}

void CUser::HealAreaCheck(int rx, int rz)
{
	MAP* pMap = GetMap();
	if (pMap == nullptr) return;

	if (rx < 0 || rz < 0 || rx > pMap->GetXRegionMax() || rz > pMap->GetZRegionMax())	
	{
		TRACE("#### CUser-HealAreaCheck() Fail : [nid=%d, name=%s], nRX=%d, nRZ=%d #####\n", GetID(), GetName().c_str(), rx, rz);
		return;
	}

	static const float fRadius = 10.0f; // 30m

	FastGuard lock(pMap->m_lock);
	CRegion *pRegion = &pMap->m_ppRegion[rx][rz];
	foreach_stlmap (itr, pRegion->m_RegionNpcArray)
	{
		CNpc * pNpc = g_pMain->GetNpcPtr(itr->first);
		if (pNpc == nullptr || pNpc->isDead()
			|| !pNpc->isHostileTo(this))
			continue;

		if (isInRangeSlow(pNpc, fRadius))
			pNpc->ChangeTarget(1004, this);
	}
}