#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <list>
#include <memory>
#include <optional>
#include <ostream>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <BabyDI.h>
#include <CString.h>
#include <IEnums.h>
#include <IUtil.h>

#include <FileSystem.h>
#include <Server.h>
#include <level/Level.h>
#include <level/LevelBaddy.h>
#include <level/LevelBoardChange.h>
#include <level/LevelChest.h>
#include <level/LevelHorse.h>
#include <level/LevelItem.h>
#include <level/LevelLink.h>
#include <level/LevelSign.h>
#include <level/LevelTiles.h>
#include <level/Map.h>
#include <level/tiletypes.h>
#include <loader/LevelLoader.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

/*
	Global Variables
*/
//CString base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
short respawningTiles[] = {
	0x1ff,
	0x3ff,
	0x2ac,
	0x002,
	0x200,
	0x022,
	0x3de,
	0x1a4,
	0x14a,
	0x674,
	0x72a,
};

/*
	Level: Constructor - Deconstructor
*/
Level::Level(short fillTile)
{
	m_tiles[0] = LevelTiles(fillTile);
}

Level::~Level()
{
	auto server = BabyDI::Get<Server>();

	// Delete NPCs.
	{
		// Remove every NPC in the level.
		for (auto& levelNPC: m_npcs)
		{
			// TODO(joey): we need to delete putnpc's, and move db-npcs to a different level
			if (auto npc = server->getNPC(levelNPC); npc && npc->storageType == NPCStorageType::LEVEL)
				server->deleteNPC(npc, false);
		}
		m_npcs.clear();
	}

	// Delete baddies.
	m_baddies.clear();

	// Delete chests.
	m_chests.clear();

	// Delete links.
	m_links.clear();

	// Delete signs.
	m_signs.clear();

	// Delete items.
	for (auto& item: m_items)
	{
		CString packet = CString() >> (char)PLO_ITEMDEL >> (char)(item.getX() * 2) >> (char)(item.getY() * 2);
		for (auto& player: m_players)
		{
			if (auto p = server->getPlayer(player); p)
				p->sendPacket(packet);
		}
	}
	m_items.clear();

	// Delete board changes.
	m_boardChanges.clear();

	// TODO: Warp players out?
}

/*
	Level: Get Crafted Packets
*/
CString Level::getBaddyPacket(int clientVersion)
{
	CString retVal;
	for (const auto& baddy: m_baddies)
	{
		if (baddy == nullptr)
			continue;

		retVal >> (char)PLO_BADDYPROPS >> (char)baddy->id << baddy->getProps(clientVersion) << "\n";
	}
	return retVal;
}

CString Level::getBoardPacket()
{
	CString retVal;
	retVal.writeGChar(PLO_BOARDPACKET);
	retVal.write((char*)m_tiles[0], sizeof(short[4096]));
	retVal << "\n";

	return retVal;
}

CString Level::getLayerPacket(int layer)
{
	CString retVal;
	retVal.writeGChar(PLO_BOARDLAYER);

	// TODO: Only send the tiles that has been placed on the layer
	retVal << (char)layer << (char)0 << (char)0 << (char)64 << (char)64;
	retVal.write((char*)m_tiles[layer], sizeof(short[4096]));
	retVal << "\n";

	return retVal;
}

CString Level::getBoardChangesPacket(time_t time)
{
	CString retVal;
	retVal >> (char)PLO_LEVELBOARD;
	for (const auto& change: m_boardChanges)
	{
		if (change.getModTime() >= time)
			retVal << change.getBoardStr();
	}
	return retVal;
}

CString Level::getBoardChangesPacket2(time_t time)
{
	CString retVal;
	retVal >> (char)PLO_BOARDMODIFY;
	for (const auto& change: m_boardChanges)
	{
		if (change.getModTime() >= time)
			retVal << change.getBoardStr();
	}
	return retVal;
}

CString Level::getChestPacket(Player* pPlayer)
{
	CString retVal;

	if (pPlayer)
	{
		for (auto& chest: m_chests)
		{
			bool hasChest = pPlayer->account.hasChest(m_levelName.toStringView(), chest->getX(), chest->getY());

			retVal >> (char)PLO_LEVELCHEST >> (char)(hasChest ? 1 : 0) >> (char)chest->getX() >> (char)chest->getY();
			if (!hasChest) retVal >> (char)chest->getItemIndex() >> (char)chest->getSignIndex();
			retVal << "\n";
		}
	}

	return retVal;
}

