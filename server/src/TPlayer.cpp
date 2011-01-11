#include "IDebug.h"
#include <boost/thread.hpp>
#include <time.h>
#include <math.h>
#include <sys/stat.h>
#include <stdio.h>

#include "TPlayer.h"
#include "IEnums.h"
#include "IUtil.h"
#include "TServer.h"
#include "TAccount.h"
#include "TLevel.h"
#include "TMap.h"
#include "TWeapon.h"
#include "TNPC.h"

/*
	Logs
*/
#define serverlog	server->getServerLog()
#define rclog		server->getRCLog()

/*
	Global Definitions
*/
const char* __defaultfiles[] = {
	"carried.gani", "carry.gani", "carrystill.gani", "carrypeople.gani", "dead.gani", "def.gani", "ghostani.gani", "grab.gani", "gralats.gani", "hatoff.gani", "haton.gani", "hidden.gani", "hiddenstill.gani", "hurt.gani", "idle.gani", "kick.gani", "lava.gani", "lift.gani", "maps1.gani", "maps2.gani", "maps3.gani", "pull.gani", "push.gani", "ride.gani", "rideeat.gani", "ridefire.gani", "ridehurt.gani", "ridejump.gani", "ridestill.gani", "ridesword.gani", "shoot.gani", "sit.gani", "skip.gani", "sleep.gani", "spin.gani", "swim.gani", "sword.gani", "walk.gani", "walkslow.gani",
	"sword?.png", "sword?.gif",
	"shield?.png", "shield?.gif",
	"body.png", "body2.png", "body3.png",
	"arrow.wav", "arrowon.wav", "axe.wav", "bomb.wav", "chest.wav", "compudead.wav", "crush.wav", "dead.wav", "extra.wav", "fire.wav", "frog.wav", "frog2.wav", "goal.wav", "horse.wav", "horse2.wav", "item.wav", "item2.wav", "jump.wav", "lift.wav", "lift2.wav", "nextpage.wav", "put.wav", "sign.wav", "steps.wav", "steps2.wav", "stonemove.wav", "sword.wav", "swordon.wav", "thunder.wav", "water.wav",
	"pics1.png",
};

// Enum per Attr
int __attrPackets[30] = { 37, 38, 39, 40, 41, 46, 47, 48, 49, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74 };

// Sent on Login
bool __sendLogin[propscount] =
{
	false, true,  true,  true,  true,  true,  // 0-5
	true,  false, true,  true,  true,  true,  // 6-11
	false, true,  false, false, false, true,  // 12-17
	true,  false, false, true,  true,  true,  // 18-23
	false, true,  true,  false, false, false, // 24-29
	false, false, true,  false, true,  true,  // 30-35
	true,  true,  true,  true,  true,  true,  // 36-41
	false, false, false, false, true,  true,  // 42-47
	true,  true,  false, false, false, false, // 48-53
	true,  true,  true,  true,  true,  true,  // 54-59
	true,  true,  true,  true,  true,  true,  // 60-65
	true,  true,  true,  true,  true,  true,  // 66-71
	true,  true,  true,  false, false, false, // 72-77
	false, false, false, false, true, // 78-82
};

bool __getLogin[propscount] =
{
	true,  false, false, false, false, false, // 0-5
	false, false, true,  true,  true,  true,  // 6-11
	true,  true,  false, true,  true,  true,  // 12-17
	true,  true,  true,  true,  false, false, // 18-23
	true,  false, false, false, false, false, // 24-29
	true,  true,  true,  false, true,  true,  // 30-35
	true,  true,  true,  true,  true,  true,  // 36-41
	false, true,  true,  true,  true,  true,  // 42-47
	true,  true,  true,  false, false, true,  // 48-53
	true,  true,  true,  true,  true,  true,  // 54-59
	true,  true,  true,  true,  true,  true,  // 60-65
	true,  true,  true,  true,  true,  true,  // 66-71
	true,  true,  true,  false, false, false, // 72-77
	true,  true,  true,  false, true, // 78-82
};

// Turn prop 14 off to see the npc-server's profile.
bool __getLoginNC[propscount] =
{
	true,  true,  true,  true,  true,  true,  // 0-5
	true,  true,  true,  true,  true,  true,  // 6-11
	true,  true,  true,  true,  true,  true,  // 12-17
	true,  true,  true,  true,  true,  true,  // 18-23
	true,  true,  true,  true,  true,  true,  // 24-29
	true,  false, true,  true,  true,  true,  // 30-35
	true,  true,  true,  true,  true,  true,  // 36-41
	false, true,  true,  true,  true,  true,  // 42-47
	true,  true,  true,  false, true,  true,  // 48-53
	true,  true,  true,  true,  true,  true,  // 54-59
	true,  true,  true,  true,  true,  true,  // 60-65
	true,  true,  true,  true,  true,  true,  // 66-71
	true,  true,  true,  true,  false, false, // 72-77
	true,  true,  true,  false, false, // 78-82
};

bool __getRCLogin[propscount] =
{
	true,  false, false, false, false, false, // 0-5
	false, false, false, false, false, true,  // 6-11
	false, false, false, false, false, false, // 12-17
	true,  false, true,  false, false, false, // 18-23
	false, false, false, false, false, false, // 24-29
	true,  true,  false, false, true,  false, // 30-35
	false, false, false, false, false, false, // 36-41
	false, false, false, false, false, false, // 42-47
	false, false, false, false, false, true,  // 48-53
	false, false, false, false, false, false, // 54-59
	false, false, false, false, false, false, // 60-65
	false, false, false, false, false, false, // 66-71
	false, false, false, false, false, false, // 72-77
	false, false, false, false, true, // 78-82
};

bool __sendLocal[propscount] =
{
	false, false, true,  false, false, false, // 0-5
	false, false, true,  true,  true,  true,  // 6-11
	true,  true,  false, true,  true,  true,  // 12-17
	true,  true,  true,  true,  false, false, // 18-23
	true,  true,  false, false, false, false, // 24-29
	true,  true,  true,  false, true,  true,  // 30-35
	true,  true,  true,  true,  true,  true,  // 36-41
	false, true,  true,  true,  true,  true,  // 42-47
	true,  true,  true,  false, false, true,  // 48-53
	true,  true,  true,  true,  true,  true,  // 54-59
	true,  true,  true,  true,  true,  true,  // 60-65
	true,  true,  true,  true,  true,  true,  // 66-71
	true,  true,  true,  false, false, false, // 72-77
	true,  true,  true,  false, true, // 78-82
};

bool __playerPropsRC[propscount] =
{
	true,  true,  true,  true,  true,  true,  // 0-5
	true,  false, true,  true,  true,  true,  // 6-11
	false, true,  false, true,  true,  false, // 12-17
	true,  false, true,  false, false, false, // 18-23
	false, false, true,  true,  true,  true,  // 24-29
	true,  false, true,  false, true,  true,  // 30-35
	true,  false, false, false, false, false, // 36-41
	false, false, false, false, false, false, // 42-47
	false, false, false, false, false, false, // 48-53
	false, false, false, false, false, false, // 54-59
	false, false, false, false, false, false, // 60-65
	false, false, false, false, false, false, // 66-71
	false, false, false, false, false, false, // 72-77
	false, false, false, false, false, // 78-82
};

/*
	Pointer-Functions for Packets
*/
bool TPlayer::created = false;
typedef bool (TPlayer::*TPLSock)(CString&);
std::vector<TPLSock> TPLFunc(256, &TPlayer::msgPLI_NULL);

void TPlayer::createFunctions()
{
	if (TPlayer::created)
		return;

	// now set non-nulls
	TPLFunc[PLI_LEVELWARP] = &TPlayer::msgPLI_LEVELWARP;
	TPLFunc[PLI_BOARDMODIFY] = &TPlayer::msgPLI_BOARDMODIFY;
	TPLFunc[PLI_PLAYERPROPS] = &TPlayer::msgPLI_PLAYERPROPS;
	TPLFunc[PLI_NPCPROPS] = &TPlayer::msgPLI_NPCPROPS;
	TPLFunc[PLI_BOMBADD] = &TPlayer::msgPLI_BOMBADD;
	TPLFunc[PLI_BOMBDEL] = &TPlayer::msgPLI_BOMBDEL;
	TPLFunc[PLI_TOALL] = &TPlayer::msgPLI_TOALL;
	TPLFunc[PLI_HORSEADD] = &TPlayer::msgPLI_HORSEADD;
	TPLFunc[PLI_HORSEDEL] = &TPlayer::msgPLI_HORSEDEL;
	TPLFunc[PLI_ARROWADD] = &TPlayer::msgPLI_ARROWADD;
	TPLFunc[PLI_FIRESPY] = &TPlayer::msgPLI_FIRESPY;
	TPLFunc[PLI_THROWCARRIED] = &TPlayer::msgPLI_THROWCARRIED;
	TPLFunc[PLI_ITEMADD] = &TPlayer::msgPLI_ITEMADD;
	TPLFunc[PLI_ITEMDEL] = &TPlayer::msgPLI_ITEMDEL;
	TPLFunc[PLI_CLAIMPKER] = &TPlayer::msgPLI_CLAIMPKER;
	TPLFunc[PLI_BADDYPROPS] = &TPlayer::msgPLI_BADDYPROPS;
	TPLFunc[PLI_BADDYHURT] = &TPlayer::msgPLI_BADDYHURT;
	TPLFunc[PLI_BADDYADD] = &TPlayer::msgPLI_BADDYADD;
	TPLFunc[PLI_FLAGSET] = &TPlayer::msgPLI_FLAGSET;
	TPLFunc[PLI_FLAGDEL] = &TPlayer::msgPLI_FLAGDEL;
	TPLFunc[PLI_OPENCHEST] = &TPlayer::msgPLI_OPENCHEST;
	TPLFunc[PLI_PUTNPC] = &TPlayer::msgPLI_PUTNPC;
	TPLFunc[PLI_NPCDEL] = &TPlayer::msgPLI_NPCDEL;
	TPLFunc[PLI_WANTFILE] = &TPlayer::msgPLI_WANTFILE;
	TPLFunc[PLI_SHOWIMG] = &TPlayer::msgPLI_SHOWIMG;

	TPLFunc[PLI_HURTPLAYER] = &TPlayer::msgPLI_HURTPLAYER;
	TPLFunc[PLI_EXPLOSION] = &TPlayer::msgPLI_EXPLOSION;
	TPLFunc[PLI_PRIVATEMESSAGE] = &TPlayer::msgPLI_PRIVATEMESSAGE;
	TPLFunc[PLI_NPCWEAPONDEL] = &TPlayer::msgPLI_NPCWEAPONDEL;
	TPLFunc[PLI_LEVELWARPMOD] = &TPlayer::msgPLI_LEVELWARP;	// Shared with PLI_LEVELWARP
	TPLFunc[PLI_PACKETCOUNT] = &TPlayer::msgPLI_PACKETCOUNT;
	TPLFunc[PLI_ITEMTAKE] = &TPlayer::msgPLI_ITEMDEL;			// Shared with PLI_ITEMDEL
	TPLFunc[PLI_WEAPONADD] = &TPlayer::msgPLI_WEAPONADD;
	TPLFunc[PLI_UPDATEFILE] = &TPlayer::msgPLI_UPDATEFILE;
	TPLFunc[PLI_ADJACENTLEVEL] = &TPlayer::msgPLI_ADJACENTLEVEL;
	TPLFunc[PLI_HITOBJECTS] = &TPlayer::msgPLI_HITOBJECTS;
	TPLFunc[PLI_LANGUAGE] = &TPlayer::msgPLI_LANGUAGE;
	TPLFunc[PLI_TRIGGERACTION] = &TPlayer::msgPLI_TRIGGERACTION;
	TPLFunc[PLI_MAPINFO] = &TPlayer::msgPLI_MAPINFO;
	TPLFunc[PLI_SHOOT] = &TPlayer::msgPLI_SHOOT;
	TPLFunc[PLI_SERVERWARP] = &TPlayer::msgPLI_SERVERWARP;

	TPLFunc[PLI_PROCESSLIST] = &TPlayer::msgPLI_PROCESSLIST;

	TPLFunc[PLI_UNKNOWN46] = &TPlayer::msgPLI_UNKNOWN46;
	TPLFunc[PLI_RAWDATA] = &TPlayer::msgPLI_RAWDATA;

	TPLFunc[PLI_RC_SERVEROPTIONSGET] = &TPlayer::msgPLI_RC_SERVEROPTIONSGET;
	TPLFunc[PLI_RC_SERVEROPTIONSSET] = &TPlayer::msgPLI_RC_SERVEROPTIONSSET;
	TPLFunc[PLI_RC_FOLDERCONFIGGET] = &TPlayer::msgPLI_RC_FOLDERCONFIGGET;
	TPLFunc[PLI_RC_FOLDERCONFIGSET] = &TPlayer::msgPLI_RC_FOLDERCONFIGSET;
	TPLFunc[PLI_RC_RESPAWNSET] = &TPlayer::msgPLI_RC_RESPAWNSET;
	TPLFunc[PLI_RC_HORSELIFESET] = &TPlayer::msgPLI_RC_HORSELIFESET;
	TPLFunc[PLI_RC_APINCREMENTSET] = &TPlayer::msgPLI_RC_APINCREMENTSET;
	TPLFunc[PLI_RC_BADDYRESPAWNSET] = &TPlayer::msgPLI_RC_BADDYRESPAWNSET;
	TPLFunc[PLI_RC_PLAYERPROPSGET] = &TPlayer::msgPLI_RC_PLAYERPROPSGET;
	TPLFunc[PLI_RC_PLAYERPROPSSET] = &TPlayer::msgPLI_RC_PLAYERPROPSSET;
	TPLFunc[PLI_RC_DISCONNECTPLAYER] = &TPlayer::msgPLI_RC_DISCONNECTPLAYER;
	TPLFunc[PLI_RC_UPDATELEVELS] = &TPlayer::msgPLI_RC_UPDATELEVELS;
	TPLFunc[PLI_RC_ADMINMESSAGE] = &TPlayer::msgPLI_RC_ADMINMESSAGE;
	TPLFunc[PLI_RC_PRIVADMINMESSAGE] = &TPlayer::msgPLI_RC_PRIVADMINMESSAGE;
	TPLFunc[PLI_RC_LISTRCS] = &TPlayer::msgPLI_RC_LISTRCS;
	TPLFunc[PLI_RC_DISCONNECTRC] = &TPlayer::msgPLI_RC_DISCONNECTRC;
	TPLFunc[PLI_RC_APPLYREASON] = &TPlayer::msgPLI_RC_APPLYREASON;
	TPLFunc[PLI_RC_SERVERFLAGSGET] = &TPlayer::msgPLI_RC_SERVERFLAGSGET;
	TPLFunc[PLI_RC_SERVERFLAGSSET] = &TPlayer::msgPLI_RC_SERVERFLAGSSET;
	TPLFunc[PLI_RC_ACCOUNTADD] = &TPlayer::msgPLI_RC_ACCOUNTADD;
	TPLFunc[PLI_RC_ACCOUNTDEL] = &TPlayer::msgPLI_RC_ACCOUNTDEL;
	TPLFunc[PLI_RC_ACCOUNTLISTGET] = &TPlayer::msgPLI_RC_ACCOUNTLISTGET;
	TPLFunc[PLI_RC_PLAYERPROPSGET2] = &TPlayer::msgPLI_RC_PLAYERPROPSGET2;
	TPLFunc[PLI_RC_PLAYERPROPSGET3] = &TPlayer::msgPLI_RC_PLAYERPROPSGET3;
	TPLFunc[PLI_RC_PLAYERPROPSRESET] = &TPlayer::msgPLI_RC_PLAYERPROPSRESET;
	TPLFunc[PLI_RC_PLAYERPROPSSET2] = &TPlayer::msgPLI_RC_PLAYERPROPSSET2;
	TPLFunc[PLI_RC_ACCOUNTGET] = &TPlayer::msgPLI_RC_ACCOUNTGET;
	TPLFunc[PLI_RC_ACCOUNTSET] = &TPlayer::msgPLI_RC_ACCOUNTSET;
	TPLFunc[PLI_RC_CHAT] = &TPlayer::msgPLI_RC_CHAT;
	TPLFunc[PLI_PROFILEGET] = &TPlayer::msgPLI_PROFILEGET;
	TPLFunc[PLI_PROFILESET] = &TPlayer::msgPLI_PROFILESET;
	TPLFunc[PLI_RC_WARPPLAYER] = &TPlayer::msgPLI_RC_WARPPLAYER;
	TPLFunc[PLI_RC_PLAYERRIGHTSGET] = &TPlayer::msgPLI_RC_PLAYERRIGHTSGET;
	TPLFunc[PLI_RC_PLAYERRIGHTSSET] = &TPlayer::msgPLI_RC_PLAYERRIGHTSSET;
	TPLFunc[PLI_RC_PLAYERCOMMENTSGET] = &TPlayer::msgPLI_RC_PLAYERCOMMENTSGET;
	TPLFunc[PLI_RC_PLAYERCOMMENTSSET] = &TPlayer::msgPLI_RC_PLAYERCOMMENTSSET;
	TPLFunc[PLI_RC_PLAYERBANGET] = &TPlayer::msgPLI_RC_PLAYERBANGET;
	TPLFunc[PLI_RC_PLAYERBANSET] = &TPlayer::msgPLI_RC_PLAYERBANSET;
	TPLFunc[PLI_RC_FILEBROWSER_START] = &TPlayer::msgPLI_RC_FILEBROWSER_START;
	TPLFunc[PLI_RC_FILEBROWSER_CD] = &TPlayer::msgPLI_RC_FILEBROWSER_CD;
	TPLFunc[PLI_RC_FILEBROWSER_END] = &TPlayer::msgPLI_RC_FILEBROWSER_END;
	TPLFunc[PLI_RC_FILEBROWSER_DOWN] = &TPlayer::msgPLI_RC_FILEBROWSER_DOWN;
	TPLFunc[PLI_RC_FILEBROWSER_UP] = &TPlayer::msgPLI_RC_FILEBROWSER_UP;
	TPLFunc[PLI_NPCSERVERQUERY] = &TPlayer::msgPLI_NPCSERVERQUERY;
	TPLFunc[PLI_RC_FILEBROWSER_MOVE] = &TPlayer::msgPLI_RC_FILEBROWSER_MOVE;
	TPLFunc[PLI_RC_FILEBROWSER_DELETE] = &TPlayer::msgPLI_RC_FILEBROWSER_DELETE;
	TPLFunc[PLI_RC_FILEBROWSER_RENAME] = &TPlayer::msgPLI_RC_FILEBROWSER_RENAME;
	TPLFunc[PLI_RC_LARGEFILESTART] = &TPlayer::msgPLI_RC_LARGEFILESTART;
	TPLFunc[PLI_RC_LARGEFILEEND] = &TPlayer::msgPLI_RC_LARGEFILEEND;
	TPLFunc[PLI_RC_FOLDERDELETE] = &TPlayer::msgPLI_RC_FOLDERDELETE;
	TPLFunc[PLI_SENDTEXT] = &TPlayer::msgPLI_SENDTEXT;
	TPLFunc[PLI_UNKNOWN157] = &TPlayer::msgPLI_UNKNOWN157;

	// NPC-Server Functions
	TPLFunc[PLI_NC_NPCGET] = &TPlayer::msgPLI_NC_QUERY;

	// Finished
	TPlayer::created = true;
}


