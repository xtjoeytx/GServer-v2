#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <CString.h>
#include <IEnums.h>

#include <Account.h>
#include <Server.h>
#include <filesystem/File.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <level/Level.h>
#include <level/LevelTileTypes.h>
#include <npcserver/NPCServer.h>
#include <npcserver/PlayerNPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <object/ShowImg.h>
#include <object/Weapon.h>
#include <player/PlayerClient.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <scripting/gs2/ScriptEngineGS2.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/PropertySerializers.h>
#include <utilities/StringUtils.h>
#include <utilities/generator/TimeoutGenerator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

NPCServer::NPCServer()
{
	m_server = BabyDI::Get<Server>();
}

void NPCServer::initialize()
{
	// TODO(Nalin): This needs to be an option somewhere.
	scripting.defaultScriptEngine = "GS1";

	// NC options.
	m_ncHost = string::toLower(m_server->getAdminSettings().get<std::string>("ns_ip").value_or("auto"));
	m_ncPort = m_server->getSettings().get<uint16_t>("serverport").value_or(14900);
	if (m_ncHost == "auto")
		m_ncHost = m_server->getServerList().getServerIP();

	// Make the NPC server player.
	m_npcServerPlayer = std::make_shared<PlayerNPCServer>(nullptr, NPCServerPlayerID);
	m_npcServerPlayer->setType(PLTYPE_NPCSERVER);

	const auto& settings = m_server->getSettings();
	auto& account = m_npcServerPlayer->account;

	// TODO(Nalin): The settings manager sees `NICK ` nodes as valid, so it doesn't get a default!  We need to redo settings.
	auto nickname = settings.get<std::string>("nickname").value_or("NPC-Server");
	if (nickname.empty())
		nickname = "NPC-Server";

	// Load the npc-server account.
	m_server->getAccountLoader().loadAccount("(npcserver)", account);
	account.character.headImage = settings.get<std::string>("staffhead").value_or("head25.png");
	account.character.nickName = std::format("{} (Server)", nickname);
	account.level = "";
	m_npcServerPlayer->setLoaded(true);

	// Add the npc-server player to the player list.
	m_server->addPlayer(m_npcServerPlayer, NPCServerPlayerID);

	// Load the GS1 and GS2 engines.
	// They must always be loaded as the client will only accept GS1 or GS2 scripts.
	scripting.registerScriptEngine("GS1", std::make_shared<gs1::ScriptEngineGS1>());
	scripting.registerScriptEngine("GS2", std::make_shared<gs2::ScriptEngineGS2>());

	log::printLine(log::server, "Loading classes...");
	loadClasses();

	log::printLine(log::server, "Loading Database NPCs...");
	loadDatabaseNPCs();

	m_runTimeout.callbackDuration = std::bind(&NPCServer::run, this, std::placeholders::_1);
	m_timedSave.callbackDuration = std::bind(&NPCServer::saveNPCs, this);

	// TODO(Nalin): Need an event system and this should be called after the Server sends an "all done loading" event.
	m_runTimeout.start();
	m_timedSave.start();

	// If we don't sleep, unset the first NPC save flag.
	// We won't run into the problem where we immediately save on server start.
	if (!m_server->cached.sleepWhenNoPlayers.getValue())
		m_firstNPCSave = false;
}

void NPCServer::setRemoteIp(const std::string_view host)
{
	if (string::equalsi(m_server->getAdminSettings().get<std::string>("ns_ip").value_or("auto"), "auto"sv))
		m_ncHost = host;
}

void NPCServer::sendNCLoginToPlayer(const std::shared_ptr<Player>& player)
{
	// RC's only!
	if (!player->isRC() || !player->account.hasRight(PLPERM_NPCCONTROL))
		return;

	// Grab NPCServer & Send
	// If the player is connecting from the same IP as the NPC server, use that IP.
	const std::string connectString = std::format("{},{}", (player->account.ipAddress == CSocket::getLocalIp() ? player->account.ipAddress : m_ncHost), m_ncPort);
	log::printLine(log::server, "-- Sending NPC-Server connection info to '{}': {}", player->account.name, connectString);

	player->sendPacket(CString() >> (char)PLO_NPCSERVERADDR >> (short)m_npcServerPlayer->getId() << connectString);
}

