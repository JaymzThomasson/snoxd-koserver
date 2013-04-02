#pragma once

#include <deque>

class Mutex;
class Condition
{
public:
	Condition(Mutex * mutex);
	~Condition();
	void BeginSynchronized();
	void EndSynchronized();
	DWORD Wait(time_t timeout);
	DWORD Wait();
	void Signal();
	void Broadcast();

private:
	HANDLE Push();
	HANDLE Pop();
	BOOL LockHeldByCallingThread();

	std::deque<HANDLE> m_deqWaitSet;
	CRITICAL_SECTION m_critsecWaitSetProtection;
	Mutex * m_externalMutex;
	int m_nLockCount;
};