#include "stdafx.h"
#include "User.h"
#include "Region.h"
#include "GameSocket.h"
#include "extern.h"
#include "Npc.h"
#include "MAP.h"

#define MORAL_GOOD		0x01
#define MORAL_BAD		0x02
#define MORAL_NEUTRAL	0x03

static float surround_fx[8] = {0.0f, -0.7071f, -1.0f, -0.7083f,  0.0f,  0.7059f,  1.0000f, 0.7083f};
static float surround_fz[8] = {1.0f,  0.7071f,  0.0f, -0.7059f, -1.0f, -0.7083f, -0.0017f, 0.7059f};

CUser::CUser() {}
CUser::~CUser() {}

void CUser::Initialize()
{
	m_MagicProcess.m_pSrcUser = this;

	memset(m_strUserID, 0, MAX_ID_SIZE+1);	// 캐릭터의 이름
	m_iUserId = -1;					// User의 번호
	m_bLive = USER_DEAD;			// 죽었니? 살았니?
	m_curx = 0.0f;				// 현재 X 좌표
	m_cury = 0.0f;				// 현재 Y 좌표
	m_curz = 0.0f;				// 현재 Z 좌표
	m_curZone = -1;				// 현재 존
	m_pMap = NULL;
	m_bNation = 0;						// 소속국가
	m_bLevel = 0;						// 레벨
	m_sHP = 0;							// HP
	m_sMP = 0;							// MP
	m_sMaxHP = 0;							// MaxHP
	m_sMaxMP = 0;							// MaxMP
	m_state = 0;				// User의 상태
	m_sRegionX = 0;						// 현재 영역 X 좌표
	m_sRegionZ = 0;						// 현재 영역 Z 좌표
	m_sOldRegionX = 0;	
	m_sOldRegionZ = 0;	
	m_bResHp = 0;						// 회복량
	m_bResMp = 0;
	m_bResSta = 0;
	m_sHitDamage = 0;					// Hit
	m_sAC = 0;
	m_sItemAC = 0;
	m_fHitrate = 0.0f;					// 타격 성공률
	m_fAvoidrate = 0;					// 회피 성공률
	m_bLogOut = false;				// Logout 중인가?
	m_byNowParty = 0;
	m_sPartyTotalLevel = 0;
	m_byPartyTotalMan = 0;
	m_sPartyNumber = -1;
	m_byIsOP = 0;
	m_bInvisibilityType = 0;
	m_lUsed = 0;
	InitNpcAttack();

	m_bMagicTypeLeftHand = 0;			
	m_bMagicTypeRightHand = 0;			
	m_sMagicAmountLeftHand = 0;       
	m_sMagicAmountRightHand = 0;      
}

void CUser::Attack(int sid, int tid)
{
	CNpc* pNpc = g_pMain->m_arNpc.GetData(tid-NPC_BAND);
	if(pNpc == NULL)	return;
	if(pNpc->m_NpcState == NPC_DEAD) return;
	if(pNpc->m_iHP == 0) return;

/*	if(pNpc->m_proto->m_tNpcType == NPCTYPE_GUARD)					// 경비병이면 타겟을 해당 유저로
	{
		pNpc->m_Target.id = m_iUserId + USER_BAND;
		pNpc->m_Target.x = m_curx;
		pNpc->m_Target.y = m_cury;
		pNpc->m_Target.failCount = 0;
		pNpc->Attack();
	//	return;
	}	*/

	int nDefence = 0, nFinalDamage = 0;
	// NPC 방어값 
	nDefence = pNpc->GetDefense();

	// 명중이면 //Damage 처리 ----------------------------------------------------------------//
	nFinalDamage = GetDamage(tid);
		
	// Calculate Target HP	 -------------------------------------------------------//
	short sOldNpcHP = pNpc->m_iHP;

	if(pNpc->SetDamage(0, nFinalDamage, m_strUserID, m_iUserId + USER_BAND) == false)
	{
		// Npc가 죽은 경우,,
		pNpc->SendExpToUserList(); // 경험치 분배!!
		pNpc->SendDead();
		SendAttackSuccess(tid, ATTACK_TARGET_DEAD, nFinalDamage, pNpc->m_iHP);
	}
	else
	{
		// 공격 결과 전송
		SendAttackSuccess(tid, ATTACK_SUCCESS, nFinalDamage, pNpc->m_iHP);
	}
}

