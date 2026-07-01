#include <algorithm>
#include <bit>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <list>
#include <memory>
#include <numbers>
#include <optional>
#include <ostream>
#include <span>
#include <string_view>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <BabyDI.h>
#include <CString.h>
#include <IEnums.h>
#include <IUtil.h>

#include <Server.h>
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
#include <level/LevelTileTypes.h>
#include <level/LevelTiles.h>
#include <level/Map.h>
#include <loader/LevelLoader.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/PropertySerializers.h>
#include <utilities/StringUtils.h>
#include <utilities/generator/TimeoutGenerator.h>
#include <utilities/std/generator.h>

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

void StaticLevelData::reload(std::shared_ptr<StaticLevelData> staticData)
{
	// Clear our data.
	staticData->tiles.reset();
	staticData->links.clear();
	staticData->chests.clear();
	staticData->signs.clear();
	staticData->baddies.clear();
	staticData->npcs.clear();
	staticData->heights.clear();

	// Reload the data from disk.
	LevelLoader::loadStaticDataInto(staticData);

	// Notify listeners that the data has been refreshed.
	staticData->onDataRefreshed.post(staticData);
}

std::optional<std::string> StaticLevelData::getChestFormattedForSave(LevelChest* chest) const
{
	if (chest == nullptr)
		return std::nullopt;

	return std::format("{}:{}:{}", chest->position.x(), chest->position.y(), levelName);
}

void StaticLevelData::sendBoardToPlayer(std::shared_ptr<Player> player) const
{
	CString retVal;
	retVal.writeGChar(PLO_BOARDPACKET);
	tiles.writeLayerToPacket(0, retVal);

	player->sendPacket(CString() >> (char)PLO_RAWDATA >> (int)((1 + (64 * 64 * 2) + 1)));
	player->sendPacket(retVal);
}

void StaticLevelData::sendBoardLayersToPlayer(std::shared_ptr<Player> player) const
{
	for (auto layer : tiles.getUsedTileLayers())
	{
		if (layer == 0) continue;
		sendBoardLayerToPlayer(player, layer);
	}
}

void StaticLevelData::sendBoardLayerToPlayer(std::shared_ptr<Player> player, size_t layer) const
{
	CString retVal;
	retVal.writeGChar(PLO_BOARDLAYER);

	// TODO: Only send the tiles that has been placed on the layer
	retVal << (char)layer << (char)0 << (char)0 << (char)64 << (char)64;

	tiles.writeLayerToPacket(layer, retVal);

	// The +1 is the \n at the end of the packet.
	player->sendPacket(CString() >> (char)PLO_RAWDATA >> (int)(retVal.length() + 1));
	player->sendPacket(retVal);
}

void StaticLevelData::sendChestsToPlayer(std::shared_ptr<Player> player) const
{
	CString packet;
	for (auto& chest : chests)
	{
		bool hasChest = player->account.hasChest(levelName, chest.position);

		packet.clear();
		packet >> (char)PLO_LEVELCHEST >> (char)(hasChest ? 1 : 0) >> (char)chest.position.x() >> (char)chest.position.y();
		if (!hasChest) packet >> (char)chest.item >> (char)chest.sign;
		player->sendPacket(packet);
	}
}

void StaticLevelData::sendLinksToPlayer(std::shared_ptr<Player> player, bool onlyMapLinks) const
{
	CString packet;
	for (const auto& link : links)
	{
		if (onlyMapLinks && !link.isProbableMapLink())
			continue;

		packet.clear();
		packet >> (char)PLO_LEVELLINK << link.getLinkStr();
		player->sendPacket(packet);
	}
}

void StaticLevelData::sendSignsToPlayer(std::shared_ptr<Player> player) const
{
	CString packet;
	for (const auto& sign : signs)
	{
		packet.clear();
		packet >> (char)PLO_LEVELSIGN << sign.getSignPacket(player.get());
		player->sendPacket(packet);
	}
}

//----------------------------

PixelRectangleArea SubLevel::clipRectangleToPart(const PixelRectangleArea& area) const noexcept
{
	PixelRectangleArea result{area};
	PixelRectangleArea localRect{{0, 0}, {1024, 1024}};
	if (mapPosition.has_value())
		localRect.position.translate(mapPosition.value().x() * 1024, mapPosition.value().y() * 1024);

	if (localRect.right() < area.left() || localRect.left() > area.right())
	{
		result.position.x() = 0;
		result.size.width() = 0;
	}
	else
	{
		if (localRect.left() > area.left())
		{
			auto diff = localRect.left() - area.left();
			result.position.x() = localRect.position.x();
			result.size.width() = static_cast<uint16_t>(area.size.width() - diff);
		}
		if (localRect.right() < area.right())
		{
			auto diff = area.right() - localRect.right();
			result.position.x() = area.position.x();
			result.size.width() = static_cast<uint16_t>(area.size.width() - diff);
		}
	}

	if (localRect.bottom() < area.top() || localRect.top() > area.bottom())
	{
		result.position.y() = 0;
		result.size.height() = 0;
	}
	else
	{
		if (localRect.top() > area.top())
		{
			auto diff = localRect.top() - area.top();
			result.position.y() = localRect.position.y();
			result.size.height() = static_cast<uint16_t>(area.size.height() - diff);
		}
		if (localRect.bottom() < area.bottom())
		{
			auto diff = area.bottom() - localRect.bottom();
			result.position.y() = area.position.y();
			result.size.height() = static_cast<uint16_t>(area.size.height() - diff);
		}
	}

	return result;
}

WholeTileRectangleArea SubLevel::clipRectangleToPart(const WholeTileRectangleArea& area) const noexcept
{
	WholeTileRectangleArea result{area};
	WholeTileRectangleArea localRect{{0, 0}, {64, 64}};
	if (mapPosition.has_value())
		localRect.position.translate(mapPosition.value().x() * 64, mapPosition.value().y() * 64);

	if (localRect.right() < area.left() || localRect.left() > area.right())
	{
		result.position.x() = 0;
		result.size.width() = 0;
	}
	else
	{
		if (localRect.left() > area.left())
		{
			auto diff = localRect.left() - area.left();
			result.position.x() = localRect.position.x();
			result.size.width() = static_cast<uint8_t>(area.size.width() - diff);
		}
		if (localRect.right() < area.right())
		{
			auto diff = area.right() - localRect.right();
			result.position.x() = area.position.x();
			result.size.width() = static_cast<uint8_t>(area.size.width() - diff);
		}
	}

	if (localRect.bottom() < area.top() || localRect.top() > area.bottom())
	{
		result.position.y() = 0;
		result.size.height() = 0;
	}
	else
	{
		if (localRect.top() > area.top())
		{
			auto diff = localRect.top() - area.top();
			result.position.y() = localRect.position.y();
			result.size.height() = static_cast<uint8_t>(area.size.height() - diff);
		}
		if (localRect.bottom() < area.bottom())
		{
			auto diff = area.bottom() - localRect.bottom();
			result.position.y() = area.position.y();
			result.size.height() = static_cast<uint8_t>(area.size.height() - diff);
		}
	}

	return result;
}

std::optional<LevelTiles*> SubLevel::getTiles() noexcept
{
	// Get the tiles.
	LevelTiles* tiles = nullptr;
	if (instancedTileUpdates.has_value())
		tiles = &instancedTileUpdates.value();
	else if (auto sdata = staticData.lock(); sdata != nullptr)
		tiles = &sdata->tiles;

	// Make sure we found tiles.
	if (tiles == nullptr)
		return std::nullopt;

	return tiles;
}

std::optional<const LevelTiles*> SubLevel::getTiles() const noexcept
{
	// Get the tiles.
	const LevelTiles* tiles = nullptr;
	if (instancedTileUpdates.has_value())
		tiles = &instancedTileUpdates.value();
	else if (auto sdata = staticData.lock(); sdata != nullptr)
		tiles = &sdata->tiles;

	// Make sure we found tiles.
	if (tiles == nullptr)
		return std::nullopt;

	return tiles;
}

std::optional<LevelTiles::TileArray*> SubLevel::getTiles(size_t layer) noexcept
{
	// Get the tiles.
	auto tiles = getTiles();
	if (!tiles.has_value())
		return std::nullopt;

	// Try to get the tiles for the specified layer.
	auto tileLayer = tiles.value()->getLayer(layer);
	if (tileLayer.has_value() && tileLayer.value() != nullptr)
		return tileLayer.value();

	return std::nullopt;
}

std::optional<const LevelTiles::TileArray*> SubLevel::getTiles(size_t layer) const noexcept
{
	// Get the tiles.
	auto tiles = getTiles();
	if (!tiles.has_value())
		return std::nullopt;

	// Try to get the tiles for the specified layer.
	auto tileLayer = tiles.value()->getLayer(layer);
	if (tileLayer.has_value() && tileLayer.value() != nullptr)
		return tileLayer.value();

	return std::nullopt;
}

