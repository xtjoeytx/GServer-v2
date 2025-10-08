#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <generator>
#include <iterator>
#include <list>
#include <memory>
#include <numbers>
#include <optional>
#include <ostream>
#include <string_view>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <BabyDI.h>
#include <CString.h>
#include <IEnums.h>
#include <IUtil.h>

#include <Server.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <level/Level.h>
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
#include <level/tiletypes.h>
#include <loader/LevelLoader.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/generator/TimeoutGenerator.h>
#include <utilities/Log.h>
#include <utilities/PropertySerializers.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

short respawningTiles[] = {
	0x1ff, // grass
	0x3ff, // grass
	0x7ff, // grass
	0x2ac, // vase
	0x002, // bush
	0x200, // sign
	0x022, // stone
	0x3de, // blackstone
	0x1a4, // swamp
	0x14a, // stake 1
	0x674, // stake 2
	0x72a, // hole
};

//----------------------------

Level::Level()
{
	// Reserve space for baddies to avoid reallocations, which will destroy timeout callback pointers.
	m_baddies.reserve(0xFF);
}

Level::~Level()
{
	auto server = BabyDI::Get<Server>();

	// Delete NPCs.
	{
		// Remove every NPC in the level.
		for (auto& levelNPC : m_npcs)
		{
			auto npc = server->getNPC(levelNPC);
			if (!npc) continue;
			if (npc->storageType == NPCStorageType::LEVEL)
				server->deleteNPC(npc, false);
			else npc->level.clear();
		}
		m_npcs.clear();
	}

	// Delete shoots.
	m_shoots.clear();

	// Delete arrows.
	m_arrows.clear();

	// Delete baddies.
	m_baddies.clear();

	// Delete chests.
	m_chests.clear();

	// Delete links.
	m_links.clear();

	// Delete signs.
	m_signs.clear();

	// Delete items.
	for (size_t i = m_items.size(); i > 0; --i)
		removeItem(inform_client, i - 1);
	m_items.clear();

	// Delete board changes.
	m_boardChanges.clear();

	// Warp players out.
	for (const auto& playerId : m_players)
		server->warpPlayerToSafePlace(playerId);
}

//----------------------------

std::shared_ptr<Level> Level::createLevel(std::string_view levelName, std::optional<uint16_t> fillTile)
{
	auto server = BabyDI::Get<Server>();
	auto& levelList = server->getLevelList();

	auto level = std::shared_ptr<Level>(new Level());
	level->levelName = levelName;

	if (fillTile)
		level->m_tiles[0] = LevelTiles(*fillTile);

	levelList.insert(std::make_pair(string::toLower(levelName), level));
	return level;
}

std::shared_ptr<Level> Level::clone(LevelPtr level)
{
	if (level == nullptr) return nullptr;
	return LevelLoader::loadLevel(std::filesystem::path{ level->levelName });
}

//----------------------------

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

	// Delete shoots.
	m_shoots.clear();

	// Delete arrows.
	m_arrows.clear();

	// Kill off all the baddies and disable respawn.
	for (size_t i = 0; i < m_baddies.size(); ++i)
	{
		m_baddies[i].setRespawn(false);
		m_baddies[i].mode = BaddyMode::DEAD;
	}

	// Delete chests.
	m_chests.clear();

	// Delete links.
	m_links.clear();

	// Delete signs.
	m_signs.clear();

	// Delete items.
	for (size_t i = m_items.size(); i > 0; --i)
		removeItem(inform_client, i - 1);
	m_items.clear();

	// Delete board changes.
	m_boardChanges.clear();

	// Clean up the rest.
	isSparringZone = false;
	isSingleplayer = false;

	// Remove all the players from the level.
	std::deque<PlayerID> oldplayers = m_players;
	for (auto& id : oldplayers)
	{
		if (auto p = server->getPlayer<PlayerClient>(id); p)
			p->leaveLevel(true);
	}

	// Reset the level cache for all the players on the server.
	auto& playerList = server->getPlayerList();
	for (const auto& [id, p] : players_of_type<PlayerClient>(playerList))
	{
		p->resetLevelCache(this);
	}

	// Re-load the level now.
	auto level = LevelLoader::loadLevelInto(shared_from_this(), std::filesystem::path{ levelName });
	bool ret = level != nullptr;

	// Warp all players back to the level (or to unstick me if loadLevel failed).
	CString uLevel = server->getSettings().getStr("unstickmelevel", "onlinestartlocal.nw");
	float uX = server->getSettings().getFloat("unstickmex", 30.0f);
	float uY = server->getSettings().getFloat("unstickmey", 35.0f);
	for (auto& id : oldplayers)
	{
		if (auto p = server->getPlayer<PlayerClient>(id); p)
			p->warp((ret ? levelName : uLevel), ret ? p->getLocalPosition() : toLocalPixelPosition(uX, uY));
	}

	return ret;
}

void Level::saveLevel(const std::string& filename)
{
	auto server = BabyDI::Get<Server>();
	auto& fileSystem = server->getFileSystem();

	auto actualFilename = getFilename(filename);
	auto path = fileSystem.findi(fs::FileCategory::LEVEL, actualFilename.toStringView());

	if (path.empty())
	{
		auto dirs = fileSystem.getManagedDirectories(fs::FileCategory::LEVEL);
		auto iter = dirs.begin();
		if (iter == dirs.end())
		{
			log::printLine(log::server, "** Error saving level: {}. No level directories are configured.", actualFilename);
			return;
		}

		path = std::filesystem::path{ *iter } / actualFilename.toStringView();
	}

	std::ofstream fileStream(path);

	fileStream << "GLEVNW01" << std::endl;

	// white space separator
	std::string s = " ";
	// write tiles
	for (size_t layer = 0; layer < getLayers().size(); layer++)
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
			for (const auto& chunk : chunks)
			{
				fileStream << "BOARD" << s << chunk.first << s << y << s << chunk.second.length() / 2 << s << layer // x, y, width, layer
					<< s << chunk.second << std::endl;
			}
		}
	}

	for (const auto& link : getLinks())
	{
		auto& bbox = link.getBoundingBox();
		fileStream
			<< std::format("LINK {} {} {} {} {} {} {}",
				link.getDestinationLevel(),
				bbox.position.x(), bbox.position.y(),
				bbox.size.width(), bbox.size.height(),
				link.getDestinationX(), link.getDestinationY()
			)
			<< std::endl;
	}

	for (const auto& sign : getSigns())
	{
		fileStream << "SIGN" << s << sign.getTileX() << s << sign.getTileY() << std::endl;
		fileStream << sign.unformattedText << std::endl;
		fileStream << "SIGNEND" << std::endl;
	}

	for (const auto& chest : getChests())
	{
		fileStream << "CHEST" << s << chest.getTileX() << s << chest.getTileY() << s << LevelItem::getItemName(chest.item) << s << chest.sign << std::endl;
	}

	for (const auto& baddy : m_baddies)
	{
		fileStream << "BADDY" << s << baddy.getTileX() << s << baddy.getTileY() << s << PROPID(baddy.type) << std::endl;

		for (const auto& verse : baddy.verses)
		{
			fileStream << verse << std::endl;
		}

		fileStream << "BADDYEND" << std::endl;
	}

	for (const auto& npcId : m_npcs)
	{
		auto npc = server->getNPC(npcId);

		// Don't save PUTNPC's or DBNPC's in the level file
		if (npc->storageType != NPCStorageType::LEVEL)
			continue;

		std::string image = npc->image;

		if (image.empty())
			image = "-"; // No image is represented by "-"

		fileStream << "NPC" << s << image << s << (npc->character.localPixelX / 16.0f) << s << (npc->character.localPixelY / 16.0f) << std::endl;
		fileStream << string::trim(npc->getScript().getOriginalSource()) << std::endl;
		fileStream << "NPCEND" << std::endl;
	}
}

//----------------------------

void Level::doTimedEvents()
{
	auto server = BabyDI::Get<Server>();
	const auto& now = server->getFrameStartTimeHighPrecision();

	// Run board change events.
	for (auto& change : m_boardChanges)
		change.update(now);

	// Run bomb events.
	for (auto& bomb : m_bombs) bomb.timeout.update(now);
	std::erase_if(m_bombs, [this](const LevelBomb& bomb)
	{
		bool exploded = !bomb.timeout.isRunning();
		if (exploded)
		{
			// Generate bomb explosions.
			// Don't send to players as they should see the bomb already.
			if (bomb.power != 2)
				addExplosion(bomb.position, bomb.owner, 1, bomb.power);
			else
			{
				addExplosion(bomb.position, bomb.owner, 4, bomb.power);
				addExplosion(translatePosition(bomb.position, -32, -32), bomb.owner, 4, bomb.power);
				addExplosion(translatePosition(bomb.position,  32, -32), bomb.owner, 4, bomb.power);
				addExplosion(translatePosition(bomb.position, -32,  32), bomb.owner, 4, bomb.power);
				addExplosion(translatePosition(bomb.position,  32,  32), bomb.owner, 4, bomb.power);
			}
		}
		return exploded;
	});

	// Run explosion events.
	for (auto& explosion : m_explosions) explosion.timeout.update(now);
	std::erase_if(m_explosions, [](const LevelExplosion& explosion) { return !explosion.timeout.isRunning(); });

	// Run item events.
	for (auto& item : m_items) item.timeout.update(now);
	std::erase_if(m_items, [](const LevelItem& item) { return !item.timeout.isRunning(); });

	// Run horse events.
	for (auto& horse : m_horses) horse.timeout.update(now);
	std::erase_if(m_horses, [](const LevelHorse& horse) { return !horse.timeout.isRunning(); });

	// Run baddy events.
	for (auto& baddy : m_baddies)
		baddy.timeout.update(now);
}

