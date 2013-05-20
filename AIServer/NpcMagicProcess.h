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

	_MAGIC_TABLE* IsAvailable( int magicid, int tid, BYTE type );
	void MagicPacket(uint8 opcode, uint32 nSkillID, int16 sCasterID, int16 sTargetID, 
		int16 sData1 = 0, int16 sData2 = 0, int16 sData3 = 0);
};