void CUser::SendAttackSuccess(short tid, uint8 bResult, short sDamage, int nHP, short sAttack_type, uint8 type /*= 1*/, short sid /*= -1*/)
{
	if (sid < 0)
		sid = m_iUserId + USER_BAND;

	Packet result(AG_ATTACK_RESULT, type);
	result << bResult << sid << tid << sDamage << nHP << uint8(sAttack_type);
	g_pMain->Send(&result);
}

void CUser::SetDamage(int damage, int tid)
{
	if (damage <= 0 || m_bLive == USER_DEAD)
		return;

	m_sHP -= (short)damage;

	if (m_sHP <= 0)
	{
		m_sHP = 0;
		Dead(tid, damage);
	}
}

void CUser::Dead(int tid, int nDamage)
{
	if (m_bLive == USER_DEAD)
		return;

	m_sHP = 0;
	m_bLive = USER_DEAD;

	InitNpcAttack();

	MAP* pMap = GetMap();
	if (pMap == NULL 
		|| m_sRegionX < 0 || m_sRegionZ < 0 
		|| m_sRegionX > pMap->GetXRegionMax() || m_sRegionZ > pMap->GetZRegionMax())
		return;

	pMap->RegionUserRemove(m_sRegionX, m_sRegionZ, m_iUserId);

	TRACE("*** User Dead = %d, %s ***\n", m_iUserId, m_strUserID);
	if (tid > 0)
		SendAttackSuccess(m_iUserId+USER_BAND, ATTACK_TARGET_DEAD, nDamage, m_sHP, 1, 2, tid /*sid*/);
}

void CUser::SendHP()
{
	if (m_bLive == USER_DEAD)
		return;

	Packet result(AG_USER_SET_HP);
	result << m_iUserId << uint32(m_sHP);
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
		//TRACE("$$ User - SetExp Level Fail : %s, exp=%d, loyalty=%d, mylevel=%d, level=%d $$\n", m_strUserID, iNpcExp, iLoyalty, m_proto->m_sLevel, iLevel);
		//return;
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
/*	else if(nLevel >= 2 && nLevel < 5)
	{
		TempValue = iNpcExp * 1.2;
		nExp = (int)TempValue;
		if(TempValue > nExp)  nExp=nExp+1;
		TempValue = iLoyalty * 1.2;
		nLoyalty = (int)TempValue;
		if(TempValue > nLoyalty)  nLoyalty=nLoyalty+1;
	}
	else if(nLevel >= 5 && nLevel < 8)
	{
		TempValue = iNpcExp * 1.5;
		nExp = (int)TempValue;
		if(TempValue > nExp)  nExp=nExp+1;
		TempValue = iLoyalty * 1.5;
		nLoyalty = (int)TempValue;
		if(TempValue > nLoyalty)  nLoyalty=nLoyalty+1;
	}
	else if(nLevel >= 8)
	{
		nExp = iNpcExp * 2;
		nLoyalty = iLoyalty * 2;
	}	*/

	//TRACE("$$ User - SetExp Level : %s, exp=%d->%d, loy=%d->%d, mylevel=%d, monlevel=%d $$\n", m_strUserID, iNpcExp, nExp, iLoyalty, nLoyalty, m_proto->m_sLevel, iLevel);

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

	//TRACE("$$ User - SetPartyExp Level : %s, exp=%d->%d, loy=%d->%d, mylevel=%d, iPartyLevel=%d $$\n", m_strUserID, iNpcExp, nExpPercent, iLoyalty, nLoyalty, m_proto->m_sLevel, iPartyLevel);

	SendExp(iNpcExp, iLoyalty);
}

//  경험치를 보낸다. (레벨업일때 관련 수치를 준다)
void CUser::SendExp(int iExp, int iLoyalty, int tType)
{
	Packet result(AG_USER_EXP);
	result << m_iUserId << uint16(iExp) << uint16(iLoyalty);
	g_pMain->Send(&result);   	
}

