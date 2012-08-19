#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "Ebenezer.h"
#include "EbenezerDlg.h"
#include "User.h"

void CUser::MarketBBS(char *pBuf)
{
	return; // disabled until we can figure out whether this system's still used
	int index = 0;
	BYTE subcommand = GetByte( pBuf, index );
		
	MarketBBSBuyPostFilter();		// Get rid of empty slots.
	MarketBBSSellPostFilter();

	switch( subcommand ) {
		case MARKET_BBS_REGISTER :			// When you register a message on the Market BBS.
			MarketBBSRegister(pBuf+index);
			break;
		case MARKET_BBS_DELETE :			// When you delete your message on the Market BBS.
			MarketBBSDelete(pBuf+index);
			break;
		case MARKET_BBS_REPORT :			// Get the 'needed' messages from the Market BBS.
			MarketBBSReport(pBuf+index, MARKET_BBS_REPORT);
			break;
		case MARKET_BBS_OPEN:				// When you first open the Market BBS.
			MarketBBSReport(pBuf+index, MARKET_BBS_OPEN);		
			break;
		case MARKET_BBS_REMOTE_PURCHASE:	// When you agree to spend money on remote bartering.
			MarketBBSRemotePurchase(pBuf+index);
			break;
		case MARKET_BBS_MESSAGE:			// USE ONLY IN EMERGENCY!!!
			MarketBBSMessage(pBuf+index);	
			break;
	}
}

void CUser::MarketBBSRegister(char *pBuf)
{
	CUser* pUser = NULL;	// Basic Initializations. 	
	int index = 0, send_index = 0; BYTE result = 0;	BYTE sub_result = 1;			 
	char send_buff[256]; memset( send_buff, NULL, 256 );
	short title_len = 0; short message_len = 0; BYTE buysell_index = 0; 
	int  i = 0, j = 0; int page_index = 0;

	buysell_index = GetByte(pBuf, index);	// Buy or sell?

	if (buysell_index == MARKET_BBS_BUY) {
		if (m_pUserData->m_iGold < BUY_POST_PRICE) {
			sub_result = 2;
			goto fail_return;
		}
	}
	else if (buysell_index == MARKET_BBS_SELL) {
		if (m_pUserData->m_iGold < SELL_POST_PRICE) {
			sub_result = 2;
			goto fail_return;
		}
	}
		
	for (i = 0 ; i < MAX_BBS_POST ; i++) {
		if (buysell_index == MARKET_BBS_BUY) {			// Buy 
			if (m_pMain->m_sBuyID[i] == -1) {
				m_pMain->m_sBuyID[i] = m_Sid;
				title_len = GetShort(pBuf, index);		
				GetString(m_pMain->m_strBuyTitle[i], pBuf, title_len, index);		
				message_len = GetShort(pBuf, index);
				GetString(m_pMain->m_strBuyMessage[i], pBuf, message_len, index);
				m_pMain->m_iBuyPrice[i] = GetDWORD(pBuf, index);
				m_pMain->m_fBuyStartTime[i] = TimeGet();
				result = 1;
				break;
			}
		}
		else if (buysell_index == MARKET_BBS_SELL) {	// Sell
			if (m_pMain->m_sSellID[i] == -1) {
				m_pMain->m_sSellID[i] = m_Sid;
				title_len = GetShort(pBuf, index);	
				GetString(m_pMain->m_strSellTitle[i], pBuf, title_len, index);		
				message_len = GetShort(pBuf, index);
				GetString(m_pMain->m_strSellMessage[i], pBuf, message_len, index);
				m_pMain->m_iSellPrice[i] = GetDWORD(pBuf, index);				
				m_pMain->m_fSellStartTime[i] = TimeGet();
				result = 1;
				break;
			}
		}
		else goto fail_return;							// Error 
	}
	
	if (result == 0) goto fail_return;	// No spaces available

	SetByte( send_buff, WIZ_GOLD_CHANGE, send_index );		// Money removal packet...
	SetByte( send_buff, 0x02, send_index );
	if (buysell_index == MARKET_BBS_BUY) {
		m_pUserData->m_iGold -= BUY_POST_PRICE;
		SetDWORD( send_buff, BUY_POST_PRICE, send_index );
	}
	else if (buysell_index == MARKET_BBS_SELL) {
		m_pUserData->m_iGold -= SELL_POST_PRICE;		
		SetDWORD( send_buff, SELL_POST_PRICE, send_index );	
	}
	SetDWORD( send_buff, m_pUserData->m_iGold, send_index );
	Send( send_buff, send_index );

	page_index = i / MAX_BBS_PAGE;
	send_index = 0; memset( send_buff, NULL, 256 );
	SetByte(send_buff, buysell_index, send_index);
	SetShort(send_buff, page_index, send_index);
	MarketBBSReport(send_buff, MARKET_BBS_REGISTER);	
	return;

fail_return:
	send_index = 0; memset( send_buff, NULL, 256 );
	SetByte(send_buff, WIZ_MARKET_BBS, send_index);
	SetByte(send_buff, MARKET_BBS_REGISTER, send_index);
	SetByte(send_buff, buysell_index, send_index);
	SetByte(send_buff, result, send_index);
	SetByte(send_buff, sub_result, send_index);
	Send(send_buff, send_index);
	return;			
}

