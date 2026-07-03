#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <numbers>
#include <optional>
#include <random>
#include <ranges>
#include <set>
#include <string.h>
#include <string_view>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <version>

#include <CSocket.h>

#include <CString.h>
#include <IEnums.h>
#include <IUtil.h>

#include <BabyDI.h>
#include <Server.h>
#include <filesystem/File.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <level/Level.h>
#include <level/LevelItem.h>
#include <level/LevelShoot.h>
#include <level/LevelTileTypes.h>
#include <level/Map.h>
#include <loader/LevelLoader.h>
#include <loader/flatfile/FlatFileAccountLoader.h>
#include <loader/flatfile/FlatFileNPCLoader.h>
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
#include <utilities/manager/GuildManager.h>
#include <utilities/manager/ITranslationManager.h>
#include <utilities/manager/TranslationManagerClassic.h>
#include <utilities/manager/TranslationManagerModern.h>

///////////////////////////////////////////////////////////////////////////////

extern std::atomic_bool shutdownProgram;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

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
	CString result{first};
	return result << second;
}

///////////////////////////////////////////////////////////////////////////////

void ExternalServerCachedSettings::bind(Server* server)
{
	auto& settings = server->getSettings();

	settings.track(maxPlayers, sleepWhenNoPlayers);
	settings.track(unstickMeLevel, unstickMeTile[0], unstickMeTile[1], unstickMeSeconds);
	settings.track(enableBushItemDrops, enableVaseItemDrops, disableItemDropping);
	settings.track(enableInsideSyncDistance, syncDistance[0], syncDistance[1]);
	settings.track(eventDistance, triggerDistance, sendTriggerActionsToPlayers);
	settings.track(enableFlagCropping, disableExplosions, enableClientsidePushPull, tileRespawnTime, enableIdleDisconnect, idleTimeoutSeconds);
	settings.track(enablePermanentTileChanges, saveTileChangesToLevelFile);

	// triggerhacks
	settings.track(enableFlaghackMovement, enableTriggerhackExecscript, enableTriggerhackFiles, enableTriggerhackGroups, enableTriggerhackGuilds, enableTriggerhackLevels, enableTriggerhackProps, enableTriggerhackRC, enableTriggerhackWeapons);

	// npc-server
	settings.track(forceClientsideLinks, forceClientsideSigns, enableItemDropEvents, itemDropEventsOnlyForGralats, projectilesStopOnWall, runAllScriptEvents);

	// security
	settings.track(normalAdminsCanChangeGralats, protectedWeapons, jailLevels);

	// player
	settings.track(enableDefaultWeapons, maxHeartLimit, enableExBodyColors, playerTouchesMeNoZ, lockPlayerZ);
	settings.track(enableAPSystem, apSystemThresholdSeconds[0], apSystemThresholdSeconds[1], apSystemThresholdSeconds[2], apSystemThresholdSeconds[3], apSystemThresholdSeconds[4]);
	settings.track(playerProfileVariables, playerStatusList);
}

