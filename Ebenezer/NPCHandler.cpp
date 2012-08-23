#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "Ebenezer.h"
#include "EbenezerDlg.h"
#include "User.h"

void CUser::ItemRepair(char *pBuf)
{
	int index = 0, send_index = 0, money = 0, quantity = 0;
	int itemid = 0, pos = 0, slot = -1, durability = 0;
	char send_buff[128]; memset( send_buff, 0x00, 128 );
	_ITEM_TABLE* pTable = NULL;

	pos = GetByte( pBuf, index );
	slot = GetByte( pBuf, index );
	itemid = GetDWORD( pBuf, index );
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
	
	money = (int)( ((pTable->m_iBuyPrice-10)/10000.0f) + pow(pTable->m_iBuyPrice, 0.75)) * quantity / (double)durability;
	if( money > m_pUserData->m_iGold ) goto fail_return;

	m_pUserData->m_iGold -= money;
	if( pos == 1 )
		m_pUserData->m_sItemArray[slot].sDuration = durability;
	else if( pos == 2 )
		m_pUserData->m_sItemArray[SLOT_MAX+slot].sDuration = durability;

	SetByte( send_buff, WIZ_ITEM_REPAIR, send_index );
	SetByte( send_buff, 0x01, send_index );
	SetDWORD( send_buff, m_pUserData->m_iGold, send_index );
	Send( send_buff, send_index );

	return;
fail_return:
	SetByte( send_buff, WIZ_ITEM_REPAIR, send_index );
	SetByte( send_buff, 0x00, send_index );
	SetDWORD( send_buff, m_pUserData->m_iGold, send_index );
	Send( send_buff, send_index );
}

void CUser::ClientEvent(char *pBuf)		// The main function for the quest procedures!!!
{										// (actually, this only takes care of the first event :(  )
	if( m_pMain->m_bPointCheckFlag == FALSE)	return;	// ?????? ?????? ???           //

	int index = 0;
	CNpc* pNpc = NULL;
	int nid = 0, eventnum = 0;
	BYTE code = 0x00;
	EVENT* pEvent = NULL;
	EVENT_DATA* pEventData = NULL;
	int eventid = -1;

	nid = GetShort( pBuf, index );
	pNpc = m_pMain->m_arNpcArray.GetData(nid);
	if( !pNpc )	return;	   // Better to check than not to check ;)

	m_sEventNid = nid;     // ??? ????? ???? ????? ?????....

//	if( pNpc->m_byEvent < 0 ) return;      // ??? ??? ??? ó?? ??? ????!!		//

	pEvent = m_pMain->m_Event.GetData(m_pUserData->m_bZone);	                    //
	if(!pEvent)	return;																//

//	pEventData = pEvent->m_arEvent.GetData(pNpc->m_byEvent);						//

	switch(pNpc->m_tNpcType) {
		case NPC_CLERIC:
			eventid = EVENT_POTION;
			break;

		case NPC_COUPON:
			eventid = EVENT_COUPON;
			break;

		case NPC_MONK_ELMORAD:
			eventid = EVENT_LOGOS_ELMORAD;
			break;

		case NPC_MONK_KARUS:
			eventid = EVENT_LOGOS_KARUS;
			break;
	}


	pEventData = pEvent->m_arEvent.GetData(eventid);		// Make sure you change this later!!!	
	if(!pEventData) return;								                           
	
	if( !CheckEventLogic(pEventData) ) return;	// Check if all 'A's meet the requirements in Event #1

	list<EXEC*>::iterator	Iter;		        // Execute the 'E' events in Event #1
	for( Iter = pEventData->m_arExec.begin(); Iter != pEventData->m_arExec.end(); Iter++ ) {
		if( !RunNpcEvent( pNpc, (*Iter) ) ){
			return;
		}
	}
}

