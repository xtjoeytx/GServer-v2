#include <set>
#include "IDebug.h"
#include "IEnums.h"
#include "TServer.h"
#include "TLevel.h"
#include "TPlayer.h"
#include "TNPC.h"

/*
	Global Variables
*/
CString base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
short respawningTiles[] = {
	0x1ff, 0x3ff, 0x2ac, 0x002, 0x200,
	0x022, 0x3de, 0x1a4, 0x14a, 0x674,
	0x72a,
};

/*
	TLevel: Constructor - Deconstructor
*/
TLevel::TLevel(TServer* pServer)
:
server(pServer), modTime(0), levelSpar(false), levelSingleplayer(false)
#ifdef V8NPCSERVER
, _scriptObject(0)
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
		for (std::vector<TNPC*>::iterator i = levelNPCs.begin(); i != levelNPCs.end(); ++i)
			server->deleteNPC(*i, this, false);
		levelNPCs.clear();
	}

	// Delete baddies.
	for (std::vector<TLevelBaddy*>::iterator i = levelBaddies.begin(); i != levelBaddies.end(); ++i) delete *i;
	levelBaddies.clear();
	levelBaddyIds.clear();

	// Delete chests.
	for (std::vector<TLevelChest*>::iterator i = levelChests.begin(); i != levelChests.end(); ++i) delete *i;
	levelChests.clear();

	// Delete links.
	for (std::vector<TLevelLink*>::iterator i = levelLinks.begin(); i != levelLinks.end(); ++i) delete *i;
	levelLinks.clear();

	// Delete signs.
	for (std::vector<TLevelSign*>::iterator i = levelSigns.begin(); i != levelSigns.end(); ++i) delete *i;
	levelSigns.clear();

	// Delete items.
	for (std::vector<TLevelItem*>::iterator i = levelItems.begin(); i != levelItems.end(); ++i)
	{
		TLevelItem* item = *i;
		CString packet = CString() >> (char)PLO_ITEMDEL >> (char)(item->getX() * 2) >> (char)(item->getY() * 2);
		for (std::vector<TPlayer*>::iterator j = levelPlayerList.begin(); j != levelPlayerList.end(); ++j) (*j)->sendPacket(packet);
		delete item;
	}
	levelItems.clear();

	// Delete board changes.
	for (std::vector<TLevelBoardChange*>::iterator i = levelBoardChanges.begin(); i != levelBoardChanges.end(); ++i) delete *i;
	levelBoardChanges.clear();

#ifdef V8NPCSERVER
	if (_scriptObject)
		delete _scriptObject;
#endif
}

