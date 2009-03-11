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

#include "CString.h"

enum
{
	ENCRYPT_GEN_1		= 0,	// No encryption/no compression.
	ENCRYPT_GEN_2		= 1,	// No encryption/zlib compression.
	ENCRYPT_GEN_3		= 2,	// Single byte insertion/zlib compression.
	ENCRYPT_GEN_4		= 3,	// Partial packet encryption/bz2 compression.
	ENCRYPT_GEN_5		= 4,	// Partial packet encryption/none, zlib, bz2 compression methods.
	ENCRYPT_GEN_6		= 5,	// Unknown (Graal v6 encryption).
};

enum
{
	COMPRESS_UNCOMPRESSED	= 0x02,
	COMPRESS_ZLIB			= 0x04,
	COMPRESS_BZ2			= 0x06,
};

class codec {
	public:
		codec();

		static const uint32_t ITERATOR_START[6];
		void reset(uint8_t key);
		void decrypt(CString& pBuf);
		CString encrypt(CString pBuf);
		void limit(int32_t limit);
		int limitFromType(uint8_t type);
		void setGen(uint32_t gen)		{ m_gen = gen; if (m_gen > 5) m_gen = 5; }
		uint32_t getGen()				{ return m_gen; }

	private:
		uint8_t		m_key;
		uint8_t		m_offset;
		uint32_t	m_iterator;
		int32_t		m_limit;
		uint32_t	m_gen;
}; 
 
#endif