void CUser::MarketBBSDelete(char *pBuf)
{
	CUser* pUser = NULL;	// Basic Initializations. 	
	int index = 0, send_index = 0; BYTE result = 0;	BYTE sub_result = 1;			 
	char send_buff[256]; memset( send_buff, NULL, 256 );
	short delete_id = 0; BYTE buysell_index = 0; 
	int  i = 0, j = 0; 

	buysell_index = GetByte(pBuf, index);	// Buy or sell?
	delete_id = GetShort(pBuf, index);		// Which message should I delete? 

	if (buysell_index == MARKET_BBS_BUY) {	// Buy 
		if (m_pMain->m_sBuyID[delete_id] == m_Sid || m_pUserData->m_bAuthority == 0) {
			MarketBBSBuyDelete(delete_id);
			result = 1;	
		}
		else goto fail_return;
	}
	else if (buysell_index == MARKET_BBS_SELL) {	// Sell
		if (m_pMain->m_sSellID[delete_id] == m_Sid || m_pUserData->m_bAuthority == 0) {
			MarketBBSSellDelete(delete_id);
			result = 1;
		}
		else goto fail_return;
	}
	else goto fail_return;			// Error 
	
	SetShort(send_buff, buysell_index, send_index);
	SetShort(send_buff, 0, send_index);
	MarketBBSReport(send_buff, MARKET_BBS_DELETE);
	return;

fail_return:
	SetByte(send_buff, WIZ_MARKET_BBS, send_index);
	SetByte(send_buff, MARKET_BBS_DELETE, send_index);
	SetByte(send_buff, buysell_index, send_index);
	SetByte(send_buff, result, send_index);
	SetByte(send_buff, sub_result, send_index);
	Send(send_buff, send_index);
	return;	
}