CString Level::getHorsePacket()
{
	CString retVal;
	for (auto& horse: m_horses)
	{
		retVal >> (char)PLO_HORSEADD << horse.getHorseStr() << "\n";
	}

	return retVal;
}

CString Level::getLinksPacket()
{
	CString retVal;
	for (const auto& link: m_links)
	{
		retVal >> (char)PLO_LEVELLINK << link->getLinkStr() << "\n";
	}

	return retVal;
}

// TODO: Replace with a function in server that sends npc props from a list of ids.
void Level::sendNPCsToPlayer(std::shared_ptr<Player> player, clock::time_point time)
{
	auto server = BabyDI::Get<Server>();
	for (const auto& npcId : m_npcs)
	{
		auto npc = server->getNPC(npcId);
		if (!npc) continue;

		auto packet = npc->getAllPropsPacket(time);
		if (packet.isEmpty())
			continue;

		player->sendPacket(CString() >> (char)PLO_NPCPROPS >> (int)npc->id << packet);
		if (player->getVersion() >= CLVER_4_0211 && !npc->getScript().getClientByteCode().empty())
		{
			CString byteCodePacket = CString() >> (char)PLO_NPCBYTECODE >> (int)npc->id;
			byteCodePacket.write(reinterpret_cast<const char*>(npc->getScript().getClientByteCode().data()), npc->getScript().getClientByteCode().size());
			player->sendPacket(CString() >> (char)PLO_RAWDATA >> (int)byteCodePacket.length());
			player->sendPacket(byteCodePacket);
		}
	}
}

CString Level::getSignsPacket(Player* pPlayer = 0)
{
	CString retVal;
	for (const auto& sign: m_signs)
	{
		retVal >> (char)PLO_LEVELSIGN << sign->getSignStr(pPlayer) << "\n";
	}
	return retVal;
}

uint8_t Level::getGmapX() const
{
	if (auto map = m_map.lock(); map && map->isGmap())
		return m_mapX;
	return 0;
}

uint8_t Level::getGmapY() const
{
	if (auto map = m_map.lock(); map && map->isGmap())
		return m_mapY;
	return 0;
}

/*
	Level: Level-Loading Functions
*/
bool Level::reload()
{
	auto server = BabyDI::Get<Server>();

	// Delete NPCs.
	// Don't delete NPCs if this level is on a gmap!  If we are on a gmap, just set them
	// back to their original positions.
	{
		// Remove every NPC in the level.
		for (auto it = m_npcs.begin(); it != m_npcs.end();)
		{
			auto npc = server->getNPC(*it);
			if (!npc || npc->storageType == NPCStorageType::LEVEL)
			{
				server->deleteNPC(npc, false);
				it = m_npcs.erase(it);
			}
			else
			{
				it++;
			}
		}
	}

	// Kill off all the baddies and disable respawn.
	for (size_t i = 0; i < m_baddies.size(); ++i)
	{
		m_baddies[i]->setRespawn(false);
		m_baddies[i]->mode = BaddyMode::DEAD;
	}

	// Delete chests.
	m_chests.clear();

	// Delete links.
	m_links.clear();

	// Delete signs.
	m_signs.clear();

	// Delete items.
	for (const auto& item: m_items)
	{
		CString packet = CString() >> (char)PLO_ITEMDEL >> (char)(item.getX() * 2) >> (char)(item.getY() * 2);
		for (auto& playerId: m_players)
		{
			if (auto player = server->getPlayer(playerId); player)
				player->sendPacket(packet);
		}
	}
	m_items.clear();

	// Delete board changes.
	m_boardChanges.clear();

	// Clean up the rest.
	m_isSparringZone = false;
	m_isSingleplayer = false;

	// Remove all the players from the level.
	std::deque<PlayerID> oldplayers = m_players;
	for (auto& id: oldplayers)
	{
		if (auto p = server->getPlayer<PlayerClient>(id); p)
			p->leaveLevel(true);
	}

	// Reset the level cache for all the players on the server.
	auto& playerList = server->getPlayerList();
	for (const auto& [id, p]: players_of_type<PlayerClient>(playerList))
	{
		p->resetLevelCache(this);
	}

	// Re-load the level now.
	auto level = LevelLoader::loadLevelInto(shared_from_this(), std::filesystem::path{ m_levelName.toStringView() });
	bool ret = level != nullptr;

	// Warp all players back to the level (or to unstick me if loadLevel failed).
	CString uLevel = server->getSettings().getStr("unstickmelevel", "onlinestartlocal.nw");
	float uX = server->getSettings().getFloat("unstickmex", 30.0f);
	float uY = server->getSettings().getFloat("unstickmey", 35.0f);
	for (auto& id: oldplayers)
	{
		if (auto p = server->getPlayer<PlayerClient>(id); p)
			p->warp((ret ? m_levelName : uLevel), (ret ? p->getX() : uX), (ret ? p->getY() : uY));
	}

	return ret;
}

