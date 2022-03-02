#include <set>
#include <tiletypes.h>
#include <cmath>
#include "IDebug.h"
#include "IEnums.h"
#include "TServer.h"
#include "TLevel.h"
#include "TMap.h"
#include "TPlayer.h"
#include "TNPC.h"

/*
	Global Variables
*/
//CString base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
short respawningTiles[] = {
	0x1ff, 0x3ff, 0x2ac, 0x002, 0x200,
	0x022, 0x3de, 0x1a4, 0x14a, 0x674,
	0x72a,
};

int getBase64Position(char c)
{
	if (c >= 'a')
		return 26 + (c - 'a');
	else if (c >= 'A')
		return (c - 'A');
	else if (c >= '0' && c <= '9')
		return 52 + (c - '0');

	switch (c)
	{
		case '+': return 52 + 10;
		case '/': return 52 + 11;
	}

	return 0;
}

/*
	TLevel: Constructor - Deconstructor
*/
TLevel::TLevel(TServer* pServer)
:
server(pServer), modTime(0), levelSpar(false), levelMap(nullptr), mapx(0), mapy(0)
#ifdef V8NPCSERVER
, _scriptObject(nullptr)
#endif
{
	memset(levelTiles, 0, sizeof(levelTiles));

	// Baddy id 0 breaks the client.  Put a null pointer in id 0.
	levelBaddyIds.resize(1, 0);
}

TLevel::~TLevel()
{
	// Delete NPCs.
	{
		// Remove every NPC in the level.
		for (auto& levelNPC : levelNPCs)
		{
			// TODO(joey): we need to delete putnpc's, and move db-npcs to a different level
			if (levelNPC->getType() == NPCType::LEVELNPC)
				server->deleteNPC(levelNPC, false);
		}
		levelNPCs.clear();
	}

	// Delete baddies.
	for (auto& levelBaddie : levelBaddies) delete levelBaddie;
	levelBaddies.clear();
	levelBaddyIds.clear();

	// Delete chests.
	levelChests.clear();

	// Delete links.
	levelLinks.clear();

	// Delete signs.
	levelSigns.clear();

	// Delete items.
	for (auto& item : levelItems)
	{
		CString packet = CString() >> (char)PLO_ITEMDEL >> (char)(item.getX() * 2) >> (char)(item.getY() * 2);
		for (auto& player : levelPlayerList)
			player->sendPacket(packet);
	}
	levelItems.clear();

	// Delete board changes.
	levelBoardChanges.clear();

#ifdef V8NPCSERVER
	if (_scriptObject)
	{
		_scriptObject.reset();
	}
#endif
}

/*
	TLevel: Get Crafted Packets
*/
CString TLevel::getBaddyPacket(int clientVersion)
{
	CString retVal;
	for (const auto& baddy : levelBaddies)
	{
		assert(baddy != nullptr);
		if (baddy == nullptr)
			continue;

		//if (baddy->getProp(BDPROP_MODE).readGChar() != BDMODE_DIE)
		retVal >> (char)PLO_BADDYPROPS >> (char)baddy->getId() << baddy->getProps(clientVersion) << "\n";
	}
	return retVal;
}

CString TLevel::getBoardPacket()
{
	CString retVal;
	retVal.writeGChar(PLO_BOARDPACKET);
	retVal.write((char *)levelTiles, sizeof(levelTiles));
	retVal << "\n";
	return retVal;
}

CString TLevel::getBoardChangesPacket(time_t time)
{
	CString retVal;
	retVal >> (char)PLO_LEVELBOARD;
	for (const auto& change : levelBoardChanges)
	{
		if (change.getModTime() >= time)
			retVal << change.getBoardStr();
	}
	return retVal;
}

CString TLevel::getBoardChangesPacket2(time_t time)
{
	CString retVal;
	retVal >> (char)PLO_BOARDMODIFY;
	for (const auto& change : levelBoardChanges)
	{
		if (change.getModTime() >= time)
			retVal << change.getBoardStr();
	}
	return retVal;
}

CString TLevel::getChestPacket(TPlayer *pPlayer)
{
	CString retVal;

	if (pPlayer)
	{
		for (auto& chest : levelChests)
		{
			bool hasChest = pPlayer->hasChest(getChestStr(chest));

			retVal >> (char)PLO_LEVELCHEST >> (char)(hasChest ? 1 : 0) >> (char)chest.getX() >> (char)chest.getY();
			if (!hasChest) retVal >> (char)chest.getItemIndex() >> (char)chest.getSignIndex();
			retVal << "\n";
		}
	}

	return retVal;
}

CString TLevel::getHorsePacket()
{
	CString retVal;
	for (auto& horse : levelHorses)
	{
		retVal >> (char)PLO_HORSEADD << horse.getHorseStr() << "\n";
	}

	return retVal;
}

CString TLevel::getLinksPacket()
{
	CString retVal;
	for (const auto& link : levelLinks)
	{
		retVal >> (char)PLO_LEVELLINK << link.getLinkStr() << "\n";
	}

	return retVal;
}