BOOL CUser::CheckEventLogic(EVENT_DATA *pEventData) 	// This part reads all the 'A' parts and checks if the 
{                                                       // requirements for the 'E' parts are met.
	if( !pEventData ) return FALSE;

	BOOL bExact = TRUE;

	list<LOGIC_ELSE*>::iterator	Iter;
	for( Iter = pEventData->m_arLogicElse.begin(); Iter != pEventData->m_arLogicElse.end(); Iter++ ) {
		bExact = FALSE;

		LOGIC_ELSE* pLE = (*Iter);
		if( !pLE ) return FALSE;

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
//
// ????? ???? >.<
		case	LOGIC_CHECK_LEVEL:		
			if( m_pUserData->m_bLevel >= pLE->m_LogicElseInt[0] && m_pUserData->m_bLevel <= pLE->m_LogicElseInt[1] ) {
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
			if ( m_pUserData->m_iGold >= pLE->m_LogicElseInt[0] && m_pUserData->m_iGold <= pLE->m_LogicElseInt[1] ) {
				bExact = TRUE;	
			}
			break;
//

//
/*
			case LOGIC_CHECK_NATION:
				if( pLE->m_LogicElseInt[0] == m_pUserData->m_bNation ) {
					bExact = TRUE;
				}
				break;

			case LOGIC_CHECK_LEVEL:		
				if( m_pUserData->m_bLevel >= pLE->m_LogicElseInt[0] && m_pUserData->m_bLevel <= pLE->m_LogicElseInt[1] ) {
					bExact = TRUE;
				}
				break;

			case LOGIC_NOEXIST_ITEM:	
				if (ItemCountChange(pLE->m_LogicElseInt[0], 1, 0) < 2) {
					bExact = TRUE;
				}
				break;

			case LOGIC_QUEST_END:	
				break;

			case LOGIC_QUEST_LOG:
				break;

			case LOGIC_CHECK_NOAH:
				if(m_pUserData->m_iGold >= pLE->m_LogicElseInt[0]) {
					bExact = TRUE;
				}
				break;

			case LOGIC_CHECK_RACE:
				if (pLE->m_LogicElseInt[0] == m_pUserData->m_bRace) {
					bExact = TRUE;
				}
				break;
///////// These logics are for the test quest.
			case LOGIC_EXIST_ITEM:
				if (CheckExistItem(pLE->m_LogicElseInt[0], pLE->m_LogicElseInt[1])) {
					bExact = TRUE;
				}
				break;

			case LOGIC_CHECK_CLASS:		
				if (CheckClass( pLE->m_LogicElseInt[0], pLE->m_LogicElseInt[1], pLE->m_LogicElseInt[2],
					pLE->m_LogicElseInt[3], pLE->m_LogicElseInt[4], pLE->m_LogicElseInt[5])) {
					bExact = TRUE;
				}
				break;

			case LOGIC_CHECK_WEIGHT:	
				if (CheckWeight( pLE->m_LogicElseInt[0], pLE->m_LogicElseInt[1])) {				
					bExact = TRUE;
				}
				break;

			case LOGIC_CHECK_SKILLPOINT:
				if (CheckSkillPoint(pLE->m_LogicElseInt[0], pLE->m_LogicElseInt[1], pLE->m_LogicElseInt[2])) {	
					bExact = TRUE;
				}
				break;
*/

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

			list<EXEC*>::iterator	Iter;
			for( Iter = pEventData->m_arExec.begin(); Iter != pEventData->m_arExec.end(); Iter++ ) 
			{
				if( !RunNpcEvent( pNpc, (*Iter) ) )
				{
					return FALSE;
				}
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

// ????? ???? >.<
	case	EXEC_SAVE_COM_EVENT:
		SaveComEvent(pExec->m_ExecInt[0]);
		break;
//
	case	EXEC_RETURN:
		return FALSE;
		break;
/*
		case EXEC_SAY:		
			break;

		case EXEC_SELECT_MSG:
			SelectMsg( pExec );
			break;

		case EXEC_RUN_EVENT:
			{
				EVENT* pEvent = NULL;
				pEvent = m_pMain->m_Quest.GetData(m_pUserData->m_bZone);		
				if(!pEvent)	break;

				EVENT_DATA* pEventData = NULL;
				pEventData = pEvent->m_arEvent.GetData(pExec->m_ExecInt[0]);
				if(!pEventData) break;

				if( !CheckEventLogic(pEventData) )	break;

				list<EXEC*>::iterator	Iter;
				for( Iter = pEventData->m_arExec.begin(); Iter != pEventData->m_arExec.end(); Iter++ ) {
					if( !RunNpcEvent( pNpc, (*Iter) ) ){
						return FALSE;
					}
				}
			}
			break;

		case EXEC_ROB_NOAH:
			GoldLose(pExec->m_ExecInt[0]);
			break;

		case EXEC_GIVE_QUEST:
			break;

		case EXEC_QUEST_END:		
			break;

		case EXEC_QUEST_SAVE:
			break;

		case EXEC_RETURN:
			return FALSE;

		case EXEC_GIVE_NOAH:
			GoldGain(pExec->m_ExecInt[0]);
/////// These events are for the test quest. ///////
		case EXEC_ROB_ITEM:		
			if (!RobItem(pExec->m_ExecInt[0], pExec->m_ExecInt[1])) {
				return FALSE;	
			}
			break;

		case EXEC_GIVE_ITEM:	
			if (!GiveItem(pExec->m_ExecInt[0], pExec->m_ExecInt[1])) {
				return FALSE;
			}
			break;
*/

		default:
			break;
	}

	return TRUE;
}

BOOL CUser::RunEvent(EVENT_DATA *pEventData)
{
	EXEC* pExec = NULL;
	list<EXEC*>::iterator	Iter;

	for( Iter = pEventData->m_arExec.begin(); Iter != pEventData->m_arExec.end(); Iter++ ) 
	{
		pExec = (*Iter);
		if( !pExec ) break;

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
				break;

/*
			case EXEC_SAY:		
				break;

			case EXEC_SELECT_MSG:
				SelectMsg( pExec );
				break;

			case EXEC_RUN_EVENT:
				{								
					EVENT* pEvent = NULL;
					pEvent = m_pMain->m_Quest.GetData(m_pUserData->m_bZone);
					if(!pEvent)	break;

					EVENT_DATA* pEventData = NULL;
					pEventData = pEvent->m_arEvent.GetData(pExec->m_ExecInt[0]);
					if(!pEventData) break;

					if( !CheckEventLogic(pEventData) )	break;

					if( !RunEvent(pEventData) ) {
						return FALSE;
					}
				}
				break;

			case EXEC_ROB_NOAH:
				break;

			case EXEC_GIVE_QUEST:
				break;

			case EXEC_QUEST_END:		
				break;

			case EXEC_QUEST_SAVE:
				break;

			case EXEC_RETURN:
				return FALSE;
/////// These events are for the test quest. ///////
			case EXEC_ROB_ITEM:
				if (!RobItem(pExec->m_ExecInt[0], pExec->m_ExecInt[1])) {
					return FALSE;	
				}
				break;

			case EXEC_GIVE_ITEM:
				if (!GiveItem(pExec->m_ExecInt[0], pExec->m_ExecInt[1])) {
					return FALSE;
				}
				break;
*/

			default:
				break;
		}
	}

	return TRUE;
}


void CUser::ClassChange(char *pBuf)
{
	int index = 0, classcode = 0, send_index = 0, type=0, sub_type = 0, money = 0, old_money=0;
	char send_buff[128]; memset( send_buff, NULL, 128 );
	BOOL bSuccess = FALSE;

	type = GetByte( pBuf, index );

	if (type == CLASS_CHANGE_REQ)	
	{
		ClassChangeReq();
		return;
	}
	else if (type == ALL_POINT_CHANGE)	
	{
		AllPointChange();
		return;
	}
	else if (type == ALL_SKILLPT_CHANGE)	
	{
		AllSkillPointChange();
		return;
	}
	else if (type == CHANGE_MONEY_REQ)	
	{
		sub_type = GetByte( pBuf, index );

		money = pow(( m_pUserData->m_bLevel * 2 ), 3.4);
		money = ( money / 100 )*100;
		if( m_pUserData->m_bLevel < 30)		money = money * 0.4;
		else if( m_pUserData->m_bLevel >= 30 && m_pUserData->m_bLevel < 60 ) money = money * 1;
		else if( m_pUserData->m_bLevel >= 60 && m_pUserData->m_bLevel <= 90 ) money = money * 1.5;

		if( sub_type == 1 )		{
			if( m_pMain->m_sDiscount == 1 && m_pMain->m_byOldVictory == m_pUserData->m_bNation )		{
				old_money = money;
				money = money * 0.5;
				//TRACE("^^ ClassChange -  point Discount ,, money=%d->%d\n", old_money, money);
			}

			if( m_pMain->m_sDiscount == 2 )		{	
				old_money = money;
				money = money * 0.5;
			}

			SetByte( send_buff, WIZ_CLASS_CHANGE, send_index );
			SetByte( send_buff, CHANGE_MONEY_REQ, send_index );
			SetDWORD( send_buff, money, send_index );
			Send( send_buff, send_index );
		}
		else if( sub_type == 2 )		{	
			money = money * 1.5;			
			if( m_pMain->m_sDiscount == 1 && m_pMain->m_byOldVictory == m_pUserData->m_bNation )		{
				old_money = money;
				money = money * 0.5;
				//TRACE("^^ ClassChange -  skillpoint Discount ,, money=%d->%d\n", old_money, money);
			}

			if( m_pMain->m_sDiscount == 2 )		{	
				old_money = money;
				money = money * 0.5;
			}
			
			SetByte( send_buff, WIZ_CLASS_CHANGE, send_index );
			SetByte( send_buff, CHANGE_MONEY_REQ, send_index );
			SetDWORD( send_buff, money, send_index );
			Send( send_buff, send_index );
		}
		return;
	}

	classcode = GetByte( pBuf, index );

	switch( m_pUserData->m_sClass ) {
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

	memset( send_buff, NULL, 128 );	send_index = 0;
	if( !bSuccess ) {
		SetByte( send_buff, WIZ_CLASS_CHANGE, send_index );
		SetByte( send_buff, CLASS_CHANGE_RESULT, send_index );
		SetByte( send_buff, 0x00, send_index );
		Send( send_buff, send_index );
	}
	else {
		m_pUserData->m_sClass = classcode;
		if( m_sPartyIndex != -1 ) {
			SetByte( send_buff, WIZ_PARTY, send_index );
			SetByte( send_buff, PARTY_CLASSCHANGE, send_index );
			SetShort( send_buff, m_Sid, send_index );
			SetShort( send_buff, m_pUserData->m_sClass, send_index );
			m_pMain->Send_PartyMember(m_sPartyIndex, send_buff, send_index);
		}
	}
}


void CUser::RecvSelectMsg(char *pBuf)	// Receive menu reply from client.
{
	int index = 0, selevent, selnum;
	EVENT* pEvent = NULL;
	EVENT_DATA* pEventData = NULL;

	selnum = GetByte( pBuf, index );
	if( selnum < 0 || selnum > MAX_MESSAGE_EVENT )
		goto fail_return;

	selevent = m_iSelMsgEvent[selnum];	// Get the event number that needs to be processed next.

	pEvent = m_pMain->m_Event.GetData(m_pUserData->m_bZone);
	if(!pEvent)	goto fail_return;

	pEventData = pEvent->m_arEvent.GetData(selevent);
	if(!pEventData) goto fail_return;

	if( !CheckEventLogic(pEventData) )	goto fail_return;

	if( !RunEvent(pEventData) )
	{
		goto fail_return;
	}

	return;

fail_return:
	for (int i = 0 ; i < MAX_MESSAGE_EVENT ; i++) {
		m_iSelMsgEvent[i] = -1;
	}
}

void CUser::SendNpcSay(EXEC *pExec)
{
	int i, send_index = 0;
	char send_buf[128];	memset(send_buf, NULL, 128);

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
	char send_buf[128];		memset(send_buf, NULL, 128);

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

void CUser::NpcEvent(char *pBuf)
{
	if( m_pMain->m_bPointCheckFlag == FALSE)	return;	

	int index = 0, send_index = 0, nid = 0, i=0, temp_index = 0;
	char send_buf[2048];
	memset( send_buf, NULL, 2048);
	CNpc* pNpc = NULL;

	nid = GetShort( pBuf, index );
	pNpc = m_pMain->m_arNpcArray.GetData(nid);
	if( !pNpc ) return;

	switch( pNpc->m_tNpcType ) {
	case NPC_MERCHANT:
		SetByte( send_buf, WIZ_TRADE_NPC, send_index );
		SetDWORD( send_buf, pNpc->m_iSellingGroup, send_index );
		Send( send_buf, send_index );
		break;
	case NPC_TINKER:
		SetByte( send_buf, WIZ_REPAIR_NPC, send_index );
		SetDWORD( send_buf, pNpc->m_iSellingGroup, send_index );
		Send( send_buf, send_index );
		break;
	case NPC_CAPTAIN:
		SetByte( send_buf, WIZ_CLASS_CHANGE, send_index );
		SetByte( send_buf, CLASS_CHANGE_REQ, send_index );
		Send( send_buf, send_index );
		break;
	case NPC_OFFICER:
		SetShort( send_buf, 0, send_index );	// default 0 page
		m_pMain->m_KnightsManager.AllKnightsList( this, send_buf );
		break;
	case NPC_WAREHOUSE:
		SetByte( send_buf, WIZ_WAREHOUSE, send_index );
		SetByte( send_buf, WAREHOUSE_REQ, send_index );
		Send( send_buf, send_index );
		break;

	case NPC_WARP:
		break;

	case NPC_CLERIC:
	case NPC_COUPON:
	case NPC_MONK_KARUS:
	case NPC_MONK_ELMORAD:
		SetShort( send_buf, nid, send_index );
		ClientEvent( send_buf );
		break;
	}   
}

// NPC shops
void CUser::ItemTrade(char *pBuf)
{
	int index = 0, send_index = 0, itemid = 0, money = 0, count = 0, group = 0, npcid = 0;
	_ITEM_TABLE* pTable = NULL;
	char send_buf[128];
	CNpc* pNpc = NULL;
	memset( send_buf, NULL, 128);
	BYTE type, pos, destpos, result;

	if (isDead())
	{
		TRACE("### ItemTrade Fail : name=%s(%d), m_bResHpType=%d, hp=%d, x=%d, z=%d ###\n", m_pUserData->m_id, m_Sid, m_bResHpType, m_pUserData->m_sHp, (int)m_pUserData->m_curx, (int)m_pUserData->m_curz);
		result = 0x01;
		goto fail_return;
	}

	type = GetByte( pBuf, index );
	if( type == 0x01 ) {// item buy
		group = GetDWORD( pBuf, index );
		npcid = GetShort( pBuf, index );
	}
	itemid = GetDWORD( pBuf, index );
	pos = GetByte( pBuf, index );
	if( type == 0x03 ) 	// item move only
		destpos = GetByte( pBuf, index );
	else
		count = GetShort( pBuf, index );

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

	if( m_sExchangeUser != -1 ) goto fail_return;
	pTable = m_pMain->m_ItemtableArray.GetData( itemid );
	if( !pTable ) {
		result = 0x01;
		goto fail_return;
	}
	if( pos >= HAVE_MAX ) {
		result = 0x02;
		goto fail_return;
	}

	if( count <= 0 || count > MAX_ITEM_COUNT) {
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
		if( m_pUserData->m_iGold < (pTable->m_iBuyPrice*count) ) {
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
	SetByte( send_buf, 0x01, send_index );
	SetDWORD( send_buf, m_pUserData->m_iGold, send_index );
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