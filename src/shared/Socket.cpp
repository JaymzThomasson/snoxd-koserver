#include "stdafx.h"
#include "SocketMgr.h"

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
	if (ip != nullptr)
		return std::string(ip);

	return std::string("noip");
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
	
	SocketOps::CloseSocket(m_fd);
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
