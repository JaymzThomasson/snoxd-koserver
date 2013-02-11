#include "StdAfx.h"

void CUser::ItemRepair(Packet & pkt)
{
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
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
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
	if (!m_pMain->m_bPointCheckFlag)
		return;

	uint16 sNpcID = pkt.read<uint16>(), sEventID = 0;
	CNpc *pNpc = m_pMain->m_arNpcArray.GetData(sNpcID);
	if (pNpc == NULL)
		return;
	m_sEventNid = sNpcID;

	// Get events for this zone
	EVENT *pEvent = m_pMain->m_Event.GetData(getZoneID());
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
			if( getLevel() >= pLE->m_LogicElseInt[0] && getLevel() <= pLE->m_LogicElseInt[1] ) {
				bExact = TRUE;
			}
			break;

		case	LOGIC_NOEXIST_COM_EVENT:
			if (!ExistComEvent(pLE->m_LogicElseInt[0])) {
				bExact = TRUE;
			}	
			break;

		case	LOGIC_EXIST_COM_EVENT:
			if (ExistComEvent(pLE->m_LogicElseInt[0])) {
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

			pEvent = m_pMain->m_Event.GetData(m_pUserData->m_bZone);		if(!pEvent)	break;
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

	case	EXEC_SAVE_COM_EVENT:
		SaveComEvent(pExec->m_ExecInt[0]);
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

					pEvent = m_pMain->m_Event.GetData(m_pUserData->m_bZone);
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

			case	EXEC_SAVE_COM_EVENT:
				SaveComEvent(pExec->m_ExecInt[0]);
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
		uint32 money = (uint32)pow((getLevel() * 2.0f), 3.4f);

		if (getLevel() < 30)	
			money = (uint32)(money * 0.4f);
		else if (getLevel() >= 60)
			money = (uint32)(money * 1.5f);

		// If nation discounts are enabled (1), and this nation has won the last war, get it half price.
		// If global discounts are enabled (2), everyone can get it for half price.
		if ((m_pMain->m_sDiscount == 1 && m_pMain->m_byOldVictory == getNation())
			|| m_pMain->m_sDiscount == 2)
			money /= 2;

		result << uint8(CHANGE_MONEY_REQ) << money;
		Send(&result);
		return;
	}

	uint8 classcode = pkt.read<uint8>();
	switch (m_pUserData->m_sClass)
	{
	case KARUWARRRIOR:
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
		m_pMain->Send_PartyMember(m_sPartyIndex, &result);
	}
}

