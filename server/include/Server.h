#ifndef TSERVER_H
#define TSERVER_H

#include <chrono>
#include <climits>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "CLog.h"
#include "CSettings.h"
#include "CSocket.h"
#include "CString.h"
#include "CTranslationManager.h"
#include "FileSystem.h"
#include "IEnums.h"
#include "ServerList.h"
#include "WordFilter.h"

#include "CommandDispatcher.h"

#ifdef UPNP
	#include "UPNP.h"
#endif

#ifdef V8NPCSERVER
	#include "CScriptEngine.h"
#endif

#include "GS2ScriptManager.h"
#include "ScriptClass.h"

// Resources
#include "Animation/GameAni.h"
#include "ResourceManager.h"
#include "UpdatePackage.h"

class Player;
class Level;
class NPC;
class ScriptClass;
class Map;
class Weapon;

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
#define FS_COUNT 7

using AnimationManager = ResourceManager<GameAni, Server*>;
using PackageManager = ResourceManager<UpdatePackage, Server*>;
using TriggerDispatcher = CommandDispatcher<std::string, Player*, std::vector<CString>&>;

class Server : public CSocketStub
{
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

	// Server Management
	int loadConfigFiles();
	void loadSettings();
	void loadAdminSettings();
	void loadAllowedVersions();
	void loadFileSystem();
	void loadServerFlags();
	void loadServerMessage();
	void loadIPBans();
	void loadClasses(bool print = false);
	void loadWeapons(bool print = false);
	void loadMaps(bool print = false);
	void loadMapLevels();
#ifdef V8NPCSERVER
	void loadNpcs(bool print = false);
#endif
	void loadTranslations();
	void loadWordFilter();

	void loadAllFolders();
	void loadFolderConfig();

	void saveServerFlags();
	void saveWeapons();
#ifdef V8NPCSERVER
	void saveNpcs();
	std::vector<std::pair<double, std::string>> calculateNpcStats();
#endif

	void reportScriptException(const ScriptRunError& error);
	void reportScriptException(const std::string& error_message);

	// Get functions.
	const CString& getName() const { return m_name; }
	FileSystem* getFileSystem(int c = 0) { return &(m_filesystem[c]); }
	FileSystem* getAccountsFileSystem() { return &m_filesystemAccounts; }
	CLog& getNPCLog() { return m_npcLog; }
	CLog& getServerLog() { return m_serverLog; }
	CLog& getRCLog() { return m_rcLog; }
	CLog& getScriptLog() { return m_scriptLog; }
	CSettings& getSettings() { return m_settings; }
	CSettings& getAdminSettings() { return m_adminSettings; }
	CSocketManager& getSocketManager() { return m_sockManager; }
	CString getServerPath() const { return m_serverPath; }
	const CString& getServerMessage() const { return m_serverMessage; }
	const CString& getAllowedVersionString() const { return m_allowedVersionString; }
	CTranslationManager& getTranslationManager() { return m_translationManager; }
	WordFilter& getWordFilter() { return m_wordFilter; }
	ServerList& getServerList() { return m_serverlist; }
	AnimationManager& getAnimationManager() { return m_animationManager; }
	PackageManager& getPackageManager() { return m_packageManager; }
	unsigned int getNWTime() const { return m_serverTime; }
	void calculateServerTime();

	std::unordered_map<std::string, std::unique_ptr<ScriptClass>>& getClassList() { return m_classList; }
	std::unordered_map<std::string, std::weak_ptr<NPC>>& getNPCNameList() { return m_npcNameList; }
	std::unordered_map<std::string, CString>& getServerFlags() { return m_serverFlags; }
	std::unordered_map<std::string, std::shared_ptr<Weapon>>& getWeaponList() { return m_weaponList; }
	std::unordered_map<uint16_t, std::shared_ptr<Player>>& getPlayerList() { return m_playerList; }
	std::unordered_map<uint32_t, std::shared_ptr<NPC>>& getNPCList() { return m_npcList; }
	std::vector<std::shared_ptr<Level>>& getLevelList() { return m_levelList; }
	const std::vector<std::shared_ptr<Map>>& getMapList() const { return m_mapList; }
	const std::vector<CString>& getStatusList() const { return m_statusList; }
	const std::vector<CString>& getAllowedVersions() const { return m_allowedVersions; }
	std::unordered_multimap<std::string, std::weak_ptr<Level>>& getGroupLevels() { return m_groupLevels; }

#ifdef V8NPCSERVER
	CScriptEngine* getScriptEngine() { return &m_scriptEngine; }
	int getNCPort() const { return m_ncPort; }
	std::shared_ptr<Player> getNPCServer() const { return m_npcServer; }
#endif

