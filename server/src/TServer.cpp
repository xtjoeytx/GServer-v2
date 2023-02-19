#include "IDebug.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>

#include <fmt/format.h>

#include "TServer.h"
#include "main.h"
#include "TPlayer.h"
#include "TWeapon.h"
#include "IUtil.h"
#include "TNPC.h"
#include "TMap.h"
#include "TLevel.h"
#include "ScriptOrigin.h"

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

template<class T, class R, class...Args>
auto methodstub(T* t, R(T::*m)(Args...)) {
	return [=](auto&&...args)->R {
		return (t->*m)(decltype(args)(args)...);
	};
}

// I don't want to deal with adding this to the gs2lib.
CString operator<<(const CString& first, const CString& second)
{
	CString result{ first };
	return result << second;
}


TServer::TServer(const CString& pName)
	: running(false), doRestart(false), name(pName), serverlist(this), wordFilter(this), animationManager(this), packageManager(this), serverStartTime(0),
	triggerActionDispatcher(methodstub(this, &TServer::createTriggerCommands))
#ifdef V8NPCSERVER
	, mScriptEngine(this)
#endif
#ifdef UPNP
	, upnp(this)
#endif
{
	auto time_now = std::chrono::high_resolution_clock::now();
	lastTimer = lastNWTimer = last1mTimer = last5mTimer = last3mTimer = time_now;
	calculateServerTime();

	// Player ids 0 and 1 break things.  NPC id 0 breaks things.
	// Don't allow anything to have one of those ids.
	// Player ids 16000 and up is used for players on other servers and "IRC"-channels.
	// The players from other servers should be unique lists for each player as they are fetched depending on
	// what the player chooses to see (buddies, "global guilds" tab, "other servers" tab)
	nextPlayerId = 2;
	nextNpcId = 10001;

	// This has the full path to the server directory.
	serverpath = CString() << getBaseHomePath() << "servers/" << name << "/";
	CFileSystem::fixPathSeparators(serverpath);

	// Set up the log files.
	CString logpath = serverpath.remove(0, getBaseHomePath().length());
	CString npcPath = CString() << logpath << "logs/npclog.txt";
	CString rcPath = CString() << logpath << "logs/rclog.txt";
	CString serverPath = CString() << logpath << "logs/serverlog.txt";
	CFileSystem::fixPathSeparators(npcPath);
	CFileSystem::fixPathSeparators(rcPath);
	CFileSystem::fixPathSeparators(serverPath);
	npclog.setFilename(npcPath);
	rclog.setFilename(rcPath);
	serverlog.setFilename(serverPath);

#ifdef V8NPCSERVER
	CString scriptPath = CString() << logpath << "logs/scriptlog.txt";
	CFileSystem::fixPathSeparators(scriptPath);
	scriptlog.setFilename(scriptPath);
#endif

	// Announce ourself to other classes.
	for (auto & fs : filesystem) {
		fs.setServer(this);
	}
	filesystem_accounts.setServer(this);
}

TServer::~TServer()
{
	cleanup();
}

