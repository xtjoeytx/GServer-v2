#ifndef LEVEL_H
#define LEVEL_H

#include <chrono>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <CString.h>

#include <level/LevelArrow.h>
#include <level/LevelBaddy.h>
#include <level/LevelBoardChange.h>
#include <level/LevelBomb.h>
#include <level/LevelChest.h>
#include <level/LevelExplosion.h>
#include <level/LevelHorse.h>
#include <level/LevelItem.h>
#include <level/LevelLink.h>
#include <level/LevelShoot.h>
#include <level/LevelSign.h>
#include <level/LevelTerrain.h>
#include <level/LevelTiles.h>
#include <level/LevelTileTypes.h>
#include <level/Map.h>
#include <scripting/Script.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Events.h>
#include <utilities/Extents.h>
#include <utilities/std/generator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class LevelLoader;
class NPC;
class Player;
class Server;

//----------------------------

/// @brief Stores the basic data for an NPC in a level.
struct LevelNPCTemplate
{
	std::string image;
	LocalPixelPosition position;
	Script script;
};

/// @brief Stores the static data of a level.
struct StaticLevelData;
struct StaticLevelData
{
	std::string levelName;
	std::filesystem::path filePath;
	clock::time_point modTime;
	LevelTiles tiles;
	std::vector<LevelLink> links;
	std::vector<LevelChest> chests;
	std::vector<LevelSign> signs;
	std::vector<LevelBaddy> baddies;
	std::vector<LevelNPCTemplate> npcs;
	std::vector<double> heights;
	EventDispatcher<std::shared_ptr<StaticLevelData>> onDataRefreshed;
	//
	static void reload(std::shared_ptr<StaticLevelData> staticData);
	//
	std::optional<std::string> getChestFormattedForSave(LevelChest* chest) const;
	void sendBoardToPlayer(std::shared_ptr<Player> player) const;
	void sendBoardLayersToPlayer(std::shared_ptr<Player> player) const;
	void sendBoardLayerToPlayer(std::shared_ptr<Player> player, size_t layer) const;
	void sendChestsToPlayer(std::shared_ptr<Player> player) const;
	void sendLinksToPlayer(std::shared_ptr<Player> player, bool onlyMapLinks) const;
	void sendSignsToPlayer(std::shared_ptr<Player> player) const;
};
using StaticLevelDataPtr = std::shared_ptr<StaticLevelData>;

//----------------------------

/// @brief Stores the data for a specific level tile on a map.
struct SubLevel
{
	std::weak_ptr<StaticLevelData> staticData;
	std::optional<MapPosition> mapPosition;
	std::optional<LevelTiles> instancedTileUpdates;
	std::optional<LevelTiles> scriptUpdatedTiles;
	std::optional<LevelTerrain> terrain;
	std::vector<LevelBaddy> baddies;
	std::vector<LevelBoardChange> boardChanges;
	bool isSparringZone = false;
	bool isNoPkZone = false;
	bool isOnGmap = false;
	bool isOnBigMap = false;
	EventHandle staticDataRefreshedHandle;
	//
	PixelRectangleArea clipRectangleToPart(const PixelRectangleArea& area) const noexcept;
	WholeTileRectangleArea clipRectangleToPart(const WholeTileRectangleArea& area) const noexcept;
	//
	std::optional<LevelTiles*> getTiles() noexcept;
	std::optional<const LevelTiles*> getTiles() const noexcept;
	std::optional<LevelTiles::TileArray*> getTiles(size_t layer) noexcept;
	std::optional<const LevelTiles::TileArray*> getTiles(size_t layer) const noexcept;
	double getHeightAt(const LocalPixelPosition& position) const noexcept;
	void sendBoardToPlayer(std::shared_ptr<Player> player) const;
	void sendBoardLayersToPlayer(std::shared_ptr<Player> player) const;
	void sendBoardLayerToPlayer(std::shared_ptr<Player> player, size_t layer) const;
	void sendBoardHeightsToPlayer(std::shared_ptr<Player> player) const;
	void sendBoardChangesToPlayer(std::shared_ptr<Player> player, std::optional<clock::time_point> time) const;
};
using SubLevelPtr = std::shared_ptr<SubLevel>;