/*
	TLevel: Get Crafted Packets
*/
CString TLevel::getBaddyPacket(int clientVersion)
{
	CString retVal;
	for (std::vector<TLevelBaddy *>::iterator i = levelBaddies.begin(); i != levelBaddies.end(); ++i)
	{
		TLevelBaddy* baddy = *i;
		if (baddy == 0) continue;
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
	for (std::vector<TLevelBoardChange*>::const_iterator i = levelBoardChanges.begin(); i != levelBoardChanges.end(); ++i)
	{
		TLevelBoardChange* change = *i;
		if (change->getModTime() >= time)
			retVal << change->getBoardStr();
	}
	return retVal;
}

CString TLevel::getBoardChangesPacket2(time_t time)
{
	CString retVal;
	retVal >> (char)PLO_BOARDMODIFY;
	for (std::vector<TLevelBoardChange*>::const_iterator i = levelBoardChanges.begin(); i != levelBoardChanges.end(); ++i)
	{
		TLevelBoardChange* change = *i;
		if (change->getModTime() >= time)
			retVal << change->getBoardStr();
	}
	return retVal;
}

CString TLevel::getChestPacket(TPlayer *pPlayer)
{
	if (pPlayer == 0)
		return CString();

	CString retVal;

	for (std::vector<TLevelChest *>::iterator i = levelChests.begin(); i != levelChests.end(); ++i)
	{
		TLevelChest *chest = *i;
		bool hasChest = pPlayer->hasChest(chest);

		retVal >> (char)PLO_LEVELCHEST >> (char)(hasChest ? 1 : 0) >> (char)chest->getX() >> (char)chest->getY();
		if (!hasChest) retVal >> (char)chest->getItemIndex() >> (char)chest->getSignIndex();
		retVal << "\n";
	}

	return retVal;
}

CString TLevel::getHorsePacket()
{
	CString retVal;
	for (std::vector<TLevelHorse *>::iterator i = levelHorses.begin(); i != levelHorses.end(); ++i)
	{
		TLevelHorse *horse = *i;
		retVal >> (char)PLO_HORSEADD << horse->getHorseStr() << "\n";
	}

	return retVal;
}

CString TLevel::getLinksPacket()
{
	CString retVal;
	for (std::vector<TLevelLink *>::iterator i = levelLinks.begin(); i != levelLinks.end(); ++i)
	{
		TLevelLink *link = *i;
		retVal >> (char)PLO_LEVELLINK << link->getLinkStr() << "\n";
	}

	return retVal;
}

CString TLevel::getNpcsPacket(time_t time, int clientVersion)
{
	CString retVal;
	for (std::vector<TNPC *>::iterator i = levelNPCs.begin(); i != levelNPCs.end(); ++i)
	{
		TNPC* npc = *i;
		retVal >> (char)PLO_NPCPROPS >> (int)npc->getId() << npc->getProps(time, clientVersion) << "\n";
	}
	return retVal;
}

CString TLevel::getSignsPacket(TPlayer *pPlayer = 0)
{
	CString retVal;
	for (std::vector<TLevelSign*>::const_iterator i = levelSigns.begin(); i != levelSigns.end(); ++i)
	{
		TLevelSign* sign = *i;
		retVal >> (char)PLO_LEVELSIGN << sign->getSignStr(pPlayer) << "\n";
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
		for (std::vector<TNPC*>::iterator i = levelNPCs.begin(); i != levelNPCs.end();)
		{
			TNPC* n = *i;
			if (n->isLevelNPC())
			{
				server->deleteNPC(n, this, false);
				i = levelNPCs.erase(i);
			}
			else i++;
		}
		//levelNPCs.clear();
	}

	// Delete baddies.
	for (std::vector<TLevelBaddy*>::iterator i = levelBaddies.begin(); i != levelBaddies.end(); ++i) delete *i;
	levelBaddies.clear();
	levelBaddyIds.clear();

	// Delete chests.
	for (std::vector<TLevelChest*>::iterator i = levelChests.begin(); i != levelChests.end(); ++i) delete *i;
	levelChests.clear();

	// Delete links.
	for (std::vector<TLevelLink*>::iterator i = levelLinks.begin(); i != levelLinks.end(); ++i) delete *i;
	levelLinks.clear();

	// Delete signs.
	for (std::vector<TLevelSign*>::iterator i = levelSigns.begin(); i != levelSigns.end(); ++i) delete *i;
	levelSigns.clear();

	// Delete items.
	for (std::vector<TLevelItem*>::iterator i = levelItems.begin(); i != levelItems.end(); ++i)
	{
		TLevelItem* item = *i;
		CString packet = CString() >> (char)PLO_ITEMDEL >> (char)(item->getX() * 2) >> (char)(item->getY() * 2);
		for (std::vector<TPlayer*>::iterator j = levelPlayerList.begin(); j != levelPlayerList.end(); ++j) (*j)->sendPacket(packet);
		delete item;
	}
	levelItems.clear();

	// Delete board changes.
	for (std::vector<TLevelBoardChange*>::iterator i = levelBoardChanges.begin(); i != levelBoardChanges.end(); ++i) delete *i;
	levelBoardChanges.clear();

	// Clean up the rest.
	levelSpar = false;
	levelSingleplayer = false;

	// Remove all the players from the level.
	std::vector<TPlayer*> oldplayers = levelPlayerList;
	for (std::vector<TPlayer*>::iterator i = oldplayers.begin(); i != oldplayers.end(); ++i)
	{
		TPlayer* p = *i;
		p->leaveLevel(true);
	}

	// Reset the level cache for all the players on the server.
	std::vector<TPlayer*>* playerList = server->getPlayerList();
	for (std::vector<TPlayer*>::iterator i = playerList->begin(); i != playerList->end(); ++i)
	{
		(*i)->resetLevelCache(this);
	}

	// Re-load the level now.
	bool ret = loadLevel(levelName);

#ifndef V8NPCSERVER
	// Update the npc-server of the level change.
	if (server->hasNPCServer())
	{
		server->NC_SendLevel(this);
		server->getSocketManager()->updateSingle(server->getNPCServer(), false, true);
	}
#endif

	// Warp all players back to the level (or to unstick me if loadLevel failed).
	CString uLevel = server->getSettings()->getStr("unstickmelevel", "onlinestartlocal.nw");
	float uX = server->getSettings()->getFloat("unstickmex", 30.0f);
	float uY = server->getSettings()->getFloat("unstickmey", 35.0f);
	for (std::vector<TPlayer*>::iterator i = oldplayers.begin(); i != oldplayers.end(); ++i)
	{
		TPlayer* p = *i;
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
		return 0;
	}
	return level;
}

bool TLevel::loadLevel(const CString& pLevelName)
{
#ifdef V8NPCSERVER
	server->getScriptEngine()->WrapObject(this);
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
	if (server->getSettings()->getBool("nofoldersconfig", false) == false)
		fileSystem = server->getFileSystem(FS_LEVEL);

	// Load file
	CString fileData;
	if (fileData.load(fileSystem->find(pLevelName)) == false) return false;

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
	if (server->getSettings()->getBool("nofoldersconfig", false) == false)
		fileSystem = server->getFileSystem(FS_LEVEL);

	// Path-To-File
	actualLevelName = levelName = pLevelName;
	fileName = fileSystem->find(pLevelName);
	modTime = fileSystem->getModTime(pLevelName);

	// Load file
	CString fileData;
	if (fileData.load(fileName) == false) return false;

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

			levelLinks.push_back(new TLevelLink(vline));
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
			if (baddy == 0)
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

			levelSigns.push_back(new TLevelSign(x, y, text, true));
		}
	}

	return true;
}

