#include "ICommon.h"
#include "CSocket.h"
#include "CSettings.h"
#include "CFileSystem.h"
#include "TServer.h"

static const char* const filesystemTypes[] =
{
	"all",
	"file",
	"level",
	"head",
	"body",
	"sword",
	"shield",
	0
};

TServer::TServer(CString pName)
: name(pName), lastTimer(time(0)), lastNWTimer(time(0)), last5mTimer(time(0))
{
	// Player ids 0 and 1 break things.  NPC id 0 breaks things.
	// Don't allow anything to have one of those ids.
	playerIds.resize(2);
	npcIds.resize(1);

	// This has the full path to the server directory.
	serverpath = CString() << getHomePath() << "servers/" << name << "/";
	CFileSystem::fixPathSeparators(&serverpath);

	// Set up the log files.
	serverlog.setFilename(CString() << serverpath << "logs/serverlog.txt");
	rclog.setFilename(CString() << serverpath << "logs/rclog.txt");

	serverlist.setServer(this);
	for (int i = 0; i < FS_COUNT; ++i)
		filesystem[i].setServer(this);
	filesystem_accounts.setServer(this);
}

TServer::~TServer()
{
	// Save server flags.
	CString out;
	for (std::vector<CString>::iterator i = serverFlags.begin(); i != serverFlags.end(); ++i)
		out << *i << "\r\n";
	out.save(CString() << serverpath << "serverflags.txt");

	for (std::vector<TPlayer*>::iterator i = playerList.begin(); i != playerList.end(); )
	{
		delete *i;
		i = playerList.erase(i);
	}

	for (std::vector<TNPC*>::iterator i = npcList.begin(); i != npcList.end(); )
	{
		delete *i;
		i = npcList.erase(i);
	}

	for (std::vector<TLevel*>::iterator i = levelList.begin(); i != levelList.end(); )
	{
		delete *i;
		i = levelList.erase(i);
	}

	for (std::vector<TMap*>::iterator i = mapList.begin(); i != mapList.end(); )
	{
		delete *i;
		i = mapList.erase(i);
	}

	for (std::vector<TWeapon*>::iterator i = weaponList.begin(); i != weaponList.end(); )
	{
		(*i)->saveWeapon(this);
		delete *i;
		i = weaponList.erase(i);
	}
}