CString TLevel::getNpcsPacket(time_t time, int clientVersion)
{
	CString retVal;
	for (auto& npc : levelNPCs)
	{
		retVal >> (char)PLO_NPCPROPS >> (int)npc->getId() << npc->getProps(time, clientVersion) << "\n";

		if (!npc->getByteCode().isEmpty())
		{
			CString byteCodePacket = CString() >> (char)PLO_NPCBYTECODE >> (int)npc->getId() << npc->getByteCode();
			if (byteCodePacket[byteCodePacket.length() - 1] != '\n')
				byteCodePacket << "\n";

			retVal >> (char)PLO_RAWDATA >> (int)byteCodePacket.length() << "\n";
			retVal << byteCodePacket;
		}
	}

	return retVal;
}

CString TLevel::getSignsPacket(TPlayer *pPlayer = 0)
{
	CString retVal;
	for (const auto & sign : levelSigns)
	{
		retVal >> (char)PLO_LEVELSIGN << sign.getSignStr(pPlayer) << "\n";
	}
	return retVal;
}

/*
	TLevel: Level-Loading Functions
*/
bool TLevel::reload()
{
	// Delete NPCs.
	// Don't delete NPCs if this level is on a gmap!  If we are on a gmap, just set them
	// back to their original positions.
	{
		// Remove every NPC in the level.
		for (auto it = levelNPCs.begin(); it != levelNPCs.end();)
		{
			TNPC *npc = *it;
			if (npc->getType() == NPCType::LEVELNPC)
			{
				server->deleteNPC(npc, false);
				it = levelNPCs.erase(it);
			}
			else
			{
#ifdef V8NPCSERVER
				npc->reloadNPC();
#endif
				it++;
			}
		}
	}

	// Delete baddies.
	for (auto levelBaddie : levelBaddies) delete levelBaddie;
	levelBaddies.clear();
	levelBaddyIds.clear();

	// Delete chests.
	levelChests.clear();

	// Delete links.
	levelLinks.clear();

	// Delete signs.
	levelSigns.clear();

	// Delete items.
	for (const auto& item : levelItems)
	{
		CString packet = CString() >> (char)PLO_ITEMDEL >> (char)(item.getX() * 2) >> (char)(item.getY() * 2);
		for (auto player : levelPlayerList)
			player->sendPacket(packet);
	}
	levelItems.clear();

	// Delete board changes.
	levelBoardChanges.clear();

	// Clean up the rest.
	levelSpar = false;

	// Remove all the players from the level.
	std::vector<TPlayer*> oldplayers = levelPlayerList;
	for (auto p : oldplayers)
	{
		p->leaveLevel(true);
	}

	// Reset the level cache for all the players on the server.
	auto playerList = server->getPlayerList();
	for (auto p : *playerList)
	{
		p->resetLevelCache(this);
	}

	// Re-load the level now.
	bool ret = loadLevel(levelName);

	// Warp all players back to the level (or to unstick me if loadLevel failed).
	CString uLevel = server->getSettings()->getStr("unstickmelevel", "onlinestartlocal.nw");
	float uX = server->getSettings()->getFloat("unstickmex", 30.0f);
	float uY = server->getSettings()->getFloat("unstickmey", 35.0f);
	for (auto p : oldplayers)
	{
		p->warp((ret ? levelName : uLevel), (ret ? p->getX() : uX), (ret ? p->getY() : uY));
	}

	return ret;
}

TLevel* TLevel::clone()
{
	TLevel *level = new TLevel(server);
	if (!level->loadLevel(levelName))
	{
		delete level;
		return nullptr;
	}
	return level;
}

bool TLevel::loadLevel(const CString& pLevelName)
{
#ifdef V8NPCSERVER
	server->getScriptEngine()->wrapScriptObject(this);
#endif

	CString ext(getExtension(pLevelName));
	if (ext == ".nw") return loadNW(pLevelName);
	else if (ext == ".graal") return loadGraal(pLevelName);
	else if (ext == ".zelda") return loadZelda(pLevelName);
	else return detectLevelType(pLevelName);
}

bool TLevel::detectLevelType(const CString& pLevelName)
{
	// Get the appropriate filesystem.
	CFileSystem* fileSystem = server->getFileSystem();
	if (!server->getSettings()->getBool("nofoldersconfig", false))
		fileSystem = server->getFileSystem(FS_LEVEL);

	// Load file
	CString fileData;
	if (!fileData.load(fileSystem->find(pLevelName)))
		return false;

	// Grab file version.
	fileVersion = fileData.readChars(8);

	// Determine the level type.
	int v = -1;
	if (fileVersion == "GLEVNW01") v = 0;
	else if (fileVersion == "GR-V1.03" || fileVersion == "GR-V1.02" || fileVersion == "GR-V1.01") v = 1;
	else if (fileVersion == "Z3-V1.04" || fileVersion == "Z3-V1.03") v = 2;

	// Not a level.
	if (v == -1) return false;

	// Load the correct level.
	if (v == 0) return loadNW(pLevelName);
	if (v == 1) return loadGraal(pLevelName);
	if (v == 2) return loadZelda(pLevelName);
	return false;
}