void CUser::MarketBBSReport(char *pBuf, BYTE type)
{
	CUser* pUser = NULL;	// Basic Initializations. 	
	int index = 0, send_index = 0;				
	BYTE result = 0; BYTE sub_result = 1; short bbs_len = 0;
	char send_buff[8192]; memset(send_buff, NULL, 8192);

	short page_index = 0; BYTE buysell_index = 0;
	short start_counter = 0; short valid_counter = 0 ;
	int  i = 0, j = 0; short BBS_Counter = 0;

	short title_length = 0;
	short message_length = 0;
	
	buysell_index = GetByte(pBuf, index);	// Buy or sell?
	page_index = GetShort(pBuf, index);		// Which message should I delete? 

	start_counter = page_index * MAX_BBS_PAGE;
	
	if (type == MARKET_BBS_OPEN) {
		start_counter = 0;
		page_index = 0;
	}

	if ( start_counter < 0 ) goto fail_return;
	if ( start_counter > MAX_BBS_POST ) goto fail_return;

	result = 1;

	SetByte(send_buff, WIZ_MARKET_BBS, send_index);
	SetByte(send_buff, type, send_index);
	SetByte(send_buff, buysell_index, send_index);
	SetByte(send_buff, result, send_index);

	for (i = 0 ; i < MAX_BBS_POST ; i++) {
		if (buysell_index == MARKET_BBS_BUY) {
			if (m_pMain->m_sBuyID[i] == -1) continue;

			pUser = m_pMain->GetUserPtr(m_pMain->m_sBuyID[i]);
			if (pUser == NULL)
			{
				MarketBBSBuyDelete(i);
				continue ;	
			}

			BBS_Counter++;		// Increment number of total messages.
				
			if (i < start_counter) continue;	// Range check codes.
			if (valid_counter >= MAX_BBS_PAGE) continue;

			SetShort(send_buff, m_pMain->m_sBuyID[i], send_index);

			SetShort(send_buff, strlen(pUser->m_pUserData->m_id), send_index);	
			SetString(send_buff, pUser->m_pUserData->m_id, strlen(pUser->m_pUserData->m_id), send_index);

			title_length = strlen(m_pMain->m_strBuyTitle[i]);
			if (title_length > MAX_BBS_TITLE) {
				title_length = MAX_BBS_TITLE;
			}

			SetShort(send_buff, title_length, send_index);
			SetString(send_buff, m_pMain->m_strBuyTitle[i], title_length, send_index);
//			SetShort(send_buff, strlen(m_pMain->m_strBuyTitle[i]), send_index);
//			SetString(send_buff, m_pMain->m_strBuyTitle[i], strlen(m_pMain->m_strBuyTitle[i]), send_index);

			message_length = strlen(m_pMain->m_strBuyMessage[i]);
			if (message_length > MAX_BBS_MESSAGE) {
				message_length = MAX_BBS_MESSAGE ;
			}

			SetShort(send_buff, message_length, send_index);
			SetString(send_buff, m_pMain->m_strBuyMessage[i], message_length, send_index);
//			SetShort(send_buff, strlen(m_pMain->m_strBuyMessage[i]), send_index);
//			SetString(send_buff, m_pMain->m_strBuyMessage[i], strlen(m_pMain->m_strBuyMessage[i]), send_index);

			SetDWORD(send_buff, m_pMain->m_iBuyPrice[i], send_index);
			SetShort(send_buff, i, send_index);

			valid_counter++;			
		}
		else if (buysell_index == MARKET_BBS_SELL) {
			if (m_pMain->m_sSellID[i] == -1) continue;

			pUser = m_pMain->GetUserPtr(m_pMain->m_sSellID[i]);
			if (pUser == NULL)
			{
				MarketBBSSellDelete(i);
				continue ;	
			}

			BBS_Counter++;
			
			if (i < start_counter) continue;	// Range check codes.
			if (valid_counter >= MAX_BBS_PAGE) continue;
		
			SetShort(send_buff, m_pMain->m_sSellID[i], send_index);

			SetShort(send_buff, strlen(pUser->m_pUserData->m_id), send_index);	
			SetString(send_buff, pUser->m_pUserData->m_id, strlen(pUser->m_pUserData->m_id), send_index);

			title_length = strlen(m_pMain->m_strSellTitle[i]);
			if (title_length > MAX_BBS_TITLE) {
				title_length = MAX_BBS_TITLE;
			}

			SetShort(send_buff, title_length, send_index);
			SetString(send_buff, m_pMain->m_strSellTitle[i], title_length, send_index);
//			SetShort(send_buff, strlen(m_pMain->m_strSellTitle[i]), send_index);
//			SetString(send_buff, m_pMain->m_strSellTitle[i], strlen(m_pMain->m_strSellTitle[i]), send_index);

			message_length = strlen(m_pMain->m_strSellMessage[i]);
			if (message_length > MAX_BBS_MESSAGE) {
				message_length = MAX_BBS_MESSAGE ;
			}

			SetShort(send_buff, message_length, send_index);
			SetString(send_buff, m_pMain->m_strSellMessage[i], message_length, send_index);
//			SetShort(send_buff, strlen(m_pMain->m_strSellMessage[i]), send_index);
//			SetString(send_buff, m_pMain->m_strSellMessage[i], strlen(m_pMain->m_strSellMessage[i]), send_index);

			SetDWORD(send_buff, m_pMain->m_iSellPrice[i], send_index);
			SetShort(send_buff, i, send_index);

			valid_counter++;	// Increment number of messages on the requested page			
		}	
	}
//
	if (valid_counter == 0 && page_index > 0) {
		goto fail_return1;
	}
//
	if (valid_counter < MAX_BBS_PAGE) {	// You still need to fill up slots.
		for (j = valid_counter ; j < MAX_BBS_PAGE ; j++) {		
			SetShort(send_buff, -1, send_index);
			SetShort(send_buff, 0, send_index);	
			SetString(send_buff, NULL, 0, send_index);
			SetShort(send_buff, 0, send_index);
			SetString(send_buff, NULL, 0, send_index);
			SetShort(send_buff, 0, send_index);
			SetString(send_buff, NULL, 0, send_index);
			SetDWORD(send_buff, 0, send_index);
			SetShort(send_buff, -1, send_index);

			valid_counter++;
		}
	}

	SetShort(send_buff, page_index, send_index);
	SetShort(send_buff, BBS_Counter, send_index);
	Send(send_buff, send_index);
	return;

fail_return:
	send_index = 0; memset(send_buff, NULL, 8192);
	SetByte(send_buff, WIZ_MARKET_BBS, send_index);
	SetByte(send_buff, MARKET_BBS_REPORT, send_index);
	SetByte(send_buff, buysell_index, send_index);
	SetByte(send_buff, result, send_index);
	SetByte(send_buff, sub_result, send_index);
	Send(send_buff, send_index);
	return;

fail_return1:
	send_index = 0; memset(send_buff, NULL, 8192); 	
	SetShort(send_buff, buysell_index, send_index);
	SetShort(send_buff, page_index - 1, send_index);
	MarketBBSReport(send_buff, type);	
	return;
}

