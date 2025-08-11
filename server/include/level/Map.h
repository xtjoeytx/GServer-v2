#ifndef MAP_H
#define MAP_H

#include <cstdint>
#include <filesystem>
#include <generator>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <vector>

#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>

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

class Level;

class Map
{
public:
	Map(is_bigmap_t, const std::filesystem::path& fileName);
	Map(is_gmap_t, const std::filesystem::path& fileName);

public:
	void loadMapLevels() const;

public:
	bool hasLevel(std::string_view levelName) const;
	std::optional<Position<uint8_t>> getLevelPosition(std::string_view levelName) const;
	std::shared_ptr<Level> getLevelAt(int x, int y) const;
	std::shared_ptr<Level> getLevelAt(const PixelPosition& globalPosition) const;
	std::generator<std::shared_ptr<Level>> getLevelsInRange(const TilePosition& position, int syncx, int syncy) const noexcept;
	std::generator<std::shared_ptr<Level>> getAllLevels() const noexcept;

public:
	[[inline]] std::string getMapName() const noexcept;
	[[inline]] bool isGmap() const noexcept;
	[[inline]] bool isBigMap() const noexcept;

public:
	const MapType mapType;
	const std::filesystem::path fileName;
	const std::string mapImage;
	const std::string miniMapImage;
	const Dimension<uint8_t> size;
	const bool keepAllLevelsLoaded = false;
	const string_map<Position<uint8_t>> levels;
	const string_set levelsToKeepInMemory;

private:
	std::shared_ptr<Level> getLevelPtr(std::string_view levelName, std::weak_ptr<Level> levelPtr) const;
	mutable std::vector<std::weak_ptr<Level>> levelsByPosition;
	mutable string_map<std::weak_ptr<Level>> levelsByName;
};

//----------------------------

inline std::string Map::getMapName() const noexcept
{
	return fileName.filename().string();
}

inline bool Map::isGmap() const noexcept
{
	return mapType == MapType::GMAP;
}

inline bool Map::isBigMap() const noexcept
{
	return mapType == MapType::BIGMAP;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // MAP_H
