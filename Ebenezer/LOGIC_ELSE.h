#pragma once

class LOGIC_ELSE  
{
public:
	void Parse_and(char* pBuf);

	BYTE m_LogicElse;
	BOOL m_bAnd;
	int m_LogicElseInt[MAX_LOGIC_ELSE_INT];

	LOGIC_ELSE();
};