void Level::doFrameEvents(precise_clock::time_point time)
{
	// Don't bother with shoot and arrow processing if we don't have an npc-server.
	if (auto server = BabyDI::Get<Server>(); server != nullptr && !server->hasNPCServer())
		return;

	// Determine elapsed time.
	auto elapsed = time - m_lastFrameTime;
	m_frameEventDuration += elapsed;
	m_lastFrameTime = time;

	// Count the iterations.
	int iterations = m_frameEventDuration / 50ms;
	if (iterations == 0)
		return;

	// Subtract our iterations.
	m_frameEventDuration -= iterations * 50ms;

	// Run shoot events.
	std::vector<size_t> deletedItems;
	for (size_t i = 0; i < m_shoots.size(); ++i)
	{
		if (!moveShoot(&m_shoots[i], iterations))
			deletedItems.push_back(i);
	}
	std::erase_if(m_shoots, [this, &deletedItems](const LevelShoot& shoot)
	{
		return std::find(deletedItems.begin(), deletedItems.end(), &shoot - &m_shoots[0]) != deletedItems.end();
	});

	// Run arrow events.
	deletedItems.clear();
	for (size_t i = 0; i < m_arrows.size(); ++i)
	{
		if (!moveArrow(&m_arrows[i], iterations))
			deletedItems.push_back(i);
	}
	std::erase_if(m_arrows, [this, &deletedItems](const LevelArrow& arrow)
	{
		return std::find(deletedItems.begin(), deletedItems.end(), &arrow - &m_arrows[0]) != deletedItems.end();
	});
}

//----------------------------

bool Level::hasTerrain() const noexcept
{
	if (m_map == nullptr || isOnBigMap())
		return false;

	return !m_map->terrain.gridBorderTileHeightsXAxis.empty();
}

double Level::getHeightAt(const LocalPixelPosition& position) const noexcept
{
	if (terrain.heightmap.empty())
		return 0.0;

	auto tilePosition = toTilePosition(position);

	// Determine the origin tile for our calculation.
	// This will be the top-left tile within the quad we are calculating the height for.
	LocalWholeTilePosition originTile = toLocalWholeTilePosition(tilePosition);
	if (tilePosition.x() > 64) originTile.x() = 64;
	if (tilePosition.y() > 64) originTile.y() = 64;

	auto heightAtPosition = [&](const TilePosition& pos) -> double
	{
		return terrain.heightmap[static_cast<size_t>(pos.y()) * 65 + pos.x()];
	};

	// Generate 3D coordinates for our tiles.
	TilePosition topLeft = toTilePosition(originTile);
	TilePosition topRight = toTilePosition(translatePosition(originTile, 1_ui8, 0_ui8));
	TilePosition bottomLeft = toTilePosition(translatePosition(originTile, 0_ui8, 1_ui8));
	topLeft.z() = heightAtPosition(topLeft);
	topRight.z() = heightAtPosition(topRight);
	bottomLeft.z() = heightAtPosition(bottomLeft);

	// Calculate our direction vectors.
	Position<float> vecU = topRight - topLeft;
	Position<float> vecV = bottomLeft - topLeft;

	// Determine our tile offset.
	TilePosition offset{ tilePosition.x() - topLeft.x(), tilePosition.y() - topLeft.y() };

	// Calculate our point using the offset along the direction vectors.
	TilePosition point = topLeft + (vecU * offset.x()) + (vecV * offset.y());

	return point.z();
}

double Level::getMapHeightAt(const PixelPosition& position) const noexcept
{
	if (m_map != nullptr)
	{
		if (auto level = m_map->getLevelAt(position); level != nullptr)
			return level->getHeightAt(toLocalPixelPosition(position));
	}

	return getHeightAt(toLocalPixelPosition(position));
}

//----------------------------

CString Level::getBoardPacket()
{
	CString retVal;
	retVal.writeGChar(PLO_BOARDPACKET);
	retVal.write((char*)m_tiles[0], sizeof(short[4096]));

	return retVal;
}

CString Level::getLayerPacket(int layer)
{
	CString retVal;
	retVal.writeGChar(PLO_BOARDLAYER);

	// TODO: Only send the tiles that has been placed on the layer
	retVal << (char)layer << (char)0 << (char)0 << (char)64 << (char)64;
	retVal.write((char*)m_tiles[layer], sizeof(short[4096]));

	return retVal;
}

CString Level::getBoardChangesPacket(time_t time)
{
	CString retVal;
	retVal >> (char)PLO_LEVELBOARD;
	for (const auto& change : m_boardChanges)
	{
		if (clock::to_time_t(change.modTime) >= time)
			retVal << change.getPropsForSingleLevel();
	}
	return retVal;
}

CString Level::getBoardChangesPacket2(time_t time)
{
	CString retVal;
	retVal >> (char)PLO_BOARDMODIFY;
	for (const auto& change : m_boardChanges)
	{
		if (clock::to_time_t(change.modTime) >= time)
			retVal << change.getPropsForSingleLevel();
	}
	return retVal;
}

void Level::sendBoardHeightsToPlayer(std::shared_ptr<Player> player) const
{
	// We only need to send heights if there are level overrides.
	if (terrain.levelHeightOverrides.empty())
		return;

	CString retVal;
	retVal.writeGChar(PLO_BOARDHEIGHTS);

	// Maybe the map position?
	retVal >> (char)mapPosition.x() >> (char)mapPosition.y();

	// Starting x/y index of the heightmap block.
	retVal >> (char)0 >> (char)0;

	// Width/height of the heightmap block.
	// 0 indexed for some reason so use 8 instead of 9.
	retVal >> (char)8 >> (char)8;

	// The heightmap data.
	for (size_t y = 0; y < 9; ++y)
	{
		for (size_t x = 0; x < 9; ++x)
		{
			auto index = y * 9 + x;
			auto height = terrain.levelHeightOverrides[index];

			// The whole number and fractional part of the height are stored separately.
			// The whole number is offset by 50, giving a range of -50 to +170.
			// The fractional part is multiplied by 128 and stored as a byte.
			double decimal = height - std::floor(height);
			double whole = std::round(height - decimal);

			uint8_t wholePart = static_cast<uint8_t>(whole + 50);
			uint8_t decimalPart = static_cast<uint8_t>(decimal * 128);

			retVal >> wholePart;
			retVal >> decimalPart;
		}
	}

	player->sendPacket(retVal);
}

void Level::sendBaddiesToPlayer(std::shared_ptr<Player> player) const
{
	CString packet;
	for (const auto& baddy : m_baddies)
	{
		packet.clear();
		packet >> (char)PLO_BADDYPROPS >> (char)baddy.id << baddy.getProps();
		player->sendPacket(packet);
	}
}

void Level::sendChestsToPlayer(std::shared_ptr<Player> player) const
{
	CString packet;
	for (auto& chest : m_chests)
	{
		bool hasChest = player->account.hasChest(levelName, chest.getTileX(), chest.getTileY());

		packet.clear();
		packet >> (char)PLO_LEVELCHEST >> (char)(hasChest ? 1 : 0) >> (char)chest.getTileX() >> (char)chest.getTileY();
		if (!hasChest) packet >> (char)chest.item >> (char)chest.sign;
		player->sendPacket(packet);
	}
}

void Level::sendHorsesToPlayer(std::shared_ptr<Player> player) const
{
	CString packet;
	for (auto& horse : m_horses)
	{
		packet.clear();
		packet >> (char)PLO_HORSEADD << horse.getPacket();
		player->sendPacket(packet);
	}
}

void Level::sendLinksToPlayer(std::shared_ptr<Player> player) const
{
	CString packet;
	for (const auto& link : m_links)
	{
		packet.clear();
		packet >> (char)PLO_LEVELLINK << link.getLinkStr();
		player->sendPacket(packet);
	}
}

void Level::sendSignsToPlayer(std::shared_ptr<Player> player) const
{
	CString packet;
	for (const auto& sign : m_signs)
	{
		packet.clear();
		packet >> (char)PLO_LEVELSIGN << sign.getSignPacket(player.get());
		player->sendPacket(packet);
	}
}

// TODO: Replace with a function in server that sends npc props from a list of ids.
void Level::sendNPCsToPlayer(std::shared_ptr<Player> player, clock::time_point time) const
{
	auto server = BabyDI::Get<Server>();
	for (const auto& npcId : m_npcs)
	{
		auto npc = server->getNPC(npcId);
		if (!npc) continue;

		auto packet = npc->getAllPropsPacket(time);
		if (!packet.isEmpty())
		{
			player->sendPacket(CString() >> (char)PLO_NPCPROPS >> (int)npc->id << packet);
			if (player->getVersion() >= CLVER_4_0211 && !npc->getScript().getClientByteCode().empty())
			{
				CString byteCodePacket = CString() >> (char)PLO_NPCBYTECODE >> (int)npc->id;
				byteCodePacket.write(reinterpret_cast<const char*>(npc->getScript().getClientByteCode().data()), npc->getScript().getClientByteCode().size());
				player->sendPacket(CString() >> (char)PLO_RAWDATA >> (int)byteCodePacket.length());
				player->sendPacket(byteCodePacket);
			}
		}

		npc->sendShowImagesToPlayer(player, time);
		npc->sendMoveQueueToPlayer(player, time);
	}
}

//----------------------------

bool Level::isPlayerLeader(PlayerID id) const
{
	if (!isOnGmap())
	{
		if (m_players.empty())
			return false;
		return m_players.front() == id;
	}
	else
	{
		auto players = getMapPlayers();
		auto iter = players.begin();
		if (iter == players.end() || *iter != id)
			return false;
		return true;
	}
}

bool Level::hasLivingBaddies() const
{
	for (const auto& baddy : m_baddies)
	{
		if (baddy.mode != BaddyMode::DEAD)
			return true;
	}
	return false;
}

//----------------------------

int Level::addPlayer(PlayerID id)
{
	m_players.push_back(id);

	// Set the player enters event on all the NPCs.
	auto server = BabyDI::Get<Server>();
	if (auto player = server->getPlayer(id); player != nullptr)
		server->queueNPCEvent(shared_from_this(), player->getGlobalPosition(), ScriptEventType::PLAYERENTERS, source::FromPlayer(id));

	return static_cast<int>(m_players.size() - 1);
}

void Level::removePlayer(PlayerID id)
{
	std::erase(m_players, id);

	// Set the player leaves event on all the NPCs.
	auto server = BabyDI::Get<Server>();
	if (auto player = server->getPlayer(id); player != nullptr)
		server->queueNPCEvent(shared_from_this(), player->getGlobalPosition(), ScriptEventType::PLAYERLEAVES, source::FromPlayer(id));
}

//----------------------------