void CUser::MarketBBSRemotePurchase(char *pBuf)
{
	CUser* pUser = NULL;	
	int send_index = 0;	
	BYTE buysell_index = 0; short message_index = -1;
	BYTE result = 0; BYTE sub_result = 1; short tid = -1 ; int index = 0;
	char send_buff[256]; memset(send_buff, NULL, 256); int i = 0;

	buysell_index = GetByte(pBuf, index);		// Buy or sell?
	message_index = GetShort(pBuf, index);		// Which message should I retrieve? 

	if (buysell_index != MARKET_BBS_BUY && buysell_index != MARKET_BBS_SELL) goto fail_return;

	if (buysell_index == MARKET_BBS_BUY) {
		if (m_pMain->m_sBuyID[message_index] == -1) {
			sub_result = 3;
			goto fail_return;
		}

		pUser = m_pMain->GetUserPtr(m_pMain->m_sBuyID[message_index]);
		if (pUser == NULL)
		{	// Something wrong with the target ID.
			sub_result = 1;
			goto fail_return;
		}
	}
	else if (buysell_index == MARKET_BBS_SELL) {
		if (m_pMain->m_sSellID[message_index] == -1) {
			sub_result = 3;
			goto fail_return;
		}

		pUser = m_pMain->GetUserPtr(m_pMain->m_sSellID[message_index]);
		if (pUser == NULL)
		{	// Something wrong with the target ID.
			sub_result = 1;
			goto fail_return;
		}
	}
	
	if ( m_pUserData->m_iGold >= REMOTE_PURCHASE_PRICE) {	// Check if user has gold.
		m_pUserData->m_iGold -= REMOTE_PURCHASE_PRICE;
		
		SetByte( send_buff, WIZ_GOLD_CHANGE, send_index );	
		SetByte( send_buff, 0x02, send_index );
		SetDWORD( send_buff, REMOTE_PURCHASE_PRICE, send_index );
		SetDWORD( send_buff, m_pUserData->m_iGold, send_index );
		Send( send_buff, send_index );

		result = 1;
	}
	else {	// User does not have gold.
		sub_result = 2;
	}

fail_return:
	memset(send_buff, NULL, 256); send_index = 0;
	SetByte(send_buff, WIZ_MARKET_BBS, send_index);
	SetByte(send_buff, MARKET_BBS_REMOTE_PURCHASE, send_index);
	SetByte(send_buff, buysell_index, send_index);
	SetByte(send_buff, result, send_index);

	if (result == 0) { // Only on errors!!!
		SetByte(send_buff, sub_result, send_index);
	}

	Send(send_buff, send_index);
}

