#include "StdAfx.h"

using namespace std;

void CUser::ExchangeProcess(Packet & pkt)
{
	uint8 opcode = pkt.read<uint8>();
	switch (opcode)
	{
	case EXCHANGE_REQ:
		ExchangeReq(pkt);
		break;
	case EXCHANGE_AGREE:
		ExchangeAgree(pkt);
		break;
	case EXCHANGE_ADD:
		ExchangeAdd(pkt);
		break;
	case EXCHANGE_DECIDE:
		ExchangeDecide();
		break;
	case EXCHANGE_CANCEL:
		ExchangeCancel();
		break;
	}
}

void CUser::ExchangeReq(Packet & pkt)
{
	Packet result(WIZ_EXCHANGE);
	if (isDead())
		goto fail_return;
	else if (isTrading())
	{
		ExchangeCancel();
		return;
	}

	uint16 destid = pkt.read<uint16>();
	CUser* pUser = g_pMain->GetUserPtr(destid);
	if (pUser == NULL
		|| pUser->isTrading()
		|| (pUser->getNation() != getNation() && pUser->getZoneID() != 21 && getZoneID() != 21)
		|| pUser->getZoneID() != getZoneID())
		goto fail_return;

	m_sExchangeUser = destid;
	pUser->m_sExchangeUser = GetSocketID();

	result << uint8(EXCHANGE_REQ) << GetSocketID();
	pUser->Send(&result);
	return;

fail_return:
	result << uint8(EXCHANGE_CANCEL);
	Send(&result);
}

void CUser::ExchangeAgree(Packet & pkt)
{
	if (!isTrading())
		return;

	uint8 bResult = pkt.read<uint8>();
	CUser *pUser = g_pMain->GetUserPtr(m_sExchangeUser);
	if (pUser == NULL) 
	{
		m_sExchangeUser = -1;
		return;
	}

	if (!bResult) // declined
	{
		m_sExchangeUser = -1;
		pUser->m_sExchangeUser = -1;
	}
	else 
	{
		InitExchange(TRUE);
		pUser->InitExchange(TRUE);
	}

	Packet result(WIZ_EXCHANGE, uint8(EXCHANGE_AGREE));
	result << uint16(bResult);
	pUser->Send(&result);
}

void CUser::ExchangeAdd(Packet & pkt)
{
	if (!isTrading())
		return;

	int index = 0, send_index = 0, itemid = 0, duration = 0;
	unsigned int count = 0;
	CUser* pUser = NULL;
	_EXCHANGE_ITEM* pItem = NULL;
	_ITEM_TABLE* pTable = NULL;
	list<_EXCHANGE_ITEM*>::iterator	Iter;
	char buff[256];
	BYTE pos;
	BOOL bAdd = TRUE, bGold = FALSE;

	pUser = g_pMain->GetUserPtr(m_sExchangeUser);
	if (pUser == NULL)
	{
		ExchangeCancel();
		return;
	}

	pkt >> pos >> itemid >> count;
	pTable = g_pMain->GetItemPtr( itemid );
	if (pTable == NULL
		|| (itemid != ITEM_GOLD && pos >= HAVE_MAX)
		|| m_bExchangeOK)
		goto add_fail;

	if( itemid == ITEM_GOLD ) {
		if (count <= 0 || count > m_pUserData->m_iGold) goto add_fail;
		foreach (itr, m_ExchangeItemList)
		{
			if ((*itr)->itemid == ITEM_GOLD)
			{
				(*itr)->count += count;
				m_pUserData->m_iGold -= count;
				bAdd = FALSE;
				break;
			}
		}
		if( bAdd )
			m_pUserData->m_iGold -= count;
	}
	else if( m_MirrorItem[pos].nNum == itemid ) {
		if( m_MirrorItem[pos].sCount < count ) goto add_fail;
		if( pTable->m_bCountable ) {
			foreach (itr, m_ExchangeItemList)
			{
				if ((*itr)->itemid == itemid)
				{
					(*itr)->count += count;
					m_MirrorItem[pos].sCount -= count;
					bAdd = FALSE;
					break;
				}
			}
		}
		if( bAdd )
			m_MirrorItem[pos].sCount -= count;
	
		duration = m_MirrorItem[pos].sDuration;
		if( m_MirrorItem[pos].sCount <= 0 || pTable->m_bCountable == 0 ) {
			m_MirrorItem[pos].nNum = 0;
			m_MirrorItem[pos].sDuration = 0;
			m_MirrorItem[pos].sCount = 0;
			m_MirrorItem[pos].nSerialNum = 0;
		}
	}
	else
		goto add_fail;

	foreach (itr, m_ExchangeItemList)
	{
		if ((*itr)->itemid == ITEM_GOLD)
		{
			bGold = TRUE;
			break;
		}
	}
	if( (int)m_ExchangeItemList.size() > ( (bGold) ? 13 : 12 ) )
		goto add_fail;

	if( bAdd ) {		// Gold ?? ?????? ??????? ??´?..
		pItem = new _EXCHANGE_ITEM;
		pItem->itemid = itemid;
		pItem->duration = duration;
		pItem->count = count;
		pItem->nSerialNum = m_MirrorItem[pos].nSerialNum;
		m_ExchangeItemList.push_back(pItem);
	}

	SetByte( buff, WIZ_EXCHANGE, send_index );
	SetByte( buff, EXCHANGE_ADD, send_index );
	SetByte( buff, 0x01, send_index );
	Send( buff, send_index );
	
	send_index = 0;
	SetByte( buff, WIZ_EXCHANGE, send_index );
	SetByte( buff, EXCHANGE_OTHERADD, send_index );
	SetDWORD( buff, itemid, send_index );
	SetDWORD( buff, count, send_index );
	SetShort( buff, duration, send_index );
	pUser->Send( buff, send_index );

	return;

add_fail:
	SetByte( buff, WIZ_EXCHANGE, send_index );
	SetByte( buff, EXCHANGE_ADD, send_index );
	SetByte( buff, 0x00, send_index );
	Send( buff, send_index );
}

