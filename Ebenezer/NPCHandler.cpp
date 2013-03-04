#include "StdAfx.h"

void CUser::ItemRepair(Packet & pkt)
{
	if (isDead())
		return;

	Packet result(WIZ_ITEM_REPAIR);
	uint32 money, itemid;
	uint16 durability, quantity;
	_ITEM_TABLE* pTable = NULL;
	uint8 pos, slot;

	pkt >> pos >> slot >> itemid;
	if( pos == 1 ) {	// SLOT
		if( slot >= SLOT_MAX ) goto fail_return;
		if( m_pUserData->m_sItemArray[slot].nNum != itemid ) goto fail_return;
	}
	else if ( pos == 2 ) {	// INVEN
		if( slot >= HAVE_MAX ) goto fail_return;
		if( m_pUserData->m_sItemArray[SLOT_MAX+slot].nNum != itemid ) goto fail_return;
	}
	pTable = g_pMain->GetItemPtr( itemid );
	if( !pTable ) goto fail_return;
	durability = pTable->m_sDuration;
	if( durability == 1 ) goto fail_return;
	if( pos == 1 )
		quantity = pTable->m_sDuration - m_pUserData->m_sItemArray[slot].sDuration;
	else if( pos == 2 ) 
		quantity = pTable->m_sDuration - m_pUserData->m_sItemArray[SLOT_MAX+slot].sDuration;
	
	money = (unsigned int)((((pTable->m_iBuyPrice-10) / 10000.0f) + pow((float)pTable->m_iBuyPrice, 0.75f)) * quantity / (double)durability);
	if( money > m_pUserData->m_iGold ) goto fail_return;

	m_pUserData->m_iGold -= money;
	if( pos == 1 )
		m_pUserData->m_sItemArray[slot].sDuration = durability;
	else if( pos == 2 )
		m_pUserData->m_sItemArray[SLOT_MAX+slot].sDuration = durability;

	result << uint8(1) << m_pUserData->m_iGold;
	Send(&result);
	return;

fail_return:
	result << uint8(0) << m_pUserData->m_iGold;
	Send(&result);
}

void CUser::ClientEvent(Packet & pkt)
{
	// Ensure AI's loaded
	if (!g_pMain->m_bPointCheckFlag
		|| isDead())
		return;

	uint16 sNpcID = pkt.read<uint16>(), sEventID = 0;
	CNpc *pNpc = g_pMain->m_arNpcArray.GetData(sNpcID);
	if (pNpc == NULL)
		return;
	m_sEventNid = sNpcID;

	// Get events for this zone
	EVENT *pEvent = g_pMain->m_Event.GetData(GetZoneID());
	if (pEvent == NULL)
		return;

	// Get the corresponding event ID for this NPC type
	switch (pNpc->GetType()) 
	{
		case NPC_CLERIC:
			sEventID = EVENT_POTION;
			break;

		case NPC_COUPON:
			sEventID = EVENT_COUPON;
			break;

		case NPC_MONK_ELMORAD:
			sEventID = EVENT_LOGOS_ELMORAD;
			break;

		case NPC_MONK_KARUS:
			sEventID = EVENT_LOGOS_KARUS;
			break;
	}

	// No event was set
	if (sEventID == 0)
		return;

	EVENT_DATA *pEventData = pEvent->m_arEvent.GetData(sEventID);
	if (pEventData == NULL
		|| !CheckEventLogic(pEventData)) return; // Check if all 'A's meet the requirements in Event #1

	foreach (itr, pEventData->m_arExec)
		if (!RunNpcEvent(pNpc, *itr))
			return;
}

