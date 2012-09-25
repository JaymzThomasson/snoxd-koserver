#pragma once
#include "Condition.h"
class RWLock
{
public: 
	__forceinline void AcquireReadLock()
	{
		_cond.BeginSynchronized();
		_readers++;
		_cond.EndSynchronized();
	}
	
	__forceinline void ReleaseReadLock()
	{
		_cond.BeginSynchronized();
		if (!(--_readers))
			if(_writers)
				_cond.Signal();
		_cond.EndSynchronized();
	}

	__forceinline void AcquireWriteLock()
	{
		_cond.BeginSynchronized();
		_writers++;
		if (_readers)
			_cond.Wait();
	}

	__forceinline void ReleaseWriteLock()
	{
		if (--_writers)
			_cond.Signal();
		_cond.EndSynchronized();
	}
	__forceinline RWLock() : _cond(&_lock) { _readers = _writers =0; }
  
	private:
		Mutex _lock;
		Condition _cond;
		volatile unsigned int _readers;
		volatile unsigned int _writers;
}; 
