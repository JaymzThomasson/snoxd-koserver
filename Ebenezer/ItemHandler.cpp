#include "StdAfx.h"
#include "EbenezerDlg.h"
#include "User.h"

void CUser::WarehouseProcess(Packet & pkt)
{
	Packet result(WIZ_WAREHOUSE);
	uint32 itemid, count;
	uint16 npcid, reference_pos;
	uint8 page, srcpos, destpos;
	_ITEM_TABLE* pTable = NULL;
	uint8 command = pkt.read<uint8>();
	bool bResult = false;

	if (isDead())
	{
		TRACE("### WarehouseProcess Fail : name=%s(%d), m_bResHpType=%d, hp=%d, x=%d, z=%d ###\n", GetName(), GetSocketID(), m_bResHpType, m_sHp, (int)m_curx, (int)m_curz);
		return;
	}
	if (isTrading())
		goto fail_return;

	if (command == WAREHOUSE_OPEN)
	{
		result << uint8(WAREHOUSE_OPEN) << uint8(WAREHOUSE_OPEN) << GetInnCoins();
		for (int i = 0; i < WAREHOUSE_MAX; i++)
		{
			_ITEM_DATA *pItem = &m_sWarehouseArray[i];
			result	<< pItem->nNum << pItem->sDuration << pItem->sCount
					<< uint8(0) << uint16(0) << uint16(0) << uint16(0) << uint16(0);
		}
		Send(&result);
		return;
	}

	pkt >> npcid >> itemid >> page >> srcpos >> destpos;
	CNpc * pNpc = g_pMain->m_arNpcArray.GetData(npcid);
	if (pNpc == NULL
		|| pNpc->GetType() != NPC_WAREHOUSE
		|| !isInRange(pNpc, MAX_NPC_RANGE))
		goto fail_return;

	pTable = g_pMain->GetItemPtr( itemid );
	if( !pTable ) goto fail_return;
	reference_pos = 24 * page;

	// TO-DO: Clean up this entire method. It's horrendous!
	switch (command)
	{
	/* stuff going into the inn */
	case WAREHOUSE_INPUT:
		pkt >> count;

		// Handle coin input.
		if (itemid == ITEM_GOLD)
		{
			if (!hasCoins(count)
				|| GetInnCoins() + count > COIN_MAX)
				goto fail_return;

			m_iBank += count;
			m_iGold -= count;
			break;
		}

		if (srcpos > HAVE_MAX
			|| reference_pos + destpos > WAREHOUSE_MAX
			|| itemid >= ITEM_NO_TRADE) // Cannot be traded, sold or stored (note: don't check the race, as these items CAN be stored).
			goto fail_return;

		if (m_sItemArray[SLOT_MAX+srcpos].isSealed()
			|| m_sItemArray[SLOT_MAX+srcpos].isRented())
			goto fail_return;

		if( m_sItemArray[SLOT_MAX+srcpos].nNum != itemid ) goto fail_return;
		if( m_sWarehouseArray[reference_pos+destpos].nNum && !pTable->m_bCountable ) goto fail_return;
		if( m_sItemArray[SLOT_MAX+srcpos].sCount < count ) goto fail_return;
		m_sWarehouseArray[reference_pos+destpos].nNum = itemid;
		m_sWarehouseArray[reference_pos+destpos].sDuration = m_sItemArray[SLOT_MAX+srcpos].sDuration;
		m_sWarehouseArray[reference_pos+destpos].nSerialNum = m_sItemArray[SLOT_MAX+srcpos].nSerialNum;
		if( pTable->m_bCountable == 0 && m_sWarehouseArray[reference_pos+destpos].nSerialNum == 0 )
			m_sWarehouseArray[reference_pos+destpos].nSerialNum = g_pMain->GenerateItemSerial();

		if( pTable->m_bCountable ) {
			m_sWarehouseArray[reference_pos+destpos].sCount += (unsigned short)count;
		}
		else {
			m_sWarehouseArray[reference_pos+destpos].sCount = m_sItemArray[SLOT_MAX+srcpos].sCount;
		}

		if( !pTable->m_bCountable ) {
			m_sItemArray[SLOT_MAX+srcpos].nNum = 0;
			m_sItemArray[SLOT_MAX+srcpos].sDuration = 0;
			m_sItemArray[SLOT_MAX+srcpos].sCount = 0;
			m_sItemArray[SLOT_MAX+srcpos].nSerialNum = 0;
		}
		else {
			m_sItemArray[SLOT_MAX+srcpos].sCount -= (unsigned short)count;
			if( m_sItemArray[SLOT_MAX+srcpos].sCount <= 0 ) {
				m_sItemArray[SLOT_MAX+srcpos].nNum = 0;
				m_sItemArray[SLOT_MAX+srcpos].sDuration = 0;
				m_sItemArray[SLOT_MAX+srcpos].sCount = 0;
				m_sItemArray[SLOT_MAX+srcpos].nSerialNum = 0;
			}
		}

		SendItemWeight();
		break;

	/* stuff being taken out of the inn */
	case WAREHOUSE_OUTPUT:
		pkt >> count;

		if (itemid == ITEM_GOLD)
		{
			if (!hasInnCoins(count)
				|| GetCoins() + count > COIN_MAX)
				goto fail_return;

			m_iGold += count;
			m_iBank -= count;
			break;
		}

		if (reference_pos + srcpos > WAREHOUSE_MAX
			|| destpos > HAVE_MAX)
			goto fail_return;

		if (pTable->m_bCountable) {	// Check weight of countable item.
			if (((pTable->m_sWeight * count)   + m_sItemWeight) > m_sMaxWeight) {			
				goto fail_return;
			}
		}
		else {	// Check weight of non-countable item.
			if ((pTable->m_sWeight + m_sItemWeight) > m_sMaxWeight) {
				goto fail_return;
			}
		}		

		if( m_sWarehouseArray[reference_pos+srcpos].nNum != itemid ) goto fail_return;
		if( m_sItemArray[SLOT_MAX+destpos].nNum && !pTable->m_bCountable ) goto fail_return;
		if( m_sWarehouseArray[reference_pos+srcpos].sCount < count ) goto fail_return;
		m_sItemArray[SLOT_MAX+destpos].nNum = itemid;
		m_sItemArray[SLOT_MAX+destpos].sDuration = m_sWarehouseArray[reference_pos+srcpos].sDuration;
		m_sItemArray[SLOT_MAX+destpos].nSerialNum = m_sWarehouseArray[reference_pos+srcpos].nSerialNum;
		if( pTable->m_bCountable )
			m_sItemArray[SLOT_MAX+destpos].sCount += (unsigned short)count;
		else {
			if( m_sItemArray[SLOT_MAX+destpos].nSerialNum == 0 )
				m_sItemArray[SLOT_MAX+destpos].nSerialNum = g_pMain->GenerateItemSerial();
			m_sItemArray[SLOT_MAX+destpos].sCount = m_sWarehouseArray[reference_pos+srcpos].sCount;
		}
		if( !pTable->m_bCountable ) {
			m_sWarehouseArray[reference_pos+srcpos].nNum = 0;
			m_sWarehouseArray[reference_pos+srcpos].sDuration = 0;
			m_sWarehouseArray[reference_pos+srcpos].sCount = 0;
			m_sWarehouseArray[reference_pos+srcpos].nSerialNum = 0;
		}
		else {
			m_sWarehouseArray[reference_pos+srcpos].sCount -= (unsigned short)count;
			if( m_sWarehouseArray[reference_pos+srcpos].sCount <= 0 ) {
				m_sWarehouseArray[reference_pos+srcpos].nNum = 0;
				m_sWarehouseArray[reference_pos+srcpos].sDuration = 0;
				m_sWarehouseArray[reference_pos+srcpos].sCount = 0;
				m_sWarehouseArray[reference_pos+srcpos].nSerialNum = 0;
			}
		}

		SendItemWeight();		
		//TRACE("WARE OUTPUT : %s %s %d %d %d %d %d", m_id, m_Accountid, ITEM_WAREHOUSE_GET, 0, itemid, count, m_sItemArray[SLOT_MAX+destpos].sDuration );
		break;
	case WAREHOUSE_MOVE:
		if( reference_pos+srcpos > WAREHOUSE_MAX ) goto fail_return;
		if( m_sWarehouseArray[reference_pos+srcpos].nNum != itemid ) goto fail_return;
		if( m_sWarehouseArray[reference_pos+destpos].nNum ) goto fail_return;
		m_sWarehouseArray[reference_pos+destpos].nNum = itemid;
		m_sWarehouseArray[reference_pos+destpos].sDuration = m_sWarehouseArray[reference_pos+srcpos].sDuration;
		m_sWarehouseArray[reference_pos+destpos].sCount = m_sWarehouseArray[reference_pos+srcpos].sCount;
		m_sWarehouseArray[reference_pos+destpos].nSerialNum = m_sWarehouseArray[reference_pos+srcpos].nSerialNum;

		m_sWarehouseArray[reference_pos+srcpos].nNum = 0;
		m_sWarehouseArray[reference_pos+srcpos].sDuration = 0;
		m_sWarehouseArray[reference_pos+srcpos].sCount = 0;
		m_sWarehouseArray[reference_pos+srcpos].nSerialNum = 0;
		break;
	case WAREHOUSE_INVENMOVE:
		if( itemid != m_sItemArray[SLOT_MAX+srcpos].nNum )
			goto fail_return;
		{
			short duration = m_sItemArray[SLOT_MAX+srcpos].sDuration;
			short itemcount = m_sItemArray[SLOT_MAX+srcpos].sCount;
			__int64 serial = m_sItemArray[SLOT_MAX+srcpos].nSerialNum;
			m_sItemArray[SLOT_MAX+srcpos].nNum = m_sItemArray[SLOT_MAX+destpos].nNum;
			m_sItemArray[SLOT_MAX+srcpos].sDuration = m_sItemArray[SLOT_MAX+destpos].sDuration;
			m_sItemArray[SLOT_MAX+srcpos].sCount = m_sItemArray[SLOT_MAX+destpos].sCount;
			m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_sItemArray[SLOT_MAX+destpos].nSerialNum;

			m_sItemArray[SLOT_MAX+destpos].nNum = itemid;
			m_sItemArray[SLOT_MAX+destpos].sDuration = duration;
			m_sItemArray[SLOT_MAX+destpos].sCount = itemcount;
			m_sItemArray[SLOT_MAX+destpos].nSerialNum = serial;
		}
		break;
	}

	bResult = true;

fail_return: // hmm...
	result << uint8(command) << bResult;
	Send(&result);
}

