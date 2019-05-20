#include "IDebug.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>

#include "TServer.h"
#include "main.h"
#include "TPlayer.h"
#include "TWeapon.h"
#include "IUtil.h"
#include "TNPC.h"
#include "TMap.h"
#include "TLevel.h"

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

extern std::atomic_bool shutdownProgram;

TServer::TServer(CString pName)
	: running(false), doRestart(false), name(pName), wordFilter(this), mPluginManager(this)
#ifdef V8NPCSERVER
	, mScriptEngine(this)
#endif
#ifdef UPNP
	, upnp(this)
#endif
{
	auto time_now = std::chrono::high_resolution_clock::now();
	lastTimer = lastNWTimer = last1mTimer = last5mTimer = last3mTimer = time_now;
#ifdef V8NPCSERVER
	lastScriptTimer = time_now;
#endif

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

#ifdef V8NPCSERVER
	CString scriptPath = CString() << logpath << "logs/scriptlog.txt";
	CFileSystem::fixPathSeparators(&scriptPath);
	scriptlog.setFilename(scriptPath);
#endif

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

int TServer::init(const CString& serverip, const CString& serverport, const CString& localip, const CString& serverinterface)
{
	// Player ids 0 and 1 break things.  NPC id 0 breaks things.
	// Don't allow anything to have one of those ids.
	// Player ids 16000 and up is used for players on other servers and "IRC"-channels.
	// The players from other servers should be unique lists for each player as they are fetched depending on
	// what the player chooses to see (buddies, "global guilds" tab, "other servers" tab)
	playerIds.resize(2);
	npcIds.resize(10001); // Starting npc ids at 10,000 for now on..

#ifdef V8NPCSERVER
	// Initialize the Script Engine
	if (!mScriptEngine.Initialize())
	{
		serverlog.out("[%s] ** [Error] Could not initialize script engine.\n", name.text());
		// TODO(joey): new error number? log is probably enough
		return ERR_SETTINGS;
	}
#endif

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
	if (!serverinterface.isEmpty())
		settings.addKey("serverinterface", serverinterface);
	overrideIP = serverip;
	overridePort = serverport;
	overrideLocalIP = localip;
	overrideInterface = serverinterface;

	// Fix up the interface to work properly with CSocket.
	CString oInter = overrideInterface;
	if (overrideInterface.isEmpty())
		oInter = settings.getStr("serverinterface");
	if (oInter == "AUTO")
		oInter.clear();

	// Initialize the player socket.
	playerSock.setType(SOCKET_TYPE_SERVER);
	playerSock.setProtocol(SOCKET_PROTOCOL_TCP);
	playerSock.setDescription("playerSock");

	// Start listening on the player socket.
	serverlog.out("[%s]      Initializing player listen socket.\n", name.text());
	if (playerSock.init((oInter.isEmpty() ? 0 : oInter.text()), settings.getStr("serverport").text()))
	{
		serverlog.out("[%s] ** [Error] Could not initialize listening socket...\n", name.text());
		return ERR_LISTEN;
	}
	if (playerSock.connect())
	{
		serverlog.out("[%s] ** [Error] Could not connect listening socket...\n", name.text());
		return ERR_LISTEN;
	}


#ifdef UPNP
	// Start a UPNP thread.  It will try to set a UPNP port forward in the background.
	serverlog.out("[%s]      Starting UPnP discovery thread.\n", name.text());
	upnp.initialize((oInter.isEmpty() ? playerSock.getLocalIp() : oInter.text()), settings.getStr("serverport").text());
	upnp_thread = std::thread(std::ref(upnp));
#endif

#ifdef V8NPCSERVER
	// Setup NPC Control port
	mNCPort = strtoint(settings.getStr("serverport"));
#endif

	// Connect to the serverlist.
	serverlog.out("[%s]      Initializing serverlist socket.\n", name.text());
	if (!serverlist.init(settings.getStr("listip"), settings.getStr("listport")))
	{
		serverlog.out("[%s] ** [Error] Could not initialize serverlist socket.\n", name.text());
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
	running = true;
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
			int ret = init(overrideIP, overridePort, overrideLocalIP, overrideInterface);
			if (ret != 0)
				break;
		}

		if (shutdownProgram)
			running = false;
	}
	cleanup();
}