std::shared_ptr<Level> Level::clone() const
{
	return LevelLoader::loadLevel(std::filesystem::path{ m_levelName.toStringView() });
}

/*
	Level: Find Level
*/
std::shared_ptr<Level> Level::findLevel(const CString& pLevelName, bool loadAbsolute)
{
	auto server = BabyDI::Get<Server>();
	auto& levelList = server->getLevelList();

	// TODO(joey): Maybe its time for a hashmap, even if a duplicate level name occurs
	// 	this is still going to break on the first occurrence.

	// Find Appropriate Level by Name
	CString levelName = pLevelName.toLower();
	if (auto it = levelList.find(levelName.toStringView()); it != levelList.end())
		return it->second;

	if (loadAbsolute)
	{
		FileSystem* fileSystem = server->getFileSystem();
		if (!server->getSettings().getBool("nofoldersconfig", false))
			fileSystem = server->getFileSystem(FS_LEVEL);

		if (fileSystem->find(pLevelName).trim().length() == 0)
		{
			fileSystem->addFile(pLevelName);
			fileSystem->addDir(getPath(pLevelName), "*", true);
		}
	}

	// Load New Level
	auto level = LevelLoader::loadLevel(std::filesystem::path{ pLevelName.toStringView() });
	if (level == nullptr)
		return nullptr;

	auto& mapList = server->getMapList();
	for (const auto& map: mapList)
	{
		int mx, my;
		if (map->isLevelOnMap(levelName.text(), mx, my))
		{
			level->setMap(map, mx, my);
			break;
		}
	}

	// Return Level
	levelList.insert(std::make_pair(levelName.toString(), level));
	return level;
}

/*
	Level: Create Level
*/
std::shared_ptr<Level> Level::createLevel(short fillTile, const std::string& levelName)
{
	auto server = BabyDI::Get<Server>();
	auto& levelList = server->getLevelList();

	// Load New Level
	auto level = std::shared_ptr<Level>(new Level(fillTile));
	level->setLevelName(levelName);

	// Return Level
	levelList.insert(std::make_pair(string::toLower(levelName), level));
	return level;
}

/*
	Level: Save Level
*/
void Level::saveLevel(const std::string& filename)
{
	auto server = BabyDI::Get<Server>();
	FileSystem* fileSystem = server->getFileSystem();
	if (!server->getSettings().getBool("nofoldersconfig", false))
		fileSystem = server->getFileSystem(FS_LEVEL);

	auto actualFilename = getFilename(filename);

	auto path = fileSystem->findi(actualFilename);

	if (path == "")
	{
		path << fileSystem->getDirByExtension(getExtension(actualFilename).text());
		path << actualFilename;

		fileSystem->addFile(path);
	}

	std::ofstream fileStream(path.text());

	fileStream << "GLEVNW01" << std::endl;

	// white space separator
	std::string s = " ";
	// write tiles
	for (int layer = 0; layer < getLayers().size(); layer++)
	{
		auto& tiles = getTiles(layer);
		for (int y = 0; y < 64 /*tiles.get_height()*/; y++)
		{
			std::string data;
			// chunk start, chunk data pairs
			std::list<std::pair<int, std::string>> chunks;
			/* Separate each row into chunks of actually non-transparent tiles.
			 * Every time we encounter a transparent tile, flush the current data
			 * into the chunk list and clear it. If we never encounter a transparent
			 * tile, flush the entire data after the loop */
			int currentStart = 0;
			for (int x = 0; x < 64 /*tiles.get_width()*/; x++)
			{
				auto tile = tiles[x + static_cast<size_t>(y) * 64];
				if (tile == -2)
				{
					if (!data.empty())
					{
						chunks.emplace_back(currentStart, data);
						currentStart = x;
						data.clear();
					}

					// Skip transparent tile
					currentStart++;
					continue;
				}

				data += CString::formatBase64(tile);
			}
			if (!data.empty())
				chunks.emplace_back(currentStart, data);

			/* Draw one BOARD entry for each chunk so transparent tile-data is culled */
			for (const auto& chunk: chunks)
			{
				fileStream << "BOARD" << s << chunk.first << s << y << s << chunk.second.length() / 2 << s << layer // x, y, width, layer
						   << s << chunk.second << std::endl;
			}
		}
	}

	for (const auto& link: getLinks())
	{
		auto& bbox = link->getBoundingBox();
		fileStream
			<< std::format("LINK {} {} {} {} {} {} {}",
				link->getDestinationLevel(),
				bbox.position.x(), bbox.position.y(),
				bbox.size.width(), bbox.size.height(),
				link->getDestinationX(), link->getDestinationY()
			)
			<< std::endl;
	}

	for (const auto& sign: getSigns())
	{
		fileStream << "SIGN" << s << sign->getX() << s << sign->getY() << std::endl;
		fileStream << sign->getUText().text() << std::endl;
		fileStream << "SIGNEND" << std::endl;
	}

	for (const auto& chest: getChests())
	{
		fileStream << "CHEST" << s << chest->getX() << s << chest->getY() << s << LevelItem::getItemName(chest->getItemIndex()) << s << chest->getSignIndex() << std::endl;
	}

	for (const auto& baddy: m_baddies)
	{
		fileStream << "BADDY" << s << baddy->x << s << baddy->y << s << PROPID(baddy->type) << std::endl;

		for (const auto& verse: baddy->verses)
		{
			fileStream << verse << std::endl;
		}

		fileStream << "BADDYEND" << std::endl;
	}

	for (const auto& npcId: getNPCs())
	{
		auto npc = server->getNPC(npcId);

		// Don't save PUTNPC's or DBNPC's in the level file
		if (npc->storageType != NPCStorageType::LEVEL)
			continue;

		std::string image = npc->image;

		if (image.empty())
			image = "-"; // No image is represented by "-"

		fileStream << "NPC" << s << image << s << (npc->character.pixelX / 16.0f) << s << (npc->character.pixelY / 16.0f) << std::endl;
		fileStream << npc->getScript().getOriginalSource() << std::endl;
		fileStream << "NPCEND" << std::endl;
	}
}