void CUser::RecvSelectMsg(Packet & pkt)	// Receive menu reply from client.
{
	uint8 bMenuIndex = pkt.read<uint8>();
	if (bMenuIndex >= MAX_MESSAGE_EVENT)
		goto fail_return;

	// Get the event number that needs to be processed next.
	int selectedEvent = m_iSelMsgEvent[bMenuIndex];
	EVENT *pEvent = m_pMain->m_Event.GetData(m_pUserData->m_bZone);
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
	if (!m_pMain->m_bPointCheckFlag)
		return;	

	Packet result;
	uint8 bUnknown = pkt.read<uint8>();
	uint16 sNpcID = pkt.read<uint16>();

	CNpc *pNpc = m_pMain->m_arNpcArray.GetData(sNpcID);
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

	case NPC_SABICE:
		result.SetOpcode(WIZ_KNIGHTS_PROCESS);
		result << uint8(KNIGHTS_CAPE_NPC);
		Send(&result);
		break;

	case NPC_CAPTAIN:
		result.SetOpcode(WIZ_CLASS_CHANGE);
		result << uint8(CLASS_CHANGE_REQ);
		Send(&result);
		break;

	case NPC_OFFICER: // this HAS to go.
		result << uint16(0); // page 0
		m_pMain->m_KnightsManager.AllKnightsList(this, result);
		break;

	case NPC_WAREHOUSE:
		result.SetOpcode(WIZ_WAREHOUSE);
		result << uint8(WAREHOUSE_REQ);
		Send(&result);
		break;

	case NPC_WARP:
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
	int send_index = 0, itemid = 0, money = 0, group = 0;
	uint16 npcid;
	uint16 count, real_count = 0;
	_ITEM_TABLE* pTable = NULL;
	char send_buf[128];
	CNpc* pNpc = NULL;
	BYTE type, pos, destpos, result;

	if (isDead())
	{
		TRACE("### ItemTrade Fail : name=%s(%d), m_bResHpType=%d, hp=%d, x=%d, z=%d ###\n", m_pUserData->m_id, m_Sid, m_bResHpType, m_pUserData->m_sHp, (int)m_pUserData->m_curx, (int)m_pUserData->m_curz);
		result = 0x01;
		goto fail_return;
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

	if (itemid >= ITEM_NO_TRADE) goto fail_return;

	if( type == 0x03 ) {	// item inven to inven move
		if( pos >= HAVE_MAX || destpos >= HAVE_MAX ) {
			SetByte( send_buf, WIZ_ITEM_TRADE, send_index );
			SetByte( send_buf, 0x04, send_index );
			Send( send_buf, send_index );
			return;
		}
		if( itemid != m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum ) {
			SetByte( send_buf, WIZ_ITEM_TRADE, send_index );
			SetByte( send_buf, 0x04, send_index );
			Send( send_buf, send_index );
			return;
		}
		short duration = m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration;
		short itemcount = m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount;
		m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum = m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum;
		m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration = m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration;
		m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount = m_pUserData->m_sItemArray[SLOT_MAX+destpos].sCount;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].nNum = itemid;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].sDuration = duration;
		m_pUserData->m_sItemArray[SLOT_MAX+destpos].sCount = itemcount;
		SetByte( send_buf, WIZ_ITEM_TRADE, send_index );
		SetByte( send_buf, 0x03, send_index );
		Send( send_buf, send_index );
		return;
	}

	if (isTrading())
		goto fail_return;
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
	if( !pTable ) {
		result = 0x01;
		goto fail_return;
	}
	if( pos >= HAVE_MAX ) {
		result = 0x02;
		goto fail_return;
	}

	if( count <= 0 || count > MAX_ITEM_COUNT ) {
		result = 0x02;
		goto fail_return;
	}

	if( type == 0x01 ) {	// buy sequence
		if( m_pMain->m_bPointCheckFlag == FALSE)	{
			result = 0x01;
			goto fail_return;
		}	

		pNpc = m_pMain->m_arNpcArray.GetData(npcid);
		if( !pNpc ) {
			result = 0x01;
			goto fail_return;
		}
		if( pNpc->m_iSellingGroup != group ) {
			result = 0x01;
			goto fail_return;
		}

		if( m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum != 0 ) {
			if( m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum == itemid ) {
				if( !pTable->m_bCountable || count <= 0 ) {
					result = 0x02;
					goto fail_return;
				}

				if( pTable->m_bCountable && (count+m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount) > MAX_ITEM_COUNT ) {
					result = 0x04;
					goto fail_return;				
				}
			}
			else {
				result = 0x02;
				goto fail_return;
			}
		}
		if( m_pUserData->m_iGold < ((uint32)pTable->m_iBuyPrice*count) ) {
			result = 0x03;
			goto fail_return;
		}
//
		if (pTable->m_bCountable) {	// Check weight of countable item.
			if (((pTable->m_sWeight * count) + m_sItemWeight) > m_sMaxWeight) {			
				result = 0x04;
				goto fail_return;
			}
		}
		else {	// Check weight of non-countable item.
			if ((pTable->m_sWeight + m_sItemWeight) > m_sMaxWeight) {
				result = 0x04;
				goto fail_return;
			}
		}		
//
		m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum = itemid;
		m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration = pTable->m_sDuration;
		if( pTable->m_bCountable && count > 0 ) {	// count ?????? ??? ??????
			m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount += count;
			m_pUserData->m_iGold -= (pTable->m_iBuyPrice * count);
		}
		else {
			m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount = 1;
			m_pUserData->m_iGold -= pTable->m_iBuyPrice;
			m_pUserData->m_sItemArray[SLOT_MAX+pos].nSerialNum = m_pMain->GenerateItemSerial();
		}

		SendItemWeight();
		ItemLogToAgent( m_pUserData->m_id, pNpc->m_strName, ITEM_MERCHANT_BUY, m_pUserData->m_sItemArray[SLOT_MAX+pos].nSerialNum, itemid, count, pTable->m_sDuration );
	}
	else {		// sell sequence
		if( m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum != itemid ) {
			result = 0x02;
			goto fail_return;
		}
		if( m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount < count ) {
			result = 0x03;
			goto fail_return;
		}
		int durability = m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration;

		if( pTable->m_bCountable && count > 0 ) {
			m_pUserData->m_iGold += (pTable->m_iSellPrice * count);
			m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount -= count;
			if( m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount <= 0 ) {
				m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum = 0;
				m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration = 0;
				m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount = 0;
			}
		}
		else {
			m_pUserData->m_iGold += pTable->m_iSellPrice;
			m_pUserData->m_sItemArray[SLOT_MAX+pos].nNum = 0;
			m_pUserData->m_sItemArray[SLOT_MAX+pos].sDuration = 0;
			m_pUserData->m_sItemArray[SLOT_MAX+pos].sCount = 0;
		}

		SendItemWeight();
		ItemLogToAgent( m_pUserData->m_id, "MERCHANT SELL", ITEM_MERCHANT_SELL, 0, itemid, count, durability );
	}

	SetByte( send_buf, WIZ_ITEM_TRADE, send_index );
	SetShort( send_buf, 0x01, send_index );
	SetDWORD( send_buf, m_pUserData->m_iGold, send_index );
	if( type == 0x01)
		SetDWORD( send_buf, (pTable->m_iBuyPrice * count), send_index );
	if( type == 0x02)
		SetDWORD( send_buf, (pTable->m_iSellPrice * count), send_index );
	Send( send_buf, send_index );
	return;

fail_return:
	send_index = 0;
	SetByte( send_buf, WIZ_ITEM_TRADE, send_index );
	SetByte( send_buf, 0x00, send_index );
	SetByte( send_buf, result, send_index );
	Send( send_buf, send_index );
}

// part of the EVT system
void CUser::SaveComEvent(int eventid)
{
	for (int i = 0 ; i < MAX_CURRENT_EVENT ; i++) {
		if (m_sEvent[i] != eventid) {
			m_sEvent[i] = eventid;
			break;
		}
	}
}

BOOL CUser::ExistComEvent(int eventid)
{
	for (int i = 0 ; i < MAX_CURRENT_EVENT ; i++) {
		if (m_sEvent[i] == eventid) {
			return TRUE;
		}
	}	

	return FALSE;
}