bool Level::addNPC(std::shared_ptr<NPC> npc)
{
	if (std::ranges::contains(m_npcs, npc->id))
		return false;

	m_npcs.push_back(npc->id);

	auto script = string::trimLeft(npc->getScript().getClientSide());

	if (script.starts_with("sparringzone"))
		isSparringZone = true;

	if (script.starts_with("noplayerkilling"))
		isNoPkZone = true;

	if (script.starts_with("singleplayer"))
		isSingleplayer = true;

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
}

void Level::removeNPC(NPCID npcId)
{
	auto server = BabyDI::Get<Server>();
	auto npc = server->getNPC(npcId);
	removeNPC(npc);
}

//----------------------------

bool Level::alterBoard(CString& tileData, const LocalWholeTileRectangleArea& area, Player* player, bool forceRespawn, bool allowRespawn)
{
	if (area.position.x() > 63 || area.position.y() > 63 ||
		area.size.width() < 1 || area.size.height() < 1 ||
		area.position.x() + area.size.width() > 64 || area.position.y() + area.size.height() > 64)
		return false;

	auto server = BabyDI::Get<Server>();
	auto& settings = server->getSettings();

	// Do the check for the push-pull block.
	if (area.size.width() == 4 && area.size.height() == 4 && settings.getBool("clientsidepushpull", true))
	{
		// Try to find the top-left corner tile.
		int i;
		for (i = 0; i < 16; ++i)
		{
			short stoneCheck = tileData.readGShort();
			if (stoneCheck == 0x06E4 || stoneCheck == 0x07CE)
				break;
		}

		// Check if we found a possible push-pull block.
		if (i != 16 && i < 11)
		{
			// Go back one full short so the first readByte2() returns the top-left corner.
			tileData.setRead(i * 2);

			int foundCount = 0;
			for (int j = 0; j < 6; ++j)
			{
				// Read a piece.
				short stoneCheck = tileData.readGShort();

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
			tileData.setRead(0);

			// Check if we found a full tile.  If so, don't accept the change.
			if (foundCount == 4 && player != nullptr)
			{
				player->sendPacket(CString() >> (char)PLO_BOARDMODIFY >> (char)area.position.x() >> (char)area.position.y() >> (char)area.size.width() >> (char)area.size.height() << tileData);
				return false;
			}
		}
	}

	// Delete any existing changes within the same region.
	for (auto i = m_boardChanges.begin(); i != m_boardChanges.end();)
	{
		LevelBoardChange& change = *i;
		auto changeLocalPos = toLocalWholeTilePosition(change.area.position);
		if ((changeLocalPos.x() >= area.position.x() && changeLocalPos.y() + change.area.size.width() <= area.position.x() + area.size.width()) &&
			(changeLocalPos.y() >= area.position.y() && changeLocalPos.y() + change.area.size.height() <= area.position.y() + area.size.height()))
		{
			i = m_boardChanges.erase(i);
		}
		else
			++i;
	}

	// Any 2x2 tile change can respawn.
	// The list of tiles is mostly for security checks and should be a list of allowed replacements.
	// TODO: Develop a way to specify valid tile replacements.
	int respawnTime = settings.getInt("respawntime", 15);
	bool doRespawn = allowRespawn && (forceRespawn || (area.size.width() == 2 && area.size.height() == 2));

	/*
	// Check if the tiles should be respawned.
	// Only tiles in the respawningTiles array are allowed to respawn.
	// These are things like signs, bushes, pots, etc.
	int respawnTime = settings.getInt("respawntime", 15);
	bool doRespawn = false;
	short testTile = m_tiles[0][area.position.x() + (static_cast<size_t>(area.position.y()) * 64)];
	int tileCount = sizeof(respawningTiles) / sizeof(short);
	for (int i = 0; i < tileCount; ++i)
		if (testTile == respawningTiles[i]) doRespawn = true;
	*/

	// Grab old tiles for the respawn.
	CString oldTiles;
	if (doRespawn)
	{
		for (int j = area.position.y(); j < area.position.y() + area.size.height(); ++j)
		{
			for (int i = area.position.x(); i < area.position.x() + area.size.width(); ++i)
				oldTiles.writeGShort(m_tiles[0][i + (static_cast<size_t>(j) * 64)]);
		}
	}

	m_boardChanges.push_back(LevelBoardChange{ shared_from_this(), area, tileData, oldTiles, (doRespawn ? std::chrono::seconds(respawnTime) : 0s) });
	return true;
}

void Level::applyBoardChangeFromScriptTiles(const LocalWholeTileRectangleArea& area, bool forceRespawn, bool allowRespawn)
{
	// Assemble the tile data.
	CString tileData;
	for (int j = area.position.y(); j < area.position.y() + area.size.height(); ++j)
	{
		for (int i = area.position.x(); i < area.position.x() + area.size.width(); ++i)
		{
			auto tile = m_scriptUpdatedTiles[i + (static_cast<size_t>(j) * 64)];
			if (tile == constants::EmptyTileInLayer)
				tile = m_tiles[0][i + (static_cast<size_t>(j) * 64)];
			tileData.writeGShort(tile);
		}
	}

	// Apply the board update.
	if (alterBoard(tileData, area, nullptr, forceRespawn, allowRespawn))
	{
		auto server = BabyDI::Get<Server>();

		// Send the board update to nearby players.
		auto& boardChange = m_boardChanges.back();
		server->sendPacketToNearby(CString() >> (char)PLO_BOARDMODIFY2 << boardChange.getPropsForMap(), toPixelPosition(boardChange.area.position), shared_from_this(), {}, [](const Player* player) { return player->getVersion() >= CLVER_4_0211; });
		server->sendPacketToOneLevel(CString() >> (char)PLO_BOARDMODIFY << boardChange.getPropsForSingleLevel(), shared_from_this(), {}, [](const Player* player) { return player->getVersion() < CLVER_4_0211; });
	}
}

void Level::saveBoardChangeFromScriptTiles(const LocalWholeTileRectangleArea& area)
{
	for (int j = area.position.y(); j < area.position.y() + area.size.height(); ++j)
	{
		for (int i = area.position.x(); i < area.position.x() + area.size.width(); ++i)
		{
			auto tile = m_scriptUpdatedTiles[i + (static_cast<size_t>(j) * 64)];
			if (tile != constants::EmptyTileInLayer)
				m_tiles[0][i + (static_cast<size_t>(j) * 64)] = tile;
		}
	}
}

void Level::updateBoard(const TileRectangleArea& area) noexcept
{
	// Non-gmaps.
	if (!isOnGmap())
	{
		auto localArea = toLocalWholeTileRectangleArea({ 0, 0 }, area);
		applyBoardChangeFromScriptTiles(localArea, true);
		return;
	}

	// Gmaps.
	for (const auto& levelPtr : m_map->getLevelsInRectangle(toPixelRectangleArea(area)))
	{
		auto localArea = toLocalWholeTileRectangleArea(levelPtr->getMapPixelOffset(), area);
		if (localArea.size.width() != 0 && localArea.size.height() != 0)
			levelPtr->applyBoardChangeFromScriptTiles(localArea, true);
	}
}

void Level::updateBoard2(const TileRectangleArea& area) noexcept
{
	// If we don't allow permanent tile modifications, just call updateBoard().
	auto server = BabyDI::Get<Server>();
	if (server->getSettings().getBool("savelevels", false) == false)
	{
		updateBoard(area);
		return;
	}

	bool levelsAutoSave = server->getSettings().getBool("levelsautosave", true);

	// Non-gmaps.
	if (!isOnGmap())
	{
		auto localArea = toLocalWholeTileRectangleArea({ 0, 0 }, area);
		applyBoardChangeFromScriptTiles(localArea, false, false);
		if (levelsAutoSave)
		{
			saveBoardChangeFromScriptTiles(localArea);
			saveLevel(levelName);
		}
		return;
	}

	// Gmaps.
	for (const auto& levelPtr : m_map->getLevelsInRectangle(toPixelRectangleArea(area)))
	{
		auto localArea = toLocalWholeTileRectangleArea(levelPtr->getMapPixelOffset(), area);
		if (localArea.size.width() != 0 && localArea.size.height() != 0)
		{
			levelPtr->applyBoardChangeFromScriptTiles(localArea, false, false);
			if (levelsAutoSave)
			{
				levelPtr->saveBoardChangeFromScriptTiles(localArea);
				levelPtr->saveLevel(levelPtr->levelName);
			}
		}
	}
}

//----------------------------

LevelArrow* Level::addArrow(inform_client_t, const PixelPosition& position, const PixelPosition& speed, uint8_t direction, int8_t type, ScriptObject from)
{
	if (auto server = BabyDI::Get<Server>(); server != nullptr && !server->hasNPCServer())
		return nullptr;

	auto result = addArrow(position, speed, direction, type, from);
	if (result != nullptr)
	{
		auto localPosition = toLocalPixelPosition(result->position);
		char x = static_cast<char>(localPosition.x() / 8.0f);
		char y = static_cast<char>(localPosition.y() / 8.0f);

		// Get the sprite for the arrow.
		uint8_t sprite = (result->type == 0 ? ballSpriteIndex : arrowSpriteIndex);
		if (result->type != 0)
			sprite += (result->direction & 0b11);

		uint8_t flags = (result->direction & 0b11) | (result->getPacketFrom() << 3);
		BabyDI::Get<Server>()->sendPacketToOneLevel(CString() >> (char)PLO_ARROWADD >> (short)0 >> (char)x >> (char)y >> (char)flags >> (char)sprite >> (char)type, shared_from_this());
	}
	return result;
}

LevelArrow* Level::addArrow(const PixelPosition& position, const PixelPosition& speed, uint8_t direction, int8_t type, ScriptObject from)
{
	if (auto server = BabyDI::Get<Server>(); server != nullptr && !server->hasNPCServer())
		return nullptr;

	LevelArrow newArrow{ .position = position, .speed = speed, .direction = direction, .type = type, .from = from };
	m_arrows.emplace_back(std::move(newArrow));
	return &m_arrows.back();
}

bool Level::removeArrow(uint8_t index)
{
	if (index >= m_arrows.size())
		return false;

	m_arrows.erase(m_arrows.begin() + index);
	return true;
}

LevelArrow* Level::getArrow(uint8_t index) const
{
	if (index >= m_arrows.size())
		return nullptr;
	return const_cast<LevelArrow*>(&m_arrows[index]);
}

//----------------------------

LevelBaddy* Level::addBaddy(const LocalPixelPosition& position, BaddyType type)
{
	// Find the next available baddy that can be used.
	size_t nextIndex = 0;
	for (nextIndex = 0; nextIndex < m_baddies.size(); ++nextIndex)
	{
		if (m_baddies[nextIndex].canBeReplaced())
			break;
	}

	// Limit of 50 baddies per level.
	if (nextIndex >= 50)
		return nullptr;

	// Clamp the index to the size of the baddy list, just in case.
	nextIndex = std::clamp(nextIndex, static_cast<size_t>(0), m_baddies.size());

	// New Baddy
	LevelBaddy newBaddy{ position, type, this->shared_from_this() };
	newBaddy.id = nextIndex + 1;

	if (nextIndex == m_baddies.size())
		m_baddies.emplace_back(std::move(newBaddy));
	else
		m_baddies[nextIndex] = std::move(newBaddy);

	return &m_baddies[nextIndex];
}

LevelBaddy* Level::putNewBaddy(const LocalPixelPosition& position, BaddyType type)
{
	auto baddy = addBaddy(position, type);
	if (baddy == nullptr)
		return nullptr;

	auto server = BabyDI::Get<Server>();
	CString packet = CString() >> (char)PLO_BADDYPROPS >> (char)baddy->id << baddy->getProps();
	for (auto& playerId : m_players)
	{
		if (auto player = server->getPlayer(playerId); player)
			player->sendPacket(packet);
	}

	return baddy;
}

LevelBaddy* Level::putNewBaddy(const LocalPixelPosition& position, BaddyType type, uint8_t power, std::string_view image)
{
	auto baddy = addBaddy(position, type);
	if (baddy == nullptr)
		return nullptr;

	baddy->setImage(image);
	baddy->power = power;

	auto server = BabyDI::Get<Server>();
	CString packet = CString() >> (char)PLO_BADDYPROPS >> (char)baddy->id << baddy->getProps();
	for (auto& playerId : m_players)
	{
		if (auto player = server->getPlayer(playerId); player)
			player->sendPacket(packet);
	}

	return baddy;
}

bool Level::removeBaddy(uint8_t pId)
{
	// Don't allow us to remove id 0 or any id over 50.
	if (pId < 1 || pId > 50 || (pId > m_baddies.size())) return false;

	// Find the baddy.
	auto& baddy = m_baddies.at(static_cast<size_t>(pId) - 1);
	if (baddy.mode == BaddyMode::DEAD)
		return false;

	// Erase the baddy.
	baddy.mode = BaddyMode::DEAD;
	baddy.setRespawn(false);

	// Set the baddy as dead for all the other players in the level.
	auto server = BabyDI::Get<Server>();
	CString props = CString() >> (char)BaddyProp::MODE >> (char)BaddyMode::DEAD;
	for (const auto& playerId : m_players)
	{
		if (auto player = server->getPlayer(playerId); player != nullptr)
			player->sendPacket(CString() >> (char)PLO_BADDYPROPS >> (char)baddy.id << props);
	}
	return true;
}

bool Level::removeAllBaddies()
{
	auto server = BabyDI::Get<Server>();
	CString propsPacket;
	for (auto& baddy : m_baddies)
	{
		if (baddy.mode == BaddyMode::DEAD)
			continue;

		baddy.mode = BaddyMode::DEAD;
		baddy.setRespawn(false);

		// Set the baddy as dead for all the other players in the level.
		propsPacket.clear();
		propsPacket >> (char)PLO_BADDYPROPS >> (char)baddy.id >> (char)BaddyProp::MODE >> (char)BaddyMode::DEAD;
		for (const auto& playerId : m_players)
		{
			if (auto player = server->getPlayer(playerId); player != nullptr)
				player->sendPacket(propsPacket);
		}
	}
	return true;
}

LevelBaddy* Level::getBaddy(uint8_t id) const
{
	if (id > m_baddies.size() || id == 0)
		return nullptr;

	auto& baddy = m_baddies.at(static_cast<size_t>(id) - 1);
	return const_cast<LevelBaddy*>(&baddy);
}

//----------------------------

LevelBomb* Level::addBomb(inform_client_t, const PixelPosition& position, uint8_t power)
{
	if (auto server = BabyDI::Get<Server>(); server != nullptr && !server->hasNPCServer())
		return nullptr;

	auto result = addBomb(position, power);
	if (result != nullptr)
	{
		auto localPosition = toLocalPixelPosition(result->position);
		char x = static_cast<char>(localPosition.x() / 8.0f);
		char y = static_cast<char>(localPosition.y() / 8.0f);
		uint8_t timeToExplode = static_cast<uint8_t>(std::min<std::chrono::milliseconds::rep>(223, std::chrono::duration_cast<std::chrono::milliseconds>(result->timeout.timeout).count() / 50));
		BabyDI::Get<Server>()->sendPacketToOneLevel(CString() >> (char)PLO_BOMBADD >> (char)x >> (char)y >> (char)result->power >> (char)timeToExplode, shared_from_this());
	}
	return result;
}

LevelBomb* Level::addBomb(const PixelPosition& position, uint8_t power)
{
	if (auto server = BabyDI::Get<Server>(); server != nullptr && !server->hasNPCServer())
		return nullptr;

	LevelBomb newBomb{ .position = position, .power = power };
	newBomb.timeout.runOnceFor(3s);
	m_bombs.emplace_back(std::move(newBomb));
	return &m_bombs.back();
}

LevelBomb* Level::addBombFromClient(const PixelPosition& position, uint8_t power, PlayerID owner, std::chrono::milliseconds timeToExplode)
{
	if (auto server = BabyDI::Get<Server>(); server != nullptr && !server->hasNPCServer())
		return nullptr;

	// If we generate an item NPC, remove the bomb from the level.
	LevelItemType itemType = (power == 2 ? LevelItemType::SUPERBOMB : (power == 3 ? LevelItemType::JOLTBOMB : LevelItemType::BOMB));
	if (auto itemNPC = generateItemNPC(position, itemType); itemNPC != nullptr)
		return nullptr;

	// Add the bomb to the level otherwise.
	LevelBomb newBomb{ .position = position, .power = power, .owner = source::FromPlayer(owner) };
	newBomb.timeout.runOnceFor(timeToExplode);
	m_bombs.emplace_back(std::move(newBomb));
	return &m_bombs.back();
}

bool Level::removeBomb(inform_client_t, size_t index)
{
	if (index < m_bombs.size())
	{
		auto localPosition = toLocalPixelPosition(m_bombs[index].position);
		CString packet = CString() >> (char)PLO_BOMBDEL >> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8);
		BabyDI::Get<Server>()->sendPacketToOneLevel(packet, this->shared_from_this());
	}
	return removeBomb(index);
}

