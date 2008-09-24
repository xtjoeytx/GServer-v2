// GraalReborn Server
// (C) GraalReborn 2008

#ifndef ENC_CODEC_HPP
#define ENC_CODEC_HPP

#if defined(_MSC_VER)
#define		int32_t		__int32
#define		uint8_t		unsigned __int8
#define		uint32_t	unsigned __int32
#else
#include <stdint.h>
#endif

#include <cstdlib>

enum
{
	ENCRYPT22_UNCOMPRESSED	= 0x02,
	ENCRYPT22_ZLIB			= 0x04,
	ENCRYPT22_BZ2			= 0x06,
};

class codec {
public:
	static const unsigned ITERATOR_START_VAL = 0x4A80B38;
	void reset(uint8_t key);
	void apply(uint8_t* buf, size_t len);
	void limit(int32_t limit);
	int limitfromtype(uint8_t type);
 
private:
	uint8_t  m_key;
	uint8_t  m_offset;
	uint32_t m_iterator;
	int32_t m_limit;
}; 
 
#endif