///////////////////////////////////////////////////////////////////////////////

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

	m_timedEvents1s.callbackIterations = std::bind(&Server::doTimedEvents, this, std::placeholders::_1);
	m_timedSave1m.callbackIterations = [this](int)
	{
		saveServerFlags();
		if (auto guild = BabyDI::Get<GuildManager>(); guild != nullptr)
			guild->saveGuilds();
	};
	m_timedNWTime5s.callbackIterations = [this](int)
	{
		calculateNWTime();
		sendPacketToAll(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(getNWTime()));
	};
	m_timedMaintenance5m.callbackIterations = [this](int)
	{
		// Reload some server settings.
		loadAllowedVersions();
		loadServerMessage();
		loadIPBans();

		// Check if we need to unload any levels.
		for (auto& [levelName, level] : m_levelList)
		{
			// TODO: Gmap sub-level (and maybe static level) unloading.  Needs to follow Map::keepAllLevelsLoaded and levelsToKeepInMemory settings.

			// Skip if the level is currently active with players in it.
			// We always do this so we can abort early.
			if (!level->timeSinceLastPlayerLeft.has_value())
				continue;
			
			// Register that we will skip if we have the unload time set to 0 (which means never unload).
			bool skip = (m_unloadInactiveLevelTime.getValue() == 0);

			// Give a 10 minute grace period after the last player leaves.
			auto inactiveDuration = timeDifference(m_frameStartTime, level->timeSinceLastPlayerLeft.value());
			skip = skip || inactiveDuration < std::chrono::seconds(m_unloadInactiveLevelTime.getValue());

			// Group maps will always unload after 10 minutes if the inactive time is not set.
			if (level->isGroupMap && inactiveDuration > std::chrono::seconds(m_unloadInactiveLevelTime.get().value_or(600)))
				skip = false;

			// Do the skip now.
			if (skip)
				continue;

			DEBUGPRINT("Unloading level '{}' due to inactivity.", levelName);

			// If we have an NPC-server, unload (or delete) our serverside NPCs.
			if (hasNPCServer())
			{
				if (level->isGroupMap)
				{
					for (const auto& id : level->getNPCs())
						m_npcServer->deleteNPC(id);
				}
				else
				{
					for (const auto& id : level->getNPCs())
						m_npcServer->unloadNPC(id);
				}
			}

			level = nullptr;
		}

		std::erase_if(m_levelList, [](const auto& entry)
		{
			return entry.second == nullptr;
		});
	};

	m_fsServer.categoryEventCallback[ENUM(fs::FileCategory::CONFIG)] = [this](fs::FileEventCollection events, fs::FileData& file)
	{
		if (events.test(fs::FileEvent::Modified))
		{
			auto fileName = fs::getANSIFileName(file.file);
			if (fileName == "serveroptions.txt")
			{
				loadSettings();

				// TODO: Map loading needs to be improved to deal with maps being added/removed, and to fix a level's link to a map.
				// Levels have a shared_ptr to the map.  Should it be switched to a weak_ptr?
				//loadMaps();
			}
			else if (fileName == "adminconfig.txt")
				loadAdminSettings();
			else if (fileName == "allowedversions.txt")
				loadAllowedVersions();
			else if (fileName == "foldersconfig.txt")
				loadWorldFileSystem();
			else if (fileName == "serverflags.txt")
				loadServerFlags();
			else if (fileName == "servermessage.html")
				loadServerMessage();
			else if (fileName == "ipbans.txt")
				loadIPBans();
			else if (fileName == "rules.txt")
				loadWordFilter();
		}
	};
	m_fsServer.categoryEventCallback[ENUM(fs::FileCategory::NPC)] = [this](fs::FileEventCollection events, fs::FileData& file)
	{
		if (!hasNPCServer())
			return;

		if (events.test(fs::FileEvent::Deleted))
		{
			auto npcName = fs::getANSIFileName(fs::getHTMLUnescapedFileName(file.file));
			if (npcName.starts_with("npc") && npcName.ends_with(".txt"))
				npcName = npcName.substr(3, npcName.size() - 7); // Remove npc and .txt

			if (auto npc = m_npcServer->getNPCByName(npcName).lock(); npc != nullptr)
			{
				log::printLine(log::server, "NPC deleted from filesystem: [{}] {}", npc->id, npc->name);
				m_npcServer->deleteNPC(npc->id);
			}
		}
		if (events.test(fs::FileEvent::Added))
		{
			auto profile = log::Profile(log::server, "", " ({1:0.6} ms)");
			if (auto npc = m_npcServer->addNPCFromFile(file.file); npc != nullptr)
			{
				// TODO: Generic prop sending function NPCs.
				CString packet = CString() >> (char)PLO_NPCPROPS >> (int)npc->id << npc->getAllPropsPacket();
				sendPacketToNearby(packet, npc->getGlobalPosition(), npc->getLevel());

				log::printLine(log::server, "NPC added to filesystem: [{}] {}", npc->id, file.file.stem().generic_string());
			}
		}
		if (events.test(fs::FileEvent::Modified))
		{
			fs::File npcFile{file.file};
			auto id = string::toNumber<NPCID>(npcFile.readConfigLine("ID", " ").value_or("0"));
			if (id == 0)
				return;

			auto npc = getNPC(id);
			if (npc == nullptr || npc->lastSaveTime == file.getModTime()) return;
			npc->lastSaveTime = file.getModTime();

			// TODO: Ability to serialize all the attributes from the file and send changed ones.

			auto script = npcFile.readConfigSection("NPCSCRIPT", "NPCSCRIPTEND");
			if (script.has_value())
			{
				npc->setScript(script.value());
				npc->scripting.events.addEvent(ScriptEventType::CREATED, source::FromServer());
				npc->sendScriptUpdatesToLevel(file.getModTime());

				std::string logMsg = std::format("NPC script updated on filesystem: [{}] {}", npc->id, npc->name);
				log::printLine(log::npc, logMsg);
				sendToNC(logMsg);
			}
		}
	};
	m_fsServer.categoryEventCallback[ENUM(fs::FileCategory::SCRIPTCLASS)] = [this](fs::FileEventCollection events, fs::FileData& file)
	{
		if (!hasNPCServer())
			return;

		auto className = file.file.stem().string();
		std::string logMsg;

		if (events.test(fs::FileEvent::Deleted))
		{
			auto className = file.file.stem().string();
			if (m_npcServer->deleteClass(className))
			{
				sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_CLASSDELETE << className);
				logMsg = std::format("Class deleted from filesystem: {}", className);
			}
		}
		if (events.test(fs::FileEvent::Added))
		{
			// Class already exists so it was added by NC.
			if (auto existingClass = m_npcServer->getClass(className); !existingClass.expired())
				return;

			auto className = file.file.stem().string();
			if (auto scriptClass = m_npcServer->loadClass(file.file); scriptClass != nullptr)
			{
				sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_CLASSADD << className);
				logMsg = std::format("Class added to filesystem: {}", className);
			}
		}
		if (events.test(fs::FileEvent::Modified))
		{
			// Class mod time matches the file mod time?  Then NC modified it, not the FS.
			auto fileModTime = fs::getFileModTime(file.file);
			if (auto existingClass = m_npcServer->getClass(className).lock(); existingClass && existingClass->modTime == fileModTime)
				return;

			fs::File script{file.file};
			m_npcServer->updateClass(className, script.readAsString());
			logMsg = std::format("Class updated on filesystem: {}", className);
		}

		if (!logMsg.empty())
		{
			log::printLine(log::npc, logMsg);
			sendToNC(logMsg);
		}
	};
	m_fsServer.categoryEventCallback[ENUM(fs::FileCategory::TRANSLATION)] = [this](fs::FileEventCollection events, fs::FileData& file)
	{
		if (events.test(fs::FileEvent::Modified))
		{
			auto translationManager = BabyDI::Get<ITranslationManager>();
			translationManager->reloadTranslation(file.file);
		}
	};
	m_fsServer.categoryEventCallback[ENUM(fs::FileCategory::WEAPON)] = [this](fs::FileEventCollection events, fs::FileData& file)
	{
		if (events.test(fs::FileEvent::Deleted))
		{
			auto weaponName = fs::getANSIFileName(fs::getHTMLEscapedFileName(file.file.stem())).substr(6);
			if (NC_DelWeapon(weaponName))
			{
				auto logMsg = std::format("Weapon deleted from filesystem: {}", weaponName);
				log::printLine(log::npc, logMsg);
				sendToNC(logMsg);
			}
		}
		if (events.test(fs::FileEvent::Modified))
		{
			auto fileName = fs::getANSIFileName(file.file);
			auto newWeapon = Weapon::loadWeapon(fileName);
			if (auto weapon = getWeapon(newWeapon->name); weapon)
			{
				if (weapon->name != newWeapon->name)
				{
					log::printLine(log::server, "Weapon name mismatch ('{}' became '{}'), old weapon will be deleted.", weapon->name, newWeapon->name);
					m_weaponList.erase(weapon->name);
					sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_NPCWEAPONDEL << weapon->name);
				}
				else
				{
					updateWeaponForPlayers(newWeapon);
				}
			}
			m_weaponList[newWeapon->name] = newWeapon;
		}
	};

	m_fsWorld.categoryEventCallback[ENUM(fs::FileCategory::LEVEL)] = [this](fs::FileEventCollection events, fs::FileData& file)
	{
		if (events.test(fs::FileEvent::Deleted))
		{
			// When the level gets deleted, players will be warped out.
			m_levelList.erase(fs::getANSIFileName(file.file));
		}
		if (events.test(fs::FileEvent::Modified))
		{
			auto fileName = fs::getANSIFileName(file.file);
			if (auto l = getCachedLevelData(fileName); l)
				StaticLevelData::reload(l);
		}
	};
	m_fsWorld.categoryEventCallback[ENUM(fs::FileCategory::FILE)] = [this](fs::FileEventCollection events, fs::FileData& file)
	{
		auto fileName = fs::getANSIFileName(file.file);
		auto ext = file.file.extension();
		if (events.test(fs::FileEvent::Deleted))
		{
			if (ext == ".gupd")
				m_packageManager.deleteResource(fileName);
		}
		if (events.test(fs::FileEvent::Modified))
		{
			if (ext == ".gupd")
				m_packageManager.findOrAddResource(fileName)->reload(this);
			else if (Generation == ServerGeneration::NEWMAIN || Generation == ServerGeneration::MODERN)
			{
				// Ganis need to be recompiled on update
				CString bytecodePacket;
				if (ext == ".gani")
				{
					// delete the resource
					m_animationManager.deleteResource(fileName);

					// reload the resource to compile the bytecode again
					if (auto findAni = m_animationManager.findOrAddResource(fileName); findAni)
						bytecodePacket << findAni->getBytecodePacket();
				}

				// Send the update packet to any v4+ clients that have seen this file
				CString updatePacket = CString() >> (char)PLO_UPDATEPACKAGEISUPDATED << fileName;
				for (const auto& [pid, pl] : players_of_type<PlayerClient>(m_playerList))
				{
					if (pl->hasSeenFile(fileName))
						pl->sendPacket(updatePacket);

					// Send GS2 gani scripts
					if (!bytecodePacket.isEmpty())
						pl->sendPacket(bytecodePacket);
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

int Server::init(std::string_view serverip, std::string_view serverport, std::string_view localip, std::string_view serverinterface)
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
	if (!serverip.empty())
		m_settings.set("serverip", serverip);
	if (!serverport.empty())
		m_settings.set("serverport", serverport);
	if (!localip.empty())
		m_settings.set("localip", localip);
	if (!serverinterface.empty())
		m_settings.set("serverinterface", serverinterface);

	m_overrideIp = serverip;
	m_overridePort = serverport;
	m_overrideLocalIp = localip;
	m_overrideInterface = serverinterface;

	// Fix up the interface to work properly with CSocket.
	std::string_view oInter = m_overrideInterface;
	auto sinter = m_settings.get<std::string>("serverinterface");
	if (m_overrideInterface.empty() && sinter.has_value())
		oInter = sinter.value();
	if (oInter == "AUTO")
		oInter = std::string_view{};

	// Initialize the player socket.
	m_playerSock.setType(SOCKET_TYPE_SERVER);
	m_playerSock.setProtocol(SOCKET_PROTOCOL_TCP);
	m_playerSock.setDescription("playerSock");

	// Start listening on the player socket.
	log::printLine(log::server, "Initializing player listen socket.");
	if (m_playerSock.init((oInter.empty() ? 0 : oInter.data()), m_settings.get<std::string>("serverport").value_or(""s).c_str()))
	{
		log::printLine(log::server, "** [Error] Could not initialize listening socket.");
		return ERR_LISTEN;
	}
	if (m_playerSock.connect())
	{
		log::printLine(log::server, "** [Error] Could not connect listening socket.");
		return ERR_LISTEN;
	}

	// Announce the ports.
	{
		auto indent = log::server.indent();
		log::printLine(log::server, "Listening on: {}:{}.", m_playerSock.getRemoteIp(), m_playerSock.getRemotePort());
	}

	// Start a UPNP thread.  It will try to set a UPNP port forward in the background.
#ifdef ENABLE_UPNP
	if (m_settings.get<bool>("upnp").value_or(true) && m_upnp == nullptr)
	{
		log::printLine(log::server, "Starting UPnP discovery thread.");
		m_upnp = std::make_unique<UPNP>();
		m_upnp->initialize((oInter.empty() ? m_playerSock.getLocalIp() : oInter.data()), m_settings.get<std::string>("serverport").value_or(""s).c_str());
		m_upnpThread = std::thread(std::ref(*m_upnp.get()));
	}
#endif

	// Register ourself with the socket manager.
	m_sockManager.registerSocket((CSocketStub*)this);

	// Start the timers.
	m_timedEvents1s.start();
	m_timedNWTime5s.start();
	m_timedSave1m.start();
	m_timedMaintenance5m.start();

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
			// Set running to false so we can properly clean up things during the cleanup.
			// If we don't do this, we can encounter infinite recursion during level cleanup.
			m_doRestart = false;
			running = false;

			cleanup();
			int ret = init(m_overrideIp, m_overridePort, m_overrideLocalIp, m_overrideInterface);
			if (ret != 0)
				break;

			// We are back up and running!
			running = true;
		}

		if (shutdownProgram)
			running = false;
	}
}

void Server::cleanup()
{
	// Save translations.
	auto translationManager = BabyDI::Get<ITranslationManager>();
	translationManager->saveTranslations();

	// Save server flags.
	saveServerFlags();

	// Save NPC-Server NPCs.
	if (hasNPCServer())
		m_npcServer->saveNPCs();

	m_npcList.clear();
	m_shootParams.clear();

	auto players = m_playerList | std::views::transform([](const auto& pair) { return pair.second; });
	std::vector<PlayerPtr> deletePlayers{std::ranges::begin(players), std::ranges::end(players)};
	for (auto& player : deletePlayers)
	{
		player->sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Server is shutting down.");
		player->cleanup();
	}

	m_npcServer.reset();
	m_playerList.clear();

	m_levelList.clear();
	m_mapList.clear();

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
	auto oldTime = m_frameStartTimeHighPrecision;
	m_frameStartTimeHighPrecision = precise_clock::now();
	m_frameStartTime = currentTime();

	// Update the NPC server.
	if (hasNPCServer())
		m_npcServer->update(m_frameStartTimeHighPrecision);

	// Update our events.
	m_timedEvents1s.update(m_frameStartTimeHighPrecision);
	m_timedSave1m.update(m_frameStartTimeHighPrecision);
	m_timedNWTime5s.update(m_frameStartTimeHighPrecision);
	m_timedMaintenance5m.update(m_frameStartTimeHighPrecision);

	// Do level frame events.
	for (auto& [name, level] : m_levelList)
	{
		if (level != nullptr)
			level->doFrameEvents(m_frameStartTimeHighPrecision);
	}

	// Execute our scheduled tasks.
	auto startingTasks = m_scheduledTasks.size();
	auto tasks = startingTasks;
	auto diff = m_frameStartTimeHighPrecision - oldTime;
	for (size_t i = 0; i < tasks;)
	{
		auto& task = m_scheduledTasks.at(i);
		if (task.first < diff)
		{
			// Execute the task.
			task.second();

			// Swap the finished task with end, then decrement the number of tasks.
			// We just want to erase the finished tasks at the end for efficiency.
			if (i < tasks - 1)
				std::swap(m_scheduledTasks.at(i), m_scheduledTasks.at(tasks - 1));

			--tasks;
		}
		else
		{
			task.first -= diff;
			++i;
		}
	}
	m_scheduledTasks.erase(m_scheduledTasks.begin() + tasks, m_scheduledTasks.begin() + startingTasks);

	return true;
}

bool Server::doTimedEvents(int)
{
	// File system events.
	m_fsServer.update();
	m_fsWorld.update();
	if (auto guildManager = BabyDI::Get<GuildManager>(); guildManager != nullptr)
		guildManager->update();

	// Do serverlist events.
	m_serverlist.doTimedEvents(getFrameStartTimeHighPrecision());

	// Do player events.
	{
		std::vector<PlayerPtr> deletePlayers;
		for (auto& [id, player] : m_playerList)
		{
			assert(player);
			if (!player->isNPCServer())
			{
				if (!player->doTimedEvents())
					deletePlayers.push_back(player);
			}
		}
		std::ranges::for_each(deletePlayers, [this](PlayerPtr& player)
		{
			deletePlayer(player);
		});
		deletePlayers.clear();
	}

	// Do level events.
	{
		for (auto& [name, level] : m_levelList)
		{
			assert(level);
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
	m_fsWorld.reset();
	m_fsWorld.bind("world");
	if (auto sharefolder = m_settings.get<std::string>("sharefolder"); sharefolder.has_value() && !sharefolder->empty())
	{
		auto folders = string::split(sharefolder.value(), ","sv);
		m_fsWorld.bind(folders);
	}
}

void Server::loadFolderConfig()
{
	auto indent = log::server.indent();
	m_fsWorld.reset();

	m_foldersConfig = CString::loadToken(CString() << "config/foldersconfig.txt", "\n", true);
	for (auto& configLine : m_foldersConfig)
	{
		// No comments.
		int cLoc = -1;
		if ((cLoc = configLine.find("#")) != -1)
			configLine.removeI(cLoc);
		configLine.trimI();
		if (configLine.length() == 0) continue;

		// Parse the line.
		std::string type = configLine.readString(" ").trimI().toString();
		auto world = std::filesystem::path{"world"};
		auto config = std::filesystem::path{configLine.readString("").trimI().toStringView()};

		fs::FileCategory typeEnum = fs::FileCategory::ALL;
		if (string::equalsi(type, "file"sv))
			typeEnum = fs::FileCategory::FILE;
		else if (string::equalsi(type, "level"sv))
			typeEnum = fs::FileCategory::LEVEL;
		else if (string::equalsi(type, "head"sv))
			typeEnum = fs::FileCategory::HEAD;
		else if (string::equalsi(type, "body"sv))
			typeEnum = fs::FileCategory::BODY;
		else if (string::equalsi(type, "sword"sv))
			typeEnum = fs::FileCategory::SWORD;
		else if (string::equalsi(type, "shield"sv))
			typeEnum = fs::FileCategory::SHIELD;
		else if (string::equalsi(type, "sound"sv))
			typeEnum = fs::FileCategory::SOUND;

		m_fsWorld.addFoldersConfigEntry(typeEnum, world / config);
		log::printLine(log::server, "adding {}/ [{}] to {}", config.parent_path().generic_string(), fs::getANSIFileName(config), type);
	}

	m_fsWorld.bind("world");
}

int Server::loadConfigFiles()
{
	log::printLine(log::server, "Loading server configuration.");

	{
		auto indent = log::server.indent();
		loadServerFileSystem();

		// Load Settings
		log::printLine(log::server, "Loading settings...");
		{
			auto indentsettings = log::server.indent();
			prepareSettings();
			loadSettings();
			log::printLine(log::server, "Server generation: {}", ServerGenerationNames[(size_t)Generation]);
		}

		// Load Admin Settings
		log::printLine(log::server, "Loading admin settings.");
		loadAdminSettings();

		// Load allowed versions.
		log::printLine(log::server, "Loading allowed client versions.");
		loadAllowedVersions();

		// Load folders config and file system.
		log::print(log::server, "Folder config: ");
		if (!m_settings.get<bool>("nofoldersconfig").value_or(false))
			log::printLine(log::server, "ENABLED");
		else
			log::printLine(log::server, "disabled");

		log::printLine(log::server, "Loading file system...");
		loadWorldFileSystem();

		// Load server message.
		log::printLine(log::server, "Loading config/servermessage.html.");
		loadServerMessage();

		// Load IP bans.
		log::printLine(log::server, "Loading config/ipbans.txt.");
		loadIPBans();

		// Load translations.
		loadTranslations();

		// Load word filter.
		log::printLine(log::server, "Loading word filter.");
		loadWordFilter();

		// Load server flags.
		log::printLine(log::server, "Loading serverflags.txt.");
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

void Server::prepareSettings()
{
	// Generation.
	m_generationString.onUpdate = [this](const std::optional<std::string>& newValue, const std::optional<std::string>& oldValue)
	{
		auto generation = string::toLower(newValue.value());
		if (auto it = std::ranges::find(ServerGenerationNames, generation); it != std::ranges::end(ServerGenerationNames))
			Generation = static_cast<ServerGeneration>(std::distance(ServerGenerationNames.begin(), it));
		else
			log::printLine(log::server, "** [Error] Invalid generation specified in settings: '{}'.", newValue.value_or("(blank)"));
	};

	// Bush drops.
	m_bushItemTypes.onUpdate = [this](const std::optional<std::vector<std::string>>& newValue, const std::optional<std::vector<std::string>>& oldValue)
	{
		// greenrupee 10, bluerupee 5, bombs 5, heart 5
		static const std::array<int, 25> defaults = {10, 5, 0, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

		m_bushDrops.clear();
		for (const auto& curItem : newValue.value())
		{
			std::string itemType = string::toLower(curItem);
			string::trimMutate(itemType);

			LevelItemType item = LevelItem::getItemId(itemType);
			if (item == LevelItemType::INVALID)
				continue;

			auto spawnRate = m_settings.get<uint32_t>(std::format("spawnrate{}", itemType)).value_or(defaults[static_cast<size_t>(item)]);
			m_bushDrops.emplace_back(item, spawnRate);
		}
	};

	// Death drops.
	m_deathItemTypes.onUpdate = [this](const std::optional<std::vector<std::string>>& newValue, const std::optional<std::vector<std::string>>& oldValue)
	{
		m_deathDrops.clear();
		for (const auto& curItem : newValue.value())
		{
			std::string itemType = string::toLower(curItem);
			string::trimMutate(itemType);

			LevelItemType item = LevelItem::getItemId(itemType);
			if (item != LevelItemType::INVALID)
				m_deathDrops.push_back(item);
		}
	};

	// Gmaps.
	m_gmaps.onUpdate = [this](const std::optional<std::vector<std::string>>& newValue, const std::optional<std::vector<std::string>>& oldValue)
	{
		if (running)
			loadMaps();
	};

	// Bigmaps.
	m_bigmaps.onUpdate = [this](const std::optional<std::vector<std::string>>& newValue, const std::optional<std::vector<std::string>>& oldValue)
	{
		if (running)
			loadMaps();
	};

	// Set the cache bindings before we load so our settings will get cached.
	cached.bind(this);
	m_settings.track(m_generationString, m_classicStyleLogs);
	m_settings.track(m_dontAddServerFlags);
	m_settings.track(m_newTilesets, m_newTilesetLevels);
	m_settings.track(m_unloadInactiveLevelTime);
	m_settings.track(m_staffList, m_bushItemTypes, m_deathItemTypes);
	m_settings.track(m_gmaps, m_bigmaps, m_groupmaps);
}

void Server::loadSettings()
{
	m_settings.load("serveroptions.txt");

	// Send our ServerHQ info in case we got changed the staffonly setting.
	getServerList().sendServerHQ();
}

void Server::loadAdminSettings()
{
	m_adminSettings.load("adminconfig.txt");
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
				for (const auto& version : string::split(versionRange, ","sv))
				{
					m_allowedVersions.push_back(version);
					auto rangeParts = string::splitToVector(version, ":"sv);
					if (rangeParts.size() == 1)
					{
						formattedVersions.push_back(getVersionString(rangeParts[0], PLTYPE_ANYCLIENT));
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

void Server::loadServerFileSystem()
{
	if (m_fsServer.empty())
	{
		m_fsServer.addFoldersConfigEntry(fs::FileCategory::ACCOUNT, "accounts/*.txt");
		m_fsServer.addFoldersConfigEntry(fs::FileCategory::CONFIG, "config/*");
		m_fsServer.addFoldersConfigEntry(fs::FileCategory::CONFIG, "serverflags.txt");
		m_fsServer.addFoldersConfigEntry(fs::FileCategory::NPC, "npcs/npc*.txt");
		m_fsServer.addFoldersConfigEntry(fs::FileCategory::SCRIPTCLASS, "scripts/*.txt");
		m_fsServer.addFoldersConfigEntry(fs::FileCategory::TRANSLATION, "translations/*");
		m_fsServer.addFoldersConfigEntry(fs::FileCategory::WEAPON, "weapons/weapon*.txt");
		m_fsServer.bind("accounts"s, "config"s, "npcs"s, "scripts"s, "translations"s, "weapons"s);
		m_fsServer.bindSingleFile("serverflags.txt");
	}
}

void Server::loadWorldFileSystem()
{
	if (m_settings.get<bool>("nofoldersconfig").value_or(false))
		loadAllFolders();
	else
		loadFolderConfig();

	for (auto fileData : m_fsWorld.info(fs::FileCategory::ALL) | toSharedPtr)
	{
		if (fileData == nullptr) continue;
		if (fileData->file.extension() == ".gupd")
			getPackageManager().findOrAddResource(fileData->file.string())->reload(this);
	}
}

void Server::loadServerFlags()
{
	if (auto file = m_fsServer.openi(fs::FileCategory::CONFIG, "serverflags.txt"); file && file->opened())
	{
		std::unordered_map<std::string, std::string, string::string_hash, std::equal_to<>> flagMap;
		for (const auto& line : file->readAllLines())
		{
			std::string_view flagPair{string::trim(line)};
			if (flagPair.empty())
				continue;

			if (!flagPair.contains('='))
				flagMap.try_emplace(std::string{flagPair}, std::string{});
			else
			{
				auto sep = flagPair.find('=');
				std::string_view flagKey = string::trimRight(flagPair.substr(0, sep));
				std::string_view flagValue = string::trimLeft(flagPair.substr(sep + 1));
				flagMap.try_emplace(std::string{flagKey}, std::string{flagValue});
			}
		}

		std::vector<std::string> removedFlags;
		bool hasNS = hasNPCServer();

		// Iterate through all the server flags finding deleted flags and sending changes.
		for (auto& [flag, value] : Scripting.variables.store | variables::no_temporary)
		{
			auto search = flagMap.find(flag);

			// The server variable is not in the map, so it was deleted.
			if (search == flagMap.end())
				removedFlags.emplace_back(flag);
			else
			// The server variable was changed.
			{
				if (search->second.empty() && !value->has<bool>())
				{
					value->unassign<std::string>();
					value->assign<bool>(true);
					if (!hasNS || flag.starts_with("serverr."))
						sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGSET << search->first);
				}
				else if (!value->has<std::string>() || search->second != value->get<std::string>().value())
				{
					value->unassign<bool>();
					value->assign<std::string>(search->second);
					if (!hasNS || flag.starts_with("serverr."))
						sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGSET << search->first << "=" << *value->get_unsafe<std::string>());
				}
				flagMap.erase(search);
			}
		}

		// Delete all the removed flags.
		for (const auto& flag : removedFlags)
		{
			auto& store = Scripting.variables.store;
			if (auto search = store.find(flag); search != store.end() && search->second != nullptr)
			{
				if (!hasNS || flag.starts_with("serverr."))
					sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGDEL << flag);
				store.erase(search);
			}
		}

		// Add the new flags.
		for (auto& [flag, value] : flagMap)
		{
			if (value.empty())
			{
				Scripting.variables.add(flag, true);
				if (!hasNS || flag.starts_with("serverr."))
					sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGSET << flag);
			}
			else
			{
				Scripting.variables.add(flag, value);
				if (!hasNS || flag.starts_with("serverr."))
					sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGSET << flag << "=" << value);
			}
		}
	}
}

void Server::loadGuilds()
{
	if (auto guild = BabyDI::Get<GuildManager>(); guild != nullptr)
		guild->loadGuilds("guilds");
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

void Server::loadTranslations() const
{
	log::print(log::server, "Loading translations: ");

	// If our translation manager already exists, save the translations first.
	if (auto translationManager = BabyDI::Get<ITranslationManager>(); translationManager != nullptr)
		translationManager->saveTranslations();

	BabyDI_RELEASE(ITranslationManager);

	// Create our translation manager.
	ITranslationManager* manager = nullptr;
	if (Generation == ServerGeneration::MODERN)
	{
		log::printLine(log::server, "modern style.");
		manager = BabyDI_PROVIDE(ITranslationManager, new TranslationManagerModern());
	}
	else
	{
		log::printLine(log::server, "classic style.");
		manager = BabyDI_PROVIDE(ITranslationManager, new TranslationManagerClassic());
	}

	// Load the translation files.
	if (manager != nullptr)
		manager->loadTranslations(std::filesystem::current_path() / "translations");
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
	for (const auto& gmapName : m_gmaps.getValue())
	{
		// Load the gmap.
		try
		{
			auto mapName = string::trim(gmapName);
			if (bool hasExtension = mapName.ends_with(".gmap"); hasExtension)
			{
				auto gmap = std::make_unique<Map>(is_gmap, mapName);
				m_mapList.push_back(std::move(gmap));
			}
			else
			{
				auto gmap = std::make_unique<Map>(is_gmap, std::format("{}.gmap", mapName));
				m_mapList.push_back(std::move(gmap));
			}
			if (print) log::printLine(log::server, "[gmap] {}", mapName);
		}
		catch (...)
		{
			auto inerr = log::server.indent_absolute(0);
			if (print) log::printLine(log::server, "** [Error] Could not load {} (gmap).", string::trim(gmapName));
		}
	}

	// Load bigmaps.
	for (const auto& bigmapName : m_bigmaps.getValue())
	{
		// Load the bigmap.
		try
		{
			auto mapName = string::trim(bigmapName);
			auto bigmap = std::make_unique<Map>(is_bigmap, mapName);
			if (print) log::printLine(log::server, "[bigmap] {}", mapName);
			m_mapList.push_back(std::move(bigmap));
		}
		catch (...)
		{
			auto inerr = log::server.indent_absolute(0);
			if (print) log::printLine(log::server, "** [Error] Could not load {} (bigmap).", string::trim(bigmapName));
		}
	}
}

void Server::loadNPCServer()
{
	if (m_settings.get<bool>("serverside").value_or(true))
	{
		log::printLine(log::server, "Loading NPC server.");
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
	log::printLine(log::server, "Loading server objects.");

	auto indent = log::server.indent();
	{
		// Load weapons.
		log::printLine(log::server, "Loading weapons...");
		loadWeapons(true);

		// Load map levels - doing this after db npcs are loaded incase
		// some level scripts may require access to the databases.
		log::printLine(log::server, "Pre-loading map levels.");
		loadMapLevels();
	}

	return 0;
}

void Server::loadWeapons(bool print)
{
	auto indent = log::server.indent();
	{
		auto sectionProfile = log::Profile(log::server, "", "(Completed in {1:0.6} ms)");

		for (auto weaponFile : m_fsServer.info(fs::FileCategory::WEAPON) | toSharedPtr)
		{
			if (weaponFile == nullptr) continue;

			auto profile = log::Profile(log::server, "", " ({1:0.6} ms)");

			auto fileName = fs::getANSIFileName(weaponFile->file);
			auto weapon = Weapon::loadWeapon(fileName);
			if (weapon == nullptr) continue;

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
	if (auto file = m_fsServer.openiForWriting(fs::FileCategory::CONFIG, "serverflags.txt"); file && file->opened())
	{
		file->clear();
		for (auto& [flag, value] : Scripting.variables.store)
		{
			if (auto serialized = value->serializeModern(flag); serialized.has_value())
				file->writeLine(serialized.value());
		}
	}
}

void Server::saveWeapons()
{
	for (auto& [weaponName, weapon] : m_weaponList)
	{
		if (weapon->isDefault())
			continue;

		std::filesystem::path weaponFile{std::format("weapon{}.txt", weaponName)};
		clock::time_point mod{clock::time_point::min()};

		auto fileData = m_fsServer.info(fs::FileCategory::WEAPON, weaponFile);
		if (fileData != nullptr)
			mod = fileData->getModTime();

		if (weapon->modTime > mod)
		{
			// The weapon in memory is newer than the weapon on disk.  Save it.
			weapon->saveWeapon();
			if (fileData != nullptr)
				fileData->setModTime(weapon->modTime);
		}
	}
}

/////////////////////////////////////////////////////

std::shared_ptr<Level> Server::getStubbedLevel(std::string_view levelName, std::string_view groupName)
{
	if (levelName.empty())
		return nullptr;

	// Get the computed level name, which includes the group name (if applicable).
	std::string lowerCaseLevel;
	if (!groupName.empty())
	{
		lowerCaseLevel = groupName;
		lowerCaseLevel += ".";
	}
	lowerCaseLevel += string::toLower(levelName);

	// Check to see if the level exists and has a value.
	auto existing = m_levelList.find(lowerCaseLevel);
	if (existing != m_levelList.end() && existing->second != nullptr)
		return existing->second;

	// Create a new stub level.
	auto level = Level::createLevel(levelName);
	if (!groupName.empty())
		level->groupMapName = groupName;

	// Add it to the list, or update the existing entry if it exists.
	if (existing == m_levelList.end())
		m_levelList.insert(std::make_pair(lowerCaseLevel, level));
	else
		existing->second = level;

	return level;
}

std::shared_ptr<Level> Server::getLoadedLevelNoHint(std::string_view levelName)
{
	if (levelName.empty())
		return nullptr;

	// Get the stub for the level.
	LevelPtr level = getStubbedLevel(levelName);
	if (level == nullptr)
		return nullptr;

	// Level was already loaded.
	if (level != nullptr && level->loaded)
		return level;

	// Load the level.
	if (LevelLoader::loadLevelInto(levelName, level))
	{
		m_levelList.insert(std::make_pair(string::toLower(levelName), level));
		return level;
	}

	return nullptr;
}

std::shared_ptr<Level> Server::getLoadedLevel(std::string_view levelName, std::shared_ptr<Player> player)
{
	if (levelName.empty())
		return nullptr;

	LevelPtr level = nullptr;

	// Check if this level matches the list of group maps.
	// If it does, and the player's group matches, try to load the group version of the level (or create it).
	if (!player->account.groupName.empty())
	{
		for (const auto& groupmap : m_groupmaps.getValue())
		{
			auto mask = string::trim(groupmap);
			if (string::match<true>(levelName, mask))
			{
				// Check if this level already exists.
				std::string groupMapName = string::toLower(std::format("{}.{}", player->account.groupName, levelName));
				if (level = findGmapForLevel(groupMapName, player); level != nullptr && level->loaded)
					return level;

				// Level doesn't exist or isn't loaded, create it.
				if (level == nullptr)
					level = std::make_shared<Level>();

				// Record that the level is going to be a group map, with the name of the group it belongs to.
				// We do this now so level NPCs get added with the correct name.
				level->isGroupMap = true;
				level->groupMapName = player->account.groupName;

				// Load the level.
				if (LevelLoader::loadLevelInto(levelName, level))
				{
					m_levelList.insert(std::make_pair(groupMapName, level));
					return level;
				}
			}
		}
	}

	// See if this level belongs to a gmap.
	if (level = findGmapForLevel(levelName, player); level != nullptr)
	{
		if (!level->loaded && LevelLoader::loadLevelInto(level->levelName, level))
			m_levelList.insert(std::make_pair(string::toLower(level->levelName), level));
		return level;
	}

	return getLoadedLevelNoHint(levelName);
}

std::shared_ptr<Level> Server::getLoadedLevel(std::string_view levelName, std::shared_ptr<Level> hintLevel)
{
	if (levelName.empty())
		return nullptr;

	// See if this level belongs to the hinted level.
	if (hintLevel != nullptr && hintLevel->isGmap() && hintLevel->getSubLevelByName(levelName) != nullptr)
	{
		if (!hintLevel->loaded && LevelLoader::loadLevelInto(hintLevel->levelName, hintLevel))
			m_levelList.insert(std::make_pair(string::toLower(hintLevel->levelName), hintLevel));
		return hintLevel;
	}

	return getLoadedLevelNoHint(levelName);
}

std::shared_ptr<StaticLevelData> Server::getCachedLevelData(std::string_view levelName)
{
	if (levelName.empty())
		return nullptr;

	std::string lowerCaseLevel = string::toLower(levelName);
	if (auto it = m_cachedLevelDataList.find(lowerCaseLevel); it != m_cachedLevelDataList.end())
		return it->second;

	auto levelData = LevelLoader::loadStaticData(levelName);
	m_cachedLevelDataList.insert(std::make_pair(lowerCaseLevel, levelData));
	return levelData;
}

std::shared_ptr<Map> Server::findMap(std::string_view mapName) const noexcept
{
	auto foundMap = std::ranges::find_if(m_mapList, [&mapName](const auto& map)
	{
		return map->getMapName() == mapName;
	});
	if (foundMap != std::ranges::end(m_mapList))
		return *foundMap;
	return nullptr;
}

std::shared_ptr<Map> Server::findMapForLevel(std::string_view levelName) const noexcept
{
	auto foundMap = std::ranges::find_if(m_mapList, [&levelName](const auto& map)
	{
		return map->hasLevel(levelName);
	});
	if (foundMap != std::ranges::end(m_mapList))
		return *foundMap;
	return nullptr;
}

std::shared_ptr<Map> Server::findMapForLevel(MapType mapType, std::string_view levelName) const noexcept
{
	auto foundMap = std::ranges::find_if(m_mapList, [&mapType, &levelName](const auto& map)
	{
		return map->mapType == mapType && map->hasLevel(levelName);
	});
	if (foundMap != std::ranges::end(m_mapList))
		return *foundMap;
	return nullptr;
}

std::shared_ptr<Level> Server::findGmapForLevel(std::string_view levelName, std::shared_ptr<Player> player) noexcept
{
	LevelPtr returnLevel = nullptr;

	// If this is the gmap itself, find it directly.
	if (levelName.ends_with(".gmap"sv))
	{
		if (auto it = m_levelList.find(levelName); it != m_levelList.end())
			return it->second;
	}

	// Check if this level belongs to a gmap.
	auto [iter, end] = m_gmapLevels.equal_range(levelName);
	while (iter != end)
	{
		if (auto level = iter->second.lock(); level != nullptr)
		{
			if (player != nullptr)
			{
				// If this is a group map, and our group matches, use this one.
				if (level->isGroupMap && player->account.groupName == level->groupMapName)
					return level;

				// If this is a singleplayer map, and our account matches, use this one.
				if (level->isSinglePlayer && player->account.name == level->groupMapName)
					return level;
			}

			// Otherwise, record this as the level we will return if we don't find anything.
			if (!level->isGroupMap && !level->isSinglePlayer)
				returnLevel = level;
		}
		++iter;
	}

	return returnLevel;
}

tileset::TilesetType Server::getTilesetTypeForLevel(std::shared_ptr<Level> level) const noexcept
{
	if (level == nullptr)
		return tileset::TilesetType::CLASSIC;

	// Levels with terrain always use the terrain tileset.
	if (level->hasTerrain())
		return tileset::TilesetType::TERRAIN;

	// Check for tileset type 1 (new tilesets).
	for (const auto& newlevel : m_newTilesetLevels.getValue())
	{
		if (string::match(level->levelName, newlevel) || level->levelName.starts_with(newlevel))
			return tileset::TilesetType::NEWFORMAT;
	}

	// If all levels are the new tileset, return that.
	if (m_newTilesets.getValue())
		return tileset::TilesetType::NEWFORMAT;

	// Otherwise, return classic.
	return tileset::TilesetType::CLASSIC;
}

tileset::TilesetType Server::getTilesetTypeForLevel(std::shared_ptr<const Level> level) const noexcept
{
	return getTilesetTypeForLevel(std::const_pointer_cast<Level>(level));
}

tileset::TileType Server::getTileTypeForTile(tileset::TilesetType tileset, uint16_t tile) const noexcept
{
	// Terrain tileset uses non-blocking for all tiles.
	if (tileset == tileset::TilesetType::TERRAIN)
		return tileset::TileType::NONBLOCKING;

	// If tile is out of range, return blocking.
	if (tile >= 4096)
		return tileset::TileType::BLOCKING;

	// Classic (type 0).
	if (tileset == tileset::TilesetType::CLASSIC)
		return ENUM<tileset::TileType>(tileset::Type0.at(tile));

	// New format (type 1).
	if (tileset == tileset::TilesetType::NEWFORMAT)
		return ENUM<tileset::TileType>(tileset::Type1.at(tile));

	// Default to blocking.
	return tileset::TileType::BLOCKING;
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

std::shared_ptr<NPC> Server::addNPC(std::string_view image, std::string_view script, float x, float y, std::weak_ptr<Level> level, NPCStorageType storageType, bool sendToPlayers, std::string_view type)
{
	LevelPtr levelPtr = level.lock();
	if (storageType == NPCStorageType::LEVEL && levelPtr == nullptr)
		return nullptr;

	// Pick the ID range for the NPC ID.
	auto startId = NPCID_GEN_LOCAL;
	if (storageType == NPCStorageType::DATABASE)
		startId = NPCID_GEN_DATABASE_LOCALN;

	// Create the NPC.
	NPCID newId = m_npcIdGenerator.getAvailableId(startId);
	auto newNPC = std::make_shared<NPC>(newId, storageType);

	// Set the script type.
	if (!type.empty())
		newNPC->scriptType = type;

	// Set NPC props.
	auto localPixelPosition = toLocalPixelPosition(x, y);
	newNPC->character.localPixelX = localPixelPosition.x();
	newNPC->character.localPixelY = localPixelPosition.y();
	newNPC->image = image;

	// Set the level details.
	if (levelPtr != nullptr)
	{
		newNPC->setLevel(levelPtr);

		// If the level is a gmap, set the modTime on the map position props.
		if (auto map = levelPtr->getMap(); map && map->isGmap())
		{
			auto mapPosition = toMapPosition(TilePosition{x, y});
			newNPC->character.mapX = mapPosition.x();
			newNPC->character.mapY = mapPosition.y();
			newNPC->modTime[PROPID(NPCProp::GMAPLEVELX)] = m_frameStartTime;
			newNPC->modTime[PROPID(NPCProp::GMAPLEVELY)] = m_frameStartTime;
		}
	}

	// Set the script and record the initial state.
	newNPC->setScript(script);
	newNPC->recordInitialState();

	// Finish adding the NPC.
	return addNPC(newNPC, sendToPlayers);
}

std::shared_ptr<NPC> Server::addNPC(NPCPtr npc, bool sendToPlayers)
{
	// Add the NPC to the list.
	m_npcList.insert(std::make_pair(npc->id, npc));

	// Set the default warp type.
	npc->warpRestrictions = hasNPCServer() ? NPCWarpRestrictions::NOTALLOWED : NPCWarpRestrictions::ALLOWED;

	// Set the default script type.
	if (npc->scriptType.empty())
	{
		if (npc->storageType == NPCStorageType::LEVEL)
			npc->scriptType = NPCTYPE_LOCAL;
		else npc->scriptType = NPCTYPE_OBJECT;
	}

	// Add the NPC to the level.
	auto level = npc->getLevel();
	if (level != nullptr)
	{
		level->addNPC(npc);
	}
	else if (!npc->level.empty())
	{
		if (level = getStubbedLevel(npc->level, npc->groupName); level != nullptr)
			level->addNPC(npc);
	}

	// Synchronize the group name of the NPC.
	if (level != nullptr && level->isGroupMap && npc->groupName != level->groupMapName)
		npc->groupName = level->groupMapName;

	// Set the NPC's name.
	if (npc->name.empty())
	{
		std::string npcNamePrefix = std::format("{}_{}{}{}_{}_",
			string::toLower(npc->scriptType),
			level != nullptr && level->isGroupMap ? level->groupMapName : "",
			level != nullptr && level->isGroupMap ? "." : "",
			string::removeExtension(npc->level), m_serverTime
		);
		auto count = std::ranges::count_if(m_npcList, [&npcNamePrefix](const auto& pair)
		{
			return pair.second->name.starts_with(npcNamePrefix);
		});

		npc->name = std::format("{}{}", npcNamePrefix, (count + 1));
	}

	// Created event.
	if (hasNPCServer())
	{
		npc->scripting.events.addEvent(ScriptEventType::CREATED, source::FromServer());
	}

#ifdef DEBUG
	if (running)
	{
		log::printLine(log::server, "Adding NPC [{}] '{}' at ({}, {})[{},{}] in level '{}'.",
			npc->id, npc->name, npc->character.localPixelX, npc->character.localPixelY, npc->character.mapX, npc->character.mapY, npc->level);
	}
#endif

	// Record the current prop modification time.
	npc->recordCurrentPropModTime();

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
	if (npc == nullptr)
		return false;

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
		// Get the sub-level the NPC is on.
		auto [subLevel, levelData] = level->getSubLevelAndStaticDataAtPosition(MapPosition{npc->character.mapX, npc->character.mapY});

		// Remove the NPC from the level
		if (eraseFromLevel)
			level->removeNPC(npc);

		// Tell the clients to delete the NPC.
		std::string levelName = npc->getLevelName();

		auto lastLevelChange = npc->modTime[PROPID(NPCProp::CURLEVEL)];
		for (auto& [pid, p] : m_playerList)
		{
			std::optional<clock::time_point> lastEntered = std::nullopt;
			auto playerClient = std::dynamic_pointer_cast<PlayerClient>(p);
			if (playerClient != nullptr)
				lastEntered = playerClient->getLevelLastEnteredTime(levelData.get());

			if (playerClient != nullptr && (!lastEntered.has_value() || lastEntered.value() >= lastLevelChange || playerClient->getLevel() == level))
			{
				if (playerClient->getLevelName() != levelName)
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
		if (hasNPCServer() && (player->isClient() || player->isRC()))
			m_npcServer->playerLogout(player);

		// Leave the level.
		// If we have an NPC-Server, we want to keep the level reference around until the NPC-Server has processed the player logout.
		if (auto client = std::dynamic_pointer_cast<PlayerClient>(player); client != nullptr)
			client->leaveLevel(hasNPCServer());

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

	const auto id = old_player->getId();

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

bool Server::warpPlayerToSafePlace(PlayerID playerId) const
{
	auto player = getPlayer<PlayerClient>(playerId);
	if (player == nullptr) return false;

	// Try unstick me level.
	return player->warp(cached.unstickMeLevel.getValue(), {static_cast<int16_t>(cached.unstickMeTile[0].getValue() * 16.0f), static_cast<int16_t>(cached.unstickMeTile[1].getValue() * 16.0f)});

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
	for (const auto& ipBan : m_ipBans)
	{
		if (ip.match(ipBan))
			return true;
	}

	return false;
}

bool Server::isStaff(const CString& accountName)
{
	const auto& staffList = m_staffList.get();
	if (!staffList.has_value())
		return false;

	for (const auto& account : staffList.value())
	{
		if (string::equalsi(accountName.toStringView(), account))
			return true;
	}

	return false;
}

std::string Server::getLogDateTimeString() const
{
	if (m_classicStyleLogs.getValue())
	{
		char buffer[33]{};

		// Non-standard, but make it at least a LITTLE easier to read these dumb logs.
		buffer[0] = '\n';

		std::time_t curTime = std::time(nullptr);
		std::strncpy(buffer + 1, std::ctime(&curTime), 31);
		buffer[32] = '\0';

		return std::string{buffer};
	}
	else
	{
#if __cpp_lib_chrono < 201907L
		// Clang doesn't support timezones, so just use system_clock time (UTC) floored to seconds.
		auto localtime = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
#else
		// Get the current time, floored to seconds.
		auto localtime = std::chrono::floor<std::chrono::seconds>(std::chrono::current_zone()->to_local(std::chrono::system_clock::now()));
#endif

		std::string result{ std::format(log::TimestampLong, localtime) };
		result += " ";
		return result;
	}
}

void Server::logToFile(std::filesystem::path fileName, std::string_view message, bool writeTimestamp) const
{
	std::filesystem::path logPath{"logs"};

	fs::FileSimpleIO file{logPath / fileName};
	if (!file.opened())
		return;

	if (writeTimestamp)
		file.write(getLogDateTimeString());

	file.writeLine(message);
}

void Server::logToFileSafely(std::filesystem::path fileName, std::string_view message, bool writeTimestamp) const
{
	std::filesystem::path logPath{"logs"};

	fs::FileIO file{logPath / fileName};
	if (!file.opened())
		return;

	if (writeTimestamp)
		file.write(getLogDateTimeString());

	file.writeLine(message);
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
	if (m_dontAddServerFlags.getValue())
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
	return setFlag(flagName, std::string{flagValue}, sendToPlayers);
}

bool Server::setFlag(std::string_view flagName, std::optional<std::string> flagValue, bool pSendToPlayers)
{
	// Function to crop flags.
	auto cropFlag = [this, &flagName](std::string& value)
	{
		if (cached.enableFlagCropping.getValue())
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
		if (m_dontAddServerFlags.getValue())
			return false;

		if (!flagValue.has_value())
			Scripting.variables.add(flagName, GameValue{true});
		else Scripting.variables.add(flagName, GameValue{cropFlag(flagValue.value())});
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

void Server::sendPacketToAll(const CString& packet, const std::set<PlayerID>& exclude, PlayerPredicate sendIf) const
{
	for (auto& [id, player] : m_playerList)
	{
		if (exclude.contains(id))
			continue;
		if (player->isNPCServer())
			continue;
		if (sendIf && !sendIf(player.get()))
			continue;

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
	for (auto& [id, player] : m_playerList)
	{
		if ((player->getType() & who) && (!pPlayer || id != pPlayer->getId()))
			player->sendPacket(pPacket);
	}
}

void Server::sendPacketToOneLevelPart(const CString& packet, const PixelPosition& position, LevelPtr level, const std::set<PlayerID>& exclude, PlayerPredicate sendIf) const
{
	auto mapPosition = toMapPosition(position);
	sendPacketToOneLevelPart(packet, level, mapPosition, exclude, sendIf);
}

void Server::sendPacketToOneLevelPart(const CString& packet, LevelPtr level, const MapPosition& mapPosition, const std::set<PlayerID>& exclude, PlayerPredicate sendIf) const
{
	for (const auto& id : level->findPlayersInLevelPart(mapPosition))
	{
		if (exclude.contains(id)) continue;
		if (auto player = this->getPlayer(id); player && player->isClient() && (!sendIf || sendIf(player.get())))
			player->sendPacket(packet);
	}
}

void Server::sendPacketToNearby(const CString& packet, const PixelPosition& position, LevelPtr level, const std::set<PlayerID>& exclude, PlayerPredicate sendIf) const
{
	if (!running || level == nullptr) return;

	auto players = level->findInRangePlayersForCommunication(position);
	for (const auto& playerId : players)
	{
		if (exclude.contains(playerId))
			continue;
		if (auto player = getPlayer<PlayerClient>(playerId); player != nullptr && (!sendIf || sendIf(player.get())))
		{
			// Are we on the same level?
			// Levels on a gmap are the same level and thus this would be false.
			bool sameLevel = level->levelName == player->getLevelName();

			// TODO: Enable nearby data for bigmaps again.
			// The current problem is that the NPC-Server will send modified NPC props to players when they don't know about the NPC yet, which breaks the NPCs.
			// We need to add the ability to send the full NPC details of adjacent levels when entering a bigmap level before we can re-enable this.
			if (!sameLevel)
				continue;

			// TODO: Figure out when PLO_SETACTIVELEVEL was introduced.
			//if (!sameLevel && player->getVersion() < CLVER_2_17)
			//	continue;
			//
			//if (!sameLevel) player->sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << level->levelName);
			player->sendPacket(packet);
			//if (!sameLevel) player->sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << player->getLevelName());
		}
	}
}

void Server::sendPacketToLevelAndPastVisitorsAfter(StaticLevelData* level, clock::time_point modTime, const CString& packet) const
{
	if (!running) return;
	for (const auto& [id, player] : players_of_type<PlayerClient>(m_playerList))
	{
		auto playerLevel = player->getLevel();
		auto levelData = playerLevel->getStaticLevelDataAtPosition(player->getMapPosition());
		auto lastEntered = player->getLevelLastEnteredTime(level);
		if ((lastEntered.has_value() && lastEntered.value() > modTime) || (levelData != nullptr && levelData.get() == level))
			player->sendPacket(packet);
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

	// Delete from the file system.
	CString name = pWeaponName;
	name.replaceAllI("\\", "_");
	name.replaceAllI("/", "_");
	name.replaceAllI("*", "@");
	name.replaceAllI(":", ";");
	name.replaceAllI("?", "!");
	std::filesystem::path weaponFile{"weapons"};
	std::filesystem::remove(weaponFile / std::format("weapon{}.txt", name));

	// Delete from Memory
	m_weaponList.erase(std::string{pWeaponName});

	// Delete from Players
	sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_NPCWEAPONDEL << pWeaponName);
	return true;
}

void Server::updateWeaponForPlayers(Weapon* weapon)
{
	if (weapon == nullptr)
		return;

	// Update Weapons
	for (auto& [id, player] : m_playerList)
	{
		if (!player->isClient())
			continue;

		if (player->account.hasWeapon(weapon->name))
		{
			player->sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << weapon->name);
			weapon->registerWeaponWithPlayer(player);
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

//----------------------------

void Server::sendShootToOneLevel(LevelShoot* shoot, std::shared_ptr<Level> level) const
{
	if (shoot == nullptr || level == nullptr)
		return;

	float pi = std::numbers::pi_v<float>;
	float halfpi = pi / 2;

	ShootPacketWrapper newPacket{};
	newPacket.source = (shoot->from.second == ScriptObjectType::NPC ? shoot->from.first : 0);
	newPacket.position = toPixelPosition(shoot->position);
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

	sendPacketToNearby(oldPacketBuf, newPacket.position, level, {0}, [](const auto pl)
	{
		return pl->getVersion() < CLVER_5_07;
	});
	sendPacketToNearby(newPacketBuf, newPacket.position, level, {0}, [](const auto pl)
	{
		return pl->getVersion() >= CLVER_5_07;
	});
}

//----------------------------

void Server::scheduleTask(precise_clock::duration delay, std::function<void()> task)
{
	m_scheduledTasks.emplace_back(delay, std::move(task));
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