bool Level::removeBomb(size_t index)
{
	if (index >= m_bombs.size())
		return false;

	m_bombs.erase(m_bombs.begin() + index);
	return true;
}

bool Level::removeBomb(const PixelPosition& position)
{
	for (auto it = m_bombs.begin(); it != m_bombs.end(); ++it)
	{
		if (it->position == position)
		{
			m_bombs.erase(it);
			return true;
		}
	}
	return false;
}

LevelBomb* Level::getBomb(size_t index) const
{
	if (index >= m_bombs.size())
		return nullptr;
	return const_cast<LevelBomb*>(&m_bombs[index]);
}

//----------------------------

LevelChest* Level::addChest(const LocalWholeTilePosition& position, const LevelItemType itemType, const int signIndex)
{
	LevelChest newChest{ .position = position, .item = itemType, .sign = (uint8_t)signIndex };
	m_chests.push_back(std::move(newChest));
	return &m_chests.back();
}

bool Level::removeChest(size_t index)
{
	if (m_chests.empty() || index < 0 || index > m_chests.size())
		return false;

	m_chests.erase(m_chests.begin() + index);
	return true;
}

LevelChest* Level::getChest(size_t index) const
{
	if (index >= m_chests.size())
		return nullptr;
	return const_cast<LevelChest*>(&m_chests[index]);
}

std::optional<const LevelChest*> Level::getChest(const LocalWholeTilePosition& position) const
{
	for (const auto& chest : m_chests)
	{
		if (chest.position == position)
			return std::make_optional(&chest);
	}

	return std::nullopt;
}

std::string Level::getChestFormattedForSave(LevelChest* chest) const
{
	return std::format("{}:{}:{}", chest->getTileX(), chest->getTileY(), levelName);
}

//----------------------------

void Level::addExplosion(inform_client_t, const PixelPosition& position, ScriptObject from, uint8_t radius, uint8_t power)
{
	if (auto server = BabyDI::Get<Server>(); server != nullptr && !server->hasNPCServer())
		return;

	addExplosion(position, from, radius, power);

	auto localPosition = toLocalPixelPosition(position);
	CString packet = CString() >> (char)PLO_EXPLOSION >> (short)0 >> (char)radius >> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8) >> (char)power;
	BabyDI::Get<Server>()->sendPacketToOneLevel(packet, shared_from_this());
}

void Level::addExplosion(const PixelPosition& position, ScriptObject from, uint8_t radius, uint8_t power)
{
	if (auto server = BabyDI::Get<Server>(); server != nullptr && !server->hasNPCServer())
		return;

	addExplosionPart(position, 2, power);
	for (size_t i = 0; i < (static_cast<size_t>(radius) * 4); ++i)
	{
		uint8_t dir = i / radius;
		int16_t step = (((i % radius) + 1) * 2) * 16;
		PixelPosition partPosition = position.translate(
			(dir == 0 || dir == 2) ? 0 : (dir == 1 ? -step : step),
			(dir == 1 || dir == 3) ? 0 : (dir == 0 ? -step : step));
		addExplosionPart(partPosition, dir, power);
	}

	// Add exploded events to NPCs in the level.
	if (auto server = BabyDI::Get<Server>(); server && server->hasNPCServer())
	{
		PixelRectangleArea vertTest = { position.translate(0, -(radius * 32)), { static_cast<uint16_t>(32), static_cast<uint16_t>((1 + (radius * 2)) * 32) } };
		PixelRectangleArea horzTest = { position.translate(-(radius * 32), 0), { static_cast<uint16_t>((1 + (radius * 2)) * 32), static_cast<uint16_t>(32) } };
		for (const NPCID& npcId : findIntersectingNPCsForCollision(vertTest))
		{
			if (auto npc = server->getNPC(npcId); npc != nullptr)
				npc->scripting.events.addEvent(ScriptEventType::EXPLODED, from);
		}
		for (const NPCID& npcId : findIntersectingNPCsForCollision(horzTest))
		{
			if (auto npc = server->getNPC(npcId); npc != nullptr)
				npc->scripting.events.addEvent(ScriptEventType::EXPLODED, from);
		}
	}
}

