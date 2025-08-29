#ifndef LEVEL_H
#define LEVEL_H

#include <chrono>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <generator>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
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
#include <level/LevelTiles.h>
#include <level/Map.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class Player;
class NPC;
class LevelLoader;

class Level : public std::enable_shared_from_this<Level>
{
	friend class LevelLoader;

private:
	Level(uint16_t fillTile = 0);

public:
	//! Destructor.
	~Level();

public:
	static std::shared_ptr<Level> createLevel(uint16_t fillTile = 511, std::string_view levelName = ""sv);
	static std::shared_ptr<Level> clone(LevelPtr level);

public:
	bool loaded = false;
	bool reload();
	void saveLevel(const std::string& filename);

public:
	void doTimedEvents();
	void doFrameEvents(precise_clock::time_point time);
	const auto& getLastFrameTime() const { return m_lastFrameTime; }

private:
	precise_clock::time_point m_lastFrameTime = precise_clock::now();
	precise_clock::duration m_frameEventDuration = 0ns;

public:
	[[inline]] void setMap(std::shared_ptr<Map> map);
	auto getMap() const noexcept { return m_map; }
	[[inline]] Dimension<uint8_t> getMapSizeInParts() const noexcept;
	[[inline]] Dimension<uint32_t> getMapSizeInTiles() const noexcept;
	[[inline]] Dimension<uint32_t> getMapSizeInPixels() const noexcept;
	[[inline]] PixelPosition getMapPixelOffset() const noexcept;
	[[inline]] PixelRectangleArea getMapBoundingBox() const noexcept;
	bool isOnGmap() const noexcept { return m_map != nullptr && m_map->isGmap(); }
	bool isOnBigMap() const noexcept { return m_map != nullptr && m_map->isBigMap(); }
	[[inline]] const std::string getMapOrLevelName() const noexcept;
	[[inline]] PixelPosition convertToMapPosition(const LocalPixelPosition& position) const noexcept;
	[[inline]] PixelPosition convertToMapPosition(const LocalWholeTilePosition& position) const noexcept;

public:
	auto& getLevelPlayers() { return m_players; }
	auto& getLevelNPCs() { return m_npcs; }
	auto& getArrows() { return m_arrows; }
	auto& getBaddies() { return m_baddies; }
	auto& getBombs() { return m_bombs; }
	auto& getChests() { return m_chests; }
	auto& getExplosions() { return m_explosions; }
	auto& getHorses() { return m_horses; }
	auto& getItems() { return m_items; }
	auto& getLinks() { return m_links; }
	auto& getSigns() { return m_signs; }

public:
	auto& getTiles(size_t layer = 0) { return m_tiles.at(layer); }
	const auto& getTiles(size_t layer = 0) const { return m_tiles.at(layer); }
	auto& getLayers() { return m_tiles; }

public:
	CString getBoardPacket();
	CString getLayerPacket(int i);
	CString getBoardChangesPacket(time_t time);
	CString getBoardChangesPacket2(time_t time);
	void sendBaddiesToPlayer(std::shared_ptr<Player> player) const;
	void sendChestsToPlayer(std::shared_ptr<Player> player) const;
	void sendHorsesToPlayer(std::shared_ptr<Player> player) const;
	void sendLinksToPlayer(std::shared_ptr<Player> player) const;
	void sendSignsToPlayer(std::shared_ptr<Player> player) const;
	void sendNPCsToPlayer(std::shared_ptr<Player> player, clock::time_point time) const;

public:
	bool hasPlayers() const { return !m_players.empty(); }
	bool isPlayerLeader(PlayerID id) const;
	bool hasLivingBaddies() const;

public:
	int addPlayer(PlayerID id);
	void removePlayer(PlayerID id);

public:
	bool addNPC(std::shared_ptr<NPC> npc);
	bool addNPC(NPCID npcId);
	void removeNPC(std::shared_ptr<NPC> npc);
	void removeNPC(NPCID npcId);

public:
	bool alterBoard(CString& tileData, const LocalWholeTileRectangleArea& area, Player* player);

public:
	LevelArrow* addArrow(inform_client_t, const PixelPosition& position, const PixelPosition& speed, uint8_t direction, int8_t type, ScriptObjectSource from);
	LevelArrow* addArrow(const PixelPosition& position, const PixelPosition& speed, uint8_t direction, int8_t type, ScriptObjectSource from);
	bool removeArrow(uint8_t index);
	LevelArrow* getArrow(uint8_t index) const;

public:
	LevelBaddy* addBaddy(const LocalPixelPosition& position, BaddyType type);
	LevelBaddy* putNewBaddy(const LocalPixelPosition& position, BaddyType type);
	LevelBaddy* putNewBaddy(const LocalPixelPosition& position, BaddyType type, uint8_t power, std::string_view image = {});
	bool removeBaddy(uint8_t pId);
	bool removeAllBaddies();
	LevelBaddy* getBaddy(uint8_t id) const;

public:
	LevelBomb* addBomb(inform_client_t, const PixelPosition& position, uint8_t power);
	LevelBomb* addBomb(const PixelPosition& position, uint8_t power);
	bool removeBomb(inform_client_t, size_t index);
	bool removeBomb(size_t index);
	bool removeBomb(const PixelPosition& position);
	LevelBomb* getBomb(size_t index) const;

public:
	LevelChest* addChest(const LocalWholeTilePosition& position, const LevelItemType itemType, const int signIndex);
	bool removeChest(size_t index);
	LevelChest* getChest(size_t index) const;
	std::optional<const LevelChest*> getChest(const LocalWholeTilePosition& position) const;
	std::string getChestFormattedForSave(LevelChest* chest) const;

public:
	void addExplosion(inform_client_t, const PixelPosition& position, ScriptObjectSource from, uint8_t radius, uint8_t power);
	void addExplosion(const PixelPosition& position, ScriptObjectSource from, uint8_t radius, uint8_t power);
	void addSpyFire(const PixelPosition& position, ScriptObjectSource from, uint8_t direction, uint8_t length, uint8_t power);
	LevelExplosion* addExplosionPart(const PixelPosition& position, uint8_t direction, uint8_t power);
	bool removeExplosion(size_t index);
	bool removeExplosion(const PixelPosition& position);
	LevelExplosion* getExplosion(size_t index) const;

public:
	LevelHorse* addHorse(inform_client_t, std::string_view image, const PixelPosition& position, uint8_t direction, uint8_t bushes);
	LevelHorse* addHorse(std::string_view image, const PixelPosition& position, uint8_t direction, uint8_t bushes);
	bool removeHorse(inform_client_t, size_t index);
	bool removeHorse(size_t index);
	bool removeHorse(const PixelPosition& position);
	LevelHorse* getHorse(size_t index) const;

public:
	LevelItem* addItem(inform_client_t, const PixelPosition& position, LevelItemType item);
	LevelItem* addItem(const PixelPosition& position, LevelItemType item);
	bool removeItem(inform_client_t, size_t index);
	bool removeItem(size_t index);
	LevelItemType removeItem(const PixelPosition& position);
	LevelItem* getItem(size_t index) const;

public:
	LevelLink* addLink(const std::vector<CString>& link);
	bool removeLink(uint32_t index);
	std::optional<const LevelLink*> getLink(const LocalWholeTilePosition& position, bool excludeOverworld = false) const;

public:
	LevelShoot* addShoot(LevelShoot* existingShoot);
	LevelShoot* addShoot(inform_client_t, const PixelPosition& position, float angle, float zangle, uint8_t power, float gravity, const std::string& gani, ScriptObjectSource from);
	LevelShoot* addShoot(const PixelPosition& position, float angle, float zangle, uint8_t power, float gravity, const std::string& gani, ScriptObjectSource from);
	LevelShoot* addShoot(const PixelPosition& position, uint8_t angle, uint8_t zangle, uint8_t power, float gravity, const std::string& gani, ScriptObjectSource from);
	bool removeShoot(uint8_t index);
	LevelShoot* getShoot(uint8_t index) const;

public:
	LevelSign* addSign(const LocalWholeTilePosition& position, const CString& sign, bool encoded = false);
	bool removeSign(uint32_t index);
	LevelSign* getSign(size_t index) const;

public:
	bool moveShoot(LevelShoot* shoot, int iterations);
	bool moveArrow(LevelArrow* arrow, int iterations);

public:
	bool isOnWall(const LocalWholeTilePosition& tilePosition) const noexcept;
	bool isOnWall(const PixelPosition& position) const noexcept;
	bool isOnWall2(const LocalWholeTileRectangleArea& tileArea) const noexcept;
	bool isOnWall2(const PixelRectangleArea& area) const noexcept;
	bool isOnWater(const LocalWholeTilePosition& tilePosition) const noexcept;
	bool isOnWater(const PixelPosition& position) const noexcept;
	bool isOnWater2(const LocalWholeTileRectangleArea& tileArea) const noexcept;
	bool isOnWater2(const PixelRectangleArea& area) const noexcept;
	bool isOnPlayer(const PixelPosition& position) const noexcept;
	bool isOnPlayer(const PixelRectangleArea& pixelArea) const noexcept;

public:
	std::generator<const PlayerID&> getMapPlayers() const noexcept;
	std::generator<const NPCID&> getMapNPCs() const noexcept;
	std::generator<LevelArrow&> getMapArrows() noexcept;
	std::generator<LevelBomb&> getMapBombs() noexcept;
	std::generator<LevelExplosion&> getMapExplosions() noexcept;
	std::generator<LevelHorse&> getMapHorses() noexcept;
	std::generator<LevelItem&> getMapItems() noexcept;
	std::generator<LevelSign&> getMapSigns() noexcept;
	size_t getMapPlayerCount() const noexcept;
	size_t getMapNPCCount() const noexcept;
	size_t getMapArrowCount() const noexcept;
	size_t getMapBombCount() const noexcept;
	size_t getMapExplosionCount() const noexcept;
	size_t getMapHorseCount() const noexcept;
	size_t getMapItemCount() const noexcept;
	size_t getMapSignCount() const noexcept;
	std::optional<LevelArrow*> getMapArrow(size_t index) noexcept;
	std::optional<LevelBomb*> getMapBomb(size_t index) noexcept;
	std::optional<LevelExplosion*> getMapExplosion(size_t index) noexcept;
	std::optional<LevelHorse*> getMapHorse(size_t index) noexcept;
	std::optional<LevelItem*> getMapItem(size_t index) noexcept;
	std::optional<LevelSign*> getMapSign(size_t index) noexcept;

public:
	std::generator<const PlayerID&> findInRangePlayers(const PixelPosition& position) const noexcept;
	std::generator<const PlayerID&> findInRangePlayersForCommunication(const PixelPosition& position) const noexcept;
	std::generator<const NPCID&> findInRangeNPCs(const PixelPosition& position) const noexcept;
	std::generator<const NPCID&> findInRangeNPCsByDistance(const PixelPosition& position, uint32_t tileDistance) const noexcept;
	std::generator<const NPCID&> findIntersectingNPCs(const PixelPosition& position, bool includeInvisible = false) const noexcept;
	std::generator<const NPCID&> findIntersectingNPCs(const PixelRectangleArea& area, bool includeInvisible = false) const noexcept;
	std::generator<const NPCID&> findIntersectingNPCsForCollision(const PixelPosition& position) const noexcept;
	std::generator<const NPCID&> findIntersectingNPCsForCollision(const PixelRectangleArea& area) const noexcept;

public:
	std::string levelName;
	Position<uint8_t> mapPosition;
	bool isSparringZone = false;
	bool isNoPkZone = false;
	bool isSingleplayer = false;
	clock::time_point modTime;
	ScriptContainer scripting;

private:
	std::filesystem::path m_filePath;
	std::shared_ptr<Map> m_map;

