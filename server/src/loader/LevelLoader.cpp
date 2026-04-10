#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <format>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <BabyDI.h>
#include <Server.h>
#include <filesystem/File.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <level/Level.h>
#include <level/LevelBaddy.h>
#include <level/LevelChest.h>
#include <level/LevelItem.h>
#include <level/LevelTerrain.h>
#include <level/LevelTiles.h>
#include <level/Map.h>
#include <loader/LevelLoader.h>
#include <object/NPC.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

/*
Z3-V1.03
	12 bit tiles
	links
	baddies (no verses)
	signs

Z3-V1.04
	12 bit tiles
	links
	baddies
	signs

GR-V1.00
	12 bit tiles
	links
	baddies
	npcs
	signs

GR-V1.01
	12 bit tiles
	links
	baddies
	npcs
	chests
	signs

GR-V1.02
	13 bit tiles
	links
	baddies
	npcs
	chests
	signs

GR-V1.03
	13 bit tiles
	links (using variables)
	baddies
	npcs
	chests
	signs

GR-V1.04

GR-V1.05
	tile layers

GLEVNW01

GWEBL001

GSERVL01
	saves modified npcs in 'levelnpcs/levelfilename.save`
	index "prop string"

	REPLACENPCS
	1 "prop string"
	REPLACENPCSEND
*/

constexpr int MAX_TILE_COUNT = 64 * 64; // 4096 tiles per level.