void Level::addSpyFire(const PixelPosition& position, ScriptObject from, uint8_t direction, uint8_t length, uint8_t power)
{
	/*
	spyfire 3,1;

	up:    x+0.5,y-1.5  2 0 0 0
	down:  x+0.5,y+2.2  0 2 2 2
	left:  x-2.0,y+0.2  3 1 1 1
	right: x+3.0,y+0.2  1 3 3 3
	*/

	if (auto server = BabyDI::Get<Server>(); server != nullptr && !server->hasNPCServer())
		return;

	const PixelPosition startingPosition = position.translate(
		(direction == 0 || direction == 2) ? 8 : (direction == 1 ? -32 : 48),
		(direction == 1 || direction == 3) ? 3 : (direction == 0 ? -24 : 35));

	for (size_t i = 0; i < static_cast<size_t>(length + 1); ++i)
	{
		uint8_t dir = (i != 0 ? direction : (direction + 2) % 4);
		int16_t stepX = (direction == 0 || direction == 2) ? 0 : (direction == 1 ? -i * 32 : i * 32);
		int16_t stepY = (direction == 1 || direction == 3) ? 0 : (direction == 0 ? -i * 32 : i * 32);
		PixelPosition partPosition = startingPosition.translate(stepX, stepY);
		addExplosionPart(partPosition, dir, power);
	}

	// Add exploded events to NPCs in the level.
	if (auto server = BabyDI::Get<Server>(); server && server->hasNPCServer())
	{
		int16_t lengthInPixels = (length + 1) * 32;
		PixelPosition testPosition = startingPosition.translate(
			static_cast<int16_t>((direction == 1) ? -lengthInPixels : 0),
			static_cast<int16_t>((direction == 0) ? -lengthInPixels : 0));
		Dimension<uint16_t> testDimension{
			static_cast<uint16_t>((direction == 0 || direction == 2) ? 32 : lengthInPixels),
			static_cast<uint16_t>((direction == 1 || direction == 3) ? 32 : lengthInPixels) };

		for (const NPCID& npcId : findIntersectingNPCsForCollision({ testPosition, testDimension }))
		{
			if (auto npc = server->getNPC(npcId); npc != nullptr)
				npc->scripting.events.addEvent(ScriptEventType::EXPLODED, from);
		}
	}
}

LevelExplosion* Level::addExplosionPart(const PixelPosition& position, uint8_t direction, uint8_t power)
{
	if (auto server = BabyDI::Get<Server>(); server != nullptr && !server->hasNPCServer())
		return nullptr;

	LevelExplosion explo{ .position = position, .power = power, .direction = direction };
	explo.timeout.runOnceFor(ExplosionDuration);
	m_explosions.emplace_back(std::move(explo));
	return &m_explosions.back();
}

bool Level::removeExplosion(size_t index)
{
	if (index >= m_explosions.size())
		return false;

	m_explosions.erase(m_explosions.begin() + index);
	return true;
}

bool Level::removeExplosion(const PixelPosition& position)
{
	for (size_t i = 0; i < m_explosions.size(); ++i)
	{
		LevelExplosion& explosion = m_explosions[i];
		if (explosion.position == position)
			return removeExplosion(i);
	}
	return false;
}

LevelExplosion* Level::getExplosion(size_t index) const
{
	if (index >= m_explosions.size())
		return nullptr;
	return const_cast<LevelExplosion*>(&m_explosions[index]);
}

//----------------------------

LevelHorse* Level::addHorse(inform_client_t, std::string_view image, const PixelPosition& position, uint8_t direction, uint8_t bushes)
{
	auto result = addHorse(image, position, direction, bushes);
	if (result != nullptr)
	{
		CString packet = CString() >> (char)PLO_HORSEADD << result->getPacket();
		BabyDI::Get<Server>()->sendPacketToOneLevel(packet, shared_from_this());
	}
	return result;
}

LevelHorse* Level::addHorse(std::string_view image, const PixelPosition& position, uint8_t direction, uint8_t bushes)
{
	auto server = BabyDI::Get<Server>();
	auto horseLife = server->getSettings().getInt("horselifetime", 30);

	LevelHorse newHorse{ .position = position, .image = std::string{ image }, .direction = direction, .bushes = bushes, .timeout = TimeoutGenerator(std::chrono::seconds(horseLife)) };
	if (isOnWater(position.translate(16, 32)))
		newHorse.type = HORSETYPE_BOAT;

	newHorse.timeout.runOnceFor(std::chrono::seconds(horseLife));
	m_horses.emplace_back(std::move(newHorse));
	return &m_horses.back();
}

bool Level::removeHorse(inform_client_t, size_t index)
{
	if (index < m_horses.size())
	{
		auto localPosition = toLocalPixelPosition(m_horses[index].position);
		CString packet = CString() >> (char)PLO_HORSEDEL >> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8);
		BabyDI::Get<Server>()->sendPacketToOneLevel(packet, this->shared_from_this());
	}
	return removeHorse(index);
}

bool Level::removeHorse(size_t index)
{
	if (index >= m_horses.size())
		return false;

	m_horses.erase(m_horses.begin() + index);
	return true;
}

bool Level::removeHorse(const PixelPosition& position)
{
	for (size_t i = 0; i < m_horses.size(); ++i)
	{
		LevelHorse& horse = m_horses[i];
		if (horse.position == position)
			return removeHorse(i);
	}
	return false;
}

LevelHorse* Level::getHorse(size_t index) const
{
	if (index >= m_horses.size())
		return nullptr;
	return const_cast<LevelHorse*>(&m_horses[index]);
}

//----------------------------

LevelItem* Level::addItem(inform_client_t, const PixelPosition& position, LevelItemType item)
{
	auto result = addItem(position, item);
	if (result != nullptr)
	{
		auto localPosition = toLocalPixelPosition(result->position);
		BabyDI::Get<Server>()->sendPacketToOneLevel(CString() >> (char)PLO_ITEMADD
			>> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8) >> (char)LevelItem::getItemTypeId(result->item),
			shared_from_this());
	}
	return result;
}

LevelItem* Level::addItem(const PixelPosition& position, LevelItemType item)
{
	auto server = BabyDI::Get<Server>();
	if (server->hasNPCServer())
	{
		// If we were able to generate the item NPC, don't add the item to the ground.
		if (auto itemNPC = generateItemNPC(position, item); itemNPC != nullptr)
			return nullptr;
	}

	LevelItem newItem{ .position = position, .item = item, .modTime = server->getFrameStartTime() };
	newItem.timeout.runOnceFor(LevelItemTimeout);
	m_items.emplace_back(std::move(newItem));
	return &m_items.back();
}

bool Level::removeItem(inform_client_t, size_t index)
{
	if (index < m_items.size())
	{
		auto localPosition = toLocalPixelPosition(m_items[index].position);
		CString packet = CString() >> (char)PLO_ITEMDEL >> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8);
		BabyDI::Get<Server>()->sendPacketToOneLevel(packet, this->shared_from_this());
	}
	return removeItem(index);
}

bool Level::removeItem(size_t index)
{
	if (index >= m_items.size())
		return false;

	m_items.erase(m_items.begin() + index);
	return true;
}

LevelItemType Level::removeItem(const PixelPosition& position)
{
	for (auto i = m_items.begin(); i != m_items.end(); ++i)
	{
		LevelItem& item = *i;
		if (item.position == position)
		{
			LevelItemType itemType = item.item;
			m_items.erase(i);
			return itemType;
		}
	}

	return LevelItemType::INVALID;
}

LevelItem* Level::getItem(size_t index) const
{
	if (index >= m_items.size())
		return nullptr;
	return const_cast<LevelItem*>(&m_items[index]);
}

//----------------------------

LevelLink* Level::addLink(const std::vector<CString>& link)
{
	LevelLink newLink{ link };
	m_links.emplace_back(std::move(newLink));
	return &m_links.back();
}

bool Level::removeLink(uint32_t index)
{
	if (m_links.empty() || index < 0 || index > m_links.size())
		return false;

	m_links.erase(m_links.begin() + index);
	return true;
}

std::optional<const LevelLink*> Level::getLink(const LocalWholeTilePosition& position, bool excludeOverworld) const
{
	for (const auto& link : m_links)
	{
		if (excludeOverworld && link.isProbableMapLink())
			continue;

		auto& bbox = link.getBoundingBox();
		if ((position.x() >= bbox.position.x() && position.x() <= bbox.position.x() + bbox.size.width())
			&& (position.y() >= bbox.position.y() && position.y() <= bbox.position.y() + bbox.size.height()))
		{
			return std::make_optional(&link);
		}
	}

	return std::nullopt;
}

//----------------------------

LevelShoot* Level::addShoot(LevelShoot* existingShoot)
{
	if (existingShoot == nullptr)
		return nullptr;

	m_shoots.push_back(*existingShoot);
	return &m_shoots.back();
}

LevelShoot* Level::addShoot(inform_client_t, const PixelPosition& position, float angle, float zangle, uint8_t power, float gravity, const std::string& gani, ScriptObject from)
{
	if (auto server = BabyDI::Get<Server>(); server != nullptr && !server->hasNPCServer())
		return nullptr;

	auto result = addShoot(position, angle, zangle, power, gravity, gani, from);
	if (result != nullptr)
		BabyDI::Get<Server>()->sendShootToOneLevel(result, shared_from_this());

	return result;
}

LevelShoot* Level::addShoot(const PixelPosition& position, float angle, float zangle, uint8_t power, float gravity, const std::string& gani, ScriptObject from)
{
	if (auto server = BabyDI::Get<Server>(); server != nullptr && !server->hasNPCServer())
		return nullptr;

	LevelShoot newShoot{ .position = toTilePosition(position), .angle = angle, .zangle = zangle, .powerIn44Pixels = power, .gani = gani, .gravity = gravity, .from = from };
	if (newShoot.gani.back() == ',')
		newShoot.gani.pop_back();
	newShoot.calculateSpeeds();
	m_shoots.emplace_back(std::move(newShoot));
	return &m_shoots.back();
}