//----------------------------

void NPCServer::update(const TimeoutGenerator::time_point currentTime)
{
	// If we are sleeping, don't process updates.
	if (m_sleeping)
	{
		// Update the timeouts so they don't have huge deltas when we wake up.
		m_runTimeout.setLastTimeout(currentTime);
		m_timedSave.setLastTimeout(currentTime);

		processDeletedNPCs();
		processUnloadedNPCs();
		return;
	}

	m_runTimeout.update(currentTime);
	m_timedSave.update(currentTime);
}

void NPCServer::run(const TimeoutGenerator::time_delta delta)
{
	//auto profile = log::Profile(log::server, "NPCServer::run");
	m_frameStartTime = clock::now();

	// Save all NPC mod times and update timeouts.
	{
		for (auto& [id, npc] : m_server->getNPCList())
		{
			npc->recordCurrentPropModTime();

			// TODO(Nalin): Replace with TimeoutGenerator.
			if (npc->timeout.count() != 0)
			{
				if (delta < npc->timeout)
					npc->timeout -= delta;
				else
					npc->timeout = -1ms;

				if (npc->timeout < std::chrono::milliseconds::zero())
				{
					npc->timeout = 0ms;
					npc->scripting.events.addEvent(ScriptEventType::TIMEOUT, source::FromNPC(id));
				}
			}
		}
	}

	// Save all player prop mod times.
	for (const auto& player : m_playerList | std::views::values)
	{
		player->recordCurrentPropModTime();
	}

	// Run all weapon scripts.
	for (const auto& weapon : m_server->getWeaponList() | std::views::values)
	{
		// Copy the shared_ptr so if we "destroy" gets called, the weapon isn't immediately deleted while we are running the script.
		const WeaponPtr copy = weapon;
		copy->executeEvents(weapon->scripting.events, source::FromWeapon(weapon));
	}

	// Process all NPC movements.
	// Run all NPC scripts.
	for (auto& [id, npc] : m_server->getNPCList())
	{
		// Process movement.
		npc->processMoveQueue(delta);

		// Process scripts.
		npc->executeEvents(npc->scripting.events, source::FromNPC(id));
	}

	// Send all changed NPC props.
	// Send all queued movements.
	{
		CString propsPacket;
		for (const auto& npc : m_server->getNPCList() | std::views::values)
		{
			if (auto level = npc->getLevel(); level != nullptr)
			{
				// Send props packet.
				propsPacket.clear();
				propsPacket.writeGChar((char)PLO_NPCPROPS) >> (int)npc->id << npc->getModifiedPropsPacket();
				if (propsPacket.length() > 4)
					m_server->sendPacketToNearby(propsPacket, npc->getGlobalPosition(), level);

				// Send movements.
				npc->sendMoveQueueUpdatesToLevel(level);
			}
		}
	}

	// Send all changed player props.
	{
		CString propsPacket;
		for (auto& player : m_playerList | std::views::values)
		{
			auto playerClient = std::dynamic_pointer_cast<PlayerClient>(player);
			if (playerClient == nullptr) continue;

			propsPacket.clear();
			propsPacket.write(player->getModifiedPropsPacket());
			if (propsPacket.isEmpty()) continue;

			player->sendPacket(CString() >> (char)PLO_PLAYERPROPS << propsPacket);
			m_server->sendPacketToNearby(CString() >> (char)PLO_OTHERPLPROPS >> (short)player->getId() << propsPacket, playerClient->getGlobalPosition(), playerClient->getLevel(), { player->getId() });
		}
	}

	// Process deleted NPCs and players.
	processDeletedNPCs();
	processUnloadedNPCs();
	processDeletedPlayers();

	// If we have no players, enter sleep mode.
	// We do it this way to give the server time to process logouts, and to force an NPC save (since saves will be disabled while sleeping).
	if (m_server->cached.sleepWhenNoPlayers.getValue() && m_playerList.empty())
	{
		m_sleeping = true;
		saveNPCs();
	}
}

//////////////////////////////////////////////////////////////////////////////