/*
	Constructor - Deconstructor
*/
TPlayer::TPlayer(TServer* pServer, CSocket* pSocket, int pId)
: TAccount(pServer),
playerSock(pSocket), key(0),
os("wind"), codepage(1252), level(0),
id(pId), type(PLTYPE_AWAIT), versionID(CLVER_2_17), allowBomb(false), allowBow(false),
pmap(0), carryNpcId(0), carryNpcThrown(false), loaded(false),
nextIsRaw(false), rawPacketSize(0), isFtp(false),
grMovementUpdated(false),
fileQueue(pSocket),
packetCount(0), firstLevel(true), invalidPackets(0)
{
	lastData = lastMovement = lastSave = time(0);
	lastChat = lastMessage = lastNick = 0;

	srand((unsigned int)time(0));

	// Create Functions
	if (!TPlayer::created)
		TPlayer::createFunctions();
}

TPlayer::~TPlayer()
{
	// Send all unsent data (for disconnect messages and whatnot).
	if (playerSock)
		fileQueue.sendCompress();

	if (id >= 0 && server != 0 && loaded)
	{
		// NPC-Server
		if (isNPCServer() && server->getNPCServer() == this)
			server->setNPCServer(0);

		// Save account.
		if (isClient() && !isLoadOnly)
			saveAccount();

		// Remove from the level.
		if (level) leaveLevel();

		// Announce our departure to other clients.
		server->sendPacketTo(PLTYPE_ANYCLIENT | PLTYPE_NPCSERVER, CString() >> (char)PLO_OTHERPLPROPS >> (short)id >> (char)PLPROP_PCONNECTED, this);
		server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_DELPLAYER >> (short)id, this);
		if (isRC() && !accountName.isEmpty())
			server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "RC Disconnected: " << accountName, this);

		// Log.
		if (isClient())
			serverlog.out("[%s] :: Client disconnected: %s\n", server->getName().text(), accountName.text());
		else if (isRC())
			serverlog.out("[%s] :: RC disconnected: %s\n", server->getName().text(), accountName.text());
		else if (isNPCServer())
			serverlog.out("[%s] :: NPC-Server disconnected.\n", server->getName().text());
	}

	// Clean up.
	for (std::vector<SCachedLevel*>::iterator i = cachedLevels.begin(); i != cachedLevels.end(); )
	{
		SCachedLevel* cl = *i;
		delete cl;
		i = cachedLevels.erase(i);
	}
	for (std::map<CString, TLevel*>::iterator i = spLevels.begin(); i != spLevels.end(); )
	{
		TLevel* cl = i->second;
		delete cl;
		spLevels.erase(i++);
	}

	if (playerSock)
		delete playerSock;
}

bool TPlayer::onRecv()
{
	// If our socket is gone, delete ourself.
	if (playerSock == 0 || playerSock->getState() == SOCKET_STATE_DISCONNECTED)
		return false;

	// Grab the data from the socket and put it into our receive buffer.
	unsigned int size = 0;
	char* data = playerSock->getData(&size);
	if (size != 0)
		rBuffer.write(data, size);
	else if (playerSock->getState() == SOCKET_STATE_DISCONNECTED)
		return false;

	// Do the main function.
	if (doMain() == false)
		return false;

	return true;
}

bool TPlayer::onSend()
{
	if (playerSock == 0 || playerSock->getState() == SOCKET_STATE_DISCONNECTED)
		return false;

	// Send data.
	fileQueue.sendCompress();

	return true;
}

void TPlayer::onUnregister()
{
	// Called when onSend() or onRecv() returns false.
	server->deletePlayer(this);
}

bool TPlayer::canRecv()
{
	if (playerSock->getState() == SOCKET_STATE_DISCONNECTED) return false;
	return true;
}

bool TPlayer::canSend()
{
	return fileQueue.canSend();
}

/*
	Socket-Control Functions
*/
bool TPlayer::doMain()
{
	// definitions
	CString unBuffer;

	// parse data
	rBuffer.setRead(0);
	while (rBuffer.length() > 1)
	{
		// New data.
		lastData = time(0);

		// packet length
		unsigned short len = (unsigned short)rBuffer.readShort();
		if ((unsigned int)len > (unsigned int)rBuffer.length()-2)
			break;

		// get packet
		unBuffer = rBuffer.readChars(len);
		rBuffer.removeI(0, len+2);

		// decrypt packet
		switch (in_codec.getGen())
		{
			case ENCRYPT_GEN_1:		// Gen 1 is not encrypted or compressed.
				break;

			// Gen 2 and 3 are zlib compressed.  Gen 3 encrypts individual packets
			// Uncompress so we can properly decrypt later on.
			case ENCRYPT_GEN_2:
			case ENCRYPT_GEN_3:
				unBuffer.zuncompressI();
				break;

			// Gen 4 and up encrypt the whole combined and compressed packet.
			// Decrypt and decompress.
			default:
				decryptPacket(unBuffer);
				break;
		}

		// well theres your buffer
		if (!parsePacket(unBuffer))
			return false;
	}

	// Update the -gr_movement packets.
	if (!grMovementPackets.isEmpty())
	{
		if (!grMovementUpdated)
		{
			std::vector<CString> pack = grMovementPackets.tokenize("\n");
			for (std::vector<CString>::iterator i = pack.begin(); i != pack.end(); ++i)
				setProps(*i, true, false);
		}
		grMovementPackets.clear(42);
	}
	grMovementUpdated = false;

	server->getSocketManager()->updateSingle(this, false, true);
	return true;
}

bool TPlayer::doTimedEvents()
{
	time_t currTime = time(0);

	// If we are disconnected, delete ourself!
	if (playerSock == 0 || playerSock->getState() == SOCKET_STATE_DISCONNECTED)
	{
		server->deletePlayer(this);
		return false;
	}

	// Only run for clients.
	if (!isClient()) return true;

	// Increase online time.
	onlineTime++;

	// Disconnect if players are inactive.
	CSettings* settings = server->getSettings();
	if (settings->getBool("disconnectifnotmoved"))
	{
		int maxnomovement = settings->getInt("maxnomovement", 1200);
		if (((int)difftime(currTime, lastMovement) > maxnomovement) && ((int)difftime(currTime, lastChat) > maxnomovement))
		{
			serverlog.out("[%s] Client %s has been disconnected due to inactivity.\n", server->getName().text(), accountName.text());
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You have been disconnected due to inactivity.");
			return false;
		}
	}

	// Disconnect if no data has been received in 5 minutes.
	if ((int)difftime(currTime, lastData) > 300)
	{
		serverlog.out("[%s] Client %s has timed out.\n", server->getName().text(), accountName.text());
		return false;
	}

	// Increase player AP.
	if (settings->getBool("apsystem") && level != 0)
	{
		if (!(status & PLSTATUS_PAUSED) && level->isSparringZone() == false)
			apCounter--;

		if (apCounter <= 0)
		{
			if (ap < 100)
			{
				ap++;
				setProps(CString() >> (char)PLPROP_ALIGNMENT >> (char)ap, true, true);
			}
			if (ap < 20) apCounter = settings->getInt("aptime0", 30);
			else if (ap < 40) apCounter = settings->getInt("aptime1", 90);
			else if (ap < 60) apCounter = settings->getInt("aptime2", 300);
			else if (ap < 80) apCounter = settings->getInt("aptime3", 600);
			else apCounter = settings->getInt("aptime4", 1200);
		}
	}

	// Do singleplayer level events.
	{
		for (std::map<CString, TLevel *>::iterator i = spLevels.begin(); i != spLevels.end(); ++i)
		{
			TLevel* level = i->second;
			if (level == 0)
				continue;

			level->doTimedEvents();
		}
	}

	// Save player account every 5 minutes.
	if ((int)difftime(currTime, lastSave) > 300)
	{
		lastSave = currTime;
		if (isClient() && loaded && !isLoadOnly) saveAccount();
	}

	return true;
}

void TPlayer::disconnect()
{
	server->deletePlayer(this);
	//server->getSocketManager()->unregisterSocket(this);
}

bool TPlayer::parsePacket(CString& pPacket)
{
	// First packet is always unencrypted zlib.  Read it in a special way.
	if (type == PLTYPE_AWAIT)
	{
		packetCount++;
		if (msgPLI_LOGIN(CString() << pPacket.readString("\n")) == false)
			return false;
	}

	while (pPacket.bytesLeft() > 0)
	{
		// Grab a packet out of the input stream.
		CString curPacket;
		if (nextIsRaw)
		{
			nextIsRaw = false;
			curPacket = pPacket.readChars(rawPacketSize);

			// The client and RC versions above 1.1 append a \n to the end of the packet.
			// Remove it now.
			if (isClient() || (isRC() && versionID > RCVER_1_1))
			{
				if (curPacket[curPacket.length() - 1] == '\n')
					curPacket.removeI(curPacket.length() - 1);
			}
		}
		else curPacket = pPacket.readString("\n");

		// Generation 3 encrypts individual packets so decrypt it now.
		if (in_codec.getGen() == ENCRYPT_GEN_3)
			decryptPacket(curPacket);

		// Get the packet id.
		unsigned char id = curPacket.readGUChar();

		// RC version 1.1 adds a "\n" string to the end of file uploads instead of a newline character.
		// This causes issues because it messes with the packet order.
		if (isRC() && versionID == RCVER_1_1 && id == PLI_RC_FILEBROWSER_UP)
		{
			curPacket.removeI(curPacket.length() - 1);
			curPacket.setRead(1);
			pPacket.readChar();	// Read out the n that got left behind.
		}

		// Call the function assigned to the packet id.
		packetCount++;
		if (!(*this.*TPLFunc[id])(curPacket))
			return false;
	}

	return true;
}

void TPlayer::decryptPacket(CString& pPacket)
{
	// Version 1.41 - 2.18 encryption
	// Was already decompressed so just decrypt the packet.
	if (in_codec.getGen() == ENCRYPT_GEN_3)
	{
		if (!isClient())
			return;

		in_codec.decrypt(pPacket);
	}

	// Version 2.19+ encryption.
	// Encryption happens before compression and depends on the compression used so
	// first decrypt and then decompress.
	if (in_codec.getGen() == ENCRYPT_GEN_4)
	{
		// Decrypt the packet.
		in_codec.limitFromType(COMPRESS_BZ2);
		in_codec.decrypt(pPacket);

		// Uncompress packet.
		pPacket.bzuncompressI();
	}
	else if (in_codec.getGen() >= ENCRYPT_GEN_5)
	{
		// Find the compression type and remove it.
		int pType = pPacket.readChar();
		pPacket.removeI(0, 1);

		// Decrypt the packet.
		in_codec.limitFromType(pType);		// Encryption is partially related to compression.
		in_codec.decrypt(pPacket);

		// Uncompress packet
		if (pType == COMPRESS_ZLIB)
			pPacket.zuncompressI();
		else if (pType == COMPRESS_BZ2)
			pPacket.bzuncompressI();
		else if (pType != COMPRESS_UNCOMPRESSED)
			serverlog.out("[%s] ** [ERROR] Client gave incorrect packet compression type! [%d]\n", server->getName().text(), pType);
	}
}

void TPlayer::sendPacket(CString pPacket, bool appendNL)
{
	// empty buffer?
	if (pPacket.isEmpty())
		return;

	// append '\n'
	if (appendNL)
	{
		if (pPacket[pPacket.length()-1] != '\n')
			pPacket.writeChar('\n');
	}

	// append buffer
	fileQueue.addPacket(pPacket);
}

bool TPlayer::sendFile(const CString& pFile)
{
	CFileSystem* fileSystem = server->getFileSystem();

	// Find file.
	CString path = fileSystem->find(pFile);
	if (path.isEmpty())
	{
		sendPacket(CString() >> (char)PLO_FILESENDFAILED << pFile);
		return false;
	}

	// Strip filename from the path.
	path.removeI(path.findl(CFileSystem::getPathSeparator()) + 1);
	if (path.find(server->getServerPath()) != -1)
		path.removeI(0, server->getServerPath().length());

	// Send the file now.
	return this->sendFile(path, pFile);
}

