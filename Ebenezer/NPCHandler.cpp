#include "StdAfx.h"
#include "Map.h"
#include "EbenezerDlg.h"
#include "KnightsManager.h"
#include "User.h"

void CUser::ItemRepair(Packet & pkt)
{
	if (isDead())
		return;

	Packet result(WIZ_ITEM_REPAIR);
	uint32 money, itemid;
	uint16 durability, quantity, sNpcID;
	_ITEM_TABLE* pTable = NULL;
	CNpc *pNpc = NULL;
	uint8 sPos, sSlot;

	pkt >> sPos >> sSlot >> sNpcID >> itemid;
	if (sPos == 1 ) {	// SLOT
		if (sSlot >= SLOT_MAX) goto fail_return;
		if (m_sItemArray[sSlot].nNum != itemid) goto fail_return;
	}
	else if (sPos == 2 ) {	// INVEN
		if (sSlot >= HAVE_MAX) goto fail_return;
		if (m_sItemArray[SLOT_MAX+sSlot].nNum != itemid) goto fail_return;
	}

	pNpc = g_pMain.m_arNpcArray.GetData(sNpcID);
	if (pNpc == NULL
		|| pNpc->GetType() != NPC_TINKER
		|| !isInRange(pNpc, MAX_NPC_RANGE))
		return;

	pTable = g_pMain.GetItemPtr( itemid );
	if( !pTable ) goto fail_return;
	durability = pTable->m_sDuration;
	if( durability == 1 ) goto fail_return;
	if( sPos == 1 )
		quantity = pTable->m_sDuration - m_sItemArray[sSlot].sDuration;
	else if( sPos == 2 ) 
		quantity = pTable->m_sDuration - m_sItemArray[SLOT_MAX+sSlot].sDuration;
	
	money = (unsigned int)((((pTable->m_iBuyPrice-10) / 10000.0f) + pow((float)pTable->m_iBuyPrice, 0.75f)) * quantity / (double)durability);
	if( money > m_iGold ) goto fail_return;

	m_iGold -= money;
	if( sPos == 1 )
		m_sItemArray[sSlot].sDuration = durability;
	else if( sPos == 2 )
		m_sItemArray[SLOT_MAX+sSlot].sDuration = durability;

	result << uint8(1) << m_iGold;
	Send(&result);
	return;

fail_return:
	result << uint8(0) << m_iGold;
	Send(&result);
}

void CUser::ClientEvent(uint16 sNpcID)
{
	// Ensure AI's loaded
	if (!g_pMain.m_bPointCheckFlag
		|| isDead())
		return;

	int32 iEventID = 0;
	CNpc *pNpc = g_pMain.m_arNpcArray.GetData(sNpcID);
	if (pNpc == NULL
		|| !isInRange(pNpc, MAX_NPC_RANGE))
		return;

	m_sEventNid = sNpcID;
	m_sEventSid = pNpc->GetEntryID(); // For convenience purposes with Lua scripts.

	// Aww.
	if (pNpc->GetType() == NPC_KISS)
	{
		KissUser();
		return;
	}

	FastGuard lock(g_pMain.m_questNpcLock);
	QuestNpcList::iterator itr = g_pMain.m_QuestNpcList.find(pNpc->GetEntryID());
	if (itr == g_pMain.m_QuestNpcList.end())
		return;

	// Copy it. We should really lock the list, but nevermind.
	QuestHelperList & pList = itr->second;
	_QUEST_HELPER * pHelper = NULL;
	foreach(itr, pList)
	{
		if ((*itr) == NULL
			|| (*itr)->sEventDataIndex
			|| (*itr)->bEventStatus
			|| ((*itr)->bNation != 3 && (*itr)->bNation != GetNation())
			|| ((*itr)->bClass != 5 && !JobGroupCheck((*itr)->bClass)))
			continue;

		pHelper = (*itr);
		break;
	}

	if (pHelper == NULL)
		return;

	QuestV2RunEvent(pHelper, pHelper->nEventTriggerIndex);
}

void CUser::KissUser()
{
	Packet result(WIZ_KISS);
	result << uint32(GetID()) << m_sEventNid;
	GiveItem(910014000); // aw, you got a 'Kiss'. How literal.
	SendToRegion(&result);
}

