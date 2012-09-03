#pragma once
#ifndef __GLOBALS_H
#define __GLOBALS_H

#include "packets.h"

#define MAX_USER			3000

#define MAX_ID_SIZE			20
#define MAX_PW_SIZE			12

#define MAX_ITEM_COUNT		9999

enum NameType
{
	TYPE_ACCOUNT,
	TYPE_CHARACTER
};

// ITEM_SLOT DEFINE
const BYTE RIGHTEAR			= 0;
const BYTE HEAD				= 1;
const BYTE LEFTEAR			= 2;
const BYTE NECK				= 3;
const BYTE BREAST			= 4;
const BYTE SHOULDER			= 5;
const BYTE RIGHTHAND		= 6;
const BYTE WAIST			= 7;
const BYTE LEFTHAND			= 8;
const BYTE RIGHTRING		= 9;
const BYTE LEG				= 10;
const BYTE LEFTRING			= 11;
const BYTE GLOVE			= 12;
const BYTE FOOT				= 13;
const BYTE RESERVED			= 14;

const BYTE SLOT_MAX			= 14;
const BYTE HAVE_MAX			= 28;
const BYTE COSP_MAX			= 7;
const BYTE MBAG_MAX			= 24;
const BYTE WAREHOUSE_MAX	= 196;

const int ITEMCOUNT_MAX		= 999;

//////////////////////////////////////////////////////////////////
// DEFINE Shared Memory Queue
//////////////////////////////////////////////////////////////////

#define E	0x00
#define R	0x01
#define W	0x02
#define WR	0x03

// DEFINE Shared Memory Queue Return VALUE

#define SMQ_BROKEN		10000
#define SMQ_FULL		10001
#define SMQ_EMPTY		10002
#define SMQ_PKTSIZEOVER	10003
#define SMQ_WRITING		10004
#define SMQ_READING		10005
#define SMQ_INVALID		10006

// DEFINE Shared Memory Costumizing

#define MAX_PKTSIZE		512
#define MAX_COUNT		4096
#define SMQ_LOGGERSEND	"KNIGHT_SEND"
#define SMQ_LOGGERRECV	"KNIGHT_RECV"

#define SMQ_ITEMLOGGER	"ITEMLOG_SEND"

struct	_ITEM_DATA
{
	int		nNum;
	short	sDuration;
	short	sCount;	
	__int64	nSerialNum;
};

enum HairData
{
	HAIR_R,
	HAIR_G,
	HAIR_B,
	HAIR_TYPE
};

struct _USER_DATA
{
	char	m_id[MAX_ID_SIZE+1];
	char	m_Accountid[MAX_ID_SIZE+1];

	BYTE	m_bZone;
	float	m_curx;	
	float	m_curz;
	float	m_cury;

	BYTE	m_bNation;
	BYTE	m_bRace;
	short	m_sClass;

	DWORD	m_nHair;

	BYTE	m_bRank;
	BYTE	m_bTitle;
	BYTE	m_bLevel;
	int		m_iExp;	
	int		m_iLoyalty;	
	int		m_iLoyaltyMonthly;
	int		m_iMannerPoint;
	BYTE	m_bFace;
	BYTE	m_bCity;
	short	m_bKnights;	
	BYTE	m_bFame;
	short	m_sHp;
	short	m_sMp;
	short	m_sSp;
	BYTE	m_bStr;	
	BYTE	m_bSta;	
	BYTE	m_bDex;
	BYTE	m_bIntel;
	BYTE	m_bCha;
	BYTE	m_bAuthority;
	short	m_sPoints;
	unsigned int		m_iGold;
	short	m_sBind;
	unsigned int		m_iBank;
	
	BYTE    m_bstrSkill[9];	
	_ITEM_DATA m_sItemArray[HAVE_MAX+SLOT_MAX + COSP_MAX + MBAG_MAX];
	_ITEM_DATA m_sWarehouseArray[WAREHOUSE_MAX];

	BYTE	m_bLogout;
	BYTE	m_bWarehouse;
	DWORD	m_dwTime;

	// this system needs replacing
	int		m_sQuestCount;
	BYTE	m_bstrQuest[400];
};

inline void GetString(char* tBuf, char* sBuf, int len, int& index)
{
	memcpy(tBuf, sBuf+index, len);
	index += len;
};

inline BYTE GetByte(char* sBuf, int& index)
{
	int t_index = index;
	index++;
	return (BYTE)(*(sBuf+t_index));
};

inline int GetShort(char* sBuf, int& index)
{
	index += 2;
	return *(short*)(sBuf+index-2);
};

inline DWORD GetDWORD(char* sBuf, int& index)
{
	index += 4;
	return *(DWORD*)(sBuf+index-4);
};

inline float Getfloat(char* sBuf, int& index)
{
	index += 4;
	return *(float*)(sBuf+index-4);
};

inline __int64 GetInt64(char* sBuf, int& index)
{
	index += 8;
	return *(__int64*)(sBuf+index-8);
};