void CUser::ExchangeDecide()
{
	int send_index = 0, getmoney = 0, putmoney = 0;
	CUser* pUser = NULL;
	_EXCHANGE_ITEM* pItem = NULL;
	char buff[256];
	BOOL bSuccess = TRUE;

	pUser = g_pMain->GetUserPtr(m_sExchangeUser);
	if (pUser == NULL)
	{
		ExchangeCancel();
		return;
	}

	if( !pUser->m_bExchangeOK ) {
		m_bExchangeOK = 0x01;
		SetByte( buff, WIZ_EXCHANGE, send_index );
		SetByte( buff, EXCHANGE_OTHERDECIDE, send_index );
		pUser->Send( buff, send_index );
	}
	else {
		if( !ExecuteExchange() || !pUser->ExecuteExchange() ) {
			foreach (itr, m_ExchangeItemList)
			{
				if ((*itr)->itemid == ITEM_GOLD )
				{
					m_pUserData->m_iGold += (*itr)->count;
					break;
				}
			}
			
			foreach (itr, pUser->m_ExchangeItemList)
			{
				if ((*itr)->itemid == ITEM_GOLD)
				{
					pUser->m_pUserData->m_iGold += (*itr)->count;
					break;
				}
			}

			bSuccess = FALSE;
		}
		if( bSuccess ) {
			getmoney = ExchangeDone();
			putmoney = pUser->ExchangeDone();
			if( getmoney > 0 )
				ItemLogToAgent( m_pUserData->m_id, pUser->m_pUserData->m_id, ITEM_EXCHANGE_GET, 0, ITEM_GOLD, getmoney, 0 );
			if( putmoney > 0 )
				ItemLogToAgent( m_pUserData->m_id, pUser->m_pUserData->m_id, ITEM_EXCHANGE_PUT, 0, ITEM_GOLD, putmoney, 0 );
			
			SetByte( buff, WIZ_EXCHANGE, send_index );
			SetByte( buff, EXCHANGE_DONE, send_index );
			SetByte( buff, 0x01, send_index );
			SetDWORD( buff, m_pUserData->m_iGold, send_index );
			SetShort( buff, pUser->m_ExchangeItemList.size(), send_index );
			foreach (itr, pUser->m_ExchangeItemList)
			{
				SetByte( buff, (*itr)->pos, send_index );
				SetDWORD( buff, (*itr)->itemid, send_index );
				SetShort( buff, (*itr)->count, send_index );
				SetShort( buff, (*itr)->duration, send_index );

				ItemLogToAgent( m_pUserData->m_id, pUser->m_pUserData->m_id, ITEM_EXCHANGE_GET, (*itr)->nSerialNum, (*itr)->itemid, (*itr)->count, (*itr)->duration );
			}
			Send( buff, send_index );

			send_index = 0;
			SetByte( buff, WIZ_EXCHANGE, send_index );
			SetByte( buff, EXCHANGE_DONE, send_index );
			SetByte( buff, 0x01, send_index );
			SetDWORD( buff, pUser->m_pUserData->m_iGold, send_index );
			SetShort( buff, m_ExchangeItemList.size(), send_index );
			foreach (itr, m_ExchangeItemList)
			{
				SetByte( buff, (*itr)->pos, send_index );
				SetDWORD( buff, (*itr)->itemid, send_index );
				SetShort( buff, (*itr)->count, send_index );
				SetShort( buff, (*itr)->duration, send_index );

				ItemLogToAgent( m_pUserData->m_id, pUser->m_pUserData->m_id, ITEM_EXCHANGE_PUT, (*itr)->nSerialNum, (*itr)->itemid, (*itr)->count, (*itr)->duration );
			}
			pUser->Send( buff, send_index );

			SendItemWeight();
			pUser->SendItemWeight();
		}
		else {
			SetByte( buff, WIZ_EXCHANGE, send_index );
			SetByte( buff, EXCHANGE_DONE, send_index );
			SetByte( buff, 0x00, send_index );
			Send( buff, send_index );
			pUser->Send( buff, send_index );
		}
		InitExchange(FALSE);
		pUser->InitExchange(FALSE);
	}
}