int TServer::init(const CString& serverip, const CString& serverport, const CString& localip, const CString& serverinterface)
{
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

	mNpcServer = std::make_shared<TPlayer>(this, nullptr, 0);
	mNpcServer->setType(PLTYPE_NPCSERVER);
	mNpcServer->loadAccount("(npcserver)");
	mNpcServer->setHeadImage(settings.getStr("staffhead", "head25.png"));
	mNpcServer->setLoaded(true);	// can't guarantee this, so forcing it

	// TODO(joey): Update this when server options is changed?
	// Set nickname, and append (Server) - required!
	CString nickName = settings.getStr("nickname");
	if (nickName.isEmpty())
		nickName = "NPC-Server";
	nickName << " (Server)";
	mNpcServer->setNick(nickName, true);

	// Add npc-server to playerlist
	addPlayer(mNpcServer);
#endif

	// Register ourself with the socket manager.
	sockManager.registerSocket((CSocketStub*)this);

	serverStartTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
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
	for (auto i = std::begin(deletedPlayers); i != std::end(deletedPlayers);)
	{
		auto& player = *i;

#ifdef V8NPCSERVER
		IScriptObject<TPlayer> *playerObject = player->getScriptObject();
		if (playerObject != nullptr)
		{
			// Process last script events for this player
			if (!player->isProcessed())
			{
				// Leave the level now while the player object is still alive
				if (player->getLevel() != nullptr)
					player->leaveLevel();

				// Send event to server that player is logging out
				if (player->isLoaded() && (player->getType() & PLTYPE_ANYPLAYER))
				{
					for (const auto& [npcName, npcPtr] : npcNameList)
					{
						if (auto npcObject = npcPtr.lock(); npcObject)
							npcObject->queueNpcAction("npc.playerlogout", player.get());
					}
				}

				// Set processed
				player->setProcessed();
			}

			// If we just added events to the player, we will have to wait for them to run before removing player.
			if (playerObject->isReferenced())
			{
				SCRIPTENV_D("Reference count: %d\n", playerObject->getReferenceCount());
				++i;
				continue;
			}
		}
#endif

		// Get rid of the player now.
		freePlayerIds.insert(player->getId());
		sockManager.unregisterSocket(player.get());
		playerList.erase(player->getId());
		player->cleanup();

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
	if (upnp_thread.joinable())
		upnp_thread.join();
	upnp.remove_all_forwarded_ports();
#endif

	// Save translations.
	this->TS_Save();

	// Save server flags.
	saveServerFlags();

#ifdef V8NPCSERVER
	// Save npcs
	saveNpcs();

	// npc-server will be cleared from playerlist, so lets invalidate the pointer here
	mNpcServer = nullptr;
#endif

	for (auto &[id, player]: playerList)
		player->cleanup();

	playerList.clear();
	deletedPlayers.clear();
	freePlayerIds.clear();
	nextPlayerId = 2;

	levelList.clear();
	mapList.clear();
	groupLevels.clear();

    npcList.clear();
	npcNameList.clear();
	freeNpcIds.clear();
	//nextNpcIdLevel = 1;
	//nextNpcIdGlobal = 10001;
	nextNpcId = 10001;

	saveWeapons();
	weaponList.clear();

#ifdef V8NPCSERVER
	// Clean up the script engine
	mScriptEngine.Cleanup();
#endif

	playerSock.disconnect();
	serverlist.getSocket().disconnect();

	// Clean up the socket manager.  Pass false so we don't cause a crash.
	sockManager.cleanup(false);
}

void TServer::restart()
{
	doRestart = true;
}

bool TServer::doMain()
{
	// Update our socket manager.
	sockManager.update(0, 5000);		// 5ms

	// Current time
	auto currentTimer = std::chrono::high_resolution_clock::now();

#ifdef V8NPCSERVER
    mScriptEngine.RunScripts(currentTimer);

	// enable when we switch to async compiling
	//gs2ScriptManager.runQueue();
#endif

	// Every second, do some events.
	auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimer - lastTimer);
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
		for (auto& [id, player] : playerList)
		{
			assert(player);
            if (!player->isNPCServer())
            {
                if (!player->doTimedEvents())
                    this->deletePlayer(player);
            }
		}
	}

	// Do level events.
	{
		for (auto& level : levelList)
		{
            assert(level);
			level->doTimedEvents();
		}

		// Group levels.
		for (auto& [group, levelPtr] : groupLevels)
		{
			if (auto level = levelPtr.lock(); level)
				level->doTimedEvents();
		}
	}

	// Send NW time.
	auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(lastTimer - lastNWTimer);
	if (time_diff.count() >= 5)
	{
		calculateServerTime();

		lastNWTimer = lastTimer;
        sendPacketToAll(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(getNWTime()));
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

		// TODO(joey): probably a better way to do this..

		// Resynchronize the file systems.
		filesystem_accounts.resync();
		for (auto & i : filesystem)
			i.resync();
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
		// TODO(joey): Is this really needed? We save weapons to disk when it is updated or created anyway..
		saveWeapons();
#ifdef V8NPCSERVER
		saveNpcs();
#endif

		// Check all of the instanced maps to see if the players have left.
		if (!groupLevels.empty())
		{
			std::unordered_set<std::string> groupKeys;
			std::for_each(std::begin(groupLevels), std::end(groupLevels), [&groupKeys](auto& pair) { groupKeys.insert(pair.first); });

			for (auto& groupName : groupKeys)
			{
				bool playersFound = false;
				auto range = groupLevels.equal_range(groupName);
				std::for_each(range.first, range.second, [&playersFound](decltype(groupLevels)::value_type& pair)
				{
					if (auto level = pair.second.lock(); level && !level->getPlayerList().empty())
						playersFound = true;
				});

				if (!playersFound)
				{
					groupLevels.erase(groupName);
				}
			}
		}
	}

	return true;
}

bool TServer::onRecv()
{
	// Create socket.
	CSocket *newSock = playerSock.accept();
	if (newSock == nullptr)
		return true;

	// Create the new player.
	auto newPlayer = std::make_shared<TPlayer>(this, newSock, 0);

	// Add the player to the server
	if (!addPlayer(newPlayer))
		return false;

	// Add them to the socket manager.
	sockManager.registerSocket((CSocketStub*)newPlayer.get());

	return true;
}

/////////////////////////////////////////////////////

void TServer::loadAllFolders()
{
	for (auto & fs : filesystem)
		fs.clear();

	filesystem[0].addDir("world");
	if (settings.getStr("sharefolder").length() > 0)
	{
		std::vector<CString> folders = settings.getStr("sharefolder").tokenize(",");
		for (auto & folder : folders)
			filesystem[0].addDir(folder.trim());
	}
}

void TServer::loadFolderConfig()
{
	for (auto & i : filesystem)
		i.clear();

	foldersConfig = CString::loadToken(CString() << serverpath << "config/foldersconfig.txt", "\n", true);
	for (auto & configLine : foldersConfig)
	{
		// No comments.
		int cLoc = -1;
		if ((cLoc = configLine.find("#")) != -1)
			configLine.removeI(cLoc);
		configLine.trimI();
		if ( configLine.length() == 0) continue;

		// Parse the line.
		CString type = configLine.readString(" ");
		CString config = configLine.readString("");
		type.trimI();
		config.trimI();
		CFileSystem::fixPathSeparators(config);

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
		if (fs != nullptr)
		{
			fs->addDir(dir, wildcard);
			serverlog.out("[%s]        adding %s [%s] to %s\n", name.text(), dir.text(), wildcard.text(), type.text());
		}
		filesystem[0].addDir(dir, wildcard);
	}
}

int TServer::loadConfigFiles()
{
	// TODO(joey): /reloadconfig reloads this, but things like server flags, weapons and npcs probably shouldn't be reloaded.
	//	Move them out of here?
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
	if ( !settings.getBool("nofoldersconfig", false))
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

	// Load map levels - doing this after db npcs are loaded incase
	// some level scripts may require access to the databases.
	loadMapLevels();

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
	if (!settings.isOpened())
	{
		settings.setSeparator("=");
		settings.loadFile(CString() << serverpath << "config/serveroptions.txt");
		if ( !settings.isOpened())
			serverlog.out("[%s] ** [Error] Could not open config/serveroptions.txt.  Will use default config.\n", name.text());
	}

	// Load status list.
	statusList = settings.getStr("playerlisticons", "Online,Away,DND,Eating,Hiding,No PMs,RPing,Sparring,PKing").tokenize(",");

	// Load staff list
	staffList = settings.getStr("staff").tokenize(",");

	// Send our ServerHQ info in case we got changed the staffonly setting.
	getServerList().sendServerHQ();
}