LevelShoot* Level::addShoot(const PixelPosition& position, uint8_t angle, uint8_t zangle, uint8_t power, float gravity, const std::string& gani, ScriptObject from)
{
	auto pi = std::numbers::pi_v<float>;
	return addShoot(position, (angle / 220.0f) * (2 * pi), ((zangle / 110.0f) - 1.0f) * (pi / 2), power, gravity, gani, from);
}

bool Level::removeShoot(uint8_t index)
{
	if (index >= m_shoots.size())
		return false;

	m_shoots.erase(m_shoots.begin() + index);
	return true;
}

LevelShoot* Level::getShoot(uint8_t index) const
{
	if (index >= m_shoots.size())
		return nullptr;
	return const_cast<LevelShoot*>(&m_shoots[index]);
}

//----------------------------

LevelSign* Level::addSign(const LocalWholeTilePosition& position, const CString& sign, bool encoded)
{
	LevelSign newSign{ position, sign, encoded };
	m_signs.emplace_back(std::move(newSign));
	return &m_signs.back();
}

bool Level::removeSign(uint32_t index)
{
	if (m_signs.empty() || index < 0 || index > m_signs.size())
		return false;

	m_signs.erase(m_signs.begin() + index);
	return true;
}

LevelSign* Level::getSign(size_t index) const
{
	if (index >= m_signs.size())
		return nullptr;
	return const_cast<LevelSign*>(&m_signs[index]);
}

//----------------------------

bool Level::moveShoot(LevelShoot* shoot, int iterations)
{
	if (shoot == nullptr)
		return false;

	for (int i = 0; i < iterations; ++i)
	{
		// If the shoot is out of bounds, try to move it to the new level, else delete it.
		auto localPosition = toLocalPixelPosition(shoot->position);
		if (localPosition.x() < 0 || localPosition.x() > 1024 || localPosition.y() < 0 || localPosition.y() > 1024)
		{
			// If we are out of the limits of the map, just delete it.
			auto mapSize = getMapSizeInParts();
			if ((localPosition.x() < 0 && mapPosition.x() == 0) ||
				(localPosition.y() < 0 && mapPosition.y() == 0) ||
				(localPosition.x() > 1024 && mapPosition.x() == mapSize.width() - 1) ||
				(localPosition.y() > 1024 && mapPosition.y() == mapSize.height() - 1))
			{
				return false;
			}

			// Move to the new level.
			if (m_map)
			{
				LevelPtr nextLevel;
				if (localPosition.x() < 0)
					nextLevel = m_map->getLevelAt(static_cast<uint8_t>(mapPosition.x() - 1), mapPosition.y());
				else if (localPosition.x() > 1024)
					nextLevel = m_map->getLevelAt(static_cast<uint8_t>(mapPosition.x() + 1), mapPosition.y());
				else if (localPosition.y() < 0)
					nextLevel = m_map->getLevelAt(mapPosition.x(), static_cast<uint8_t>(mapPosition.y() - 1));
				else if (localPosition.y() > 1024)
					nextLevel = m_map->getLevelAt(mapPosition.x(), static_cast<uint8_t>(mapPosition.y() + 1));

				if (nextLevel != nullptr)
				{
					auto newShoot = nextLevel->addShoot(shoot);

					// If the new level was already processed this frame, have it process the new shoot.
					if (newShoot != nullptr && nextLevel->getLastFrameTime() == m_lastFrameTime)
						nextLevel->moveShoot(newShoot, iterations);
				}
			}

			return false;
		}

		// Move the shoot.
		shoot->move();

		// If the Z is <= 3, and we aren't going up, check for walls and NPCs.
		// TODO: Player / NPC checks should be 0 <= (shoot Z - other Z) <= 3.
		if (shoot->position.z() <= 3.0f && (DoubleIsZero(shoot->movementPerFrame.z()) || shoot->movementPerFrame.z() <= 0.0))
		{
			auto server = BabyDI::Get<Server>();
			bool collided = false;
			std::string eventParams;

			auto constructEventParams = [&collided, &eventParams, &shoot]()
			{
				if (!eventParams.empty()) return;
				collided = true;
				eventParams = std::format("{},{}", shoot->position.x(), shoot->position.y());
				if (!shoot->shootParams.empty())
				{
					eventParams += ",";
					eventParams += string::toCSV(shoot->shootParams);
				}
			};

			// Check for NPC collisions.
			PixelPosition searchPosition = toPixelPosition(shoot->position).translate(8_i16, 16_i16);
			bool fromPlayer = (shoot->from.second == ScriptObjectType::PLAYER);
			for (const auto& npc : findIntersectingNPCsForCollision({ searchPosition, { 32_ui16, 32_ui16 } }))
			{
				if (shoot->from.second == ScriptObjectType::NPC && shoot->from.first == npc)
					continue;
				if (auto npcPtr = server->getNPC(npc); npcPtr != nullptr)
				{
					constructEventParams();
					npcPtr->scripting.events.addEvent(ScriptEventType::TRIGGERACTION, shoot->from, (fromPlayer ? "projectile" : "sprojectile"), eventParams);
				}
			}

			// Check for wall collisions.
			if (!collided && isOnWall2({ toLocalWholeTilePosition(searchPosition), { 1_ui8, 1_ui8 } }))
				constructEventParams();

			// Check if we hit the ground.
			if (DoubleIsZero(shoot->position.z()) || shoot->position.z() < 0)
				constructEventParams();

			// We collided, so tell the control-NPC and delete the shoot projectile.
			if (collided)
			{
				server->getNPCServer()->addEventToControlNPC(ScriptEventType::TRIGGERACTION, shoot->from, (fromPlayer ? "projectile" : "sprojectile"), eventParams);
				return false;
			}
		}
	}

	return true;
}

bool Level::moveArrow(LevelArrow* arrow, int iterations)
{
	if (arrow == nullptr)
		return false;

	for (int i = 0; i < iterations; ++i)
	{
		// Move the arrow.
		arrow->position.translate(arrow->speed.x(), arrow->speed.y());

		// If the arrow has gone out of bounds, delete it.
		// TODO: Maybe just set a max range and make it behave like a shoot?  Like the sync distance?  Or 2 levels distance?
		auto localPosition = toLocalPixelPosition(arrow->position);
		if (localPosition.x() < 0 || localPosition.x() > 1024 || localPosition.y() < 0 || localPosition.y() > 1024)
			return false;

		bool produceExplosion = false;

		// Check for NPC collision.
		PixelRectangleArea searchBox = { translatePosition(arrow->position, 16_i32, 0_i32), { 32_ui16, 32_ui16  } };
		for (const auto& npc : findIntersectingNPCsForCollision(searchBox))
		{
			if (arrow->from.second == ScriptObjectType::NPC && arrow->from.first == npc)
				continue;
			if (auto npcPtr = BabyDI::Get<Server>()->getNPC(npc); npcPtr != nullptr)
			{
				npcPtr->scripting.events.addEvent(ScriptEventType::WASSHOT, arrow->from);
				produceExplosion = (arrow->type == arrowTypeFireblast || arrow->type == arrowTypeNukeshot);
			}
		}

		// If the arrow is a fireblast or nukeshot, check for walls.
		if (!produceExplosion && (arrow->type == arrowTypeFireblast || arrow->type == arrowTypeNukeshot))
		{
			if (isOnWall(toLocalWholeTilePosition(arrow->position).translate(1_ui8, 0_ui8)))
				produceExplosion = true;
		}

		// If we are producing an explosion, add it and remove the arrow.
		if (produceExplosion)
		{
			addExplosion(arrow->position, arrow->from, 1_ui8, 1_ui8);
			return false;
		}
	}

	return true;
}

//----------------------------

bool Level::isOnWall(const LocalWholeTilePosition& tilePosition) const noexcept
{
	if (tilePosition.x() > 63 || tilePosition.y() > 63)
		return true;

	return tiletypes[getTiles(0)[static_cast<size_t>(tilePosition.y()) * 64 + tilePosition.x()]] >= 20;
}

bool Level::isOnWall(const PixelPosition& position) const noexcept
{
	if (!isOnGmap())
		return isOnWall(toLocalWholeTilePosition(position));

	if (auto level = m_map->getLevelAt(position); level != nullptr)
		return level->isOnWall(toLocalWholeTilePosition(position));

	return false;
}

bool Level::isOnWall2(const LocalWholeTileRectangleArea& tileArea) const noexcept
{
	for (auto cy = tileArea.position.y(); cy < tileArea.position.y() + tileArea.size.height(); ++cy)
	{
		for (auto cx = tileArea.position.x(); cx < tileArea.position.x() + tileArea.size.width(); ++cx)
		{
			if (isOnWall(LocalWholeTilePosition{ cx, cy }))
				return true;
		}
	}
	return false;
}

bool Level::isOnWall2(const PixelRectangleArea& area) const noexcept
{
	if (!isOnGmap())
		return isOnWall2(toLocalWholeTileRectangleArea({ 0, 0 }, area));

	for (LevelPtr level : m_map->getLevelsInRectangle(area))
	{
		if (level->isOnWall2(toLocalWholeTileRectangleArea(level->getMapPixelOffset(), area)))
			return true;
	}

	return false;
}

bool Level::isOnWater(const LocalWholeTilePosition& tilePosition) const noexcept
{
	return (tiletypes[getTiles(0)[static_cast<size_t>(tilePosition.y()) * 64 + tilePosition.x()]] == 11);
}

bool Level::isOnWater(const PixelPosition& position) const noexcept
{
	if (!isOnGmap())
		return isOnWater(toLocalWholeTilePosition(position));

	if (auto level = m_map->getLevelAt(position); level != nullptr)
		return level->isOnWater(toLocalWholeTilePosition(position));

	return false;
}

bool Level::isOnWater2(const LocalWholeTileRectangleArea& tileArea) const noexcept
{
	for (auto cy = tileArea.position.y(); cy < tileArea.position.y() + tileArea.size.height(); ++cy)
	{
		for (auto cx = tileArea.position.x(); cx < tileArea.position.x() + tileArea.size.width(); ++cx)
		{
			if (isOnWater(LocalWholeTilePosition{ cx, cy }))
				return true;
		}
	}
	return false;
}

