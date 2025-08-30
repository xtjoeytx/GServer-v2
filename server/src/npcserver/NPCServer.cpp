#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <CString.h>
#include <IEnums.h>

#include <Account.h>
#include <FileSystem.h>
#include <Server.h>
#include <level/Level.h>
#include <npcserver/NPCServer.h>
#include <npcserver/PlayerNPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <object/ShowImg.h>
#include <object/Weapon.h>
#include <player/PlayerClient.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <scripting/gs2/ScriptEngineGS2.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/generator/TimeoutGenerator.h>
#include <utilities/Log.h>
#include <utilities/PropertySerializers.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

void NPCServer::initialize()
{
	// TODO(Nalin): This needs to be an option somewhere.
	scripting.defaultScriptEngine = "GS1";

	// NC options.
	m_ncHost = m_server->getAdminSettings().getStr("ns_ip", "auto").toLower().toString();
	m_ncPort = m_server->getSettings().getInt("serverport", 14900);
	if (m_ncHost == "auto")
		m_ncHost = m_server->getServerList().getServerIP();

	// Make the NPC server player.
	m_npcServerPlayer = std::make_shared<PlayerNPCServer>(nullptr, NPCServerPlayerID);
	m_npcServerPlayer->setType(PLTYPE_NPCSERVER);

	auto& settings = m_server->getSettings();
	auto& account = m_npcServerPlayer->account;

	// TODO(Nalin): The settings manager sees `NICK ` nodes as valid, so it doesn't get a default!  We need to redo settings.
	auto nickname = settings.getStr("nickname", "NPC-Server");
	if (nickname.isEmpty())
		nickname = "NPC-Server";

	// Load the npc-server account.
	m_server->getAccountLoader().loadAccount("(npcserver)", account);
	account.character.headImage = settings.getStr("staffhead", "head25.png").toString();
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
	bool sleepwhennoplayers = m_server->getSettings().getBool("sleepwhennoplayers", true);
	if (!sleepwhennoplayers)
		m_firstNPCSave = false;
}

void NPCServer::setRemoteIp(std::string_view host)
{
	if (m_server->getAdminSettings().getStr("ns_ip", "auto").toLower() == "auto")
		m_ncHost = host;
}

void NPCServer::sendNCLoginToPlayer(std::shared_ptr<Player> player)
{
	// RC's only!
	if (!player->isRC() || !player->account.hasRight(PLPERM_NPCCONTROL))
		return;

	// Grab NPCServer & Send
	// If the player is connecting from the same IP as the NPC server, use that IP.
	std::string connectString = std::format("{},{}", (player->account.ipAddress == m_npcServerPlayer->getSocket()->getLocalIp() ? player->account.ipAddress : m_ncHost), m_ncPort);
	log::printLine(log::server, "-- Sending NPC-Server connection info to '{}': {}", player->account.name, connectString);

	player->sendPacket(CString() >> (char)PLO_NPCSERVERADDR >> (short)m_npcServerPlayer->getId() << connectString);
}

//----------------------------

void NPCServer::update(TimeoutGenerator::time_point currentTime)
{
	// If we are sleeping, don't process updates.
	if (m_sleeping)
	{
		// Update the timeouts so they don't have huge deltas when we wake up.
		m_runTimeout.setLastTimeout(currentTime);
		m_timedSave.setLastTimeout(currentTime);
		return;
	}

	m_runTimeout.update(currentTime);
	m_timedSave.update(currentTime);
}

void NPCServer::run(TimeoutGenerator::time_delta delta)
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
				else npc->timeout = -1ms;

				if (npc->timeout < std::chrono::milliseconds::zero())
				{
					npc->timeout = 0ms;
					npc->scripting.events.addEvent(ScriptEventType::TIMEOUT, source::FromNPC(id));
				}
			}
		}
	}

	// Save all player prop mod times.
	for (auto& [id, player] : m_playerList)
	{
		player->recordCurrentPropModTime();
	}

	// Run all weapon scripts.
	for (auto& [name, weapon] : m_server->getWeaponList())
	{
		// Copy the shared_ptr so if we "destroy" gets called, the weapon isn't immediately deleted while we are running the script.
		WeaponPtr copy = weapon;
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
		for (auto& [id, npc] : m_server->getNPCList())
		{
			if (auto level = npc->getLevel(); level != nullptr)
			{
				// Send props packet.
				propsPacket.clear();
				propsPacket.writeGChar((char)PLO_NPCPROPS) >> (int)npc->id << npc->getModifiedPropsPacket();
				if (propsPacket.length() > 4)
					m_server->sendPacketToNearby(propsPacket, npc->getGlobalPosition(), level);

				// Send movements.
				npc->sendMoveQueueToLevel(level, m_server->getFrameStartTime());
			}
		}
	}

	// Send all changed player props.
	{
		CString propsPacket;
		for (auto& [id, player] : m_playerList)
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
	processDeletedPlayers();

	// If we have no players, enter sleep mode.
	// We do it this way to give the server time to process logouts, and to force an NPC save (since saves will be disabled while sleeping).
	bool sleepwhennoplayers = m_server->getSettings().getBool("sleepwhennoplayers", true);
	if (sleepwhennoplayers && m_playerList.empty())
	{
		m_sleeping = true;
		saveNPCs();
	}
}