void TServer::loadAdminSettings()
{
	adminsettings.setSeparator("=");
	adminsettings.loadFile(CString() << serverpath << "config/adminconfig.txt");
	if (!adminsettings.isOpened())
		serverlog.out("[%s] ** [Error] Could not open config/adminconfig.txt.  Will use default config.\n", name.text());
	else getServerList().sendServerHQ();
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
	for (auto & allowedVersion : allowedVersions)
	{
		if (!allowedVersionString.isEmpty())
			allowedVersionString << ", ";

		int loc = allowedVersion.find(":");
		if (loc == -1)
			allowedVersionString << getVersionString(allowedVersion, PLTYPE_ANYCLIENT);
		else
		{
			CString s = allowedVersion.subString(0, loc);
			CString f = allowedVersion.subString(loc + 1);
			int vid = getVersionID(s);
			int vid2 = getVersionID(f);
			if (vid != -1 && vid2 != -1)
				allowedVersionString << getVersionString(s, PLTYPE_ANYCLIENT) << " - " << getVersionString(f, PLTYPE_ANYCLIENT);
		}
	}
}

void TServer::loadFileSystem()
{
	for (auto & i : filesystem)
		i.clear();
	filesystem_accounts.clear();
	filesystem_accounts.addDir("accounts", "*.txt");
	if ( settings.getBool("nofoldersconfig", false))
		loadAllFolders();
	else
		loadFolderConfig();
}

void TServer::loadServerFlags()
{
	std::vector<CString> lines = CString::loadToken(CString() << serverpath << "serverflags.txt", "\n", true);
	for (auto & line : lines)
		this->setFlag(line, false);
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
	const std::map<CString, CString> &scriptFileList = scriptFS.getFileList();
	for (auto & scriptFile : scriptFileList)
	{
		std::string className = scriptFile.first.subString(0, scriptFile.first.length() - 4).text();

		CString scriptData;
		scriptData.load(scriptFile.second);
		classList[className] = std::make_unique<TScriptClass>(this, className, scriptData.text());

		updateClassForPlayers(getClass(className));
	}
}

void TServer::loadWeapons(bool print)
{
	CFileSystem weaponFS(this);
	weaponFS.addDir("weapons", "weapon*.txt");
	CFileSystem bcweaponFS(this);
	bcweaponFS.addDir("weapon_bytecode", "*");

	auto& weaponFileList = weaponFS.getFileList();
	for (auto& weaponFile : weaponFileList)
	{
		auto weapon = TWeapon::loadWeapon(weaponFile.first, this);
		if (weapon == nullptr) continue;
		if (weapon->getByteCodeFile().empty())
			weapon->setModTime(weaponFS.getModTime(weaponFile.first));
		else
			weapon->setModTime(bcweaponFS.getModTime(weapon->getByteCodeFile()));

		// Check if the weapon exists.
		if (weaponList.find(weapon->getName()) == weaponList.end())
		{
			weaponList[weapon->getName()] = weapon;
			if (print) serverlog.out("[%s]        %s\n", name.text(), weapon->getName().c_str());
		}
		else
		{
			// If the weapon exists, and the version on disk is newer, reload it.
			auto& w = weaponList[weapon->getName()];
			if (w->getModTime() < weapon->getModTime())
			{
				weaponList[weapon->getName()] = weapon;
				updateWeaponForPlayers(weapon);
				if (print)
				{
					serverlog.out("[%s]        %s [updated]\n", name.text(), weapon->getName().c_str());
					TServer::sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: Updated weapon " << weapon->getName() << " ");
				}
			}
			else
			{
				// TODO(joey): even though were deleting the weapon because its skipped, its still queuing its script action
				//	and attempting to execute it. Technically the code needs to be run again though, will fix soon.
				if (print) serverlog.out("[%s]        %s [skipped]\n", name.text(), weapon->getName().c_str());
			}
		}
	}

	// Add the default weapons.
	if (!weaponList.contains("bow"))	weaponList["bow"] = std::make_shared<TWeapon>(this, TLevelItem::getItemId("bow"));
	if (!weaponList.contains("bomb")) weaponList["bomb"] = std::make_shared<TWeapon>(this, TLevelItem::getItemId("bomb"));
	if (!weaponList.contains("superbomb")) weaponList["superbomb"] = std::make_shared<TWeapon>(this, TLevelItem::getItemId("superbomb"));
	if (!weaponList.contains("fireball")) weaponList["fireball"] = std::make_shared<TWeapon>(this, TLevelItem::getItemId("fireball"));
	if (!weaponList.contains("fireblast")) weaponList["fireblast"] = std::make_shared<TWeapon>(this, TLevelItem::getItemId("fireblast"));
	if (!weaponList.contains("nukeshot")) weaponList["nukeshot"] = std::make_shared<TWeapon>(this, TLevelItem::getItemId("nukeshot"));
	if (!weaponList.contains("joltbomb")) weaponList["joltbomb"] = std::make_shared<TWeapon>(this, TLevelItem::getItemId("joltbomb"));
}

void TServer::loadMapLevels()
{
	// Load gmap levels based on options provided by the gmap file
	for (const auto& map : mapList)
	{
        if (map->getType() == MapType::GMAP)
			map->loadMapLevels(this);
	}
}