//----------------------------

class Level : public std::enable_shared_from_this<Level>
{
	friend class LevelLoader;

public:
	Level();
	~Level();

public:
	static std::shared_ptr<Level> createLevel(std::string_view levelName = ""sv);
	static std::shared_ptr<Level> clone(LevelPtr level, std::string_view name);

public:
	bool loaded = false;
	bool reload(std::string_view levelName);
	bool reload(const MapPosition& position);
	void reload(StaticLevelDataPtr staticData);
	bool saveLevel(const MapPosition& mapPosition, std::string_view filename);

public:
	void doTimedEvents();
	void doFrameEvents(precise_clock::time_point time);
	const auto& getLastFrameTime() const { return m_lastFrameTime; }
	const auto& getFilePath() const { return m_filePath; }

private:
	precise_clock::time_point m_lastFrameTime = precise_clock::now();
	precise_clock::duration m_frameEventDuration = precise_clock::duration::zero();

public:
	[[inline]] void setMap(std::shared_ptr<Map> map);
	[[inline]] auto getMap() const noexcept;
	bool isGmap() const noexcept;
	[[inline]] bool isOnBigMap() const noexcept;
	[[inline]] static constexpr Dimension<uint8_t> tilesPerSubLevel() noexcept;
	[[inline]] static constexpr Dimension<uint8_t> pixelsPerTile() noexcept;
	[[inline]] static constexpr Dimension<uint16_t> pixelsPerSubLevel() noexcept;
	[[inline]] Dimension<uint8_t> sizeInSubLevels() const noexcept;
	[[inline]] Dimension<uint32_t> sizeInTiles() const noexcept;
	[[inline]] Dimension<uint32_t> sizeInPixels() const noexcept;
	[[inline]] Rectangle<uint32_t, uint32_t> getBoundingBox() const noexcept;
	uint16_t* getMapTileForEditing(const TilePosition& position) noexcept;
	[[inline]] std::string_view getLevelNameAtPosition(const PixelPosition& position) const noexcept;

public:
	[[inline]] std::optional<size_t> getSubLevelIndex(std::string_view levelPart) const noexcept;
	[[inline]] std::optional<PixelPosition> getSubLevelOrigin(SubLevelPtr part) const noexcept;
	[[inline]] std::optional<MapPosition> getSubLevelPositionInMap(std::string_view levelPart) const noexcept;
	[[inline]] SubLevelPtr getSubLevelByName(std::string_view levelPart) const noexcept;
	[[inline]] SubLevelPtr getSubLevelAtPosition(const PixelPosition& position) const noexcept;
	[[inline]] SubLevelPtr getSubLevelAtPosition(const TilePosition& position) const noexcept;
	[[inline]] SubLevelPtr getSubLevelAtPosition(const MapPosition& position) const noexcept;
	[[inline]] StaticLevelDataPtr getStaticLevelDataByName(std::string_view levelPart) const noexcept;
	[[inline]] StaticLevelDataPtr getStaticLevelDataAtPosition(const MapPosition& mapPosition) const noexcept;
	[[inline]] std::pair<SubLevelPtr, StaticLevelDataPtr> getSubLevelAndStaticDataAtPosition(const MapPosition& position) const noexcept;
	[[inline]] PixelPosition convertToMapPosition(std::string_view levelPart, const LocalPixelPosition& position) const noexcept;
	[[inline]] PixelPosition convertToMapPosition(std::string_view levelPart, const LocalWholeTilePosition& position) const noexcept;
	[[inline]] PixelPosition convertToMapPosition(const MapPosition& mapPosition, const LocalPixelPosition& position) const noexcept;
	[[inline]] PixelPosition convertToMapPosition(const MapPosition& mapPosition, const LocalWholeTilePosition& position) const noexcept;
	std::generator<SubLevelPtr> getSubLevelsInRectangle(const PixelRectangleArea& area) const noexcept;
	std::generator<SubLevelPtr> getSubLevelsInRectangle(const WholeTileRectangleArea& area) const noexcept;
	std::generator<SubLevelPtr> getNearbySubLevels(const PixelPosition& position, uint32_t tileDistance = 64) const noexcept;

public:
	[[inline]] auto& getPlayers() noexcept;
	[[inline]] auto& getNPCs() noexcept;
	[[inline]] auto& getArrows() noexcept;
	[[inline]] auto& getBombs() noexcept;
	[[inline]] auto& getExplosions() noexcept;
	[[inline]] auto& getHorses() noexcept;
	[[inline]] auto& getItems() noexcept;

