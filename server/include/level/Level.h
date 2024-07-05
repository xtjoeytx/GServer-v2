#ifndef TLEVEL_H
#define TLEVEL_H

#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

#include <CString.h>
#include <IUtil.h>

#include "level/LevelBaddy.h"
#include "level/LevelBoardChange.h"
#include "level/LevelChest.h"
#include "level/LevelHorse.h"
#include "level/LevelItem.h"
#include "level/LevelLink.h"
#include "level/LevelSign.h"
#include "level/LevelTiles.h"
#include "utilities/IdGenerator.h"


// Starting baddy id.  Baddy id 0 breaks the client so always start here.
constexpr uint8_t BADDYID_INIT = 1;


class Server;
class Player;
class NPC;
class Map;

class Level : public std::enable_shared_from_this<Level>
{
public:
	//! Destructor.
	~Level();

	//! Finds a level with the specified level name and returns it.  If not found, it tries to load it from the disk.
	//! \param pLevelName The name of the level to search for.
	//! \param server The server the level belongs to.
	//! \return A pointer to the level found.
	static std::shared_ptr<Level> findLevel(const CString& pLevelName, Server* server, bool loadAbsolute = false);
	static std::shared_ptr<Level> createLevel(Server* server, short fillTile = 511, const std::string& levelName = "");

	//! Re-loads the level.
	//! \return True if it succeeds in re-loading the level.
	bool reload();

	void saveLevel(const std::string& filename);

	//! Returns a clone of the level.
	std::shared_ptr<Level> clone() const;

	// get crafted packets
	CString getBaddyPacket(int clientVersion = CLVER_2_17);
	CString getBoardPacket();
	CString getLayerPacket(int i);
	CString getBoardChangesPacket(time_t time);
	CString getBoardChangesPacket2(time_t time);
	CString getChestPacket(Player* pPlayer);
	CString getHorsePacket();
	CString getLinksPacket();
	CString getNpcsPacket(time_t time, int clientVersion = CLVER_2_17);
	CString getSignsPacket(Player* pPlayer);

	//! Gets the actual level name.
	//! \return The actual level name.
	CString getActualLevelName() const { return m_actualLevelName; }

	//! Gets the level name.
	//! \return The level name.
	CString getLevelName() const { return m_levelName; }

	//! Sets the level name.
	//! \param pLevelName The new name of the level.
	void setLevelName(CString pLevelName) { m_levelName = pLevelName; }

	//! Gets the raw level tile data.
	//! \return A pointer to all 4096 raw level tiles.
	LevelTiles& getTiles(int layer = 0) { return m_tiles[layer]; }

	//! Gets the level mod time.
	//! \return The modified time of the level when it was first loaded from the disk.
	time_t getModTime() const { return m_modTime; }

	//! Gets a vector full of all the level chests.
	//! \return The level chests.
	std::vector<LevelChestPtr>& getChests() { return m_chests; }

	//! Gets a vector full of the level npc ids.
	//! \return The level npcs.
	std::set<uint32_t>& getNPCs() { return m_npcs; }

	//! Gets a vector full of the level signs.
	//! \return The level signs.
	std::vector<LevelSignPtr>& getSigns() { return m_signs; }

	//! Gets a vector full of the level links.
	//! \return The level links.
	std::vector<LevelLinkPtr>& getLinks() { return m_links; }

	//! Gets the gmap this level belongs to.
	//! \return The gmap this level belongs to.
	std::shared_ptr<Map> getMap() const { return m_map.lock(); }

	//! Gets the map x of this level.
	//! \return The map x of this level on the map
	int getMapX() const { return m_mapX; }

	//! Gets the map y of this level.
	//! \return The map y of this level on the map
	int getMapY() const { return m_mapY; }

	//! Gets a vector full of the players on the level.
	//! \return The players on the level.
	std::deque<uint16_t>& getPlayers() { return m_players; }

	//! Gets the server this level belongs to.
	//! \return The server this level belongs to.
	Server* getServer() const { return m_server; }

	std::map<uint8_t, LevelTiles> getLayers() const { return m_tiles; }

	//! Gets the status on whether players are on the level.
	//! \return The level has players.  If true, the level has players on it.
	bool hasPlayers() const { return !m_players.empty(); }

