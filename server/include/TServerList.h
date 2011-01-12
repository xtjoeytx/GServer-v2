#ifndef TSERVERLIST_H
#define TSERVERLIST_H

#include <time.h>
#include "CString.h"
#include "CSocket.h"


enum
{
	SVF_HEAD			= 0,
	SVF_BODY			= 1,
	SVF_SWORD			= 2,
	SVF_SHIELD			= 3,
	SVF_FILE			= 4,
};

class TPlayer;
class TServer;
class TServerList : public CSocketStub
{
	public:
		// Required by CSocketStub.
		bool onRecv();
		bool onSend();
		bool onRegister()			{ return true; }
		void onUnregister()			{ return; }
		SOCKET getSocketHandle()	{ return sock.getHandle(); }
		bool canRecv();
		bool canSend();

		// Constructor - Deconstructor
		TServerList();
		~TServerList();
		void setServer(TServer* pServer) { server = pServer; }

		bool doTimedEvents();
		
		// Socket-Control Functions
		bool getConnected() const;
		bool main();
		bool init(const CString& pserverIp, const CString& pServerPort = "14900");
		bool connectServer();
		CSocket* getSocket()					{ return &sock; }
		void sendPacket(CString& pPacket);

		// Altering Player Information
		void addPlayer(TPlayer *pPlayer);
		void remPlayer(const CString& pAccountName, int pType = ((int)(1 << 0) | (int)(1 << 5)));
		void sendPlayers();

		// Send New Server-Info
		void sendServerHQ();
		void setDesc(const CString& pServerDesc);
		void setIp(const CString& pServerIp);
		void setName(const CString& pServerName);
		void setPort(const CString& pServerPort);
		void setUrl(const CString& pServerUrl);
		void setVersion(const CString& pServerVersion);

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
		
	protected:
		// Packet Functions
		void parsePacket(CString& pPacket);
		void sendCompress();

		// Socket Variables
		bool nextIsRaw;
		int rawPacketSize;
		CString rBuffer, sBuffer;
		CSocket sock;
		time_t lastData, lastPing, lastTimer, lastPlayerSync;
		TServer *server;
};

#endif // TSERVERLIST_H
