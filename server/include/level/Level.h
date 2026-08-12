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
#include <level/LevelThrownItem.h>
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
	static void reload(const std::shared_ptr<StaticLevelData>& staticData);
	//
	std::optional<std::string> getChestFormattedForSave(LevelChest* chest) const;
	void sendBoardToPlayer(const std::shared_ptr<Player>& player) const;
	void sendBoardLayersToPlayer(const std::shared_ptr<Player>& player) const;
	void sendBoardLayerToPlayer(const std::shared_ptr<Player>& player, size_t layer) const;
	void sendChestsToPlayer(const std::shared_ptr<Player>& player) const;
	void sendLinksToPlayer(const std::shared_ptr<Player>& player, bool onlyMapLinks) const;
	void sendSignsToPlayer(const std::shared_ptr<Player>& player) const;
};
using StaticLevelDataPtr = std::shared_ptr<StaticLevelData>;

//----------------------------

/// @brief Stores the data for a specific level tile on a map.
struct SubLevel
{
	std::weak_ptr<Level> parentLevel;
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
	[[nodiscard]] PixelRectangleArea clipRectangleToPart(const PixelRectangleArea& area) const noexcept;
	[[nodiscard]] WholeTileRectangleArea clipRectangleToPart(const WholeTileRectangleArea& area) const noexcept;
	//
	[[nodiscard]] std::optional<LevelTiles*> getTiles() noexcept;
	[[nodiscard]] std::optional<const LevelTiles*> getTiles() const noexcept;
	[[nodiscard]] std::optional<LevelTiles::TileArray*> getTiles(size_t layer) noexcept;
	[[nodiscard]] std::optional<const LevelTiles::TileArray*> getTiles(size_t layer) const noexcept;
	[[nodiscard]] double getHeightAt(const LocalPixelPosition& position) const noexcept;
	void sendBoardToPlayer(const std::shared_ptr<Player>& player) const;
	void sendBoardLayersToPlayer(const std::shared_ptr<Player>& player) const;
	void sendBoardLayerToPlayer(const std::shared_ptr<Player>& player, size_t layer) const;
	void sendBoardHeightsToPlayer(const std::shared_ptr<Player>& player) const;
	void sendBoardChangesToPlayer(const std::shared_ptr<Player>& player, std::optional<clock::time_point> time) const;
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
	static std::shared_ptr<Level> clone(const LevelPtr& level, std::string_view name);

public:
	bool loaded = false;
	bool reload(std::string_view level) const;
	bool reload(const MapPosition& position) const;
	void reload(const StaticLevelDataPtr& staticData);
	bool saveLevel(const MapPosition& mapPosition, std::string_view filename) const;

public:
	void doTimedEvents();
	void doFrameEvents(precise_clock::time_point time);
	const auto& getLastFrameTime() const { return m_lastFrameTime; }
	const auto& getFilePath() const { return m_filePath; }

private:
	precise_clock::time_point m_lastFrameTime = precise_clock::now();
	precise_clock::duration m_frameEventDuration = precise_clock::duration::zero();

public:
	[[a::inline]] void setMap(const std::shared_ptr<Map>& map) const;
	[[a::inline]] auto getMap() const noexcept;
	bool isGmap() const noexcept;
	[[a::inline]] bool isOnBigMap() const noexcept;
	[[a::inline]] static constexpr Dimension<uint8_t> tilesPerSubLevel() noexcept;
	[[a::inline]] static constexpr Dimension<uint8_t> pixelsPerTile() noexcept;
	[[a::inline]] static constexpr Dimension<uint16_t> pixelsPerSubLevel() noexcept;
	[[a::inline]] Dimension<uint8_t> sizeInSubLevels() const noexcept;
	[[a::inline]] Dimension<uint32_t> sizeInTiles() const noexcept;
	[[a::inline]] Dimension<uint32_t> sizeInPixels() const noexcept;
	[[a::inline]] Rectangle<uint32_t, uint32_t> getBoundingBox() const noexcept;
	std::optional<uint16_t> getMapTileAtPosition(const TilePosition& position) const noexcept;
	uint16_t* getMapTileForEditing(const TilePosition& position) const noexcept;
	[[a::inline]] std::string_view getLevelNameAtPosition(const PixelPosition& position) const noexcept;

public:
	[[a::inline]] std::optional<size_t> getSubLevelIndex(std::string_view levelPart) const noexcept;
	[[a::inline]] static std::optional<PixelPosition> getSubLevelOrigin(const SubLevelPtr& part) noexcept;
	[[a::inline]] std::optional<MapPosition> getSubLevelPositionInMap(std::string_view levelPart) const noexcept;
	[[a::inline]] SubLevelPtr getSubLevelByName(std::string_view levelPart) const noexcept;
	[[a::inline]] SubLevelPtr getSubLevelAtPosition(const PixelPosition& position) const noexcept;
	[[a::inline]] SubLevelPtr getSubLevelAtPosition(const TilePosition& position) const noexcept;
	[[a::inline]] SubLevelPtr getSubLevelAtPosition(const MapPosition& position) const noexcept;
	[[a::inline]] StaticLevelDataPtr getStaticLevelDataByName(std::string_view levelPart) const noexcept;
	[[a::inline]] StaticLevelDataPtr getStaticLevelDataAtPosition(const MapPosition& mapPosition) const noexcept;
	[[a::inline]] std::pair<SubLevelPtr, StaticLevelDataPtr> getSubLevelAndStaticDataAtPosition(const MapPosition& position) const noexcept;
	[[a::inline]] PixelPosition convertToMapPosition(std::string_view levelPart, const LocalPixelPosition& position) const noexcept;
	[[a::inline]] PixelPosition convertToMapPosition(std::string_view levelPart, const LocalWholeTilePosition& position) const noexcept;
	[[a::inline]] PixelPosition convertToMapPosition(const MapPosition& mapPosition, const LocalPixelPosition& position) const noexcept;
	[[a::inline]] PixelPosition convertToMapPosition(const MapPosition& mapPosition, const LocalWholeTilePosition& position) const noexcept;
	std::generator<SubLevelPtr> getSubLevelsInRectangle(const PixelRectangleArea& area) const noexcept;
	std::generator<SubLevelPtr> getSubLevelsInRectangle(const WholeTileRectangleArea& area) const noexcept;
	std::generator<SubLevelPtr> getNearbySubLevels(const PixelPosition& position, uint32_t tileDistance = 64) const noexcept;

public:
	[[a::inline]] auto& getPlayers() noexcept;
	[[a::inline]] auto& getNPCs() noexcept;
	[[a::inline]] auto& getArrows() noexcept;
	[[a::inline]] auto& getBombs() noexcept;
	[[a::inline]] auto& getExplosions() noexcept;
	[[a::inline]] auto& getHorses() noexcept;
	[[a::inline]] auto& getItems() noexcept;

