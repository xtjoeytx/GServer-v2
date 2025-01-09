#include <IDebug.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

#include <fmt/format.h>

#include <CString.h>
#include <IUtil.h>

#include "main.h"

#include "NPC.h"
#include "Player.h"
#include "Server.h"
#include "Weapon.h"
#include "level/Level.h"
#include "level/Map.h"
#include "scripting/ScriptOrigin.h"
#include "scripting/ScriptClass.h"

static const char* const filesystemTypes[] = {
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

template<class T, class R, class... Args>
auto methodstub(T* t, R (T::*m)(Args...))
{
	return [=](auto&&... args) -> R
	{
		return (t->*m)(decltype(args)(args)...);
	};
}

// I don't want to deal with adding this to the gs2lib.
CString operator<<(const CString& first, const CString& second)
{
	CString result{ first };
	return result << second;
}

Server::Server(const CString& pName)
	: running(false), m_doRestart(false), m_name(pName), m_animationManager(this), m_packageManager(this), m_serverStartTime(0),
	  m_triggerActionDispatcher(methodstub(this, &Server::createTriggerCommands))
{
	auto time_now = std::chrono::high_resolution_clock::now();
	m_lastTimer = m_lastNewWorldTimer = m_last1mTimer = m_last5mTimer = m_last3mTimer = time_now;
	calculateServerTime();

	// This has the full path to the server directory.
	m_serverPath = CString() << getBaseHomePath() << "servers/" << m_name << "/";
	FileSystem::fixPathSeparators(m_serverPath);

	// Set up the log files.
	CString logpath = m_serverPath.remove(0, static_cast<int>(getBaseHomePath().length()));
	CString npcPath = CString() << logpath << "logs/npclog.txt";
	CString rcPath = CString() << logpath << "logs/rclog.txt";
	CString serverPath = CString() << logpath << "logs/serverlog.txt";
	FileSystem::fixPathSeparators(npcPath);
	FileSystem::fixPathSeparators(rcPath);
	FileSystem::fixPathSeparators(serverPath);
	m_npcLog.setFilename(npcPath);
	m_rcLog.setFilename(rcPath);
	m_serverLog.setFilename(serverPath);

#ifdef V8NPCSERVER
	CString scriptPath = CString() << logpath << "logs/scriptlog.txt";
	FileSystem::fixPathSeparators(scriptPath);
	m_scriptLog.setFilename(scriptPath);
#endif
}

Server::~Server()
{
	cleanup();
}

int Server::init(const CString& serverip, const CString& serverport, const CString& localip, const CString& serverinterface)
{
#ifdef V8NPCSERVER
	// Initialize the Script Engine
	if (!m_scriptEngine.initialize())
	{
		m_serverLog.out("** [Error] Could not initialize script engine.\n");
		// TODO(joey): new error number? log is probably enough
		return ERR_SETTINGS;
	}
#endif

	// Load the config files.
	int ret = loadConfigFiles();
	if (ret) return ret;

	// If an override serverip and serverport were specified, fix the options now.
	if (!serverip.isEmpty())
		m_settings.addKey("serverip", serverip);
	if (!serverport.isEmpty())
		m_settings.addKey("serverport", serverport);
	if (!localip.isEmpty())
		m_settings.addKey("localip", localip);
	if (!serverinterface.isEmpty())
		m_settings.addKey("serverinterface", serverinterface);

	m_overrideIp = serverip;
	m_overridePort = serverport;
	m_overrideLocalIp = localip;
	m_overrideInterface = serverinterface;

	// Fix up the interface to work properly with CSocket.
	CString oInter = m_overrideInterface;
	if (m_overrideInterface.isEmpty())
		oInter = m_settings.getStr("serverinterface");
	if (oInter == "AUTO")
		oInter.clear();

	// Initialize the player socket.
	m_playerSock.setType(SOCKET_TYPE_SERVER);
	m_playerSock.setProtocol(SOCKET_PROTOCOL_TCP);
	m_playerSock.setDescription("playerSock");

	// Start listening on the player socket.
	m_serverLog.out(":: Initializing player listen socket.\n");
	if (m_playerSock.init((oInter.isEmpty() ? 0 : oInter.text()), m_settings.getStr("serverport").text()))
	{
		m_serverLog.out("** [Error] Could not initialize listening socket...\n");
		return ERR_LISTEN;
	}
	if (m_playerSock.connect())
	{
		m_serverLog.out("** [Error] Could not connect listening socket...\n");
		return ERR_LISTEN;
	}

#ifdef UPNP
	// Start a UPNP thread.  It will try to set a UPNP port forward in the background.
	serverlog.out(":: Starting UPnP discovery thread.\n");
	m_upnp.initialize((oInter.isEmpty() ? m_playerSock.getLocalIp() : oInter.text()), m_settings.getStr("serverport").text());
	m_upnpThread = std::thread(std::ref(m_upnp));
#endif

#ifdef V8NPCSERVER
	// Setup NPC Control port
	m_ncPort = strtoint(m_settings.getStr("serverport"));

	m_npcServer = std::make_shared<Player>(nullptr, 0);
	m_npcServer->setType(PLTYPE_NPCSERVER);
	m_npcServer->loadAccount("(npcserver)");
	m_npcServer->setHeadImage(m_settings.getStr("staffhead", "head25.png"));
	m_npcServer->setLoaded(true); // can't guarantee this, so forcing it

	// TODO(joey): Update this when server options is changed?
	// Set nickname, and append (Server) - required!
	CString nickName = m_settings.getStr("nickname");
	if (nickName.isEmpty())
		nickName = "NPC-Server";
	nickName << " (Server)";
	m_npcServer->setNick(nickName, true);

	// Add npc-server to playerlist
	addPlayer(m_npcServer);
#endif

	// Register ourself with the socket manager.
	m_sockManager.registerSocket((CSocketStub*)this);

	// Register the server start time.
	m_serverStartTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

	return 0;
}

// Called when the Server is put into its own thread.
void Server::operator()()
{
	running = true;
	while (running)
	{
		// Do a server loop.
		doMain();

		// Clean up deleted players here.
		cleanupDeletedPlayers();

		// Check if we should do a restart.
		if (m_doRestart)
		{
			m_doRestart = false;
			cleanup();
			int ret = init(m_overrideIp, m_overridePort, m_overrideLocalIp, m_overrideInterface);
			if (ret != 0)
				break;
		}

		if (shutdownProgram)
			running = false;
	}
	cleanup();
}

void Server::cleanupDeletedPlayers()
{
	if (m_deletedPlayers.empty()) return;
	for (auto i = std::begin(m_deletedPlayers); i != std::end(m_deletedPlayers);)
	{
		// Value copy so the shared_ptr exists until the end.
		PlayerPtr player = *i;

#ifdef V8NPCSERVER
		IScriptObject<Player>* playerObject = player->getScriptObject();
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
					for (const auto& [npcName, npcPtr]: m_npcNameList)
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
		m_playerIdGenerator.freeId(player->getId());
		m_sockManager.unregisterSocket(player.get());
		m_playerList.erase(player->getId());
		player->cleanup();

		i = m_deletedPlayers.erase(i);
	}
	//m_deletedPlayers.clear();
}

void Server::cleanup()
{
	// Close our UPNP port forward.
	// First, make sure the thread has completed already.
	// This can cause an issue if the server is about to be deleted.
#ifdef UPNP
	if (m_upnpThread.joinable())
		m_upnpThread.join();
	m_upnremoveAllForwardedPortsts();
#endif

	// Save translations.
	this->TS_Save();

	// Save server flags.
	saveServerFlags();

#ifdef V8NPCSERVER
	// Save npcs
	saveNpcs();

	// npc-server will be cleared from playerlist, so lets invalidate the pointer here
	m_npcServer = nullptr;
#endif

	for (auto& [id, player]: m_playerList)
		player->cleanup();

	m_playerList.clear();
	m_deletedPlayers.clear();
	m_playerIdGenerator.resetAndSetNext(PLAYERID_INIT);

	m_levelList.clear();
	m_mapList.clear();
	m_groupLevels.clear();

	m_npcList.clear();
	m_npcNameList.clear();
	m_npcIdGenerator.resetAndSetNext(NPCID_INIT);

	saveWeapons();
	m_weaponList.clear();

#ifdef V8NPCSERVER
	// Clean up the script engine
	m_scriptEngine.cleanup();
#endif

	m_playerSock.disconnect();
	m_serverlist.getSocket().disconnect();

	// Clean up the socket manager.  Pass false so we don't cause a crash.
	m_sockManager.cleanup(false);
}

void Server::restart()
{
	m_doRestart = true;
}

bool Server::doMain()
{
	// Update our socket manager.
	m_sockManager.update(0, 5000); // 5ms

	// Current time
	auto currentTimer = std::chrono::high_resolution_clock::now();

#ifdef V8NPCSERVER
	m_scriptEngine.runScripts(currentTimer);

	// enable when we switch to async compiling
	//m_gs2ScriptManager.runQueue();
#endif

	// Every second, do some events.
	auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimer - m_lastTimer);
	if (time_diff.count() >= 1000)
	{
		m_lastTimer = currentTimer;
		doTimedEvents();
	}

	return true;
}