bool TLevel::loadZelda(const CString& pLevelName)
{
	// Get the appropriate filesystem.
	CFileSystem* fileSystem = server->getFileSystem();
	if (!server->getSettings()->getBool("nofoldersconfig", false))
		fileSystem = server->getFileSystem(FS_LEVEL);

	// Path-To-File
	actualLevelName = levelName = pLevelName;
	fileName = fileSystem->find(pLevelName);
	modTime = fileSystem->getModTime(pLevelName);

	// Load file
	CString fileData;
	if (!fileData.load(fileName)) return false;

	// Grab file version.
	fileVersion = fileData.readChars(8);

	// Check if it is actually a .graal level.  The 1.39-1.41r1 client actually
	// saved .zelda as .graal.
	if (fileVersion.subString(0, 2) == "GR")
		return loadGraal(pLevelName);

	int v = -1;
	if (fileVersion == "Z3-V1.03") v = 3;
	else if (fileVersion == "Z3-V1.04") v = 4;
	if (v == -1) return false;

	// Load tiles.
	{
		int bits = (v > 4 ? 13 : 12);
		int read = 0;
		unsigned int buffer = 0;
		unsigned short code = 0;
		short tiles[2] = {-1,-1};
		int boardIndex = 0;
		int count = 1;
		bool doubleMode = false;

		// Read the tiles.
		while (boardIndex < 64*64 && fileData.bytesLeft() != 0)
		{
			// Every control code/tile is either 12 or 13 bits.  WTF.
			// Read in the bits.
			while (read < bits)
			{
				buffer += ((unsigned char)fileData.readChar()) << read;
				read += 8;
			}

			// Pull out a single 12/13 bit code from the buffer.
			code = buffer & (bits == 12 ? 0xFFF : 0x1FFF);
			buffer >>= bits;
			read -= bits;

			// See if we have an RLE control code.
			// Control codes determine how the RLE scheme works.
			if (code & (bits == 12 ? 0x800 : 0x1000))
			{
				// If the 0x100 bit is set, we are in a double repeat mode.
				// {double 4}56 = 56565656
				if (code & 0x100) doubleMode = true;

				// How many tiles do we count?
				count = code & 0xFF;
				continue;
			}

			// If our count is 1, just read in a tile.  This is the default mode.
			if (count == 1)
			{
				levelTiles[boardIndex++] = (short)code;
				continue;
			}

			// If we reach here, we have an RLE scheme.
			// See if we are in double repeat mode or not.
			if (doubleMode)
			{
				// Read in our first tile.
				if (tiles[0] == -1)
				{
					tiles[0] = (short)code;
					continue;
				}

				// Read in our second tile.
				tiles[1] = (short)code;

				// Add the tiles now.
				for (int i = 0; i < count && boardIndex < 64*64-1; ++i)
				{
					levelTiles[boardIndex++] = tiles[0];
					levelTiles[boardIndex++] = tiles[1];
				}

				// Clean up.
				tiles[0] = tiles[1] = -1;
				doubleMode = false;
				count = 1;
			}
			// Regular RLE scheme.
			else
			{
				for (int i = 0; i < count && boardIndex < 64*64; ++i)
					levelTiles[boardIndex++] = (short)code;
				count = 1;
			}
		}
	}

	// Load the links.
	{
		while (fileData.bytesLeft())
		{
			CString line = fileData.readString("\n");
			if (line.length() == 0 || line == "#") break;

			// Assemble the level string.
			std::vector<CString> vline = line.tokenize();
			CString level = vline[0];
			if (vline.size() > 7)
			{
				for (unsigned int i = 0; i < vline.size() - 7; ++i)
					level << " " << vline[1 + i];
			}

			if (fileSystem->find(level).isEmpty())
				continue;

			levelLinks.emplace_back(vline);
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
				fileData.readString("\n");	// Empty verses.
				break;
			}

			// Add the baddy.
			TLevelBaddy* baddy = addBaddy((float)x, (float)y, type);
			if (baddy == nullptr)
				continue;

			// Only v1.04+ baddies have verses.
			if (v > 3)
			{
				// Load the verses.
				std::vector<CString> bverse = fileData.readString("\n").tokenize("\\");
				CString props;
				for (char j = 0; j < (char)bverse.size(); ++j)
					props >> (char)(BDPROP_VERSESIGHT + j) >> (char)bverse[j].length() << bverse[j];
				if (props.length() != 0) baddy->setProps(props);
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

			levelSigns.push_back(TLevelSign(x, y, text, true));
		}
	}

	return true;
}

