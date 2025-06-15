#ifndef NPCSERVER_H
#define NPCSERVER_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <level/Level.h>
#include <npcserver/PlayerNPCServer.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptSystem.h>
#include <utilities/TimeoutGenerator.h>
#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class Server;
class NPC;
class Player;

class NPCServer
{
public:
	NPCServer() = default;
	NPCServer(const NPCServer&) = delete;
	NPCServer(NPCServer&&) = delete;
	NPCServer& operator=(const NPCServer&) = delete;
	NPCServer& operator=(NPCServer&&) = delete;

public:
	void initialize();

public:
	void update(TimeoutGenerator::time_point currentTime = std::chrono::high_resolution_clock::now());

private:
	void run(TimeoutGenerator::time_delta delta);

private:
	void loadClasses();
	void loadDatabaseNPCs();

public:
	//void saveNPCs();
	//std::vector<std::pair<double, std::string>> calculateNPCStats();

public:
	[[inline]] std::shared_ptr<Player> getPlayer() const;
	[[inline]] std::shared_ptr<PlayerNPCServer> getPlayerNPCServer() const;

public:
	[[inline]] const std::unordered_map<NPCID, std::weak_ptr<NPC>>& getGlobalNPCList() const noexcept;
	[[inline]] const std::unordered_map<std::string, std::unique_ptr<ScriptClass>>& getClassList() const noexcept;

public:
	std::weak_ptr<NPC> getNPCByName(const std::string& name);

public:
	bool hasClass(const std::string& name) const;
	ScriptClass* getClass(const std::string& name) const;
	bool deleteClass(const std::string& className);
	void updateClass(const std::string& className, const std::string& classCode);

public:
	ScriptSystem scripting;

private:
	BabyDI_INJECT(Server, m_server);

	std::shared_ptr<PlayerNPCServer> m_npcServerPlayer;

	TimeoutGenerator m_runTimeout{ 100ms, true };

	std::unordered_map<NPCID, std::weak_ptr<NPC>> m_globalNPCList;
	std::unordered_map<std::string, std::unique_ptr<ScriptClass>> m_classList;
};

inline std::shared_ptr<Player> NPCServer::getPlayer() const
{
	return std::dynamic_pointer_cast<Player>(m_npcServerPlayer);
}

inline std::shared_ptr<PlayerNPCServer> NPCServer::getPlayerNPCServer() const
{
	return m_npcServerPlayer;
}

inline const std::unordered_map<NPCID, std::weak_ptr<NPC>>& NPCServer::getGlobalNPCList() const noexcept
{
	return m_globalNPCList;
}

inline const std::unordered_map<std::string, std::unique_ptr<ScriptClass>>& NPCServer::getClassList() const noexcept
{
	return m_classList;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // NPCSERVER_H