bool CUser::CheckWeight(int itemid, short count)
{
	_ITEM_TABLE* pTable = g_pMain->GetItemPtr(itemid);
	return (pTable != NULL // Make sure the item exists
			// and that the weight doesn't exceed our limit
			&& (m_sItemWeight + (pTable->m_sWeight * count)) <= m_sMaxWeight
			// and we have room for the item.
			&& FindSlotForItem(itemid, count) >= 0);
}

bool CUser::CheckExistItem(int itemid, short count)
{
	_ITEM_TABLE* pTable = g_pMain->GetItemPtr(itemid);
	if (pTable == NULL)
		return false;	

	// Search for the existance of all items in the player's inventory storage and onwards (includes magic bags)
	for (int i = SLOT_MAX; i < INVENTORY_TOTAL; i++)
	{
		// This implementation fixes a bug where it ignored the possibility for multiple stacks.
		if (m_sItemArray[i].nNum == itemid
				&& m_sItemArray[i].sCount >= count)
			return true;
	}

	return false;
}

// Pretend you didn't see me. This really needs to go (just copying official)
bool CUser::CheckExistItemAnd(int32 nItemID1, int16 sCount1, int32 nItemID2, int16 sCount2,
		int32 nItemID3, int16 sCount3, int32 nItemID4, int16 sCount4, int32 nItemID5, int16 sCount5)
{
	if (nItemID1
		&& !CheckExistItem(nItemID1, sCount1))
		return false;

	if (nItemID2
		&& !CheckExistItem(nItemID2, sCount2))
		return false;

	if (nItemID3
		&& !CheckExistItem(nItemID3, sCount3))
		return false;

	if (nItemID4
		&& !CheckExistItem(nItemID4, sCount4))
		return false;

	if (nItemID5
		&& !CheckExistItem(nItemID5, sCount5))
		return false;

	return true;
}

