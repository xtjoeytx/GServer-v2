#include "IDebug.h"
#include <boost/thread.hpp>
#include "ICommon.h"
#include "IEnums.h"
#include "CSocket.h"
#include "CSettings.h"
#include "CFileSystem.h"
#include "CWordFilter.h"
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
: doRestart(false), name(pName), wordFilter(this), mNpcServer(0), mPluginManager(this)
{
	lastTimer = lastNWTimer = last1mTimer = last5mTimer = last3mTimer = time(0);

	// This has the full path to the server directory.
	serverpath = CString() << getHomePath() << "servers/" << name << "/";
	CFileSystem::fixPathSeparators(&serverpath);

	// Set up the log files.
	CString logpath = serverpath.remove(0, getHomePath().length());
	CString npcPath = CString() << logpath << "logs/npclog.txt";
	CString rcPath = CString() << logpath << "logs/rclog.txt";
	CString serverPath = CString() << logpath << "logs/serverlog.txt";
	CFileSystem::fixPathSeparators(&npcPath);
	CFileSystem::fixPathSeparators(&rcPath);
	CFileSystem::fixPathSeparators(&serverPath);
	npclog.setFilename(npcPath);
	rclog.setFilename(rcPath);
	serverlog.setFilename(serverPath);

	// Announce ourself to other classes.
	serverlist.setServer(this);
	for (int i = 0; i < FS_COUNT; ++i)
		filesystem[i].setServer(this);
	filesystem_accounts.setServer(this);
}

TServer::~TServer()
{
	cleanup();
}

int TServer::init(const CString& serverip, const CString& serverport, const CString& localip)
{
	// Player ids 0 and 1 break things.  NPC id 0 breaks things.
	// Don't allow anything to have one of those ids.
	playerIds.resize(2);
	npcIds.resize(1);

	// Load the config files.
	int ret = loadConfigFiles();
	if (ret) return ret;

	// If an override serverip and serverport were specified, fix the options now.
	if (!serverip.isEmpty())
		settings.addKey("serverip", serverip);
	if (!serverport.isEmpty())
		settings.addKey("serverport", serverport);
	if (!localip.isEmpty())
		settings.addKey("localip", localip);

	// Initialize the player socket.
	playerSock.setType(SOCKET_TYPE_SERVER);
	playerSock.setProtocol(SOCKET_PROTOCOL_TCP);
	playerSock.setDescription("playerSock");

	// Start listening on the player socket.
	serverlog.out("[%s]      Initializing player listen socket.\n", name.text());
	if (playerSock.init(0, settings.getStr("serverport").text()))
	{
		serverlog.out("[%s] ** [Error] Could not initialize listening socket...\n", name.text());
		return ERR_LISTEN;
	}
	if (playerSock.connect())
	{
		serverlog.out("[%s] ** [Error] Could not connect listening socket...\n", name.text());
		return ERR_LISTEN;
	}

	// Connect to the serverlist.
	serverlog.out("[%s]      Initializing serverlist socket.\n", name.text());
	if (!serverlist.init(settings.getStr("listip"), settings.getStr("listport")))
	{
		serverlog.out("[%s] ** [Error] Cound not initialize serverlist socket.\n", name.text());
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
		// Do a server loop.
		doMain();

		// Clean up deleted players here.
		cleanupDeletedPlayers();

		// Check if we should do a restart.
		if (doRestart)
		{
			doRestart = false;
			cleanup();
			int ret = init();
			if (ret != 0)
				break;
		}

		try
		{
			boost::this_thread::interruption_point();
		}
		catch (boost::thread_interrupted)
		{
			running = false;
		}
	}
	cleanup();
}

void TServer::cleanupDeletedPlayers()
{
	if (deletedPlayers.empty()) return;
	for (std::vector<TPlayer*>::iterator i = deletedPlayers.begin(); i != deletedPlayers.end(); )
	{
		TPlayer* player = *i;
		if (player == 0) { ++i; continue; }

		// Remove the player from the socket manager.
		sockManager.unregisterSocket((CSocketStub*)player);

		// Get rid of the player now.
		playerIds[player->getId()] = 0;
		for (std::vector<TPlayer*>::iterator j = playerList.begin(); j != playerList.end();)
		{
			TPlayer* p = *j;
			if (p == player)
			{
				delete p;
				j = playerList.erase(j);
			}
			else ++j;
		}
		i = deletedPlayers.erase(i);
	}
	deletedPlayers.clear();
}

