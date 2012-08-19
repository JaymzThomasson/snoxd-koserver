#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "Ebenezer.h"
#include "EbenezerDlg.h"
#include "User.h"

void CUser::ExchangeProcess(char *pBuf)
{
	int index = 0;
	BYTE subcommand = GetByte( pBuf, index );

	switch( subcommand ) {
	case EXCHANGE_REQ:
		ExchangeReq( pBuf+index );
		break;
	case EXCHANGE_AGREE:
		ExchangeAgree( pBuf+index );
		break;
	case EXCHANGE_ADD:
		ExchangeAdd( pBuf+index );
		break;
	case EXCHANGE_DECIDE:
		ExchangeDecide();
		break;
	case EXCHANGE_CANCEL:
		ExchangeCancel();
		break;
	}
}

void CUser::ExchangeReq(char *pBuf)
{
	int index = 0, destid = -1, send_index = 0, type = 0;
	CUser* pUser = NULL;
	char buff[256];	memset( buff, 0x00, 256 );

	if (isDead())
	{
		TRACE("### ExchangeProcess Fail : name=%s(%d), m_bResHpType=%d, hp=%d, x=%d, z=%d ###\n", m_pUserData->m_id, m_Sid, m_bResHpType, m_pUserData->m_sHp, (int)m_pUserData->m_curx, (int)m_pUserData->m_curz);
		goto fail_return;
	}

	destid = GetShort( pBuf, index );
	pUser = m_pMain->GetUserPtr(destid);
	if (pUser == NULL
		|| pUser->m_sExchangeUser != -1
		|| pUser->getNation() != getNation())
		goto fail_return;

	m_sExchangeUser = destid;
	pUser->m_sExchangeUser = m_Sid;

	SetByte( buff, WIZ_EXCHANGE, send_index );
	SetByte( buff, EXCHANGE_REQ, send_index );
	SetShort( buff, m_Sid, send_index );
	pUser->Send( buff, send_index );
	
	return;

fail_return:
	SetByte( buff, WIZ_EXCHANGE, send_index );
	SetByte( buff, EXCHANGE_CANCEL, send_index );
	Send( buff, send_index );
}

void CUser::ExchangeAgree(char* pBuf)
{
	int index = 0, destid = -1, send_index = 0;
	CUser* pUser = NULL;
	char buff[256];	memset( buff, 0x00, 256 );

	BYTE result = GetByte( pBuf, index );

	pUser = m_pMain->GetUserPtr(m_sExchangeUser);
	if (pUser == NULL) 
	{
		m_sExchangeUser = -1;
		return;
	}

	if (result == 0x00) // declined
	{
		m_sExchangeUser = -1;
		pUser->m_sExchangeUser = -1;
	}
	else 
	{
		InitExchange(TRUE);
		pUser->InitExchange(TRUE);
	}

	SetByte( buff, WIZ_EXCHANGE, send_index );
	SetByte( buff, EXCHANGE_AGREE, send_index );
	SetShort( buff, result, send_index );
	pUser->Send( buff, send_index );
}

