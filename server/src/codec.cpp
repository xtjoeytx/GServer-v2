// GraalReborn Server
// (C) GraalReborn 2008

#include "codec.h"

void codec::reset(uint8_t key) {
	m_key = key;
	m_offset = 0;
	m_iterator = ITERATOR_START_VAL;
	m_limit = -1;
}

void codec::apply(uint8_t* buf, size_t len) {
	const uint8_t* iterator = reinterpret_cast<const uint8_t*>(
		&m_iterator);
 
	for (size_t i = 0; i < len; ++i) {
		const size_t i_ = i + m_offset;
		if (i_ % 4 == 0) {
			if (m_limit == 0) return;
			m_iterator *= 0x8088405;
			m_iterator += m_key;
			m_offset = 0;
			if (m_limit > 0) m_limit--;
		}
 
		buf[i] ^= iterator[i_%4];
	}
}

void codec::limit(int32_t limit)
{
	m_limit = limit;
}

int codec::limitfromtype(uint8_t type)
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
