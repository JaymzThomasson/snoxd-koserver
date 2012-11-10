#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "Ebenezer.h"
#include "EbenezerDlg.h"
#include "User.h"

enum MerchantOpenResponseCodes
{
MERCHANT_OPEN_SUCCESS = 1,
MERCHANT_OPEN_NO_SESSION = -1,
MERCHANT_OPEN_DEAD = -2,
MERCHANT_OPEN_TRADING = -3,
MERCHANT_OPEN_MERCHANTING = -4,
MERCHANT_OPEN_INVALID_ZONE = -5,
MERCHANT_OPEN_SHOPPING = -6,
MERCHANT_OPEN_UNDERLEVELED = 30
};

void CUser::MerchantProcess(char *pBuf)
{
	int index = 0;
	BYTE subcommand = GetByte(pBuf, index);

	switch (subcommand)
	{
		// Regular merchants
		case MERCHANT_OPEN: 
			MerchantOpen(pBuf+index); 
			break;

		case MERCHANT_CLOSE: 
			MerchantClose(); 
			break;

		case MERCHANT_ITEM_ADD: 
			MerchantItemAdd(pBuf+index); 
			break;

		case MERCHANT_ITEM_CANCEL: 
			MerchantItemCancel(pBuf+index); 
			break;

		case MERCHANT_ITEM_LIST: 
			MerchantItemList(pBuf+index); 
			break;

		case MERCHANT_ITEM_BUY: 
			MerchantItemBuy(pBuf+index); 
			break;

		case MERCHANT_INSERT: 
			MerchantInsert(pBuf+index); 
			break;

		case MERCHANT_TRADE_CANCEL: 
			CancelMerchant(); 
			break;

#if __VERSION >= 1700
		// Buying merchants
		case MERCHANT_BUY_OPEN: 
			BuyingMerchantOpen(pBuf+index); 
			break;

		case MERCHANT_BUY_CLOSE: 
			BuyingMerchantClose(); 
			break;

		case MERCHANT_BUY_LIST: 
			BuyingMerchantList(pBuf+index); 
			break;

		case MERCHANT_BUY_INSERT: 
			BuyingMerchantInsert(pBuf+index); 
			break;

		case MERCHANT_BUY_BUY: // seeya!
			BuyingMerchantBuy(pBuf+index); 
			break;
#endif
	}
}

/*
	Regular merchants
*/
void CUser::MerchantOpen(char *pBuf)
{
	int index = 0, send_index = 0;
	char send_buff[1024];

	SetByte( send_buff, WIZ_MERCHANT, send_index );
	SetByte( send_buff, MERCHANT_OPEN, send_index );

		if( isDead() ) {
			SetShort( send_buff, MERCHANT_OPEN_DEAD, send_index );
			Send( send_buff, send_index );
			return;
		}

		if( isStoreOpen() ) {
			SetShort( send_buff, MERCHANT_OPEN_SHOPPING, send_index );
			Send( send_buff, send_index );
			return;
		}

		if( isTrading() ) {
			SetShort( send_buff, MERCHANT_OPEN_TRADING, send_index );
			Send( send_buff, send_index );
			return;
		}

		if( getZoneID() > 21 || getZoneID() < 1 ) {
			SetShort( send_buff, MERCHANT_OPEN_INVALID_ZONE, send_index );
			Send( send_buff, send_index );
			return;
		}

		if( getLevel() < 30 ) {
			SetShort( send_buff, MERCHANT_OPEN_UNDERLEVELED, send_index );
			Send( send_buff, send_index );
			return;
		}

		if(m_bIsMerchanting) {
			MerchantClose();
			SetShort( send_buff, 0, send_index );
			Send( send_buff, send_index );
			return;
		}
		
	SetShort( send_buff, MERCHANT_OPEN_SUCCESS, send_index );
		for(int i = 0; i < MAX_MERCH_ITEMS; i++) {
				ClearSellingItems(i); // Making sure the m_arSellingItems array is empty before filling it up again, incase it's leftover from a previous merchant.
		}
	Send( send_buff, send_index );
}

void CUser::MerchantClose()
{
	int send_index = 0;
	char send_buff[1024];

	if( m_bIsMerchanting ) {
			m_bIsMerchanting = FALSE;
			GiveMerchantItems(); //Give back to the user that which hasn't been sold, if any.
			for(int i = 0; i < MAX_MERCH_ITEMS; i++) {
				ClearSellingItems(i);
			}
			SetByte( send_buff, WIZ_MERCHANT, send_index );
			SetByte( send_buff, MERCHANT_CLOSE, send_index );
			SetByte( send_buff, -1, send_index );
			SetByte( send_buff, -1, send_index );
			Send( send_buff, send_index );
	}
}