bool Server::doTimedEvents()
{
	// Do serverlist events.
	m_serverlist.doTimedEvents();

	// Do player events.
	{
		for (auto& [id, player]: m_playerList)
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
		for (auto& level: m_levelList)
		{
			assert(level);
			level->doTimedEvents();
		}

		// Group levels.
		for (auto& [group, levelPtr]: m_groupLevels)
		{
			if (auto level = levelPtr.lock(); level)
				level->doTimedEvents();
		}
	}

	// Send NW time.
	auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(m_lastTimer - m_lastNewWorldTimer);
	if (time_diff.count() >= 5)
	{
		calculateServerTime();

		m_lastNewWorldTimer = m_lastTimer;
		sendPacketToAll(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(getNWTime()));
	}

	// Stuff that happens every minute.
	time_diff = std::chrono::duration_cast<std::chrono::seconds>(m_lastTimer - m_last1mTimer);
	if (time_diff.count() >= 60)
	{
		m_last1mTimer = m_lastTimer;

		// Save server flags.
		this->saveServerFlags();
	}

	// Stuff that happens every 3 minutes.
	time_diff = std::chrono::duration_cast<std::chrono::seconds>(m_lastTimer - m_last3mTimer);
	if (time_diff.count() >= 180)
	{
		m_last3mTimer = m_lastTimer;

		// TODO(joey): probably a better way to do this..

		// Resynchronize the file systems.
		m_filesystemAccounts.resync();
		for (auto& i: m_filesystem)
			i.resync();
	}

	// Save stuff every 5 minutes.
	time_diff = std::chrono::duration_cast<std::chrono::seconds>(m_lastTimer - m_last5mTimer);
	if (time_diff.count() >= 300)
	{
		m_last5mTimer = m_lastTimer;

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
		if (!m_groupLevels.empty())
		{
			std::unordered_set<std::string> groupKeys;
			std::for_each(std::begin(m_groupLevels), std::end(m_groupLevels), [&groupKeys](auto& pair)
						  {
							  groupKeys.insert(pair.first);
						  });

			for (auto& groupName: groupKeys)
			{
				bool playersFound = false;
				auto range = m_groupLevels.equal_range(groupName);
				std::for_each(range.first, range.second, [&playersFound](decltype(m_groupLevels)::value_type& pair)
							  {
								  if (auto level = pair.second.lock(); level && !level->getPlayers().empty())
									  playersFound = true;
							  });

				if (!playersFound)
				{
					m_groupLevels.erase(groupName);
				}
			}
		}
	}

	return true;
}

bool Server::onRecv()
{
	// Create socket.
	CSocket* newSock = m_playerSock.accept();
	if (newSock == nullptr)
		return true;

	// Create the new player.
	auto newPlayer = std::make_shared<Player>(newSock, 0);

	// Add the player to the server
	if (!addPlayer(newPlayer))
		return false;

	// Add them to the socket manager.
	m_sockManager.registerSocket((CSocketStub*)newPlayer.get());

	return true;
}

/////////////////////////////////////////////////////

void Server::loadAllFolders()
{
	for (auto& fs: m_filesystem)
		fs.clear();

	m_filesystem[0].addDir("world");
	if (m_settings.getStr("sharefolder").length() > 0)
	{
		std::vector<CString> folders = m_settings.getStr("sharefolder").tokenize(",");
		for (auto& folder: folders)
			m_filesystem[0].addDir(folder.trim());
	}
}