bool Level::isOnWater2(const PixelRectangleArea& area) const noexcept
{
	if (!isOnGmap())
		return isOnWater2(toLocalWholeTileRectangleArea({ 0, 0 }, area));

	for (LevelPtr level : m_map->getLevelsInRectangle(area))
	{
		if (level->isOnWater2(toLocalWholeTileRectangleArea(level->getMapPixelOffset(), area)))
			return true;
	}

	return false;
}

bool Level::isOnPlayer(const PixelPosition& position) const noexcept
{
	auto server = BabyDI::Get<Server>();
	for (const auto& playerId : findInRangePlayers(position))
	{
		if (auto player = server->getPlayer(playerId); player != nullptr)
		{
			if (positionInRectangle(position, player->getBoundingBox()))
				return true;
		}
	}
	return false;
}

bool Level::isOnPlayer(const PixelRectangleArea& pixelArea) const noexcept
{
	auto server = BabyDI::Get<Server>();
	for (const auto& playerId : findInRangePlayers(pixelArea.position))
	{
		if (auto player = server->getPlayer(playerId); player != nullptr)
		{
			if (rectanglesIntersect(pixelArea, player->getBoundingBox()))
				return true;
		}
	}
	return false;
}

//----------------------------

uint16_t* Level::getMapTileForEditing(const TilePosition& position) noexcept
{
	auto mapTileOrigin = mapPosition * 64;
	if (position.x() >= mapTileOrigin.x() && position.x() < mapTileOrigin.x() + 64
		&& position.y() >= mapTileOrigin.y() && position.y() < mapTileOrigin.y() + 64)
	{
		auto localTilePos = toLocalWholeTilePosition(position);
		return &m_scriptUpdatedTiles[static_cast<size_t>(localTilePos.y()) * 64 + localTilePos.x()];
	}

	if (isOnGmap() && m_map != nullptr)
	{
		auto level = m_map->getLevelAt(toPixelPosition(position));
		return level->getMapTileForEditing(position);
	}

	return nullptr;
}

//----------------------------

std::generator<const PlayerID&> Level::getMapPlayers() const noexcept
{
	if (!isOnGmap())
		co_yield std::ranges::elements_of(m_players);
	else
	{
		for (const auto& levelPtr : m_map->getAllLevels())
			co_yield std::ranges::elements_of(levelPtr->m_players);
	}
}

std::generator<const NPCID&> Level::getMapNPCs() const noexcept
{
	if (!isOnGmap())
		co_yield std::ranges::elements_of(m_npcs);
	else
	{
		for (const auto& levelPtr : m_map->getAllLevels())
			co_yield std::ranges::elements_of(levelPtr->m_npcs);
	}
}

std::generator<LevelArrow&> Level::getMapArrows() noexcept
{
	if (!isOnGmap())
		co_yield std::ranges::elements_of(m_arrows);
	else
	{
		for (const auto& levelPtr : m_map->getAllLevels())
			co_yield std::ranges::elements_of(levelPtr->m_arrows);
	}
}

std::generator<LevelBomb&> Level::getMapBombs() noexcept
{
	if (!isOnGmap())
		co_yield std::ranges::elements_of(m_bombs);
	else
	{
		for (const auto& levelPtr : m_map->getAllLevels())
			co_yield std::ranges::elements_of(levelPtr->m_bombs);
	}
}

std::generator<LevelExplosion&> Level::getMapExplosions() noexcept
{
	if (!isOnGmap())
		co_yield std::ranges::elements_of(m_explosions);
	else
	{
		for (const auto& levelPtr : m_map->getAllLevels())
			co_yield std::ranges::elements_of(levelPtr->m_explosions);
	}
}

std::generator<LevelHorse&> Level::getMapHorses() noexcept
{
	if (!isOnGmap())
		co_yield std::ranges::elements_of(m_horses);
	else
	{
		for (const auto& levelPtr : m_map->getAllLevels())
			co_yield std::ranges::elements_of(levelPtr->m_horses);
	}
}

std::generator<LevelItem&> Level::getMapItems() noexcept
{
	if (!isOnGmap())
		co_yield std::ranges::elements_of(m_items);
	else
	{
		for (const auto& levelPtr : m_map->getAllLevels())
			co_yield std::ranges::elements_of(levelPtr->m_items);
	}
}

std::generator<LevelSign&> Level::getMapSigns() noexcept
{
	if (!isOnGmap())
		co_yield std::ranges::elements_of(m_signs);
	else
	{
		for (const auto& levelPtr : m_map->getAllLevels())
			co_yield std::ranges::elements_of(levelPtr->m_signs);
	}
}

size_t Level::getMapPlayerCount() const noexcept
{
	if (!isOnGmap())
		return m_players.size();

	size_t result = 0;
	for (const auto& levelPtr : m_map->getAllLevels())
		result += levelPtr->m_players.size();

	return result;
}

size_t Level::getMapNPCCount() const noexcept
{
	if (!isOnGmap())
		return m_npcs.size();

	size_t result = 0;
	for (const auto& levelPtr : m_map->getAllLevels())
		result += levelPtr->m_npcs.size();

	return result;
}

size_t Level::getMapArrowCount() const noexcept
{
	if (!isOnGmap())
		return m_arrows.size();

	size_t result = 0;
	for (const auto& levelPtr : m_map->getAllLevels())
		result += levelPtr->m_arrows.size();

	return result;
}

size_t Level::getMapBombCount() const noexcept
{
	if (!isOnGmap())
		return m_bombs.size();

	size_t result = 0;
	for (const auto& levelPtr : m_map->getAllLevels())
		result += levelPtr->m_bombs.size();

	return result;
}

size_t Level::getMapExplosionCount() const noexcept
{
	if (!isOnGmap())
		return m_explosions.size();

	size_t result = 0;
	for (const auto& levelPtr : m_map->getAllLevels())
		result += levelPtr->m_explosions.size();

	return result;
}

size_t Level::getMapHorseCount() const noexcept
{
	if (!isOnGmap())
		return m_horses.size();

	size_t result = 0;
	for (const auto& levelPtr : m_map->getAllLevels())
		result += levelPtr->m_horses.size();

	return result;
}

size_t Level::getMapItemCount() const noexcept
{
	if (!isOnGmap())
		return m_items.size();

	size_t result = 0;
	for (const auto& levelPtr : m_map->getAllLevels())
		result += levelPtr->m_items.size();

	return result;
}

size_t Level::getMapSignCount() const noexcept
{
	if (!isOnGmap())
		return m_signs.size();

	size_t result = 0;
	for (const auto& levelPtr : m_map->getAllLevels())
		result += levelPtr->m_signs.size();

	return result;
}

std::optional<LevelArrow*> Level::getMapArrow(size_t index) noexcept
{
	auto objects = getMapArrows();
	auto iter = objects.begin();
	std::ranges::advance(iter, index, objects.end());
	if (iter == objects.end())
		return std::nullopt;
	return std::make_optional(&(*iter));
}

std::optional<LevelBomb*> Level::getMapBomb(size_t index) noexcept
{
	auto objects = getMapBombs();
	auto iter = objects.begin();
	std::ranges::advance(iter, index, objects.end());
	if (iter == objects.end())
		return std::nullopt;
	return std::make_optional(&(*iter));
}

std::optional<LevelExplosion*> Level::getMapExplosion(size_t index) noexcept
{
	auto objects = getMapExplosions();
	auto iter = objects.begin();
	std::ranges::advance(iter, index, objects.end());
	if (iter == objects.end())
		return std::nullopt;
	return std::make_optional(&(*iter));
}

std::optional<LevelHorse*> Level::getMapHorse(size_t index) noexcept
{
	auto objects = getMapHorses();
	auto iter = objects.begin();
	std::ranges::advance(iter, index, objects.end());
	if (iter == objects.end())
		return std::nullopt;
	return std::make_optional(&(*iter));
}

std::optional<LevelItem*> Level::getMapItem(size_t index) noexcept
{
	auto objects = getMapItems();
	auto iter = objects.begin();
	std::ranges::advance(iter, index, objects.end());
	if (iter == objects.end())
		return std::nullopt;
	return std::make_optional(&(*iter));
}

std::optional<LevelSign*> Level::getMapSign(size_t index) noexcept
{
	auto objects = getMapSigns();
	auto iter = objects.begin();
	std::ranges::advance(iter, index, objects.end());
	if (iter == objects.end())
		return std::nullopt;
	return std::make_optional(&(*iter));
}

//----------------------------

std::generator<const PlayerID&> Level::findInRangePlayers(const PixelPosition& position) const noexcept
{
	// Bigmap test.
	// We only want to search for players in our level.
	if (isOnBigMap())
	{
		co_yield std::ranges::elements_of(m_players);
		co_return;
	}

	auto server = BabyDI::Get<Server>();
	bool syncInside = server->getSettings().getBool("syncbydistanceinside", false);

	unsigned int syncx = (unsigned int)server->getSettings().getInt("syncdistancex", 192);
	unsigned int syncy = (unsigned int)server->getSettings().getInt("syncdistancey", 192);
	auto mapSize = getMapSizeInTiles();
	auto tilePosition = toTilePosition(position);

	// If this is an inside level and we aren't going to sync by distance inside,
	// or the sync distance is larger than the level, return all the level players.
	if (m_map == nullptr && (!syncInside || (syncx >= mapSize.width() && syncy >= mapSize.height())))
	{
		co_yield std::ranges::elements_of(m_players);
		co_return;
	}

	auto playerInRange = [&](const PlayerID& playerId)
	{
		if (auto player = server->getPlayer(playerId); player != nullptr)
		{
			auto otherTilePosition = player->getTilePosition();
			return std::abs(tilePosition.x() - otherTilePosition.x()) <= syncx && std::abs(tilePosition.y() - otherTilePosition.y()) <= syncy;
		}
		return false;
	};

	// Inside level.
	if (m_map == nullptr)
	{
		for (const auto& playerId : m_players)
		{
			if (playerInRange(playerId))
				co_yield playerId;
		}
		co_return;
	}

	// Map levels.
	for (LevelPtr level : m_map->getLevelsInRange(tilePosition, syncx, syncy))
	{
		for (const auto& playerId : level->m_players)
		{
			if (playerInRange(playerId))
				co_yield playerId;
		}
	}
}

