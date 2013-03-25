#include "stdafx.h"
#include "SocketMgr.h"

Socket::Socket(SOCKET fd, uint32 sendbuffersize, uint32 recvbuffersize) 
	: m_fd(fd), m_connected(false),	m_deleted(false), m_socketMgr(NULL)
{
	// Allocate buffers
	readBuffer.Allocate(recvbuffersize);
	writeBuffer.Allocate(sendbuffersize);

	// IOCP member variables
	m_writeLock = 0;
	m_completionPort = 0;

	// Check for needed fd allocation.
	if (m_fd == 0)
		m_fd = WSASocket(AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED);
}

bool Socket::Connect(const char * Address, uint32 Port)
{
	struct hostent * ci = gethostbyname(Address);
	if (ci == 0)
		return false;

	m_client.sin_family = ci->h_addrtype;
	m_client.sin_port = ntohs((u_short)Port);
	memcpy(&m_client.sin_addr.s_addr, ci->h_addr_list[0], ci->h_length);

	SetBlocking(true);

	if (m_fd == 0)
		m_fd = WSASocket(AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED);

	if (connect(m_fd, (const sockaddr*)&m_client, sizeof(m_client)) == -1)
		return false;

	// at this point the connection was established
	m_completionPort = m_socketMgr->GetCompletionPort();
	m_socketMgr->OnConnect(this);

	_OnConnect();
	return true;
}

void Socket::Accept(sockaddr_in * address)
{
	memcpy(&m_client, address, sizeof(*address));
	_OnConnect();
}

void Socket::SetBlocking(bool block /*= false*/)
{
	u_long arg = (block ? 0 : 1);
	ioctlsocket(m_fd, FIONBIO, &arg);
}

void Socket::SetBuffering(bool enable /*= true*/)
{
	uint32 arg = (enable ? 0 : 1);
	setsockopt(m_fd, 0x6, TCP_NODELAY, (const char*)&arg, sizeof(arg));
}

void Socket::_OnConnect()
{
	// set common parameters on the file descriptor
	// SetBlocking(false);
	SetBlocking(false);
	SetBuffering(true);
	m_connected = true;

	// IOCP stuff
	AssignToCompletionPort();

	// Call virtual onconnect
	OnConnect();

	// Setting the read event up after calling OnConnect() ensures OnConnect() & subsequent connection setup code is run first (which is NOT GUARANTEED otherwise)
	SetupReadEvent();
}

bool Socket::Send(const uint8 * Bytes, uint32 Size)
{
	bool rv;

	// This is really just a wrapper for all the burst stuff.
	BurstBegin();
	rv = BurstSend(Bytes, Size);
	if (rv)
		BurstPush();
	BurstEnd();

	return rv;
}

bool Socket::BurstSend(const uint8 * Bytes, uint32 Size)
{
	return writeBuffer.Write(Bytes, Size);
}

std::string Socket::GetRemoteIP()
{
	char* ip = (char*)inet_ntoa(m_client.sin_addr);
	if (ip != NULL)
		return std::string(ip);

	return std::string("noip");
}

void Socket::WriteCallback()
{
	if (IsDeleted() || !IsConnected())
		return;

	// We don't want any writes going on while this is happening.
	Guard<Mutex> lock(m_writeMutex);
	if(writeBuffer.GetContiguousBytes())
	{
		DWORD w_length = 0;
		DWORD flags = 0;

		// attempt to push all the data out in a non-blocking fashion.
		WSABUF buf;
		buf.len = (ULONG)writeBuffer.GetContiguousBytes();
		buf.buf = (char*)writeBuffer.GetBufferStart();

		m_writeEvent.Mark();
		m_writeEvent.Reset(SOCKET_IO_EVENT_WRITE_END);
		int r = WSASend(m_fd, &buf, 1, &w_length, flags, &m_writeEvent.m_overlap, 0);
		if (r == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			m_writeEvent.Unmark();
			DecSendLock();
			Disconnect();
		}
	}
	else
	{
		// Write operation is completed.
		DecSendLock();
	}
}

void Socket::SetupReadEvent()
{
	if (IsDeleted() || !IsConnected())
		return;

	Guard<Mutex> lock(m_readMutex);
	DWORD r_length = 0;
	DWORD flags = 0;
	WSABUF buf;
	buf.len = (ULONG)readBuffer.GetSpace();
	buf.buf = (char*)readBuffer.GetBuffer();	

	// event that will trigger after data is receieved
	m_readEvent.Mark();
	m_readEvent.Reset(SOCKET_IO_EVENT_READ_COMPLETE);
	if (WSARecv(m_fd, &buf, 1, &r_length, &flags, &m_readEvent.m_overlap, 0) == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			m_readEvent.Unmark();
			Disconnect();
		}
	}
}

void Socket::ReadCallback(uint32 len)
{
	readBuffer.IncrementWritten(len);
	OnRead();
	SetupReadEvent();
}

void Socket::AssignToCompletionPort()
{
	CreateIoCompletionPort((HANDLE)m_fd, m_completionPort, (ULONG_PTR)this, 0);
}

void Socket::BurstPush()
{
	if (AcquireSendLock())
		WriteCallback();
}

void Socket::Disconnect()
{
	if (!IsConnected())
		return;

	m_connected = false;

	shutdown(m_fd, SD_BOTH);
	closesocket(m_fd);
	m_fd = NULL;

	m_readEvent.Unmark();

	// Call virtual ondisconnect
	OnDisconnect();

	// remove from mgr
	m_socketMgr->OnDisconnect(this);

	//if (!IsDeleted())
	//	Delete();
}

void Socket::Delete()
{
	if (IsDeleted())
		return;

	m_deleted = true;

	if (IsConnected()) 
		Disconnect();

	delete this;
	// sSocketGarbageCollector.QueueSocket(this);
}

Socket::~Socket()
{
}
