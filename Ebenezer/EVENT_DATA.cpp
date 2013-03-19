#include "stdafx.h"

EVENT_DATA::EVENT_DATA()
{
}

EVENT_DATA::~EVENT_DATA()
{
	EXEC* pExec = NULL;
	LOGIC_ELSE* pLogicElse = NULL;

	while(m_arExec.size()) {
		pExec = m_arExec.front();
		if( pExec )
			delete pExec;
		m_arExec.pop_front();
	}
	m_arExec.clear();

	while(m_arLogicElse.size()) {
		pLogicElse = m_arLogicElse.front();
		if( pLogicElse )
			delete pLogicElse;
		m_arLogicElse.pop_front();
	}
	m_arLogicElse.clear();
}
