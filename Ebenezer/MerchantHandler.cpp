#include "StdAfx.h"

enum MerchantOpenResponseCodes
{
	MERCHANT_OPEN_SUCCESS = 1,
	MERCHANT_OPEN_NO_SESSION = -1,
	MERCHANT_OPEN_DEAD = -2,
	MERCHANT_OPEN_TRADING = -3,
	MERCHANT_OPEN_MERCHANTING = -4,
	MERCHANT_OPEN_INVALID_ZONE = -5,
	MERCHANT_OPEN_SHOPPING = -6,
	MERCHANT_OPEN_UNDERLEVELED = 30
};

void CUser::MerchantProcess(Packet & pkt)
{
	uint8 opcode = pkt.read<uint8>();
	switch (opcode)
	{
		// Regular merchants
		case MERCHANT_OPEN: 
			MerchantOpen(); 
			break;

		case MERCHANT_CLOSE: 
			MerchantClose(); 
			break;

		case MERCHANT_ITEM_ADD: 
			MerchantItemAdd(pkt); 
			break;

		case MERCHANT_ITEM_CANCEL: 
			MerchantItemCancel(pkt); 
			break;

		case MERCHANT_ITEM_LIST: 
			MerchantItemList(pkt); 
			break;

		case MERCHANT_ITEM_BUY: 
			MerchantItemBuy(pkt); 
			break;

		case MERCHANT_INSERT: 
			MerchantInsert(pkt); 
			break;

		case MERCHANT_TRADE_CANCEL: 
			CancelMerchant(); 
			break;

#if __VERSION >= 1700
		// Buying merchants
		case MERCHANT_BUY_OPEN: 
			BuyingMerchantOpen(pkt); 
			break;

		case MERCHANT_BUY_CLOSE: 
			BuyingMerchantClose(); 
			break;

		case MERCHANT_BUY_LIST: 
			BuyingMerchantList(pkt); 
			break;

		case MERCHANT_BUY_INSERT: 
			BuyingMerchantInsert(pkt); 
			break;

		case MERCHANT_BUY_BUY: // seeya!
			BuyingMerchantBuy(pkt); 
			break;
#endif
	}
}

/*
	Regular merchants
*/
void CUser::MerchantOpen()
{
	int16 errorCode = 0;
	if (isDead())
		errorCode = MERCHANT_OPEN_DEAD;
	else if (isStoreOpen())
		errorCode = MERCHANT_OPEN_SHOPPING;
	else if (isTrading())
		errorCode = MERCHANT_OPEN_TRADING;
	else if (getZoneID() > 21 || getZoneID() <= ELMORAD)
		errorCode = MERCHANT_OPEN_INVALID_ZONE;
	else if (getLevel() < 30)
		errorCode = MERCHANT_OPEN_UNDERLEVELED;
	else if (isMerchanting())
		errorCode = MERCHANT_OPEN_MERCHANTING;
	else 
		errorCode = MERCHANT_OPEN_SUCCESS;

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_OPEN));
	result << errorCode;
	Send(&result);

	// If we're already merchanting, user may be desynced
	// so we need to close our current merchant first.
	if (errorCode == MERCHANT_OPEN_MERCHANTING)
		MerchantClose();
}

void CUser::MerchantClose()
{
	if (!isMerchanting())
		return;

	m_bIsMerchanting = false;
	GiveMerchantItems(); // Give back to the user that which hasn't been sold, if any.
	Packet result(WIZ_MERCHANT, uint8(MERCHANT_CLOSE));
	result << GetSocketID();
	SendToRegion(&result);
}

