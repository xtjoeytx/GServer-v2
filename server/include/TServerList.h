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
	SVO_SETNAME			= 0,
	SVO_SETDESC			= 1,
	SVO_SETLANG			= 2,
	SVO_SETVERS			= 3,
	SVO_SETURL			= 4,
	SVO_SETIP			= 5,
	SVO_SETPORT			= 6,
	SVO_SETPLYR			= 7,
	SVO_VERIACC			= 8,	// deprecated
	SVO_VERIGUILD		= 9,
	SVO_GETFILE			= 10,	// deprecated
	SVO_NICKNAME		= 11,
	SVO_GETPROF			= 12,
	SVO_SETPROF			= 13,
	SVO_PLYRADD			= 14,
	SVO_PLYRREM			= 15,
	SVO_PING			= 16,
	SVO_VERIACC2		= 17,
	SVO_SETLOCALIP		= 18,
	SVO_GETFILE2		= 19,	// deprecated
	SVO_UPDATEFILE		= 20,
	SVO_GETFILE3		= 21,
	SVO_NEWSERVER		= 22,
	SVO_SERVERHQPASS	= 23,
	SVO_SERVERHQLEVEL	= 24,
};

enum
{
	SVI_VERIACC			= 0,	// deprecated
	SVI_VERIGUILD		= 1,
	SVI_FILESTART		= 2,	// deprecated
	SVI_FILEEND			= 3,	// deprecated
	SVI_FILEDATA		= 4,	// deprecated
	SVI_VERSIONOLD		= 5,
	SVI_VERSIONCURRENT	= 6,
	SVI_PROFILE			= 7,
	SVI_ERRMSG			= 8,
	SVI_NULL4			= 9,
	SVI_NULL5			= 10,
	SVI_VERIACC2		= 11,
	SVI_FILESTART2		= 12,	// deprecated
	SVI_FILEDATA2		= 13,	// deprecated
	SVI_FILEEND2		= 14,	// deprecated
	SVI_FILESTART3		= 15,
	SVI_FILEDATA3		= 16,
	SVI_FILEEND3		= 17,
	SVI_PING			= 99,
	SVI_RAWDATA			= 100,
};

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
		
	protected:
		// Packet Functions
		void parsePacket(CString& pPacket);
		void sendCompress();

		// Socket Variables
		bool nextIsRaw;
		int rawPacketSize;
		CString rBuffer, sBuffer;
		CSocket sock;
		time_t lastData, lastPing, lastTimer;
		TServer *server;
};

#endif // TSERVERLIST_H