	[[a::inline]] const auto& getPlayers() const noexcept;
	[[a::inline]] const auto& getNPCs() const noexcept;
	[[a::inline]] const auto& getArrows() const noexcept;
	[[a::inline]] const auto& getBombs() const noexcept;
	[[a::inline]] const auto& getExplosions() const noexcept;
	[[a::inline]] const auto& getHorses() const noexcept;
	[[a::inline]] const auto& getItems() const noexcept;

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
	std::optional<LevelTiles::TileArray*> getTiles(const MapPosition& mapLevel, size_t layer = 0) const noexcept;
	std::optional<LevelTiles::TileArray*> getTiles(std::string_view levelPart, size_t layer = 0) const noexcept;

public:
	bool hasTerrain() const noexcept;
	double getHeightAt(const PixelPosition& position) const noexcept;

public:
	void sendBoardToPlayer(const std::shared_ptr<Player>& player) const;
	void sendBoardLayersToPlayer(const std::shared_ptr<Player>& player) const;
	void sendBoardHeightsToPlayer(const std::shared_ptr<Player>& player) const;
	void sendBoardChangesToPlayer(const std::shared_ptr<Player>& player, std::optional<clock::time_point> time) const;
	//
	void sendChestsToPlayer(const std::shared_ptr<Player>& player) const;
	void sendLinksToPlayer(const std::shared_ptr<Player>& player, bool onlyMapLinks) const;
	void sendSignsToPlayer(const std::shared_ptr<Player>& player) const;
	//
	void sendBaddiesToPlayer(const std::shared_ptr<Player>& player) const;
	void sendHorsesToPlayer(const std::shared_ptr<Player>& player) const;
	void sendNPCsToPlayer(const std::shared_ptr<Player>& player, std::optional<clock::time_point> time) const;

public:
	bool hasPlayers() const { return !m_players.empty(); }
	bool isPlayerLeader(PlayerID id) const;
	bool hasLivingBaddies() const;
	[[a::inline]] bool isSparringZone(const MapPosition& mapPosition) const noexcept;
	[[a::inline]] bool isNoPkZone(const MapPosition& mapPosition) const noexcept;
	[[a::inline]] bool isPrivateMap() const noexcept;

public:
	int addPlayer(PlayerID id);
	void removePlayer(PlayerID id);

public:
	bool addNPC(const std::shared_ptr<NPC>& npc);
	bool addNPC(NPCID npcId);
	void removeNPC(const std::shared_ptr<NPC>& npc);
	void removeNPC(NPCID npcId);

public:
	bool alterBoard(CString& tileData, const WholeTileRectangleArea& area, Player* player, bool forceRespawn = false, bool allowRespawn = true, bool sendToPlayers = false);
	void applyBoardChangeFromScriptTiles(const WholeTileRectangleArea& area, bool forceRespawn = false, bool allowRespawn = true);
	void saveBoardChangeFromScriptTiles(const WholeTileRectangleArea& area) const;
	void updateBoard(const TileRectangleArea& area) noexcept;
	void updateBoard2(const TileRectangleArea& area) noexcept;

public:
	LevelArrow* addArrow(inform_client_t, const PixelPosition& position, const PixelPosition& speed, uint8_t direction, int8_t type, const ScriptObject& from);
	LevelArrow* addArrow(const PixelPosition& position, const PixelPosition& speed, uint8_t direction, int8_t type, const ScriptObject& from);
	bool removeArrow(uint8_t index);
	std::optional<LevelArrow*> getArrow(size_t index) noexcept;

public:
	LevelBaddy* addBaddy(const LocalPixelPosition& position, BaddyType type);
	LevelBaddy* putNewBaddy(const LocalPixelPosition& position, BaddyType type);
	LevelBaddy* putNewBaddy(const LocalPixelPosition& position, BaddyType type, uint8_t power, std::string_view image = {});
	bool removeBaddy(uint8_t pId) const;
	bool removeAllBaddies() const;
	std::optional<LevelBaddy*> getBaddyById(uint8_t id) const noexcept;
	std::optional<LevelBaddy*> getAliveBaddyByIndex(size_t index) const noexcept;

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
	void addExplosion(inform_client_t, const PixelPosition& position, const ScriptObject& from, uint8_t radius, uint8_t power);
	void addExplosion(const PixelPosition& position, const ScriptObject& from, uint8_t radius, uint8_t power);
	void addSpyFire(const PixelPosition& position, const ScriptObject& from, uint8_t direction, uint8_t length, uint8_t power);
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
	LevelItem* addItem(inform_client_t, const PixelPosition& position, LevelItemType item, std::optional<PlayerID> addedBy = std::nullopt);
	LevelItem* addItem(const PixelPosition& position, LevelItemType item, std::optional<PlayerID> addedBy = std::nullopt);
	bool removeItem(inform_client_t, size_t index);
	bool removeItem(size_t index);
	LevelItemType removeItem(const PixelPosition& position);
	std::optional<LevelItem*> getItem(size_t index) noexcept;

public:
	std::optional<const LevelLink*> getLink(size_t index) const noexcept;
	std::optional<const LevelLink*> getLink(std::string_view levelPart, const LocalWholeTilePosition& position, bool excludeOverworld = false) const noexcept;
	std::optional<const LevelLink*> getLink(const TilePosition& position, bool excludeOverworld = false) const noexcept;

public:
	LevelShoot* addShoot(const LevelShoot* existingShoot);
	LevelShoot* addShoot(inform_client_t, const PixelPosition& position, float angle, float zangle, uint8_t power, float gravity, const std::string& gani, const ScriptObject& from);
	LevelShoot* addShoot(const PixelPosition& position, float angle, float zangle, uint8_t power, float gravity, const std::string& gani, const ScriptObject& from);
	LevelShoot* addShoot(const PixelPosition& position, uint8_t angle, uint8_t zangle, uint8_t power, float gravity, const std::string& gani, const ScriptObject& from);
	bool removeShoot(uint8_t index);
	LevelShoot* getShoot(uint8_t index) const;

public:
	std::optional<const LevelSign*> getSign(size_t index) const noexcept;

public:
	void addThrownItem(const TilePosition& position, uint8_t direction, CarryObjectSprite item, const ScriptObject& from);

public:
	bool moveShoot(LevelShoot* shoot, int iterations) const;
	bool moveArrow(LevelArrow* arrow, int iterations);
	bool moveThrownItem(size_t index, int iterations);

public:
	bool isOnWall(const WholeTilePosition& tilePosition) const noexcept;
	bool isOnWall(const PixelPosition& position) const noexcept;
	bool isOnWall2(const WholeTileRectangleArea& tileArea) const noexcept;
	bool isOnWall2(const PixelRectangleArea& area) const noexcept;
	bool isOnWater(const WholeTilePosition& tilePosition) const noexcept;
	bool isOnWater(const PixelPosition& position) const noexcept;
	bool isOnWater2(const WholeTileRectangleArea& tileArea) const noexcept;
	bool isOnWater2(const PixelRectangleArea& area) const noexcept;
	bool isOnNPC(const PixelPosition& position) const noexcept;
	bool isOnNPC(const PixelRectangleArea& pixelArea) const noexcept;
	bool isOnPlayer(const PixelPosition& position) const noexcept;
	bool isOnPlayer(const PixelRectangleArea& pixelArea) const noexcept;
	tileset::TileType getTileTypeAt(const WholeTilePosition& tilePosition) const noexcept;
	tileset::TileType getTileTypeAt(const PixelPosition& position) const noexcept;

public:
	std::generator<PlayerID> findInRangePlayers(const PixelPosition& position, std::optional<std::pair<uint32_t, uint32_t>> range = std::nullopt) const noexcept;
	std::generator<PlayerID> findInRangePlayersForCommunication(const PixelPosition& position) const noexcept;
	std::generator<PlayerID> findPlayersInLevelPart(std::string_view levelPart) const noexcept;
	std::generator<PlayerID> findPlayersInLevelPart(const MapPosition& mapLevel) const noexcept;
	std::generator<NPCID> findInRangeNPCs(const PixelPosition& position) const noexcept;
	std::generator<NPCID> findInRangeNPCsByDistance(const PixelPosition& position, uint32_t tileDistance) const noexcept;
	std::generator<NPCID> findIntersectingNPCs(const PixelPosition& position, bool includeInvisible = false) const noexcept;
	std::generator<NPCID> findIntersectingNPCs(const PixelRectangleArea& area, bool includeInvisible = false) const noexcept;
	std::generator<NPCID> findIntersectingNPCsForCollision(const PixelPosition& position) const noexcept;
	std::generator<NPCID> findIntersectingNPCsForCollision(const PixelRectangleArea& area) const noexcept;

public:
	std::string levelName;
	clock::time_point modTime;
	std::optional<clock::time_point> timeSinceLastPlayerLeft;
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
	std::vector<std::optional<LevelThrownItem>> m_thrownItems;
};