bool TPlayer::sendFile(const CString& pPath, const CString& pFile)
{
	CString filepath = CString() << server->getServerPath() << pPath << pFile;
	CString fileData;
	fileData.load(filepath);

	time_t modTime = 0;
	struct stat fileStat;
	if (stat(filepath.text(), &fileStat) != -1)
		modTime = fileStat.st_mtime;

	// See if the file exists.
	if (fileData.length() == 0)
	{
		sendPacket(CString() >> (char)PLO_FILESENDFAILED << pFile);
		return false;
	}

	// Warn for very large files.  These are the cause of many bug reports.
	if (fileData.length() > 3145728)	// 3MB
		serverlog.out("[%s] [WARNING] Sending a large file (over 3MB): %s\n", server->getName().text(), pFile.text());

	// See if we have enough room in the packet for the file.
	// If not, we need to send it as a big file.
	// 1 (PLO_FILE) + 5 (modTime) + 1 (file.length()) + file.length() + 1 (\n)
	bool isBigFile = false;
	int packetLength = 1 + 5 + 1 + pFile.length() + 1;
	if (fileData.length() > 32000)
		isBigFile = true;

	// Clients before 2.14 didn't support large files.
	if (isClient() && versionID < CLVER_2_14)
	{
		if (versionID < CLVER_2_1) packetLength -= 5;	// modTime isn't sent.
		if (fileData.length() > 64000)
		{
			sendPacket(CString() >> (char)PLO_FILESENDFAILED << pFile);
			return false;
		}
		isBigFile = false;
	}

	// If we are sending a big file, let the client know now.
	if (isBigFile)
	{
		sendPacket(CString() >> (char)PLO_LARGEFILESTART << pFile);
		//sendPacket(CString() >> (char)PLO_LARGEFILESIZE >> (long long)fileData.length());
	}

	// Send the file now.
	while (fileData.length() != 0)
	{
		int sendSize = clip(32000, 0, fileData.length());
		if (isClient() && versionID < CLVER_2_14) sendSize = fileData.length();

		// Older client versions didn't send the modTime.
		if (isClient() && versionID < CLVER_2_1)
		{
			// We don't add a \n to the end of the packet, so subtract 1 from the packet length.
			sendPacket(CString() >> (char)PLO_RAWDATA >> (int)(packetLength - 1 + sendSize));
			sendPacket(CString() >> (char)PLO_FILE >> (char)pFile.length() << pFile << fileData.subString(0, sendSize), false);
		}
		else
		{
			sendPacket(CString() >> (char)PLO_RAWDATA >> (int)(packetLength + sendSize));
			sendPacket(CString() >> (char)PLO_FILE >> (long long)modTime >> (char)pFile.length() << pFile << fileData.subString(0, sendSize) << "\n", false);
		}

		fileData.removeI(0, sendSize);
	}

	// If we had sent a large file, let the client know we finished sending it.
	if (isBigFile) sendPacket(CString() >> (char)PLO_LARGEFILEEND << pFile);

	return true;
}

bool TPlayer::testSign()
{
	CSettings* settings = server->getSettings();
	if (settings->getBool("serverside", false) == false) return true;	// TODO: NPC server check instead

	// Check for sign collisions.
	if ((sprite % 4) == 0)
	{
		std::vector<TLevelSign*>* signs = level->getLevelSigns();
		for (std::vector<TLevelSign*>::iterator i = signs->begin(); i != signs->end(); ++i)
		{
			TLevelSign* sign = *i;
			float signLoc[] = {(float)sign->getX(), (float)sign->getY()};
			if (y == signLoc[1] && inrange(x, signLoc[0]-1.5f, signLoc[0]+0.5f))
			{
				sendPacket(CString() >> (char)PLO_SAY2 << sign->getUText().replaceAll("\n", "#b"));
			}
		}
	}
	return true;
}

void TPlayer::dropItemsOnDeath()
{
	if (server->getSettings()->getBool("dropitemsdead", true) == false)
		return;

	int mindeathgralats = server->getSettings()->getInt("mindeathgralats", 1);
	int maxdeathgralats = server->getSettings()->getInt("maxdeathgralats", 50);

	// Determine how many gralats to remove from the account.
	int drop_gralats = 0;
	if (maxdeathgralats > 0)
	{
		drop_gralats = rand() % maxdeathgralats;
		clip(drop_gralats, mindeathgralats, maxdeathgralats);
		if (drop_gralats > gralatc) drop_gralats = gralatc;
	}

	// Determine how many arrows and bombs to remove from the account.
	int drop_arrows = rand() % 4;
	int drop_bombs = rand() % 4;
	if ((drop_arrows * 5) > arrowc) drop_arrows = arrowc / 5;
	if ((drop_bombs * 5) > bombc) drop_bombs = bombc / 5;

	// Remove gralats/bombs/arrows.
	gralatc -= drop_gralats;
	arrowc -= (drop_arrows * 5);
	bombc -= (drop_bombs * 5);
	sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PLPROP_RUPEESCOUNT >> (int)gralatc >> (char)PLPROP_ARROWSCOUNT >> (char)arrowc >> (char)PLPROP_BOMBSCOUNT >> (char)bombc);

	// Add gralats to the level.
	while (drop_gralats != 0)
	{
		char item = 0;
		if (drop_gralats % 100 != drop_gralats)
		{
			drop_gralats -= 100;
			item = 19;
		}
		else if (drop_gralats % 30 != drop_gralats)
		{
			drop_gralats -= 30;
			item = 2;
		}
		else if (drop_gralats % 5 != drop_gralats)
		{
			drop_gralats -= 5;
			item = 1;
		}
		else if (drop_gralats != 0)
		{
			--drop_gralats;
			item = 0;
		}

		float pX = x + 1.5f + (rand() % 8) - 2.0f;
		float pY = y + 2.0f + (rand() % 8) - 2.0f;

		CString packet = CString() >> (char)PLI_ITEMADD >> (char)(pX * 2) >> (char)(pY * 2) >> (char)item;
		packet.readGChar();		// So msgPLI_ITEMADD works.

		msgPLI_ITEMADD(packet);
		sendPacket(CString() >> (char)PLO_ITEMADD << packet.subString(1));
	}

	// Add arrows and bombs to the level.
	for (int i = 0; i < drop_arrows; ++i)
	{
		float pX = x + 1.5f + (rand() % 8) - 2.0f;
		float pY = y + 2.0f + (rand() % 8) - 2.0f;

		CString packet = CString() >> (char)PLI_ITEMADD >> (char)(pX * 2) >> (char)(pY * 2) >> (char)4;	// 4 = arrows
		packet.readGChar();		// So msgPLI_ITEMADD works.

		msgPLI_ITEMADD(packet);
		sendPacket(CString() >> (char)PLO_ITEMADD << packet.subString(1));
	}
	for (int i = 0; i < drop_bombs; ++i)
	{
		float pX = x + 1.5f + (rand() % 8) - 2.0f;
		float pY = y + 2.0f + (rand() % 8) - 2.0f;

		CString packet = CString() >> (char)PLI_ITEMADD >> (char)(pX * 2) >> (char)(pY * 2) >> (char)3;	// 3 = bombs
		packet.readGChar();		// So msgPLI_ITEMADD works.

		msgPLI_ITEMADD(packet);
		sendPacket(CString() >> (char)PLO_ITEMADD << packet.subString(1));
	}
}

bool TPlayer::processChat(CString pChat)
{
	std::vector<CString> chatParse = pChat.tokenizeConsole();
	if (chatParse.size() == 0) return false;
	bool processed = false;
	bool setcolorsallowed = server->getSettings()->getBool("setcolorsallowed", true);

	if (chatParse[0] == "setnick")
	{
		processed = true;
		if ((int)difftime(time(0), lastNick) >= 10)
		{
			lastNick = time(0);
			CString newName = pChat.subString(8).trim();

			// Word filter.
			int filter = server->getWordFilter()->apply(this, newName, FILTER_CHECK_NICK);
			if (filter & FILTER_ACTION_WARN)
			{
				setChat(newName);
				return true;
			}

			setProps(CString() >> (char)PLPROP_NICKNAME >> (char)newName.length() << newName, true, true);
		}
		else
			setChat("Wait 10 seconds before changing your nick again!");
	}
	else if (chatParse[0] == "sethead" && chatParse.size() == 2)
	{
		if (server->getSettings()->getBool("setheadallowed", true) == false) return false;
		processed = true;

		// Make sure it isn't one of the default files.
		bool isDefault = false;
		for (unsigned int i = 0; i < sizeof(__defaultfiles) / sizeof(char*); ++i)
			if (chatParse[1].match(CString(__defaultfiles[i])) == true) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			setProps(CString() >> (char)PLPROP_HEADGIF >> (char)(chatParse[1].length() + 100) << chatParse[1], true, true);
			return false;
		}

		// Get the appropriate filesystem.
		CFileSystem* filesystem = server->getFileSystem();
		if (server->getSettings()->getBool("nofoldersconfig", false) == false)
			filesystem = server->getFileSystem(FS_HEAD);

		// Try to find the file.
		CString file = filesystem->findi(chatParse[1]);
		if (file.length() == 0)
		{
			int i = 0;
			const char* ext[] = {".png", ".mng", ".gif"};
			while (i < 3)
			{
				file = filesystem->findi(CString() << chatParse[1] << ext[i]);
				if (file.length() != 0)
				{
					chatParse[1] << ext[i];
					break;
				}
				++i;
			}
		}

		// Try to load the file.
		if (file.length() != 0)
			setProps(CString() >> (char)PLPROP_HEADGIF >> (char)(chatParse[1].length() + 100) << chatParse[1], true, true);
		else
			server->getServerList()->sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)id >> (char)0 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setbody" && chatParse.size() == 2)
	{
		if (server->getSettings()->getBool("setbodyallowed", true) == false) return false;
		processed = true;

		// Make sure it isn't one of the default files.
		bool isDefault = false;
		for (unsigned int i = 0; i < sizeof(__defaultfiles) / sizeof(char*); ++i)
			if (chatParse[1].match(CString(__defaultfiles[i])) == true) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			setProps(CString() >> (char)PLPROP_BODYIMG >> (char)chatParse[1].length() << chatParse[1], true, true);
			return false;
		}

		// Get the appropriate filesystem.
		CFileSystem* filesystem = server->getFileSystem();
		if (server->getSettings()->getBool("nofoldersconfig", false) == false)
			filesystem = server->getFileSystem(FS_BODY);

		// Try to find the file.
		CString file = filesystem->findi(chatParse[1]);
		if (file.length() == 0)
		{
			int i = 0;
			const char* ext[] = {".png", ".mng", ".gif"};
			while (i < 3)
			{
				file = filesystem->findi(CString() << chatParse[1] << ext[i]);
				if (file.length() != 0)
				{
					chatParse[1] << ext[i];
					break;
				}
				++i;
			}
		}

		// Try to load the file.
		if (file.length() != 0)
			setProps(CString() >> (char)PLPROP_BODYIMG >> (char)chatParse[1].length() << chatParse[1], true, true);
		else
			server->getServerList()->sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)id >> (char)1 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setsword" && chatParse.size() == 2)
	{
		if (server->getSettings()->getBool("setswordallowed", true) == false) return false;
		processed = true;

		// Make sure it isn't one of the default files.
		bool isDefault = false;
		for (unsigned int i = 0; i < sizeof(__defaultfiles) / sizeof(char*); ++i)
			if (chatParse[1].match(CString(__defaultfiles[i])) == true) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			setProps(CString() >> (char)PLPROP_SWORDPOWER >> (char)(swordPower + 30) >> (char)chatParse[1].length() << chatParse[1], true, true);
			return false;
		}

		// Get the appropriate filesystem.
		CFileSystem* filesystem = server->getFileSystem();
		if (server->getSettings()->getBool("nofoldersconfig", false) == false)
			filesystem = server->getFileSystem(FS_SWORD);

		// Try to find the file.
		CString file = filesystem->findi(chatParse[1]);
		if (file.length() == 0)
		{
			int i = 0;
			const char* ext[] = {".png", ".mng", ".gif"};
			while (i < 3)
			{
				file = filesystem->findi(CString() << chatParse[1] << ext[i]);
				if (file.length() != 0)
				{
					chatParse[1] << ext[i];
					break;
				}
				++i;
			}
		}

		// Try to load the file.
		if (file.length() != 0)
			setProps(CString() >> (char)PLPROP_SWORDPOWER >> (char)(swordPower + 30) >> (char)chatParse[1].length() << chatParse[1], true, true);
		else
			server->getServerList()->sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)id >> (char)2 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setshield" && chatParse.size() == 2)
	{
		if (server->getSettings()->getBool("setshieldallowed", true) == false) return false;
		processed = true;

		// Make sure it isn't one of the default files.
		bool isDefault = false;
		for (unsigned int i = 0; i < sizeof(__defaultfiles) / sizeof(char*); ++i)
			if (chatParse[1].match(CString(__defaultfiles[i])) == true) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			setProps(CString() >> (char)PLPROP_SHIELDPOWER >> (char)(shieldPower + 10) >> (char)chatParse[1].length() << chatParse[1], true, true);
			return false;
		}

		// Get the appropriate filesystem.
		CFileSystem* filesystem = server->getFileSystem();
		if (server->getSettings()->getBool("nofoldersconfig", false) == false)
			filesystem = server->getFileSystem(FS_SHIELD);

		// Try to find the file.
		CString file = filesystem->findi(chatParse[1]);
		if (file.length() == 0)
		{
			int i = 0;
			const char* ext[] = {".png", ".mng", ".gif"};
			while (i < 3)
			{
				file = filesystem->findi(CString() << chatParse[1] << ext[i]);
				if (file.length() != 0)
				{
					chatParse[1] << ext[i];
					break;
				}
				++i;
			}
		}

		// Try to load the file.
		if (file.length() != 0)
			setProps(CString() >> (char)PLPROP_SHIELDPOWER >> (char)(shieldPower + 10) >> (char)chatParse[1].length() << chatParse[1], true, true);
		else
			server->getServerList()->sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)id >> (char)3 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setskin" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 0
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			colors[0] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4], true, true);
		}
	}
	else if (chatParse[0] == "setcoat" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 1
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			colors[1] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4], true, true);
		}
	}
	else if (chatParse[0] == "setsleeves" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 2
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			colors[2] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4], true, true);
		}
	}
	else if (chatParse[0] == "setshoes" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 3
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			colors[3] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4], true, true);
		}
	}
	else if (chatParse[0] == "setbelt" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 4
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			colors[4] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4], true, true);
		}
	}
	else if (chatParse[0] == "warpto")
	{
		processed = true;

		// To player
		if (chatParse.size() == 2)
		{
			// Permission check.
			if (!hasRight(PLPERM_WARPTOPLAYER) && !server->getSettings()->getBool("warptoforall", false))
			{
				setChat("(not authorized to warp)");
				return true;
			}

			TPlayer* player = server->getPlayer(chatParse[1], false);
			if (player && player->getLevel() != 0)
				warp(player->getLevel()->getLevelName(), player->getX(), player->getY());
		}
		// To x/y location
		else if (chatParse.size() == 3)
		{
			// Permission check.
			if (!hasRight(PLPERM_WARPTO) && !server->getSettings()->getBool("warptoforall", false))
			{
				setChat("(not authorized to warp)");
				return true;
			}

			setProps(CString() >> (char)PLPROP_X >> (char)(strtofloat(chatParse[1]) * 2) >> (char)PLPROP_Y >> (char)(strtofloat(chatParse[2]) * 2), true, true);
		}
		// To x/y level
		else if (chatParse.size() == 4)
		{
			// Permission check.
			if (!hasRight(PLPERM_WARPTO) && !server->getSettings()->getBool("warptoforall", false))
			{
				setChat("(not authorized to warp)");
				return true;
			}

			warp(chatParse[3], (float)strtofloat(chatParse[1]), (float)strtofloat(chatParse[2]));
		}
	}
	else if (chatParse[0] == "summon" && chatParse.size() == 2)
	{
		processed = true;

		// Permission check.
		if (!hasRight(PLPERM_SUMMON))
		{
			setChat("(not authorized to summon)");
			return true;
		}

		TPlayer* p = server->getPlayer(chatParse[1], false);
		if (p) p->warp(levelName, x, y);
	}
	else if (chatParse[0] == "unstick" || chatParse[0] == "unstuck")
	{
		if (chatParse.size() == 2 && chatParse[1] == "me")
		{
			processed = true;

			// Check if the player is in a jailed level.
			std::vector<CString> jailList = server->getSettings()->getStr("jaillevels").tokenize(",");
			for (std::vector<CString>::iterator i = jailList.begin(); i != jailList.end(); ++i)
				if (i->trim() == levelName) return false;

			if ((int)difftime(time(0), lastMovement) >= 30)
			{
				lastMovement = time(0);
				CString unstickLevel = server->getSettings()->getStr("unstickmelevel", "onlinestartlocal.nw");
				float unstickX = server->getSettings()->getFloat("unstickmex", 30.0f);
				float unstickY = server->getSettings()->getFloat("unstickmey", 30.5f);
				warp(unstickLevel, unstickX, unstickY);
				setChat("Warped!");
			}
			else
				setChat(CString() << "Don't move for 30 seconds before doing '" << pChat << "'!");
		}
	}
	else if (pChat == "update level" && hasRight(PLPERM_UPDATELEVEL))
	{
		processed = true;
		level->reload();
	}
	else if (pChat == "showadmins")
	{
		processed = true;

		// Search through the player list for all RC's.
		CString msg;
		{
			std::vector<TPlayer*>* playerList = server->getPlayerList();
			for (std::vector<TPlayer*>::iterator i = playerList->begin(); i != playerList->end(); ++i)
			{
				TPlayer* p = *i;

				// If an RC was found, add it to our string.
				if (p->getType() & PLTYPE_ANYRC)
					msg << (msg.length() == 0 ? "" : ", ") << p->getAccountName();
			}
		}
		if (msg.length() == 0)
			msg << "(no one)";
		setChat(CString("admins: ") << msg);
	}
	else if (chatParse[0] == "showguild")
	{
		processed = true;
		CString g = guild;

		// If a guild was specified, overwrite our guild with it.
		if (chatParse.size() == 2)
			g = chatParse[1];

		if (g.length() != 0)
		{
			CString msg;
			{
				std::vector<TPlayer*>* playerList = server->getPlayerList();
				for (std::vector<TPlayer*>::iterator i = playerList->begin(); i != playerList->end(); ++i)
				{
					TPlayer* p = *i;

					// If our guild matches, add it to our string.
					if (p->getGuild() == g)
						msg << (msg.length() == 0 ? "" : ", ") << p->getNickname().subString(0, p->getNickname().find('(')).trimI();
				}
			}
			if (msg.length() == 0)
				msg << "(no one)";
			setChat(CString("members of '") << g << "': " << msg);
		}
	}
	else if (pChat == "showkills")
	{
		processed = true;
		setChat(CString() << "kills: " << CString((int)kills));
	}
	else if (pChat == "showdeaths")
	{
		processed = true;
		setChat(CString() << "deaths: " << CString((int)deaths));
	}
	else if (pChat == "showonlinetime")
	{
		processed = true;
		int seconds = onlineTime % 60;
		int minutes = (onlineTime / 60) % 60;
		int hours = onlineTime / 3600;
		CString msg;
		if (hours != 0) msg << CString(hours) << "h ";
		if (minutes != 0 || hours != 0) msg << CString(minutes) << "m ";
		msg << CString(seconds) << "s";
		setChat(CString() << "onlinetime: " << msg);
	}
	else if (chatParse[0] == "toguild:")
	{
		processed = true;
		if (guild.length() == 0) return false;

		// Get the PM.
		CString pm = pChat.text() + 8;
		pm.trimI();
		if (pm.length() == 0) return false;

		// Send PM to guild members.
		int num = 0;
		{
			std::vector<TPlayer*>* playerList = server->getPlayerList();
			for (std::vector<TPlayer*>::iterator i = playerList->begin(); i != playerList->end(); ++i)
			{
				TPlayer* p = *i;
				if (p == this) continue;

				// If our guild matches, send the PM.
				if (p->getGuild() == guild)
				{
					p->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)id << "\"\",\"Guild message:\",\"" << pm << "\"");
					++num;
				}
			}
		}

		// Tell the player how many guild members received his message.
		setChat(CString() << "(" << CString(num) << " guild member" << (num != 0 ? "s" : "") << " received your message)");
	}

	return processed;
}