void TServer::cleanup()
{
	// Save translations.
	this->TS_Save();

	// Save server flags.
	saveServerFlags();

	for (std::vector<TPlayer*>::iterator i = playerList.begin(); i != playerList.end(); )
	{
		delete *i;
		i = playerList.erase(i);
	}
	playerList.clear();

	for (std::vector<TNPC*>::iterator i = npcList.begin(); i != npcList.end(); )
	{
		delete *i;
		i = npcList.erase(i);
	}
	npcList.clear();
	npcIds.clear();

	for (std::vector<TLevel*>::iterator i = levelList.begin(); i != levelList.end(); )
	{
		delete *i;
		i = levelList.erase(i);
	}
	levelList.clear();

	for (std::vector<TMap*>::iterator i = mapList.begin(); i != mapList.end(); )
	{
		delete *i;
		i = mapList.erase(i);
	}
	mapList.clear();

	saveWeapons();
	for (std::map<CString, TWeapon *>::iterator i = weaponList.begin(); i != weaponList.end(); )
	{
		delete i->second;
		weaponList.erase(i++);
	}
	weaponList.clear();

	playerSock.disconnect();
	serverlist.getSocket()->disconnect();

	// Clean up the socket manager.  Pass false so we don't cause a crash.
	sockManager.cleanup(false);
}

void TServer::restart()
{
	doRestart = true;
}

bool TServer::doMain()
{
	// If we aren't connected to the serverlist, reconnect.
	if (!serverlist.getConnected())
		serverlist.connectServer();

	// Update our socket manager.
	sockManager.update(0, 5000);		// 5ms

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
		for (std::vector<TPlayer *>::iterator i = playerList.begin(); i != playerList.end(); ++i)
		{
			TPlayer *player = (TPlayer*)*i;
			if (player == 0)
				continue;

			if (!player->doTimedEvents())
				this->deletePlayer(player);
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

		// Group levels.
		for (std::map<CString, std::map<CString, TLevel*> >::iterator i = groupLevels.begin(); i != groupLevels.end(); ++i)
		{
			for (std::map<CString, TLevel*>::iterator j = (*i).second.begin(); j != (*i).second.end(); ++j)
			{
				TLevel* level = j->second;
				if (level == 0)
					continue;

				level->doTimedEvents();
			}
		}
	}

	// Send NW time.
	if ((int)difftime(lastTimer, lastNWTimer) >= 5)
	{
		lastNWTimer = lastTimer;
		sendPacketToAll(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(getNWTime()));
	}

	// Stuff that happens every minute.
	if ((int)difftime(lastTimer, last1mTimer) >= 60)
	{
		last1mTimer = lastTimer;

		// Save server flags.
		this->saveServerFlags();
	}

	// Stuff that happens every 3 minutes.
	if ((int)difftime(lastTimer, last3mTimer) >= 180)
	{
		last3mTimer = lastTimer;

		// Resynchronize the file systems.
		filesystem_accounts.resync();
		for (int i = 0; i < FS_COUNT; ++i)
			filesystem[i].resync();
	}

	// Save stuff every 5 minutes.
	if ((int)difftime(lastTimer, last5mTimer) >= 300)
	{
		last5mTimer = lastTimer;

		// Reload some server settings.
		loadAllowedVersions();
		loadServerMessage();
		loadIPBans();
		
		// Save some stuff.
		saveWeapons();

		// Check all of the instanced maps to see if the players have left.
		if (!groupLevels.empty())
		{
			for (std::map<CString, std::map<CString, TLevel*> >::iterator i = groupLevels.begin(); i != groupLevels.end();)
			{
				// Check if any players are found.
				bool playersFound = false;
				for (std::map<CString, TLevel*>::iterator j = (*i).second.begin(); j != (*i).second.end(); ++j)
				{
					TLevel* level = (*j).second;
					if (!level->getPlayerList()->empty())
					{
						playersFound = true;
						break;
					}
				}

				// If no players are found, delete all the levels in this instance.
				if (!playersFound)
				{
					for (std::map<CString, TLevel*>::iterator j = (*i).second.begin(); j != (*i).second.end(); ++j)
					{
						TLevel* level = (*j).second;
						delete level;
					}
					(*i).second.clear();
					groupLevels.erase(i++);
				}
				else ++i;
			}
		}
	}

	return true;
}