bool TLevel::loadGraal(const CString& pLevelName)
{
	// Get the appropriate filesystem.
	CFileSystem* fileSystem = server->getFileSystem();
	if (server->getSettings()->getBool("nofoldersconfig", false) == false)
		fileSystem = server->getFileSystem(FS_LEVEL);

	// Path-To-File
	actualLevelName = levelName = pLevelName;
	fileName = fileSystem->find(pLevelName);
	modTime = fileSystem->getModTime(pLevelName);

	// Load file
	CString fileData;
	if (fileData.load(fileName) == false) return false;

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

			levelLinks.push_back(new TLevelLink(vline));
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
			if (baddy == 0)
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
			CString code = line.readString("");

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

			signed char x = line.readGChar();
			signed char y = line.readGChar();
			signed char item = line.readGChar();
			signed char signindex = line.readGChar();

			levelChests.push_back(new TLevelChest(x, y, item, signindex));
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

			levelSigns.push_back(new TLevelSign(x, y, text, true));
		}
	}

	return true;
}

bool TLevel::loadNW(const CString& pLevelName)
{
	// Get the appropriate filesystem.
	CFileSystem* fileSystem = server->getFileSystem();
	if (server->getSettings()->getBool("nofoldersconfig", false) == false)
		fileSystem = server->getFileSystem(FS_LEVEL);

	// Path-To-File
	actualLevelName = levelName = pLevelName;
	fileName = fileSystem->find(pLevelName);
	modTime = fileSystem->getModTime(pLevelName);

	// Load File
	std::vector<CString> fileData = CString::loadToken(fileName, "\n", true);
	if (fileData.size() == 0)
		return false;

	// Grab File Version
	fileVersion = fileData[0];

	// Parse Level
	for (std::vector<CString>::iterator i = fileData.begin(); i != fileData.end(); ++i)
	{
		// Tokenize
		std::vector<CString> curLine = i->tokenize();
		if (curLine.size() < 1)
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
					short tile = base64.find(left) << 6;
					tile += base64.find(top);
					levelTiles[ii + y*64] = tile;
				}
			}
		}
		else if (curLine[0] == "CHEST")
		{
			if (curLine.size() != 5)
				continue;
			if ((curLine[3] = CString(TLevelItem::getItemId(curLine[3]))) == "-1")
				continue;

			levelChests.push_back(new TLevelChest(curLine));
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
				for (unsigned int i = 0; i < link.size() - 7; ++i)
					level << " " << link[1 + i];
			}

			if (fileSystem->find(level).isEmpty())
				continue;

			levelLinks.push_back(new TLevelLink(link));
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
				offset = curLine.size() - 4;
				for (unsigned int i = 0; i < offset; ++i)
					image << " " << curLine[2 + i];
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
			levelSigns.push_back(new TLevelSign(x, y, text));
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

	// Find Appropriate Level by Name
	for (std::vector<TLevel *>::iterator i = levelList->begin(); i != levelList->end(); )
	{
		// TODO(joey): Would this ever even happen? Why is this here. Maybe we should use a hashmap for levels?
		if ((*i) == 0)
		{
			i = levelList->erase(i);
			continue;
		}

		if ((*i)->getLevelName().toLower() == pLevelName.toLower())
			return (*i);

		++i;
	}

	// Load New Level
	TLevel *level = new TLevel(server);
	if (!level->loadLevel(pLevelName))
	{
		delete level;
		return 0;
	}