bool TPlayer::isAdminIp()
{
	std::vector<CString> adminIps = adminIp.tokenize(",");
	for (std::vector<CString>::iterator i = adminIps.begin(); i != adminIps.end(); ++i)
	{
		if (accountIpStr.match(*i))
			   return true;
	}

	return false;
}

bool TPlayer::isStaff()
{
	std::vector<CString> staffList = server->getSettings()->getStr("staff").tokenize(",");
	for (std::vector<CString>::iterator i = staffList.begin(); i != staffList.end(); ++i)
	{
		if (accountName.toLower() == (*i).trim().toLower())
			return true;
	}

	return false;
}

/*
	TPlayer: Set Properties
*/
bool TPlayer::warp(const CString& pLevelName, float pX, float pY, time_t modTime)
{
	CSettings* settings = server->getSettings();

	// Save our current level.
	TLevel* currentLevel = level;

	// Find the level.
	TLevel* newLevel = TLevel::findLevel(pLevelName, server);

	// If we are warping to the same level, just update the player's location.
	if (currentLevel != 0 && newLevel == currentLevel)
	{
		CString packet = CString() >> (char)PLPROP_X >> (char)(pX * 2) >> (char)PLPROP_Y >> (char)(pY * 2);
		setProps(packet, true, true);
		return true;
	}

	// Find the unstickme level.
	TLevel* unstickLevel = TLevel::findLevel(settings->getStr("unstickmelevel", "onlinestartlocal.nw"), server);
	float unstickX = settings->getFloat("unstickmex", 30.0f);
	float unstickY = settings->getFloat("unstickmey", 35.0f);

	// Leave our current level.
	leaveLevel();

	// See if the new level is on a gmap.
	pmap = server->getMap(newLevel);

	// Set x/y location.
	float oldX = x, oldY = y;
	x = pX;
	y =	pY;

	// Try warping to the new level.
	if (setLevel(pLevelName, modTime) == false)
	{
		// Failed, so try warping back to our old level.
		bool warped = true;
		if (currentLevel == 0) warped = false;
		else
		{
			x = oldX;
			y = oldY;
			pmap = server->getMap(currentLevel);
			warped = setLevel(currentLevel->getLevelName());
		}
		if (warped == false)
		{
			// Failed, so try warping to the unstick level.  If that fails, we disconnect.
			if (unstickLevel == 0) return false;

			// Try to warp to the unstick me level.
			x = unstickX;
			y =	unstickY;
			pmap = server->getMap(unstickLevel);
			if (setLevel(unstickLevel->getLevelName()) == false)
				return false;
		}
	}
	return true;
}

bool TPlayer::setLevel(const CString& pLevelName, time_t modTime)
{
	// Open Level
	level = TLevel::findLevel(pLevelName, server);
	if (level == 0)
	{
		sendPacket(CString() >> (char)PLO_WARPFAILED << pLevelName);
		return false;
	}

	// Check if the level is a singleplayer level.
	// If so, see if we have been there before.  If not, duplicate it.
	if (level->isSingleplayer())
	{
		TLevel* nl = (spLevels.find(level->getLevelName()) != spLevels.end() ? spLevels[level->getLevelName()] : 0);
		if (nl == 0)
		{
			level = level->clone();
			spLevels[level->getLevelName()] = level;
		}
		else level = nl;
	}

	// Check if the map is a group map.
	if (pmap && pmap->isGroupMap())
	{
		if (!levelGroup.isEmpty())
		{
			// If any players are in this level, they might have been cached on the client.  Solve this by manually removing them.
			std::vector<TPlayer*>* plist = level->getPlayerList();
			for (std::vector<TPlayer*>::iterator i = plist->begin(); i != plist->end(); ++i)
			{
				TPlayer* p = *i;
				sendPacket(p->getProps(0, 0) >> (char)PLPROP_CURLEVEL >> (char)(level->getLevelName().length() + 1 + 7) << level->getLevelName() << ".unknown" >> (char)PLPROP_X << p->getProp(PLPROP_X) >> (char)PLPROP_Y << p->getProp(PLPROP_Y));
			}

			// Set the correct level now.
			std::map<CString, std::map<CString, TLevel*> >* groupLevels = server->getGroupLevels();
			std::map<CString, TLevel*>& group = (*groupLevels)[levelGroup];
			TLevel* nl = group[level->getLevelName()];
			if (nl == 0)
			{
				level = level->clone();
				level->setLevelName(level->getLevelName());
				group[level->getLevelName()] = level;
			}
			else level = nl;
		}
	}

	// Add myself to the level playerlist.
	level->addPlayer(this);
	levelName = level->getLevelName();

	// Tell the client their new level.
	if (modTime == 0 || versionID < CLVER_2_1)
	{
		if (pmap && pmap->getType() == MAPTYPE_GMAP && versionID >= CLVER_2_1)
		{
			gmaplevelx = pmap->getLevelX(levelName);
			gmaplevely = pmap->getLevelY(levelName);
			sendPacket(CString() >> (char)PLO_PLAYERWARP2
				>> (char)(x * 2) >> (char)(y * 2) >> (char)(z + 50)
				>> (char)gmaplevelx >> (char)gmaplevely
				<< pmap->getMapName());
		}
		else
			sendPacket(CString() >> (char)PLO_PLAYERWARP >> (char)(x * 2) >> (char)(y * 2) << levelName);
	}

	// Send the level now.
	bool succeed = true;
	if (versionID >= CLVER_2_1)
		succeed = sendLevel(level, modTime, false);
	else succeed = sendLevel141(level, modTime, false);

	if (!succeed)
	{
		sendPacket(CString() >> (char)PLO_WARPFAILED << pLevelName);
		return false;
	}

	// If the level is a sparring zone and you have 100 AP, change AP to 99 and
	// the apcounter to 1.
	if (level->isSparringZone() && ap == 100)
	{
		ap = 99;
		apCounter = 1;
		setProps(CString() >> (char)PLPROP_ALIGNMENT >> (char)ap, true, true);
	}

	// Inform everybody as to the client's new location.  This will update the minimap.
	CString minimap = this->getProps(0, 0) >> (char)PLPROP_CURLEVEL << this->getProp(PLPROP_CURLEVEL) >> (char)PLPROP_X << this->getProp(PLPROP_X) >> (char)PLPROP_Y << this->getProp(PLPROP_Y);
	for (std::vector<TPlayer*>::iterator i = server->getPlayerList()->begin(); i != server->getPlayerList()->end(); ++i)
	{
		TPlayer* p = *i;
		if (p == this)
			continue;
		if (pmap && pmap->isGroupMap() && levelGroup != p->getGroup())
			continue;

		p->sendPacket(minimap);
	}
	//server->sendPacketToAll(this->getProps(0, 0) >> (char)PLPROP_CURLEVEL << this->getProp(PLPROP_CURLEVEL) >> (char)PLPROP_X << this->getProp(PLPROP_X) >> (char)PLPROP_Y << this->getProp(PLPROP_Y), this);

	return true;
}

bool TPlayer::sendLevel(TLevel* pLevel, time_t modTime, bool fromAdjacent)
{
	if (pLevel == 0) return false;
	CSettings* settings = server->getSettings();

	// Send Level
	sendPacket(CString() >> (char)PLO_LEVELNAME << pLevel->getLevelName());
	time_t l_time = getCachedLevelModTime(pLevel);
	if (modTime == -1) modTime = pLevel->getModTime();
	if (l_time == 0)
	{
		if (modTime != pLevel->getModTime())
		{
			sendPacket(CString() >> (char)PLO_RAWDATA >> (int)(1+(64*64*2)+1));
			sendPacket(CString() << pLevel->getBoardPacket());
		}

		// Send links, signs, and mod time.
		sendPacket(CString() >> (char)PLO_LEVELMODTIME >> (long long)pLevel->getModTime());
		//if (!server->hasNPCServer())
		{
			sendPacket(CString() << pLevel->getLinksPacket());
			sendPacket(CString() << pLevel->getSignsPacket(this));
		}
	}

	// Send board changes, chests, horses, and baddies.
	if (fromAdjacent == false)
	{
		sendPacket(CString() << pLevel->getBoardChangesPacket(l_time));
		sendPacket(CString() << pLevel->getChestPacket(this));
		sendPacket(CString() << pLevel->getHorsePacket());
		sendPacket(CString() << pLevel->getBaddyPacket(versionID));
	}

	// If we are on a gmap, change our level back to the gmap.
	if (pmap && pmap->getType() == MAPTYPE_GMAP)
		sendPacket(CString() >> (char)PLO_LEVELNAME << pmap->getMapName());

	// Tell the client if there are any ghost players in the level.
	// Graal Reborn doesn't support trial accounts so pass 0 (no ghosts) instead of 1 (ghosts present).
	sendPacket(CString() >> (char)PLO_GHOSTICON >> (char)0);

	if (fromAdjacent == false || pmap != 0)
	{
		// If we are the leader, send it now.
		if (pLevel->getPlayer(0) == this || pLevel->isSingleplayer() == true)
			sendPacket(CString() >> (char)PLO_ISLEADER);
	}

	// Send new world time.
	sendPacket(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(server->getNWTime()));
	if (fromAdjacent == false || pmap != 0)
	{
		// Send NPCs.
		if (pmap && pmap->getType() == MAPTYPE_GMAP)
			sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << pmap->getMapName());
		else sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << pLevel->getLevelName());
		sendPacket(CString() << pLevel->getNpcsPacket(l_time, versionID));
	}

	// Do props stuff.
	// Maps send to players in adjacent levels too.
	if (level->isSingleplayer() == false)
	{
		if (pmap)
		{
			server->sendPacketToLevel(this->getProps(__getLogin, sizeof(__getLogin)/sizeof(bool)), pmap, this, false);
			std::vector<TPlayer*>* playerList = server->getPlayerList();
			for (std::vector<TPlayer*>::iterator i = playerList->begin(); i != playerList->end(); ++i)
			{
				TPlayer* player = (TPlayer*)*i;
				if (player == 0) continue;
				if (player == this || player->getMap() != pmap) continue;
				if (pmap->isGroupMap() && levelGroup != player->getGroup()) continue;

				if (pmap->getType() == MAPTYPE_GMAP)
				{
					int ogmap[2] = {player->getProp(PLPROP_GMAPLEVELX).readGUChar(), player->getProp(PLPROP_GMAPLEVELY).readGUChar()};
					if (abs(ogmap[0] - gmaplevelx) < 2 && abs(ogmap[1] - gmaplevely) < 2)
						this->sendPacket(player->getProps(__getLogin, sizeof(__getLogin)/sizeof(bool)));
				}
				else if (pmap->getType() == MAPTYPE_BIGMAP)
				{
					if (player->getLevel() == 0) continue;
					int ogmap[2] = {pmap->getLevelX(player->getLevel()->getActualLevelName()), pmap->getLevelY(player->getLevel()->getActualLevelName())};
					int sgmap[2] = {pmap->getLevelX(pLevel->getActualLevelName()), pmap->getLevelY(pLevel->getActualLevelName())};
					if (abs(ogmap[0] - sgmap[0]) < 2 && abs(ogmap[1] - sgmap[1]) < 2)
						this->sendPacket(player->getProps(__getLogin, sizeof(__getLogin)/sizeof(bool)));
				}
			}
		}
		else
		{
			server->sendPacketToLevel(this->getProps(__getLogin, sizeof(__getLogin)/sizeof(bool)), 0, level, this);
			std::vector<TPlayer*>* playerList = level->getPlayerList();
			for (std::vector<TPlayer*>::iterator i = playerList->begin(); i != playerList->end(); ++i)
			{
				TPlayer* player = (TPlayer*)*i;
				if (player == this) continue;
				this->sendPacket(player->getProps(__getLogin, sizeof(__getLogin)/sizeof(bool)));
			}
		}
	}

	return true;
}