BOOL CUser::CheckEventLogic(EVENT_DATA *pEventData) 	// This part reads all the 'A' parts and checks if the 
{                                                       // requirements for the 'E' parts are met.
	if( !pEventData ) return FALSE;

	BOOL bExact = TRUE;

	foreach (itr, pEventData->m_arLogicElse)
	{
		bExact = FALSE;

		LOGIC_ELSE* pLE = (*itr);
		if (pLE == NULL) return FALSE;

		switch( pLE->m_LogicElse ) {
		case	LOGIC_CHECK_UNDER_WEIGHT:
			if( pLE->m_LogicElseInt[0]+m_sItemWeight >= m_sMaxWeight )
				bExact = TRUE;
			break;

		case	LOGIC_CHECK_OVER_WEIGHT:
			if( pLE->m_LogicElseInt[0]+m_sItemWeight < m_sMaxWeight )
				bExact = TRUE;
			break;

		case	LOGIC_CHECK_SKILL_POINT:
			if( CheckSkillPoint(pLE->m_LogicElseInt[0], pLE->m_LogicElseInt[1], pLE->m_LogicElseInt[2]) )
				bExact = TRUE;
			break;

		case	LOGIC_EXIST_ITEM:
			if ( CheckExistItem(pLE->m_LogicElseInt[0], pLE->m_LogicElseInt[1]) )	bExact = TRUE;
			break;

		case	LOGIC_CHECK_CLASS:		
			if (CheckClass( pLE->m_LogicElseInt[0], pLE->m_LogicElseInt[1], pLE->m_LogicElseInt[2],
				pLE->m_LogicElseInt[3], pLE->m_LogicElseInt[4], pLE->m_LogicElseInt[5])) {
				bExact = TRUE;
			}
			break;

		case	LOGIC_CHECK_WEIGHT:	
			if (!CheckWeight( pLE->m_LogicElseInt[0], pLE->m_LogicElseInt[1])) {				
				bExact = TRUE;
			}
			break;	

		case	LOGIC_RAND:
			if (CheckRandom(pLE->m_LogicElseInt[0])) {
				bExact = TRUE;
			}
			break;

		case	LOGIC_CHECK_LEVEL:		
			if( GetLevel() >= pLE->m_LogicElseInt[0] && GetLevel() <= pLE->m_LogicElseInt[1] ) {
				bExact = TRUE;
			}
			break;

		case	LOGIC_HOWMUCH_ITEM:
			if ( CheckItemCount(pLE->m_LogicElseInt[0], pLE->m_LogicElseInt[1], pLE->m_LogicElseInt[2]) ) {
				bExact = TRUE;
			}
			break;

		case	LOGIC_CHECK_NOAH:
			if ( m_pUserData->m_iGold >= (unsigned int)pLE->m_LogicElseInt[0] && m_pUserData->m_iGold <= (unsigned int)pLE->m_LogicElseInt[1] ) {
				bExact = TRUE;	
			}
			break;

		default:
			return FALSE;
		}

		if( !pLE->m_bAnd ) {	// OR ????? ??? bExact?? TRUE??? ??ü?? TRUE???
			if(bExact) return TRUE;
		}
		else {					// AND ????? ??? bExact?? FALSE??? ??ü?? FALSE???
			if(!bExact) return FALSE;
		}
	}

	return bExact;
}

BOOL CUser::RunNpcEvent(CNpc *pNpc, EXEC *pExec)	// This part executes all the 'E' lines!
{
	switch( pExec->m_Exec ) {
	case EXEC_SAY:
		SendNpcSay( pExec );
		break;

	case	EXEC_SELECT_MSG:
		SelectMsg( pExec );
		break;

	case	EXEC_RUN_EVENT:
		{
			EVENT* pEvent = NULL;
			EVENT_DATA* pEventData = NULL;				

			pEvent = g_pMain->m_Event.GetData(m_pUserData->m_bZone);		if(!pEvent)	break;
			pEventData = pEvent->m_arEvent.GetData(pExec->m_ExecInt[0]);	if(!pEventData) break;

			if( !CheckEventLogic(pEventData) )	break;

			foreach (itr, pEventData->m_arExec) 
			{
				if  (!RunNpcEvent(pNpc, *itr))
					return FALSE;
			}
		}
		break;

	case	EXEC_GIVE_ITEM:
		if ( !GiveItem(pExec->m_ExecInt[0], pExec->m_ExecInt[1]) )		return FALSE;
		break;

	case	EXEC_ROB_ITEM:
		if ( !RobItem(pExec->m_ExecInt[0], pExec->m_ExecInt[1]) )		return FALSE;
		break;

	case	EXEC_GIVE_NOAH:
		GoldGain(pExec->m_ExecInt[0]);
		break;

	case	EXEC_RETURN:
		return FALSE;

		default:
			break;
	}

	return TRUE;
}

