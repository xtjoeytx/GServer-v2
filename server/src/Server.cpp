#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <numbers>
#include <optional>
#include <random>
#include <ranges>
#include <set>
#include <string_view>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <CSocket.h>

#include <CString.h>
#include <CTranslationManager.h>
#include <IEnums.h>
#include <IUtil.h>

#include <FileSystem.h>
#include <Server.h>
#include <level/Level.h>
#include <level/LevelItem.h>
#include <level/LevelShoot.h>
#include <level/Map.h>
#include <loader/flatfile/FlatFileAccountLoader.h>
#include <loader/flatfile/FlatFileNPCLoader.h>
#include <loader/LevelLoader.h>
#include <misc/UPNP.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <object/Weapon.h>
#include <player/PlayerClient.h>
#include <player/PlayerLogin.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////

extern std::atomic_bool shutdownProgram;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

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

template<class T, class R, class... Args>
auto methodstub(T* t, R (T::*m)(Args...))
{
	return [=](auto&&... args) -> R
	{
		return (t->*m)(decltype(args)(args)...);
	};
}

// I don't want to deal with adding this to the gs2lib.
[[maybe_unused]] static CString operator<<(const CString& first, const CString& second)
{
	CString result{ first };
	return result << second;
}

Server::Server(const CString& pName)
	: m_animationManager(this), m_packageManager(this), m_name(pName),
	  m_triggerActionDispatcher(methodstub(this, &Server::createTriggerCommands))
{
	calculateNWTime();

	m_npcIdGenerator.createSegment(NPCID_GEN_LOCAL);
	m_npcIdGenerator.createSegment(NPCID_GEN_DATABASE);
	m_npcIdGenerator.createSegment(NPCID_GEN_MANUAL);
	//m_playerIdGenerator.createSegment(PLAYERID_GEN_EXTERNAL);

	m_accountLoader = std::make_unique<FlatFileAccountLoader>();
	m_npcLoader = std::make_unique<FlatFileNPCLoader>();

	m_timedEvents.callbackIterations = std::bind(&Server::doTimedEvents, this, std::placeholders::_1);
	m_timedSave.callbackIterations = [this](int)
	{
		saveServerFlags();
		m_guildManager.saveGuilds();
	};
	m_timedNWTime.callbackIterations = [this](int)
	{
		calculateNWTime();
		sendPacketToAll(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(getNWTime()));
	};
	m_timedMaintenance.callbackIterations = [this](int)
	{
		// Reload some server settings.
		loadAllowedVersions();
		loadServerMessage();
		loadIPBans();

		// Save some stuff.
		// TODO(joey): Is this really needed? We save weapons to disk when it is updated or created anyway..
		saveWeapons();

		// Check all of the instanced maps to see if the players have left.
		if (!m_groupLevels.empty())
		{
			std::unordered_set<std::string> groupKeys;
			std::for_each(std::begin(m_groupLevels), std::end(m_groupLevels), [&groupKeys](auto& pair)
			{
				groupKeys.insert(pair.first);
			});

			for (auto& groupName : groupKeys)
			{
				bool playersFound = false;
				auto range = m_groupLevels.equal_range(groupName);
				std::for_each(range.first, range.second, [&playersFound](decltype(m_groupLevels)::value_type& pair)
				{
					if (auto level = pair.second.lock(); level && !level->getLevelPlayers().empty())
						playersFound = true;
				});

				if (!playersFound)
				{
					m_groupLevels.erase(groupName);
				}
			}
		}
	};
}

Server::~Server()
{
	cleanup();

	// Close our UPNP port forward.
	// First, make sure the thread has completed already.
	// This can cause an issue if the server is about to be deleted.
#ifdef ENABLE_UPNP
	if (m_upnpThread.joinable())
		m_upnpThread.join();
	if (m_upnp)
		m_upnp->removeAllForwardedPorts();
#endif
}

int Server::init(const CString& serverip, const CString& serverport, const CString& localip, const CString& serverinterface)
{
	// Register the server start time.
	m_serverStartTime = std::chrono::system_clock::now();

	// Load the config files.
	int ret = loadConfigFiles();
	if (ret) return ret;

	// Load the NPC-Server.
	loadNPCServer();

	// Load the server objects.
	ret = loadServerObjects();
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
	log::printLine(log::server, ":: Initializing player listen socket.");
	if (m_playerSock.init((oInter.isEmpty() ? 0 : oInter.text()), m_settings.getStr("serverport").text()))
	{
		log::printLine(log::server, "** [Error] Could not initialize listening socket...");
		return ERR_LISTEN;
	}
	if (m_playerSock.connect())
	{
		log::printLine(log::server, "** [Error] Could not connect listening socket...");
		return ERR_LISTEN;
	}

	// Announce the ports.
	{
		auto indent = log::server.indent();
		log::printLine(log::server, "Listening on: {}:{}.", m_playerSock.getRemoteIp(), m_playerSock.getRemotePort());
	}

	// Start a UPNP thread.  It will try to set a UPNP port forward in the background.
#ifdef ENABLE_UPNP
	if (m_settings.getBool("upnp", true) && m_upnp == nullptr)
	{
		log::printLine(log::server, ":: Starting UPnP discovery thread.");
		m_upnp = std::make_unique<UPNP>();
		m_upnp->initialize((oInter.isEmpty() ? m_playerSock.getLocalIp() : oInter.text()), m_settings.getStr("serverport").text());
		m_upnpThread = std::thread(std::ref(*m_upnp.get()));
	}
#endif

	// Register ourself with the socket manager.
	m_sockManager.registerSocket((CSocketStub*)this);

	// Start the timers.
	m_timedEvents.start();
	m_timedNWTime.start();
	m_timedSave.start();
	m_timedMaintenance.start();

#ifdef PACKETLOGGING
	log::printLine(log::networkdump, "------------------------------ START ------------------------------");
#endif

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
}