short CUser::GetDamage(int tid, int magicid)
{
	short damage = 0;	float Attack = 0;	float Avoid = 0;
	short Hit = 0;	short HitB = 0;
	short Ac = 0; int random = 0;	uint8 result;

	_MAGIC_TABLE* pTable = NULL;
	_MAGIC_TYPE1* pType1 = NULL; 
	_MAGIC_TYPE2* pType2 = NULL;

	if (tid < NPC_BAND || tid > INVALID_BAND)	return damage;
	CNpc* pNpc = NULL;
	pNpc = g_pMain->m_arNpc.GetData(tid-NPC_BAND);
	if(pNpc == NULL)		return damage;
	if(pNpc->m_proto->m_tNpcType == NPC_ARTIFACT || pNpc->m_proto->m_tNpcType == NPC_PHOENIX_GATE || pNpc->m_proto->m_tNpcType == NPC_GATE_LEVER || pNpc->m_proto->m_tNpcType == NPC_SPECIAL_GATE ) return damage;
	
	Attack = (float)m_fHitrate;			// 공격민첩
	Avoid = (float)pNpc->m_sEvadeRate;	// 방어민첩	
	Hit = m_sHitDamage;					// 공격자 Hit 
//	Ac = (short)(pNpc->m_sDefense) + pNpc->m_sLevel;		// 방어자 Ac 2002.07.06
	Ac = (short)(pNpc->m_sDefense);		// 방어자 Ac 
	HitB = (int)((Hit * 200) / (Ac + 240)) ;	// 새로운 공격식의 B

	if( magicid > 0 )	{	 // Skill Hit.
		pTable = g_pMain->m_MagictableArray.GetData( magicid );     // Get main magic table.
		if( !pTable ) return -1; 
		
		if (pTable->bType[0] == 1)	{	// SKILL HIT!
			pType1 = g_pMain->m_Magictype1Array.GetData( magicid );	    // Get magic skill table type 1.
			if( !pType1 ) return -1;     	                                

			if(pType1->bHitType)	{  // Non-relative hit.
				random = myrand(0,100) ;       
				if (pType1->sHitRate <= random)			result = FAIL;
				else 									result = SUCCESS;
			}
			else	{     // Relative hit.
				result = GetHitRate( (Attack / Avoid) * (pType1->sHitRate / 100.0f) );
			}
/*
			if(pType1->bHitType) {  // Non-relative hit.
				Hit = m_sHitDamage * (pType1->sHit / 100.0f);
			}
			else {
//				Hit = (m_sHitDamage - pNpc->m_sDefense) 
				Hit = HitB * (pType1->sHit / 100.0f) ;
			}		
*/
			Hit = (short)(HitB * (pType1->sHit / 100.0f));
		}
		else if (pTable->bType[0] == 2)   { // ARROW HIT!
			pType2 = g_pMain->m_Magictype2Array.GetData( magicid );	    // Get magic skill table type 1.
			if( !pType2 ) return -1; 
			
			if(pType2->bHitType == 1 || pType2->bHitType == 2 )   {  // Non-relative/Penetration hit.
				random = myrand(0,100) ; 
				if (pType2->sHitRate <= random)			result = FAIL;
				else									result = SUCCESS;
				//result = SUCCESS;
			}
			else	{     // Relative hit/Arc hit.
				result = GetHitRate( (Attack / Avoid) * (pType2->sHitRate / 100.0f) );
			}
			
			if(pType2->bHitType == 1 /* || pType2->bHitType == 2 */ )   {
				Hit = (short)(m_sHitDamage * (pType2->sAddDamage / 100.0f));
			}
			else{				
//				Hit = (m_sHitDamage - pNpc->m_sDefense) * (pType2->sAddDamage / 100.0f);
				Hit = (short)(HitB * (pType2->sAddDamage / 100.0f));
			}
		}
	}
	else	{	// Normal Hit.
		result = GetHitRate(Attack/Avoid);		// 타격비 구하기
	}

	switch(result)	{
		case GREAT_SUCCESS:
		case SUCCESS:
		case NORMAL:		
			if( magicid > 0 ) {	 // 스킬 공격
				damage = (short)Hit;
				random = myrand(0, damage);
//				damage = (short)((0.85f * (float)Hit) + 0.3f * (float)random);
				if (pTable->bType[0] == 1) {
					damage = (short)((float)Hit + 0.3f * (float)random + 0.99);
				}
				else {
					damage = (short)((float)(Hit*0.6f) + 1.0f * (float)random + 0.99);
				}
			}				
			else {			//일반 공격	
				damage = (short)(HitB);
				random = myrand(0, damage);
				damage = (short)((0.85f * (float)HitB) + 0.3f * (float)random);
			}

			break;
		case FAIL:  // 사장님 요구 
				damage = 0;
			break;
	}
	
	if (damage <= 0)
		return 0;

	damage = GetMagicDamage(damage,tid);	// 2. Magical item damage....

	//return 3000;
	return damage;		
}