void Server::loadFolderConfig()
{
	for (auto& i: m_filesystem)
		i.clear();

	m_foldersConfig = CString::loadToken(CString() << m_serverPath << "config/foldersconfig.txt", "\n", true);
	for (auto& configLine: m_foldersConfig)
	{
		// No comments.
		int cLoc = -1;
		if ((cLoc = configLine.find("#")) != -1)
			configLine.removeI(cLoc);
		configLine.trimI();
		if (configLine.length() == 0) continue;

		// Parse the line.
		CString type = configLine.readString(" ");
		CString config = configLine.readString("");
		type.trimI();
		config.trimI();
		FileSystem::fixPathSeparators(config);

		// Get the directory.
		CString dirNoWild;
		int pos = -1;
		if ((pos = config.findl(FileSystem::getPathSeparator())) != -1)
			dirNoWild = config.remove(pos + 1);
		CString dir = CString("world/") << dirNoWild;
		CString wildcard = config.remove(0, dirNoWild.length());

		// Find out which file system to add it to.
		FileSystem* fs = getFileSystemByType(type);

		// Add it to the appropriate file system.
		if (fs != nullptr)
		{
			fs->addDir(dir, wildcard);
			m_serverLog.out("       adding %s [%s] to %s\n", dir.text(), wildcard.text(), type.text());
		}
		m_filesystem[0].addDir(dir, wildcard);
	}
}

int Server::loadConfigFiles()
{
	// TODO(joey): /reloadconfig reloads this, but things like server flags, weapons and npcs probably shouldn't be reloaded.
	//	Move them out of here?
	m_serverLog.out(":: Loading server configuration...\n");

	// Load Settings
	m_serverLog.out("     Loading settings...\n");
	loadSettings();

	// Load Admin Settings
	m_serverLog.out("     Loading admin settings...\n");
	loadAdminSettings();

	// Load allowed versions.
	m_serverLog.out("     Loading allowed client versions...\n");
	loadAllowedVersions();

	// Load folders config and file system.
	m_serverLog.out("     Folder config: ", m_settings.getBool("nofoldersconfig", false) ? "DISABLED" : "ENABLED");

	m_serverLog.out("     Loading file system...\n");
	loadFileSystem();

	// Load server flags.
	m_serverLog.out("     Loading serverflags.txt...\n");
	loadServerFlags();

	// Load server message.
	m_serverLog.out("     Loading config/servermessage.html...\n");
	loadServerMessage();

	// Load IP bans.
	m_serverLog.out("     Loading config/ipbans.txt...\n");
	loadIPBans();

	// Load weapons.
	m_serverLog.out("     Loading weapons...\n");
	loadWeapons(true);

	// Load classes.
	m_serverLog.out("     Loading classes...\n");
	loadClasses(true);

	// Load maps.
	m_serverLog.out("     Loading maps...\n");
	loadMaps(true);

#ifdef V8NPCSERVER
	// Load database npcs.
	m_serverLog.out("     Loading npcs...\n");
	loadNpcs(true);
#endif

	// Load map levels - doing this after db npcs are loaded incase
	// some level scripts may require access to the databases.
	loadMapLevels();

	// Load translations.
	m_serverLog.out("     Loading translations...\n");
	loadTranslations();

	// Load word filter.
	m_serverLog.out("     Loading word filter...\n");
	loadWordFilter();

	return 0;
}

void Server::loadSettings()
{
	if (!m_settings.isOpened())
	{
		m_settings.setSeparator("=");
		m_settings.loadFile(CString() << m_serverPath << "config/serveroptions.txt");
		if (!m_settings.isOpened())
			m_serverLog.out("** [Error] Could not open config/serveroptions.txt.  Will use default config.\n");
	}

	// Load status list.
	m_statusList = m_settings.getStr("playerlisticons", "Online,Away,DND,Eating,Hiding,No PMs,RPing,Sparring,PKing").tokenize(",");

	// Load staff list
	m_staffList = m_settings.getStr("staff").tokenize(",");

	// Send our ServerHQ info in case we got changed the staffonly setting.
	getServerList().sendServerHQ();
}

void Server::loadAdminSettings()
{
	m_adminSettings.setSeparator("=");
	m_adminSettings.loadFile(CString() << m_serverPath << "config/adminconfig.txt");
	if (!m_adminSettings.isOpened())
		m_serverLog.out("** [Error] Could not open config/adminconfig.txt.  Will use default config.\n");
	else
		getServerList().sendServerHQ();
}

void Server::loadAllowedVersions()
{
	CString versions;
	versions.load(CString() << m_serverPath << "config/allowedversions.txt");
	versions = removeComments(versions);
	versions.removeAllI("\r");
	versions.removeAllI("\t");
	versions.removeAllI(" ");
	m_allowedVersions = versions.tokenize("\n");
	m_allowedVersionString.clear();
	for (auto& allowedVersion: m_allowedVersions)
	{
		if (!m_allowedVersionString.isEmpty())
			m_allowedVersionString << ", ";

		int loc = allowedVersion.find(":");
		if (loc == -1)
			m_allowedVersionString << getVersionString(allowedVersion, PLTYPE_ANYCLIENT);
		else
		{
			CString s = allowedVersion.subString(0, loc);
			CString f = allowedVersion.subString(loc + 1);
			int vid = getVersionID(s);
			int vid2 = getVersionID(f);
			if (vid != -1 && vid2 != -1)
				m_allowedVersionString << getVersionString(s, PLTYPE_ANYCLIENT) << " - " << getVersionString(f, PLTYPE_ANYCLIENT);
		}
	}
}

void Server::loadFileSystem()
{
	for (auto& i: m_filesystem)
		i.clear();
	m_filesystemAccounts.clear();
	m_filesystemAccounts.addDir("accounts", "*.txt");
	if (m_settings.getBool("nofoldersconfig", false))
		loadAllFolders();
	else
		loadFolderConfig();

	for (auto &[file, path] : m_filesystem[0].getFileList()) {
		if (getExtension(file) == ".gupd") {
			getPackageManager().findOrAddResource(file.toString())->reload(this);
		}
	}
}

void Server::loadServerFlags()
{
	std::vector<CString> lines = CString::loadToken(CString() << m_serverPath << "serverflags.txt", "\n", true);
	for (auto& line: lines)
		this->setFlag(line, false);
}