void NPCServer::loadClasses()
{
	auto indent = log::server.indent();

	for (auto info : m_server->getFileSystemServer().info(fs::FileCategory::SCRIPTCLASS) | toSharedPtr)
	{
		if (info == nullptr) continue;

		auto profile = log::Profile(log::server, "", " ({1:0.6} ms)");
		std::string fileName = fs::getANSIFileName(info->file);
		std::string className = fileName.substr(0, fileName.length() - 4);

		CString scriptData;
		scriptData.load(info->file.string());

		const auto scriptClass = std::make_shared<ScriptClass>(className, scriptData.text());
		scriptClass->modTime = info->getModTime();
		m_classList[className] = scriptClass;

		log::print(log::server, "{}", className);
	}
}

void NPCServer::loadDatabaseNPCs()
{
	auto indent = log::server.indent();

	for (auto info : m_server->getFileSystemServer().info(fs::FileCategory::NPC) | toSharedPtr)
	{
		if (info == nullptr) continue;

		auto profile = log::Profile(log::server, "", " ({1:0.6} ms)");
		if (auto npc = addNPCFromFile(info->file); npc != nullptr)
			log::print(log::server, "[{}] {}", npc->id, npc->name);
	}
}

void NPCServer::saveNPCs()
{
	// Avoid saving NPCs immediately after the server starts.
	if (m_firstNPCSave)
	{
		m_firstNPCSave = false;
		return;
	}

	log::printLine(log::server, "Saving NPCs.");
	for (const auto& npcPtr : m_globalNPCList | std::views::values)
	{
		if (auto npc = npcPtr.lock(); npc != nullptr)
			m_server->getNPCLoader().saveNPC(npc);
	}
}

//////////////////////////////////////////////////////////////////////////////

void NPCServer::playerLogin(const std::shared_ptr<Player>& player)
{
	m_playerList[player->getId()] = player;
	m_sleeping = false;
}

void NPCServer::playerLogout(const std::shared_ptr<Player>& player)
{
	m_deletedPlayers.insert(player);
	addEventToControlNPC(ScriptEventType::PLAYERLOGOUT, source::FromPlayer(player->getId()));
}

void NPCServer::processDeletedPlayers()
{
	for (const auto& player : m_deletedPlayers)
		m_playerList.erase(player->getId());

	m_deletedPlayers.clear();
}

//////////////////////////////////////////////////////////////////////////////

std::shared_ptr<NPC> NPCServer::getNPC(const NPCID id) const
{
	return m_server->getNPC(id);
}

std::shared_ptr<NPC> NPCServer::getNPCByName(const std::string& name)
{
	for (const auto& npc : m_globalNPCList | std::views::values)
	{
		if (auto npcptr = npc.lock(); npcptr != nullptr)
		{
			if (string::equalsi(name, npcptr->name))
				return npcptr;
		}
	}

	return nullptr;
}

std::shared_ptr<NPC> NPCServer::addNPC(const std::string_view image, const std::string_view script, const std::shared_ptr<Level>& level, const TilePosition& location, const std::string_view type)
{
	auto npc = m_server->addNPC(image, script, location.x(), location.y(), level, NPCStorageType::DATABASE, true, type);
	m_globalNPCList[npc->id] = npc;
	return npc;
}

std::shared_ptr<NPC> NPCServer::addNPC(const std::string_view name, const NPCID id, const std::string_view type, const std::string_view scripter, const std::shared_ptr<Level>& level, const TilePosition& location)
{
	NPCPtr npc = nullptr;

	if (type == NPCTYPE_LOCAL)
		npc = std::make_shared<NPC>(id, NPCStorageType::LEVEL);
	else
		npc = std::make_shared<NPC>(id, NPCStorageType::DATABASE);

	const auto pixelPosition = toPixelPosition(location);
	auto localPixelPosition = toLocalPixelPosition(pixelPosition);
	auto mapPosition = toMapPosition(pixelPosition);

	npc->name = name;
	npc->setPropWith<NPCProp::TYPE>(SetBy::SERVER, type);
	npc->setPropWith<NPCProp::SCRIPTER>(SetBy::SERVER, scripter);
	npc->setPropWith<NPCProp::X2>(SetBy::SERVER, localPixelPosition.x());
	npc->setPropWith<NPCProp::Y2>(SetBy::SERVER, localPixelPosition.y());

	if (level)
	{
		npc->level = level->levelName;
		if (level->isGmap())
		{
			npc->setPropWith<NPCProp::GMAPLEVELX>(SetBy::SERVER, mapPosition.x());
			npc->setPropWith<NPCProp::GMAPLEVELY>(SetBy::SERVER, mapPosition.y());
		}
	}

	m_server->addNPC(npc, true);
	m_globalNPCList[npc->id] = npc;

	if (type != NPCTYPE_LOCAL)
	{
		const CString props = npc->getPropsPacketFor<NPCProp::NAME, NPCProp::TYPE, NPCProp::LEVEL>();
		m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_NPCADD >> (int)npc->id << props);
	}

	return npc;
}

