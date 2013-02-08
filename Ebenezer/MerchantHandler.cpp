#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"

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

void CUser::MerchantProcess(char *pBuf)
{
	int index = 0;
	BYTE subcommand = GetByte(pBuf, index);

	switch (subcommand)
	{
		// Regular merchants
		case MERCHANT_OPEN: 
			MerchantOpen(pBuf+index); 
			break;

		case MERCHANT_CLOSE: 
			MerchantClose(); 
			break;

		case MERCHANT_ITEM_ADD: 
			MerchantItemAdd(pBuf+index); 
			break;

		case MERCHANT_ITEM_CANCEL: 
			MerchantItemCancel(pBuf+index); 
			break;

		case MERCHANT_ITEM_LIST: 
			MerchantItemList(pBuf+index); 
			break;

		case MERCHANT_ITEM_BUY: 
			MerchantItemBuy(pBuf+index); 
			break;

		case MERCHANT_INSERT: 
			MerchantInsert(pBuf+index); 
			break;

		case MERCHANT_TRADE_CANCEL: 
			CancelMerchant(); 
			break;

#if __VERSION >= 1700
		// Buying merchants
		case MERCHANT_BUY_OPEN: 
			BuyingMerchantOpen(pBuf+index); 
			break;

		case MERCHANT_BUY_CLOSE: 
			BuyingMerchantClose(); 
			break;

		case MERCHANT_BUY_LIST: 
			BuyingMerchantList(pBuf+index); 
			break;

		case MERCHANT_BUY_INSERT: 
			BuyingMerchantInsert(pBuf+index); 
			break;

		case MERCHANT_BUY_BUY: // seeya!
			BuyingMerchantBuy(pBuf+index); 
			break;
#endif
	}
}

/*
	Regular merchants
*/
void CUser::MerchantOpen(char *pBuf)
{
	Packet result(WIZ_MERCHANT);
	result << uint8(MERCHANT_OPEN);

		if( isDead() ) {
			result << uint16(MERCHANT_OPEN_DEAD);
			Send(&result);
			return;
		}

		if( isStoreOpen() ) {
			result << uint16(MERCHANT_OPEN_SHOPPING);
			Send(&result);
			return;
		}

		if( isTrading() ) {
			result << uint16(MERCHANT_OPEN_TRADING);
			Send(&result);
			return;
		}

		if( getZoneID() > 21 || getZoneID() < 1 ) {
			result << uint16(MERCHANT_OPEN_INVALID_ZONE);
			Send(&result);
			return;
		}

		if( getLevel() < 30 ) {
			result << uint16(MERCHANT_OPEN_UNDERLEVELED);
			Send(&result);
			return;
		}

		if (isMerchanting())
		{
			MerchantClose(); //Close the current merchant session first before allowing a new one.
			result << uint16(0);
			Send(&result);
			return;
		}
		
	result << uint16(MERCHANT_OPEN_SUCCESS);
	Send(&result);
}

void CUser::MerchantClose()
{
	if (!isMerchanting())
		return;

	m_bIsMerchanting = false;
	GiveMerchantItems(); // Give back to the user that which hasn't been sold, if any.
	Packet result(WIZ_MERCHANT, uint8(MERCHANT_CLOSE));
	result << uint16(GetSocketID());
	SendToRegion(&result);
}

void CUser::MerchantItemAdd(char *pBuf)
{
	int index = 0;
	uint32 nGold, nItemID;
	uint16 sCount = 0, sDuration = 0;
	uint8 bInventorySlot = 0, bSellingSlot = 0, bMode = 0;

	nItemID = GetDWORD(pBuf, index);
	sCount = GetShort(pBuf, index);
	nGold = GetDWORD(pBuf, index);
	bInventorySlot = GetByte(pBuf, index); // It sends the "actual" inventory slot (SLOT_MAX -> INVENTORY_MAX-SLOT_MAX), so need to allow for it. 
	bSellingSlot = GetByte(pBuf, index);
	bMode = GetByte(pBuf, index); // Might be a flag for normal / "premium" merchant mode, once skills are implemented take another look at this.

	if (bInventorySlot >= HAVE_MAX
		|| bSellingSlot >= MAX_MERCH_ITEMS)
		return;

	bInventorySlot += SLOT_MAX;
	if (m_pUserData->m_sItemArray[bInventorySlot].nNum != nItemID
		|| m_pUserData->m_sItemArray[bInventorySlot].sCount < sCount)
		return;

	m_arSellingItems[bSellingSlot].nNum = nItemID;
	m_arSellingItems[bSellingSlot].nPrice = nGold;
	m_arSellingItems[bSellingSlot].sCount = sCount;
	m_arSellingItems[bSellingSlot].sDuration = m_pUserData->m_sItemArray[bInventorySlot].sDuration;
	m_arSellingItems[bSellingSlot].nSerialNum = m_pUserData->m_sItemArray[bInventorySlot].nSerialNum; // NOTE: Stackable items will have an issue with this.
	m_arSellingItems[bSellingSlot].bOriginalSlot = bInventorySlot;

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_ITEM_ADD));
	result	<< uint16(1)
			<< nItemID << sCount << sDuration << nGold 
			<< bInventorySlot << bSellingSlot;
	Send(&result);
}

void CUser::MerchantItemCancel(char *pBuf)
{
	Packet result(WIZ_MERCHANT, uint8(MERCHANT_ITEM_CANCEL));
	int index = 0;
	uint8 bSrcPos = GetByte(pBuf, index);

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

void CUser::MerchantItemList(char *pBuf)
{
}

void CUser::MerchantItemBuy(char *pBuf)
{
}

void CUser::MerchantInsert(char *pBuf)
{
	int index = 0;
	char mText[40];
	if (!GetKOString(mText, pBuf, index, sizeof(mText)))
		return;

	m_bIsMerchanting = true;
	TakeMerchantItems(); // Removing the items from the user's inventory

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_INSERT));
	result << uint16(1) << mText << uint16(GetSocketID())
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
void CUser::BuyingMerchantOpen(char *pBuf)
{
}

void CUser::BuyingMerchantClose()
{
}

void CUser::BuyingMerchantInsert(char *pBuf)
{
}

void CUser::BuyingMerchantList(char *pBuf)
{
}

void CUser::BuyingMerchantBuy(char *pBuf)
{
}