bool TPlayer::sendLevel141(TLevel* pLevel, time_t modTime, bool fromAdjacent)
{
	if (pLevel == 0) return false;
	CSettings* settings = server->getSettings();

	time_t l_time = getCachedLevelModTime(pLevel);
	if (modTime == -1) modTime = pLevel->getModTime();
	if (l_time != 0)
	{
		sendPacket(CString() << pLevel->getBoardChangesPacket(l_time));
	}
	else
	{
		if (modTime != pLevel->getModTime())
		{
			sendPacket(CString() >> (char)PLO_RAWDATA >> (int)(1+(64*64*2)+1));
			sendPacket(CString() << pLevel->getBoardPacket());

			if (firstLevel)
				sendPacket(CString() >> (char)PLO_LEVELNAME << pLevel->getLevelName());
			firstLevel = false;

			// Send links, signs, and mod time.
			if (settings->getBool("serverside", false) == false)	// TODO: NPC server check instead.
			{
				sendPacket(CString() << pLevel->getLinksPacket());
				sendPacket(CString() << pLevel->getSignsPacket(this));
			}
			sendPacket(CString() >> (char)PLO_LEVELMODTIME >> (long long)pLevel->getModTime());
		}
		else
			sendPacket(CString() >> (char)PLO_LEVELBOARD);

		if (fromAdjacent == false)
		{
			sendPacket(CString() << pLevel->getBoardChangesPacket2(l_time));
			sendPacket(CString() << pLevel->getChestPacket(this));
		}
	}

	// Send board changes, chests, horses, and baddies.
	if (fromAdjacent == false)
	{
		sendPacket(CString() << pLevel->getHorsePacket());
		sendPacket(CString() << pLevel->getBaddyPacket(versionID));
	}

	// Tell the client if there are any ghost players in the level.
	// Graal Reborn doesn't support trial accounts so pass 0 (no ghosts) instead of 1 (ghosts present).
	//sendPacket(CString() >> (char)PLO_GHOSTICON >> (char)0);

	if (fromAdjacent == false)
	{
		// If we are the leader, send it now.
		if (pLevel->getPlayer(0) == this || pLevel->isSingleplayer() == true)
			sendPacket(CString() >> (char)PLO_ISLEADER);
	}

	// Send new world time.
	sendPacket(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(server->getNWTime()));

	// Send NPCs.
	if (fromAdjacent == false)
		sendPacket(CString() << pLevel->getNpcsPacket(l_time, versionID));

	// Do props stuff.
	// Maps send to players in adjacent levels too.
	if (level->isSingleplayer() == false && fromAdjacent == false)
	{
		server->sendPacketToLevel(this->getProps(__getLogin, sizeof(__getLogin)/sizeof(bool)), 0, level, this);
		std::vector<TPlayer*>* playerList = level->getPlayerList();
		for (std::vector<TPlayer*>::iterator i = playerList->begin(); i != playerList->end(); ++i)
		{
			TPlayer* player = (TPlayer*)*i;
			if (player == this) continue;
			this->sendPacket(player->getProps(__getLogin, sizeof(__getLogin)/sizeof(bool)));
		}
	}

	return true;
}

bool TPlayer::leaveLevel(bool resetCache)
{
	// Make sure we are on a level first.
	if (level == 0) return true;

	// Save the time we left the level for the client-side caching.
	bool found = false;
	for (std::vector<SCachedLevel*>::iterator i = cachedLevels.begin(); i != cachedLevels.end();)
	{
		SCachedLevel* cl = *i;
		if (cl->level == level)
		{
			cl->modTime = (resetCache ? 0 : time(0));
			found = true;
			i = cachedLevels.end();
		} else ++i;
	}
	if (found == false) cachedLevels.push_back(new SCachedLevel(level, time(0)));

	// Remove self from list of players in level.
	level->removePlayer(this);

	// Send PLO_ISLEADER to new level leader.
	TPlayer* leader = level->getPlayer(0);
	if (leader != 0) leader->sendPacket(CString() >> (char)PLO_ISLEADER);

	// Tell everyone I left.
	// This prop isn't used at all???  Maybe it is required for 1.41?
//	if (pmap && pmap->getType() != MAPTYPE_GMAP)
	{
		server->sendPacketToLevel(this->getProps(0, 0) >> (char)PLPROP_JOINLEAVELVL >> (char)0, 0, level, this);

		std::vector<TPlayer*>* playerList = server->getPlayerList();
		for (std::vector<TPlayer*>::iterator i = playerList->begin(); i != playerList->end(); ++i)
		{
			TPlayer* player = (TPlayer*)*i;
			if (player == this) continue;
			if (player->getLevel() != level) continue;
			this->sendPacket(player->getProps(0, 0) >> (char)PLPROP_JOINLEAVELVL >> (char)0);
		}
	}

	// Set the level pointer to 0.
	level = 0;

	return true;
}

time_t TPlayer::getCachedLevelModTime(const TLevel* level) const
{
	for (std::vector<SCachedLevel*>::const_iterator i = cachedLevels.begin(); i != cachedLevels.end(); ++i)
	{
		SCachedLevel* cl = *i;
		if (cl->level == level)
			return cl->modTime;
	}
	return 0;
}

void TPlayer::resetLevelCache(const TLevel* level)
{
	for (std::vector<SCachedLevel*>::const_iterator i = cachedLevels.begin(); i != cachedLevels.end(); ++i)
	{
		SCachedLevel* cl = *i;
		if (cl->level == level)
		{
			cl->modTime = 0;
			return;
		}
	}
}

void TPlayer::setChat(const CString& pChat)
{
	setProps(CString() >> (char)PLPROP_CURCHAT >> (char)pChat.length() << pChat, true, true);
}

void TPlayer::setNick(const CString& pNickName, bool force)
{
	CString newNick, nick, guild;
	int guild_start = pNickName.find('(');
	int guild_end = pNickName.find(')', guild_start);

	// If the player ommitted the ), make sure the guild calculations will work.
	if (guild_end == -1 && guild_start != -1)
		guild_end = pNickName.length();

	// If there was no guild, just use the given nickname.
	if (guild_start == -1)
		nick = pNickName.trim();
	else
	{
		// We have a guild.  Separate the nickname from the guild.
		nick = pNickName.subString(0, guild_start);
		guild = pNickName.subString(guild_start + 1, guild_end - guild_start - 1);
		nick.trimI();
		guild.trimI();
		if (guild[guild.length() - 1] == ')')
			guild.removeI(guild.length() - 1);
	}

	if (force)
	{
		nickName = pNickName;
		this->guild = guild;
		return;
	}

	// If a player has put a * before his nick, remove it.
	while (!nick.isEmpty() && nick[0] == '*')
		nick.removeI(0,1);

	// If the nickname is now empty, set it to unknown.
	if (nick.isEmpty()) nick = "unknown";

	// If the nickname is equal to the account name, add the *.
	if (nick == accountName)
		newNick = CString("*");

	// Add the nick name.
	newNick << nick;

	// If a guild was specified, add the guild.
	if (guild.length() != 0 && !isNPCServer())
	{
		// Read the guild list.
		CFileSystem guildFS(server);
		guildFS.addDir("guilds");
		CString guildList = guildFS.load(CString() << "guild" << guild << ".txt");
		if (guildList.isEmpty())
			guildList = guildFS.load(CString() << "guild" << guild.replaceAll(" ", "_") << ".txt");

		// Find the account in the guild list.
		// Will also return -1 if the guild does not exist.
		if (guildList.findi(accountName) != -1)
		{
			guildList.setRead(guildList.findi(accountName));
			CString line = guildList.readString("\n");
			line.removeAllI("\r");
			if (line.find(":") != -1)
			{
				std::vector<CString> line2 = line.tokenize(":");
				if ((line2[1])[0] == '*') line2[1].removeI(0, 1);
				if ((line2[1]) == nick)	// Use nick instead of newNick because nick doesn't include the *
				{
					nickName = newNick;
					nickName << " (" << guild << ")";
					this->guild = guild;
					return;
				}
			}
			else
			{
				nickName = newNick;
				nickName << " (" << guild << ")";
				this->guild = guild;
				return;
			}
		}
		else nickName = newNick;

		// See if we can ask if it is a global guild.
		bool askGlobal = server->getSettings()->getBool("globalguilds", true);
		if (askGlobal == false)
		{
			// Check for whitelisted global guilds.
			std::vector<CString> allowed = server->getSettings()->getStr("allowedglobalguilds").tokenize(",");
			for (std::vector<CString>::iterator i = allowed.begin(); i != allowed.end(); ++i)
			{
				if (*i == guild)
				{
					askGlobal = true;
					break;
				}
			}
		}

		// See if it is a global guild.
		if (askGlobal)
		{
			server->getServerList()->sendPacket(
				CString() >> (char)SVO_VERIGUILD >> (short)id
				>> (char)accountName.length() << accountName
				>> (char)newNick.length() << newNick
				>> (char)guild.length() << guild
				);
		}
	}
	else
	{
		if (!isNPCServer())
		{
			// Save it.
			nickName = newNick;
			this->guild.clear();
		}
		else nickName = CString() << newNick << " (Server)";
	}
}

bool TPlayer::addWeapon(int defaultWeapon)
{
	// Allow Default Weapons..?
	CSettings *settings = server->getSettings();
	if (settings->getBool("defaultweapons", true) == false)
		return false;

	// Prevent the Bow and Bomb from being added.
	if (defaultWeapon == 7 && allowBow == false)
	{
		allowBow = true;
		return false;
	}
	if (defaultWeapon == 8 && allowBomb == false)
	{
		allowBomb = true;
		return false;
	}

	TWeapon *weapon = server->getWeapon(TLevelItem::getItemName(defaultWeapon));
	if (weapon == 0)
	{
		weapon = new TWeapon(defaultWeapon);
		server->NC_AddWeapon(weapon);
	}

	// See if the player already has the weapon.
	if (vecSearch<CString>(weaponList, weapon->getName()) == -1)
	{
		weaponList.push_back(weapon->getName());
		sendPacket(CString() << weapon->getWeaponPacket());
	}

	return true;
}

bool TPlayer::addWeapon(const CString& name)
{
	TWeapon* weapon = server->getWeapon(name);
	return this->addWeapon(weapon);
}

bool TPlayer::addWeapon(TWeapon* weapon)
{
	if (weapon == 0) return false;

	// See if the player already has the weapon.
	if (vecSearch<CString>(weaponList, weapon->getName()) == -1)
	{
		weaponList.push_back(weapon->getName());
		if (id == -1) return true;

		// Send weapon.
		sendPacket(CString() << weapon->getWeaponPacket());

		// Send to npc-server.
		if (server->hasNPCServer())
			server->getNPCServer()->sendPacket(CString() >> (char)PLO_NC_CONTROL >> (char)1 /*NCO_PLAYERWEAPONADD*/ >> (short)id << weapon->getName());
	}

	return true;
}

bool TPlayer::deleteWeapon(int defaultWeapon)
{
	TWeapon* weapon = server->getWeapon(TLevelItem::getItemName(defaultWeapon));
	this->deleteWeapon(weapon);

	return true;
}

bool TPlayer::deleteWeapon(const CString& name)
{
	TWeapon* weapon = server->getWeapon(name);
	return this->deleteWeapon(weapon);
}

bool TPlayer::deleteWeapon(TWeapon* weapon)
{
	if (weapon == 0) return false;

	// Remove the weapon.
	if (vecRemove<CString>(weaponList, weapon->getName()))
	{
		if (id == -1) return true;

		// Send delete notice.
		sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << weapon->getName());

		// Send to npc-server.
		if (server->hasNPCServer())
			server->getNPCServer()->sendPacket(CString() >> (char)PLO_NPCWEAPONDEL >> (short)id >> (char)0 << weapon->getName());
	}

	return true;
}

/*
	TPlayer: Flag Functions
*/
void TPlayer::setFlag(const CString& pFlagName, const CString& pFlagValue, bool sendToPlayer, bool sendToNPCServer)
{
	// Call Default Set Flag
	TAccount::setFlag(pFlagName, pFlagValue);
	
	// Send to Player
	if (sendToPlayer)
	{
		if (pFlagValue.isEmpty())
			sendPacket(CString() >> (char)PLO_FLAGDEL << pFlagName);
		else
			sendPacket(CString() >> (char)PLO_FLAGSET << pFlagName << "=" << pFlagValue);
	}

	// Send to NPC-Server
	if (sendToNPCServer && server->getNPCServer() != 0)
		server->getNPCServer()->sendPacket(CString() >> (char)PLO_FLAGSET >> (short)id << pFlagName << "=" << pFlagValue);
}

/*
	TPlayer: Packet functions
*/
bool TPlayer::msgPLI_NULL(CString& pPacket)
{
	pPacket.setRead(0);
	printf("Unknown Player Packet: %i (%s)\n", pPacket.readGUChar(), pPacket.text()+1);
	for (int i = 0; i < pPacket.length(); ++i) printf("%02x ", (unsigned char)((pPacket.text())[i])); printf("\n");

	// If we are getting a whole bunch of invalid packets, something went wrong.  Disconnect the player.
	invalidPackets++;
	if (invalidPackets > 5) return false;

	return true;
}

bool TPlayer::msgPLI_LOGIN(CString& pPacket)
{
	// Read Player-Ip
	accountIpStr = playerSock->getRemoteIp();
	accountIp = inet_addr(accountIpStr.text());

	// Read Client-Type
	serverlog.out("[%s] :: New login:\t", server->getName().text());
	type = (1 << pPacket.readGChar());
	bool getKey = false;
	switch (type)
	{
		case PLTYPE_CLIENT:
			serverlog.out("Client\n");
			in_codec.setGen(ENCRYPT_GEN_3);
			break;
		case PLTYPE_RC:
			serverlog.out("RC\n");
			in_codec.setGen(ENCRYPT_GEN_3);
			break;
		case PLTYPE_NPCSERVER:
			serverlog.out("NPCSERVER\n");
			in_codec.setGen(ENCRYPT_GEN_3);
			break;
		//case PLTYPE_NC:
		//	serverlog.out("NC\n");
		//	in_codec.setGen(ENCRYPT_GEN_3);
		//	getKey = false;
		//	break;
		case PLTYPE_CLIENT2:
			serverlog.out("New Client (2.19 - 2.21, 3 - 3.01)\n");
			in_codec.setGen(ENCRYPT_GEN_4);
			break;
		case PLTYPE_CLIENT3:
			serverlog.out("New Client (2.22+)\n");
			in_codec.setGen(ENCRYPT_GEN_5);
			break;
		case PLTYPE_RC2:
			serverlog.out("New RC (2.22+)\n");
			in_codec.setGen(ENCRYPT_GEN_5);
			getKey = true;
			break;
		default:
			serverlog.out("Unknown (%d)\n", type);
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Your client type is unknown.  Please inform the Graal Reborn staff.  Type: " << CString((int)type) << ".");
			return false;
			break;
	}

	// Get Iterator-Key
	// 2.19+ RC and any client should get the key.
	if (isClient() || (isRC() && in_codec.getGen() > ENCRYPT_GEN_3) || getKey == true)
	{
		key = (unsigned char)pPacket.readGChar();
		in_codec.reset(key);
		if (in_codec.getGen() > ENCRYPT_GEN_3)
			fileQueue.setCodec(in_codec.getGen(), key);
	}

	// Read Client-Version
	version = pPacket.readChars(8);
	if (isClient() || isNPCServer()) versionID = getVersionID(version);
	else if (isRC()) versionID = getRCVersionID(version);
	else versionID = CLVER_UNKNOWN;

	// Read Account & Password
	accountName = pPacket.readChars(pPacket.readGUChar());
	CString password = pPacket.readChars(pPacket.readGUChar());

	//serverlog.out("[%s]    Key: %d\n", server->getName().text(), key);
	serverlog.out("[%s]    Version:\t%s (%s)\n", server->getName().text(), version.text(), getVersionString(version, type));
	serverlog.out("[%s]    Account:\t%s\n", server->getName().text(), accountName.text());

	// Check for available slots on the server.
	if (server->getPlayerList()->size() >= (unsigned int)server->getSettings()->getInt("maxplayers", 128))
	{
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "This server has reached its player limit.");
		return false;
	}

	// Check if they are ip-banned or not.
	if (server->isIpBanned(playerSock->getRemoteIp()) && !hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You have been banned from this server.");
		return false;
	}

	// Check if the specified client is allowed access.
	if (isClient())
	{
		std::vector<CString>* allowedVersions = server->getAllowedVersions();
		bool allowed = false;
		for (std::vector<CString>::iterator i = allowedVersions->begin(); i != allowedVersions->end(); ++i)
		{
			CString ver = *i;
			if (ver.find(":") != -1)
			{
				CString ver1 = ver.readString(":").trim();
				CString ver2 = ver.readString("").trim();
				int aVersion[2] = { getVersionID(ver1), getVersionID(ver2) };
				if (versionID >= aVersion[0] && versionID <= aVersion[1])
				{
					allowed = true;
					break;
				}
			}
			else
			{
				int aVersion = getVersionID(ver);
				if (versionID == aVersion)
				{
					allowed = true;
					break;
				}
			}
		}
		if (!allowed)
		{
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Your client version is not allowed on this server.\rAllowed: " << *server->getAllowedVersionString());
			return false;
		}
	}

	// Verify login details with the serverlist.
	// TODO: localhost mode.
	if (!isNPCServer())
	{
		if (server->getServerList()->getConnected() == false)
		{
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "The login server is offline.  Try again later.");
			return false;
		}
		server->getServerList()->sendPacket(CString() >> (char)SVO_VERIACC2
			>> (char)accountName.length() << accountName
			>> (char)password.length() << password
			>> (short)id >> (char)type
			);
	}
	else
	{
		// Check if we supplied the correct password.
		CSettings* adminsettings = server->getAdminSettings();
		if (password != adminsettings->getStr("ns_password"))
		{
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Invalid password.");
			return false;
		}

		// If an NPC-Server is already logged into the server, disconnect.
		// TODO: Should we boot the other one off?
		if (server->hasNPCServer())
		{
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "An NPC-Server is already logged in.");
			return false;
		}

		if (!sendLogin())
		{
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "NPC-Server failed to login.");
			return false;
		}

		// NPC-Server has full rights.
		this->adminRights = 0xFFFFF;

		// NPC-Server
		int port = pPacket.readGShort();
		server->setNPCServer(this, port);

		// Send Levels & Weapons
		this->sendNC_Maps();
		this->sendNC_Levels();
		this->sendNC_Weapons();

		serverlog.out("[%s] :: NPC-Server connected on port: %d\n", server->getName().text(), port);
	}

	return true;
}

