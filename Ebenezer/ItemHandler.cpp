#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"

void CUser::WarehouseProcess(char *pBuf)
{
	int index = 0, send_index = 0, itemid = 0, srcpos = -1, destpos = -1, page = -1, reference_pos = -1;
	DWORD count = 0;
	char send_buff[2048];
	_ITEM_TABLE* pTable = NULL;
	BYTE command = 0;
	command = GetByte( pBuf, index );

	// â?? ??j?...
	if (isDead())
	{
		TRACE("### WarehouseProcess Fail : name=%s(%d), m_bResHpType=%d, hp=%d, x=%d, z=%d ###\n", m_pUserData->m_id, m_Sid, m_bResHpType, m_pUserData->m_sHp, (int)m_pUserData->m_curx, (int)m_pUserData->m_curz);
		return;
	}
	if( m_sExchangeUser != -1 ) goto fail_return;

	if( command == WAREHOUSE_OPEN )	{
		SetByte( send_buff, WIZ_WAREHOUSE, send_index );
		SetByte( send_buff, WAREHOUSE_OPEN, send_index );
		SetDWORD( send_buff, m_pUserData->m_iBank, send_index );
		for(int i=0; i<WAREHOUSE_MAX; i++ ) {
			SetDWORD( send_buff, m_pUserData->m_sWarehouseArray[i].nNum, send_index );
			SetShort( send_buff, m_pUserData->m_sWarehouseArray[i].sDuration, send_index );
			SetShort( send_buff, m_pUserData->m_sWarehouseArray[i].sCount, send_index );
		}
		SendCompressingPacket( send_buff, send_index );	
		return;
	}

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

	SetByte( send_buff, WIZ_WAREHOUSE, send_index );
	SetByte( send_buff, command, send_index );
	SetByte( send_buff, 0x01, send_index );
	Send( send_buff, send_index );
	return;
fail_return:
	SetByte( send_buff, WIZ_WAREHOUSE, send_index );
	SetByte( send_buff, command, send_index );
	SetByte( send_buff, 0x00, send_index );
	Send( send_buff, send_index );
}



BOOL CUser::CheckWeight(int itemid, short count)
{
	_ITEM_TABLE* pTable = NULL;
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
	if (!pTable) return FALSE;

	if (!pTable->m_bCountable) {
		if ((m_sItemWeight + pTable->m_sWeight ) <= m_sMaxWeight) {		// Check weight first!
			if (GetEmptySlot(itemid, 0) != 0xFF) {		// Now check empty slots :P
				return TRUE;
			}
			else {
				return FALSE;
			}
		}
		else {
			return FALSE;
		}
	}
	else {
		if (((pTable->m_sWeight * count) + m_sItemWeight) <= m_sMaxWeight) {	// Check weight first!
			if (GetEmptySlot(itemid, pTable->m_bCountable) != 0xFF) {	// Now check empty slots :P
				return TRUE;
			}
			else {
				return FALSE;
			}			
		}
		else {
			return FALSE;
		}
	}

	return FALSE;
}

