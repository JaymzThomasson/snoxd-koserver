#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"

void CUser::ShoppingMall(char *pBuf)
{
	int index = 0;
	BYTE subcommand = GetByte( pBuf, index );
		
	switch (subcommand)
	{
	case STORE_OPEN:
		TRACE("STORE_OPEN\n");
		break;

	case STORE_CLOSE:
		TRACE("STORE_CLOSE\n");
		HandleStoreClose();
		break;

	case STORE_BUY:
		TRACE("STORE_BUY\n");
		break;

	case STORE_MINI: // not sure what this is
		TRACE("STORE_MINI\n");
		break;

	case STORE_PROCESS:
		TRACE("STORE_PROCESS\n");
		break;

	case STORE_LETTER:
		TRACE("STORE_LETTER\n");
		LetterSystem(pBuf + index);
		break;

	default:
		TRACE("Unknown shoppingmall packet: %X\n", subcommand);
	}
}

void CUser::HandleStoreClose()
{
	char send_buff[256];
	int send_index = 0;

	m_bStoreOpen = false;

	// Tell Aujard we're closing the PUS so that we can call LOAD_WEB_ITEMMALL and load the extra items.
	SetByte(send_buff, WIZ_SHOPPING_MALL, send_index);
	SetByte(send_buff, STORE_CLOSE, send_index);
	SetShort(send_buff, GetSocketID(), send_index);
	m_pMain->m_LoggerSendQueue.PutData(send_buff, send_index);
}

void CUser::RecvStore(char *pData)
{
	int index = 0;
	BYTE opcode = GetByte(pData, index);
	switch (opcode)
	{
	case STORE_CLOSE:
		RecvStoreClose(pData+index);
		break;
	}
}

// Presumably received item data back from Aujard.
void CUser::RecvStoreClose(char *pData)
{
	char send_buff[2048];
	int index = 0, send_index = 0;
	BYTE result = GetByte(pData, index);

	// If it was succesful, i.e. it loaded data, give it to us
	if (result)
	{
		short count = GetShort(pData, index);
		for (int i = 0; i < count; i++)
		{
			int nItemID = GetDWORD(pData, index);
			short sCount = GetShort(pData, index);

			// reuse the GiveItem() method for giving them the item, just don't send that particular packet.
			GiveItem(nItemID, sCount, false); 
		}
	}
	
	SetByte(send_buff, WIZ_SHOPPING_MALL, send_index);
	SetByte(send_buff, STORE_CLOSE, send_index);

	// not sure if this limit's correct
	for (int i = 0; i < HAVE_MAX + SLOT_MAX + COSP_MAX + MBAG_MAX; i++)
	{
		SetDWORD(send_buff, m_pUserData->m_sItemArray[i].nNum, send_index);
		SetShort(send_buff, m_pUserData->m_sItemArray[i].sDuration, send_index);
		SetShort(send_buff, m_pUserData->m_sItemArray[i].sCount, send_index);
		SetByte(send_buff, 0, send_index);  // item type flag (e.g. rented)
		SetShort(send_buff, 0, send_index); // remaining time
		SetDWORD(send_buff, 0, send_index); // unknown
		SetDWORD(send_buff, 0, send_index); // expiration date
	}

	Send(send_buff, send_index);
}