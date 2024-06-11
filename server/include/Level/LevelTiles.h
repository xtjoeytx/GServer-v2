#ifndef GS2EMU_TLEVELTILES_H
#define GS2EMU_TLEVELTILES_H

#include <cstring>

class LevelTiles
{
public:
	LevelTiles(short fillTile = 0x00)
	{
		memset(m_tiles, fillTile, sizeof(m_tiles));
	}

	short& operator[](uint32_t index) { return m_tiles[index]; }

	explicit operator char*() const { return (char*)m_tiles; };

private:
	short m_tiles[4096];
};

#endif //GS2EMU_TLEVELTILES_H
