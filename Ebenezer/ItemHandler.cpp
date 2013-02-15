#include "StdAfx.h"

void CUser::WarehouseProcess(Packet & pkt)
{
	Packet result(WIZ_WAREHOUSE);
	uint32 itemid, count;
	uint16 npcid, reference_pos;
	uint8 page, srcpos, destpos;
	_ITEM_TABLE* pTable = NULL;
	uint8 command = pkt.read<uint8>();

	if (isDead())
	{
		TRACE("### WarehouseProcess Fail : name=%s(%d), m_bResHpType=%d, hp=%d, x=%d, z=%d ###\n", m_pUserData->m_id, m_Sid, m_bResHpType, m_pUserData->m_sHp, (int)m_pUserData->m_curx, (int)m_pUserData->m_curz);
		return;
	}
	if (isTrading())
		goto fail_return;

	if( command == WAREHOUSE_OPEN )	{
		result << uint8(WAREHOUSE_OPEN) << uint8(WAREHOUSE_OPEN) << uint32(m_pUserData->m_iBank);
		for(int i=0; i<WAREHOUSE_MAX; i++ ) {
			result << m_pUserData->m_sWarehouseArray[i].nNum << m_pUserData->m_sWarehouseArray[i].sDuration <<  m_pUserData->m_sWarehouseArray[i].sCount <<
				uint8(0) << uint16(0) << uint16(0) << uint16(0) << uint16(0);
		}
		Send(&result);
		return;
	}

	pkt >> npcid >> itemid >> page >> srcpos >> destpos;
	pTable = m_pMain->GetItemPtr( itemid );
	if( !pTable ) goto fail_return;
	reference_pos = 24 * page;

	switch( command ) {
	case WAREHOUSE_INPUT:
		pkt >> count;
		if( itemid == ITEM_GOLD ) {
			if( m_pUserData->m_iBank+count > 2100000000 ) goto fail_return;
			if( m_pUserData->m_iGold-count < 0 ) goto fail_return;
			m_pUserData->m_iBank += count;
			m_pUserData->m_iGold -= count;
			break;
		}
		if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum != itemid ) goto fail_return;
		if( reference_pos+destpos > WAREHOUSE_MAX ) goto fail_return;
		if( m_pUserData->m_sWarehouseArray[reference_pos+destpos].nNum && !pTable->m_bCountable ) goto fail_return;
		if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount < count ) goto fail_return;
		m_pUserData->m_sWarehouseArray[reference_pos+destpos].nNum = itemid;
		m_pUserData->m_sWarehouseArray[reference_pos+destpos].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
		m_pUserData->m_sWarehouseArray[reference_pos+destpos].nSerialNum = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;
		if( pTable->m_bCountable == 0 && m_pUserData->m_sWarehouseArray[reference_pos+destpos].nSerialNum == 0 )
			m_pUserData->m_sWarehouseArray[reference_pos+destpos].nSerialNum = m_pMain->GenerateItemSerial();

		if( pTable->m_bCountable ) {
			m_pUserData->m_sWarehouseArray[reference_pos+destpos].sCount += (unsigned short)count;
		}
		else {
			m_pUserData->m_sWarehouseArray[reference_pos+destpos].sCount = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount;
		}

		if( !pTable->m_bCountable ) {
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = 0;
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = 0;
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = 0;
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = 0;
		}
		else {
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount -= (unsigned short)count;
			if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount <= 0 ) {
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = 0;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = 0;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = 0;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = 0;
			}
		}

		SendItemWeight();
		ItemLogToAgent( m_pUserData->m_Accountid, m_pUserData->m_id, ITEM_WAREHOUSE_PUT, 0, itemid, count, m_pUserData->m_sWarehouseArray[reference_pos+destpos].sDuration );
		break;
	case WAREHOUSE_OUTPUT:
		pkt >> count;

		if( itemid == ITEM_GOLD ) {
			if( m_pUserData->m_iGold+count > 2100000000 ) goto fail_return;
			if( m_pUserData->m_iBank-count < 0 ) goto fail_return;
			m_pUserData->m_iGold += count;
			m_pUserData->m_iBank -= count;
			break;
		}