bool TLevel::loadGraal(const CString& pLevelName)
{
	// Get the appropriate filesystem.
	CFileSystem* fileSystem = server->getFileSystem();
	if (!server->getSettings()->getBool("nofoldersconfig", false))
		fileSystem = server->getFileSystem(FS_LEVEL);

	// Path-To-File
	actualLevelName = levelName = pLevelName;
	fileName = fileSystem->find(pLevelName);
	modTime = fileSystem->getModTime(pLevelName);

	// Load file
	CString fileData;
	if (!fileData.load(fileName)) return false;

	// Grab file version.
	fileVersion = fileData.readChars(8);
	int v = -1;
	if (fileVersion == "GR-V1.00") v = 0;
	else if (fileVersion == "GR-V1.01") v = 1;
	else if (fileVersion == "GR-V1.02") v = 2;
	else if (fileVersion == "GR-V1.03") v = 3;
	if (v == -1) return false;

	// Load tiles.
	{
		int bits = (v > 0 ? 13 : 12);
		int read = 0;
		unsigned int buffer = 0;
		unsigned short code = 0;
		short tiles[2] = {-1,-1};
		int boardIndex = 0;
		int count = 1;
		bool doubleMode = false;

		// Read the tiles.
		while (boardIndex < 64*64 && fileData.bytesLeft() != 0)
		{
			// Every control code/tile is either 12 or 13 bits.  WTF.
			// Read in the bits.
			while (read < bits)
			{
				buffer += ((unsigned char)fileData.readChar()) << read;
				read += 8;
			}

			// Pull out a single 12/13 bit code from the buffer.
			code = buffer & (bits == 12 ? 0xFFF : 0x1FFF);
			buffer >>= bits;
			read -= bits;

			// See if we have an RLE control code.
			// Control codes determine how the RLE scheme works.
			if (code & (bits == 12 ? 0x800 : 0x1000))
			{
				// If the 0x100 bit is set, we are in a double repeat mode.
				// {double 4}56 = 56565656
				if (code & 0x100) doubleMode = true;

				// How many tiles do we count?
				count = code & 0xFF;
				continue;
			}

			// If our count is 1, just read in a tile.  This is the default mode.
			if (count == 1)
			{
				levelTiles[boardIndex++] = (short)code;
				continue;
			}

			// If we reach here, we have an RLE scheme.
			// See if we are in double repeat mode or not.
			if (doubleMode)
			{
				// Read in our first tile.
				if (tiles[0] == -1)
				{
					tiles[0] = (short)code;
					continue;
				}

				// Read in our second tile.
				tiles[1] = (short)code;

				// Add the tiles now.
				for (int i = 0; i < count && boardIndex < 64*64-1; ++i)
				{
					levelTiles[boardIndex++] = tiles[0];
					levelTiles[boardIndex++] = tiles[1];
				}

				// Clean up.
				tiles[0] = tiles[1] = -1;
				doubleMode = false;
				count = 1;
			}
			// Regular RLE scheme.
			else
			{
				for (int i = 0; i < count && boardIndex < 64*64; ++i)
					levelTiles[boardIndex++] = (short)code;
				count = 1;
			}
		}
	}

	// Load the links.
	{
		while (fileData.bytesLeft())
		{
			CString line = fileData.readString("\n");
			if (line.length() == 0 || line == "#") break;

			// Assemble the level string.
			std::vector<CString> vline = line.tokenize();
			CString level = vline[0];
			if (vline.size() > 7)
			{
				for (unsigned int i = 0; i < vline.size() - 7; ++i)
					level << " " << vline[1 + i];
			}

			if (fileSystem->find(level).isEmpty())
				continue;

			levelLinks.push_back(TLevelLink(vline));
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
				fileData.readString("\n");	// Empty verses.
				break;
			}

			// Add the baddy.
			TLevelBaddy* baddy = addBaddy((float)x, (float)y, type);
			if (baddy == nullptr)
				continue;

			// Load the verses.
			std::vector<CString> bverse = fileData.readString("\n").tokenize("\\");
			CString props;
			for (char j = 0; j < (char)bverse.size(); ++j)
				props >> (char)(BDPROP_VERSESIGHT + j) >> (char)bverse[j].length() << bverse[j];
			if (props.length() != 0) baddy->setProps(props);
		}
	}

	// Load NPCs.
	{
		while (fileData.bytesLeft())
		{
			CString line = fileData.readString("\n");
			if (line.length() == 0 || line == "#") break;

			signed char x = line.readGChar();
			signed char y = line.readGChar();
			CString image = line.readString("#");
			CString code = line.readString("").replaceAll("\xa7", "\n");

			TNPC* npc = server->addNPC(image, code, x, y, this, true, false);
			levelNPCs.push_back(npc);
		}
	}

	// Load chests.
	if (v > 0)
	{
		while (fileData.bytesLeft())
		{
			CString line = fileData.readString("\n");
			if (line.length() == 0 || line == "#") break;

			char x = line.readGChar();
			char y = line.readGChar();
			char item = line.readGChar();
			char signindex = line.readGChar();

			levelChests.push_back(TLevelChest(x, y, LevelItemType(item), signindex));
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

			levelSigns.push_back(TLevelSign(x, y, text, true));
		}
	}

	return true;
}

