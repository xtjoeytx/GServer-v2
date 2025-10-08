#ifndef NPCSERVER_H
#define NPCSERVER_H

#include <chrono>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <ranges>
#include <string_view>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <BabyDI.h>
#include <level/Level.h>
#include <npcserver/PlayerNPCServer.h>
#include <object/NPC.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/generator/TimeoutGenerator.h>
#include <utilities/StringUtils.h>

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
	void setRemoteIp(std::string_view host);
	void sendNCLoginToPlayer(std::shared_ptr<Player> player);

public:
	void update(TimeoutGenerator::time_point currentTime = std::chrono::high_resolution_clock::now());

public:
	template<class T = Player> std::shared_ptr<T> getPlayer(const PlayerID id) const;
	template<class T = Player> std::shared_ptr<T> getPlayer(const PlayerID id, int type) const;
	template<class T = Player> std::shared_ptr<T> getPlayer(const std::string& account, int type) const;
	[[inline]] std::shared_ptr<PlayerNPCServer> getPlayerNPCServer() const;

public:
	[[inline]] const auto& getGlobalNPCList() const noexcept;
	[[inline]] const auto& getClassList() const noexcept;
	[[inline]] auto& getPlayerList() noexcept;

public:
	[[inline]] void addEventToControlNPC(ScriptEventType type, ScriptObject source, string::NotInputRangeNotString auto&&... args);
	[[inline]] void addEventToControlNPC(ScriptEventType type, ScriptObject source, string::InputRangeNotString auto&& range);
	[[inline]] size_t addEventToLevelNPCsAtPosition(ScriptEventType type, ScriptObject source, std::weak_ptr<Level> level, PixelPosition pos, auto&& arg1, auto&&... args);
	[[inline]] size_t addEventToLevelNPCsAtPosition(ScriptEventType type, ScriptObject source, std::weak_ptr<Level> level, PixelPosition pos, std::ranges::forward_range auto&& range);

public:
	void playerLogin(std::shared_ptr<Player> player);
	void playerLogout(std::shared_ptr<Player> player);

public:
	std::weak_ptr<NPC> getNPCByName(const std::string& name);
	std::shared_ptr<NPC> addNPC(std::string_view image, std::string_view script, std::shared_ptr<Level> level, Position<float> location, std::string_view type = NPCTYPE_LOCAL);
	std::shared_ptr<NPC> addNPC(std::string_view name, NPCID id, std::string_view type, std::string_view scripter, std::shared_ptr<Level> level, Position<float> location);
	std::shared_ptr<NPC> addNPCFromFile(const std::filesystem::path& filePath);
	void deleteNPC(NPCID id);
	void saveNPCs();
	//std::vector<std::pair<double, std::string>> calculateNPCStats();

public:
	bool hasClass(std::string_view name) const;
	std::weak_ptr<ScriptClass> getClass(std::string_view name) const;
	std::shared_ptr<ScriptClass> addClass(std::string_view className, std::string_view classCode);
	std::shared_ptr<ScriptClass> loadClass(const std::filesystem::path& filePath);
	bool deleteClass(std::string_view className);
	void updateClass(std::string_view className, std::string_view classCode);

public:
	void showImage(std::shared_ptr<NPC> npc, uint8_t index, const PixelPosition& position, std::string_view image) const;
	void showText(std::shared_ptr<NPC> npc, uint8_t index, const PixelPosition& position, std::string_view text, std::string_view font = {}, std::string_view style = {}) const;
	void showGani(std::shared_ptr<NPC> npc, uint8_t index, const PixelPosition& position, std::string_view animation, uint8_t direction) const;
	void showPoly(std::shared_ptr<NPC> npc, uint8_t index, const std::vector<double>& points) const;
	void changeShowImgColors(std::shared_ptr<NPC> npc, uint8_t index, float red, float green, float blue, float alpha) const;
	void changeShowImgMode(std::shared_ptr<NPC> npc, uint8_t index, uint8_t drawMode) const;
	void changeShowImgPart(std::shared_ptr<NPC> npc, uint8_t index, const ImagePartRectangle& imagePart) const;
	void changeShowImgLayer(std::shared_ptr<NPC> npc, uint8_t index, uint8_t layer) const;
	void changeShowImgZoom(std::shared_ptr<NPC> npc, uint8_t index, float zoom) const;
	void hideImages(std::shared_ptr<NPC> npc, uint8_t index, std::optional<uint8_t> endIndex = std::nullopt) const;

public:
	ScriptSystem scripting;

private:
	void run(TimeoutGenerator::time_delta delta);
	void processDeletedNPCs();
	void processDeletedPlayers();

private:
	void loadClasses();
	void loadDatabaseNPCs();

