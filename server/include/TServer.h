#ifndef TSERVER_H
#define TSERVER_H

#include <climits>
#include <chrono>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>
#include <set>
#include <string>
#include <thread>
#include <unordered_set>

#include "IEnums.h"
#include "CString.h"
#include "CLog.h"
#include "CFileSystem.h"
#include "CSettings.h"
#include "CSocket.h"
#include "CTranslationManager.h"
#include "CWordFilter.h"
#include "TServerList.h"

#include "CommandDispatcher.h"

#ifdef UPNP
#include "CUPNP.h"
#endif

#ifdef V8NPCSERVER
#include "CScriptEngine.h"
#endif

#include "GS2ScriptManager.h"
#include "TScriptClass.h"

// Resources
#include "ResourceManager.h"
#include "Animation/TGameAni.h"
#include "TUpdatePackage.h"

class TPlayer;
class TLevel;
class TNPC;
class TScriptClass;
class TMap;
class TWeapon;

enum // Socket Type
{
	SOCK_PLAYER = 0,
	SOCK_SERVER = 1,
};

enum
{
	FS_ALL			= 0,
	FS_FILE			= 1,
	FS_LEVEL		= 2,
	FS_HEAD			= 3,
	FS_BODY			= 4,
	FS_SWORD		= 5,
	FS_SHIELD		= 6,
};
#define FS_COUNT	7

using AnimationManager = ResourceManager<TGameAni, TServer *>;
using PackageManager = ResourceManager<TUpdatePackage, TServer *>;
using TriggerDispatcher = CommandDispatcher<std::string, TPlayer *, std::vector<CString>&>;

class TServer : public CSocketStub
{
	public:
		// Required by CSocketStub.
		bool onRecv();
		bool onSend()				{ return true; }
		bool onRegister()			{ return true; }
		void onUnregister()			{ return; }
		SOCKET getSocketHandle()	{ return playerSock.getHandle(); }
		bool canRecv()				{ return true; }
		bool canSend()				{ return false; }

		TServer(const CString& pName);
		~TServer();
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
		const CString& getName()						{ return name; }
		CFileSystem* getFileSystem(int c = 0)			{ return &(filesystem[c]); }
		CFileSystem* getAccountsFileSystem()			{ return &filesystem_accounts; }
		CLog& getNPCLog()								{ return npclog; }
		CLog& getServerLog()							{ return serverlog; }
		CLog& getRCLog()								{ return rclog; }
		CLog& getScriptLog()							{ return scriptlog; }
		CSettings* getSettings()						{ return &settings; }
		CSettings* getAdminSettings()					{ return &adminsettings; }
		CSocketManager* getSocketManager()				{ return &sockManager; }
		CString getServerPath()							{ return serverpath; }
		CString* getServerMessage()						{ return &servermessage; }
		CString* getAllowedVersionString()				{ return &allowedVersionString; }
		CTranslationManager* getTranslationManager()	{ return &mTranslationManager; }
		CWordFilter* getWordFilter()					{ return &wordFilter; }
		TServerList* getServerList()					{ return &serverlist; }
		AnimationManager& getAnimationManager()			{ return animationManager; }
		PackageManager& getPackageManager()				{ return packageManager; }
		unsigned int getNWTime() const					{ return serverTime; }
		void calculateServerTime();

		std::unordered_map<std::string, std::unique_ptr<TScriptClass>>& getClassList()	{ return classList; }
		std::unordered_map<std::string, TNPC *>* getNPCNameList()		{ return &npcNameList; }
		std::unordered_map<std::string, CString>* getServerFlags()		{ return &mServerFlags; }
		std::map<CString, TWeapon *>* getWeaponList()	{ return &weaponList; }
		std::vector<TPlayer *>* getPlayerList()			{ return &playerList; }
		std::vector<TNPC *>* getNPCList()				{ return &npcList; }
		std::vector<TLevel *>* getLevelList()			{ return &levelList; }
		const std::vector<std::unique_ptr<TMap>>& getMapList() const { return mapList; }
		const std::vector<CString>& getStatusList() const		{ return statusList; }
		const std::vector<CString>& getAllowedVersions() const	{ return allowedVersions; }
		std::map<CString, std::map<CString, TLevel*> >* getGroupLevels()	{ return &groupLevels; }

#ifdef V8NPCSERVER
		CScriptEngine * getScriptEngine() { return &mScriptEngine; }
		int getNCPort() const { return mNCPort; }
		TPlayer * getNPCServer() const { return mNpcServer; }
#endif

