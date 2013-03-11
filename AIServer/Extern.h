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
	uint8	byIndex;
	uint16	sClass[MAX_UPGRADE_WEAPON];
	_MAKE_WEAPON() { memset(&sClass, 0, sizeof(sClass)); }
};

struct _MAKE_ITEM_GRADE_CODE
{
	uint8	byItemIndex;		// item grade
	uint16	sGrade[9];
};	

struct _MAKE_ITEM_LARE_CODE
{
	uint8	byItemLevel;
	uint16	sLareItem;
	uint16	sMagicItem;
	uint16	sGeneralItem;
};

#include "../shared/database/structs.h"