	[[inline]] const auto& getPlayers() const noexcept;
	[[inline]] const auto& getNPCs() const noexcept;
	[[inline]] const auto& getArrows() const noexcept;
	[[inline]] const auto& getBombs() const noexcept;
	[[inline]] const auto& getExplosions() const noexcept;
	[[inline]] const auto& getHorses() const noexcept;
	[[inline]] const auto& getItems() const noexcept;

	std::generator<const LevelBaddy&> getBaddies() const noexcept;
	std::generator<const LevelChest&> getChests() const noexcept;
	std::generator<const LevelLink&> getLinks() const noexcept;
	std::generator<const LevelSign&> getSigns() const noexcept;
	std::generator<std::pair<const LevelSign*, WholeTilePosition>> getSignPositions() const noexcept;

	size_t getBaddyCount() const noexcept;
	size_t getChestCount() const noexcept;
	size_t getLinkCount() const noexcept;
	size_t getSignCount() const noexcept;

public:
	std::optional<LevelTiles::TileArray*> getTiles(const MapPosition& mapLevel, size_t layer = 0) noexcept;
	std::optional<const LevelTiles::TileArray*> getTiles(const MapPosition& mapLevel, size_t layer = 0) const noexcept;
	std::optional<LevelTiles::TileArray*> getTiles(std::string_view levelPart, size_t layer = 0) noexcept;
	std::optional<const LevelTiles::TileArray*> getTiles(std::string_view levelPart, size_t layer = 0) const noexcept;

public:
	bool hasTerrain() const noexcept;
	double getHeightAt(const PixelPosition& position) const noexcept;

public:
	void sendBoardToPlayer(std::shared_ptr<Player> player) const;
	void sendBoardLayersToPlayer(std::shared_ptr<Player> player) const;
	void sendBoardHeightsToPlayer(std::shared_ptr<Player> player) const;
	void sendBoardChangesToPlayer(std::shared_ptr<Player> player, std::optional<clock::time_point> time) const;
	//
	void sendChestsToPlayer(std::shared_ptr<Player> player) const;
	void sendLinksToPlayer(std::shared_ptr<Player> player, bool onlyMapLinks) const;
	void sendSignsToPlayer(std::shared_ptr<Player> player) const;
	//
	void sendBaddiesToPlayer(std::shared_ptr<Player> player) const;
	void sendHorsesToPlayer(std::shared_ptr<Player> player) const;
	void sendNPCsToPlayer(std::shared_ptr<Player> player, std::optional<clock::time_point> time) const;

public:
	bool hasPlayers() const { return !m_players.empty(); }
	bool isPlayerLeader(PlayerID id) const;
	bool hasLivingBaddies() const;
	[[inline]] bool isSparringZone(const MapPosition& mapPosition) const noexcept;
	[[inline]] bool isNoPkZone(const MapPosition& mapPosition) const noexcept;

public:
	int addPlayer(PlayerID id);
	void removePlayer(PlayerID id);

public:
	bool addNPC(std::shared_ptr<NPC> npc);
	bool addNPC(NPCID npcId);
	void removeNPC(std::shared_ptr<NPC> npc);
	void removeNPC(NPCID npcId);

public:
	bool alterBoard(CString& tileData, const WholeTileRectangleArea& area, Player* player, bool forceRespawn = false, bool allowRespawn = true, bool sendToPlayers = false);
	void applyBoardChangeFromScriptTiles(const WholeTileRectangleArea& area, bool forceRespawn = false, bool allowRespawn = true);
	void saveBoardChangeFromScriptTiles(const WholeTileRectangleArea& area);
	void updateBoard(const TileRectangleArea& area) noexcept;
	void updateBoard2(const TileRectangleArea& area) noexcept;

public:
	LevelArrow* addArrow(inform_client_t, const PixelPosition& position, const PixelPosition& speed, uint8_t direction, int8_t type, ScriptObject from);
	LevelArrow* addArrow(const PixelPosition& position, const PixelPosition& speed, uint8_t direction, int8_t type, ScriptObject from);
	bool removeArrow(uint8_t index);
	std::optional<LevelArrow*> getArrow(size_t index) noexcept;

public:
	LevelBaddy* addBaddy(const LocalPixelPosition& position, BaddyType type);
	LevelBaddy* putNewBaddy(const LocalPixelPosition& position, BaddyType type);
	LevelBaddy* putNewBaddy(const LocalPixelPosition& position, BaddyType type, uint8_t power, std::string_view image = {});
	bool removeBaddy(uint8_t pId);
	bool removeAllBaddies();
	std::optional<LevelBaddy*> getBaddyById(uint8_t id) noexcept;
	std::optional<LevelBaddy*> getAliveBaddyByIndex(size_t index) noexcept;

public:
	LevelBomb* addBomb(inform_client_t, const PixelPosition& position, uint8_t power);
	LevelBomb* addBomb(const PixelPosition& position, uint8_t power);
	LevelBomb* addBombFromClient(const PixelPosition& position, uint8_t power, PlayerID owner, std::chrono::milliseconds timeToExplode);
	bool removeBomb(inform_client_t, size_t index);
	bool removeBomb(size_t index);
	bool removeBomb(const PixelPosition& position);
	std::optional<LevelBomb*> getBomb(size_t index) noexcept;

public:
	std::optional<const LevelChest*> getChest(size_t index) const noexcept;
	std::optional<const LevelChest*> getChest(const WholeTilePosition& position) const noexcept;
	std::optional<const LevelChest*> getChest(const MapPosition& mapPosition, const LocalWholeTilePosition& position) const noexcept;

public:
	void addExplosion(inform_client_t, const PixelPosition& position, ScriptObject from, uint8_t radius, uint8_t power);
	void addExplosion(const PixelPosition& position, ScriptObject from, uint8_t radius, uint8_t power);
	void addSpyFire(const PixelPosition& position, ScriptObject from, uint8_t direction, uint8_t length, uint8_t power);
	LevelExplosion* addExplosionPart(const PixelPosition& position, uint8_t direction, uint8_t power);
	bool removeExplosion(size_t index);
	bool removeExplosion(const PixelPosition& position);
	std::optional<LevelExplosion*> getExplosion(size_t index) noexcept;

public:
	LevelHorse* addHorse(inform_client_t, std::string_view image, const PixelPosition& position, uint8_t direction, uint8_t bushes);
	LevelHorse* addHorse(std::string_view image, const PixelPosition& position, uint8_t direction, uint8_t bushes);
	bool removeHorse(inform_client_t, size_t index);
	bool removeHorse(size_t index);
	bool removeHorse(const PixelPosition& position);
	std::optional<LevelHorse*> getHorse(size_t index) noexcept;

public:
	LevelItem* addItem(inform_client_t, const PixelPosition& position, LevelItemType item);
	LevelItem* addItem(const PixelPosition& position, LevelItemType item);
	bool removeItem(inform_client_t, size_t index);
	bool removeItem(size_t index);
	LevelItemType removeItem(const PixelPosition& position);
	std::optional<LevelItem*> getItem(size_t index) noexcept;

public:
	std::optional<const LevelLink*> getLink(size_t index) const noexcept;
	std::optional<const LevelLink*> getLink(std::string_view levelPart, const LocalWholeTilePosition& position, bool excludeOverworld = false) const noexcept;
	std::optional<const LevelLink*> getLink(const TilePosition& position, bool excludeOverworld = false) const noexcept;

public:
	LevelShoot* addShoot(LevelShoot* existingShoot);
	LevelShoot* addShoot(inform_client_t, const PixelPosition& position, float angle, float zangle, uint8_t power, float gravity, const std::string& gani, ScriptObject from);
	LevelShoot* addShoot(const PixelPosition& position, float angle, float zangle, uint8_t power, float gravity, const std::string& gani, ScriptObject from);
	LevelShoot* addShoot(const PixelPosition& position, uint8_t angle, uint8_t zangle, uint8_t power, float gravity, const std::string& gani, ScriptObject from);
	bool removeShoot(uint8_t index);
	LevelShoot* getShoot(uint8_t index) const;

public:
	std::optional<const LevelSign*> getSign(size_t index) const noexcept;

public:
	bool moveShoot(LevelShoot* shoot, int iterations);
	bool moveArrow(LevelArrow* arrow, int iterations);

public:
	bool isOnWall(const WholeTilePosition& tilePosition) const noexcept;
	bool isOnWall(const PixelPosition& position) const noexcept;
	bool isOnWall2(const WholeTileRectangleArea& tileArea) const noexcept;
	bool isOnWall2(const PixelRectangleArea& area) const noexcept;
	bool isOnWater(const WholeTilePosition& tilePosition) const noexcept;
	bool isOnWater(const PixelPosition& position) const noexcept;
	bool isOnWater2(const WholeTileRectangleArea& tileArea) const noexcept;
	bool isOnWater2(const PixelRectangleArea& area) const noexcept;
	bool isOnPlayer(const PixelPosition& position) const noexcept;
	bool isOnPlayer(const PixelRectangleArea& pixelArea) const noexcept;
	tileset::TileType getTileTypeAt(const WholeTilePosition& tilePosition) const noexcept;
	tileset::TileType getTileTypeAt(const PixelPosition& position) const noexcept;

public:
	std::generator<const PlayerID&> findInRangePlayers(const PixelPosition& position, std::optional<std::pair<uint32_t, uint32_t>> range = std::nullopt) const noexcept;
	std::generator<const PlayerID&> findInRangePlayersForCommunication(const PixelPosition& position) const noexcept;
	std::generator<const PlayerID&> findPlayersInLevelPart(std::string_view levelPart) const noexcept;
	std::generator<const PlayerID&> findPlayersInLevelPart(const MapPosition& mapLevel) const noexcept;
	std::generator<const NPCID&> findInRangeNPCs(const PixelPosition& position) const noexcept;
	std::generator<const NPCID&> findInRangeNPCsByDistance(const PixelPosition& position, uint32_t tileDistance) const noexcept;
	std::generator<const NPCID&> findIntersectingNPCs(const PixelPosition& position, bool includeInvisible = false) const noexcept;
	std::generator<const NPCID&> findIntersectingNPCs(const PixelRectangleArea& area, bool includeInvisible = false) const noexcept;
	std::generator<const NPCID&> findIntersectingNPCsForCollision(const PixelPosition& position) const noexcept;
	std::generator<const NPCID&> findIntersectingNPCsForCollision(const PixelRectangleArea& area) const noexcept;

public:
	std::string levelName;
	clock::time_point modTime;
	ScriptContainer scripting;

public:
	bool isSinglePlayer = false;
	bool isGroupMap = false;
	std::string groupMapName;

protected:
	std::shared_ptr<NPC> generateItemNPC(const PixelPosition& position, LevelItemType item);
	size_t getMapIndexAtPosition(const MapPosition& mapLevel) const noexcept;

private:
	Server* m_server;
	std::filesystem::path m_filePath;
	mutable std::shared_ptr<Map> m_map;