bool TPlayer::msgPLI_LEVELWARP(CString& pPacket)
{
	time_t modTime = 0;

	if (pPacket[0] - 32 == PLI_LEVELWARPMOD)
		modTime = (time_t)pPacket.readGUInt5();

	float loc[2] = {(float)(pPacket.readGChar() / 2.0f), (float)(pPacket.readGChar() / 2.0f)};
	CString newLevel = pPacket.readString("");
	warp(newLevel, loc[0], loc[1], modTime);

	return true;
}

bool TPlayer::msgPLI_BOARDMODIFY(CString& pPacket)
{
	CSettings* settings = server->getSettings();
	signed char loc[2] = {pPacket.readGChar(), pPacket.readGChar()};
	signed char dim[2] = {pPacket.readGChar(), pPacket.readGChar()};
	CString tiles = pPacket.readString("");

	// Alter level data.
	if (level->alterBoard(tiles, loc[0], loc[1], dim[0], dim[1], this))
		server->sendPacketToLevel(CString() >> (char)PLO_BOARDMODIFY << (pPacket.text() + 1), 0, level);

	if (loc[0] < 0 || loc[0] > 63 || loc[1] < 0 || loc[1] > 63) return true;

	// Older clients drop items clientside.
	if (versionID < CLVER_2_1)
		return true;

	// Lay items when you destroy objects.
	short oldTile = (getLevel()->getTiles())[loc[0] + (loc[1] * 64)];
	int dropItem = -1;
	bool bushitems = settings->getBool("bushitems", true);
	bool vasesdrop = settings->getBool("vasesdrop", true);
	int tiledroprate = settings->getInt("tiledroprate", 50);

	// Bushes, grass, swamp.
	if ((oldTile == 2 || oldTile == 0x1a4 || oldTile == 0x1ff ||
		oldTile == 0x3ff) && bushitems)
	{
		if ( tiledroprate > 0 )
		{
			if ( (rand() % 100) < tiledroprate )
			{
				int index = rand() % 6;
				dropItem = index;
			}
		}
	}
	// Vase.
	else if (oldTile == 0x2ac && vasesdrop)
		dropItem = 5;

	// Send the item now.
	// TODO: Make this a more generic function.
	if (dropItem >= 0)
	{
		CString packet = CString() >> (char)(loc[0] * 2) >> (char)(loc[1] * 2) >> (char)dropItem;
		CString packet2 = CString() >> (char)PLI_ITEMADD << packet;
		packet2.readGChar();		// So msgPLI_ITEMADD works.

		msgPLI_ITEMADD(packet2);
		sendPacket(CString() >> (char)PLO_ITEMADD << packet);
	}

	return true;
}

bool TPlayer::msgPLI_PLAYERPROPS(CString& pPacket)
{
	setProps(pPacket, true);
	return true;
}

bool TPlayer::msgPLI_NPCPROPS(CString& pPacket)
{
	if (server->hasNPCServer()) return true;

	unsigned int npcId = pPacket.readGUInt();
	CString npcProps = pPacket.readString("");

	//printf( "npcId: %d\n", npcId );
	//printf( "pPacket: %s\n", npcProps.text());
	//for (int i = 0; i < pPacket.length(); ++i) printf( "%02x ", (unsigned char)pPacket[i] );
	//printf( "\n" );

	TNPC* npc = server->getNPC(npcId);
	if (npc == 0)
		return true;

	if (npc->getLevel() != level)
		return true;

	CString packet = CString() >> (char)PLO_NPCPROPS << (pPacket.text() + 1);
	server->sendPacketToLevel(packet, pmap, this, false, true);
	npc->setProps(npcProps, versionID);

	return true;
}

bool TPlayer::msgPLI_BOMBADD(CString& pPacket)
{
	float loc[2] = {(float)pPacket.readGChar() / 2.0f, (float)pPacket.readGChar() / 2.0f};
	unsigned char player_power = pPacket.readGUChar();
	unsigned char player = player_power >> 2;
	unsigned char power = player_power & 0x03;
	unsigned char timeToExplode = pPacket.readGUChar();		// How many 0.05 sec increments until it explodes.  Defaults to 55 (2.75 seconds.)

	//for (int i = 0; i < pPacket.length(); ++i) printf( "%02x ", (unsigned char)pPacket[i] ); printf( "\n" );
	server->sendPacketToLevel(CString() >> (char)PLO_BOMBADD >> (short)id << (pPacket.text() + 1), 0, level, this);
	return true;
}

bool TPlayer::msgPLI_BOMBDEL(CString& pPacket)
{
	server->sendPacketToLevel(CString() >> (char)PLO_BOMBDEL << (pPacket.text() + 1), 0, level, this);
	return true;
}

bool TPlayer::msgPLI_TOALL(CString& pPacket)
{
	// Check if the player is in a jailed level.
	std::vector<CString> jailList = server->getSettings()->getStr("jaillevels").tokenize(",");
	for (std::vector<CString>::iterator i = jailList.begin(); i != jailList.end(); ++i)
		if (i->trim() == levelName) return true;

	CString message = pPacket.readString(pPacket.readGUChar());

	// Word filter.
	int filter = server->getWordFilter()->apply(this, message, FILTER_CHECK_TOALL);
	if (filter & FILTER_ACTION_WARN)
	{
		setChat(message);
		return true;
	}

	std::vector<TPlayer*>* playerList = server->getPlayerList();
	for (std::vector<TPlayer*>::iterator i = playerList->begin(); i != playerList->end(); ++i)
	{
		TPlayer* player = *i;
		if (player == this) continue;

		// See if the player is allowing toalls.
		unsigned char flags = strtoint(player->getProp(PLPROP_ADDITFLAGS));
		if (flags & PLFLAG_NOTOALL) continue;

		player->sendPacket(CString() >> (char)PLO_TOALL >> (short)id >> (char)message.length() << message);
	}
	return true;
}

bool TPlayer::msgPLI_HORSEADD(CString& pPacket)
{
	server->sendPacketToLevel(CString() >> (char)PLO_HORSEADD << (pPacket.text() + 1), 0, level, this);

	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};
	unsigned char dir_bush = pPacket.readGUChar();
	char hdir = dir_bush & 0x03;
	char hbushes = dir_bush >> 2;
	CString image = pPacket.readString("");

	level->addHorse(image, loc[0], loc[1], hdir, hbushes);
	return true;
}

bool TPlayer::msgPLI_HORSEDEL(CString& pPacket)
{
	server->sendPacketToLevel(CString() >> (char)PLO_HORSEDEL << (pPacket.text() + 1), 0, level, this);

	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};

	level->removeHorse(loc[0], loc[1]);
	return true;
}

bool TPlayer::msgPLI_ARROWADD(CString& pPacket)
{
	server->sendPacketToLevel(CString() >> (char)PLO_ARROWADD >> (short)id << (pPacket.text() + 1), 0, level, this);
	return true;
}

bool TPlayer::msgPLI_FIRESPY(CString& pPacket)
{
	server->sendPacketToLevel(CString() >> (char)PLO_FIRESPY >> (short)id << (pPacket.text() + 1), 0, level, this);
	return true;
}

bool TPlayer::msgPLI_THROWCARRIED(CString& pPacket)
{
	// TODO: Remove when an npcserver is created.
	if (server->getSettings()->getBool("duplicatecanbecarried", false) == false)
	{
		TNPC* npc = 0;
		if (carryNpcId != 0) npc = server->getNPC(carryNpcId);
		if (npc != 0)
		{
			carryNpcThrown = true;

			// Add the NPC back to the level if it never left.
			if (npc->getLevel() == level)
				level->addNPC(npc);
		}
	}
	server->sendPacketToLevel(CString() >> (char)PLO_THROWCARRIED >> (short)id << (pPacket.text() + 1), 0, level, this);
	return true;
}

bool TPlayer::msgPLI_ITEMADD(CString& pPacket)
{
	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};
	unsigned char item = pPacket.readGUChar();

	level->addItem(loc[0], loc[1], item);
	server->sendPacketToLevel(CString() >> (char)PLO_ITEMADD << (pPacket.text() + 1), 0, level, this);
	return true;
}

bool TPlayer::msgPLI_ITEMDEL(CString& pPacket)
{
	server->sendPacketToLevel(CString() >> (char)PLO_ITEMDEL << (pPacket.text() + 1), 0, level, this);

	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};

	// Remove the item from the level, getting the type of the item in the process.
	char item = level->removeItem(loc[0], loc[1]);
	if (item == -1) return true;

	// If this is a PLI_ITEMTAKE packet, give the item to the player.
	if (pPacket[0] - 32 == PLI_ITEMTAKE)
		this->setProps(CString() << TLevelItem::getItemPlayerProp(item, this), true, true);

	return true;
}

bool TPlayer::msgPLI_CLAIMPKER(CString& pPacket)
{
	// Get the player who killed us.
	unsigned int pId = pPacket.readGUShort();
	TPlayer* player = server->getPlayer(pId);
	if (player == 0 || player == this) return true;

	// Sparring zone rating code.
	// Uses the glicko rating system.
	if (level == 0) return true;
	if (level->isSparringZone())
	{
		// Get some stats we are going to use.
		// Need to parse the other player's PLPROP_RATING.
		unsigned int otherRating = player->getProp(PLPROP_RATING).readGUInt();
		float oldStats[4] = { rating, deviation, (float)((otherRating >> 9) & 0xFFF), (float)(otherRating & 0x1FF) };

		// If the IPs are the same, don't update the rating to prevent cheating.
		if (CString(playerSock->getRemoteIp()) == CString(player->getSocket()->getRemoteIp()))
			return true;

		float gSpar[2] = {1.0f / pow((1.0f+3.0f*pow(0.0057565f,2)*(pow(oldStats[3],2))/pow(3.14159265f,2)),0.5f),	//Winner
					  	  1.0f / pow((1.0f+3.0f*pow(0.0057565f,2)*(pow(oldStats[1],2))/pow(3.14159265f,2)),0.5f)};	//Loser
		float ESpar[2] = {1.0f / (1.0f + pow(10.0f,(-gSpar[1]*(oldStats[2]-oldStats[0])/400.0f))),					//Winner
						  1.0f / (1.0f + pow(10.0f,(-gSpar[0]*(oldStats[0]-oldStats[2])/400.0f)))};					//Loser
		float dSpar[2] = {1.0f / (pow(0.0057565f,2)*pow(gSpar[0],2)*ESpar[0]*(1.0f-ESpar[0])),						//Winner
						  1.0f / (pow(0.0057565f,2)*pow(gSpar[1],2)*ESpar[1]*(1.0f-ESpar[1]))};						//Loser

		float tWinRating = oldStats[2] + (0.0057565f / ( 1.0f / pow(oldStats[3],2) + 1.0f/dSpar[0])) * (gSpar[0] * (1.0f - ESpar[0]));
		float tLoseRating = oldStats[0] + (0.0057565f / ( 1.0f / pow(oldStats[1],2) + 1.0f/dSpar[1])) * (gSpar[1] * (0.0f - ESpar[1]));
  		float tWinDeviation = pow((1.0f/(1.0f/pow(oldStats[3],2)+1/dSpar[0])),0.5f);
  		float tLoseDeviation = pow((1.0f/(1.0f/pow(oldStats[1],2)+1/dSpar[1])),0.5f);

		// Cap the rating.
		tWinRating = clip( tWinRating, 0.0f, 4000.0f );
		tLoseRating = clip( tLoseRating, 0.0f, 4000.0f );
		tWinDeviation = clip( tWinDeviation, 50.0f, 350.0f );
		tLoseDeviation = clip( tLoseDeviation, 50.0f, 350.0f );

		// Update the Ratings.
		// setProps will cause it to grab the new rating and send it to everybody in the level.
		// Therefore, just pass a dummy value.  setProps doesn't alter your rating for packet hacking reasons.
		if (oldStats[0] != tLoseRating || oldStats[1] != tLoseDeviation)
		{
			setRating((int)tLoseRating, (int)tLoseDeviation);
			this->setProps(CString() >> (char)PLPROP_RATING >> (int)0, true, true);
		}
		if (oldStats[2] != tWinRating || oldStats[3] != tWinDeviation)
		{
			player->setRating((int)tWinRating, (int)tWinDeviation);
			player->setProps(CString() >> (char)PLPROP_RATING >> (int)0, true, true);
		}
		this->setLastSparTime(time(0));
		player->setLastSparTime(time(0));
	}
	else
	{
		CSettings* settings = server->getSettings();

		// Give a kill to the player who killed me.
		if (settings->getBool("dontchangekills", false) == false)
			player->setKills(player->getProp(PLPROP_KILLSCOUNT).readGInt() + 1);

		// Now, adjust their AP if allowed.
		if (settings->getBool("apsystem", true))
		{
			char oAp = player->getProp(PLPROP_ALIGNMENT).readGChar();

			// If I have 20 or more AP, they lose AP.
			if (oAp > 0 && ap > 19)
			{
				int aptime[] = {settings->getInt("aptime0", 30), settings->getInt("aptime1", 90),
					settings->getInt("aptime2", 300), settings->getInt("aptime3", 600),
					settings->getInt("aptime4", 1200)};
				oAp -= (((oAp / 20) + 1) * (ap / 20));
				if (oAp < 0) oAp = 0;
				player->setApCounter((oAp < 20 ? aptime[0] : (oAp < 40 ? aptime[1] : (oAp < 60 ? aptime[2] : (oAp < 80 ? aptime[3] : aptime[4])))));
				player->setProps(CString() >> (char)PLPROP_ALIGNMENT >> (char)oAp, true, true);
			}
		}
	}

	return true;
}

bool TPlayer::msgPLI_BADDYPROPS(CString& pPacket)
{
	if (level == 0) return true;
	unsigned char id = pPacket.readGUChar();
	CString props = pPacket.readString("");

	// Get the baddy.
	TLevelBaddy* baddy = level->getBaddy(id);
	if (baddy == 0) return true;

	// Get the leader.
	TPlayer* leader = level->getPlayer(0);

	// Set the props and send to everybody in the level, except the leader.
	server->sendPacketToLevel(CString() >> (char)PLO_BADDYPROPS >> (char)baddy->getId() << props, 0, level, leader);
	baddy->setProps(props);
	return true;
}

bool TPlayer::msgPLI_BADDYHURT(CString& pPacket)
{
	TPlayer* leader = level->getPlayer(0);
	if (leader == 0) return true;
	leader->sendPacket(CString() >> (char)PLO_BADDYHURT << (pPacket.text() + 1));
	return true;
}