//
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
//
		if( reference_pos+srcpos > WAREHOUSE_MAX ) goto fail_return;
		if( m_pUserData->m_sWarehouseArray[reference_pos+srcpos].nNum != itemid ) goto fail_return;
		if( m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum && !pTable->m_bCountable ) goto fail_return;
		if( m_pUserData->m_sWarehouseArray[reference_pos+srcpos].sCount < count ) goto fail_return;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum = itemid;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration = m_pUserData->m_sWarehouseArray[reference_pos+srcpos].sDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum = m_pUserData->m_sWarehouseArray[reference_pos+srcpos].nSerialNum;
		if( pTable->m_bCountable )
			m_pUserData->m_sItemArray[SLOT_MAX+destpos].sCount += (unsigned short)count;
		else {
			if( m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum == 0 )
				m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum = m_pMain->GenerateItemSerial();
			m_pUserData->m_sItemArray[SLOT_MAX+destpos].sCount = m_pUserData->m_sWarehouseArray[reference_pos+srcpos].sCount;
		}
		if( !pTable->m_bCountable ) {
			m_pUserData->m_sWarehouseArray[reference_pos+srcpos].nNum = 0;
			m_pUserData->m_sWarehouseArray[reference_pos+srcpos].sDuration = 0;
			m_pUserData->m_sWarehouseArray[reference_pos+srcpos].sCount = 0;
			m_pUserData->m_sWarehouseArray[reference_pos+srcpos].nSerialNum = 0;
		}
		else {
			m_pUserData->m_sWarehouseArray[reference_pos+srcpos].sCount -= (unsigned short)count;
			if( m_pUserData->m_sWarehouseArray[reference_pos+srcpos].sCount <= 0 ) {
				m_pUserData->m_sWarehouseArray[reference_pos+srcpos].nNum = 0;
				m_pUserData->m_sWarehouseArray[reference_pos+srcpos].sDuration = 0;
				m_pUserData->m_sWarehouseArray[reference_pos+srcpos].sCount = 0;
				m_pUserData->m_sWarehouseArray[reference_pos+srcpos].nSerialNum = 0;
			}
		}

		SendItemWeight();		
		ItemLogToAgent( m_pUserData->m_id, m_pUserData->m_Accountid, ITEM_WAREHOUSE_GET, 0, itemid, count, m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration );
		//TRACE("WARE OUTPUT : %s %s %d %d %d %d %d", m_pUserData->m_id, m_pUserData->m_Accountid, ITEM_WAREHOUSE_GET, 0, itemid, count, m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration );
		break;
	case WAREHOUSE_MOVE:
		if( reference_pos+srcpos > WAREHOUSE_MAX ) goto fail_return;
		if( m_pUserData->m_sWarehouseArray[reference_pos+srcpos].nNum != itemid ) goto fail_return;
		if( m_pUserData->m_sWarehouseArray[reference_pos+destpos].nNum ) goto fail_return;
		m_pUserData->m_sWarehouseArray[reference_pos+destpos].nNum = itemid;
		m_pUserData->m_sWarehouseArray[reference_pos+destpos].sDuration = m_pUserData->m_sWarehouseArray[reference_pos+srcpos].sDuration;
		m_pUserData->m_sWarehouseArray[reference_pos+destpos].sCount = m_pUserData->m_sWarehouseArray[reference_pos+srcpos].sCount;
		m_pUserData->m_sWarehouseArray[reference_pos+destpos].nSerialNum = m_pUserData->m_sWarehouseArray[reference_pos+srcpos].nSerialNum;

		m_pUserData->m_sWarehouseArray[reference_pos+srcpos].nNum = 0;
		m_pUserData->m_sWarehouseArray[reference_pos+srcpos].sDuration = 0;
		m_pUserData->m_sWarehouseArray[reference_pos+srcpos].sCount = 0;
		m_pUserData->m_sWarehouseArray[reference_pos+srcpos].nSerialNum = 0;
		break;
	case WAREHOUSE_INVENMOVE:
		if( itemid != m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum )
			goto fail_return;
		{
			short duration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
			short itemcount = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount;
			__int64 serial = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum;
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration;
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = m_pUserData->m_sItemArray[SLOT_MAX+destpos].sCount;
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum;

			m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum = itemid;
			m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration = duration;
			m_pUserData->m_sItemArray[SLOT_MAX+destpos].sCount = itemcount;
			m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum = serial;
		}
		break;
	}

	m_pUserData->m_bWarehouse = 1;

	result << uint8(command) << uint8(1);
	Send(&result);
	return;