BOOL CUser::RunEvent(EVENT_DATA *pEventData)
{
	foreach (itr, pEventData->m_arExec) 
	{
		EXEC* pExec = *itr;
		if (pExec == NULL)
			break;

		switch(pExec->m_Exec){
			case EXEC_SAY:
				SendNpcSay( pExec );
				break;

			case	EXEC_SELECT_MSG:
				SelectMsg( pExec );
				break;

			case	EXEC_RUN_EVENT:
				{
					EVENT* pEvent = NULL;
					EVENT_DATA* pEventData = NULL;				

					pEvent = g_pMain->m_Event.GetData(m_pUserData->m_bZone);
					if(!pEvent)	break;

					pEventData = pEvent->m_arEvent.GetData(pExec->m_ExecInt[0]);
					if(!pEventData) break;

					if( !CheckEventLogic(pEventData) )	break;

					if( !RunEvent(pEventData) ){
						return FALSE;
					}
				}
				break;

			case	EXEC_GIVE_ITEM:
				if ( !GiveItem(pExec->m_ExecInt[0], pExec->m_ExecInt[1]) )
					return FALSE;
				break;

			case	EXEC_ROB_ITEM:
				if ( !RobItem(pExec->m_ExecInt[0], pExec->m_ExecInt[1]) )
					return FALSE;
				break;

			case	EXEC_GIVE_NOAH:
				GoldGain(pExec->m_ExecInt[0]);
				break;

			case	EXEC_ROB_NOAH:
				GoldLose(pExec->m_ExecInt[0]);
				break;
//
			case	EXEC_RETURN:
				return FALSE;

			default:
				break;
		}
	}

	return TRUE;
}

void CUser::ClassChange(Packet & pkt)
{
	Packet result(WIZ_CLASS_CHANGE);
	BOOL bSuccess = FALSE;
	uint8 opcode = pkt.read<uint8>();
	if (opcode == CLASS_CHANGE_REQ)	
	{
		ClassChangeReq();
		return;
	}
	else if (opcode == ALL_POINT_CHANGE)	
	{
		AllPointChange();
		return;
	}
	else if (opcode == ALL_SKILLPT_CHANGE)	
	{
		AllSkillPointChange();
		return;
	}
	else if (opcode == CHANGE_MONEY_REQ)	
	{
		uint8 sub_type = pkt.read<uint8>(); // type is irrelevant
		uint32 money = (uint32)pow((GetLevel() * 2.0f), 3.4f);

		if (GetLevel() < 30)	
			money = (uint32)(money * 0.4f);
		else if (GetLevel() >= 60)
			money = (uint32)(money * 1.5f);

		// If nation discounts are enabled (1), and this nation has won the last war, get it half price.
		// If global discounts are enabled (2), everyone can get it for half price.
		if ((g_pMain->m_sDiscount == 1 && g_pMain->m_byOldVictory == GetNation())
			|| g_pMain->m_sDiscount == 2)
			money /= 2;

		result << uint8(CHANGE_MONEY_REQ) << money;
		Send(&result);
		return;
	}

	uint8 classcode = pkt.read<uint8>();
	switch (m_pUserData->m_sClass)
	{
	case KARUWARRIOR:
		if( classcode == BERSERKER || classcode == GUARDIAN )
			bSuccess = TRUE;
		break;
	case KARUROGUE:
		if( classcode == HUNTER || classcode == PENETRATOR )
			bSuccess = TRUE;
		break;
	case KARUWIZARD:
		if( classcode == SORSERER || classcode == NECROMANCER )
			bSuccess = TRUE;
		break;
	case KARUPRIEST:
		if( classcode == SHAMAN || classcode == DARKPRIEST )
			bSuccess = TRUE;
		break;
	case ELMORWARRRIOR:
		if( classcode == BLADE || classcode == PROTECTOR )
			bSuccess = TRUE;
		break;
	case ELMOROGUE:
		if( classcode == RANGER || classcode == ASSASSIN )
			bSuccess = TRUE;
		break;
	case ELMOWIZARD:
		if( classcode == MAGE || classcode == ENCHANTER )
			bSuccess = TRUE;
		break;
	case ELMOPRIEST:
		if( classcode == CLERIC || classcode == DRUID )
			bSuccess = TRUE;
		break;
	}

	// Not allowed this job change
	if (!bSuccess)
	{
		result << uint8(CLASS_CHANGE_RESULT) << uint8(0);
		Send(&result);
		return;
	}

	m_pUserData->m_sClass = classcode;
	if (isInParty())
	{
		// TO-DO: Move this somewhere better.
		result.SetOpcode(WIZ_PARTY);
		result << uint8(PARTY_CLASSCHANGE) << GetSocketID() << uint16(classcode);
		g_pMain->Send_PartyMember(m_sPartyIndex, &result);
	}
}

void CUser::RecvSelectMsg(Packet & pkt)	// Receive menu reply from client.
{
	uint8 bMenuIndex = pkt.read<uint8>();
	if (bMenuIndex >= MAX_MESSAGE_EVENT
		|| isDead())
		goto fail_return;

	// Get the event number that needs to be processed next.
	int selectedEvent = m_iSelMsgEvent[bMenuIndex];
	EVENT *pEvent = g_pMain->m_Event.GetData(m_pUserData->m_bZone);
	if (pEvent == NULL)	
		goto fail_return;

	EVENT_DATA *pEventData = pEvent->m_arEvent.GetData(selectedEvent);
	if (pEventData == NULL
		|| !CheckEventLogic(pEventData)
		|| !RunEvent(pEventData))
		goto fail_return;

	// wonderful logic, need to fix it later.
	return;

fail_return:
	for (int i = 0 ; i < MAX_MESSAGE_EVENT; i++)
		m_iSelMsgEvent[i] = -1;
}