void CUser::ClassChange(Packet & pkt)
{
	Packet result(WIZ_CLASS_CHANGE);
	bool bSuccess = false;
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
		if ((g_pMain.m_sDiscount == 1 && g_pMain.m_byOldVictory == GetNation())
			|| g_pMain.m_sDiscount == 2)
			money /= 2;

		result << uint8(CHANGE_MONEY_REQ) << money;
		Send(&result);
		return;
	}

	uint8 classcode = pkt.read<uint8>();
	switch (m_sClass)
	{
	case KARUWARRIOR:
		if( classcode == BERSERKER || classcode == GUARDIAN )
			bSuccess = true;
		break;
	case KARUROGUE:
		if( classcode == HUNTER || classcode == PENETRATOR )
			bSuccess = true;
		break;
	case KARUWIZARD:
		if( classcode == SORSERER || classcode == NECROMANCER )
			bSuccess = true;
		break;
	case KARUPRIEST:
		if( classcode == SHAMAN || classcode == DARKPRIEST )
			bSuccess = true;
		break;
	case ELMORWARRRIOR:
		if( classcode == BLADE || classcode == PROTECTOR )
			bSuccess = true;
		break;
	case ELMOROGUE:
		if( classcode == RANGER || classcode == ASSASSIN )
			bSuccess = true;
		break;
	case ELMOWIZARD:
		if( classcode == MAGE || classcode == ENCHANTER )
			bSuccess = true;
		break;
	case ELMOPRIEST:
		if( classcode == CLERIC || classcode == DRUID )
			bSuccess = true;
		break;
	}

	// Not allowed this job change
	if (!bSuccess)
	{
		result << uint8(CLASS_CHANGE_RESULT) << uint8(0);
		Send(&result);
		return;
	}

	m_sClass = classcode;
	if (isInParty())
	{
		// TO-DO: Move this somewhere better.
		result.SetOpcode(WIZ_PARTY);
		result << uint8(PARTY_CLASSCHANGE) << GetSocketID() << uint16(classcode);
		g_pMain.Send_PartyMember(m_sPartyIndex, &result);
	}
}

void CUser::RecvSelectMsg(Packet & pkt)	// Receive menu reply from client.
{
	uint8 bMenuID = pkt.read<uint8>();
	if (!AttemptSelectMsg(bMenuID))
		memset(&m_iSelMsgEvent, -1, sizeof(m_iSelMsgEvent));
}

bool CUser::AttemptSelectMsg(uint8 bMenuID)
{
	_QUEST_HELPER * pHelper = NULL;
	if (bMenuID >= MAX_MESSAGE_EVENT
		|| isDead()
		|| m_nQuestHelperID == 0)
		return false;

	// Get the event number that needs to be processed next.
	int32 selectedEvent = m_iSelMsgEvent[bMenuID];
	if (selectedEvent < 0
		|| (pHelper = g_pMain.m_QuestHelperArray.GetData(m_nQuestHelperID)) == NULL
		|| !QuestV2RunEvent(pHelper, selectedEvent))
		return false;

	return true;
}

void CUser::SelectMsg(uint8 bFlag, int32 nQuestID, int32 menuHeaderText, 
					  int32 menuButtonText[MAX_MESSAGE_EVENT], int32 menuButtonEvents[MAX_MESSAGE_EVENT])
{
	_QUEST_HELPER * pHelper = g_pMain.m_QuestHelperArray.GetData(m_nQuestHelperID);
	if (pHelper == NULL)
		return;

	// Send the menu to the client
	Packet result(WIZ_SELECT_MSG);
	result.SByte();

	result << m_sEventNid << bFlag << nQuestID << menuHeaderText;
	foreach_array_n(i, menuButtonText, MAX_MESSAGE_EVENT)
		result << menuButtonText[i];
	result << pHelper->strLuaFilename;
	Send(&result);

	// and store the corresponding event IDs.
	memcpy(&m_iSelMsgEvent, menuButtonEvents, sizeof(menuButtonEvents));
}