void CUser::MerchantItemAdd(char *pBuf)
{
	int nItem = 0, nGold = 0, index = 0, send_index = 0;
	unsigned short nCount = 0;
	short nDuration = 0;
	__int64 nSerialNum = 0;
	BYTE nInventorySlot = 0, nSellingSlot = 0, nMode = 0;
	char send_buff[1024];

	nItem = GetDWORD( pBuf, index );
	nCount = GetShort( pBuf, index );
	nGold = GetDWORD( pBuf, index );
	nInventorySlot = GetByte( pBuf, index ); //It sends the "actual" inventory slot hence we need to add 14 "slots" when comparing to m_sItemArray.
	nSellingSlot = GetByte( pBuf, index );
	nMode = GetByte( pBuf, index ); // Might be a flag for normal / "premium" merchant mode, once skills are implemented take another look at this.

	if( m_pUserData->m_sItemArray[nInventorySlot+14].nNum != nItem || m_pUserData->m_sItemArray[nInventorySlot+14].sCount < nCount || nSellingSlot >= MAX_MERCH_ITEMS)
			return;

	nDuration = m_pUserData->m_sItemArray[nInventorySlot+14].sDuration;
	nSerialNum = m_pUserData->m_sItemArray[nInventorySlot+14].nSerialNum;

	m_arSellingItems[nSellingSlot].nNum = nItem;
	m_arSellingItems[nSellingSlot].nPrice = nGold;
	m_arSellingItems[nSellingSlot].sCount = nCount;
	m_arSellingItems[nSellingSlot].sDuration = nDuration;
	m_arSellingItems[nSellingSlot].nSerialNum = nSerialNum;
	m_arSellingItems[nSellingSlot].nOriginalSlot = nInventorySlot;

	SetByte( send_buff, WIZ_MERCHANT, send_index );
	SetByte( send_buff, MERCHANT_ITEM_ADD, send_index );
	SetShort( send_buff, 0x01, send_index );
	SetDWORD( send_buff, nItem, send_index );
	SetShort( send_buff, nCount, send_index );
	SetShort( send_buff, nDuration, send_index );
	SetDWORD( send_buff, nGold, send_index );
	SetByte( send_buff, nInventorySlot, send_index );
	SetByte( send_buff, nSellingSlot, send_index );
	Send( send_buff, send_index );
}

void CUser::MerchantItemCancel(char *pBuf)
{
	int index = 0, send_index = 0;
	BYTE nSellingSlot = 0;
	char send_buff[1024];
	nSellingSlot = GetByte( pBuf, index );
	
	if( m_arSellingItems[nSellingSlot].nNum != 0 && nSellingSlot < MAX_MERCH_ITEMS) {
			ClearSellingItems(nSellingSlot);

			SetByte( send_buff, WIZ_MERCHANT, send_index );
			SetByte( send_buff, MERCHANT_ITEM_CANCEL, send_index );
			SetShort( send_buff, 0x01, send_index );
			SetByte( send_buff, nSellingSlot, send_index );
			Send( send_buff, send_index );
	}
}

void CUser::MerchantItemList(char *pBuf)
{
}

void CUser::MerchantItemBuy(char *pBuf)
{
}

void CUser::MerchantInsert(char *pBuf)
{
	int index = 0, send_index = 0;
	short mLength = 0;
	char mText[40];
	char send_buff[1024];

	mLength = GetShort( pBuf, index );
	if(mLength > 40)
		return;

	m_bIsMerchanting = TRUE;

	TakeMerchantItems(); // Removing the items from the user's inventory

	GetString( mText, pBuf, mLength, index );

	SetByte( send_buff, WIZ_MERCHANT, send_index );
	SetByte( send_buff, MERCHANT_INSERT, send_index );
	SetShort( send_buff, 0x01, send_index );
	SetShort( send_buff, mLength, send_index );
	SetString( send_buff, mText, mLength, send_index);
	SetShort( send_buff, m_Sid, send_index );
	SetByte( send_buff, 0x00, send_index ); // 0 is for "normal" merchant mode, 1 for "premium" merchant mode. Send "normal" until we have support for the skills.

	for( int i = 0; i < MAX_MERCH_ITEMS ;i++ ) {
		SetDWORD( send_buff, m_arSellingItems[i].nNum, send_index);
	}

	Send( send_buff, send_index );
}

void CUser::TakeMerchantItems()
{
	BYTE nOriginalSlot = 0;
	for (int i = 0; i < MAX_MERCH_ITEMS ;i++ ) {
		nOriginalSlot = m_arSellingItems[i].nOriginalSlot + 14; //Still need to add 14 to make sure it matches with our m_sItemArray place!
		if(nOriginalSlot = 0)
			return;
		m_pUserData->m_sItemArray[nOriginalSlot].nNum = 0;
		m_pUserData->m_sItemArray[nOriginalSlot].nSerialNum = 0;
		m_pUserData->m_sItemArray[nOriginalSlot].sCount = 0;
		m_pUserData->m_sItemArray[nOriginalSlot].sDuration = 0;
	}
}

void CUser::GiveMerchantItems()
{
	byte nOriginalSlot = 0;
	for (int i = 0; i < MAX_MERCH_ITEMS ;i++) {
		nOriginalSlot = m_arSellingItems[i].nOriginalSlot + 14; //Still need to add 14 to make sure it matches with our m_sItemArray place!
		if(nOriginalSlot = 0)
			return;
		m_pUserData->m_sItemArray[nOriginalSlot].nNum = m_arSellingItems[i].nNum;
		m_pUserData->m_sItemArray[nOriginalSlot].nSerialNum = m_arSellingItems[i].nSerialNum;
		m_pUserData->m_sItemArray[nOriginalSlot].sCount = m_arSellingItems[i].sCount;
		m_pUserData->m_sItemArray[nOriginalSlot].sDuration = m_arSellingItems[i].sDuration;
	}
}

void CUser::ClearSellingItems(int nSellingSlot) 
{
	m_arSellingItems[nSellingSlot].nNum = 0;
	m_arSellingItems[nSellingSlot].nPrice = 0;
	m_arSellingItems[nSellingSlot].nSerialNum = 0;
	m_arSellingItems[nSellingSlot].sCount = 0;
	m_arSellingItems[nSellingSlot].sDuration = 0;
	m_arSellingItems[nSellingSlot].nOriginalSlot = 0;
}

void CUser::CancelMerchant()
{
}

/*
	Buying merchants: 1.7XX only
*/
void CUser::BuyingMerchantOpen(char *pBuf)
{
}

void CUser::BuyingMerchantClose()
{
}

void CUser::BuyingMerchantInsert(char *pBuf)
{
}

void CUser::BuyingMerchantList(char *pBuf)
{
}

void CUser::BuyingMerchantBuy(char *pBuf)
{
}
