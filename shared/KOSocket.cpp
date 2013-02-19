#include "stdafx.h"
#include "KOSocket.h"
#include "packets.h"
#include "lzf.h"

KOSocket::KOSocket(uint16 socketID, SocketMgr * mgr, SOCKET fd) 
	: Socket(fd, 16384, 16384), 
	m_socketID(socketID), m_remaining(0), m_usingCrypto(false), 
	m_readTries(0), m_sequence(0), m_lastResponse(0) 
{
	SetSocketMgr(mgr);
}

KOSocket::KOSocket(uint16 socketID, SocketMgr * mgr, SOCKET fd, uint32 sendBufferSize, uint32 recvBufferSize) 
	: Socket(fd, sendBufferSize, recvBufferSize), 
	m_socketID(socketID), m_remaining(0),  m_usingCrypto(false), 
	m_readTries(0), m_sequence(0), m_lastResponse(0) 
{
	SetSocketMgr(mgr);
}

void KOSocket::OnConnect()
{
	TRACE("Connection received from %s:%d\n", GetRemoteIP().c_str(), GetRemotePort());
}

void KOSocket::OnRead() 
{
	Packet pkt;
	uint8 in_stream[MAX_PACKET_LEN];

	for (;;) 
	{
		if (m_remaining == 0) 
		{
			if (GetReadBuffer().GetSize() < 5)
				return; //check for opcode as well

			uint16 header = 0;
			GetReadBuffer().Read(&header, 2);
			if (header != 0x55aa) 
			{
				TRACE("%s: Got packet without header 0x55AA, got 0x%X\n", GetRemoteIP().c_str(), header);

				Disconnect();
				return;
			}

			GetReadBuffer().Read(&m_remaining, 2);
		}

		if (m_remaining > MAX_PACKET_LEN) 
		{
			TRACE("%s: Packet received which was %u bytes in size, maximum of %u.\n", GetRemoteIP().c_str(), m_remaining, MAX_PACKET_LEN);
			Disconnect();
			return;
		}

		if (m_remaining > GetReadBuffer().GetSize()) 
		{
			if (m_readTries > 4)
			{
				TRACE("%s: packet fragmentation count is over 4, disconnecting as they're probably up to something bad\n", GetRemoteIP().c_str());
				Disconnect();
				return;
			}
			m_readTries++;
			return;
		}

		m_readTries = 0;
		GetReadBuffer().Read(in_stream, m_remaining);

		uint16 footer = 0;
		GetReadBuffer().Read(&footer, 2);

		bool footerCheck = footer != 0xaa55;
		bool decryptCheck = !DecryptPacket(in_stream, pkt);
		bool handlerCheck = !HandlePacket(pkt);

		if (footerCheck
			|| decryptCheck
			|| handlerCheck)
		{

			TRACE("KOSocket: Footer invalid (%X), failed to decrypt or the handler for packet %X returned false\n", footer, pkt.GetOpcode());
			Disconnect();
			return;
		}

		m_remaining = 0;
	}
}

bool KOSocket::DecryptPacket(uint8 *in_stream, Packet & pkt)
{
	uint8 out_stream[MAX_PACKET_LEN];
	uint8* final_packet = NULL;

	if (isCryptoEnabled())
	{
		// decrypt to our out stream
		if (m_remaining < 4 // Invalid packet (all encrypted packets need a CRC32 checksum!)
			|| m_crypto.JvDecryptionWithCRC32(m_remaining, in_stream, out_stream) < 0 // Invalid checksum 
			|| ++m_sequence != *(uint32*)(out_stream)) // Invalid sequence ID
			return false;

		m_remaining -= 4; // remove the sequence ID
		final_packet = &out_stream[4];
	}
	else
	{
		final_packet = in_stream; // for simplicity :P
	}

	m_remaining--;
	pkt = Packet(final_packet[0], (size_t)m_remaining);
	if (m_remaining > 0) 
	{
		pkt.resize(m_remaining);
		memcpy((void*)pkt.contents(), &final_packet[1], m_remaining);
	}

	return true;
}

bool KOSocket::Send(char *buff, int len) 
{
	Packet result(*buff, (size_t)len - 1);
	if (len > 0)
		result.append(buff + 1, len - 1);
	return Send(&result);
}

bool KOSocket::SendCompressed(char *buff, int len) 
{
	Packet result(*buff, (size_t)len - 1);
	if (len > 0)
		result.append(buff + 1, len - 1);
	return SendCompressed(&result);
}

bool KOSocket::Send(Packet * pkt) 
{
	if (!IsConnected() || pkt->size() + 1 > MAX_SEND_LEN)
		return false;

	bool r;
	uint8 opcode = pkt->GetOpcode();
	uint8 * out_stream = NULL;
	uint16 len = (uint16)(pkt->size() + 1);

	if (isCryptoEnabled())
	{
		len += 5;

		uint32 seq = m_sequence; // this isn't actually incremented here
		uint8 *in_stream = new uint8[len];

		in_stream[0] = 0xfc;
		in_stream[1] = 0x1e;
		*(uint16*)&in_stream[2] = (uint16)seq;
		in_stream[4] = 0;
		in_stream[5] = pkt->GetOpcode();

		if (pkt->size() > 0)
			memcpy(&in_stream[6], pkt->contents(), pkt->size());

		out_stream = new uint8[len];
		m_crypto.JvEncryptionFast(len, in_stream, out_stream);
		delete [] in_stream;
	}
	else
	{
		out_stream = new uint8[len];
		out_stream[0] = pkt->GetOpcode();
		if (pkt->size() > 0)
			memcpy(&out_stream[1], pkt->contents(), pkt->size());
	}

	BurstBegin();

	if (GetWriteBuffer().GetSpace() < size_t(len + 6))
	{
		BurstEnd();
		Disconnect();
		return false;
	}

	r = BurstSend((const uint8*)"\xaa\x55", 2);
	if (r) r = BurstSend((const uint8*)&len, 2);
	if (r) r = BurstSend((const uint8*)out_stream, len);
	if (r) r = BurstSend((const uint8*)"\x55\xaa", 2);
	if (r) BurstPush();
	BurstEnd();

	delete [] out_stream;
	return r;
}

bool KOSocket::SendCompressed(Packet * pkt)
{
	uint32 inLength = pkt->size() + 1, outLength, crc;
	uint8 *buffer = new uint8[inLength];

	*buffer = pkt->GetOpcode();
	if (pkt->size() > 0)
		memcpy(buffer + 1, pkt->contents(), pkt->size());

	crc = (uint32)crc32(buffer, inLength);
	outLength = lzf_compress(buffer, inLength, buffer, inLength + LZF_MARGIN);

	pkt->Initialize(WIZ_COMPRESS_PACKET);

#if __VERSION >= 1900 // 32-bit
	*pkt << outLength << inLength;
#else // 16-bit
	*pkt << uint16(outLength) << uint16(inLength);
#endif
	*pkt << uint32(crc);
	pkt->append(buffer, outLength);

	return Send(pkt);
}

void KOSocket::OnDisconnect()
{
	TRACE("Connection closed from %s:%d\n", GetRemoteIP().c_str(), GetRemotePort());
}

void KOSocket::EnableCrypto(uint64 key)
{
	m_crypto.SetPublicKey(key);
	m_crypto.Init();
	m_usingCrypto = true;
}