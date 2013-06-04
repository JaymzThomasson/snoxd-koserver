#pragma once

template <typename T>
class Atomic
{
public:
	Atomic() {}
	Atomic(T value) : m_atomic(value) {}

	INLINE Atomic& operator++() { increment(); return *this; }
	INLINE Atomic& operator--() { decrement(); return *this; }

#ifdef USE_STD_ATOMIC
	template <typename T2> 
	INLINE Atomic& operator=(const T2 & rhs) { m_atomic = rhs; return *this; }

	INLINE T increment() { return ++m_atomic; }
	INLINE T decrement() { return --m_atomic; }

	INLINE bool compare_exchange(T & expected, T desired) { return m_atomic.compare_exchange_strong(expected, desired); }
#else
	template <typename T2> 
	INLINE Atomic& operator=(const T2 & rhs) { InterlockedExchange(&m_atomic, rhs); return *this; }

	INLINE T increment() { return (T) InterlockedIncrement(&m_atomic); }
	INLINE T decrement() { return (T) InterlockedDecrement(&m_atomic); }

	INLINE bool compare_exchange(T & expected, T desired) 
	{
		long val = InterlockedCompareExchange(&m_atomic, desired, expected);
		return (val == (long) expected);
	}
#endif

protected:
#ifdef USE_STD_ATOMIC
	std::atomic<T>	m_atomic;
#else
	volatile long	m_atomic;
#endif

private:
	Atomic(const Atomic & other); /* disable copy constructor */
};
