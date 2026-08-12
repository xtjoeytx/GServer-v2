#ifndef SERVERLIST_H
#define SERVERLIST_H

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <CSocket.h>

#include <CFileQueue.h>
#include <CString.h>

#include <BabyDI.h>
#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

enum
{
	SVF_HEAD = 0,
	SVF_BODY = 1,
	SVF_SWORD = 2,
	SVF_SHIELD = 3,
	SVF_FILE = 4,
};

class Player;
class Server;
class ServerList : public CSocketStub
{
public:
	// Required by CSocketStub.
	bool onRecv() override;
	bool onSend() override;
	bool onRegister() override { return true; }
	void onUnregister() override;
	SOCKET getSocketHandle() override { return m_socket.getHandle(); }
	bool canRecv() override;
	bool canSend() override { return m_fileQueue.canSend(); }

	// Constructor - Deconstructor
	ServerList();
	~ServerList() override = default;

	bool doTimedEvents(precise_clock::time_point time);

	// Socket-Control Functions
	[[nodiscard]] bool getConnected() const;
	bool main(precise_clock::time_point time = precise_clock::now());
	bool connectServer();
	CSocket& getSocket() { return m_socket; }
	void sendPacket(CString& pPacket, bool sendNow = false);

	// Send players to the listserver
	void addPlayer(const std::shared_ptr<Player>& player);
	void deletePlayer(const std::shared_ptr<Player>& player);
	void sendPlayers();
	void handleText(const CString& data);
	void sendText(const CString& data);
	void sendText(const std::vector<CString>& stringList);
	void sendTextForPlayer(const std::shared_ptr<Player>& player, const CString& data);

	void sendLoginPacketForPlayer(const std::shared_ptr<Player>& player, const CString& password, const CString& identity);

	const std::map<std::string, int>& getServerList() { return m_serverListCount; }
	[[nodiscard]] const std::string& getLocalIP() const { return m_serverLocalIp; }
	[[nodiscard]] const std::string& getServerIP() const { return m_serverRemoteIp; }

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
	Server* m_server = nullptr;

	// Packet Functions
	bool parsePacket(CString& pPacket);

	// Socket Variables
	bool m_nextIsRaw = false;
	int m_rawPacketSize = 0;
	CFileQueue m_fileQueue;
	CString m_readBuffer;
	CSocket m_socket;
	precise_clock::time_point m_lastData;
	precise_clock::time_point m_lastTimer;
	precise_clock::time_point m_nextConnectionAttempt;
	precise_clock::time_point m_lastPingTime;
	uint8_t m_connectionAttempts = 0;

	std::map<std::string, int> m_serverListCount;
	std::string m_serverLocalIp;
	std::string m_serverRemoteIp{"127.0.0.1"};
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SERVERLIST_H