	std::map<uint8_t, LevelTiles> m_tiles;
	std::vector<LevelBoardChange> m_boardChanges;

	std::deque<PlayerID> m_players;
	std::vector<NPCID> m_npcs;
	std::vector<LevelArrow> m_arrows;
	std::vector<LevelBaddy> m_baddies;
	std::vector<LevelBomb> m_bombs;
	std::vector<LevelChest> m_chests;
	std::vector<LevelExplosion> m_explosions;
	std::vector<LevelHorse> m_horses;
	std::vector<LevelItem> m_items;
	std::vector<LevelLink> m_links;
	std::vector<LevelShoot> m_shoots;
	std::vector<LevelSign> m_signs;
};

using LevelPtr = std::shared_ptr<Level>;

//----------------------------

inline void Level::setMap(std::shared_ptr<Map> map)
{
	m_map = map;
}

inline Dimension<uint8_t> Level::getMapSizeInParts() const noexcept
{
	if (m_map == nullptr) return { 1, 1 };
	return m_map->size;
}

inline Dimension<uint32_t> Level::getMapSizeInTiles() const noexcept
{
	auto size = getMapSizeInParts();
	return { static_cast<uint32_t>(size.width() * 64), static_cast<uint32_t>(size.height() * 64) };
}

inline Dimension<uint32_t> Level::getMapSizeInPixels() const noexcept
{
	auto size = getMapSizeInParts();
	return { static_cast<uint32_t>(size.width() * 1024), static_cast<uint32_t>(size.height() * 1024) };
}