void TServer::loadMaps(bool print)
{
	// Remove players off all maps
	for (auto& [id, player] : playerList)
		player->setMap(nullptr);

	// Remove existing maps.
	mapList.clear();

	// Load gmaps.
	std::vector<CString> gmaps = settings.getStr("gmaps").guntokenize().tokenize("\n");
	for (CString & gmapName : gmaps)
	{
		// Check for blank lines.
		if ( gmapName == "\r") continue;

		// Gmaps in server options don't need the .gmap suffix, so we will add the suffix
		if (gmapName.right(5) != ".gmap") {
			gmapName << ".gmap";
		}

		// Load the gmap.
		auto gmap = std::make_unique<TMap>(MapType::GMAP);
		if ( !gmap->load(CString() << gmapName, this))
		{
			if (print) serverlog.out(CString() << "[" << name << "] " << "** [Error] Could not load " << gmapName << ".gmap" << "\n");
			continue;
		}

		if (print) serverlog.out("[%s]        [gmap] %s\n", name.text(), gmapName.text());
		mapList.push_back(std::move(gmap));
	}

	// Load bigmaps.
	std::vector<CString> bigmaps = settings.getStr("maps").guntokenize().tokenize("\n");
	for (auto & i : bigmaps)
	{
		// Check for blank lines.
		if (i == "\r") continue;

		// Load the bigmap.
		auto bigmap = std::make_unique<TMap>(MapType::BIGMAP);
		if ( !bigmap->load(i.trim(), this))
		{
			if (print) serverlog.out(CString() << "[" << name << "] " << "** [Error] Could not load " << i << "\n");
			continue;
		}

		if (print) serverlog.out("[%s]        [bigmap] %s\n", name.text(), i.text());
		mapList.push_back(std::move(bigmap));
	}

	// Load group maps.
	std::vector<CString> groupmaps = settings.getStr("groupmaps").guntokenize().tokenize("\n");
	for (auto & groupmap : groupmaps)
	{
		// Check for blank lines.
		if (groupmap == "\r") continue;

		// Determine the type of map we are loading.
		CString ext(getExtension(groupmap));
		ext.toLowerI();

		// Create the new map based on the file extension.
		std::unique_ptr<TMap> gmap;
		if (ext == ".txt")
			gmap = std::make_unique<TMap>(MapType::BIGMAP, true);
		else if (ext == ".gmap")
			gmap = std::make_unique<TMap>(MapType::GMAP, true);
		else continue;

		// Load the map.
		if ( !gmap->load(CString() << groupmap, this))
		{
			if (print) serverlog.out(CString() << "[" << name << "] " << "** [Error] Could not load " << groupmap << "\n");
			continue;
		}

		if (print) serverlog.out("[%s]        [group map] %s\n", name.text(), groupmap.text());
		mapList.push_back(std::move(gmap));
	}

	// Update all map <--> level relationships
	for (const auto& level : levelList)
	{
		bool found = false;
		for (const auto& map : mapList)
		{
			int mx, my;
			if (map->isLevelOnMap(level->getLevelName().toLower().text(), mx, my))
			{
				level->setMap(map, mx, my);
				found = true;
				break;
			}
		}

		if (!found) {
			level->setMap({});
		}
	}
}

