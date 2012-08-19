#ifndef _DEFINE_H
#define _DEFINE_H

#define MAX_USER			3000
#define MAX_ID_SIZE			20
#define MAX_PW_SIZE			12
#define MAX_ITEM			28

////////////////////////////////////////////////////////////
// Socket Define
////////////////////////////////////////////////////////////
#define SOCKET_BUFF_SIZE	(1024*8)
#define MAX_PACKET_SIZE		(1024*2)

#define PACKET_START1				0XAA
#define PACKET_START2				0X55
#define PACKET_END1					0X55
#define PACKET_END2					0XAA
//#define PROTOCOL_VER				0X01

// status
#define STATE_CONNECTED			0X01
#define STATE_DISCONNECTED		0X02
#define STATE_GAMESTART			0x03

/////////////////////////////////////////////////////
// ITEM_SLOT DEFINE
#define RIGHTEAR			0
#define HEAD				1
#define LEFTEAR				2
#define NECK				3
#define BREAST				4
#define SHOULDER			5
#define RIGHTHAND			6
#define WAIST				7
#define LEFTHAND			8
#define RIGHTRING			9
#define LEG					10
#define LEFTRING			11
#define GLOVE				12
#define FOOT				13
/////////////////////////////////////////////////////

////////////////////////////////////////////////////////////

typedef union{
	short int	i;
	BYTE		b[2];
} MYSHORT;

typedef union{
	int			i;
	BYTE		b[4];
} MYINT;

typedef union{
	DWORD		w;
	BYTE		b[4];
} MYDWORD;


// DEFINE MACRO PART...
#define BufInc(x) (x)++;(x) %= SOCKET_BUF_SIZE;

// DEFINE Shared Memory Queue Flag

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

// DEFINE Shared Memory Costumizing

#define MAX_PKTSIZE		512
#define MAX_COUNT		4096
#define SMQ_LOGGERSEND	"KNIGHT_SEND"
#define SMQ_LOGGERRECV	"KNIGHT_RECV"

// Packet Define...
#define WIZ_LOGIN				0x01	// Account Login
#define WIZ_NEW_CHAR			0x02	// Create Character DB
#define WIZ_DEL_CHAR			0x03	// Delete Character DB
#define WIZ_SEL_CHAR			0x04	// Select Character
#define WIZ_SEL_NATION			0x05	// Select Nation
#define WIZ_ALLCHAR_INFO_REQ	0x0C	// Account All Character Info Request
#define WIZ_LOGOUT				0x0F	// Request Logout
#define WIZ_DATASAVE			0x37	// User GameData DB Save Request
#define WIZ_KNIGHTS_PROCESS		0x3C	// Knights Related Packet..
#define WIZ_CLAN_PROCESS		0x4E	// Clans Related Packet..
#define WIZ_LOGIN_INFO			0x50	// define for DBAgent Communication
#define WIZ_KICKOUT				0x51	// Account ID forbid duplicate connection
#define WIZ_BATTLE_EVENT		0x57	// Battle Event Result

#define DB_COUPON_EVENT			0x10	// coupon event
		#define CHECK_COUPON_EVENT		0x01
		#define UPDATE_COUPON_EVENT		0x02

////////////////////////////////////////////////////////////////
// Knights Packet sub define 
////////////////////////////////////////////////////////////////
#define KNIGHTS_CREATE			0x11		// 생성
#define KNIGHTS_JOIN			0x12		// 가입
#define KNIGHTS_WITHDRAW		0x13		// 탈퇴
#define KNIGHTS_REMOVE			0x14		// 멤버 삭제
#define KNIGHTS_DESTROY			0x15		// 뽀개기
#define KNIGHTS_ADMIT			0x16		// 멤버 가입 허가
#define KNIGHTS_REJECT			0x17		// 멤버 가입 거절
#define KNIGHTS_PUNISH			0x18		// 멤버 징계
#define KNIGHTS_CHIEF			0x19		// 단장 임명
#define KNIGHTS_VICECHIEF		0x1A		// 부단장 임명
#define KNIGHTS_OFFICER			0x1B		// 장교임명
#define KNIGHTS_ALLLIST_REQ		0x1C		// 리스트를 10개 단위로 Page 요청
#define KNIGHTS_MEMBER_REQ		0x1D		// 모든 멤버 요청
#define KNIGHTS_CURRENT_REQ		0x1E		// 현재 접속 리스트
#define KNIGHTS_STASH			0x1F		// 기사단 창고
#define KNIGHTS_MODIFY_FAME		0x20		// 멤버의 직위 변경.. 해당 멤버에게 간다
#define KNIGHTS_JOIN_REQ		0x21		// 해당멤버에게 가입요청을 한다
#define KNIGHTS_LIST_REQ		0x22		// 기사단 리스트를  요청 ( index 검색 )

////////////////////////////////////////////////////////////////
// Clan Packet sub define
////////////////////////////////////////////////////////////////
#define CLAN_CREATE				0x01
#define CLAN_JOIN				0x02

////////////////////////////////////////////////////////////////
// Update User Data type define
////////////////////////////////////////////////////////////////
#define UPDATE_LOGOUT			0x01
#define UPDATE_ALL_SAVE			0x02
#define UPDATE_PACKET_SAVE		0x03


// Reply packet define...

#define SEND_ME					0x01
#define SEND_REGION				0x02
#define SEND_ALL				0x03

const BYTE SLOT_MAX			= 14;
const BYTE HAVE_MAX			= 28;
const BYTE COSP_MAX			= 7;
const BYTE MBAG_MAX			= 24;
const BYTE WAREHOUSE_MAX	= 196;
const int ITEMCOUNT_MAX		= 999;

/////////////////////////////////////////////////////////////////////////////////
// Structure Define
/////////////////////////////////////////////////////////////////////////////////
struct	_ITEM_DATA
{
	int		nNum;		// item 번호
	short	sDuration;	// item 내구력
	short	sCount;		// item 갯수 or item 축복 속성에 해당 값
	__int64	nSerialNum;	// item serial code
};

struct _ITEM_TABLE
{
	int   m_iNum;				// item num
	BYTE  m_bCountable;			// 개수 개념 아이템
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
	int		m_iGold;
	short	m_sBind;
	int		m_iBank;
	
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


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//
//	Global Function Define
//

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

inline void LogFileWrite( LPCTSTR logstr )
{
	CString ProgPath, LogFileName;
	CFile file;
	int loglength;

	ProgPath = GetProgPath();
	loglength = strlen( logstr );

	LogFileName.Format("%s\\Aujard.log", ProgPath);
	
	file.Open( LogFileName, CFile::modeCreate|CFile::modeNoTruncate|CFile::modeWrite );

	file.SeekToEnd();
	file.Write(logstr, loglength);
	file.Close();
};

inline int DisplayErrorMsg(SQLHANDLE hstmt)
{
	SQLCHAR       SqlState[6], Msg[1024];
	SQLINTEGER    NativeError;
	SQLSMALLINT   i, MsgLen;
	SQLRETURN     rc2;
	char		  logstr[512];
	memset( logstr, NULL, 512 );

	i = 1;
	while ((rc2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, i, SqlState, &NativeError, Msg, sizeof(Msg), &MsgLen)) != SQL_NO_DATA)
	{
		sprintf_s( logstr, sizeof(logstr), "*** %s, %d, %s, %d ***\r\n", SqlState,NativeError,Msg,MsgLen );
		LogFileWrite( logstr );

		i++;
	}

	if( strcmp((char *)SqlState, "08S01") == 0 )
		return -1;
	else
		return 0;
};

#endif