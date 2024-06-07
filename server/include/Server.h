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

#include "FileSystem.h"
#include "CLog.h"
#include "CSettings.h"
#include "CSocket.h"
#include "CString.h"
#include "CTranslationManager.h"
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
	FS_ALL    = 0,
	FS_FILE   = 1,
	FS_LEVEL  = 2,
	FS_HEAD   = 3,
	FS_BODY   = 4,
	FS_SWORD  = 5,
	FS_SHIELD = 6,
};
#define FS_COUNT 7

using AnimationManager  = ResourceManager<GameAni, Server*>;
using PackageManager    = ResourceManager<UpdatePackage, Server*>;
using TriggerDispatcher = CommandDispatcher<std::string, Player*, std::vector<CString>&>;

class Server : public CSocketStub
{
public:
	// Required by CSocketStub.
	bool onRecv();
	bool onSend() { return true; }
	bool onRegister() { return true; }
	void onUnregister() { return; }
	SOCKET getSocketHandle() { return playerSock.getHandle(); }
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
	const CString& getName() const { return name; }
	FileSystem* getFileSystem(int c = 0) { return &(filesystem[c]); }
	FileSystem* getAccountsFileSystem() { return &filesystem_accounts; }
	CLog& getNPCLog() { return npclog; }
	CLog& getServerLog() { return serverlog; }
	CLog& getRCLog() { return rclog; }
	CLog& getScriptLog() { return scriptlog; }
	CSettings& getSettings() { return settings; }
	CSettings& getAdminSettings() { return adminsettings; }
	CSocketManager& getSocketManager() { return sockManager; }
	CString getServerPath() const { return serverpath; }
	const CString& getServerMessage() const { return servermessage; }
	const CString& getAllowedVersionString() const { return allowedVersionString; }
	CTranslationManager& getTranslationManager() { return mTranslationManager; }
	WordFilter& getWordFilter() { return wordFilter; }
	ServerList& getServerList() { return serverlist; }
	AnimationManager& getAnimationManager() { return animationManager; }
	PackageManager& getPackageManager() { return packageManager; }
	unsigned int getNWTime() const { return serverTime; }
	void calculateServerTime();

	std::unordered_map<std::string, std::unique_ptr<ScriptClass>>& getClassList() { return classList; }
	std::unordered_map<std::string, std::weak_ptr<NPC>>& getNPCNameList() { return npcNameList; }
	std::unordered_map<std::string, CString>& getServerFlags() { return mServerFlags; }
	std::unordered_map<std::string, std::shared_ptr<Weapon>>& getWeaponList() { return weaponList; }
	std::unordered_map<uint16_t, std::shared_ptr<Player>>& getPlayerList() { return playerList; }
	std::unordered_map<uint32_t, std::shared_ptr<NPC>>& getNPCList() { return npcList; }
	std::vector<std::shared_ptr<Level>>& getLevelList() { return levelList; }
	const std::vector<std::shared_ptr<Map>>& getMapList() const { return mapList; }
	const std::vector<CString>& getStatusList() const { return statusList; }
	const std::vector<CString>& getAllowedVersions() const { return allowedVersions; }
	std::unordered_multimap<std::string, std::weak_ptr<Level>>& getGroupLevels() { return groupLevels; }

#ifdef V8NPCSERVER
	CScriptEngine* getScriptEngine() { return &mScriptEngine; }
	int getNCPort() const { return mNCPort; }
	std::shared_ptr<Player> getNPCServer() const { return mNpcServer; }
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
		return serverStartTime;
	}

	TriggerDispatcher getTriggerDispatcher() const
	{
		return triggerActionDispatcher;
	}

	void setShootParams(const std::string& params)
	{
		shootParams = params;
	}

	const std::string& getShootParams() const
	{
		return shootParams;
	}

private:
	GS2ScriptManager gs2ScriptManager;

	template<typename ScriptObjType>
	void compileScript(ScriptObjType& obj, GS2ScriptManager::user_callback_type& cb);

	void handleGS2Errors(const std::vector<GS2CompilerError>& errors, const std::string& origin);

private:
	bool doTimedEvents();
	void cleanupDeletedPlayers();

	bool doRestart;

	FileSystem filesystem[FS_COUNT], filesystem_accounts;
	CLog npclog, rclog, serverlog, scriptlog; //("logs/npclog|rclog|serverlog|scriptlog.txt");
	CSettings adminsettings, settings;
	CSocket playerSock;
	CSocketManager sockManager;
	CTranslationManager mTranslationManager;
	WordFilter wordFilter;
	AnimationManager animationManager;
	PackageManager packageManager;
	CString allowedVersionString, name, servermessage, serverpath;
	CString overrideIP, overrideLocalIP, overridePort, overrideInterface;

	std::vector<CString> allowedVersions, foldersConfig, ipBans, statusList, staffList;

	std::unordered_map<std::string, CString> mServerFlags;
	std::unordered_map<std::string, std::shared_ptr<Weapon>> weaponList;
	std::unordered_map<std::string, std::unique_ptr<ScriptClass>> classList;

	std::unordered_map<uint32_t, std::shared_ptr<NPC>> npcList;
	std::unordered_map<std::string, std::weak_ptr<NPC>> npcNameList;
	std::set<uint32_t> freeNpcIds;
	uint32_t nextNpcId;

	std::vector<std::shared_ptr<Map>> mapList;
	std::vector<std::shared_ptr<Level>> levelList;
	std::unordered_multimap<std::string, std::weak_ptr<Level>> groupLevels;

	std::unordered_map<uint16_t, std::shared_ptr<Player>> playerList;
	std::set<uint16_t> freePlayerIds;
	uint16_t nextPlayerId;
	std::unordered_set<std::shared_ptr<Player>> deletedPlayers;

	ServerList serverlist;
	std::chrono::high_resolution_clock::time_point lastTimer, lastNWTimer, last1mTimer, last5mTimer, last3mTimer;
	std::time_t serverStartTime;
	unsigned int serverTime;

	// Trigger dispatcher
	TriggerDispatcher triggerActionDispatcher;
	void createTriggerCommands(TriggerDispatcher::Builder cmdBuilder);

	std::string shootParams;

#ifdef V8NPCSERVER
	CScriptEngine mScriptEngine;
	int mNCPort;
	std::shared_ptr<Player> mNpcServer;
	std::shared_ptr<NPC> mPmHandlerNpc;
#endif

#ifdef UPNP
	UPNP upnp;
	std::thread upnp_thread;
#endif
};

inline std::shared_ptr<NPC> Server::getNPC(const uint32_t id) const
{
	auto iter = npcList.find(id);
	if (iter != std::end(npcList))
		return iter->second;

	return nullptr;
}

inline bool Server::hasClass(const std::string& className) const
{
	return classList.find(className) != classList.end();
}

inline ScriptClass* Server::getClass(const std::string& className) const
{
	auto classIter = classList.find(className);
	if (classIter != classList.end())
		return classIter->second.get();

	return nullptr;
}

#ifdef V8NPCSERVER

inline std::shared_ptr<NPC> Server::getNPCByName(const std::string& name) const
{
	auto npcIter = npcNameList.find(name);
	if (npcIter != npcNameList.end())
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