void CUser::ExchangeAdd(char *pBuf)
{
	int index = 0, send_index = 0, count = 0, itemid = 0, duration = 0;
	CUser* pUser = NULL;
	_EXCHANGE_ITEM* pItem = NULL;
	_ITEM_TABLE* pTable = NULL;
	list<_EXCHANGE_ITEM*>::iterator	Iter;
	char buff[256];	memset( buff, 0x00, 256 );
	BYTE pos;
	BOOL bAdd = TRUE, bGold = FALSE;

	pUser = m_pMain->GetUserPtr(m_sExchangeUser);
	if (pUser == NULL)
	{
		ExchangeCancel();
		return;
	}

	pos = GetByte( pBuf, index );
	itemid = GetDWORD( pBuf, index );
	count = GetDWORD( pBuf, index );
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
	if( !pTable )
		goto add_fail;
	if( itemid != ITEM_GOLD && pos >= HAVE_MAX )
		goto add_fail;
	if( m_bExchangeOK )
		goto add_fail;
	if( itemid == ITEM_GOLD ) {
		if( count > m_pUserData->m_iGold ) goto add_fail;
		if( count <= 0 ) goto add_fail;
		for( Iter = m_ExchangeItemList.begin(); Iter != m_ExchangeItemList.end(); Iter++ ) {
			if( (*Iter)->itemid == ITEM_GOLD ) {
				(*Iter)->count += count;
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
		if( pTable->m_bCountable ) {		// ?????? ??????
			for( Iter = m_ExchangeItemList.begin(); Iter != m_ExchangeItemList.end(); Iter++ ) {
				if( (*Iter)->itemid == itemid ) {
					(*Iter)->count += count;
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
	for( Iter = m_ExchangeItemList.begin(); Iter != m_ExchangeItemList.end(); Iter++ ) {
		if( (*Iter)->itemid == ITEM_GOLD ) {
			bGold = TRUE;
			break;
		}
	}
	if( m_ExchangeItemList.size() > ( (bGold) ? 13 : 12 ) )
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
	char buff[256];	memset( buff, 0x00, 256 );
	BOOL bSuccess = TRUE;

	pUser = m_pMain->GetUserPtr(m_sExchangeUser);
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
	else {										// ??? ?????? ???
		list<_EXCHANGE_ITEM*>::iterator	Iter;
		if( !ExecuteExchange() || !pUser->ExecuteExchange() ) {				// ??? ????
			for( Iter = m_ExchangeItemList.begin(); Iter != m_ExchangeItemList.end(); Iter++ ) {
				if( (*Iter)->itemid == ITEM_GOLD ) {
					m_pUserData->m_iGold += (*Iter)->count;		// ???? ???
					break;
				}
			}
			
			for( Iter = pUser->m_ExchangeItemList.begin(); Iter != pUser->m_ExchangeItemList.end(); Iter++ ) {
				if( (*Iter)->itemid == ITEM_GOLD ) {
					pUser->m_pUserData->m_iGold += (*Iter)->count;		// ???? ???
					break;
				}
			}

			bSuccess = FALSE;
		}
		if( bSuccess ) {
			getmoney = ExchangeDone();						// ??? ?????? ???...
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
			for( Iter = pUser->m_ExchangeItemList.begin(); Iter != pUser->m_ExchangeItemList.end(); Iter++ ) {
				SetByte( buff, (*Iter)->pos, send_index );		// ??? ??? ????? ?g
				SetDWORD( buff, (*Iter)->itemid, send_index );
				SetShort( buff, (*Iter)->count, send_index );
				SetShort( buff, (*Iter)->duration, send_index );

				ItemLogToAgent( m_pUserData->m_id, pUser->m_pUserData->m_id, ITEM_EXCHANGE_GET, (*Iter)->nSerialNum, (*Iter)->itemid, (*Iter)->count, (*Iter)->duration );
			}
			Send( buff, send_index );		// ?????? ??????...

			memset( buff, 0x00, 256 ); send_index = 0;
			SetByte( buff, WIZ_EXCHANGE, send_index );
			SetByte( buff, EXCHANGE_DONE, send_index );
			SetByte( buff, 0x01, send_index );
			SetDWORD( buff, pUser->m_pUserData->m_iGold, send_index );
			SetShort( buff, m_ExchangeItemList.size(), send_index );
			for( Iter = m_ExchangeItemList.begin(); Iter != m_ExchangeItemList.end(); Iter++ ) {
				SetByte( buff, (*Iter)->pos, send_index );		// ??? ??? ????? ?g
				SetDWORD( buff, (*Iter)->itemid, send_index );
				SetShort( buff, (*Iter)->count, send_index );
				SetShort( buff, (*Iter)->duration, send_index );

				ItemLogToAgent( m_pUserData->m_id, pUser->m_pUserData->m_id, ITEM_EXCHANGE_PUT, (*Iter)->nSerialNum, (*Iter)->itemid, (*Iter)->count, (*Iter)->duration );
			}
			pUser->Send( buff, send_index );	// ???? ???????. 

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
	char buff[256];	memset( buff, 0x00, 256 );
	CUser* pUser = NULL;
	BOOL bFind = TRUE;

	pUser = m_pMain->GetUserPtr(m_sExchangeUser);
	if (pUser == NULL) bFind = FALSE;

	list<_EXCHANGE_ITEM*>::iterator	Iter;
	for( Iter = m_ExchangeItemList.begin(); Iter != m_ExchangeItemList.end(); Iter++ ) {
		if( (*Iter)->itemid == ITEM_GOLD ) {
			m_pUserData->m_iGold += (*Iter)->count;		// ???? ???
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
	short weight = 0, i=0;

	pUser = m_pMain->GetUserPtr(m_sExchangeUser);
	if( !pUser ) return FALSE;

	list<_EXCHANGE_ITEM*>::iterator	Iter;
	int iCount = pUser->m_ExchangeItemList.size(); 
	for( Iter = pUser->m_ExchangeItemList.begin(); Iter != pUser->m_ExchangeItemList.end(); Iter++ ) {

		if( (*Iter)->itemid >= ITEM_NO_TRADE)
			return FALSE;
		else if( (*Iter)->itemid == ITEM_GOLD ) {
//
//		if( (*Iter)->itemid == ITEM_GOLD ) {
			money = (*Iter)->count;
		}
		else {
			pTable = m_pMain->m_ItemtableArray.GetData( (*Iter)->itemid );
			if( !pTable ) continue;
			for( i=0; i<HAVE_MAX; i++ ) {
				if( m_MirrorItem[i].nNum == 0  && pTable->m_bCountable == 0 ) {  // ????? ???? ??????!!!
					m_MirrorItem[i].nNum = (*Iter)->itemid;
					m_MirrorItem[i].sDuration = (*Iter)->duration;
					m_MirrorItem[i].sCount = (*Iter)->count;
					m_MirrorItem[i].nSerialNum = (*Iter)->nSerialNum;
					(*Iter)->pos = i;							// ??Y?? ??????...
					weight += pTable->m_sWeight;
					break;
				}
				else if( m_MirrorItem[i].nNum == (*Iter)->itemid && pTable->m_bCountable == 1 ) {	// ????? ??????!!!				
					m_MirrorItem[i].sCount += (*Iter)->count;
					if( m_MirrorItem[i].sCount > MAX_ITEM_COUNT )
						m_MirrorItem[i].sCount = MAX_ITEM_COUNT;
					weight += ( pTable->m_sWeight * (*Iter)->count );
					(*Iter)->pos = i;							// ??Y?? ??????...
					break;
				}
			}

			if( i == HAVE_MAX && pTable->m_bCountable == 1 ) {	// ??? ??? ???????e? ????? ????? ???? ??? ??? ??? ???			
				for( i=0; i<HAVE_MAX; i++ ) {
					if( m_MirrorItem[i].nNum == 0 ) {
						m_MirrorItem[i].nNum = (*Iter)->itemid;
						m_MirrorItem[i].sDuration = (*Iter)->duration;
						m_MirrorItem[i].sCount = (*Iter)->count;
						(*Iter)->pos = i;							// ??Y?? ??????...
						weight += ( pTable->m_sWeight * (*Iter)->count );
						break;
					}
				}
			}
			
			if( Iter != pUser->m_ExchangeItemList.end() && i == HAVE_MAX )
				return FALSE;		// ????? ?? ???...
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

	pUser = m_pMain->GetUserPtr(m_sExchangeUser);
	if (pUser == NULL)
		return 0;

	list<_EXCHANGE_ITEM*>::iterator	Iter;
	for( Iter = pUser->m_ExchangeItemList.begin(); Iter != pUser->m_ExchangeItemList.end(); ) {
		if( (*Iter)->itemid == ITEM_GOLD ) {
			money = (*Iter)->count;
			delete (*Iter);
			Iter = pUser->m_ExchangeItemList.erase(Iter);
			continue;
		}

		Iter++;
	}
	
	if( money > 0 ) 
		m_pUserData->m_iGold += money;		// ?????? ?? ??.
	for( int i=0; i<HAVE_MAX; i++ ) {			// ????? ???????..
		m_pUserData->m_sItemArray[SLOT_MAX+i].nNum = m_MirrorItem[i].nNum;
		m_pUserData->m_sItemArray[SLOT_MAX+i].sDuration = m_MirrorItem[i].sDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+i].sCount = m_MirrorItem[i].sCount;
		m_pUserData->m_sItemArray[SLOT_MAX+i].nSerialNum = m_MirrorItem[i].nSerialNum;

		pTable = m_pMain->m_ItemtableArray.GetData(m_pUserData->m_sItemArray[SLOT_MAX+i].nNum);
		if( !pTable ) continue;
		if( pTable->m_bCountable == 0 && m_pUserData->m_sItemArray[SLOT_MAX+i].nSerialNum == 0 )
			m_pUserData->m_sItemArray[SLOT_MAX+i].nSerialNum = m_pMain->GenerateItemSerial();
	}

	return money;
}