#pragma once

class EXEC  
{
public:
	EXEC();

	BYTE m_Exec;
	int m_ExecInt[MAX_EXEC_INT];

	void Parse(char* pBuf);
};