void CUser::ExchangeCancel()
{
	int send_index = 0;
	char buff[256];
	CUser* pUser = NULL;
	BOOL bFind = TRUE;

	if (!isTrading())
		return;

	pUser = g_pMain->GetUserPtr(m_sExchangeUser);
	if (pUser == NULL) 
		bFind = FALSE;

	foreach (itr, m_ExchangeItemList)
	{
		if ((*itr)->itemid == ITEM_GOLD)
		{
			m_pUserData->m_iGold += (*itr)->count;
			break;
		}
	}

	InitExchange(FALSE);

	if( bFind ) {
		pUser->ExchangeCancel();
		SetByte( buff, WIZ_EXCHANGE, send_index );
		SetByte( buff, EXCHANGE_CANCEL, send_index );
		pUser->Send( buff, send_index );
	}
}

void CUser::InitExchange(BOOL bStart)
{
	_EXCHANGE_ITEM* pItem = NULL;

	while(m_ExchangeItemList.size()) {
		pItem = m_ExchangeItemList.front();
		if( pItem )
			delete pItem;
		m_ExchangeItemList.pop_front();
	}
	m_ExchangeItemList.clear();

	if( bStart ) {						// ??? ????? ???
		for(int i=0; i<HAVE_MAX; i++ ) {
			m_MirrorItem[i].nNum = m_pUserData->m_sItemArray[SLOT_MAX+i].nNum;
			m_MirrorItem[i].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+i].sDuration;
			m_MirrorItem[i].sCount = m_pUserData->m_sItemArray[SLOT_MAX+i].sCount;
			m_MirrorItem[i].nSerialNum = m_pUserData->m_sItemArray[SLOT_MAX+i].nSerialNum;
		}
	}
	else {								// ??? ???? U????
		m_sExchangeUser = -1;
		m_bExchangeOK = 0x00;
		for(int i=0; i<HAVE_MAX; i++ ) {
			m_MirrorItem[i].nNum = 0;
			m_MirrorItem[i].sDuration = 0;
			m_MirrorItem[i].sCount = 0;
			m_MirrorItem[i].nSerialNum = 0;
		}
	}
}

