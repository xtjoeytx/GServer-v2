#ifndef SERVER_H
#define SERVER_H

#include <array>
#include <chrono>
#include <climits>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <string_view>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <CSocket.h>

#include <CSettings.h>
#include <CString.h>
#include <IEnums.h>

#include <Account.h>
#include <ServerList.h>
#include <UpdatePackage.h>
#include <animation/GameAni.h>
#include <filesystem/FileSystem.h>
#include <level/Level.h>
#include <level/LevelItem.h>
#include <level/LevelShoot.h>
#include <loader/IAccountLoader.h>
#include <loader/INPCLoader.h>
#include <misc/UPNP.h>
#include <misc/WordFilter.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommandDispatcher.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/generator/IdGenerator.h>
#include <utilities/generator/TimeoutGenerator.h>
#include <utilities/manager/ResourceManager.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class PlayerClient;
class ScriptClass;
class Map;
class Weapon;
class NPCServer;

enum // Socket Type
{
	SOCK_PLAYER = 0,
	SOCK_SERVER = 1,
};

enum
{
	FS_ALL = 0,
	FS_FILE = 1,
	FS_LEVEL = 2,
	FS_HEAD = 3,
	FS_BODY = 4,
	FS_SWORD = 5,
	FS_SHIELD = 6,
};
constexpr int FS_COUNT = 7;

enum class ServerGeneration
{
	// 1.x
	ORIGINAL = 0,

	// 2.x/3.x
	CLASSIC,

	// 4.x to 5.007
	NEWMAIN,

	// 5.1 and up
	MODERN
};

inline constexpr std::array<std::string_view, 4> ServerGenerationNames =
{
	"original",
	"classic",
	"newmain",
	"modern"
};

using AnimationManager = ResourceManager<GameAni, Server*>;
using PackageManager = ResourceManager<UpdatePackage, Server*>;
using TriggerDispatcher = CommandDispatcher<std::string, Player*, std::vector<std::string>&>;

class Server : public CSocketStub
{
	friend class NPCServer;
	friend class FlatFileNPCLoader;

public:
	// Required by CSocketStub.
	bool onRecv();
	bool onSend() { return true; }
	bool onRegister() { return true; }
	void onUnregister() { return; }
	SOCKET getSocketHandle() { return m_playerSock.getHandle(); }
	bool canRecv() { return true; }
	bool canSend() { return false; }

	Server(const CString& pName);
	virtual ~Server();
	void operator()();
	void cleanup();
	void restart();
	bool running = false;

	int init(const CString& serverip = "", const CString& serverport = "", const CString& localip = "", const CString& serverinterface = "");
	bool doMain();

	// Server Configuration
	int loadConfigFiles();
	void loadSettings();
	void loadAdminSettings();
	void loadAllowedVersions();
	void loadFileSystem();
	void loadServerMessage();
	void loadIPBans();
	void loadTranslations() const;
	void loadWordFilter();
	void loadServerFlags();
	void loadGuilds();
	void loadMaps(bool print = false);
	int loadServerObjects();
	void loadWeapons(bool print = false);
	void loadMapLevels();

	// Folder Configuration
	void loadAllFolders();
	void loadFolderConfig();

