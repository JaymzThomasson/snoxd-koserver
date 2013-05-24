#include "stdafx.h"
#include "ReferenceObject.h"

ReferenceObject::ReferenceObject() 
#ifndef USE_STD_ATOMIC
	: m_refCount(0)
#endif
{
	IncRef();
}

void ReferenceObject::IncRef() 
{
#ifdef USE_STD_ATOMIC
	m_refCount++;
#else /* win32 only */
	InterlockedIncrement(&m_refCount);
#endif
}

void ReferenceObject::DecRef()
{
#ifdef USE_STD_ATOMIC
	if (--m_refCount == 0)
		delete this;
#else /* win32 only */
	if (InterlockedDecrement(&m_refCount) == 0) 
		delete this; 
#endif
}