private:
	BabyDI_INJECT(Server, m_server);

	std::shared_ptr<PlayerNPCServer> m_npcServerPlayer;
	std::string m_ncHost;
	uint16_t m_ncPort = 14900;

	clock::time_point m_frameStartTime;

	TimeoutGenerator m_runTimeout{ 100ms, true };
	TimeoutGenerator m_timedSave{ 5min, true };

	bool m_sleeping = false;
	bool m_firstNPCSave = true;

	std::unordered_map<NPCID, std::weak_ptr<NPC>> m_globalNPCList;
	std::unordered_set<NPCID> m_deletedNPCs;
	std::unordered_map<PlayerID, std::shared_ptr<Player>> m_playerList;
	std::unordered_set<std::shared_ptr<Player>> m_deletedPlayers;
	string_map<std::shared_ptr<ScriptClass>> m_classList;
};

template<class T>
inline std::shared_ptr<T> NPCServer::getPlayer(const PlayerID id) const
{
	auto iter = m_playerList.find(id);
	if (iter == std::end(m_playerList))
		return nullptr;

	if constexpr (std::same_as<T, Player>)
		return iter->second;

	return std::dynamic_pointer_cast<T>(iter->second);
}

template<class T>
inline std::shared_ptr<T> NPCServer::getPlayer(const PlayerID id, int type) const
{
	auto player = getPlayer<T>(id);
	if (player == nullptr || !(player->getType() & type))
		return nullptr;

	return player;
}

template<class T>
inline std::shared_ptr<T> NPCServer::getPlayer(const std::string& account, int type) const
{
	for (const auto& [id, player] : m_playerList)
	{
		// Check if its the type of player we are looking for
		if (!player || !(player->getType() & type))
			continue;

		// Compare account names.
		if (string::equalsi(player->account.name, account))
		{
			if constexpr (std::same_as<T, Player>)
				return player;

			return std::dynamic_pointer_cast<T>(player);
		}
	}

	return nullptr;
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

inline auto& NPCServer::getPlayerList() noexcept
{
	return m_playerList;
}

inline void NPCServer::addEventToControlNPC(ScriptEventType type, ScriptObject source, string::NotInputRangeNotString auto&&... args)
{
	for (auto& [id, npcPtr] : m_globalNPCList)
	{
		if (auto npc = npcPtr.lock(); npc != nullptr && npc->scriptType == NPCTYPE_CONTROL)
			npc->scripting.events.addEvent(type, source, args...);
	}
}

inline void NPCServer::addEventToControlNPC(ScriptEventType type, ScriptObject source, string::InputRangeNotString auto&& range)
{
	for (auto& [id, npcPtr] : m_globalNPCList)
	{
		if (auto npc = npcPtr.lock(); npc != nullptr && npc->scriptType == NPCTYPE_CONTROL)
			npc->scripting.events.addEvent(type, source, std::forward<decltype(range)>(range));
	}
}

inline size_t NPCServer::addEventToLevelNPCsAtPosition(ScriptEventType type, ScriptObject source, std::weak_ptr<Level> level, PixelPosition pos, auto&& arg1, auto&&... args)
{
	auto levelPtr = level.lock();
	if (levelPtr == nullptr)
		return 0;

	uint32_t eventDistance = static_cast<uint32_t>(m_server->getSettings().getInt("eventdistance", 64));
	if (type == ScriptEventType::TRIGGERACTION)
		eventDistance = static_cast<uint32_t>(m_server->getSettings().getInt("triggerdistance", 10));

	size_t count = 0;
	for (const auto& id : levelPtr->findInRangeNPCsByDistance(pos, eventDistance))
	{
		if (auto npc = m_server->getNPC(id); npc != nullptr)
		{
			LocalPixelRectangleArea npcRect = { npc->getLocalPosition(), npc->shape };
			if (positionInRectangle(pos, npcRect))
			{
				++count;
				npc->scripting.events.addEvent(type, source, arg1, args...);
			}
		}
	}
	return count;
}

inline size_t NPCServer::addEventToLevelNPCsAtPosition(ScriptEventType type, ScriptObject source, std::weak_ptr<Level> level, PixelPosition pos, std::ranges::forward_range auto&& range)
{
	auto levelPtr = level.lock();
	if (levelPtr == nullptr)
		return 0;

	uint32_t eventDistance = static_cast<uint32_t>(m_server->getSettings().getInt("eventdistance", 64));
	if (type == ScriptEventType::TRIGGERACTION)
		eventDistance = static_cast<uint32_t>(m_server->getSettings().getInt("triggerdistance", 10));

	size_t count = 0;
	for (const auto& id : levelPtr->findInRangeNPCsByDistance(pos, eventDistance))
	{
		if (auto npc = m_server->getNPC(id); npc != nullptr)
		{
			LocalPixelRectangleArea npcRect = { npc->getLocalPosition(), npc->shape };
			if (positionInRectangle(pos, npcRect))
			{
				++count;
				npc->scripting.events.addEvent(type, source, std::forward<decltype(range)>(range));
			}
		}
	}
	return count;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // NPCSERVER_H