double SubLevel::getHeightAt(const LocalPixelPosition& position) const noexcept
{
	if (!terrain.has_value() || terrain.value().heightmap.empty())
		return 0.0;

	auto tilePosition = toTilePosition(position);

	// Determine the origin tile for our calculation.
	// This will be the top-left tile within the quad we are calculating the height for.
	LocalWholeTilePosition originTile = toLocalWholeTilePosition(tilePosition);
	if (tilePosition.x() > 64) originTile.x() = 64;
	if (tilePosition.y() > 64) originTile.y() = 64;

	auto heightAtPosition = [&](const TilePosition& pos) -> double
	{
		return terrain.value().heightmap[static_cast<size_t>(pos.y()) * 65 + pos.x()];
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
	TilePosition offset{tilePosition.x() - topLeft.x(), tilePosition.y() - topLeft.y()};

	// Calculate our point using the offset along the direction vectors.
	TilePosition point = topLeft + (vecU * offset.x()) + (vecV * offset.y());

	return point.z();
}

void SubLevel::sendBoardToPlayer(std::shared_ptr<Player> player) const
{
	auto tiles = getTiles();
	if (!tiles.has_value())
		return;

	CString retVal;
	retVal.writeGChar(PLO_BOARDPACKET);
	tiles.value()->writeLayerToPacket(0, retVal);

	player->sendPacket(CString() >> (char)PLO_RAWDATA >> (int)((1 + (64 * 64 * 2) + 1)));
	player->sendPacket(retVal);
}

void SubLevel::sendBoardLayersToPlayer(std::shared_ptr<Player> player) const
{
	auto tiles = getTiles();
	if (!tiles.has_value())
		return;

	for (auto layer : tiles.value()->getUsedTileLayers())
	{
		if (layer == 0) continue;
		sendBoardLayerToPlayer(player, layer);
	}
}

void SubLevel::sendBoardLayerToPlayer(std::shared_ptr<Player> player, size_t layer) const
{
	auto tiles = getTiles();
	if (!tiles.has_value())
		return;

	CString retVal;
	retVal.writeGChar(PLO_BOARDLAYER);

	// TODO: Only send the tiles that has been placed on the layer
	retVal << (char)layer << (char)0 << (char)0 << (char)64 << (char)64;

	tiles.value()->writeLayerToPacket(layer, retVal);

	// The +1 is the \n at the end of the packet.
	player->sendPacket(CString() >> (char)PLO_RAWDATA >> (int)(retVal.length() + 1));
	player->sendPacket(retVal);
}

void SubLevel::sendBoardHeightsToPlayer(std::shared_ptr<Player> player) const
{
	// We only need to send heights if there are level overrides.
	if (!mapPosition.has_value() || !terrain.has_value() || terrain.value().levelHeightOverrides.empty())
		return;

	CString retVal;
	retVal.writeGChar(PLO_BOARDHEIGHTS);

	// Maybe the map position?
	retVal >> (char)mapPosition.value().x() >> (char)mapPosition.value().y();

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
			auto height = terrain.value().levelHeightOverrides[index];

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

void SubLevel::sendBoardChangesToPlayer(std::shared_ptr<Player> player, std::optional<clock::time_point> time) const
{
	if (player == nullptr)
		return;

	// Determine the style of board changes to send.
	// 0 = PLO_BOARDMODIFY2 with pixel position (bad)
	// 1 = PLO_BOARDMODIFY2 with map position
	// 2 = PLO_LEVELBOARD with batched changes
	// 3 = PLO_BOARDMODIFY
	int style = 1;
	if (player->getVersion() < CLVER_2_1)
		style = 3;
	else if (player->getVersion() < CLVER_4_0211)
		style = mapPosition.has_value() ? 1 : 3; // 2;

	// The batched board changes seem to be sent when the player enters a level that it has cached.
	// TODO: The current level sending implementation doesn't easily allow use to use this right now, so send individual changes (it won't hurt things).
	if (style == 2)
	{
		CString retVal;
		retVal >> (char)PLO_LEVELBOARD;
		for (const auto& change : boardChanges)
		{
			if (!time.has_value() || change.modTime >= time.value())
				retVal << change.getPropsForSingleLevel();
		}
		if (retVal.length() > 1)
			player->sendPacket(retVal);
		return;
	}

	// Send all board changes.
	for (const auto& change : boardChanges)
	{
		/*
		if (style == 0)
			player->sendPacket(CString() >> (char)PLO_BOARDMODIFY2 << change.getPropsForMapNewMain());
		else
		*/
		if (style == 1)
			player->sendPacket(CString() >> (char)PLO_BOARDMODIFY2 << change.getPropsForMapClassic());
		else if (style == 3)
			player->sendPacket(CString() >> (char)PLO_BOARDMODIFY << change.getPropsForSingleLevel());
	}
}

//----------------------------

Level::Level()
{
	m_server = BabyDI::Get<Server>();
	assert(m_server != nullptr);
}

Level::~Level()
{
	// Delete NPCs.
	{
		// Remove every NPC in the level.
		for (auto& levelNPC : m_npcs)
		{
			auto npc = m_server->getNPC(levelNPC);
			if (!npc) continue;
			if (npc->storageType == NPCStorageType::LEVEL)
				m_server->deleteNPC(npc, false);
		}
		m_npcs.clear();
	}

	// Erase our levels from the gmap level list.
	if (isGmap())
	{
		auto& gmapLevels = m_server->getGmapLevelList();
		using GT = std::remove_cvref_t<decltype(gmapLevels)>::value_type;
		std::erase_if(gmapLevels, [this](const GT& pair)
		{
			return getSubLevelIndex(pair.first).has_value() && pair.second.expired();
		});

		if (m_server != nullptr && m_server->running)
		{
			// Create stubs for our levels so they can be reloaded later if needed.
			// We need to do this for map levels because we many things refer to the sublevels by name, so we need to link them to a gmap.
			auto stub = m_server->getStubbedLevel(levelName, groupMapName);
			for (const auto& [levelName, position] : m_map->levels)
				gmapLevels.insert({ levelName, stub });
		}
	}

	// Delete shoots.
	m_shoots.clear();

	// Delete arrows.
	m_arrows.clear();

	// Delete items.
	for (size_t i = m_items.size(); i > 0; --i)
		removeItem(inform_client, i - 1);
	m_items.clear();

	// Delete sub level data.
	for (auto& subLevel : m_levelParts)
	{
		subLevel->boardChanges.clear();
		subLevel->instancedTileUpdates.reset();
		subLevel->scriptUpdatedTiles.reset();
		subLevel->isNoPkZone = false;
		subLevel->isSparringZone = false;
	}

	// Warp players out.
	for (const auto& playerId : m_players)
		m_server->warpPlayerToSafePlace(playerId);
}

//----------------------------

std::shared_ptr<Level> Level::createLevel(std::string_view levelName)
{
	auto server = BabyDI::Get<Server>();

	auto level = std::shared_ptr<Level>(new Level());
	level->levelName = levelName;

	if (!levelName.empty())
	{
		auto& levelList = server->getLevelList();
		levelList.insert(std::make_pair(string::toLower(levelName), level));
	}
	return level;
}

std::shared_ptr<Level> Level::clone(LevelPtr level, std::string_view name)
{
	if (level == nullptr) return nullptr;
	/*
	* TODO: The level needs to be stubbed, and the new name has to be set, without being overwritten.
	* If not, then serverside NPCs are going to muck everything up when they try to register to the level.
	auto server = BabyDI::Get<Server>();
	auto cloned = server->getStubbedLevel(name);
	LevelLoader::loadLevelInto(cloned, std::filesystem::path{ level->levelName });
	cloned->levelName = name;
	cloned->m_filePath = level->m_filePath.parent_path() / name;
	*/
	return nullptr;
}

//----------------------------

bool Level::reload(std::string_view levelName)
{
	auto staticData = getStaticLevelDataByName(levelName);
	if (staticData == nullptr)
		return false;

	StaticLevelData::reload(staticData);
	return true;
}

bool Level::reload(const MapPosition& position)
{
	auto staticData = getStaticLevelDataAtPosition(position);
	if (staticData == nullptr)
		return false;

	StaticLevelData::reload(staticData);
	return true;
}

void Level::reload(StaticLevelDataPtr staticData)
{
	if (staticData == nullptr)
		return;

	auto mapPosition = getSubLevelPositionInMap(staticData->levelName);
	auto subLevelIndex = getMapIndexAtPosition(mapPosition.value_or(MapPosition{}));
	auto oldSubLevel = m_levelParts.size() > subLevelIndex ? m_levelParts[subLevelIndex] : nullptr;

	// Delete arrows.
	for (auto it = m_arrows.begin(); it != m_arrows.end();)
	{
		auto& arrow = *it;
		if (toMapPosition(arrow.position) == mapPosition)
		{
			it = m_arrows.erase(it);
			continue;
		}
		++it;
	}

	// Delete bombs.
	for (auto it = m_bombs.begin(); it != m_bombs.end();)
	{
		auto& bomb = *it;
		if (toMapPosition(bomb.position) == mapPosition)
		{
			it = m_bombs.erase(it);
			continue;
		}
		++it;
	}

	// Delete explosions.
	for (auto it = m_explosions.begin(); it != m_explosions.end();)
	{
		auto& explosion = *it;
		if (toMapPosition(explosion.position) == mapPosition)
		{
			it = m_explosions.erase(it);
			continue;
		}
		++it;
	}

	// Delete horses.
	for (auto it = m_horses.begin(); it != m_horses.end();)
	{
		auto& horse = *it;
		if (toMapPosition(horse.position) == mapPosition)
		{
			it = m_horses.erase(it);
			continue;
		}
		++it;
	}

	// Delete items.
	for (size_t i = m_items.size(); i > 0; --i)
	{
		if (toMapPosition(m_items[i].position) == mapPosition)
			removeItem(inform_client, i - 1);
	}

	// Delete shoots.
	for (auto it = m_shoots.begin(); it != m_shoots.end();)
	{
		auto& shoot = *it;
		if (toMapPosition(shoot.position) == mapPosition)
		{
			it = m_shoots.erase(it);
			continue;
		}
		++it;
	}

	// Delete NPCs.
	// We want to delete them while players are still in the level so they get the appropriate delete packets.
	// Older clients (1.x) may crash if things don't happen in the right order.
	for (auto iter = m_npcs.begin(); iter != m_npcs.end();)
	{
		if (auto npc = m_server->getNPC(*iter); npc == nullptr || npc->storageType == NPCStorageType::LEVEL)
		{
			if (npc && (!mapPosition.has_value() || npc->character.getMapPosition() == mapPosition))
			{
				m_server->deleteNPC(npc, false);
				iter = m_npcs.erase(iter);
				continue;
			}
		}
		++iter;
	}

	// Remove all the players from the level.
	std::deque<PlayerID> oldplayers = m_players;
	for (auto& id : oldplayers)
	{
		if (auto p = m_server->getPlayer<PlayerClient>(id); p)
		{
			if (p->getMapPosition() == mapPosition)
				p->leaveLevel();
		}
	}

	// Clear the level cache for all players on the server.
	// Make sure this always gets called AFTER we leave the level.
	auto& playerList = m_server->getPlayerList();
	for (const auto& [id, p] : players_of_type<PlayerClient>(playerList))
	{
		p->resetLevelCache(staticData.get());
		if (oldSubLevel != nullptr)
			p->resetLevelCache(oldSubLevel.get());
	}

	// Attach the static data to the level part.
	// This will remove any existing static data association.
	auto subLevel = LevelLoader::attachStaticDataToLevel(shared_from_this(), mapPosition, staticData);
	if (subLevel == nullptr)
		return;

	// Bind listeners for level data changes.
	subLevel->staticDataRefreshedHandle = staticData->onDataRefreshed.subscribe([weakSelf = std::weak_ptr<Level>(shared_from_this())](StaticLevelDataPtr staticData)
	{
		if (auto self = weakSelf.lock(); self != nullptr)
			self->reload(staticData);
	});

	// Replace the sub-level with the new one.
	m_levelParts[subLevelIndex] = subLevel;

	// Load NPCs for the sub-level.
	LevelLoader::loadStaticDataNPCs(shared_from_this(), mapPosition, staticData);

	// Warp all players back to the level.
	for (auto& id : oldplayers)
	{
		if (auto p = m_server->getPlayer<PlayerClient>(id); p)
			p->warp(shared_from_this(), p->getGlobalPosition());
	}
}

bool Level::saveLevel(const MapPosition& mapPosition, std::string_view filename)
{
	const auto& [subLevel, staticData] = getSubLevelAndStaticDataAtPosition(mapPosition);
	if (subLevel == nullptr || staticData == nullptr)
		return false;

	auto& fileSystem = m_server->getFileSystem();

	auto actualFilename = getFilename(filename);
	auto path = fileSystem.findi(fs::FileCategory::LEVEL, actualFilename.toStringView());

	if (path.empty())
	{
		auto dirs = fileSystem.getManagedDirectories(fs::FileCategory::LEVEL);
		auto iter = dirs.begin();
		if (iter == dirs.end())
		{
			log::printLine(log::server, "** Error saving level: {}. No level directories are configured.", actualFilename);
			return false;
		}

		path = std::filesystem::path{*iter} / actualFilename.toStringView();
	}

	std::ofstream fileStream(path);

	fileStream << "GLEVNW01" << std::endl;

	// Write tiles.
	if (auto tiles = subLevel->getTiles(); tiles.has_value())
	{
		for (const auto& layerIndex : tiles.value()->getUsedTileLayers())
		{
			auto layer = tiles.value()->getLayer(layerIndex).value_or(nullptr);
			if (layer == nullptr)
				continue;

			std::string data;
			std::list<std::pair<int, std::string>> chunks;
			for (int y = 0; y < 64; ++y)
			{
				data.clear();
				chunks.clear();
				int currentStart = 0;

				// Separate each row into chunks of actually non-transparent tiles.
				for (int x = 0; x < 64; ++x)
				{
					auto tile = layer->at(x + static_cast<size_t>(y) * 64);
					if (tile == constants::EmptyTileInLayer)
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

					// Swap to big-endian for storage.
					if constexpr (std::endian::native == std::endian::little)
					{
						// We only store the first 12 bits of the tile, so shift by 4 and swap to big-endian.
						tile <<= 4;
						tile = std::byteswap(tile);
					}

					// Append the base64 encoded tile data.
					std::span<uint8_t> tileData{reinterpret_cast<uint8_t*>(&tile), sizeof(decltype(tile))};
					data += string::toBase64(tileData).substr(0, 2);
				}

				// Write any remaining data as a chunk.
				if (!data.empty())
					chunks.emplace_back(currentStart, data);

				// Write one BOARD entry for each chunk so transparent tile-data is culled.
				for (const auto& chunk : chunks)
				{
					// BOARD x y width layer data
					fileStream << std::format("BOARD {} {} {} {} {}", chunk.first, y, chunk.second.length() / 2, layerIndex, chunk.second) << std::endl;
				}
			}
		}
	}

	for (const auto& link : staticData->links)
	{
		auto& bbox = link.getBoundingBox();
		fileStream << std::format("LINK {} {} {} {} {} {} {}", link.getDestinationLevel(), bbox.position.x(), bbox.position.y(), bbox.size.width(), bbox.size.height(), link.getDestinationX(), link.getDestinationY()) << std::endl;
	}

	for (const auto& sign : staticData->signs)
	{
		fileStream << std::format("SIGN {} {}", sign.getTileX(), sign.getTileY()) << std::endl;
		fileStream << sign.text << std::endl;
		fileStream << "SIGNEND" << std::endl;
	}

	for (const auto& chest : staticData->chests)
	{
		fileStream << std::format("CHEST {} {} {} {}", chest.position.x(), chest.position.y(), LevelItem::getItemName(chest.item), chest.sign) << std::endl;
	}

	for (const auto& baddy : staticData->baddies)
	{
		fileStream << std::format("BADDY {} {} {}", baddy.getTileX(), baddy.getTileY(), PROPID(baddy.type)) << std::endl;

		for (const auto& verse : baddy.verses)
			fileStream << verse << std::endl;

		fileStream << "BADDYEND" << std::endl;
	}

	for (const auto& npcId : m_npcs)
	{
		auto npc = m_server->getNPC(npcId);
		if (npc == nullptr || npc->storageType != NPCStorageType::LEVEL)
			continue;

		// Only include NPCs for this sub-level.
		if (npc->character.getMapPosition() != mapPosition)
			continue;

		// Empty image and characters are stored as "-".
		std::string_view image = npc->image;
		if (image.empty() || image == "#c#"sv)
			image = "-"sv;

		fileStream << std::format("NPC {} {} {}", image, (npc->character.localPixelX / 16.0f), (npc->character.localPixelY / 16.0f)) << std::endl;
		fileStream << string::trim(npc->getScript().getOriginalSource()) << std::endl;
		fileStream << "NPCEND" << std::endl;
	}

	return true;
}

//----------------------------

void Level::doTimedEvents()
{
	const auto& now = m_server->getFrameStartTimeHighPrecision();

	// Run board change events.
	for (auto& part : m_levelParts | removeNulls)
	{
		for (auto& change : part->boardChanges)
			change.update(now);
	}

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
				addExplosion(translatePosition(bomb.position, 32, -32), bomb.owner, 4, bomb.power);
				addExplosion(translatePosition(bomb.position, -32, 32), bomb.owner, 4, bomb.power);
				addExplosion(translatePosition(bomb.position, 32, 32), bomb.owner, 4, bomb.power);
			}
		}
		return exploded;
	});

	// Run explosion events.
	for (auto& explosion : m_explosions) explosion.timeout.update(now);
	std::erase_if(m_explosions, [](const LevelExplosion& explosion)
	{
		return !explosion.timeout.isRunning();
	});

	// Run item events.
	for (auto& item : m_items) item.timeout.update(now);
	std::erase_if(m_items, [](const LevelItem& item)
	{
		return !item.timeout.isRunning();
	});

	// Run horse events.
	for (auto& horse : m_horses) horse.timeout.update(now);
	std::erase_if(m_horses, [](const LevelHorse& horse)
	{
		return !horse.timeout.isRunning();
	});

	// Run baddy events.
	if (auto subLevel = getSubLevelAtPosition(MapPosition{}); subLevel != nullptr && !isGmap())
	{
		for (auto& baddy : subLevel->baddies)
			baddy.timeout.update(now);
	}
}