//////////////////////////////////////////////////////////////////////////////

void NPCServer::loadClasses()
{
	auto indent = log::server.indent();

	FileSystem scriptFS;
	scriptFS.addDir("scripts", "*.txt");
	const std::map<CString, CString>& scriptFileList = scriptFS.getFileList();
	for (auto& scriptFile : scriptFileList)
	{
		auto profile = log::Profile(log::server, "", " ({1:0.6} ms)");
		std::string className = scriptFile.first.subString(0, scriptFile.first.length() - 4).text();

		CString scriptData;
		scriptData.load(scriptFile.second);

		auto scriptClass = std::make_shared<ScriptClass>(className, scriptData.text());
		scriptClass->modTime = clock::from_time_t(scriptFS.getModTime(scriptFile.second));
		m_classList[className] = scriptClass;

		log::print(log::server, "{}", className);
	}
}

void NPCServer::loadDatabaseNPCs()
{
	auto indent = log::server.indent();

	FileSystem npcFS;
	npcFS.addDir("npcs", "npc*.txt");

	auto& npcLoader = m_server->getNPCLoader();

	auto& npcFileList = npcFS.getFileList();
	for (const auto& [npcName, fileName] : npcFileList)
	{
		auto profile = log::Profile(log::server, "", " ({1:0.6} ms)");
		auto npc = npcLoader.loadNPC(std::filesystem::path{ fileName.toString()});
		if (npc)
		{
			log::print(log::server, "[{}] {}", npc->id, npcName);
			npc->scripting.events.addEvent(ScriptEventType::INITIALIZED, source::FromServer());
			if (npc->scriptType != NPCTYPE_LOCAL)
				m_globalNPCList[npc->id] = npc;
		}
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

	log::printLine(log::server, ":: Saving NPCs...");
	for (const auto& [npcId, npcPtr] : m_globalNPCList)
	{
		if (auto npc = npcPtr.lock(); npc != nullptr)
			m_server->getNPCLoader().saveNPC(npc);
	}
}

//////////////////////////////////////////////////////////////////////////////

void NPCServer::playerLogin(std::shared_ptr<Player> player)
{
	m_playerList[player->getId()] = player;
	m_sleeping = false;
}

void NPCServer::playerLogout(std::shared_ptr<Player> player)
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

std::weak_ptr<NPC> NPCServer::getNPCByName(const std::string& name)
{
	for (const auto& [_, npc] : m_globalNPCList)
	{
		if (npc.lock()->name == name)
			return npc;
	}

	return {};
}

std::shared_ptr<NPC> NPCServer::addNPC(std::string_view image, std::string_view script, std::shared_ptr<Level> level, Position<float> location)
{
	auto npc = m_server->addNPC(image, script, location.x(), location.y(), level, NPCStorageType::DATABASE, true);
	npc->setPropWith<NPCProp::TYPE>(SetBy::SERVER, NPCTYPE_LOCAL);
	m_globalNPCList[npc->id] = npc;
	return npc;
}

std::shared_ptr<NPC> NPCServer::addNPC(std::string_view name, NPCID id, std::string_view type, std::string_view scripter, std::shared_ptr<Level> level, Position<float> location)
{
	NPCPtr npc = nullptr;

	if (type == NPCTYPE_LOCAL)
		npc = std::make_shared<NPC>(id, NPCStorageType::LEVEL);
	else npc = std::make_shared<NPC>(id, NPCStorageType::DATABASE);

	npc->name = name;
	npc->setLevel(level);
	npc->setPropWith<NPCProp::TYPE>(SetBy::SERVER, type);
	npc->setPropWith<NPCProp::SCRIPTER>(SetBy::SERVER, scripter);
	npc->setPropWith<NPCProp::X>(SetBy::SERVER, location.x());
	npc->setPropWith<NPCProp::Y>(SetBy::SERVER, location.y());

	if (level)
	{
		if (level->isOnGmap())
		{
			npc->setPropWith<NPCProp::GMAPLEVELX>(SetBy::SERVER, level->mapPosition.x());
			npc->setPropWith<NPCProp::GMAPLEVELY>(SetBy::SERVER, level->mapPosition.y());
		}
		level->addNPC(npc);
	}

	m_server->addNPC(npc, true);
	m_globalNPCList[npc->id] = npc;

	if (type != NPCTYPE_LOCAL)
	{
		CString props = npc->getPropsPacketFor<NPCProp::NAME, NPCProp::TYPE, NPCProp::CURLEVEL>();
		m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_NPCADD >> (int)npc->id << props);
	}

	return npc;
}