void CUser::SendNpcSay(EXEC *pExec)
{
	int i, send_index = 0;
	char send_buf[128];

	if( !pExec ) return;

	SetByte( send_buf, WIZ_NPC_SAY, send_index );
	for( i=0; i<MAX_MESSAGE_EVENT; i++) {
		SetDWORD( send_buf, pExec->m_ExecInt[i], send_index );
	}

	Send( send_buf, send_index );
}

void CUser::SelectMsg(EXEC *pExec)
{
	int i, chat, send_index = 0;
	char send_buf[128];

	if( !pExec ) return;

	SetByte( send_buf, WIZ_SELECT_MSG, send_index );
	SetShort( send_buf, m_sEventNid, send_index );
	SetDWORD( send_buf, pExec->m_ExecInt[1], send_index );	

	chat = 2;

	for( i = 0 ; i < MAX_MESSAGE_EVENT ; i++ ) {
		SetDWORD( send_buf, pExec->m_ExecInt[chat], send_index );
		chat += 2;
	}

	Send( send_buf, send_index );

	for (int j = 0 ; j < MAX_MESSAGE_EVENT ; j++) {
		m_iSelMsgEvent[j] = pExec->m_ExecInt[(2 * j) + 3];
	}
}

void CUser::NpcEvent(Packet & pkt)
{
	// Ensure AI is loaded first
	if (!g_pMain->m_bPointCheckFlag
		|| isDead())
		return;	

	Packet result;
	uint8 bUnknown = pkt.read<uint8>();
	uint16 sNpcID = pkt.read<uint16>();

	CNpc *pNpc = g_pMain->m_arNpcArray.GetData(sNpcID);
	if (pNpc == NULL)
		return;

	switch (pNpc->GetType())
	{
	case NPC_MERCHANT:
	case NPC_TINKER:
		result.SetOpcode(pNpc->GetType() == NPC_MERCHANT ? WIZ_TRADE_NPC : WIZ_REPAIR_NPC);
		result << pNpc->m_iSellingGroup;
		Send(&result);
		break;

	/*case NPC_MENU:
		result.SetOpcode(WIZ_QUEST);
		result	<< uint8(7) << uint16(SendNPCMenu(pNpc->m_sSid))
				<< uint16(0) << uint16(pNpc->m_sSid);
		Send(&result);
		break; */

	case NPC_MARK:
		result.SetOpcode(WIZ_KNIGHTS_PROCESS);
		result << uint8(KNIGHTS_CAPE_NPC);
		Send(&result);
		break;

	case NPC_RENTAL:
		result.SetOpcode(WIZ_RENTAL);
		result	<< uint8(RENTAL_NPC) 
				<< uint16(1) // 1 = enabled, -1 = disabled 
				<< pNpc->m_iSellingGroup;
		Send(&result);
		break;

	case NPC_ELECTION:
		result.SetOpcode(WIZ_KING);
		result.SByte();
		result << uint8(KING_NPC) << "king name here";
		Send(&result);
		break;
	
	case NPC_TREASURY:
		result.SetOpcode(WIZ_KING);
		result	<< uint8(KING_TAX) << uint8(1) // success
				<< uint16(/*isKing() ? 1 : 2*/ 2) // 1 enables king-specific stuff (e.g. scepter), 2 is normal user stuff
				<< uint32(1234); // amount in nation's treasury
		Send(&result);
		break;

	case NPC_CAPTAIN:
		result.SetOpcode(WIZ_CLASS_CHANGE);
		result << uint8(CLASS_CHANGE_REQ);
		Send(&result);
		break;

	case NPC_CLAN: // this HAS to go.
		result << uint16(0); // page 0
		g_pMain->m_KnightsManager.AllKnightsList(this, result);
		break;

	case NPC_WAREHOUSE:
		result.SetOpcode(WIZ_WAREHOUSE);
		result << uint8(WAREHOUSE_REQ);
		Send(&result);
		break;

	case NPC_CLERIC:
	case NPC_COUPON:
	case NPC_MONK_KARUS:
	case NPC_MONK_ELMORAD:
		result << sNpcID; // this HAS to go.
		ClientEvent(result);
		break;
	}   
}