int TServer::init()
{
	// Load Settings
	serverlog.out("     Loading settings...\n");
	settings.setSeparator("=");
	settings.loadFile(CString() << serverpath << "config/serveroptions.txt");
	if (!settings.isOpened())
	{
		serverlog.out("** [Error] Could not open config/serveroptions.txt\n");
		return ERR_SETTINGS;
	}

	// Load allowed versions.
	serverlog.out("     Loading allowed client versions...\n");
	CString versions;
	versions.load(CString() << serverpath << "config/allowedversions.txt");
	versions = removeComments(versions);
	versions.removeAllI("\r");
	allowedVersions = versions.tokenize("\n");

	// Load folders config.
	// Load before file system.
	serverlog.out("     Folder config: ");
	if (settings.getBool("nofoldersconfig", false) == false)
	{
		serverlog.out("ENABLED\n");
	} else serverlog.out("disabled\n");

	// Load file system.
	serverlog.out("     Loading file system...\n");
	filesystem_accounts.addDir("accounts");
	if (settings.getBool("nofoldersconfig", false) == true)
		loadAllFolders();
	else
		loadFolderConfig();

	// Load status list.
	serverlog.out("     Loading status list...\n");
	statusList = settings.getStr("playerlisticons", "Online,Away,DND,Eating,Hiding,No PMs,RPing,Sparring,PKing").tokenize(",");

	// Load server flags.
	serverlog.out("     Loading serverflags.txt...\n");
	serverFlags = CString::loadToken(CString() << serverpath << "serverflags.txt", "\n", true);

	// Load server message.
	serverlog.out("     Loading config/servermessage.html...\n");
	servermessage.load(CString() << serverpath << "config/servermessage.html");
	servermessage.removeAllI("\r");
	servermessage.replaceAllI("\n", " ");

	// Load IP bans.
	serverlog.out("     Loading config/ipbans.txt...\n");
	ipBans = CString::loadToken(CString() << serverpath << "config/ipbans.txt", "\n", true);

	// Load weapons.
	serverlog.out("     Loading weapons...\n");
	{
		CFileSystem weaponFS(this);
		weaponFS.addDir("weapons");
		std::map<CString, CString>* weaponFileList = weaponFS.getFileList();
		for (std::map<CString, CString>::iterator i = weaponFileList->begin(); i != weaponFileList->end(); ++i)
		{
			TWeapon* weapon = TWeapon::loadWeapon(i->first.removeAll(".txt"), this);
			if (weapon != 0)
			{
				serverlog.out("       %s\n", weapon->getName().text());
				weaponList.push_back(weapon);
			}
		}

		// Add the default weapons.
		weaponList.push_back(new TWeapon(TLevelItem::getItemId("bow")));
		weaponList.push_back(new TWeapon(TLevelItem::getItemId("bomb")));
		weaponList.push_back(new TWeapon(TLevelItem::getItemId("superbomb")));
		weaponList.push_back(new TWeapon(TLevelItem::getItemId("fireball")));
		weaponList.push_back(new TWeapon(TLevelItem::getItemId("fireblast")));
		weaponList.push_back(new TWeapon(TLevelItem::getItemId("nukeshot")));
		weaponList.push_back(new TWeapon(TLevelItem::getItemId("joltbomb")));
	}

	// Load gmaps.
	serverlog.out("     Loading gmaps...\n");
	std::vector<CString> gmaps = settings.getStr("gmaps").guntokenize().tokenize("\n");
	for (std::vector<CString>::iterator i = gmaps.begin(); i != gmaps.end(); ++i)
	{
		// Check for blank lines.
		if (*i == "\r") continue;

		// Load the gmap.
		TMap* gmap = new TMap(MAPTYPE_GMAP);
		if (gmap->load(CString() << *i << ".gmap", this) == false)
		{
			serverlog.out(CString() << "** [Error] Could not load " << *i << ".gmap" << "\n");
			delete gmap;
			continue;
		}

		serverlog.out("       %s\n", i->text());
		mapList.push_back(gmap);
	}

	// Load bigmaps.
	serverlog.out("     Loading bigmaps...\n");
	std::vector<CString> bigmaps = settings.getStr("maps").guntokenize().tokenize("\n");
	for (std::vector<CString>::iterator i = bigmaps.begin(); i != bigmaps.end(); ++i)
	{
		// Check for blank lines.
		if (*i == "\r") continue;

		// Load the bigmap.
		TMap* bigmap = new TMap(MAPTYPE_BIGMAP);
		if (bigmap->load(*i, this) == false)
		{
			serverlog.out(CString() << "** [Error] Could not load " << *i << "\n");
			delete bigmap;
			continue;
		}

		serverlog.out("       %s\n", i->text());
		mapList.push_back(bigmap);
	}

	// Initialize the player socket.
	playerSock.setType(SOCKET_TYPE_SERVER);
	playerSock.setProtocol(SOCKET_PROTOCOL_TCP);
	playerSock.setDescription("playerSock");

	// Start listening on the player socket.
	serverlog.out("     Initializing player listen socket.\n");
	if (playerSock.init(0, settings.getStr("serverport").text()))
	{
		serverlog.out("** [Error] Could not initialize listening socket...\n");
		return ERR_LISTEN;
	}
	if (playerSock.connect())
	{
		serverlog.out("** [Error] Could not connect listening socket...\n");
		return ERR_LISTEN;
	}

	// Connect to the serverlist.
	serverlog.out("     Initializing serverlist socket.\n");
	if (!serverlist.init(settings.getStr("listip"), settings.getStr("listport")))
	{
		serverlog.out("** [Error] Cound not initialize serverlist socket.\n");
		return ERR_LISTEN;
	}
	serverlist.connectServer();

	// Register ourself with the socket manager.
	sockManager.registerSocket((CSocketStub*)this);
	sockManager.registerSocket((CSocketStub*)&serverlist);

	return 0;
}

