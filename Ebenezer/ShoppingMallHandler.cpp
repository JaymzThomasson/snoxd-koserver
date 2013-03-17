#include "StdAfx.h"

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

// We're closing the PUS so that we can call LOAD_WEB_ITEMMALL and load the extra items.
void CUser::HandleStoreClose()
{
	Packet result(WIZ_SHOPPING_MALL, uint8(STORE_CLOSE));
	m_bStoreOpen = false;
	g_pMain.AddDatabaseRequest(result, this);
}

void CUser::RecvStore(Packet & pkt)
{
	uint8 opcode = pkt.read<uint8>();
	switch (opcode)
	{
	case STORE_CLOSE:
		RecvStoreClose(pkt);
		break;
	}
}

// Presumably received item data back from the database.
void CUser::RecvStoreClose(Packet & pkt)
{
	Packet result(WIZ_SHOPPING_MALL, uint8(STORE_CLOSE));
	uint8 bResult = pkt.read<uint8>();

	// If it was succesful, i.e. it loaded data, give it to us
	if (bResult)
	{
		uint16 count = pkt.read<uint16>();
		for (int i = 0; i < count; i++)
		{
			uint32 nItemID; uint16 sCount;
			pkt >> nItemID >> sCount;

			// reuse the GiveItem() method for giving them the item, just don't send that particular packet.
			GiveItem(nItemID, sCount, false); 
		}

		SendItemWeight();
	}

	for (int i = 0; i < INVENTORY_TOTAL; i++)
	{
		result	<< m_sItemArray[i].nNum
				<< m_sItemArray[i].sDuration
				<< m_sItemArray[i].sCount
				<< uint8(0) // item type flag (e.g. rented)
				<< uint16(0) // remaining time
				<< uint32(0) // unknown
				<< uint32(0); // expiration date
	}

	Send(&result);
}