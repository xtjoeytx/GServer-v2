#ifndef LEVEL_H
#define LEVEL_H

#include <chrono>
#include <cstdint>
#include <deque>
#include <filesystem>
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
#include <level/LevelSign.h>
#include <level/LevelTiles.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class Player;
class NPC;
class Map;
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
	//! Finds a level with the specified level name and returns it.  If not found, it tries to load it from the disk.
	//! \param pLevelName The name of the level to search for.
	//! \param server The server the level belongs to.
	//! \return A pointer to the level found.
	static std::shared_ptr<Level> findLevel(std::string_view levelName, bool loadAbsolute = false);
	static std::shared_ptr<Level> createLevel(uint16_t fillTile = 511, std::string_view levelName = ""sv);

	//! Returns a clone of the level.
	static std::shared_ptr<Level> clone(LevelPtr level);

public:
	//! Re-loads the level.
	//! \return True if it succeeds in re-loading the level.
	bool reload();

	void saveLevel(const std::string& filename);

	//! Does special events that should happen every second.
	void doTimedEvents();

public:
	//! Gets the original level name.
	//! \return The original level name.
	const auto& getOriginalLevelName() const { return m_originalLevelName; }

	//! Gets a vector full of the players on the level.
	//! \return The players on the level.
	std::deque<PlayerID>& getPlayers() { return m_players; }

	//! Gets a vector full of the level npc ids.
	//! \return The level npcs.
	auto& getNPCs() { return m_npcs; }

	auto& getArrows() { return m_arrows; }
	auto& getBaddies() { return m_baddies; }
	auto& getBombs() { return m_bombs; }
	auto& getChests() { return m_chests; }
	auto& getExplosions() { return m_explosions; }
	auto& getHorses() { return m_horses; }
	auto& getItems() { return m_items; }
	auto& getLinks() { return m_links; }
	auto& getSigns() { return m_signs; }

	//! Gets the raw level tile data.
	//! \return A pointer to all 4096 raw level tiles.
	LevelTiles& getTiles(int layer = 0) { return m_tiles[layer]; }

	//! Gets the tile data for all layers.
	//! \return A map of all the layers and their tile data.
	std::map<uint8_t, LevelTiles> getLayers() const { return m_tiles; }

	//! Gets the gmap this level belongs to.
	//! \return The gmap this level belongs to.
	std::shared_ptr<Map> getMap() const { return m_map.lock(); }

	//! Gets the map x of this level.
	//! \return The map x of this level on the map
	uint8_t getMapX() const { return m_mapX; }

	//! Gets the map y of this level.
	//! \return The map y of this level on the map
	uint8_t getMapY() const { return m_mapY; }

	//! Gets the gmap x of this level or 0 if it doesn't belong to a gmap.
	//! \return The gmap x of this level on the map or 0 if it doesn't belong to a gmap.
	uint8_t getGmapX() const;

	//! Gets the gmap y of this level or 0 if it doesn't belong to a gmap.
	//! \return The gmap y of this level on the map or 0 if it doesn't belong to a gmap.
	uint8_t getGmapY() const;

	//! Gets the status on whether players are on the level.
	//! \return The level has players.  If true, the level has players on it.
	bool hasPlayers() const { return !m_players.empty(); }

	//! Gets if the player is the current level leader.
	//! \param id The player id to check.
	//! \return True if the player is the leader.
	bool isPlayerLeader(PlayerID id) const;

	bool hasLivingBaddies() const;

public:
	CString getBaddyPacket();
	CString getBoardPacket();
	CString getLayerPacket(int i);
	CString getBoardChangesPacket(time_t time);
	CString getBoardChangesPacket2(time_t time);
	void sendChestsToPlayer(std::shared_ptr<Player> player) const;
	void sendHorsesToPlayer(std::shared_ptr<Player> player) const;
	void sendLinksToPlayer(std::shared_ptr<Player> player) const;
	void sendSignsToPlayer(std::shared_ptr<Player> player) const;
	void sendNPCsToPlayer(std::shared_ptr<Player> player, clock::time_point time) const;

public:
	//! Adds a player to the level.
	//! \param player The player to add.
	//! \return The id number of the player in the level.
	int addPlayer(PlayerID id);

	//! Removes a player from the level.
	//! \param player The player to remove.
	void removePlayer(PlayerID id);

public:
	//! Adds an NPC to the level.
	//! \param npc NPC to add to the level.
	//! \return True if the NPC was successfully added or false if it already exists in the level.
	bool addNPC(std::shared_ptr<NPC> npc);
	bool addNPC(NPCID npcId);

	//! Removes an NPC from the level.
	//! \param npc The NPC to remove.
	void removeNPC(std::shared_ptr<NPC> npc);
	void removeNPC(NPCID npcId);

public:
	//! Adds a board change to the level.
	//! \param pTileData Linear array of Graal-packed tiles.  Starts with the top-left tile, ends with the bottom-right.
	//! \param pX X location of the top-left tile.
	//! \param pY Y location of the top-left tile.
	//! \param pWidth How many tiles wide we are altering.
	//! \param pHeight How many tiles high we are altering.
	//! \param player The player who initiated this board change.
	//! \return True if it succeeds, false if it doesn't.
	bool alterBoard(CString& tileData, const Rectangle<uint8_t, uint8_t>& area, Player* player);