	// NPC-Server
	void loadNPCServer();

public:
	// Save Functions
	void saveServerFlags();
	void saveWeapons();
	//void reportScriptException(const std::string& error_message);

public:
	const auto& getName() const { return m_name; }
	const auto& getServerMessage() const { return m_serverMessage; }
	const auto& getAllowedVersionString() const { return m_allowedVersionString; }
	const auto& getNWTime() const { return m_serverTime; }
	auto& getFileSystem() { return m_fsWorld; }
	auto& getFileSystemServer() { return m_fsServer; }
	auto& getAccountLoader() { return *m_accountLoader; }
	auto& getAdminSettings() { return m_adminSettings; }
	auto& getAnimationManager() { return m_animationManager; }
	auto& getGroupLevels() { return m_groupLevels; }
	auto& getLevelList() { return m_levelList; }
	auto& getNPCList() { return m_npcList; }
	auto& getNPCLoader() { return *m_npcLoader; }
	auto& getPackageManager() { return m_packageManager; }
	auto& getPlayerIdGenerator() { return m_playerIdGenerator; }
	auto& getPlayerList() { return m_playerList; }
	auto& getServerList() { return m_serverlist; }
	auto& getSettings() { return m_settings; }
	auto& getSocketManager() { return m_sockManager; }
	auto& getTriggerDispatcher() { return m_triggerActionDispatcher; }
	auto& getWeaponList() { return m_weaponList; }
	auto& getWordFilter() { return m_wordFilter; }
	const auto& getAllowedVersions() const { return m_allowedVersions; }
	const auto& getFrameStartTime() const { return m_frameStartTime; }
	const auto& getFrameStartTimeHighPrecision() const { return m_frameStartTimeHighPrecision; }
	const auto& getMapList() const { return m_mapList; }
	const auto& getServerStartTime() const { return m_serverStartTime; }
	const auto& getStatusList() const { return m_statusList; }

public:
	std::shared_ptr<Level> stubOrGetLevel(std::string_view levelName);
	std::shared_ptr<Level> getLevel(std::string_view levelName);
	std::shared_ptr<Map> findMap(std::string_view mapName) const noexcept;

public:
	LevelItemType rollBushItemDrop() const;
	[[inline]] const auto& getAllowedDeathDrops() const noexcept;

public:
	std::shared_ptr<NPC> getNPC(const NPCID id) const;
	std::shared_ptr<NPC> addNPC(std::string_view image, std::string_view script, float x, float y, std::weak_ptr<Level> level, NPCStorageType storageType, bool sendToPlayers = false, std::string_view type = {});
	std::shared_ptr<NPC> addNPC(NPCPtr npc, bool sendToPlayers = false);
	bool deleteNPC(int id, bool eraseFromLevel = true);
	bool deleteNPC(std::shared_ptr<NPC> npc, bool eraseFromLevel = true);

public:
	template<class T = Player> std::shared_ptr<T> getPlayer(const PlayerID id) const;
	template<class T = Player> std::shared_ptr<T> getPlayer(const PlayerID id, int type) const;
	template<class T = Player> std::shared_ptr<T> getPlayer(const CString& account, int type) const;

