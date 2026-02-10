#ifndef LEVELTILES_H
#define LEVELTILES_H

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <span>

#include <CString.h>

#include <utilities/Extents.h>
#include <utilities/std/generator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

namespace constants
{
inline constexpr uint16_t EmptyTileInBase = 0x0;
inline constexpr uint16_t EmptyTileInLayer = 0xFFFF;
inline constexpr uint16_t BasicGrassTilePics1 = 511;

inline constexpr Dimension<uint8_t> LevelPartSizeInTiles = Dimension<uint8_t>{ 64, 64 };
inline constexpr size_t LevelPartTileCount = static_cast<size_t>(LevelPartSizeInTiles.width()) * static_cast<size_t>(LevelPartSizeInTiles.height());
} // end namespace constants

/// @brief Gets the tile index at a specific local whole tile position.
/// @param position The local whole tile position.
/// @return The tile index.
[[nodiscard]] inline constexpr size_t GetTileIndexAtPosition(const LocalWholeTilePosition& position) noexcept
{
	return static_cast<size_t>(position.y()) * 64 + static_cast<size_t>(position.x());
}

/// @brief Contains the tile data for a specific part of a level on a map.
class LevelTiles
{
public:
	using TileArray = std::array<uint16_t, constants::LevelPartTileCount>;

public:
	[[inline]] void reset();

public:
	[[inline]] std::generator<uint8_t> getUsedTileLayers() const noexcept;
	[[inline]] TileArray* getOrCreateLayer(uint8_t layer) noexcept;
	[[inline]] std::optional<TileArray*> getLayer(uint8_t layer) noexcept;
	[[inline]] std::optional<const TileArray*> getLayer(uint8_t layer) const noexcept;

public:
	[[inline]] void writeTiles(const LocalWholeTilePosition& position, uint8_t width, std::span<uint16_t> sourceTiles, uint8_t layer = 0) noexcept;

public:
	[[inline]] void writeLayerToPacket(uint8_t layer, CString& packet) const noexcept;
	[[inline]] void writeLayerToPacket(uint8_t layer, const LocalWholeTilePosition& position, uint8_t width, CString& packet) const noexcept;

public:
	uint16_t BaseLayerEmptyTile = constants::EmptyTileInBase;

private:
	std::map<uint8_t, TileArray> m_tiles;
};

//----------------------------

inline void LevelTiles::reset()
{
	m_tiles.clear();
}

inline std::generator<uint8_t> LevelTiles::getUsedTileLayers() const noexcept
{
	for (const auto& [layer, tiles] : m_tiles)
		co_yield layer;
}

inline LevelTiles::TileArray* LevelTiles::getOrCreateLayer(uint8_t layer) noexcept
{
	auto it = m_tiles.find(layer);
	if (it == m_tiles.end())
	{
		m_tiles[layer] = TileArray{};
		m_tiles[layer].fill(layer == 0 ? BaseLayerEmptyTile : constants::EmptyTileInLayer);
		return &m_tiles[layer];
	}
	return &it->second;
}

inline std::optional<LevelTiles::TileArray*> LevelTiles::getLayer(uint8_t layer) noexcept
{
	auto it = m_tiles.find(layer);
	if (it != m_tiles.end())
		return &it->second;
	return std::nullopt;
}

inline std::optional<const LevelTiles::TileArray*> LevelTiles::getLayer(uint8_t layer) const noexcept
{
	auto it = m_tiles.find(layer);
	if (it != m_tiles.end())
		return &it->second;
	return std::nullopt;
}

inline void LevelTiles::writeTiles(const LocalWholeTilePosition& position, uint8_t width, std::span<uint16_t> sourceTiles, uint8_t layer) noexcept
{
	auto it = m_tiles.find(layer);
	if (it == m_tiles.end())
	{
		m_tiles[layer] = TileArray{};
		m_tiles[layer].fill(layer == 0 ? BaseLayerEmptyTile : constants::EmptyTileInLayer);
	}

	auto& tiles = m_tiles[layer];
	size_t startIndex = GetTileIndexAtPosition(position);
	for (size_t y = 0; y < width; ++y)
	{
		for (size_t x = 0; x < width; ++x)
		{
			tiles[startIndex + (y * 64) + x] = sourceTiles[y * width + x];
		}
	}
}

inline void LevelTiles::writeLayerToPacket(uint8_t layer, CString& packet) const noexcept
{
	auto it = m_tiles.find(layer);
	if (it == m_tiles.end())
	{
		for (int i = 0; i < 4096; ++i)
			packet.writeShort(layer == 0 ? BaseLayerEmptyTile : constants::EmptyTileInLayer);
		return;
	}

	const TileArray& tiles = it->second;
	packet.write(reinterpret_cast<const char*>(tiles.data()), sizeof(short) * 4096);
}

inline void LevelTiles::writeLayerToPacket(uint8_t layer, const LocalWholeTilePosition& position, uint8_t width, CString& packet) const noexcept
{
	auto it = m_tiles.find(layer);
	if (it == m_tiles.end())
	{
		for (size_t y = 0; y < width; ++y)
		{
			for (size_t x = 0; x < width; ++x)
				packet.writeShort(layer == 0 ? BaseLayerEmptyTile : constants::EmptyTileInLayer);
		}
		return;
	}

	const TileArray& tiles = it->second;
	size_t startIndex = GetTileIndexAtPosition(position);
	for (size_t y = 0; y < width; ++y)
	{
		for (size_t x = 0; x < width; ++x)
			packet.writeShort(tiles[startIndex + (y * 64) + x]);
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELTILES_H
