#ifndef TSERVERLIST_H
#define TSERVERLIST_H

#include <time.h>
#include "ICommon.h"
#include "IUtil.h"
#include "CString.h"
#include "CSocket.h"
//#include "TPlayer.h"		// FOR THE LOVE OF GOD DON'T UNCOMMENT!
//#include "TServer.h"

/*
	Enumerators
*/
enum
{
	SVO_SETNAME		= 0,
	SVO_SETDESC		= 1,
	SVO_SETLANG		= 2,
	SVO_SETVERS		= 3,
	SVO_SETURL		= 4,
	SVO_SETIP		= 5,
	SVO_SETPORT		= 6,
	SVO_SETPLYR		= 7,
	SVO_VERIACC		= 8,	// deprecated
	SVO_VERIGUILD	= 9,
	SVO_GETFILE		= 10,
	SVO_NICKNAME	= 11,
	SVO_GETPROF		= 12,
	SVO_SETPROF		= 13,
	SVO_PLYRADD		= 14,
	SVO_PLYRREM		= 15,
	SVO_PING		= 16,
	SVO_VERIACC2	= 17,
};

enum
{
	SVI_VERIACC			= 0,
	SVI_VERIGUILD		= 1,
	SVI_FILESTART		= 2,
	SVI_FILEDATA		= 3,
	SVI_FILEEND			= 4,
	SVI_VERSIONOLD		= 5,
	SVI_VERSIONCURRENT	= 6,
	SVI_PROFILE			= 7,
	SVI_ERRMSG			= 8,
	SVI_NULL4			= 9,
	SVI_NULL5			= 10,
	SVI_VERIACC2		= 11,
	SVI_PING			= 99
};

class TPlayer;
class TServer;
class TServerList
{
	public:
		// Constructor - Deconstructor
		TServerList();
		~TServerList();
		void setServer(TServer* pServer) { server = pServer; }

		// Socket-Control Functions
		bool getConnected();
		bool main();
		bool init(const CString& pserverIp, const CString& pServerPort = "14900");
		bool connectServer();
		void sendPacket(CString& pPacket);

		// Altering Player Information
		void addPlayer(TPlayer *pPlayer);
		void remPlayer(const CString& pAccountName, int pType = CLIENTTYPE_CLIENT);
		void sendPlayers();

		// Send New Server-Info
		void setDesc(const CString& pServerDesc);
		void setIp(const CString& pServerIp);
		void setName(const CString& pServerName);
		void setPort(const CString& pServerPort);
		void setUrl(const CString& pServerUrl);
		void setVersion(const CString& pServerVersion);

		// Incoming message parsing functions
		void msgSVI_NULL(CString& pPacket);
		void msgSVI_VERIACC(CString& pPacket);
		void msgSVI_VERIGUILD(CString& pPacket);
		void msgSVI_FILESTART(CString& pPacket);
		void msgSVI_FILEDATA(CString& pPacket);
		void msgSVI_FILEEND(CString& pPacket);
		void msgSVI_VERSIONOLD(CString& pPacket);
		void msgSVI_VERSIONCURRENT(CString& pPacket);
		void msgSVI_PROFILE(CString& pPacket);
		void msgSVI_ERRMSG(CString& pPacket);
		//void msgSVI_NULL4(CString& pPacket);
		//void msgSVI_NULL5(CString& pPacket);
		void msgSVI_VERIACC2(CString& pPacket);
		void msgSVI_PING(CString& pPacket);

	protected:
		// Packet Functions
		void parsePacket(CString& pPacket);
		void sendCompress();

		bool doTimedEvents();

		// Socket Variables
		bool isConnected;
		CString rBuffer, sBuffer;
		CSocket sock;
		time_t lastData, lastPing, lastTimer;
		TServer* server;
};

// Packet-Functions
typedef void (TServerList::*TSLSock)(CString&);
void createSLFunctions();

#endif // TSERVERLIST_H