	bool addPlayer(PlayerPtr player, PlayerID id = USHRT_MAX);
	bool deletePlayer(PlayerPtr player);
	bool swapPlayer(PlayerPtr old_player, PlayerPtr new_player);
	void recordPlayerLoggedIn(PlayerPtr player);
	bool warpPlayerToSafePlace(PlayerID playerId);

public:
	std::optional<std::string> getFlag(std::string_view flagName) const;
	bool deleteFlag(std::string_view flagName, bool sendToPlayers = true);
	bool setFlag(std::string_view flagPair, bool sendToPlayers = true);
	bool setFlag(std::string_view flagName, std::optional<std::string> flagValue, bool sendToPlayers = true);

public:
	void calculateNWTime();
	bool isIpBanned(const CString& ip);
	bool isStaff(const CString& accountName);

public:
	void hitObjectsAtPoint(const TilePosition& pos, int8_t power, std::weak_ptr<Level> level, PlayerPtr source) const;
	void hitObjectsAtPoint(const TilePosition& pos, int8_t power, std::weak_ptr<Level> level, NPCPtr source) const;
	void hitPlayer(PlayerID playerId, int8_t power, float fromX, float fromY, std::shared_ptr<NPC> source) const;
	void logToFile(const std::string& fileName, const std::string& message) const;
	void sendToRC(const CString& pMessage, std::weak_ptr<Player> pSender = {}) const;
	void sendToNC(const CString& pMessage, std::weak_ptr<Player> pSender = {}) const;
	void sendTriggerAction(PlayerID toPlayerId, NPCID fromNpcId, const LocalPixelPosition& localPosition, std::string_view action, std::string_view params) const;
	void sendTriggerAction(LevelPtr toLevel, NPCID fromNpcId, const PixelPosition& position, std::string_view action, std::string_view params) const;

public:
	using PlayerPredicate = std::function<bool(const Player*)>;
	void sendPacketToAll(const CString& packet, const std::set<PlayerID>& exclude = {}, PlayerPredicate sendIf = nullptr) const;
	void sendPacketToOneLevel(const CString& packet, std::weak_ptr<Level> level, const std::set<PlayerID>& exclude = {}, PlayerPredicate sendIf = nullptr) const;
	void sendPacketToType(int who, const CString& pPacket, std::weak_ptr<Player> pPlayer = {}) const;
	void sendPacketToType(int who, const CString& pPacket, Player* pPlayer) const;
	void sendPacketToLevelAndPastVisitorsAfter(Level* level, time_t modTime, const CString& packet) const;
	void sendPacketToNearby(const CString& packet, const PixelPosition& position, std::shared_ptr<Level> level, const std::set<PlayerID>& exclude = {}, PlayerPredicate sendIf = nullptr) const;

public:
	void sendShootToOneLevel(LevelShoot* shoot, std::shared_ptr<Level> level) const;

public:
	// Weapon Management
	std::shared_ptr<Weapon> getWeapon(std::string_view name);
	bool NC_AddWeapon(std::shared_ptr<Weapon> pWeaponObj);
	bool NC_DelWeapon(std::string_view pWeaponName);
	void updateWeaponForPlayers(Weapon* weapon);
	void updateWeaponForPlayers(std::shared_ptr<Weapon> weapon);
	void updateClassForPlayers(std::shared_ptr<ScriptClass> scriptClass);

public:
	bool hasNPCServer() const { return m_playerList.find(NPCServerPlayerID) != m_playerList.end(); }
	std::shared_ptr<NPCServer> getNPCServer() const { return m_npcServer; }

	void queueNPCEventLocal(LevelPtr level, ScriptEventType type, ScriptObject source, auto&&... args)
	{
		if (!hasNPCServer()) return;
		if (level == nullptr) return;
		for (auto& npcid : level->getLevelNPCs())
		{
			if (auto npc = getNPC(npcid); npc)
				npc->scripting.events.addEvent(type, source, std::forward<decltype(args)>(args)...);
		}
	}

	void queueNPCEvent(LevelPtr level, const PixelPosition& position, ScriptEventType type, ScriptObject source, auto&&... args)
	{
		if (!hasNPCServer()) return;
		if (level == nullptr) return;
		uint32_t eventDistance = static_cast<uint32_t>(m_settings.getInt("eventdistance", 64));
		for (auto& npcid : level->findInRangeNPCsByDistance(position, eventDistance))
		{
			if (auto npc = getNPC(npcid); npc)
				npc->scripting.events.addEvent(type, source, std::forward<decltype(args)>(args)...);
		}
	}

public:
	[[inline]] const std::vector<std::string>& getShootParams() const;
	[[inline]] void setShootParams(std::vector<std::string>&& params);

	void setShootParams(std::ranges::forward_range auto&& params)
		requires std::same_as<std::ranges::range_value_t<decltype(params)>, std::string>
	{
		m_shootParams.clear();
		m_shootParams.assign(std::ranges::begin(params), std::ranges::end(params));
	}

public:
	ServerGeneration Generation{ ServerGeneration::CLASSIC };
	ScriptContainer Scripting;

private:
	bool doTimedEvents(int iterations);

	bool m_doRestart = false;

	fs::FileSystem m_fsWorld, m_fsServer;
	CSettings m_adminSettings, m_settings;
	CSocket m_playerSock;
	CSocketManager m_sockManager;
	WordFilter m_wordFilter;
	AnimationManager m_animationManager;
	PackageManager m_packageManager;
	CString m_allowedVersionString, m_name, m_serverMessage;
	CString m_overrideIp, m_overrideLocalIp, m_overridePort, m_overrideInterface;
	std::vector<CString> m_allowedVersions, m_foldersConfig, m_ipBans, m_statusList, m_staffList;
	std::vector<std::pair<LevelItemType, int>> m_bushDrops;
	std::vector<LevelItemType> m_deathDrops;