// Called when the TServer is put into its own thread.
void TServer::operator()()
{
	bool running = true;
	while (running)
	{
		// TODO: If something happens, attempt to restart the server.
		if (doMain() == false)
			break;

		try
		{
			boost::this_thread::interruption_point();
		}
		catch (boost::thread_interrupted e)
		{
			running = false;
		}
/*
		boost::xtime xt;
		boost::xtime_get(&xt, boost::TIME_UTC);
		xt.nsec += 5000000;		// 5 milliseconds
		boost::this_thread::sleep(xt);
*/
		boost::this_thread::yield();
	}
}

bool TServer::doMain()
{
	// If we aren't connected to the serverlist, reconnect.
	if (!serverlist.main())
		serverlist.connectServer();

	// Update our socket manager.
	sockManager.update(1, 0);

	// Every second, do some events.
	if (time(0) != lastTimer) doTimedEvents();

	return true;
}

bool TServer::doTimedEvents()
{
	lastTimer = time(0);

	// Do serverlist events.
	serverlist.doTimedEvents();

	// Do player events.
	{
		for (std::vector<TPlayer *>::iterator i = playerList.begin(); i != playerList.end();)
		{
			TPlayer *player = (TPlayer*)*i;
			if (player == 0)
			{
				++i;
				continue;
			}

			if (!player->doTimedEvents())
			{
				this->deletePlayer(player);
				i = playerList.begin();
			}
			else ++i;
		}
	}

	// Do level events.
	{
		for (std::vector<TLevel *>::iterator i = levelList.begin(); i != levelList.end(); ++i)
		{
			TLevel* level = *i;
			if (level == 0)
				continue;

			level->doTimedEvents();
		}
	}

	// Send NW time.
	if ((int)difftime(lastTimer, lastNWTimer) >= 5)
	{
		lastNWTimer = lastTimer;
		sendPacketToAll(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(getNWTime()));
	}

	// Save stuff every 5 minutes.
	if ((int)difftime(lastTimer, last5mTimer) >= 300)
	{
		last5mTimer = lastTimer;

		// Save server flags.
		CString out;
		for (std::vector<CString>::iterator i = serverFlags.begin(); i != serverFlags.end(); ++i)
			out << *i << "\r\n";
		out.save(CString() << serverpath << "serverflags.txt");
	}

	return true;
}

bool TServer::onRecv()
{
	// Create socket.
	CSocket *newSock = playerSock.accept();	// 1 second
	if (newSock == 0)
		return true;

	// Get a free id to be assigned to the new player.
	unsigned int newId = 0;
	for (unsigned int i = 2; i < playerIds.size(); ++i)
	{
		if (playerIds[i] == 0)
		{
			newId = i;
			i = playerIds.size();
		}
	}
	if (newId == 0)
	{
		newId = playerIds.size();
		playerIds.push_back(0);
	}

	// Create the new player.
	TPlayer* newPlayer = new TPlayer(this, newSock, newId);
	playerIds[newId] = newPlayer;

	// Add them to the player list.
	playerList.push_back(newPlayer);

	// Add them to the socket manager.
	sockManager.registerSocket((CSocketStub*)newPlayer);

	return true;
}

/////////////////////////////////////////////////////

void TServer::loadAllFolders()
{
	for (int i = 0; i < FS_COUNT; ++i)
		filesystem[i].clear();

	filesystem[0].addDir("world");
	if (settings.getStr("sharefolder").length() > 0)
	{
		std::vector<CString> folders = settings.getStr("sharefolder").tokenize(",");
		for (std::vector<CString>::iterator i = folders.begin(); i != folders.end(); ++i)
			filesystem[0].addDir(i->trim());
	}
}

