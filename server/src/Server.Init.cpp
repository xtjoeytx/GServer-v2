#include <algorithm>
#include <format>
#include <memory>
#include <ranges>
#include <string>
#include <variant>
#include <vector>

#include <CString.h>

#include <Server.h>
#include <filesystem/File.h>
#include <npcserver/NPCServer.h>
#include <object/Player.h>
#include <object/Weapon.h>
#include <player/PlayerClient.h>
#include <scripting/IScriptEngine.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/Log.h>
#include <utilities/manager/GuildManager.h>
#include <utilities/manager/ITranslationManager.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

void Server::initVariables()
{
	// Scripting variables.
	// clang-format off
	auto playerFilter = std::views::filter([](auto& kvp)
	{
		return (kvp.second->isClient() || kvp.second->isRC()) && kvp.second->getId() != 0;
	});

	Scripting.variables.add("gravity"sv, GameValue{2.0});
	Scripting.variables.add("waterheight"sv, GameValue{0.0});
	Scripting.variables.add<double>("timevar"sv, bindGETSIMPLE(static_cast<double>(getNWTime()), this), {});
	Scripting.variables.add<double>("timevar2"sv, bindGETSIMPLE(static_cast<double>(getFrameStartTimeHighPrecision().time_since_epoch().count()), this), {});
	Scripting.variables.add<double>("allplayerscount"sv,
		[this, playerFilter](std::optional<int64_t> index) -> GameValueVariantForGetter
		{
			if (!hasNPCServer()) return 0.0;
			const auto size = std::ranges::distance(getNPCServer()->getPlayerList() | playerFilter);
			return static_cast<double>(size);
		},
		{});
	Scripting.variables.add<std::vector<ScriptObject>>("allplayers"sv,
		[this, playerFilter](std::optional<int64_t> index) -> GameValueVariantForGetter
		{
			if (!hasNPCServer()) return std::vector<ScriptObject>{};
			auto playerObjects = getNPCServer()->getPlayerList()
				| playerFilter
				| std::views::transform([](auto& kvp) { return ScriptObject{ std::make_pair(static_cast<size_t>(kvp.first), ScriptObjectType::PLAYER) }; });

			// I don't know why I have to do this but it was crashing when I tried to use my normal take/drop ranges logic.
			std::vector<ScriptObject> players{std::ranges::begin(playerObjects), std::ranges::end(playerObjects)};
			if (index.has_value() && index.value() >= 0 && index.value() < static_cast<int64_t>(players.size()))
			{
				players[0] = players[index.value()];
				players.resize(1);
			}
			return players;
		},
		{});

	constexpr uint32_t toHours = 60;
	constexpr uint32_t toDays = 1440;
	constexpr uint32_t toWeeks = 10080;
	constexpr uint32_t toMonths = 40320;
	constexpr uint32_t toYears = 403200;
	Scripting.variables.add<double>("nwtime"sv, bindGETSIMPLE(static_cast<double>(getNWTime() % 1440), this), {});                 // minutes of the day
	Scripting.variables.add<double>("nwmin"sv, bindGETSIMPLE(static_cast<double>(getNWTime() % 60), this), {});                    // 60 min in an hour
	Scripting.variables.add<double>("nwhour"sv, bindGETSIMPLE(static_cast<double>((getNWTime() / toHours) % 24), this), {});       // 24 hours in a day
	Scripting.variables.add<double>("nwday"sv, bindGETSIMPLE(static_cast<double>((getNWTime() / toDays) % 28 + 1), this), {});     // 28 days in a month (1..28)
	Scripting.variables.add<double>("nwweekday"sv, bindGETSIMPLE(static_cast<double>((getNWTime() / toDays) % 7 + 1), this), {});  // Sunday-Saturday (1..7)
	Scripting.variables.add<double>("nwweek"sv, bindGETSIMPLE(static_cast<double>((getNWTime() / toWeeks) % 40 + 1), this), {});   // 40 weeks in a year (1..40)
	Scripting.variables.add<double>("nwmonth"sv, bindGETSIMPLE(static_cast<double>((getNWTime() / toMonths) % 10 + 1), this), {}); // 10 months in a year (1..10)
	Scripting.variables.add<double>("nwyear"sv, bindGETSIMPLE(static_cast<double>((getNWTime() / toYears) + 1000), this), {});     // Years start at 1000

	GameVariable groundHeightsVar{.name = "groundheights"};
	groundHeightsVar.registerGetter<double>([this](const std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		if (!index.has_value() || index.value() < 0 || index.value() >= static_cast<int64_t>(groundHeights.size()))
			return 0.0;
		return groundHeights.at(index.value());
	});
	groundHeightsVar.registerGetter<std::vector<double>>(bindGETSIMPLE((std::vector<double>{groundHeights.begin(), groundHeights.end()}), this));
	groundHeightsVar.registerSetter<double>([this](const GameValueVariantForSetter& value, const std::optional<int64_t> index)
	{
		if (!index.has_value() || index.value() < 0 || index.value() >= static_cast<int64_t>(groundHeights.size()))
			return;
		if (const auto wrap = std::get_if<std::reference_wrapper<double>>(&value); wrap != nullptr)
			groundHeights.at(index.value()) = wrap->get();
	});
	groundHeightsVar.registerSetter<std::vector<double>>([this](const GameValueVariantForSetter& value, std::optional<int64_t> index)
	{
		groundHeights.fill(0.0);
		if (const auto wrap = std::get_if<std::reference_wrapper<std::vector<double>>>(&value); wrap != nullptr)
		{
			for (size_t i = 0; i < std::min(groundHeights.size(), wrap->get().size()); ++i)
				groundHeights[i] = wrap->get()[i];
		}
	});
	Scripting.variables.add(std::move(groundHeightsVar));
	// clang-format on
}

