#ifndef NPCSERVER_H
#define NPCSERVER_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <ranges>
#include <string_view>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <BabyDI.h>
#include <level/Level.h>
#include <npcserver/PlayerNPCServer.h>
#include <object/NPC.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>
#include <utilities/TimeoutGenerator.h>

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

public:
	[[inline]] std::shared_ptr<Player> getPlayer() const;
	[[inline]] std::shared_ptr<PlayerNPCServer> getPlayerNPCServer() const;

public:
	[[inline]] const auto& getGlobalNPCList() const noexcept;
	[[inline]] const auto& getClassList() const noexcept;

public:
	[[inline]] void addEventToControlNPC(ScriptEventType type, ScriptObjectSource source, string::NotForwardRangeNotString auto&&... args);
	[[inline]] void addEventToControlNPC(ScriptEventType type, ScriptObjectSource source, string::ForwardRangeNotString auto&& range);
	[[inline]] void addEventToLevelNPCsAtPosition(ScriptEventType type, ScriptObjectSource source, std::weak_ptr<Level> level, Position<int16_t> pos, auto&& arg1, auto&&... args);
	[[inline]] void addEventToLevelNPCsAtPosition(ScriptEventType type, ScriptObjectSource source, std::weak_ptr<Level> level, Position<int16_t> pos, std::ranges::forward_range auto&& range);

public:
	std::weak_ptr<NPC> getNPCByName(const std::string& name);
	std::shared_ptr<NPC> addNPC(std::string_view name, NPCID id, std::string_view type, std::string_view scripter, std::shared_ptr<Level> level, Position<float> location);
	void deleteNPC(NPCID id);
	void saveNPCs();
	//std::vector<std::pair<double, std::string>> calculateNPCStats();

public:
	bool hasClass(std::string_view name) const;
	std::weak_ptr<ScriptClass> getClass(std::string_view name) const;
	std::weak_ptr<ScriptClass> addClass(std::string_view className, std::string_view classCode);
	bool deleteClass(std::string_view className);
	void updateClass(std::string_view className, std::string_view classCode);

public:
	ScriptSystem scripting;

private:
	void run(TimeoutGenerator::time_delta delta);
	void processDeletedNPCs();

private:
	void loadClasses();
	void loadDatabaseNPCs();

private:
	BabyDI_INJECT(Server, m_server);

	std::shared_ptr<PlayerNPCServer> m_npcServerPlayer;
	std::string m_ncHost;
	uint16_t m_ncPort = 14900;

	TimeoutGenerator m_runTimeout{ 100ms, true };
	TimeoutGenerator m_timedSave{ 5min, true };

	std::unordered_map<NPCID, std::weak_ptr<NPC>> m_globalNPCList;
	std::unordered_set<NPCID> m_deletedNPCs;
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

inline void NPCServer::addEventToControlNPC(ScriptEventType type, ScriptObjectSource source, string::NotForwardRangeNotString auto&&... args)
{
	for (auto& [id, npcPtr] : m_globalNPCList)
	{
		if (auto npc = npcPtr.lock(); npc != nullptr && npc->scriptType == NPCTYPE_CONTROL)
			npc->scripting.events.addEvent(type, source, std::forward<decltype(args)>(args)...);
	}
}

inline void NPCServer::addEventToControlNPC(ScriptEventType type, ScriptObjectSource source, string::ForwardRangeNotString auto&& range)
{
	for (auto& [id, npcPtr] : m_globalNPCList)
	{
		if (auto npc = npcPtr.lock(); npc != nullptr && npc->scriptType == NPCTYPE_CONTROL)
			npc->scripting.events.addEvent(type, source, std::forward<decltype(range)>(range));
	}
}

inline void NPCServer::addEventToLevelNPCsAtPosition(ScriptEventType type, ScriptObjectSource source, std::weak_ptr<Level> level, Position<int16_t> pos, auto&& arg1, auto&&... args)
{
	auto levelPtr = level.lock();
	if (levelPtr == nullptr)
		return;

	for (const auto& id : levelPtr->getNPCs())
	{
		if (auto npc = m_server->getNPC(id); npc != nullptr)
		{
			Rectangle<int16_t, uint16_t> npcRect = { { npc->character.pixelX, npc->character.pixelY }, npc->shape };
			if (positionInRectangle(pos, npcRect))
				npc->scripting.events.addEvent(type, source, std::forward<decltype(arg1)>(arg1), std::forward<decltype(args)...>(args)...);
		}
	}
}

inline void NPCServer::addEventToLevelNPCsAtPosition(ScriptEventType type, ScriptObjectSource source, std::weak_ptr<Level> level, Position<int16_t> pos, std::ranges::forward_range auto&& range)
{
	auto levelPtr = level.lock();
	if (levelPtr == nullptr)
		return;

	for (const auto& id : levelPtr->getNPCs())
	{
		if (auto npc = m_server->getNPC(id); npc != nullptr)
		{
			Rectangle<int16_t, uint16_t> npcRect = { { npc->character.pixelX, npc->character.pixelY }, npc->shape };
			if (positionInRectangle(pos, npcRect))
				npc->scripting.events.addEvent(type, source, std::forward<decltype(range)>(range));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // NPCSERVER_H
