#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"

void CUser::WarehouseProcess(char *pBuf)
{
	Packet result(WIZ_WAREHOUSE);
	int index = 0, itemid = 0, srcpos = -1, destpos = -1, page = -1, reference_pos = -1, npcid = 0;
	DWORD count = 0;
	_ITEM_TABLE* pTable = NULL;
	BYTE command = 0;
	command = GetByte( pBuf, index );

	// â?? ??j?...
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

	npcid = GetShort(pBuf,index);
	itemid = GetDWORD( pBuf, index );
	page = GetByte( pBuf, index );
	srcpos = GetByte( pBuf, index );
	destpos = GetByte( pBuf, index );
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
	if( !pTable ) goto fail_return;
	reference_pos = 24 * page;

	switch( command ) {
	case WAREHOUSE_INPUT:
		count = GetDWORD( pBuf, index );
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
		count = GetDWORD( pBuf, index );

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
	_ITEM_TABLE* pTable = m_pMain->m_ItemtableArray.GetData(itemid);
	return (pTable != NULL // Make sure the item exists
			// and that the weight doesn't exceed our limit
			&& (m_sItemWeight + (pTable->m_sWeight * count)) <= m_sMaxWeight
			// and we have room for the item.
			&& GetEmptySlot(itemid, pTable->m_bCountable) >= 0);
}

BOOL CUser::CheckExistItem(int itemid, short count)
{
	_ITEM_TABLE* pTable = m_pMain->m_ItemtableArray.GetData(itemid);
	if (pTable == NULL)
		return FALSE;	

	for (int i = 0; i < SLOT_MAX + HAVE_MAX; i++)
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
	int send_index = 0, i = 0;					
	char send_buff[256];
	BYTE type = 1;

	_ITEM_TABLE* pTable = NULL;				// This checks if such an item exists.
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
	if( !pTable ) return FALSE;

	for ( i = SLOT_MAX ; i < SLOT_MAX + HAVE_MAX * type ; i++ ) {
		if( m_pUserData->m_sItemArray[i].nNum == itemid ) {		
			if (!pTable->m_bCountable) {	// Remove item from inventory (Non-countable items)
				m_pUserData->m_sItemArray[i].nNum = 0;
				m_pUserData->m_sItemArray[i].sCount = 0;
				m_pUserData->m_sItemArray[i].sDuration = 0;
				goto success_return;
			}
			else {	// Remove the number of items from the inventory (Countable Items)
				if (m_pUserData->m_sItemArray[i].sCount >= count) {
					m_pUserData->m_sItemArray[i].sCount -= count ;

					if (m_pUserData->m_sItemArray[i].sCount == 0) {
						m_pUserData->m_sItemArray[i].nNum = 0 ;
						m_pUserData->m_sItemArray[i].sCount = 0;
						m_pUserData->m_sItemArray[i].sDuration = 0;
					}					
					goto success_return;
				}
				else {
					return FALSE;	
				}
			}			
		}		
	}
	
	return FALSE;

success_return:
	SendItemWeight();	// Change weight first :)
	SetByte( send_buff, WIZ_ITEM_COUNT_CHANGE, send_index );	
	SetShort( send_buff, 0x01, send_index );	// The number of for-loops
	SetByte( send_buff, 0x01, send_index );
	SetByte( send_buff, i - SLOT_MAX, send_index );
	SetDWORD( send_buff, itemid, send_index );	// The ID of item.
	SetDWORD( send_buff, m_pUserData->m_sItemArray[i].sCount, send_index );
	Send( send_buff, send_index );
	return TRUE;
}

BOOL CUser::GiveItem(int itemid, short count, bool send_packet /*= true*/)
{
	uint8 pos;
	bool bNewItem = true;
	_ITEM_TABLE* pTable = m_pMain->m_ItemtableArray.GetData( itemid );
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
	if (pTable == NULL
		|| pTable->m_bReqLevel > m_pUserData->m_bLevel
		|| m_pUserData->m_bLevel > pTable->m_bReqLevelMax
		|| pTable->m_bReqRank > m_pUserData->m_bRank
		|| pTable->m_bReqTitle > m_pUserData->m_bTitle
		|| pTable->m_bReqStr > m_pUserData->m_bStr
		|| pTable->m_bReqSta > m_pUserData->m_bSta
		|| pTable->m_bReqDex > m_pUserData->m_bDex
		|| pTable->m_bReqIntel > m_pUserData->m_bIntel
		|| pTable->m_bReqCha > m_pUserData->m_bCha)
		return FALSE;

	return TRUE;
}

void CUser::ItemMove(char *pBuf)
{
	int index = 0, itemid = 0, srcpos = -1, destpos = -1;
	int sItemID = 0, sItemCount = 0, sItemDuration = 0;
	__int64 sItemSerial = 0;
	_ITEM_TABLE* pTable = NULL;
	BYTE dir;

	dir = GetByte( pBuf, index );
	itemid = GetDWORD( pBuf, index );
	srcpos = GetByte( pBuf, index );
	destpos = GetByte( pBuf, index );

	if (isTrading())
		goto fail_return;
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
	if( !pTable )
		goto fail_return;
//	if( dir == ITEM_INVEN_SLOT && ((pTable->m_sWeight + m_sItemWeight) > m_sMaxWeight) )
//		goto fail_return;
	if( dir > 0x0B || srcpos >= SLOT_MAX+HAVE_MAX+COSP_MAX+MBAG_MAX || destpos >= SLOT_MAX+HAVE_MAX+COSP_MAX+MBAG_MAX )
		goto fail_return;
	if( (dir == ITEM_INVEN_SLOT || dir == ITEM_SLOT_SLOT ) && destpos > SLOT_MAX )
		goto fail_return;
	if( dir == ITEM_SLOT_INVEN && srcpos > SLOT_MAX )
		goto fail_return;
	if( dir == ITEM_INVEN_SLOT && destpos == RESERVED )
		goto fail_return;
	if( dir == ITEM_SLOT_INVEN && srcpos == RESERVED )
		goto fail_return;
	if( dir == ITEM_INVEN_SLOT || dir == ITEM_SLOT_SLOT ) {
		if( pTable->m_bRace != 0 ) {
			if( pTable->m_bRace != m_pUserData->m_bRace )
				goto fail_return;
		}
		if( !ItemEquipAvailable( pTable ) )
			goto fail_return;
	}

	switch( dir ) {
	case ITEM_MBAG_TO_MBAG:
		if(SLOT_MAX+HAVE_MAX+COSP_MAX+destpos >= 49 && SLOT_MAX+HAVE_MAX+COSP_MAX+destpos <=60 && m_pUserData->m_sItemArray[BAG1].nNum==0)
			goto fail_return;
		if(SLOT_MAX+HAVE_MAX+COSP_MAX+destpos >= 61 && SLOT_MAX+HAVE_MAX+COSP_MAX+destpos <=72 && m_pUserData->m_sItemArray[BAG2].nNum==0)
			goto fail_return;
		if( itemid != m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].nNum ) 
			goto fail_return;

		if( m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].nNum != 0 ) {
			sItemID = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].nNum;
			sItemDuration = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].sDuration;
			sItemCount = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].sCount;
			sItemSerial = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].nSerialNum;
		}

		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].nNum = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].nNum ;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].sDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].sCount = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].sCount;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].nSerialNum = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].nSerialNum;
		if( m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].nSerialNum == 0 ) 
			m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].nSerialNum = m_pMain->GenerateItemSerial();

		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].nNum = sItemID;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].sDuration = sItemDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].sCount = sItemCount;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].nSerialNum = sItemSerial;
		break;
	case ITEM_MBAG_TO_INVEN:
		if( itemid != m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].nNum ) 
			goto fail_return;
		
		if( m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum != 0 ) {
			sItemID = m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum;
			sItemCount = m_pUserData->m_sItemArray[SLOT_MAX+destpos].sCount;
			sItemDuration = m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration;
			sItemSerial = m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum;
		}

		m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].nNum;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].sDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].sCount = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].sCount;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].nSerialNum;
		if( m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum == 0 ) 
			m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum = m_pMain->GenerateItemSerial();

		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].nNum = sItemID;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].sDuration = sItemDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].sCount = sItemCount;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+srcpos].nSerialNum = sItemSerial;
		break;

	case ITEM_INVEN_TO_MBAG:
		// TO-DO : Change the m_sItemArray[BAG1/2].nNum check to the actual magic bag ID.
		if(SLOT_MAX+HAVE_MAX+COSP_MAX+destpos >= 49 && SLOT_MAX+HAVE_MAX+COSP_MAX+destpos <=60 && m_pUserData->m_sItemArray[BAG1].nNum == 0)
			goto fail_return;
		if(SLOT_MAX+HAVE_MAX+COSP_MAX+destpos >= 61 && SLOT_MAX+HAVE_MAX+COSP_MAX+destpos <=73 && m_pUserData->m_sItemArray[BAG2].nNum == 0)
			goto fail_return;
		if( itemid != m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum ) 
			goto fail_return;
		if( m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].nNum != 0 ) {
			sItemID = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].nNum;
			sItemCount = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].sCount;
			sItemDuration = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].sDuration;
			sItemSerial = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].nSerialNum;
		}

		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].nNum = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].sCount = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].nSerialNum = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;
		if( m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].nSerialNum == 0 ) 
			m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+COSP_MAX+destpos].nSerialNum = m_pMain->GenerateItemSerial();

		m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = sItemID;
		m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = sItemDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = sItemCount;
		m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = sItemSerial;
		break;

	case ITEM_COSP_TO_INVEN:
		if( itemid != m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+srcpos].nNum ) //Make sure we actually have that item.
			goto fail_return;

		if( m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum != 0 )
			goto fail_return;

		m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+srcpos].nNum;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+srcpos].sDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].sCount = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+srcpos].sCount;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+srcpos].nSerialNum;

		if( m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum == 0 ) 
			m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum = m_pMain->GenerateItemSerial();

		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+srcpos].nNum = 0;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+srcpos].sDuration = 0;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+srcpos].sCount = 0;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+srcpos].nSerialNum = 0;
		break;

	case ITEM_INVEN_TO_COSP:
		if( itemid != m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum ) //Make sure we actually have that item.
			goto fail_return;

		if( m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+destpos].nNum != 0 ) {
			sItemID = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+destpos].nNum;
			sItemCount = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+destpos].sCount;
			sItemDuration = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+destpos].sDuration;
			sItemSerial = m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+destpos].nSerialNum;
		}

		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+destpos].nNum = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+destpos].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+destpos].sCount = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount;
		m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+destpos].nSerialNum = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;

		if( m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+destpos].nSerialNum == 0 ) 
			m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+destpos].nSerialNum = m_pMain->GenerateItemSerial();

		m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = sItemID;
		m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = sItemDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = sItemCount;
		m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = sItemSerial;
		break;

	case ITEM_INVEN_SLOT:
		if( itemid != m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum )
			goto fail_return;
		if( !IsValidSlotPos( pTable, destpos ) )
			goto fail_return;
		else if( pTable->m_bSlot == 0x01 || (pTable->m_bSlot == 0x00 && destpos == RIGHTHAND) ) {	// ???????? ????(??? ?????? ??? ???????? ?g?? ?????) ?e? ?????? ?µ???? ??? üu
			if(m_pUserData->m_sItemArray[LEFTHAND].nNum != 0) {
				_ITEM_TABLE* pTable2 = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[LEFTHAND].nNum );
				if( pTable2 ) {
					if( pTable2->m_bSlot == 0x04 ) {
						m_pUserData->m_sItemArray[RIGHTHAND].nNum = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum;	// ?????? ???..
						m_pUserData->m_sItemArray[RIGHTHAND].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
						m_pUserData->m_sItemArray[RIGHTHAND].sCount = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount;
						m_pUserData->m_sItemArray[RIGHTHAND].nSerialNum = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;
						if( m_pUserData->m_sItemArray[RIGHTHAND].nSerialNum == 0 )
							m_pUserData->m_sItemArray[RIGHTHAND].nSerialNum = m_pMain->GenerateItemSerial();
						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = m_pUserData->m_sItemArray[LEFTHAND].nNum; // ?????? ?????? ??????.
						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = m_pUserData->m_sItemArray[LEFTHAND].sDuration;
						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = m_pUserData->m_sItemArray[LEFTHAND].sCount;
						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pUserData->m_sItemArray[LEFTHAND].nSerialNum;
						if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum == 0 )
							m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pMain->GenerateItemSerial();
						m_pUserData->m_sItemArray[LEFTHAND].nNum = 0;			// ???? ?????? ????...
						m_pUserData->m_sItemArray[LEFTHAND].sDuration = 0;
						m_pUserData->m_sItemArray[LEFTHAND].sCount = 0;
						m_pUserData->m_sItemArray[LEFTHAND].nSerialNum = 0;
					}
					else {
						short duration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
						__int64 serial = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;

						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = m_pUserData->m_sItemArray[destpos].nNum;	// Swaping
						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = m_pUserData->m_sItemArray[destpos].sDuration;
						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = m_pUserData->m_sItemArray[destpos].sCount;
						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pUserData->m_sItemArray[destpos].nSerialNum;
						if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum != 0 && m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum == 0 )
							m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pMain->GenerateItemSerial();
						m_pUserData->m_sItemArray[destpos].nNum = itemid;
						m_pUserData->m_sItemArray[destpos].sDuration = duration;
						m_pUserData->m_sItemArray[destpos].sCount = 1;
						m_pUserData->m_sItemArray[destpos].nSerialNum = serial;
						if( m_pUserData->m_sItemArray[destpos].nSerialNum == 0 ) 
							m_pUserData->m_sItemArray[destpos].nSerialNum = m_pMain->GenerateItemSerial();
					}
				}
			}
			else {
				short duration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
				__int64 serial = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;

				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = m_pUserData->m_sItemArray[destpos].nNum;	// Swaping
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = m_pUserData->m_sItemArray[destpos].sDuration;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = m_pUserData->m_sItemArray[destpos].sCount;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pUserData->m_sItemArray[destpos].nSerialNum;
				if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum != 0 && m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum == 0 )
					m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pMain->GenerateItemSerial();
				m_pUserData->m_sItemArray[destpos].nNum = itemid;
				m_pUserData->m_sItemArray[destpos].sDuration = duration;
				m_pUserData->m_sItemArray[destpos].sCount = 1;
				m_pUserData->m_sItemArray[destpos].nSerialNum = serial;
				if( m_pUserData->m_sItemArray[destpos].nSerialNum == 0 )
					m_pUserData->m_sItemArray[destpos].nSerialNum = m_pMain->GenerateItemSerial();
			}
		}
		else if( pTable->m_bSlot == 0x02 || (pTable->m_bSlot == 0x00 && destpos == LEFTHAND) ) {	// ?????? ????(??? ?????? ??? ???????? ?g?? ???) ?e? ?????? ?µ???? ??? üu
			if(m_pUserData->m_sItemArray[RIGHTHAND].nNum != 0) {
				_ITEM_TABLE* pTable2 = m_pMain->m_ItemtableArray.GetData(m_pUserData->m_sItemArray[RIGHTHAND].nNum);
				if( pTable2 ) {
					if( pTable2->m_bSlot == 0x03 ) {
						m_pUserData->m_sItemArray[LEFTHAND].nNum = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum;
						m_pUserData->m_sItemArray[LEFTHAND].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
						m_pUserData->m_sItemArray[LEFTHAND].sCount = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount;
						m_pUserData->m_sItemArray[LEFTHAND].nSerialNum = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;
						if( m_pUserData->m_sItemArray[LEFTHAND].nSerialNum == 0 )
							m_pUserData->m_sItemArray[LEFTHAND].nSerialNum = m_pMain->GenerateItemSerial();
						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = m_pUserData->m_sItemArray[RIGHTHAND].nNum; // ???????? ?????? ??????.
						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = m_pUserData->m_sItemArray[RIGHTHAND].sDuration;
						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = m_pUserData->m_sItemArray[RIGHTHAND].sCount;
						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pUserData->m_sItemArray[RIGHTHAND].nSerialNum;
						if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum == 0 ) 
							m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pMain->GenerateItemSerial();
						m_pUserData->m_sItemArray[RIGHTHAND].nNum = 0;
						m_pUserData->m_sItemArray[RIGHTHAND].sDuration = 0;
						m_pUserData->m_sItemArray[RIGHTHAND].sCount = 0;
						m_pUserData->m_sItemArray[RIGHTHAND].nSerialNum = 0;
					}
					else {
						short duration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
						__int64 serial = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;

						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = m_pUserData->m_sItemArray[destpos].nNum;	// Swaping
						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = m_pUserData->m_sItemArray[destpos].sDuration;
						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = m_pUserData->m_sItemArray[destpos].sCount;
						m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pUserData->m_sItemArray[destpos].nSerialNum;
						if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum != 0 && m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum == 0 )
							m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pMain->GenerateItemSerial();
						m_pUserData->m_sItemArray[destpos].nNum = itemid;
						m_pUserData->m_sItemArray[destpos].sDuration = duration;
						m_pUserData->m_sItemArray[destpos].sCount = 1;
						m_pUserData->m_sItemArray[destpos].nSerialNum = serial;
						if( m_pUserData->m_sItemArray[destpos].nSerialNum == 0 )
							m_pUserData->m_sItemArray[destpos].nSerialNum = m_pMain->GenerateItemSerial();
					}
				}
			}
			else {
				short duration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
				__int64 serial = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;

				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = m_pUserData->m_sItemArray[destpos].nNum;	// Swaping
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = m_pUserData->m_sItemArray[destpos].sDuration;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = m_pUserData->m_sItemArray[destpos].sCount;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pUserData->m_sItemArray[destpos].nSerialNum;
				if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum != 0 && m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum == 0 )
					m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pMain->GenerateItemSerial();
				m_pUserData->m_sItemArray[destpos].nNum = itemid;
				m_pUserData->m_sItemArray[destpos].sDuration = duration;
				m_pUserData->m_sItemArray[destpos].sCount = 1;
				m_pUserData->m_sItemArray[destpos].nSerialNum = serial;
				if( m_pUserData->m_sItemArray[destpos].nSerialNum == 0 )
					m_pUserData->m_sItemArray[destpos].nSerialNum = m_pMain->GenerateItemSerial();
			}
		}
		else if( pTable->m_bSlot == 0x03 ) {	// ?µ? ?????? ????? ????
			if( m_pUserData->m_sItemArray[LEFTHAND].nNum != 0 && m_pUserData->m_sItemArray[RIGHTHAND].nNum != 0 )
				goto fail_return;
			else if( m_pUserData->m_sItemArray[LEFTHAND].nNum != 0 ) {
				m_pUserData->m_sItemArray[RIGHTHAND].nNum = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum;
				m_pUserData->m_sItemArray[RIGHTHAND].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
				m_pUserData->m_sItemArray[RIGHTHAND].sCount = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount;
				m_pUserData->m_sItemArray[RIGHTHAND].nSerialNum = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;
				if( m_pUserData->m_sItemArray[RIGHTHAND].nSerialNum == 0 )
					m_pUserData->m_sItemArray[RIGHTHAND].nSerialNum = m_pMain->GenerateItemSerial();
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = m_pUserData->m_sItemArray[LEFTHAND].nNum;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = m_pUserData->m_sItemArray[LEFTHAND].sDuration;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = m_pUserData->m_sItemArray[LEFTHAND].sCount;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pUserData->m_sItemArray[LEFTHAND].nSerialNum;
				if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum == 0 )
					m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pMain->GenerateItemSerial();
				m_pUserData->m_sItemArray[LEFTHAND].nNum = 0;
				m_pUserData->m_sItemArray[LEFTHAND].sDuration = 0;
				m_pUserData->m_sItemArray[LEFTHAND].sCount = 0;
				m_pUserData->m_sItemArray[LEFTHAND].nSerialNum = 0;
			}
			else {
				short duration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
				__int64 serial = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;

				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = m_pUserData->m_sItemArray[destpos].nNum;	// Swaping
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = m_pUserData->m_sItemArray[destpos].sDuration;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = m_pUserData->m_sItemArray[destpos].sCount;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pUserData->m_sItemArray[destpos].nSerialNum;
				if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum != 0 && m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum == 0 ) 
					m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pMain->GenerateItemSerial();
				m_pUserData->m_sItemArray[destpos].nNum = itemid;
				m_pUserData->m_sItemArray[destpos].sDuration = duration;
				m_pUserData->m_sItemArray[destpos].sCount = 1;
				m_pUserData->m_sItemArray[destpos].nSerialNum = serial;
				if( m_pUserData->m_sItemArray[destpos].nSerialNum == 0 )
					m_pUserData->m_sItemArray[destpos].nSerialNum = m_pMain->GenerateItemSerial();
			}
		}
		else if ( pTable->m_bSlot == 0x04 ) {	// ?µ? ?????? ??? ????
			if( m_pUserData->m_sItemArray[LEFTHAND].nNum != 0 && m_pUserData->m_sItemArray[RIGHTHAND].nNum != 0 )
				goto fail_return;
			else if( m_pUserData->m_sItemArray[RIGHTHAND].nNum != 0 ) {
				m_pUserData->m_sItemArray[LEFTHAND].nNum = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum;
				m_pUserData->m_sItemArray[LEFTHAND].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
				m_pUserData->m_sItemArray[LEFTHAND].sCount = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount;
				m_pUserData->m_sItemArray[LEFTHAND].nSerialNum = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;
				if( m_pUserData->m_sItemArray[LEFTHAND].nSerialNum == 0 )
					m_pUserData->m_sItemArray[LEFTHAND].nSerialNum = m_pMain->GenerateItemSerial();
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = m_pUserData->m_sItemArray[RIGHTHAND].nNum;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = m_pUserData->m_sItemArray[RIGHTHAND].sDuration;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = m_pUserData->m_sItemArray[RIGHTHAND].sCount;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pUserData->m_sItemArray[RIGHTHAND].nSerialNum;
				if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum == 0 )
					m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pMain->GenerateItemSerial();
				m_pUserData->m_sItemArray[RIGHTHAND].nNum = 0;
				m_pUserData->m_sItemArray[RIGHTHAND].sDuration = 0;
				m_pUserData->m_sItemArray[RIGHTHAND].sCount = 0;
				m_pUserData->m_sItemArray[RIGHTHAND].nSerialNum = 0;
			}
			else {
				short duration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
				__int64 serial = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;

				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = m_pUserData->m_sItemArray[destpos].nNum;	// Swaping
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = m_pUserData->m_sItemArray[destpos].sDuration;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = m_pUserData->m_sItemArray[destpos].sCount;
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pUserData->m_sItemArray[destpos].nSerialNum;
				if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum != 0 && m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum == 0 )
					m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pMain->GenerateItemSerial();
				m_pUserData->m_sItemArray[destpos].nNum = itemid;
				m_pUserData->m_sItemArray[destpos].sDuration = duration;
				m_pUserData->m_sItemArray[destpos].sCount = 1;
				m_pUserData->m_sItemArray[destpos].nSerialNum = serial;
				if( m_pUserData->m_sItemArray[destpos].nSerialNum == 0 )
					m_pUserData->m_sItemArray[destpos].nSerialNum = m_pMain->GenerateItemSerial();
			}
		}
		else {
			short duration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
			__int64 serial = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;

			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = m_pUserData->m_sItemArray[destpos].nNum;	// Swaping
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = m_pUserData->m_sItemArray[destpos].sDuration;
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = m_pUserData->m_sItemArray[destpos].sCount;
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pUserData->m_sItemArray[destpos].nSerialNum;
			if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum != 0 && m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum == 0 )
				m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pMain->GenerateItemSerial();
			m_pUserData->m_sItemArray[destpos].nNum = itemid;
			m_pUserData->m_sItemArray[destpos].sDuration = duration;
			m_pUserData->m_sItemArray[destpos].sCount = 1;
			m_pUserData->m_sItemArray[destpos].nSerialNum = serial;
			if( m_pUserData->m_sItemArray[destpos].nSerialNum == 0 )
				m_pUserData->m_sItemArray[destpos].nSerialNum = m_pMain->GenerateItemSerial();
		}
		break;

	case ITEM_SLOT_INVEN:
		if( itemid != m_pUserData->m_sItemArray[srcpos].nNum )
			goto fail_return;
		if( m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum != 0 )
			goto fail_return;

		m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum = m_pUserData->m_sItemArray[srcpos].nNum;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration = m_pUserData->m_sItemArray[srcpos].sDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].sCount = m_pUserData->m_sItemArray[srcpos].sCount;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum = m_pUserData->m_sItemArray[srcpos].nSerialNum;
		if( m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum == 0 ) 
			m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum = m_pMain->GenerateItemSerial();
		m_pUserData->m_sItemArray[srcpos].nNum = 0;
		m_pUserData->m_sItemArray[srcpos].sDuration = 0;
		m_pUserData->m_sItemArray[srcpos].sCount = 0;
		m_pUserData->m_sItemArray[srcpos].nSerialNum = 0;
		break;

	case ITEM_INVEN_INVEN:
		if( itemid != m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum )
			goto fail_return;
		{
			short duration = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration;
			short itemcount = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount;
			__int64 serial = m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum;
			_ITEM_TABLE* pTable2 = NULL;

			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum = m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum;
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration;
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].sCount = m_pUserData->m_sItemArray[SLOT_MAX+destpos].sCount;
			m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum;
			if( m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum == 0 ) {
				pTable2 = m_pMain->m_ItemtableArray.GetData(m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nNum);
				if( pTable && pTable->m_bCountable == 0 )
					m_pUserData->m_sItemArray[SLOT_MAX+srcpos].nSerialNum = m_pMain->GenerateItemSerial();
			}

			m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum = itemid;
			m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration = duration;
			m_pUserData->m_sItemArray[SLOT_MAX+destpos].sCount = itemcount;
			m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum = serial;
			if( m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum == 0 ) {
				pTable2 = m_pMain->m_ItemtableArray.GetData(m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum);
				if( pTable && pTable->m_bCountable == 0 )
					m_pUserData->m_sItemArray[SLOT_MAX+destpos].nSerialNum = m_pMain->GenerateItemSerial();
			}
		}
		break;

	case ITEM_SLOT_SLOT:
		if( itemid != m_pUserData->m_sItemArray[srcpos].nNum )
			goto fail_return;
		if( !IsValidSlotPos( pTable, destpos ) )
			goto fail_return;

		if( m_pUserData->m_sItemArray[destpos].nNum != 0 ) {
			_ITEM_TABLE* pTable2 = m_pMain->m_ItemtableArray.GetData( m_pUserData->m_sItemArray[destpos].nNum );	// dest slot exist some item
			if( pTable2 ) {
				if( pTable2->m_bSlot != 0x00 )
					goto fail_return;
				else {
					short duration = m_pUserData->m_sItemArray[srcpos].sDuration;
					short count = m_pUserData->m_sItemArray[srcpos].sCount;
					__int64 serial = m_pUserData->m_sItemArray[srcpos].nSerialNum;
					m_pUserData->m_sItemArray[srcpos].nNum = m_pUserData->m_sItemArray[destpos].nNum;				// Swaping
					m_pUserData->m_sItemArray[srcpos].sDuration = m_pUserData->m_sItemArray[destpos].sDuration;		// Swaping
					m_pUserData->m_sItemArray[srcpos].sCount = m_pUserData->m_sItemArray[destpos].sCount;			// Swaping
					m_pUserData->m_sItemArray[srcpos].nSerialNum = m_pUserData->m_sItemArray[destpos].nSerialNum;	// Swaping
					if( m_pUserData->m_sItemArray[srcpos].nSerialNum == 0 )
						m_pUserData->m_sItemArray[srcpos].nSerialNum = m_pMain->GenerateItemSerial();
					m_pUserData->m_sItemArray[destpos].nNum = itemid;
					m_pUserData->m_sItemArray[destpos].sDuration = duration;
					m_pUserData->m_sItemArray[destpos].sCount = count;
					m_pUserData->m_sItemArray[destpos].nSerialNum = serial;
					if( m_pUserData->m_sItemArray[destpos].nSerialNum == 0 )
						m_pUserData->m_sItemArray[destpos].nSerialNum = m_pMain->GenerateItemSerial();
				}
			}
		}
		else {
			short duration = m_pUserData->m_sItemArray[srcpos].sDuration;
			short count = m_pUserData->m_sItemArray[srcpos].sCount;
			__int64 serial = m_pUserData->m_sItemArray[srcpos].nSerialNum;
			m_pUserData->m_sItemArray[srcpos].nNum = m_pUserData->m_sItemArray[destpos].nNum;				// Swaping
			m_pUserData->m_sItemArray[srcpos].sDuration = m_pUserData->m_sItemArray[destpos].sDuration;		// Swaping
			m_pUserData->m_sItemArray[srcpos].sCount = m_pUserData->m_sItemArray[destpos].sCount;			// Swaping
			m_pUserData->m_sItemArray[srcpos].nSerialNum = m_pUserData->m_sItemArray[destpos].nSerialNum;	// Swaping
			m_pUserData->m_sItemArray[destpos].nNum = itemid;
			m_pUserData->m_sItemArray[destpos].sDuration = duration;
			m_pUserData->m_sItemArray[destpos].sCount = count;
			m_pUserData->m_sItemArray[destpos].nSerialNum = serial;
			if( m_pUserData->m_sItemArray[destpos].nSerialNum == 0 )
				m_pUserData->m_sItemArray[destpos].nSerialNum = m_pMain->GenerateItemSerial();
		}
		break;
	}

	if( dir != ITEM_INVEN_INVEN ) {	// ?????? ???? ????? ???..
		SetSlotItemValue();
		SetUserAbility();
	}

	SendItemMove();
	SendItemWeight();

	if( (dir == ITEM_INVEN_SLOT ) && ( destpos == HEAD || destpos == BREAST || destpos == SHOULDER || destpos == LEFTHAND || destpos == RIGHTHAND || destpos == LEG || destpos == GLOVE || destpos == FOOT) ) 
		UserLookChange( destpos, itemid, m_pUserData->m_sItemArray[destpos].sDuration );	
	if( (dir == ITEM_SLOT_INVEN ) && ( srcpos == HEAD || srcpos == BREAST || srcpos == SHOULDER || srcpos == LEFTHAND || srcpos == RIGHTHAND || srcpos == LEG || srcpos == GLOVE || srcpos == FOOT) ) 
		UserLookChange( srcpos, 0, 0 );	
	if( (dir == ITEM_INVEN_TO_COSP ) && ( SLOT_MAX+HAVE_MAX+destpos == CWING || SLOT_MAX+HAVE_MAX+destpos == CHELMET || SLOT_MAX+HAVE_MAX+destpos == CLEFT || SLOT_MAX+HAVE_MAX+destpos == CRIGHT || SLOT_MAX+HAVE_MAX+destpos == CTOP ) )
		UserLookChange( SLOT_MAX+HAVE_MAX+destpos, itemid, m_pUserData->m_sItemArray[SLOT_MAX+HAVE_MAX+destpos].sDuration );
	if( (dir == ITEM_COSP_TO_INVEN ) && ( SLOT_MAX+HAVE_MAX+srcpos == CWING || SLOT_MAX+HAVE_MAX+srcpos == CHELMET || SLOT_MAX+HAVE_MAX+srcpos == CLEFT || SLOT_MAX+HAVE_MAX+srcpos == CRIGHT || SLOT_MAX+HAVE_MAX+srcpos == CTOP ) )
		UserLookChange( SLOT_MAX+HAVE_MAX+srcpos, 0, 0 );

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
	_ITEM_TABLE* pTable = m_pMain->m_ItemtableArray.GetData( itemid );
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