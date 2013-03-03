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
		|| (pUser->GetNation() != GetNation() && pUser->GetZoneID() != 21 && GetZoneID() != 21)
		|| pUser->GetZoneID() != GetZoneID())
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
	if (!isTrading()
		|| isDead())
		return;

	uint8 bResult = pkt.read<uint8>();
	CUser *pUser = g_pMain->GetUserPtr(m_sExchangeUser);
	if (pUser == NULL) 
	{
		m_sExchangeUser = -1;
		return;
	}

	if (!bResult || pUser->isDead()) // declined
	{
		m_sExchangeUser = -1;
		pUser->m_sExchangeUser = -1;
		bResult = 0;
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

	Packet result(WIZ_EXCHANGE, uint8(EXCHANGE_ADD));
	uint64 nSerialNum;
	uint32 nItemID, count = 0;
	uint16 duration = 0;
	_EXCHANGE_ITEM* pItem = NULL;
	list<_EXCHANGE_ITEM*>::iterator	Iter;
	BYTE pos;
	BOOL bAdd = TRUE, bGold = FALSE;

	CUser *pUser = g_pMain->GetUserPtr(m_sExchangeUser);
	if (pUser == NULL
		|| pUser->isDead()
		|| isDead())
	{
		ExchangeCancel();
		return;
	}

	pkt >> pos >> nItemID >> count;
	_ITEM_TABLE *pTable = g_pMain->GetItemPtr(nItemID);
	if (pTable == NULL
		|| (nItemID != ITEM_GOLD && pos >= HAVE_MAX)
		|| m_bExchangeOK)
		goto add_fail;

	if (nItemID == ITEM_GOLD)
	{
		if (count <= 0 || count > m_pUserData->m_iGold) 
			goto add_fail;

		foreach (itr, m_ExchangeItemList)
		{
			if ((*itr)->nItemID == ITEM_GOLD)
			{
				(*itr)->nCount += count;
				m_pUserData->m_iGold -= count;
				bAdd = FALSE;
				break;
			}
		}
		if (bAdd)
			m_pUserData->m_iGold -= count;
	}
	else if (m_MirrorItem[pos].nNum == nItemID)
	{
		_ITEM_DATA *pItem = &m_MirrorItem[pos];
		if (pItem->sCount < count)
			goto add_fail;

		if (pTable->m_bCountable)
		{
			foreach (itr, m_ExchangeItemList)
			{
				if ((*itr)->nItemID == nItemID)
				{
					(*itr)->nCount += count;
					pItem->sCount -= count;
					bAdd = FALSE;
					break;
				}
			}
		}

		if (bAdd)
			pItem->sCount -= count;
	
		duration = pItem->sDuration;
		nSerialNum = pItem->nSerialNum;

		if (pItem->sCount <= 0 || pTable->m_bCountable == 0)
			memset(pItem, 0, sizeof(_ITEM_DATA));
	}
	else
		goto add_fail;

	foreach (itr, m_ExchangeItemList)
	{
		if ((*itr)->nItemID == ITEM_GOLD)
		{
			bGold = TRUE;
			break;
		}
	}
	if ((int)m_ExchangeItemList.size() > (bGold ? 13 : 12))
		goto add_fail;

	if (bAdd)
	{
		pItem = new _EXCHANGE_ITEM;
		pItem->nItemID = nItemID;
		pItem->sDurability = duration;
		pItem->nCount = count;
		pItem->nSerialNum = nSerialNum;
		m_ExchangeItemList.push_back(pItem);
	}

	result << uint8(1);
	Send(&result);

	result.clear();

	result << uint8(EXCHANGE_OTHERADD)
			<< nItemID << count << duration;
	pUser->Send(&result);
	return;

add_fail:
	result << uint8(0);
	Send(&result);
}

void CUser::ExchangeDecide()
{
	BOOL bSuccess = TRUE;

	CUser *pUser = g_pMain->GetUserPtr(m_sExchangeUser);
	if (pUser == NULL
		|| pUser->isDead()
		|| isDead())
	{
		ExchangeCancel();
		return;
	}

	Packet result(WIZ_EXCHANGE);
	if (!pUser->m_bExchangeOK)
	{
		m_bExchangeOK = 1;
		result << uint8(EXCHANGE_OTHERDECIDE);
		pUser->Send(&result);
		return;
	}

	if (!ExecuteExchange() || !pUser->ExecuteExchange())
	{
		foreach (itr, m_ExchangeItemList)
		{
			if ((*itr)->nItemID == ITEM_GOLD)
			{
				m_pUserData->m_iGold += (*itr)->nCount;
				break;
			}
		}
			
		foreach (itr, pUser->m_ExchangeItemList)
		{
			if ((*itr)->nItemID == ITEM_GOLD)
			{
				pUser->m_pUserData->m_iGold += (*itr)->nCount;
				break;
			}
		}

		bSuccess = FALSE;
	}

	if (bSuccess)
	{
		result << uint8(EXCHANGE_DONE) << uint8(1)
				<< m_pUserData->m_iGold
				<< uint16(pUser->m_ExchangeItemList.size());

		foreach (itr, pUser->m_ExchangeItemList)
		{
			result	<< (*itr)->bSrcPos << (*itr)->nItemID
					<< uint16((*itr)->nCount) << (*itr)->sDurability;
		}
		Send(&result);

		result.clear();

		result << uint8(EXCHANGE_DONE) << uint8(1)
				<< pUser->m_pUserData->m_iGold
				<< uint16(m_ExchangeItemList.size());

		foreach (itr, m_ExchangeItemList)
		{
			result	<< (*itr)->bSrcPos << (*itr)->nItemID
					<< uint16((*itr)->nCount) << (*itr)->sDurability;
		}
		pUser->Send(&result);

		SendItemWeight();
		pUser->SendItemWeight();
	}
	else 
	{
		result << uint8(EXCHANGE_DONE) << uint8(0);
		Send(&result);
		pUser->Send(&result);
	}

	InitExchange(FALSE);
	pUser->InitExchange(FALSE);
}

