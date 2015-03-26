#include "../../Headers/stdafx.h"
#include "JvCryption.h"

// Cryption
/*#if __VERSION >= 1700
#define g_private_key 0x1207500120128966
#elif __VERSION == 1298
#define g_private_key 0x1234567890123456
#else
#define g_private_key 0x1257091582190465
#endif*/

#define g_private_key 0x7467148522014785
#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
void CJvCryption::Init() { m_tkey = m_public_key ^ g_private_key; }

boost::mt19937 gen;

unsigned long long CJvCryption::GenerateKey()
{
#ifdef USE_CRYPTION
	// because of their sucky encryption method, 0 means it effectively won't be encrypted. 
	// We don't want that happening...
	do
	{
		boost::uniform_int<unsigned long long> uInt64Dist(0, std::numeric_limits<uint64_t>::max());
		boost::variate_generator<boost::mt19937&, boost::uniform_int<unsigned long long> > getRand(gen, uInt64Dist);
		m_public_key = getRand();
	} while (!m_public_key);
#endif

	//TRACE("Generated public key for session : (%X)", &m_public_key);
	return m_public_key;
}

void CJvCryption::JvEncryptionFast(int len, unsigned char *datain, unsigned char *dataout)
{
	unsigned char *pkey, lkey, rsk;
	int rkey = 2157;

	pkey = (unsigned char *)&m_tkey;
	lkey = (len * 157) & 0xff;

	for (int i = 0; i < len; i++)
	{
		rsk = (rkey >> 8) & 0xff;
		dataout[i] = ((datain[i] ^ rsk) ^ pkey[(i % 8)]) ^ lkey;
		rkey *= 2171;
	}
}

void CJvCryption::JvEncryptionFastBetweenIndex(int len, int start, int end, unsigned char *datain, unsigned char *dataout)
{
	unsigned char *pkey, lkey, rsk;
	int rkey = 2157;

	pkey = (unsigned char *)&m_tkey;
	lkey = (len * 157) & 0xff;

	for (int i = start; i < start + len; i++)
	{
		rsk = (rkey >> 8) & 0xff;
		dataout[i] = ((datain[i] ^ rsk) ^ pkey[((i - start) % 8)]) ^ lkey;
		rkey *= 2171;
	}
}

int CJvCryption::JvDecryptionWithCRC32(int len, unsigned char *datain, unsigned char *dataout)
{
	int result;
	JvDecryptionFast(len, datain, dataout);

	if (crc32(dataout, len - 4, -1) == *(unsigned __int32 *)(len - 4 + dataout))
		result = len - 4;
	else
		result = -1;

	return result;
}