void CUser::MerchantItemAdd(Packet & pkt)
{
	uint32 nGold, nItemID;
	uint16 sCount;
	uint8 bSrcPos, // It sends the "actual" inventory slot (SLOT_MAX -> INVENTORY_MAX-SLOT_MAX), so need to allow for it. 
		bDstPos, 
		bMode; // Might be a flag for normal / "premium" merchant mode, once skills are implemented take another look at this.


	pkt >> nItemID >> sCount >> nGold >> bSrcPos >> bDstPos >> bMode;
	if (bSrcPos >= HAVE_MAX
		|| bDstPos >= MAX_MERCH_ITEMS)
		return;

	bSrcPos += SLOT_MAX;
	if (m_pUserData->m_sItemArray[bSrcPos].nNum != nItemID
		|| m_pUserData->m_sItemArray[bSrcPos].sCount < sCount)
		return;

	m_arSellingItems[bDstPos].nNum = nItemID;
	m_arSellingItems[bDstPos].nPrice = nGold;
	m_arSellingItems[bDstPos].sCount = sCount;
	m_arSellingItems[bDstPos].sDuration = m_pUserData->m_sItemArray[bSrcPos].sDuration;
	m_arSellingItems[bDstPos].nSerialNum = m_pUserData->m_sItemArray[bSrcPos].nSerialNum; // NOTE: Stackable items will have an issue with this.
	m_arSellingItems[bDstPos].bOriginalSlot = bSrcPos;

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_ITEM_ADD));
	result	<< uint16(1)
			<< nItemID << sCount << m_arSellingItems[bDstPos].sDuration << nGold 
			<< bSrcPos << bDstPos;
	Send(&result);
}

void CUser::MerchantItemCancel(Packet & pkt)
{
	Packet result(WIZ_MERCHANT, uint8(MERCHANT_ITEM_CANCEL));
	uint8 bSrcPos = pkt.read<uint8>();

/*	if (this == NULL)
		result << int16(-1);*/
	// Invalid source position
	if (bSrcPos >= MAX_MERCH_ITEMS)
		result << int16(-2);
	// There's no item in that list..?
	else if (m_arSellingItems[bSrcPos].nNum == 0)
		result << int16(-3);
	// Check to make sure we've got a valid stack
	else if (m_arSellingItems[bSrcPos].sCount + m_pUserData->m_sItemArray[m_arSellingItems[bSrcPos].bOriginalSlot].sCount > HAVE_MAX) 
		result << int16(-3); // custom error
	else
	{
		// NOTE: As we don't remove the item from the inventory (I think we should, though), we can just unset the item in the selling list.
		memset(&m_arSellingItems[bSrcPos], 0, sizeof(_MERCH_DATA));
		result << int16(1) << bSrcPos;
	}

	Send(&result);
}

void CUser::MerchantItemList(Packet & pkt)
{
}

void CUser::MerchantItemBuy(Packet & pkt)
{
}

void CUser::MerchantInsert(Packet & pkt)
{
	std::string advertMessage;
	pkt >> advertMessage;
	if (advertMessage.empty() || advertMessage.size() >= MAX_MERCH_MESSAGE)
		return;

	m_bIsMerchanting = true;
	TakeMerchantItems(); // Removing the items from the user's inventory

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_INSERT));
	result << uint16(1) << advertMessage << GetSocketID()
		<< uint8(0); // 0 is for "normal" merchant mode, 1 for "premium" merchant mode. Send "normal" until we have support for the skills.

	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
		result << uint32(m_arSellingItems[i].nNum);

	Send(&result);
}

void CUser::TakeMerchantItems()
{
	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
		memset(&m_pUserData->m_sItemArray[m_arSellingItems[i].bOriginalSlot], 0, sizeof(_ITEM_DATA));
}

void CUser::GiveMerchantItems()
{
	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
	{
		uint8 bOriginalSlot = m_arSellingItems[i].bOriginalSlot;
		m_pUserData->m_sItemArray[bOriginalSlot].nNum = m_arSellingItems[i].nNum;
		m_pUserData->m_sItemArray[bOriginalSlot].nSerialNum = m_arSellingItems[i].nSerialNum;
		m_pUserData->m_sItemArray[bOriginalSlot].sCount = m_arSellingItems[i].sCount;
		m_pUserData->m_sItemArray[bOriginalSlot].sDuration = m_arSellingItems[i].sDuration;
	}
}

void CUser::CancelMerchant()
{
}

/*
	Buying merchants: 1.7XX only
*/
void CUser::BuyingMerchantOpen(Packet & pkt)
{
}

void CUser::BuyingMerchantClose()
{
}

void CUser::BuyingMerchantInsert(Packet & pkt)
{
}

void CUser::BuyingMerchantList(Packet & pkt)
{
}

void CUser::BuyingMerchantBuy(Packet & pkt)
{
}