bool TServer::onRecv()
{
	// Create socket.
	CSocket *newSock = playerSock.accept();
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
		CFileSystem* fs = getFileSystemByType(type);

		// Add it to the appropriate file system.
		if (fs != 0)
		{
			fs->addDir(dir, wildcard);
			serverlog.out("[%s]        adding %s [%s] to %s\n", name.text(), dir.text(), wildcard.text(), type.text());
		}
		filesystem[0].addDir(dir, wildcard);
	}
}

int TServer::loadConfigFiles()
{
	serverlog.out("[%s] :: Loading server configuration...\n", name.text());

	// Load Settings
	serverlog.out("[%s]      Loading settings...\n", name.text());
	loadSettings();

	// Load Admin Settings
	serverlog.out("[%s]      Loading admin settings...\n", name.text());
	loadAdminSettings();

	// Load allowed versions.
	serverlog.out("[%s]      Loading allowed client versions...\n", name.text());
	loadAllowedVersions();

	// Load folders config and file system.
	serverlog.out("[%s]      Folder config: ", name.text());
	if (settings.getBool("nofoldersconfig", false) == false)
	{
		serverlog.out("ENABLED\n");
	} else serverlog.out("disabled\n");
	serverlog.out("[%s]      Loading file system...\n", name.text());
	loadFileSystem();

	// Load server flags.
	serverlog.out("[%s]      Loading serverflags.txt...\n", name.text());
	loadServerFlags();

	// Load server message.
	serverlog.out("[%s]      Loading config/servermessage.html...\n", name.text());
	loadServerMessage();

	// Load IP bans.
	serverlog.out("[%s]      Loading config/ipbans.txt...\n", name.text());
	loadIPBans();

	// Load weapons.
	serverlog.out("[%s]      Loading weapons...\n", name.text());
	loadWeapons(true);

	// Load maps.
	serverlog.out("[%s]      Loading maps...\n", name.text());
	loadMaps(true);

	// Load translations.
	serverlog.out("[%s]      Loading translations...\n", name.text());
	loadTranslations();

	// Load word filter.
	serverlog.out("[%s]      Loading word filter...\n", name.text());
	loadWordFilter();

	return 0;
}

void TServer::loadSettings()
{
	settings.setSeparator("=");
	settings.loadFile(CString() << serverpath << "config/serveroptions.txt");
	if (!settings.isOpened())
		serverlog.out("[%s] ** [Error] Could not open config/serveroptions.txt.  Will use default config.\n", name.text());

	// Load status list.
	statusList = settings.getStr("playerlisticons", "Online,Away,DND,Eating,Hiding,No PMs,RPing,Sparring,PKing").tokenize(",");
}

void TServer::loadAdminSettings()
{
	adminsettings.setSeparator("=");
	adminsettings.loadFile(CString() << serverpath << "config/adminconfig.txt");
	if (!adminsettings.isOpened())
		serverlog.out("[%s] ** [Error] Could not open config/adminconfig.txt.  Will use default config.\n", name.text());
}

void TServer::loadAllowedVersions()
{
	CString versions;
	versions.load(CString() << serverpath << "config/allowedversions.txt");
	versions = removeComments(versions);
	versions.removeAllI("\r");
	versions.removeAllI("\t");
	versions.removeAllI(" ");
	allowedVersions = versions.tokenize("\n");
	allowedVersionString.clear();
	for (std::vector<CString>::iterator i = allowedVersions.begin(); i != allowedVersions.end(); ++i)
	{
		if (!allowedVersionString.isEmpty())
			allowedVersionString << ", ";

		int loc = (*i).find(":");
		if (loc == -1)
			allowedVersionString << getVersionString(*i, PLTYPE_ANYCLIENT);
		else
		{
			CString s = (*i).subString(0, loc);
			CString f = (*i).subString(loc + 1);
			int vid = getVersionID(s);
			int vid2 = getVersionID(f);
			if (vid != -1 && vid2 != -1)
				allowedVersionString << getVersionString(s, PLTYPE_ANYCLIENT) << " - " << getVersionString(f, PLTYPE_ANYCLIENT);
		}
	}
}