///////////////////////////////////////////////////////////////////////////////

void Server::initTimedEvents()
{
	m_timedEvents1s.callbackIterations = [this](const int iterations)
	{
		doTimedEvents(iterations);
	};
	m_timedSave1m.callbackIterations = [this](int)
	{
		saveServerFlags();
		if (const auto guild = BabyDI::Get<GuildManager>(); guild != nullptr)
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

			// Group maps and single player maps will always unload after 10 minutes if the inactive time is not set.
			if (level->isPrivateMap() && inactiveDuration > std::chrono::seconds(m_unloadInactiveLevelTime.get().value_or(600)))
				skip = false;

			// Do the skip now.
			if (skip)
				continue;

			DEBUGPRINT("Unloading level '{}' due to inactivity.", levelName);

			// If we have an NPC-server, unload (or delete) our serverside NPCs.
			if (hasNPCServer())
			{
				if (level->isPrivateMap())
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

			// Archive the board changes so we can restore them if the level is reloaded.
			for (const auto& subLevel : level->getSubLevels())
			{
				if (auto staticData = subLevel->staticData.lock(); staticData != nullptr)
					m_archivedBoardChanges[staticData->levelName] = subLevel->boardChanges;
			}

			level = nullptr;
		}

		std::erase_if(m_levelList, [](const auto& entry)
		{
			return entry.second == nullptr;
		});
	};
}

///////////////////////////////////////////////////////////////////////////////