void TServer::loadFolderConfig()
{
	for (int i = 0; i < FS_COUNT; ++i)
		filesystem[i].clear();

	foldersConfig = CString::loadToken(CString() << serverpath << "config/foldersconfig.txt", "\n", true);
	for (std::vector<CString>::iterator i = foldersConfig.begin(); i != foldersConfig.end(); ++i)
	{
		// No comments.
		int cLoc = -1;
		if ((cLoc = (*i).find("#")) != -1)
			(*i).removeI(cLoc);
		(*i).trimI();
		if ((*i).length() == 0) continue;

		// Parse the line.
		CString type = i->readString(" ");
		CString config = i->readString("");
		type.trimI();
		config.trimI();
		CFileSystem::fixPathSeparators(&config);

		// Get the directory.
		CString dirNoWild;
		int pos = -1;
		if ((pos = config.findl(CFileSystem::getPathSeparator())) != -1)
			dirNoWild = config.remove(pos + 1);
		CString dir = CString("world/") << dirNoWild;
		CString wildcard = config.remove(0, dirNoWild.length());

		// Find out which file system to add it to.
		int fs = -1;
		int j = 0;
		while (filesystemTypes[j] != 0)
		{
			if (type.comparei(CString(filesystemTypes[j])))
			{
				fs = j;
				break;
			}
			++j;
		}

		// Add it to the appropriate file system.
		if (fs != -1) { filesystem[fs].addDir(dir, wildcard); printf("adding %s [%s] to %s\n", dir.text(), wildcard.text(), filesystemTypes[fs]); }
		filesystem[0].addDir(dir, wildcard);
	}
}

/////////////////////////////////////////////////////

TPlayer* TServer::getPlayer(const unsigned short id, bool includeRC) const
{
	if (id >= (unsigned short)playerIds.size()) return 0;
	if (!includeRC && playerIds[id]->isRC()) return 0;
	return playerIds[id];
}

TPlayer* TServer::getPlayer(const CString& account, bool includeRC) const
{
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		TPlayer *player = (TPlayer*)*i;
		if (player == 0)
			continue;

		if (!includeRC && player->isRC())
			continue;

		// Compare account names.
		if (player->getProp(PLPROP_ACCOUNTNAME).subString(1) == account)
			return player;
	}
	return 0;
}

TNPC* TServer::getNPC(const unsigned int id) const
{
	if (id >= npcIds.size()) return 0;
	return npcIds[id];
}

TLevel* TServer::getLevel(const CString& pLevel)
{
	return TLevel::findLevel(pLevel, this);
}

TMap* TServer::getMap(const CString& name) const
{
	for (std::vector<TMap*>::const_iterator i = mapList.begin(); i != mapList.end(); ++i)
	{
		TMap* map = *i;
		if (map->getMapName() == name)
			return map;
	}
	return 0;
}

TMap* TServer::getMap(const TLevel* pLevel) const
{
	if (pLevel == 0) return 0;

	for (std::vector<TMap*>::const_iterator i = mapList.begin(); i != mapList.end(); ++i)
	{
		TMap* pMap = *i;
		if (pMap->isLevelOnMap(pLevel->getLevelName()))
			return pMap;
	}
	return 0;
}

TWeapon* TServer::getWeapon(const CString& name) const
{
	for (std::vector<TWeapon*>::const_iterator i = weaponList.begin(); i != weaponList.end(); ++i)
	{
		TWeapon* weapon = *i;
		if (weapon->getName() == name)
			return weapon;
	}
	return 0;
}

CString TServer::getFlag(const CString& pName) const
{
	for (std::vector<CString>::const_iterator i = serverFlags.begin(); i != serverFlags.end(); ++i)
	{
		if (*i == pName)
			return *i;
	}
	return CString();
}

TNPC* TServer::addNPC(const CString& pImage, const CString& pScript, float pX, float pY, TLevel* pLevel, bool pLevelNPC, bool sendToPlayers)
{
	// New Npc
	TNPC* newNPC = new TNPC(pImage, pScript, pX, pY, this, pLevel, pLevelNPC, settings.getBool("trimnpccode", false));
	npcList.push_back(newNPC);

	// Assign NPC Id
	bool assignedId = false;
	for (unsigned int i = 1; i < npcIds.size(); ++i)
	{
		if (npcIds[i] == 0)
		{
			npcIds[i] = newNPC;
			newNPC->setId(i);
			assignedId = true;
		}
	}

	// Assign NPC Id
	if (assignedId == false)
	{
		newNPC->setId(npcIds.size());
		npcIds.push_back(newNPC);
	}

	// Send the NPC's props to everybody in range.
	if (sendToPlayers)
	{
		TMap* map = getMap(pLevel);
		sendPacketToLevel(CString() >> (char)PLO_NPCPROPS >> (int)newNPC->getId() << newNPC->getProps(0), map, pLevel, 0, true);
	}

	return newNPC;
}