BOOL CUser::ExecuteExchange()
{
	_ITEM_TABLE* pTable = NULL;
	CUser* pUser = NULL;
	DWORD money = 0;
	short weight = 0;
	BYTE i = 0;

	pUser = g_pMain->GetUserPtr(m_sExchangeUser);
	if (pUser == NULL) return FALSE;

	foreach (Iter, pUser->m_ExchangeItemList)
	{
		if( (*Iter)->itemid >= ITEM_NO_TRADE)
			return FALSE;
		else if( (*Iter)->itemid == ITEM_GOLD ) {
			money = (*Iter)->count;
		}
		else {
			pTable = g_pMain->GetItemPtr( (*Iter)->itemid );
			if( !pTable ) continue;
			for (i=0; i<HAVE_MAX; i++ ) {
				if( m_MirrorItem[i].nNum == 0  && pTable->m_bCountable == 0 ) {
					m_MirrorItem[i].nNum = (*Iter)->itemid;
					m_MirrorItem[i].sDuration = (*Iter)->duration;
					m_MirrorItem[i].sCount = (*Iter)->count;
					m_MirrorItem[i].nSerialNum = (*Iter)->nSerialNum;
					(*Iter)->pos = i;	
					weight += pTable->m_sWeight;
					break;
				}
				else if( m_MirrorItem[i].nNum == (*Iter)->itemid && pTable->m_bCountable == 1 ) {			
					m_MirrorItem[i].sCount += (*Iter)->count;
					if( m_MirrorItem[i].sCount > MAX_ITEM_COUNT )
						m_MirrorItem[i].sCount = MAX_ITEM_COUNT;
					weight += ( pTable->m_sWeight * (*Iter)->count );
					(*Iter)->pos = i;	
					break;
				}
			}

			if( i == HAVE_MAX && pTable->m_bCountable == 1 ) {
				for( i=0; i<HAVE_MAX; i++ ) {
					if( m_MirrorItem[i].nNum == 0 ) {
						m_MirrorItem[i].nNum = (*Iter)->itemid;
						m_MirrorItem[i].sDuration = (*Iter)->duration;
						m_MirrorItem[i].sCount = (*Iter)->count;
						(*Iter)->pos = i;
						weight += ( pTable->m_sWeight * (*Iter)->count );
						break;
					}
				}
			}
			
			if( Iter != pUser->m_ExchangeItemList.end() && i == HAVE_MAX )
				return FALSE;
		}
	}

	if( (weight + m_sItemWeight) > m_sMaxWeight ) return FALSE;		// Too much weight! 

	return TRUE;
}

int CUser::ExchangeDone()
{
	int money = 0;
	CUser* pUser = NULL;
	_ITEM_TABLE* pTable = NULL;

	pUser = g_pMain->GetUserPtr(m_sExchangeUser);
	if (pUser == NULL)
		return 0;

	// TO-DO: Clean this up. Not sure why the coin entry is the only thing being cleaned up.
	// It's cleaned up properly in InitExchange()... it's no wonder this system's so abusable.
	// The mirror item setup is also confusing when there's the existing exchange list.
	foreach (itr, pUser->m_ExchangeItemList)
	{
		if ((*itr)->itemid != ITEM_GOLD)
			continue;

		money = (*itr)->count;
		delete (*itr);
		pUser->m_ExchangeItemList.erase(itr);
		break;
	}
	
	if( money > 0 ) 
		m_pUserData->m_iGold += money;
	for( int i=0; i<HAVE_MAX; i++ ) {
		m_pUserData->m_sItemArray[SLOT_MAX+i].nNum = m_MirrorItem[i].nNum;
		m_pUserData->m_sItemArray[SLOT_MAX+i].sDuration = m_MirrorItem[i].sDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+i].sCount = m_MirrorItem[i].sCount;
		m_pUserData->m_sItemArray[SLOT_MAX+i].nSerialNum = m_MirrorItem[i].nSerialNum;

		pTable = g_pMain->GetItemPtr(m_pUserData->m_sItemArray[SLOT_MAX+i].nNum);
		if( !pTable ) continue;
		if( pTable->m_bCountable == 0 && m_pUserData->m_sItemArray[SLOT_MAX+i].nSerialNum == 0 )
			m_pUserData->m_sItemArray[SLOT_MAX+i].nSerialNum = g_pMain->GenerateItemSerial();
	}

	return money;
}