void TServer::cleanupDeletedPlayers()
{
	if (deletedPlayers.empty()) return;
	for (std::set<TPlayer*>::iterator i = deletedPlayers.begin(); i != deletedPlayers.end();)
	{
		TPlayer* player = *i;
		if (player == 0)
		{
			i = deletedPlayers.erase(i);
			continue;
		}

#ifdef V8NPCSERVER
		IScriptWrapped<TPlayer> *playerObject = player->getScriptObject();
		if (playerObject != 0)
		{
			// Process last script events for this player
			if (!player->isProcessed())
			{
				// Leave the level now while the player object is still alive 
				if (player->getLevel() != 0)
					player->leaveLevel();

				// Send event to server that player is logging out
				for (auto it = npcNameList.begin(); it != npcNameList.end(); ++it)
				{
					TNPC *npcObject = (*it).second;
					npcObject->queueNpcAction("npc.playerlogout", player);
				}

				// Set processed
				player->setProcessed();
			}

			// If we just added events to the player, we will have to wait for them to run before removing player.
			if (playerObject->isReferenced())
			{
				printf("Reference count: %d\n", playerObject->getReferenceCount());
				++i;
				continue;
			}
		}
#endif

		// Get rid of the player now.
		playerIds[player->getId()] = 0;
		for (std::vector<TPlayer*>::iterator j = playerList.begin(); j != playerList.end();)
		{
			TPlayer* p = *j;
			if (p == player)
			{
				// Unregister the player.
				sockManager.unregisterSocket(p);

				delete p;
				j = playerList.erase(j);
			}
			else ++j;
		}

		i = deletedPlayers.erase(i);
	}
	//deletedPlayers.clear();
}

void TServer::cleanup()
{
	// Close our UPNP port forward.
	// First, make sure the thread has completed already.
	// This can cause an issue if the server is about to be deleted.
#ifdef UPNP
	upnp_thread.join();
	upnp.remove_all_forwarded_ports();
#endif

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

#ifdef V8NPCSERVER
	// Clean up the script engine
	mScriptEngine.Cleanup();
#endif

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

	//
	auto currentTimer = std::chrono::high_resolution_clock::now();
	auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimer - lastTimer);

#ifdef V8NPCSERVER
	// Run scripts every 0.05 seconds (incl. catching up)
	time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimer - lastScriptTimer);
	auto time_ms = time_diff.count();
	
	// TODO(joey): maybe run events at any time
	if (time_ms >= 50)
	{
		lastScriptTimer = currentTimer;

		do {
			mScriptEngine.RunScripts(true);
			time_ms -= 50;
		} while (time_ms >= 50);
	}
	else mScriptEngine.RunScripts();
#endif

	// Every second, do some events.
	time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimer - lastTimer);
	if (time_diff.count() >= 1000)
	{
		lastTimer = currentTimer;
		doTimedEvents();
	}
	
	return true;
}

bool TServer::doTimedEvents()
{
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
	auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(lastTimer - lastNWTimer);
	if (time_diff.count() >= 5)
	{
		lastNWTimer = lastTimer;
		sendPacketToAll(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(getNWTime()));
		this->getServerList()->sendPacket(CString() >> (char)SVO_REQUESTLIST >> (short)0 << CString(CString() << "" << "\n" << "GraalEngine" << "\n" << "lister" << "\n" << "simpleserverlist" << "\n").gtokenizeI());
	}

	// Stuff that happens every minute.
	time_diff = std::chrono::duration_cast<std::chrono::seconds>(lastTimer - last1mTimer);
	if (time_diff.count() >= 60)
	{
		last1mTimer = lastTimer;

		// Save server flags.
		this->saveServerFlags();
	}

	// Stuff that happens every 3 minutes.
	time_diff = std::chrono::duration_cast<std::chrono::seconds>(lastTimer - last3mTimer);
	if (time_diff.count() >= 180)
	{
		last3mTimer = lastTimer;

		// Resynchronize the file systems.
		filesystem_accounts.resync();
		for (int i = 0; i < FS_COUNT; ++i)
			filesystem[i].resync();
	}

	// Save stuff every 5 minutes.
	time_diff = std::chrono::duration_cast<std::chrono::seconds>(lastTimer - last5mTimer);
	if (time_diff.count() >= 300)
	{
		last5mTimer = lastTimer;

		// Reload some server settings.
		loadAllowedVersions();
		loadServerMessage();
		loadIPBans();
		
		// Save some stuff.
		saveWeapons();
#ifdef V8NPCSERVER
		saveNpcs();
#endif

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

#ifdef V8NPCSERVER
	mScriptEngine.WrapObject(newPlayer);
#endif

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
		serverlog.append("ENABLED\n");
	} else serverlog.append("disabled\n");
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

	// Load classes.
	serverlog.out("[%s]      Loading classes...\n", name.text());
	loadClasses(true);

	// Load maps.
	serverlog.out("[%s]      Loading maps...\n", name.text());
	loadMaps(true);

#ifdef V8NPCSERVER
	// Load database npcs.
	serverlog.out("[%s]      Loading npcs...\n", name.text());
	loadNpcs(true);
#endif

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

	// Send our ServerHQ info in case we got changed the staffonly setting.
	getServerList()->sendServerHQ();
}