std::generator<const PlayerID&> Level::findInRangePlayersForCommunication(const PixelPosition& position) const noexcept
{
	// If this is not a bigmap, use the default search.
	if (!isOnBigMap())
	{
		co_yield std::ranges::elements_of(findInRangePlayers(position));
		co_return;
	}

	int startX = mapPosition.x() - 1, endX = mapPosition.x() + 1;
	int startY = mapPosition.y() - 1, endY = mapPosition.y() + 1;

	if (startX < 0) startX = 0;
	if (startY < 0) startY = 0;
	if (endX >= m_map->size.width()) endX = m_map->size.width() - 1;
	if (endY >= m_map->size.height()) endY = m_map->size.height() - 1;

	for (int y = startY; y <= endY; ++y)
	{
		for (int x = startX; x <= endX; ++x)
		{
			if (auto level = m_map->getLevelAt(x, y); level != nullptr)
				co_yield std::ranges::elements_of(level->m_players);
		}
	}
}

std::generator<const NPCID&> Level::findInRangeNPCs(const PixelPosition& position) const noexcept
{
	auto server = BabyDI::Get<Server>();
	bool syncInside = server->getSettings().getBool("syncbydistanceinside", false);

	// If this is an inside level and we aren't going to sync by distance inside, return all level NPCs.
	if (!isOnGmap() && !syncInside)
	{
		co_yield std::ranges::elements_of(m_npcs);
		co_return;
	}

	unsigned int syncx = (unsigned int)server->getSettings().getInt("syncdistancex", 192);
	unsigned int syncy = (unsigned int)server->getSettings().getInt("syncdistancey", 192);
	auto mapSize = getMapSizeInTiles();
	auto tilePosition = toTilePosition(position);

	auto npcInRange = [&](const NPCID& npcId)
	{
		if (auto npc = server->getNPC(npcId); npc != nullptr)
		{
			auto otherTilePosition = npc->getTilePosition();
			return std::abs(tilePosition.x() - otherTilePosition.x()) <= syncx && std::abs(tilePosition.y() - otherTilePosition.y()) <= syncy;
		}
		return false;
	};

	// Inside level (or bigmap), so just check the NPCs for the level.
	if (m_map == nullptr || m_map->isBigMap())
	{
		// Sync is greater than the level bounds so return all the NPCs.
		if (syncx >= mapSize.width() && syncy >= mapSize.height())
		{
			co_yield std::ranges::elements_of(m_npcs);
			co_return;
		}

		for (const auto& npcId : m_npcs)
		{
			if (npcInRange(npcId))
				co_yield npcId;
		}
		co_return;
	}

	// Gmaps.
	for (LevelPtr level : m_map->getLevelsInRange(toTilePosition(position), syncx, syncy))
	{
		for (const auto& npcId : level->m_npcs)
		{
			if (npcInRange(npcId))
				co_yield npcId;
		}
	}
}

std::generator<const NPCID&> Level::findInRangeNPCsByDistance(const PixelPosition& position, uint32_t tileDistance) const noexcept
{
	// If this is not a map level, return all level NPCs.
	if (!isOnGmap())
	{
		co_yield std::ranges::elements_of(m_npcs);
		co_return;
	}

	auto server = BabyDI::Get<Server>();
	auto tilePosition = toTilePosition(position);

	auto npcInRange = [&](const NPCID& npcId)
	{
		if (auto npc = server->getNPC(npcId); npc != nullptr)
		{
			auto otherTilePosition = npc->getTilePosition();
			auto distance = std::hypotf(tilePosition.x() - otherTilePosition.x(), tilePosition.y() - otherTilePosition.y());
			return distance <= tileDistance;
		}
		return false;
	};

	for (LevelPtr level : m_map->getLevelsInRange(tilePosition, tileDistance, tileDistance))
	{
		for (const auto& npcId : level->m_npcs)
		{
			if (npcInRange(npcId))
				co_yield npcId;
		}
	}
}

std::generator<const NPCID&> Level::findIntersectingNPCs(const PixelPosition& position, bool includeInvisible) const noexcept
{
	for (const auto& id : findIntersectingNPCs({ { position.x(), position.y() }, { 0, 0 } }, includeInvisible))
		co_yield id;
}

std::generator<const NPCID&> Level::findIntersectingNPCs(const PixelRectangleArea& area, bool includeInvisible) const noexcept
{
	auto server = BabyDI::Get<Server>();
	for (const auto& npcId : findInRangeNPCs(area.position))
	{
		if (auto npc = server->getNPC(npcId); npc != nullptr)
		{
			// If the NPC is invisible and we don't want to include invisible NPCs, skip it.
			if (!includeInvisible && (npc->visFlags & PROPID(NPCVisFlags::VISIBLE)) == 0)
				continue;

			// Check if the NPC intersects with the area.
			if (rectanglesIntersect(area, npc->getBoundingBox()))
				co_yield npcId;
		}
	}
}

std::generator<const NPCID&> Level::findIntersectingNPCsForCollision(const PixelPosition& position) const noexcept
{
	for (const auto& id : findIntersectingNPCsForCollision({ position, { 0, 0 } }))
		co_yield id;
}

std::generator<const NPCID&> Level::findIntersectingNPCsForCollision(const PixelRectangleArea& area) const noexcept
{
	auto server = BabyDI::Get<Server>();
	for (const auto& npcId : findInRangeNPCs(area.position))
	{
		if (auto npc = server->getNPC(npcId); npc != nullptr)
		{
			// If the NPC is invisible, skip it.
			if ((npc->visFlags & PROPID(NPCVisFlags::VISIBLE)) == 0)
				continue;

			// Check if the NPC intersects with the area.
			if (rectanglesIntersect(area, npc->getCollisionBoundingBox()))
				co_yield npcId;
		}
	}
}

//----------------------------

std::shared_ptr<NPC> Level::generateItemNPC(const PixelPosition& position, LevelItemType item)
{
	auto server = BabyDI::Get<Server>();
	if (!server->hasNPCServer())
		return nullptr;

	auto itemName = LevelItem::getItemName(item);
	if (LevelItem::isRupeeType(item))
		itemName = "gralats";

	auto itemclass = server->getNPCServer()->getClass(itemName).lock();
	if (itemclass == nullptr)
		return nullptr;

	static std::unordered_map<LevelItemType, NPCProp> stackableItems =
	{
		{ LevelItemType::GREENRUPEE, NPCProp::RUPEES },
		{ LevelItemType::BLUERUPEE, NPCProp::RUPEES },
		{ LevelItemType::REDRUPEE, NPCProp::RUPEES },
		{ LevelItemType::GOLDRUPEE, NPCProp::RUPEES },
		{ LevelItemType::BOMBS, NPCProp::BOMBS },
		{ LevelItemType::DARTS, NPCProp::ARROWS },
		{ LevelItemType::HEART, NPCProp::POWER },
	};
	static std::unordered_map<LevelItemType, uint32_t> stackableCount =
	{
		{ LevelItemType::GREENRUPEE, 1 },
		{ LevelItemType::BLUERUPEE, 5 },
		{ LevelItemType::REDRUPEE, 30 },
		{ LevelItemType::GOLDRUPEE, 100 },
		{ LevelItemType::BOMBS, 5 },
		{ LevelItemType::DARTS, 5 },
		{ LevelItemType::HEART, 2 },
	};

	std::shared_ptr<NPC> itemNPC = nullptr;

	// Determine the NPC location.
	TilePosition loc = toTilePosition(position).translate(-0.5f, -1.0f);

	auto stackable = stackableItems.find(item);
	if (stackable != stackableItems.end())
	{
		// Find existing items, and stack with the existing.
		PixelRectangleArea searchArea{ toPixelPosition(loc).translate(-2 * 16, -2 * 16), { 6 * 16, 6 * 16 } };
		auto npcList = findIntersectingNPCs(searchArea);
		for (const auto& npcId : npcList)
		{
			if (auto npc = server->getNPC(npcId); npc != nullptr && npc->hasJoinedClass(itemName))
				itemNPC = npc;
		}
	}

	// Create a new npc for this item.
	bool isNew = !itemNPC;
	if (isNew)
	{
		itemNPC = server->getNPCServer()->addNPC("", std::format("if (created) join {};", itemName), shared_from_this(), { loc[0], loc[1] }, NPCTYPE_ITEM);
		itemNPC->character.gralats = itemNPC->character.arrows = itemNPC->character.bombs = itemNPC->character.hitpointsInHalves = 0;
	}

	// If this NPC has stackable items, set the count.
	if (stackable != stackableItems.end())
	{
		uint8_t stackCount = stackableCount[item];
		props::SetResults results;
		switch (stackable->second)
		{
			case NPCProp::RUPEES:
				results = itemNPC->setPropWith<NPCProp::RUPEES>(props::SetBy::SERVER, static_cast<props::GBYTE3>(itemNPC->getProp<NPCProp::RUPEES>().value + stackCount));
				break;
			case NPCProp::BOMBS:
				results = itemNPC->setPropWith<NPCProp::BOMBS>(props::SetBy::SERVER, static_cast<props::GBYTE1>(itemNPC->getProp<NPCProp::BOMBS>().value + stackCount));
				break;
			case NPCProp::ARROWS:
				results = itemNPC->setPropWith<NPCProp::ARROWS>(props::SetBy::SERVER, static_cast<props::GBYTE1>(itemNPC->getProp<NPCProp::ARROWS>().value + stackCount));
				break;
			case NPCProp::POWER:
				results = itemNPC->setPropWith<NPCProp::POWER>(props::SetBy::SERVER, static_cast<props::GBYTE1>(itemNPC->getProp<NPCProp::POWER>().value + stackCount));
				break;
		}
		itemNPC->sendPropsFromResults(results);
	}

	// Update the item.
	itemNPC->scripting.events.addEvent(ScriptEventType::CUSTOM, source::FromNPC(itemNPC->id), "updategani");
	return itemNPC;
}

//----------------------------

ScriptObject source::FromLevel(LevelPtr level)
{
	size_t hash = string::string_hash{}(level->levelName);
	return std::make_pair(hash, ScriptObjectType::LEVEL);
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