#ifndef V8NPCSERVER
	// Send NC Level
	if (server->hasNPCServer())
	{
		server->NC_SendLevel(level);
		server->getSocketManager()->updateSingle(server->getNPCServer(), false, true);
	}
#endif
	
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
	for (std::vector<TLevelBoardChange*>::iterator i = levelBoardChanges.begin(); i != levelBoardChanges.end(); )
	{
		TLevelBoardChange* change = *i;
		if ((change->getX() >= pX && change->getX() + change->getWidth() <= pX + pWidth) &&
			(change->getY() >= pY && change->getY() + change->getHeight() <= pY + pHeight))
		{
			delete change;
			i = levelBoardChanges.erase(i);
		} else ++i;
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
	levelBoardChanges.push_back(new TLevelBoardChange(pX, pY, pWidth, pHeight, pTileData, oldTiles, (doRespawn ? respawnTime : -1)));
	return true;
}

bool TLevel::addItem(float pX, float pY, char pItem)
{
	levelItems.push_back(new TLevelItem(pX, pY, pItem));
	return true;
}

signed char TLevel::removeItem(float pX, float pY)
{
	for (std::vector<TLevelItem*>::iterator i = levelItems.begin(); i != levelItems.end(); ++i)
	{
		TLevelItem* item = *i;
		if (item->getX() == pX && item->getY() == pY)
		{
			signed char itemType = item->getItem();
			delete item;
			levelItems.erase(i);
			return itemType;
		}
	}
	return -1;
}

bool TLevel::addHorse(CString& pImage, float pX, float pY, char pDir, char pBushes)
{
	levelHorses.push_back(new TLevelHorse(server, pImage, pX, pY, pDir, pBushes));
	return true;
}

void TLevel::removeHorse(float pX, float pY)
{
	for (std::vector<TLevelHorse *>::iterator i = levelHorses.begin(); i != levelHorses.end(); ++i)
	{
		TLevelHorse* horse = *i;
		if (horse->getX() == pX && horse->getY() == pY)
		{
			delete horse;
			levelHorses.erase(i);
			return;
		}
	}
}