void TServer::loadAdminSettings()
{
	adminsettings.setSeparator("=");
	adminsettings.loadFile(CString() << serverpath << "config/adminconfig.txt");
	if (!adminsettings.isOpened())
		serverlog.out("[%s] ** [Error] Could not open config/adminconfig.txt.  Will use default config.\n", name.text());
	else getServerList()->sendServerHQ();
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

void TServer::loadClasses(bool print)
{
	CFileSystem scriptFS(this);
	scriptFS.addDir("scripts", "*.txt");
	std::map<CString, CString> *scriptFileList = scriptFS.getFileList();
	for (auto it = scriptFileList->begin(); it != scriptFileList->end(); ++it)
	{
		std::string className = it->first.subString(0, it->first.length() - 4).text();

		CString scriptData;
		scriptData.load(it->second);
		classList[className] = scriptData.text();
	}
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
				NC_UpdateWeapon(weapon);
				if (print) serverlog.out("[%s]        %s [updated]\n", name.text(), weapon->getName().text());
			}
			else
			{
				// TODO(joey): even though were deleting the weapon because its skipped, its still queuing its script action
				//	and attempting to execute it. Technically the code needs to be run again though, will fix soon.
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
		TMap* map = *i;
		for (std::vector<TPlayer*>::iterator j = playerList.begin(); j != playerList.end(); ++j)
		{
			if ((*j)->getMap() == map)
				(*j)->setMap(0);
		}
		delete map;
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
		if (bigmap->load((*i).trim(), this) == false)
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

#ifdef V8NPCSERVER
void TServer::loadNpcs(bool print)
{
	CFileSystem npcFS(this);
	npcFS.addDir("npcs", "npc*.txt");
	std::map<CString, CString> *npcFileList = npcFS.getFileList();
	for (auto it = npcFileList->begin(); it != npcFileList->end(); ++it)
	{
		bool loaded = false;

		// Create the npc
		TNPC *newNPC = new TNPC("", "", 30, 30.5, this, nullptr, false);
		if (newNPC->loadNPC((*it).second))
		{
			int npcId = newNPC->getId();
			if (npcId < 1000)
			{
				printf("Database npcs must be greater than 1000\n");
			}
			else if (npcId < npcIds.size() && npcIds[npcId] != 0)
			{
				printf("Error creating database npc: Id is in use!\n");
			}
			else
			{
				// Assign id to npc
				if (npcIds.size() <= npcId)
					npcIds.resize((size_t)npcId + 10);

				npcIds[npcId] = newNPC;
				npcList.push_back(newNPC);
				assignNPCName(newNPC, newNPC->getName());

				// Add npc to level
				TLevel *level = newNPC->getLevel();
				if (level)
					level->addNPC(newNPC);

				loaded = true;
			}
		}

		if (!loaded)
			delete newNPC;
	}
}
#endif

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
	for (auto i = mServerFlags.begin(); i != mServerFlags.end(); ++i)
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

#ifdef V8NPCSERVER

void TServer::saveNpcs()
{
	for (auto it = npcList.begin(); it != npcList.end(); ++it)
	{
		TNPC *npc = *it;
		if (npc->getPersist())
			npc->saveNPC();
	}
}

#endif

/////////////////////////////////////////////////////

TPlayer* TServer::getPlayer(const unsigned short id, bool includeRC) const
{
	if (id >= (unsigned short)playerIds.size()) return 0;
	if (!includeRC && playerIds[id]->isRemoteClient()) return 0;
	return playerIds[id];
}

TPlayer* TServer::getPlayer(const CString& account, bool includeRC) const
{
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		TPlayer *player = (TPlayer*)*i;
		if (player == 0)
			continue;

		if (!includeRC && player->isRemoteClient())
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

CString TServer::getFlag(const std::string& pFlagName)
{
#ifdef V8NPCSERVER
	if (mServerFlags.find(pFlagName) != mServerFlags.end())
		return mServerFlags[pFlagName];
	return "";
#else
	return mServerFlags[pFlagName];
#endif
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

#ifdef V8NPCSERVER
void TServer::assignNPCName(TNPC *npc, const std::string& name)
{
	std::string newName = name;
	int num = 0;
	while (npcNameList.find(newName) != npcNameList.end())
		newName = name + std::to_string(++num);

	npc->setName(newName);
	npcNameList[newName] = npc;
}

void TServer::removeNPCName(TNPC *npc)
{
	auto npcIter = npcNameList.find(npc->getName());
	if (npcIter != npcNameList.end())
		npcNameList.erase(npcIter);
}

TNPC* TServer::addServerNpc(int npcId, float pX, float pY, TLevel *pLevel, bool sendToPlayers)
{
	// Force database npc ids to be >= 1000
	if (npcId < 1000)
	{
		printf("Database npcs need to be greater than 1000\n");
		return nullptr;
	}

	// Make sure the npc id isn't in use
	if (npcId < npcIds.size() && npcIds[npcId] != 0)
	{
		printf("Error creating database npc: Id is in use!\n");
		return nullptr;
	}

	// Create the npc
	TNPC* newNPC = new TNPC("", "", pX, pY, this, pLevel, false);
	newNPC->setId(npcId);
	npcList.push_back(newNPC);

	if (npcIds.size() <= npcId)
		npcIds.resize((size_t)npcId + 10);
	npcIds[npcId] = newNPC;

	// Add the npc to the level
	if (pLevel)
		pLevel->addNPC(newNPC);

	// Send the NPC's props to everybody in range.
	if (sendToPlayers)
	{
		CString packet = CString() >> (char)PLO_NPCPROPS >> (int)newNPC->getId() << newNPC->getProps(0);

		// Send to level.
		TMap* map = getMap(pLevel);
		sendPacketToLevel(packet, map, pLevel, 0, true);
	}

	return newNPC;
}
#endif

TNPC* TServer::addNPC(const CString& pImage, const CString& pScript, float pX, float pY, TLevel* pLevel, bool pLevelNPC, bool sendToPlayers)
{
	// New Npc
	TNPC* newNPC = new TNPC(pImage, pScript, pX, pY, this, pLevel, pLevelNPC);
	npcList.push_back(newNPC);

	// Assign NPC Id
	bool assignedId = false;
	for (unsigned int i = 10000; i < npcIds.size(); ++i)
	{
		if (npcIds[i] == 0)
		{
			npcIds[i] = newNPC;
			newNPC->setId(i);
			assignedId = true;
			break;
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
	}
	
	return newNPC;
}

bool TServer::deleteNPC(const unsigned int pId, bool eraseFromLevel)
{
	// Grab the NPC.
	TNPC* npc = getNPC(pId);
	if (npc == 0) return false;

	return deleteNPC(npc, eraseFromLevel);
}

bool TServer::deleteNPC(TNPC* npc, bool eraseFromLevel)
{
	if (npc == 0) return false;
	if (npc->getId() >= npcIds.size()) return false;

	TLevel *level = npc->getLevel();

	// Remove the NPC from all the lists.
	if (level != nullptr && eraseFromLevel)
		level->removeNPC(npc);
	npcIds[npc->getId()] = 0;
		
	for (std::vector<TNPC*>::iterator i = npcList.begin(); i != npcList.end(); )
	{
		if ((*i) == npc)
			i = npcList.erase(i);
		else ++i;
	}

	// Tell the client to delete the NPC.
	bool isOnMap = (level && level->getMap() ? true : false);
	CString tmpLvlName = (isOnMap ? npc->getLevel()->getMap()->getMapName() : npc->getLevel()->getLevelName());

	for (std::vector<TPlayer*>::iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		TPlayer* p = *i;
		if (p->isRemoteClient()) continue;

		if (isOnMap || p->getVersion() < CLVER_2_1)
			p->sendPacket(CString() >> (char)PLO_NPCDEL >> (int)npc->getId());
		else
			p->sendPacket(CString() >> (char)PLO_NPCDEL2 >> (char)tmpLvlName.length() << tmpLvlName >> (int)npc->getId());
	}

#ifdef V8NPCSERVER
	// TODO(joey): Need to deal with illegal characters
	// If we persist this npc, delete the file
	if (npc->getPersist())
	{
		CString filePath = getServerPath() << "npcs/npc" << npc->getName() << ".txt";
		CFileSystem::fixPathSeparators(&filePath);
		remove(filePath.text());
	}

	// Remove npc name assignment
	if (!npc->isLevelNPC() && !npc->getName().empty())
		removeNPCName(npc);
#endif

	// Delete the NPC from memory.
	delete npc;

	return true;
}

bool TServer::deleteClass(const std::string& className)
{
	auto classIter = classList.find(className);
	if (classIter == classList.end())
		return false;

	classList.erase(classIter);
	CString filePath = getServerPath() << "scripts/" << className << ".txt";
	CFileSystem::fixPathSeparators(&filePath);
	remove(filePath.text());

	return true;
}

void TServer::updateClass(const std::string& className, const std::string& classCode)
{
	// TODO(joey): filenames...
	classList[className] = classCode;

	CString filePath = getServerPath() << "scripts/" << className << ".txt";
	CFileSystem::fixPathSeparators(&filePath);
	
	CString fileData(classCode);
	fileData.save(filePath);
}

bool TServer::deletePlayer(TPlayer* player)
{
	if (player == 0)
		return true;

	// Add the player to the set of players to delete.
	if (deletedPlayers.insert(player).second == true)
	{
		// Remove the player from the serverlist.
		serverlist.remPlayer(player->getAccountName(), player->getType());
	}

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

void TServer::playerLoggedIn(TPlayer *player)
{
#ifdef V8NPCSERVER
	// Send event to server that player is logging out
	for (auto it = npcNameList.begin(); it != npcNameList.end(); ++it)
	{
		TNPC *npcObject = (*it).second;
		npcObject->queueNpcAction("npc.playerlogin", player);
	}
#endif
}

/*
	TServer: Server Flag Management
*/
bool TServer::deleteFlag(const std::string& pFlagName, bool pSendToPlayers)
{
	if (settings.getBool("dontaddserverflags", false) == true)
		return false;

	std::unordered_map<std::string, CString>::iterator i;
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
	std::string flagName = pFlag.readString("=").text();
	CString flagValue = pFlag.readString("");
	return this->setFlag(flagName, (flagValue.isEmpty() ? "1" : flagValue), pSendToPlayers);
}

bool TServer::setFlag(const std::string& pFlagName, const CString& pFlagValue, bool pSendToPlayers)
{
	if (settings.getBool("dontaddserverflags", false) == true)
		return false;

	// delete flag
	if (pFlagValue.isEmpty())
		return deleteFlag(pFlagName);

	// optimize
	if (mServerFlags[pFlagName] == pFlagValue)
		return true;

	// set flag
	if (settings.getBool("cropflags", true))
	{
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
	bool _groupMap = (pPlayer == 0 ? false : pPlayer->getMap()->isGroupMap());
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		TPlayer* other = *i;
		if (!other->isClient() || other == pPlayer || other->getLevel() == 0) continue;
		if (_groupMap && pPlayer != 0 && pPlayer->getGroup() != other->getGroup()) continue;

		if (other->getMap() == pMap)
		{
			int sgmap[2] = {pMap->getLevelX(pLevel->getActualLevelName()), pMap->getLevelY(pLevel->getActualLevelName())};
			int ogmap[2];
			switch (pMap->getType())
			{
				case MAPTYPE_GMAP:
					ogmap[0] = other->getProp(PLPROP_GMAPLEVELX).readGUChar();
					ogmap[1] = other->getProp(PLPROP_GMAPLEVELY).readGUChar();
					break;

				default:
				case MAPTYPE_BIGMAP:
					ogmap[0] = pMap->getLevelX(other->getLevel()->getActualLevelName());
					ogmap[1] = pMap->getLevelY(other->getLevel()->getActualLevelName());
					break;
			}

			if (abs(ogmap[0] - sgmap[0]) < 2 && abs(ogmap[1] - sgmap[1]) < 2)
				other->sendPacket(pPacket);
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
	CString name = pWeaponName;
	name.replaceAllI("\\", "_");
	name.replaceAllI("/", "_");
	name.replaceAllI("*", "@");
	name.replaceAllI(":", ";");
	name.replaceAllI("?", "!");
	CString filePath = getServerPath() << "weapons/weapon" << name << ".txt";
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
	return mTranslationManager.translate(pLanguage.toLower().text(), pKey.text());
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