void TServer::loadFileSystem()
{
	for (int i = 0; i < FS_COUNT; ++i)
		filesystem[i].clear();
	filesystem_accounts.clear();
	filesystem_accounts.addDir("accounts");
	if (settings.getBool("nofoldersconfig", false) == true)
		loadAllFolders();
	else
		loadFolderConfig();
}

void TServer::loadServerFlags()
{
	std::vector<CString> lines = CString::loadToken(CString() << serverpath << "serverflags.txt", "\n", true);
	for (std::vector<CString>::const_iterator i = lines.begin(); i != lines.end(); ++i)
		this->setFlag(*i, false);
}

void TServer::loadServerMessage()
{
	servermessage.load(CString() << serverpath << "config/servermessage.html");
	servermessage.removeAllI("\r");
	servermessage.replaceAllI("\n", " ");
}

void TServer::loadIPBans()
{
	ipBans = CString::loadToken(CString() << serverpath << "config/ipbans.txt", "\n", true);
}

void TServer::loadWeapons(bool print)
{
	CFileSystem weaponFS(this);
	weaponFS.addDir("weapons", "weapon*.txt");
	std::map<CString, CString> *weaponFileList = weaponFS.getFileList();
	for (std::map<CString, CString>::iterator i = weaponFileList->begin(); i != weaponFileList->end(); ++i)
	{
		TWeapon *weapon = TWeapon::loadWeapon(i->first, this);
		if (weapon == 0) continue;
		weapon->setModTime(weaponFS.getModTime(i->first));

		// Check if the weapon exists.
		if (weaponList.find(weapon->getName()) == weaponList.end())
		{
			weaponList[weapon->getName()] = weapon;
			if (print) serverlog.out("[%s]        %s\n", name.text(), weapon->getName().text());
		}
		else
		{
			// If the weapon exists, and the version on disk is newer, reload it.
			TWeapon* w = weaponList[weapon->getName()];
			if (w->getModTime() < weapon->getModTime())
			{
				delete w;
				weaponList[weapon->getName()] = weapon;
				if (print) serverlog.out("[%s]        %s [updated]\n", name.text(), weapon->getName().text());
			}
			else
			{
				if (print) serverlog.out("[%s]        %s [skipped]\n", name.text(), weapon->getName().text());
				delete weapon;
			}
		}
	}

	// Add the default weapons.
	if (weaponList.find("bow") == weaponList.end())	weaponList["bow"] = new TWeapon(TLevelItem::getItemId("bow"));
	if (weaponList.find("bomb") == weaponList.end()) weaponList["bomb"] = new TWeapon(TLevelItem::getItemId("bomb"));
	if (weaponList.find("superbomb") == weaponList.end()) weaponList["superbomb"] = new TWeapon(TLevelItem::getItemId("superbomb"));
	if (weaponList.find("fireball") == weaponList.end()) weaponList["fireball"] = new TWeapon(TLevelItem::getItemId("fireball"));
	if (weaponList.find("fireblast") == weaponList.end()) weaponList["fireblast"] = new TWeapon(TLevelItem::getItemId("fireblast"));
	if (weaponList.find("nukeshot") == weaponList.end()) weaponList["nukeshot"] = new TWeapon(TLevelItem::getItemId("nukeshot"));
	if (weaponList.find("joltbomb") == weaponList.end()) weaponList["joltbomb"] = new TWeapon(TLevelItem::getItemId("joltbomb"));
}

