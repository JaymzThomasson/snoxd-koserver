#include "JvCryption.h"
#include "../shared/crc32.h"

extern T_KEY g_private_key;

CJvCryption::CJvCryption()
{
	m_public_key = 0;
	m_private_key = g_private_key;
}

void CJvCryption::SetPublicKey(T_KEY pk)
{
	m_public_key = pk;
}

void CJvCryption::SetPrivateKey(T_KEY pk)
{
	m_private_key = pk;
}

void CJvCryption::Init()
{
	m_tkey = m_public_key ^ m_private_key;
}

void CJvCryption::JvEncryptionFast(int len, T_OCTET *datain, T_OCTET *dataout)
{
	T_OCTET *pkey, lkey, rsk;
	int rkey = 2157;

	pkey = (T_OCTET *)&m_tkey;
	lkey = (len * 157) & 0xff;

	for (int i = 0; i < len; i++)
	{
		rsk = (rkey >> 8) & 0xff;
		dataout[i] = ((datain[i] ^ rsk) ^ pkey[(i % 8)]) ^ lkey;
		rkey *= 2171;
	}
}

int CJvCryption::JvDecryptionWithCRC32(int len, T_OCTET *datain, T_OCTET *dataout)
{
	int result;
	JvDecryptionFast(len, datain, dataout);
	if (crc32(dataout, len - 4, -1) == *(unsigned long*)(len - 4 + dataout))
		result = len - 4;
	else
		result = -1;

	return result;
}

CJvCryption::~CJvCryption()
{
}