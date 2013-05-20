#pragma once

class CNpc;
struct _MAGIC_TABLE;

class CNpcMagicProcess  
{
public:
	CNpc*			m_pSrcNpc;	

	BYTE	m_bMagicState;

public:
	CNpcMagicProcess();
	virtual ~CNpcMagicProcess();

	short GetMagicDamage(int tid, int total_hit, int attribute, int dexpoint);
	void ExecuteType3(int magicid, int tid, int data1, int data2, int data3, int moral);
	void ExecuteType2(int magicid, int tid, int data1, int data2, int data3 );
	void ExecuteType1(int magicid, int tid, int data1, int data2, int data3 );	// sequence => type1 or type2

	_MAGIC_TABLE* IsAvailable( int magicid, int tid, BYTE type );
	void MagicPacket(Packet & pkt);
};