std::shared_ptr<NPC> NPCServer::addNPCFromFile(const std::filesystem::path& filePath)
{
	auto& npcLoader = m_server->getNPCLoader();
	auto npc = npcLoader.loadNPC(filePath);
	if (npc)
	{
		npc->scripting.events.addEvent(ScriptEventType::INITIALIZED, source::FromServer());
		if (npc->scriptType != NPCTYPE_LOCAL)
		{
			m_globalNPCList[npc->id] = npc;

			const CString props = npc->getPropsPacketFor<NPCProp::NAME, NPCProp::TYPE, NPCProp::LEVEL>();
			m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_NPCADD >> (int)npc->id << props);
		}
	}
	return npc;
}

void NPCServer::deleteNPC(const NPCID id)
{
	m_deletedNPCs.insert(id);
}

void NPCServer::unloadNPC(const NPCID id)
{
	m_unloadedNPCs.insert(id);
}

void NPCServer::processControlNPCs() const
{
	for (auto& [id, npc] : m_server->getNPCList())
	{
		if (npc->scriptType != NPCTYPE_CONTROL)
			continue;

		// Process scripts.
		npc->executeEvents(npc->scripting.events, source::FromNPC(id));
	}
}

void NPCServer::processDeletedNPCs()
{
	if (m_deletedNPCs.empty())
		return;

	for (const auto& npcId : m_deletedNPCs)
	{
		auto npc = m_server->getNPC(npcId);

		// Remove from the global list.
		m_globalNPCList.erase(npcId);

		// Remove from the server's NPC list.
		m_server->deleteNPC(npcId, true);

		// Delete the NPC from the filesystem.
		if (npc != nullptr)
			std::filesystem::remove(std::filesystem::path{ "npcs" } / fs::getHTMLEscapedFileName(std::format("npc{}.txt", npc->name)));
	}
	m_deletedNPCs.clear();
}

void NPCServer::processUnloadedNPCs()
{
	if (m_unloadedNPCs.empty())
		return;

	auto& npcLoader = m_server->getNPCLoader();
	for (const auto& npcId : m_unloadedNPCs)
	{
		auto npc = m_server->getNPC(npcId);

		// Don't remove database NPCs.
		// TODO: Make it so database NPCs can be loaded/unloaded on demand.
		if (npc != nullptr && npc->storageType == NPCStorageType::DATABASE)
		{
			npcLoader.saveNPC(npc);

			// If the level no longer exists, stub the level so this NPC works again once it comes back.
			if (npc->getLevel() == nullptr)
			{
				if (auto level = m_server->getStubbedLevel(npc->level, npc->groupName); level != nullptr)
					level->addNPC(npc);
			}
			continue;
		}

		// Remove from the global list.
		m_globalNPCList.erase(npcId);

		// Remove from the server's NPC list.
		m_server->deleteNPC(npcId, true);
	}
	m_unloadedNPCs.clear();
}

//----------------------------

bool NPCServer::hasClass(const std::string_view name) const
{
	return m_classList.contains(name);
}

std::shared_ptr<ScriptClass> NPCServer::getClass(const std::string_view name) const
{
	const auto classIter = m_classList.find(name);
	if (classIter == m_classList.end())
		return nullptr;

	return classIter->second;
}

bool NPCServer::deleteClass(const std::string_view className)
{
	const auto classIter = m_classList.find(className);
	if (classIter == m_classList.end())
		return false;

	m_classList.erase(classIter);
	std::filesystem::remove(std::filesystem::path{ "scripts" } / std::format("{}.txt", className));

	// TODO: Send blank class?

	return true;
}