bool TLevel::loadNW(const CString& pLevelName)
{
	// Get the appropriate filesystem.
	CFileSystem* fileSystem = server->getFileSystem();
	if (!server->getSettings()->getBool("nofoldersconfig", false))
		fileSystem = server->getFileSystem(FS_LEVEL);

	// Path-To-File
	actualLevelName = levelName = pLevelName;
	fileName = fileSystem->find(pLevelName);
	modTime = fileSystem->getModTime(pLevelName);

	// Load File
	std::vector<CString> fileData = CString::loadToken(fileName, "\n", true);
	if (fileData.empty())
		return false;

	// Grab File Version
	fileVersion = fileData[0];

	// Parse Level
	for (auto i = fileData.begin(); i != fileData.end(); ++i)
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

			if (curLine[5].length() >= w*2)
			{
				for(int ii = x; ii < x + w; ii++)
				{
					char left = curLine[5].readChar();
					char top = curLine[5].readChar();
					short tile = getBase64Position(left) << 6;
					tile += getBase64Position(top);
					levelTiles[ii + y*64] = tile;
				}
			}
		}
		else if (curLine[0] == "CHEST")
		{
			if (curLine.size() != 5)
				continue;

			LevelItemType itemType = TLevelItem::getItemId(curLine[3]);
			if (itemType != LevelItemType::INVALID) {
				char chestx = strtoint(curLine[1]);
				char chesty = strtoint(curLine[2]);
				char signidx = strtoint(curLine[4]);
				levelChests.push_back(TLevelChest(chestx, chesty, itemType, signidx));
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
			CString level(link[0]);
			if (link.size() > 7)
			{
				for (auto i = 0; i < link.size() - 7; ++i)
					level << " " << link[i + 1];
			}

			if (fileSystem->find(level).isEmpty())
				continue;

			levelLinks.push_back(TLevelLink(link));
		}
		else if (curLine[0] == "NPC")
		{
			unsigned int offset = 0;
			if (curLine.size() < 4)
				continue;

			// Grab the image properties.
			CString image(curLine[1]);
			if (curLine.size() > 4)
			{
				offset = (int)curLine.size() - 4;
				for (unsigned int i = 0; i < offset; ++i)
					image << " " << curLine[i + 2];
			}

			// Grab the NPC location.
			float x = (float)strtofloat(curLine[2 + offset]);
			float y = (float)strtofloat(curLine[3 + offset]);

			// Grab the NPC code.
			CString code;
			++i;
			while (i != fileData.end())
			{
				if (*i == "NPCEND") break;
				code << *i << "\n";
				++i;
			}
			//printf( "image: %s, x: %.2f, y: %.2f, code: %s\n", image.text(), x, y, code.text() );
			// Add the new NPC.
			TNPC* npc = server->addNPC(image, code, x, y, this, true, false);
			levelNPCs.push_back(npc);
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
			while (i != fileData.end())
			{
				if (*i == "SIGNEND") break;
				text << *i << "\n";
				++i;
			}

			// Add the new sign.
			levelSigns.push_back(TLevelSign(x, y, text));
		}
		else if (curLine[0] == "BADDY")
		{
			if (curLine.size() != 4)
				continue;

			int x = strtoint(curLine[1]);
			int y = strtoint(curLine[2]);
			int type = strtoint(curLine[3]);

			// Add the baddy.
			TLevelBaddy* baddy = addBaddy((float)x, (float)y, type);
			if (baddy == 0)
				continue;

			// Load the verses.
			std::vector<CString> bverse;
			++i;
			while (i != fileData.end())
			{
				if (*i == "BADDYEND") break;
				bverse.push_back(*i);
				++i;
			}
			CString props;
			for (char j = 0; j < (char)bverse.size(); ++j)
				props >> (char)(BDPROP_VERSESIGHT + j) >> (char)bverse[j].length() << bverse[j];
			if (props.length() != 0) baddy->setProps(props);
		}
		if (i == fileData.end()) break;
	}

	return true;
}

/*
	TLevel: Find Level
*/
TLevel* TLevel::findLevel(const CString& pLevelName, TServer* server)
{
	std::vector<TLevel*>* levelList = server->getLevelList();

	// TODO(joey): Maybe its time for a hashmap, even if a duplicate level name occurs
	// 	this is still going to break on the first occurence.

	// Find Appropriate Level by Name
	CString levelName = pLevelName.toLower();
	for (auto & it : *levelList)
	{
		if (it->getLevelName().toLower() == levelName)
			return it;
	}

	// Load New Level
	TLevel *level = new TLevel(server);
	if (!level->loadLevel(pLevelName))
	{
		delete level;
		return nullptr;
	}

	auto& mapList = server->getMapList();
	for (const auto& map : mapList)
	{
		int mx, my;
		if (map->isLevelOnMap(levelName.text(), mx, my))
		{
			level->setMap(map.get(), mx, my);
			break;
		}
	}

	// Return Level
	levelList->push_back(level);
	return level;
}