bool Level::alterBoard(CString& pTileData, int pX, int pY, int pWidth, int pHeight, Player* player)
{
	if (pX < 0 || pY < 0 || pX > 63 || pY > 63 ||
		pWidth < 1 || pHeight < 1 ||
		pX + pWidth > 64 || pY + pHeight > 64)
		return false;

	auto server = BabyDI::Get<Server>();
	auto& settings = server->getSettings();

	// Do the check for the push-pull block.
	if (pWidth == 4 && pHeight == 4 && settings.getBool("clientsidepushpull", true))
	{
		// Try to find the top-left corner tile.
		int i;
		for (i = 0; i < 16; ++i)
		{
			short stoneCheck = pTileData.readGShort();
			if (stoneCheck == 0x06E4 || stoneCheck == 0x07CE)
				break;
		}

		// Check if we found a possible push-pull block.
		if (i != 16 && i < 11)
		{
			// Go back one full short so the first readByte2() returns the top-left corner.
			pTileData.setRead(i * 2);

			int foundCount = 0;
			for (int j = 0; j < 6; ++j)
			{
				// Read a piece.
				short stoneCheck = pTileData.readGShort();

				// A valid stone will have pieces at the following j locations.
				if (j == 0 || j == 1 || j == 4 || j == 5)
				{
					switch (stoneCheck)
					{
						// red
						case 0x6E4:
						case 0x6E5:
						case 0x6F4:
						case 0x6F5:
						// blue
						case 0x7CE:
						case 0x7CF:
						case 0x7DE:
						case 0x7DF:
							foundCount++;
							break;
					}
				}
			}
			pTileData.setRead(0);

			// Check if we found a full tile.  If so, don't accept the change.
			if (foundCount == 4)
			{
				player->sendPacket(CString() >> (char)PLO_BOARDMODIFY >> (char)pX >> (char)pY >> (char)pWidth >> (char)pHeight << pTileData);
				return false;
			}
		}
	}

	// Delete any existing changes within the same region.
	for (auto i = m_boardChanges.begin(); i != m_boardChanges.end();)
	{
		LevelBoardChange& change = *i;
		if ((change.getX() >= pX && change.getX() + change.getWidth() <= pX + pWidth) &&
			(change.getY() >= pY && change.getY() + change.getHeight() <= pY + pHeight))
		{
			i = m_boardChanges.erase(i);
		}
		else
			++i;
	}

	// Check if the tiles should be respawned.
	// Only tiles in the respawningTiles array are allowed to respawn.
	// These are things like signs, bushes, pots, etc.
	int respawnTime = settings.getInt("respawntime", 15);
	bool doRespawn = false;
	short testTile = m_tiles[0][pX + (static_cast<size_t>(pY) * 64)];
	int tileCount = sizeof(respawningTiles) / sizeof(short);
	for (int i = 0; i < tileCount; ++i)
		if (testTile == respawningTiles[i]) doRespawn = true;

	// Grab old tiles for the respawn.
	CString oldTiles;
	if (doRespawn)
	{
		for (int j = pY; j < pY + pHeight; ++j)
		{
			for (int i = pX; i < pX + pWidth; ++i)
				oldTiles.writeGShort(m_tiles[0][i + (static_cast<size_t>(j) * 64)]);
		}
	}

	// TODO: old gserver didn't save the board change if oldTiles.length() == 0.
	// Should we do it that way still?
	m_boardChanges.push_back(LevelBoardChange(pX, pY, pWidth, pHeight, pTileData, oldTiles, (doRespawn ? respawnTime : -1)));
	return true;
}