fail_return: // hmm...
	result << uint8(command) << uint8(0);
	Send(&result);
}

BOOL CUser::CheckWeight(int itemid, short count)
{
	_ITEM_TABLE* pTable = m_pMain->GetItemPtr(itemid);
	return (pTable != NULL // Make sure the item exists
			// and that the weight doesn't exceed our limit
			&& (m_sItemWeight + (pTable->m_sWeight * count)) <= m_sMaxWeight
			// and we have room for the item.
			&& GetEmptySlot(itemid, pTable->m_bCountable) >= 0);
}

BOOL CUser::CheckExistItem(int itemid, short count)
{
	_ITEM_TABLE* pTable = m_pMain->GetItemPtr(itemid);
	if (pTable == NULL)
		return FALSE;	

	// Search for the existance of all items in the player's inventory storage and onwards (includes magic bags)
	for (int i = SLOT_MAX; i < INVENTORY_TOTAL; i++)
	{
		// This implementation fixes a bug where it ignored the possibility for multiple stacks.
		if (m_pUserData->m_sItemArray[i].nNum == itemid
				&& m_pUserData->m_sItemArray[i].sCount >= count)
			return TRUE;
	}

	return FALSE;
}

BOOL CUser::RobItem(int itemid, short count)
{
	_ITEM_TABLE* pTable = m_pMain->GetItemPtr( itemid );
	if (pTable == NULL)
		return FALSE;

	// Search for the existance of all items in the player's inventory storage and onwards (includes magic bags)
	for (int i = SLOT_MAX; i < INVENTORY_TOTAL; i++)
	{
		_ITEM_DATA *pItem = &m_pUserData->m_sItemArray[i];
		if (pItem->nNum != itemid
			|| m_pUserData->m_sItemArray[i].sCount < count)
			continue;

		pItem->sCount -= count;
		if (pItem->sCount == 0)
			memset(&m_pUserData->m_sItemArray[i], 0, sizeof(_ITEM_DATA));

		SendItemWeight();
		SendStackChange(itemid, pItem->sCount, pItem->sDuration, i);
		return TRUE;
	}
	
	return FALSE;
}

BOOL CUser::GiveItem(int itemid, short count, bool send_packet /*= true*/)
{
	uint8 pos;
	bool bNewItem = true;
	_ITEM_TABLE* pTable = m_pMain->GetItemPtr( itemid );
	if (pTable == NULL)
		return FALSE;	
	
	pos = GetEmptySlot( itemid, pTable->m_bCountable );
	if (pos < 0)
		return FALSE;

	if (m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum != 0)
	{	
		// If non-stackable item and there's an item there already... not happening.
		if (!pTable->m_bCountable
			// otherwise, if the item isn't what the client thinks it is, better not change anything either.
			|| m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum != itemid) 
			return FALSE;

		bNewItem = false;
	}

	m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum = itemid;
	m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount += count;
	if (m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount > MAX_ITEM_COUNT)
		m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount = MAX_ITEM_COUNT;
		
	m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration = pTable->m_sDuration;

	SendItemWeight();	// Change weight first -- do this regardless.
	if (send_packet)
		SendStackChange(itemid, m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount, m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration, pos, true);
	return TRUE;
}

void CUser::ItemLogToAgent(const char *srcid, const char *tarid, int type, __int64 serial, int itemid, int count, int durability)
{
	// NOTE: item log implementation removed, keeping this method in for future reworkings
}

void CUser::SendItemWeight()
{
	Packet result(WIZ_WEIGHT_CHANGE);
	SetSlotItemValue();
	result << m_sItemWeight;
	Send(&result);
}