void CUser::MarketBBSTimeCheck()
{
	CUser* pUser = NULL;	// Basic Initializations. 	
	int send_index = 0;			
	char send_buff[256]; memset(send_buff, NULL, 256);	
	float currenttime = 0.0f;
	int price = 0;

	currenttime = TimeGet();

	for (int i = 0 ; i < MAX_BBS_POST ; i++) {		
		if (m_pMain->m_sBuyID[i] != -1) {	// BUY!!!
			pUser = m_pMain->GetUserPtr(m_pMain->m_sBuyID[i]);			
			if (pUser == NULL)
			{
				MarketBBSBuyDelete(i);
				continue;
			}
			
			if (m_pMain->m_fBuyStartTime[i] + BBS_CHECK_TIME < currenttime) {
				if (pUser->m_pUserData->m_iGold >= BUY_POST_PRICE) {
					pUser->m_pUserData->m_iGold -= BUY_POST_PRICE ;
					m_pMain->m_fBuyStartTime[i] = TimeGet();

					memset(send_buff, NULL, 256); send_index = 0;	
					SetByte( send_buff, WIZ_GOLD_CHANGE, send_index );	// Now the target
					SetByte( send_buff, 0x02, send_index );
					SetDWORD( send_buff, BUY_POST_PRICE, send_index );
					SetDWORD( send_buff, pUser->m_pUserData->m_iGold, send_index );
					pUser->Send( send_buff, send_index );	
				}
				else {
					MarketBBSBuyDelete(i);
				}
			}
		}
		
		if (m_pMain->m_sSellID[i] != -1) {	// SELL!!!
			pUser = m_pMain->GetUserPtr(m_pMain->m_sSellID[i]);
			if (pUser == NULL)
			{
				MarketBBSSellDelete(i);
				continue;
			}
		
			if (m_pMain->m_fSellStartTime[i] + BBS_CHECK_TIME < currenttime) {
				if (pUser->m_pUserData->m_iGold >= SELL_POST_PRICE) {
					pUser->m_pUserData->m_iGold -= SELL_POST_PRICE ;
					m_pMain->m_fSellStartTime[i] = TimeGet();

					memset(send_buff, NULL, 256); send_index = 0;
					SetByte( send_buff, WIZ_GOLD_CHANGE, send_index );	// Now the target
					SetByte( send_buff, 0x02, send_index );
					SetDWORD( send_buff, SELL_POST_PRICE, send_index );
					SetDWORD( send_buff, pUser->m_pUserData->m_iGold, send_index );
					pUser->Send( send_buff, send_index );	
				}
				else {
					MarketBBSSellDelete(i);
				}
			}
		}
	}
}

void CUser::MarketBBSUserDelete()
{
	for (int i = 0 ; i < MAX_BBS_POST ; i++) {
		if (m_pMain->m_sBuyID[i] == m_Sid) {	// BUY!!!
			MarketBBSBuyDelete(i);
		}

		if (m_pMain->m_sSellID[i] == m_Sid) {	// SELL!!
			MarketBBSSellDelete(i);
		}	
	}
}

void CUser::MarketBBSBuyDelete(short index)
{
	m_pMain->m_sBuyID[index] = -1;
	memset( m_pMain->m_strBuyTitle[index], NULL, MAX_BBS_TITLE);
	memset( m_pMain->m_strBuyMessage[index], NULL, MAX_BBS_MESSAGE);
	m_pMain->m_iBuyPrice[index] = 0;
	m_pMain->m_fBuyStartTime[index] = 0.0f;	
}

void CUser::MarketBBSSellDelete(short index)
{
	m_pMain->m_sSellID[index] = -1;
	memset( m_pMain->m_strSellTitle[index], NULL, MAX_BBS_TITLE);
	memset( m_pMain->m_strSellMessage[index], NULL, MAX_BBS_MESSAGE);
	m_pMain->m_iSellPrice[index] = 0;
	m_pMain->m_fSellStartTime[index] = 0.0f;
}