public:
	bool addArrow();

public:
	LevelBaddy* addBaddy(const PixelPosition& position, BaddyType type);
	LevelBaddy* putNewBaddy(const PixelPosition& position, BaddyType type);
	LevelBaddy* putNewBaddy(const PixelPosition& position, BaddyType type, uint8_t power, std::string_view image = {});
	bool removeBaddy(uint8_t pId);
	bool removeAllBaddies();
	LevelBaddy* getBaddy(uint8_t id);

public:
	LevelBomb* addBomb(inform_client_t, const PixelPosition& position, uint8_t power);
	LevelBomb* addBomb(const PixelPosition& position, uint8_t power);
	bool removeBomb(inform_client_t, size_t index);
	bool removeBomb(size_t index);
	bool removeBomb(const PixelPosition& position);
	LevelBomb* getBomb(size_t index);

public:
	LevelChest* addChest(const WholeTilePosition& position, const LevelItemType itemType, const int signIndex);
	bool removeChest(size_t index);
	LevelChest* getChest(size_t index);
	std::optional<const LevelChest*> getChest(const WholeTilePosition& position) const;
	CString getChestStr(LevelChest* chest) const;

public:
	void addExplosion(inform_client_t, const PixelPosition& position, uint8_t radius, uint8_t power);
	void addExplosion(const PixelPosition& position, uint8_t radius, uint8_t power);
	void addSpyFire(const PixelPosition& position, uint8_t direction, uint8_t length, uint8_t power);
	LevelExplosion* addExplosionPart(const PixelPosition& position, uint8_t direction, uint8_t power);
	bool removeExplosion(size_t index);
	bool removeExplosion(const PixelPosition& position);
	LevelExplosion* getExplosion(size_t index);

public:
	LevelHorse* addHorse(inform_client_t, std::string_view image, const PixelPosition& position, uint8_t direction, uint8_t bushes);
	LevelHorse* addHorse(std::string_view image, const PixelPosition& position, uint8_t direction, uint8_t bushes);
	bool removeHorse(inform_client_t, size_t index);
	bool removeHorse(size_t index);
	bool removeHorse(const PixelPosition& position);
	LevelHorse* getHorse(size_t index);

public:
	LevelItem* addItem(inform_client_t, const PixelPosition& position, LevelItemType item);
	LevelItem* addItem(const PixelPosition& position, LevelItemType item);
	bool removeItem(inform_client_t, size_t index);
	bool removeItem(size_t index);
	LevelItemType removeItem(const PixelPosition& position);
	LevelItem* getItem(size_t index);

public:
	LevelLink* addLink();
	LevelLink* addLink(const std::vector<CString>& pLink);
	bool removeLink(uint32_t index);
	std::optional<const LevelLink*> getLink(const WholeTilePosition& position, bool excludeOverworld = false) const;

public:
	LevelSign* addSign(const WholeTilePosition& position, const CString& sign, bool encoded = false);
	bool removeSign(uint32_t index);
	LevelSign* getSign(size_t index);

public:
	void setMap(std::weak_ptr<Map> pMap, int pMapX = 0, int pMapY = 0);

public:
	bool isOnWall(const Position<uint8_t>& tilePosition);
	bool isOnWall2(const Rectangle<uint8_t, uint8_t>& tileArea, uint8_t flags = 0);
	bool isOnWater(const Position<uint8_t>& tilePosition);
	bool isOnPlayer(const Position<uint8_t>& tilePosition);
	bool isOnPlayer(const Rectangle<uint8_t, uint8_t>& tileArea);

	std::vector<NPCID> findIntersectingNPCs(const Position<int16_t>& position, bool includeInvisible = false);
	std::vector<NPCID> findIntersectingNPCs(const Rectangle<int16_t, uint16_t>& area, bool includeInvisible = false);
	std::vector<NPCID> findIntersectingNPCsForCollision(const Position<int16_t>& position);
	std::vector<NPCID> findIntersectingNPCsForCollision(const Rectangle<int16_t, uint16_t>& area);

public:
	std::string levelName;
	bool isSparringZone = false;
	bool isNoPkZone = false;
	bool isSingleplayer = false;
	clock::time_point modTime;
	ScriptContainer scripting;

private:
	uint8_t m_mapX = 0;
	uint8_t m_mapY = 0;
	std::map<uint8_t, LevelTiles> m_tiles;
	std::weak_ptr<Map> m_map;
	std::string m_originalLevelName;
	std::string m_fileVersion;
	std::filesystem::path m_filePath;

	std::vector<LevelBaddy> m_baddies;
	std::vector<LevelBoardChange> m_boardChanges;
	std::vector<LevelArrow> m_arrows;
	std::vector<LevelBomb> m_bombs;
	std::vector<LevelChest> m_chests;
	std::vector<LevelExplosion> m_explosions;
	std::vector<LevelHorse> m_horses;
	std::vector<LevelItem> m_items;
	std::vector<LevelLink> m_links;
	std::vector<LevelSign> m_signs;
	std::vector<NPCID> m_npcs;
	std::deque<PlayerID> m_players;
};

using LevelPtr = std::shared_ptr<Level>;

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVEL_H