void Level::doFrameEvents(precise_clock::time_point time)
{
	// Don't bother with shoot and arrow processing if we don't have an npc-server.
	if (!m_server->hasNPCServer())
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

std::generator<const LevelBaddy&> Level::getBaddies() const noexcept
{
	if (isGmap())
		co_return;

	if (auto subLevel = getSubLevelAtPosition(MapPosition{}); subLevel != nullptr)
	{
		for (const auto& baddy : subLevel->baddies)
			co_yield baddy;
	}
}

std::generator<const LevelChest&> Level::getChests() const noexcept
{
	if (!isGmap())
	{
		if (m_levelParts.empty() || m_levelParts.at(0) == nullptr)
			co_return;
		if (auto sdata = m_levelParts.at(0)->staticData.lock(); sdata != nullptr)
		{
			for (const auto& chest : sdata->chests)
				co_yield chest;
		}
	}
	else
	{
		for (const auto& levelPtr : m_levelParts | removeNulls)
		{
			if (auto sdata = levelPtr->staticData.lock(); sdata != nullptr)
			{
				for (const auto& chest : sdata->chests)
					co_yield chest;
			}
		}
	}
}

std::generator<const LevelLink&> Level::getLinks() const noexcept
{
	if (!isGmap())
	{
		if (m_levelParts.empty() || m_levelParts.at(0) == nullptr)
			co_return;
		if (auto sdata = m_levelParts.at(0)->staticData.lock(); sdata != nullptr)
		{
			for (const auto& link : sdata->links)
				co_yield link;
		}
	}
	else
	{
		for (const auto& levelPtr : m_levelParts | removeNulls)
		{
			if (auto sdata = levelPtr->staticData.lock(); sdata != nullptr)
			{
				for (const auto& link : sdata->links)
					co_yield link;
			}
		}
	}
}

std::generator<const LevelSign&> Level::getSigns() const noexcept
{
	if (!isGmap())
	{
		if (m_levelParts.empty() || m_levelParts.at(0) == nullptr)
			co_return;
		if (auto sdata = m_levelParts.at(0)->staticData.lock(); sdata != nullptr)
		{
			for (const auto& sign : sdata->signs)
				co_yield sign;
		}
	}
	else
	{
		for (const auto& levelPtr : m_levelParts | removeNulls)
		{
			if (auto sdata = levelPtr->staticData.lock(); sdata != nullptr)
			{
				for (const auto& sign : sdata->signs)
					co_yield sign;
			}
		}
	}
}

std::generator<std::pair<const LevelSign*, WholeTilePosition>> Level::getSignPositions() const noexcept
{
	if (!isGmap())
	{
		if (m_levelParts.empty() || m_levelParts.at(0) == nullptr)
			co_return;
		if (auto sdata = m_levelParts.at(0)->staticData.lock(); sdata != nullptr)
		{
			for (const auto& sign : sdata->signs)
				co_yield std::make_pair(&sign, WholeTilePosition{(uint16_t)sign.getTileX(), (uint16_t)sign.getTileY()});
		}
	}
	else
	{
		for (const auto& levelPtr : m_levelParts | removeNulls)
		{
			if (auto sdata = levelPtr->staticData.lock(); sdata != nullptr)
			{
				auto origin = toWholeTilePosition(getSubLevelOrigin(levelPtr).value_or(PixelPosition{}));
				for (const auto& sign : sdata->signs)
					co_yield std::make_pair(&sign, translatePosition(origin, (uint16_t)sign.getTileX(), (uint16_t)sign.getTileY()));
			}
		}
	}
}

size_t Level::getBaddyCount() const noexcept
{
	if (isGmap())
		return 0;

	if (auto subLevel = getSubLevelAtPosition(MapPosition{}); subLevel != nullptr)
		return subLevel->baddies.size();

	return 0;
}

size_t Level::getChestCount() const noexcept
{
	if (!isGmap())
	{
		if (m_levelParts.empty() || m_levelParts.at(0) == nullptr)
			return 0;
		if (auto sdata = m_levelParts.at(0)->staticData.lock(); sdata != nullptr)
			return sdata->chests.size();
	}
	else
	{
		size_t result = 0;

		for (const auto& levelPtr : m_levelParts | removeNulls)
		{
			if (auto sdata = levelPtr->staticData.lock(); sdata != nullptr)
				result += sdata->chests.size();
		}

		return result;
	}

	return 0;
}

size_t Level::getLinkCount() const noexcept
{
	if (!isGmap())
	{
		if (m_levelParts.empty() || m_levelParts.at(0) == nullptr)
			return 0;
		if (auto sdata = m_levelParts.at(0)->staticData.lock(); sdata != nullptr)
			return sdata->links.size();
	}
	else
	{
		size_t result = 0;

		for (const auto& levelPtr : m_levelParts | removeNulls)
		{
			if (auto sdata = levelPtr->staticData.lock(); sdata != nullptr)
				result += sdata->links.size();
		}

		return result;
	}

	return 0;
}

size_t Level::getSignCount() const noexcept
{
	if (!isGmap())
	{
		if (m_levelParts.empty() || m_levelParts.at(0) == nullptr)
			return 0;
		if (auto sdata = m_levelParts.at(0)->staticData.lock(); sdata != nullptr)
			return sdata->signs.size();
	}
	else
	{
		size_t result = 0;

		for (const auto& levelPtr : m_levelParts | removeNulls)
		{
			if (auto sdata = levelPtr->staticData.lock(); sdata != nullptr)
				result += sdata->signs.size();
		}

		return result;
	}

	return 0;
}

//----------------------------

std::optional<LevelTiles::TileArray*> Level::getTiles(const MapPosition& mapLevel, size_t layer) noexcept
{
	if (auto part = getSubLevelAtPosition(mapLevel); part != nullptr)
		return part->getTiles(layer);
	return std::nullopt;
}

std::optional<const LevelTiles::TileArray*> Level::getTiles(const MapPosition& mapLevel, size_t layer) const noexcept
{
	if (auto part = getSubLevelAtPosition(mapLevel); part != nullptr)
		return part->getTiles(layer);
	return std::nullopt;
}

std::optional<LevelTiles::TileArray*> Level::getTiles(std::string_view levelPart, size_t layer) noexcept
{
	if (auto mapPosition = getSubLevelPositionInMap(levelPart); mapPosition.has_value())
		return getTiles(mapPosition.value(), layer);
	return std::nullopt;
}

std::optional<const LevelTiles::TileArray*> Level::getTiles(std::string_view levelPart, size_t layer) const noexcept
{
	if (auto mapPosition = getSubLevelPositionInMap(levelPart); mapPosition.has_value())
		return getTiles(mapPosition.value(), layer);
	return std::nullopt;
}

//----------------------------

bool Level::hasTerrain() const noexcept
{
	if (m_map == nullptr || isOnBigMap())
		return false;

	return !m_map->terrain.gridBorderTileHeightsXAxis.empty();
}

double Level::getHeightAt(const PixelPosition& position) const noexcept
{
	if (auto part = getSubLevelAtPosition(position); part != nullptr)
		return part->getHeightAt(toLocalPixelPosition(position));

	return 0.0;
}

//----------------------------

void Level::sendBoardToPlayer(std::shared_ptr<Player> player) const
{
	if (auto subLevel = getSubLevelAtPosition(player->getMapPosition()); subLevel != nullptr)
		subLevel->sendBoardToPlayer(player);
}

void Level::sendBoardLayersToPlayer(std::shared_ptr<Player> player) const
{
	if (auto subLevel = getSubLevelAtPosition(player->getMapPosition()); subLevel != nullptr)
		subLevel->sendBoardLayersToPlayer(player);
}

void Level::sendBoardHeightsToPlayer(std::shared_ptr<Player> player) const
{
	if (auto subLevel = getSubLevelAtPosition(player->getMapPosition()); subLevel != nullptr)
		subLevel->sendBoardHeightsToPlayer(player);
}

void Level::sendBoardChangesToPlayer(std::shared_ptr<Player> player, std::optional<clock::time_point> time) const
{
	if (auto subLevel = getSubLevelAtPosition(player->getMapPosition()); subLevel != nullptr)
		subLevel->sendBoardChangesToPlayer(player, time);
}

void Level::sendChestsToPlayer(std::shared_ptr<Player> player) const
{
	if (auto staticData = getStaticLevelDataAtPosition(player->getMapPosition()); staticData != nullptr)
		staticData->sendChestsToPlayer(player);
}

void Level::sendLinksToPlayer(std::shared_ptr<Player> player, bool onlyMapLinks) const
{
	if (auto staticData = getStaticLevelDataAtPosition(player->getMapPosition()); staticData != nullptr)
		staticData->sendLinksToPlayer(player, onlyMapLinks);
}

void Level::sendSignsToPlayer(std::shared_ptr<Player> player) const
{
	if (auto staticData = getStaticLevelDataAtPosition(player->getMapPosition()); staticData != nullptr)
		staticData->sendSignsToPlayer(player);
}

void Level::sendBaddiesToPlayer(std::shared_ptr<Player> player) const
{
	auto subLevel = getSubLevelAtPosition(MapPosition{});
	if (isGmap() || subLevel == nullptr)
		return;

	CString packet;
	for (const auto& baddy : subLevel->baddies)
	{
		packet.clear();
		packet >> (char)PLO_BADDYPROPS >> (char)baddy.id << baddy.getProps();
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

// TODO: Replace with a function in server that sends npc props from a list of ids.
void Level::sendNPCsToPlayer(std::shared_ptr<Player> player, std::optional<clock::time_point> time) const
{
	for (const auto& npcId : m_npcs)
	{
		auto npc = m_server->getNPC(npcId);
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
	if (m_players.empty())
		return false;
	return m_players.front() == id;
}

bool Level::hasLivingBaddies() const
{
	auto subLevel = getSubLevelAtPosition(MapPosition{});
	if (isGmap() || subLevel == nullptr)
		return false;

	for (const auto& baddy : subLevel->baddies)
	{
		if (baddy.mode != BaddyMode::DEAD)
			return true;
	}
	return false;
}

//----------------------------

int Level::addPlayer(PlayerID id)
{
	log::debug_assert(std::ranges::contains(m_players, id) == false, "Player already in level");

	m_players.push_back(id);
	timeSinceLastPlayerLeft.reset();

	// Set the player enters event on all the NPCs.
	if (auto player = m_server->getPlayer(id); player != nullptr)
		m_server->queueNPCEvent(shared_from_this(), player->getGlobalPosition(), ScriptEventType::PLAYERENTERS, source::FromPlayer(id));

	return static_cast<int>(m_players.size() - 1);
}

void Level::removePlayer(PlayerID id)
{
	std::erase(m_players, id);

	// Set the player leaves event on all the NPCs.
	if (auto player = m_server->getPlayer(id); player != nullptr)
		m_server->queueNPCEvent(shared_from_this(), player->getGlobalPosition(), ScriptEventType::PLAYERLEAVES, source::FromPlayer(id));

	// If there are no more players in the level, record the time so we can do level cleanup after a delay.
	if (m_players.empty())
		timeSinceLastPlayerLeft = m_server->getFrameStartTime();
}

//----------------------------

bool Level::addNPC(std::shared_ptr<NPC> npc)
{
	if (std::ranges::contains(m_npcs, npc->id))
		return false;

	m_npcs.insert(npc->id);
	npc->setLevel(shared_from_this());

	if (auto part = getSubLevelAtPosition(npc->getGlobalPosition()); part != nullptr)
	{
		auto script = string::trimLeft(npc->getScript().getClientSide());

		if (script.starts_with("sparringzone"))
			part->isSparringZone = true;

		if (script.starts_with("noplayerkilling"))
			part->isNoPkZone = true;

		//if (script.starts_with("singleplayer"))
		//	isSingleplayer = true;
	}

	return true;
}

bool Level::addNPC(NPCID npcId)
{
	auto npc = m_server->getNPC(npcId);
	return addNPC(npc);
}

void Level::removeNPC(std::shared_ptr<NPC> npc)
{
	if (npc == nullptr)
		return;

	m_npcs.erase(npc->id);
}

void Level::removeNPC(NPCID npcId)
{
	auto npc = m_server->getNPC(npcId);
	removeNPC(npc);
}

//----------------------------

bool Level::alterBoard(CString& tileData, const WholeTileRectangleArea& area, Player* player, bool forceRespawn, bool allowRespawn, bool sendToPlayers)
{
	// Do the check for the push-pull block.
	if (area.position.z() == 0 && area.size.width() == 4 && area.size.height() == 4 && m_server->cached.enableClientsidePushPull.getValue())
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

	// Any 2x2 tile change can respawn.
	// The list of tiles is mostly for security checks and should be a list of allowed replacements.
	// TODO: Develop a way to specify valid tile replacements.
	auto respawnTime = m_server->cached.tileRespawnTime.getValue();
	bool doRespawn = allowRespawn && (forceRespawn || (area.size.width() == 2 && area.size.height() == 2));

	/*
	// Check if the tiles should be respawned.
	// Only tiles in the respawningTiles array are allowed to respawn.
	// These are things like signs, bushes, pots, etc.
	auto respawnTime = m_server->cached.tileRespawnTime.getValue();
	bool doRespawn = false;
	short testTile = m_tiles[0][area.position.x() + (static_cast<size_t>(area.position.y()) * 64)];
	int tileCount = sizeof(respawningTiles) / sizeof(short);
	for (int i = 0; i < tileCount; ++i)
		if (testTile == respawningTiles[i]) doRespawn = true;
	*/

	// Split up the board change into level parts.
	std::pair<uint8_t, uint8_t> mapPartsX{area.left() / tilesPerSubLevel().width(), area.right() / tilesPerSubLevel().width()};
	std::pair<uint8_t, uint8_t> mapPartsY{area.top() / tilesPerSubLevel().height(), area.bottom() / tilesPerSubLevel().height()};
	for (auto partY = mapPartsY.first; partY <= mapPartsY.second; ++partY)
	{
		for (auto partX = mapPartsX.first; partX <= mapPartsX.second; ++partX)
		{
			MapPosition mapPosition{partX, partY};

			// Get the level part.
			auto sourcePart = getSubLevelAtPosition(mapPosition);
			if (sourcePart == nullptr)
				continue;

			// Determine the area within the part.
			auto localRect = clipLocalWholeTileRectangleArea(mapPosition, area);

			// Delete any existing changes contained within the same region.
			std::erase_if(sourcePart->boardChanges, [&localRect](const LevelBoardChange& change)
			{
				return rectangleContained(change.area, localRect);
			});

			// Grab the old tiles for respawn.
			CString oldTiles;
			if (doRespawn)
			{
				auto tiles = sourcePart->getTiles(area.position.z());
				if (!tiles.has_value())
					continue;

				for (int j = localRect.position.y(); j < localRect.position.y() + localRect.size.height(); ++j)
				{
					for (int i = localRect.position.x(); i < localRect.position.x() + localRect.size.width(); ++i)
						oldTiles.writeGShort((*tiles.value())[i + (static_cast<size_t>(j) * 64)]);
				}
			}

			// Construct the tile data for this part.
			CString partTileData;
			for (int j = localRect.position.y(); j < localRect.position.y() + localRect.size.height(); ++j)
			{
				for (int i = localRect.position.x(); i < localRect.position.x() + localRect.size.width(); ++i)
				{
					// Determine the index in the full tileData.
					int globalX = i + (static_cast<int32_t>(mapPosition.x()) * tilesPerSubLevel().width());
					int globalY = j + (static_cast<int32_t>(mapPosition.y()) * tilesPerSubLevel().height());
					int index = globalX - area.position.x() + ((globalY - area.position.y()) * area.size.width());

					// Read the tile from the full tileData.
					tileData.setRead(index * 2);
					short tile = tileData.readGShort();
					partTileData.writeGShort(tile);
				}
			}

			// Apply the board update to this part.
			sourcePart->boardChanges.push_back(LevelBoardChange{shared_from_this(), MapPosition{partX, partY}, localRect, partTileData, oldTiles, (doRespawn ? std::chrono::seconds(respawnTime) : 0s)});
			if (sendToPlayers) sourcePart->boardChanges.back().sendToPlayersOnLevel();
		}
	}

	return true;
}

void Level::applyBoardChangeFromScriptTiles(const WholeTileRectangleArea& area, bool forceRespawn, bool allowRespawn)
{
	// Prepare a tile array for the area.
	std::vector<uint16_t> tiles{0};
	tiles.resize(static_cast<size_t>(area.size.width()) * area.size.height());

	// Fill in the tile array with script updated tiles.
	// The tiles are stored in each sub-level.
	for (const auto& subLevel : getSubLevelsInRectangle(area))
	{
		auto clippedArea = subLevel->clipRectangleToPart(area);
		if (clippedArea.size.width() == 0 || clippedArea.size.height() == 0)
			continue;

		if (!subLevel->scriptUpdatedTiles.has_value())
			subLevel->scriptUpdatedTiles = LevelTiles();

		auto layer = subLevel->scriptUpdatedTiles.value().getLayer(0);
		if (!layer.has_value() || layer.value() == nullptr)
			continue;

		auto displacement = clippedArea.position - area.position;
		auto localArea = toLocalWholeTileRectangleArea(clippedArea);

		// Collect the tiles.
		auto& subTiles = layer.value();
		for (size_t y = 0; y < localArea.size.height(); ++y)
		{
			for (size_t x = 0; x < localArea.size.width(); ++x)
			{
				// Get the tile from the sub-level.
				auto sourcePosition = Position<size_t>(localArea.position.x() + x, localArea.position.y() + y);
				auto tile = subTiles->at(sourcePosition.y() * 64 + sourcePosition.x());

				// Set the tile in the destination array, calculating the position relative to the origin of the area.
				// This will let us pick the appropriate index.
				auto destPosition = Position<size_t>(displacement.x() + x, displacement.y() + y);
				tiles[destPosition.y() * area.size.width() + destPosition.x()] = tile;
			}
		}
	}

	CString tileData;
	for (auto& tile : tiles)
		tileData.writeGShort(tile);

	// Apply the board update.
	alterBoard(tileData, area, nullptr, forceRespawn, allowRespawn, true);
}

void Level::saveBoardChangeFromScriptTiles(const WholeTileRectangleArea& area)
{
	for (const auto& levelPart : getSubLevelsInRectangle(area))
	{
		// We need to have script updated tiles.
		if (!levelPart->scriptUpdatedTiles.has_value())
			continue;

		// And we need to have updates in this layer.
		auto updatedTiles = levelPart->scriptUpdatedTiles.value().getLayer(area.position.z());
		if (!updatedTiles.has_value() || updatedTiles.value() == nullptr)
			continue;

		// And the updates need to be within this part.
		auto localArea = toLocalWholeTileRectangleArea(levelPart->clipRectangleToPart(area));
		if (localArea.size.width() == 0 || localArea.size.height() == 0)
			continue;

		// And we need to have static data.
		auto sData = levelPart->staticData.lock();
		if (sData == nullptr)
			continue;

		// Initialize the instanced tile updates if not already done.
		if (!levelPart->instancedTileUpdates.has_value())
			levelPart->instancedTileUpdates = sData->tiles;

		// Get the level part's tiles for this layer.
		if (auto tiles = levelPart->getTiles(area.position.z()); tiles.has_value())
		{
			auto& destTiles = *tiles.value();
			auto& sourceTiles = *updatedTiles.value();
			for (int j = localArea.position.y(); j < localArea.position.y() + localArea.size.height(); ++j)
			{
				for (int i = localArea.position.x(); i < localArea.position.x() + localArea.size.width(); ++i)
				{
					auto index = i + (static_cast<size_t>(j) * 64);
					auto tile = sourceTiles.at(index);
					if (tile != constants::EmptyTileInLayer)
						destTiles.at(index) = tile;
				}
			}
		}
	}
}

void Level::updateBoard(const TileRectangleArea& area) noexcept
{
	applyBoardChangeFromScriptTiles(toWholeTileRectangleArea(area), true);
}

void Level::updateBoard2(const TileRectangleArea& area) noexcept
{
	// If we don't allow permanent tile modifications, just call updateBoard().
	if (m_server->cached.enablePermanentTileChanges.getValue() == false)
	{
		updateBoard(area);
		return;
	}

	auto wholeTileArea = toWholeTileRectangleArea(area);
	applyBoardChangeFromScriptTiles(wholeTileArea, false, false);

	bool levelsAutoSave = m_server->cached.saveTileChangesToLevelFile.getValue();
	if (levelsAutoSave)
	{
		auto mapPosition = toMapPosition(area.position);
		if (auto staticData = getStaticLevelDataAtPosition(mapPosition); staticData != nullptr)
		{
			saveBoardChangeFromScriptTiles(wholeTileArea);
			saveLevel(mapPosition, staticData->levelName);
		}
	}
}

//----------------------------

LevelArrow* Level::addArrow(inform_client_t, const PixelPosition& position, const PixelPosition& speed, uint8_t direction, int8_t type, ScriptObject from)
{
	if (!m_server->hasNPCServer())
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
		m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_ARROWADD >> (short)0 >> (char)x >> (char)y >> (char)flags >> (char)sprite >> (char)type, position, shared_from_this());
	}
	return result;
}

LevelArrow* Level::addArrow(const PixelPosition& position, const PixelPosition& speed, uint8_t direction, int8_t type, ScriptObject from)
{
	if (!m_server->hasNPCServer())
		return nullptr;

	LevelArrow newArrow{.startPosition = position, .position = position, .speed = speed, .direction = direction, .type = type, .from = from};
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

std::optional<LevelArrow*> Level::getArrow(size_t index) noexcept
{
	if (index >= m_arrows.size())
		return std::nullopt;
	return &m_arrows.at(index);
}

//----------------------------

LevelBaddy* Level::addBaddy(const LocalPixelPosition& position, BaddyType type)
{
	auto subLevel = getSubLevelAtPosition(MapPosition{});
	if (isGmap() || subLevel == nullptr)
		return nullptr;

	// Find the next available baddy that can be used.
	size_t nextIndex = 0;
	for (nextIndex = 0; nextIndex < subLevel->baddies.size(); ++nextIndex)
	{
		if (subLevel->baddies[nextIndex].canBeReplaced())
			break;
	}

	// Limit of 50 baddies per level.
	if (nextIndex >= 50)
		return nullptr;

	// Clamp the index to the size of the baddy list, just in case.
	nextIndex = std::clamp(nextIndex, static_cast<size_t>(0), subLevel->baddies.size());

	// New Baddy
	LevelBaddy newBaddy{position, type, this->shared_from_this()};
	newBaddy.id = nextIndex + 1;

	if (nextIndex == subLevel->baddies.size())
		subLevel->baddies.emplace_back(std::move(newBaddy));
	else
		subLevel->baddies[nextIndex] = std::move(newBaddy);

	return &subLevel->baddies[nextIndex];
}

LevelBaddy* Level::putNewBaddy(const LocalPixelPosition& position, BaddyType type)
{
	auto baddy = addBaddy(position, type);
	if (baddy == nullptr)
		return nullptr;

	CString packet = CString() >> (char)PLO_BADDYPROPS >> (char)baddy->id << baddy->getProps();
	for (auto& playerId : m_players)
	{
		if (auto player = m_server->getPlayer(playerId); player)
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

	CString packet = CString() >> (char)PLO_BADDYPROPS >> (char)baddy->id << baddy->getProps();
	for (auto& playerId : m_players)
	{
		if (auto player = m_server->getPlayer(playerId); player)
			player->sendPacket(packet);
	}

	return baddy;
}

bool Level::removeBaddy(uint8_t pId)
{
	auto subLevel = getSubLevelAtPosition(MapPosition{});
	if (isGmap() || subLevel == nullptr)
		return false;

	// Don't allow us to remove id 0 or any id over 50.
	if (pId < 1 || pId > 50 || (pId > subLevel->baddies.size())) return false;

	// Find the baddy.
	auto& baddy = subLevel->baddies.at(static_cast<size_t>(pId) - 1);
	if (baddy.mode == BaddyMode::DEAD)
		return false;

	// Erase the baddy.
	baddy.mode = BaddyMode::DEAD;
	baddy.setRespawn(false);

	// Set the baddy as dead for all the other players in the level.
	CString props = CString() >> (char)BaddyProp::MODE >> (char)BaddyMode::DEAD;
	for (const auto& playerId : m_players)
	{
		if (auto player = m_server->getPlayer(playerId); player != nullptr)
			player->sendPacket(CString() >> (char)PLO_BADDYPROPS >> (char)baddy.id << props);
	}
	return true;
}

bool Level::removeAllBaddies()
{
	auto subLevel = getSubLevelAtPosition(MapPosition{});
	if (isGmap() || subLevel == nullptr)
		return false;

	CString propsPacket;
	for (auto& baddy : subLevel->baddies)
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
			if (auto player = m_server->getPlayer(playerId); player != nullptr)
				player->sendPacket(propsPacket);
		}
	}
	return true;
}

std::optional<LevelBaddy*> Level::getBaddyById(uint8_t id) noexcept
{
	auto subLevel = getSubLevelAtPosition(MapPosition{});
	if (isGmap() || subLevel == nullptr)
		return std::nullopt;

	if (id > subLevel->baddies.size() || id == 0)
		return std::nullopt;
	return &subLevel->baddies.at(static_cast<size_t>(id) - 1);
}

std::optional<LevelBaddy*> Level::getAliveBaddyByIndex(size_t index) noexcept
{
	auto subLevel = getSubLevelAtPosition(MapPosition{});
	if (isGmap() || subLevel == nullptr)
		return std::nullopt;

	if (index >= subLevel->baddies.size())
		return std::nullopt;

	size_t pos = 0;
	for (auto& baddy : subLevel->baddies)
	{
		if (!baddy.isAlive())
			continue;
		if (index == pos++)
			return &baddy;
	}

	return std::nullopt;
}

//----------------------------

LevelBomb* Level::addBomb(inform_client_t, const PixelPosition& position, uint8_t power)
{
	if (!m_server->hasNPCServer())
		return nullptr;

	auto result = addBomb(position, power);
	if (result != nullptr)
	{
		auto localPosition = toLocalPixelPosition(result->position);
		char x = static_cast<char>(localPosition.x() / 8.0f);
		char y = static_cast<char>(localPosition.y() / 8.0f);
		uint8_t timeToExplode = static_cast<uint8_t>(std::min<std::chrono::milliseconds::rep>(223, std::chrono::duration_cast<std::chrono::milliseconds>(result->timeout.timeout).count() / 50));
		m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_BOMBADD >> (short)0 >> (char)x >> (char)y >> (char)result->power >> (char)timeToExplode, position, shared_from_this());
		// PLO_BOMBADD might support a bomb image at the end of the packet.
	}
	return result;
}

LevelBomb* Level::addBomb(const PixelPosition& position, uint8_t power)
{
	if (!m_server->hasNPCServer())
		return nullptr;

	LevelBomb newBomb{.position = position, .power = power};
	newBomb.timeout.runOnceFor(3s);
	m_bombs.emplace_back(std::move(newBomb));
	return &m_bombs.back();
}

LevelBomb* Level::addBombFromClient(const PixelPosition& position, uint8_t power, PlayerID owner, std::chrono::milliseconds timeToExplode)
{
	if (!m_server->hasNPCServer())
		return nullptr;

	// If we generate an item NPC, remove the bomb from the level.
	LevelItemType itemType = (power == 2 ? LevelItemType::SUPERBOMB : (power == 3 ? LevelItemType::JOLTBOMB : LevelItemType::BOMB));
	if (auto itemNPC = generateItemNPC(position, itemType); itemNPC != nullptr)
		return nullptr;

	// Add the bomb to the level otherwise.
	LevelBomb newBomb{.position = position, .power = power, .owner = source::FromPlayer(owner)};
	newBomb.timeout.runOnceFor(timeToExplode);
	m_bombs.emplace_back(std::move(newBomb));
	return &m_bombs.back();
}

bool Level::removeBomb(inform_client_t, size_t index)
{
	if (index < m_bombs.size())
	{
		auto mapPosition = toMapPosition(m_bombs[index].position);
		auto localPosition = toLocalPixelPosition(m_bombs[index].position);
		CString packet = CString() >> (char)PLO_BOMBDEL >> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8);
		m_server->sendPacketToOneLevelPart(packet, this->shared_from_this(), mapPosition);
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

std::optional<LevelBomb*> Level::getBomb(size_t index) noexcept
{
	if (index >= m_bombs.size())
		return std::nullopt;
	return &m_bombs.at(index);
}

//----------------------------

std::optional<const LevelChest*> Level::getChest(size_t index) const noexcept
{
	auto objects = getChests();
	auto iter = objects.begin();
	std::ranges::advance(iter, index, objects.end());
	if (iter == objects.end())
		return std::nullopt;
	return std::make_optional(&(*iter));
}

std::optional<const LevelChest*> Level::getChest(const WholeTilePosition& position) const noexcept
{
	auto mapPosition = toMapPosition(position);
	auto localPosition = toLocalWholeTilePosition(position);
	return getChest(mapPosition, localPosition);
}

std::optional<const LevelChest*> Level::getChest(const MapPosition& mapPosition, const LocalWholeTilePosition& position) const noexcept
{
	auto index = getMapIndexAtPosition(mapPosition);
	if (index >= m_levelParts.size())
		return std::nullopt;

	auto& part = m_levelParts.at(index);
	if (part == nullptr)
		return std::nullopt;

	if (auto sdata = part->staticData.lock(); sdata != nullptr)
	{
		for (auto& chest : sdata->chests)
		{
			if (chest.position == position)
				return std::make_optional(&chest);
		}
	}

	return std::nullopt;
}

//----------------------------

void Level::addExplosion(inform_client_t, const PixelPosition& position, ScriptObject from, uint8_t radius, uint8_t power)
{
	if (!m_server->hasNPCServer())
		return;

	addExplosion(position, from, radius, power);

	auto localPosition = toLocalPixelPosition(position);
	CString packet = CString() >> (char)PLO_EXPLOSION >> (short)0 >> (char)radius >> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8) >> (char)power;
	m_server->sendPacketToOneLevelPart(packet, position, shared_from_this());
}

void Level::addExplosion(const PixelPosition& position, ScriptObject from, uint8_t radius, uint8_t power)
{
	if (!m_server->hasNPCServer())
		return;

	addExplosionPart(position, 2, power);
	for (size_t i = 0; i < (static_cast<size_t>(radius) * 4); ++i)
	{
		uint8_t dir = i / radius;
		int16_t step = (((i % radius) + 1) * 2) * 16;
		PixelPosition partPosition = position.translate(
			(dir == 0 || dir == 2) ? 0 : (dir == 1 ? -step : step),
			(dir == 1 || dir == 3) ? 0 : (dir == 0 ? -step : step)
		);
		addExplosionPart(partPosition, dir, power);
	}

	// Add exploded events to NPCs in the level.
	if (m_server->hasNPCServer())
	{
		PixelRectangleArea vertTest = {position.translate(0, -(radius * 32)), {static_cast<uint16_t>(32), static_cast<uint16_t>((1 + (radius * 2)) * 32)}};
		PixelRectangleArea horzTest = {position.translate(-(radius * 32), 0), {static_cast<uint16_t>((1 + (radius * 2)) * 32), static_cast<uint16_t>(32)}};
		auto center = vertTest.center();
		for (const NPCID& npcId : findIntersectingNPCsForCollision(vertTest))
		{
			if (auto npc = m_server->getNPC(npcId); npc != nullptr)
				npc->hurtAndPush(power, center, ScriptEventType::EXPLODED, from);
		}
		for (const NPCID& npcId : findIntersectingNPCsForCollision(horzTest))
		{
			if (auto npc = m_server->getNPC(npcId); npc != nullptr)
				npc->hurtAndPush(power, center, ScriptEventType::EXPLODED, from);
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

	if (!m_server->hasNPCServer())
		return;

	const PixelPosition startingPosition = position.translate(
		(direction == 0 || direction == 2) ? 8 : (direction == 1 ? -32 : 48),
		(direction == 1 || direction == 3) ? 3 : (direction == 0 ? -24 : 35)
	);

	for (size_t i = 0; i < static_cast<size_t>(length + 1); ++i)
	{
		uint8_t dir = (i != 0 ? direction : (direction + 2) % 4);
		int16_t stepX = (direction == 0 || direction == 2) ? 0 : (direction == 1 ? -i * 32 : i * 32);
		int16_t stepY = (direction == 1 || direction == 3) ? 0 : (direction == 0 ? -i * 32 : i * 32);
		PixelPosition partPosition = startingPosition.translate(stepX, stepY);
		addExplosionPart(partPosition, dir, power);
	}

	// Add exploded events to NPCs in the level.
	if (m_server->hasNPCServer())
	{
		int16_t lengthInPixels = (length + 1) * 32;
		PixelPosition testPosition = startingPosition.translate(
			static_cast<int16_t>((direction == 1) ? -lengthInPixels : 0),
			static_cast<int16_t>((direction == 0) ? -lengthInPixels : 0)
		);
		Dimension<uint16_t> testDimension{
			static_cast<uint16_t>((direction == 0 || direction == 2) ? 32 : lengthInPixels),
			static_cast<uint16_t>((direction == 1 || direction == 3) ? 32 : lengthInPixels)
		};

		auto center = translatePosition(startingPosition, 16, 16);
		for (const NPCID& npcId : findIntersectingNPCsForCollision({testPosition, testDimension}))
		{
			if (auto npc = m_server->getNPC(npcId); npc != nullptr)
				npc->hurtAndPush(power, center, ScriptEventType::EXPLODED, from);
		}
	}
}

LevelExplosion* Level::addExplosionPart(const PixelPosition& position, uint8_t direction, uint8_t power)
{
	if (!m_server->hasNPCServer())
		return nullptr;

	LevelExplosion explo{.position = position, .power = power, .direction = direction};
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

std::optional<LevelExplosion*> Level::getExplosion(size_t index) noexcept
{
	if (index >= m_explosions.size())
		return std::nullopt;
	return &m_explosions.at(index);
}

//----------------------------

LevelHorse* Level::addHorse(inform_client_t, std::string_view image, const PixelPosition& position, uint8_t direction, uint8_t bushes)
{
	auto result = addHorse(image, position, direction, bushes);
	if (result != nullptr)
		m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_HORSEADD << result->getPacket(), position, shared_from_this());
	return result;
}

LevelHorse* Level::addHorse(std::string_view image, const PixelPosition& position, uint8_t direction, uint8_t bushes)
{
	auto horseLife = m_server->getSettings().get<uint32_t>("horselifetime").value_or(30);

	LevelHorse newHorse{.position = position, .image = std::string{image}, .direction = direction, .bushes = bushes, .timeout = TimeoutGenerator(std::chrono::seconds(horseLife))};
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
		auto mapPosition = toMapPosition(m_horses[index].position);
		auto localPosition = toLocalPixelPosition(m_horses[index].position);
		CString packet = CString() >> (char)PLO_HORSEDEL >> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8);
		m_server->sendPacketToOneLevelPart(packet, this->shared_from_this(), mapPosition);
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

std::optional<LevelHorse*> Level::getHorse(size_t index) noexcept
{
	if (index >= m_horses.size())
		return std::nullopt;
	return &m_horses.at(index);
}

//----------------------------

LevelItem* Level::addItem(inform_client_t, const PixelPosition& position, LevelItemType item, std::optional<PlayerID> addedBy)
{
	auto result = addItem(position, item, addedBy);
	if (result != nullptr)
	{
		auto localPosition = toLocalPixelPosition(result->position);
		m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_ITEMADD >> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8) >> (char)LevelItem::getItemTypeId(result->item), result->position, shared_from_this());
	}
	return result;
}

LevelItem* Level::addItem(const PixelPosition& position, LevelItemType item, std::optional<PlayerID> addedBy)
{
	if (m_server->hasNPCServer())
	{
		// If we were able to generate the item NPC, don't add the item to the ground.
		if (auto itemNPC = generateItemNPC(position, item); itemNPC != nullptr)
			return nullptr;

		// Check if we should send item drop events to the Control-NPC.
		bool itemDropEvents = m_server->cached.enableItemDropEvents.getValue();
		if (itemDropEvents && m_server->cached.itemDropEventsOnlyForGralats.getValue() && !LevelItem::isRupeeType(item))
			itemDropEvents = false;

		// If item drop events are enabled, send the item drop event to the Control-NPC.
		// This will prevent all client item drops, so beware.
		if (itemDropEvents)
		{
			ScriptObject source = (addedBy.has_value() ? source::FromPlayer(addedBy.value()) : source::FromLevel(shared_from_this()));
			auto tilePosition = toTilePosition(position);

			m_server->getNPCServer()->addEventToControlNPC(ScriptEventType::CUSTOM, source, "itemdrop", levelName, std::format("{}", tilePosition.x()), std::format("{}", tilePosition.y()), LevelItem::getItemName(item));
			if (addedBy.has_value())
			{
				if (auto player = m_server->getPlayer(addedBy.value()); player != nullptr)
				{
					auto localTilePosition = toLocalWholeTilePosition(tilePosition);
					player->sendPacket(CString() >> (char)PLO_ITEMDEL >> (char)(localTilePosition.x() * 2) >> (char)(localTilePosition.y() * 2));
				}
			}
			return nullptr;
		}
	}

	LevelItem newItem{.position = position, .item = item, .modTime = m_server->getFrameStartTime()};
	newItem.timeout.runOnceFor(LevelItemTimeout);
	m_items.emplace_back(std::move(newItem));
	return &m_items.back();
}

bool Level::removeItem(inform_client_t, size_t index)
{
	if (index < m_items.size())
	{
		auto mapPosition = toMapPosition(m_items[index].position);
		auto localPosition = toLocalPixelPosition(m_items[index].position);
		CString packet = CString() >> (char)PLO_ITEMDEL >> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8);
		m_server->sendPacketToOneLevelPart(packet, this->shared_from_this(), mapPosition);
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

std::optional<LevelItem*> Level::getItem(size_t index) noexcept
{
	if (index >= m_items.size())
		return std::nullopt;
	return &m_items.at(index);
}

//----------------------------

std::optional<const LevelLink*> Level::getLink(size_t index) const noexcept
{
	auto objects = getLinks();
	auto iter = objects.begin();
	std::ranges::advance(iter, index, objects.end());
	if (iter == objects.end())
		return std::nullopt;
	return std::make_optional(&(*iter));
}

std::optional<const LevelLink*> Level::getLink(std::string_view levelPart, const LocalWholeTilePosition& position, bool excludeOverworld) const noexcept
{
	auto sdata = getStaticLevelDataByName(levelPart);
	if (sdata == nullptr)
		return std::nullopt;

	for (auto& link : sdata->links)
	{
		if (excludeOverworld && link.isProbableMapLink())
			continue;

		auto& bbox = link.getBoundingBox();
		if ((position.x() >= bbox.position.x() && position.x() <= bbox.position.x() + bbox.size.width()) && (position.y() >= bbox.position.y() && position.y() <= bbox.position.y() + bbox.size.height()))
		{
			return std::make_optional(&link);
		}
	}

	return std::nullopt;
}

std::optional<const LevelLink*> Level::getLink(const TilePosition& position, bool excludeOverworld) const noexcept
{
	auto part = getSubLevelAtPosition(position);
	if (part == nullptr)
		return std::nullopt;
	auto sdata = part->staticData.lock();
	if (sdata == nullptr)
		return std::nullopt;

	for (auto& link : sdata->links)
	{
		if (excludeOverworld && link.isProbableMapLink())
			continue;

		auto localPosition = toLocalWholeTilePosition(position);
		auto& bbox = link.getBoundingBox();
		if ((localPosition.x() >= bbox.position.x() && localPosition.x() <= bbox.position.x() + bbox.size.width()) && (localPosition.y() >= bbox.position.y() && localPosition.y() <= bbox.position.y() + bbox.size.height()))
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
	if (!m_server->hasNPCServer())
		return nullptr;

	auto result = addShoot(position, angle, zangle, power, gravity, gani, from);
	if (result != nullptr)
		m_server->sendShootToOneLevel(result, shared_from_this());

	return result;
}

LevelShoot* Level::addShoot(const PixelPosition& position, float angle, float zangle, uint8_t power, float gravity, const std::string& gani, ScriptObject from)
{
	if (!m_server->hasNPCServer())
		return nullptr;

	auto tilePosition = toTilePosition(position);
	double ground = getHeightAt(position.translate(8, 16));
	tilePosition.translate(0, 0, ground);

	LevelShoot newShoot{.position = tilePosition, .angle = angle, .zangle = zangle, .powerIn44Pixels = power, .gani = gani, .gravity = gravity, .from = from};
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

std::optional<const LevelSign*> Level::getSign(size_t index) const noexcept
{
	auto objects = getSigns();
	auto iter = objects.begin();
	std::ranges::advance(iter, index, objects.end());
	if (iter == objects.end())
		return std::nullopt;
	return std::make_optional(&(*iter));
}

//----------------------------

bool Level::moveShoot(LevelShoot* shoot, int iterations)
{
	if (shoot == nullptr)
		return false;

	for (int i = 0; i < iterations; ++i)
	{
		// If the shoot is out of bounds, delete it.
		auto levelDimensions = sizeInTiles();
		if (shoot->position.x() < 0 || shoot->position.y() < 0 || shoot->position.x() >= levelDimensions.width() || shoot->position.y() >= levelDimensions.height())
			return false;

		// Move the shoot.
		shoot->move();

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

		// Determine our absolute projectile position in the world space.
		// The Z location is relative from the starting Z.
		PixelPosition pixelPosition = toPixelPosition(shoot->position).translate(8_i16, 16_i16);

		// Determine the ground level at the shoot position.
		auto currentGroundLevel = static_cast<int32_t>(getHeightAt(pixelPosition) * 16);

		// Check for NPC collisions.
		//log::printLine(log::server, "Collision search pos: ({}), ground: {}", searchPosition / 16.0f, currentGroundLevel / 16.0f);
		bool fromPlayer = (shoot->from.second == ScriptObjectType::PLAYER);
		for (const auto& npc : findIntersectingNPCsForCollision({pixelPosition, {24_ui16, 24_ui16, 48_ui16}}))
		{
			if (shoot->from.second == ScriptObjectType::NPC && shoot->from.first == npc)
				continue;
			if (auto npcPtr = m_server->getNPC(npc); npcPtr != nullptr)
			{
				//log::printLine(log::server, "Collision ({}) with NPC '{}' at ({})", searchPosition / 16.0f, npcPtr->name, npcPos);
				constructEventParams();
				npcPtr->scripting.events.addEvent(ScriptEventType::TRIGGERACTION, shoot->from, (fromPlayer ? "projectile" : "sprojectile"), eventParams);
			}
		}

		// If we are within 3 tiles of the ground, and we aren't going up, check for walls and the ground.
		int32_t groundDiff = pixelPosition.z() - currentGroundLevel;
		if (!collided && groundDiff <= 48 && (DoubleIsZero(shoot->movementPerFrame.z()) || shoot->movementPerFrame.z() <= 0.0))
		{
			// Check if we hit the ground.
			if (pixelPosition.z() <= currentGroundLevel)
				constructEventParams();

			// Check for wall collisions.
			bool onWallDetection = m_server->cached.projectilesStopOnWall.getValue() && groundDiff < 48;
			if (!collided && onWallDetection && isOnWall2(WholeTileRectangleArea{toWholeTilePosition(pixelPosition), {1_ui8, 1_ui8}}))
				constructEventParams();
		}

		// We collided, so tell the control-NPC and delete the shoot projectile.
		if (collided)
		{
			m_server->getNPCServer()->addEventToControlNPC(ScriptEventType::TRIGGERACTION, shoot->from, (fromPlayer ? "projectile" : "sprojectile"), eventParams);
			return false;
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
		constexpr auto maxDistance = pixelsPerSubLevel().width() * 2;
		if (std::abs(arrow->position.x() - arrow->startPosition.x()) > maxDistance || std::abs(arrow->position.y() - arrow->startPosition.y()) > maxDistance)
			return false;

		bool hitWall = false;

		// Check for NPC collision.
		PixelRectangleArea searchBox = {translatePosition(arrow->position, 16_i32, -8_i32), {32_ui16, 32_ui16}};
		auto center = searchBox.center();
		int8_t arrowPower = arrow->type == arrowTypeFireball ? 2 : 1;
		for (const auto& npc : findIntersectingNPCsForCollision(searchBox))
		{
			if (arrow->from.second == ScriptObjectType::NPC && arrow->from.first == npc)
				continue;
			if (auto npcPtr = m_server->getNPC(npc); npcPtr != nullptr)
				npcPtr->hurtAndPush(arrowPower, center, ScriptEventType::WASSHOT, arrow->from);

			hitWall = true;
		}

		// If the arrow is a fireblast or nukeshot, check for walls.
		if (!hitWall && (arrow->type == arrowTypeFireblast || arrow->type == arrowTypeNukeshot))
		{
			if (isOnWall(toWholeTilePosition(arrow->position).translate(1_ui8, 0_ui8)))
				hitWall = true;
		}

		// We hit a wall (or an NPC), so destroy the arrow.
		if (hitWall)
		{
			// If we are producing an explosion on hit, do it now.
			if (arrow->type == arrowTypeFireblast || arrow->type == arrowTypeNukeshot)
				addExplosion(arrow->position, arrow->from, 1_ui8, 1_ui8);
			return false;
		}
	}

	return true;
}

//----------------------------

bool Level::isOnWall(const WholeTilePosition& tilePosition) const noexcept
{
	auto tiletype = getTileTypeAt(tilePosition);
	switch (tiletype)
	{
		case tileset::TileType::THROW_THROUGH:
		case tileset::TileType::JUMP_STONE:
		case tileset::TileType::BLOCKING:
			return true;
	}

	return false;
}

bool Level::isOnWall(const PixelPosition& position) const noexcept
{
	return isOnWall(toWholeTilePosition(position));
}

bool Level::isOnWall2(const WholeTileRectangleArea& tileArea) const noexcept
{
	// TODO: Optimize this.
	for (auto cy = tileArea.position.y(); cy < tileArea.position.y() + tileArea.size.height(); ++cy)
	{
		for (auto cx = tileArea.position.x(); cx < tileArea.position.x() + tileArea.size.width(); ++cx)
		{
			if (isOnWall(WholeTilePosition{cx, cy}))
				return true;
		}
	}
	return false;
}

bool Level::isOnWall2(const PixelRectangleArea& area) const noexcept
{
	return isOnWall2(toWholeTileRectangleArea(area));
}

bool Level::isOnWater(const WholeTilePosition& tilePosition) const noexcept
{
	auto tiletype = getTileTypeAt(tilePosition);
	return tiletype == tileset::TileType::WATER;
}

bool Level::isOnWater(const PixelPosition& position) const noexcept
{
	return isOnWater(toWholeTilePosition(position));
}

bool Level::isOnWater2(const WholeTileRectangleArea& tileArea) const noexcept
{
	// TODO: Optimize this.
	for (auto cy = tileArea.position.y(); cy < tileArea.position.y() + tileArea.size.height(); ++cy)
	{
		for (auto cx = tileArea.position.x(); cx < tileArea.position.x() + tileArea.size.width(); ++cx)
		{
			if (isOnWater(WholeTilePosition{cx, cy}))
				return true;
		}
	}
	return false;
}

bool Level::isOnWater2(const PixelRectangleArea& area) const noexcept
{
	return isOnWater2(toWholeTileRectangleArea(area));
}

bool Level::isOnPlayer(const PixelPosition& position) const noexcept
{
	for (const auto& playerId : findInRangePlayers(position))
	{
		if (auto player = m_server->getPlayer(playerId); player != nullptr)
		{
			if (positionInRectangle(position, player->getBoundingBox()))
				return true;
		}
	}
	return false;
}

bool Level::isOnPlayer(const PixelRectangleArea& pixelArea) const noexcept
{
	for (const auto& playerId : findInRangePlayers(pixelArea.position))
	{
		if (auto player = m_server->getPlayer(playerId); player != nullptr)
		{
			if (rectanglesIntersect(pixelArea, player->getBoundingBox()))
				return true;
		}
	}
	return false;
}

tileset::TileType Level::getTileTypeAt(const WholeTilePosition& tilePosition) const noexcept
{
	using namespace tileset;

	auto tileDimensions = sizeInTiles();
	if (tilePosition.x() >= tileDimensions.width() || tilePosition.y() >= tileDimensions.height())
		return TileType::BLOCKING;

	auto mapPosition = toMapPosition(tilePosition);
	auto tiles = getTiles(mapPosition);
	if (!tiles.has_value())
		return TileType::BLOCKING;

	auto localPosition = toLocalWholeTilePosition(tilePosition);
	auto tile = tiles.value()->at(static_cast<size_t>(localPosition.y()) * 64 + localPosition.x());

	auto tileset = m_server->getTilesetTypeForLevel(shared_from_this());
	return m_server->getTileTypeForTile(tileset, tile);
}

tileset::TileType Level::getTileTypeAt(const PixelPosition& position) const noexcept
{
	return getTileTypeAt(toWholeTilePosition(position));
}

//----------------------------

bool Level::isGmap() const noexcept
{
	// Kind of a hacky way to determine if it's a gmap when the map is not loaded yet (stubbed).
	// The server might try to save an NPC on a stubbed level that doesn't have its map set, so it won't write the map position to the file.
	// TODO: Find a better way to handle this.
	if (m_map == nullptr && levelName.ends_with(".gmap"sv))
		m_map = m_server->findMap(levelName);

	return m_map != nullptr && m_map->isGmap();
}

uint16_t* Level::getMapTileForEditing(const TilePosition& position) noexcept
{
	auto subLevel = getSubLevelAtPosition(position);
	if (subLevel == nullptr)
		return nullptr;

	auto localTilePos = toLocalWholeTilePosition(position);
	if (!subLevel->scriptUpdatedTiles.has_value())
		subLevel->scriptUpdatedTiles = LevelTiles();

	auto layer = subLevel->scriptUpdatedTiles.value().getOrCreateLayer(0);
	if (layer == nullptr)
		return nullptr;

	return &layer->at(static_cast<size_t>(localTilePos.y()) * 64 + localTilePos.x());
}

std::generator<SubLevelPtr> Level::getSubLevelsInRectangle(const PixelRectangleArea& area) const noexcept
{
	std::pair<uint8_t, uint8_t> mapPartsX{area.left() / pixelsPerSubLevel().width(), area.right() / pixelsPerSubLevel().width()};
	std::pair<uint8_t, uint8_t> mapPartsY{area.top() / pixelsPerSubLevel().height(), area.bottom() / pixelsPerSubLevel().height()};
	for (auto partY = mapPartsY.first; partY <= mapPartsY.second; ++partY)
	{
		for (auto partX = mapPartsX.first; partX <= mapPartsX.second; ++partX)
		{
			if (auto sourcePart = getSubLevelAtPosition(MapPosition{partX, partY}); sourcePart != nullptr)
				co_yield sourcePart;
		}
	}
}

std::generator<SubLevelPtr> Level::getSubLevelsInRectangle(const WholeTileRectangleArea& area) const noexcept
{
	std::pair<uint8_t, uint8_t> mapPartsX{area.left() / tilesPerSubLevel().width(), area.right() / tilesPerSubLevel().width()};
	std::pair<uint8_t, uint8_t> mapPartsY{area.top() / tilesPerSubLevel().height(), area.bottom() / tilesPerSubLevel().height()};
	for (auto partY = mapPartsY.first; partY <= mapPartsY.second; ++partY)
	{
		for (auto partX = mapPartsX.first; partX <= mapPartsX.second; ++partX)
		{
			if (auto sourcePart = getSubLevelAtPosition(MapPosition{partX, partY}); sourcePart != nullptr)
				co_yield sourcePart;
		}
	}
}

std::generator<SubLevelPtr> Level::getNearbySubLevels(const PixelPosition& position, uint32_t tileDistance) const noexcept
{
	auto sourcePart = getSubLevelAtPosition(position);
	if (sourcePart == nullptr)
		co_return;

	// Always yield the source part first.
	co_yield sourcePart;

	// Now yield parts in an expanding square around the source part.
	auto levelPixelSize = pixelsPerSubLevel().width();
	uint32_t levelPartSize = tilesPerSubLevel().width();
	uint32_t distance = 1;
	while (distance * levelPartSize <= tileDistance)
	{
		int32_t negDistance = -static_cast<int32_t>(distance);
		std::pair<int32_t, int32_t> offsets[] = {
			// top
			{negDistance, negDistance},
			{0, negDistance},
			{distance, negDistance},
			// middle
			{negDistance, 0},
			{distance, 0},
			// bottom
			{negDistance, distance},
			{0, distance},
			{distance, distance},
		};

		for (const auto& offset : offsets)
		{
			auto part = getSubLevelAtPosition(translatePosition(position, offset.first * levelPixelSize, offset.second * levelPixelSize));
			if (part != nullptr && part != sourcePart)
				co_yield part;
		}

		++distance;
	}
}

//----------------------------

std::generator<PlayerID> Level::findInRangePlayers(const PixelPosition& position, std::optional<std::pair<uint32_t, uint32_t>> range) const noexcept
{
	bool syncInside = m_server->cached.enableInsideSyncDistance.getValue();
	bool isInsideLevel = !isGmap();

	// If this is not a gmap, and we aren't syncing by distance inside, return all level players.
	if (isInsideLevel && !syncInside)
	{
		for (const auto& playerId : m_players)
			co_yield playerId;
		co_return;
	}

	auto syncx = m_server->cached.syncDistance[0].getValue();
	auto syncy = m_server->cached.syncDistance[1].getValue();
	auto mapSize = sizeInTiles();
	auto tilePosition = toTilePosition(position);

	if (range.has_value())
	{
		syncx = range->first;
		syncy = range->second;
	}

	// If the sync distance is larger than the level, return all the level players.
	if (syncx >= mapSize.width() && syncy >= mapSize.height())
	{
		for (const auto& playerId : m_players)
			co_yield playerId;
		co_return;
	}

	auto playerInRange = [&](const PlayerID& playerId)
	{
		if (auto player = m_server->getPlayer(playerId); player != nullptr)
		{
			auto otherTilePosition = player->getTilePosition();
			return std::abs(tilePosition.x() - otherTilePosition.x()) <= syncx && std::abs(tilePosition.y() - otherTilePosition.y()) <= syncy;
		}
		return false;
	};

	// Find all players in range.
	for (const auto& playerId : m_players)
	{
		if (playerInRange(playerId))
			co_yield playerId;
	}
}

std::generator<PlayerID> Level::findInRangePlayersForCommunication(const PixelPosition& position) const noexcept
{
	// If this is not a bigmap, use the default search.
	if (!isOnBigMap())
	{
		for (const auto& playerId : findInRangePlayers(position))
			co_yield playerId;
		co_return;
	}

	auto mapPositionOpt = m_map->getLevelPosition(levelName);
	if (!mapPositionOpt.has_value())
	{
		co_return;
	}

	auto& mapPosition = mapPositionOpt.value();
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
			auto hintLevel = std::const_pointer_cast<Level>(shared_from_this());
			if (auto level = m_server->getLoadedLevel(m_map->getLevelNameAt(x, y), hintLevel); level != nullptr)
			{
				for (const auto& playerId : level->m_players)
					co_yield playerId;
			}
		}
	}
}

std::generator<PlayerID> Level::findPlayersInLevelPart(std::string_view levelPart) const noexcept
{
	auto position = getSubLevelPositionInMap(levelPart);
	if (!position.has_value())
		co_return;
	for (const auto& playerId : findPlayersInLevelPart(position.value()))
		co_yield playerId;
}

std::generator<PlayerID> Level::findPlayersInLevelPart(const MapPosition& mapLevel) const noexcept
{
	for (const auto& playerId : m_players)
	{
		if (auto player = m_server->getPlayer(playerId); player != nullptr)
		{
			if (player->account.character.mapX == mapLevel.x() && player->account.character.mapY == mapLevel.y())
				co_yield playerId;
		}
	}
}

std::generator<NPCID> Level::findInRangeNPCs(const PixelPosition& position) const noexcept
{
	bool syncInside = m_server->cached.enableInsideSyncDistance.getValue();
	bool isInsideLevel = !isGmap();

	// If this is an inside level and we aren't going to sync by distance inside, return all level NPCs.
	if (isInsideLevel && !syncInside)
	{
		for (const auto& npcId : m_npcs)
			co_yield npcId;
		co_return;
	}

	auto syncx = m_server->cached.syncDistance[0].getValue();
	auto syncy = m_server->cached.syncDistance[1].getValue();
	auto mapSize = sizeInTiles();
	auto tilePosition = toTilePosition(position);

	auto npcInRange = [&](const NPCID& npcId)
	{
		if (auto npc = m_server->getNPC(npcId); npc != nullptr)
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
			for (const auto& npcId : m_npcs)
				co_yield npcId;
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
	// TODO: Optimize by only checking levels in range.
	for (const auto& npcId : m_npcs)
	{
		if (npcInRange(npcId))
			co_yield npcId;
	}
}

std::generator<NPCID> Level::findInRangeNPCsByDistance(const PixelPosition& position, uint32_t tileDistance) const noexcept
{
	// If this is not a map level, return all level NPCs.
	if (!isGmap())
	{
		for (const auto& npcId : m_npcs)
			co_yield npcId;
		co_return;
	}

	auto tilePosition = toTilePosition(position);

	auto npcInRange = [&](const NPCID& npcId)
	{
		if (auto npc = m_server->getNPC(npcId); npc != nullptr)
		{
			auto otherTilePosition = npc->getTilePosition();
			auto distance = std::hypotf(tilePosition.x() - otherTilePosition.x(), tilePosition.y() - otherTilePosition.y());
			return distance <= tileDistance;
		}
		return false;
	};

	// TODO: Optimize by only checking levels in range.
	for (const auto& npcId : m_npcs)
	{
		if (npcInRange(npcId))
			co_yield npcId;
	}
}

std::generator<NPCID> Level::findIntersectingNPCs(const PixelPosition& position, bool includeInvisible) const noexcept
{
	for (const auto& id : findIntersectingNPCs({position, {0, 0, 48}}, includeInvisible))
		co_yield id;
}

std::generator<NPCID> Level::findIntersectingNPCs(const PixelRectangleArea& area, bool includeInvisible) const noexcept
{
	for (const auto& npcId : findInRangeNPCs(area.position))
	{
		if (auto npc = m_server->getNPC(npcId); npc != nullptr)
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

std::generator<NPCID> Level::findIntersectingNPCsForCollision(const PixelPosition& position) const noexcept
{
	for (const auto& id : findIntersectingNPCsForCollision({position, {0, 0, 48}}))
		co_yield id;
}

std::generator<NPCID> Level::findIntersectingNPCsForCollision(const PixelRectangleArea& area) const noexcept
{
	for (const auto& npcId : findInRangeNPCs(area.position))
	{
		if (auto npc = m_server->getNPC(npcId); npc != nullptr)
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
	if (!m_server->hasNPCServer())
		return nullptr;

	auto itemName = LevelItem::getItemName(item);
	if (LevelItem::isRupeeType(item))
		itemName = "gralats";

	auto itemclass = m_server->getNPCServer()->getClass(itemName).lock();
	if (itemclass == nullptr)
		return nullptr;

	static std::unordered_map<LevelItemType, NPCProp> stackableItems =
		{
			{LevelItemType::GREENRUPEE, NPCProp::RUPEES},
			{LevelItemType::BLUERUPEE, NPCProp::RUPEES},
			{LevelItemType::REDRUPEE, NPCProp::RUPEES},
			{LevelItemType::GOLDRUPEE, NPCProp::RUPEES},
			{LevelItemType::BOMBS, NPCProp::BOMBS},
			{LevelItemType::DARTS, NPCProp::ARROWS},
			{LevelItemType::HEART, NPCProp::POWER},
		};
	static std::unordered_map<LevelItemType, uint32_t> stackableCount =
		{
			{LevelItemType::GREENRUPEE, 1},
			{LevelItemType::BLUERUPEE, 5},
			{LevelItemType::REDRUPEE, 30},
			{LevelItemType::GOLDRUPEE, 100},
			{LevelItemType::BOMBS, 5},
			{LevelItemType::DARTS, 5},
			{LevelItemType::HEART, 2},
		};

	std::shared_ptr<NPC> itemNPC = nullptr;

	// Determine the NPC location.
	TilePosition loc = toTilePosition(position).translate(-0.5f, -1.0f);

	auto stackable = stackableItems.find(item);
	if (stackable != stackableItems.end())
	{
		// Find existing items, and stack with the existing.
		PixelRectangleArea searchArea{toPixelPosition(loc).translate(-2 * 16, -2 * 16), {6 * 16, 6 * 16}};
		auto npcList = findIntersectingNPCs(searchArea);
		for (const auto& npcId : npcList)
		{
			if (auto npc = m_server->getNPC(npcId); npc != nullptr && npc->hasJoinedClass(itemName))
				itemNPC = npc;
		}
	}

	// Create a new npc for this item.
	bool isNew = !itemNPC;
	if (isNew)
	{
		itemNPC = m_server->getNPCServer()->addNPC("", std::format("if (created) join {};", itemName), shared_from_this(), {loc[0], loc[1]}, NPCTYPE_ITEM);
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

size_t Level::getMapIndexAtPosition(const MapPosition& mapLevel) const noexcept
{
	if (m_map == nullptr)
		return 0;

	size_t maxLevels = static_cast<size_t>(m_map->size.width()) * m_map->size.height();
	return std::min(maxLevels, (static_cast<size_t>(m_map->size.width()) * mapLevel.y()) + mapLevel.x());
}

//----------------------------

ScriptObject source::FromLevel(LevelPtr level)
{
	size_t hash = string::string_hash{}(level->levelName);
	return std::make_pair(hash, ScriptObjectType::LEVEL);
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
