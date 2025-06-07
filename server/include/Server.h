#ifndef SERVER_H
#define SERVER_H

#include <climits>
#include <filesystem>
#include <thread>

#include <CSettings.h>
#include <CSocket.h>
#include <CString.h>
#include <CTranslationManager.h>
#include <IEnums.h>

#undef ERROR
#undef TRANSPARENT

#include <common.h>

#include <Account.h>
#include <FileSystem.h>
#include <ServerList.h>
#include <level/Level.h>
#include <loader/IAccountLoader.h>
#include <loader/INPCLoader.h>
#include <object/Player.h>
#include <object/NPC.h>
#include <misc/UPNP.h>
#include <misc/WordFilter.h>
#include <scripting/GS2ScriptManager.h>
#include <scripting/ScriptClass.h>
#include <utilities/CommandDispatcher.h>
#include <utilities/FlagContainer.h>
#include <utilities/IdGenerator.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

// Resources
#include <animation/GameAni.h>
#include <utilities/ResourceManager.h>
#include <UpdatePackage.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

//class Player;
class PlayerClient;
class PlayerNpcServer;
class Level;
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
	ORIGINAL,

	// 2.x/3.x
	CLASSIC,

	// 4.x to 5.007
	NEWMAIN,

	// 5.1 and up
	MODERN
};

// Player ids 0 and 1 break things.
// NPC ids under 1000 can't be deleted, so start there.
// Player ids 16000 and up is used for players on other servers and "IRC"-channels.
// The players from other servers should be unique lists for each player as they are fetched depending on
// what the player chooses to see (buddies, "global guilds" tab, "other servers" tab)
constexpr PlayerID PLAYERID_INIT = 3;
constexpr NPCID NPCID_INIT = 1000;
constexpr NPCID NPCID_DATABASE_START = 10000;

using AnimationManager = ResourceManager<GameAni, Server*>;
using PackageManager = ResourceManager<UpdatePackage, Server*>;
using TriggerDispatcher = CommandDispatcher<std::string, Player*, std::vector<CString>&>;

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
	~Server();
	void operator()();
	void cleanup();
	void restart();
	bool running;

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
	void loadTranslations();
	void loadWordFilter();
	int loadServerObjects();
	void loadServerFlags();
	void loadWeapons(bool print = false);
	void loadMaps(bool print = false);
	void loadMapLevels();

	// Folder Configuration
	void loadAllFolders();
	void loadFolderConfig();

	// NPC-Server
	void loadNpcServer();

	// Save Functions
	void saveServerFlags();
	void saveWeapons();

	void reportScriptException(const std::string& error_message);

	// Get functions.
	const CString& getName() const { return m_name; }
	FileSystem* getFileSystem(int c = 0) { return &(m_filesystem[c]); }
	FileSystem* getAccountsFileSystem() { return &m_filesystemAccounts; }
	CSettings& getSettings() { return m_settings; }
	CSettings& getAdminSettings() { return m_adminSettings; }
	CSocketManager& getSocketManager() { return m_sockManager; }
	const CString& getServerMessage() const { return m_serverMessage; }
	const CString& getAllowedVersionString() const { return m_allowedVersionString; }
	CTranslationManager& getTranslationManager() { return m_translationManager; }
	WordFilter& getWordFilter() { return m_wordFilter; }
	ServerList& getServerList() { return m_serverlist; }
	AnimationManager& getAnimationManager() { return m_animationManager; }
	PackageManager& getPackageManager() { return m_packageManager; }
	unsigned int getNWTime() const { return m_serverTime; }
	void calculateServerTime();

	//std::unordered_map<std::string, std::unique_ptr<ScriptClass>>& getClassList() { return m_classList; }
	//std::unordered_map<std::string, std::weak_ptr<NPC>>& getNPCNameList() { return m_npcNameList; }
	auto& getWeaponList() { return m_weaponList; }
	auto& getPlayerList() { return m_playerList; }
	auto& getNPCList() { return m_npcList; }
	auto& getLevelList() { return m_levelList; }
	const auto& getMapList() const { return m_mapList; }
	const auto& getStatusList() const { return m_statusList; }
	const auto& getAllowedVersions() const { return m_allowedVersions; }
	auto& getGroupLevels() { return m_groupLevels; }

	IAccountLoader& getAccountLoader() { return *m_accountLoader; }
	INPCLoader& getNPCLoader() { return *m_npcLoader; }

	FileSystem* getFileSystemByType(CString& type);
	std::string getFlag(std::string_view flagName) const;
	std::shared_ptr<Level> getLevel(const std::string& pLevel);
	std::shared_ptr<NPC> getNPC(const NPCID id) const;

	template<class T = Player> std::shared_ptr<T> getPlayer(const PlayerID id) const;
	template<class T = Player> std::shared_ptr<T> getPlayer(const PlayerID id, int type) const;
	template<class T = Player> std::shared_ptr<T> getPlayer(const CString& account, int type) const;

