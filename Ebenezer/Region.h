#pragma once

#include <set>
#include "define.h"
#include "GameDefine.h"
#include "../shared/STLMap.h"

typedef CSTLMap <_ZONE_ITEM>	ZoneItemArray;
typedef std::set<uint16>		ZoneUserArray;
typedef std::set<uint16>		ZoneNpcArray;

class CNpc;
class CUser;

class CRegion  
{
public:
	ZoneItemArray	m_RegionItemArray;
	ZoneUserArray	m_RegionUserArray;
	ZoneNpcArray	m_RegionNpcArray;

	void Add(CUser * pUser);
	void Remove(CUser * pUser);
	void Add(CNpc * pNpc);
	void Remove(CNpc * pNpc);
};