	//! Gets the sparring zone status of the level.
	//! \return The sparring zone status.  If true, the level is a sparring zone.
	bool isSparringZone() const { return m_isSparringZone; }

	//! Sets the sparring zone status of the level.
	//! \param pLevelSpar If true, the level becomes a sparring zone level.
	void setSparringZone(bool pLevelSpar) { m_isSparringZone = pLevelSpar; }

	//! Gets the singleplayer status of the level.
	//! \return The singleplayer status.  If true, the level is singleplayer.
	bool isSingleplayer() const { return m_isSingleplayer; }

	//! Sets the singleplayer status of the level.
	//! \param pLevelSingleplayer If true, the level becomes a singleplayer level.
	void setSingleplayer(bool pLevelSingleplayer) { m_isSingleplayer = pLevelSingleplayer; }

	//! Adds a board change to the level.
	//! \param pTileData Linear array of Graal-packed tiles.  Starts with the top-left tile, ends with the bottom-right.
	//! \param pX X location of the top-left tile.
	//! \param pY Y location of the top-left tile.
	//! \param pWidth How many tiles wide we are altering.
	//! \param pHeight How many tiles high we are altering.
	//! \param player The player who initiated this board change.
	//! \return True if it succeeds, false if it doesn't.
	bool alterBoard(CString& pTileData, int pX, int pY, int pWidth, int pHeight, Player* player);

	//! Adds an item to the level.
	//! \param pX X location of the item to add.
	//! \param pY Y location of the item to add.
	//! \param pItem The item we are adding.  Use LevelItem::getItemId() to get the item type from an item name.
	//! \return True if it succeeds, false if it doesn't.
	bool addItem(float pX, float pY, LevelItemType pItem);

	//! Removes an item from the level.
	//! \param pX X location of the item to remove.
	//! \param pY Y location of the item to remove.
	//! \return The type of item removed.  Use LevelItem::getItemId() to get the item type from an item name.
	LevelItemType removeItem(float pX, float pY);

	//! Adds a new horse to the level.
	//! \param pImage The image of the horse.
	//! \param pX X location of the horse.
	//! \param pY Y location of the horse.
	//! \param pDir The direction of the horse.
	//! \param pBushes The bushes the horse has eaten.
	//! \return Returns true if it succeeds.
	bool addHorse(CString& pImage, float pX, float pY, char pDir, char pBushes);

	//! Removes a horse from the level.
	//! \param pX X location of the horse to remove.
	//! \param pY Y location of the horse to remove.
	void removeHorse(float pX, float pY);

	//! Adds a baddy to the level.
	//! \param pX X location of the baddy to add.
	//! \param pY Y location of the baddy to add.
	//! \param pType The type of baddy to add.
	//! \return A pointer to the new LevelBaddy.
	LevelBaddy* addBaddy(float pX, float pY, char pType);

	//! Removes a baddy from the level.
	//! \param pId ID of the baddy to remove.
	void removeBaddy(uint8_t pId);

	//! Finds a baddy by the specified id number.
	//! \param pId The ID number of the baddy to find.
	//! \return A pointer to the found LevelBaddy.
	LevelBaddy* getBaddy(uint8_t id);

	//! Adds a player to the level.
	//! \param player The player to add.
	//! \return The id number of the player in the level.
	int addPlayer(uint16_t id);

	//! Removes a player from the level.
	//! \param player The player to remove.
	void removePlayer(uint16_t id);

	//! Gets if the player is the current level leader.
	//! \param id The player id to check.
	//! \return True if the player is the leader.
	bool isPlayerLeader(uint16_t id);

	//! Adds an NPC to the level.
	//! \param npc NPC to add to the level.
	//! \return True if the NPC was successfully added or false if it already exists in the level.
	bool addNPC(std::shared_ptr<NPC> npc);
	bool addNPC(uint32_t npcId);

	//! Adds a level link to the level.
	//! \return A pointer to the new LevelLink.
	LevelLink* addLink();

	//! Adds a level link to the level.
	//! \param pLink link string to parse
	//! \return A pointer to the new LevelLink.
	LevelLink* addLink(const std::vector<CString>& pLink);

	//! Removes a level link from the level.
	//! \param index link index to remove
	//! \return true if removed, false otherwise
	bool removeLink(uint32_t index);

