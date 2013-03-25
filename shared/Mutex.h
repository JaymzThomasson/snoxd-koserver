#pragma once

#include <WinBase.h>

class Mutex
{
public:
	friend class Condition;

	/** Initializes a mutex class, with InitializeCriticalSection / pthread_mutex_init
	 */
	Mutex();

	/** Deletes the associated critical section / mutex
	 */
	~Mutex();

	/** Acquires this mutex. If it cannot be acquired immediately, it will block.
	 */
	void Acquire();

	/** Releases this mutex. No error checking performed
	 */
	void Release();

	/** Attempts to acquire this mutex. If it cannot be acquired (held by another thread)
	 * it will return false.
	 * @return false if cannot be acquired, true if it was acquired.
	 */
	bool AttemptAcquire();

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
	FastMutex() : m_lock(0), m_recursiveCount(0) {}
	~FastMutex() {}

	void Acquire();
	bool AttemptAcquire();
	void Release();
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