void CUser::MarketBBSMessage(char *pBuf)
{
	int index = 0, send_index = 0;				
	BYTE result = 0; BYTE sub_result = 1; 
	char send_buff[256]; memset(send_buff, NULL, 256);

	short message_index = 0; BYTE buysell_index = 0;
	short message_length = 0;

	buysell_index = GetByte(pBuf, index);		// Buy or sell?
	message_index = GetShort(pBuf, index);		// Which message should I retrieve? 

	if (buysell_index != MARKET_BBS_BUY && buysell_index != MARKET_BBS_SELL) goto fail_return;

	SetByte(send_buff, WIZ_MARKET_BBS, send_index);
	SetByte(send_buff, MARKET_BBS_MESSAGE, send_index);
	SetByte(send_buff, result, send_index);

	switch(buysell_index) {
		case MARKET_BBS_BUY:
			if (m_pMain->m_sBuyID[message_index] == -1) goto fail_return;

			message_length = strlen(m_pMain->m_strBuyMessage[message_index]);
			if (message_length > MAX_BBS_MESSAGE) {
				message_length = MAX_BBS_MESSAGE ;
			}

			SetShort(send_buff, message_length, send_index);
			SetString(send_buff, m_pMain->m_strBuyMessage[message_index], message_length, send_index);
			break;

		case MARKET_BBS_SELL:
			if (m_pMain->m_sSellID[message_index] == -1) goto fail_return;

			message_length = strlen(m_pMain->m_strSellMessage[message_index]);
			if (message_length > MAX_BBS_MESSAGE) {
				message_length = MAX_BBS_MESSAGE ;
			}

			SetShort(send_buff, message_length, send_index);
			SetString(send_buff, m_pMain->m_strSellMessage[message_index], message_length, send_index);
			break;
	}

	Send(send_buff, send_index);
	return;

fail_return:
	memset(send_buff, NULL, 256); send_index = 0;
	SetByte(send_buff, WIZ_MARKET_BBS, send_index);
	SetByte(send_buff, MARKET_BBS_MESSAGE, send_index);
	SetByte(send_buff, result, send_index);
	SetByte(send_buff, sub_result, send_index);
	Send(send_buff, send_index);
}

void CUser::MarketBBSBuyPostFilter()
{
	int empty_counter = 0;

	for (int i = 0 ; i < MAX_BBS_POST ; i++) {
		if (m_pMain->m_sBuyID[i] == -1) {	// BUY!!!
			empty_counter++;	
			continue;
		}

		if (empty_counter > 0) {
			if (m_pMain->m_sBuyID[i] != -1) {
				m_pMain->m_sBuyID[i - empty_counter] = m_pMain->m_sBuyID[i] ;
				strcpy(m_pMain->m_strBuyTitle[i- empty_counter], m_pMain->m_strBuyTitle[i]);
				strcpy(m_pMain->m_strBuyMessage[i- empty_counter], m_pMain->m_strBuyMessage[i]);
				m_pMain->m_iBuyPrice[i- empty_counter] = m_pMain->m_iBuyPrice[i];
				m_pMain->m_fBuyStartTime[i- empty_counter] = m_pMain->m_fBuyStartTime[i];	

				MarketBBSBuyDelete(i);
			}
		}
	}
}

void CUser::MarketBBSSellPostFilter()
{
	int empty_counter = 0;

	for (int i = 0 ; i < MAX_BBS_POST ; i++) {
		if (m_pMain->m_sSellID[i] == -1) {	// BUY!!!
			empty_counter++;	
			continue;
		}

		if (empty_counter > 0) {
			if (m_pMain->m_sSellID[i] != -1) {
				m_pMain->m_sSellID[i - empty_counter] = m_pMain->m_sSellID[i];
				strcpy( m_pMain->m_strSellTitle[i- empty_counter], m_pMain->m_strSellTitle[i]);
				strcpy( m_pMain->m_strSellMessage[i- empty_counter], m_pMain->m_strSellMessage[i]);
				m_pMain->m_iSellPrice[i- empty_counter] = m_pMain->m_iSellPrice[i];
				m_pMain->m_fSellStartTime[i- empty_counter] = m_pMain->m_fSellStartTime[i];	

				MarketBBSSellDelete(i);
			}
		}
	}
}