#ifndef GS2EMU_TLEVELTILES_H
#define GS2EMU_TLEVELTILES_H

#include <cstring>

class TLevelTiles
{
public:
	TLevelTiles(short fillTile = 0x00)
	{
		memset(levelTiles, fillTile, sizeof(levelTiles));
	}

	short& operator[](uint32_t index) { return levelTiles[index]; }
	explicit operator char*() const { return (char*)levelTiles; };

private:
	short levelTiles[4096];
};

#endif //GS2EMU_TLEVELTILES_H
