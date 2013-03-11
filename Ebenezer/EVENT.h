#pragma once

#include "EVENT_DATA.h"
#include "../shared/STLMap.h"

typedef CSTLMap <EVENT_DATA>				EventDataArray;

class EVENT  
{
public:
	void DeleteAll();
	void Parsing(char *pBuf);
	void Init();
	BOOL LoadEvent(int zone);
	int m_Zone;

	EventDataArray m_arEvent;

	EVENT();
	virtual ~EVENT();

};
