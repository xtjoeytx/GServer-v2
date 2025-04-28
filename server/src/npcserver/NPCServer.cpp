#include "BabyDI.h"

#include "Server.h"
#include "npcserver/NPCServer.h"
#include "npcserver/PlayerNpcServer.h"
#include "scripting/gs1/ScriptEngineGS1.h"
#include "scripting/gs2/ScriptEngineGS2.h"
#include "object/NPC.h"
#include "object/Player.h"
#include "utilities/Log.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

void NPCServer::initialize()
{
	m_npcServerPlayer = std::make_shared<PlayerNpcServer>(nullptr, 0);
	m_npcServerPlayer->setType(PLTYPE_NPCSERVER);

	auto& settings = m_server->getSettings();
	auto& account = m_npcServerPlayer->account;

	m_server->getAccountLoader().loadAccount("(npcserver)", account);
	account.character.headImage = settings.getStr("staffhead", "head25.png").toString();
	account.character.nickName = std::format("{} (Server)", settings.getStr("nickname", "NPC-Server"));
	m_npcServerPlayer->setLoaded(true);

	// Load the GS1 and GS2 engines.
	// They must always be loaded as the client will only accept GS1 or GS2 scripts.
	scripting.registerScriptEngine("GS1", std::make_shared<ScriptEngineGS1>());
	scripting.registerScriptEngine("GS2", std::make_shared<ScriptEngineGS2>());

	log::printLine(log::server, "Loading classes...");
	loadClasses();

	log::printLine(log::server, "Loading NPCs...");
	loadNpcs();
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

void NPCServer::loadNpcs()
{
	auto indent = log::server.indent();

	FileSystem npcFS;
	npcFS.addDir("npcs", "npc*.txt");

	auto& npcFileList = npcFS.getFileList();
	for (const auto& [npcName, fileName] : npcFileList)
	{
		bool loaded = false;

		// Create the npc
		//auto newNPC = std::make_shared<NPC>("", "", 30.f, 30.5f, nullptr, NPCType::DBNPC);
		/*
		if (newNPC->loadNPC(fileName))
		{
			int npcId = newNPC->getId();
			if (npcId < 1000)
			{
				log::printLine(log::server, "! NPC '{}' has an id of {}, skipping (under 1000).", newNPC->getName(), npcId);
			}
			/*
			else if (auto existing = m_npcList.find(npcId); existing != std::end(m_npcList))
			{
				log::printLine(log::server, "! NPC '{}' has an id of {}, skipping (id in use).", newNPC->getName(), npcId);
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
			*/
			//}
	}
}

//////////////////////////////////////////////////////////////////////////////

std::weak_ptr<NPC> NPCServer::getNPCByName(const std::string& name)
{
	for (const auto& [_, npc] : m_globalNpcList)
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
				const auto& bytecode = pClass->getSource().getServerByteCode();

				CString out;
				out >> (char)PLO_RAWDATA >> (int)bytecode->size() << "\n";
				out >> (char)PLO_NPCWEAPONSCRIPT;
				out.write(reinterpret_cast<const char*>(bytecode->data()), bytecode->size());

				player->sendPacket(out);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal
