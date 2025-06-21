#include <chrono>
#include <filesystem>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <string_view>
#include <string>

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
#include <object/Weapon.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <scripting/gs2/ScriptEngineGS2.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>
#include <utilities/PropsContainer.h>
#include <utilities/TimeoutGenerator.h>

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

	// TODO(Nalin): Need an event system and this should be called after the Server sends an "all done loading" event.
	m_runTimeout.start();
}

void NPCServer::sendNCLoginToPlayer(std::shared_ptr<Player> player)
{
	// RC's only!
	if (!player->isRC() || !player->account.hasRight(PLPERM_NPCCONTROL))
		return;

	// Grab NPCServer & Send
	// If the player is connecting from the same IP as the NPC server, use that IP.
	std::string connectString = std::format("{},{}", (player->account.ipAddress == player->getSocket()->getLocalIp() ? player->account.ipAddress : m_ncHost), m_ncPort);

	player->sendPacket(CString() >> (char)PLO_NPCSERVERADDR >> (short)m_npcServerPlayer->getId() << connectString);
}

//----------------------------

void NPCServer::update(TimeoutGenerator::time_point currentTime)
{
	m_runTimeout.update(currentTime);
}

void NPCServer::run(TimeoutGenerator::time_delta delta)
{
	//auto profile = log::Profile(log::server, "NPCServer::run");

	// Save all NPC mod times and update timeouts.
	{
		for (auto& [id, npc] : m_server->getNPCList())
		{
			npc->recordCurrentPropModTime();

			// TODO(Nalin): Replace with TimeoutGenerator.
			if (npc->timeout.count() != 0)
			{
				npc->timeout -= delta;
				if (npc->timeout < std::chrono::milliseconds::zero())
				{
					npc->timeout = 0ms;
					npc->scripting.events.addEvent(ScriptEventType::TIMEOUT, source::FromNPC(id));
				}
			}
		}
	}

	// Save all player prop mod times.
	for (auto& [id, player] : m_server->getPlayerList())
	{
		player->recordCurrentPropModTime();
	}

	// Run all weapon scripts.
	for (auto& [name, weapon] : m_server->getWeaponList())
	{
		weapon->executeEvents(weapon->scripting.events, source::FromWeapon(weapon));
	}

	// Run all NPC scripts.
	for (auto& [id, npc] : m_server->getNPCList())
	{
		npc->executeEvents(npc->scripting.events, source::FromNPC(id));
	}

	// Send all changed NPC props.
	{
		CString propsPacket;
		for (auto& [id, npc] : m_server->getNPCList())
		{
			if (auto level = npc->level.lock(); level != nullptr)
			{
				propsPacket.clear();
				propsPacket.writeGChar((char)PLO_NPCPROPS) >> (int)npc->id << npc->getModifiedPropsPacket();
				if (propsPacket.length() > 4)
					m_server->sendPacketToLevelArea(propsPacket, level);
			}
		}
	}

	// Send all changed player props.
	{
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
		std::string className = scriptFile.first.subString(0, scriptFile.first.length() - 4).text();

		CString scriptData;
		scriptData.load(scriptFile.second);

		auto scriptClass = std::make_shared<ScriptClass>(className, scriptData.text());
		scriptClass->modTime = clock::from_time_t(scriptFS.getModTime(scriptFile.second));
		m_classList[className] = scriptClass;

		log::printLine(log::server, "{}", className);
		//updateClassForPlayers(getClass(className));
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
			m_globalNPCList[npc->id] = npc;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////

std::shared_ptr<NPC> NPCServer::addNPC(std::string_view name, NPCID id, std::string_view type, std::string_view scripter, std::shared_ptr<Level> level, Position<float> location)
{
	NPCPtr npc = nullptr;

	if (type == NPCTYPE_LOCAL)
		npc = std::make_shared<NPC>(id, NPCStorageType::LEVEL);
	else npc = std::make_shared<NPC>(id, NPCStorageType::DATABASE);

	npc->name = name;
	npc->setPropWith<NPCProp::TYPE>(SetBy::SERVER, type);
	npc->setPropWith<NPCProp::SCRIPTER>(SetBy::SERVER, scripter);
	npc->setPropWith<NPCProp::X>(SetBy::SERVER, location.x());
	npc->setPropWith<NPCProp::Y>(SetBy::SERVER, location.y());
	npc->level = level;
	level->addNPC(npc);
	m_server->addNPC(npc, true);
	m_globalNPCList[npc->id] = npc;

	return npc;
}

std::weak_ptr<NPC> NPCServer::getNPCByName(const std::string& name)
{
	for (const auto& [_, npc] : m_globalNPCList)
	{
		if (npc.lock()->name == name)
		{
			return npc;
		}
	}

	return {};
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

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