bool TLevel::alterBoard(CString& pTileData, int pX, int pY, int pWidth, int pHeight, TPlayer* player)
{
	if (pX < 0 || pY < 0 || pX > 63 || pY > 63 ||
		pWidth < 1 || pHeight < 1 ||
		pX + pWidth > 64 || pY + pHeight > 64)
		return false;

	CSettings* settings = server->getSettings();

	// Do the check for the push-pull block.
	if (pWidth == 4 && pHeight == 4 && settings->getBool("clientsidepushpull", true))
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
	for (auto i = levelBoardChanges.begin(); i != levelBoardChanges.end(); )
	{
		TLevelBoardChange& change = *i;
		if ((change.getX() >= pX && change.getX() + change.getWidth() <= pX + pWidth) &&
			(change.getY() >= pY && change.getY() + change.getHeight() <= pY + pHeight))
		{
			i = levelBoardChanges.erase(i);
		}
		else ++i;
	}

	// Check if the tiles should be respawned.
	// Only tiles in the respawningTiles array are allowed to respawn.
	// These are things like signs, bushes, pots, etc.
	int respawnTime = settings->getInt("respawntime", 15);
	bool doRespawn = false;
	short testTile = levelTiles[pX + (pY * 64)];
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
				oldTiles.writeGShort(levelTiles[i + (j * 64)]);
		}
	}

	// TODO: old gserver didn't save the board change if oldTiles.length() == 0.
	// Should we do it that way still?
	levelBoardChanges.push_back(TLevelBoardChange(pX, pY, pWidth, pHeight, pTileData, oldTiles, (doRespawn ? respawnTime : -1)));
	return true;
}

bool TLevel::addItem(float pX, float pY, LevelItemType pItem)
{
#ifdef V8NPCSERVER
#ifdef GRALATNPC
	if (TLevelItem::isRupeeType(pItem))
	{
		TNPC* gralatNPC = nullptr;

		// Find existing rupees, and add to the npc
		auto npcList = findAreaNpcs(pX - 0.5, pY - 0.5, 32, 32);
		for (auto& npc : npcList)
		{
			if (npc->joinedClass("gralats"))
			{
				gralatNPC = npc;
				break;
			}
		}

		// Create a new gralat npc for these rupees
		if (!gralatNPC)
		{
			gralatNPC = server->addNPC("", "npc.join(\"gralats\");", pX, pY, this, false, true);
			gralatNPC->setScriptType("LOCALN");
			addNPC(gralatNPC);
		}

		// Update rupees
		gralatNPC->setRupees(gralatNPC->getRupees() + TLevelItem::GetRupeeCount(pItem));
		gralatNPC->updatePropModTime(NPCPROP_RUPEES);
		gralatNPC->queueNpcTrigger("update", nullptr, "");

		return false;
	}
#endif
#endif

	levelItems.push_back(TLevelItem(pX, pY, pItem));
	return true;
}

LevelItemType TLevel::removeItem(float pX, float pY)
{
	for (auto i = levelItems.begin(); i != levelItems.end(); ++i)
	{
		TLevelItem& item = *i;
		if (item.getX() == pX && item.getY() == pY)
		{
			LevelItemType itemType = item.getItem();
			levelItems.erase(i);
			return itemType;
		}
	}

	return LevelItemType::INVALID;
}

bool TLevel::addHorse(CString& pImage, float pX, float pY, char pDir, char pBushes)
{
	auto horseLife = server->getSettings()->getInt("horselifetime", 30);
	levelHorses.push_back(TLevelHorse(horseLife, pImage, pX, pY, pDir, pBushes));
	return true;
}

void TLevel::removeHorse(float pX, float pY)
{
	for (auto it = levelHorses.begin(); it != levelHorses.end(); ++it)
	{
		TLevelHorse& horse = *it;
		if (horse.getX() == pX && horse.getY() == pY)
		{
			levelHorses.erase(it);
			return;
		}
	}
}

TLevelBaddy* TLevel::addBaddy(float pX, float pY, char pType)
{
	// Limit of 50 baddies per level.
	if (levelBaddies.size() > 50) return nullptr;

	// New Baddy
	TLevelBaddy* newBaddy = new TLevelBaddy(pX, pY, pType, this, server);
	levelBaddies.push_back(newBaddy);

	// Assign Baddy Id
	// Don't assign id 0.
	for (unsigned int i = 1; i < levelBaddyIds.size(); ++i)
	{
		if (levelBaddyIds[i] == nullptr)
		{
			levelBaddyIds[i] = newBaddy;
			newBaddy->setId((char)i);
			return newBaddy;
		}
	}

	newBaddy->setId((char)levelBaddyIds.size());
	levelBaddyIds.push_back(newBaddy);

	return newBaddy;
}

void TLevel::removeBaddy(char pId)
{
	// Don't allow us to remove id 0 or any id over 50.
	if (pId < 1 || pId > 50) return;

	// Find the baddy.
	TLevelBaddy* baddy = levelBaddyIds[pId];
	if (baddy == nullptr) return;

	// Erase the baddy.
	if (!levelBaddies.empty())
	{
		for (auto i = levelBaddies.begin(); i != levelBaddies.end();)
		{
			TLevelBaddy* b = *i;
			if (b == baddy)
				i = levelBaddies.erase(i);
			else ++i;
		}
	}
	//vecRemove(levelBaddies, baddy);
	levelBaddyIds[pId] = nullptr;

	// Clean up.
	delete baddy;
}