void Server::initFilesystemCallbacks()
{
	m_fsServer.categoryEventCallback[ENUM(fs::FileCategory::CONFIG)] = [this](const fs::FileEventCollection events, const fs::FileData& file)
	{
		if (events.test(fs::FileEvent::Modified))
		{
			const auto fileName = fs::getANSIFileName(file.file);
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
			else if (fileName.starts_with("scriptengine-") && fileName.ends_with(".txt"))
			{
				if (const auto npcServer = getNPCServer(); npcServer != nullptr)
				{
					const auto engineName = fileName.substr(13, fileName.size() - 17); // Remove scriptengine- and .txt
					if (const auto engine = npcServer->scripting.getScriptEngine(engineName); engine != nullptr)
						engine->loadConfiguration(file.file);
				}
			}
		}
	};

	m_fsServer.categoryEventCallback[ENUM(fs::FileCategory::ACCOUNT)] = [this](const fs::FileEventCollection events, const fs::FileData& file)
	{
		if (events.test(fs::FileEvent::Modified))
		{
			const auto playerName = fs::getANSIFileName(file.file.stem());
			if (const auto player = getPlayer<PlayerClient>(playerName, PLTYPE_ANYCLIENT); player != nullptr)
			{
				player->recordCurrentPropModTime();
				m_accountLoader->loadAccount(playerName, *player);

				if (player->wasPropModified(PlayerProp::LEVEL))
				{
					// Clear the level so the warp works.
					// Otherwise, it will detect it as a same-level warp and just update the position.
					const auto level = player->account.level;
					player->account.level.clear();

					player->warp(level, player->getGlobalPosition());
				}

				CString propsPacket;
				propsPacket.write(player->getModifiedPropsPacket());
				if (!propsPacket.isEmpty())
				{
					player->sendPacket(CString() >> (char)PLO_PLAYERPROPS << propsPacket);
					sendNonLevelBoundPacketToNearby(CString() >> (char)PLO_OTHERPLPROPS >> (short)player->getId() << propsPacket, player->getGlobalPosition(), player->getLevel(), { player->getId() });
				}
			}
		}
	};

	m_fsServer.categoryEventCallback[ENUM(fs::FileCategory::NPC)] = [this](const fs::FileEventCollection events, const fs::FileData& file)
	{
		if (!hasNPCServer())
			return;

		if (events.test(fs::FileEvent::Deleted))
		{
			auto npcName = fs::getANSIFileName(fs::getHTMLUnescapedFileName(file.file));
			if (npcName.starts_with("npc") && npcName.ends_with(".txt"))
				npcName = npcName.substr(3, npcName.size() - 7); // Remove npc and .txt

			if (const auto npc = m_npcServer->getNPCByName(npcName); npc != nullptr)
			{
				log::printLine(log::server, "NPC deleted from filesystem: [{}] {}", npc->id, npc->name);
				m_npcServer->deleteNPC(npc->id);
			}
		}
		if (events.test(fs::FileEvent::Added))
		{
			auto profile = log::Profile(log::server, "", " ({1:0.6} ms)");
			if (const auto npc = m_npcServer->addNPCFromFile(file.file); npc != nullptr)
			{
				// TODO: Generic prop sending function NPCs.
				const CString packet = CString() >> (char)PLO_NPCPROPS >> (int)npc->id << npc->getAllPropsPacket();
				sendPacketToNearby(packet, npc->getGlobalPosition(), npc->getLevel());

				log::printLine(log::server, "NPC added to filesystem: [{}] {}", npc->id, file.file.stem().generic_string());
			}
		}
		if (events.test(fs::FileEvent::Modified))
		{
			fs::File npcFile{file.file};
			const auto id = string::toNumber<NPCID>(npcFile.readConfigLine("ID", " ").value_or("0"));
			if (id == 0)
				return;

			auto npc = getNPC(id);
			if (npc == nullptr || npc->lastSaveTime == file.getModTime()) return;
			npc->lastSaveTime = file.getModTime();

			// Record the current props and reload the NPC.
			npc->recordCurrentPropModTime();
			m_npcLoader->loadNPC(npcFile, npc, m_frameStartTime);

			// If the NPC's script was updated, queue events, delete the NPC, and resend to everybody (so they get the new script).
			if (npc->wasPropModified(NPCProp::SCRIPT))
			{
				npc->scripting.events.addEvent(ScriptEventType::CREATED, source::FromServer());
				npc->sendScriptUpdatesToLevel(file.getModTime());
			}
			// Otherwise, if the NPC is on a level, try to send only the modified props.
			else if (auto level = npc->getLevel(); level != nullptr)
			{
				// If the level was changed, warp, which will send the props to players in the new level.
				if (npc->level != level->levelName)
				{
					// NPC props ignore values that don't change.
					// Since the level warping is done through NPC props, briefly reset the NPC level so the warp actually happens.
					const auto warpLevel = getLoadedLevel(npc->level, level);
					npc->level = level->levelName;
					npc->warp(warpLevel, npc->getGlobalPosition());
				}
				else
				{
					// Send the modified properties to nearby players.
					CString propsPacket = CString() >> (char)PLO_NPCPROPS >> (int)npc->id << npc->getModifiedPropsPacket();
					if (propsPacket.length() > 4)
						sendPacketToNearby(propsPacket, npc->getGlobalPosition(), level);
				}
			}

			const std::string logMsg = std::format("NPC updated on filesystem: [{}] {}", npc->id, npc->name);
			log::printLine(log::npc, logMsg);
			sendToNC(logMsg);
		}
	};

	m_fsServer.categoryEventCallback[ENUM(fs::FileCategory::SCRIPTCLASS)] = [this](const fs::FileEventCollection events, const fs::FileData& file)
	{
		if (!hasNPCServer())
			return;

		auto className = file.file.stem().string();
		std::string logMsg;

		if (events.test(fs::FileEvent::Deleted))
		{
			if (m_npcServer->deleteClass(className))
			{
				sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_CLASSDELETE << className);
				logMsg = std::format("Class deleted from filesystem: {}", className);
			}
		}
		if (events.test(fs::FileEvent::Added))
		{
			// Class already exists so it was added by NC.
			if (const auto existingClass = m_npcServer->getClass(className); existingClass != nullptr)
				return;

			if (const auto scriptClass = m_npcServer->loadClass(file.file); scriptClass != nullptr)
			{
				sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_CLASSADD << className);
				logMsg = std::format("Class added to filesystem: {}", className);
			}
		}
		if (events.test(fs::FileEvent::Modified))
		{
			// Class mod time matches the file mod time?  Then NC modified it, not the FS.
			const auto fileModTime = fs::getFileModTime(file.file);
			if (const auto existingClass = m_npcServer->getClass(className); existingClass != nullptr && existingClass->modTime == fileModTime)
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

	m_fsServer.categoryEventCallback[ENUM(fs::FileCategory::TRANSLATION)] = [](const fs::FileEventCollection events, const fs::FileData& file)
	{
		if (events.test(fs::FileEvent::Modified))
		{
			if (const auto translationManager = BabyDI::Get<ITranslationManager>(); translationManager != nullptr)
				translationManager->reloadTranslation(file.file);
		}
	};

	m_fsServer.categoryEventCallback[ENUM(fs::FileCategory::WEAPON)] = [this](const fs::FileEventCollection events, const fs::FileData& file)
	{
		if (events.test(fs::FileEvent::Deleted))
		{
			auto weaponName = fs::getANSIFileName(fs::getHTMLEscapedFileName(file.file.stem())).substr(6);
			if (NC_DelWeapon(weaponName))
			{
				const auto logMsg = std::format("Weapon deleted from filesystem: {}", weaponName);
				log::printLine(log::npc, logMsg);
				sendToNC(logMsg);
			}
		}
		if (events.test(fs::FileEvent::Modified))
		{
			const auto fileName = fs::getANSIFileName(file.file);
			const auto newWeapon = Weapon::loadWeapon(fileName);
			if (const auto weapon = getWeapon(newWeapon->name); weapon)
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

	//----------------------------

	m_fsWorld.categoryEventCallback[ENUM(fs::FileCategory::LEVEL)] = [this](const fs::FileEventCollection events, const fs::FileData& file)
	{
		if (events.test(fs::FileEvent::Deleted))
		{
			// When the level gets deleted, players will be warped out.
			m_levelList.erase(fs::getANSIFileName(file.file));
		}
		if (events.test(fs::FileEvent::Modified))
		{
			const auto fileName = fs::getANSIFileName(file.file);
			if (const auto l = getCachedLevelData(fileName); l)
				StaticLevelData::reload(l);
		}
	};

	m_fsWorld.categoryEventCallback[ENUM(fs::FileCategory::FILE)] = [this](const fs::FileEventCollection events, const fs::FileData& file)
	{
		const auto fileName = fs::getANSIFileName(file.file);
		const auto ext = file.file.extension();
		if (events.test(fs::FileEvent::Deleted))
		{
			if (ext == ".gupd")
				m_packageManager.deleteResource(fileName);
		}
		if (events.test(fs::FileEvent::Modified))
		{
			if (ext == ".gupd")
				m_packageManager.findOrAddResource(fileName)->reload(this);
			else if (Generation == ServerGeneration::MODERN)
			{
				// Ganis need to be recompiled on update
				CString bytecodePacket;
				if (ext == ".gani")
				{
					// delete the resource
					m_animationManager.deleteResource(fileName);

					// reload the resource to compile the bytecode again
					if (const auto findAni = m_animationManager.findOrAddResource(fileName); findAni)
						bytecodePacket << findAni->getBytecodePacket();
				}

				// Send the update packet to any v4+ clients that have seen this file
				const CString updatePacket = CString() >> (char)PLO_UPDATEPACKAGEISUPDATED << fileName;
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

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
