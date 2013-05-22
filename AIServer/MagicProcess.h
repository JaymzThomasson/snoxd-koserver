#pragma once

class CUser;
class CNpc;
class Packet;

struct _MAGIC_TABLE;
struct _MAGIC_TYPE4;

class CMagicProcess  
{
public:
	CUser*			m_pSrcUser;	

	uint8	m_bMagicState;
public:
	CMagicProcess();
	virtual ~CMagicProcess();

	short GetWeatherDamage(short damage, short attribute);
	void ExecuteType4(int magicid, int sid, int tid, int data1, int data2, int data3, int moral );
	void ExecuteType3(int magicid, int tid, int data1, int data2, int data3, int moral, int dexpoint, int righthand_damage);
	uint8 ExecuteType2(int magicid, int tid, int data1, int data2, int data3);
	uint8 ExecuteType1(int magicid, int tid, int data1, int data2, int data3, uint8 sequence );	// sequence => type1 or type2
	short GetMagicDamage(int tid, int total_hit, int attribute, int dexpoint, int righthand_damage);
	short AreaAttack(int magictype, int magicid, int moral, int data1, int data2, int data3, int dexpoint, int righthand_damage);
	void  AreaAttackDamage(int magictype, int rx, int rz, int magicid, int moral, int data1, int data2, int data3, int dexpoint, int righthand_damage);

	_MAGIC_TABLE* IsAvailable( int magicid, int tid, uint8 type );
	void MagicPacket(Packet & pkt);
};