TLevelBaddy* TLevel::getBaddy(char id)
{
	if ((unsigned char)id >= levelBaddyIds.size()) return nullptr;
	return levelBaddyIds[id];
}

int TLevel::addPlayer(TPlayer* player)
{
	levelPlayerList.push_back(player);

#ifdef V8NPCSERVER
	for (auto& npc : levelNPCs)
	{
		if (npc->hasScriptEvent(NPCEVENTFLAG_PLAYERENTERS))
		{
			npc->queueNpcAction("npc.playerenters", player);
		}
	}
#endif

	return levelPlayerList.size() - 1;
}

void TLevel::removePlayer(TPlayer* player)
{
	for (auto it = levelPlayerList.begin(); it != levelPlayerList.end(); )
	{
		TPlayer* search = *it;
		if (player == search)
			it = levelPlayerList.erase(it);
		else ++it;
	}

#ifdef V8NPCSERVER
	for (auto& npc : levelNPCs)
	{
		if (npc->hasScriptEvent(NPCEVENTFLAG_PLAYERLEAVES))
		{
			npc->queueNpcAction("npc.playerleaves", player);
		}
	}
#endif
}

TPlayer* TLevel::getPlayer(unsigned int id)
{
	if (id >= levelPlayerList.size())
	{
		return nullptr;
	}

	return levelPlayerList[id];
}

bool TLevel::addNPC(TNPC* npc)
{
	if (std::find(levelNPCs.begin(), levelNPCs.end(), npc) != levelNPCs.end())
	{
		return false;
	}

	levelNPCs.push_back(npc);
	return true;
}

void TLevel::removeNPC(TNPC* npc)
{
	for (auto i = levelNPCs.begin(); i != levelNPCs.end(); )
	{
		TNPC* search = *i;
		if (npc == search)
			i = levelNPCs.erase(i);
		else ++i;
	}
}

void TLevel::setMap(TMap* pMap, int pMapX, int pMapY)
{
	levelMap = pMap;
	mapx = pMapX;
	mapy = pMapY;
}

bool TLevel::doTimedEvents()
{
	// Check if we should revert any board changes.
	for (auto& change : levelBoardChanges)
	{
		int respawnTimer = change.timeout.doTimeout();
		if (respawnTimer == 0)
		{
			// Put the old data back in.  DON'T DELETE THE CHANGE.
			// The client remembers board changes and if we delete the
			// change, the client won't get the new data.
			change.swapTiles();
			change.setModTime(time(0));
			server->sendPacketToLevel(CString() >> (char)PLO_BOARDMODIFY << change.getBoardStr(), 0, this);
		}
	}

	// Check if any items have timed out.
	// This allows us to delete items that have disappeared if nobody is in the level to send
	// the PLI_ITEMDEL packet.
	for (auto i = levelItems.begin(); i != levelItems.end(); )
	{
		TLevelItem& item = *i;
		int deleteTimer = item.timeout.doTimeout();
		if (deleteTimer == 0)
		{
			i = levelItems.erase(i);
		}
		else ++i;
	}

	// Check if any horses need to be deleted.
	for (auto i = levelHorses.begin(); i != levelHorses.end(); )
	{
		TLevelHorse& horse = *i;
		int deleteTimer = horse.timeout.doTimeout();
		if (deleteTimer == 0)
		{
			server->sendPacketToLevel(CString() >> (char)PLO_HORSEDEL >> (char)(horse.getX() * 2) >> (char)(horse.getY() * 2), 0, this);
			i = levelHorses.erase(i);
		}
		else ++i;
	}

	// Check if any baddies need to be marked as dead or respawned.
	std::set<TLevelBaddy*> set_dead;
	for (auto i = levelBaddies.begin(); i != levelBaddies.end(); )
	{
		TLevelBaddy* baddy = *i;
		if (baddy == nullptr)
		{
			i = levelBaddies.erase(i);
			continue;
		}
		++i;

		// See if we can respawn him.
		int respawnTimer = baddy->timeout.doTimeout();
		if (respawnTimer == 0)
		{
			if (baddy->getType() == 4 /*swamp arrow baddy*/ && baddy->getMode() == BDMODE_HURT)
			{
				if (baddy->getPower() == 1)
				{
					// Unset the hurt mode on the baddy.
					CString props = CString() >> (char)BDPROP_MODE >> (char)BDMODE_SWAMPSHOT;
					baddy->setProps(props);
					for (unsigned int i = 1; i < levelPlayerList.size(); ++i)
						levelPlayerList[i]->sendPacket(CString() >> (char)PLO_BADDYPROPS >> (char)baddy->getId() << props);
				}
			}
			else if (baddy->getMode() == BDMODE_DIE)
			{
				// Setting the baddy props could delete the baddy and invalidate our iterator.
				// So, save a list of all the baddies we are setting as dead and do it after this loop.
				set_dead.insert(baddy);

				// Set the baddy as dead for all the other players in the level.
				CString props = CString() >> (char)BDPROP_MODE >> (char)BDMODE_DEAD;
				for (unsigned int i = 1; i < levelPlayerList.size(); ++i)
					levelPlayerList[i]->sendPacket(CString() >> (char)PLO_BADDYPROPS >> (char)baddy->getId() << props);
			}
			else
			{
				baddy->reset();
				for (auto p : levelPlayerList)
				{
					p->sendPacket(CString() >> (char)PLO_BADDYPROPS >> (char)baddy->getId() << baddy->getProps(p->getVersion()));
				}
			}
		}
	}
	{	// Mark all the baddies as dead now.
		CString props = CString() >> (char)BDPROP_MODE >> (char)BDMODE_DEAD;
		for (auto baddy : set_dead)
		{
			baddy->setProps(props);
		}
	}

	return true;
}

