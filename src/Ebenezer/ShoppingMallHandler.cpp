#include "stdafx.h"
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

// We're closing the PUS so that we can call LOAD_WEB_ITEMMALL and load the extra items.
void CUser::HandleStoreClose()
{
	Packet result(WIZ_SHOPPING_MALL, uint8(STORE_CLOSE));
	m_bStoreOpen = false;
	g_pMain->AddDatabaseRequest(result, this);
}

void CUser::ReqLoadWebItemMall()
{
	Packet result(WIZ_SHOPPING_MALL, uint8(STORE_CLOSE));
	std::vector<_ITEM_DATA> itemList;

	if (g_DBAgent.LoadWebItemMall(itemList, this))
		return;

	// reuse the GiveItem() method for giving them the item, just don't send the packet
	// as it's handled by STORE_CLOSE.
	foreach (itr, itemList)
		GiveItem(itr->nNum, itr->sCount, false); 

	SendItemWeight();

	for (int i = 0; i < INVENTORY_TOTAL; i++)
	{
		_ITEM_DATA * pItem = &m_sItemArray[i];
		result	<< pItem->nNum
				<< pItem->sDuration
				<< pItem->sCount
				<< pItem->bFlag // item type flag (e.g. rented)
				<< pItem->sRemainingRentalTime // remaining time
				<< uint32(0) // unknown
				<< pItem->nExpirationTime; // expiration date
	}

	Send(&result);
}