	//! Adds a level sign to the level.
	//! \param pX x position
	//! \param pY y position
	//! \param pSign sign text
	//! \param encoded true if the sign text is encoded
	//! \return A pointer to the new LevelSign.
	LevelSign* addSign(const int pX, const int pY, const CString& pSign, bool encoded = false);

	//! Adds a level sign to the level.
	//! \param index x position
	//! \return true if removed, false otherwise.
	bool removeSign(uint32_t index);

	//! Adds a level chest to the level.
	//! \param pX x position
	//! \param pY y position
	//! \param itemType which type of item the chest contains
	//! \param signIndex signIndex of sign to pop when chest is opened
	//! \return A pointer to the new LevelChest.
	LevelChest* addChest(const int pX, const int pY, const LevelItemType itemType, const int signIndex);

	//! Adds a level chest to the level.
	//! \param index x position
	//! \return true if removed, false otherwise.
	bool removeChest(uint32_t index);

	//! Removes an NPC from the level.
	//! \param npc The NPC to remove.
	void removeNPC(std::shared_ptr<NPC> npc);
	void removeNPC(uint32_t npcId);

	//! Sets the map for the current level.
	//! \param pMap Map the level is on.
	//! \param pMapX X location on the map.
	//! \param pMapY Y location on the map.
	void setMap(std::weak_ptr<Map> pMap, int pMapX = 0, int pMapY = 0);

	//! Does special events that should happen every second.
	//! \return Currently, it always returns true.
	bool doTimedEvents();

	bool isOnWall(int pX, int pY);
	bool isOnWall2(int pX, int pY, int pWidth, int pHeight, uint8_t flags = 0);
	bool isOnWater(int pX, int pY);
	std::optional<LevelChest*> getChest(int x, int y) const;
	std::optional<LevelLink*> getLink(int pX, int pY) const;
	CString getChestStr(LevelChest* chest) const;

#ifdef V8NPCSERVER
	std::vector<NPC*> findAreaNpcs(int pX, int pY, int pWidth, int pHeight);
	std::vector<NPC*> testTouch(int pX, int pY);
	NPC* isOnNPC(float pX, float pY, bool checkEventFlag = false);
	void sendChatToLevel(const Player* player, const std::string& message);

	IScriptObject<Level>* getScriptObject() const;
	void setScriptObject(std::unique_ptr<IScriptObject<Level>> object);
#endif

	void modifyBoardDirect(uint32_t index, short tile);

private:
	Level(Server* pServer);
	Level(short fillTile = 0xFF, Server* pServer = nullptr);

	// level-loading functions
	bool loadLevel(const CString& pLevelName);
	bool detectLevelType(const CString& pLevelName);
	bool loadGraal(const CString& pLevelName);
	bool loadZelda(const CString& pLevelName);
	bool loadNW(const CString& pLevelName);

private:
	Server* m_server;
	time_t m_modTime;
	bool m_isSparringZone;
	bool m_isSingleplayer;
	std::map<uint8_t, LevelTiles> m_tiles;
	int m_mapX, m_mapY;
	std::weak_ptr<Map> m_map;
	CString m_fileName, m_fileVersion, m_actualLevelName, m_levelName;

	std::unordered_map<uint8_t, LevelBaddyPtr> m_baddies;
	IdGenerator<uint8_t> m_baddyIdGenerator{ BADDYID_INIT };

	std::vector<LevelBoardChange> m_boardChanges;
	std::vector<LevelChestPtr> m_chests;
	std::vector<LevelHorse> m_horses;
	std::vector<LevelItem> m_items;
	std::vector<LevelLinkPtr> m_links;
	std::vector<LevelSignPtr> m_signs;
	std::set<uint32_t> m_npcs;
	std::deque<uint16_t> m_players;

#ifdef V8NPCSERVER
	std::unique_ptr<IScriptObject<Level>> m_scriptObject;
#endif
};

using LevelPtr = std::shared_ptr<Level>;

#ifdef V8NPCSERVER

inline IScriptObject<Level>* Level::getScriptObject() const
{
	return m_scriptObject.get();
}

inline void Level::setScriptObject(std::unique_ptr<IScriptObject<Level>> object)
{
	m_scriptObject = std::move(object);
}

#endif

#endif // TLEVEL_H
