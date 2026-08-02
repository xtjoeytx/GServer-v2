#ifndef MAP_H
#define MAP_H

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <filesystem/File.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/std/generator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

struct is_bigmap_t { explicit is_bigmap_t() = default; };
inline constexpr is_bigmap_t is_bigmap{};

struct is_gmap_t { explicit is_gmap_t() = default; };
inline constexpr is_gmap_t is_gmap{};

enum class MapType
{
	BIGMAP = 0,
	GMAP = 1,
};

class Server;
struct StaticLevelData;

struct MapTerrain
{
	/// @brief Seed for the whole map.
	uint32_t mapSeed = 0;

	/// @brief The base terrain height for the map.
	///
	/// Not used outside of initial map generation.
	/// If evenBorders is false, the four corners of the map are set to the base height.
	/// If evenBorders is true, the whole outline of the map is set to the base height.
	double heightBase = 0;

	/// @brief If true, the whole outline of the map is set to the base height.
	///
	/// Not used outside of initial map generation.
	bool evenBorders = false;

	/// @brief Possible height variation.
	double heightDeviation = 65.0;

	/// @brief Dampening multiplier for the height value as the vertices get generated.
	double mapChaos = 0.6;

	/// @brief Base level height.
	///
	/// Originally calculated with: pow(mapChaos, (width / 2.0)) * mapTerrainHeight
	double levelHeightDeviation = 4.0;

	/// @brief Dampening multiplier for the height value as the level vertices get generated.
	double levelChaos = 0.6;

	/// @brief A vector containing seed values for levels.
	std::vector<uint32_t> levelSeeds;

	std::vector<double> gridBorderTileHeightsXAxis;
	std::vector<double> gridBorderTileHeightsYAxis;
};

class Map
{
public:
	Map(is_bigmap_t, const std::filesystem::path& fileName);
	Map(is_gmap_t, const std::filesystem::path& fileName);

public:
	void loadMapLevels() const;
	void setLevelDataLoaded(std::shared_ptr<StaticLevelData> level);

public:
	bool hasLevel(std::string_view levelName) const;
	std::optional<MapPosition> getLevelPosition(std::string_view levelName) const;
	std::string getLevelNameAt(int x, int y) const;
	std::shared_ptr<StaticLevelData> getLevelDataAt(int x, int y) const;
	std::shared_ptr<StaticLevelData> getLevelDataAt(const PixelPosition& globalPosition) const;
	std::generator<std::pair<std::shared_ptr<StaticLevelData>, MapPosition>> getLevelDataInRange(const TilePosition& position, int syncTilesX, int syncTilesY) const noexcept;
	std::generator<std::pair<std::shared_ptr<StaticLevelData>, MapPosition>> getLevelDataInRectangle(const PixelRectangleArea& area) const noexcept;
	std::generator<std::pair<std::shared_ptr<StaticLevelData>, MapPosition>> getAllLevelData() const noexcept;

public:
	[[inline]] std::string getMapName() const noexcept;
	[[inline]] bool isGmap() const noexcept;
	[[inline]] bool isBigMap() const noexcept;
	[[inline]] bool hasTerrain() const noexcept;

public:
	const MapType mapType;
	const std::filesystem::path fileName;
	const std::string mapImage;
	const std::string miniMapImage;
	const Dimension<uint8_t> size;
	const bool keepAllLevelsLoaded = false;
	const string_map<MapPosition> levels;
	const string_set levelsToKeepInMemory;
	const MapTerrain terrain;

private:
	void forceSetLevelDataLoaded(std::shared_ptr<StaticLevelData> level) const noexcept;
	std::shared_ptr<StaticLevelData> getLevelDataPtr(std::string_view levelName, std::weak_ptr<StaticLevelData> levelPtr) const noexcept;

private:
	Server* m_server;
	mutable std::vector<std::weak_ptr<StaticLevelData>> levelDataByPosition;
	mutable string_map<std::weak_ptr<StaticLevelData>> levelDataByName;
};

//----------------------------

inline std::string Map::getMapName() const noexcept
{
	return fs::getANSIFileName(fileName);
}

inline bool Map::isGmap() const noexcept
{
	return mapType == MapType::GMAP;
}

inline bool Map::isBigMap() const noexcept
{
	return mapType == MapType::BIGMAP;
}

inline bool Map::hasTerrain() const noexcept
{
	return !terrain.gridBorderTileHeightsXAxis.empty();
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // MAP_H
