// IOCPSocket2.cpp: implementation of the CIOCPSocket2 class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IOCPSocket2.h"
#include "../shared/CircularBuffer.h"
#include "define.h"
#include "../shared/Compress.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

void bb() {};		// nop function


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIOCPSocket2::CIOCPSocket2()
{
	m_pBuffer = new CCircularBuffer(SOCKET_BUFF_SIZE);
	m_pRegionBuffer = new _REGION_BUFFER;
	m_Socket = INVALID_SOCKET;

	m_pIOCPort = NULL;
	m_Type = TYPE_ACCEPT;

	// Cryption
	m_CryptionFlag = 0;
	m_Sen_val = 0;
}

CIOCPSocket2::~CIOCPSocket2()
{
	delete m_pBuffer;
	delete m_pRegionBuffer;
}


BOOL CIOCPSocket2::Create( UINT nSocketPort, int nSocketType, long lEvent, LPCTSTR lpszSocketAddress)
{
	int ret;
	struct linger lingerOpt;

	m_Socket = socket( AF_INET, nSocketType/*SOCK_STREAM*/, 0 );
	if( m_Socket == INVALID_SOCKET ) {
		ret = WSAGetLastError();
		TRACE("Socket Create Fail! - %d\n", ret);
		return FALSE;
	}

	m_hSockEvent = WSACreateEvent();
	if( m_hSockEvent == WSA_INVALID_EVENT ) {
		ret = WSAGetLastError();
		TRACE("Event Create Fail! - %d\n", ret);
		return FALSE;
	}

	// Linger off -> close socket immediately regardless of existance of data 
	//
	lingerOpt.l_onoff = 0;
	lingerOpt.l_linger = 0;

	setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (char *)&lingerOpt, sizeof(lingerOpt));

	int socklen;

	socklen = SOCKET_BUFF_SIZE*4;
	setsockopt( m_Socket, SOL_SOCKET, SO_RCVBUF, (char*)&socklen, sizeof(socklen));
	
	socklen = SOCKET_BUFF_SIZE*4;
	setsockopt( m_Socket, SOL_SOCKET, SO_SNDBUF, (char*)&socklen, sizeof(socklen));
	
	return TRUE;
}

