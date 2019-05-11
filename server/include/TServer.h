#ifndef TSERVER_H
#define TSERVER_H

#include <vector>
#include <map>
#include <set>
#include <thread>
#include <chrono>
#ifdef V8NPCSERVER
#include <string>
#include <unordered_map>
#include <unordered_set>
#endif

#include "CString.h"

#include "CLog.h"
#include "CFileSystem.h"
#include "CPluginManager.h"
#include "CSettings.h"
#include "CSocket.h"
#include "CTranslationManager.h"
#include "CWordFilter.h"
#include "TServerList.h"

#ifdef UPNP
#include "CUPNP.h"
#endif

#ifdef V8NPCSERVER
#include "CScriptEngine.h"

// TODO(joey): remove below
#include "ScriptAction.h"
#include "ScriptWrapped.h"
#endif

class TPlayer;
class TLevel;
class TNPC;
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

		TServer(CString pName);
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
		void loadWeapons(bool print = false);
		void loadMaps(bool print = false);
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
#endif

		// Get functions.
		const CString& getName()						{ return name; }
		CFileSystem* getFileSystem(int c = 0)			{ return &(filesystem[c]); }
		CFileSystem* getAccountsFileSystem()			{ return &filesystem_accounts; }
		CLog& getNPCLog()								{ return npclog; }
		CLog& getServerLog()							{ return serverlog; }
		CLog& getRCLog()								{ return rclog; }
		CPluginManager& getPluginManager()				{ return mPluginManager; }
		CSettings* getSettings()						{ return &settings; }
		CSettings* getAdminSettings()					{ return &adminsettings; }
		CSocketManager* getSocketManager()				{ return &sockManager; }
		CString getServerPath()							{ return serverpath; }
		CString* getServerMessage()						{ return &servermessage; }
		CString* getAllowedVersionString()				{ return &allowedVersionString; }
		CTranslationManager* getTranslationManager()	{ return &mTranslationManager; }
		CWordFilter* getWordFilter()					{ return &wordFilter; }
		TServerList* getServerList()					{ return &serverlist; }
		unsigned int getNWTime() const;

		std::map<CString, CString>* getServerFlags()	{ return &mServerFlags; }
		std::map<CString, TWeapon *>* getWeaponList()	{ return &weaponList; }
		std::vector<TPlayer *>* getPlayerList()			{ return &playerList; }
		std::vector<TNPC *>* getNPCList()				{ return &npcList; }
		std::vector<TLevel *>* getLevelList()			{ return &levelList; }
		std::vector<TMap *>* getMapList()				{ return &mapList; }
		std::vector<CString>* getStatusList()			{ return &statusList; }
		std::vector<CString>* getAllowedVersions()		{ return &allowedVersions; }
		std::map<CString, std::map<CString, TLevel*> >* getGroupLevels()	{ return &groupLevels; }
		
		CFileSystem* getFileSystemByType(CString& type);
		CString getFlag(const CString& pFlagName);
		TLevel* getLevel(const CString& pLevel);
		TMap* getMap(const CString& name) const;
		TMap* getMap(const TLevel* pLevel) const;
		TNPC* getNPC(const unsigned int id) const;
		TPlayer* getPlayer(const unsigned short id, bool includeRC = true) const;
		TPlayer* getPlayer(const CString& account, bool includeRC = true) const;
		TPlayer* getRC(const unsigned short id, bool includePlayer = false) const;
		TPlayer* getRC(const CString& account, bool includePlayer = false) const;

#ifdef V8NPCSERVER
		TNPC* addServerNpc(int npcId, float pX, float pY, TLevel *pLevel, bool sendToPlayers = false);