#ifdef V8NPCSERVER
void TServer::loadNpcs(bool print)
{
	CFileSystem npcFS(this);
	npcFS.addDir("npcs", "npc*.txt");

	auto& npcFileList = npcFS.getFileList();
	for (const auto& [npcName, fileName] : npcFileList)
	{
		bool loaded = false;

		// Create the npc
		auto newNPC = std::make_shared<TNPC>("", "", 30.f, 30.5f, this, nullptr, NPCType::DBNPC);
		if (newNPC->loadNPC(fileName))
		{
			int npcId = newNPC->getId();
			if (npcId < 1000)
			{
				printf("Database npcs must be greater than 1000\n");
			}
			else if (auto existing = npcList.find(npcId); existing != std::end(npcList))
			{
				printf("Error creating database npc: Id is in use!\n");
			}
			else
			{
				npcList.insert(std::make_pair(npcId, newNPC));
				assignNPCName(newNPC, newNPC->getName());

				// Add npc to level
				if (auto level = newNPC->getLevel(); level)
					level->addNPC(newNPC);

				loaded = true;
			}
		}
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
	for (auto & mServerFlag : mServerFlags)
		out << mServerFlag.first << "=" << mServerFlag.second << "\r\n";
	out.save(CString() << serverpath << "serverflags.txt");
}

void TServer::saveWeapons()
{
	CFileSystem weaponFS(this);
	weaponFS.addDir("weapons", "weapon*.txt");
	const std::map<CString, CString>& weaponFileList = weaponFS.getFileList();

	for (auto& [weaponName, weapon] : weaponList)
	{
		if (weapon->isDefault())
			continue;

		// TODO(joey): add a function to weapon to get the filename?
		CString weaponFile = CString("weapon") << weaponName << ".txt";
		time_t mod = weaponFS.getModTime(weaponFile);
		if (weapon->getModTime() > mod)
		{
			// The weapon in memory is newer than the weapon on disk.  Save it.
			weapon->saveWeapon();
			weaponFS.setModTime(weaponFS.find(weaponFile), weapon->getModTime());
		}
	}
}

#ifdef V8NPCSERVER
void TServer::saveNpcs()
{
	for (const auto& [npcId, npc] : npcList)
	{
		if (npc->getType() != NPCType::LEVELNPC)
			npc->saveNPC();
	}
}

std::vector<std::pair<double, std::string>> TServer::calculateNpcStats()
{
	std::vector<std::pair<double, std::string>> script_profiles;

	// Iterate npcs
	for (const auto& [npcId, npc] : npcList)
	{
		ScriptExecutionContext& context = npc->getExecutionContext();
		std::pair<unsigned int, double> executionData = context.getExecutionData();
		if (executionData.second > 0.0)
		{
			std::string npcName = npc->getName();
			if (npcName.empty())
				npcName = "Level npc " + std::to_string(npc->getId());

			auto npcLevel = npc->getLevel();
			if (npcLevel != nullptr) {
				npcName.append(" (in level ").append(npcLevel->getLevelName().text()).
					append(" at pos (").append(CString(npc->getY() / 16.0).text()).
					append(", ").append(CString(npc->getX() / 16.0).text()).append(")");
			}

			script_profiles.push_back(std::make_pair(executionData.second, npcName));
		}
	}

	// Iterate weapons
	for (const auto& [weaponName, weapon] : weaponList)
	{
		ScriptExecutionContext& context = weapon->getExecutionContext();
		std::pair<unsigned int, double> executionData = context.getExecutionData();

		if (executionData.second > 0.0)
		{
			std::string weaponName("Weapon ");
			weaponName.append(weaponName);
			script_profiles.push_back(std::make_pair(executionData.second, weaponName));
		}
	}

	std::sort(script_profiles.rbegin(), script_profiles.rend());
	return script_profiles;
}
#endif

std::string transformString(const std::string& str) {
	std::string newStr;
	for (char ch : str)
	{
		if (ch == '"' || ch == '\\')
			newStr += "\\";
		else if (ch == '%')
			newStr += '%';
		newStr += ch;
	}

	return newStr;
}

void TServer::reportScriptException(const ScriptRunError& error)
{
	std::string error_message = transformString(error.getErrorString());
	sendToNC(error_message);
	error_message += "\n";
	getScriptLog().out(error_message);
}

void TServer::reportScriptException(const std::string& error_message)
{
	auto lines = CString{ error_message }.tokenize("\n");

	for (const auto& line : lines)
	{
		sendToNC(line);
		getScriptLog().out(line + "\n");
	}
}

/////////////////////////////////////////////////////

std::shared_ptr<TPlayer> TServer::getPlayer(unsigned short id) const
{
	auto iter = playerList.find(id);
	if (iter == std::end(playerList))
		return nullptr;

	return iter->second;
}

std::shared_ptr<TPlayer> TServer::getPlayer(unsigned short id, int type) const
{
	auto player = getPlayer(id);
	if (player == nullptr || !(player->getType() & type))
		return nullptr;

	return player;
}

std::shared_ptr<TPlayer> TServer::getPlayer(const CString& account, int type) const
{
	for (auto& [id, player] : playerList)
	{
		// Check if its the type of player we are looking for
		if (!player || !(player->getType() & type))
			continue;

		// Compare account names.
		if (player->getAccountName().toLower() == account.toLower())
			return player;
	}

	return nullptr;
}

std::shared_ptr<TLevel> TServer::getLevel(const std::string& pLevel)
{
	return TLevel::findLevel(pLevel, this);
}

std::shared_ptr<TWeapon> TServer::getWeapon(const std::string& name)
{
	auto iter = weaponList.find(name);
	if (iter == std::end(weaponList))
		return nullptr;
	return iter->second;
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
void TServer::assignNPCName(std::shared_ptr<TNPC> npc, const std::string& name)
{
	std::string newName = name;
	int num = 0;
	while (npcNameList.find(newName) != npcNameList.end())
		newName = name + std::to_string(++num);

	npc->setName(newName);
	npcNameList[newName] = npc;
}

void TServer::removeNPCName(std::shared_ptr<TNPC> npc)
{
	auto npcIter = npcNameList.find(npc->getName());
	if (npcIter != npcNameList.end())
		npcNameList.erase(npcIter);
}

std::shared_ptr<TNPC> TServer::addServerNpc(int npcId, float pX, float pY, std::shared_ptr<TLevel> pLevel, bool sendToPlayers)
{
	// Force database npc ids to be >= 1000
	if (npcId < 1000)
	{
		printf("Database npcs need to be greater than 1000\n");
		return nullptr;
	}

	// Make sure the npc id isn't in use
	auto existing = npcList.find(npcId);
	if (existing != std::end(npcList))
	{
		printf("Error creating database npc: Id is in use!\n");
		return nullptr;
	}

	// Create the npc
	auto newNPC = std::make_shared<TNPC>("", "", pX, pY, this, pLevel, NPCType::DBNPC);
	newNPC->setId(npcId);
	npcList.insert(std::make_pair(npcId, newNPC));

	// Add the npc to the level
	if (pLevel)
	{
		pLevel->addNPC(npcId);

		// Send the NPC's props to everybody in range.
		if (sendToPlayers)
		{
			CString packet = CString() >> (char)PLO_NPCPROPS >> (int)newNPC->getId() << newNPC->getProps(0);
			sendPacketToLevelArea(packet, pLevel);
		}
	}

	return newNPC;
}

void TServer::handlePM(TPlayer * player, const CString & message)
{
	if (!mPmHandlerNpc)
	{
		CString npcServerMsg;
		npcServerMsg = "I am the npcserver for\nthis game server. Almost\nall npc actions are controlled\nby me.";
		player->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)mNpcServer->getId() << "\"\"," << npcServerMsg.gtokenize());
		return;
	}

	// TODO(joey): This sets the first argument as the npc object, so we can't use it here for now.
	//mPmHandlerNpc->queueNpcEvent("npcserver.playerpm", true, player->getScriptObject(), std::string(message.text()));

	mPmHandlerNpc->getExecutionContext().addAction(mScriptEngine.CreateAction("npcserver.playerpm", player->getScriptObject(), message.toString()));
	mScriptEngine.RegisterNpcUpdate(mPmHandlerNpc.get());
}

void TServer::setPMFunction(uint32_t npcId, IScriptFunction *function)
{
	auto npc = getNPC(npcId);
	if (npc == nullptr || function == nullptr)
	{
		mPmHandlerNpc = nullptr;
		mScriptEngine.removeCallBack("npcserver.playerpm");
		return;
	}

	mScriptEngine.setCallBack("npcserver.playerpm", function);
	mPmHandlerNpc = npc;
}
#endif

