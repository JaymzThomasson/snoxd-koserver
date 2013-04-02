#include "WindowsHeaders.h"
#include "ReferenceObject.h"

ReferenceObject::ReferenceObject() 
	: m_refCount(1)
{
}

void ReferenceObject::IncRef() 
{
#ifdef _WIN32
	InterlockedIncrement(&m_refCount);
#endif
}

void ReferenceObject::DecRef()
{
#ifdef _WIN32
	if (InterlockedDecrement(&m_refCount) == 0) 
		delete this; 
#endif
}