short CUser::GetMagicDamage(int damage, short tid)
{
	short total_r = 0;
	short temp_damage = 0;

	CNpc* pNpc = NULL;
	pNpc = g_pMain->m_arNpc.GetData(tid-NPC_BAND);
	if(!pNpc) return damage;
	
	if (m_bMagicTypeRightHand > 4 && m_bMagicTypeRightHand < 8) {
		temp_damage = damage * m_sMagicAmountRightHand / 100;
	}

	switch (m_bMagicTypeRightHand) {	// RIGHT HAND!!!
		case 1 :	// Fire Damage
			total_r = pNpc->m_byFireR ;
			break;
		case 2 :	// Ice Damage
			total_r = pNpc->m_byColdR ;
			break;
		case 3 :	// Lightning Damage
			total_r = pNpc->m_byLightningR ;
			break;
		case 4 :	// Poison Damage
			total_r = pNpc->m_byPoisonR ;
			break;
		case 5 :	// HP Drain						
			break;
		case 6 :	// MP Damage		
			pNpc->MSpChange(2, -temp_damage);
			break;
		case 7 :	// MP Drain				
			break;
		case 0:
			break;
	}

	if (m_bMagicTypeRightHand > 0 && m_bMagicTypeRightHand < 5) {	
		temp_damage = m_sMagicAmountRightHand - m_sMagicAmountRightHand * total_r / 200;
		damage = damage + temp_damage;
	}

	total_r = 0 ;		// Reset all temporary data.
	temp_damage = 0 ;

	if (m_bMagicTypeLeftHand > 4 && m_bMagicTypeLeftHand < 8) {
		temp_damage = damage * m_sMagicAmountLeftHand / 100;
	}

	switch (m_bMagicTypeLeftHand) {	// LEFT HAND!!!
		case 1 :	// Fire Damage
			total_r = pNpc->m_byFireR;
			break;
		case 2 :	// Ice Damage
			total_r = pNpc->m_byColdR;
			break;
		case 3 :	// Lightning Damage
			total_r = pNpc->m_byLightningR;
			break;
		case 4 :	// Poison Damage
			total_r = pNpc->m_byPoisonR;
			break;
		case 5 :	// HP Drain					
			break;
		case 6 :	// MP Damage		
			pNpc->MSpChange(2, -temp_damage);
			break;
		case 7 :	// MP Drain		
			break;
		case 0:
			break;
	}

	if (m_bMagicTypeLeftHand > 0 && m_bMagicTypeLeftHand < 5) {
		if (total_r > 200) total_r = 200;
		temp_damage = m_sMagicAmountLeftHand - m_sMagicAmountLeftHand * total_r / 200;
		damage = damage + temp_damage;
	}

	return damage;
}