std::shared_ptr<TNPC> TServer::addNPC(const CString& pImage, const CString& pScript, float pX, float pY, std::weak_ptr<TLevel> pLevel, bool pLevelNPC, bool sendToPlayers)
{
	// New Npc
	auto level = pLevel.lock();
	auto newNPC = std::make_shared<TNPC>(pImage, pScript.toString(), pX, pY, this, level, (pLevelNPC ? NPCType::LEVELNPC : NPCType::PUTNPC));

	// Get available NPC Id.
	uint32_t newId = nextNpcId;
	if (!freeNpcIds.empty())
	{
		newId = *freeNpcIds.begin();
		freeNpcIds.erase(newId);
	}
	else ++nextNpcId;

	// Assign NPC Id and add to list.
	newNPC->setId(newId);
	npcList.insert(std::make_pair(newId, newNPC));

	// Send the NPC's props to everybody in range.
	if (sendToPlayers)
	{
		CString packet = CString() >> (char)PLO_NPCPROPS >> (int)newNPC->getId() << newNPC->getProps(0);
		sendPacketToLevelArea(packet, level);
	}

	return newNPC;
}

bool TServer::deleteNPC(int id, bool eraseFromLevel)
{
	auto npc = getNPC(id);
	return deleteNPC(npc, eraseFromLevel);
}

bool TServer::deleteNPC(std::shared_ptr<TNPC> npc, bool eraseFromLevel)
{
	assert(npc);

	// Erase NPC from the list.
	npcList.erase(npc->getId());

	if (auto level = npc->getLevel(); level)
	{
		// Remove the NPC from the level
		if (eraseFromLevel)
			level->removeNPC(npc);

		// Tell the clients to delete the NPC.
		auto map = level->getMap();
		bool isOnMap = map != nullptr;
		CString tmpLvlName = (isOnMap ? map->getMapName() : level->getLevelName());

		for (auto& [pid, p] : playerList)
		{
			if (p->isClient())
			{
				if (isOnMap || p->getVersion() < CLVER_2_1)
					p->sendPacket(CString() >> (char)PLO_NPCDEL >> (int)npc->getId());
				else
					p->sendPacket(CString() >> (char)PLO_NPCDEL2 >> (char)tmpLvlName.length() << tmpLvlName >> (int)npc->getId());
			}
		}
	}

#ifdef V8NPCSERVER
	// TODO(joey): Need to deal with illegal characters
	// TODO(joey): add putnpc storage
	// If we persist this npc, delete the file  [ maybe should add a parameter if we should remove the npc from disk ]
	if (npc->getType() == NPCType::DBNPC)
	{
		CString filePath = getServerPath() << "npcs/npc" << npc->getName() << ".txt";
		CFileSystem::fixPathSeparators(filePath);
		remove(filePath.text());
	}

	if (npc->getType() == NPCType::DBNPC)
	{
		// Remove npc name assignment
		if (!npc->getName().empty())
			removeNPCName(npc);

		// If this is the npc that handles pms, clear it
		if (mPmHandlerNpc == npc)
			mPmHandlerNpc = nullptr;
	}
#endif

	return true;
}

bool TServer::deleteClass(const std::string& className)
{
	auto classIter = classList.find(className);
	if (classIter == classList.end())
		return false;

	classList.erase(classIter);
	CString filePath = getServerPath() << "scripts/" << className << ".txt";
	CFileSystem::fixPathSeparators(filePath);
	remove(filePath.text());

	return true;
}

void TServer::updateClass(const std::string& className, const std::string& classCode)
{
	classList[className] = std::make_unique<TScriptClass>(this, className, classCode);

	CString filePath = getServerPath() << "scripts/" << className << ".txt";
	CFileSystem::fixPathSeparators(filePath);

	CString fileData(classCode);
	fileData.save(filePath);
}

uint16_t TServer::getFreePlayerId()
{
	uint16_t newId = nextPlayerId;
	if (!freePlayerIds.empty())
	{
		newId = *(freePlayerIds.begin());
		freePlayerIds.erase(newId);
	}
	else ++nextPlayerId;

	return newId;
}

bool TServer::addPlayer(TPlayerPtr player, uint16_t id)
{
    assert(player);

	// No id was passed, so we will fetch one
	if (id == USHRT_MAX)
		id = getFreePlayerId();

	// Add them to the player list.
	player->setId(id);
	playerList[id] = player;

#ifdef V8NPCSERVER
	// Create script object for player
	mScriptEngine.wrapScriptObject(player.get());
#endif

	return true;
}

bool TServer::deletePlayer(TPlayerPtr player)
{
	if (player == nullptr)
		return true;

	// Add the player to the set of players to delete.
	if (deletedPlayers.insert(player).second)
	{
		// Remove the player from the serverlist.
		getServerList().deletePlayer(player);
	}

	return true;
}

void TServer::playerLoggedIn(TPlayerPtr player)
{
	// Tell the serverlist that the player connected.
	getServerList().addPlayer(player);

#ifdef V8NPCSERVER
	// Send event to server that player is logging in
	for (const auto& [npcName, npcPtr] : npcNameList)
	{
		// TODO(joey): check if they have the event before queueing for them
		if (auto npcObject = npcPtr.lock(); npcObject)
			npcObject->queueNpcAction("npc.playerlogin", player.get());
	}
#endif
}

bool TServer::warpPlayerToSafePlace(uint16_t playerId) {
	auto player = getPlayer(playerId);
	if (player == nullptr) return false;

	// Try unstick me level.
	CString unstickLevel = settings.getStr("unstickmelevel", "onlinestartlocal.nw");
	float unstickX = settings.getFloat("unstickmex", 30.0f);
	float unstickY = settings.getFloat("unstickmey", 30.5f);
	return player->warp(unstickLevel, unstickX, unstickY);

	// TODO: Maybe try the default account level?
}