static constexpr int getBase64Position(char c)
{
	if (c >= 'a')
		return 26 + (c - 'a');
	else if (c >= 'A')
		return (c - 'A');
	else if (c >= '0' && c <= '9')
		return 52 + (c - '0');

	switch (c)
	{
		case '+':
			return 52 + 10;
		case '/':
			return 52 + 11;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

LevelPtr LevelLoader::loadLevel(const std::filesystem::path& levelName)
{
	auto level = std::make_shared<Level>();
	if (loadLevelInto(levelName, level))
		return level;

	return nullptr;
}

bool LevelLoader::loadLevelInto(const std::filesystem::path& levelName, LevelPtr level)
{
	auto server = BabyDI::Get<Server>();
	level->levelName = fs::getANSIFileName(levelName);

	// Normal level loading (just one sub-level).
	bool isGmap = string::ends_withi(level->levelName, ".gmap"sv);
	if (!isGmap)
	{
		auto levelData = server->getCachedLevelData(level->levelName);
		if (levelData == nullptr)
			return false;

		// Check if this level belongs to a bigmap.
		if (auto map = server->findMapForLevel(MapType::BIGMAP, level->levelName); map != nullptr)
			level->setMap(map);

		level->m_filePath = levelData->filePath;
		level->modTime = levelData->modTime;
		level->m_levelParts.push_back(attachStaticDataToLevel(level, std::nullopt, levelData));
		loadStaticDataNPCs(level, std::nullopt, levelData);

		// Bind listeners for level data changes.
		auto handle = levelData->onDataRefreshed.subscribe([weakSelf = std::weak_ptr<Level>(level)](StaticLevelDataPtr staticData)
		{
			if (auto self = weakSelf.lock(); self != nullptr)
				self->reload(staticData);
		});
		level->m_levelParts.front()->staticDataRefreshedHandle = handle;

		level->loaded = true;
		return true;
	}

	// Check if the server generation supports gmaps.
	if (server->Generation == ServerGeneration::ORIGINAL)
	{
		log::printLine(log::server, "[ERROR] Server generation does not support gmaps, refusing to load {}.", level->levelName);
		return false;
	}

	// Find the map for the gmap level.
	auto map = server->findMap(level->levelName);
	if (map == nullptr)
		return false;

	// Set the map and make space for the level parts.
	level->setMap(map);
	level->m_levelParts.resize(static_cast<size_t>(map->size.width()) * map->size.height());

	// Load all the sub-levels for the gmap.
	auto& gmapLevelList = server->getGmapLevelList();
	for (const auto& [levelData, levelPos] : map->getAllLevelData())
	{
		if (levelData == nullptr)
			continue;

		auto index = static_cast<size_t>(levelPos.y()) * map->size.width() + levelPos.x();
		level->m_levelParts[index] = attachStaticDataToLevel(level, levelPos, levelData);
		loadStaticDataNPCs(level, levelPos, levelData);

		// Register the level in the gmap level list for faster access.
		gmapLevelList.insert({ levelData->levelName, level });

		// Bind listeners for level data changes.
		auto handle = levelData->onDataRefreshed.subscribe([weakSelf = std::weak_ptr<Level>(level)](StaticLevelDataPtr staticData)
		{
			if (auto self = weakSelf.lock(); self != nullptr)
				self->reload(staticData);
		});
		level->m_levelParts[index]->staticDataRefreshedHandle = handle;
	}

	level->loaded = true;
	return true;
}

//----------------------------

StaticLevelDataPtr LevelLoader::loadStaticData(const std::filesystem::path& levelName)
{
	auto data = std::make_shared<StaticLevelData>();

	// Find the level file.
	auto levelString = fs::getANSIFileName(levelName);
	data->levelName = levelString;

	// Load the data.
	if (loadStaticDataInto(data))
		return data;

	return nullptr;
}

bool LevelLoader::loadStaticDataInto(StaticLevelDataPtr staticLevelData)
{
	auto* server = BabyDI::Get<Server>();
	auto& fileSystem = server->getFileSystem();

	auto fileInfo = fileSystem.infoi(fs::FileCategory::LEVEL, staticLevelData->levelName);
	if (fileInfo == nullptr)
		return false;

	// Open it for loading.
	auto fileData = fileInfo->openFile();
	if (fileData == nullptr || !fileData->opened())
		return false;

	// Get the file version.
	auto version = fileData->readChars(8);

	// Save some level details.
	staticLevelData->filePath = fileInfo->file;
	staticLevelData->modTime = fileInfo->getModTime();

	// Load the level data.
	if (version == "GLEVNW01")
		loadNW(staticLevelData, version, fileSystem, fileData);
	if (version.substr(0, 3) == "GR-")
		loadGraal(staticLevelData, version, fileSystem, fileData);
	if (version.substr(0, 3) == "Z3-")
		loadZelda(staticLevelData, version, fileSystem, fileData);

	return true;
}

SubLevelPtr LevelLoader::attachStaticDataToLevel(LevelPtr level, std::optional<MapPosition> mapPosition, StaticLevelDataPtr staticData)
{
	auto subLevel = std::make_shared<SubLevel>();
	subLevel->parentLevel = level;
	subLevel->staticData = staticData;
	subLevel->mapPosition = mapPosition;

	// Record if this sub-level is related to a level that is a gmap or on a bigmap.
	if (level->isGmap())
		subLevel->isOnGmap = true;
	if (level->isOnBigMap())
		subLevel->isOnBigMap = true;

	// Reserve space for baddies to avoid reallocations, which will destroy timeout callback pointers.
	subLevel->baddies.reserve(0xFF);

	// Load baddies.
	if (!level->isGmap())
	{
		// Mark all existing baddies as dead and non-respawning.
		for (auto& baddy : subLevel->baddies)
		{
			baddy.setRespawn(false);
			baddy.mode = BaddyMode::DEAD;
		}

		// Copy over the new baddies.
		for (size_t i = 0; i < staticData->baddies.size() && i < 50; ++i)
		{
			if (i < subLevel->baddies.size())
				subLevel->baddies[i] = staticData->baddies[i];
			else
				subLevel->baddies.push_back(staticData->baddies[i]);

			subLevel->baddies.back().setLevel(level);
		}
	}

	// Load heights.
	if (level->isGmap())
	{
		// Check for map terrain.
		if (auto map = level->getMap(); map != nullptr && !map->terrain.levelSeeds.empty())
		{
			auto& terrain = subLevel->terrain.emplace();
			auto seedIndex = mapPosition.value().y() * map->size.width() + mapPosition.value().x();
			terrain.levelSeed = map->terrain.levelSeeds[seedIndex];
			terrain.levelHeight = map->terrain.levelHeightDeviation;
			terrain.levelChaos = map->terrain.levelChaos;
			terrain.levelHeightOverrides = staticData->heights;

			generateTerrain(terrain, map->terrain, mapPosition.value(), map->size);
		}
	}

	return subLevel;
}

void LevelLoader::loadStaticDataNPCs(LevelPtr level, std::optional<MapPosition> mapPosition, StaticLevelDataPtr staticData)
{
	// The sub-level must exist before this method gets called.

	auto server = BabyDI::Get<Server>();

	// Delete existing level NPCs.
	auto& npcs = level->getNPCs();
	for (auto iter = npcs.begin(); iter != npcs.end();)
	{
		if (auto npc = server->getNPC(*iter); npc == nullptr || npc->storageType == NPCStorageType::LEVEL)
		{
			if (npc && (!mapPosition.has_value() || npc->character.getMapPosition() == mapPosition))
			{
				server->deleteNPC(npc, false);
				iter = npcs.erase(iter);
				continue;
			}
		}
		++iter;
	}

	// Add new NPCs.
	for (const auto& npcData : staticData->npcs)
	{
		auto& gen = server->getNPCIdGenerator();
		auto npcId = gen.getAvailableId(NPCID_GEN_LOCAL);
		auto npc = std::make_shared<NPC>(npcId, NPCStorageType::LEVEL);

		// Cached data.
		npc->character.localPixelX = npcData.position.x();
		npc->character.localPixelY = npcData.position.y();
		npc->image = npcData.image;
		npc->setLevel(level);

		// Map position.
		if (mapPosition.has_value())
		{
			npc->character.mapX = mapPosition.value().x();
			npc->character.mapY = mapPosition.value().y();
			npc->modTime[PROPID(NPCProp::GMAPLEVELX)] = server->getFrameStartTime();
			npc->modTime[PROPID(NPCProp::GMAPLEVELY)] = server->getFrameStartTime();
		}

		// Script.
		npc->setScript(npcData.script);

		// Add.
		npc->recordInitialState();
		server->addNPC(npc, level->loaded);
	}
}

///////////////////////////////////////////////////////////////////////////////

bool LevelLoader::loadZelda(StaticLevelDataPtr levelData, std::string_view fileVersion, fs::FileSystem& fileSystem, fs::FilePtr& fileData)
{
	int version = -1;
	if (fileVersion == "Z3-V1.03")
		version = 3;
	else if (fileVersion == "Z3-V1.04")
		version = 4;
	else return false;

	// Load tiles.
	loadBinaryTiles(levelData, fileData, 12, 1);

	// Load links.
	loadBinaryLinks(levelData, fileData, fileSystem);

	// Load the baddies.
	loadBinaryBaddies(levelData, fileData, version);

	// Load signs.
	loadBinarySigns(levelData, fileData);

	return true;
}

bool LevelLoader::loadGraal(StaticLevelDataPtr levelData, std::string_view fileVersion, fs::FileSystem& fileSystem, fs::FilePtr& fileData)
{
	// Grab file version.
	int version = -1;
	if (fileVersion == "GR-V1.00")
		version = 0;
	else if (fileVersion == "GR-V1.01")
		version = 1;
	else if (fileVersion == "GR-V1.02")
		version = 2;
	else if (fileVersion == "GR-V1.03")
		version = 3;
	else if (fileVersion == "GR-V1.05")
		version = 5;
	else return false;

	// Determine layer count.
	uint8_t layers = 1;
	if (version >= 5)
	{
		// Read the layer count.
		layers = (uint8_t)fileData->readPackedIntegral<1>();
	}

	// Load tiles.
	loadBinaryTiles(levelData, fileData, version > 1 ? 13 : 12, layers);

	// Load links.
	loadBinaryLinks(levelData, fileData, fileSystem);

	// Load baddies.
	loadBinaryBaddies(levelData, fileData, version);

	// Load NPCs.
	loadBinaryNPCs(levelData, fileData);

	// Load chests.
	if (version > 0)
	{
		loadBinaryChests(levelData, fileData);
	}

	// Load signs.
	loadBinarySigns(levelData, fileData);

	return true;
}

void LevelLoader::loadBinaryTiles(StaticLevelDataPtr levelData, fs::FilePtr& fileData, uint32_t bits, int layers)
{
	for (uint8_t layer = 0; layer < layers; ++layer)
	{
		if (fileData->finishedReading())
			break;

		auto tiles = levelData->tiles.getOrCreateLayer(layer);

		uint32_t buffer = 0;
		uint32_t read = 0;
		uint16_t code = 0;
		int tileReadAmount = 1;
		int boardWriteIndex = 0;
		bool isExtraLayer = layer != 0;

		bool doubleTileRLEMode = false;
		int32_t rleTiles[2] = { -1, -1 };

		// Read the tiles.
		while (boardWriteIndex < MAX_TILE_COUNT && !fileData->finishedReading())
		{
			// Every control code/tile is either 12 or 13 bits.  WTF.
			// Read in the bits.
			while (read < bits)
			{
				buffer += fileData->readIntegral<1>() << read;
				read += 8;
			}

			// Pull out a single 12/13 bit code from the buffer.
			code = buffer & (bits == 12 ? 0xFFF : 0x1FFF);
			buffer >>= bits;
			read -= bits;

			// See if we have an RLE control code.
			// Control codes determine how the RLE scheme works.
			if (code & ((bits == 12) ? 0x800 : 0x1000))
			{
				// If the 0x100 bit is set, we are in a double repeat mode.
				// {double 4}56 = 56565656
				if (code & 0x100) doubleTileRLEMode = true;

				// How many tiles do we count?
				tileReadAmount = code & 0xFF;
				continue;
			}

			// If our count is 1, just read in a tile.  This is the default mode.
			if (tileReadAmount == 1)
			{
				// Extra layer tiles are 0xFFFF.
				if (isExtraLayer && code == 0xFFF)
					code = std::numeric_limits<uint16_t>::max();

				tiles->at(boardWriteIndex++) = code;
				continue;
			}

			// If we reach here, we have an RLE scheme.
			// See if we are in double repeat mode or not.
			if (doubleTileRLEMode)
			{
				// Read in our first tile.
				if (rleTiles[0] == -1)
				{
					rleTiles[0] = code;
					continue;
				}

				// Read in our second tile.
				rleTiles[1] = code;

				// Determine the actual tiles we are going to write.
				// Tiles on additional layers are 0xFFFF if not set, so handle that.
				uint16_t first = std::numeric_limits<uint16_t>::max();
				uint16_t second = rleTiles[1];
				if (!isExtraLayer || rleTiles[0] != 0xFFF)
					first = rleTiles[0];
				if (isExtraLayer && rleTiles[1] == 0xFFF)
					second = std::numeric_limits<uint16_t>::max();

				// Add the tiles now.
				for (int i = 0; i < tileReadAmount && boardWriteIndex < MAX_TILE_COUNT - 1; ++i)
				{
					tiles->at(boardWriteIndex++) = first;
					tiles->at(boardWriteIndex++) = second;
				}

				// Clean up.
				rleTiles[0] = rleTiles[1] = -1;
				doubleTileRLEMode = false;
				tileReadAmount = 1;
			}
			// Regular RLE scheme.
			else
			{
				for (int i = 0; i < tileReadAmount && boardWriteIndex < MAX_TILE_COUNT; ++i)
				{
					// Extra layer tiles are 0xFFFF.
					if (isExtraLayer && code == 0xFFF)
						code = std::numeric_limits<uint16_t>::max();

					tiles->at(boardWriteIndex++) = code;
				}
				tileReadAmount = 1;
			}
		}
	}
}

void LevelLoader::loadBinaryLinks(StaticLevelDataPtr levelData, fs::FilePtr& fileData, fs::FileSystem& fileSystem)
{
	while (!fileData->finishedReading())
	{
		auto line = fileData->readLine();
		if (line.length() == 0 || line == "#") break;

		// Assemble the level string.
		auto splitData = string::splitToVectorView(line, " "sv);
		if (splitData.size() < 7)
			continue;

		auto end = splitData.size();

		// Get the positions and destinations of the link.
		Rectangle<uint8_t, uint8_t> rect;
		rect.position[0] = string::toNumber<uint8_t>(splitData[end - 6]);
		rect.position[1] = string::toNumber<uint8_t>(splitData[end - 5]);
		rect.size[0] = string::toNumber<uint8_t>(splitData[end - 4]);
		rect.size[1] = string::toNumber<uint8_t>(splitData[end - 3]);
		auto destX = splitData[end - 2];
		auto destY = splitData[end - 1];

		// Get the level name.
		// Levels with spaces in the name are not supposed to be allowed, but we support it anyway.
		// TODO: This will not work with levels with two+ spaces in a row.
		auto destLevel = string::join(splitData | std::views::take(end - 6), " "sv);

		if (!fileSystem.has(fs::FileCategory::LEVEL, destLevel))
			continue;

		levelData->links.emplace_back(rect, destX, destY, destLevel);
	}
}

void LevelLoader::loadBinaryBaddies(StaticLevelDataPtr levelData, fs::FilePtr& fileData, int version)
{
	while (!fileData->finishedReading())
	{
		int8_t x = fileData->readIntegral<1>();
		int8_t y = fileData->readIntegral<1>();
		int8_t type = fileData->readIntegral<1>();

		// Ends with an invalid baddy.
		if (x == -1 && y == -1 && type == -1)
		{
			fileData->readLine(); // Empty verses.
			break;
		}

		// Add the baddy.
		LevelBaddy baddy{ toLocalPixelPosition((float)x, (float)y), (BaddyType)type, {} };

		// Only v1.04+ baddies have verses.
		if (version > 3)
		{
			// Load the verses.
			auto verseLine = fileData->readLine();
			auto verseParts = string::splitToVectorView(verseLine, "\\"sv);
			for (size_t j = 0; j < std::min((size_t)3, verseParts.size()); ++j)
				baddy.verses[j] = verseParts[j];
		}

		levelData->baddies.emplace_back(std::move(baddy));
	}
}

void LevelLoader::loadBinaryNPCs(StaticLevelDataPtr levelData, fs::FilePtr& fileData)
{
	int index = 0;
	while (!fileData->finishedReading())
	{
		++index;

		auto line = fileData->readLine();
		if (line.length() == 0 || line == "#") break;

		TilePosition position;
		position[0] = line[0] - 32;
		position[1] = line[1] - 32;

		std::string_view lineView{ line };
		lineView.remove_prefix(2);

		auto image = string::extractLine(lineView, '#');
		auto code = string::replace(lineView, "\xa7", "\n");

		LevelNPCTemplate npc{ .image = image, .position = toLocalPixelPosition(position) };
		npc.script.setOriginalSource(std::format("{}.{}", levelData->levelName, index), code);
		levelData->npcs.emplace_back(std::move(npc));
	}
}

void LevelLoader::loadBinaryChests(StaticLevelDataPtr levelData, fs::FilePtr& fileData)
{
	while (!fileData->finishedReading())
	{
		auto line = fileData->readLine();
		if (line.length() == 0 || line == "#") break;

		uint8_t x = line[0];
		uint8_t y = line[1];
		char item = line[2];
		char signindex = line[3];

		LevelChest chest{ .position = LocalWholeTilePosition{ x, y }, .item = LevelItemType(item), .sign = (uint8_t)signindex };
		levelData->chests.emplace_back(std::move(chest));
	}
}

void LevelLoader::loadBinarySigns(StaticLevelDataPtr levelData, fs::FilePtr& fileData)
{
	while (!fileData->finishedReading())
	{
		auto line = fileData->readLine();
		if (line.length() == 0) break;

		uint8_t x = line[0];
		uint8_t y = line[1];
		std::string_view text{ line };
		text.remove_prefix(2);

		levelData->signs.emplace_back(LocalWholeTilePosition{ x, y }, text, true);
	}
}

//----------------------------

bool LevelLoader::loadNW(StaticLevelDataPtr levelData, std::string_view fileVersion, fs::FileSystem& fileSystem, fs::FilePtr& fileData)
{
	std::string curLine;
	std::vector<std::string_view> splitData;
	int npcIndex = 0;

	while (!fileData->finishedReading())
	{
		// Read the line.
		curLine = fileData->readLine();
		std::string_view line{ string::trim(curLine) };
		if (line.empty())
			continue;

		// Get the line data.
		auto [section, data] = string::extractConfigParts(line);
		splitData = string::splitToVectorView(data);

		// Parse each line.
		if (section == "BOARD")
		{
			if (splitData.size() != 5)
				continue;

			uint8_t x = string::toNumber<uint8_t>(splitData[0]);
			uint8_t y = string::toNumber<uint8_t>(splitData[1]);
			uint8_t width = string::toNumber<uint8_t>(splitData[2]);
			uint8_t layer = string::toNumber<uint8_t>(splitData[3]);
			if (!inRangeInclusive(x, 0, 64) || !inRangeInclusive(y, 0, 64) || width <= 0 || x + width > 64)
				continue;
			if (splitData[4].length() < static_cast<size_t>(width) * 2)
				continue;

			auto tiles = levelData->tiles.getOrCreateLayer(layer);
			for (size_t index = 0; index < width; ++index)
			{
				char left = splitData[4].at(index * 2);
				char top = splitData[4].at(index * 2 + 1);
				short tile = getBase64Position(left) << 6;
				tile += getBase64Position(top);
				if (tile == 0x3FFF)
					tile = constants::EmptyTileInLayer;

				tiles->at(static_cast<size_t>(x + index) + static_cast<size_t>(y * 64)) = tile;
			}
		}
		else if (section == "CHEST")
		{
			if (splitData.size() < 4)
				continue;

			LevelItemType itemType = LevelItem::getItemId(std::string{ splitData[2] });
			if (itemType != LevelItemType::INVALID)
			{
				uint8_t chestx = string::toNumber<uint8_t>(splitData[0]);
				uint8_t chesty = string::toNumber<uint8_t>(splitData[1]);
				char signidx = string::toNumber<char>(splitData[3]);

				LevelChest chest{ .position = LocalWholeTilePosition{ chestx, chesty }, .item = itemType, .sign = (uint8_t)signidx };
				levelData->chests.emplace_back(std::move(chest));
			}
		}
		else if (section == "LINK")
		{
			if (splitData.size() < 7)
				continue;

			auto end = splitData.size();

			// Get the positions and destinations of the link.
			Rectangle<uint8_t, uint8_t> rect;
			rect.position[0] = string::toNumber<uint8_t>(splitData[end - 6]);
			rect.position[1] = string::toNumber<uint8_t>(splitData[end - 5]);
			rect.size[0] = string::toNumber<uint8_t>(splitData[end - 4]);
			rect.size[1] = string::toNumber<uint8_t>(splitData[end - 3]);
			auto destX = splitData[end - 2];
			auto destY = splitData[end - 1];

			// Get the level name.
			// Levels with spaces in the name are not supposed to be allowed, but we support it anyway.
			// TODO: This will not work with levels with two+ spaces in a row.
			auto destLevel = string::join(splitData | std::views::take(end - 6), " "sv);

			if (!fileSystem.has(fs::FileCategory::LEVEL, destLevel))
				continue;

			levelData->links.emplace_back(rect, destX, destY, destLevel);
		}
		else if (section == "SIGN")
		{
			if (splitData.size() != 2)
				continue;

			uint8_t x = string::toNumber<uint8_t>(splitData[0]);
			uint8_t y = string::toNumber<uint8_t>(splitData[1]);

			// Grab the sign code.
			std::string text;
			for (const auto& line : fileData->readLinesUntilSectionEnd("SIGNEND"))
			{
				text += line;
				text += '\n';
			}

			// Erase the final newline.
			if (text.back() == '\n')
				text.pop_back();

			// Add the new sign.
			levelData->signs.emplace_back(LocalWholeTilePosition{ x, y }, text, false);
		}
		else if (section == "BADDY")
		{
			if (splitData.size() != 3)
				continue;

			TilePosition position;
			position[0] = string::toFloat(splitData[0]);
			position[1] = string::toFloat(splitData[1]);
			BaddyType type = LevelBaddy::getBaddyTypeFromString(std::string{ splitData[2] });

			LevelBaddy baddy{ toLocalPixelPosition(position), type, {} };
			baddy.id = static_cast<uint8_t>(levelData->baddies.size() + 1);

			int i = 0;
			for (const auto& verse : fileData->readLinesUntilSectionEnd("BADDYEND"))
			{
				if (i < 3) baddy.verses[i] = verse;
				++i;
			}

			levelData->baddies.emplace_back(std::move(baddy));
		}
		else if (section == "NPC")
		{
			++npcIndex;

			if (splitData.size() < 3)
				continue;

			auto end = splitData.size();

			TilePosition position;
			position[0] = string::toFloat(splitData[end - 2]);
			position[1] = string::toFloat(splitData[end - 1]);

			// Remove the back 2 entries from the split data.
			splitData.erase(splitData.begin() + (end - 2), splitData.end());

			// Combine all the rest.
			std::string image = string::join(splitData, " "sv);

			// If the image is just a hyphen, clear it.
			if (image == "-")
				image.clear();

			std::string code;
			for (const auto& line : fileData->readLinesUntilSectionEnd("NPCEND"))
			{
				code += line;
				code += '\n';
			}

			LevelNPCTemplate npc{ .image = image, .position = toLocalPixelPosition(position) };
			npc.script.setOriginalSource(std::format("{}.{}", levelData->levelName, npcIndex), code);
			levelData->npcs.emplace_back(std::move(npc));
		}
		else if (section == "HEIGHTS")
		{
			for (const auto& heights : fileData->readLinesUntilSectionEnd("HEIGHTSEND"))
			{
				auto values = string::splitToVectorView(heights, ","sv);
				for (const auto& val : values)
					levelData->heights.push_back(string::toDouble(string::trim(val)));
			}

			if (levelData->heights.size() != 81)
			{
				log::printLine(log::server, "[WARNING] Level '{}' has an improper amount of heights. Expected: {}, found: {}.", levelData->levelName, 81, levelData->heights.size());
				levelData->heights.clear();
			}
		}
		else
		{
			log::printLine(log::server, "[WARNING] Level '{}' has unhandled section '{}'.", levelData->levelName, section);
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