void Server::cleanup()
{
	// Save translations.
	this->TS_Save();

	// Save server flags.
	saveServerFlags();

	// Save NPC-Server NPCs.
	if (hasNPCServer())
		m_npcServer->saveNPCs();

	m_npcList.clear();
	m_shootParams.clear();

	auto players = m_playerList | std::views::transform([](const auto& pair) { return pair.second; });
	std::vector<PlayerPtr> deletePlayers{ std::ranges::begin(players), std::ranges::end(players) };
	for (auto& player: deletePlayers)
		player->cleanup();

	m_npcServer.reset();
	m_playerList.clear();

	m_levelList.clear();
	m_mapList.clear();
	m_groupLevels.clear();

	saveWeapons();
	m_weaponList.clear();

	m_npcIdGenerator.reset();
	m_playerIdGenerator.reset();

	m_playerSock.disconnect();
	m_serverlist.getSocket().disconnect();

	// Clean up the socket manager.  Pass false so we don't cause a crash.
	m_sockManager.cleanup(false);
	m_adminSettings.clear();

	log::printLine(log::server, "-------------------------------------");
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
	m_frameStartTimeHighPrecision = precise_clock::now();
	m_frameStartTime = currentTime();

	// Update the NPC server.
	if (hasNPCServer())
		m_npcServer->update(m_frameStartTimeHighPrecision);

	// Update our events.
	m_timedEvents.update(m_frameStartTimeHighPrecision);
	m_timedSave.update(m_frameStartTimeHighPrecision);
	m_timedNWTime.update(m_frameStartTimeHighPrecision);
	m_timedMaintenance.update(m_frameStartTimeHighPrecision);

	// Do level frame events.
	for (auto& [name, level]: m_levelList)
	{
		if (level != nullptr)
			level->doFrameEvents(m_frameStartTimeHighPrecision);
	}

	return true;
}

bool Server::doTimedEvents(int)
{
	// Do serverlist events.
	m_serverlist.doTimedEvents();

	// Do player events.
	{
		std::vector<PlayerPtr> deletePlayers;
		for (auto& [id, player]: m_playerList)
		{
			assert(player);
			if (!player->isNPCServer())
			{
				if (!player->doTimedEvents())
					deletePlayers.push_back(player);
			}
		}
		std::ranges::for_each(deletePlayers, [this](PlayerPtr& player) { deletePlayer(player); });
		deletePlayers.clear();
	}

	// Do level events.
	{
		for (auto& [name, level]: m_levelList)
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

	return true;
}

bool Server::onRecv()
{
	// Create socket.
	CSocket* newSock = m_playerSock.accept();
	if (newSock == nullptr)
		return true;

	// Create the new player.
	auto newPlayer = std::make_shared<PlayerLogin>(newSock, USHRT_MAX);

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
	auto indent = log::server.indent();

	for (auto& i: m_filesystem)
		i.clear();

	m_foldersConfig = CString::loadToken(CString() << "config/foldersconfig.txt", "\n", true);
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
			log::printLine(log::server, "adding {} [{}] to {}", dir, wildcard, type);
		}
		m_filesystem[0].addDir(dir, wildcard);
	}
}

int Server::loadConfigFiles()
{
	log::printLine(log::server, ":: Loading server configuration...");

	{
		auto indent = log::server.indent();

		// Load Settings
		log::printLine(log::server, "Loading settings...");
		{
			auto indentsettings = log::server.indent();
			loadSettings();
			log::printLine(log::server, "Server generation: {}", ServerGenerationNames[(size_t)Generation]);
		}

		// Load Admin Settings
		log::printLine(log::server, "Loading admin settings...");
		loadAdminSettings();

		// Load allowed versions.
		log::printLine(log::server, "Loading allowed client versions...");
		loadAllowedVersions();

		// Load folders config and file system.
		log::print(log::server, "Folder config: ");
		if (!m_settings.getBool("nofoldersconfig", false))
			log::printLine(log::server, "ENABLED");
		else
			log::printLine(log::server, "disabled");

		log::printLine(log::server, "Loading file system...");
		loadFileSystem();

		// Load server message.
		log::printLine(log::server, "Loading config/servermessage.html...");
		loadServerMessage();

		// Load IP bans.
		log::printLine(log::server, "Loading config/ipbans.txt...");
		loadIPBans();

		// Load translations.
		log::printLine(log::server, "Loading translations...");
		loadTranslations();

		// Load word filter.
		log::printLine(log::server, "Loading word filter...");
		loadWordFilter();

		// Load server flags.
		log::printLine(log::server, "Loading serverflags.txt...");
		loadServerFlags();

		// Load guilds.
		log::printLine(log::server, "Loading guilds...");
		loadGuilds();

		// Load maps.
		log::printLine(log::server, "Loading maps...");
		loadMaps(true);
	}

	return 0;
}