BOOL CIOCPSocket2::Connect( CIOCPort* pIocp, LPCTSTR lpszHostAddress, UINT nHostPort )
{
	struct sockaddr_in addr;

	memset((void *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(lpszHostAddress);
	addr.sin_port = htons(nHostPort);

	int result = connect( m_Socket,(struct sockaddr *)&addr,sizeof(addr) );
	if ( result == SOCKET_ERROR )
	{
		int err = WSAGetLastError();
		TRACE("CONNECT FAIL : %d\n", err);
		closesocket( m_Socket );
		return FALSE;
	}

	ASSERT( pIocp );

	InitSocket( pIocp );

	m_Sid = m_pIOCPort->GetClientSid();
	if( m_Sid < 0 )
		return FALSE;

	m_ConnectAddress = lpszHostAddress;
	m_State = STATE_CONNECTED;
	m_Type = TYPE_CONNECT;

	m_pIOCPort->m_ClientSockArray[m_Sid] = this;
	
	if ( !m_pIOCPort->Associate(this, m_pIOCPort->m_hClientIOCPort) )
	{
		TRACE("Socket Connecting Fail - Associate\n");
		m_pIOCPort->RidIOCPSocket(m_Sid, this);
		return FALSE;
	}

	Receive();

	return TRUE;
}

int CIOCPSocket2::Send(char *pBuf, long length)
{
	int ret_value = 0;
	WSABUF out;
	DWORD sent = 0;
	OVERLAPPED *pOvl;
	HANDLE	hComport = NULL;

	if (length + 5 /* crypto */ >= MAX_SEND_SIZE)
		return 0;

	BYTE pTIBuf[MAX_SEND_SIZE], pTOutBuf[MAX_SEND_SIZE];
	int index = 0;

	if( m_CryptionFlag )
	{
		unsigned short len = (unsigned short)(length + sizeof(WORD)+2+1);

		m_Sen_val++;
		m_Sen_val &= 0x00ffffff;

		pTIBuf[0] = 0xfc;
		pTIBuf[1] = 0x1e;
		memcpy( &pTIBuf[2], &m_Sen_val, sizeof(WORD) );
		memcpy( &pTIBuf[5], pBuf, length );
		jct.JvEncryptionFast( len, pTIBuf, pTOutBuf );
		
		pTIBuf[index++] = (BYTE)PACKET_START1;
		pTIBuf[index++] = (BYTE)PACKET_START2;
		memcpy( pTIBuf+index, &len, 2 );
		index += 2;
		memcpy( pTIBuf+index, pTOutBuf, len );
		index += len;
		pTIBuf[index++] = (BYTE)PACKET_END1;
		pTIBuf[index++] = (BYTE)PACKET_END2;
	}
	else
	{
		pTIBuf[index++] = (BYTE)PACKET_START1;
		pTIBuf[index++] = (BYTE)PACKET_START2;
		memcpy( pTIBuf+index, &length, 2 );
		index += 2;
		memcpy( pTIBuf+index, pBuf, length );
		index += length;
		pTIBuf[index++] = (BYTE)PACKET_END1;
		pTIBuf[index++] = (BYTE)PACKET_END2;
	}

	out.buf = (char*)pTIBuf;
	out.len = index;
	
	pOvl = &m_SendOverlapped;
	pOvl->Offset = OVL_SEND;
	pOvl->OffsetHigh = out.len;

	ret_value = WSASend( m_Socket, &out, 1, &sent, 0, pOvl, NULL);
	
	if ( ret_value == SOCKET_ERROR )
	{
		int last_err;
		last_err = WSAGetLastError();

		if ( last_err == WSA_IO_PENDING ) {
			TRACE("SEND : IO_PENDING[SID=%d]\n", m_Sid);
			m_nPending++;
			if( m_nPending > 3 )
				goto close_routine;
			sent = length; 
		}
		else if ( last_err == WSAEWOULDBLOCK )
		{
			TRACE("SEND : WOULDBLOCK[SID=%d]\n", m_Sid);

			m_nWouldblock++;
			if( m_nWouldblock > 3 )
				goto close_routine;
			return 0;
		}
		else
		{
			TRACE("SEND : ERROR [SID=%d] - %d\n", m_Sid, last_err);
			m_nSocketErr++;
			goto close_routine;
		}
	}
	else if ( !ret_value )
	{
		m_nPending = 0;
		m_nWouldblock = 0;
		m_nSocketErr = 0;
	}

	return sent;

close_routine:
	pOvl = &m_RecvOverlapped;
	pOvl->Offset = OVL_CLOSE;
	
	if( m_Type == TYPE_ACCEPT )
		hComport = m_pIOCPort->m_hServerIOCPort;
	else
		hComport = m_pIOCPort->m_hClientIOCPort;
	
	PostQueuedCompletionStatus( hComport, (DWORD)0, (DWORD)m_Sid, pOvl );
	
	return -1;
}

int CIOCPSocket2::Send(Packet *result)
{
	int ret_value = 0;
	WSABUF out;
	DWORD sent = 0;
	OVERLAPPED *pOvl;
	HANDLE	hComport = NULL;

	BYTE pTIBuf[MAX_SEND_SIZE], pTOutBuf[MAX_SEND_SIZE];
	int index = 0;
	uint16 length = (uint16)result->size() + 1;

	if( m_CryptionFlag )
	{
		if (length + 5 >= MAX_SEND_SIZE) // crypto
			return 0;

		uint16 totalLen = length + 5;

		m_Sen_val++;
		m_Sen_val &= 0x00ffffff;

		pTIBuf[0] = 0xfc;
		pTIBuf[1] = 0x1e;
		memcpy(&pTIBuf[2], &m_Sen_val, 2);
		pTIBuf[5] = result->GetOpcode();
		memcpy(&pTIBuf[6], result->contents(), length - 1);

		// encrypt to output buffer
		jct.JvEncryptionFast( totalLen, pTIBuf, pTOutBuf );
		
		// replace input buffer with full, encrypted packet.
		pTIBuf[index++] = (BYTE)PACKET_START1; // packet header
		pTIBuf[index++] = (BYTE)PACKET_START2;
		memcpy(pTIBuf+index, &totalLen, 2);
		index += 2;
		memcpy(pTIBuf+index, pTOutBuf, totalLen); // throw encrypted packet data into packet body
		index += totalLen;
		pTIBuf[index++] = (BYTE)PACKET_END1; // packet tail
		pTIBuf[index++] = (BYTE)PACKET_END2;
	}
	else
	{
		if (length >= MAX_SEND_SIZE) // no crypto
			return 0;

		pTIBuf[index++] = (BYTE)PACKET_START1;
		pTIBuf[index++] = (BYTE)PACKET_START2;
		memcpy(pTIBuf+index, &length, 2);
		index += 2;
		pTIBuf[index++] = result->GetOpcode();
		memcpy(pTIBuf+index, result->contents(), length - 1);
		index += length - 1;
		pTIBuf[index++] = (BYTE)PACKET_END1;
		pTIBuf[index++] = (BYTE)PACKET_END2;
	}

	out.buf = (char*)pTIBuf;
	out.len = index;
	
	pOvl = &m_SendOverlapped;
	pOvl->Offset = OVL_SEND;
	pOvl->OffsetHigh = out.len;

	ret_value = WSASend( m_Socket, &out, 1, &sent, 0, pOvl, NULL);
	
	if ( ret_value == SOCKET_ERROR )
	{
		int last_err;
		last_err = WSAGetLastError();

		if ( last_err == WSA_IO_PENDING ) {
			TRACE("SEND : IO_PENDING[SID=%d]\n", m_Sid);
			m_nPending++;
			if( m_nPending > 3 )
				goto close_routine;
			sent = length; 
		}
		else if ( last_err == WSAEWOULDBLOCK )
		{
			TRACE("SEND : WOULDBLOCK[SID=%d]\n", m_Sid);

			m_nWouldblock++;
			if( m_nWouldblock > 3 )
				goto close_routine;
			return 0;
		}
		else
		{
			TRACE("SEND : ERROR [SID=%d] - %d\n", m_Sid, last_err);
			m_nSocketErr++;
			goto close_routine;
		}
	}
	else if ( !ret_value )
	{
		m_nPending = 0;
		m_nWouldblock = 0;
		m_nSocketErr = 0;
	}

	return sent;

close_routine:
	pOvl = &m_RecvOverlapped;
	pOvl->Offset = OVL_CLOSE;
	
	if( m_Type == TYPE_ACCEPT )
		hComport = m_pIOCPort->m_hServerIOCPort;
	else
		hComport = m_pIOCPort->m_hClientIOCPort;
	
	PostQueuedCompletionStatus( hComport, (DWORD)0, (DWORD)m_Sid, pOvl );
	
	return -1;
}


int CIOCPSocket2::Receive()
{
	int RetValue;
	WSABUF in;
	DWORD insize, dwFlag=0;
	OVERLAPPED *pOvl;
	HANDLE	hComport = NULL;

	memset(m_pRecvBuff, NULL, MAX_PACKET_SIZE );
	in.len = MAX_PACKET_SIZE;
	in.buf = m_pRecvBuff;

	pOvl = &m_RecvOverlapped;
	pOvl->Offset = OVL_RECEIVE;

	RetValue = WSARecv( m_Socket, &in, 1, &insize, &dwFlag, pOvl, NULL );

 	if ( RetValue == SOCKET_ERROR )
	{
		int last_err;
		last_err = WSAGetLastError();

		if ( last_err == WSA_IO_PENDING ) {
//			TRACE("RECV : IO_PENDING[SID=%d]\n", m_Sid);
//			m_nPending++;
//			if( m_nPending > 3 )
//				goto close_routine;
			return 0;
		}
		else if ( last_err == WSAEWOULDBLOCK )
		{
			TRACE("RECV : WOULDBLOCK[SID=%d]\n", m_Sid);

			m_nWouldblock++;
			if( m_nWouldblock > 3 )
				goto close_routine;
			return 0;
		}
		else
		{
			TRACE("RECV : ERROR [SID=%d] - %d\n", m_Sid, last_err);

			m_nSocketErr++;
			if( m_nSocketErr == 2 )
				goto close_routine;
			return -1;
		}
	}

	return (int)insize;

close_routine:
	pOvl = &m_RecvOverlapped;
	pOvl->Offset = OVL_CLOSE;
	
	if( m_Type == TYPE_ACCEPT )
		hComport = m_pIOCPort->m_hServerIOCPort;
	else
		hComport = m_pIOCPort->m_hClientIOCPort;
	
	PostQueuedCompletionStatus( hComport, (DWORD)0, (DWORD)m_Sid, pOvl );
	
	return -1;
}

void CIOCPSocket2::ReceivedData(int length)
{
	if (length <= 0 || length >= MAX_PACKET_SIZE) return;

	int len = 0;
	char *pData;

	// read received bytes into our circular buffer
	m_pBuffer->PutData(m_pRecvBuff, length); 

	// go over our circular buffer to try and find any KO packets, so we can parse them
	while (PullOutCore(pData, len))
	{
		if (pData)
		{
			// found a packet - it's parse time!
			Parsing(len, pData);

			delete [] pData;
			pData = NULL;
		}
	}
}

BOOL CIOCPSocket2::PullOutCore(char *&data, int &length)
{
	BYTE		pTmp[MAX_PACKET_SIZE], *pBuff;
	int			len;
	BOOL		foundCore;
	MYSHORT		slen;
	DWORD		wSerial = 0, recv_packet = 0;
	int index = 1;

	len = m_pBuffer->GetValidCount();

	if (len <= 0 || len >= MAX_PACKET_SIZE) 
		return FALSE;

	m_pBuffer->GetData((char*)pTmp, len);
	foundCore = FALSE;

	int	sPos = 0, ePos = 0;

	for (int i = 0; i < len && !foundCore; i++)
	{
		if (i+2 >= len) 
			break;

		if (pTmp[i] == PACKET_START1 && pTmp[i+1] == PACKET_START2)
		{
			sPos = i+2;

			slen.b[0] = pTmp[sPos];
			slen.b[1] = pTmp[sPos + 1];

			length = slen.w;

			if (length < 0 || length > len || length >= MAX_PACKET_SIZE)
				goto cancelRoutine;

			ePos = sPos+length + 2;

			if ((ePos + 2) > len)
				goto cancelRoutine;

			if (pTmp[ePos] == PACKET_END1 && pTmp[ePos+1] == PACKET_END2)
			{
				if (m_CryptionFlag)
				{
					pBuff = new BYTE[length+1]; 
					if (jct.JvDecryptionWithCRC32(length, (unsigned char*)pTmp+sPos+2, pBuff) < 0)
					{
						TRACE("CIOCPSocket2::PutOutCore - Decryption Error... sockid(%d)\n", m_Socket);
						delete[] pBuff;
						Close();
						goto cancelRoutine;
					}

					recv_packet = GetDWORD((char*)pBuff, index);
					//TRACE("^^^ IOCPSocket2,, PullOutCore ,,, recv_val = %d ^^^\n", recv_packet);

					m_Rec_val = recv_packet;
					length = length-8;
					if( length <= 0 )	{
						TRACE("CIOCPSocket2::PutOutCore - length Error... sockid(%d), len=%d\n", m_Socket, length);
						delete[] pBuff;
						Close();
						goto cancelRoutine;
					}
					data = new char[length+1];
					CopyMemory((void *)data, (const void *)(pBuff+4), length);
					data[length] = 0;
					foundCore = TRUE;
					int head = m_pBuffer->GetHeadPos(), tail = m_pBuffer->GetTailPos();
					delete[] pBuff;
				}
				else	
				{
					data = new char[length+1];
					CopyMemory((void *)data, (const void *)(pTmp+sPos+2), length);
					data[length] = 0;
					foundCore = TRUE;
					int head = m_pBuffer->GetHeadPos(), tail = m_pBuffer->GetTailPos();
				}
				break;
			}
			else 
			{
				m_pBuffer->HeadIncrease(3);
				break;
			}
		}
	}
	if (foundCore)
		m_pBuffer->HeadIncrease( (m_CryptionFlag ? 10 : 6) + length); // 6: header 2+ end 2+ length 2 + cryption 4

cancelRoutine:
	return foundCore;
}

BOOL CIOCPSocket2::AsyncSelect( long lEvent )
{
	int retEventResult, err;

	retEventResult = WSAEventSelect( m_Socket, m_hSockEvent, lEvent );
	err = WSAGetLastError();

	return ( !retEventResult );
}

BOOL CIOCPSocket2::ShutDown( int nHow )
{
	int retValue;
	retValue = shutdown( m_Socket, nHow );

	return ( !retValue );
}

void CIOCPSocket2::Close()
{
	if ( m_pIOCPort == NULL ) return;

	HANDLE	hComport = NULL;
	OVERLAPPED		*pOvl;
	pOvl = &m_RecvOverlapped;
	pOvl->Offset = OVL_CLOSE;

	if( m_Type == TYPE_ACCEPT )
		hComport = m_pIOCPort->m_hServerIOCPort;
	else
		hComport = m_pIOCPort->m_hClientIOCPort;

	int retValue = PostQueuedCompletionStatus( hComport, (DWORD)0, (DWORD)m_Sid, pOvl );

	if ( !retValue ) {
		int errValue;
		errValue = GetLastError();
		TRACE("PostQueuedCompletionStatus Error : %d\n", errValue);
	}
}


void CIOCPSocket2::CloseProcess()
{
	m_State = STATE_DISCONNECTED;

	if( m_Socket != INVALID_SOCKET )
		closesocket( m_Socket );
}

void CIOCPSocket2::InitSocket( CIOCPort* pIOCPort )
{
	m_pIOCPort = pIOCPort;
	m_RecvOverlapped.hEvent = NULL;
	m_SendOverlapped.hEvent = NULL;
	m_pBuffer->SetEmpty();
	m_nSocketErr = 0;
	m_nPending = 0;
	m_nWouldblock = 0;

	Initialize();
}

BOOL CIOCPSocket2::Accept( SOCKET listensocket, struct sockaddr* addr, int* len )
{
	m_Socket = accept( listensocket, addr, len);
	if( m_Socket == INVALID_SOCKET) {
		int err = WSAGetLastError();
		DEBUG_LOG_FILE("Socket Accepting Fail - %d\n", err);
		return FALSE;
	}


	struct linger lingerOpt;

	lingerOpt.l_onoff = 1;
	lingerOpt.l_linger = 0;

	setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (const char *)&lingerOpt, sizeof(lingerOpt));

	return TRUE;
}

void CIOCPSocket2::Parsing(int length, char *pData)
{
}

void CIOCPSocket2::Parsing(Packet & pkt)
{
}

void CIOCPSocket2::Initialize()
{
	m_wPacketSerial = 0;
	m_CryptionFlag = 0;
}

void CIOCPSocket2::SendCompressingPacket(const char *pData, int len)
{
	// Data's too short to bother with compression...
	if (len < 500)
	{
		Send((char *)pData, len);
		return;
	}
	
	CCompressMng comp;
	comp.PreCompressWork(pData, len);
	comp.Compress();

	Packet result(WIZ_COMPRESS_PACKET);
	result << comp.m_nOutputBufferCurPos << comp.m_nOrgDataLength << uint32(comp.m_dwCrc);
	result.append(comp.m_pOutputBuffer, comp.m_nOutputBufferCurPos);
	Send(&result);
}

void CIOCPSocket2::SendCompressingPacket(Packet *pkt)
{
	// Data's too short to bother with compression...
	if (pkt->size() < 500)
	{
		Send(pkt);
		return;
	}

	// TO-DO: Replace this with LZF (again), so much simpler.
	CCompressMng comp;
	ByteBuffer buff(pkt->size() + 1);
	buff << pkt->GetOpcode() << *pkt;
	comp.PreCompressWork((const char *)buff.contents(), buff.size());
	comp.Compress();

	Packet result(WIZ_COMPRESS_PACKET);
	result << comp.m_nOutputBufferCurPos << comp.m_nOrgDataLength << uint32(comp.m_dwCrc);
	result.append(comp.m_pOutputBuffer, comp.m_nOutputBufferCurPos);
	Send(&result);
}

void CIOCPSocket2::RegionPacketAdd(char *pBuf, int len)
{
	m_pRegionBuffer->Lock.AcquireWriteLock();
	m_pRegionBuffer->Buffer << uint16(len);
	m_pRegionBuffer->Buffer.append(pBuf, len);
	m_pRegionBuffer->Lock.ReleaseWriteLock();
}

void CIOCPSocket2::RegionPacketAdd(Packet *pkt)
{
	m_pRegionBuffer->Lock.AcquireWriteLock();
	m_pRegionBuffer->Buffer << pkt->size() << *pkt;
	m_pRegionBuffer->Lock.ReleaseWriteLock();
}

void CIOCPSocket2::SendRegionPackets()
{
	m_pRegionBuffer->Lock.AcquireReadLock();
	if (m_pRegionBuffer->Buffer.size())
	{
		Packet result(WIZ_CONTINOUS_PACKET);
		result << uint16(m_pRegionBuffer->Buffer.size()) << m_pRegionBuffer->Buffer;
		m_pRegionBuffer->Buffer.clear();
		SendCompressingPacket(&result);
	}
	m_pRegionBuffer->Lock.ReleaseReadLock();
}