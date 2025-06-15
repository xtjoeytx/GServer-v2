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
#include <IUtil.h>

#include <Account.h>
#include <FileSystem.h>
#include <Server.h>
#include <level/Level.h>
#include <npcserver/NPCServer.h>
#include <npcserver/PlayerNPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
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

	m_npcServerPlayer = std::make_shared<PlayerNpcServer>(nullptr, NPCServerPlayerID);
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
}

//----------------------------

void NPCServer::run(std::chrono::milliseconds timeDelta)
{
	//auto profile = log::Profile(log::server, "NPCServer::run");

	// Save all NPC mod times and update timeouts.
	{
		for (auto& [id, npc] : m_server->getNPCList())
		{
			npc->recordCurrentPropModTime();
			if (npc->timeout.count() != 0)
			{
				npc->timeout -= timeDelta;
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

	// Run all NPC scripts.
	{
		for (auto& [id, npc] : m_server->getNPCList())
			npc->getScript().executeEvents(npc->scripting.events, source::FromNPC(id));
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
		m_classList[className] = std::make_unique<ScriptClass>(className, scriptData.text());

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
		auto npc = npcLoader.loadNPC(std::filesystem::path{ fileName.toString()});

		if (npc)
		{
			log::printLine(log::server, "[{}] {}", npc->id, npcName);
			npc->scripting.events.addEvent(ScriptEventType::INITIALIZED, source::FromServer());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////

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

bool NPCServer::hasClass(const std::string& name) const
{
	return m_classList.find(name) != m_classList.end();
}

ScriptClass* NPCServer::getClass(const std::string& name) const
{
	auto classIter = m_classList.find(name);
	if (classIter == m_classList.end())
		return nullptr;

	return classIter->second.get();
}

bool NPCServer::deleteClass(const std::string& className)
{
	auto classIter = m_classList.find(className);
	if (classIter == m_classList.end())
		return false;

	m_classList.erase(classIter);
	std::filesystem::remove(std::filesystem::path{ "scripts" } / (className + ".txt"));

	// TODO: Send blank class?

	return true;
}

void NPCServer::updateClass(const std::string& className, const std::string& classCode)
{
	m_classList[className] = std::make_unique<ScriptClass>(className, classCode);
	auto pClass = getClass(className);

	CString filePath = CString("scripts/") << className << ".txt";
	FileSystem::fixPathSeparators(filePath);

	CString fileData(classCode);
	fileData.save(filePath);

	// Update players.
	for (auto& [id, player] : m_server->getPlayerList())
	{
		if (!player->isClient())
			continue;

		if (player->getVersion() >= CLVER_4_0211)
		{
			if (pClass != nullptr)
			{
				const auto& bytecode = pClass->getSource().getClientByteCode();
				if (!bytecode.empty())
				{
					CString out;
					out >> (char)PLO_RAWDATA >> (int)bytecode.size() << "\n";
					out >> (char)PLO_NPCWEAPONSCRIPT;
					out.write(reinterpret_cast<const char*>(bytecode.data()), bytecode.size());

					player->sendPacket(out);
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