void Server::loadServerMessage()
{
	m_serverMessage.load(CString() << m_serverPath << "config/servermessage.html");
	m_serverMessage.removeAllI("\r");
	m_serverMessage.replaceAllI("\n", " ");
}

void Server::loadIPBans()
{
	m_ipBans = CString::loadToken(CString() << m_serverPath << "config/ipbans.txt", "\n", true);
}

void Server::loadClasses(bool print)
{
	FileSystem scriptFS;
	scriptFS.addDir("scripts", "*.txt");
	const std::map<CString, CString>& scriptFileList = scriptFS.getFileList();
	for (auto& scriptFile: scriptFileList)
	{
		std::string className = scriptFile.first.subString(0, scriptFile.first.length() - 4).text();

		CString scriptData;
		scriptData.load(scriptFile.second);
		m_classList[className] = std::make_unique<ScriptClass>(className, scriptData.text());

		updateClassForPlayers(getClass(className));
	}
}

void Server::loadWeapons(bool print)
{
	FileSystem weaponFS;
	weaponFS.addDir("weapons", "weapon*.txt");
	FileSystem bcweaponFS;
	bcweaponFS.addDir("weapon_bytecode", "*");

	auto& weaponFileList = weaponFS.getFileList();
	for (auto& weaponFile: weaponFileList)
	{
		auto weapon = Weapon::loadWeapon(weaponFile.first);
		if (weapon == nullptr) continue;
		if (weapon->getByteCodeFile().empty())
			weapon->setModTime(weaponFS.getModTime(weaponFile.first));
		else
			weapon->setModTime(bcweaponFS.getModTime(weapon->getByteCodeFile()));

		// Check if the weapon exists.
		if (m_weaponList.find(weapon->getName()) == m_weaponList.end())
		{
			m_weaponList[weapon->getName()] = weapon;
			if (print) m_serverLog.out("       %s\n", weapon->getName().c_str());
		}
		else
		{
			// If the weapon exists, and the version on disk is newer, reload it.
			auto& w = m_weaponList[weapon->getName()];
			if (w->getModTime() < weapon->getModTime())
			{
				m_weaponList[weapon->getName()] = weapon;
				updateWeaponForPlayers(weapon);
				if (print)
				{
					m_serverLog.out("       %s [updated]\n", weapon->getName().c_str());
					Server::sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: Updated weapon " << weapon->getName() << " ");
				}
			}
			else
			{
				// TODO(joey): even though were deleting the weapon because its skipped, its still queuing its script action
				//	and attempting to execute it. Technically the code needs to be run again though, will fix soon.
				if (print) m_serverLog.out("       %s [skipped]\n", weapon->getName().c_str());
			}
		}
	}

	// Add the default weapons.
	if (!m_weaponList.contains("bow")) m_weaponList["bow"] = std::make_shared<Weapon>(LevelItem::getItemId("bow"));
	if (!m_weaponList.contains("bomb")) m_weaponList["bomb"] = std::make_shared<Weapon>(LevelItem::getItemId("bomb"));
	if (!m_weaponList.contains("superbomb")) m_weaponList["superbomb"] = std::make_shared<Weapon>(LevelItem::getItemId("superbomb"));
	if (!m_weaponList.contains("fireball")) m_weaponList["fireball"] = std::make_shared<Weapon>(LevelItem::getItemId("fireball"));
	if (!m_weaponList.contains("fireblast")) m_weaponList["fireblast"] = std::make_shared<Weapon>(LevelItem::getItemId("fireblast"));
	if (!m_weaponList.contains("nukeshot")) m_weaponList["nukeshot"] = std::make_shared<Weapon>(LevelItem::getItemId("nukeshot"));
	if (!m_weaponList.contains("joltbomb")) m_weaponList["joltbomb"] = std::make_shared<Weapon>(LevelItem::getItemId("joltbomb"));
}

void Server::loadMapLevels()
{
	// Load gmap levels based on options provided by the gmap file
	for (const auto& map: m_mapList)
	{
		if (map->getType() == MapType::GMAP)
			map->loadMapLevels();
	}
}

void Server::loadMaps(bool print)
{
	// Remove players off all maps
	for (auto& [id, player]: m_playerList)
		player->setMap(nullptr);

	// Remove existing maps.
	m_mapList.clear();

	// Load gmaps.
	std::vector<CString> gmaps = m_settings.getStr("gmaps").guntokenize().tokenize("\n");
	for (CString& gmapName: gmaps)
	{
		// Check for blank lines.
		if (gmapName == "\r") continue;

		// Gmaps in server options don't need the .gmap suffix, so we will add the suffix
		if (gmapName.right(5) != ".gmap")
		{
			gmapName << ".gmap";
		}

		// Load the gmap.
		auto gmap = std::make_unique<Map>(MapType::GMAP);
		if (!gmap->load(CString() << gmapName))
		{
			if (print) m_serverLog.out(CString() << "[" << m_name << "] "
												 << "** [Error] Could not load " << gmapName
												 << "\n");
			continue;
		}

		if (print) m_serverLog.out("       [gmap] %s\n", gmapName.text());
		m_mapList.push_back(std::move(gmap));
	}

	// Load bigmaps.
	std::vector<CString> bigmaps = m_settings.getStr("maps").guntokenize().tokenize("\n");
	for (auto& i: bigmaps)
	{
		// Check for blank lines.
		if (i == "\r") continue;

		// Load the bigmap.
		auto bigmap = std::make_unique<Map>(MapType::BIGMAP);
		if (!bigmap->load(i.trim()))
		{
			if (print) m_serverLog.out(CString() << "[" << m_name << "] "
												 << "** [Error] Could not load " << i << "\n");
			continue;
		}

		if (print) m_serverLog.out("       [bigmap] %s\n", i.text());
		m_mapList.push_back(std::move(bigmap));
	}

	// Load group maps.
	std::vector<CString> groupmaps = m_settings.getStr("groupmaps").guntokenize().tokenize("\n");
	for (auto& groupmap: groupmaps)
	{
		// Check for blank lines.
		if (groupmap == "\r") continue;

		// Determine the type of map we are loading.
		CString ext(getExtension(groupmap));
		ext.toLowerI();

		// Create the new map based on the file extension.
		std::unique_ptr<Map> gmap;
		if (ext == ".txt")
			gmap = std::make_unique<Map>(MapType::BIGMAP, true);
		else if (ext == ".gmap")
			gmap = std::make_unique<Map>(MapType::GMAP, true);
		else
			continue;

		// Load the map.
		if (!gmap->load(CString() << groupmap))
		{
			if (print) m_serverLog.out(CString() << "[" << m_name << "] "
												 << "** [Error] Could not load " << groupmap << "\n");
			continue;
		}

		if (print) m_serverLog.out("       [group map] %s\n", groupmap.text());
		m_mapList.push_back(std::move(gmap));
	}

	// Update all map <--> level relationships
	for (const auto& level: m_levelList)
	{
		bool found = false;
		for (const auto& map: m_mapList)
		{
			int mx, my;
			if (map->isLevelOnMap(level->getLevelName().toLower().text(), mx, my))
			{
				level->setMap(map, mx, my);
				found = true;
				break;
			}
		}

		if (!found)
		{
			level->setMap({});
		}
	}
}