BOOL CUser::ItemEquipAvailable(_ITEM_TABLE *pTable)
{
	return (pTable != NULL
		&& getLevel() >= pTable->m_bReqLevel 
		&& getLevel() <= pTable->m_bReqLevelMax
		&& m_pUserData->m_bRank >= pTable->m_bReqRank // this needs to be verified
		&& m_pUserData->m_bTitle >= pTable->m_bReqTitle // this is unused
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

	if (isTrading())
		goto fail_return;

	_ITEM_TABLE *pTable = m_pMain->GetItemPtr(nItemID);
	if (pTable == NULL
		//  || dir == ITEM_INVEN_SLOT && ((pTable->m_sWeight + m_sItemWeight) > m_sMaxWeight))
		//  || dir > ITEM_MBAG_TO_MBAG || bSrcPos >= SLOT_MAX+HAVE_MAX+COSP_MAX+MBAG_MAX || bDstPos >= SLOT_MAX+HAVE_MAX+COSP_MAX+MBAG_MAX
			|| (dir == ITEM_INVEN_SLOT || dir == ITEM_SLOT_SLOT) && bDstPos > SLOT_MAX
			|| (dir == ITEM_SLOT_INVEN && bSrcPos > SLOT_MAX)
			|| (dir == ITEM_INVEN_SLOT && bDstPos == RESERVED)
			|| (dir == ITEM_SLOT_INVEN && bDstPos == RESERVED)
			|| (dir == ITEM_INVEN_SLOT || dir == ITEM_SLOT_SLOT)
			|| (pTable->m_bRace != 0 && pTable->m_bRace != m_pUserData->m_bRace)
			|| !ItemEquipAvailable(pTable))
			goto fail_return;

	switch (dir)
	{
	case ITEM_MBAG_TO_MBAG:
		if (bDstPos >= MBAG_TOTAL || bSrcPos >= MBAG_TOTAL
			// We also need to make sure that if we're setting an item in a magic bag, we need to actually
			// have a magic back to put the item in!
			|| (INVENTORY_MBAG+bDstPos <  INVENTORY_MBAG2 && m_pUserData->m_sItemArray[BAG1].nNum == 0)
			|| (INVENTORY_MBAG+bDstPos >= INVENTORY_MBAG2 && m_pUserData->m_sItemArray[BAG2].nNum == 0)
			// Make sure that the item actually exists there.
			|| nItemID != m_pUserData->m_sItemArray[INVENTORY_MBAG + bSrcPos].nNum)
			goto fail_return;

		pSrcItem = &m_pUserData->m_sItemArray[INVENTORY_MBAG + bSrcPos];
		pDstItem = &m_pUserData->m_sItemArray[INVENTORY_MBAG + bDstPos];
		break;

	case ITEM_MBAG_TO_INVEN:
		if (bDstPos >= HAVE_MAX || bSrcPos >= MBAG_TOTAL
			// We also need to make sure that if we're taking an item from a magic bag, we need to actually
			// have a magic back to take it from!
			|| (INVENTORY_MBAG+bSrcPos <  INVENTORY_MBAG2 && m_pUserData->m_sItemArray[BAG1].nNum == 0)
			|| (INVENTORY_MBAG+bSrcPos >= INVENTORY_MBAG2 && m_pUserData->m_sItemArray[BAG2].nNum == 0)
			// Make sure that the item actually exists there.
			|| nItemID != m_pUserData->m_sItemArray[INVENTORY_MBAG + bSrcPos].nNum)
			goto fail_return;
		
		pSrcItem = &m_pUserData->m_sItemArray[INVENTORY_MBAG + bSrcPos];
		pDstItem = &m_pUserData->m_sItemArray[INVENTORY_INVENT + bDstPos];
		break;

	case ITEM_INVEN_TO_MBAG:
		if (bDstPos >= MBAG_TOTAL || bSrcPos >= HAVE_MAX
			// We also need to make sure that if we're adding an item to a magic bag, we need to actually
			// have a magic back to put the item in!
			|| (INVENTORY_MBAG + bDstPos < INVENTORY_MBAG2 && m_pUserData->m_sItemArray[BAG1].nNum == 0)
			|| (INVENTORY_MBAG + bDstPos >= INVENTORY_MBAG2 && m_pUserData->m_sItemArray[BAG2].nNum == 0)
			// Make sure that the item actually exists there.
			|| nItemID != m_pUserData->m_sItemArray[INVENTORY_INVENT + bSrcPos].nNum)
			goto fail_return;
		
		pSrcItem = &m_pUserData->m_sItemArray[INVENTORY_INVENT + bSrcPos];
		pDstItem = &m_pUserData->m_sItemArray[INVENTORY_MBAG + bDstPos];
		break;

	case ITEM_COSP_TO_INVEN:
		if (bDstPos >= HAVE_MAX || bSrcPos >= COSP_MAX
			// Make sure that the item actually exists there.
			|| nItemID != m_pUserData->m_sItemArray[INVENTORY_COSP + bSrcPos].nNum)
			goto fail_return;
		
		pSrcItem = &m_pUserData->m_sItemArray[INVENTORY_COSP + bSrcPos];
		pDstItem = &m_pUserData->m_sItemArray[INVENTORY_INVENT + bDstPos];
		break;

	case ITEM_INVEN_TO_COSP: // TO-DO: Update IsValidSlotPos() for cospre items?
		if (bDstPos >= COSP_MAX || bSrcPos >= HAVE_MAX
			// Make sure that the item actually exists there.
			|| nItemID != m_pUserData->m_sItemArray[INVENTORY_INVENT + bSrcPos].nNum)
			goto fail_return;
		
		pSrcItem = &m_pUserData->m_sItemArray[INVENTORY_INVENT + bSrcPos];
		pDstItem = &m_pUserData->m_sItemArray[INVENTORY_COSP + bDstPos];
		break;

	case ITEM_INVEN_SLOT:
		if (bDstPos >= SLOT_MAX || bSrcPos >= SLOT_MAX
			// Make sure that the item actually exists there.
			|| nItemID != m_pUserData->m_sItemArray[INVENTORY_INVENT + bSrcPos].nNum
			// Ensure the item is able to be equipped in that slot
			|| !IsValidSlotPos(pTable, bDstPos))
			goto fail_return;

		pSrcItem = &m_pUserData->m_sItemArray[bSrcPos];
		pDstItem = &m_pUserData->m_sItemArray[INVENTORY_INVENT + bDstPos];
		break;

	case ITEM_SLOT_INVEN:
		if (bDstPos >= HAVE_MAX || bSrcPos >= SLOT_MAX
			// Make sure that the item actually exists there.
			|| nItemID != m_pUserData->m_sItemArray[bSrcPos].nNum)
			goto fail_return;
		
		pSrcItem = &m_pUserData->m_sItemArray[bSrcPos];
		pDstItem = &m_pUserData->m_sItemArray[INVENTORY_INVENT + bDstPos];
		break;

	case ITEM_INVEN_INVEN:
		if (bDstPos >= HAVE_MAX || bSrcPos >= HAVE_MAX
			// Make sure that the item actually exists there.
			|| nItemID != m_pUserData->m_sItemArray[INVENTORY_INVENT + bSrcPos].nNum)
			goto fail_return;
		
		pSrcItem = &m_pUserData->m_sItemArray[INVENTORY_INVENT + bSrcPos];
		pDstItem = &m_pUserData->m_sItemArray[INVENTORY_INVENT + bDstPos];
		break;

	case ITEM_SLOT_SLOT:
		if (bDstPos >= SLOT_MAX || bSrcPos >= SLOT_MAX
			// Make sure that the item actually exists there.
			|| nItemID != m_pUserData->m_sItemArray[bSrcPos].nNum
			// Ensure the item is able to be equipped in that slot
			|| !IsValidSlotPos(pTable, bDstPos))
			goto fail_return;
		
		pSrcItem = &m_pUserData->m_sItemArray[bSrcPos];
		pDstItem = &m_pUserData->m_sItemArray[bDstPos];
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
		|| dir == ITEM_INVEN_TO_COSP || dir == ITEM_COSP_TO_INVEN)
	{
		// Re-update item stats
		SetSlotItemValue();
		SetUserAbility();
	}

	SendItemMove();
	SendItemWeight();

	// Update everyone else, so that they can see your shiny new items (you didn't take them off did you!? DID YOU!?)
	if (dir == ITEM_INVEN_SLOT)
		UserLookChange(bDstPos, nItemID, pDstItem->sDuration);	
	if (dir == ITEM_SLOT_INVEN) 
		UserLookChange(bSrcPos, 0, 0);	
	if (dir == ITEM_INVEN_TO_COSP)
		UserLookChange(INVENTORY_COSP + bDstPos, nItemID, pDstItem->sDuration);
	if (dir == ITEM_COSP_TO_INVEN)
		UserLookChange(INVENTORY_INVENT + bSrcPos, 0, 0);

	Send2AI_UserUpdateInfo();
	return;

fail_return:
	SendItemMove(true);
}

BOOL CUser::IsValidSlotPos(_ITEM_TABLE* pTable, int destpos)
{
	if( !pTable )
		return FALSE;

	switch( pTable->m_bSlot ) {
	case 0:
		if( destpos != RIGHTHAND && destpos != LEFTHAND )
			return FALSE;
		break;
	case 1:
	case 3:
		if( destpos != RIGHTHAND )
			return FALSE;
		break;
	case 2:
	case 4:
		if( destpos != LEFTHAND )
			return FALSE;
		break;
	case 5:
		if( destpos != BREAST )
			return FALSE;
		break;
	case 6:
		if( destpos != LEG )
			return FALSE;
		break;
	case 7:
		if( destpos != HEAD )
			return FALSE;
		break;
	case 8:
		if( destpos != GLOVE )
			return FALSE;
		break;
	case 9:
		if( destpos != FOOT )
			return FALSE;
		break;
	case 10:
		if( destpos != RIGHTEAR && destpos != LEFTEAR )
			return FALSE;
		break;
	case 11:
		if( destpos != NECK )
			return FALSE;
		break;
	case 12:
		if( destpos != RIGHTRING && destpos != LEFTRING )
			return FALSE;
		break;
	case 13:
		if( destpos != SHOULDER )
			return FALSE;
		break;
	case 14:
		if( destpos != WAIST )
			return FALSE;
		break;
	default:
		return FALSE;
	}

	return TRUE;
}

void CUser::SendStackChange(uint32 nItemID, uint32 nCount /* needs to be 4 bytes, not a bug */, uint16 sDurability, uint8 bPos, bool bNewItem /* = false */)
{
	Packet result(WIZ_ITEM_COUNT_CHANGE);

	result << uint16(1);
	result << uint8(1);
	result << uint8(bPos - SLOT_MAX);
	result << nItemID << nCount;
	result << uint8(bNewItem ? 100 : 0);
	result << sDurability;

	SendItemWeight();
	Send(&result);
}

/**
 * Firstly, hello there weary traveler! You've come a long way.
 * I hate to point out the obvious, but well... no, you don't need to get your 
 * glasses/eyes checked.
 *
 * Yes, you're looking at a piece of (luckily for you) cleaned up work of mgame's
 * that seems completely out of place.
 *
 * But why, you ask? Why would we check a stack's range? There's really no reason 
 * for ever doing that, is there? 
 * Surely there's no NPC that says "I ONLY WANT 10 OF THIS ITEM, IF YOU GET 11, 
 * I'LL.. I'LL.. WELL, I'M USELESS SO I'LL JUST NOT TALK TO YOU!"
 *
 * You'd think not.
 *
 * This method still exists purely because of EVT's dependence upon it
 * to check when a required item count does NOT exist. Wait -- what?!
 *
 * This could *easily* be fixed with reverse logic in the EVT, but as we don't 
 * want to break existing EVT implementations we'll leave this method intact.
 *
 * For now.
 **/
BOOL CUser::CheckItemCount(int itemid, short min, short max)
{
	_ITEM_TABLE* pTable = m_pMain->GetItemPtr( itemid );
	if (pTable == NULL)
		return FALSE;	

	for (int i = 0 ; i < SLOT_MAX + HAVE_MAX; i++)
	{
		if (m_pUserData->m_sItemArray[i].nNum != itemid)
			continue;

		return (m_pUserData->m_sItemArray[i].sCount >= min && m_pUserData->m_sItemArray[i].sCount <= max);
	}

	return FALSE;		
}