		CFileSystem* getFileSystemByType(CString& type);
		CString getFlag(const std::string& pFlagName);
		TLevel* getLevel(const CString& pLevel);
		TNPC* getNPC(unsigned int id) const;
		TPlayer* getPlayer(unsigned short id) const;
		TPlayer* getPlayer(unsigned short id, int type) const; // = PLTYPE_ANYCLIENT) const;
		TPlayer* getPlayer(const CString& account, int type) const;

#ifdef V8NPCSERVER
		void assignNPCName(TNPC *npc, const std::string& name);
		void removeNPCName(TNPC *npc);
		TNPC* getNPCByName(const std::string& name) const;
		TNPC* addServerNpc(int npcId, float pX, float pY, TLevel *pLevel, bool sendToPlayers = false);

		void handlePM(TPlayer *player, const CString& message);
		void setPMFunction(TNPC *npc, IScriptFunction *function = nullptr);
#endif
		TNPC* addNPC(const CString& pImage, const CString& pScript, float pX, float pY, TLevel* pLevel, bool pLevelNPC, bool sendToPlayers = false);
		bool deleteNPC(TNPC* npc, bool eraseFromLevel = true);
		bool deleteClass(const std::string& className);
		bool hasClass(const std::string& className) const;
		TScriptClass * getClass(const std::string& className) const;
		void updateClass(const std::string& className, const std::string& classCode);
		bool isIpBanned(const CString& ip);
		bool isStaff(const CString& accountName);
		void logToFile(const std::string& fileName, const std::string& message);

		bool deleteFlag(const std::string& pFlagName, bool pSendToPlayers = true);
		bool setFlag(CString pFlag, bool pSendToPlayers = true);
		bool setFlag(const std::string& pFlagName, const CString& pFlagValue, bool pSendToPlayers = true);

		// Admin chat functions
		void sendToRC(const CString& pMessage, TPlayer *pSender = nullptr) const;
		void sendToNC(const CString& pMessage, TPlayer *pSender = nullptr) const;

		// Packet sending.
		using PlayerPredicate = std::function<bool(const TPlayer *)>;

		void sendPacketToAll(char packetId, CString pPacket, TPlayer *pSender) const;
		void sendPacketToLevel(char packetId, CString pPacket, TLevel* pLevel, TPlayer* pPlayer = 0) const;
		void sendPacketToLevel(char packetId, CString pPacket, TMap* pMap, TLevel* pLevel, TPlayer* pPlayer = 0, bool onlyGmap = false) const;
		void sendPacketToLevel(char packetId, CString pPacket, TMap* pMap, TPlayer* pPlayer, bool sendToSelf = false, bool onlyGmap = false) const;
		void sendPacketToLevel(PlayerPredicate predicate, char packetId, CString pPacket, TMap* pMap, TPlayer* pPlayer, bool sendToSelf = false, bool onlyGmap = false) const;
		void sendPacketTo(char packetId, int who, CString pPacket, TPlayer* pPlayer = 0) const;

		// Player Management
		unsigned int getFreePlayerId();
		bool addPlayer(TPlayer *player, unsigned int id = UINT_MAX);
		bool deletePlayer(TPlayer* player);
		void playerLoggedIn(TPlayer *player);

		// Translation Management
		bool TS_Load(const CString& pLanguage, const CString& pFileName);
		CString TS_Translate(const CString& pLanguage, const CString& pKey);
		void TS_Reload();
		void TS_Save();

		// Weapon Management
		TWeapon *getWeapon(const CString& name);
		bool NC_AddWeapon(TWeapon *pWeaponObj);
		bool NC_DelWeapon(const CString& pWeaponName);
		void updateWeaponForPlayers(TWeapon *pWeapon);
		void updateClassForPlayers(TScriptClass *pClass);

