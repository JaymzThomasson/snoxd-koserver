#pragma once

#include "SocketDefines.h"
#include "CircularBuffer.h"
#include "Mutex.h"
#include <string>

class SocketMgr;
class Socket
{
public:
	// Constructor. If fd = 0, it will be assigned 
	Socket(SOCKET fd, uint32 sendbuffersize, uint32 recvbuffersize);
	
	// Open a connection to another machine.
	bool Connect(const char * Address, uint32 Port);

	// Disconnect the socket.
	void Disconnect();

	// Enable/disable blocking on a socket
	void SetBlocking(bool block = false);

	// Enable/disable nagle buffering on a socket
	void SetBuffering(bool enable = true);

	// Accept from the already-set fd.
	void Accept(sockaddr_in * address);

	/* Implementable methods */

	// Called when data is received.
	virtual void OnRead() {}

	// Called when a connection is first successfully established.
	virtual void OnConnect() {}

	// Called when the socket is disconnected from the client (either forcibly or by the connection dropping)
	virtual void OnDisconnect() {}

	/* Send Operations */

	// Locks sending mutex, adds bytes, unlocks mutex.
	bool Send(const uint8 * Bytes, uint32 Size);

	// Burst system - Locks the sending mutex.
	__forceinline  void BurstBegin() { m_writeMutex.Acquire(); }

	// Burst system - Adds bytes to output buffer.
	bool BurstSend(const uint8 * Bytes, uint32 Size);

	// Burst system - Pushes event to queue - do at the end of write events.
	void BurstPush();

	// Burst system - Unlocks the sending mutex.
	__forceinline void BurstEnd() { m_writeMutex.Release(); }

	/* Client Operations */

	// Get the client's ip in numerical form.
	std::string GetRemoteIP();
	__forceinline sockaddr_in & GetRemoteStruct() { return m_client; }
	__forceinline in_addr GetRemoteAddress() { return m_client.sin_addr; }
	__forceinline uint32 GetRemotePort() { return ntohs(m_client.sin_port); }
	__forceinline SOCKET GetFd() { return m_fd; }
	__forceinline SocketMgr * GetSocketMgr() { return m_socketMgr; }
	
	void SetupReadEvent();
	void ReadCallback(uint32 len);
	void WriteCallback();

	__forceinline bool IsDeleted() { return m_deleted; }
	__forceinline bool IsConnected() { return m_connected; }
	__forceinline CircularBuffer& GetReadBuffer() { return readBuffer; }
	__forceinline CircularBuffer& GetWriteBuffer() { return writeBuffer; }

	__forceinline void SetFd(SOCKET fd) { m_fd = fd; }
	__forceinline void SetSocketMgr(SocketMgr *mgr) { m_socketMgr = mgr; }

	/* Deletion */
	void Delete();

	// Destructor.
	virtual ~Socket();

protected:
	// Called when connection is opened.
	void _OnConnect();
  
	SOCKET m_fd;

	CircularBuffer readBuffer, writeBuffer;

	Mutex m_writeMutex, m_readMutex;

	// are we connected? stop from posting events.
	bool m_connected;

	// are we deleted? stop us from posting events.
	bool m_deleted;

	sockaddr_in m_client;

	SocketMgr *m_socketMgr;

	// IOCP specific
public:

	// Set completion port that this socket will be assigned to.
	__forceinline void SetCompletionPort(HANDLE cp) { m_completionPort = cp; }
	
	// Atomic wrapper functions for increasing read/write locks
	__forceinline void IncSendLock() { InterlockedIncrement(&m_writeLock); }
	__forceinline void DecSendLock() { InterlockedDecrement(&m_writeLock); }
	__forceinline bool AcquireSendLock()
	{
		if (m_writeLock)
			return false;

		IncSendLock();
		return true;
	}

	OverlappedStruct m_readEvent, m_writeEvent;

private:
	// Completion port socket is assigned to
	HANDLE m_completionPort;
	
	// Write lock, stops multiple write events from being posted.
	volatile long m_writeLock;
	
	// Assigns the socket to his completion port.
	void AssignToCompletionPort();
};