uint8 CUser::GetHitRate(float rate)
{
	int random = myrand(1, 10000);
	if (rate >= 5.0f)
	{
		if (random >= 1 && random <= 3500)
			return GREAT_SUCCESS;
		else if (random >= 3501 && random <= 7500)
			return SUCCESS;
		else if (random >= 7501 && random <= 9800)
			return NORMAL;
	}
	else if (rate < 5.0f && rate >= 3.0f)
	{
		if (random >= 1 && random <= 2500)
			return GREAT_SUCCESS;
		else if (random >= 2501 && random <= 6000)
			return SUCCESS;
		else if (random >= 6001 && random <= 9600)
			return NORMAL;
	}
	else if (rate < 3.0f && rate >= 2.0f)
	{
		if (random >= 1 && random <= 2000)
			return GREAT_SUCCESS;
		else if (random >= 2001 && random <= 5000)
			return SUCCESS;
		else if (random >= 5001 && random <= 9400)
			return NORMAL;
	}
	else if (rate < 2.0f && rate >= 1.25f)
	{
		if (random >= 1 && random <= 1500)
			return GREAT_SUCCESS;
		else if (random >= 1501 && random <= 4000)
			return SUCCESS;
		else if (random >= 4001 && random <= 9200)
			return NORMAL;
	}
	else if (rate < 1.25f && rate >= 0.8f)
	{
		if (random >= 1 && random <= 1000)
			return GREAT_SUCCESS;
		else if (random >= 1001 && random <= 3000)
			return SUCCESS;
		else if (random >= 3001 && random <= 9000)
			return NORMAL;
	}	
	else if (rate < 0.8f && rate >= 0.5f)
	{
		if (random >= 1 && random <= 800)
			return GREAT_SUCCESS;
		else if (random >= 801 && random <= 2500)
			return SUCCESS;
		else if (random >= 2501 && random <= 8000)
			return NORMAL;
	}
	else if (rate < 0.5f && rate >= 0.33f)
	{
		if (random >= 1 && random <= 600)
			return GREAT_SUCCESS;
		else if (random >= 601 && random <= 2000)
			return SUCCESS;
		else if (random >= 2001 && random <= 7000)
			return NORMAL;
	}
	else if (rate < 0.33f && rate >= 0.2f)
	{
		if (random >= 1 && random <= 400)
			return GREAT_SUCCESS;
		else if (random >= 401 && random <= 1500)
			return SUCCESS;
		else if (random >= 1501 && random <= 6000)
			return NORMAL;
	}
	else
	{
		if (random >= 1 && random <= 200)
			return GREAT_SUCCESS;
		else if (random >= 201 && random <= 1000)
			return SUCCESS;
		else if (random >= 1001 && random <= 5000)
			return NORMAL;
	}
	
	return FAIL;
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
			fDX = m_curx + surround_fx[i]; 
			fDZ = m_curz + surround_fz[i]; 
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
	int region_x = (int)(m_curx / VIEW_DIST);
	int region_z = (int)(m_curz / VIEW_DIST);

	MAP* pMap = GetMap();
	if (pMap == NULL) return;
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
	if (pMap == NULL) return;
	// 자신의 region에 있는 NpcArray을 먼저 검색하여,, 가까운 거리에 Monster가 있는지를 판단..
	if(rx < 0 || rz < 0 || rx > pMap->GetXRegionMax() || rz > pMap->GetZRegionMax())	{
		TRACE("#### CUser-HealAreaCheck() Fail : [nid=%d, name=%s], nRX=%d, nRZ=%d #####\n", m_iUserId, m_strUserID, rx, rz);
		return;
	}

	float fRadius = 10.0f;				// 30m

	__Vector3 vStart, vEnd;
	CNpc* pNpc = NULL ;      // Pointer initialization!
	float fDis = 0.0f;
	vStart.Set(m_curx, (float)0, m_curz);
	int send_index=0, result = 1, count = 0; 

	pMap->m_lock.Acquire();
	CRegion *pRegion = &pMap->m_ppRegion[rx][rz];
	int total_mon = pRegion->m_RegionNpcArray.GetSize();
	int *pNpcIDList = new int[total_mon];
	foreach_stlmap (itr, pRegion->m_RegionNpcArray)
		pNpcIDList[count++] = *itr->second;
	pMap->m_lock.Release();

	for(int i = 0 ; i < total_mon; i++ ) {
		int nid = pNpcIDList[i];
		if( nid < NPC_BAND ) continue;
		pNpc = (CNpc*)g_pMain->m_arNpc.GetData(nid - NPC_BAND);

		if( pNpc != NULL && pNpc->m_NpcState != NPC_DEAD)	{
			if( m_bNation == pNpc->m_byGroup ) continue;
			vEnd.Set(pNpc->m_fCurX, pNpc->m_fCurY, pNpc->m_fCurZ); 
			fDis = pNpc->GetDistance(vStart, vEnd);

			if(fDis <= fRadius)	{	// NPC가 반경안에 있을 경우...
				pNpc->ChangeTarget(1004, this);
			}	
		}
	}

	if (pNpcIDList)
		delete [] pNpcIDList;
}