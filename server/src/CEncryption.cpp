// GraalReborn Server
// (C) GraalReborn 2008

#include "IDebug.h"
#include "CString.h"
#include "CEncryption.h"

const uint32_t CEncryption::ITERATOR_START[6] = {0, 0, 0x04A80B38, 0x4A80B38, 0x4A80B38, 0};

CEncryption::CEncryption()
: m_key(0), m_limit(-1), m_gen(ENCRYPT_GEN_3)
{
	m_iterator = ITERATOR_START[m_gen];
}

void CEncryption::reset(uint8_t key) {
	m_key = key;
	m_iterator = ITERATOR_START[m_gen];
	m_limit = -1;
}

void CEncryption::decrypt(CString& pBuf) {
	// If we don't have anything, just return.
	if (pBuf.isEmpty()) return;

	// Apply the correct decryption algorithm.
	switch (m_gen)
	{
		// No encryption.
		case ENCRYPT_GEN_1:
		case ENCRYPT_GEN_2:
			return;

		// Single byte insertion/zlib compression.
		case ENCRYPT_GEN_3:
		{
			m_iterator *= 0x8088405;
			m_iterator += m_key;
			int pos = ((m_iterator & 0x0FFFF) % pBuf.length());
			pBuf.removeI(pos, 1);
		}
		break;

		// Partial packet encryption/none, zlib, bz2 compression methods.
		// Gen 4 is only bz2.
		case ENCRYPT_GEN_4:
		case ENCRYPT_GEN_5:
		{
			const uint8_t* iterator = reinterpret_cast<const uint8_t*>(&m_iterator);

			for (int32_t i = 0; i < pBuf.length(); ++i)
			{
				if (i % 4 == 0)
				{
					if (m_limit == 0) return;
					m_iterator *= 0x8088405;
					m_iterator += m_key;
					if (m_limit > 0) m_limit--;
				}

				pBuf[i] ^= iterator[i%4];
			}
		}
		break;

		// Future encryption method.
		case ENCRYPT_GEN_6:
			return;
	}
}

CString CEncryption::encrypt(CString pBuf)
{
	// If we don't have anything, just return.
	if (pBuf.isEmpty()) return pBuf;

	switch (m_gen)
	{
		// No encryption.
		case ENCRYPT_GEN_1:
		case ENCRYPT_GEN_2:
			break;

		// Single byte insertion.
		case ENCRYPT_GEN_3:
		{
			m_iterator *= 0x8088405;
			m_iterator += m_key;
			int pos = ((m_iterator & 0x0FFFF) % pBuf.length());
			return CString() << pBuf.subString(0, pos) << ")" << pBuf.subString(pos);
			break;
		}

		// Partial packet encryption/none, zlib, bz2 compression methods.
		// Gen 4 is only bz2.
		case ENCRYPT_GEN_4:
		case ENCRYPT_GEN_5:
		{
			const uint8_t* iterator = reinterpret_cast<const uint8_t*>(&m_iterator);

			for (int32_t i = 0; i < pBuf.length(); ++i)
			{
				if (i % 4 == 0)
				{
					if (m_limit == 0) return pBuf;
					m_iterator *= 0x8088405;
					m_iterator += m_key;
					if (m_limit > 0) m_limit--;
				}

				pBuf[i] ^= iterator[i%4];
			}
			return pBuf;
			break;
		}
	}
	return pBuf;
}

void CEncryption::limit(int32_t limit)
{
	m_limit = limit;
}

int CEncryption::limitFromType(uint8_t type)
{
	// { type, limit, type2, limit2, ... }
	static int limits[] = { 0x02, 0x0C, 0x04, 0x04, 0x06, 0x04 };
	for (unsigned int i = 0; i < sizeof(limits) / sizeof(int); i+=2)
	{
		// If we found a valid type, set the limit.
		if (limits[i] == type)
		{
			m_limit = (int32_t)limits[i+1];
			return 0;
		}
	}

	// Error.
	return 1;
}