void CUser::ExchangeCancel()
{
	if (!isTrading()
		|| isDead())
		return;

	CUser *pUser = g_pMain->GetUserPtr(m_sExchangeUser);
	foreach (itr, m_ExchangeItemList)
	{
		if ((*itr)->nItemID == ITEM_GOLD)
		{
			m_pUserData->m_iGold += (*itr)->nCount;
			break;
		}
	}

	InitExchange(FALSE);

	if (pUser != NULL)
	{
		pUser->ExchangeCancel();

		Packet result(WIZ_EXCHANGE, uint8(EXCHANGE_CANCEL));
		pUser->Send(&result);
	}
}

void CUser::InitExchange(BOOL bStart)
{
	while (m_ExchangeItemList.size())
	{
		_EXCHANGE_ITEM *pItem = m_ExchangeItemList.front();
		if (pItem != NULL)
			delete pItem;

		m_ExchangeItemList.pop_front();
	}

	if (!bStart)
	{
		m_sExchangeUser = -1;
		m_bExchangeOK = 0;
		memset(&m_MirrorItem, 0, sizeof(m_MirrorItem));
		return;
	}

	foreach_array (i, m_MirrorItem)
	{
		_ITEM_DATA *pItem = &m_pUserData->m_sItemArray[SLOT_MAX+i];
		m_MirrorItem[i].nNum = pItem->nNum;
		m_MirrorItem[i].sDuration = pItem->sDuration;
		m_MirrorItem[i].sCount = pItem->sCount;
		m_MirrorItem[i].nSerialNum = pItem->nSerialNum;
	}
}

BOOL CUser::ExecuteExchange()
{
	DWORD money = 0;
	short weight = 0;
	BYTE i = 0;

	CUser *pUser = g_pMain->GetUserPtr(m_sExchangeUser);
	if (pUser == NULL)
		return FALSE;

	foreach (Iter, pUser->m_ExchangeItemList)
	{
		// This should be checked before the item's even gone into the list...
		if( (*Iter)->nItemID >= ITEM_NO_TRADE)
			return FALSE;
		
		if ((*Iter)->nItemID == ITEM_GOLD)
		{
			money = (*Iter)->nCount;
			continue;
		}

		_ITEM_TABLE *pTable = g_pMain->GetItemPtr((*Iter)->nItemID);
		if (pTable == NULL)
			continue;

		for (i = 0; i < HAVE_MAX; i++)
		{
			_ITEM_DATA *pItem = &m_MirrorItem[i];
			if (pItem->nNum == 0 && !pTable->m_bCountable)
			{
				pItem->nNum = (*Iter)->nItemID;
				pItem->sDuration = (*Iter)->sDurability;
				pItem->sCount = (*Iter)->nCount;
				pItem->nSerialNum = (*Iter)->nSerialNum;
				(*Iter)->bSrcPos = i;	
				weight += pTable->m_sWeight;
				break;
			}

			if (pItem->nNum == (*Iter)->nItemID && pTable->m_bCountable)
			{			
				pItem->sCount += (*Iter)->nCount;
				if (pItem->sCount > MAX_ITEM_COUNT )
					pItem->sCount = MAX_ITEM_COUNT;
				weight += pTable->m_sWeight * (*Iter)->nCount;
				(*Iter)->bSrcPos = i;
				break;
			}
		}

		if (i == HAVE_MAX && pTable->m_bCountable)
		{
			for (i = 0; i < HAVE_MAX; i++)
			{
				_ITEM_DATA *pItem = &m_MirrorItem[i];
				if (pItem->nNum != 0)
					continue;

				pItem->nNum = (*Iter)->nItemID;
				pItem->sDuration = (*Iter)->sDurability;
				pItem->sCount = (*Iter)->nCount;
				(*Iter)->bSrcPos = i;
				weight += pTable->m_sWeight * (*Iter)->nCount;
				break;
			}
		}
	}

	return ((weight + m_sItemWeight) <= m_sMaxWeight);
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
		if ((*itr)->nItemID != ITEM_GOLD)
			continue;

		money = (*itr)->nCount;
		delete (*itr);
		pUser->m_ExchangeItemList.erase(itr);
		break;
	}
	
	if (money > 0) 
		m_pUserData->m_iGold += money;

	for (int i = 0; i < HAVE_MAX; i++)
	{
		_ITEM_DATA *pItem = &m_pUserData->m_sItemArray[SLOT_MAX+i];

		pItem->nNum = m_MirrorItem[i].nNum;
		pItem->sDuration = m_MirrorItem[i].sDuration;
		pItem->sCount = m_MirrorItem[i].sCount;
		pItem->nSerialNum = m_MirrorItem[i].nSerialNum;

		pTable = g_pMain->GetItemPtr(pItem->nNum);
		if (pTable == NULL)
			continue;

		if (!pTable->m_bCountable && pItem->nSerialNum == 0)
			pItem->nSerialNum = g_pMain->GenerateItemSerial();
	}

	return money;
}