bool Level::addItem(float pX, float pY, LevelItemType pItem)
{
	// TODO(NPCSERVER): Gralat NPC.
	m_items.push_back(LevelItem(pX, pY, pItem));
	return true;
}

LevelItemType Level::removeItem(float pX, float pY)
{
	for (auto i = m_items.begin(); i != m_items.end(); ++i)
	{
		LevelItem& item = *i;
		if (item.getX() == pX && item.getY() == pY)
		{
			LevelItemType itemType = item.getItem();
			m_items.erase(i);
			return itemType;
		}
	}

	return LevelItemType::INVALID;
}

bool Level::addHorse(CString& pImage, float pX, float pY, char pDir, char pBushes)
{
	auto server = BabyDI::Get<Server>();
	auto horseLife = server->getSettings().getInt("horselifetime", 30);
	m_horses.push_back(LevelHorse(horseLife, pImage, pX, pY, pDir, pBushes));
	return true;
}

void Level::removeHorse(float pX, float pY)
{
	for (auto it = m_horses.begin(); it != m_horses.end(); ++it)
	{
		LevelHorse& horse = *it;
		if (horse.getX() == pX && horse.getY() == pY)
		{
			m_horses.erase(it);
			return;
		}
	}
}

LevelBaddy* Level::addBaddy(float pX, float pY, BaddyType pType)
{
	// Find the next available baddy that can be used.
	size_t nextId = 0;
	for (nextId = 0; nextId < m_baddies.size(); ++nextId)
	{
		if (m_baddies[nextId] == nullptr || m_baddies[nextId]->canBeReplaced())
			break;
	}

	// Limit of 50 baddies per level.
	if ((nextId + 1) > 50)
		return nullptr;

	// New Baddy
	auto newBaddy = std::make_unique<LevelBaddy>(pX, pY, pType, this->shared_from_this());

	// Assign the new id.
	newBaddy->id = nextId + 1;

	auto* baddy = newBaddy.get();
	if (nextId >= m_baddies.size())
		m_baddies.push_back(std::move(newBaddy));
	else
		m_baddies[nextId] = std::move(newBaddy);

	return baddy;
}

LevelBaddy* Level::putNewBaddy(float x, float y, BaddyType type)
{
	auto baddy = addBaddy(x, y, type);
	if (baddy == nullptr)
		return nullptr;

	auto server = BabyDI::Get<Server>();
	CString packet = CString() >> (char)PLO_BADDYPROPS >> (char)baddy->id << baddy->getProps();
	for (auto& player : m_players)
	{
		if (auto p = server->getPlayer(player); p)
			p->sendPacket(packet);
	}

	return baddy;
}

LevelBaddy* Level::putNewBaddy(float x, float y, BaddyType type, uint8_t power, std::string_view image)
{
	auto baddy = addBaddy(x, y, type);
	if (baddy == nullptr)
		return nullptr;

	baddy->setImage(image);
	baddy->power = power;

	auto server = BabyDI::Get<Server>();
	CString packet = CString() >> (char)PLO_BADDYPROPS >> (char)baddy->id << baddy->getProps();
	for (auto& player : m_players)
	{
		if (auto p = server->getPlayer(player); p)
			p->sendPacket(packet);
	}

	return baddy;
}