void CUser::NpcEvent(Packet & pkt)
{
	// Ensure AI is loaded first
	if (!g_pMain.m_bPointCheckFlag
		|| isDead())
		return;	

	Packet result;
	uint8 bUnknown = pkt.read<uint8>();
	uint16 sNpcID = pkt.read<uint16>();
	int32 nQuestID = pkt.read<int32>();

	CNpc *pNpc = g_pMain.m_arNpcArray.GetData(sNpcID);
	if (pNpc == NULL
		|| !isInRange(pNpc, MAX_NPC_RANGE))
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
		CKnightsManager::AllKnightsList(this, result);
		break;

	case NPC_WAREHOUSE:
		result.SetOpcode(WIZ_WAREHOUSE);
		result << uint8(WAREHOUSE_REQ);
		Send(&result);
		break;

	default:
		ClientEvent(sNpcID);
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
	{
		pkt >> group >> npcid;
		if (!g_pMain.m_bPointCheckFlag
			|| (pNpc = g_pMain.m_arNpcArray.GetData(npcid)) == NULL
			|| pNpc->GetType() != NPC_MERCHANT
			|| pNpc->m_iSellingGroup != group
			|| !isInRange(pNpc, MAX_NPC_RANGE))
			goto fail_return;
	}

	pkt >> itemid >> pos;

	if (type == 3) 	// Move only (this is so useless mgame -- why not just handle it with the CUser::ItemMove(). Gah.)
		pkt >> destpos;
	else
		pkt >> count;

	// Moving an item in the inventory
	if (type == 3)
	{
		if (pos >= HAVE_MAX || destpos >= HAVE_MAX
			|| itemid != m_sItemArray[SLOT_MAX+pos].nNum)
		{
			errorCode = 4;
			goto send_packet;
		}

		short duration = m_sItemArray[SLOT_MAX+pos].sDuration;
		short itemcount = m_sItemArray[SLOT_MAX+pos].sCount;
		m_sItemArray[SLOT_MAX+pos].nNum = m_sItemArray[SLOT_MAX+destpos].nNum;
		m_sItemArray[SLOT_MAX+pos].sDuration = m_sItemArray[SLOT_MAX+destpos].sDuration;
		m_sItemArray[SLOT_MAX+pos].sCount = m_sItemArray[SLOT_MAX+destpos].sCount;
		m_sItemArray[SLOT_MAX+destpos].nNum = itemid;
		m_sItemArray[SLOT_MAX+destpos].sDuration = duration;
		m_sItemArray[SLOT_MAX+destpos].sCount = itemcount;

		result << uint8(3);
		Send(&result);
		return;
	}

	if (isTrading()
		|| (pTable = g_pMain.GetItemPtr(itemid)) == NULL
		|| (type == 2 // if we're selling an item...
				&& (itemid >= ITEM_NO_TRADE // Cannot be traded, sold or stored.
					|| pTable->m_bRace == RACE_UNTRADEABLE))) // Cannot be traded or sold.
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
		if (m_sItemArray[SLOT_MAX+pos].nNum != 0)
		{
			if (m_sItemArray[SLOT_MAX+pos].nNum != itemid)
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
				&& (count + m_sItemArray[SLOT_MAX+pos].sCount) > MAX_ITEM_COUNT)
			{
				errorCode = 4;
				goto fail_return;				
			}
		}

		transactionPrice = ((uint32)pTable->m_iBuyPrice * count);
		if (m_iGold < transactionPrice)
		{
			errorCode = 3;
			goto fail_return;
		}

		if (((pTable->m_sWeight * count) + m_sItemWeight) > m_sMaxWeight)
		{
			errorCode = 4;
			goto fail_return;
		}

		m_sItemArray[SLOT_MAX+pos].nNum = itemid;
		m_sItemArray[SLOT_MAX+pos].sDuration = pTable->m_sDuration;
		m_sItemArray[SLOT_MAX+pos].sCount += count;
		m_iGold -= transactionPrice;

		if (!pTable->m_bCountable)
			m_sItemArray[SLOT_MAX+pos].nSerialNum = g_pMain.GenerateItemSerial();

		SendItemWeight();
	}
	// Selling an item to an NPC
	else
	{
		_ITEM_DATA *pItem = &m_sItemArray[SLOT_MAX+pos];
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

		if (m_iGold + transactionPrice > COIN_MAX)
			m_iGold = COIN_MAX;
		else
			m_iGold += transactionPrice;

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
		result << pTable->m_bSellingGroup << m_iGold << transactionPrice; // price bought or sold for
	Send(&result);
}