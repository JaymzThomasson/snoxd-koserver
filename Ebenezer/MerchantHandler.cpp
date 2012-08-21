#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "Ebenezer.h"
#include "EbenezerDlg.h"
#include "User.h"

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
}

void CUser::MerchantClose()
{
}

void CUser::MerchantItemAdd(char *pBuf)
{
}

void CUser::MerchantItemCancel(char *pBuf)
{
}

void CUser::MerchantItemList(char *pBuf)
{
}

void CUser::MerchantItemBuy(char *pBuf)
{
}

void CUser::MerchantInsert(char *pBuf)
{
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