	FileSystem* getFileSystemByType(CString& type);
	CString getFlag(const std::string& pFlagName);
	std::shared_ptr<Level> getLevel(const std::string& pLevel);
	std::shared_ptr<NPC> getNPC(const uint32_t id) const;
	std::shared_ptr<Player> getPlayer(const uint16_t id) const;
	std::shared_ptr<Player> getPlayer(const uint16_t id, int type) const; // = PLTYPE_ANYCLIENT) const;
	std::shared_ptr<Player> getPlayer(const CString& account, int type) const;

#ifdef V8NPCSERVER
	void assignNPCName(std::shared_ptr<NPC> npc, const std::string& name);
	void removeNPCName(std::shared_ptr<NPC> npc);
	std::shared_ptr<NPC> getNPCByName(const std::string& name) const;
	std::shared_ptr<NPC> addServerNpc(int npcId, float pX, float pY, std::shared_ptr<Level> pLevel, bool sendToPlayers = false);

	void handlePM(Player* player, const CString& message);
	void setPMFunction(uint32_t npcId, IScriptFunction* function = nullptr);
#endif
	std::shared_ptr<NPC> addNPC(const CString& pImage, const CString& pScript, float pX, float pY, std::weak_ptr<Level> pLevel, bool pLevelNPC, bool sendToPlayers = false);
	bool deleteNPC(int id, bool eraseFromLevel = true);
	bool deleteNPC(std::shared_ptr<NPC> npc, bool eraseFromLevel = true);
	bool deleteClass(const std::string& className);
	bool hasClass(const std::string& className) const;
	ScriptClass* getClass(const std::string& className) const;
	void updateClass(const std::string& className, const std::string& classCode);
	bool isIpBanned(const CString& ip);
	bool isStaff(const CString& accountName);
	void logToFile(const std::string& fileName, const std::string& message);

	bool deleteFlag(const std::string& pFlagName, bool pSendToPlayers = true);
	bool setFlag(CString pFlag, bool pSendToPlayers = true);
	bool setFlag(const std::string& pFlagName, const CString& pFlagValue, bool pSendToPlayers = true);

	// Admin chat functions
	void sendToRC(const CString& pMessage, std::weak_ptr<Player> pSender = {}) const;
	void sendToNC(const CString& pMessage, std::weak_ptr<Player> pSender = {}) const;

	// Packet sending.
	using PlayerPredicate = std::function<bool(const Player*)>;
	void sendPacketToAll(const CString& packet, const std::set<uint16_t>& exclude = {}) const;
	void sendPacketToLevelArea(const CString& packet, std::weak_ptr<Level> level, const std::set<uint16_t>& exclude = {}, PlayerPredicate sendIf = nullptr) const;
	void sendPacketToLevelArea(const CString& packet, std::weak_ptr<Player> player, const std::set<uint16_t>& exclude = {}, PlayerPredicate sendIf = nullptr) const;
	void sendPacketToOneLevel(const CString& packet, std::weak_ptr<Level> level, const std::set<uint16_t>& exclude = {}) const;
	void sendPacketToType(int who, const CString& pPacket, std::weak_ptr<Player> pPlayer = {}) const;
	void sendPacketToType(int who, const CString& pPacket, Player* pPlayer) const;

	// Specific packet sending
	void sendShootToOneLevel(const std::weak_ptr<Level>& sharedPtr, float x, float y, float z, float angle, float zangle, float strength, const std::string& ani, const std::string& aniArgs) const;

	// Player Management
	uint16_t getFreePlayerId();
	bool addPlayer(std::shared_ptr<Player> player, uint16_t id = USHRT_MAX);
	bool deletePlayer(std::shared_ptr<Player> player);
	void playerLoggedIn(std::shared_ptr<Player> player);
	bool warpPlayerToSafePlace(uint16_t playerId);

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
	void updateClassForPlayers(ScriptClass* pClass);

