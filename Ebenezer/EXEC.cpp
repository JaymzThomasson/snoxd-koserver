#include "stdafx.h"

EXEC::EXEC()
{
	memset(&m_ExecInt, -1, sizeof(m_ExecInt));
}

void EXEC::Parse(char *pBuf)
{
	int index = 0, i = 0;
	char temp[1024];

	index += ParseSpace(temp, pBuf+index);

	if (!strcmp(temp, "SAY"))
	{
		m_Exec = EXEC_SAY;
		PARSE_ARGUMENTS(10, temp, pBuf, m_ExecInt, i, index);
	}
	else if (!strcmp(temp, "SELECT_MSG"))
	{
		m_Exec = EXEC_SELECT_MSG;
		PARSE_ARGUMENTS(20, temp, pBuf, m_ExecInt, i, index);
	}
	else if (!strcmp(temp, "RUN_EVENT"))
	{
		m_Exec = EXEC_RUN_EVENT;
		PARSE_ARGUMENTS(1, temp, pBuf, m_ExecInt, i, index);
	}
	else if (!strcmp(temp, "GIVE_ITEM"))
	{
		m_Exec = EXEC_GIVE_ITEM;
		PARSE_ARGUMENTS(2, temp, pBuf, m_ExecInt, i, index);
	}
	else if (!strcmp(temp, "ROB_ITEM"))
	{
		m_Exec = EXEC_ROB_ITEM;
		PARSE_ARGUMENTS(2, temp, pBuf, m_ExecInt, i, index);
	}
	else if (!strcmp(temp, "GIVE_NOAH")) 
	{
		m_Exec = EXEC_GIVE_NOAH;
		PARSE_ARGUMENTS(1, temp, pBuf, m_ExecInt, i, index);
	}
	else if (!strcmp(temp, "RETURN"))
	{
		m_Exec = EXEC_RETURN;
	}
}