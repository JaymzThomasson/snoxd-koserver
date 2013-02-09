#include "StdAfx.h" // oh god, this needs reworking, a LOT.
#include "EbenezerDlg.h"
#include "User.h"
#include "AIPacket.h"

// we'll throw the party BBS system in here too

void CUser::PartyProcess(Packet & pkt)
{
	// TO-DO: Clean this entire system up.
	std::string strUserID;
	CUser *pUser;
	uint8 opcode = pkt.read<uint8>();
	switch (opcode)
	{
	case PARTY_CREATE: // Attempt to create a party.
	case PARTY_INSERT: // Attempt to invite a user to an existing party.
		pkt >> strUserID;
		if (strUserID.empty() || strUserID.size() > MAX_ID_SIZE)
			return;

		pUser = m_pMain->GetUserPtr(strUserID.c_str(), TYPE_CHARACTER);
		if (pUser == NULL)
			return;

		PartyRequest(pUser->GetSocketID(), (opcode == PARTY_CREATE));
		break;

	// Did the user we invited accept our party invite?
	case PARTY_PERMIT:
		if (pkt.read<uint8>()) 
			PartyInsert();
		else							
			PartyCancel();
		break;

	// Remove a user from our party.
	case PARTY_REMOVE:
		PartyRemove(pkt.read<uint16>());
		break;

	// Disband our party.
	case PARTY_DELETE:
		PartyDelete();
		break;
	}
}

void CUser::PartyCancel()
{
	int leader_id = -1, count = 0;

	if (!isInParty())
		return;

	_PARTY_GROUP *pParty = m_pMain->m_PartyArray.GetData(m_sPartyIndex);
	m_sPartyIndex = -1;
	if (pParty == NULL)
		return;
	
	leader_id = pParty->uid[0];
	CUser *pUser = m_pMain->GetUserPtr(leader_id);
	if (pUser == NULL)
		return;

	for (int i = 0; i < 8; i++)
	{		
		if (pParty->uid[i] >= 0)
			count++;
	}

	if (count == 1)
		pUser->PartyDelete();			

	Packet result(WIZ_PARTY, uint8(PARTY_INSERT));
	result << int16(-1);
	pUser->Send(&result);
}