/*
	bool deleteClass(const std::string& className);
	bool hasClass(const std::string& className) const;
	ScriptClass* getClass(const std::string& className) const;
	void updateClass(const std::string& className, const std::string& classCode);
*/
	std::shared_ptr<NPC> addNPC(std::string_view image, std::string_view script, float x, float y, std::weak_ptr<Level> level, NPCType type, bool sendToPlayers = false);
	std::shared_ptr<NPC> addNPC(NPCPtr npc, bool sendToPlayers = false);
	bool deleteNPC(int id, bool eraseFromLevel = true);
	bool deleteNPC(std::shared_ptr<NPC> npc, bool eraseFromLevel = true);
	bool isIpBanned(const CString& ip);
	bool isStaff(const CString& accountName);
	void logToFile(const std::string& fileName, const std::string& message) const;

	bool deleteFlag(const std::string& pFlagName, bool pSendToPlayers = true);
	bool setFlag(CString pFlag, bool pSendToPlayers = true);
	bool setFlag(const std::string& pFlagName, const CString& pFlagValue, bool pSendToPlayers = true);

	// Admin chat functions
	void sendToRC(const CString& pMessage, std::weak_ptr<Player> pSender = {}) const;
	void sendToNC(const CString& pMessage, std::weak_ptr<Player> pSender = {}) const;

	// Packet sending.
	using PlayerPredicate = std::function<bool(const Player*)>;
	void sendPacketToAll(const CString& packet, const std::set<PlayerID>& exclude = {}) const;
	void sendPacketToLevelArea(const CString& packet, std::weak_ptr<Level> level, const std::set<PlayerID>& exclude = {}, PlayerPredicate sendIf = nullptr) const;
	void sendPacketToLevelArea(const CString& packet, std::weak_ptr<PlayerClient> player, const std::set<PlayerID>& exclude = {}, PlayerPredicate sendIf = nullptr) const;
	void sendPacketToLevelArea(const CString& packet, std::weak_ptr<PlayerClient> player, std::weak_ptr<Level> source_level, const std::set<PlayerID>& exclude = {}, PlayerPredicate sendIf = nullptr) const;
	void sendPacketToLevelOnlyGmapArea(const CString& packet, std::weak_ptr<Level> level, const std::set<PlayerID>& exclude = {}, PlayerPredicate sendIf = nullptr) const;
	void sendPacketToLevelOnlyGmapArea(const CString& packet, std::weak_ptr<PlayerClient> player, const std::set<PlayerID>& exclude = {}, PlayerPredicate sendIf = nullptr) const;
	void sendPacketToOneLevel(const CString& packet, std::weak_ptr<Level> level, const std::set<PlayerID>& exclude = {}) const;
	void sendPacketToType(int who, const CString& pPacket, std::weak_ptr<Player> pPlayer = {}) const;
	void sendPacketToType(int who, const CString& pPacket, Player* pPlayer) const;

	// Specific packet sending
	void sendShootToOneLevel(const std::weak_ptr<Level>& sharedPtr, float x, float y, float z, float angle, float zangle, float strength, const std::string& ani, const std::string& aniArgs) const;

	// Player Management
	bool addPlayer(PlayerPtr player, PlayerID id = USHRT_MAX);
	bool deletePlayer(PlayerPtr player);
	bool swapPlayer(PlayerPtr old_player, PlayerPtr new_player);
	void playerLoggedIn(PlayerPtr player);
	bool warpPlayerToSafePlace(PlayerID playerId);

	// Translation Management
	bool TS_Load(const CString& pLanguage, const CString& pFileName);
	CString TS_Translate(const CString& pLanguage, const CString& pKey);
	void TS_Reload();
	void TS_Save();

	// Weapon Management
	std::shared_ptr<Weapon> getWeapon(const std::string& name);
	bool NC_AddWeapon(std::shared_ptr<Weapon> pWeaponObj);
	bool NC_DelWeapon(const std::string& pWeaponName);
	void updateWeaponForPlayers(std::shared_ptr<Weapon> pWeapon);
	//void updateClassForPlayers(ScriptClass* pClass);

	// NPC-Server Management
	bool isNpcServerEnabled() const { return m_playerList.find(NPCServerPlayerID) != m_playerList.end(); }
	std::shared_ptr<NPCServer> getNpcServer() const { return m_npcServer; }

	void queueNPCEvent(LevelPtr level, ScriptEventType type, ScriptObjectSource source)
	{
		if (level == nullptr) return;
		for (auto& npcid : level->getNPCs())
		{
			if (auto npc = getNPC(npcid); npc)
				npc->scripting.events.addEvent(type, source);
		}
	}

	template<class ...Args>
	void queueNPCEvent(LevelPtr level, ScriptEventType type, ScriptObjectSource source, Args... args)
	{
		if (level == nullptr) return;
		for (auto& npcid : level->getNPCs())
		{
			if (auto npc = getNPC(npcid); npc)
				npc->scripting.events.addEvent(type, source, std::forward<Args>(args)...);
		}
	}

	std::chrono::system_clock::time_point getServerStartTime() const
	{
		return m_serverStartTime;
	}

	TriggerDispatcher getTriggerDispatcher() const
	{
		return m_triggerActionDispatcher;
	}

	void setShootParams(const std::string& params)
	{
		m_shootParams = params;
	}

	const std::string& getShootParams() const
	{
		return m_shootParams;
	}