void TServer::calculateServerTime()
{
	// Thu Feb 01 2001 17:33:34 GMT+0000
	// this is likely the actual start time of timevar
	serverTime = ((unsigned int)time(nullptr) - 981048814) / 5;
}

bool TServer::isIpBanned(const CString& ip)
{
	for (const auto& ipBan : ipBans)
	{
		if (ip.match(ipBan))
			return true;
	}

	return false;
}

bool TServer::isStaff(const CString& accountName)
{
	for (const auto& account : staffList)
	{
		if (accountName.toLower() == account.trim().toLower())
			return true;
	}

	return false;
}

void TServer::logToFile(const std::string & fileName, const std::string & message)
{
	CString fileNamePath = CString() << getServerPath().remove(0, getBaseHomePath().length()) << "logs/";

	// Remove leading characters that may try to go up a directory
	int idx = 0;
	while (fileName[idx] == '.' || fileName[idx] == '/' || fileName[idx] == '\\')
		idx++;
	fileNamePath << fileName.substr(idx);

	CLog logFile(fileNamePath, true);
	logFile.open();
	logFile.out("\n%s\n", message.c_str());
}

/*
	TServer: Server Flag Management
*/
bool TServer::deleteFlag(const std::string& pFlagName, bool pSendToPlayers)
{
	if ( settings.getBool("dontaddserverflags", false))
		return false;

	std::unordered_map<std::string, CString>::iterator mServerFlag;
	if ((mServerFlag = mServerFlags.find(pFlagName)) != mServerFlags.end())
	{
		mServerFlags.erase(mServerFlag);
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
	if (settings.getBool("dontaddserverflags", false))
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
		int fixedLength = 223 - 1 - (int)pFlagName.length();
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

void TServer::sendPacketToAll(const CString& packet, const std::set<uint16_t>& exclude) const
{
	for (auto& [id, player] : playerList)
	{
		if (exclude.contains(id))
			continue;
		if (player->isNPCServer())
			continue;

		player->sendPacket(packet);
	}
}

void TServer::sendPacketToLevelArea(const CString& packet, std::weak_ptr<TLevel> level, const std::set<uint16_t> &exclude, PlayerPredicate sendIf) const
{
	auto levelp = level.lock();
	if (!levelp) return;

	// If we have no map, just send to the level players.
	auto map = levelp->getMap();
	if (!map)
	{
		for (auto id : levelp->getPlayerList())
		{
			if (exclude.contains(id)) continue;
			if (auto other = this->getPlayer(id); other->isClient() && (sendIf == nullptr || sendIf(other.get())))
				other->sendPacket(packet);
		}
	}
	else
	{
		std::pair<int, int> sgmap{ levelp->getMapX(), levelp->getMapY() };

		for (auto& [id, other] : playerList)
		{
			if (exclude.contains(id)) continue;
			if (!other->isClient()) continue;
			if (sendIf != nullptr && !sendIf(other.get())) continue;

			auto othermap = other->getMap().lock();
			if (!othermap || othermap != map) continue;

			// Check if they are nearby before sending the packet.
			auto ogmap{ other->getMapPosition() };
			if (abs(ogmap.first - sgmap.first) < 2 && abs(ogmap.second - sgmap.second) < 2)
				other->sendPacket(packet);
		}
	}
}

void TServer::sendPacketToLevelArea(const CString& packet, std::weak_ptr<TPlayer> player, const std::set<uint16_t> &exclude, PlayerPredicate sendIf) const
{
	auto playerp = player.lock();
	if (!playerp) return;

	auto level = playerp->getLevel();
	if (!level) return;

	// If we have no map, just send to the level players.
	auto map = level->getMap();
	if (!map)
	{
		for (auto id : level->getPlayerList())
		{
			if (exclude.contains(id)) continue;
			if (auto other = this->getPlayer(id); other->isClient() && (sendIf == nullptr || sendIf(other.get())))
				other->sendPacket(packet);
		}
	}
	else
	{
		auto isGroupMap = map->isGroupMap();
		auto sgmap{ playerp->getMapPosition() };

		for (auto& [id, other] : playerList)
		{
			if (exclude.contains(id)) continue;
			if (!other->isClient()) continue;
			if (sendIf != nullptr && !sendIf(other.get())) continue;

			auto othermap = other->getMap().lock();
			if (!othermap || othermap != map) continue;
			if (isGroupMap && playerp->getGroup() != other->getGroup()) continue;

			// Check if they are nearby before sending the packet.
			auto ogmap{ other->getMapPosition() };
			if (abs(ogmap.first - sgmap.first) < 2 && abs(ogmap.second - sgmap.second) < 2)
				other->sendPacket(packet);
		}
	}
}

void TServer::sendPacketToOneLevel(const CString& packet, std::weak_ptr<TLevel> level, const std::set<uint16_t> &exclude) const
{
	auto levelp = level.lock();
	if (!levelp) return;

	for (auto id: levelp->getPlayerList())
	{
		if (exclude.contains(id)) continue;
		if (auto player = this->getPlayer(id); player->isClient())
			player->sendPacket(packet);
	}
}

void TServer::sendPacketToType(int who, const CString& pPacket, std::weak_ptr<TPlayer> pPlayer) const
{
	auto p = pPlayer.lock();
	if (!running) return;

	sendPacketToType(who, pPacket, p.get());
}

void TServer::sendPacketToType(int who, const CString& pPacket, TPlayer* pPlayer) const
{
	if (!running) return;
	for (auto& [id, player] : playerList)
	{
		if ((player->getType() & who) && (!pPlayer || id != pPlayer->getId()))
			player->sendPacket(pPacket);
	}
}

/*
	NPC-Server Functionality
*/
bool TServer::NC_AddWeapon(std::shared_ptr<TWeapon> pWeaponObj)
{
	if (pWeaponObj == nullptr)
		return false;

	weaponList[pWeaponObj->getName()] = pWeaponObj;
	return true;
}

bool TServer::NC_DelWeapon(const std::string& pWeaponName)
{
	// Definitions
	auto weaponObj = getWeapon(pWeaponName);
	if (!weaponObj || weaponObj->isDefault())
		return false;

	// Delete from File Browser
	CString name = pWeaponName;
	name.replaceAllI("\\", "_");
	name.replaceAllI("/", "_");
	name.replaceAllI("*", "@");
	name.replaceAllI(":", ";");
	name.replaceAllI("?", "!");
	CString filePath = getServerPath() << "weapons/weapon" << name << ".txt";
	CFileSystem::fixPathSeparators(filePath);
	remove(filePath.text());

	// Delete from Memory
	weaponList.erase(pWeaponName);

	// Delete from Players
	sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_NPCWEAPONDEL << pWeaponName);
	return true;
}

void TServer::updateWeaponForPlayers(std::shared_ptr<TWeapon> pWeapon)
{
	// Update Weapons
	for (auto& [id, player] : playerList)
	{
		if (!player->isClient())
			continue;

		if (player->hasWeapon(pWeapon->getName()))
		{
			player->sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << pWeapon->getName());
			player->sendPacket(pWeapon->getWeaponPacket(player->getVersion()));
		}
	}
}