void NPCServer::deleteNPC(NPCID id)
{
	m_deletedNPCs.insert(id);
}

void NPCServer::processDeletedNPCs()
{
	for (const auto& npcId : m_deletedNPCs)
	{
		auto npc = m_server->getNPC(npcId);

		// Remove from the global list.
		m_globalNPCList.erase(npcId);

		// Remove from the server's NPC list.
		m_server->deleteNPC(npcId, true);

		// Delete the NPC from the filesystem.
		if (npc != nullptr)
			std::filesystem::remove(std::filesystem::path{ "npcs" } / std::format("npc{}.txt", npc->name));
	}
	m_deletedNPCs.clear();
}

//----------------------------

bool NPCServer::hasClass(std::string_view name) const
{
	return m_classList.find(name) != m_classList.end();
}

std::weak_ptr<ScriptClass> NPCServer::getClass(std::string_view name) const
{
	auto classIter = m_classList.find(name);
	if (classIter == m_classList.end())
		return {};

	return classIter->second;
}

bool NPCServer::deleteClass(std::string_view className)
{
	auto classIter = m_classList.find(className);
	if (classIter == m_classList.end())
		return false;

	m_classList.erase(classIter);
	std::filesystem::remove(std::filesystem::path{ "scripts" } / std::format("{}.txt", className));

	// TODO: Send blank class?

	return true;
}

std::weak_ptr<ScriptClass> NPCServer::addClass(std::string_view className, std::string_view classCode)
{
	CString filePath = CString("scripts/") << className << ".txt";
	FileSystem::fixPathSeparators(filePath);

	CString fileData(classCode);
	fileData.save(filePath);

	auto scriptClass = std::make_shared<ScriptClass>(className, classCode);
	scriptClass->modTime = clock::now();
	m_classList[std::string{ className }] = scriptClass;

	m_server->updateClassForPlayers(scriptClass);
	return scriptClass;
}

void NPCServer::updateClass(std::string_view className, std::string_view classCode)
{
	auto it = m_classList.find(className);
	if (it == m_classList.end())
		return;

	auto& scriptClass = it->second;
	scriptClass->setScript(classCode);

	CString filePath = CString("scripts/") << className << ".txt";
	FileSystem::fixPathSeparators(filePath);

	CString fileData(classCode);
	fileData.save(filePath);

	// Classic servers were GS1 only and did not support GS2 classes.
	if (m_server->Generation == ServerGeneration::CLASSIC)
		return;

	// Update players.
	m_server->updateClassForPlayers(scriptClass);
}

//----------------------------

void NPCServer::showImage(std::shared_ptr<NPC> npc, uint8_t index, const PixelPosition& position, std::string_view image) const
{
	if (index > 199)
		return;

	auto level = npc->getLevel();
	if (level == nullptr)
		return;

	auto showimg = ShowImg::ConstructImage(m_frameStartTime, position, image);
	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGNPC >> (int)npc->id >> (char)(index + 10) << showimg.getAllPropsPacket(), npc->getGlobalPosition(), level);
	npc->showImgList[index] = std::move(showimg);
}

void NPCServer::showText(std::shared_ptr<NPC> npc, uint8_t index, const PixelPosition& position, std::string_view text, std::string_view font, std::string_view style) const
{
	if (index > 199 || text.empty())
		return;

	auto level = npc->getLevel();
	if (level == nullptr)
		return;

	auto showimg = ShowImg::ConstructText(m_frameStartTime, position, text, font, style);
	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGNPC >> (int)npc->id >> (char)(index + 10) << showimg.getAllPropsPacket(), npc->getGlobalPosition(), level);
	npc->showImgList[index] = std::move(showimg);
}