		/*
		 * GS2 Functionality
		 */
		void compileGS2Script(const std::string& source, GS2ScriptManager::user_callback_type cb);
		void compileGS2Script(TNPC *npc, GS2ScriptManager::user_callback_type cb);
		void compileGS2Script(TWeapon *weapon, GS2ScriptManager::user_callback_type cb);
		void compileGS2Script(TScriptClass *cls, GS2ScriptManager::user_callback_type cb);

		std::time_t getServerStartTime() const {
			return serverStartTime;
		}

		TriggerDispatcher getTriggerDispatcher() const {
			return triggerActionDispatcher;
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

		CFileSystem filesystem[FS_COUNT], filesystem_accounts;
		CLog npclog, rclog, serverlog, scriptlog; //("logs/npclog|rclog|serverlog|scriptlog.txt");
		CSettings adminsettings, settings;
		CSocket playerSock;
		CSocketManager sockManager;
		CString allowedVersionString, name, servermessage, serverpath;
		CTranslationManager mTranslationManager;
		CWordFilter wordFilter;
		CString overrideIP, overrideLocalIP, overridePort, overrideInterface;
		AnimationManager animationManager;
		PackageManager packageManager;

		std::unordered_map<std::string, CString> mServerFlags;
		std::map<CString, TWeapon *> weaponList;
		std::map<CString, std::map<CString, TLevel*> > groupLevels;
		std::unordered_map<std::string, std::unique_ptr<TScriptClass>> classList;
		std::unordered_map<std::string, TNPC *> npcNameList;
		std::vector<CString> allowedVersions, foldersConfig, ipBans, statusList, staffList;
		std::vector<TLevel *> levelList;
		std::vector<std::unique_ptr<TMap>> mapList;
		std::vector<TNPC *> npcIds, npcList;
		std::vector<TPlayer *> playerIds, playerList;

		std::set<TPlayer *> deletedPlayers;

		TServerList serverlist;
		std::chrono::high_resolution_clock::time_point lastTimer, lastNWTimer, last1mTimer, last5mTimer, last3mTimer;
		std::time_t serverStartTime;
		unsigned int serverTime;

		// Trigger dispatcher
		TriggerDispatcher triggerActionDispatcher;
		void createTriggerCommands(TriggerDispatcher::Builder cmdBuilder);

#ifdef V8NPCSERVER
		CScriptEngine mScriptEngine;
		int mNCPort;
		TPlayer *mNpcServer;
		TNPC *mPmHandlerNpc;
#endif

#ifdef UPNP
		CUPNP upnp;
		std::thread upnp_thread;
#endif
};

inline TNPC * TServer::getNPC(const unsigned int id) const
{
	if (id >= npcIds.size())
		return nullptr;

	return npcIds[id];
}

inline bool TServer::hasClass(const std::string& className) const
{
	return classList.find(className) != classList.end();
}

inline TScriptClass * TServer::getClass(const std::string& className) const
{
	auto classIter = classList.find(className);
	if (classIter != classList.end())
		return classIter->second.get();

	return nullptr;
}

#ifdef V8NPCSERVER

inline TNPC * TServer::getNPCByName(const std::string& name) const
{
	auto npcIter = npcNameList.find(name);
	if (npcIter != npcNameList.end())
		return npcIter->second;

	return nullptr;
}

#endif

#include "IEnums.h"

inline void TServer::sendToRC(const CString& pMessage, TPlayer *pSender) const
{
	int len = pMessage.find("\n");
	if (len == -1)
		len = pMessage.length();

	sendPacketTo(PLO_RC_CHAT, PLTYPE_ANYRC, CString() << pMessage.subString(0, len), pSender);
}

inline void TServer::sendToNC(const CString& pMessage, TPlayer *pSender) const
{
	int len = pMessage.find("\n");
	if (len == -1)
		len = pMessage.length();

	sendPacketTo(PLO_RC_CHAT, PLTYPE_ANYNC, CString() << pMessage.subString(0, len), pSender);
}

#endif