void CUser::PartyRequest(int memberid, BOOL bCreate)
{
	int index = 0, send_index = 0, result = -1, i=0;
	CUser* pUser = NULL;
	_PARTY_GROUP* pParty = NULL;
	char send_buff[256];

	pUser = m_pMain->GetUserPtr(memberid);
	if (pUser == NULL
		|| pUser->isInParty()) goto fail_return;

	if (getNation() != pUser->getNation())
	{
		result = -3;
		goto fail_return;
	}

	if( !(   ( pUser->m_pUserData->m_bLevel <= (int)(m_pUserData->m_bLevel * 1.5) && pUser->m_pUserData->m_bLevel >= (int)(m_pUserData->m_bLevel * 1.5)) 
		  || ( pUser->m_pUserData->m_bLevel <= (m_pUserData->m_bLevel+8) && pUser->m_pUserData->m_bLevel >= ((int)(m_pUserData->m_bLevel)-8) ) 
		 )
	  )  {
		result = -2;
		goto fail_return;
	}

	if( !bCreate ) {	// ????? ????? ?????? ???
		pParty = m_pMain->m_PartyArray.GetData(m_sPartyIndex);
		if( !pParty ) goto fail_return;
		for(i=0; i<8; i++) {
			if( pParty->uid[i] < 0 ) 
				break;
		}
		if( i==8 ) goto fail_return;	// ??? ??? Full
	}

	if( bCreate ) {
		if( isInParty() ) goto fail_return;	// can't create a party ifw e're already in one
		if (!m_pMain->CreateParty(this))
			goto fail_return;

		// AI Server
		send_index = 0;
		SetByte( send_buff, AG_USER_PARTY, send_index );
		SetByte( send_buff, PARTY_CREATE, send_index );
		SetShort( send_buff, pParty->wIndex, send_index );
		SetShort( send_buff, pParty->uid[0], send_index );
		//SetShort( send_buff, pParty->sHp[0], send_index );
		//SetByte( send_buff, pParty->bLevel[0], send_index );
		//SetShort( send_buff, pParty->sClass[0], send_index );
		m_pMain->Send_AIServer(send_buff, send_index);
	}

	pUser->m_sPartyIndex = m_sPartyIndex;

	send_index = 0;
	SetByte( send_buff, WIZ_PARTY, send_index );
	SetByte( send_buff, PARTY_PERMIT, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetKOString(send_buff, m_pUserData->m_id, send_index);
	pUser->Send( send_buff, send_index );
	return;

fail_return:
	SetByte( send_buff, WIZ_PARTY, send_index );
	SetByte( send_buff, PARTY_INSERT, send_index );
	SetShort( send_buff, result, send_index );
	Send( send_buff, send_index );
}

void CUser::PartyInsert()
{
	int send_index = 0;
	CUser* pUser = NULL;
	_PARTY_GROUP* pParty = NULL;
	BYTE byIndex = -1;
	char send_buff[256];
	if( !isInParty() ) return;

	pParty = m_pMain->m_PartyArray.GetData( m_sPartyIndex );
	if( !pParty ) {	
		m_sPartyIndex = -1;
		return;
	}
	
	// make sure user isn't already in the array...
	// kind of slow, but it works for the moment
	foreach_array (i, pParty->uid)
	{
		if (iValue == GetSocketID())
		{
			m_sPartyIndex = -1;
			return;
		}
	}

	for(int i=0; i<8; i++) {	// Send your info to the rest of the party members.
		if (pParty->uid[i] == GetSocketID())
			continue;

		pUser = m_pMain->GetUserPtr(pParty->uid[i]);
		if (pUser == NULL)
			continue;

		send_index = 0;
		SetByte( send_buff, WIZ_PARTY, send_index );
		SetByte( send_buff, PARTY_INSERT, send_index );
		SetShort( send_buff, pParty->uid[i], send_index );
		SetKOString(send_buff, pUser->m_pUserData->m_id, send_index);
		SetShort( send_buff, pParty->sMaxHp[i], send_index );
		SetShort( send_buff, pParty->sHp[i], send_index );
		SetByte( send_buff, pParty->bLevel[i], send_index );
		SetShort( send_buff, pParty->sClass[i], send_index );
		SetShort( send_buff, pUser->m_iMaxMp, send_index );
		SetShort( send_buff, pUser->m_pUserData->m_sMp, send_index );
		Send( send_buff, send_index );
	}

	for (int i = 0; i < 8; i++ ) {
		if( pParty->uid[i] == -1 ) {
			pParty->uid[i] = m_Sid;
			pParty->sMaxHp[i] = m_iMaxHp;
			pParty->sHp[i] = m_pUserData->m_sHp;
			pParty->bLevel[i] = m_pUserData->m_bLevel;
			pParty->sClass[i] = m_pUserData->m_sClass;
			byIndex = i;
			break;
		}
	}

	pUser = m_pMain->GetUserPtr(pParty->uid[0]);
	if (pUser == NULL)
		return;

	if (pUser->m_bNeedParty == 2 && pUser->isInParty())
		pUser->StateChangeServerDirect(2, 1);

	if (m_bNeedParty == 2 && isInParty()) 
		StateChangeServerDirect(2, 1);

	send_index = 0;
	SetByte( send_buff, WIZ_PARTY, send_index );
	SetByte( send_buff, PARTY_INSERT, send_index );
	SetShort( send_buff, m_Sid, send_index );
	SetKOString( send_buff, m_pUserData->m_id, send_index );
	SetShort( send_buff, m_iMaxHp, send_index );		
	SetShort( send_buff, m_pUserData->m_sHp, send_index );
	SetByte( send_buff, m_pUserData->m_bLevel, send_index );
	SetShort( send_buff, m_pUserData->m_sClass, send_index );
	SetShort( send_buff, m_iMaxMp, send_index );
	SetShort( send_buff, m_pUserData->m_sMp, send_index );
	m_pMain->Send_PartyMember( m_sPartyIndex, send_buff, send_index );

	// AI Server
	send_index = 0;
	SetByte( send_buff, AG_USER_PARTY, send_index );
	SetByte( send_buff, PARTY_INSERT, send_index );
	SetShort( send_buff, pParty->wIndex, send_index );
	SetByte( send_buff, byIndex, send_index );
	SetShort( send_buff, pParty->uid[byIndex], send_index );
	//SetShort( send_buff, pParty->sHp[i], send_index );
	//SetByte( send_buff, pParty->bLevel[i], send_index );
	//SetShort( send_buff, pParty->sClass[i], send_index );
	m_pMain->Send_AIServer(send_buff, send_index);
}

void CUser::PartyRemove(int memberid)
{
	int index = 0, send_index = 0, count = 0, i = 0;
	CUser* pUser = NULL;
	_PARTY_GROUP* pParty = NULL;

	if (!isInParty()) 
		return;

	pUser = m_pMain->GetUserPtr(memberid);
	if (pUser == NULL)
		return;

	pParty = m_pMain->m_PartyArray.GetData(m_sPartyIndex);
	if (!pParty) 
	{
		pUser->m_sPartyIndex = -1;
		m_sPartyIndex = -1;
		return;
	}

	if (memberid != GetSocketID())
	{		
		if (pParty->uid[0] != GetSocketID()) 
			return;
	}
	else 
	{
		if (pParty->uid[0] == memberid)
		{
			PartyDelete();
			return;
		}
	}

	for( i=0; i<8; i++ ) {
		if( pParty->uid[i] != -1 ) {
			if( pParty->uid[i] == memberid ) {
				count--;
			}
			count++;
		}
	}
	if( count == 1 ) {
		PartyDelete();
		return;
	}

	char send_buff[256]; 
	SetByte( send_buff, WIZ_PARTY, send_index );
	SetByte( send_buff, PARTY_REMOVE, send_index );
	SetShort( send_buff, memberid, send_index );
	m_pMain->Send_PartyMember( m_sPartyIndex, send_buff, send_index );

	for( i=0; i<8; i++ ) {
		if( pParty->uid[i] != -1 ) {
			if( pParty->uid[i] == memberid ) {
				pParty->uid[i] = -1;
				pParty->sHp[i] = 0;
				pParty->bLevel[i] = 0;
				pParty->sClass[i] = 0;
				pUser->m_sPartyIndex = -1;
			}
		}
	}
	// AI Server
	send_index = 0;
	SetByte( send_buff, AG_USER_PARTY, send_index );
	SetByte( send_buff, PARTY_REMOVE, send_index );
	SetShort( send_buff, pParty->wIndex, send_index );
	SetShort( send_buff, memberid, send_index );
	m_pMain->Send_AIServer(send_buff, send_index);
}

void CUser::PartyDelete()
{
	int send_index = 0;
	CUser* pUser = NULL;
	_PARTY_GROUP* pParty = NULL;
	if( !isInParty() ) return;

	pParty = m_pMain->m_PartyArray.GetData( m_sPartyIndex );
	if( !pParty ) {
		m_sPartyIndex = -1;
		return;
	}
	for( int i=0; i<8; i++ ) {
		pUser = m_pMain->GetUserPtr(pParty->uid[i]);
		if (pUser == NULL) 
			continue;
		pUser->m_sPartyIndex = -1;
	}

	char send_buff[256];
	SetByte( send_buff, WIZ_PARTY, send_index );
	SetByte( send_buff, PARTY_DELETE, send_index );
	m_pMain->Send_PartyMember( pParty->wIndex, send_buff, send_index );	// ????? ???? ??e??????..

	// AI Server
	send_index = 0;
	SetByte( send_buff, AG_USER_PARTY, send_index );
	SetByte( send_buff, PARTY_DELETE, send_index );
	SetShort( send_buff, pParty->wIndex, send_index );
	m_pMain->Send_AIServer(send_buff, send_index);

	m_pMain->DeleteParty(pParty->wIndex);
}

// Seeking party system
void CUser::PartyBBS(Packet & pkt)
{
	uint8 opcode = pkt.read<uint8>();
	switch (opcode)
	{
		case PARTY_BBS_REGISTER:
			PartyBBSRegister(pkt);
			break;
		case PARTY_BBS_DELETE:
			PartyBBSDelete(pkt);
			break;
		case PARTY_BBS_NEEDED:
			PartyBBSNeeded(pkt, PARTY_BBS_NEEDED);
			break;
	}
}

void CUser::PartyBBSRegister(Packet & pkt)
{
#if 0 // temporarily disabled
	CUser* pUser = NULL;
	int index = 0, send_index = 0;	// Basic Initializations. 			
	BYTE result = 0; short bbs_len = 0;
	char send_buff[256]; 
	int i = 0, counter = 0;

	if (isInParty()) goto fail_return;	// You are already in a party!
	if (m_bNeedParty == 2) goto fail_return;	// You are already on the BBS!

	result = 1;
	StateChangeServerDirect(2, 2); // seeking a party

	send_index = 0; // Now, let's find out which page the user is on.
	for (i = 0 ; i < MAX_USER ; i++) {
		pUser = m_pMain->GetUnsafeUserPtr(i);
		if (pUser == NULL
			|| pUser->getNation() != getNation()
			|| pUser->m_bNeedParty == 1) 
			continue;

		if( !(   ( pUser->m_pUserData->m_bLevel <= (int)(m_pUserData->m_bLevel * 1.5) && pUser->m_pUserData->m_bLevel >= (int)(m_pUserData->m_bLevel * 1.5)) 
			  || ( pUser->m_pUserData->m_bLevel <= (m_pUserData->m_bLevel+8) && pUser->m_pUserData->m_bLevel >= ((int)(m_pUserData->m_bLevel)-8) ) 
		) ) continue;

		if (pUser->GetSocketID() == GetSocketID()) break;
		counter++;		
	}

	SetShort(send_buff, counter / MAX_BBS_PAGE, send_index);
	PartyBBSNeeded(send_buff, PARTY_BBS_REGISTER);
	return;

fail_return:
	SetByte(send_buff, WIZ_PARTY_BBS, send_index);
	SetByte(send_buff, PARTY_BBS_REGISTER, send_index);
	SetByte(send_buff, result, send_index);
	Send(send_buff, send_index);
	return;
#endif
}

void CUser::PartyBBSDelete(Packet & pkt)
{
#if 0 // temporarily disabled
	int send_index = 0;	// Basic Initializations. 			
	BYTE result = 0; 
	char send_buff[256]; 

	if (m_bNeedParty == 1) goto fail_return;	// You don't need anymore 

	result = 1;

	StateChangeServerDirect(2, 1); // not looking for a party

	send_index = 0; // Now, let's find out which page the user is on.
	SetShort(send_buff, 0, send_index);
	PartyBBSNeeded(send_buff, PARTY_BBS_DELETE);
	return;

fail_return:
	SetByte(send_buff, WIZ_PARTY_BBS, send_index);
	SetByte(send_buff, PARTY_BBS_DELETE, send_index);
	SetByte(send_buff, result, send_index);
	Send(send_buff, send_index);
	return;	
#endif
}

void CUser::PartyBBSNeeded(Packet & pkt, BYTE type)
{
#if 0 // temporarily disabled
	CUser* pUser = NULL;	// Basic Initializations. 	
	int index = 0, send_index = 0;				
	BYTE result = 0; short bbs_len = 0;
	char send_buff[256];
	short page_index = 0; short start_counter = 0; BYTE valid_counter = 0 ;
	int  i = 0, j = 0; short BBS_Counter = 0;
	
	page_index = GetShort(pBuf, index);
	start_counter = page_index * MAX_BBS_PAGE;

	if ( start_counter < 0 ) goto fail_return;
	if ( start_counter > MAX_USER ) goto fail_return;

	result = 1;

	SetByte(send_buff, WIZ_PARTY_BBS, send_index);
	SetByte(send_buff, type, send_index);
	SetByte(send_buff, result, send_index);

	for (i = 0 ; i < MAX_USER ; i++) {
		pUser = m_pMain->GetUnsafeUserPtr(i);
		if (pUser == NULL
			|| pUser->getNation() != getNation()
			|| pUser->m_bNeedParty == 1) 
			continue;

		if( !(   ( pUser->m_pUserData->m_bLevel <= (int)(m_pUserData->m_bLevel * 1.5) && pUser->m_pUserData->m_bLevel >= (int)(m_pUserData->m_bLevel * 1.5)) 
			  || ( pUser->m_pUserData->m_bLevel <= (m_pUserData->m_bLevel+8) && pUser->m_pUserData->m_bLevel >= ((int)(m_pUserData->m_bLevel)-8) ) 
		) ) continue;

		BBS_Counter++;

		if (i < start_counter) continue;	// Range check codes.
		if (valid_counter >= MAX_BBS_PAGE) continue;

		SetKOString(send_buff, pUser->m_pUserData->m_id, send_index);
		SetByte(send_buff, pUser->m_pUserData->m_bLevel, send_index);
		SetShort(send_buff, pUser->m_pUserData->m_sClass, send_index);

		valid_counter++;		// Increment counters.
//		BBS_Counter++;		
	}

	if ( valid_counter < MAX_BBS_PAGE ) {	// You still need to fill up ten slots.
		for (j = valid_counter ; j < MAX_BBS_PAGE ; j++) {
			SetShort(send_buff, 0, send_index);
			SetString(send_buff, NULL, 0, send_index);
			SetByte(send_buff, 0, send_index);
			SetShort(send_buff, 0, send_index);
		}
	}

	SetShort(send_buff, page_index, send_index);
	SetShort(send_buff, BBS_Counter, send_index);
	Send(send_buff, send_index);
	return;

fail_return:
	SetByte(send_buff, WIZ_PARTY_BBS, send_index);
	SetByte(send_buff, PARTY_BBS_NEEDED, send_index);
	SetByte(send_buff, result, send_index);
	Send(send_buff, send_index);
	return;		
#endif
}