#ifndef LEVELTILES_H
#define LEVELTILES_H

#include <array>
#include <cstdint>
#include <cstring>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

namespace constants
{
inline constexpr uint16_t EmptyTileInBase = 0x0;
inline constexpr uint16_t EmptyTileInLayer = 0xFFFF;
inline constexpr uint16_t BasicGrassTilePics1 = 511;
} // end namespace constants

class LevelTiles
{
public:
	LevelTiles(uint16_t fillTile = constants::EmptyTileInBase)
	{
		m_tiles.fill(fillTile);
	}

	uint16_t& operator[](size_t index) { return m_tiles[index]; }
	const uint16_t& operator[](size_t index) const { return m_tiles[index]; }

	explicit operator char* () const { return (char*)m_tiles.data(); };
	const auto& tiles() const { return m_tiles; }

private:
	std::array<uint16_t, 4096> m_tiles;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELTILES_H
