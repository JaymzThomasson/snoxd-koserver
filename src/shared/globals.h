#pragma once

#include "version.h"
#include "packets.h"
#include "Packet.h"

#define MAX_USER			3000

#define MAX_ID_SIZE			20
#if __VERSION >= 1453
#define MAX_PW_SIZE			28
#else
#define MAX_PW_SIZE			12
#endif

#define MAX_ITEM_COUNT		9999

#define VIEW_DISTANCE		48

// Define a second as 1000ms.
#define SECOND				1000u

enum NameType
{
	TYPE_ACCOUNT,
	TYPE_CHARACTER
};

enum Nation
{
	ALL = 0,
	KARUS,
	ELMORAD,
	NONE
};

// ITEM_SLOT DEFINE
const uint8 RIGHTEAR			= 0;
const uint8 HEAD				= 1;
const uint8 LEFTEAR				= 2;
const uint8 NECK				= 3;
const uint8 BREAST				= 4;
const uint8 SHOULDER			= 5;
const uint8 RIGHTHAND			= 6;
const uint8 WAIST				= 7;
const uint8 LEFTHAND			= 8;
const uint8 RIGHTRING			= 9;
const uint8 LEG					= 10;
const uint8 LEFTRING			= 11;
const uint8 GLOVE				= 12;
const uint8 FOOT				= 13;
const uint8 RESERVED			= 14;

const uint8 CWING				= 42;
const uint8 CHELMET				= 43;
const uint8 CLEFT				= 44;
const uint8 CRIGHT				= 45;
const uint8 CTOP				= 46;
const uint8 BAG1				= 47;
const uint8 BAG2				= 48;

const uint8 SLOT_MAX			= 14; // 14 equipped item slots
const uint8 HAVE_MAX			= 28; // 28 inventory slots
const uint8 COSP_MAX			= 5; // 5 cospre slots
const uint8 MBAG_COUNT			= 2; // 2 magic bag slots
const uint8 MBAG_MAX			= 12; // 12 slots per magic bag

// Total number of magic bag slots
#define MBAG_TOTAL			(MBAG_MAX * MBAG_COUNT)

// Start of inventory area
#define INVENTORY_INVENT	(SLOT_MAX)

// Start of cospre area
#define INVENTORY_COSP		(SLOT_MAX+HAVE_MAX)

// Start of magic bag slots (after the slots for the bags themselves)
#define INVENTORY_MBAG		(SLOT_MAX+HAVE_MAX+COSP_MAX+MBAG_COUNT)

// Start of magic bag 1 slots (after the slots for the bags themselves)
#define INVENTORY_MBAG1		(INVENTORY_MBAG)

// Start of magic bag 2 slots (after the slots for the bags themselves)
#define INVENTORY_MBAG2		(INVENTORY_MBAG+MBAG_MAX)

// Total slots in the general-purpose inventory storage
#define INVENTORY_TOTAL		(INVENTORY_MBAG2+MBAG_MAX)

const uint8 WAREHOUSE_MAX	= 192;
const uint8 MAX_MERCH_ITEMS	= 12;

#define MAX_MERCH_MESSAGE	40

const int ITEMCOUNT_MAX		= 9999;

#define MAX_KNIGHTS_MARK	2400
#define CLAN_SYMBOL_COST	5000000

#define NEWCHAR_SUCCESS						uint8(0)
#define NEWCHAR_NO_MORE						uint8(1)
#define NEWCHAR_INVALID_DETAILS				uint8(2)
#define NEWCHAR_EXISTS						uint8(3)
#define NEWCHAR_DB_ERROR					uint8(4)
#define NEWCHAR_INVALID_NAME				uint8(5)
#define NEWCHAR_BAD_NAME					uint8(6)
#define NEWCHAR_INVALID_RACE				uint8(7)
#define NEWCHAR_NOT_SUPPORTED_RACE			uint8(8)
#define NEWCHAR_INVALID_CLASS				uint8(9)
#define NEWCHAR_POINTS_REMAINING			uint8(10)
#define NEWCHAR_STAT_TOO_LOW				uint8(11)

enum ItemFlag
{
	ITEM_FLAG_NONE		= 0,
	ITEM_FLAG_RENTED	= 1,
	ITEM_FLAG_SEALED	= 4,
	ITEM_FLAG_NOT_BOUND	= 7,
	ITEM_FLAG_BOUND		= 8
};

struct	_ITEM_DATA
{
	uint32		nNum;
	int16		sDuration;
	uint16		sCount;	
	uint8		bFlag; // see ItemFlag
	uint16		sRemainingRentalTime; // in minutes
	uint32		nExpirationTime; // in unix time
	uint64		nSerialNum;

	INLINE bool isSealed() { return bFlag == ITEM_FLAG_SEALED; }
	INLINE bool isRented() { return bFlag == ITEM_FLAG_RENTED; }
};

enum HairData
{
	HAIR_R,
	HAIR_G,
	HAIR_B,
	HAIR_TYPE
};

struct _MERCH_DATA
{
	uint32 nNum;
	int16 sDuration;
	uint16 sCount;
	uint64 nSerialNum;
	uint32 nPrice;
	uint8 bOriginalSlot;
};

enum StatType
{
	STAT_STR = 0,
	STAT_STA = 1,
	STAT_DEX = 2,
	STAT_INT = 3, 
	STAT_CHA = 4, // MP
	STAT_COUNT
};

#define STAT_MAX 255
#define QUEST_ARRAY_SIZE 600 // That's a limit of 200 quests (3 bytes per quest)

INLINE int myrand( int min, int max )
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

INLINE bool CheckPercent(short percent)
{
	if (percent < 0 || percent > 1000) 
		return false;

	return (percent > myrand(0, 1000));
}

INLINE time_t getMSTime()
{
#ifdef _WIN32
#if WINVER >= 0x0600
	typedef ULONGLONG (WINAPI *GetTickCount64_t)(void);
	static GetTickCount64_t pGetTickCount64 = nullptr;

	if (!pGetTickCount64)
	{
		HMODULE hModule = LoadLibraryA("KERNEL32.DLL");
		pGetTickCount64 = (GetTickCount64_t)GetProcAddress(hModule, "GetTickCount64");
		if (!pGetTickCount64)
			pGetTickCount64 = (GetTickCount64_t)GetTickCount;
		FreeLibrary(hModule);
	}

	return pGetTickCount64();
#else
	return GetTickCount();
#endif
#else
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return (tv.tv_sec * SECOND) + (tv.tv_usec / SECOND);
#endif
}

INLINE void STRTOLOWER(std::string& str)
{
	for(size_t i = 0; i < str.length(); ++i)
		str[i] = (char)tolower(str[i]);
};

INLINE void STRTOUPPER(std::string& str)
{
	for(size_t i = 0; i < str.length(); ++i)
		str[i] = (char)toupper(str[i]);
};

#define foreach(itr, arr) for (auto itr = arr.begin(); itr != arr.end(); itr++)
#define foreach_stlmap(itr, arr) for (auto itr = arr.m_UserTypeMap.begin(); itr != arr.m_UserTypeMap.end(); itr++)
#define foreach_array(itr, arr) foreach_array_n(itr, arr, sizeof(arr) / sizeof(arr[0]))
#define foreach_array_n(itr, arr, len) for (auto itr = 0; itr < len; itr++)
#define foreach_region(x, z) for (int x = -1; x <= 1; x++) \
	for (int z = -1; z <= 1; z++)

