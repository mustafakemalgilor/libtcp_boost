#pragma once

#define USE_CRYPTION
#include "crc32.h"

class CJvCryption
{
private:
	unsigned long long m_public_key, m_tkey;

public:
	CJvCryption() : m_public_key(0) {}

	__inline unsigned long long GetPublicKey() { return m_public_key; }
	unsigned long long GenerateKey();

	void Init();

	void JvEncryptionFast(int len, unsigned char *datain, unsigned char *dataout);
	void JvEncryptionFastBetweenIndex(int len, int start, int end, unsigned char * datain, unsigned char * dataout);
	__inline void JvDecryptionFast(int len, unsigned char *datain, unsigned char *dataout) { JvEncryptionFast(len, datain, dataout); };

	int JvDecryptionWithCRC32(int len, unsigned char *datain, unsigned char *dataout);
};