void Level::removeBaddy(uint8_t pId)
{
	// Don't allow us to remove id 0 or any id over 50.
	if (pId < 1 || pId > 50 || (pId >= m_baddies.size())) return;

	// Find the baddy.
	auto& baddy = m_baddies.at(static_cast<size_t>(pId) - 1);

	// Erase the baddy.
	baddy->mode = BaddyMode::DEAD;
	baddy->setRespawn(false);

	// Set the baddy as dead for all the other players in the level.
	auto server = BabyDI::Get<Server>();
	CString props = CString() >> (char)BaddyProp::MODE >> (char)BaddyMode::DEAD;
	for (const auto& playerId : m_players)
	{
		auto player = server->getPlayer(playerId);
		player->sendPacket(CString() >> (char)PLO_BADDYPROPS >> (char)baddy->id << props);
	}
}

void Level::removeAllBaddies()
{
	auto server = BabyDI::Get<Server>();
	CString propsPacket;
	for (auto& baddy : m_baddies)
	{
		baddy->mode = BaddyMode::DEAD;
		baddy->setRespawn(false);

		// Set the baddy as dead for all the other players in the level.
		propsPacket.clear();
		propsPacket >> (char)PLO_BADDYPROPS >> (char)baddy->id >> (char)BaddyProp::MODE >> (char)BaddyMode::DEAD;
		for (const auto& playerId : m_players)
		{
			auto player = server->getPlayer(playerId);
			player->sendPacket(propsPacket);
		}
	}
}

LevelBaddy* Level::getBaddy(uint8_t id)
{
	if (id >= m_baddies.size())
		return nullptr;

	auto& baddy = m_baddies.at(id);
	return baddy.get();
}

int Level::addPlayer(PlayerID id)
{
	m_players.push_back(id);

	// Set the player enters event on all the NPCs.
	auto server = BabyDI::Get<Server>();
	server->queueNPCEvent(shared_from_this(), ScriptEventType::PLAYERENTERS, source::FromPlayer(id));

	return static_cast<int>(m_players.size() - 1);
}

void Level::removePlayer(PlayerID id)
{
	std::erase(m_players, id);

	// Set the player leaves event on all the NPCs.
	auto server = BabyDI::Get<Server>();
	server->queueNPCEvent(shared_from_this(), ScriptEventType::PLAYERLEAVES, source::FromPlayer(id));
}

bool Level::isPlayerLeader(PlayerID id)
{
	if (m_players.empty())
		return false;
	return m_players.front() == id;
}

bool Level::addNPC(std::shared_ptr<NPC> npc)
{
	m_npcs.push_back(npc->id);

	auto script = string::trimLeft(npc->getScript().getClientSide());

	if (script.starts_with("sparringzone"))
		setSparringZone(true);

	if (script.starts_with("noplayerkilling"))
		setNoPkZone(true);

	if (script.starts_with("singleplayer"))
		setSingleplayer(true);

	/*
	if (npc->isCharacter())
	{
		// Set the player enters event on all the NPCs.
		auto server = BabyDI::Get<Server>();
		server->queueNPCEvent(shared_from_this(), ScriptEventType::PLAYERENTERS, source::FromNPC(npc->id));
	}
	*/

	return true;
}

bool Level::addNPC(NPCID npcId)
{
	auto server = BabyDI::Get<Server>();
	auto npc = server->getNPC(npcId);
	return addNPC(npc);
}

void Level::removeNPC(std::shared_ptr<NPC> npc)
{
	if (npc == nullptr)
		return;

	std::erase(m_npcs, npc->id);

	if (npc->isCharacter())
	{
		// Set the player leaves event on all the NPCs.
		auto server = BabyDI::Get<Server>();
		server->queueNPCEvent(shared_from_this(), ScriptEventType::PLAYERLEAVES, source::FromNPC(npc->id));
	}
}

void Level::removeNPC(NPCID npcId)
{
	auto server = BabyDI::Get<Server>();
	auto npc = server->getNPC(npcId);
	removeNPC(npc);
}

void Level::setMap(std::weak_ptr<Map> pMap, int pMapX, int pMapY)
{
	m_map = pMap;
	m_mapX = pMapX;
	m_mapY = pMapY;
}

