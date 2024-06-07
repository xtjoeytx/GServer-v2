#ifndef TSERVERLIST_H
#define TSERVERLIST_H

#include "CFileQueue.h"
#include "CSocket.h"
#include "CString.h"
#include <assert.h>
#include <map>
#include <memory>
#include <time.h>

enum
{
	SVF_HEAD   = 0,
	SVF_BODY   = 1,
	SVF_SWORD  = 2,
	SVF_SHIELD = 3,
	SVF_FILE   = 4,
};

class Player;
class Server;
class ServerList : public CSocketStub
{
public:
	// Required by CSocketStub.
	bool onRecv();
	bool onSend();
	bool onRegister() { return true; }
	void onUnregister();
	SOCKET getSocketHandle() { return sock.getHandle(); }
	bool canRecv();
	bool canSend() { return _fileQueue.canSend(); }

	// Constructor - Deconstructor
	ServerList(Server* server);
	~ServerList();

	bool doTimedEvents();

	// Socket-Control Functions
	bool getConnected() const;
	bool main();
	bool connectServer();
	CSocket& getSocket() { return sock; }
	void sendPacket(CString& pPacket, bool sendNow = false);

	// Send players to the listserver
	void addPlayer(std::shared_ptr<Player> player);
	void deletePlayer(std::shared_ptr<Player> player);
	void sendPlayers();
	void handleText(const CString& data);
	void sendText(const CString& data);
	void sendText(const std::vector<CString>& stringList);
	void sendTextForPlayer(std::shared_ptr<Player> player, const CString& data);

	void sendLoginPacketForPlayer(std::shared_ptr<Player> player, const CString& password, const CString& identity);

	const std::map<std::string, int>& getServerList() { return serverListCount; }
	const std::string& getLocalIP() const { return _serverLocalIp; }
	const std::string& getServerIP() const { return _serverRemoteIp; }

	// Send New Server-Info
	void sendServerHQ();
	void sendVersionConfig();

	// Incoming message parsing functions
	static bool created;
	static void createFunctions();

	void msgSVI_NULL(CString& pPacket);
	void msgSVI_VERIACC(CString& pPacket);
	void msgSVI_VERIGUILD(CString& pPacket);
	void msgSVI_FILESTART(CString& pPacket);
	void msgSVI_FILEEND(CString& pPacket);
	void msgSVI_FILEDATA(CString& pPacket);
	void msgSVI_VERSIONOLD(CString& pPacket);
	void msgSVI_VERSIONCURRENT(CString& pPacket);
	void msgSVI_PROFILE(CString& pPacket);
	void msgSVI_ERRMSG(CString& pPacket);
	//void msgSVI_NULL4(CString& pPacket);
	//void msgSVI_NULL5(CString& pPacket);
	void msgSVI_VERIACC2(CString& pPacket);
	void msgSVI_FILESTART2(CString& pPacket);
	void msgSVI_FILEDATA2(CString& pPacket);
	void msgSVI_FILEEND2(CString& pPacket);
	void msgSVI_PING(CString& pPacket);
	void msgSVI_RAWDATA(CString& pPacket);
	void msgSVI_FILESTART3(CString& pPacket);
	void msgSVI_FILEDATA3(CString& pPacket);
	void msgSVI_FILEEND3(CString& pPacket);
	void msgSVI_SERVERINFO(CString& pPacket);
	void msgSVI_REQUESTTEXT(CString& pPacket);
	void msgSVI_SENDTEXT(CString& pPacket);
	void msgSVI_PMPLAYER(CString& pPacket);
	void msgSVI_ASSIGNPCID(CString& pPacket);

protected:
	// Packet Functions
	bool parsePacket(CString& pPacket);

	// Socket Variables
	bool nextIsRaw;
	int rawPacketSize;
	CFileQueue _fileQueue;
	CString readBuffer;
	CSocket sock;
	time_t lastData, lastTimer;
	time_t nextConnectionAttempt;
	uint8_t connectionAttempts;
	Server* _server;

	std::map<std::string, int> serverListCount;
	std::string _serverLocalIp;
	std::string _serverRemoteIp;
};

#endif // TSERVERLIST_H