bool CUser::RobItem(uint32 itemid, uint16 count)
{
	// Allow unused exchanges.
	if (count == 0)
		return true;

	_ITEM_TABLE* pTable = g_pMain->GetItemPtr( itemid );
	if (pTable == NULL)
		return false;

	// Search for the existance of all items in the player's inventory storage and onwards (includes magic bags)
	for (int i = SLOT_MAX; i < INVENTORY_TOTAL; i++)
	{
		_ITEM_DATA *pItem = &m_sItemArray[i];
		if (pItem->nNum != itemid
			|| m_sItemArray[i].sCount < count)
			continue;

		pItem->sCount -= count;

		// This is a hackfix to ensure the duration is tweaked as well as the count
		// for those special items that use this instead.
		// This may or may not be accurate.
		if (pTable->m_bKind == 255)
			pItem->sDuration = pItem->sCount;

		if (pItem->sCount == 0)
			memset(&m_sItemArray[i], 0, sizeof(_ITEM_DATA));

		SendStackChange(itemid, pItem->sCount, pItem->sDuration, i - SLOT_MAX);
		return true;
	}
	
	return false;
}

bool CUser::GiveItem(uint32 itemid, uint16 count, bool send_packet /*= true*/)
{
	uint8 pos;
	bool bNewItem = true;
	_ITEM_TABLE* pTable = g_pMain->GetItemPtr( itemid );
	if (pTable == NULL)
		return false;	
	
	pos = FindSlotForItem(itemid, count);
	if (pos < 0)
		return false;

	_ITEM_DATA *pItem = &m_sItemArray[SLOT_MAX+pos];
	if (pItem->nNum != 0)
		bNewItem = false;

	pItem->nNum = itemid;
	pItem->sCount += count;
	if (pItem->sCount > MAX_ITEM_COUNT)
		pItem->sCount = MAX_ITEM_COUNT;
	
	pItem->sDuration = pTable->m_sDuration;

	// This is really silly, but match the count up with the duration
	// for this special items that behave this way.
	if (pTable->m_bKind == 255)
		pItem->sCount = pItem->sDuration;

	if (send_packet)
		SendStackChange(itemid, m_sItemArray[SLOT_MAX+pos].sCount, m_sItemArray[SLOT_MAX+pos].sDuration, pos, true);
	return true;
}