void TServer::loadMaps(bool print)
{
	// Remove existing maps.
	for (std::vector<TMap*>::iterator i = mapList.begin(); i != mapList.end(); )
	{
		delete *i;
		i = mapList.erase(i);
	}

	// Load gmaps.
	std::vector<CString> gmaps = settings.getStr("gmaps").guntokenize().tokenize("\n");
	for (std::vector<CString>::iterator i = gmaps.begin(); i != gmaps.end(); ++i)
	{
		// Check for blank lines.
		if (*i == "\r") continue;

		// Load the gmap.
		TMap* gmap = new TMap(MAPTYPE_GMAP);
		if (gmap->load(CString() << *i << ".gmap", this) == false)
		{
			if (print) serverlog.out(CString() << "[" << name << "] " << "** [Error] Could not load " << *i << ".gmap" << "\n");
			delete gmap;
			continue;
		}

		if (print) serverlog.out("[%s]        [gmap] %s\n", name.text(), i->text());
		mapList.push_back(gmap);
	}

	// Load bigmaps.
	std::vector<CString> bigmaps = settings.getStr("maps").guntokenize().tokenize("\n");
	for (std::vector<CString>::iterator i = bigmaps.begin(); i != bigmaps.end(); ++i)
	{
		// Check for blank lines.
		if (*i == "\r") continue;

		// Load the bigmap.
		TMap* bigmap = new TMap(MAPTYPE_BIGMAP);
		if (bigmap->load(*i, this) == false)
		{
			if (print) serverlog.out(CString() << "[" << name << "] " << "** [Error] Could not load " << *i << "\n");
			delete bigmap;
			continue;
		}

		if (print) serverlog.out("[%s]        [bigmap] %s\n", name.text(), i->text());
		mapList.push_back(bigmap);
	}

	// Load group maps.
	std::vector<CString> groupmaps = settings.getStr("groupmaps").guntokenize().tokenize("\n");
	for (std::vector<CString>::iterator i = groupmaps.begin(); i != groupmaps.end(); ++i)
	{
		// Check for blank lines.
		if (*i == "\r") continue;

		// Determine the type of map we are loading.
		CString ext(getExtension(*i));
		ext.toLowerI();

		// Create the new map based on the file extension.
		TMap* gmap = 0;
		if (ext == ".txt")
			gmap = new TMap(MAPTYPE_BIGMAP, true);
		else if (ext == ".gmap")
			gmap = new TMap(MAPTYPE_GMAP, true);
		else continue;

		// Load the map.
		if (gmap->load(CString() << *i, this) == false)
		{
			if (print) serverlog.out(CString() << "[" << name << "] " << "** [Error] Could not load " << *i << "\n");
			delete gmap;
			continue;
		}

		if (print) serverlog.out("[%s]        [group map] %s\n", name.text(), i->text());
		mapList.push_back(gmap);
	}
}

void TServer::loadTranslations()
{
	this->TS_Reload();
}

void TServer::loadWordFilter()
{
	wordFilter.load(CString() << serverpath << "config/rules.txt");
}

void TServer::saveServerFlags()
{
	CString out;
	for (std::map<CString, CString>::iterator i = mServerFlags.begin(); i != mServerFlags.end(); ++i)
		out << i->first << "=" << i->second << "\r\n";
	out.save(CString() << serverpath << "serverflags.txt");
}

void TServer::saveWeapons()
{
	CFileSystem weaponFS(this);
	weaponFS.addDir("weapons", "weapon*.txt");
	std::map<CString, CString> *weaponFileList = weaponFS.getFileList();

	for (std::map<CString, TWeapon *>::iterator i = weaponList.begin(); i != weaponList.end(); ++i)
	{
		TWeapon* w = i->second;
		time_t mod = weaponFS.getModTime(i->first);
		if (w->getModTime() > mod)
		{
			// The weapon in memory is newer than the weapon on disk.  Save it.
			w->saveWeapon(this);
			weaponFS.setModTime(weaponFS.find(i->first), w->getModTime());
		}
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
		if (player->getAccountName().toLower() == account.toLower())
			return player;
	}
	return 0;
}

TPlayer* TServer::getRC(const unsigned short id, bool includePlayer) const
{
	if (id >= (unsigned short)playerIds.size()) return 0;
	if (!includePlayer && playerIds[id]->isClient()) return 0;
	return playerIds[id];
}

TPlayer* TServer::getRC(const CString& account, bool includePlayer) const
{
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		TPlayer *player = (TPlayer*)*i;
		if (player == 0)
			continue;

		if (!includePlayer && player->isClient())
			continue;

		// Compare account names.
		if (player->getAccountName().toLower() == account.toLower())
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

TWeapon* TServer::getWeapon(const CString& name)
{
	return (weaponList.find(name) != weaponList.end() ? weaponList[name] : 0);
}

CString TServer::getFlag(const CString& pFlagName)
{
	return mServerFlags[pFlagName];
}

CFileSystem* TServer::getFileSystemByType(CString& type)
{
	// Find out the filesystem.
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

	if (fs == -1) return 0;
	return &filesystem[fs];
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
		CString packet = CString() >> (char)PLO_NPCPROPS >> (int)newNPC->getId() << newNPC->getProps(0);

		// Send to level.
		TMap* map = getMap(pLevel);
		sendPacketToLevel(packet, map, pLevel, 0, true);

		// Send to npc-server.
		if (mNpcServer != 0)
			mNpcServer->sendPacket(packet);
	}

	return newNPC;
}