inline PixelPosition Level::getMapPixelOffset() const noexcept
{
	return { static_cast<int32_t>(mapPosition.x() * 1024), static_cast<int32_t>(mapPosition.y() * 1024) };
}

inline PixelPosition Level::convertToMapPosition(const LocalPixelPosition& position) const noexcept
{
	if (isOnGmap())
		return { position.x() + static_cast<int32_t>(mapPosition.x() * 1024), position.y() + static_cast<int32_t>(mapPosition.y() * 1024) };
	return { position.x(), position.y() };
}

inline PixelPosition Level::convertToMapPosition(const LocalWholeTilePosition& position) const noexcept
{
	if (isOnGmap())
		return { static_cast<int32_t>((position.x() * 16) + (mapPosition.x() * 1024)), static_cast<int32_t>((position.y() * 16) + (mapPosition.y() * 1024)) };
	return { static_cast<int32_t>(position.x() * 16), static_cast<int32_t>(position.y() * 16) };
}

inline PixelRectangleArea Level::getMapBoundingBox() const noexcept
{
	return { getMapPixelOffset(), { 1024_ui16, 1024_ui16 } };
}

inline const std::string Level::getMapOrLevelName() const noexcept
{
	if (m_map == nullptr || m_map->isBigMap())
		return levelName;

	return m_map->getMapName();
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVEL_H