#endif
		TNPC* addNPC(const CString& pImage, const CString& pScript, float pX, float pY, TLevel* pLevel, bool pLevelNPC, bool sendToPlayers = false);
		bool deleteNPC(const unsigned int pId, TLevel* pLevel = 0, bool eraseFromLevel = true);
		bool deleteNPC(TNPC* npc, TLevel* pLevel = 0, bool eraseFromLevel = true);
		bool deletePlayer(TPlayer* player);
		bool isIpBanned(const CString& ip);

		bool deleteFlag(const CString& pFlagName, bool pSendToPlayers = true);
		bool setFlag(CString pFlag, bool pSendToPlayers = true);
		bool setFlag(const CString& pFlagName, const CString& pFlagValue, bool pSendToPlayers = true);

		// Admin chat functions
		inline void sendToRC(const CString& pMessage, TPlayer *pPlayer = 0) const;
		inline void sendToNC(const CString& pMessage, TPlayer *pPlayer = 0) const;

		// Packet sending.
		void sendPacketToAll(CString pPacket, TPlayer *pPlayer = 0, bool pNpcServer = false) const;
		void sendPacketToLevel(CString pPacket, TMap* pMap, TLevel* pLevel, TPlayer* pPlayer = 0, bool onlyGmap = false) const;
		void sendPacketToLevel(CString pPacket, TMap* pMap, TPlayer* pPlayer, bool sendToSelf = false, bool onlyGmap = false) const;
		void sendPacketTo(int who, CString pPacket, TPlayer* pPlayer = 0) const;

		// NPC-Server Functionality
		inline bool hasNPCServer()		{ return mNpcServer != 0; }
		inline TPlayer *getNPCServer()	{ return mNpcServer; }
		inline int getNCPort()			{ return mNCPort; }

		void setNPCServer(TPlayer * pNpcServer, int pNCPort = 0);

		// Translation Management
		bool TS_Load(const CString& pLanguage, const CString& pFileName);
		CString TS_Translate(const CString& pLanguage, const CString& pKey);
		void TS_Reload();
		void TS_Save();

		// Weapon Management
		TWeapon *getWeapon(const CString& name);

		bool NC_AddWeapon(TWeapon *pWeaponObj);
		bool NC_DelWeapon(const CString& pWeaponName);
		void NC_UpdateWeapon(TWeapon *pWeapon);
		bool NC_SendMap(TMap *map);
		bool NC_SendLevel(TLevel *level);
	
#ifdef V8NPCSERVER
		inline CScriptEngine * getScriptEngine() {
			return &mScriptEngine;
		}
#endif

	private:
		bool doTimedEvents();
		void acceptSock(CSocket& pSocket);
		void cleanupDeletedPlayers();

		bool doRestart;

		CFileSystem filesystem[FS_COUNT], filesystem_accounts;
		CLog npclog, rclog, serverlog; //("logs/npclog|rclog|serverlog.txt");
		CPluginManager mPluginManager;
		CSettings adminsettings, settings;
		CSocket playerSock;
		CSocketManager sockManager;
		CString allowedVersionString, name, servermessage, serverpath;
		CTranslationManager mTranslationManager;
		CWordFilter wordFilter;
		CString overrideIP, overrideLocalIP, overridePort, overrideInterface;

		std::map<CString, CString> mServerFlags;
		std::map<CString, TWeapon *> weaponList;
		std::map<CString, std::map<CString, TLevel*> > groupLevels;
		std::vector<CString> allowedVersions, foldersConfig, ipBans, statusList;
		std::vector<TLevel *> levelList;
		std::vector<TMap *> mapList;
		std::vector<TNPC *> npcIds, npcList;
		std::vector<TPlayer *> playerIds, playerList;

		std::set<TPlayer *> deletedPlayers;

		TServerList serverlist;
		std::chrono::high_resolution_clock::time_point lastTimer, lastNWTimer, last1mTimer, last5mTimer, last3mTimer;
#ifdef V8NPCSERVER
		std::chrono::high_resolution_clock::time_point lastScriptTimer;
#endif
	
#ifdef UPNP
		CUPNP upnp;
		std::thread upnp_thread;
#endif

		// NPC-Server Functionality
		TPlayer *mNpcServer;
		int mNCPort;
	
#ifdef V8NPCSERVER
		CScriptEngine mScriptEngine;
#endif
};

#include "IEnums.h"

inline void TServer::sendToRC(const CString& pMessage, TPlayer *pPlayer) const
{
	sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << pMessage, pPlayer);
}

inline void TServer::sendToNC(const CString& pMessage, TPlayer *pPlayer) const
{
	sendPacketTo(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << pMessage, pPlayer);
}


#endif
