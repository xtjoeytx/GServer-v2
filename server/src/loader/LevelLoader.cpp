#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

#include <CString.h>
#include <IUtil.h>

#include <BabyDI.h>
#include <FileSystem.h>
#include <Server.h>
#include <level/Level.h>
#include <level/LevelBaddy.h>
#include <level/LevelItem.h>
#include <level/LevelTiles.h>
#include <level/Map.h>
#include <loader/LevelLoader.h>
#include <object/NPC.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>

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

static size_t readBinaryTiles(uint8_t bits, std::span<uint8_t>& data, LevelTiles& outputTiles, bool isExtraLayer)
{
	uint32_t buffer = 0;
	uint32_t read = 0;
	uint16_t code = 0;
	int tileReadAmount = 1;
	int boardWriteIndex = 0;

	bool doubleTileRLEMode = false;
	int32_t rleTiles[2] = { -1, -1 };

	// Read the tiles.
	size_t readPos = 0;
	while (boardWriteIndex < MAX_TILE_COUNT && readPos < data.size())
	{
		// Every control code/tile is either 12 or 13 bits.  WTF.
		// Read in the bits.
		while (read < bits)
		{
			buffer += data[readPos++] << read;
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
				code = ~0;

			outputTiles[boardWriteIndex++] = code;
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
			uint16_t first = ~0;
			uint16_t second = rleTiles[1];
			if (!isExtraLayer || rleTiles[0] != 0xFFF)
				first = rleTiles[0];
			if (isExtraLayer && rleTiles[1] == 0xFFF)
				second = ~0;

			// Add the tiles now.
			for (int i = 0; i < tileReadAmount && boardWriteIndex < MAX_TILE_COUNT - 1; ++i)
			{
				outputTiles[boardWriteIndex++] = first;
				outputTiles[boardWriteIndex++] = second;
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
					code = ~0;

				outputTiles[boardWriteIndex++] = code;
			}
			tileReadAmount = 1;
		}
	}
	return readPos;
}

///////////////////////////////////////////////////////////////////////////////

LevelPtr LevelLoader::loadLevel(const std::filesystem::path& levelName)
{
	// We need to construct the level object this way so the "friend" status applies.
	// If we use the normal std::make_shared, the constructor is inaccessible.
	std::shared_ptr<Level> level{ new Level() };

	return loadLevelInto(level, levelName);
}