BOOL CUser::CheckExistItem(int itemid, short count)
{
	_ITEM_TABLE* pTable = NULL;				// This checks if such an item exists.
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
	if( !pTable ) return FALSE;	

	for ( int i = 0 ; i < SLOT_MAX + HAVE_MAX ; i++ ) {		// Check every slot in this case.....
		if( m_pUserData->m_sItemArray[i].nNum == itemid ) {		
			if (!pTable->m_bCountable) {	// Non-countable item. Automatically return TRUE				
				return TRUE;
			}
			else {
				if (m_pUserData->m_sItemArray[i].sCount >= count) {	// Countable items. Make sure the amount is 
					return TRUE;                                    // same or higher.
				}
				else {
					return FALSE;
				}
			}
		}
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
	int pos = 255;
	int send_index = 0 ;					
	char send_buff[128];

	_ITEM_TABLE* pTable = NULL;				// This checks if such an item exists.
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
	if( !pTable ) return FALSE;	
	
	pos = GetEmptySlot( itemid, pTable->m_bCountable );

	if( pos != 0xFF ) {	// Common Item
		if( pos >= HAVE_MAX ) return FALSE;

		if( m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum != 0 ) {	
			if( pTable->m_bCountable != 1) return FALSE;
			else if( m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum != itemid ) return FALSE;
		}
/*	
		if (pTable->m_bCountable) {	// Check weight of countable item.
			if (((pTable->m_sWeight * count) + m_sItemWeight) > m_sMaxWeight) {			
				return FALSE;
			}
		}
		else {	// Check weight of non-countable item.
			if ((pTable->m_sWeight + m_sItemWeight) > m_sMaxWeight) {
				return FALSE;
			}
		}
*/
		m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum = itemid;	// Add item to inventory. 

		if( pTable->m_bCountable) {	// Apply number of items to a countable item.
			m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount += count;
			if( m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount > MAX_ITEM_COUNT ) {
				m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount = MAX_ITEM_COUNT;
			}
		}
		else {		// Just add uncountable item to inventory.
			m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount = 1;
		}
		
		m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration = pTable->m_sDuration;	// Apply duration to item.
	}
	else {
		return FALSE;	// No empty slots.
	}


	SendItemWeight();	// Change weight first -- do this regardless.
	if (send_packet)
	{
		SetByte( send_buff, WIZ_ITEM_COUNT_CHANGE, send_index );	
		SetShort( send_buff, 0x01, send_index );	// The number of for-loops
		SetByte( send_buff, 0x01, send_index );
		SetByte( send_buff, pos, send_index );
		SetDWORD( send_buff, itemid, send_index );	// The ID of item.
		SetDWORD( send_buff, m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount, send_index );
		Send( send_buff, send_index );
	}
	return TRUE;
}

void CUser::ItemLogToAgent(const char *srcid, const char *tarid, int type, __int64 serial, int itemid, int count, int durability)
{
	// NOTE: item log implementation removed, keeping this method in for future reworkings
}

void CUser::SendItemWeight()
{
	int send_index = 0 ;
	char send_buff[3];

	SetSlotItemValue();
	SetByte( send_buff, WIZ_WEIGHT_CHANGE, send_index );
	SetShort( send_buff, m_sItemWeight, send_index );
	Send(send_buff, send_index);
}


BOOL CUser::ItemEquipAvailable(_ITEM_TABLE *pTable)
{
	if( !pTable )
		return FALSE;
//	if( pTable->m_bReqLevel > m_pUserData->m_bLevel )
//		return FALSE;
	if( pTable->m_bReqRank > m_pUserData->m_bRank )
		return FALSE;
	if( pTable->m_bReqTitle > m_pUserData->m_bTitle )
		return FALSE;
	if( pTable->m_bReqStr > m_pUserData->m_bStr )
		return FALSE;
	if( pTable->m_bReqSta > m_pUserData->m_bSta )
		return FALSE;
	if( pTable->m_bReqDex > m_pUserData->m_bDex )
		return FALSE;
	if( pTable->m_bReqIntel > m_pUserData->m_bIntel )
		return FALSE;
	if( pTable->m_bReqCha > m_pUserData->m_bCha )
		return FALSE;

	return TRUE;
}

void CUser::ItemMove(char *pBuf)
{
	int index = 0, itemid = 0, srcpos = -1, destpos = -1, send_index = 0;
	char send_buff[128];
	_ITEM_TABLE* pTable = NULL;
	BYTE dir;

	dir = GetByte( pBuf, index );
	itemid = GetDWORD( pBuf, index );
	srcpos = GetByte( pBuf, index );
	destpos = GetByte( pBuf, index );

	if( m_sExchangeUser != -1 ) goto fail_return;
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
	if( !pTable )
		goto fail_return;
//	if( dir == ITEM_INVEN_SLOT && ((pTable->m_sWeight + m_sItemWeight) > m_sMaxWeight) )
//		goto fail_return;
	if( dir > 0x04 || srcpos >= SLOT_MAX+HAVE_MAX || destpos >= SLOT_MAX+HAVE_MAX )
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
/*
	SetByte( send_buff, WIZ_ITEM_MOVE, send_index );
	SetByte( send_buff, 0x01, send_index );
	SetShort( send_buff, m_sTotalHit, send_index );
	SetShort( send_buff, m_sTotalAc, send_index );
	SetShort( send_buff, m_sMaxWeight, send_index );
	SetShort( send_buff, m_iMaxHp, send_index );
	SetShort( send_buff, m_iMaxMp, send_index );
	SetByte( send_buff, m_sItemStr, send_index );
	SetByte( send_buff, m_sItemSta, send_index );
	SetByte( send_buff, m_sItemDex, send_index );
	SetByte( send_buff, m_sItemIntel, send_index );
	SetByte( send_buff, m_sItemCham, send_index );
	SetByte( send_buff, m_bFireR, send_index );
	SetByte( send_buff, m_bColdR, send_index );
	SetByte( send_buff, m_bLightningR, send_index );
	SetByte( send_buff, m_bMagicR, send_index );
	SetByte( send_buff, m_bDiseaseR, send_index );
	SetByte( send_buff, m_bPoisonR, send_index );
	Send( send_buff, send_index );

	BYTE	m_bStrAmount;
	BYTE	m_bStaAmount;
	BYTE	m_bDexAmount;
	BYTE	m_bIntelAmount;
	BYTE	m_bChaAmount;
*/

	SetByte( send_buff, WIZ_ITEM_MOVE, send_index );
	SetByte( send_buff, 0x01, send_index );
	SetShort( send_buff, m_sTotalHit, send_index );
	SetShort( send_buff, m_sTotalAc, send_index );
	SetShort( send_buff, m_sMaxWeight, send_index );
	SetShort( send_buff, m_iMaxHp, send_index );
	SetShort( send_buff, m_iMaxMp, send_index );
	SetByte( send_buff, m_sItemStr + m_bStrAmount, send_index );		
	SetByte( send_buff, m_sItemSta + m_bStaAmount, send_index );		
	SetByte( send_buff, m_sItemDex + m_bDexAmount, send_index );		
	SetByte( send_buff, m_sItemIntel + m_bIntelAmount, send_index );	
	SetByte( send_buff, m_sItemCham + m_bChaAmount, send_index );		
	SetByte( send_buff, m_bFireR, send_index );
	SetByte( send_buff, m_bColdR, send_index );
	SetByte( send_buff, m_bLightningR, send_index );
	SetByte( send_buff, m_bMagicR, send_index );
	SetByte( send_buff, m_bDiseaseR, send_index );
	SetByte( send_buff, m_bPoisonR, send_index );
	Send( send_buff, send_index );
//
	SendItemWeight();

	if( (dir == ITEM_INVEN_SLOT ) && ( destpos == HEAD || destpos == BREAST || destpos == SHOULDER || destpos == LEFTHAND || destpos == RIGHTHAND || destpos == LEG || destpos == GLOVE || destpos == FOOT) ) 
		UserLookChange( destpos, itemid, m_pUserData->m_sItemArray[destpos].sDuration );	
	if( (dir == ITEM_SLOT_INVEN ) && ( srcpos == HEAD || srcpos == BREAST || srcpos == SHOULDER || srcpos == LEFTHAND || srcpos == RIGHTHAND || srcpos == LEG || srcpos == GLOVE || srcpos == FOOT) ) 
		UserLookChange( srcpos, 0, 0 );	

	Send2AI_UserUpdateInfo();

	return;

fail_return:
	send_index = 0;
	SetByte( send_buff, WIZ_ITEM_MOVE, send_index );
	SetByte( send_buff, 0x00, send_index );
	Send( send_buff, send_index );
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
	}

	return TRUE;
}

BOOL CUser::CheckItemCount(int itemid, short min, short max)
{
	_ITEM_TABLE* pTable = NULL;				// This checks if such an item exists.
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
	if( !pTable ) return FALSE;	

	for ( int i = 0 ; i < SLOT_MAX + HAVE_MAX ; i++ ) {		// Check every slot in this case.....
		if( m_pUserData->m_sItemArray[i].nNum == itemid ) {		
			if (!pTable->m_bCountable) {	// Non-countable item. Automatically return TRUE				
				return FALSE;	// Let's return false in this case.
			}
			else {
				if (m_pUserData->m_sItemArray[i].sCount >= min && m_pUserData->m_sItemArray[i].sCount <= max) {	// Countable items. Make sure the amount is 
					return TRUE;                                    // same or higher.
				}
				else {
					return FALSE;
				}
			}
		}
	}

	return FALSE;		
}