bool TServer::deleteNPC(const unsigned int pId, TLevel* pLevel)
{
	// Grab the NPC.
	TNPC* npc = getNPC(pId);
	if (npc == 0) return false;

	return deleteNPC(npc, pLevel);
}

bool TServer::deleteNPC(TNPC* npc, TLevel* pLevel)
{
	if (npc == 0) return false;
	if (npc->getId() >= npcIds.size()) return false;

	// If pLevel == 0, then it is an npc-server NPC.
	// Not currently supported so just exit.
	if (pLevel == 0) return false;

	// Remove the NPC from all the lists.
	pLevel->removeNPC(npc);
	npcIds[npc->getId()] = 0;

	for (std::vector<TNPC*>::iterator i = npcList.begin(); i != npcList.end(); )
	{
		if ((*i) == npc)
			i = npcList.erase(i);
		else ++i;
	}

	// Tell the client to delete the NPC.
	sendPacketTo(PLTYPE_ANYCLIENT, CString() >> (char)PLO_NPCDEL2 >> (char)npc->getLevel()->getLevelName().length() << npc->getLevel()->getLevelName() >> (int)npc->getId());

	// Delete the NPC from memory.
	delete npc;

	return true;
}

bool TServer::addFlag(const CString& pFlag)
{
	if (settings.getBool("dontaddserverflags", false) == true)
		return false;

	CString flag(pFlag);
	CString flagName = flag.readString("=").trim();
	CString flagValue = flag.readString("").trim();
	CString flagNew = CString() << flagName << "=" << flagValue;

	for (std::vector<CString>::iterator i = serverFlags.begin(); i != serverFlags.end(); ++i)
	{
		CString tflagName = i->readString("=").trim();
		if (tflagName == flagName)
		{
			// A flag with a value of 0 means we should unset it.
			if (flagValue.length() == 0)
			{
				sendPacketToAll(CString() >> (char)PLO_FLAGDEL << flagName);
				serverFlags.erase(i);
				return true;
			}

			// If we didn't unset it, alter the existing flag.
			sendPacketToAll(CString() >> (char)PLO_FLAGSET << flagNew);
			*i = flagNew;
			return true;
		}
	}

	// We didn't find a pre-existing flag so let's create a new one.
	sendPacketToAll(CString() >> (char)PLO_FLAGSET << flagNew);
	serverFlags.push_back(flagNew);

	return true;
}

bool TServer::deleteFlag(const CString& pFlag)
{
	if (settings.getBool("dontaddserverflags", false) == true)
		return false;

	CString flag(pFlag);
	CString flagName = flag.readString("=").trim();

	// Loop for flags now.
	for (std::vector<CString>::iterator i = serverFlags.begin(); i != serverFlags.end(); )
	{
		CString tflagName = i->readString("=").trim();
		if (tflagName == flagName)
		{
			sendPacketToAll(CString() >> (char)PLO_FLAGDEL << flagName);
			serverFlags.erase(i);
			return true;
		}
	}
	return false;
}

bool TServer::deletePlayer(TPlayer* player)
{
	// Remove the player from the serverlist.
	serverlist.remPlayer(player->getProp(PLPROP_ACCOUNTNAME).removeI(0,1), player->getType());

	// Get rid of the player now.
	playerIds[player->getId()] = 0;
	for (std::vector<TPlayer*>::iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if (*i == player)
		{
			playerList.erase(i);
			break;
		}
	}

	// Remove the player from the socket manager.
	sockManager.unregisterSocket((CSocketStub*)player);

	// Clean up.
	delete player;

	return true;
}

unsigned int TServer::getNWTime() const
{
	return ((unsigned int)time(0) - 11078 * 24 * 60 * 60) * 2 / 10;
}

bool TServer::isIpBanned(const CString& ip)
{
	for (std::vector<CString>::const_iterator i = ipBans.begin(); i != ipBans.end(); ++i)
	{
		if (ip.match(*i)) return true;
	}
	return false;
}