bool TServer::deleteNPC(const unsigned int pId, TLevel* pLevel, bool eraseFromLevel)
{
	// Grab the NPC.
	TNPC* npc = getNPC(pId);
	if (npc == 0) return false;

	return deleteNPC(npc, pLevel, eraseFromLevel);
}

bool TServer::deleteNPC(TNPC* npc, TLevel* pLevel, bool eraseFromLevel)
{
	if (npc == 0) return false;
	if (npc->getId() >= npcIds.size()) return false;

	// Remove the NPC from all the lists.
	if (pLevel != 0 && eraseFromLevel) pLevel->removeNPC(npc);
	npcIds[npc->getId()] = 0;

	for (std::vector<TNPC*>::iterator i = npcList.begin(); i != npcList.end(); )
	{
		if ((*i) == npc)
			i = npcList.erase(i);
		else ++i;
	}

	// Tell the client to delete the NPC.
	//sendPacketTo(PLTYPE_ANYCLIENT, CString() >> (char)PLO_NPCDEL2 >> (char)npc->getLevel()->getLevelName().length() << npc->getLevel()->getLevelName() >> (int)npc->getId());
	for (std::vector<TPlayer*>::iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		TPlayer* p = *i;
		if (p->isRC()) continue;

		if (p->getVersion() < CLVER_2_1 && !p->isNPCServer())
			p->sendPacket(CString() >> (char)PLO_NPCDEL >> (int)npc->getId());
		else p->sendPacket(CString() >> (char)PLO_NPCDEL2 >> (char)npc->getLevel()->getLevelName().length() << npc->getLevel()->getLevelName() >> (int)npc->getId());
	}

	// Delete the NPC from memory.
	delete npc;

	return true;
}

bool TServer::deletePlayer(TPlayer* player)
{
	if (player == 0) return true;

	// Remove the player from the serverlist.
	serverlist.remPlayer(player->getAccountName(), player->getType());

	// Add the player to the list of players to delete.
	if (std::find(deletedPlayers.begin(), deletedPlayers.end(), player) == deletedPlayers.end())
		deletedPlayers.push_back(player);

	return true;
}

unsigned int TServer::getNWTime() const
{
	// timevar apparently subtracts 11078 days from time(0) then divides by 5.
	return ((unsigned int)time(0) - 11078 * 24 * 60 * 60) / 5;
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
	TServer: Server Flag Management
*/
bool TServer::deleteFlag(const CString& pFlagName, bool pSendToPlayers)
{
	if (settings.getBool("dontaddserverflags", false) == true)
		return false;

	std::map<CString, CString>::iterator i;
	if ((i = mServerFlags.find(pFlagName)) != mServerFlags.end())
	{
		mServerFlags.erase(i);
		if (pSendToPlayers)
			sendPacketToAll(CString() >> (char)PLO_FLAGDEL << pFlagName);
		return true;
	}

	return false;
}

bool TServer::setFlag(CString pFlag, bool pSendToPlayers)
{
	CString flagName = pFlag.readString("=");
	CString flagValue = pFlag.readString("");
	return this->setFlag(flagName, (flagValue.isEmpty() ? "1" : flagValue), pSendToPlayers);
}

bool TServer::setFlag(const CString& pFlagName, const CString& pFlagValue, bool pSendToPlayers)
{
	if (settings.getBool("dontaddserverflags", false) == true)
		return false;

	// delete flag
	if (pFlagValue.isEmpty())
		return deleteFlag(pFlagName);

	// set flag
	if (settings.getBool("cropflags", true))
	{
		int totalLength = pFlagName.length() + 1 + pFlagValue.length();
		int fixedLength = 223 - 1 - pFlagName.length();
		mServerFlags[pFlagName] = pFlagValue.subString(0, fixedLength);
	}
	else mServerFlags[pFlagName] = pFlagValue;

	if (pSendToPlayers)
		sendPacketToAll(CString() >> (char)PLO_FLAGSET << pFlagName << "=" << pFlagValue);
	return true;
}

/*
	Packet-Sending Functions
*/
void TServer::sendPacketToAll(CString pPacket, TPlayer *pPlayer, bool pNpcServer) const
{
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if ((*i) == pPlayer) continue;
		if (pNpcServer && (*i)->isNPCServer()) continue;

		(*i)->sendPacket(pPacket);
	}
}

