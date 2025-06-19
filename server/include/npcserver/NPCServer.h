#ifndef NPCSERVER_H
#define NPCSERVER_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <BabyDI.h>
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
	void sendNCLoginToPlayer(std::shared_ptr<Player> player);

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
	[[inline]] const auto& getGlobalNPCList() const noexcept;
	[[inline]] const auto& getClassList() const noexcept;

public:
	std::shared_ptr<NPC> addNPC(std::string_view name, NPCID id, std::string_view type, std::string_view scripter, std::shared_ptr<Level> level, Position<float> location);
	std::weak_ptr<NPC> getNPCByName(const std::string& name);

public:
	bool hasClass(std::string_view name) const;
	std::weak_ptr<ScriptClass> getClass(std::string_view name) const;
	std::weak_ptr<ScriptClass> addClass(std::string_view className, std::string_view classCode);
	bool deleteClass(std::string_view className);
	void updateClass(std::string_view className, std::string_view classCode);

public:
	ScriptSystem scripting;

private:
	BabyDI_INJECT(Server, m_server);

	std::shared_ptr<PlayerNPCServer> m_npcServerPlayer;
	std::string m_ncHost;
	uint16_t m_ncPort = 14900;

	TimeoutGenerator m_runTimeout{ 100ms, true };

	std::unordered_map<NPCID, std::weak_ptr<NPC>> m_globalNPCList;
	string_map<std::shared_ptr<ScriptClass>> m_classList;
};

inline std::shared_ptr<Player> NPCServer::getPlayer() const
{
	return std::dynamic_pointer_cast<Player>(m_npcServerPlayer);
}

inline std::shared_ptr<PlayerNPCServer> NPCServer::getPlayerNPCServer() const
{
	return m_npcServerPlayer;
}

inline const auto& NPCServer::getGlobalNPCList() const noexcept
{
	return m_globalNPCList;
}

inline const auto& NPCServer::getClassList() const noexcept
{
	return m_classList;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // NPCSERVER_H