	std::unique_ptr<IAccountLoader> m_accountLoader;
	std::unique_ptr<INPCLoader> m_npcLoader;

	std::vector<std::shared_ptr<Map>> m_mapList;
	string_map<std::shared_ptr<Level>> m_levelList;
	std::unordered_multimap<std::string, std::weak_ptr<Level>> m_groupLevels;

	string_map<std::shared_ptr<Weapon>> m_weaponList;
	std::unordered_map<NPCID, std::shared_ptr<NPC>> m_npcList;
	IdGenerator<NPCID> m_npcIdGenerator{ NPCID_GEN_DATABASE_LOCALN };

	std::unordered_map<PlayerID, std::shared_ptr<Player>> m_playerList;
	IdGenerator<PlayerID> m_playerIdGenerator{ PLAYERID_GEN };

	TimeoutGenerator m_timedEvents{ 1s, true };
	TimeoutGenerator m_timedNWTime{ 5s, true };
	TimeoutGenerator m_timedSave{ 1min, true };
	TimeoutGenerator m_timedMaintenance{ 5min, true };
	clock::time_point m_serverStartTime;
	clock::time_point m_frameStartTime;
	precise_clock::time_point m_frameStartTimeHighPrecision;
	uint32_t m_serverTime;

	// Trigger dispatcher
	TriggerDispatcher m_triggerActionDispatcher;
	void createTriggerCommands(TriggerDispatcher::Builder cmdBuilder);

	std::vector<std::string> m_shootParams;

	std::shared_ptr<NPCServer> m_npcServer;
	ServerList m_serverlist;

	std::unique_ptr<UPNP> m_upnp;
	std::thread m_upnpThread;
};

inline const auto& Server::getAllowedDeathDrops() const noexcept
{
	return m_deathDrops;
}

inline std::shared_ptr<NPC> Server::getNPC(const NPCID id) const
{
	auto iter = m_npcList.find(id);
	if (iter != std::end(m_npcList))
		return iter->second;

	return nullptr;
}

inline void Server::sendToRC(const CString& pMessage, std::weak_ptr<Player> pSender) const
{
	int len = pMessage.find("\n");
	if (len == -1)
		len = pMessage.length();

	sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << pMessage.subString(0, len), pSender);
}

inline void Server::sendToNC(const CString& pMessage, std::weak_ptr<Player> pSender) const
{
	int len = pMessage.find("\n");
	if (len == -1)
		len = pMessage.length();

	sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << pMessage.subString(0, len), pSender);
}

template<class T>
inline std::shared_ptr<T> Server::getPlayer(const PlayerID id) const
{
	auto iter = m_playerList.find(id);
	if (iter == std::end(m_playerList))
		return nullptr;

	if constexpr (std::same_as<T, Player>)
		return iter->second;

	return std::dynamic_pointer_cast<T>(iter->second);
}

template<class T>
inline std::shared_ptr<T> Server::getPlayer(const PlayerID id, int type) const
{
	auto player = getPlayer<T>(id);
	if (player == nullptr || !(player->getType() & type))
		return nullptr;

	return player;
}

template<class T>
inline std::shared_ptr<T> Server::getPlayer(const CString& account, int type) const
{
	for (const auto& [id, player] : m_playerList)
	{
		// Check if its the type of player we are looking for
		if (!player || !(player->getType() & type))
			continue;

		// Compare account names.
		if (string::equalsi(player->account.name, account.toStringView()))
		{
			if constexpr (std::same_as<T, Player>)
				return player;

			return std::dynamic_pointer_cast<T>(player);
		}
	}

	return nullptr;
}

inline const std::vector<std::string>& Server::getShootParams() const
{
	return m_shootParams;
}

inline void Server::setShootParams(std::vector<std::string>&& params)
{
	m_shootParams = std::move(params);
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SERVER_H