bool TPlayer::msgPLI_BADDYADD(CString& pPacket)
{
	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};
	unsigned char bType = pPacket.readGUChar();
	unsigned char bPower = pPacket.readGUChar();
	CString bImage = pPacket.readString("");
	bPower = MIN(bPower, 12);		// Hard-limit to 6 hearts.

	// Fix the image for 1.41 clients.
	if (!bImage.isEmpty() && getExtension(bImage).isEmpty())
		bImage << ".gif";

	// Add the baddy.
	TLevelBaddy* baddy = level->addBaddy(loc[0], loc[1], bType);
	if (baddy == 0) return true;

	// Set the baddy props.
	baddy->setRespawn(false);
	baddy->setProps(CString() >> (char)BDPROP_POWERIMAGE >> (char)bPower >> (char)bImage.length() << bImage);

	// Send the props to everybody in the level.
	server->sendPacketToLevel(CString() >> (char)PLO_BADDYPROPS >> (char)baddy->getId() << baddy->getProps(), 0, level);
	return true;
}

bool TPlayer::msgPLI_FLAGSET(CString& pPacket)
{
	CSettings* settings = server->getSettings();
	CString flagPacket = pPacket.readString("");
	CString flagName, flagValue;
	if (flagPacket.find("=") != -1)
	{
		flagName  = flagPacket.readString("=");
		flagValue = flagPacket.readString("");

		// If the value is empty, delete the flag instead.
		if (flagValue.isEmpty())
		{
			pPacket.setRead(1);	// Don't let us read the packet ID.
			return msgPLI_FLAGDEL(pPacket);
		}
	}
	else flagName = flagPacket;

	// Add a little hack for our special gr.strings.
	if (flagName.find("gr.") != -1)
	{
		if (settings->getBool("flaghack_movement", true) == true)
		{
			// gr.x and gr.y are used by the -gr_movement NPC to help facilitate smoother
			// movement amongst pre-2.3 clients.
			if (flagName == "gr.x")
			{
				if (versionID >= CLVER_2_3) return true;
				float pos = (float)atof(flagValue.text());
				if (pos != x)
					grMovementPackets >> (char)PLPROP_X >> (char)(pos * 2.0f) << "\n";
				return true;
			}
			else if (flagName == "gr.y")
			{
				if (versionID >= CLVER_2_3) return true;
				float pos = (float)atof(flagValue.text());
				if (pos != y)
					grMovementPackets >> (char)PLPROP_Y >> (char)(pos * 2.0f) << "\n";
				return true;
			}
			else if (flagName == "gr.z")
			{
				if (versionID >= CLVER_2_3) return true;
				float pos = (float)atof(flagValue.text());
				if (pos != z)
					grMovementPackets >> (char)PLPROP_Z >> (char)((pos + 0.5f) + 50.0f) << "\n";
				return true;
			}
		}
	}

	// 2.171 clients didn't support this.strings and tried to set them as a
	// normal flag.  Don't allow that.
	if (flagName.find("this.") != -1) return true;

	// Don't allow anybody to set read-only strings.
	if (flagName.find("clientr.") != -1) return true;
	if (flagName.find("serverr.") != -1) return true;

	// Server flags are handled differently than client flags.
	if (flagName.find("server.") != -1)
	{
		server->setFlag(flagName, flagValue);
		return true;
	}

	// Set Flag
	this->setFlag(flagName, flagValue, false, true);
	return true;
}

bool TPlayer::msgPLI_FLAGDEL(CString& pPacket)
{
	CString flagPacket = pPacket.readString("");
	CString flagName;
	if (flagPacket.find("=") != -1)
		flagName = flagPacket.readString("=").trim();
	else flagName = flagPacket;

	// this.flags should never be in any server flag list, so just exit.
	if (flagName.find("this.") != -1) return true;

	// Don't allow anybody to alter read-only strings.
	if (flagName.find("clientr.") != -1) return true;
	if (flagName.find("serverr.") != -1) return true;

	// Server flags are handled differently than client flags.
	// TODO: check serveroptions
	if (flagName.find("server.") != -1)
	{
		server->deleteFlag(flagName);
		return true;
	}

	// Remove Flag
	this->deleteFlag(flagName);
	return true;
}

bool TPlayer::msgPLI_OPENCHEST(CString& pPacket)
{
	unsigned char cX = pPacket.readGUChar();
	unsigned char cY = pPacket.readGUChar();
	std::vector<TLevelChest *>* levelChests = level->getLevelChests();

	for (std::vector<TLevelChest*>::iterator i = levelChests->begin(); i != levelChests->end(); ++i)
	{
		TLevelChest* chest = *i;
		if (chest->getX() == cX && chest->getY() == cY)
		{
			if (!hasChest(chest))
			{
				int chestItem = chest->getItemIndex();
				this->setProps(CString() << TLevelItem::getItemPlayerProp((char)chestItem, this), true, true);
				sendPacket(CString() >> (char)PLO_LEVELCHEST >> (char)1 >> (char)cX >> (char)cY);
				chestList.push_back(chest->getChestStr(levelName));
			}
		}
	}
	return true;
}

bool TPlayer::msgPLI_PUTNPC(CString& pPacket)
{
	// Disable if we have an NPC-Server.
	if (server->hasNPCServer())
		return true;

	CSettings* settings = server->getSettings();

	CString nimage = pPacket.readChars(pPacket.readGUChar());
	CString ncode = pPacket.readChars(pPacket.readGUChar());
	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};

	// See if putnpc is allowed.
	if (settings->getBool("putnpcenabled") == false)
		return true;

	// Load the code.
	CString code = server->getFileSystem(0)->load(ncode);
	code.removeAllI("\r");
	code.replaceAllI("\n", "\xa7");

	// Add NPC to level
	TNPC* npc = server->addNPC(nimage, code, loc[0], loc[1], level, true, true);

	return true;
}

bool TPlayer::msgPLI_NPCDEL(CString& pPacket)
{
	// Disable if we have an NPC-Server.
	if (server->hasNPCServer())
		return true;

	unsigned int nid = pPacket.readGUInt();

	// Remove the NPC.
	server->deleteNPC(nid, level);
	return true;
}

bool TPlayer::msgPLI_WANTFILE(CString& pPacket)
{
	// Get file.
	CString file = pPacket.readString("");

	// If we are the 1.41 client, make sure a file extension was sent.
	if (versionID < CLVER_2_1 && getExtension(file).isEmpty())
		file << ".gif";

	//printf("WANTFILE: %s\n", file.text());

	// Send file.
	this->sendFile(file);
	return true;
}

bool TPlayer::msgPLI_SHOWIMG(CString& pPacket)
{
	server->sendPacketToLevel(CString() >> (char)PLO_SHOWIMG >> (short)id << (pPacket.text() + 1), pmap, level, this);
	return true;
}

bool TPlayer::msgPLI_HURTPLAYER(CString& pPacket)
{
	unsigned short pId = pPacket.readGUShort();
	char hurtdx = pPacket.readGChar();
	char hurtdy = pPacket.readGChar();
	unsigned char power = pPacket.readGUChar();
	unsigned int npc = pPacket.readGUInt();

	// Get the victim.
	TPlayer* victim = server->getPlayer(pId);
	if (victim == 0) return true;

	// If they are paused, they don't get hurt.
	if (victim->getProp(PLPROP_STATUS).readGChar() & PLSTATUS_PAUSED) return true;

	// Send the packet.
	victim->sendPacket(CString() >> (char)PLO_HURTPLAYER >> (short)id >> (char)hurtdx >> (char)hurtdy >> (char)power >> (int)npc);

	return true;
}

bool TPlayer::msgPLI_EXPLOSION(CString& pPacket)
{
	CSettings* settings = server->getSettings();
	if (settings->getBool("noexplosions", false) == true) return true;

	unsigned char eradius = pPacket.readGUChar();
	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};
	unsigned char epower = pPacket.readGUChar();

	// Send the packet out.
	CString packet = CString() >> (char)PLO_EXPLOSION >> (short)id >> (char)eradius >> (char)(loc[0] * 2) >> (char)(loc[1] * 2) >> (char)epower;
	server->sendPacketToLevel(packet, pmap, this, false);

	return true;
}

bool TPlayer::msgPLI_PRIVATEMESSAGE(CString& pPacket)
{
	const int sendLimit = 4;
	if (isClient() && (int)difftime(time(0), lastMessage) <= 4)
	{
		sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE <<
			"Server message:\xa7You can only send messages once every " << CString((int)sendLimit) << " seconds.");
		return true;
	}
	lastMessage = time(0);

	// Check if the player is in a jailed level.
	std::vector<CString> jailList = server->getSettings()->getStr("jaillevels").tokenize(",");
	bool jailed = false;
	for (std::vector<CString>::iterator i = jailList.begin(); i != jailList.end(); ++i)
	{
		if (i->trim() == levelName)
		{
			jailed = true;
			break;
		}
	}

	// Get the players this message was addressed to.
	std::vector<unsigned short> pmPlayers;
	unsigned short pmPlayerCount = pPacket.readGUShort();
	for (int i = 0; i < pmPlayerCount; ++i)
		pmPlayers.push_back(pPacket.readGUShort());

	// Start constructing the message based on if it is a mass message or a private message.
	CString pmMessageType("\"\",");
	if (pmPlayerCount > 1) pmMessageType << "\"Mass message:\",";
	else pmMessageType << "\"Private message:\",";

	// Grab the message.
	CString pmMessage = pPacket.readString("");
	int messageLimit = 1024;
	if (pmMessage.length() > messageLimit)
	{
		sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE <<
			"Server message:\xa7There is a message limit of " << CString((int)messageLimit) << " characters.");
		return true;
	}

	// Word filter.
	if (isClient())
	{
		pmMessage.guntokenizeI();
		int filter = server->getWordFilter()->apply(this, pmMessage, FILTER_CHECK_PM);
		if (filter & FILTER_ACTION_WARN)
		{
			sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE <<
				"Word Filter:\xa7Your PM could not be sent because it was caught by the word filter.");
			return true;
		}
		pmMessage.gtokenizeI();
	}

	// Send the message out.
	for (std::vector<unsigned short>::iterator i = pmPlayers.begin(); i != pmPlayers.end(); ++i)
	{
		TPlayer* pmPlayer = server->getPlayer(*i);
		if (pmPlayer == 0 || pmPlayer == this) continue;

		// Don't send to people who don't want mass messages.
		if (pmPlayerCount != 1 && (pmPlayer->getProp(PLPROP_ADDITFLAGS).readGUChar() & PLFLAG_NOMASSMESSAGE))
			continue;

		// Jailed people cannot send PMs to normal players.
		if (jailed && !isStaff() && !pmPlayer->isStaff())
		{
			sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)pmPlayer->getId() << "\"Server Message:\"," << "\"From jail you can only send PMs to admins (RCs).\"");
			continue;
		}

		// Send the message.
		pmPlayer->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)id << pmMessageType << pmMessage);
	}

	return true;
}

bool TPlayer::msgPLI_NPCWEAPONDEL(CString& pPacket)
{
	CString weapon = pPacket.readString("");
	for (std::vector<CString>::iterator i = weaponList.begin(); i != weaponList.end(); )
	{
		if (*i == weapon)
		{
			i = weaponList.erase(i);

			// send to npc-server
			if (server->hasNPCServer())
				server->getNPCServer()->sendPacket(CString() >> (char)PLO_NPCWEAPONDEL >> (short)id >> (char)0 << weapon);
		}
		else ++i;
	}
	return true;
}

bool TPlayer::msgPLI_PACKETCOUNT(CString& pPacket)
{
	unsigned short count = pPacket.readGUShort();
	if (count != packetCount || packetCount > 10000)
	{
		serverlog.out("[%s] :: Warning - Player %s had an invalid packet count.\n", server->getName().text(), accountName.text());
	}
	packetCount = 0;

	return true;
}

bool TPlayer::msgPLI_WEAPONADD(CString& pPacket)
{
	// NPC-Server -- don't allow adding weapons
	if (server->hasNPCServer())
		return true;

	unsigned char type = pPacket.readGUChar();

	// Type 0 means it is a default weapon.
	if (type == 0)
	{
		this->addWeapon(pPacket.readGChar());
	}
	// NPC weapons.
	else
	{
		// Get the NPC id.
		unsigned int npcId = pPacket.readGUInt();
		TNPC* npc = server->getNPC(npcId);
		if (npc == 0)
			return true;

		// Get the name of the weapon.
		CString name = npc->getWeaponName();
		if (name.length() == 0)
			return true;

		// See if we can find the weapon in the server weapon list.
		TWeapon *weapon = server->getWeapon(name);

		// If weapon is 0, that means the weapon was not found.  Add the weapon to the list.
		if (weapon == 0)
		{
			weapon = new TWeapon(server, name, npc->getProp(NPCPROP_IMAGE).subString(1), npc->getProp(NPCPROP_SCRIPT).subString(2), npc->getLevel()->getModTime(), true);
			server->NC_AddWeapon(weapon);
		}

		// Check and see if the weapon has changed recently.  If it has, we should
		// send the new NPC to everybody on the server.  After updating the script, of course.
		if (weapon->getModTime() < npc->getLevel()->getModTime())
		{
			// Update Weapon
			weapon->updateWeapon(server, npc->getProp(NPCPROP_IMAGE).subString(1), npc->getClientScript(), npc->getLevel()->getModTime());
			
			// Send to Players
			server->NC_UpdateWeapon(weapon);
		}

		// Send the weapon to the player now.
		if (!hasWeapon(weapon->getName()))
			this->addWeapon(weapon);
	}
	return true;
}

bool TPlayer::msgPLI_UPDATEFILE(CString& pPacket)
{
	CFileSystem* fileSystem = server->getFileSystem();

	// Get the packet data and file mod time.
	time_t modTime = pPacket.readGUInt5();
	CString file = pPacket.readString("");
	time_t fModTime = fileSystem->getModTime(file);

	// If we are the 1.41 client, make sure a file extension was sent.
	if (versionID < CLVER_2_1 && getExtension(file).isEmpty())
		file << ".gif";

	//printf("UPDATEFILE: %s\n", file.text());

	// Make sure it isn't one of the default files.
	bool isDefault = false;
	for (unsigned int i = 0; i < sizeof(__defaultfiles) / sizeof(char*); ++i)
	{
		if (file.match(CString(__defaultfiles[i])) == true)
		{
			isDefault = true;
			break;
		}
	}

	// If the file on disk is different, send it to the player.
	file.setRead(0);
	if (isDefault == false && fModTime > modTime)
		return msgPLI_WANTFILE(file);

	if (versionID < CLVER_2_1)
		sendPacket(CString() >> (char)PLO_FILESENDFAILED << file);
	else sendPacket(CString() >> (char)PLO_FILEUPTODATE << file);
	return true;
}

bool TPlayer::msgPLI_ADJACENTLEVEL(CString& pPacket)
{
	time_t modTime = pPacket.readGUInt5();
	CString levelName = pPacket.readString("");
	CString packet;
	TLevel* adjacentLevel = TLevel::findLevel(levelName, server);

	if (adjacentLevel == 0)
		return true;

	bool alreadyVisited = false;
	for (std::vector<SCachedLevel*>::const_iterator i = cachedLevels.begin(); i != cachedLevels.end(); ++i)
	{
		SCachedLevel* cl = *i;
		if (cl->level == adjacentLevel)
		{
			alreadyVisited = true;
			break;
		}
	}

	// Send the level.
	if (versionID >= CLVER_2_1)
		sendLevel(adjacentLevel, modTime, true);
	else sendLevel141(adjacentLevel, modTime, true);

	// Set our old level back to normal.
	//sendPacket(CString() >> (char)PLO_LEVELNAME << level->getLevelName());
	if (pmap && pmap->getType() == MAPTYPE_GMAP)
		sendPacket(CString() >> (char)PLO_LEVELNAME << pmap->getMapName());
	else sendPacket(CString() >> (char)PLO_LEVELNAME << level->getLevelName());
	if (level->getPlayer(0) == this)
		sendPacket(CString() >> (char)PLO_ISLEADER);

	return true;
}

