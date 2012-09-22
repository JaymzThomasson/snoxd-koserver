#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"
void CUser::RentalSystem(char *pBuf)
{
	int index = 0;
	BYTE opcode = GetByte(pBuf, index);

	// TO-DO
	/*if (opcode == 1)
		PremiumRentalProcess(pBuf + index);
	else if (opcode == 2)
		PvPRentalProcess(pBuf + index);*/
}

