#pragma once

#include <map>
#include "RWLock.h"

#include "SocketMgr.h"
#include "KOSocket.h"

typedef std::map<uint16, KOSocket *> SessionMap;

template <class T>
class KOSocketMgr : public SocketMgr
{
public:
	KOSocketMgr<T>() : m_server(NULL) {}

	virtual void InitSessions(uint16 sTotalSessions);
	virtual bool Listen(uint16 sPort, uint16 sTotalSessions);

	virtual Socket *AssignSocket(SOCKET socket);
	virtual void DisconnectCallback(Socket *pSock);

	void RunServer()
	{
		SpawnWorkerThreads();
		GetServer()->run();
	}

	// Prevent new connections from being made
	void SuspendServer()
	{
		GetServer()->suspend();
	}

	// Allow new connections to be made
	void ResumeServer()
	{
		GetServer()->resume();
	}

	// Send a packet to all active sessions
	void SendAll(char *pBuf, int nLength) // pending deprecation
	{
		AcquireLock();
		SessionMap & sessMap = m_activeSessions;
		for (auto itr = sessMap.begin(); itr != sessMap.end(); ++itr)
			itr->second->Send(pBuf, nLength);
		ReleaseLock();
	}

	// Send a packet to all active sessions
	void SendAll(Packet * pkt) 
	{
		AcquireLock();
		SessionMap & sessMap = m_activeSessions;
		for (auto itr = sessMap.begin(); itr != sessMap.end(); ++itr)
			itr->second->Send(pkt);
		ReleaseLock();
	}

	ListenSocket<T> * GetServer() { return m_server; }
	__forceinline SessionMap & GetIdleSessionMap()
	{
		AcquireLock();
		return m_idleSessions;
	}

	__forceinline SessionMap & GetActiveSessionMap()
	{
		AcquireLock();
		return m_activeSessions;
	}

	__forceinline void AcquireLock() { m_lock.AcquireReadLock(); }
	__forceinline void ReleaseLock() { m_lock.ReleaseReadLock(); }

	T * operator[] (uint16 id)
	{
		T * result = NULL;

		AcquireLock();
		auto itr = m_activeSessions.find(id);
		if (itr != m_activeSessions.end())
			result = static_cast<T *>(itr->second);
		ReleaseLock();

		return result;
	}

	virtual ~KOSocketMgr();

protected:
	SessionMap m_idleSessions, m_activeSessions;
	RWLock m_lock;

private:
	ListenSocket<T> * m_server;
};

template <class T>
void KOSocketMgr<T>::InitSessions(uint16 sTotalSessions)
{
	m_lock.AcquireWriteLock();
	for (uint16 i = 0; i < sTotalSessions; i++)
		m_idleSessions.insert(std::make_pair(i, new T(i, this)));
	m_lock.ReleaseWriteLock();
}

template <class T>
bool KOSocketMgr<T>::Listen(uint16 sPort, uint16 sTotalSessions)
{
	if (m_server != NULL)
		return false;

	CreateCompletionPort();
	m_server = new ListenSocket<T>(this, "0.0.0.0", sPort);
	if (!m_server->IsOpen())
		return false;

	InitSessions(sTotalSessions);
	return true;
}

template <class T>
Socket * KOSocketMgr<T>::AssignSocket(SOCKET socket)
{
	Socket *pSock = NULL;

	m_lock.AcquireWriteLock();
	auto itr = m_idleSessions.begin();
	if (itr != m_idleSessions.end())
	{
		m_activeSessions.insert(std::make_pair(itr->first, itr->second));
		pSock = itr->second;
		m_idleSessions.erase(itr);
		pSock->SetFd(socket);
	}
	m_lock.ReleaseWriteLock();
	return pSock;
}

template <class T>
void KOSocketMgr<T>::DisconnectCallback(Socket *pSock)
{
	m_lock.AcquireWriteLock();
	auto itr = m_activeSessions.find(static_cast<KOSocket *>(pSock)->GetSocketID());
	if (itr != m_activeSessions.end())
	{
		m_idleSessions.insert(std::make_pair(itr->first, itr->second));
		m_activeSessions.erase(itr);
	}
	m_lock.ReleaseWriteLock();
}

template <class T>
KOSocketMgr<T>::~KOSocketMgr()
{
	SessionMap killMap;
	m_lock.AcquireWriteLock();

	killMap = m_activeSessions; // copy active session map (don't want to break the iterator)
	for (auto itr = killMap.begin(); itr != killMap.end(); ++itr)
		itr->second->Disconnect();

	for (auto itr = m_idleSessions.begin(); itr != m_idleSessions.end(); ++itr)
		itr->second->Delete();

	m_idleSessions.clear();
	m_lock.ReleaseWriteLock();

	if (m_server != NULL)
		delete m_server;
}