bool Level::doTimedEvents()
{
	auto server = BabyDI::Get<Server>();

	// Check if we should revert any board changes.
	for (auto& change: m_boardChanges)
	{
		int respawnTimer = change.timeout.doTimeout();
		if (respawnTimer == 0)
		{
			// Put the old data back in.  DON'T DELETE THE CHANGE.
			// The client remembers board changes and if we delete the
			// change, the client won't get the new data.
			change.swapTiles();
			change.setModTime(time(0));
			server->sendPacketToOneLevel(CString() >> (char)PLO_BOARDMODIFY << change.getBoardStr(), this->shared_from_this());
		}
	}

	// Check if any items have timed out.
	// This allows us to delete items that have disappeared if nobody is in the level to send
	// the PLI_ITEMDEL packet.
	for (auto i = m_items.begin(); i != m_items.end();)
	{
		LevelItem& item = *i;
		int deleteTimer = item.timeout.doTimeout();
		if (deleteTimer == 0)
		{
			i = m_items.erase(i);
		}
		else
			++i;
	}

	// Check if any horses need to be deleted.
	for (auto i = m_horses.begin(); i != m_horses.end();)
	{
		LevelHorse& horse = *i;
		int deleteTimer = horse.timeout.doTimeout();
		if (deleteTimer == 0)
		{
			server->sendPacketToOneLevel(CString() >> (char)PLO_HORSEDEL >> (char)(horse.getX() * 2) >> (char)(horse.getY() * 2), this->shared_from_this());
			i = m_horses.erase(i);
		}
		else
			++i;
	}

	// Check if any baddies need to be marked as dead or respawned.
	for (auto& baddy : m_baddies)
	{
		// See if we can respawn him.
		int respawnTimer = baddy->timeout.doTimeout();
		if (respawnTimer == 0)
		{
			if (baddy->type == BaddyType::SWAMPSOLDIER && baddy->mode == BaddyMode::HURT)
			{
				if (baddy->power == 1)
				{
					// Unset the hurt mode on the baddy.
					CString props = CString() >> (char)BaddyProp::MODE >> (char)BaddyMode::SWAMPSHOT;
					baddy->setPropsFromPacket(props);
					for (unsigned int i = 1; i < m_players.size(); ++i)
					{
						auto player = server->getPlayer(m_players[i]);
						player->sendPacket(CString() >> (char)PLO_BADDYPROPS >> (char)baddy->id << props);
					}
				}
			}
			else if (baddy->mode == BaddyMode::DIE)
			{
				baddy->mode = BaddyMode::DEAD;
				if (baddy->canRespawn())
					baddy->timeout.setTimeout(server->getSettings().getInt("baddyrespawntime", 60));

				// Set the baddy as dead for all the other players in the level.
				CString props = CString() >> (char)BaddyProp::MODE >> (char)BaddyMode::DEAD;
				for (unsigned int i = 1; i < m_players.size(); ++i)
				{
					auto player = server->getPlayer(m_players[i]);
					player->sendPacket(CString() >> (char)PLO_BADDYPROPS >> (char)baddy->id << props);
				}

				// TODO(Nalin): Record the last player who hit the baddy so we can record the source properly.
				if (!hasLivingBaddies())
					server->queueNPCEvent(shared_from_this(), ScriptEventType::PLAYERLAYSITEM, source::FromLevel(shared_from_this()));
			}
			else
			{
				baddy->reset();
				for (auto p: m_players)
				{
					auto player = server->getPlayer(p);
					player->sendPacket(CString() >> (char)PLO_BADDYPROPS >> (char)baddy->id << baddy->getProps(player->getVersion()));
				}
			}
		}
	}

	return true;
}

std::optional<LevelLink*> Level::getLink(int pX, int pY) const
{
	for (const auto& link: m_links)
	{
		auto& bbox = link->getBoundingBox();
		if ((pX >= bbox.position.x() && pX <= bbox.position.y() + bbox.size.width())
			&& (pY >= bbox.position.y() && pY <= bbox.position.y() + bbox.size.height()))
		{
			return std::make_optional(link.get());
		}
	}

	return std::nullopt;
}

std::optional<LevelChest*> Level::getChest(int x, int y) const
{
	for (const auto& chest: m_chests)
	{
		if (chest->getX() == x && chest->getY() == y)
			return std::make_optional(chest.get());
	}

	return std::nullopt;
}

CString Level::getChestStr(LevelChest* chest) const
{
	static char retVal[500];
	sprintf(retVal, "%i:%i:%s", chest->getX(), chest->getY(), m_levelName.text());
	return retVal;
}

LevelLink* Level::addLink()
{
	// New level link
	auto newLink = std::make_shared<LevelLink>();
	auto* link = newLink.get();

	m_links.push_back(std::move(newLink));

	return link;
}

LevelLink* Level::addLink(const std::vector<CString>& pLink)
{
	// New level link
	auto newLink = std::make_unique<LevelLink>(pLink);
	auto* link = newLink.get();

	m_links.push_back(std::move(newLink));

	return link;
}