	std::vector<SubLevelPtr> m_levelParts;

	std::deque<PlayerID> m_players;

	// TODO: Could be optimized with flat_set, whenever that becomes generally available.
	std::unordered_set<NPCID> m_npcs;

	std::vector<LevelArrow> m_arrows;
	std::vector<LevelBomb> m_bombs;
	std::vector<LevelExplosion> m_explosions;
	std::vector<LevelHorse> m_horses;
	std::vector<LevelItem> m_items;
	std::vector<LevelShoot> m_shoots;
};

using LevelPtr = std::shared_ptr<Level>;

//----------------------------

inline void Level::setMap(std::shared_ptr<Map> map)
{
	m_map = map;
}

inline auto Level::getMap() const noexcept
{
	return m_map;
}

inline bool Level::isOnBigMap() const noexcept
{
	return m_map != nullptr && m_map->isBigMap();
}

inline constexpr Dimension<uint8_t> Level::tilesPerSubLevel() noexcept
{
	return { 64_ui8, 64_ui8 };
}

inline constexpr Dimension<uint8_t> Level::pixelsPerTile() noexcept
{
	return { 16_ui8, 16_ui8 };
}

inline constexpr Dimension<uint16_t> Level::pixelsPerSubLevel() noexcept
{
	return Dimension<uint16_t>{ tilesPerSubLevel() } * pixelsPerTile();
}

