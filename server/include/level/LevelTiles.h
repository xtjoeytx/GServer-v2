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
	LevelTiles(short fillTile = 0x00)
	{
		memset(m_tiles, fillTile, sizeof(m_tiles));
	}

	short& operator[](uint32_t index) { return m_tiles[index]; }
	explicit operator char* () const { return (char*)m_tiles; };

private:
	short m_tiles[4096];
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // LEVELTILES_H
