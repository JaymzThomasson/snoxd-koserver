#include "stdafx.h"
#include "SharedMem.h"
#include <process.h>

void aa() {};		// nop function

CSharedMemQueue::CSharedMemQueue()
{
	m_hMMFile = NULL;
	m_lpMMFile = NULL;
	m_bMMFCreate = FALSE;
	m_nMaxCount = 0;
	m_wOffset = 0;
	m_pHeader = NULL;
}

CSharedMemQueue::~CSharedMemQueue()
{
	if( m_lpMMFile )
		UnmapViewOfFile(m_lpMMFile);
	if( m_hMMFile )
		CloseHandle(m_hMMFile);
}

BOOL CSharedMemQueue::InitailizeMMF(DWORD dwOffsetsize, int maxcount, LPCTSTR lpname, BOOL bCreate )
{
	if( maxcount < 1 )
		return FALSE;
	DWORD dwfullsize = dwOffsetsize * maxcount + sizeof(_SMQ_HEADER);

	m_nMaxCount = maxcount;
	m_wOffset = dwOffsetsize;

	if( bCreate )
		m_hMMFile = CreateFileMapping( (HANDLE)0xFFFFFFFF, NULL, PAGE_READWRITE, 0, dwfullsize, lpname );
	else
		m_hMMFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, TRUE, lpname );
	
	if( m_hMMFile == NULL )
		return FALSE;

    m_lpMMFile = (char *)MapViewOfFile(m_hMMFile, FILE_MAP_WRITE, 0, 0, 0);
	if( !m_lpMMFile )
		return FALSE;

	m_bMMFCreate = bCreate;
	m_pHeader = (_SMQ_HEADER *)m_lpMMFile;
	m_lReference = (LONG)(m_lpMMFile + sizeof(_SMQ_HEADER));		// 초기 위치 셋팅

	if( bCreate ) {
		memset( m_lpMMFile, 0x00, dwfullsize );
		m_pHeader->Rear = m_pHeader->Front = 0;
		m_pHeader->nCount = 0;
		m_pHeader->RearMode = m_pHeader->FrontMode = E;
		m_pHeader->CreatePid = _getpid();
	}

	return TRUE;
}

// TO-DO: Remove this ENTIRE queue system.
int CSharedMemQueue::PutData(Packet *pkt, int16 uid /*= -1*/)
{
	BYTE BlockMode;
	int index = 0, count = 0;
	short size = pkt->size() + 1;

	if ((DWORD)size > m_wOffset)
	{
		TRACE("DataSize Over.. - %d bytes\n", pkt->size());
		return SMQ_PKTSIZEOVER;
	}

	do {
		if( m_pHeader->RearMode == W ) {
			aa();
			count++;
			continue;
		}

		m_pHeader->RearMode = W;
		m_pHeader->WritePid = ::GetCurrentThreadId();	// writing side (game server) is multi thread

		aa();	// no operation function

		if( m_pHeader->WritePid != ::GetCurrentThreadId() ) {
			count++;
			continue;
		}

		LONG pQueue = m_lReference + (m_pHeader->Rear * m_wOffset);
		BlockMode = GetByte( (char*)pQueue, index );
		if( BlockMode == WR && m_pHeader->nCount >= MAX_COUNT-1 ) {
			m_pHeader->RearMode = WR;
			return SMQ_FULL;
		}

		index = 0;
		char *queue = ((char *)pQueue);
		queue[index++] = WR;	// Block Mode Set to WR	-> Data Exist
		SetShort(queue, size, index);
		SetShort(queue, uid, index);
		queue[index++] = pkt->GetOpcode();
		if (pkt->size() > 0)
			memcpy(queue+index, pkt->contents(), size);
		m_pHeader->nCount++;

		m_pHeader->Rear = (m_pHeader->Rear + 1) % MAX_COUNT;
		m_pHeader->RearMode = WR;
		
		break;

	} while( count < 50 );
	if( count >= 50 ) {
		m_pHeader->RearMode = WR;
		return SMQ_WRITING;
	}

	return 1;
}

int CSharedMemQueue::GetData(Packet & pkt, int16 * uid)
{
	int index = 0, size = 0, temp_front = 0;
	BYTE BlockMode;
	
	if( m_pHeader->FrontMode == R ) 
		return SMQ_READING;

	m_pHeader->FrontMode = R;
	m_pHeader->ReadPid = _getpid();	// reading side ( agent ) is multi process ( one process -> act each single thread )

	aa();	// no operation function
	
	if( m_pHeader->ReadPid != _getpid() ) {
		m_pHeader->FrontMode = WR;
		return SMQ_READING;
	}

	LONG pQueue = m_lReference + (m_pHeader->Front * m_wOffset);

	index = 0;
	BlockMode = GetByte( (char *)pQueue, index );
	if( BlockMode == E ) {
		m_pHeader->FrontMode = WR;
		if( m_pHeader->Front < m_pHeader->Rear || (m_pHeader->Front > m_pHeader->Rear && m_pHeader->Front > MAX_COUNT-100) ) {
			temp_front = (m_pHeader->Front + 1) % MAX_COUNT;
			m_pHeader->Front = temp_front;
			m_pHeader->nCount--;
			TRACE("SMQ EMPTY Block Find - F:%d, R:%d\n", m_pHeader->Front, m_pHeader->Rear);
		}
		return SMQ_EMPTY;
	}

	size = GetShort((char *)pQueue, index);
	if (size <= 0 || size > MAX_PKTSIZE)
		return 0;

	*uid = GetShort((char *)pQueue, index);

	pkt.Initialize(*(char *)(pQueue + index));
	if (size > 1)
		pkt.append((char *)(pQueue + index + 1), size - 1);

	m_pHeader->nCount--;

	temp_front = (m_pHeader->Front + 1) % MAX_COUNT;
	m_pHeader->Front = temp_front;

	memset( (void*)pQueue, 0x00, m_wOffset );

	m_pHeader->FrontMode = WR;

	return size;
}