inline Dimension<uint8_t> Level::sizeInSubLevels() const noexcept
{
	if (m_map == nullptr) return { 1, 1 };
	return m_map->size;
}

inline Dimension<uint32_t> Level::sizeInTiles() const noexcept
{
	auto size = sizeInSubLevels();
	auto tileSize = tilesPerSubLevel();
	return { static_cast<uint32_t>(size.width() * tileSize.width()), static_cast<uint32_t>(size.height() * tileSize.height()) };
}

inline Dimension<uint32_t> Level::sizeInPixels() const noexcept
{
	auto size = sizeInSubLevels();
	auto pixelSize = pixelsPerSubLevel();
	return { static_cast<uint32_t>(size.width()) * pixelSize.width(), static_cast<uint32_t>(size.height()) * pixelSize.height() };
}

inline Rectangle<uint32_t, uint32_t> Level::getBoundingBox() const noexcept
{
	//return { getMapPixelOffset(), { 1024_ui16, 1024_ui16 } };
	return { { 0_ui32, 0_ui32 }, sizeInPixels() };
}

inline std::string_view Level::getLevelNameAtPosition(const PixelPosition& position) const noexcept
{
	if (!isGmap())
		return levelName;

	if (auto subLevel = getSubLevelAtPosition(position); subLevel != nullptr)
	{
		if (auto staticData = subLevel->staticData.lock(); staticData != nullptr)
			return staticData->levelName;
	}

	return levelName;
}

