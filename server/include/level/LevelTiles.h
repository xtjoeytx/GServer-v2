#ifndef LEVELTILES_H
#define LEVELTILES_H

#include <cstdint>
#include <cstring>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class LevelTiles
{
public:
	LevelTiles(uint16_t fillTile = 0x00)
	{
		memset(m_tiles, fillTile, sizeof(m_tiles));
	}

	uint16_t& operator[](size_t index) { return m_tiles[index]; }
	explicit operator char* () const { return (char*)m_tiles; };

private:
	uint16_t m_tiles[4096];
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELTILES_H