void CUser::SendItemWeight()
{
	Packet result(WIZ_WEIGHT_CHANGE);
	SetSlotItemValue();
	result << m_sItemWeight;
	Send(&result);
}

bool CUser::ItemEquipAvailable(_ITEM_TABLE *pTable)
{
	return (pTable != NULL
		&& GetLevel() >= pTable->m_bReqLevel 
		&& GetLevel() <= pTable->m_bReqLevelMax
		&& (pTable->m_bRace == 0 || pTable->m_bRace == m_bRace)
		&& m_bRank >= pTable->m_bReqRank // this needs to be verified
		&& m_bTitle >= pTable->m_bReqTitle // this is unused
		&& getStat(STAT_STR) >= pTable->m_bReqStr 
		&& getStat(STAT_STA) >= pTable->m_bReqSta 
		&& getStat(STAT_DEX) >= pTable->m_bReqDex 
		&& getStat(STAT_INT) >= pTable->m_bReqIntel 
		&& getStat(STAT_CHA) >= pTable->m_bReqCha);
}

void CUser::ItemMove(Packet & pkt)
{
	_ITEM_DATA *pSrcItem, *pDstItem, tmpItem;
	uint32 nItemID;
	uint8 dir, bSrcPos, bDstPos;

	pkt >> dir >> nItemID >> bSrcPos >> bDstPos;

	if (isTrading() || isMerchanting())
		goto fail_return;

	_ITEM_TABLE *pTable = g_pMain->GetItemPtr(nItemID);
	if (pTable == NULL
		//  || dir == ITEM_INVEN_SLOT && ((pTable->m_sWeight + m_sItemWeight) > m_sMaxWeight))
		//  || dir > ITEM_MBAG_TO_MBAG || bSrcPos >= SLOT_MAX+HAVE_MAX+COSP_MAX+MBAG_MAX || bDstPos >= SLOT_MAX+HAVE_MAX+COSP_MAX+MBAG_MAX
			|| ((dir == ITEM_INVEN_SLOT || dir == ITEM_SLOT_SLOT) 
				&& (bDstPos > SLOT_MAX || !ItemEquipAvailable(pTable)))
			|| (dir == ITEM_SLOT_INVEN && bSrcPos > SLOT_MAX)
			|| (dir == ITEM_INVEN_SLOT && bDstPos == RESERVED)
			|| (dir == ITEM_SLOT_INVEN && bDstPos == RESERVED))
			goto fail_return;

	switch (dir)
	{
	case ITEM_MBAG_TO_MBAG:
		if (bDstPos >= MBAG_TOTAL || bSrcPos >= MBAG_TOTAL
			// We also need to make sure that if we're setting an item in a magic bag, we need to actually
			// have a magic back to put the item in!
			|| (INVENTORY_MBAG+bDstPos <  INVENTORY_MBAG2 && m_sItemArray[BAG1].nNum == 0)
			|| (INVENTORY_MBAG+bDstPos >= INVENTORY_MBAG2 && m_sItemArray[BAG2].nNum == 0)
			// Make sure that the item actually exists there.
			|| nItemID != m_sItemArray[INVENTORY_MBAG + bSrcPos].nNum)
			goto fail_return;

		pSrcItem = &m_sItemArray[INVENTORY_MBAG + bSrcPos];
		pDstItem = &m_sItemArray[INVENTORY_MBAG + bDstPos];
		break;

	case ITEM_MBAG_TO_INVEN:
		if (bDstPos >= HAVE_MAX || bSrcPos >= MBAG_TOTAL
			// We also need to make sure that if we're taking an item from a magic bag, we need to actually
			// have a magic back to take it from!
			|| (INVENTORY_MBAG+bSrcPos <  INVENTORY_MBAG2 && m_sItemArray[BAG1].nNum == 0)
			|| (INVENTORY_MBAG+bSrcPos >= INVENTORY_MBAG2 && m_sItemArray[BAG2].nNum == 0)
			// Make sure that the item actually exists there.
			|| nItemID != m_sItemArray[INVENTORY_MBAG + bSrcPos].nNum)
			goto fail_return;
		
		pSrcItem = &m_sItemArray[INVENTORY_MBAG + bSrcPos];
		pDstItem = &m_sItemArray[INVENTORY_INVENT + bDstPos];
		break;

	case ITEM_INVEN_TO_MBAG:
		if (bDstPos >= MBAG_TOTAL || bSrcPos >= HAVE_MAX
			// We also need to make sure that if we're adding an item to a magic bag, we need to actually
			// have a magic back to put the item in!
			|| (INVENTORY_MBAG + bDstPos < INVENTORY_MBAG2 && m_sItemArray[BAG1].nNum == 0)
			|| (INVENTORY_MBAG + bDstPos >= INVENTORY_MBAG2 && m_sItemArray[BAG2].nNum == 0)
			// Make sure that the item actually exists there.
			|| nItemID != m_sItemArray[INVENTORY_INVENT + bSrcPos].nNum)
			goto fail_return;
		
		pSrcItem = &m_sItemArray[INVENTORY_INVENT + bSrcPos];
		pDstItem = &m_sItemArray[INVENTORY_MBAG + bDstPos];
		break;

	case ITEM_COSP_TO_INVEN:
		if (bDstPos >= HAVE_MAX || bSrcPos >= COSP_MAX
			// Make sure that the item actually exists there.
			|| nItemID != m_sItemArray[INVENTORY_COSP + bSrcPos].nNum)
			goto fail_return;
		
		pSrcItem = &m_sItemArray[INVENTORY_COSP + bSrcPos];
		pDstItem = &m_sItemArray[INVENTORY_INVENT + bDstPos];
		break;

	case ITEM_INVEN_TO_COSP: // TO-DO: Update IsValidSlotPos() for cospre items?
		if (bDstPos >= COSP_MAX || bSrcPos >= HAVE_MAX
			// Make sure that the item actually exists there.
			|| nItemID != m_sItemArray[INVENTORY_INVENT + bSrcPos].nNum)
			goto fail_return;
		
		pSrcItem = &m_sItemArray[INVENTORY_INVENT + bSrcPos];
		pDstItem = &m_sItemArray[INVENTORY_COSP + bDstPos];
		break;

	case ITEM_INVEN_SLOT:
		if (bDstPos >= SLOT_MAX || bSrcPos >= HAVE_MAX
			// Make sure that the item actually exists there.
			|| nItemID != m_sItemArray[INVENTORY_INVENT + bSrcPos].nNum
			// Ensure the item is able to be equipped in that slot
			|| !IsValidSlotPos(pTable, bDstPos))
			goto fail_return;

		pSrcItem = &m_sItemArray[INVENTORY_INVENT + bSrcPos];
		pDstItem = &m_sItemArray[bDstPos];
		break;

	case ITEM_SLOT_INVEN:
		if (bDstPos >= HAVE_MAX || bSrcPos >= SLOT_MAX
			// Make sure that the item actually exists there.
			|| nItemID != m_sItemArray[bSrcPos].nNum)
			goto fail_return;
		
		pSrcItem = &m_sItemArray[bSrcPos];
		pDstItem = &m_sItemArray[INVENTORY_INVENT + bDstPos];
		break;

	case ITEM_INVEN_INVEN:
		if (bDstPos >= HAVE_MAX || bSrcPos >= HAVE_MAX
			// Make sure that the item actually exists there.
			|| nItemID != m_sItemArray[INVENTORY_INVENT + bSrcPos].nNum)
			goto fail_return;
		
		pSrcItem = &m_sItemArray[INVENTORY_INVENT + bSrcPos];
		pDstItem = &m_sItemArray[INVENTORY_INVENT + bDstPos];
		break;

	case ITEM_SLOT_SLOT:
		if (bDstPos >= SLOT_MAX || bSrcPos >= SLOT_MAX
			// Make sure that the item actually exists there.
			|| nItemID != m_sItemArray[bSrcPos].nNum
			// Ensure the item is able to be equipped in that slot
			|| !IsValidSlotPos(pTable, bDstPos))
			goto fail_return;
		
		pSrcItem = &m_sItemArray[bSrcPos];
		pDstItem = &m_sItemArray[bDstPos];
		break;

	default:
		return;
	}

	// If there's an item already in the target slot already, we need to just swap the items
	if (pDstItem->nNum != 0)
	{
		memcpy(&tmpItem, pDstItem, sizeof(_ITEM_DATA)); // Temporarily store the target item
		memcpy(pDstItem, pSrcItem, sizeof(_ITEM_DATA)); // Replace the target item with the source
		memcpy(pSrcItem, &tmpItem, sizeof(_ITEM_DATA)); // Now replace the source with the old target (swapping them)
	}
	// Since there's no way to move a partial stack using this handler, just overwrite the destination.
	else
	{
		memcpy(pDstItem, pSrcItem, sizeof(_ITEM_DATA)); // Shift the item over
		memset(pSrcItem, 0, sizeof(_ITEM_DATA)); // Clear out the source item's data
	}

	// If equipping/de-equipping an item
	if (dir == ITEM_INVEN_SLOT || dir == ITEM_SLOT_INVEN
		// or moving an item to/from our cospre item slots
		|| dir == ITEM_INVEN_TO_COSP || dir == ITEM_COSP_TO_INVEN
		|| dir == ITEM_SLOT_SLOT)
	{
		// Re-update item stats
		SetSlotItemValue();
		SetUserAbility(false);
	}

	SendItemMove(1);
	SendItemWeight();

	// Update everyone else, so that they can see your shiny new items (you didn't take them off did you!? DID YOU!?)
	if (dir == ITEM_INVEN_SLOT)
		UserLookChange(bDstPos, nItemID, pDstItem->sDuration);	
	if (dir == ITEM_SLOT_INVEN) 
		UserLookChange(bSrcPos, 0, 0);
	if (dir == ITEM_SLOT_SLOT)
	{
		UserLookChange(bSrcPos, pSrcItem->nNum, pSrcItem->sDuration);
		UserLookChange(bDstPos, pDstItem->nNum, pDstItem->sDuration);
	}
	if (dir == ITEM_INVEN_TO_COSP)
		UserLookChange(INVENTORY_COSP + bDstPos, nItemID, pDstItem->sDuration);
	if (dir == ITEM_COSP_TO_INVEN)
		UserLookChange(INVENTORY_INVENT + bSrcPos, 0, 0);

	Send2AI_UserUpdateInfo();
	return;

fail_return:
	SendItemMove(0);
}