TLevelBaddy* TLevel::addBaddy(float pX, float pY, char pType)
{
	// Limit of 50 baddies per level.
	if (levelBaddies.size() > 50) return 0;

	// New Baddy
	TLevelBaddy* newBaddy = new TLevelBaddy(pX, pY, pType, this, server);
	levelBaddies.push_back(newBaddy);

	// Assign Baddy Id
	// Don't assign id 0.
	for (unsigned int i = 1; i < levelBaddyIds.size(); ++i)
	{
		if (levelBaddyIds[i] == 0)
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
	if (baddy == 0) return;

	// Erase the baddy.
	if (!levelBaddies.empty())
	{
		for (std::vector<TLevelBaddy*>::iterator i = levelBaddies.begin(); i != levelBaddies.end();)
		{
			TLevelBaddy* b = *i;
			if (b == baddy)
				i = levelBaddies.erase(i);
			else ++i;
		}
	}
	//vecRemove(levelBaddies, baddy);
	levelBaddyIds[pId] = 0;

	// Clean up.
	delete baddy;
}

TLevelBaddy* TLevel::getBaddy(char id)
{
	if ((unsigned char)id >= levelBaddyIds.size()) return 0;
	return levelBaddyIds[id];
}

int TLevel::addPlayer(TPlayer* player)
{
	levelPlayerList.push_back(player);

#ifdef V8NPCSERVER
	for (std::vector<TNPC *>::iterator it = levelNPCs.begin(); it != levelNPCs.end(); ++it)
	{
		TNPC *npc = *it;
		if (npc->hasScriptEvent(NPCEVENTFLAG_PLAYERENTERS))
			npc->queueNpcAction("npc.playerenters", player);
	}
#endif

	return levelPlayerList.size() - 1;
}

void TLevel::removePlayer(TPlayer* player)
{
	for (std::vector<TPlayer *>::iterator i = levelPlayerList.begin(); i != levelPlayerList.end(); )
	{
		TPlayer* search = *i;
		if (player == search)
			i = levelPlayerList.erase(i);
		else ++i;
	}

#ifdef V8NPCSERVER
	for (std::vector<TNPC *>::iterator it = levelNPCs.begin(); it != levelNPCs.end(); ++it) {
		TNPC *npc = *it;
		if (npc->hasScriptEvent(NPCEVENTFLAG_PLAYERLEAVES))
			npc->queueNpcAction("npc.playerleaves", player);
	}
#endif
}

TPlayer* TLevel::getPlayer(unsigned int id)
{
	if (id >= levelPlayerList.size()) return 0;
	return levelPlayerList[id];
}

TMap* TLevel::getMap() const
{
	return server->getMap(this);
}

bool TLevel::addNPC(TNPC* npc)
{
	for (std::vector<TNPC*>::iterator i = levelNPCs.begin(); i != levelNPCs.end(); ++i)
	{
		TNPC* search = *i;
		if (npc == search) return false;
	}
	levelNPCs.push_back(npc);
	return true;
}

void TLevel::removeNPC(TNPC* npc)
{
	for (std::vector<TNPC*>::iterator i = levelNPCs.begin(); i != levelNPCs.end(); )
	{
		TNPC* search = *i;
		if (npc == search)
			i = levelNPCs.erase(i);
		else ++i;
	}
}

bool TLevel::doTimedEvents()
{
	// Check if we should revert any board changes.
	for (std::vector<TLevelBoardChange*>::iterator i = levelBoardChanges.begin(); i != levelBoardChanges.end(); ++i)
	{
		TLevelBoardChange* change = *i;
		int respawnTimer = change->timeout.doTimeout();
		if (respawnTimer == 0)
		{
			// Put the old data back in.  DON'T DELETE THE CHANGE.
			// The client remembers board changes and if we delete the
			// change, the client won't get the new data.
			change->swapTiles();
			change->setModTime(time(0));
			server->sendPacketToLevel(CString() >> (char)PLO_BOARDMODIFY << change->getBoardStr(), 0, this);
		}
	}

	// Check if any items have timed out.
	// This allows us to delete items that have disappeared if nobody is in the level to send
	// the PLI_ITEMDEL packet.
	for (std::vector<TLevelItem*>::iterator i = levelItems.begin(); i != levelItems.end(); )
	{
		TLevelItem* item = *i;
		int deleteTimer = item->timeout.doTimeout();
		if (deleteTimer == 0)
		{
			delete item;
			i = levelItems.erase(i);
		} else ++i;
	}

	// Check if any horses need to be deleted.
	for (std::vector<TLevelHorse *>::iterator i = levelHorses.begin(); i != levelHorses.end(); )
	{
		TLevelHorse* horse = *i;
		int deleteTimer = horse->timeout.doTimeout();
		if (deleteTimer == 0)
		{
			server->sendPacketToLevel(CString() >> (char)PLO_HORSEDEL >> (char)(horse->getX() * 2) >> (char)(horse->getY() * 2), 0, this);
			delete horse;
			i = levelHorses.erase(i);
		}
		else ++i;
	}

	// Check if any baddies need to be marked as dead or respawned.
	std::set<TLevelBaddy*> set_dead;
	for (std::vector<TLevelBaddy *>::iterator i = levelBaddies.begin(); i != levelBaddies.end(); )
	{
		TLevelBaddy* baddy = *i;
		if (baddy == 0)
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
				for (std::vector<TPlayer*>::iterator i = levelPlayerList.begin(); i != levelPlayerList.end(); ++i)
				{
					TPlayer* p = *i;
					p->sendPacket(CString() >> (char)PLO_BADDYPROPS >> (char)baddy->getId() << baddy->getProps(p->getVersion()));
				}
			}
		}
	}
	{	// Mark all the baddies as dead now.
		CString props = CString() >> (char)BDPROP_MODE >> (char)BDMODE_DEAD;
		for (std::set<TLevelBaddy*>::iterator i = set_dead.begin(); i != set_dead.end(); ++i)
		{
			TLevelBaddy* baddy = *i;
			baddy->setProps(props);
		}
	}

	return true;
}

