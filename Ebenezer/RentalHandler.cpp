#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"

void CUser::RentalSystem(Packet & pkt)
{
	uint8 opcode = pkt.read<uint8>();

	// TO-DO
	/*if (opcode == 1)
		PremiumRentalProcess(pBuf + index);
	else if (opcode == 2)
		PvPRentalProcess(pBuf + index);*/
}