bool CUser::CheckExchange(int nExchangeID)
{
	// Does the exchange exist?
	_ITEM_EXCHANGE * pExchange = g_pMain->m_ItemExchangeArray.GetData(nExchangeID);
	if (pExchange == NULL)
		return false;

	// Find free slots in the inventory, so that we can check against this later.
	uint8 bFreeSlots = 0;
	for (int i = SLOT_MAX; i < SLOT_MAX+HAVE_MAX; i++)
	{
		if (GetItem(i)->nNum == 0
			&& ++bFreeSlots >= ITEMS_IN_EXCHANGE_GROUP)
			break;
	}

	// Add up the rates for this exchange to obtain a total percentage
	int nTotalPercent = 0;
	for (int i = 0; i < ITEMS_IN_EXCHANGE_GROUP; i++)
		nTotalPercent += pExchange->sExchangeItemCount[i];

	if (nTotalPercent > 9000)
		return (bFreeSlots > 0);

	// Can we hold all of these items? If we can't, we have a problem.
	uint8 bReqSlots = 0;
	uint32 nReqWeight = 0;
	for (int i = 0; i < ITEMS_IN_EXCHANGE_GROUP; i++)
	{
		uint32 nItemID = pExchange->nExchangeItemNum[i];

		// Does the item exist? If not, we'll ignore it (NOTE: not official behaviour).
		_ITEM_TABLE * pTable = NULL;
		if (nItemID == 0
			|| (pTable = g_pMain->GetItemPtr(nItemID)) == NULL)
			continue;

		// Try to find a slot for the item.
		// If we can't find an applicable slot with our inventory as-is,
		// there's no point even checking further.
		int pos;
		if ((pos = FindSlotForItem(nItemID, 1)) < 0)
			return false;

		// Now that we have our slot, see if it's in use (i.e. if adding a stackable item)
		// If it's in use, then we don't have to worry about requiring an extra slot for this item.
		// The only caveat here is with having multiple of the same stackable item: 
		// theoretically we could give them OK, but when it comes time to adding them, we'll find that
		// there's too many of them and they can't fit in the same slot. 
		// As this isn't an issue with real use cases, we can ignore it.
		_ITEM_DATA *pItem = GetItem(pos);
		if (pItem->nNum == 0)
			bReqSlots++; // new item? new required slot.

		// Also take into account weight (not official behaviour)
		nReqWeight += pTable->m_sWeight;
	}

	// Holding too much already?
	if (m_sItemWeight + nReqWeight > m_sMaxWeight)
		return false;

	// Do we have enough slots?
	return (bFreeSlots >= bReqSlots);
}

