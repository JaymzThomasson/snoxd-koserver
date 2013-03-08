#pragma once

#include "../shared/STLMap.h"

typedef CSTLMap <int>			ZoneUserArray;
typedef CSTLMap <int>			ZoneNpcArray;

class CRegion  
{
public:
	ZoneUserArray	m_RegionUserArray;
	ZoneNpcArray	m_RegionNpcArray;
	BYTE	m_byMoving;			// move : 1, not moving : 0
};