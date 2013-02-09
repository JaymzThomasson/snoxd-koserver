#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"

void CUser::ShoppingMall(Packet & pkt)
{
	uint8 opcode = pkt.read<uint8>();
		
	switch (opcode)
	{
	case STORE_OPEN:
		TRACE("STORE_OPEN\n");
		break;

	case STORE_CLOSE:
		TRACE("STORE_CLOSE\n");
		HandleStoreClose();
		break;

	case STORE_BUY:
		TRACE("STORE_BUY\n");
		break;

	case STORE_MINI: // not sure what this is
		TRACE("STORE_MINI\n");
		break;

	case STORE_PROCESS:
		TRACE("STORE_PROCESS\n");
		break;

	case STORE_LETTER:
		TRACE("STORE_LETTER\n");
		LetterSystem(pkt);
		break;

	default:
		TRACE("Unknown shoppingmall packet: %X\n", opcode);
	}
}

// Tell Aujard we're closing the PUS so that we can call LOAD_WEB_ITEMMALL and load the extra items.
void CUser::HandleStoreClose()
{
	Packet result(WIZ_SHOPPING_MALL, uint8(STORE_CLOSE));
	m_bStoreOpen = false;
	result << uint16(GetSocketID());
	m_pMain->m_LoggerSendQueue.PutData(&result);
}

void CUser::RecvStore(char *pData)
{
	int index = 0;
	BYTE opcode = GetByte(pData, index);
	switch (opcode)
	{
	case STORE_CLOSE:
		RecvStoreClose(pData+index);
		break;
	}
}

// Presumably received item data back from Aujard.
void CUser::RecvStoreClose(char *pData)
{
	Packet result(WIZ_SHOPPING_MALL, uint8(STORE_CLOSE));
	int index = 0;
	uint8 bResult = GetByte(pData, index);

	// If it was succesful, i.e. it loaded data, give it to us
	if (bResult)
	{
		short count = GetShort(pData, index);
		for (int i = 0; i < count; i++)
		{
			int nItemID = GetDWORD(pData, index);
			short sCount = GetShort(pData, index);

			// reuse the GiveItem() method for giving them the item, just don't send that particular packet.
			GiveItem(nItemID, sCount, false); 
		}
	}

	// not sure if this limit's correct
	for (int i = 0; i < HAVE_MAX + SLOT_MAX + COSP_MAX + MBAG_MAX; i++)
	{
		result	<< m_pUserData->m_sItemArray[i].nNum
				<< m_pUserData->m_sItemArray[i].sDuration
				<< m_pUserData->m_sItemArray[i].sCount
				<< uint8(0) // item type flag (e.g. rented)
				<< uint16(0) // remaining time
				<< uint32(0) // unknown
				<< uint32(0); // expiration date
	}

	Send(&result);
}