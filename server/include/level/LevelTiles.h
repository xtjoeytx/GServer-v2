#ifndef LEVELTILES_H
#define LEVELTILES_H

#include <array>
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
		m_tiles.fill(fillTile);
	}

	uint16_t& operator[](size_t index) { return m_tiles[index]; }
	explicit operator char* () const { return (char*)m_tiles.data(); };

	const auto& tiles() const { return m_tiles; }

private:
	std::array<uint16_t, 4096> m_tiles;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELTILES_H
