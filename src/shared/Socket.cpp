#include "stdafx.h"
#include "SocketMgr.h"

Socket::Socket(SOCKET fd, uint32 sendbuffersize, uint32 recvbuffersize) 
	: m_fd(fd), m_connected(false),	m_deleted(false), m_socketMgr(nullptr)
{
	// Allocate buffers
	readBuffer.Allocate(recvbuffersize);
	writeBuffer.Allocate(sendbuffersize);

#ifdef CONFIG_USE_IOCP
	// IOCP member variables
	m_writeLock = 0;
	m_completionPort = 0;

	// Check for needed fd allocation.
	if (m_fd == 0) // TO-DO: Wrap this up into its own method
		m_fd = WSASocket(AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED);
#else
	ASSERT(m_fd != 0);
#endif
}

bool Socket::Connect(const char * Address, uint32 Port)
{
	struct hostent * ci = gethostbyname(Address);
	if (ci == 0)
		return false;

	m_client.sin_family = ci->h_addrtype;
	m_client.sin_port = ntohs((u_short)Port);
	memcpy(&m_client.sin_addr.s_addr, ci->h_addr_list[0], ci->h_length);

	SocketOps::Blocking(m_fd);

#ifdef CONFIG_USE_IOCP
	if (m_fd == 0)
		m_fd = WSASocket(AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED);
#endif

	if (connect(m_fd, (const sockaddr*)&m_client, sizeof(m_client)) == -1)
		return false;

	// at this point the connection was established
#ifdef CONFIG_USE_IOCP
	m_completionPort = m_socketMgr->GetCompletionPort();
#endif
	m_socketMgr->OnConnect(this);

	_OnConnect();
	return true;
}
void Socket::Accept(sockaddr_in * address)
{
	memcpy(&m_client, address, sizeof(*address));
	_OnConnect();
}

void Socket::_OnConnect()
{
	// set common parameters on the file descriptor
	SocketOps::Nonblocking(m_fd);
	SocketOps::EnableBuffering(m_fd);
	m_connected = true;

	// IOCP stuff
#ifdef CONFIG_USE_IOCP
	AssignToCompletionPort();
#endif

	// Call virtual onconnect
	OnConnect();

	// Setting the read event up after calling OnConnect() ensures OnConnect() & subsequent connection setup code is run first (which is NOT GUARANTEED otherwise)
#ifdef CONFIG_USE_IOCP
	SetupReadEvent();
#endif
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
	if (ip != nullptr)
		return std::string(ip);

	return std::string("noip");
}

void Socket::Disconnect()
{
	if (!IsConnected())
		return;

	m_connected = false;
	
	SocketOps::CloseSocket(m_fd);
	m_fd = 0;

#ifdef CONFIG_USE_IOCP
	m_readEvent.Unmark();
#endif

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