inline bool GetKOString(char* sBuf, char* tBuf, int& index, unsigned int maxLen, int lenSize = 2)
{
	unsigned short len = 0;
	if (lenSize == 1)
		len = GetByte(sBuf, index);
	else 
		len = GetShort(sBuf, index);

	if (len > maxLen)
		return false;

	GetString(tBuf, sBuf, len, index);
	return true;
};

inline void SetString(char* tBuf, char* sBuf, int len, int& index)
{
	memcpy(tBuf+index, sBuf, len);
	index += len;
};

inline void SetByte(char* tBuf, BYTE sByte, int& index)
{
	*(tBuf+index) = (char)sByte;
	index++;
};

inline void SetShort(char* tBuf, int sShort, int& index)
{
	short temp = (short)sShort;

	CopyMemory( tBuf+index, &temp, 2);
	index += 2;
};

inline void SetDWORD(char* tBuf, DWORD sDWORD, int& index)
{
	CopyMemory( tBuf+index, &sDWORD, 4);
	index += 4;
};

inline void Setfloat ( char* tBuf, float sFloat, int& index )
{
	CopyMemory( tBuf+index, &sFloat, 4);
	index += 4;
};

inline void SetInt64 ( char* tBuf, __int64 nInt64, int& index )
{
	CopyMemory( tBuf+index, &nInt64, 8);
	index += 8;
};
// sungyong 2001.11.06
inline int GetVarString(TCHAR* tBuf, TCHAR* sBuf, int nSize, int& index)
{
	int nLen = 0;
	
	if(nSize == sizeof(BYTE))	nLen = GetByte(sBuf, index);
	else nLen = GetShort(sBuf, index);

	GetString(tBuf, sBuf, nLen, index);
	*(tBuf + nLen) = 0;

	return nLen;
};

inline void SetVarString(TCHAR *tBuf, TCHAR* sBuf, int len, int &index)
{
	*(tBuf+index) = (BYTE)len;
	index ++;

	CopyMemory(tBuf+index, sBuf, len);
	index += len;
};

inline void SetKOString(char* tBuf, char* sBuf, int& index, int lenSize = 2)
{
	short len = strlen(sBuf);
	if (lenSize == 1)
		SetByte(tBuf, (BYTE)len, index);
	else if (lenSize == 2)
		SetShort(tBuf, len, index);

	SetString(tBuf, sBuf, len, index);
};

// ~sungyong 2001.11.06
inline int ParseSpace( char* tBuf, char* sBuf)
{
	int i = 0, index = 0;
	BOOL flag = FALSE;
	
	while(sBuf[index] == ' ' || sBuf[index] == '\t')index++;
	while(sBuf[index] !=' ' && sBuf[index] !='\t' && sBuf[index] !=(BYTE) 0){
		tBuf[i++] = sBuf[index++];
		flag = TRUE;
	}
	tBuf[i] = 0;

	while(sBuf[index] == ' ' || sBuf[index] == '\t')index++;
	if(!flag) return 0;	
	return index;
};

inline CString GetProgPath()
{
	char Buf[_MAX_PATH], Path[_MAX_PATH];
	char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

	::GetModuleFileName(AfxGetApp()->m_hInstance, Buf, 256);
	_splitpath_s(Buf, drive, sizeof(drive), dir, sizeof(dir), fname, sizeof(fname), ext, sizeof(ext));
	strcpy_s(Path, sizeof(Path), drive);
	strcat_s(Path, sizeof(Path), dir);		
	return (CString)Path;
};

inline int myrand( int min, int max )
{
	if( min == max ) return min;
	if( min > max )
	{
		int temp = min;
		min = max;
		max = temp;
	}

	double gap = max - min + 1;
	double rrr = (double)RAND_MAX / gap;

	double rand_result;

	rand_result = (double)rand() / rrr;

	if( (int)( min + (int)rand_result ) < min ) return min;
	if( (int)( min + (int)rand_result ) > max ) return max;

	return (int)( min + (int)rand_result );
};

inline void	TimeTrace(TCHAR* pMsg)
{
	CString szMsg = _T("");
	CTime time = CTime::GetCurrentTime();
	szMsg.Format("%s,,  time : %d-%d-%d, %d:%d]\n", pMsg, time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute());
	TRACE(szMsg);
};


/*
	Yes, this is ugly and crude.
	I want to wrap all the existing log code into this, and slowly get rid of it... bit by bit.
*/
#define DEBUG_LOG(...) _DEBUG_LOG(false, __VA_ARGS__)
#define DEBUG_LOG_FILE(...) _DEBUG_LOG(true, __VA_ARGS__)
inline void _DEBUG_LOG(bool toFile, char * format, ...)
{
	char buffer[256];
	memset(buffer, 0x00, sizeof(buffer));

	va_list args;
	va_start(args, format);
	_vsnprintf_s(buffer, sizeof(buffer) - 1, format, args);
	va_end(args);

	TRACE("%s\n", buffer);

	if (toFile)
		LogFileWrite(buffer);
};

#endif