std::shared_ptr<ScriptClass> NPCServer::addClass(const std::string_view className, const std::string_view classCode)
{
	const auto file = m_server->getFileSystemServer().openiForWriting(fs::FileCategory::SCRIPTCLASS, std::format("{}.txt", className), true);
	if (!file) return nullptr;

	const auto& filePath = file->filePath();
	file->clear();
	file->write(classCode);
	file->close();

	auto scriptClass = std::make_shared<ScriptClass>(className, classCode);
	scriptClass->modTime = fs::getFileModTime(filePath);
	m_classList[std::string{ className }] = scriptClass;

	m_server->updateClassForPlayers(scriptClass);
	return scriptClass;
}

std::shared_ptr<ScriptClass> NPCServer::loadClass(const std::filesystem::path& filePath)
{
	CString fileData;
	fileData.load(filePath.string());

	auto className = filePath.stem().string();
	auto scriptClass = std::make_shared<ScriptClass>(className, fileData.toStringView());
	scriptClass->modTime = fs::getFileModTime(filePath);
	m_classList[std::string{ className }] = scriptClass;

	m_server->updateClassForPlayers(scriptClass);
	return scriptClass;
}

void NPCServer::updateClass(const std::string_view className, const std::string_view classCode)
{
	const auto it = m_classList.find(className);
	if (it == m_classList.end())
		return;

	const auto& scriptClass = it->second;
	scriptClass->setScript(classCode);

	const auto file = m_server->getFileSystemServer().openiForWriting(fs::FileCategory::SCRIPTCLASS, std::format("{}.txt", className), true);
	if (!file) return;

	const auto& filePath = file->filePath();
	file->clear();
	file->write(classCode);
	file->close();

	scriptClass->modTime = fs::getFileModTime(filePath);

	m_server->updateClassForPlayers(scriptClass);
}

//----------------------------

void NPCServer::showImage(const std::shared_ptr<NPC>& npc, const uint8_t index, const PixelPosition& position, const std::string_view image) const
{
	if (index > 199)
		return;

	const auto level = npc->getLevel();
	if (level == nullptr)
		return;

	auto showimg = ShowImg::ConstructImage(m_frameStartTime, position, image);
	npc->addShowImg(index, std::move(showimg));
}

void NPCServer::showText(const std::shared_ptr<NPC>& npc, const uint8_t index, const PixelPosition& position, const std::string_view text, const std::string_view font, const std::string_view style) const
{
	if (index > 199 || text.empty())
		return;

	const auto level = npc->getLevel();
	if (level == nullptr)
		return;

	auto showimg = ShowImg::ConstructText(m_frameStartTime, position, text, font, style);
	npc->addShowImg(index, std::move(showimg));
}

void NPCServer::showGani(const std::shared_ptr<NPC>& npc, const uint8_t index, const PixelPosition& position, const std::string_view animation, const uint8_t direction) const
{
	if (index > 199)
		return;

	const auto level = npc->getLevel();
	if (level == nullptr)
		return;

	auto showimg = ShowImg::ConstructGani(m_frameStartTime, position, animation, direction);
	npc->addShowImg(index, std::move(showimg));
}

void NPCServer::showPoly(const std::shared_ptr<NPC>& npc, const uint8_t index, const uint8_t dimensions, const std::vector<double>& points) const
{
	if (index > 199 || dimensions < 2 || dimensions > 3)
		return;

	const auto level = npc->getLevel();
	if (level == nullptr)
		return;

	auto showimg = ShowImg::ConstructPoly(m_frameStartTime, dimensions, points);
	npc->addShowImg(index, std::move(showimg));
}