bool CUser::RunExchange(int nExchangeID)
{
	_ITEM_EXCHANGE * pExchange = g_pMain->m_ItemExchangeArray.GetData(nExchangeID);

	// Does the exchange exist?
	if (pExchange == NULL
		// Is it a valid exchange (do we have room?)
		|| !CheckExchange(nExchangeID)
		// We handle flags from 0-101 only. Anything else is broken.
		|| pExchange->bRandomFlag > 101
		// Do we have all of the required items?
		|| !CheckExistItemAnd(
			pExchange->nOriginItemNum[0], pExchange->sOriginItemCount[0], 
			pExchange->nOriginItemNum[1], pExchange->sOriginItemCount[1], 
			pExchange->nOriginItemNum[2], pExchange->sOriginItemCount[2], 
			pExchange->nOriginItemNum[3], pExchange->sOriginItemCount[3], 
			pExchange->nOriginItemNum[4], pExchange->sOriginItemCount[4])
		// These checks are a little pointless, but remove the required items as well.
		|| !RobItem(pExchange->nOriginItemNum[0], pExchange->sOriginItemCount[0])
		|| !RobItem(pExchange->nOriginItemNum[1], pExchange->sOriginItemCount[1])
		|| !RobItem(pExchange->nOriginItemNum[2], pExchange->sOriginItemCount[2])
		|| !RobItem(pExchange->nOriginItemNum[3], pExchange->sOriginItemCount[3])
		|| !RobItem(pExchange->nOriginItemNum[4], pExchange->sOriginItemCount[4]))
		return false;
		
	// No random element? We're just exchanging x items for y items.
	if (!pExchange->bRandomFlag)
	{
		for (int i = 0; i < ITEMS_IN_EXCHANGE_GROUP; i++)
		    GiveItem(pExchange->nExchangeItemNum[i], pExchange->sExchangeItemCount[i]);
	}
	// For these items the rate set by bRandomFlag.
	else if (pExchange->bRandomFlag <= 100)
	{
		int rand = myrand(0, 1000 * pExchange->bRandomFlag) / 1000;
		if (rand == 5)
			rand = 4;

		if (rand <= 4)
			GiveItem(pExchange->nExchangeItemNum[rand], pExchange->sExchangeItemCount[rand]);
	}
	// For 101, the rates are determined by sExchangeItemCount.
	else if (pExchange->bRandomFlag == 101)
	{
		uint32 nTotalPercent = 0;
		for (int i = 0; i < ITEMS_IN_EXCHANGE_GROUP; i++)
			nTotalPercent += pExchange->sExchangeItemCount[i];

		// If they add up to more than 100%, 
		if (nTotalPercent > 10000)
		{
			TRACE("Exchange %d is invalid. Rates add up to more than 100%% (%d%%)", nExchangeID, nTotalPercent / 100);
			return false;
		}

		// Holy stack batman! We're just going ahead and copying official for now.
		// NOTE: Officially they even use 2 bytes per element. Yikes.
		uint8 bRandArray[10000];
		memset(&bRandArray, 0, sizeof(bRandArray)); // default to 0 in case it's lower than 100% (in which case, first item's rate increases)

		// Copy the counts, as we're going to adjust them locally.
		uint16 sExchangeCount[ITEMS_IN_EXCHANGE_GROUP];

		memcpy(&sExchangeCount, &pExchange->sExchangeItemCount, sizeof(pExchange->sExchangeItemCount));

		// Build array of exchange item slots (0-4)
		int offset = 0;
		for (int n = 0, i = 0; n < 5; n++)
		{
			if (sExchangeCount[n] > 0)
			{
				memset(&bRandArray[offset], n, sExchangeCount[n]);
				offset += sExchangeCount[n];
			}
		}

		// Pull our exchange item slot out of our hat (the array we generated).
		uint8 bRandSlot = bRandArray[myrand(0, 9999)];
		uint32 nItemID = pExchange->nExchangeItemNum[bRandSlot];

		// Finally, give our item.
		GiveItem(nItemID, 1);
	}

	return true;
}

