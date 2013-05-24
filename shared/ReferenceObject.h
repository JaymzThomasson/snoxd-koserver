#pragma once

class ReferenceObject
{
public:
	ReferenceObject();

	// Increment the reference count
	void IncRef();

	// Decrease the reference count and delete the object if it hits 0 (i.e. no more references).
	void DecRef();

	virtual ~ReferenceObject() {}

private:
#ifdef USE_STD_ATOMIC
	std::atomic_ulong m_refCount;
#else
	volatile long m_refCount;
#endif
};