// NPC shops
void CUser::ItemTrade(Packet & pkt)
{
	Packet result(WIZ_ITEM_TRADE);
	uint32 transactionPrice;
	int itemid = 0, money = 0, group = 0;
	uint16 npcid;
	uint16 count, real_count = 0;
	_ITEM_TABLE* pTable = NULL;
	CNpc* pNpc = NULL;
	BYTE type, pos, destpos, errorCode = 1;
	bool bSuccess = true;

	if (isDead())
	{
		errorCode = 1;
		goto send_packet;
	}

	pkt >> type;
	// Buy == 1, Sell == 2
	if (type == 1 || type == 2)
		pkt >> group >> npcid;
	pkt >> itemid >> pos;

	if (type == 3) 	// Move only (this is so useless mgame -- why not just handle it with the CUser::ItemMove(). Gah.)
		pkt >> destpos;
	else
		pkt >> count;

	if (itemid >= ITEM_NO_TRADE)
		goto fail_return;

	// Moving an item in the inventory
	if (type == 3)
	{
		if (pos >= HAVE_MAX || destpos >= HAVE_MAX
			|| itemid != m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum)
		{
			errorCode = 4;
			goto send_packet;
		}

		short duration = m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration;
		short itemcount = m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount;
		m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum = m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum;
		m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount = m_pUserData->m_sItemArray[SLOT_MAX+destpos].sCount;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum = itemid;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration = duration;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].sCount = itemcount;

		result << uint8(3);
		Send(&result);
		return;
	}

	if (isTrading()
		|| (pTable = g_pMain->GetItemPtr(itemid)) == NULL)
		goto fail_return;

	if (pos >= HAVE_MAX
		|| count <= 0 || count > MAX_ITEM_COUNT)
	{
		errorCode = 2;
		goto fail_return;
	}

	// Buying from an NPC
	if (type == 1)
	{	
		if (!g_pMain->m_bPointCheckFlag
			|| (pNpc = g_pMain->m_arNpcArray.GetData(npcid)) == NULL
			|| pNpc->m_iSellingGroup != group)
			goto fail_return;

		if (m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum != 0)
		{
			if (m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum != itemid)
			{
				errorCode = 2;
				goto fail_return;
			}

			if (!pTable->m_bCountable || count <= 0)
			{
				errorCode = 2;
				goto fail_return;
			}

			if (pTable->m_bCountable 
				&& (count + m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount) > MAX_ITEM_COUNT)
			{
				errorCode = 4;
				goto fail_return;				
			}
		}

		transactionPrice = ((uint32)pTable->m_iBuyPrice * count);
		if (m_pUserData->m_iGold < transactionPrice)
		{
			errorCode = 3;
			goto fail_return;
		}

		if (((pTable->m_sWeight * count) + m_sItemWeight) > m_sMaxWeight)
		{
			errorCode = 4;
			goto fail_return;
		}

		m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum = itemid;
		m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration = pTable->m_sDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount += count;
		m_pUserData->m_iGold -= transactionPrice;

		if (!pTable->m_bCountable)
			m_pUserData->m_sItemArray[SLOT_MAX+pos].nSerialNum = g_pMain->GenerateItemSerial();

		SendItemWeight();
	}
	// Selling an item to an NPC
	else
	{
		_ITEM_DATA *pItem = &m_pUserData->m_sItemArray[SLOT_MAX+pos];
		if (pItem->nNum != itemid
			|| pItem->isSealed() // need to check the error codes for these
			|| pItem->isRented())
		{
			errorCode = 2;
			goto fail_return;
		}

		if (pItem->sCount < count)
		{
			errorCode = 3;
			goto fail_return;
		}

		short oldDurability = pItem->sDuration;
		if (!pTable->m_iSellPrice) // NOTE: 0 sells normally, 1 sells at full price, not sure what 2's used for...
			transactionPrice = ((pTable->m_iBuyPrice / 6) * count); // /6 is normal, /4 for prem/discount
		else
			transactionPrice = (pTable->m_iBuyPrice * count);

		if (m_pUserData->m_iGold + transactionPrice > COIN_MAX)
			m_pUserData->m_iGold = COIN_MAX;
		else
			m_pUserData->m_iGold += transactionPrice;

		if (count >= pItem->sCount)
			memset(pItem, 0, sizeof(_ITEM_DATA));
		else
			pItem->sCount -= count;

		SendItemWeight();
	}

	goto send_packet;

fail_return:
	bSuccess = false;

send_packet:
	result << bSuccess;
	if (!bSuccess)
		result << errorCode;
	else 
		result << pTable->m_bSellingGroup << m_pUserData->m_iGold << transactionPrice; // price bought or sold for
	Send(&result);
}