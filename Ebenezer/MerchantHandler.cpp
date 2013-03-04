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
	else if (GetZoneID() > 21 || GetZoneID() <= ELMORAD)
		errorCode = MERCHANT_OPEN_INVALID_ZONE;
	else if (GetLevel() < 30)
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
	
	memset(&m_arSellingItems, 0, sizeof(m_arSellingItems));
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
	if (m_sMerchantsSocketID >= 0)
		RemoveFromMerchantLookers(); //This check should never be hit...
	
	uint16 uid;
	pkt >> uid;

	if (uid >= MAX_USER)
		return;

	CUser *pMerchantUser = g_pMain->GetUserPtr(uid);
	if(!pMerchantUser || !pMerchantUser->isMerchanting())
		return;

	m_sMerchantsSocketID = uid;
	pMerchantUser->m_arMerchantLookers.push_front(GetSocketID());

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_ITEM_LIST));
	result << uint16(1) << uint16(uid);
	for(int i = 0; i < MAX_MERCH_ITEMS; i++) {
		result << pMerchantUser->m_arSellingItems[i].nNum
		<< pMerchantUser->m_arSellingItems[i].sCount
		<< pMerchantUser->m_arSellingItems[i].sDuration
		<< pMerchantUser->m_arSellingItems[i].nPrice
		<< uint32(0); //Not sure what this one is, maybe serial?
	}
	Send(&result);
}

void CUser::MerchantItemBuy(Packet & pkt)
{
	uint32 itemid, req_gold;
	uint16 item_count, leftover_count;
	uint8 item_slot, dest_slot;
	CUser * m_MerchantUser = NULL;

	if (m_sMerchantsSocketID < 0 || m_sMerchantsSocketID > MAX_USER)
		return;

	m_MerchantUser = g_pMain->GetUserPtr(m_sMerchantsSocketID);
	if (m_MerchantUser == NULL)
		return;

	pkt >> itemid >> item_count >> item_slot >> dest_slot;

	if (item_slot < 0 
		|| item_slot > MAX_MERCH_ITEMS
		|| item_count == 0)
		return;

	if (m_MerchantUser->m_arSellingItems[item_slot].nNum != itemid
		||m_MerchantUser->m_arSellingItems[item_slot].sCount < item_count)
		return;

	req_gold = m_MerchantUser->m_arSellingItems[item_slot].nPrice * item_count;
	if (m_pUserData->m_iGold < req_gold)
		return;

	if (m_pUserData->m_sItemArray[SLOT_MAX+dest_slot].nNum != 0 && m_pUserData->m_sItemArray[SLOT_MAX+dest_slot].nNum != itemid)
		return;

	leftover_count = m_MerchantUser->m_arSellingItems[item_slot].sCount - item_count;
	m_MerchantUser->GoldChange(GetSocketID(), req_gold);
	m_pUserData->m_sItemArray[SLOT_MAX+dest_slot].nNum = itemid;
	m_pUserData->m_sItemArray[SLOT_MAX+dest_slot].sCount += item_count;
	m_pUserData->m_sItemArray[SLOT_MAX+dest_slot].sDuration = m_MerchantUser->m_arSellingItems[item_slot].sDuration;
	m_pUserData->m_sItemArray[SLOT_MAX+dest_slot].nSerialNum = m_MerchantUser->m_arSellingItems[item_slot].nSerialNum;
	//TO-DO : Proper checks for the removal of the items in the array, we're now assuming everything gets bought
	if(item_count == m_MerchantUser->m_arSellingItems[item_slot].sCount)
		memset(&m_MerchantUser->m_arSellingItems[item_slot], 0, sizeof(_MERCH_DATA)); //Remove the item from the arSellingItems array.
	else
		m_MerchantUser->m_arSellingItems[item_slot].sCount -= item_count;

	SetSlotItemValue();
	m_MerchantUser->SetSlotItemValue();

	SetUserAbility();
	m_MerchantUser->SetUserAbility();


	if (m_pUserData->m_sItemArray[SLOT_MAX+dest_slot].sCount == item_count)
		SendStackChange(itemid, m_pUserData->m_sItemArray[SLOT_MAX+dest_slot].sCount, m_pUserData->m_sItemArray[SLOT_MAX+dest_slot].sDuration, dest_slot, true);
	else
		SendStackChange(itemid, m_pUserData->m_sItemArray[SLOT_MAX+dest_slot].sCount, m_pUserData->m_sItemArray[SLOT_MAX+dest_slot].sDuration, dest_slot);

	m_MerchantUser->SendStackChange(itemid, leftover_count, m_MerchantUser->m_arSellingItems[item_slot].sDuration, m_MerchantUser->m_arSellingItems[item_slot].bOriginalSlot - SLOT_MAX);

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_ITEM_PURCHASED));
	result << itemid << m_pUserData->m_id;
	m_MerchantUser->Send(&result);

	result.clear();

	result	<< uint8(MERCHANT_ITEM_BUY) << uint16(1)
			<< itemid
			<< leftover_count
			<< item_slot << dest_slot;
	Send(&result);

	if(item_slot < 4 && leftover_count == 0)
	{
		result.Initialize(WIZ_MERCHANT_INOUT);
		result << uint8(2) << m_sMerchantsSocketID << uint16(item_slot);
		m_MerchantUser->SendToRegion(&result);
	}

	int n = 0;
	for(int i = 0; i < MAX_MERCH_ITEMS; i++)
		if(m_MerchantUser->m_arSellingItems[i].nNum == 0)
			n++;
	if(n == 0)
		MerchantClose();
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

	SendToRegion(&result);
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
	if (m_sMerchantsSocketID < 0)
		return;

	RemoveFromMerchantLookers();
	Packet result(WIZ_MERCHANT, uint8(MERCHANT_TRADE_CANCEL));
	result << uint16(1);
	Send(&result);
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

void CUser::RemoveFromMerchantLookers()
{
	CUser *pPreviousMerchantUser = g_pMain->GetUserPtr(m_sMerchantsSocketID);
	if (pPreviousMerchantUser == NULL)
		return;

	pPreviousMerchantUser->m_arMerchantLookers.remove(GetSocketID());
	m_sMerchantsSocketID = -1;
}