// IOCPSocket2.h: interface for the CIOCPSocket2 class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IOCPSOCKET2_H__36499609_63DD_459C_B4D0_1686FEEC67C2__INCLUDED_)
#define AFX_IOCPSOCKET2_H__36499609_63DD_459C_B4D0_1686FEEC67C2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IOCPort.h"
#include "Define.h"
// Cryption
#include "JvCryption.h"
///~

#define receives				0
#define sends					1
#define both					2 

class CCircularBuffer;
class CIOCPSocket2  
{
public:
	void SendCompressingPacket(Packet *pkt);
	void InitSocket( CIOCPort* pIOCPort );
	void Close();
	BOOL AsyncSelect( long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE );
	BOOL ShutDown( int nHow = sends );
	void ReceivedData(int length);
	virtual void HandlePacket(char *pBuf, int len);
	int  Receive();
	int  Send(char *pBuf, long length);
	int  Send(Packet *result);
	BOOL Connect( CIOCPort* pIocp, LPCTSTR lpszHostAddress, UINT nHostPort );
	BOOL Create( UINT nSocketPort = 0,
				 int nSocketType = SOCK_STREAM, 
				 long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE,
				 LPCTSTR lpszSocketAddress = NULL );
	BOOL Accept( SOCKET listensocket, struct sockaddr* addr, int* len );

	__forceinline uint16 GetSocketID() { return m_Sid; };
	__forceinline void SetSocketID(uint16 sid) { m_Sid = sid; };
	__forceinline HANDLE GetSocketHandle() { return (HANDLE)m_Socket; };
	__forceinline BYTE GetState() { return m_State; };
	__forceinline BYTE GetSockType() { return m_Type; };
	__forceinline bool isCryptoEnabled() { return m_CryptionFlag; };

	virtual void CloseProcess();
	virtual void Parsing(Packet & pkt);
	virtual void Initialize();

	CIOCPSocket2();
	virtual ~CIOCPSocket2();

	short			m_nSocketErr;
	short			m_nPending;
	short			m_nWouldblock;

protected:
	CIOCPort* m_pIOCPort;
	CCircularBuffer*	m_pBuffer;
	uint16 m_remaining;
	uint8 m_retryAttempts;

	SOCKET				m_Socket;

	char				m_pRecvBuff[MAX_PACKET_SIZE];
	char				m_pSendBuff[MAX_PACKET_SIZE];

	HANDLE				m_hSockEvent;

	OVERLAPPED		m_RecvOverlapped;
	OVERLAPPED		m_SendOverlapped;

	BYTE			m_Type;
	BYTE			m_State;
	uint16			m_Sid;
	LPCTSTR		m_ConnectAddress;

	// Cryption
	CJvCryption			jct;
	bool				m_CryptionFlag;
	T_KEY				m_Public_key;
	DWORD				m_Sen_val;
	uint32				m_Rec_val;
	///~

	DWORD		m_wPacketSerial;
};

#endif // !defined(AFX_IOCPSOCKET2_H__36499609_63DD_459C_B4D0_1686FEEC67C2__INCLUDED_)
