#ifndef NPCSERVER_H
#define NPCSERVER_H

#include "common.h"

#include "npcserver/PlayerNpcServer.h"
#include "scripting/ScriptSystem.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

class Server;
class NPC;
class Player;
class ScriptClass;

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
	void run(std::chrono::milliseconds timeDelta);

private:
	void loadClasses();
	void loadNpcs();

public:
	//void saveNpcs();
	//std::vector<std::pair<double, std::string>> calculateNpcStats();

public:
	[[inline]] std::shared_ptr<Player> getPlayer() const;
	[[inline]] std::shared_ptr<PlayerNpcServer> getPlayerNpcServer() const;

public:
	[[inline]] const std::unordered_map<NPCID, std::weak_ptr<NPC>>& getGlobalNpcList() const noexcept;
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

	std::shared_ptr<PlayerNpcServer> m_npcServerPlayer;

	std::chrono::high_resolution_clock::time_point m_lastUpdate;

	std::unordered_map<NPCID, std::weak_ptr<NPC>> m_globalNpcList;
	std::unordered_map<std::string, std::unique_ptr<ScriptClass>> m_classList;
};

inline std::shared_ptr<Player> NPCServer::getPlayer() const
{
	return std::dynamic_pointer_cast<Player>(m_npcServerPlayer);
}

inline std::shared_ptr<PlayerNpcServer> NPCServer::getPlayerNpcServer() const
{
	return m_npcServerPlayer;
}

inline const std::unordered_map<NPCID, std::weak_ptr<NPC>>& NPCServer::getGlobalNpcList() const noexcept
{
	return m_globalNpcList;
}

inline const std::unordered_map<std::string, std::unique_ptr<ScriptClass>>& NPCServer::getClassList() const noexcept
{
	return m_classList;
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // NPCSERVER_H