LevelPtr LevelLoader::loadLevelInto(LevelPtr level, const std::filesystem::path& levelName)
{
	auto* server = BabyDI::Get<Server>();

	// Get the appropriate filesystem.
	FileSystem* fileSystem = server->getFileSystem();
	if (!server->getSettings().getBool("nofoldersconfig", false))
		fileSystem = server->getFileSystem(FS_LEVEL);

	// Find the level file.
	auto levelPath = fileSystem->find(levelName.string());

	// Load it.
	CString fileData;
	if (!fileData.load(levelPath))
		return nullptr;

	// Grab file version.
	CString fileVersion = fileData.readChars(8);

	// Save level details.
	level->m_fileVersion = fileVersion;
	level->m_fileName = levelPath;
	level->m_modTime = fileSystem->getModTime(levelName.string());
	level->m_actualLevelName = level->m_levelName = levelName.string();

	// If the level is on a map, record that now.
	for (const auto& map : server->getMapList())
	{
		int mx, my;
		if (map->isLevelOnMap(levelName.string(), mx, my))
		{
			level->setMap(map, mx, my);
			break;
		}
	}

	// Load the level.
	if (fileVersion == "GLEVNW01")
		return loadNW(level, fileSystem, fileData);
	if (fileVersion.subString(0, 3) == "GR-")
		return loadGraal(level, fileSystem, fileData);
	if (fileVersion.subString(0, 3) == "Z3-")
		return loadZelda(level, fileSystem, fileData);

	// Bad level version.
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

LevelPtr LevelLoader::loadZelda(LevelPtr level, FileSystem* fileSystem, CString& fileData)
{
	int version = -1;
	if (level->m_fileVersion == "Z3-V1.03")
		version = 3;
	else if (level->m_fileVersion == "Z3-V1.04")
		version = 4;

	if (version == -1) return nullptr;

	// Load tiles.
	std::span<uint8_t> tiles(reinterpret_cast<uint8_t*>(fileData.text() + fileData.readPos()), fileData.bytesLeft());
	auto read = readBinaryTiles(12, tiles, level->m_tiles[0], false);
	fileData.setRead(fileData.readPos() + read);

	// Load the links.
	{
		while (fileData.bytesLeft())
		{
			CString line = fileData.readString("\n");
			if (line.length() == 0 || line == "#") break;

			// Assemble the level string.
			std::vector<CString> vline = line.tokenize();
			CString linkLevel = vline[0];
			if (vline.size() > 7)
			{
				for (size_t i = 0; i < vline.size() - 7; ++i)
					linkLevel << " " << vline[1 + i];
			}

			if (fileSystem->find(linkLevel).isEmpty())
				continue;

			level->addLink(vline);
		}
	}

	// Load the baddies.
	{
		while (fileData.bytesLeft())
		{
			signed char x = fileData.readChar();
			signed char y = fileData.readChar();
			signed char type = fileData.readChar();

			// Ends with an invalid baddy.
			if (x == -1 && y == -1 && type == -1)
			{
				fileData.readString("\n"); // Empty verses.
				break;
			}

			// Add the baddy.
			LevelBaddy* baddy = level->addBaddy((float)x, (float)y, static_cast<BaddyType>(type));
			if (baddy == nullptr)
				continue;

			// Only v1.04+ baddies have verses.
			if (version > 3)
			{
				// Load the verses.
				std::vector<CString> bverse = fileData.readString("\n").tokenize("\\");
				CString props;
				for (char j = 0; j < (char)bverse.size(); ++j)
					props >> (char)(PROPID(BaddyProp::VERSESIGHT) + j) >> (char)bverse[j].length() << bverse[j];
				if (props.length() != 0) baddy->setPropsFromPacket(props);
			}
		}
	}

	// Load signs.
	{
		while (fileData.bytesLeft())
		{
			CString line = fileData.readString("\n");
			if (line.length() == 0) break;

			signed char x = line.readGChar();
			signed char y = line.readGChar();
			CString text = line.readString("");

			level->addSign(x, y, text, true);
		}
	}

	return level;
}

LevelPtr LevelLoader::loadGraal(LevelPtr level, FileSystem* fileSystem, CString& fileData)
{
	// Grab file version.
	int version = -1;
	if (level->m_fileVersion == "GR-V1.00")
		version = 0;
	else if (level->m_fileVersion == "GR-V1.01")
		version = 1;
	else if (level->m_fileVersion == "GR-V1.02")
		version = 2;
	else if (level->m_fileVersion == "GR-V1.03")
		version = 3;
	else if (level->m_fileVersion == "GR-V1.05")
		version = 5;

	if (version == -1) return nullptr;

	uint8_t layers = 1;
	if (version >= 5)
	{
		// Read the layer count.
		layers = fileData.readGUChar();
	}

	// Load tiles.
	for (uint8_t i = 0; i < layers; ++i)
	{
		std::span<uint8_t> tiles(reinterpret_cast<uint8_t*>(fileData.text() + fileData.readPos()), fileData.bytesLeft());
		auto read = readBinaryTiles(version > 1 ? 13 : 12, tiles, level->m_tiles[i], i != 0);
		fileData.setRead(fileData.readPos() + read);
	}

	// Load the links.
	{
		while (fileData.bytesLeft())
		{
			CString line = fileData.readString("\n");
			if (line.length() == 0 || line == "#") break;

			// Assemble the level string.
			std::vector<CString> vline = line.tokenize();
			CString linkLevel = vline[0];
			if (vline.size() > 7)
			{
				for (size_t i = 0; i < vline.size() - 7; ++i)
					linkLevel << " " << vline[1 + i];
			}

			if (fileSystem->find(linkLevel).isEmpty())
				continue;

			level->addLink(vline);
		}
	}

	// Load the baddies.
	{
		while (fileData.bytesLeft())
		{
			signed char x = fileData.readChar();
			signed char y = fileData.readChar();
			signed char type = fileData.readChar();

			// Ends with an invalid baddy.
			if (x == -1 && y == -1 && type == -1)
			{
				fileData.readString("\n"); // Empty verses.
				break;
			}

			// Add the baddy.
			LevelBaddy* baddy = level->addBaddy((float)x, (float)y, static_cast<BaddyType>(type));
			if (baddy == nullptr)
				continue;

			// Load the verses.
			std::vector<CString> bverse = fileData.readString("\n").tokenize("\\");
			CString props;
			for (char j = 0; j < (char)bverse.size(); ++j)
				props >> (char)(PROPID(BaddyProp::VERSESIGHT) + j) >> (char)bverse[j].length() << bverse[j];
			if (props.length() != 0) baddy->setPropsFromPacket(props);
		}
	}

	// Load NPCs.
	{
		auto* server = BabyDI::Get<Server>();

		std::unique_ptr<log::Indent> indent;
		if (server->hasNPCServer())
		{
			indent = std::make_unique<log::Indent>(log::server.indent(log::server.indentLevel != 0 ? 1 : 0));
			log::printLine(log::server, "Loading NPCs for level '{}'...", level->getLevelName());
		}

		while (fileData.bytesLeft())
		{
			std::unique_ptr<log::Profile> profile;
			if (server->hasNPCServer())
				profile = std::make_unique<log::Profile>(log::server, "", " ({1:0.6} ms)");

			CString line = fileData.readString("\n");
			if (line.length() == 0 || line == "#") break;

			signed char x = line.readGChar();
			signed char y = line.readGChar();
			CString image = line.readString("#");
			CString code = line.readString("").replaceAll("\xa7", "\n");

			auto npc = server->addNPC(image, code, x, y, level, NPCStorageType::LEVEL, false);
			if (server->hasNPCServer())
			{
				auto indent2 = log::server.indent();
				log::print(log::server, "[{}] {}", npc->id, npc->name);
			}
		}
	}

	// Load chests.
	if (version > 0)
	{
		while (fileData.bytesLeft())
		{
			CString line = fileData.readString("\n");
			if (line.length() == 0 || line == "#") break;

			char x = line.readGChar();
			char y = line.readGChar();
			char item = line.readGChar();
			char signindex = line.readGChar();

			level->addChest(x, y, LevelItemType(item), signindex);
		}
	}

	// Load signs.
	{
		while (fileData.bytesLeft())
		{
			CString line = fileData.readString("\n");
			if (line.length() == 0) break;

			signed char x = line.readGChar();
			signed char y = line.readGChar();
			CString text = line.readString("");

			level->addSign(x, y, text, true);
		}
	}

	return level;
}

LevelPtr LevelLoader::loadNW(LevelPtr level, FileSystem* fileSystem, CString& fileData)
{
	// Load File
	std::vector<CString> fileLines = fileData.removeAllI("\r").tokenize("\n");
	if (fileLines.empty())
		return nullptr;

	auto* server = BabyDI::Get<Server>();

	std::unique_ptr<log::Indent> indent;
	if (server->hasNPCServer() && fileData.contains("NPCEND"))
	{
		indent = std::make_unique<log::Indent>(log::server.indent(log::server.indentLevel != 0 ? 1 : 0));
		log::printLine(log::server, "Loading NPCs for level '{}'...", level->getLevelName());
	}

	// Parse Level
	for (auto i = fileLines.begin(); i != fileLines.end(); ++i)
	{
		// Tokenize
		std::vector<CString> curLine = i->tokenize();
		if (curLine.empty())
			continue;

		// Parse Each Type
		if (curLine[0] == "BOARD")
		{
			if (curLine.size() != 6)
				continue;

			int x, y, w, layer;
			x = strtoint(curLine[1]);
			y = strtoint(curLine[2]);
			w = strtoint(curLine[3]);
			layer = strtoint(curLine[4]);

			if (!inrange(x, 0, 64) || !inrange(y, 0, 64) || w <= 0 || x + w > 64)
				continue;

			if (curLine[5].length() >= w * 2)
			{
				for (int ii = x; ii < x + w; ii++)
				{
					char left = curLine[5].readChar();
					char top = curLine[5].readChar();
					short tile = getBase64Position(left) << 6;
					tile += getBase64Position(top);
					level->m_tiles[layer][ii + static_cast<size_t>(y * 64)] = tile;
				}
			}
		}
		else if (curLine[0] == "CHEST")
		{
			if (curLine.size() != 5)
				continue;

			LevelItemType itemType = LevelItem::getItemId(curLine[3].toString());
			if (itemType != LevelItemType::INVALID)
			{
				char chestx = strtoint(curLine[1]);
				char chesty = strtoint(curLine[2]);
				char signidx = strtoint(curLine[4]);
				level->addChest(chestx, chesty, itemType, signidx);
			}
		}
		else if (curLine[0] == "LINK")
		{
			if (curLine.size() < 8)
				continue;

			// Get link string.
			std::vector<CString>::iterator i = curLine.begin();
			std::vector<CString> link(++i, curLine.end());

			// Find the whole level name.
			CString linkLevel(link[0]);
			if (link.size() > 7)
			{
				for (size_t i = 0; i < link.size() - 7; ++i)
					linkLevel << " " << link[i + 1];
			}

			if (fileSystem->find(linkLevel).isEmpty())
				continue;

			level->addLink(link);
		}
		else if (curLine[0] == "NPC")
		{
			unsigned int offset = 0;
			if (curLine.size() < 4)
				continue;

			std::unique_ptr<log::Profile> profile;
			if (server->hasNPCServer())
				profile = std::make_unique<log::Profile>(log::server, "", " ({1:0.6} ms)");

			// Grab the image properties.
			CString image(curLine[1]);
			if (curLine.size() > 4)
			{
				offset = (int)curLine.size() - 4;
				for (size_t i = 0; i < offset; ++i)
					image << " " << curLine[i + 2];
			}

			// If the image is just a hyphen, clear it.
			if (image == "-")
				image.clear();

			// Grab the NPC location.
			float x = (float)strtofloat(curLine[2 + offset]);
			float y = (float)strtofloat(curLine[3 + offset]);

			// Grab the NPC code.
			CString code;
			++i;
			while (i != fileLines.end())
			{
				if (*i == "NPCEND") break;
				code << *i << "\n";
				++i;
			}
			//printf( "image: %s, x: %.2f, y: %.2f, code: %s\n", image.text(), x, y, code.text() );
			// Add the new NPC.
			auto npc = server->addNPC(image, code, x, y, level, NPCStorageType::LEVEL, false);
			if (server->hasNPCServer())
			{
				auto indent2 = log::server.indent();
				log::print(log::server, "[{}] {}", npc->id, npc->name);
			}
		}
		else if (curLine[0] == "SIGN")
		{
			if (curLine.size() != 3)
				continue;

			int x = strtoint(curLine[1]);
			int y = strtoint(curLine[2]);

			// Grab the sign code.
			CString text;
			++i;
			while (i != fileLines.end())
			{
				if (*i == "SIGNEND") break;
				text << *i << "\n";
				++i;
			}

			// Add the new sign.
			level->addSign(x, y, text);
		}
		else if (curLine[0] == "BADDY")
		{
			if (curLine.size() != 4)
				continue;

			int x = strtoint(curLine[1]);
			int y = strtoint(curLine[2]);
			int type = strtoint(curLine[3]);

			// Add the baddy.
			LevelBaddy* baddy = level->addBaddy((float)x, (float)y, static_cast<BaddyType>(type));
			if (baddy == nullptr)
				continue;

			// Load the verses.
			std::vector<CString> bverse;
			++i;
			while (i != fileLines.end())
			{
				if (*i == "BADDYEND") break;
				bverse.push_back(*i);
				++i;
			}
			CString props;
			for (char j = 0; j < (char)bverse.size(); ++j)
				props >> (char)(PROPID(BaddyProp::VERSESIGHT) + j) >> (char)bverse[j].length() << bverse[j];
			if (props.length() != 0) baddy->setPropsFromPacket(props);
		}
		if (i == fileLines.end()) break;
	}

	return level;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