void NPCServer::changeShowImgColors(const std::shared_ptr<NPC>& npc, const uint8_t index, const float red, const float green, const float blue, const float alpha) const
{
	if (index > 199)
		return;

	const auto level = npc->getLevel();
	if (level == nullptr)
		return;

	const auto iter = npc->showImgList.find(index);
	if (iter == std::end(npc->showImgList))
		return;

	ShowImg& showimg = iter->second;
	showimg.colors[0] = std::clamp(red, 0.0f, 1.0f);
	showimg.colors[1] = std::clamp(green, 0.0f, 1.0f);
	showimg.colors[2] = std::clamp(blue, 0.0f, 1.0f);
	showimg.colors[3] = std::clamp(alpha, 0.0f, 1.0f);
	showimg.modTime[PROPID(ShowImgProp::COLORS)] = m_frameStartTime;

	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGNPC >> (int)npc->id >> (char)(index + 10) << showimg.getAllPropsPacket(m_frameStartTime), npc->getGlobalPosition(), level);
}

void NPCServer::changeShowImgMode(const std::shared_ptr<NPC>& npc, const uint8_t index, const uint8_t drawMode) const
{
	if (index > 199)
		return;

	const auto level = npc->getLevel();
	if (level == nullptr)
		return;

	const auto iter = npc->showImgList.find(index);
	if (iter == std::end(npc->showImgList))
		return;

	ShowImg& showimg = iter->second;
	showimg.drawMode = drawMode;
	showimg.modTime[PROPID(ShowImgProp::DRAWMODE)] = m_frameStartTime;

	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGNPC >> (int)npc->id >> (char)(index + 10) << showimg.getAllPropsPacket(m_frameStartTime), npc->getGlobalPosition(), level);
}

void NPCServer::changeShowImgPart(const std::shared_ptr<NPC>& npc, const uint8_t index, const ImagePartRectangle& imagePart) const
{
	if (index > 199)
		return;

	const auto level = npc->getLevel();
	if (level == nullptr)
		return;

	const auto iter = npc->showImgList.find(index);
	if (iter == std::end(npc->showImgList))
		return;

	ShowImg& showimg = iter->second;
	showimg.imagePart = imagePart;
	showimg.modTime[PROPID(ShowImgProp::IMAGEPART)] = m_frameStartTime;

	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGNPC >> (int)npc->id >> (char)(index + 10) << showimg.getAllPropsPacket(m_frameStartTime), npc->getGlobalPosition(), level);
}

void NPCServer::changeShowImgLayer(const std::shared_ptr<NPC>& npc, const uint8_t index, const uint8_t layer) const
{
	if (index > 199)
		return;

	const auto level = npc->getLevel();
	if (level == nullptr)
		return;

	const auto iter = npc->showImgList.find(index);
	if (iter == std::end(npc->showImgList))
		return;

	ShowImg& showimg = iter->second;
	showimg.layer = layer;
	showimg.modTime[PROPID(ShowImgProp::LAYER)] = m_frameStartTime;

	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGNPC >> (int)npc->id >> (char)(index + 10) << showimg.getAllPropsPacket(m_frameStartTime), npc->getGlobalPosition(), level);
}

void NPCServer::changeShowImgZoom(const std::shared_ptr<NPC>& npc, const uint8_t index, const float zoom) const
{
	if (index > 199)
		return;

	const auto level = npc->getLevel();
	if (level == nullptr)
		return;

	const auto iter = npc->showImgList.find(index);
	if (iter == std::end(npc->showImgList))
		return;

	ShowImg& showimg = iter->second;
	showimg.zoom = std::clamp(zoom, 0.0f, 22.0f);
	showimg.modTime[PROPID(ShowImgProp::ZOOM)] = m_frameStartTime;

	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGNPC >> (int)npc->id >> (char)(index + 10) << showimg.getAllPropsPacket(m_frameStartTime), npc->getGlobalPosition(), level);
}

void NPCServer::hideImages(const std::shared_ptr<NPC>& npc, const uint8_t index, const std::optional<uint8_t> endIndex) const
{
	if (index > 199)
		return;

	for (uint8_t i = index; i <= endIndex.value_or(index); ++i)
		npc->showImgList.erase(i);

	npc->sendAllShowImagesToLevel();
}

///////////////////////////////////////////////////////////////////////////////

tileset::TileType NPCServer::getTileType(const uint16_t tile, const std::shared_ptr<Level>& level) const noexcept
{
	const auto tilesetType = m_server->getTilesetTypeForLevel(level);
	return m_server->getTileTypeForTile(tilesetType, tile);
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