void Server::loadSettings()
{
	if (!m_settings.isOpened())
	{
		m_settings.setSeparator("=");
		m_settings.loadFile(CString() << "config/serveroptions.txt");
		if (!m_settings.isOpened())
			log::printLine(log::server, "** [Error] Could not open config/serveroptions.txt.  Will use default config.");
	}

	// Load status list.
	m_statusList = m_settings.getStr("playerlisticons", "Online,Away,DND,Eating,Hiding,No PMs,RPing,Sparring,PKing").tokenize(",");

	// Load staff list
	m_staffList = m_settings.getStr("staff").tokenize(",");

	// Load the generation.
	auto generation = m_settings.getStr("generation", "classic").toLower();
	if (auto it = std::ranges::find(ServerGenerationNames, generation.toStringView()); it != std::ranges::end(ServerGenerationNames))
		Generation = static_cast<ServerGeneration>(std::distance(ServerGenerationNames.begin(), it));

	// Send our ServerHQ info in case we got changed the staffonly setting.
	getServerList().sendServerHQ();

	// Bush item drops.
	auto bushitemtypes = m_settings.getStr("bushitemtypes", "greenrupee,bluerupee,heart,bombs").tokenize(",");
	{
		// greenrupee 10, bluerupee 5, bombs 5, heart 5
		static const std::array<int, 25> defaults = { 10, 5, 0, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

		m_bushDrops.clear();
		for (auto& itemtype : bushitemtypes)
		{
			itemtype.trimI().toLowerI();
			LevelItemType item = LevelItem::getItemId(itemtype.toString());
			if (item == LevelItemType::INVALID)
				continue;

			auto spawnRate = m_settings.getInt(std::format("spawnrate{}", itemtype.toStringView()), defaults[static_cast<size_t>(item)]);
			m_bushDrops.emplace_back(item, spawnRate);
		}
	}

	// Death item drops.
	auto deathitemtypes = m_settings.getStr("deathitemtypes", "greenrupee,bluerupee,redrupee,goldrupee,bombs,darts").tokenize(",");
	{
		m_deathDrops.clear();
		for (auto& itemtype : deathitemtypes)
		{
			itemtype.trimI().toLowerI();
			LevelItemType item = LevelItem::getItemId(itemtype.toString());
			if (item != LevelItemType::INVALID)
				m_deathDrops.push_back(item);
		}
	}
}

void Server::loadAdminSettings()
{
	m_adminSettings.setSeparator("=");
	m_adminSettings.loadFile(CString() << "config/adminconfig.txt");
	if (!m_adminSettings.isOpened())
		log::printLine(log::server, "** [Error] Could not open config/adminconfig.txt.  Will use default config.");
	else
		getServerList().sendServerHQ();
}

void Server::loadAllowedVersions()
{
	CString versions;
	versions.load(CString() << "config/allowedversions.txt");
	versions = removeComments(versions);
	versions.removeAllI("\r");
	versions.removeAllI("\t");
	versions.removeAllI(" ");
	m_allowedVersions.clear();
	m_allowedVersionString.clear();

	// New version.
	if (versions.contains("[generation-range]"))
	{
		const auto& generation = ServerGenerationNames.at(static_cast<size_t>(Generation));
		if (auto line = versions.find(generation); line != -1)
		{
			if (auto sep = versions.find("=", line); sep != -1)
			{
				versions.setRead(sep + 1);
				std::string versionRange = string::trimMutate(versions.readString("\n").toString());

				std::vector<std::string> formattedVersions;
				for (const auto& version : string::splitHard(versionRange, ","sv))
				{
					m_allowedVersions.push_back(version);
					auto rangeParts = string::splitHard(version, ":"sv);
					if (rangeParts.size() == 1)
					{
						if (int id = getVersionID(rangeParts[0]); id != 0)
							formattedVersions.push_back(getVersionString(id, PLTYPE_ANYCLIENT));
					}
					else if (rangeParts.size() == 2)
					{
						int startId = getVersionID(rangeParts[0]);
						int endId = getVersionID(rangeParts[1]);
						if (startId != 0 && endId != 0)
							formattedVersions.emplace_back(std::format("{} - {}", getVersionString(rangeParts[0], PLTYPE_ANYCLIENT), getVersionString(rangeParts[1], PLTYPE_ANYCLIENT)));
					}
				}
				m_allowedVersionString = string::join(formattedVersions, ", "sv);
			}
		}

		if (m_allowedVersions.empty())
			log::printLine(log::server, "** [Error] Could not find generation range for '{}' in allowedversions.txt.", generation);
	}
	// Old version.
	else
	{
		m_allowedVersions = versions.tokenize("\n");
		for (auto& allowedVersion : m_allowedVersions)
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
	std::vector<CString> lines = CString::loadToken(CString() << "serverflags.txt", "\n", true);
	for (auto& line: lines)
		this->setFlag(line, false);
}

void Server::loadGuilds()
{
	m_guildManager.loadGuilds("guilds");
}

void Server::loadServerMessage()
{
	m_serverMessage.load(CString() << "config/servermessage.html");
	m_serverMessage.removeAllI("\r");
	m_serverMessage.replaceAllI("\n", " ");
}

void Server::loadIPBans()
{
	m_ipBans = CString::loadToken(CString() << "config/ipbans.txt", "\n", true);
}

void Server::loadTranslations()
{
	this->TS_Reload();
}

void Server::loadWordFilter()
{
	m_wordFilter.load(CString() << "config/rules.txt");
}

void Server::loadMaps(bool print)
{
	assert(m_levelList.empty() && "Levels should be loaded after maps.");

	auto indent = log::server.indent();

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
		try
		{
			auto gmap = std::make_unique<Map>(is_gmap, gmapName.toString());
			if (print) log::printLine(log::server, "[gmap] {}", gmapName);
			m_mapList.push_back(std::move(gmap));
		}
		catch (...)
		{
			auto inerr = log::server.indent_absolute(0);
			if (print) log::printLine(log::server, "** [Error] Could not load {}.", gmapName);
		}
	}

	// Load bigmaps.
	std::vector<CString> bigmaps = m_settings.getStr("maps").guntokenize().tokenize("\n");
	for (auto& i: bigmaps)
	{
		// Check for blank lines.
		if (i == "\r") continue;

		// Load the bigmap.
		try
		{
			auto bigmap = std::make_unique<Map>(is_bigmap, i.trim().toString());
			if (print) log::printLine(log::server, "[bigmap] {}", i);
			m_mapList.push_back(std::move(bigmap));
		}
		catch (...)
		{
			auto inerr = log::server.indent_absolute(0);
			if (print) log::printLine(log::server, "** [Error] Could not load {}.", i);
		}
	}

	// Load group maps.
	/*
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
			auto inerr = log::server.indent_absolute(0);
			if (print) log::printLine(log::server, "** [Error] Could not load {}.", groupmap);
			continue;
		}

		if (print) log::printLine(log::server, "[group map] {}", groupmap);
		m_mapList.push_back(std::move(gmap));
	}
	*/
}

void Server::loadNPCServer()
{
	if (m_settings.getBool("serverside", true))
	{
		log::printLine(log::server, ":: Loading NPC server...");
		{
			auto indent = log::server.indent();
			{
				auto sectionProfile = log::Profile(log::server, "", "(Completed in {1:0.6} ms)");
				m_npcServer = std::make_shared<NPCServer>();
				m_npcServer->initialize();
			}
		}
	}
}

int Server::loadServerObjects()
{
	log::printLine(log::server, ":: Loading server objects...");

	auto indent = log::server.indent();
	{
		// Load weapons.
		log::printLine(log::server, "Loading weapons...");
		loadWeapons(true);

		// Load map levels - doing this after db npcs are loaded incase
		// some level scripts may require access to the databases.
		log::printLine(log::server, "Pre-loading map levels...");
		loadMapLevels();
	}

	return 0;
}

void Server::loadWeapons(bool print)
{
	auto indent = log::server.indent();
	{
		auto sectionProfile = log::Profile(log::server, "", "(Completed in {1:0.6} ms)");

		FileSystem weaponFS;
		weaponFS.addDir("weapons", "weapon*.txt");
		FileSystem bcweaponFS;
		bcweaponFS.addDir("weapon_bytecode", "*");

		auto& weaponFileList = weaponFS.getFileList();
		for (auto& weaponFile : weaponFileList)
		{
			auto profile = log::Profile(log::server, "", " ({1:0.6} ms)");

			auto weapon = Weapon::loadWeapon(weaponFile.first);
			if (weapon == nullptr) continue;

			/*
			if (weapon->getByteCodeFile().empty())
				weapon->setModTime(weaponFS.getModTime(weaponFile.first));
			else
				weapon->setModTime(bcweaponFS.getModTime(weapon->getByteCodeFile()));
			*/

			// Check if the weapon exists.
			if (m_weaponList.find(weapon->name) == m_weaponList.end())
			{
				m_weaponList[weapon->name] = weapon;
				if (print) log::print(log::server, weapon->name);
			}
			else
			{
				// If the weapon exists, and the version on disk is newer, reload it.
				auto& w = m_weaponList[weapon->name];
				if (w->modTime < weapon->modTime)
				{
					m_weaponList[weapon->name] = weapon;
					updateWeaponForPlayers(weapon);
					if (print)
					{
						log::print(log::server, "{} [updated]", weapon->name);
						Server::sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: Updated weapon " << weapon->name << " ");
					}
				}
				else
				{
					// TODO(joey): even though were deleting the weapon because its skipped, its still queuing its script action
					//	and attempting to execute it. Technically the code needs to be run again though, will fix soon.
					if (print) log::print(log::server, "{} [skipped]", weapon->name);
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
}

void Server::loadMapLevels()
{
	auto indent = log::server.indent();
	{
		auto sectionProfile = log::Profile(log::server, "", "(Completed in {1:0.6} ms)");
		for (const auto& map : m_mapList)
			map->loadMapLevels();
	}
}

void Server::saveServerFlags()
{
	CString out;
	for (auto& [flag, value] : Scripting.variables.store)
	{
		if (auto serialized = value->serializeModern(flag); serialized.has_value())
			out << serialized.value() << "\r\n";
	}
	out.save(CString() << "serverflags.txt");
}

void Server::saveWeapons()
{
	FileSystem weaponFS;
	weaponFS.addDir("weapons", "weapon*.txt");
	//const std::map<CString, CString>& weaponFileList = weaponFS.getFileList();

	for (auto& [weaponName, weapon]: m_weaponList)
	{
		if (weapon->isDefault())
			continue;

		// TODO(joey): add a function to weapon to get the filename?
		CString weaponFile = CString("weapon") << weaponName << ".txt";
		auto mod = clock::from_time_t(weaponFS.getModTime(weaponFile));
		if (weapon->modTime > mod)
		{
			// The weapon in memory is newer than the weapon on disk.  Save it.
			weapon->saveWeapon();
			weaponFS.setModTime(weaponFS.find(weaponFile), weapon->modTime.time_since_epoch().count());
		}
	}
}

/////////////////////////////////////////////////////

std::shared_ptr<Level> Server::stubOrGetLevel(std::string_view levelName)
{
	std::string lowerCaseLevel = string::toLower(levelName);
	if (auto it = m_levelList.find(lowerCaseLevel); it != m_levelList.end())
		return it->second;

	auto level = Level::createLevel(511, levelName);
	m_levelList.insert(std::make_pair(lowerCaseLevel, level));
	return level;
}

std::shared_ptr<Level> Server::getLevel(std::string_view levelName)
{
	if (levelName.empty())
		return nullptr;

	LevelPtr level = nullptr;

	// Find the level.
	std::string lowerCaseLevel = string::toLower(levelName);
	if (auto it = m_levelList.find(lowerCaseLevel); it != m_levelList.end())
		level = it->second;

	// Level was already loaded.
	if (level != nullptr && level->loaded)
		return level;

	FileSystem* fileSystem = &m_filesystem[FS_ALL];
	if (!m_settings.getBool("nofoldersconfig", false))
		fileSystem = &m_filesystem[FS_LEVEL];

	// If the level was not found, check if it was an absolute path.
	if (fileSystem->find(levelName).trim().length() == 0)
	{
		if (std::filesystem::exists(levelName))
		{
			fileSystem->addFile(levelName);
			fileSystem->addDir(getPath(levelName), "*", true);
		}
		else return nullptr;
	}

	// Load the level.
	if (level != nullptr)
		level = LevelLoader::loadLevelInto(level, std::filesystem::path{ levelName });
	else
	{
		level = LevelLoader::loadLevel(std::filesystem::path{ levelName });
		if (level != nullptr)
			m_levelList.insert(std::make_pair(lowerCaseLevel, level));
	}

	return level;
}

std::shared_ptr<Map> Server::findMap(std::string_view mapName) const noexcept
{
	auto foundMap = std::ranges::find_if(m_mapList, [&mapName](const auto& map) { return map->getMapName() == mapName; });
	if (foundMap != std::ranges::end(m_mapList))
		return *foundMap;
	return nullptr;
}

/////////////////////////////////////////////////////

LevelItemType Server::rollBushItemDrop() const
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 100);
    int roll = dist(gen);

	for (const auto& [itemType, rate] : m_bushDrops)
	{
		if (roll <= rate)
			return itemType;
		roll -= rate;
	}

	return LevelItemType::INVALID;
}

/////////////////////////////////////////////////////

std::shared_ptr<Weapon> Server::getWeapon(std::string_view name)
{
	auto iter = m_weaponList.find(name);
	if (iter == std::end(m_weaponList))
		return nullptr;
	return iter->second;
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

std::shared_ptr<NPC> Server::addNPC(std::string_view image, std::string_view script, float x, float y, std::weak_ptr<Level> level, NPCStorageType storageType, bool sendToPlayers)
{
	LevelPtr levelPtr = level.lock();
	if (storageType == NPCStorageType::LEVEL && levelPtr == nullptr)
		return nullptr;

	// Pick the ID range for the NPC ID.
	auto startId = NPCID_GEN_LOCAL;
	if (storageType == NPCStorageType::DATABASE)
		startId = NPCID_GEN_DATABASE_LOCALN;

	// Get available NPC ID.
	NPCID newId = m_npcIdGenerator.getAvailableId(startId);

	// New NPC
	auto newNPC = std::make_shared<NPC>(newId, storageType);

	// Add the NPC to the list.
	m_npcList.insert(std::make_pair(newId, newNPC));

	// Set the default warp type.
	newNPC->warpRestrictions = hasNPCServer() ? NPCWarpRestrictions::NOTALLOWED : NPCWarpRestrictions::ALLOWED;

	// Set the NPC's name.
	{
		std::string npcNamePrefix = std::format("localnpc_{}_{}_", removeExtension(levelPtr->levelName), m_serverTime);
		auto count = std::ranges::count_if(m_npcList, [&npcNamePrefix](const auto& pair)
		{
			return pair.second->name.starts_with(npcNamePrefix);
		});

		newNPC->name = std::format("{}{}", npcNamePrefix, (count + 1));
	}

	// Set the script type.
	if (storageType == NPCStorageType::LEVEL)
		newNPC->scriptType = NPCTYPE_LOCAL;
	else newNPC->scriptType = NPCTYPE_OBJECT;

	// Set NPC props.
	newNPC->setLevel(level.lock());
	newNPC->character.localPixelX = x * 16;
	newNPC->character.localPixelY = y * 16;
	newNPC->image = image;

	// If the level is a gmap, set the modTime on the level props.
	if (auto map = levelPtr->getMap(); map && map->isGmap())
	{
		newNPC->character.mapX = levelPtr->mapPosition.x();
		newNPC->character.mapY = levelPtr->mapPosition.y();
		newNPC->modTime[PROPID(NPCProp::GMAPLEVELX)] = m_frameStartTime;
		newNPC->modTime[PROPID(NPCProp::GMAPLEVELY)] = m_frameStartTime;
	}

	// Set the script and record the initial state.
	newNPC->setScript(script);
	newNPC->recordInitialState();

	// Add the NPC to the level.
	if (levelPtr != nullptr)
		levelPtr->addNPC(newNPC);

	// Created event.
	if (hasNPCServer())
	{
		newNPC->scripting.events.addEvent(ScriptEventType::CREATED, source::FromServer());
	}

#ifdef DEBUG
	log::printLine(log::server, "Adding NPC [{}] '{}' at ({}, {}) in level '{}'.", newNPC->id, newNPC->name, newNPC->character.localPixelX, newNPC->character.localPixelY, levelPtr ? levelPtr->levelName : "null");
#endif

	// Send the NPC's props to everybody in range.
	if (sendToPlayers)
	{
		CString packet = CString() >> (char)PLO_NPCPROPS >> (int)newNPC->id << newNPC->getAllPropsPacket();
		sendPacketToNearby(packet, newNPC->getGlobalPosition(), levelPtr);
	}

	return newNPC;
}

std::shared_ptr<NPC> Server::addNPC(NPCPtr npc, bool sendToPlayers)
{
	// Add the NPC to the list.
	m_npcList.insert(std::make_pair(npc->id, npc));

	// Send the NPC's props to everybody in range.
	if (sendToPlayers)
	{
		CString packet = CString() >> (char)PLO_NPCPROPS >> (int)npc->id << npc->getAllPropsPacket();
		sendPacketToNearby(packet, npc->getGlobalPosition(), npc->getLevel());
	}

	return npc;
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
	m_npcList.erase(npc->id);
	m_npcIdGenerator.freeId(npc->id);

	if (auto level = npc->getLevel(); level)
	{
		// Remove the NPC from the level
		if (eraseFromLevel)
			level->removeNPC(npc);

		// Tell the clients to delete the NPC.
		std::string levelName = npc->getLevelName();

		auto lastLevelChange = clock::to_time_t(npc->modTime[PROPID(NPCProp::CURLEVEL)]);
		for (auto& [pid, p]: m_playerList)
		{
			auto playerClient = std::dynamic_pointer_cast<PlayerClient>(p);
			if (playerClient != nullptr && (playerClient->getLevelLastEnteredTime(level.get()) >= lastLevelChange || playerClient->getLevel() == level))
			{
				if (playerClient->getComputedLevelName() != levelName)
					p->sendPacket(CString() >> (char)PLO_NPCDEL2 >> (char)levelName.length() << levelName >> (int)npc->id);
				else p->sendPacket(CString() >> (char)PLO_NPCDEL >> (int)npc->id);
			}
			else if (p->isNC())
			{
				p->sendPacket(CString() >> (char)PLO_NC_NPCDELETE >> (int)npc->id);
			}
		}
	}

	return true;
}

bool Server::addPlayer(PlayerPtr player, PlayerID id)
{
	assert(player);

	// No id was passed, so we will fetch one
	if (id == USHRT_MAX)
		id = m_playerIdGenerator.getAvailableId();

	// Add them to the player list.
	player->setId(id);
	m_playerList[id] = player;

	return true;
}

bool Server::deletePlayer(PlayerPtr player)
{
	if (player == nullptr)
		return true;

	if (player->isLoaded())
	{
		// If we have an NPC-Server, let it process the player first.
		// TODO(NPCServer): Might need to check for remote NPC-Servers in the future here.
		if (hasNPCServer() && player->isClient())
			m_npcServer->playerLogout(player);

		// Leave the level.
		if (auto client = std::dynamic_pointer_cast<PlayerClient>(player); client != nullptr)
			client->leaveLevel();

		// Add the player to the set of players to delete.
		getServerList().deletePlayer(player);
	}

	m_playerList.erase(player->getId());

	// The ID will be freed in Player::cleanup.
	// If we clear it now, then a player who presses F8 and reconnects will enter a race condition where they
	// may get the same ID as their previous connection and then be immediately disconnected.
	// m_playerIdGenerator.freeId(player->getId());

	return true;
}

bool Server::swapPlayer(PlayerPtr old_player, PlayerPtr new_player)
{
	if (old_player == nullptr || new_player == nullptr)
		return false;

	auto id = old_player->getId();

	// Swap the player in the player list.
	m_playerList.erase(id);
	m_playerList[id] = new_player;

	// Set the id on the new player.
	new_player->setId(id);

	// Update the socket manager.
	m_sockManager.unregisterSocket(old_player.get());
	m_sockManager.registerSocket(new_player.get());

	// If we are an npc-server, fix our id.
	if (new_player->isNPCServer() && id != NPCServerPlayerID)
	{
		m_playerList.erase(id);
		new_player->setId(NPCServerPlayerID);
		m_playerList[NPCServerPlayerID] = new_player;
	}

	return true;
}

void Server::recordPlayerLoggedIn(PlayerPtr player)
{
	// Tell the serverlist that the player connected.
	getServerList().addPlayer(player);
}

bool Server::warpPlayerToSafePlace(PlayerID playerId)
{
	auto player = getPlayer<PlayerClient>(playerId);
	if (player == nullptr) return false;

	// Try unstick me level.
	CString unstickLevel = m_settings.getStr("unstickmelevel", "onlinestartlocal.nw");
	float unstickX = m_settings.getFloat("unstickmex", 30.0f);
	float unstickY = m_settings.getFloat("unstickmey", 30.5f);
	return player->warp(unstickLevel, { static_cast<int16_t>(unstickX * 16.0f), static_cast<int16_t>(unstickY * 16.0f) });

	// TODO: Maybe try the default account level?
}

//----------------------------

void Server::calculateNWTime()
{
	// Thu Feb 01 2001 17:33:34 GMT+0000
	// this is likely the actual start time of timevar
	m_serverTime = static_cast<uint32_t>((time(nullptr) - 981048814) / 5);
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

void Server::logToFile(const std::string& fileName, const std::string& message) const
{
	std::filesystem::path file{ fileName };
	log::Log logFile{ .filename = std::filesystem::path{ "servers" } / m_name.text() / "logs" / file.filename() };

	log::printLine(logFile, "\n{}", message);
	logFile.close();
}

/*
	Server: Server Flag Management
*/

std::optional<std::string> Server::getFlag(std::string_view flagName) const
{
	auto flagVal = Scripting.variables.get(flagName);
	if (auto flag = flagVal.lock(); flag != nullptr)
		return flag->get<std::string>();
	return std::nullopt;
}

bool Server::deleteFlag(std::string_view flagName, bool sendToPlayers)
{
	if (m_settings.getBool("dontaddserverflags", false))
		return false;

	if (Scripting.variables.remove(flagName))
	{
		if (sendToPlayers)
			sendPacketToAll(CString() >> (char)PLO_FLAGDEL << flagName);
		return true;
	}

	return false;
}

bool Server::setFlag(std::string_view flagPair, bool sendToPlayers)
{
	if (!flagPair.contains('='))
		return setFlag(flagPair, std::nullopt, sendToPlayers);

	auto separator = flagPair.find('=');
	auto flagName = string::trim(flagPair.substr(0, separator));
	auto flagValue = string::trim(flagPair.substr(separator + 1));
	return setFlag(flagName, std::string{ flagValue }, sendToPlayers);
}

bool Server::setFlag(std::string_view flagName, std::optional<std::string> flagValue, bool pSendToPlayers)
{
	if (m_settings.getBool("dontaddserverflags", false))
		return false;

	// Function to crop flags.
	auto cropFlag = [this, &flagName](std::string& value)
	{
		if (m_settings.getBool("cropflags", true))
			value.erase(std::min(value.length(), static_cast<size_t>(223 - 1) - flagName.length()));
		return value;
	};

	// Alter the flag if it exists.
	auto existing = Scripting.variables.get(flagName).lock();
	if (existing != nullptr)
	{
		//bool isFlag = existing->has<bool>() && !existing->has<std::string>();
		bool isStringFlag = existing->has<std::string>();

		// No change.
		if (!flagValue.has_value())
			return true;

		// If flag value is empty, delete.
		if (isStringFlag && flagValue.value().empty())
			return deleteFlag(flagName);

		// Alter value.
		existing->assign<std::string>(cropFlag(flagValue.value()));
	}
	// New flag.
	else
	{
		if (!flagValue.has_value())
			Scripting.variables.add(flagName, GameValue{ true });
		else Scripting.variables.add(flagName, GameValue{ cropFlag(flagValue.value()) });
	}

	// And share it.
	if (pSendToPlayers && (!hasNPCServer() || flagName.starts_with("serverr.")))
	{
		if (!flagValue.has_value())
			sendPacketToAll(CString() >> (char)PLO_FLAGSET << flagName);
		else sendPacketToAll(CString() >> (char)PLO_FLAGSET << flagName << "=" << flagValue.value());
	}

	return true;
}

void Server::hitObjectsAtPoint(const TilePosition& pos, int8_t power, std::weak_ptr<Level> level, PlayerPtr source) const
{
	// Client ignores if not within 2 tiles in both X/Y.
	sendPacketToNearby(CString() >> (char)PLO_HITOBJECTS >> (short)source->getId() >> (char)power >> (char)(pos.x() * 2) >> (char)(pos.y() * 2), toPixelPosition(pos), level.lock());
}

void Server::hitObjectsAtPoint(const TilePosition& pos, int8_t power, std::weak_ptr<Level> level, NPCPtr source) const
{
	// Client ignores if not within 2 tiles in both X/Y.
	sendPacketToNearby(CString() >> (char)PLO_HITOBJECTS >> (short)0 >> (char)power >> (char)(pos.x() * 2) >> (char)(pos.y() * 2) >> (int)source->id, toPixelPosition(pos), level.lock());
}

void Server::hitPlayer(PlayerID playerId, int8_t power, float fromX, float fromY, std::shared_ptr<NPC> source) const
{
	auto player = getPlayer(playerId);
	if (player == nullptr)
		return;

	// Client ignores if PLO_DISABLECLASSICMODE was sent.
	// Client ignores if the source wasn't within 10 tiles.

	auto tilePosition = player->getTilePosition();
	auto dx = tilePosition.x() - fromX;
	auto dy = tilePosition.y() - fromY;

	// Normalize the direction vector.
	float length = std::sqrt(dx * dx + dy * dy);
	if (!DoubleIsZero(dx))
		dx /= length;
	if (!DoubleIsZero(dy))
		dy /= length;

	// Push out 4 tiles.
	dx *= 4;
	dy *= 4;

	// Pixel position.
	auto encodedDX = static_cast<uint8_t>(static_cast<int16_t>(dx * 16) + 64);
	auto encodedDY = static_cast<uint8_t>(static_cast<int16_t>(dy * 16) + 64);

	// Send the final packet.
	player->sendPacket(CString() >> (char)PLO_HURTPLAYER >> (short)0 >> (char)(encodedDX) >> (char)(encodedDY) >> (char)power >> (int)source->id);
}

void Server::sendTriggerAction(PlayerID toPlayerId, NPCID fromNpcId, const LocalPixelPosition& localPosition, std::string_view action, std::string_view params) const
{
	auto player = getPlayer(toPlayerId);
	if (player == nullptr)
		return;

	CString packet = CString() >> (char)PLO_TRIGGERACTION >> (short)0 >> (int)fromNpcId >> (char)(localPosition.x() / 8.0f) >> (char)(localPosition.y() / 8.0f) << action << "," << params;
	player->sendPacket(packet);
}

void Server::sendTriggerAction(LevelPtr toLevel, NPCID fromNpcId, const PixelPosition& position, std::string_view action, std::string_view params) const
{
	if (toLevel == nullptr)
		return;

	auto localPosition = toLocalPixelPosition(position);
	CString packet = CString() >> (char)PLO_TRIGGERACTION >> (short)0 >> (int)fromNpcId >> (char)(localPosition.x() / 8.0f) >> (char)(localPosition.y() / 8.0f) << action << "," << params;
	sendPacketToNearby(packet, position, toLevel);
}

/*
	Packet-Sending Functions
*/

void Server::sendPacketToAll(const CString& packet, const std::set<PlayerID>& exclude) const
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

void Server::sendPacketToOneLevel(const CString& packet, std::weak_ptr<Level> level, const std::set<PlayerID>& exclude) const
{
	auto levelp = level.lock();
	if (!levelp) return;

	for (auto id: levelp->getLevelPlayers())
	{
		if (exclude.contains(id)) continue;
		if (auto player = this->getPlayer(id); player && player->isClient())
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

void Server::sendPacketToLevelAndPastVisitorsAfter(Level* level, time_t modTime, const CString& packet) const
{
	if (!running) return;
	for (const auto& [id, player] : players_of_type<PlayerClient>(m_playerList))
	{
		auto playerLevel = player->getLevel();
		if (player->getLevelLastEnteredTime(level) > modTime || (playerLevel != nullptr && playerLevel.get() == level))
			player->sendPacket(packet);
	}
}

void Server::sendPacketToNearby(const CString& packet, const PixelPosition& position, std::shared_ptr<Level> level, const std::set<PlayerID>& exclude, PlayerPredicate sendIf) const
{
	if (!running || level == nullptr) return;

	auto levelName = level->getMapOrLevelName();
	auto players = level->findInRangePlayersForCommunication(position);
	for (const auto& playerId : players)
	{
		if (exclude.contains(playerId))
			continue;
		if (auto player = getPlayer<PlayerClient>(playerId); player != nullptr && (!sendIf || sendIf(player.get())))
		{
			bool sameLevel = levelName == player->getComputedLevelName();

			// TODO: Figure out when PLO_SETACTIVELEVEL was introduced.
			if (!sameLevel && player->getVersion() < CLVER_2_17)
				continue;

			if (!sameLevel) player->sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << level->levelName);
			player->sendPacket(packet);
			if (!sameLevel) player->sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << player->getComputedLevelName());
		}
	}
}

/// @brief Sends a board update packet to all nearby players in a level (sending PLO_LEVELNAME before and after the packet).
///
/// Board updates are processed in a different way by the client.  In order to cross gmap levels correctly, we need to force the level name first.
void Server::sendBoardUpdatePacketToNearby(const CString& packet, const PixelPosition& position, std::shared_ptr<Level> level, const std::set<PlayerID>& exclude, PlayerPredicate sendIf) const
{
	if (!running || level == nullptr) return;

	auto levelName = level->getMapOrLevelName();
	auto players = level->findInRangePlayersForCommunication(position);
	for (const auto& playerId : players)
	{
		if (exclude.contains(playerId))
			continue;
		if (auto player = getPlayer<PlayerClient>(playerId); player != nullptr && (!sendIf || sendIf(player.get())))
		{
			player->sendPacket(CString() >> (char)PLO_LEVELNAME << level->levelName);
			player->sendPacket(packet);
			player->sendPacket(CString() >> (char)PLO_LEVELNAME << player->getComputedLevelName());
		}
	}
}

/*
	NPC-Server Functionality
*/
bool Server::NC_AddWeapon(std::shared_ptr<Weapon> pWeaponObj)
{
	if (pWeaponObj == nullptr)
		return false;

	m_weaponList[pWeaponObj->name] = pWeaponObj;
	return true;
}

bool Server::NC_DelWeapon(std::string_view pWeaponName)
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
	CString filePath = CString() << "weapons/weapon" << name << ".txt";
	FileSystem::fixPathSeparators(filePath);
	remove(filePath.text());

	// Delete from Memory
	m_weaponList.erase(std::string{ pWeaponName });

	// Delete from Players
	sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_NPCWEAPONDEL << pWeaponName);
	return true;
}

void Server::updateWeaponForPlayers(Weapon* weapon)
{
	if (weapon == nullptr)
		return;

	CString weaponPacket = weapon->getAddWeaponPacket();

	// Update Weapons
	for (auto& [id, player]: m_playerList)
	{
		if (!player->isClient())
			continue;

		if (player->account.hasWeapon(weapon->name))
		{
			player->sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << weapon->name);
			if (!weaponPacket.isEmpty())
				player->sendPacket(weaponPacket);
		}
	}
}

void Server::updateWeaponForPlayers(std::shared_ptr<Weapon> weapon)
{
	updateWeaponForPlayers(weapon.get());
}

// TODO(Nalin): This should probably be in the NPCServer class.
void Server::updateClassForPlayers(std::shared_ptr<ScriptClass> scriptClass)
{
	CString classPacket = scriptClass->getClassPacket();
	if (classPacket.isEmpty())
		return;

	// Update players.
	for (auto& [id, player] : m_playerList)
	{
		if (!player->isClient())
			continue;

		player->sendPacket(CString() >> (char)PLO_RAWDATA >> (int)classPacket.length());
		player->sendPacket(classPacket);
	}
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
		output.trimRight().save(CString() << "translations/" << language.first.c_str() << ".po");
	}
}

void Server::sendShootToOneLevel(LevelShoot* shoot, std::shared_ptr<Level> level) const
{
	if (shoot == nullptr || level == nullptr)
		return;

	float pi = std::numbers::pi_v<float>;
	float halfpi = pi / 2;

	ShootPacketWrapper newPacket{};
	newPacket.source = (shoot->from.second == ScriptObjectType::NPC ? shoot->from.first : 0);
	newPacket.position = toLocalPixelPosition(shoot->position);
	newPacket.offsetx = 0;
	newPacket.offsety = 0;
	newPacket.sangle = static_cast<uint8_t>(220 * (std::clamp(shoot->angle, 0.0f, 2 * pi) / (2 * pi)));
	newPacket.sanglez = std::clamp(110 + static_cast<uint8_t>(110 * (std::clamp(shoot->zangle, -halfpi, halfpi) / halfpi)), 0, 220);
	newPacket.power = shoot->powerIn44Pixels;
	newPacket.gravity = static_cast<uint8_t>(shoot->gravity * 16);
	newPacket.gani = shoot->gani;
	newPacket.shootParams = string::toCSV(getShootParams());

	CString oldPacketBuf = CString() >> (char)PLO_SHOOT >> (short)0 << newPacket.constructShootV1();
	CString newPacketBuf = CString() >> (char)PLO_SHOOT2 >> (short)0 << newPacket.constructShootV2();

	sendPacketToNearby(oldPacketBuf, level->convertToMapPosition(newPacket.position), level, { 0 }, [](const auto pl) { return pl->getVersion() < CLVER_5_07; });
	sendPacketToNearby(newPacketBuf, level->convertToMapPosition(newPacket.position), level, { 0 }, [](const auto pl) { return pl->getVersion() >= CLVER_5_07; });
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