void NPCServer::showGani(std::shared_ptr<NPC> npc, uint8_t index, const PixelPosition& position, std::string_view animation, uint8_t direction) const
{
	if (index > 199)
		return;

	auto level = npc->getLevel();
	if (level == nullptr)
		return;

	auto showimg = ShowImg::ConstructGani(m_frameStartTime, position, animation, direction);
	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGNPC >> (int)npc->id >> (char)(index + 10) << showimg.getAllPropsPacket(), npc->getGlobalPosition(), level);
	npc->showImgList[index] = std::move(showimg);
}

void NPCServer::showPoly(std::shared_ptr<NPC> npc, uint8_t index, const std::vector<double>& points) const
{
	if (index > 199)
		return;

	auto level = npc->getLevel();
	if (level == nullptr)
		return;

	auto showimg = ShowImg::ConstructPoly(m_frameStartTime, points);
	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGNPC >> (int)npc->id >> (char)(index + 10) << showimg.getAllPropsPacket(), npc->getGlobalPosition(), level);
	npc->showImgList[index] = std::move(showimg);
}

void NPCServer::changeShowImgColors(std::shared_ptr<NPC> npc, uint8_t index, float red, float green, float blue, float alpha) const
{
	if (index > 199)
		return;

	auto level = npc->getLevel();
	if (level == nullptr)
		return;

	auto iter = npc->showImgList.find(index);
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

void NPCServer::changeShowImgMode(std::shared_ptr<NPC> npc, uint8_t index, uint8_t drawMode) const
{
	if (index > 199)
		return;

	auto level = npc->getLevel();
	if (level == nullptr)
		return;

	auto iter = npc->showImgList.find(index);
	if (iter == std::end(npc->showImgList))
		return;

	ShowImg& showimg = iter->second;
	showimg.drawMode = drawMode;
	showimg.modTime[PROPID(ShowImgProp::DRAWMODE)] = m_frameStartTime;

	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGNPC >> (int)npc->id >> (char)(index + 10) << showimg.getAllPropsPacket(m_frameStartTime), npc->getGlobalPosition(), level);
}

void NPCServer::changeShowImgPart(std::shared_ptr<NPC> npc, uint8_t index, const ImagePartRectangle& imagePart) const
{
	if (index > 199)
		return;

	auto level = npc->getLevel();
	if (level == nullptr)
		return;

	auto iter = npc->showImgList.find(index);
	if (iter == std::end(npc->showImgList))
		return;

	ShowImg& showimg = iter->second;
	showimg.imagePart = imagePart;
	showimg.modTime[PROPID(ShowImgProp::IMAGEPART)] = m_frameStartTime;

	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGNPC >> (int)npc->id >> (char)(index + 10) << showimg.getAllPropsPacket(m_frameStartTime), npc->getGlobalPosition(), level);
}

void NPCServer::changeShowImgLayer(std::shared_ptr<NPC> npc, uint8_t index, uint8_t layer) const
{
	if (index > 199)
		return;

	auto level = npc->getLevel();
	if (level == nullptr)
		return;

	auto iter = npc->showImgList.find(index);
	if (iter == std::end(npc->showImgList))
		return;

	ShowImg& showimg = iter->second;
	showimg.layer = layer;
	showimg.modTime[PROPID(ShowImgProp::LAYER)] = m_frameStartTime;

	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGNPC >> (int)npc->id >> (char)(index + 10) << showimg.getAllPropsPacket(m_frameStartTime), npc->getGlobalPosition(), level);
}

void NPCServer::changeShowImgZoom(std::shared_ptr<NPC> npc, uint8_t index, float zoom) const
{
	if (index > 199)
		return;

	auto level = npc->getLevel();
	if (level == nullptr)
		return;

	auto iter = npc->showImgList.find(index);
	if (iter == std::end(npc->showImgList))
		return;

	ShowImg& showimg = iter->second;
	showimg.zoom = std::clamp(zoom, 0.0f, 22.0f);
	showimg.modTime[PROPID(ShowImgProp::ZOOM)] = m_frameStartTime;

	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGNPC >> (int)npc->id >> (char)(index + 10) << showimg.getAllPropsPacket(m_frameStartTime), npc->getGlobalPosition(), level);
}

void NPCServer::hideImages(std::shared_ptr<NPC> npc, uint8_t index, std::optional<uint8_t> endIndex) const
{
	if (index > 199)
		return;

	for (uint8_t i = index; i <= endIndex.value_or(index); ++i)
		npc->showImgList.erase(i);

	npc->sendAllShowImagesToLevel();
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