bool CUser::IsValidSlotPos(_ITEM_TABLE* pTable, int destpos)
{
	if( !pTable )
		return false;

	switch( pTable->m_bSlot ) {
	case 0:
		if( destpos != RIGHTHAND && destpos != LEFTHAND )
			return false;
		break;
	case 1:
	case 3:
		if( destpos != RIGHTHAND )
			return false;
		break;
	case 2:
	case 4:
		if( destpos != LEFTHAND )
			return false;
		break;
	case 5:
		if( destpos != BREAST )
			return false;
		break;
	case 6:
		if( destpos != LEG )
			return false;
		break;
	case 7:
		if( destpos != HEAD )
			return false;
		break;
	case 8:
		if( destpos != GLOVE )
			return false;
		break;
	case 9:
		if( destpos != FOOT )
			return false;
		break;
	case 10:
		if( destpos != RIGHTEAR && destpos != LEFTEAR )
			return false;
		break;
	case 11:
		if( destpos != NECK )
			return false;
		break;
	case 12:
		if( destpos != RIGHTRING && destpos != LEFTRING )
			return false;
		break;
	case 13:
		if( destpos != SHOULDER )
			return false;
		break;
	case 14:
		if( destpos != WAIST )
			return false;
		break;
	default:
		return false;
	}

	return true;
}