void TServer::sendPacketToLevel(CString pPacket, TMap* pMap, TLevel* pLevel, TPlayer* pPlayer, bool onlyGmap) const
{
	if (pMap == 0 || (onlyGmap && pMap->getType() == MAPTYPE_BIGMAP))// || pLevel->isGroupLevel())
	{
		for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
		{
			if ((*i) == pPlayer || !(*i)->isClient()) continue;
			if ((*i)->getLevel() == pLevel)
				(*i)->sendPacket(pPacket);
		}
		return;
	}

	if (pLevel == 0) return;
	bool _groupMap = pPlayer->getMap()->isGroupMap();
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if (!(*i)->isClient() || (*i) == pPlayer) continue;
		if (_groupMap && pPlayer->getGroup() != (*i)->getGroup()) continue;

		if ((*i)->getMap() == pMap)
		{
			int sgmap[2] = {pMap->getLevelX(pLevel->getActualLevelName()), pMap->getLevelY(pLevel->getActualLevelName())};
			int ogmap[2];
			switch (pMap->getType())
			{
				case MAPTYPE_GMAP:
					ogmap[0] = (*i)->getProp(PLPROP_GMAPLEVELX).readGUChar();
					ogmap[1] = (*i)->getProp(PLPROP_GMAPLEVELY).readGUChar();
					break;

				default:
				case MAPTYPE_BIGMAP:
					ogmap[0] = pMap->getLevelX((*i)->getLevel()->getActualLevelName());
					ogmap[1] = pMap->getLevelY((*i)->getLevel()->getActualLevelName());
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

	if (pMap == 0 || (onlyGmap && pMap->getType() == MAPTYPE_BIGMAP) || pPlayer->getLevel()->isSingleplayer() == true)
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

	bool _groupMap = pPlayer->getMap()->isGroupMap();
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if (!(*i)->isClient()) continue;
		if ((*i) == pPlayer)
		{
			if (sendToSelf) pPlayer->sendPacket(pPacket);
			continue;
		}
		if ((*i)->getLevel() == 0) continue;
		if (_groupMap && pPlayer->getGroup() != (*i)->getGroup()) continue;

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
					ogmap[0] = pMap->getLevelX((*i)->getLevel()->getActualLevelName());
					ogmap[1] = pMap->getLevelY((*i)->getLevel()->getActualLevelName());
					sgmap[0] = pMap->getLevelX(pPlayer->getLevel()->getActualLevelName());
					sgmap[1] = pMap->getLevelY(pPlayer->getLevel()->getActualLevelName());
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

/*
	NPC-Server Functionality
*/
void TServer::setNPCServer(TPlayer *pNpcServer, int pNCPort)
{
	mNpcServer = pNpcServer;
	mNCPort = pNCPort;
}

bool TServer::NC_AddWeapon(TWeapon *pWeaponObj)
{
	if (pWeaponObj == 0)
		return false;

	weaponList[pWeaponObj->getName()] = pWeaponObj;
	return true;
}

bool TServer::NC_DelWeapon(const CString& pWeaponName)
{
	// Definitions
	TWeapon *weaponObj = getWeapon(pWeaponName);
	if (weaponObj == 0 || weaponObj->isDefault())
		return false;

	// Delete from File Browser
	CString filePath = getServerPath() << "weapons/weapon" << pWeaponName.replaceAll("*", "@").replaceAll("/", "_") << ".txt";
	CFileSystem::fixPathSeparators(&filePath);
	remove(filePath.text());

	// Delete from Memory
	mapRemove<CString, TWeapon *>(weaponList, weaponObj);
	delete weaponObj;

	// Delete from Players
	sendPacketTo(PLTYPE_ANYCLIENT, CString() >> (char)PLO_NPCWEAPONDEL << pWeaponName); 
	return true;
}

void TServer::NC_UpdateWeapon(TWeapon *pWeapon)
{
	// Update Weapons
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		TPlayer *player = *i;
		if (!player->isClient())
			continue;

		if (player->hasWeapon(pWeapon->getName()))
		{
			player->sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << pWeapon->getName());
			player->sendPacket(CString() << pWeapon->getWeaponPacket());
		}
	}
}

bool TServer::NC_SendLevel(TLevel* level)
{
	if (level == 0 || mNpcServer == 0) return false;

	// Send the level name.
	mNpcServer->sendPacket(CString() >> (char)PLO_LEVELNAME << level->getLevelName());

	// Send links, signs, and mod time.
	mNpcServer->sendPacket(CString() >> (char)PLO_LEVELMODTIME >> (long long)level->getModTime());
	mNpcServer->sendPacket(CString() << level->getLinksPacket());
	mNpcServer->sendPacket(CString() << level->getSignsPacket(0));

	// Send NPCs.
	mNpcServer->sendPacket(CString() << level->getNpcsPacket(0, CLVER_NPCSERVER));
	return true;
}

/*
	Translation Functionality
*/
bool TServer::TS_Load(const CString& pLanguage, const CString& pFileName)
{
	// Load File
	std::vector<CString> fileData = CString::loadToken(pFileName, "\n", true);
	if (fileData.size() == 0)
		return false;

	// Parse File
	std::vector<CString>::const_iterator cur, next;
	for (cur = fileData.begin(); cur != fileData.end(); ++cur)
	{
		if (cur->find("msgid") == 0)
		{
			CString msgId = cur->subString(7, cur->length() - 8);
			CString msgStr = "";
			bool isStr = false;

			++cur;
			while (cur != fileData.end())
			{
				// Make sure our string isn't empty.
				if (cur->isEmpty())
				{
					++cur;
					continue;
				}

				if ((*cur)[0] == '"' && (*cur)[cur->length() - 1] == '"')
				{
					CString str('\n');
					str.write(cur->subString(1, cur->length() - 2));
					(isStr == true ? msgStr.write(str) : msgId.write(str));
				}
				else if (cur->find("msgstr") == 0)
				{
					msgStr = cur->subString(8, cur->length() - 9);
					isStr = true;
				}
				else { --cur; break; }

				++cur;
			}
			
			mTranslationManager.add(pLanguage.text(), msgId.text(), msgStr.text());
		}

		if (cur == fileData.end())
			break;
	}

	return true;
}

CString TServer::TS_Translate(const CString& pLanguage, const CString& pKey)
{
	return mTranslationManager.translate(pLanguage.toLower().text(), pKey.trim().text());
}

void TServer::TS_Reload()
{
	// Save Translations
	this->TS_Save();

	// Reset Translations
	mTranslationManager.reset();

	// Load Translation Folder
	CFileSystem translationFS(this);
	translationFS.addDir("translations", "*.po");

	// Load Each File
	std::map<CString, CString> *temp = translationFS.getFileList();
	for (std::map<CString, CString>::const_iterator i = temp->begin(); i != temp->end(); ++i)
		this->TS_Load(removeExtension(i->first), i->second);
}

void TServer::TS_Save()
{
	// Grab Translations
	std::map<std::string, STRMAP> *languages = mTranslationManager.getTranslationList();
	
	// Iterate each Language
	for (std::map<std::string, STRMAP>::const_iterator i = languages->begin(); i != languages->end(); ++i)
	{
		// Create Output
		CString output;

		// Iterate each Translation
		for (std::map<std::string, std::string>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			output << "msgid ";
			std::vector<CString> sign = CString(j->first.c_str()).removeAll("\r").tokenize("\n");
			for (std::vector<CString>::iterator s = sign.begin(); s != sign.end(); ++s)
				output << "\"" << *s << "\"\r\n";
			output << "msgstr ";
			if (!j->second.empty())
			{
				std::vector<CString> lines = CString(j->second.c_str()).removeAll("\r").tokenize("\n");
				for (std::vector<CString>::const_iterator k = lines.begin(); k != lines.end(); ++k)
					output << "\"" << *k << "\"\r\n";
			}
			else output << "\"\"\r\n";

			output << "\r\n";
		}

		// Save File
		output.trimRight().save(getServerPath() << "translations/" << i->first.c_str() << ".po");
	}
}
