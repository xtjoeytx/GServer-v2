#ifndef TSERVER_H
#define TSERVER_H

#include <vector>
#include <map>
#include "ICommon.h"
#include "CSettings.h"
#include "CSocket.h"
#include "CLog.h"
#include "CFileSystem.h"
#include "CWordFilter.h"
#include "TPlayer.h"
#include "TServerList.h"
#include "TLevel.h"
#include "TMap.h"
#include "TNPC.h"
#include "TWeapon.h"

enum // Socket Type
{
	SOCK_PLAYER = 0,
	SOCK_SERVER = 1,
};

enum
{
	FS_ALL		= 0,
	FS_FILE		= 1,
	FS_LEVEL	= 2,
	FS_HEAD		= 3,
	FS_BODY		= 4,
	FS_SWORD	= 5,
	FS_SHIELD	= 6,
};
#define FS_COUNT	7

//class TPlayer;
//class TLevel;
class TServer : public CSocketStub
{
	public:
		// Required by CSocketStub.
		bool onRecv();
		bool onSend()				{ return true; }
		SOCKET getSocketHandle()	{ return playerSock.getHandle(); }
		bool canRecv()				{ return true; }
		bool canSend()				{ return false; }

		TServer(CString pName);
		~TServer();
		void operator()();
		void cleanup();

		int init();
		bool doMain();

		void loadAllFolders();
		void loadFolderConfig();
		int loadConfigFiles();

		// Get functions.
		CSettings* getSettings()						{ return &settings; }
		CSettings* getAdminSettings()					{ return &adminsettings; }
		std::map<CString, TWeapon *>* getWeaponList()	{ return &weaponList; }
		std::vector<TPlayer*>* getPlayerList()			{ return &playerList; }
		std::vector<TPlayer*>* getPlayerIdList()		{ return &playerIds; }
		std::vector<TNPC*>* getNPCList()				{ return &npcList; }
		std::vector<TNPC*>* getNPCIdList()				{ return &npcIds; }
		std::vector<TLevel*>* getLevelList()			{ return &levelList; }
		std::vector<TMap*>* getMapList()				{ return &mapList; }
		std::vector<CString>* getServerFlags()			{ return &serverFlags; }
		std::vector<CString>* getStatusList()			{ return &statusList; }
		std::vector<CString>* getAllowedVersions()		{ return &allowedVersions; }
		TServerList* getServerList()					{ return &serverlist; }
		CFileSystem* getFileSystem(int c = 0)			{ return &(filesystem[c]); }
		CFileSystem* getAccountsFileSystem()			{ return &filesystem_accounts; }
		CSocketManager* getSocketManager()				{ return &sockManager; }
		CString getServerPath()							{ return serverpath; }
		CLog& getNPCLog()								{ return npclog; }
		CLog& getServerLog()							{ return serverlog; }
		CLog& getRCLog()								{ return rclog; }
		CString* getServerMessage()						{ return &servermessage; }
		CString* getAllowedVersionString()				{ return &allowedVersionString; }
		CWordFilter* getWordFilter()					{ return &wordFilter; }
		unsigned int getNWTime() const;

		TPlayer* getPlayer(const unsigned short id, bool includeRC = true) const;
		TPlayer* getPlayer(const CString& account, bool includeRC = true) const;
		TPlayer* getRC(const unsigned short id, bool includePlayer = false) const;
		TPlayer* getRC(const CString& account, bool includePlayer = false) const;
		TNPC* getNPC(const unsigned int id) const;
		TLevel* getLevel(const CString& pLevel);
		TMap* getMap(const CString& name) const;
		TMap* getMap(const TLevel* pLevel) const;
		CString getFlag(const CString& pName) const;
		CFileSystem* getFileSystemByType(CString& type);

		TNPC* addNPC(const CString& pImage, const CString& pScript, float pX, float pY, TLevel* pLevel, bool pLevelNPC, bool sendToPlayers = false);
		bool deleteNPC(const unsigned int pId, TLevel* pLevel = 0);
		bool deleteNPC(TNPC* npc, TLevel* pLevel = 0);
		bool addFlag(const CString& pFlag);
		bool deleteFlag(const CString& pFlag);
		bool deletePlayer(TPlayer* player);
		bool isIpBanned(const CString& ip);
		
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

		// Weapon Management
		TWeapon *getWeapon(const CString& name);

		bool NC_AddWeapon(TWeapon *pWeaponObj);
		bool NC_DelWeapon(const CString& pWeaponName);
		void NC_UpdateWeapon(TWeapon *pWeapon);

	private:
		bool doTimedEvents();
		void acceptSock(CSocket& pSocket);

		CSettings settings;
		CSettings adminsettings;
		std::map<CString, TWeapon *> weaponList;
		std::vector<TPlayer*> playerIds, playerList;
		std::vector<TNPC*> npcIds, npcList;
		std::vector<TLevel*> levelList;
		std::vector<TMap*> mapList;
		std::vector<CString> serverFlags;
		std::vector<CString> ipBans;
		std::vector<CString> foldersConfig;
		std::vector<CString> statusList;
		std::vector<CString> allowedVersions;
		CSocket playerSock;
		CSocketManager sockManager;
		TServerList serverlist;
		CFileSystem filesystem[FS_COUNT];
		CFileSystem filesystem_accounts;
		CString name;
		CString serverpath;
		CString servermessage;
		CString allowedVersionString;
		CWordFilter wordFilter;

		CLog npclog;//("logs/npclog.txt");
		CLog rclog;//("logs/rclog.txt");
		CLog serverlog;//("logs/serverlog.txt");

		time_t lastTimer, lastNWTimer, last1mTimer, last5mTimer, last3mTimer;

		// NPC-Server Functionality
		TPlayer *mNpcServer;
		int mNCPort;
};

#endif
