#pragma once

//	unix defines
#ifndef CONFIG_USE_IOCP
#	define SOCKET int
#	define SD_BOTH SHUT_RDWR

#	ifdef CONFIG_USE_EPOLL
#		include <sys/epoll.h>
#	else
#		include <sys/event.h>
#	endif
#else // IOCP-specific functionality
enum SocketIOEvent
{
	SOCKET_IO_EVENT_READ_COMPLETE   = 0,
	SOCKET_IO_EVENT_WRITE_END		= 1,
	SOCKET_IO_THREAD_SHUTDOWN		= 2,
	NUM_SOCKET_IO_EVENTS			= 3,
};

class OverlappedStruct
{
public:
	OVERLAPPED m_overlap;
	SocketIOEvent m_event;

#ifdef USE_STD_ATOMIC
	std::atomic_bool m_inUse;
#else
	volatile long m_inUse;
#endif

	OverlappedStruct(SocketIOEvent ev) : m_event(ev)
	{
		memset(&m_overlap, 0, sizeof(OVERLAPPED));
		m_inUse = 0;
	};

	OverlappedStruct()
	{
		memset(&m_overlap, 0, sizeof(OVERLAPPED));
		m_inUse = 0;
	}

	INLINE void Reset(SocketIOEvent ev)
	{
		memset(&m_overlap, 0, sizeof(OVERLAPPED));
		m_event = ev;
	}

	void Mark()
	{
#ifdef USE_STD_ATOMIC
		bool val = 0;
		if (!m_inUse.compare_exchange_strong(val, 1))
#else
		long val = InterlockedCompareExchange(&m_inUse, 1, 0);
		if (val != 0)
#endif
			TRACE("!!!! Network: Detected double use of read/write event! Previous event was %u.\n", m_event);
	}

	void Unmark()
	{
#ifdef USE_STD_ATOMIC
		m_inUse = 0;
#else
		InterlockedExchange(&m_inUse, 0);
#endif
	}
};
#endif