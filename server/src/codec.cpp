// GraalReborn Server
// (C) GraalReborn 2008

#include "CString.h"
#include "codec.h"

const uint32_t codec::ITERATOR_START[5] = {0, 0, 0x04A80B38, 0x4A80B38, 0};

codec::codec()
: m_key(0), m_offset(0), m_limit(-1), m_gen(ENCRYPT_GEN_3)
{
	m_iterator = ITERATOR_START[m_gen];
}

void codec::reset(uint8_t key) {
	m_key = key;
	m_offset = 0;
	m_iterator = ITERATOR_START[m_gen];
	m_limit = -1;
}

void codec::apply(CString& pBuf) {
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
			int pos  = ((m_iterator & 0x0FFFF) % pBuf.length());
			pBuf.removeI(pos, 1);
		}
		break;

		// Partial packet encryption/none, zlib, bz2 compression methods.
		case ENCRYPT_GEN_4:
		{
			const uint8_t* iterator = reinterpret_cast<const uint8_t*>(&m_iterator);
		 
			for (int32_t i = 0; i < pBuf.length(); ++i) {
				const uint32_t i_ = i + m_offset;
				if (i_ % 4 == 0) {
					if (m_limit == 0) return;
					m_iterator *= 0x8088405;
					m_iterator += m_key;
					m_offset = 0;
					if (m_limit > 0) m_limit--;
				}
		 
				pBuf[i] ^= iterator[i_%4];
			}
		}
		break;

		// Future encryption method.
		case ENCRYPT_GEN_5:
			return;
	}
}

void codec::limit(int32_t limit)
{
	m_limit = limit;
}

int codec::limitFromType(uint8_t type)
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