bool TPlayer::msgPLI_HITOBJECTS(CString& pPacket)
{
	float power = (float)pPacket.readGChar() / 2.0f;
	float loc[2] = {(float)pPacket.readGChar() / 2.0f, (float)pPacket.readGChar() / 2.0f};
	int nid = (pPacket.bytesLeft() != 0) ? pPacket.readGUInt() : -1;

	// Construct the packet.
	// {46}{SHORT player_id / 0 for NPC}{CHAR power}{CHAR x}{CHAR y}[{INT npc_id}]
	CString nPacket;
	nPacket >> (char)PLO_HITOBJECTS;
	nPacket >> (short)((nid == -1) ? id : 0);	// If it came from an NPC, send 0 for the id.
	nPacket >> (char)(power * 2) >> (char)(loc[0] * 2) >> (char)(loc[1] * 2);
	if (nid != -1) nPacket >> (int)nid;

	server->sendPacketToLevel(nPacket, pmap, this, true);
	return true;
}

bool TPlayer::msgPLI_LANGUAGE(CString& pPacket)
{
	language = pPacket.readString("");
	if (language.isEmpty())
		language = "English";
	return true;
}

bool TPlayer::msgPLI_TRIGGERACTION(CString& pPacket)
{
	unsigned int npcId = pPacket.readGUInt();
	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};
	CString action = pPacket.readString("").trim();
	CSettings* settings = server->getSettings();

	// (int)(loc[0]) % 64 == 0.0f, for gmap?
	if (loc[0] == 0.0f && loc[1] == 0.0f)
	{
		if (settings->getBool("triggerhack_weapons", false) == true)
		{
			if (action.find("gr.addweapon") == 0)
			{
				std::vector<CString> actionParts = action.tokenize(",");
				if (actionParts.size() != 1)
				{
					std::vector<CString>::iterator i = actionParts.begin();
					for (++i; i != actionParts.end(); ++i)
						this->addWeapon(i->trim());
				}
				return true;
			}
			else if (action.find("gr.deleteweapon") == 0)
			{
				std::vector<CString> actionParts = action.tokenize(",");
				if (actionParts.size() != 1)
				{
					std::vector<CString>::iterator i = actionParts.begin();
					for (++i; i != actionParts.end(); ++i)
						this->deleteWeapon(i->trim());
				}
				return true;
			}
		}

		if (settings->getBool("triggerhack_execscript", false) == true)
		{
			if (action.find("gr.es_clear") == 0)
			{
				// Clear the parameters.
				grExecParameterList.clear();
				return true;
			}
			else if (action.find("gr.es_set") == 0)
			{
				// Add the parameter to our saved parameter list.
				CString parameters = action.subString(9);
				if (grExecParameterList.isEmpty())
					grExecParameterList = parameters;
				else grExecParameterList << "," << parameters;
				return true;
			}
			else if (action.find("gr.es_append") == 0)
			{
				// Append doesn't add the beginning comma.
				CString parameters = action.subString(9);
				if (grExecParameterList.isEmpty())
					grExecParameterList = parameters;
				else grExecParameterList << parameters;
				return true;
			}
			else if (action.find("gr.es") == 0)
			{
				std::vector<CString> actionParts = action.tokenize(",");
				if (actionParts.size() != 1)
				{
					CString account = actionParts[1];
					CString wepname = CString() << "-gr_exec_" << removeExtension(actionParts[2]);
					CString wepimage = "wbomb1.png";

					// Load in all the execscripts.
					CFileSystem execscripts(server);
					execscripts.addDir("execscripts");
					CString wepscript = execscripts.load(actionParts[2]);

					// Check to see if we were able to load the weapon.
					if (wepscript.isEmpty())
					{
						serverlog.out("[%s] Error: Player %s tried to load execscript %s, but the script was not found.\n", server->getName().text(), accountName.text(), actionParts[2].text());
						return true;
					}

					// Format the weapon script properly.
					wepscript.removeAllI("\r");
					wepscript.replaceAllI("\n", "\xa7");

					// Replace parameters.
					std::vector<CString> parameters = grExecParameterList.tokenize(",");
					for (int i = 0; i < (int)parameters.size(); i++)
					{
						CString parmName = "*PARM" + CString(i);
						wepscript.replaceAllI(parmName, parameters[i]);
					}

					// Set all unreplaced parameters to 0.
					for (int i = 0; i < 128; i++)
					{
						CString parmName = "*PARM" + CString(i);
						wepscript.replaceAllI(parmName, "0");
					}

					// Create the weapon packet.
					CString weapon_packet = CString() >> (char)PLO_NPCWEAPONADD
							>> (char)wepname.length() << wepname
							>> (char)0 >> (char)wepimage.length() << wepimage
							>> (char)1 >> (short)wepscript.length() << wepscript;

					// Send it to the players now.
					if (actionParts[1] == "ALLPLAYERS")
						server->sendPacketTo(PLTYPE_ANYCLIENT, weapon_packet);
					else
					{
						TPlayer* p;
						p = server->getPlayer(actionParts[1], false);
						if (p) p->sendPacket(weapon_packet);
					}
					grExecParameterList.clear();
				}
				return true;
			}
		}

		if (settings->getBool("triggerhack_guilds", false) == true)
		{
			if (action.find("gr.addguildmember") == 0)
			{
				std::vector<CString> actionParts = action.tokenize(",");
				CString guild, account, nick;
				if (actionParts.size() > 1) guild = actionParts[1];
				if (actionParts.size() > 2) account = actionParts[2];
				if (actionParts.size() > 3) nick = actionParts[3];

				if (!guild.isEmpty() && !account.isEmpty())
				{
					// Read the guild list.
					CFileSystem guildFS(server);
					guildFS.addDir("guilds");
					CString guildList = guildFS.load(CString() << "guild" << guild << ".txt");

					if (guildList.find(account) == -1)
					{
						if (guildList[guildList.length() - 1] != '\n') guildList << "\n";
						guildList << account;
						if (!nick.isEmpty()) guildList << ":" << nick;

						guildList.save(CString() << server->getServerPath() << "guilds/guild" << guild << ".txt");
					}
				}
				return true;
			}
			else if (action.find("gr.removeguildmember") == 0)
			{
				std::vector<CString> actionParts = action.tokenize(",");
				CString guild, account;
				if (actionParts.size() > 1) guild = actionParts[1];
				if (actionParts.size() > 2) account = actionParts[2];

				if (!guild.isEmpty() && !account.isEmpty())
				{
					// Read the guild list.
					CFileSystem guildFS(server);
					guildFS.addDir("guilds");
					CString guildList = guildFS.load(CString() << "guild" << guild << ".txt");

					if (guildList.find(account) != -1)
					{
						int pos = guildList.find(account);
						int length = guildList.find("\n", pos) - pos;
						if (length < 0) length = -1;
						else ++length;

						guildList.removeI(pos, length);
						guildList.save(CString() << server->getServerPath() << "guilds/guild" << guild << ".txt");
					}
				}
				return true;
			}
			else if (action.find("gr.removeguild") == 0)
			{
				std::vector<CString> actionParts = action.tokenize(",");
				CString guild;
				if (actionParts.size() > 1) guild = actionParts[1];

				if (!guild.isEmpty())
				{
					// Read the guild list.
					CFileSystem guildFS(server);
					guildFS.addDir("guilds");
					CString path = guildFS.find(CString() << "guild" << guild << ".txt");

					// Remove the guild.
					remove(path.text());

					// Remove the guild from all players.
					for (std::vector<TPlayer*>::iterator i = server->getPlayerList()->begin(); i != server->getPlayerList()->end(); ++i)
					{
						TPlayer* p = *i;
						if (p->getGuild() == guild)
						{
							p->setNick(p->getNickname().readString("(").trimI());
							p->sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PLPROP_NICKNAME << p->getProp(PLPROP_NICKNAME));
							server->sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)p->getId() >> (char)PLPROP_NICKNAME << p->getProp(PLPROP_NICKNAME), p);
						}
					}
				}
				return true;
			}
			else if (action.find("gr.setguild") == 0)
			{
				std::vector<CString> actionParts = action.tokenize(",");
				CString guild, account;
				if (actionParts.size() > 1) guild = actionParts[1];
				if (actionParts.size() > 2) account = actionParts[2];

				if (!guild.isEmpty())
				{
					TPlayer* p = this;
					if (!account.isEmpty()) p = server->getPlayer(account, false);
					if (p)
					{
						CString nick = p->getNickname();
						p->setNick(CString() << nick.readString("(").trimI() << " (" << guild << ")", true);
						p->sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PLPROP_NICKNAME >> (char)p->getNickname().length() << p->getNickname());
						server->sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)p->getId() >> (char)PLPROP_NICKNAME >> (char)p->getNickname().length() << p->getNickname(), p);
					}
				}
				return true;
			}
		}

		if (settings->getBool("triggerhack_groups", true) == true)
		{
			if (action.find("gr.setgroup") == 0)
			{
				std::vector<CString> actionParts = action.tokenize(",");
				if (actionParts.size() == 2)
					levelGroup = actionParts[1];
				return true;
			}
			else if (action.find("gr.setlevelgroup") == 0)
			{
				std::vector<CString> actionParts = action.tokenize(",");
				if (actionParts.size() == 2)
				{
					std::vector<TPlayer*>* playerList = level->getPlayerList();
					for (std::vector<TPlayer*>::iterator i = playerList->begin(); i != playerList->end(); ++i)
					{
						TPlayer* player = *i;
						player->setGroup(actionParts[1]);
					}
				}
				return true;
			}
			else if (action.find("gr.setplayergroup") == 0)
			{
				std::vector<CString> actionParts = action.tokenize(",");
				if (actionParts.size() == 3)
				{
					TPlayer* player = server->getPlayer(actionParts[1], false);
					player->setGroup(actionParts[2]);
				}
			}
		}

		if (settings->getBool("triggerhack_files", false) == true)
		{
			if  (action.find("gr.appendfile") == 0)
			{
				int start = action.find(",") + 1;
				if (start == 0) return true;
				int finish = action.find(",", start) + 1;
				if (finish == 0) return true;

				// Assemble the file name.
				CString filename = action.subString(start, finish - start - 1);
				filename.removeAllI("../");
				filename.removeAllI("..\\");

				// Load the file.
				CString file;
				file.load(server->getServerPath() << "logs/" << filename);

				// Save the file.
				file << action.subString(finish) << "\r\n";
				file.save(server->getServerPath() << "logs/" << filename);
				return true;
			}
			else if (action.find("gr.writefile") == 0)
			{
				int start = action.find(",") + 1;
				if (start == 0) return true;
				int finish = action.find(",", start) + 1;
				if (finish == 0) return true;

				// Load the file.
				CString filename = action.subString(start, finish - start - 1);
				filename.removeAllI("../");
				filename.removeAllI("..\\");

				// Save the file.
				CString file = action.subString(finish) << "\r\n";
				file.save(server->getServerPath() << "logs/" << filename);
				return true;
			}
		}

		if (settings->getBool("triggerhack_rc", false) == true)
		{
			if (action.find("gr.rcchat") == 0)
			{
				int start = action.find(",");
				if (start != -1)
				{
					++start;
					server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << action.subString(start));
				}
				return true;
			}
		}

		if (settings->getBool("triggerhack_props", false) == true)
		{
			if (action.find("gr.attr") == 0)
			{
				int start = action.find(",");
				if (start != -1)
				{
					int attrNum = strtoint(action.subString(7, start - 7));
					if (attrNum > 0 && attrNum <= 30)
					{
						++start;
						CString val = action.subString(start);
						setProps(CString() >> (char)(__attrPackets[attrNum - 1]) >> (char)val.length() << val, true, true);
					}
				}
			}
		}

		if (settings->getBool("triggerhack_levels", false) == true)
		{
			if (action.find("gr.updatelevel") == 0)
			{
				int start = action.find(",");
				if (start != -1)
				{
					++start;
					CString levelName = action.subString(start).trim();
					if (levelName.isEmpty())
						level->reload();
					else
					{
						TLevel* targetLevel = server->getLevel(levelName);
						if (targetLevel != 0)
							targetLevel->reload();
					}
				}
				else level->reload();
			}
		}
	}

	// Send to the level.
	server->sendPacketToLevel(CString() >> (char)PLO_TRIGGERACTION >> (short)id << (pPacket.text() + 1), 0, level, this);

	// Send to the NPC-server.
	if (server->hasNPCServer())
		server->getNPCServer()->sendPacket(CString() >> (char)PLO_TRIGGERACTION >> (short)id << (pPacket.text() + 1));

	return true;
}

bool TPlayer::msgPLI_MAPINFO(CString& pPacket)
{
	// Don't know what this does exactly.  Might be gmap related.
	pPacket.readString("");
	return true;
}

bool TPlayer::msgPLI_SHOOT(CString& pPacket)
{
	int unknown = pPacket.readGInt();				// May be a shoot id for the npc-server.
	float loc[3] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};
	unsigned char sangle = pPacket.readGUChar();	// 0-pi = 0-220
	unsigned char sanglez = pPacket.readGUChar();	// 0-pi = 0-220
	unsigned char sspeed = pPacket.readGUChar();	// speed = pixels per 0.05 seconds.  In gscript, each value of 1 translates to 44 pixels.
	CString sgani = pPacket.readChars(pPacket.readGUChar());
	unsigned char unknown2 = pPacket.readGUChar();

	// Send data now.
	server->sendPacketToLevel(CString() >> (char)PLO_SHOOT >> (short)id << (pPacket.text() + 1), pmap, this, false);

//	printf("shoot: %s\n", pPacket.text());
//	for (int i = 0; i < pPacket.length(); ++i) printf("%02x ", (unsigned char)pPacket[i]); printf("\n");
	return true;
}

bool TPlayer::msgPLI_SERVERWARP(CString& pPacket)
{
	CString servername = pPacket.readString("");
	server->getServerList()->sendPacket(CString() >> (char)SVO_SERVERINFO >> (short)id << servername);
	return true;
}

bool TPlayer::msgPLI_PROCESSLIST(CString& pPacket)
{
	std::vector<CString> processes = pPacket.readString("").guntokenize().tokenize("\n");
	return true;
}

bool TPlayer::msgPLI_UNKNOWN46(CString& pPacket)
{
	printf("TODO: TPlayer::msgPLI_UNKNOWN46: ");
	CString packet = pPacket.readString("");
	for (int i = 0; i < packet.length(); ++i) printf( "%02x ", (unsigned char)packet[i] ); printf( "\n" );
	return true;
}

bool TPlayer::msgPLI_RAWDATA(CString& pPacket)
{
	nextIsRaw = true;
	rawPacketSize = pPacket.readGUInt();
	return true;
}

bool TPlayer::msgPLI_PROFILEGET(CString& pPacket)
{
	// Send the packet ID for backwards compatibility.
	server->getServerList()->sendPacket(CString() >> (char)SVO_GETPROF >> (short)id << pPacket);
	return true;
}

bool TPlayer::msgPLI_PROFILESET(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc != accountName) return true;

	// Old gserver would send the packet ID with pPacket so, for
	// backwards compatibility, do that here.
	server->getServerList()->sendPacket(CString() >> (char)SVO_SETPROF << pPacket);
	return true;
}

// IRC Stuff
bool TPlayer::msgPLI_SENDTEXT(CString& pPacket)
{
	// Client -> Return
	if (isClient())
		return false;

	return true;
}

bool TPlayer::msgPLI_UNKNOWN157(CString& pPacket)
{
	// v4 and up needs this for some reason.
	time_t mod = pPacket.readGUInt5();
	CString gani = pPacket.readString("");
	CString ganiData = server->getFileSystem()->load(CString() << gani << ".gani");
	if (!ganiData.isEmpty())
	{
		ganiData.readString("SETBACKTO");
		if (ganiData.bytesLeft())
		{
			CString backGani = ganiData.readString("\n").trim();
			sendPacket(CString() >> (char)PLO_UNKNOWN195 >> (char)gani.length() << gani << "\"SETBACKTO " << backGani << "\"");
			return true;
		}
	}
	sendPacket(CString() >> (char)PLO_UNKNOWN195 >> (char)gani.length() << gani << "\"SETBACKTO \"");
	return true;
}
