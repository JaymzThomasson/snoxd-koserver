#pragma once

extern BOOL	g_bNpcExit;

struct	_PARTY_GROUP
{
	WORD wIndex;
	short uid[8];		// 하나의 파티에 8명까지 가입가능
	_PARTY_GROUP() {
		for(int i=0;i<8;i++)
			uid[i] = -1;
	};
};

struct _MAKE_WEAPON
{
	BYTE	byIndex;		// 몹의 레벨 기준
	short	sClass[MAX_UPGRADE_WEAPON];		// 1차무기 확률
	_MAKE_WEAPON() {
		for(int i=0;i<MAX_UPGRADE_WEAPON;i++)
			sClass[i] = 0;
	};
};

struct _MAKE_ITEM_GRADE_CODE
{
	BYTE	byItemIndex;		// item grade
	short	sGrade[9];
};	

struct _MAKE_ITEM_LARE_CODE
{
	BYTE	byItemLevel;			// item level 판단 
	short	sLareItem;				// lareitem 나올 확률
	short	sMagicItem;				// magicitem 나올 확률
	short	sGereralItem;			// gereralitem 나올 확률
};

struct _MAGIC_TABLE
{
	long	iNum;
	BYTE	bBeforeAction;
	BYTE	bTargetAction;
	BYTE	bSelfEffect;
	BYTE	bFlyingEffect;
	int		iTargetEffect;
	BYTE	bMoral;
	int		sSkillLevel;	
	int		sSkill;
	int		sMsp;
	int		sHP;
	BYTE	bItemGroup;
	long	iUseItem;
	BYTE	bCastTime;
	int		sReCastTime;
	BYTE	bSuccessRate;
	BYTE	bType[2];
	int		sRange;
	BYTE	bEtc;
};

struct _MAGIC_TYPE4
{
	long    iNum;
	BYTE    bBuffType;
	BYTE    bRadius;
	int		sDuration;
	BYTE    bAttackSpeed;
	BYTE    bSpeed;
	int		sAC;
	int		sACPct;
	BYTE    bAttack;
	BYTE	bMagicAttack;
	int		sMaxHP;
	int		sMaxHPPct;
	int		sMaxMP;
	int		sMaxMPPct;
	BYTE    bHitRate;
	int		sAvoidRate;
	BYTE    bStr;
	BYTE    bSta;
	BYTE    bDex;
	BYTE    bIntel;
	BYTE    bCha;
	BYTE    bFireR;
	BYTE    bColdR;
	BYTE    bLightningR;
	BYTE    bMagicR;
	BYTE    bDiseaseR;
	BYTE    bPoisonR;
	BYTE	bExpPct;
};

struct _MAGIC_TYPE3
{
	long	iNum;
	BYTE	bRadius;
	int		sAngle;
	int		sFirstDamage;
	int		sEndDamage;
	int		sTimeDamage;
	BYTE	bDirectType;
	BYTE	bDuration;
	BYTE	bAttribute;
};

struct _MAGIC_TYPE2
{
	long    iNum;
	BYTE    bHitType;
	int		sHitRate;
	int		sAddDamage;
	int		sAddRange;
	BYTE    bNeedArrow;
};

struct _MAGIC_TYPE1
{
	long	iNum;
	BYTE	bHitType;
	int		sHitRate;
	int		sHit;
	BYTE	bDelay;
	BYTE	bComboType;
	BYTE	bComboCount;
	int		sComboDamage;
	int		sRange;
};