//----------------------------

inline std::optional<size_t> Level::getSubLevelIndex(std::string_view levelPart) const noexcept
{
	if (!isGmap())
	{
		if (levelPart == levelName)
			return 0;
		return std::nullopt;
	}

	if (auto pos = getSubLevelPositionInMap(levelPart); pos.has_value())
		return static_cast<size_t>(pos.value().y()) * m_map->size.width() + pos.value().x();

	return std::nullopt;
}

inline std::optional<PixelPosition> Level::getSubLevelOrigin(SubLevelPtr part) const noexcept
{
	if (part == nullptr || !part->mapPosition.has_value())
		return std::nullopt;

	auto pixelPerPart = pixelsPerSubLevel();
	return PixelPosition{ static_cast<int32_t>(part->mapPosition.value().x()) * pixelPerPart.width(), static_cast<int32_t>(part->mapPosition.value().y()) * pixelPerPart.height(), 0};
}

inline std::optional<MapPosition> Level::getSubLevelPositionInMap(std::string_view levelPart) const noexcept
{
	if (!isGmap())
	{
		if (levelPart == levelName)
			return MapPosition{ 0, 0 };
		return std::nullopt;
	}

	if (auto mapPosOpt = m_map->getLevelPosition(levelPart); mapPosOpt.has_value())
		return mapPosOpt.value();

	return std::nullopt;
}

