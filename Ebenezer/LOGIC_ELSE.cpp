#include "stdafx.h"

LOGIC_ELSE::LOGIC_ELSE() : m_bAnd(TRUE)
{
	memset(&m_LogicElseInt, -1, sizeof(m_LogicElseInt));
}

void LOGIC_ELSE::Parse_and(char *pBuf)
{
	int index = 0, i = 0;
	char temp[1024];

	index += ParseSpace(temp, pBuf+index);

	if (!strcmp(temp, "CHECK_UNDER_WEIGHT"))
	{
		m_LogicElse = LOGIC_CHECK_UNDER_WEIGHT;
		PARSE_ARGUMENTS(1, temp, pBuf, m_LogicElseInt, i, index);
	}
	else if (!strcmp(temp, "CHECK_OVER_WEIGHT"))
	{
		m_LogicElse = LOGIC_CHECK_OVER_WEIGHT;
		PARSE_ARGUMENTS(1, temp, pBuf, m_LogicElseInt, i, index);
	}
	else if (!strcmp(temp, "CHECK_SKILL_POINT"))
	{
		m_LogicElse = LOGIC_CHECK_SKILL_POINT; // Point type | Min | Max
		PARSE_ARGUMENTS(3, temp, pBuf, m_LogicElseInt, i, index);
	}
	else if (!strcmp(temp, "CHECK_EXIST_ITEM"))
	{
		m_LogicElse = LOGIC_EXIST_ITEM; // Item ID | Item count
		PARSE_ARGUMENTS(2, temp, pBuf, m_LogicElseInt, i, index);
	}
	else if (!strcmp(temp, "CHECK_CLASS"))
	{
		m_LogicElse = LOGIC_CHECK_CLASS;
		PARSE_ARGUMENTS(6, temp, pBuf, m_LogicElseInt, i, index);
	}
	else if (!strcmp(temp, "CHECK_WEIGHT"))
	{
		m_LogicElse = LOGIC_CHECK_WEIGHT;
		PARSE_ARGUMENTS(2, temp, pBuf, m_LogicElseInt, i, index);
	}
	else if (!strcmp(temp, "RAND"))
	{
		m_LogicElse = LOGIC_RAND;
		PARSE_ARGUMENTS(1, temp, pBuf, m_LogicElseInt, i, index);
	}

	m_bAnd = TRUE;
}