#ifdef V8NPCSERVER
TLevelLink * TLevel::isOnLink(int pX, int pY)
{
	if (!levelLinks.empty())
	{
		for (auto it = levelLinks.begin(); it != levelLinks.end(); ++it)
		{
			TLevelLink *levelLink = *it;
//			printf("Check %d in %d, %d\nCheck %d in %d, %d\n", pX, levelLink->getX(), levelLink->getX() + levelLink->getWidth(), pY, levelLink->getY(), levelLink->getY() + levelLink->getHeight());
			if ((pX >= levelLink->getX() && pX <= levelLink->getX() + levelLink->getWidth()) &&
				(pY >= levelLink->getY() && pY <= levelLink->getY() + levelLink->getHeight()))
			{
				return levelLink;
			}
		}
	}

	return nullptr;
}

TNPC * TLevel::isOnNPC(int pX, int pY, bool checkEventFlag)
{
	for (auto it = levelNPCs.begin(); it != levelNPCs.end(); ++it)
	{
		TNPC *npc = *it;
		if (checkEventFlag && !npc->hasScriptEvent(NPCEVENTFLAG_PLAYERTOUCHSME))
			continue;

		//if (!npc->getImage().isEmpty())
		{
			if ((npc->getVisFlags() & 1) != 0)
			{
				if ((pX >= npc->getPixelX() && pX <= npc->getPixelX() + npc->getWidth()) &&
					(pY >= npc->getPixelY() && pY <= npc->getPixelY() + npc->getHeight()))
				{
					// what if it touches multiple npcs? hm. not sure how graal did it.
					return npc;
				}
			}
		}
	}

	return nullptr;
}

bool TLevel::isOnWall(double pX, double pY)
{
	return false;
}

bool TLevel::isOnWater(double pX, double pY)
{
	return false;
}

void TLevel::sendChatToLevel(const TPlayer *player, const CString& message)
{
	for (std::vector<TNPC *>::iterator it = levelNPCs.begin(); it != levelNPCs.end(); ++it)
	{
		TNPC *npc = *it;
		if (npc->hasScriptEvent(NPCEVENTFLAG_PLAYERCHATS))
			npc->queueNpcEvent("npc.playerchats", true, player->getScriptObject(), std::string(message.text()));
	}
}

#endif