using LevelPtr = std::shared_ptr<Level>;

//----------------------------

inline void Level::setMap(const std::shared_ptr<Map>& map) const
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
	if (m_map == nullptr || m_map->isBigMap()) return { 1, 1 };
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

	if (const auto subLevel = getSubLevelAtPosition(position); subLevel != nullptr)
	{
		if (const auto staticData = subLevel->staticData.lock(); staticData != nullptr)
			return staticData->levelName;
	}

	return levelName;
}

//----------------------------

inline std::optional<size_t> Level::getSubLevelIndex(const std::string_view levelPart) const noexcept
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

inline std::optional<PixelPosition> Level::getSubLevelOrigin(const SubLevelPtr& part) noexcept
{
	if (part == nullptr || !part->mapPosition.has_value())
		return std::nullopt;

	auto pixelPerPart = pixelsPerSubLevel();
	return PixelPosition{ static_cast<int32_t>(part->mapPosition.value().x()) * pixelPerPart.width(), static_cast<int32_t>(part->mapPosition.value().y()) * pixelPerPart.height(), 0};
}

inline std::optional<MapPosition> Level::getSubLevelPositionInMap(const std::string_view levelPart) const noexcept
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

inline SubLevelPtr Level::getSubLevelByName(const std::string_view levelPart) const noexcept
{
	if (const auto index = getSubLevelIndex(levelPart); index.has_value() && index.value() < m_levelParts.size())
		return m_levelParts[index.value()];

	return nullptr;
}