void CUser::SendStackChange(uint32 nItemID, uint32 nCount /* needs to be 4 bytes, not a bug */, uint16 sDurability, uint8 bPos, bool bNewItem /* = false */)
{
	Packet result(WIZ_ITEM_COUNT_CHANGE);

	result << uint16(1);
	result << uint8(1);
	result << uint8(bPos);
	result << nItemID << nCount;
	result << uint8(bNewItem ? 100 : 0);
	result << sDurability;

	SendItemWeight();
	Send(&result);
}

void CUser::ItemRemove(Packet & pkt)
{
	Packet result(WIZ_ITEM_REMOVE);
	uint8 bType, bPos;
	uint32 nItemID;

	pkt >> bType >> bPos >> nItemID;

	// Inventory
	if (bType == 0)
	{
		if (bPos >= HAVE_MAX)
			goto fail_return;

		bPos += SLOT_MAX;
	}
	// Equipped items
	else if (bType == 1)
	{
		if (bPos >= SLOT_MAX)
			goto fail_return;
	}
	else if (bType == 2)
	{
		if (bPos >= HAVE_MAX)
			goto fail_return;
		bPos += SLOT_MAX;
	}

	_ITEM_DATA *pItem = &m_sItemArray[bPos];

	// Make sure the item matches what the client says it is
	if (pItem->nNum != nItemID
		|| pItem->isSealed() 
		|| pItem->isRented())
		goto fail_return;
	memset(pItem, 0, sizeof(_ITEM_DATA));

	SendItemWeight();
	result << uint8(1);
	Send(&result);

	return;
fail_return:
	result << uint8(0);
	Send(&result);
}