/*
	Packet-Sending Functions
*/
void TServer::sendPacketToAll(CString pPacket, TPlayer *pPlayer) const
{
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if ((*i) == pPlayer) continue;
		(*i)->sendPacket(pPacket);
	}
}

void TServer::sendPacketToLevel(CString pPacket, TMap* pMap, TLevel* pLevel, TPlayer* pPlayer, bool onlyGmap) const
{
	if (pMap == 0 || (onlyGmap && pMap->getType() == MAPTYPE_BIGMAP))
	{
		for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
		{
			if ((*i) == pPlayer || !(*i)->isClient()) continue;
			if ((*i)->getLevel() == pLevel)
				(*i)->sendPacket(pPacket);
		}
		return;
	}

	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if (!(*i)->isClient() || (*i) == pPlayer) continue;
		if ((*i)->getMap() == pMap)
		{
			int sgmap[2] = {pMap->getLevelX(pLevel->getLevelName()), pMap->getLevelY(pLevel->getLevelName())};
			int ogmap[2];
			switch (pMap->getType())
			{
				case MAPTYPE_GMAP:
					ogmap[0] = (*i)->getProp(PLPROP_GMAPLEVELX).readGUChar();
					ogmap[1] = (*i)->getProp(PLPROP_GMAPLEVELY).readGUChar();
					break;

				default:
				case MAPTYPE_BIGMAP:
					ogmap[0] = pMap->getLevelX((*i)->getLevel()->getLevelName());
					ogmap[1] = pMap->getLevelY((*i)->getLevel()->getLevelName());
					break;
			}

			if (abs(ogmap[0] - sgmap[0]) < 2 && abs(ogmap[1] - sgmap[1]) < 2)
				(*i)->sendPacket(pPacket);
		}
	}
}

void TServer::sendPacketToLevel(CString pPacket, TMap* pMap, TPlayer* pPlayer, bool sendToSelf, bool onlyGmap) const
{
	if (pPlayer->getLevel() == 0) return;
	if (pMap == 0 || (onlyGmap && pMap->getType() == MAPTYPE_BIGMAP))
	{
		TLevel* level = pPlayer->getLevel();
		if (level == 0) return;
		for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
		{
			if (((*i) == pPlayer && !sendToSelf) || !(*i)->isClient()) continue;
			if ((*i)->getLevel() == level)
				(*i)->sendPacket(pPacket);
		}
		return;
	}

	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if (!(*i)->isClient()) continue;
		if ((*i) == pPlayer)
		{
			if (sendToSelf) pPlayer->sendPacket(pPacket);
			continue;
		}
		if ((*i)->getLevel() == 0) continue;
		if ((*i)->getMap() == pMap)
		{
			int ogmap[2], sgmap[2];
			switch (pMap->getType())
			{
				case MAPTYPE_GMAP:
					ogmap[0] = (*i)->getProp(PLPROP_GMAPLEVELX).readGUChar();
					ogmap[1] = (*i)->getProp(PLPROP_GMAPLEVELY).readGUChar();
					sgmap[0] = pPlayer->getProp(PLPROP_GMAPLEVELX).readGUChar();
					sgmap[1] = pPlayer->getProp(PLPROP_GMAPLEVELY).readGUChar();
					break;

				default:
				case MAPTYPE_BIGMAP:
					ogmap[0] = pMap->getLevelX((*i)->getLevel()->getLevelName());
					ogmap[1] = pMap->getLevelY((*i)->getLevel()->getLevelName());
					sgmap[0] = pMap->getLevelX(pPlayer->getLevel()->getLevelName());
					sgmap[1] = pMap->getLevelY(pPlayer->getLevel()->getLevelName());
					break;
			}

			if (abs(ogmap[0] - sgmap[0]) < 2 && abs(ogmap[1] - sgmap[1]) < 2)
				(*i)->sendPacket(pPacket);
		}
	}
}

void TServer::sendPacketTo(int who, CString pPacket, TPlayer* pPlayer) const
{
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if ((*i) == pPlayer) continue;
		if ((*i)->getType() & who)
			(*i)->sendPacket(pPacket);
	}
}