	/*
	 * GS2 Functionality
	 */
	void compileGS2Script(const std::string& source, GS2ScriptManager::user_callback_type cb);
	void compileGS2Script(NPC* npc, GS2ScriptManager::user_callback_type cb);
	void compileGS2Script(Weapon* weapon, GS2ScriptManager::user_callback_type cb);
	void compileGS2Script(ScriptClass* cls, GS2ScriptManager::user_callback_type cb);

	std::time_t getServerStartTime() const
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

private:
	GS2ScriptManager m_gs2ScriptManager;

	template<typename ScriptObjType>
	void compileScript(ScriptObjType& obj, GS2ScriptManager::user_callback_type& cb);

	void handleGS2Errors(const std::vector<GS2CompilerError>& errors, const std::string& origin);

private:
	bool doTimedEvents();
	void cleanupDeletedPlayers();

	bool m_doRestart;

	FileSystem m_filesystem[FS_COUNT], m_filesystemAccounts;
	CLog m_npcLog, m_rcLog, m_serverLog, m_scriptLog; //("logs/npclog|rclog|serverlog|scriptlog.txt");
	CSettings m_adminSettings, m_settings;
	CSocket m_playerSock;
	CSocketManager m_sockManager;
	CTranslationManager m_translationManager;
	WordFilter m_wordFilter;
	AnimationManager m_animationManager;
	PackageManager m_packageManager;
	CString m_allowedVersionString, m_name, m_serverMessage, m_serverPath;
	CString m_overrideIp, m_overrideLocalIp, m_overridePort, m_overrideInterface;

	std::vector<CString> m_allowedVersions, m_foldersConfig, m_ipBans, m_statusList, m_staffList;

	std::unordered_map<std::string, CString> m_serverFlags;
	std::unordered_map<std::string, std::shared_ptr<Weapon>> m_weaponList;
	std::unordered_map<std::string, std::unique_ptr<ScriptClass>> m_classList;

	std::unordered_map<uint32_t, std::shared_ptr<NPC>> m_npcList;
	std::unordered_map<std::string, std::weak_ptr<NPC>> m_npcNameList;
	std::set<uint32_t> m_freeNpcIds;
	uint32_t m_nextNpcId;

	std::vector<std::shared_ptr<Map>> m_mapList;
	std::vector<std::shared_ptr<Level>> m_levelList;
	std::unordered_multimap<std::string, std::weak_ptr<Level>> m_groupLevels;

	std::unordered_map<uint16_t, std::shared_ptr<Player>> m_playerList;
	std::set<uint16_t> m_freePlayerIds;
	uint16_t m_nextPlayerId;
	std::unordered_set<std::shared_ptr<Player>> m_deletedPlayers;

	ServerList m_serverlist;
	std::chrono::high_resolution_clock::time_point m_lastTimer, m_lastNewWorldTimer, m_last1mTimer, m_last5mTimer, m_last3mTimer;
	std::time_t m_serverStartTime;
	unsigned int m_serverTime;

	// Trigger dispatcher
	TriggerDispatcher m_triggerActionDispatcher;
	void createTriggerCommands(TriggerDispatcher::Builder cmdBuilder);

	std::string m_shootParams;

#ifdef V8NPCSERVER
	CScriptEngine m_scriptEngine;
	int m_ncPort;
	std::shared_ptr<Player> m_npcServer;
	std::shared_ptr<NPC> m_pmHandlerNpc;
#endif

#ifdef UPNP
	UPNP m_upnp;
	std::thread m_upnpThread;
#endif
};

inline std::shared_ptr<NPC> Server::getNPC(const uint32_t id) const
{
	auto iter = m_npcList.find(id);
	if (iter != std::end(m_npcList))
		return iter->second;

	return nullptr;
}

inline bool Server::hasClass(const std::string& className) const
{
	return m_classList.find(className) != m_classList.end();
}

inline ScriptClass* Server::getClass(const std::string& className) const
{
	auto classIter = m_classList.find(className);
	if (classIter != m_classList.end())
		return classIter->second.get();

	return nullptr;
}

#ifdef V8NPCSERVER

inline std::shared_ptr<NPC> Server::getNPCByName(const std::string& name) const
{
	auto npcIter = m_npcNameList.find(name);
	if (npcIter != m_npcNameList.end())
		return npcIter->second.lock();

	return nullptr;
}

#endif

#include "IEnums.h"

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

#endif