bool TLevel::isOnWall(int pX, int pY) const
{
	if (pX < 0 || pY < 0 || pX > 63 || pY > 63)
	{
		return true;
	}

	return tiletypes[levelTiles[pY * 64 + pX]] >= 20;
}

bool TLevel::isOnWall2(int pX, int pY, int pWidth, int pHeight, uint8_t flags) const
{
	for (int cy = pY; cy < pY + pHeight; ++cy)
	{
		for (int cx = pX; cx < pX + pWidth; ++cx)
		{
			if (isOnWall(cx, cy))
			{
				return true;
			}
		}
	}

	return false;
}

bool TLevel::isOnWater(int pX, int pY) const
{
	return (tiletypes[levelTiles[pY * 64 + pX]] == 11);
}

std::optional<TLevelLink> TLevel::getLink(int pX, int pY) const
{
	for (const auto& link : levelLinks)
	{
		if ((pX >= link.getX() && pX <= link.getX() + link.getWidth()) &&
			(pY >= link.getY() && pY <= link.getY() + link.getHeight()))
		{
			return std::make_optional(link);
		}
	}

	return std::nullopt;
}

std::optional<TLevelChest> TLevel::getChest(int x, int y) const
{
	for (const auto& chest : levelChests)
	{
		if (chest.getX() == x && chest.getY() == y)
		{
			return std::make_optional(chest);
		}
	}

	return std::nullopt;
}

CString TLevel::getChestStr(const TLevelChest& chest) const
{
	static char retVal[500];
	sprintf(retVal, "%i:%i:%s", chest.getX(), chest.getY(), levelName.text());
	return retVal;
}

#ifdef V8NPCSERVER

std::vector<TNPC*> TLevel::findAreaNpcs(int pX, int pY, int pWidth, int pHeight)
{
	int testEndX = pX + pWidth;
	int testEndY = pY + pHeight;

	std::vector<TNPC *> npcList;
	for (const auto& npc : levelNPCs)
	{
		if (pX < npc->getX() + npc->getWidth() && testEndX > npc->getX() &&
			pY < npc->getY() + npc->getHeight() && testEndY > npc->getY())
		{
			npcList.push_back(npc);
		}
	}

	return npcList;
}

std::vector<TNPC*> TLevel::testTouch(int pX, int pY)
{
	std::vector<TNPC*> npcList;
	for (const auto& npc : levelNPCs)
	{
		if (npc->hasScriptEvent(NPCEVENTFLAG_PLAYERTOUCHSME) && (npc->getVisibleFlags() & NPCVISFLAG_VISIBLE) != 0)
		{
			if (npc->getX() <= pX && npc->getX() + npc->getWidth() >= pX &&
				npc->getY() <= pY && npc->getY() + npc->getHeight() >= pY)
			{
				npcList.push_back(npc);
			}
		}
	}

	return npcList;
}

TNPC * TLevel::isOnNPC(float pX, float pY, bool checkEventFlag)
{
	for (const auto& npc : levelNPCs)
	{
		if (checkEventFlag && !npc->hasScriptEvent(NPCEVENTFLAG_PLAYERTOUCHSME))
			continue;

		//if (!npc->getImage().isEmpty())
		{
			if ((npc->getVisibleFlags() & 1) != 0)
			{
				if ((pX >= npc->getX() && pX <= npc->getX() + (float)(npc->getWidth() / 16.0f)) &&
					(pY >= npc->getY() && pY <= npc->getY() + (float)(npc->getHeight() / 16.0f)))
				{
					// what if it touches multiple npcs? hm. not sure how graal did it.
					return npc;
				}
			}
		}
	}

	return nullptr;
}

void TLevel::sendChatToLevel(const TPlayer *player, const std::string& message)
{
	for (const auto& npc : levelNPCs)
	{
		if (npc->hasScriptEvent(NPCEVENTFLAG_PLAYERCHATS))
			npc->queueNpcEvent("npc.playerchats", true, player->getScriptObject(), message);
	}
}

#endif