public:
	ServerGeneration Generation{ ServerGeneration::CLASSIC };
	FlagContainer Flags;
	GameVariableStore Variables;

private:
	bool doTimedEvents();
	void cleanupDeletedPlayers();

	bool m_doRestart;

	FileSystem m_filesystem[FS_COUNT], m_filesystemAccounts;
	CSettings m_adminSettings, m_settings;
	CSocket m_playerSock;
	CSocketManager m_sockManager;
	CTranslationManager m_translationManager;
	WordFilter m_wordFilter;
	AnimationManager m_animationManager;
	PackageManager m_packageManager;
	CString m_allowedVersionString, m_name, m_serverMessage;
	CString m_overrideIp, m_overrideLocalIp, m_overridePort, m_overrideInterface;

	std::vector<CString> m_allowedVersions, m_foldersConfig, m_ipBans, m_statusList, m_staffList;

	std::unordered_map<std::string, std::shared_ptr<Weapon>> m_weaponList;
	std::unordered_map<NPCID, std::shared_ptr<NPC>> m_npcList;
	IdGenerator<NPCID> m_npcIdGenerator{ NPCID_INIT };

	std::vector<std::shared_ptr<Map>> m_mapList;
	std::unordered_map<std::string, std::shared_ptr<Level>, string::string_hash, string::string_hash_equal> m_levelList;
	std::unordered_multimap<std::string, std::weak_ptr<Level>> m_groupLevels;

	std::unordered_map<PlayerID, std::shared_ptr<Player>> m_playerList;
	std::unordered_set<std::shared_ptr<Player>> m_deletedPlayers;
	IdGenerator<PlayerID> m_playerIdGenerator{ PLAYERID_INIT };

	ServerList m_serverlist;
	std::chrono::high_resolution_clock::time_point m_lastTimer, m_lastNpcServerTimer, m_lastNewWorldTimer, m_last1mTimer, m_last5mTimer, m_last3mTimer;
	std::chrono::system_clock::time_point m_serverStartTime;
	unsigned int m_serverTime;

	// Trigger dispatcher
	TriggerDispatcher m_triggerActionDispatcher;
	void createTriggerCommands(TriggerDispatcher::Builder cmdBuilder);

	std::string m_shootParams;

	std::unique_ptr<IAccountLoader> m_accountLoader;
	std::unique_ptr<INPCLoader> m_npcLoader;

	std::shared_ptr<NPCServer> m_npcServer;

	UPNP m_upnp;
	std::thread m_upnpThread;
};

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
		if (string::comparei(player->account.name, account.toStringView()) == 0)
		{
			if constexpr (std::same_as<T, Player>)
				return player;

			return std::dynamic_pointer_cast<T>(player);
		}
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SERVER_H