#ifdef V8NPCSERVER
void Server::loadNpcs(bool print)
{
	FileSystem npcFS;
	npcFS.addDir("npcs", "npc*.txt");

	auto& npcFileList = npcFS.getFileList();
	for (const auto& [npcName, fileName]: npcFileList)
	{
		bool loaded = false;

		// Create the npc
		auto newNPC = std::make_shared<NPC>("", "", 30.f, 30.5f, nullptr, NPCType::DBNPC);
		if (newNPC->loadNPC(fileName))
		{
			int npcId = newNPC->getId();
			if (npcId < 1000)
			{
				printf("Database npcs must be greater than 1000\n");
			}
			else if (auto existing = m_npcList.find(npcId); existing != std::end(m_npcList))
			{
				printf("Error creating database npc: Id is in use!\n");
			}
			else
			{
				m_npcList.insert(std::make_pair(npcId, newNPC));
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

void Server::loadTranslations()
{
	this->TS_Reload();
}

void Server::loadWordFilter()
{
	m_wordFilter.load(CString() << m_serverPath << "config/rules.txt");
}

void Server::saveServerFlags()
{
	CString out;
	for (auto& mServerFlag: m_serverFlags)
		out << mServerFlag.first << "=" << mServerFlag.second << "\r\n";
	out.save(CString() << m_serverPath << "serverflags.txt");
}

void Server::saveWeapons()
{
	FileSystem weaponFS;
	weaponFS.addDir("weapons", "weapon*.txt");
	const std::map<CString, CString>& weaponFileList = weaponFS.getFileList();

	for (auto& [weaponName, weapon]: m_weaponList)
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
void Server::saveNpcs()
{
	for (const auto& [npcId, npc]: m_npcList)
	{
		if (npc->getType() != NPCType::LEVELNPC)
			npc->saveNPC();
	}
}

std::vector<std::pair<double, std::string>> Server::calculateNpcStats()
{
	std::vector<std::pair<double, std::string>> script_profiles;

	// Iterate npcs
	for (const auto& [npcId, npc]: m_npcList)
	{
		ScriptExecutionContext& context = npc->getExecutionContext();
		std::pair<unsigned int, double> executionData = context.getExecutionData();
		if (executionData.second > 0.0)
		{
			std::string npcName = npc->getName();
			if (npcName.empty())
				npcName = "Level npc " + std::to_string(npc->getId());

			auto npcLevel = npc->getLevel();
			if (npcLevel != nullptr)
			{
				npcName.append(" (in level ").append(npcLevel->getLevelName().text()).append(" at pos (").append(CString(npc->getY() / 16.0).text()).append(", ").append(CString(npc->getX() / 16.0).text()).append(")");
			}

			script_profiles.push_back(std::make_pair(executionData.second, npcName));
		}
	}

	// Iterate weapons
	for (const auto& [weaponName, weapon]: m_weaponList)
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

std::string transformString(const std::string& str)
{
	std::string newStr;
	for (char ch: str)
	{
		if (ch == '"' || ch == '\\')
			newStr += "\\";
		else if (ch == '%')
			newStr += '%';
		newStr += ch;
	}

	return newStr;
}

void Server::reportScriptException(const ScriptRunError& error)
{
	std::string error_message = transformString(error.getErrorString());
	sendToNC(error_message);
	error_message += "\n";
	getScriptLog().out(error_message);
}

void Server::reportScriptException(const std::string& error_message)
{
	auto lines = CString{ error_message }.tokenize("\n");

	for (const auto& line: lines)
	{
		sendToNC(line);
		getScriptLog().out(line + "\n");
	}
}

/////////////////////////////////////////////////////

std::shared_ptr<Player> Server::getPlayer(unsigned short id) const
{
	auto iter = m_playerList.find(id);
	if (iter == std::end(m_playerList))
		return nullptr;

	return iter->second;
}

std::shared_ptr<Player> Server::getPlayer(unsigned short id, int type) const
{
	auto player = getPlayer(id);
	if (player == nullptr || !(player->getType() & type))
		return nullptr;

	return player;
}

std::shared_ptr<Player> Server::getPlayer(const CString& account, int type) const
{
	for (auto& [id, player]: m_playerList)
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

std::shared_ptr<Level> Server::getLevel(const std::string& pLevel)
{
	return Level::findLevel(pLevel);
}

std::shared_ptr<Weapon> Server::getWeapon(const std::string& name)
{
	auto iter = m_weaponList.find(name);
	if (iter == std::end(m_weaponList))
		return nullptr;
	return iter->second;
}

CString Server::getFlag(const std::string& pFlagName)
{
#ifdef V8NPCSERVER
	if (m_serverFlags.find(pFlagName) != m_serverFlags.end())
		return m_serverFlags[pFlagName];
	return "";
#else
	return m_serverFlags[pFlagName];
#endif
}

FileSystem* Server::getFileSystemByType(CString& type)
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
	return &m_filesystem[fs];
}

#ifdef V8NPCSERVER
void Server::assignNPCName(std::shared_ptr<NPC> npc, const std::string& name)
{
	std::string newName = name;
	int num = 0;
	while (m_npcNameList.find(newName) != m_npcNameList.end())
		newName = name + std::to_string(++num);

	npc->setName(newName);
	m_npcNameList[newName] = npc;
}

void Server::removeNPCName(std::shared_ptr<NPC> npc)
{
	auto npcIter = m_npcNameList.find(npc->getName());
	if (npcIter != m_npcNameList.end())
		m_npcNameList.erase(npcIter);
}

std::shared_ptr<NPC> Server::addServerNpc(int npcId, float pX, float pY, std::shared_ptr<Level> pLevel, bool sendToPlayers)
{
	// Force database npc ids to be >= 1000
	if (npcId < 1000)
	{
		printf("Database npcs need to be greater than 1000\n");
		return nullptr;
	}

	// Make sure the npc id isn't in use
	auto existing = m_npcList.find(npcId);
	if (existing != std::end(m_npcList))
	{
		printf("Error creating database npc: Id is in use!\n");
		return nullptr;
	}

	// Create the npc
	auto newNPC = std::make_shared<NPC>("", "", pX, pY, pLevel, NPCType::DBNPC);
	newNPC->setId(npcId);
	m_npcList.insert(std::make_pair(npcId, newNPC));

	// Add the npc to the level
	if (pLevel)
	{
		pLevel->addNPC(npcId);

		// Send the NPC's props to everybody in range.
		if (sendToPlayers)
		{
			CString packet = CString() >> (char)PLO_NPCPROPS >> (int)newNPC->getId() << newNPC->getProps(0);
			sendPacketToLevelOnlyGmapArea(packet, pLevel);
		}
	}

	return newNPC;
}

void Server::handlePM(Player* player, const CString& message)
{
	if (!m_pmHandlerNpc)
	{
		CString npcServerMsg;
		npcServerMsg = "I am the npcserver for\nthis game server. Almost\nall npc actions are controlled\nby me.";
		player->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)m_npcServer->getId() << "\"\"," << npcServerMsg.gtokenize());
		return;
	}

	// TODO(joey): This sets the first argument as the npc object, so we can't use it here for now.
	//m_pmHandlerNpc->queueNpcEvent("npcserver.playerpm", true, player->getScriptObject(), std::string(message.text()));

	m_pmHandlerNpc->getExecutionContext().addAction(m_scriptEngine.createAction("npcserver.playerpm", player->getScriptObject(), message.toString()));
	m_scriptEngine.registerNpcUpdate(m_pmHandlerNpc.get());
}

void Server::setPMFunction(uint32_t npcId, IScriptFunction* function)
{
	auto npc = getNPC(npcId);
	if (npc == nullptr || function == nullptr)
	{
		m_pmHandlerNpc = nullptr;
		m_scriptEngine.removeCallBack("npcserver.playerpm");
		return;
	}

	m_scriptEngine.setCallBack("npcserver.playerpm", function);
	m_pmHandlerNpc = npc;
}
#endif

std::shared_ptr<NPC> Server::addNPC(const CString& pImage, const CString& pScript, float pX, float pY, std::weak_ptr<Level> pLevel, bool pLevelNPC, bool sendToPlayers)
{
	// New Npc
	auto level = pLevel.lock();
	auto newNPC = std::make_shared<NPC>(pImage, pScript.toString(), pX, pY, level, (pLevelNPC ? NPCType::LEVELNPC : NPCType::PUTNPC));

	// Get available NPC Id.
	uint32_t newId = m_npcIdGenerator.getAvailableId();

	// Assign NPC Id and add to list.
	newNPC->setId(newId);
	m_npcList.insert(std::make_pair(newId, newNPC));

	// Send the NPC's props to everybody in range.
	if (sendToPlayers)
	{
		CString packet = CString() >> (char)PLO_NPCPROPS >> (int)newNPC->getId() << newNPC->getProps(0);
		sendPacketToLevelOnlyGmapArea(packet, level);
	}

	return newNPC;
}

bool Server::deleteNPC(int id, bool eraseFromLevel)
{
	auto npc = getNPC(id);
	return deleteNPC(npc, eraseFromLevel);
}

bool Server::deleteNPC(std::shared_ptr<NPC> npc, bool eraseFromLevel)
{
	assert(npc);

	// Erase NPC from the list.
	m_npcList.erase(npc->getId());

	if (auto level = npc->getLevel(); level)
	{
		// Remove the NPC from the level
		if (eraseFromLevel)
			level->removeNPC(npc);

		// Tell the clients to delete the NPC.
		auto map = level->getMap();
		bool isOnMap = map != nullptr;
		CString tmpLvlName = (isOnMap ? map->getMapName() : level->getLevelName());

		for (auto& [pid, p]: m_playerList)
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
		FileSystem::fixPathSeparators(filePath);
		remove(filePath.text());
	}

	if (npc->getType() == NPCType::DBNPC)
	{
		// Remove npc name assignment
		if (!npc->getName().empty())
			removeNPCName(npc);

		// If this is the npc that handles pms, clear it
		if (m_pmHandlerNpc == npc)
			m_pmHandlerNpc = nullptr;
	}
#endif

	return true;
}

bool Server::deleteClass(const std::string& className)
{
	auto classIter = m_classList.find(className);
	if (classIter == m_classList.end())
		return false;

	m_classList.erase(classIter);
	CString filePath = getServerPath() << "scripts/" << className << ".txt";
	FileSystem::fixPathSeparators(filePath);
	remove(filePath.text());

	return true;
}

void Server::updateClass(const std::string& className, const std::string& classCode)
{
	m_classList[className] = std::make_unique<ScriptClass>(className, classCode);

	CString filePath = getServerPath() << "scripts/" << className << ".txt";
	FileSystem::fixPathSeparators(filePath);

	CString fileData(classCode);
	fileData.save(filePath);
}

bool Server::addPlayer(PlayerPtr player, uint16_t id)
{
	assert(player);

	// No id was passed, so we will fetch one
	if (id == USHRT_MAX)
		id = m_playerIdGenerator.getAvailableId();

	// Add them to the player list.
	player->setId(id);
	m_playerList[id] = player;

#ifdef V8NPCSERVER
	// Create script object for player
	m_scriptEngine.wrapScriptObject(player.get());
#endif

	return true;
}

bool Server::deletePlayer(PlayerPtr player)
{
	if (player == nullptr)
		return true;

	// Add the player to the set of players to delete.
	if (m_deletedPlayers.insert(player).second)
	{
		// Remove the player from the serverlist.
		getServerList().deletePlayer(player);
	}

	return true;
}

void Server::playerLoggedIn(PlayerPtr player)
{
	// Tell the serverlist that the player connected.
	getServerList().addPlayer(player);

#ifdef V8NPCSERVER
	// Send event to server that player is logging in
	for (const auto& [npcName, npcPtr]: m_npcNameList)
	{
		// TODO(joey): check if they have the event before queueing for them
		if (auto npcObject = npcPtr.lock(); npcObject)
			npcObject->queueNpcAction("npc.playerlogin", player.get());
	}
#endif
}

bool Server::warpPlayerToSafePlace(uint16_t playerId)
{
	auto player = getPlayer(playerId);
	if (player == nullptr) return false;

	// Try unstick me level.
	CString unstickLevel = m_settings.getStr("unstickmelevel", "onlinestartlocal.nw");
	float unstickX = m_settings.getFloat("unstickmex", 30.0f);
	float unstickY = m_settings.getFloat("unstickmey", 30.5f);
	return player->warp(unstickLevel, unstickX, unstickY);

	// TODO: Maybe try the default account level?
}

void Server::calculateServerTime()
{
	// Thu Feb 01 2001 17:33:34 GMT+0000
	// this is likely the actual start time of timevar
	m_serverTime = ((unsigned int)time(nullptr) - 981048814) / 5;
}

bool Server::isIpBanned(const CString& ip)
{
	for (const auto& ipBan: m_ipBans)
	{
		if (ip.match(ipBan))
			return true;
	}

	return false;
}

bool Server::isStaff(const CString& accountName)
{
	for (const auto& account: m_staffList)
	{
		if (accountName.toLower() == account.trim().toLower())
			return true;
	}

	return false;
}

void Server::logToFile(const std::string& fileName, const std::string& message)
{
	CString fileNamePath = CString() << getServerPath().remove(0, static_cast<int>(getBaseHomePath().length())) << "logs/";

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
	Server: Server Flag Management
*/
bool Server::deleteFlag(const std::string& pFlagName, bool pSendToPlayers)
{
	if (m_settings.getBool("dontaddserverflags", false))
		return false;

	std::unordered_map<std::string, CString>::iterator mServerFlag;
	if ((mServerFlag = m_serverFlags.find(pFlagName)) != m_serverFlags.end())
	{
		m_serverFlags.erase(mServerFlag);
		if (pSendToPlayers)
			sendPacketToAll(CString() >> (char)PLO_FLAGDEL << pFlagName);
		return true;
	}

	return false;
}

bool Server::setFlag(CString pFlag, bool pSendToPlayers)
{
	std::string flagName = pFlag.readString("=").text();
	CString flagValue = pFlag.readString("");
	return this->setFlag(flagName, (flagValue.isEmpty() ? "1" : flagValue), pSendToPlayers);
}

bool Server::setFlag(const std::string& pFlagName, const CString& pFlagValue, bool pSendToPlayers)
{
	if (m_settings.getBool("dontaddserverflags", false))
		return false;

	// delete flag
	if (pFlagValue.isEmpty())
		return deleteFlag(pFlagName);

	// optimize
	if (m_serverFlags[pFlagName] == pFlagValue)
		return true;

	// set flag
	if (m_settings.getBool("cropflags", true))
	{
		int fixedLength = 223 - 1 - (int)pFlagName.length();
		m_serverFlags[pFlagName] = pFlagValue.subString(0, fixedLength);
	}
	else
		m_serverFlags[pFlagName] = pFlagValue;

	if (pSendToPlayers)
		sendPacketToAll(CString() >> (char)PLO_FLAGSET << pFlagName << "=" << pFlagValue);
	return true;
}

/*
	Packet-Sending Functions
*/

void Server::sendPacketToAll(const CString& packet, const std::set<uint16_t>& exclude) const
{
	for (auto& [id, player]: m_playerList)
	{
		if (exclude.contains(id))
			continue;
		if (player->isNPCServer())
			continue;

		player->sendPacket(packet);
	}
}

void Server::sendPacketToLevelArea(const CString& packet, std::weak_ptr<Level> level, const std::set<uint16_t>& exclude, PlayerPredicate sendIf) const
{
	auto levelp = level.lock();
	if (!levelp) return;

	// If we have no map, just send to the level players.
	auto map = levelp->getMap();
	if (!map)
	{
		for (auto id: levelp->getPlayers())
		{
			if (exclude.contains(id)) continue;
			if (auto other = this->getPlayer(id); other->isClient() && (sendIf == nullptr || sendIf(other.get())))
				other->sendPacket(packet);
		}
	}
	else
	{
		std::pair<int, int> sgmap{ levelp->getMapX(), levelp->getMapY() };

		for (auto& [id, other]: m_playerList)
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

void Server::sendPacketToLevelArea(const CString& packet, std::weak_ptr<Player> player, const std::set<uint16_t>& exclude, PlayerPredicate sendIf) const
{
	auto playerp = player.lock();
	if (!playerp) return;

	auto level = playerp->getLevel();
	if (!level) return;

	// If we have no map, just send to the level players.
	auto map = level->getMap();
	if (!map)
	{
		for (auto id: level->getPlayers())
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

		for (auto& [id, other]: m_playerList)
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

void Server::sendPacketToLevelOnlyGmapArea(const CString& packet, std::weak_ptr<Level> level, const std::set<uint16_t>& exclude, PlayerPredicate sendIf) const
{
	auto levelp = level.lock();
	if (!levelp) return;

	// If we have no map, just send to the level players.
	// If it we are on a bigmap, also just send to level players.
	auto map = levelp->getMap();
	if (!map || map->getType() == MapType::BIGMAP)
	{
		for (auto id : levelp->getPlayers())
		{
			if (exclude.contains(id)) continue;
			if (auto other = this->getPlayer(id); other->isClient() && (sendIf == nullptr || sendIf(other.get())))
				other->sendPacket(packet);
		}
	}
	else
	{
		std::pair<int, int> sgmap{ levelp->getMapX(), levelp->getMapY() };

		for (auto& [id, other] : m_playerList)
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

void Server::sendPacketToLevelOnlyGmapArea(const CString& packet, std::weak_ptr<Player> player, const std::set<uint16_t>& exclude, PlayerPredicate sendIf) const
{
	auto playerp = player.lock();
	if (!playerp) return;

	auto level = playerp->getLevel();
	if (!level) return;

	// If we have no map, just send to the level players.
	auto map = level->getMap();
	if (!map || map->getType() == MapType::BIGMAP)
	{
		for (auto id : level->getPlayers())
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

		for (auto& [id, other] : m_playerList)
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

void Server::sendPacketToOneLevel(const CString& packet, std::weak_ptr<Level> level, const std::set<uint16_t>& exclude) const
{
	auto levelp = level.lock();
	if (!levelp) return;

	for (auto id: levelp->getPlayers())
	{
		if (exclude.contains(id)) continue;
		if (auto player = this->getPlayer(id); player->isClient())
			player->sendPacket(packet);
	}
}

void Server::sendPacketToType(int who, const CString& pPacket, std::weak_ptr<Player> pPlayer) const
{
	auto p = pPlayer.lock();
	if (!running) return;

	sendPacketToType(who, pPacket, p.get());
}

void Server::sendPacketToType(int who, const CString& pPacket, Player* pPlayer) const
{
	if (!running) return;
	for (auto& [id, player]: m_playerList)
	{
		if ((player->getType() & who) && (!pPlayer || id != pPlayer->getId()))
			player->sendPacket(pPacket);
	}
}

/*
	NPC-Server Functionality
*/
bool Server::NC_AddWeapon(std::shared_ptr<Weapon> pWeaponObj)
{
	if (pWeaponObj == nullptr)
		return false;

	m_weaponList[pWeaponObj->getName()] = pWeaponObj;
	return true;
}

bool Server::NC_DelWeapon(const std::string& pWeaponName)
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
	FileSystem::fixPathSeparators(filePath);
	remove(filePath.text());

	// Delete from Memory
	m_weaponList.erase(pWeaponName);

	// Delete from Players
	sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_NPCWEAPONDEL << pWeaponName);
	return true;
}

void Server::updateWeaponForPlayers(std::shared_ptr<Weapon> pWeapon)
{
	// Update Weapons
	for (auto& [id, player]: m_playerList)
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

void Server::updateClassForPlayers(ScriptClass* pClass)
{
	// Update Weapons
	for (auto& [id, player]: m_playerList)
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
void Server::compileScript(ScriptObjType& scriptObject, GS2ScriptManager::user_callback_type& cb)
{
	std::string script{ scriptObject.getSource().getClientGS2() };

	m_gs2ScriptManager.compileScript(script, [cb, &scriptObject, this](const CompilerResponse& resp)
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

void Server::compileGS2Script(const std::string& source, GS2ScriptManager::user_callback_type cb)
{
	m_gs2ScriptManager.compileScript(source, cb);
}

void Server::compileGS2Script(NPC* scriptObject, GS2ScriptManager::user_callback_type cb)
{
	if (scriptObject)
	{
		compileScript(*scriptObject, cb);
	}
}

void Server::compileGS2Script(ScriptClass* scriptObject, GS2ScriptManager::user_callback_type cb)
{
	if (scriptObject)
	{
		compileScript(*scriptObject, cb);
	}
}

void Server::compileGS2Script(Weapon* scriptObject, GS2ScriptManager::user_callback_type cb)
{
	if (scriptObject)
	{
		compileScript(*scriptObject, cb);
	}
}

void Server::handleGS2Errors(const std::vector<GS2CompilerError>& errors, const std::string& origin)
{
	std::string errorMsg;
	for (auto& err: errors)
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
bool Server::TS_Load(const CString& pLanguage, const CString& pFileName)
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
				else
				{
					--cur;
					break;
				}

				++cur;
			}

			m_translationManager.add(pLanguage.text(), msgId.text(), msgStr.text());
		}

		if (cur == fileData.end())
			break;
	}

	return true;
}

CString Server::TS_Translate(const CString& pLanguage, const CString& pKey)
{
	return m_translationManager.translate(pLanguage.toLower().text(), pKey.text());
}

void Server::TS_Reload()
{
	// Save Translations
	this->TS_Save();

	// Reset Translations
	m_translationManager.reset();

	// Load Translation Folder
	FileSystem translationFS;
	translationFS.addDir("translations", "*.po");

	// Load Each File
	const std::map<CString, CString>& temp = translationFS.getFileList();
	for (auto& i: temp)
		this->TS_Load(removeExtension(i.first), i.second);
}

void Server::TS_Save()
{
	// Grab Translations
	std::map<std::string, STRMAP>* languages = m_translationManager.getTranslationList();

	// Iterate each Language
	for (auto& language: *languages)
	{
		// Create Output
		CString output;

		// Iterate each Translation
		for (auto& lang: language.second)
		{
			output << "msgid ";
			std::vector<CString> sign = CString(lang.first.c_str()).removeAll("\r").tokenize("\n");
			for (auto& s: sign)
				output << "\"" << s << "\"\r\n";
			output << "msgstr ";
			if (!lang.second.empty())
			{
				std::vector<CString> lines = CString(lang.second.c_str()).removeAll("\r").tokenize("\n");
				for (auto& line: lines)
					output << "\"" << line << "\"\r\n";
			}
			else
				output << "\"\"\r\n";

			output << "\r\n";
		}

		// Save File
		output.trimRight().save(getServerPath() << "translations/" << language.first.c_str() << ".po");
	}
}

void Server::sendShootToOneLevel(const std::weak_ptr<Level>& level, float x, float y, float z, float angle, float zangle, float strength, const std::string& ani, const std::string& aniArgs) const
{
	auto levelLock = level.lock();
	ShootPacketNew newPacket{};
	newPacket.pixelx = (int16_t)(x * 16);
	newPacket.pixely = (int16_t)(y * 16);
	newPacket.pixelz = (int16_t)(z * 16);
	newPacket.offsetx = 0;
	newPacket.offsety = 0;
	newPacket.sangle = (int8_t)angle;
	newPacket.sanglez = (int8_t)zangle;
	newPacket.speed = (int8_t)strength;
	newPacket.gravity = 8;
	newPacket.gani = ani;
	newPacket.ganiArgs = aniArgs;
	newPacket.shootParams = getShootParams();

	CString oldPacketBuf = CString() >> (char)PLO_SHOOT >> (short)0 << newPacket.constructShootV1();
	CString newPacketBuf = CString() >> (char)PLO_SHOOT2 >> (short)0 << newPacket.constructShootV2();

	sendPacketToLevelArea(oldPacketBuf, levelLock, { 0 }, [](const auto pl)
						  {
							  return pl->getVersion() < CLVER_5_07;
						  });
	sendPacketToLevelArea(newPacketBuf, levelLock, { 0 }, [](const auto pl)
						  {
							  return pl->getVersion() >= CLVER_5_07;
						  });
}