bool Level::removeLink(uint32_t index)
{
	if (m_links.empty())
		return false;
	if (index < 0 || index > m_links.size())
	{
		return false;
	}
	else
	{
		m_links.erase(m_links.begin() + index);
		return true;
	}

	return false;
}

LevelSign* Level::addSign(const int pX, const int pY, const CString& pSign, bool encoded)
{
	// New level link
	auto newSign = std::make_unique<LevelSign>(pX, pY, pSign, encoded);
	auto* sign = newSign.get();

	m_signs.push_back(std::move(newSign));

	return sign;
}

bool Level::removeSign(uint32_t index)
{
	if (m_signs.empty())
		return false;

	if (index < 0 || index > getSigns().size())
	{
		return false;
	}
	else
	{
		getSigns().erase(getSigns().begin() + index);

		return true;
	}

	return false;
}

LevelChest* Level::addChest(const int pX, const int pY, const LevelItemType itemType, const int signIndex)
{
	// New level link
	auto newChest = std::make_unique<LevelChest>(pX, pY, itemType, signIndex);
	auto* chest = newChest.get();

	m_chests.push_back(std::move(newChest));

	return chest;
}

bool Level::removeChest(uint32_t index)
{
	if (getChests().empty())
		return false;

	if (index < 0 || index > getChests().size())
	{
		return false;
	}
	else
	{
		getChests().erase(getChests().begin() + index);

		return true;
	}

	return false;
}

bool Level::hasLivingBaddies() const
{
	for (const auto& baddy: m_baddies)
	{
		if (baddy->mode != BaddyMode::DEAD)
			return true;
	}
	return false;
}

bool Level::isOnWall(const Position<uint8_t>& tilePosition)
{
	if (tilePosition.x() > 63 || tilePosition.y() > 63)
		return true;

	return tiletypes[getTiles(0)[static_cast<size_t>(tilePosition.y()) * 64 + tilePosition.x()]] >= 20;
}

bool Level::isOnWall2(const Rectangle<uint8_t, uint8_t>& tileArea, uint8_t flags)
{
	for (auto cy = tileArea.position.y(); cy < tileArea.position.y() + tileArea.size.height(); ++cy)
	{
		for (auto cx = tileArea.position.x(); cx < tileArea.position.x() + tileArea.size.width(); ++cx)
		{
			if (isOnWall({ cx, cy }))
				return true;
		}
	}

	return false;
}

bool Level::isOnWater(const Position<uint8_t>& tilePosition)
{
	return (tiletypes[getTiles(0)[static_cast<size_t>(tilePosition.y()) * 64 + tilePosition.x()]] == 11);
}

std::vector<NPCID> Level::findIntersectingNPCs(const Position<int16_t>& position, bool includeInvisible)
{
	return findIntersectingNPCs({ { position.x(), position.y() }, { 0, 0 } }, includeInvisible);
}

std::vector<NPCID> Level::findIntersectingNPCs(const Rectangle<int16_t, uint16_t>& area, bool includeInvisible)
{
	std::vector<NPCID> results;

	auto* server = BabyDI::Get<Server>();
	for (const auto& npcId : m_npcs)
	{
		if (auto npc = server->getNPC(npcId); npc != nullptr)
		{
			// If the NPC is invisible and we don't want to include invisible NPCs, skip it.
			if (!includeInvisible && (npc->visFlags & PROPID(NPCVisFlags::VISIBLE)) == 0)
				continue;

			// Check if the NPC intersects with the area.
			if (rectanglesIntersect(area, npc->getBoundingBox()))
				results.push_back(npcId);
		}
	}

	return results;
}

std::vector<NPCID> Level::findIntersectingNPCsForCollision(const Position<int16_t>& position)
{
	return findIntersectingNPCsForCollision({ { position.x(), position.y() }, { 0, 0 } });
}

std::vector<NPCID> Level::findIntersectingNPCsForCollision(const Rectangle<int16_t, uint16_t>& area)
{
	std::vector<NPCID> results;

	auto* server = BabyDI::Get<Server>();
	for (const auto& npcId : m_npcs)
	{
		if (auto npc = server->getNPC(npcId); npc != nullptr)
		{
			// If the NPC is invisible, skip it.
			if ((npc->visFlags & PROPID(NPCVisFlags::VISIBLE)) == 0)
				continue;

			// Check if the NPC intersects with the area.
			if (rectanglesIntersect(area, npc->getCollisionBoundingBox()))
				results.push_back(npcId);
		}
	}

	return results;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
