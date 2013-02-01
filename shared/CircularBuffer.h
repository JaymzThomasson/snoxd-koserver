// CircularBuffer.h: interface for the CCircularBuffer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CIRCULARBUFFER_H__F4D345A4_CE05_11D1_8BEE_0060979C5900__INCLUDED_)
#define AFX_CIRCULARBUFFER_H__F4D345A4_CE05_11D1_8BEE_0060979C5900__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CCircularBuffer  
{
public:
	CCircularBuffer(int size=16384);
	virtual ~CCircularBuffer();

	void	PutData(char *pData, int len);
	void	GetData(char *pData, int len);
	char&	GetHeadData() { return m_pBuffer[m_iHeadPos]; }

	BOOL	HeadIncrease(int increasement=1);
	void	SetEmpty() { m_iOffset = m_iHeadPos = m_iTailPos = 0; }

	int		GetValidCount();

	unsigned int GetSize() { return m_iOffset; }
	bool isEmpty() { return (m_iOffset == 0); }

protected:
	BOOL	IsOverFlowCondition(int &len) { return (len >= m_iBufSize-GetValidCount()); }
	BOOL	IsIndexOverFlow(int &len) { return (len+m_iTailPos>=m_iBufSize); }
	void	BufferResize(); 
protected:
	int		m_iBufSize;
	char	*m_pBuffer;

	int		m_iHeadPos;
	int		m_iTailPos;

	unsigned int	m_iOffset;
};

inline int CCircularBuffer::GetValidCount()
{
	int count = m_iTailPos-m_iHeadPos;
	if (count<0) count = m_iBufSize+count;
	return count;
}

inline void CCircularBuffer::BufferResize()
{
	int prevBufSize;
	
	prevBufSize = m_iBufSize;
	m_iBufSize <<= 1;
	char *pNewData = new char[m_iBufSize];
	CopyMemory(pNewData, m_pBuffer, prevBufSize);
	if (m_iTailPos<m_iHeadPos) 
	{
		CopyMemory(pNewData+prevBufSize, m_pBuffer, m_iTailPos);
		m_iTailPos += prevBufSize;
	}
	delete [] m_pBuffer;
	m_pBuffer = pNewData;
}

inline void CCircularBuffer::PutData(char *pData, int len)
{
	if (len<=0) {TRACE("CCircularBuffer::PutData len is <=0\n"); return;}
	while (IsOverFlowCondition(len)) BufferResize();
	if (IsIndexOverFlow(len))
	{
		int FirstCopyLen = m_iBufSize-m_iTailPos;
		int SecondCopyLen = len - FirstCopyLen;
		ASSERT(FirstCopyLen);
		CopyMemory(m_pBuffer+m_iTailPos, pData, FirstCopyLen);
		if (SecondCopyLen)
		{
			CopyMemory(m_pBuffer, pData+FirstCopyLen, SecondCopyLen);
			m_iTailPos = SecondCopyLen;
		}
		else m_iTailPos = 0;

		m_iOffset += len;
	}
	else
	{
		CopyMemory(m_pBuffer+m_iTailPos, pData, len);
		m_iTailPos += len;
		m_iOffset += len;
	}
}

inline void CCircularBuffer::GetData(char *pData, int len)
{
	ASSERT(len>0&&len<=GetValidCount());
	if (len < m_iBufSize-m_iHeadPos)
		CopyMemory(pData, m_pBuffer+m_iHeadPos, len);
	else
	{
		int fc, sc;
		fc = m_iBufSize-m_iHeadPos;
		sc = len - fc;
		CopyMemory(pData, m_pBuffer+m_iHeadPos, fc);
		if (sc) CopyMemory(pData+fc, m_pBuffer, sc);
	}

	ASSERT(m_iOffset >= (unsigned int)len);
	m_iOffset -= len;
}

inline BOOL CCircularBuffer::HeadIncrease(int increasement)
{
	ASSERT(increasement<=GetValidCount());
	m_iHeadPos += increasement;
	m_iHeadPos %= m_iBufSize;
	return m_iHeadPos!=m_iTailPos;
}

#endif // !defined(AFX_CIRCULARBUFFER_H__F4D345A4_CE05_11D1_8BEE_0060979C5900__INCLUDED_)