inline SubLevelPtr Level::getSubLevelAtPosition(const PixelPosition& position) const noexcept
{
	if (!isGmap())
		return !m_levelParts.empty() ? m_levelParts[0] : nullptr;

	const auto mapPosition = position / 1024;
	const auto index = static_cast<size_t>(mapPosition.y()) * m_map->size.width() + mapPosition.x();
	if (index < m_levelParts.size())
		return m_levelParts[index];
	return nullptr;
}

inline SubLevelPtr Level::getSubLevelAtPosition(const TilePosition& position) const noexcept
{
	if (!isGmap())
		return !m_levelParts.empty() ? m_levelParts[0] : nullptr;

	const auto mapPosition = position / 64;
	const auto index = static_cast<size_t>(mapPosition.y()) * m_map->size.width() + static_cast<size_t>(mapPosition.x());
	if (index < m_levelParts.size())
		return m_levelParts[index];
	return nullptr;
}

inline SubLevelPtr Level::getSubLevelAtPosition(const MapPosition& position) const noexcept
{
	if (!isGmap())
		return !m_levelParts.empty() ? m_levelParts[0] : nullptr;

	const auto index = static_cast<size_t>(position.y()) * m_map->size.width() + static_cast<size_t>(position.x());
	if (index < m_levelParts.size())
		return m_levelParts[index];
	return nullptr;
}

