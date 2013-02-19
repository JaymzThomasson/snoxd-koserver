///////////////////////////////////////////////////////////////////////////////
/* ENCYPTION MAIN 
/* AND ITS API                                */
/* THIS PROGRAM IS PROGRAMMED FOR WIZGATE     */
/* BUT ALL COPYRIGHT RESERVED BY JIYOON CHUNG */
/* (c) ABLEX AND OHSUNGINC 2001.6.1           */
/* Classed by ks2000 2001.6.1				  */	
#ifndef __CJVCRYPTION_H
#define __CJVCRYPTION_H

typedef unsigned char 	T_OCTET;
typedef _int64			T_KEY;
#include "crc32.h"

class CJvCryption
{
private:
	T_KEY m_public_key, m_tkey;

public:
	CJvCryption();
	~CJvCryption();

	__forceinline T_KEY GetPublicKey() { return m_public_key; }

	void SetPublicKey(T_KEY pk);

	void Init();

	void JvEncryptionFast( int len, T_OCTET *datain, T_OCTET *dataout );
	__forceinline void JvDecryptionFast( int len, T_OCTET *datain, T_OCTET *dataout ) { JvEncryptionFast(len, datain, dataout); };
	
	int JvDecryptionWithCRC32(int len, T_OCTET *datain, T_OCTET *dataout);
};

#endif