inline SubLevelPtr Level::getSubLevelByName(std::string_view levelPart) const noexcept
{
	if (auto index = getSubLevelIndex(levelPart); index.has_value() && index.value() < m_levelParts.size())
		return m_levelParts[index.value()];

	return nullptr;
}

inline SubLevelPtr Level::getSubLevelAtPosition(const PixelPosition& position) const noexcept
{
	if (!isGmap())
		return m_levelParts.size() > 0 ? m_levelParts[0] : nullptr;

	auto mapPosition = position / 1024;
	auto index = static_cast<size_t>(mapPosition.y()) * m_map->size.width() + mapPosition.x();
	if (index < m_levelParts.size())
		return m_levelParts[index];
	return nullptr;
}

inline SubLevelPtr Level::getSubLevelAtPosition(const TilePosition& position) const noexcept
{
	if (!isGmap())
		return m_levelParts.size() > 0 ? m_levelParts[0] : nullptr;

	auto mapPosition = position / 64;
	auto index = static_cast<size_t>(mapPosition.y()) * m_map->size.width() + static_cast<size_t>(mapPosition.x());
	if (index < m_levelParts.size())
		return m_levelParts[index];
	return nullptr;
}

inline SubLevelPtr Level::getSubLevelAtPosition(const MapPosition& mapPosition) const noexcept
{
	if (!isGmap())
		return m_levelParts.size() > 0 ? m_levelParts[0] : nullptr;

	auto index = static_cast<size_t>(mapPosition.y()) * m_map->size.width() + static_cast<size_t>(mapPosition.x());
	if (index < m_levelParts.size())
		return m_levelParts[index];
	return nullptr;
}

inline StaticLevelDataPtr Level::getStaticLevelDataByName(std::string_view levelPart) const noexcept
{
	auto levelPartData = getSubLevelByName(levelPart);
	if (levelPartData != nullptr)
		return levelPartData->staticData.lock();
	return nullptr;
}

inline StaticLevelDataPtr Level::getStaticLevelDataAtPosition(const MapPosition& mapPosition) const noexcept
{
	auto levelPartData = getSubLevelAtPosition(mapPosition);
	if (levelPartData != nullptr)
		return levelPartData->staticData.lock();
	return nullptr;
}

inline std::pair<SubLevelPtr, StaticLevelDataPtr> Level::getSubLevelAndStaticDataAtPosition(const MapPosition& position) const noexcept
{
	auto subLevel = getSubLevelAtPosition(position);
	if (subLevel != nullptr)
	{
		if (auto levelData = subLevel->staticData.lock(); levelData != nullptr)
			return { subLevel, levelData };
	}

	return { nullptr, nullptr };
}

inline PixelPosition Level::convertToMapPosition(std::string_view levelPart, const LocalPixelPosition& position) const noexcept
{
	if (isGmap())
	{
		auto mapPosition = getSubLevelPositionInMap(levelPart).value_or(MapPosition{});
		auto pixelPerPart = pixelsPerSubLevel();
		return { position.x() + static_cast<int32_t>(mapPosition.x()) * pixelPerPart.width(), position.y() + static_cast<int32_t>(mapPosition.y()) * pixelPerPart.height(), static_cast<int32_t>(position.z()) };
	}
	return { position.x(), position.y(), position.z() };
}