void TServer::updateClassForPlayers(TScriptClass *pClass)
{
	// Update Weapons
	for (auto& [id, player] : playerList)
	{
		if (!player->isClient())
			continue;

		if (player->getVersion() >= CLVER_4_0211)
		{
			if (pClass != nullptr)
			{
				CString out;
				CString b = pClass->getByteCode();
				out >> (char)PLO_RAWDATA >> (int)b.length() << "\n";
				out >> (char)PLO_NPCWEAPONSCRIPT << b;

				player->sendPacket(out);
			}
		}
	}
}


/*
	GS2 Functionality
*/
template<typename ScriptObjType>
void TServer::compileScript(ScriptObjType& scriptObject, GS2ScriptManager::user_callback_type& cb)
{
	std::string script{ scriptObject.getSource().getClientGS2() };

	gs2ScriptManager.compileScript(script, [cb, &scriptObject, this](const CompilerResponse& resp)
	{
		if (!resp.errors.empty())
		{
			handleGS2Errors(resp.errors, scripting::getErrorOrigin(scriptObject));
		}

		// Compile any referenced joined classes, disabled for now as all classes should be compiled immediately
		//if (resp.success)
		//{
		//	for (auto& joinedClass : resp.joinedClasses)
		//	{
		//		auto cls = getClass(joinedClass);
		//		if (cls && cls->getByteCode().isEmpty())
		//		{
		//			GS2ScriptManager::user_callback_type fn = [](const auto& resp) {};
		//			compileScript(*cls, fn);
		//		}
		//	}
		//}

		if (cb)
		{
			cb(resp);
		}
	});
}

void TServer::compileGS2Script(const std::string& source, GS2ScriptManager::user_callback_type cb)
{
	gs2ScriptManager.compileScript(source, cb);
}

void TServer::compileGS2Script(TNPC *scriptObject, GS2ScriptManager::user_callback_type cb)
{
	if (scriptObject)
	{
		compileScript(*scriptObject, cb);
	}
}

void TServer::compileGS2Script(TScriptClass *scriptObject, GS2ScriptManager::user_callback_type cb)
{
	if (scriptObject)
	{
		compileScript(*scriptObject, cb);
	}
}

void TServer::compileGS2Script(TWeapon *scriptObject, GS2ScriptManager::user_callback_type cb)
{
	if (scriptObject)
	{
		compileScript(*scriptObject, cb);
	}
}

void TServer::handleGS2Errors(const std::vector<GS2CompilerError>& errors, const std::string& origin)
{
	std::string errorMsg;
	for (auto& err : errors)
	{
		switch (err.level())
		{
			case ErrorLevel::E_INFO:
				errorMsg += fmt::format("info: {}\n", err.msg());
				break;
			case ErrorLevel::E_WARNING:
				errorMsg += fmt::format("warning: {}\n", err.msg());
				break;
			default:
				errorMsg += fmt::format("error: {}\n", err.msg());
				break;
		}
	}

	if (!errorMsg.empty())
		reportScriptException(fmt::format("Script compiler output for {}:\n{}", origin, errorMsg));
}

/*
	Translation Functionality
*/
bool TServer::TS_Load(const CString& pLanguage, const CString& pFileName)
{
	// Load File
	std::vector<CString> fileData = CString::loadToken(pFileName, "\n", true);
	if (fileData.empty())
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
					(isStr ? msgStr.write(str) : msgId.write(str));
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
	const std::map<CString, CString> &temp = translationFS.getFileList();
	for (auto & i : temp)
		this->TS_Load(removeExtension(i.first), i.second);
}

void TServer::TS_Save()
{
	// Grab Translations
	std::map<std::string, STRMAP> *languages = mTranslationManager.getTranslationList();

	// Iterate each Language
	for (auto & language : *languages)
	{
		// Create Output
		CString output;

		// Iterate each Translation
		for (auto & lang : language.second)
		{
			output << "msgid ";
			std::vector<CString> sign = CString(lang.first.c_str()).removeAll("\r").tokenize("\n");
			for (auto & s : sign)
				output << "\"" << s << "\"\r\n";
			output << "msgstr ";
			if (!lang.second.empty())
			{
				std::vector<CString> lines = CString(lang.second.c_str()).removeAll("\r").tokenize("\n");
				for (auto & line : lines)
					output << "\"" << line << "\"\r\n";
			}
			else output << "\"\"\r\n";

			output << "\r\n";
		}

		// Save File
		output.trimRight().save(getServerPath() << "translations/" << language.first.c_str() << ".po");
	}
}
