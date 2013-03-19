#pragma once

#include "SocketMgr.h"
#include "Packet.h"
#include "JvCryption.h"
#include "lzf.h"

class KOSocket : public Socket
{
public:
	KOSocket(uint16 socketID, SocketMgr * mgr, SOCKET fd, uint32 sendBufferSize, uint32 recvBufferSize);

	__forceinline bool isCryptoEnabled() { return m_usingCrypto; };
	__forceinline uint16 GetSocketID() { return m_socketID; };

	virtual void OnConnect();
	virtual void OnRead();
	virtual bool DecryptPacket(uint8 *in_stream, Packet & pkt);
	virtual bool HandlePacket(Packet & pkt) = 0;

	bool Send(char *buff, int len); // DEPRECATED
	bool SendCompressed(char *buff, int len); // DEPRECATED
	virtual bool Send(Packet * pkt);
	virtual bool SendCompressed(Packet * pkt);

	virtual void OnDisconnect();

	void EnableCrypto();

protected:
	CJvCryption m_crypto;
	time_t m_lastResponse;
	uint32 m_sequence;
	uint16 m_remaining, m_socketID;
	uint8 m_readTries;
	bool m_usingCrypto;
};