inline PixelPosition Level::convertToMapPosition(std::string_view levelPart, const LocalWholeTilePosition& position) const noexcept
{
	auto mapPosition = getSubLevelPositionInMap(levelPart).value_or(MapPosition{});
	return convertToMapPosition(mapPosition, position);
}

inline PixelPosition Level::convertToMapPosition(const MapPosition& mapPosition, const LocalPixelPosition& position) const noexcept
{
	if (isGmap())
	{
		auto pixelPerPart = pixelsPerSubLevel();
		return { static_cast<int32_t>(position.x()) + (mapPosition.x() * pixelPerPart.width()), static_cast<int32_t>(position.y()) + (mapPosition.y() * pixelPerPart.height()), static_cast<int32_t>(position.z()) };
	}
	return { static_cast<int32_t>(position.x()), static_cast<int32_t>(position.y()), static_cast<int32_t>(position.z()) };
}

inline PixelPosition Level::convertToMapPosition(const MapPosition& mapPosition, const LocalWholeTilePosition& position) const noexcept
{
	auto pixelPerTile = pixelsPerTile();

	if (isGmap())
	{
		auto pixelPerPart = pixelsPerSubLevel();
		return { (static_cast<int32_t>(position.x()) * pixelPerTile.width()) + (mapPosition.x() * pixelPerPart.width()), (static_cast<int32_t>(position.y()) * pixelPerTile.height()) + (mapPosition.y() * pixelPerPart.height()), static_cast<int32_t>(position.z()) * pixelPerTile.length() };
	}
	return { static_cast<int32_t>(position.x() * pixelPerTile.width()), static_cast<int32_t>(position.y() * pixelPerTile.height()), static_cast<int32_t>(position.z() * pixelPerTile.length()) };
}

//----------------------------

inline auto& Level::getPlayers() noexcept
{
	return m_players;
}

inline auto& Level::getNPCs() noexcept
{
	return m_npcs;
}

inline auto& Level::getArrows() noexcept
{
	return m_arrows;
}

inline auto& Level::getBombs() noexcept
{
	return m_bombs;
}

inline auto& Level::getExplosions() noexcept
{
	return m_explosions;
}

inline auto& Level::getHorses() noexcept
{
	return m_horses;
}

inline auto& Level::getItems() noexcept
{
	return m_items;
}

inline const auto& Level::getPlayers() const noexcept
{
	return m_players;
}

inline const auto& Level::getNPCs() const noexcept
{
	return m_npcs;
}

inline const auto& Level::getArrows() const noexcept
{
	return m_arrows;
}

inline const auto& Level::getBombs() const noexcept
{
	return m_bombs;
}

inline const auto& Level::getExplosions() const noexcept
{
	return m_explosions;
}

inline const auto& Level::getHorses() const noexcept
{
	return m_horses;
}

inline const auto& Level::getItems() const noexcept
{
	return m_items;
}

inline bool Level::isSparringZone(const MapPosition& mapPosition) const noexcept
{
	if (auto subLevel = getSubLevelAtPosition(mapPosition); subLevel != nullptr)
		return subLevel->isSparringZone;
	return false;
}

inline bool Level::isNoPkZone(const MapPosition& mapPosition) const noexcept
{
	if (auto subLevel = getSubLevelAtPosition(mapPosition); subLevel != nullptr)
		return subLevel->isNoPkZone;
	return false;
}

//----------------------------

namespace source
{
/// @brief Creates a ScriptObject from a Level by hashing the level's name.
ScriptObject FromLevel(LevelPtr level);
} // end namespace source

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVEL_H
