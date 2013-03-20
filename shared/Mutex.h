#pragma once

#include <WinBase.h>

class Mutex
{
public:
	friend class Condition;

	/** Initializes a mutex class, with InitializeCriticalSection / pthread_mutex_init
	 */
	Mutex() { InitializeCriticalSection(&cs); }

	/** Deletes the associated critical section / mutex
	 */
	~Mutex() { DeleteCriticalSection(&cs); }

	/** Acquires this mutex. If it cannot be acquired immediately, it will block.
	 */
	__forceinline void Acquire()
	{
		EnterCriticalSection(&cs);
	}

	/** Releases this mutex. No error checking performed
	 */
	__forceinline void Release()
	{
		LeaveCriticalSection(&cs);
	}

	/** Attempts to acquire this mutex. If it cannot be acquired (held by another thread)
	 * it will return false.
	 * @return false if cannot be acquired, true if it was acquired.
	 */
	__forceinline bool AttemptAcquire()
	{
		return (TryEnterCriticalSection(&cs) == TRUE ? true : false);
	}

protected:
	/** Critical section used for system calls
	 */
	CRITICAL_SECTION cs;
};

class FastMutex
{
#pragma pack(push,8)
	volatile long m_lock;
#pragma pack(pop)
	DWORD m_recursiveCount;

public:
	__forceinline FastMutex() : m_lock(0), m_recursiveCount(0) {}
	__forceinline ~FastMutex() {}

	__forceinline void Acquire()
	{
		DWORD thread_id = GetCurrentThreadId(), owner;
		if (thread_id == (DWORD)m_lock)
		{
			++m_recursiveCount;
			return;
		}

		for (;;)
		{
			owner = InterlockedCompareExchange(&m_lock, thread_id, 0);
			if(owner == 0)
				break;

			Sleep(0);
		}

		++m_recursiveCount;
	}

	__forceinline bool AttemptAcquire()
	{
		DWORD thread_id = GetCurrentThreadId();
		if(thread_id == (DWORD)m_lock)
		{
			++m_recursiveCount;
			return true;
		}

		DWORD owner = InterlockedCompareExchange(&m_lock, thread_id, 0);
		if(owner == 0)
		{
			++m_recursiveCount;
			return true;
		}

		return false;
	}

	__forceinline void Release()
	{
		if ((--m_recursiveCount) == 0)
			InterlockedExchange(&m_lock, 0);
	}
};

template <class T>
class Guard
{
public:
	Guard(T& mutex) : target(mutex)
	{
		target.Acquire();
	}

	~Guard()
	{
		target.Release();
	}

	Guard& operator=(Guard& src)
	{
		this->target = src.target;
		return *this;
	}

protected:
	T& target;
};

typedef Guard<FastMutex> FastGuard;