inline StaticLevelDataPtr Level::getStaticLevelDataByName(const std::string_view levelPart) const noexcept
{
	const auto levelPartData = getSubLevelByName(levelPart);
	if (levelPartData != nullptr)
		return levelPartData->staticData.lock();
	return nullptr;
}

inline StaticLevelDataPtr Level::getStaticLevelDataAtPosition(const MapPosition& mapPosition) const noexcept
{
	const auto levelPartData = getSubLevelAtPosition(mapPosition);
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

inline PixelPosition Level::convertToMapPosition(const std::string_view levelPart, const LocalPixelPosition& position) const noexcept
{
	if (isGmap())
	{
		auto mapPosition = getSubLevelPositionInMap(levelPart).value_or(MapPosition{});
		auto pixelPerPart = pixelsPerSubLevel();
		return { position.x() + static_cast<int32_t>(mapPosition.x()) * pixelPerPart.width(), position.y() + static_cast<int32_t>(mapPosition.y()) * pixelPerPart.height(), static_cast<int32_t>(position.z()) };
	}
	return { position.x(), position.y(), position.z() };
}

inline PixelPosition Level::convertToMapPosition(const std::string_view levelPart, const LocalWholeTilePosition& position) const noexcept
{
	const auto mapPosition = getSubLevelPositionInMap(levelPart).value_or(MapPosition{});
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
	if (const auto subLevel = getSubLevelAtPosition(mapPosition); subLevel != nullptr)
		return subLevel->isSparringZone;
	return false;
}

inline bool Level::isNoPkZone(const MapPosition& mapPosition) const noexcept
{
	if (const auto subLevel = getSubLevelAtPosition(mapPosition); subLevel != nullptr)
		return subLevel->isNoPkZone;
	return false;
}

inline bool Level::isPrivateMap() const noexcept
{
	return isSinglePlayer || isGroupMap;
}

//----------------------------

namespace source
{
/// @brief Creates a ScriptObject from a Level by hashing the level's name.
ScriptObject FromLevel(const LevelPtr& level);
} // end namespace source

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVEL_H
