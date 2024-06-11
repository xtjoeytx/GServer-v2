#include <cmath>
#include <ctime>
#include <sys/stat.h>
#include <cstdio>

#include <IConfig.h>
#include "utilities/stringutils.h"
#include "TPlayer.h"
#include "IUtil.h"
#include "TServer.h"
#include "TAccount.h"
#include "TLevel.h"
#include "TMap.h"
#include "TWeapon.h"
#include "TNPC.h"
#include "TPacket.h"

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>


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
	"pics1.png"
};
const char* __defaultbodies[] = {
	"body.png", "body2.png", "body3.png"
};
const char* __defaultswords[] = {
	"sword1.png", "sword2.png", "sword3.png", "sword4.png"
};
const char* __defaultshields[] = {
	"shield1.png", "shield2.png", "shield3.png"
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
	TPLFunc[PLI_REQUESTUPDATEBOARD] = &TPlayer::msgPLI_REQUESTUPDATEBOARD;
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
	TPLFunc[PLI_SHOOT2] = &TPlayer::msgPLI_SHOOT2;
	TPLFunc[PLI_SERVERWARP] = &TPlayer::msgPLI_SERVERWARP;

	TPLFunc[PLI_PROCESSLIST] = &TPlayer::msgPLI_PROCESSLIST;

	TPLFunc[PLI_UNKNOWN46] = &TPlayer::msgPLI_UNKNOWN46;
	TPLFunc[PLI_VERIFYWANTSEND] = &TPlayer::msgPLI_VERIFYWANTSEND;
	TPLFunc[PLI_UPDATECLASS] = &TPlayer::msgPLI_UPDATECLASS;
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
	TPLFunc[PLI_REQUESTTEXT] = &TPlayer::msgPLI_REQUESTTEXT;
	TPLFunc[PLI_SENDTEXT] = &TPlayer::msgPLI_SENDTEXT;
	TPLFunc[PLI_UPDATEGANI] = &TPlayer::msgPLI_UPDATEGANI;
	TPLFunc[PLI_UPDATESCRIPT] = &TPlayer::msgPLI_UPDATESCRIPT;
	TPLFunc[PLI_UPDATEPACKAGEREQUESTFILE] = &TPlayer::msgPLI_UPDATEPACKAGEREQUESTFILE;
	TPLFunc[PLI_RC_UNKNOWN162] = &TPlayer::msgPLI_RC_UNKNOWN162;

	// NPC-Server Functions
#ifdef V8NPCSERVER
	TPLFunc[PLI_NC_NPCGET] = &TPlayer::msgPLI_NC_NPCGET;
	TPLFunc[PLI_NC_NPCDELETE] = &TPlayer::msgPLI_NC_NPCDELETE;
	TPLFunc[PLI_NC_NPCRESET] = &TPlayer::msgPLI_NC_NPCRESET;
	TPLFunc[PLI_NC_NPCSCRIPTGET] = &TPlayer::msgPLI_NC_NPCSCRIPTGET;
	TPLFunc[PLI_NC_NPCWARP] = &TPlayer::msgPLI_NC_NPCWARP;
	TPLFunc[PLI_NC_NPCFLAGSGET] = &TPlayer::msgPLI_NC_NPCFLAGSGET;
	TPLFunc[PLI_NC_NPCSCRIPTSET] = &TPlayer::msgPLI_NC_NPCSCRIPTSET;
	TPLFunc[PLI_NC_NPCFLAGSSET] = &TPlayer::msgPLI_NC_NPCFLAGSSET;
	TPLFunc[PLI_NC_NPCADD] = &TPlayer::msgPLI_NC_NPCADD;
	TPLFunc[PLI_NC_CLASSEDIT] = &TPlayer::msgPLI_NC_CLASSEDIT;
	TPLFunc[PLI_NC_CLASSADD] = &TPlayer::msgPLI_NC_CLASSADD;
	TPLFunc[PLI_NC_LOCALNPCSGET] = &TPlayer::msgPLI_NC_LOCALNPCSGET;
	TPLFunc[PLI_NC_WEAPONLISTGET] = &TPlayer::msgPLI_NC_WEAPONLISTGET;
	TPLFunc[PLI_NC_WEAPONGET] = &TPlayer::msgPLI_NC_WEAPONGET;
	TPLFunc[PLI_NC_WEAPONADD] = &TPlayer::msgPLI_NC_WEAPONADD;
	TPLFunc[PLI_NC_WEAPONDELETE] = &TPlayer::msgPLI_NC_WEAPONDELETE;
	TPLFunc[PLI_NC_CLASSDELETE] = &TPlayer::msgPLI_NC_CLASSDELETE;
	TPLFunc[PLI_NC_LEVELLISTGET] = &TPlayer::msgPLI_NC_LEVELLISTGET;
#endif

	// Finished
	TPlayer::created = true;
}


/*
	Constructor - Deconstructor
*/
TPlayer::TPlayer(TServer* pServer, CSocket* pSocket, uint16_t pId)
: TAccount(pServer),
playerSock(pSocket), key(0),
os("wind"), codepage(1252),
id(pId), type(PLTYPE_AWAIT), versionID(CLVER_UNKNOWN),
carryNpcId(0), carryNpcThrown(false), loaded(false),
nextIsRaw(false), rawPacketSize(0), isFtp(false),
grMovementUpdated(false),
fileQueue(pSocket),
packetCount(0), firstLevel(true), invalidPackets(0), newProtocol(false)
#ifdef V8NPCSERVER
, _processRemoval(false)
#endif
{
	lastData = lastMovement = lastSave = last1m = time(nullptr);
	lastChat = lastMessage = lastNick = 0;
	isExternal = false;
	serverName = server->getName();
	nextExternalPlayerId = 16000;

	srand((unsigned int)time(nullptr));

	// Create Functions
	if (!TPlayer::created)
		TPlayer::createFunctions();
}

TPlayer::~TPlayer()
{
	cleanup();
}

void TPlayer::cleanup()
{
	if (playerSock == nullptr)
		return;

	// Send all unsent data (for disconnect messages and whatnot).
	fileQueue.sendCompress();

	if (id >= 0 && server != nullptr && loaded)
	{
		// Save account.
		if (isClient() && !isLoadOnly)
			saveAccount();

		// Remove from the level.
		if (!curlevel.expired()) leaveLevel();

		// Announce our departure to other clients.
		if (!isNC()) {
			server->sendPacketToType(PLTYPE_ANYCLIENT, {PLO_OTHERPLPROPS, CString() >> (short)id >> (char)PLPROP_PCONNECTED}, this);
			server->sendPacketToType(PLTYPE_ANYRC, {PLO_DELPLAYER, CString() >> (short)id}, this);
		}

		if (!accountName.isEmpty()) {
			if (isRC())
				server->sendPacketToType(PLTYPE_ANYRC, {PLO_RC_CHAT, CString() << "RC Disconnected: " << accountName}, this);
			else if (isNC())
				server->sendPacketToType(PLTYPE_ANYNC, {PLO_RC_CHAT, CString() << "NC Disconnected: " << accountName}, this);
		}

		// Log.
		if (isClient())
			serverlog.out("[%s] :: Client disconnected: %s\n", server->getName().text(), accountName.text());
		else if (isRC())
			serverlog.out("[%s] :: RC disconnected: %s\n", server->getName().text(), accountName.text());
		else if (isNC())
			serverlog.out("[%s] :: NC disconnected: %s\n", server->getName().text(), accountName.text());
	}

	// Clean up.
	cachedLevels.clear();
	spLevels.clear();

	delete playerSock;
	playerSock = nullptr;

#ifdef V8NPCSERVER
	if (_scriptObject) {
		_scriptObject.reset();
	}
#endif
}

bool TPlayer::onRecv()
{
	// If our socket is gone, delete ourself.
	if (playerSock == nullptr || playerSock->getState() == SOCKET_STATE_DISCONNECTED)
		return false;

	// Grab the data from the socket and put it into our receive buffer.
	unsigned int size = 0;
	char* data = playerSock->getData(&size);
	if (size != 0) {
		rBuffer.write(data, (int)size);
#if defined(WOLFSSL_ENABLED)
		if (this->playerSock->webSocket)
			if (webSocketFixIncomingPacket(rBuffer) < 0) return true;
#endif
	}
	else if (playerSock->getState() == SOCKET_STATE_DISCONNECTED)
		return false;

	// Do the main function.
	return doMain();

}

bool TPlayer::onSend()
{
	if (playerSock == nullptr || playerSock->getState() == SOCKET_STATE_DISCONNECTED)
		return false;

	// Send data.
	fileQueue.sendCompress();

	return true;
}

void TPlayer::onUnregister()
{
	// Called when onSend() or onRecv() returns false.
	server->deletePlayer(shared_from_this());
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

/* server recv callback */
int TPlayer::ServerRecv(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
	printf("ServerRecv\n");

	auto* instance = (TPlayer*)(wolfSSL_get_ex_data(ssl, 0));
	if (instance && instance->playerSock->getState() == SOCKET_STATE_CONNECTED) {
		if (instance->rBuffer.bytesLeft() > 0) {

		} else {
			char* data = instance->playerSock->getData(reinterpret_cast<unsigned int *>(&sz));
			if (sz != 0) {
				instance->rBuffer.clear();
				instance->rBuffer.write(data, (int)sz);
#if defined(WOLFSSL_ENABLED)
				if (instance->playerSock->webSocket)
					if (webSocketFixIncomingPacket(instance->rBuffer) < 0) return true;
#endif
			}
		}
		memcpy(buf, instance->rBuffer.readChars(sz).text(), sz);
		return sz;
	}

	return -1;
}

/* client send callback */
int TPlayer::ServerSend(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
	printf("ServerSend\n");

	auto* instance = (TPlayer*)(wolfSSL_get_ex_data(ssl, 0));
	if (instance && instance->playerSock->getState() == SOCKET_STATE_CONNECTED) {
		return instance->playerSock->sendData(buf, (unsigned int*)&sz);

		return sz;
	}

	return -1;
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
#if defined(WOLFSSL_ENABLED)
		if (!this->playerSock->webSocket && rBuffer.findi("GET /") > -1 && rBuffer.findi("HTTP/1.1\r\n") > -1)
		{

			CString webSocketKeyHeader = "Sec-WebSocket-Key:";
			if (rBuffer.findi(webSocketKeyHeader) < 0) {
				CString simpleHtml = CString() << "<html><head><title>" APP_VENDOR " " APP_NAME " v" APP_VERSION "</title></head><body><h1>Welcome to " << server->getSettings().getStr("name") << "!</h1>" << server->getServerMessage().replaceAll("my server", server->getSettings().getStr("name")).text() << "<p style=\"font-style: italic;font-weight: bold;\">Powered by " APP_VENDOR " " APP_NAME "<br/>Programmed by " << CString(APP_CREDITS) << "</p></body></html>";
				CString webResponse = CString() << "HTTP/1.1 200 OK\r\nServer: " APP_VENDOR " " APP_NAME " v" APP_VERSION "\r\nContent-Length: " << CString(simpleHtml.length()) << "\r\nContent-Type: text/html\r\n\r\n" << simpleHtml << "\r\n";
				unsigned int dsize = webResponse.length();
				this->playerSock->sendData(webResponse.text(), &dsize);
				return false;
			}
			this->playerSock->webSocket = true;
			// Get the WebSocket handshake key
			rBuffer.setRead(rBuffer.findi(webSocketKeyHeader));
			CString webSocketKey = rBuffer.readString("\r").subString(webSocketKeyHeader.length()+1).trimI();

			// Append GUID
			webSocketKey << "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

			// Calculate sha1 has of key + GUID and base64 encode it for sending back
			webSocketKey.sha1I().base64encodeI();
			webSocketKeyHeader.clear();

			CString webSockHandshake = CString() <<"HTTP/1.1 101 Switching Protocols\r\n"
											 << "Upgrade: websocket\r\n"
											 << "Connection: Upgrade\r\n"
											 << "Sec-WebSocket-Protocol: binary\r\n"
											 << "Sec-WebSocket-Accept: "
											 << webSocketKey
											 << "\r\n\r\n";

			unsigned int dsize = webSockHandshake.length();

			this->playerSock->sendData(webSockHandshake.text(), &dsize);

			rBuffer.removeI(0, rBuffer.length());
			return true;
		}
#endif
		// New data.
		lastData = time(nullptr);
		if (packetCount == 0)
		{
			packetCount++;
			if (rBuffer.bytesLeft() >= 8)
			{
				if (rBuffer.subString(0, 8) == "GNP1905C")
				{
					serverlog.out("[%s] New Protocol client connected! Sending weapons!\n", server->getName().text());
					in_codec.setGen(ENCRYPT_GEN_6);
					fileQueue.setCodec(ENCRYPT_GEN_6, 0);
					newProtocol = true;

					for (const auto& weapon : server->getWeaponList()) {
						for (const auto& packet : weapon.second->getWeaponPackets(CLVER_4_0211)) {
							sendPacket(packet, true);
						}
						sendPacket({PLO_NPCWEAPONSCRIPT, weapon.second->getByteCode()}, true);
					}

					rBuffer.removeI(0, 8);
					return true;
				}
			}
		}

		if (newProtocol) {
			rBuffer.removeI(0, rBuffer.length());
			break;
		}

		rBuffer.save("indata.bin");
		if (rBuffer[0] == 0x16) {
			wolfSSL_Debugging_ON();
			char *hostname = nullptr;

			WOLFSSL_METHOD *method;
			/* Initialize wolfSSL library */
			wolfSSL_Init();

			/* Get encryption method */
			method = wolfTLS_server_method();

			/* Create wolfSSL_CTX */
			if ((ctx = wolfSSL_CTX_new(method)) == nullptr) {
				printf("wolfSSL_CTX_new error\n");

				return false;
			}

			if (wolfSSL_CTX_set_cipher_list(ctx, "ALL") != SSL_SUCCESS) {
				printf("Error setting cipher suites.\n");

				return false;
			}

			/* Load server certs into ctx */
			if (wolfSSL_CTX_use_certificate_file(ctx, "certs/server-cert.pem", SSL_FILETYPE_PEM) != SSL_SUCCESS) {
				printf("Error loading certs/server-cert.pem\n");

				return false;
			}

			/* Load server key into ctx */
			if (wolfSSL_CTX_use_PrivateKey_file(ctx, "certs/server-key.pem", SSL_FILETYPE_PEM) != SSL_SUCCESS) {
				printf("Error loading certs/server-key.pem\n");

				return false;
			}

			wolfSSL_SetIOSend(ctx, ServerSend);
			wolfSSL_SetIORecv(ctx, ServerRecv);

			/* Create wolfSSL object */
			if ((ssl = wolfSSL_new(ctx)) == nullptr) {
				printf("wolfSSL_new error\n");

				return false;
			}

			iBuffer = rBuffer;
			//rBuffer.removeI(0, rBuffer.length());

			wolfSSL_set_ex_data(ssl, 0, this);

			/* accept tls connection without tcp sockets */
			int ret = wolfSSL_accept(ssl);
			if (ret != SSL_SUCCESS) {
				int err = wolfSSL_get_error(ssl, ret);
				const char *errString = wolfSSL_ERR_reason_error_string(err);
				printf("SSL/TLS handshake error: %s\n", errString);
				// Additional error handling code
				WOLFSSL_CIPHER *currentCipher = wolfSSL_get_current_cipher(ssl);
				if (currentCipher != nullptr) {
					const char *cipherSuiteName = wolfSSL_CIPHER_get_name(currentCipher);
					char availableCiphers[1024];
					wolfSSL_get_ciphers(availableCiphers, 1024);
					printf("Client requested cipher suite: %s\nciphers: %s\n", cipherSuiteName, availableCiphers);
					printf("\n");
				}
				return false;
			} else {
				return true;
			}
		}

		if (ssl != nullptr) {
			iBuffer = rBuffer;
			//rBuffer.removeI(0, rBuffer.length());

			unsigned char buf[80];
			memset(buf, 0, sizeof(buf));
			int ret = wolfSSL_read(ssl, buf, sizeof(buf)-1);
			if (ret != SSL_SUCCESS) {
				int err = wolfSSL_get_error(ssl, ret);
				const char *errString = wolfSSL_ERR_reason_error_string(err);
				printf("SSL/TLS error: %s\n", errString);

				return false;
			}
			rBuffer << *buf;
			printf("client msg = %s\n", buf);
		}
		// packet length
		auto len = (unsigned short)rBuffer.readShort();
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
			// Uncompress, so we can properly decrypt later on.
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

		// well there's your buffer
		if (!parsePacket(unBuffer))
			return false;
	}

	// Update the -gr_movement packets.
	if (!grMovementPackets.isEmpty())
	{
		if (!grMovementUpdated)
		{
			std::vector<CString> pack = grMovementPackets.tokenize("\n");
			for (auto & i : pack)
				setProps(i, PLSETPROPS_FORWARD);
		}
		grMovementPackets.clear(42);
	}
	grMovementUpdated = false;

	server->getSocketManager().updateSingle(this, false, true);
	return true;
}

bool TPlayer::doTimedEvents()
{
	time_t currTime = time(nullptr);

	// If we are disconnected, delete ourself!
	if (playerSock == nullptr || playerSock->getState() == SOCKET_STATE_DISCONNECTED)
	{
		server->deletePlayer(shared_from_this());
		return false;
	}

	// Only run for clients.
	if (!isClient()) return true;

	// Increase online time.
	onlineTime++;

	// Disconnect if players are inactive.
	CSettings& settings = server->getSettings();
	if (settings.getBool("disconnectifnotmoved"))
	{
		int maxnomovement = settings.getInt("maxnomovement", 1200);
		if (((int)difftime(currTime, lastMovement) > maxnomovement) && ((int)difftime(currTime, lastChat) > maxnomovement))
		{
			serverlog.out("[%s] Client %s has been disconnected due to inactivity.\n", server->getName().text(), accountName.text());
			sendPacket({PLO_DISCMESSAGE, CString() << "You have been disconnected due to inactivity."});
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
	if (settings.getBool("apsystem") && !curlevel.expired())
	{
		auto level = getLevel();
		if (level)
		{
			if (!(status & PLSTATUS_PAUSED) && !level->isSparringZone())
				apCounter--;

			if (apCounter <= 0)
			{
				if (ap < 100)
				{
					ap++;
					setProps(CString() >> (char)PLPROP_ALIGNMENT >> (char)ap, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
				}
				if (ap < 20) apCounter = settings.getInt("aptime0", 30);
				else if (ap < 40) apCounter = settings.getInt("aptime1", 90);
				else if (ap < 60) apCounter = settings.getInt("aptime2", 300);
				else if (ap < 80) apCounter = settings.getInt("aptime3", 600);
				else apCounter = settings.getInt("aptime4", 1200);
			}
		}
	}

	// Do singleplayer level events.
	{
		for (auto& spLevel : spLevels)
		{
			auto& level = spLevel.second;
			if (level)
				level->doTimedEvents();
		}
	}

	// Save player account every 5 minutes.
	if ((int)difftime(currTime, lastSave) > 300)
	{
		lastSave = currTime;
		if (isClient() && loaded && !isLoadOnly) saveAccount();
	}

	// Events that happen every minute.
	if ((int)difftime(currTime, last1m) > 60)
	{
		last1m = currTime;
		invalidPackets = 0;
	}

	return true;
}

void TPlayer::disconnect()
{
	server->deletePlayer(shared_from_this());
	//server->getSocketManager()->unregisterSocket(this);
}

bool TPlayer::parsePacket(CString& pPacket)
{
	// First packet is always unencrypted zlib.  Read it in a special way.
	if (type == PLTYPE_AWAIT)
	{
		packetCount++;
		if ( !msgPLI_LOGIN(CString() << pPacket.readString("\n")))
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
		unsigned char packetId = curPacket.readGUChar();

		// RC version 1.1 adds a "\n" string to the end of file uploads instead of a newline character.
		// This causes issues because it messes with the packet order.
		if (isRC() && versionID == RCVER_1_1 && packetId == PLI_RC_FILEBROWSER_UP)
		{
			curPacket.removeI(curPacket.length() - 1);
			curPacket.setRead(1);
			pPacket.readChar();	// Read out the n that got left behind.
		}

		// Call the function assigned to the packet id.
		packetCount++;
		//printf("Packet: (%i) %s\n", id, curPacket.text() + 1);

		// Forwards packets from server back to client as rc chat (for debugging)
		//sendPacket(CString() >> (char)PLO_RC_CHAT << "Server Data [" << CString(id) << "]:" << (curPacket.text() + 1));
		if (!(*this.*TPLFunc[packetId])(curPacket))
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
	else if (in_codec.getGen() == ENCRYPT_GEN_5)
	{
		// Find the compression type and remove it.
		int pType = (uint8_t)pPacket.readChar();
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

void TPlayer::sendPacketNewProtocol(unsigned char packetId, const CString& pPacket, bool sendNow, bool appendNL)
{
	serverlog.out("[%s][%i] %s\n", server->getName().text(), (int)packetId, pPacket.text());

	// We ignore appendNL here because new protocol doesn't end with newlines
	CString buf2 = CString() << (char)0 << (char)packetCount;
	packetCount++;
	buf2.writeInt3(pPacket.length()+6, false);
	buf2.writeChar((char)packetId, false);
	buf2.write(pPacket.text(), pPacket.length(), true);

	if (sendNow)
	{
		if (playerSock->webSocket)
			webSocketFixOutgoingPacket(buf2);
		unsigned int dsize = buf2.length();
		playerSock->sendData(buf2.text(), &dsize);
	}
	else
	{
		// append buffer
		fileQueue.addPacket(buf2);
	}
}

void TPlayer::sendPacketOldProtocol(CString pPacket, bool appendNL)
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
	// Add the filename to the list of known files, so we can resend the file
	// to the client if it gets changed after it was originally sent
	if (isClient())
		knownFiles.insert(pFile.toString());

	CFileSystem* fileSystem = server->getFileSystem();

	// Find file.
	CString path = fileSystem->find(pFile);
	if (path.isEmpty())
	{
		sendPacket({PLO_FILESENDFAILED, CString() << pFile});

		return false;
	}

	// Strip filename from the path.
	path.removeI(path.findl(CFileSystem::getPathSeparator()) + 1);
	if (path.find(server->getServerPath()) != -1)
		path.removeI(0, server->getServerPath().length());

	// Send the file now.
	return sendFile(path, pFile);
}

bool TPlayer::sendFile(const CString& pPath, const CString& pFile)
{
	CString filepath = server->getServerPath() << pPath << pFile;
	CString fileData;
	fileData.load(filepath);

	time_t modTime = 0;
	struct stat fileStat{};
	if (stat(filepath.text(), &fileStat) != -1)
		modTime = fileStat.st_mtime;

	// See if the file exists.
	if (fileData.length() == 0)
	{
		sendPacket({PLO_FILESENDFAILED, CString() << pFile});

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
			sendPacket({PLO_FILESENDFAILED, CString() << pFile});
			return false;
		}
		isBigFile = false;
	}

	// If we are sending a big file, let the client know now.
	if (isBigFile)
	{
		sendPacket({PLO_LARGEFILESTART, CString() << pFile});
		sendPacket({PLO_LARGEFILESIZE, CString() >> (long long)fileData.length()});
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
			if (!newProtocol)
				sendPacket({PLO_RAWDATA, CString() >> (int)(packetLength - 1 + sendSize)});
			sendPacket({PLO_FILE, CString() >> (char)pFile.length() << pFile << fileData.subString(0, sendSize)}, false);
		}
		else
		{
			if (!newProtocol)
				sendPacket({PLO_RAWDATA, CString() >> (int)(packetLength + sendSize)});
			sendPacket({PLO_FILE, CString() >> (long long)modTime >> (char)pFile.length() << pFile << fileData.subString(0, sendSize) << "\n"}, false);
		}

		fileData.removeI(0, sendSize);
	}

	// If we had sent a large file, let the client know we finished sending it.
	if (isBigFile) sendPacket({PLO_LARGEFILEEND, CString() << pFile});

	return true;
}

bool TPlayer::testSign()
{
	CSettings& settings = server->getSettings();
	if (!settings.getBool("serverside", false)) return true;	// TODO: NPC server check instead

	// Check for sign collisions.
	if ((sprite % 4) == 0)
	{
		auto level = getLevel();
		if (level)
		{
			auto signs = level->getLevelSigns();
			for (auto sign : signs)
			{
				float signLoc[] = { (float)sign->getX(), (float)sign->getY() };
				if (y == signLoc[1] && inrange(x, signLoc[0] - 1.5f, signLoc[0] + 0.5f))
				{
					sendPacket({PLO_SAY2, CString() << sign->getUText().replaceAll("\n", "#b")});
				}
			}
		}
	}
	return true;
}

void TPlayer::testTouch()
{
#ifdef V8NPCSERVER
	static const int touchtestd[] = { 24,16, 0,32, 24,56, 48,32 };
	int dir = sprite % 4;

	int pixelX = int(x * 16.0);
	int pixelY = int(y * 16.0);

	auto level = getLevel();
	auto npcList = level->testTouch(pixelX + touchtestd[dir * 2], pixelY + touchtestd[dir * 2 + 1]);
	for (const auto& npc : npcList)
	{
		npc->queueNpcAction("npc.playertouchsme", this);
	}
#endif
}

void TPlayer::dropItemsOnDeath()
{
	if (!server->getSettings().getBool("dropitemsdead", true))
		return;

	int mindeathgralats = server->getSettings().getInt("mindeathgralats", 1);
	int maxdeathgralats = server->getSettings().getInt("maxdeathgralats", 50);

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
	sendPacket({PLO_PLAYERPROPS, CString() >> (char)PLPROP_RUPEESCOUNT >> (int)gralatc >> (char)PLPROP_ARROWSCOUNT >> (char)arrowc >> (char)PLPROP_BOMBSCOUNT >> (char)bombc});

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

		float pX = x + 1.5f + (float)(rand() % 8) - 2.0f;
		float pY = y + 2.0f + (float)(rand() % 8) - 2.0f;

		CString packet = CString() >> (char)PLI_ITEMADD >> (char)(pX * 2) >> (char)(pY * 2) >> (char)item;
		packet.readGChar();		// So msgPLI_ITEMADD works.

		msgPLI_ITEMADD(packet);
		sendPacket({PLO_ITEMADD, CString() << packet.subString(1)});
	}

	// Add arrows and bombs to the level.
	for (int i = 0; i < drop_arrows; ++i)
	{
		float pX = x + 1.5f + (float)(rand() % 8) - 2.0f;
		float pY = y + 2.0f + (float)(rand() % 8) - 2.0f;

		CString packet = CString() >> (char)PLI_ITEMADD >> (char)(pX * 2) >> (char)(pY * 2) >> (char)4;	// 4 = arrows
		packet.readGChar();		// So msgPLI_ITEMADD works.

		msgPLI_ITEMADD(packet);
		sendPacket({PLO_ITEMADD, CString() << packet.subString(1)});
	}
	for (int i = 0; i < drop_bombs; ++i)
	{
		float pX = x + 1.5f + (float)(rand() % 8) - 2.0f;
		float pY = y + 2.0f + (float)(rand() % 8) - 2.0f;

		CString packet = CString() >> (char)PLI_ITEMADD >> (char)(pX * 2) >> (char)(pY * 2) >> (char)3;	// 3 = bombs
		packet.readGChar();		// So msgPLI_ITEMADD works.

		msgPLI_ITEMADD(packet);
		sendPacket({PLO_ITEMADD, CString() << packet.subString(1)});
	}
}

bool TPlayer::processChat(CString pChat)
{
	std::vector<CString> chatParse = pChat.tokenizeConsole();
	if (chatParse.empty()) return false;
	bool processed = false;
	bool setcolorsallowed = server->getSettings().getBool("setcolorsallowed", true);

	if (chatParse[0] == "setnick")
	{
		processed = true;
		if ((int)difftime(time(nullptr), lastNick) >= 10)
		{
			lastNick = time(nullptr);
			CString newName = pChat.subString(8).trim();

			// Word filter.
			int filter = server->getWordFilter().apply(this, newName, FILTER_CHECK_NICK);
			if (filter & FILTER_ACTION_WARN)
			{
				setChat(newName);
				return true;
			}

			setProps(CString() >> (char)PLPROP_NICKNAME >> (char)newName.length() << newName, PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		}
		else
			setChat("Wait 10 seconds before changing your nick again!");
	}
	else if (chatParse[0] == "sethead" && chatParse.size() == 2)
	{
		if ( !server->getSettings().getBool("setheadallowed", true)) return false;
		processed = true;

		// Get the appropriate filesystem.
		CFileSystem* filesystem = server->getFileSystem();
		if ( !server->getSettings().getBool("nofoldersconfig", false))
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
			setProps(CString() >> (char)PLPROP_HEADGIF >> (char)(chatParse[1].length() + 100) << chatParse[1], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		else
			server->getServerList().sendPacket({SVO_GETFILE3, CString() >> (short)id >> (char)0 >> (char)chatParse[1].length() << chatParse[1]});
	}
	else if (chatParse[0] == "setbody" && chatParse.size() == 2)
	{
		if (!server->getSettings().getBool("setbodyallowed", true)) return false;
		processed = true;

		// Check to see if it is a default body.
		bool isDefault = false;
		for (auto & __defaultbody : __defaultbodies)
			if ( chatParse[1].match(CString(__defaultbody))) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			setProps(CString() >> (char)PLPROP_BODYIMG >> (char)chatParse[1].length() << chatParse[1], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
			return false;
		}

		// Get the appropriate filesystem.
		CFileSystem* filesystem = server->getFileSystem();
		if ( !server->getSettings().getBool("nofoldersconfig", false))
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
			setProps(CString() >> (char)PLPROP_BODYIMG >> (char)chatParse[1].length() << chatParse[1], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		else
			server->getServerList().sendPacket({SVO_GETFILE3, CString() >> (short)id >> (char)1 >> (char)chatParse[1].length() << chatParse[1]});
	}
	else if (chatParse[0] == "setsword" && chatParse.size() == 2)
	{
		if ( !server->getSettings().getBool("setswordallowed", true)) return false;
		processed = true;

		// Check to see if it is a default sword.
		bool isDefault = false;
		for (auto & __defaultsword : __defaultswords)
			if ( chatParse[1].match(CString(__defaultsword))) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			setProps(CString() >> (char)PLPROP_SWORDPOWER >> (char)(swordPower + 30) >> (char)chatParse[1].length() << chatParse[1], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
			return false;
		}

		// Get the appropriate filesystem.
		CFileSystem* filesystem = server->getFileSystem();
		if ( !server->getSettings().getBool("nofoldersconfig", false))
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
			setProps(CString() >> (char)PLPROP_SWORDPOWER >> (char)(swordPower + 30) >> (char)chatParse[1].length() << chatParse[1], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		else
			server->getServerList().sendPacket({SVO_GETFILE3, CString() >> (short)id >> (char)2 >> (char)chatParse[1].length() << chatParse[1]});
	}
	else if (chatParse[0] == "setshield" && chatParse.size() == 2)
	{
		if ( !server->getSettings().getBool("setshieldallowed", true)) return false;
		processed = true;

		// Check to see if it is a default shield.
		bool isDefault = false;
		for (auto & __defaultshield : __defaultshields)
			if ( chatParse[1].match(CString(__defaultshield))) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			setProps(CString() >> (char)PLPROP_SHIELDPOWER >> (char)(shieldPower + 10) >> (char)chatParse[1].length() << chatParse[1], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
			return false;
		}

		// Get the appropriate filesystem.
		CFileSystem* filesystem = server->getFileSystem();
		if ( !server->getSettings().getBool("nofoldersconfig", false))
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
			setProps(CString() >> (char)PLPROP_SHIELDPOWER >> (char)(shieldPower + 10) >> (char)chatParse[1].length() << chatParse[1], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		else
			server->getServerList().sendPacket({SVO_GETFILE3, CString() >> (short)id >> (char)3 >> (char)chatParse[1].length() << chatParse[1]});
	}
	else if (chatParse[0] == "setskin" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 0
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		signed char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			colors[0] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		}
	}
	else if (chatParse[0] == "setcoat" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 1
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		signed char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			colors[1] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		}
	}
	else if (chatParse[0] == "setsleeves" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 2
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		signed char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			colors[2] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		}
	}
	else if (chatParse[0] == "setshoes" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 3
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		signed char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			colors[3] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		}
	}
	else if (chatParse[0] == "setbelt" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 4
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		signed char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			colors[4] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		}
	}
	else if (chatParse[0] == "warpto")
	{
		processed = true;

		// To player
		if (chatParse.size() == 2)
		{
			// Permission check.
			if (!hasRight(PLPERM_WARPTOPLAYER) && !server->getSettings().getBool("warptoforall", false))
			{
				setChat("(not authorized to warp)");
				return true;
			}

			auto player = server->getPlayer(chatParse[1], PLTYPE_ANYCLIENT);
			if (player && player->getLevel())
				warp(player->getLevel()->getLevelName(), player->getX(), player->getY());
		}
		// To x/y location
		else if (chatParse.size() == 3)
		{
			// Permission check.
			if (!hasRight(PLPERM_WARPTO) && !server->getSettings().getBool("warptoforall", false))
			{
				setChat("(not authorized to warp)");
				return true;
			}

			setProps(CString() >> (char)PLPROP_X >> (char)(strtofloat(chatParse[1]) * 2) >> (char)PLPROP_Y >> (char)(strtofloat(chatParse[2]) * 2), PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		}
		// To x/y level
		else if (chatParse.size() == 4)
		{
			// Permission check.
			if (!hasRight(PLPERM_WARPTO) && !server->getSettings().getBool("warptoforall", false))
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

		auto p = server->getPlayer(chatParse[1], PLTYPE_ANYCLIENT);
		if (p) p->warp(levelName, x, y);
	}
	else if (chatParse[0] == "unstick" || chatParse[0] == "unstuck")
	{
		if (chatParse.size() == 2 && chatParse[1] == "me")
		{
			processed = true;

			// Check if the player is in a jailed level.
			std::vector<CString> jailLevels = server->getSettings().getStr("jaillevels").tokenize(",");
			for (auto & jailLevel : jailLevels)
				if (jailLevel.trim() == levelName) return false;

			int unstickTime = server->getSettings().getInt("unstickmetime", 30);
			if ((int)difftime(time(nullptr), lastMovement) >= unstickTime)
			{
				lastMovement = time(nullptr);
				CString unstickLevel = server->getSettings().getStr("unstickmelevel", "onlinestartlocal.nw");
				float unstickX = server->getSettings().getFloat("unstickmex", 30.0f);
				float unstickY = server->getSettings().getFloat("unstickmey", 30.5f);
				warp(unstickLevel, unstickX, unstickY);
				setChat("Warped!");
			}
			else
				setChat(CString() << "Don't move for " << CString(unstickTime) << " seconds before doing '" << pChat << "'!");
		}
	}
	else if (pChat == "update level" && hasRight(PLPERM_UPDATELEVEL))
	{
		processed = true;
		if (auto level = getLevel(); level)
			level->reload();
	}
	else if (pChat == "showadmins")
	{
		processed = true;

		// Search through the player list for all RC's.
		CString msg;
		{
			auto& playerList = server->getPlayerList();
			for (auto& [pid, player] : playerList)
			{
				// If an RC was found, add it to our string.
				if (player->getType() & PLTYPE_ANYRC)
					msg << (msg.length() == 0 ? "" : ", ") << player->getAccountName();
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
				auto& playerList = server->getPlayerList();
				for (auto& [pid, player] : playerList)
				{
					// If our guild matches, add it to our string.
					if (player->getGuild() == g)
						msg << (msg.length() == 0 ? "" : ", ") << player->getNickname().subString(0, player->getNickname().find('(')).trimI();
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
			auto& playerList = server->getPlayerList();
			for (auto& [pid, player] : playerList)
			{
				// If our guild matches, send the PM.
				if (player->getGuild() == guild)
				{
					player->sendPacket({PLO_PRIVATEMESSAGE, CString() >> (short)id << R"("","Guild message:",")" << pm << "\""});
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
	for (auto & adminIp : adminIps)
	{
		if (accountIpStr.match(adminIp))
			   return true;
	}

	return false;
}

bool TPlayer::isStaff()
{
	return server->isStaff(accountName);
}

/*
	TPlayer: Set Properties
*/
bool TPlayer::warp(const CString& pLevelName, float pX, float pY, time_t modTime)
{
	CSettings& settings = server->getSettings();

	// Save our current level.
	auto currentLevel = curlevel.lock();

	// Find the level.
	auto newLevel = TLevel::findLevel(pLevelName, server);

	// If we are warping to the same level, just update the player's location.
	if (currentLevel != nullptr && newLevel == currentLevel)
	{
		setProps(CString() >> (char)PLPROP_X >> (char)(pX * 2) >> (char)PLPROP_Y >> (char)(pY * 2), PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		return true;
	}

	// Find the unstickme level.
	auto unstickLevel = TLevel::findLevel(settings.getStr("unstickmelevel", "onlinestartlocal.nw"), server);
	float unstickX = settings.getFloat("unstickmex", 30.0f);
	float unstickY = settings.getFloat("unstickmey", 35.0f);

	// Leave our current level.
	leaveLevel();

	// See if the new level is on a gmap.
	pmap.reset();
	if (newLevel)
		pmap = newLevel->getMap();

	// Set x/y location.
	float oldX = x, oldY = y;
	x = pX;
	y =	pY;

	// Try warping to the new level.
	bool warpSuccess = setLevel(pLevelName, modTime);
	if (!warpSuccess)
	{
		// Failed, so try warping back to our old level.
		bool warped = true;
		if (currentLevel == nullptr) warped = false;
		else
		{
			x = oldX;
			y = oldY;
			pmap = currentLevel->getMap();
			warped = setLevel(currentLevel->getLevelName());
		}
		if ( !warped )
		{
			// Failed, so try warping to the unstick level.  If that fails, we disconnect.
			if (unstickLevel == nullptr) return false;

			// Try to warp to the unstick me level.
			x = unstickX;
			y =	unstickY;
			pmap = unstickLevel->getMap();
			if ( !setLevel(unstickLevel->getLevelName()))
				return false;
		}
	}

	return warpSuccess;
}

std::shared_ptr<TLevel> TPlayer::getLevel() const
{
	if (isHiddenClient()) return {};

	auto pLevel = curlevel.lock();
	if (pLevel) return pLevel;

	if (isClient() && server->warpPlayerToSafePlace(id)) {
		return curlevel.lock();
	}

	return {};
}

bool TPlayer::setLevel(const CString& pLevelName, time_t modTime)
{
	// Open Level
	auto newLevel = TLevel::findLevel(pLevelName, server);
	if (newLevel == nullptr)
	{
		sendPacket({PLO_WARPFAILED, CString() << pLevelName});
		return false;
	}
	curlevel = newLevel;

	// Check if the level is a singleplayer level.
	// If so, see if we have been there before.  If not, duplicate it.
	if (newLevel->isSingleplayer())
	{
		auto nl = (spLevels.find(newLevel->getLevelName()) != spLevels.end() ? spLevels[newLevel->getLevelName()] : nullptr);
		if (nl == nullptr)
		{
			newLevel = newLevel->clone();
			curlevel = newLevel;
			spLevels[newLevel->getLevelName()] = newLevel;
		}
		else curlevel = nl;
	}

	// Check if the map is a group map.
	if (auto map = pmap.lock(); map && map->isGroupMap())
	{
		if (!levelGroup.isEmpty())
		{
			// If any players are in this level, they might have been cached on the client.  Solve this by manually removing them.
			auto& plist = newLevel->getPlayerList();
			for (auto playerId : plist)
			{
				auto p = server->getPlayer(playerId);
				sendPacket({PLO_OTHERPLPROPS, p->getProps(nullptr, 0) >> (char)PLPROP_CURLEVEL >> (char)(newLevel->getLevelName().length() + 1 + 7) << newLevel->getLevelName() << ".unknown" >> (char)PLPROP_X << p->getProp(PLPROP_X) >> (char)PLPROP_Y << p->getProp(PLPROP_Y)});
			}

			// Set the correct level now.
			const auto& levelName = newLevel->getLevelName();
			auto& groupLevels = server->getGroupLevels();
			auto [start, end] = groupLevels.equal_range(levelName.toString());
			while (start != end)
			{
				if (auto nl = start->second.lock(); nl)
				{
					if (nl->getLevelName() == levelName)
					{
						curlevel = nl;
						break;
					}
				}
				++start;
			}
			if (start == end)
			{
				newLevel = newLevel->clone();
				curlevel = newLevel;
				newLevel->setLevelName(levelName);
				groupLevels.insert(std::make_pair(levelName.toString(), newLevel));
			}
		}
	}

	// Add myself to the level playerlist.
	newLevel->addPlayer(id);
	levelName = newLevel->getLevelName();

	// Tell the client their new level.
	if (modTime == 0 || versionID < CLVER_2_1)
	{
		if (auto map = pmap.lock(); map && map->getType() == MapType::GMAP && versionID >= CLVER_2_1)
		{
			sendPacket(
				{
					PLO_PLAYERWARP2,
					CString()
						>> (char)(x * 2) >> (char)(y * 2) >> (char)(z + 50)
						>> (char)newLevel->getMapX() >> (char)newLevel->getMapY()
						<< map->getMapName()
				}
			);
		}
		else
			sendPacket({PLO_PLAYERWARP, CString() >> (char)(x * 2) >> (char)(y * 2) << levelName});
	}

	// Send the level now.
	bool succeed = true;
	if (versionID >= CLVER_2_1)
		succeed = sendLevel(newLevel, modTime, false);
	else succeed = sendLevel141(newLevel, modTime, false);

	if (!succeed)
	{
		sendPacket({PLO_WARPFAILED, CString() << pLevelName});
		return false;
	}

	// If the level is a sparring zone, and you have 100 AP, change AP to 99 and
	// the apcounter to 1.
	if (newLevel->isSparringZone() && ap == 100)
	{
		ap = 99;
		apCounter = 1;
		setProps(CString() >> (char)PLPROP_ALIGNMENT >> (char)ap, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
	}

	// Inform everybody as to the client's new location.  This will update the minimap.
	CString minimap = this->getProps(nullptr, 0) >> (char)PLPROP_CURLEVEL << this->getProp(PLPROP_CURLEVEL) >> (char)PLPROP_X << this->getProp(PLPROP_X) >> (char)PLPROP_Y << this->getProp(PLPROP_Y);
	for (auto& [pid, player] : server->getPlayerList())
	{
		if (pid == this->getId())
			continue;
		if (auto map = pmap.lock(); map && map->isGroupMap() && levelGroup != player->getGroup())
			continue;

		player->sendPacket({PLO_OTHERPLPROPS, minimap});
	}
	//server->sendPacketToAll(getProps(0, 0) >> (char)PLPROP_CURLEVEL << getProp(PLPROP_CURLEVEL) >> (char)PLPROP_X << getProp(PLPROP_X) >> (char)PLPROP_Y << getProp(PLPROP_Y), this);

	return true;
}

bool TPlayer::sendLevel(std::shared_ptr<TLevel> pLevel, time_t modTime, bool fromAdjacent)
{
	if (pLevel == nullptr) return false;
	CSettings& settings = server->getSettings();

	// Send Level
	sendPacket({PLO_LEVELNAME, CString() << pLevel->getLevelName()});
	time_t l_time = getCachedLevelModTime(pLevel.get());
	if (modTime == -1) modTime = pLevel->getModTime();
	if (l_time == 0)
	{
		if (modTime != pLevel->getModTime())
		{
			if (!newProtocol)
				sendPacket({PLO_RAWDATA, CString() >> (int)(1+(64*64*2)+1)});
			sendPacket(pLevel->getBoardPacket());

			for (auto layer : pLevel->getLayers()) {
				if (layer.first == 0) continue;

				auto layerPacket = pLevel->getLayerPacket(layer.first);
				if (!newProtocol)
					sendPacket({PLO_RAWDATA, CString() >> (int)(layerPacket.Data.length()+2)});

				sendPacket(layerPacket);
			}
		}

		// Send links, signs, and mod time.
		sendPacket({PLO_LEVELMODTIME, CString() >> (long long)pLevel->getModTime()});
		for (const auto& packet : pLevel->getLinkPackets()) {
			sendPacket(packet);
		}
		for (const auto& packet : pLevel->getSignPackets(this)) {
			sendPacket(packet);
		}
	}

	// Send board changes, chests, horses, and baddies.
	if (!fromAdjacent)
	{
		sendPacket(pLevel->getBoardChangesPacket(l_time));

		for (const auto& packet : pLevel->getChestPackets(this)) {
			sendPacket(packet);
		}

		for (const auto& packet : pLevel->getHorsePackets()) {
			sendPacket(packet);
		}

		for (const auto& packet : pLevel->getBaddyPackets(versionID)) {
			sendPacket(packet);
		}
	}

	// If we are on a gmap, change our level back to the gmap.
	if (auto map = pmap.lock(); map && map->getType() == MapType::GMAP)
		sendPacket({PLO_LEVELNAME, CString() << map->getMapName()});

	// Tell the client if there are any ghost players in the level.
	// We don't support trial accounts so pass 0 (no ghosts) instead of 1 (ghosts present).
	sendPacket({PLO_GHOSTICON, CString() >> (char)0});

	if (!fromAdjacent || !pmap.expired())
	{
		// If we are the leader, send it now.
		if (pLevel->isPlayerLeader(getId()) || pLevel->isSingleplayer())
			sendPacket({PLO_ISLEADER, CString() << ""});
	}

	// Send new world time.
	sendPacket({PLO_NEWWORLDTIME, CString() << CString().writeGInt4(server->getNWTime())});
	if (!fromAdjacent || !pmap.expired())
	{
		// Send NPCs.
		if (auto map = pmap.lock(); map && map->getType() == MapType::GMAP)
		{
			sendPacket({PLO_SETACTIVELEVEL, CString() << map->getMapName()});

			auto val = pLevel->getNpcPackets(this, l_time, versionID);

			for (const auto& packet : val)
				sendPacket(packet);

			/*sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << pmap->getMapName());
			CString pmapLevels = pmap->getLevels();
			TLevel* tmpLvl;
			while (pmapLevels.bytesLeft() > 0)
			{
				CString tmpLvlName = pmapLevels.readString("\n");
				tmpLvl = TLevel::findLevel(tmpLvlName.guntokenizeI(), server);
				if (tmpLvl != NULL)
					sendPacket(CString() << tmpLvl->getNpcsPacket(l_time, versionID));
			}*/
		}
		else
		{
			sendPacket({PLO_SETACTIVELEVEL, CString() << pLevel->getLevelName()});
			for (const auto& packet : pLevel->getNpcPackets(this, l_time, versionID)) {
				sendPacket(packet);
			}
		}
	}

	// Send connecting player props to players in nearby levels.
	if (auto level = curlevel.lock(); level && !level->isSingleplayer())
	{
		// Send my props.
		server->sendPacketToLevelArea({PLO_OTHERPLPROPS, this->getProps(__getLogin, sizeof(__getLogin) / sizeof(bool))}, this->shared_from_this(), { id });

		// Get other player props.
		if (auto map = pmap.lock(); map)
		{
			auto sgmap{this->getMapPosition()};
			auto isGroupMap = map->isGroupMap();

			for (const auto &[otherid, other]: server->getPlayerList())
			{
				if (id == otherid) continue;
				if (!other->isClient()) continue;

				auto othermap = other->getMap().lock();
				if (!othermap || othermap != map) continue;
				if (isGroupMap && this->getGroup() != other->getGroup()) continue;

				// Check if they are nearby before sending the packet.
				auto ogmap{other->getMapPosition()};
				if (abs(ogmap.first - sgmap.first) < 2 && abs(ogmap.second - sgmap.second) < 2)
					this->sendPacket({PLO_OTHERPLPROPS, other->getProps(__getLogin, sizeof(__getLogin) / sizeof(bool))});
			}
		}
		else
		{
			for (auto otherid : level->getPlayerList())
			{
				if (id == otherid) continue;
				auto other = server->getPlayer(otherid);
				this->sendPacket({PLO_OTHERPLPROPS, other->getProps(__getLogin, sizeof(__getLogin) / sizeof(bool))});
			}
		}
	}

	return true;
}

bool TPlayer::sendLevel141(std::shared_ptr<TLevel> pLevel, time_t modTime, bool fromAdjacent)
{
	if (pLevel == nullptr) return false;
	CSettings& settings = server->getSettings();

	time_t l_time = getCachedLevelModTime(pLevel.get());
	if (modTime == -1) modTime = pLevel->getModTime();
	if (l_time != 0)
	{
		sendPacket(pLevel->getBoardChangesPacket(l_time));
	}
	else
	{
		if (modTime != pLevel->getModTime())
		{
			if (!newProtocol)
				sendPacket({PLO_RAWDATA,CString() >> (int)(1+(64*64*2)+1)});
			sendPacket(pLevel->getBoardPacket());

			if (firstLevel)
				sendPacket({PLO_LEVELNAME, CString() << pLevel->getLevelName()});
			firstLevel = false;

			// Send links, signs, and mod time.
			if ( !settings.getBool("serverside", false))	// TODO: NPC server check instead.
			{
				for (const auto& packet : pLevel->getLinkPackets()) {
					sendPacket(packet);
				}

				for (const auto& packet : pLevel->getSignPackets(this)) {
					sendPacket(packet);
				}
			}
			sendPacket({PLO_LEVELMODTIME, CString() >> (long long)pLevel->getModTime()});
		}
		else
			sendPacket({PLO_LEVELBOARD, CString() << ""});

		if ( !fromAdjacent )
		{
			sendPacket(pLevel->getBoardChangesPacket2(l_time));

			for (const auto& packet : pLevel->getChestPackets(this)) {
				sendPacket(packet);
			}
		}
	}

	// Send board changes, chests, horses, and baddies.
	if ( !fromAdjacent )
	{
		for (const auto& packet : pLevel->getHorsePackets()) {
			sendPacket(packet);
		}

		for (const auto& packet : pLevel->getBaddyPackets(versionID)) {
			sendPacket(packet);
		}

		// If we are the leader, send it now.
		if (pLevel->isPlayerLeader(getId()) || pLevel->isSingleplayer())
			sendPacket({PLO_ISLEADER, CString()});
	}

	// Send new world time.
	sendPacket({PLO_NEWWORLDTIME, CString() << CString().writeGInt4(server->getNWTime())});

	// Send NPCs.
	if ( !fromAdjacent )
		for (const auto& packet : pLevel->getNpcPackets(this, l_time, versionID))
			sendPacket(packet);

	// Send connecting player props to players in nearby levels.
	if (!pLevel->isSingleplayer() && !fromAdjacent)
	{
		server->sendPacketToLevelArea({PLO_OTHERPLPROPS, this->getProps(__getLogin, sizeof(__getLogin) / sizeof(bool))}, this->shared_from_this(), { id });

		for (auto playerId : pLevel->getPlayerList())
		{
			if (playerId == getId()) continue;

			auto player = server->getPlayer(playerId);
			this->sendPacket({PLO_OTHERPLPROPS, player->getProps(__getLogin, sizeof(__getLogin)/sizeof(bool))});
		}
	}

	return true;
}

bool TPlayer::leaveLevel(bool resetCache)
{
	// Make sure we are on a level first.
	auto levelp = curlevel.lock();
	if (!levelp) return true;

	// Save the time we left the level for the client-side caching.
	bool found = false;
	for (auto& cl : cachedLevels)
	{
		auto cllevel = cl->level.lock();
		if (cllevel == levelp)
		{
			cl->modTime = (resetCache ? 0 : time(nullptr));
			found = true;
			break;
		}
	}
	if (!found) cachedLevels.push_back(std::make_unique<SCachedLevel>(curlevel, time(nullptr)));

	// Remove self from list of players in level.
	levelp->removePlayer(id);

	// Send PLO_ISLEADER to new level leader.
	if (auto& levelPlayerList = levelp->getPlayerList(); !levelPlayerList.empty())
	{
		auto leader = server->getPlayer(levelPlayerList.front());
		leader->sendPacket({PLO_ISLEADER, CString()});
	}

	// Tell everyone I left.
	// This prop isn't used at all???  Maybe it is required for 1.41?
//	if (pmap && pmap->getType() != MAPTYPE_GMAP)
	{
		server->sendPacketToLevelArea({PLO_OTHERPLPROPS, this->getProps(nullptr, 0) >> (char)PLPROP_JOINLEAVELVL >> (char)0}, this->shared_from_this(), { id });

		for (auto& [pid, player] : server->getPlayerList())
		{
			if (pid == getId()) continue;
			if (player->getLevel() != getLevel()) continue;
			this->sendPacket({PLO_OTHERPLPROPS, player->getProps(nullptr, 0) >> (char)PLPROP_JOINLEAVELVL >> (char)0});
		}
	}

	// Set the level pointer to 0.
	curlevel.reset();

	return true;
}

time_t TPlayer::getCachedLevelModTime(const TLevel* level) const
{
	for (auto& cl : cachedLevels)
	{
		auto cllevel = cl->level.lock();
		if (cllevel && cllevel.get() == level)
			return cl->modTime;
	}
	return 0;
}

void TPlayer::resetLevelCache(const TLevel* level)
{
	for (auto& cl : cachedLevels)
	{
		auto cllevel = cl->level.lock();
		if (cllevel && cllevel.get() == level)
		{
			cl->modTime = 0;
			return;
		}
	}
}

std::pair<int, int> TPlayer::getMapPosition() const
{
	if (curlevel.expired()) return { 0, 0 };
	if (pmap.expired()) return { 0, 0 };

	auto level = getLevel();
	auto map = pmap.lock();
	if (!level || !map) return { 0, 0 };

	switch (map->getType())
	{
	case MapType::BIGMAP:
		return { level->getMapX(), level->getMapY() };
	default:
	case MapType::GMAP:
		return { getProp(PLPROP_GMAPLEVELX).readGUChar() , getProp(PLPROP_GMAPLEVELY).readGUChar() };
	}

	return { 0, 0 };
}

void TPlayer::setChat(const CString& pChat)
{
	setProps(CString() >> (char)PLPROP_CURCHAT >> (char)pChat.length() << pChat, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

void TPlayer::setNick(CString pNickName, bool force)
{
	CString newNick, nick, guild;

	// Limit the nickname to 223 characters
	if (pNickName.length() > 223)
		pNickName = pNickName.subString(0, 223);

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

	if (force || (guild == "RC" && isRC()))
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

	// Add the nickname.
	newNick << nick;

	// If a guild was specified, add the guild.
	if (guild.length() != 0)
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
		bool askGlobal = server->getSettings().getBool("globalguilds", true);
		if (!askGlobal)
		{
			// Check for whitelisted global guilds.
			std::vector<CString> allowed = server->getSettings().getStr("allowedglobalguilds").tokenize(",");
			if (std::find(allowed.begin(), allowed.end(), guild) != allowed.end())
				askGlobal = true;
		}

		// See if it is a global guild.
		if (askGlobal)
		{
			server->getServerList().sendPacket(
				{
					SVO_VERIGUILD,
					CString()  >> (short)id
						>> (char)accountName.length() << accountName
						>> (char)newNick.length() << newNick
						>> (char)guild.length() << guild
				}
			);
		}
	}
	else
	{
		// Save it.
		nickName = newNick;
		this->guild.clear();
	}

	if (isExternal)
	{
		nickName = pNickName;
	}

}

bool TPlayer::addWeapon(LevelItemType defaultWeapon)
{
	// Allow Default Weapons..?
	CSettings& settings = server->getSettings();
	if (!settings.getBool("defaultweapons", true))
		return false;

	auto weapon = server->getWeapon(TLevelItem::getItemName(defaultWeapon));
	if (!weapon)
	{
		weapon = std::make_shared<TWeapon>(server, defaultWeapon);
		server->NC_AddWeapon(weapon);
	}

	return addWeapon(weapon);
}

bool TPlayer::addWeapon(const std::string& name)
{
	auto weapon = server->getWeapon(name);
	return this->addWeapon(weapon);
}

bool TPlayer::addWeapon(std::shared_ptr<TWeapon> weapon)
{
	if (weapon == nullptr) return false;

	// See if the player already has the weapon.
	if (vecSearch<CString>(weaponList, weapon->getName()) == -1)
	{
		weaponList.emplace_back(weapon->getName());
		if (id == -1) return true;

		// Send weapon.
		for (const auto& packet : weapon->getWeaponPackets(versionID)) {
			sendPacket(packet, true);
		}
	}

	return true;
}

bool TPlayer::deleteWeapon(LevelItemType defaultWeapon)
{
	auto weapon = server->getWeapon(TLevelItem::getItemName(defaultWeapon));
	return this->deleteWeapon(weapon);
}

bool TPlayer::deleteWeapon(const std::string& name)
{
	auto weapon = server->getWeapon(name);
	return this->deleteWeapon(weapon);
}

bool TPlayer::deleteWeapon(std::shared_ptr<TWeapon> weapon)
{
	if (weapon == nullptr) return false;

	// Remove the weapon.
	if (vecRemove<CString>(weaponList, weapon->getName()))
	{
		if (id == -1) return true;

		// Send delete notice.
		sendPacket({PLO_NPCWEAPONDEL, CString() << weapon->getName()});
	}

	return true;
}

void TPlayer::disableWeapons()
{
	status &= ~PLSTATUS_ALLOWWEAPONS;
	sendPacket({PLO_PLAYERPROPS, CString() >> (char)PLPROP_STATUS << getProp(PLPROP_STATUS)});
}

void TPlayer::enableWeapons()
{
	status |= PLSTATUS_ALLOWWEAPONS;
	sendPacket({PLO_PLAYERPROPS, CString() >> (char)PLPROP_STATUS << getProp(PLPROP_STATUS)});
}

void TPlayer::freezePlayer()
{
	sendPacket({PLO_FREEZEPLAYER2, CString() << ""});
}

void TPlayer::unfreezePlayer()
{
	sendPacket({PLO_UNFREEZEPLAYER, CString() << ""});
}

void TPlayer::sendRPGMessage(const CString &message)
{
	sendPacket({PLO_RPGWINDOW, CString() << message.gtokenize()}, true);
}

void TPlayer::sendSignMessage(const CString &message)
{
	sendPacket({PLO_SAY2, CString() << message.replaceAll("\n", "#b")});
}

void TPlayer::setAni(CString gani)
{
	if (gani.length() > 223)
		gani.remove(223);

	CString propPackage;
	propPackage >> (char)PLPROP_GANI >> (char)gani.length() << gani;
	setProps(propPackage, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

/*
	TPlayer: Flag Functions
*/

void TPlayer::deleteFlag(const std::string& pFlagName, bool sendToPlayer)
{
	TAccount::deleteFlag(pFlagName);

	if (sendToPlayer) {
		sendPacket({PLO_FLAGDEL, CString() << pFlagName});
	}
}

void TPlayer::setFlag(const std::string& pFlagName, const CString& pFlagValue, bool sendToPlayer)
{
	// Call Default Set Flag
	TAccount::setFlag(pFlagName, pFlagValue);

	// Send to Player
	if (sendToPlayer)
	{
		if (pFlagValue.isEmpty())
			sendPacket({PLO_FLAGSET, CString() << pFlagName});
		else
			sendPacket({PLO_FLAGSET, CString() << pFlagName << "=" << pFlagValue});
	}
}

/*
	TPlayer: Packet functions
*/
bool TPlayer::msgPLI_NULL(CString& pPacket)
{
	pPacket.setRead(0);
	printf("Unknown Player Packet: %u (%s) Length: %d\n", (unsigned int)pPacket.readGUChar(), pPacket.text()+1, pPacket.length());
	for (int i = 0; i < pPacket.length(); ++i) printf("%02x ", (unsigned char)((pPacket.text())[i])); printf("\n");

	// If we are getting a bunch of invalid packets, something went wrong.  Disconnect the player.
	invalidPackets++;
	if (invalidPackets > 5)
	{
		serverlog.out("[%s] Player %s is sending invalid packets.\n", server->getName().text(), nickName.text());
		sendPacket({PLO_DISCMESSAGE, CString() << "Disconnected for sending invalid packets."});
		return false;
	}

	return true;
}

bool TPlayer::msgPLI_LOGIN(CString& pPacket)
{
	// Read Player-Ip
	accountIpStr = playerSock->getRemoteIp();
#ifdef HAVE_INET_PTON
	inet_pton(AF_INET, accountIpStr.text(), &accountIp);
#else
	accountIp = inet_addr(accountIpStr.text());
#endif

	// TODO(joey): Hijack type based on what graal sends, rather than use it directly.

	// Read Client-Type
	serverlog.out("[%s] :: New login:\t", server->getName().text());
	type = (1 << pPacket.readGChar());
	bool getKey = false;
	switch (type)
	{
		case PLTYPE_CLIENT:
			serverlog.append("Client\n");
			in_codec.setGen(ENCRYPT_GEN_2);
			break;
		case PLTYPE_RC:
			serverlog.append("RC\n");
			in_codec.setGen(ENCRYPT_GEN_3);
			break;
		case PLTYPE_NPCSERVER:
			serverlog.append("NPCSERVER\n");
			in_codec.setGen(ENCRYPT_GEN_3);
			break;
		case PLTYPE_NC:
			serverlog.append("NC\n");
			in_codec.setGen(ENCRYPT_GEN_3);
			getKey = false;
			break;
		case PLTYPE_CLIENT2:
			serverlog.append("New Client (2.19 - 2.21, 3 - 3.01)\n");
			in_codec.setGen(ENCRYPT_GEN_4);
			break;
		case PLTYPE_CLIENT3:
			serverlog.append("New Client (2.22+)\n");
			in_codec.setGen(ENCRYPT_GEN_5);
			break;
		case PLTYPE_RC2:
			serverlog.append("New RC (2.22+)\n");
			in_codec.setGen(ENCRYPT_GEN_5);
			getKey = true;
			break;
		case PLTYPE_WEB:
			serverlog.append("Web\n");
			in_codec.setGen(ENCRYPT_GEN_1);
			fileQueue.setCodec(ENCRYPT_GEN_1, key);
			getKey = false;
			break;
		default:
			serverlog.append("Unknown (%d)\n", type);
			sendPacket({PLO_DISCMESSAGE, CString() << "Your client type is unknown.  Please inform the " << APP_VENDOR << " Team.  Type: " << CString((int)type) << "."});
			return false;
			break;
	}

	if (type == PLTYPE_CLIENT) {
		// Read Client-Version for v1.3 clients
		version = pPacket.readChars(8);
		versionID = getVersionID(version);

		if (versionID == CLVER_UNKNOWN) {
			in_codec.setGen(ENCRYPT_GEN_3);
			pPacket.setRead(1);
		}
	}

	if (versionID == CLVER_UNKNOWN) {
		// Get Iterator-Key
		// 2.19+ RC and any client should get the key.
		if ( (isClient() && type != PLTYPE_WEB) || (isRC() && in_codec.getGen() > ENCRYPT_GEN_3) || getKey ) {
			key = (unsigned char)pPacket.readGChar();

			in_codec.reset(key);
			if ( in_codec.getGen() > ENCRYPT_GEN_3 )
				fileQueue.setCodec(in_codec.getGen(), key);
		}

		// Read Client-Version
		version = pPacket.readChars(8);
		versionID = getVersionIDByVersion(version);
	}

	// Read Account & Password
	accountName = pPacket.readChars(pPacket.readGUChar());
	CString password = pPacket.readChars(pPacket.readGUChar());

	// Client Identity: win,"",02e2465a2bf38f8a115f6208e9938ac8,ff144a9abb9eaff4b606f0336d6d8bc5,"6.2 9200 "
	//					{platform}, {mobile provides 'dc:id2'}, {md5hash:harddisk-id}, {md5hash:network-id}, {uname(release, version)}, {android-id}
	CString identity = pPacket.readString("");

	//serverlog.out("[%s]    Key: %d\n", server->getName().text(), key);
	serverlog.out("[%s]    Version:\t%s (%s)\n", server->getName().text(), version.text(), getVersionString(version, type));
	serverlog.out("[%s]    Account:\t%s\n", server->getName().text(), accountName.text());
	if (!identity.isEmpty()) {
		serverlog.out("[%s]    Identity:\t%s\n", server->getName().text(), identity.text());
		auto identityTokens = identity.tokenize(",", true);
		os = identityTokens[0];
	}

	// Check for available slots on the server.
	if (server->getPlayerList().size() >= (unsigned int)server->getSettings().getInt("maxplayers", 128))
	{
		sendPacket({PLO_DISCMESSAGE, CString() << "This server has reached its player limit."});
		return false;
	}

	// Check if they are ip-banned or not.
	if (server->isIpBanned(playerSock->getRemoteIp()) && !hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		sendPacket({PLO_DISCMESSAGE, CString() << "You have been banned from this server."});
		return false;
	}

	// Check if the specified client is allowed access.
	if (isClient())
	{
		auto& allowedVersions = server->getAllowedVersions();
		bool allowed = false;
		for (auto ver : allowedVersions)
		{
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
			sendPacket({PLO_DISCMESSAGE, CString() << "Your client version is not allowed on this server.\rAllowed: " << server->getAllowedVersionString()});
			return false;
		}
	}

	// Verify login details with the serverlist.
	// TODO: localhost mode.
	if ( !server->getServerList().getConnected())
	{
		sendPacket({PLO_DISCMESSAGE, CString() << "The login server is offline.  Try again later."});
		return false;
	}

	server->getServerList().sendLoginPacketForPlayer(shared_from_this(), password, identity);
	return true;
}

int TPlayer::getVersionIDByVersion(const CString& versionInput) const {
	if ( isClient()) return getVersionID(versionInput);
	else if ( isNC()) return getNCVersionID(versionInput);
	else if ( isRC()) return getRCVersionID(versionInput);
	else return CLVER_UNKNOWN;
}

bool TPlayer::msgPLI_LEVELWARP(CString& pPacket)
{
	time_t modTime = 0;

	if (pPacket[0] - 32 == PLI_LEVELWARPMOD)
		modTime = (time_t)pPacket.readGUInt5();

	float loc[2] = {(float)((float)pPacket.readGChar() / 2.0f), (float)((float)pPacket.readGChar() / 2.0f)};
	CString newLevel = pPacket.readString("");
	warp(newLevel, loc[0], loc[1], modTime);

	return true;
}

bool TPlayer::msgPLI_BOARDMODIFY(CString& pPacket)
{
	CSettings& settings = server->getSettings();
	signed char loc[2] = {pPacket.readGChar(), pPacket.readGChar()};
	signed char dim[2] = {pPacket.readGChar(), pPacket.readGChar()};
	CString tiles = pPacket.readString("");

	// Alter level data.
	auto level = getLevel();
	if (level->alterBoard(tiles, loc[0], loc[1], dim[0], dim[1], this))
		server->sendPacketToOneLevel({PLO_BOARDMODIFY, CString() << (pPacket.text() + 1)}, level);

	if (loc[0] < 0 || loc[0] > 63 || loc[1] < 0 || loc[1] > 63) return true;

	// Older clients drop items clientside.
	if (versionID < CLVER_2_1)
		return true;

	// Lay items when you destroy objects.
	short oldTile = (getLevel()->getTiles())[loc[0] + (loc[1] * 64)];
	bool bushitems = settings.getBool("bushitems", true);
	bool vasesdrop = settings.getBool("vasesdrop", true);
	int tiledroprate = settings.getInt("tiledroprate", 50);
	LevelItemType dropItem = LevelItemType::INVALID;

	// Bushes, grass, swamp.
	if ((oldTile == 2 || oldTile == 0x1a4 || oldTile == 0x1ff ||
		oldTile == 0x3ff) && bushitems)
	{
		if ( tiledroprate > 0 )
		{
			if ( (rand() % 100) < tiledroprate )
			{
				dropItem = TLevelItem::getItemId(rand() % 6);
			}
		}
	}
	// Vase.
	else if (oldTile == 0x2ac && vasesdrop)
		dropItem = LevelItemType::HEART;

	// Send the item now.
	// TODO: Make this a more generic function.
	if (dropItem != LevelItemType::INVALID)
	{
		// TODO: GS2 replacement of item drops. How does it work?
		CString packet = CString() >> (char)(loc[0] * 2) >> (char)(loc[1] * 2) >> (char)TLevelItem::getItemTypeId(dropItem);
		CString packet2 = CString() >> (char)PLI_ITEMADD << packet;
		packet2.readGChar();		// So msgPLI_ITEMADD works.

		spawnLevelItem(packet2, false);

		if (getVersion() <= CLVER_5_12)
			sendPacket({PLO_ITEMADD, CString() << packet});
	}

	return true;
}

bool TPlayer::msgPLI_REQUESTUPDATEBOARD(CString& pPacket)
{
	// {130}{CHAR level length}{level}{INT5 modtime}{SHORT x}{SHORT y}{SHORT width}{SHORT height}
	CString level = pPacket.readChars(pPacket.readGUChar());

	time_t modTime = (time_t)pPacket.readGUInt5();

	short x = pPacket.readGShort();
	short y = pPacket.readGShort();
	short w = pPacket.readGShort();
	short h = pPacket.readGShort();

	// TODO: What to return?
	serverlog.out("[%s] :: Received PLI_REQUESTUPDATEBOARD - level: %s - x: %d - y: %d - w: %d - h: %d - modtime: %d\n", server->getName().text(), level.text(), x, y, w, h, modTime);

	return true;
}

bool TPlayer::msgPLI_PLAYERPROPS(CString& pPacket)
{
	setProps(pPacket, PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD);
	return true;
}

bool TPlayer::msgPLI_NPCPROPS(CString& pPacket)
{
	// Don't accept npc-properties from clients when a npc-server is present
#ifdef V8NPCSERVER
	return true;
#endif

	unsigned int npcId = pPacket.readGUInt();
	CString npcProps = pPacket.readString("");

	//printf( "npcId: %d\n", npcId );
	//printf( "pPacket: %s\n", npcProps.text());
	//for (int i = 0; i < pPacket.length(); ++i) printf( "%02x ", (unsigned char)pPacket[i] );
	//printf( "\n" );

	auto level = getLevel();
	auto npc = server->getNPC(npcId);
	if (!npc)
		return true;

	if (npc->getLevel() != level)
		return true;

	CString packet = CString() >> (int)npcId;
	packet << npc->setProps(npcProps, versionID);
	server->sendPacketToLevelArea({PLO_NPCPROPS, packet}, shared_from_this(), { id });

	return true;
}

bool TPlayer::msgPLI_BOMBADD(CString& pPacket)
{
	// TODO(joey): gmap support
	unsigned char loc[2] = { pPacket.readGUChar(), pPacket.readGUChar() };
	//float loc[2] = {(float)pPacket.readGChar() / 2.0f, (float)pPacket.readGChar() / 2.0f};
	unsigned char player_power = pPacket.readGUChar();
	unsigned char player = player_power >> 2;
	unsigned char power = player_power & 0x03;
	unsigned char timeToExplode = pPacket.readGUChar();		// How many 0.05 sec increments until it explodes.  Defaults to 55 (2.75 seconds.)

	/*
	printf("Place bomb\n");
	printf("Position: (%d, %d)\n", loc[0], loc[1]);
	//printf("Position: (%0.2f, %0.2f)\n", loc[0], loc[1]);
	printf("Player (?): %d\n", player);
	printf("Bomb Power: %d\n", power);
	printf("Bomb Explode Timer: %d\n", timeToExplode);
	//for (int i = 0; i < pPacket.length(); ++i) printf( "%02x ", (unsigned char)pPacket[i] ); printf( "\n" );
	*/

	server->sendPacketToOneLevel({PLO_BOMBADD, CString() >> (short)id << (pPacket.text() + 1)}, curlevel, { id });
	return true;
}

bool TPlayer::msgPLI_BOMBDEL(CString& pPacket)
{
	server->sendPacketToOneLevel({PLO_BOMBDEL, CString() << (pPacket.text() + 1)}, curlevel, { id });
	return true;
}

bool TPlayer::msgPLI_TOALL(CString& pPacket)
{
	// Check if the player is in a jailed level.
	std::vector<CString> jailList = server->getSettings().getStr("jaillevels").tokenize(",");
	if (std::find_if(jailList.begin(), jailList.end(), [&levelName = this->levelName](CString &level) { return level.trim() == levelName; }) != jailList.end())
		return true;

	CString message = pPacket.readString(pPacket.readGUChar());

	// Word filter.
	int filter = server->getWordFilter().apply(this, message, FILTER_CHECK_TOALL);
	if (filter & FILTER_ACTION_WARN)
	{
		setChat(message);
		return true;
	}

	for (auto& [pid, player] : server->getPlayerList())
	{
		if (pid == id) continue;

		// See if the player is allowing toalls.
		unsigned char flags = strtoint(player->getProp(PLPROP_ADDITFLAGS));
		if (flags & PLFLAG_NOTOALL) continue;

		player->sendPacket({PLO_TOALL, CString() >> (short)id >> (char)message.length() << message});
	}
	return true;
}

bool TPlayer::msgPLI_HORSEADD(CString& pPacket)
{
	server->sendPacketToOneLevel({PLO_HORSEADD, CString() << (pPacket.text() + 1)}, curlevel, { id });

	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};
	unsigned char dir_bush = pPacket.readGUChar();
	char hdir = dir_bush & 0x03;
	char hbushes = dir_bush >> 2;
	CString image = pPacket.readString("");

	auto level = getLevel();
	level->addHorse(image, loc[0], loc[1], hdir, hbushes);
	return true;
}

bool TPlayer::msgPLI_HORSEDEL(CString& pPacket)
{
	server->sendPacketToOneLevel({PLO_HORSEDEL, CString() << (pPacket.text() + 1)}, curlevel, { id });

	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};

	auto level = getLevel();
	level->removeHorse(loc[0], loc[1]);
	return true;
}

bool TPlayer::msgPLI_ARROWADD(CString& pPacket)
{
	server->sendPacketToOneLevel({PLO_ARROWADD, CString() >> (short)id << (pPacket.text() + 1)}, curlevel, { id });
	return true;
}

bool TPlayer::msgPLI_FIRESPY(CString& pPacket)
{
	server->sendPacketToOneLevel({PLO_FIRESPY, CString() >> (short)id << (pPacket.text() + 1)}, curlevel, { id });
	return true;
}

bool TPlayer::msgPLI_THROWCARRIED(CString& pPacket)
{
	// TODO: Remove when an npcserver is created.
	if (!server->getSettings().getBool("duplicatecanbecarried", false) && carryNpcId != 0)
	{
		auto npc = server->getNPC(carryNpcId);
		if (npc)
		{
			carryNpcThrown = true;

			// Add the NPC back to the level if it never left.
			auto level = getLevel();
			if (npc->getLevel() == level)
				level->addNPC(npc);
		}
	}
	server->sendPacketToOneLevel({PLO_THROWCARRIED, CString() >> (short)id << (pPacket.text() + 1)}, curlevel, { id });
	return true;
}

bool TPlayer::removeItem(LevelItemType itemType)
{
	switch (itemType)
	{
		case LevelItemType::GREENRUPEE:		// greenrupee
		case LevelItemType::BLUERUPEE:		// bluerupee
		case LevelItemType::REDRUPEE:		// redrupee
		case LevelItemType::GOLDRUPEE:		// goldrupee
		{
			int gralatsRequired;
			if (itemType == LevelItemType::GOLDRUPEE) gralatsRequired = 100;
			else if (itemType == LevelItemType::REDRUPEE) gralatsRequired = 30;
			else if (itemType == LevelItemType::BLUERUPEE) gralatsRequired = 5;
			else gralatsRequired = 1;

			if (gralatc >= gralatsRequired)
			{
				gralatc -= gralatsRequired;
				return true;
			}

			return false;
		}

		case LevelItemType::BOMBS:
		{
			if (bombc >= 5)
			{
				bombc -= 5;
				return true;
			}
			return false;
		}

		case LevelItemType::DARTS:
		{
			if (arrowc >= 5)
			{
				arrowc -= 5;
				return true;
			}
			return false;
		}

		case LevelItemType::HEART:
		{
			if (power > 1.0f)
			{
				power -= 1.0f;
				return true;
			}
			return false;
		}

#ifndef V8NPCSERVER
		// NOTE: not receiving PLI_ITEMTAKE for >2.31, so we will not remove the item
		// same is true for sword/shield. assuming it's true for the weapon-items, but
		// It's currently not tested.
		case LevelItemType::GLOVE1:
		case LevelItemType::GLOVE2:
		{
			if (glovePower > 1)
			{
				glovePower--;
				return true;
			}
			return false;
		}
#endif

		/*
		case LevelItemType::BOW:		// bow
		case LevelItemType::BOMB:		// bomb
			return false;

		case LevelItemType::SUPERBOMB:	// superbomb
		case LevelItemType::FIREBALL:	// fireball
		case LevelItemType::FIREBLAST:	// fireblast
		case LevelItemType::NUKESHOT:	// nukeshot
		case LevelItemType::JOLTBOMB:	// joltbomb
			return false;

		case LevelItemType::SHIELD:			// shield
		case LevelItemType::MIRRORSHIELD:	// mirrorshield
		case LevelItemType::LIZARDSHIELD:	// lizardshield
			return false;

		case LevelItemType::SWORD:			// sword
		case LevelItemType::BATTLEAXE:		// battleaxe
		case LevelItemType::LIZARDSWORD:	// lizardsword
		case LevelItemType::GOLDENSWORD:	// goldensword
			return false;

		case LevelItemType::FULLHEART:	// fullheart
			return false;
		*/

		case LevelItemType::SPINATTACK:
		{
			if (status & PLSTATUS_HASSPIN)
			{
				status &= ~PLSTATUS_HASSPIN;
				return true;
			}
			return false;
		}
	}

	return false;
}

bool TPlayer::msgPLI_ITEMADD(CString& pPacket)
{
	return spawnLevelItem(pPacket, true);
}

bool TPlayer::spawnLevelItem(CString& pPacket, bool playerDrop) {
	// TODO(joey): serverside item checking
	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};
	unsigned char item = pPacket.readGUChar();

	LevelItemType itemType = TLevelItem::getItemId(item);
	if (itemType != LevelItemType::INVALID)
	{
#ifdef V8NPCSERVER
		if (removeItem(itemType) || !playerDrop)
		{
#endif
			auto level = getLevel();
			if (level->addItem(loc[0], loc[1], itemType))
			{
				server->sendPacketToOneLevel({PLO_ITEMADD, CString() << (pPacket.text() + 1)}, level, { id });
			}
			else
			{
				sendPacket({PLO_ITEMDEL, CString() << (pPacket.text() + 1)});
			}

#ifdef V8NPCSERVER
		}
#endif
	}

	return true;
}

bool TPlayer::msgPLI_ITEMDEL(CString& pPacket)
{
	server->sendPacketToOneLevel({PLO_ITEMDEL, CString() << (pPacket.text() + 1)}, curlevel, { id });

	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};

	// Remove the item from the level, getting the type of the item in the process.
	auto level = getLevel();
	LevelItemType item = level->removeItem(loc[0], loc[1]);
	if (item == LevelItemType::INVALID) return true;

	// If this is a PLI_ITEMTAKE packet, give the item to the player.
	if (pPacket[0] - 32 == PLI_ITEMTAKE)
		setProps(CString() << TLevelItem::getItemPlayerProp(item, this), PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);

	return true;
}

bool TPlayer::msgPLI_CLAIMPKER(CString& pPacket)
{
	// Get the player who killed us.
	unsigned int pId = pPacket.readGUShort();
	auto killer = server->getPlayer(pId, PLTYPE_ANYCLIENT);
	if (killer == nullptr || killer.get() == this)
		return true;

	// Sparring zone rating code.
	// Uses the glicko rating system.
	auto level = getLevel();
	if (level == nullptr) return true;
	if (level->isSparringZone())
	{
		// Get some stats we are going to use.
		// Need to parse the other player's PLPROP_RATING.
		unsigned int otherRating = killer->getProp(PLPROP_RATING).readGUInt();
		float oldStats[4] = { rating, deviation, (float)((otherRating >> 9) & 0xFFF), (float)(otherRating & 0x1FF) };

		// If the IPs are the same, don't update the rating to prevent cheating.
		if (CString(playerSock->getRemoteIp()) == CString(killer->getSocket()->getRemoteIp()))
			return true;

		float gSpar[2] = {static_cast<float>(1.0f / std::pow((1.0f+3.0f*std::pow(0.0057565f,2)*(std::pow(oldStats[3],2))/std::pow(3.14159265f,2)),0.5f)),	//Winner
			static_cast<float>(1.0f / pow((1.0f+3.0f*std::pow(0.0057565f,2)*(std::pow(oldStats[1],2))/std::pow(3.14159265f,2)),0.5f))};	//Loser
		float ESpar[2] = {1.0f / (1.0f + (float)std::pow(10.0f,(-gSpar[1]*(oldStats[2]-oldStats[0])/400.0f))),					//Winner
						  1.0f / (1.0f + (float)std::pow(10.0f,(-gSpar[0]*(oldStats[0]-oldStats[2])/400.0f)))};					//Loser
		float dSpar[2] = {static_cast<float>(1.0f / (std::pow(0.0057565f,2)*std::pow(gSpar[0],2)*ESpar[0]*(1.0f-ESpar[0]))),						//Winner
			static_cast<float>(1.0f / (std::pow(0.0057565f,2)*std::pow(gSpar[1],2)*ESpar[1]*(1.0f-ESpar[1])))};						//Loser

		float tWinRating = oldStats[2] + (0.0057565f / ( 1.0f / powf(oldStats[3],2) + 1.0f/dSpar[0])) * (gSpar[0] * (1.0f - ESpar[0]));
		float tLoseRating = oldStats[0] + (0.0057565f / ( 1.0f / powf(oldStats[1],2) + 1.0f/dSpar[1])) * (gSpar[1] * (0.0f - ESpar[1]));
  		float tWinDeviation = powf((1.0f/(1.0f/powf(oldStats[3],2)+1/dSpar[0])),0.5f);
  		float tLoseDeviation = powf((1.0f/(1.0f/powf(oldStats[1],2)+1/dSpar[1])),0.5f);

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
			setProps(CString() >> (char)PLPROP_RATING >> (int)0, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		}
		if (oldStats[2] != tWinRating || oldStats[3] != tWinDeviation)
		{
			killer->setRating((int)tWinRating, (int)tWinDeviation);
			killer->setProps(CString() >> (char)PLPROP_RATING >> (int)0, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		}
		this->setLastSparTime(time(nullptr));
		killer->setLastSparTime(time(nullptr));
	}
	else
	{
		const CSettings& settings = server->getSettings();

		// Give a kill to the player who killed me.
		if (!settings.getBool("dontchangekills", false))
			killer->setKills(killer->getProp(PLPROP_KILLSCOUNT).readGInt() + 1);

		// Now, adjust their AP if allowed.
		if (settings.getBool("apsystem", true))
		{
			signed char oAp = killer->getProp(PLPROP_ALIGNMENT).readGChar();

			// If I have 20 or more AP, they lose AP.
			if (oAp > 0 && ap > 19)
			{
				const int aptime[] = {settings.getInt("aptime0", 30), settings.getInt("aptime1", 90),
					settings.getInt("aptime2", 300), settings.getInt("aptime3", 600),
					settings.getInt("aptime4", 1200)};
				oAp -= (((oAp / 20) + 1) * (ap / 20));
				if (oAp < 0) oAp = 0;
				killer->setApCounter((oAp < 20 ? aptime[0] : (oAp < 40 ? aptime[1] : (oAp < 60 ? aptime[2] : (oAp < 80 ? aptime[3] : aptime[4])))));
				killer->setProps(CString() >> (char)PLPROP_ALIGNMENT >> (char)oAp, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
			}
		}
	}

	return true;
}

bool TPlayer::msgPLI_BADDYPROPS(CString& pPacket) {
	const auto level = getLevel();
	if (level == nullptr) return true;

	const unsigned char baddyId = pPacket.readGUChar();
	CString props = pPacket.readString("");

	// Get the baddy.
	TLevelBaddy* baddy = level->getBaddy(baddyId);
	if (baddy == nullptr) return true;

	// Get the leader.
	auto leaderId = level->getPlayerList().front();
	auto leader = server->getPlayer(leaderId);

	// Set the props and send to everybody in the level, except the leader.
	server->sendPacketToOneLevel({PLO_BADDYPROPS, CString() >> (char)baddyId << props}, level, { leaderId });
	baddy->setProps(props);
	return true;
}

bool TPlayer::msgPLI_BADDYHURT(CString& pPacket) {
	const auto level = getLevel();
	const auto leaderId = level->getPlayerList().front();
	const auto leader = server->getPlayer(leaderId);
	if (leader == nullptr) return true;
	leader->sendPacket({PLO_BADDYHURT, CString() << (pPacket.text() + 1)});
	return true;
}

bool TPlayer::msgPLI_BADDYADD(CString& pPacket)
{
	// Don't add a baddy if we aren't in a level!
	if (curlevel.expired())
		return true;

	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};
	unsigned char bType = pPacket.readGUChar();
	unsigned char bPower = pPacket.readGUChar();
	CString bImage = pPacket.readString("");
	bPower = MIN(bPower, 12);		// Hard-limit to 6 hearts.

	// Fix the image for 1.41 clients.
	if (!bImage.isEmpty() && getExtension(bImage).isEmpty())
		bImage << ".gif";

	// Add the baddy.
	auto level = getLevel();
	TLevelBaddy* baddy = level->addBaddy(loc[0], loc[1], bType);
	if (baddy == nullptr) return true;

	// Set the baddy props.
	baddy->setRespawn(false);
	baddy->setProps(CString() >> (char)BDPROP_POWERIMAGE >> (char)bPower >> (char)bImage.length() << bImage);

	// Send the props to everybody in the level.
	server->sendPacketToOneLevel({PLO_BADDYPROPS, CString() >> (char)baddy->getId() << baddy->getProps()}, level);
	return true;
}

bool TPlayer::msgPLI_FLAGSET(CString& pPacket)
{
	CSettings& settings = server->getSettings();
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
		if (flagName == "gr.fileerror" || flagName == "gr.filedata")
			return true;

		if ( settings.getBool("flaghack_movement", true))
		{
			// gr.x and gr.y are used by the -gr_movement NPC to help facilitate smoother
			// movement amongst pre-2.3 clients.
			if (flagName == "gr.x")
			{
				if (versionID >= CLVER_2_3) return true;
				auto pos = (float)atof(flagValue.text());
				if (pos != x)
					grMovementPackets >> (char)PLPROP_X >> (char)(pos * 2.0f) << "\n";
				return true;
			}
			else if (flagName == "gr.y")
			{
				if (versionID >= CLVER_2_3) return true;
				auto pos = (float)atof(flagValue.text());
				if (pos != y)
					grMovementPackets >> (char)PLPROP_Y >> (char)(pos * 2.0f) << "\n";
				return true;
			}
			else if (flagName == "gr.z")
			{
				if (versionID >= CLVER_2_3) return true;
				auto pos = (float)atof(flagValue.text());
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
		server->setFlag(flagName.text(), flagValue);
		return true;
	}

	// Set Flag
	setFlag(flagName.text(), flagValue, (versionID > CLVER_2_31));
	return true;
}

bool TPlayer::msgPLI_FLAGDEL(CString& pPacket)
{
	CString flagPacket = pPacket.readString("");
	std::string flagName;
	if (flagPacket.find("=") != -1)
		flagName = flagPacket.readString("=").trim().text();
	else flagName = flagPacket.text();

	// this.flags should never be in any server flag list, so just exit.
	if (flagName.find("this.") != std::string::npos) return true;

	// Don't allow anybody to alter read-only strings.
	if (flagName.find("clientr.") != std::string::npos) return true;
	if (flagName.find("serverr.") != std::string::npos) return true;

	// Server flags are handled differently than client flags.
	// TODO: check serveroptions
	if (flagName.find("server.") != std::string::npos)
	{
		server->deleteFlag(flagName);
		return true;
	}

	// Remove Flag
	deleteFlag(flagName);
	return true;
}

bool TPlayer::msgPLI_OPENCHEST(CString& pPacket)
{
	unsigned char cX = pPacket.readGUChar();
	unsigned char cY = pPacket.readGUChar();

	if (auto level = getLevel(); level)
	{
		auto chest = level->getChest(cX, cY);
		if (chest.has_value())
		{
			auto chestStr = level->getChestStr(chest.value());

			if (!hasChest(chestStr))
			{
				LevelItemType chestItem = chest.value()->getItemIndex();
				setProps(CString() << TLevelItem::getItemPlayerProp(chestItem, this), PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
				sendPacket({PLO_LEVELCHEST, CString() >> (char)1 >> (char)cX >> (char)cY});
				chestList.push_back(chestStr);
			}
		}
	}

	return true;
}

bool TPlayer::msgPLI_PUTNPC(CString& pPacket)
{
#ifdef V8NPCSERVER
	// Disable if we have an NPC-Server.
	return true;
#endif

	CSettings& settings = server->getSettings();

	CString nimage = pPacket.readChars(pPacket.readGUChar());
	CString ncode = pPacket.readChars(pPacket.readGUChar());
	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};

	// See if putnpc is allowed.
	if ( !settings.getBool("putnpcenabled"))
		return true;

	// Load the code.
	CString code = server->getFileSystem(0)->load(ncode);
	code.removeAllI("\r");
	code.replaceAllI("\n", "\xa7");

	// Add NPC to level
	server->addNPC(nimage, code, loc[0], loc[1], curlevel, true, true);

	return true;
}

bool TPlayer::msgPLI_NPCDEL(CString& pPacket)
{
#ifdef V8NPCSERVER
	// Disable if we have an NPC-Server.
	return true;
#endif

	unsigned int nid = pPacket.readGUInt();

	// Remove the NPC.
	if (auto npc = server->getNPC(nid); npc)
		server->deleteNPC(npc, !curlevel.expired());

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
	sendFile(file);
	return true;
}

bool TPlayer::msgPLI_SHOWIMG(CString& pPacket)
{
	server->sendPacketToLevelArea({PLO_SHOWIMG, CString() >> (short)id << (pPacket.text() + 1)}, this->shared_from_this(), { id });
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
	auto victim = server->getPlayer(pId, PLTYPE_ANYCLIENT);
	if (victim == nullptr) return true;

	// If they are paused, they don't get hurt.
	if (victim->getProp(PLPROP_STATUS).readGChar() & PLSTATUS_PAUSED) return true;

	// Send the packet.
	victim->sendPacket({PLO_HURTPLAYER, CString() >> (short)id >> (char)hurtdx >> (char)hurtdy >> (char)power >> (int)npc});

	return true;
}

bool TPlayer::msgPLI_EXPLOSION(CString& pPacket)
{
	CSettings& settings = server->getSettings();
	if ( settings.getBool("noexplosions", false)) return true;

	unsigned char eradius = pPacket.readGUChar();
	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};
	unsigned char epower = pPacket.readGUChar();

	// Send the packet out.
	CString packet = CString() >> (short)id >> (char)eradius >> (char)(loc[0] * 2) >> (char)(loc[1] * 2) >> (char)epower;
	server->sendPacketToOneLevel({PLO_EXPLOSION, packet}, curlevel, { id });

	return true;
}

bool TPlayer::msgPLI_PRIVATEMESSAGE(CString& pPacket)
{
	// TODO(joey): Is this needed?
	const int sendLimit = 4;
	if (isClient() && (int)difftime(time(nullptr), lastMessage) <= 4)
	{
		sendPacket({PLO_RC_ADMINMESSAGE, CString() << "Server message:\xa7You can only send messages once every " << CString((int)sendLimit) << " seconds."});
		return true;
	}
	lastMessage = time(nullptr);

	// Check if the player is in a jailed level.
	std::vector<CString> jailList = server->getSettings().getStr("jaillevels").tokenize(",");
	bool jailed = false;
	for (auto & jail : jailList)
	{
		if (jail.trim() == levelName)
		{
			jailed = true;
			break;
		}
	}

	// Get the players this message was addressed to.
	std::vector<uint16_t> pmPlayers;
	auto pmPlayerCount = pPacket.readGUShort();
	for (auto i = 0; i < pmPlayerCount; ++i)
		pmPlayers.push_back(static_cast<uint16_t>(pPacket.readGUShort()));

	// Start constructing the message based on if it is a mass message or a private message.
	CString pmMessageType("\"\",");
	if (pmPlayerCount > 1) pmMessageType << "\"Mass message:\",";
	else pmMessageType << "\"Private message:\",";

	// Grab the message.
	CString pmMessage = pPacket.readString("");
	int messageLimit = 1024;
	if (pmMessage.length() > messageLimit)
	{
		sendPacket({PLO_RC_ADMINMESSAGE, CString() << "Server message:\xa7There is a message limit of " << CString((int)messageLimit) << " characters."});
		return true;
	}

	// Word filter.
	pmMessage.guntokenizeI();
	if (isClient())
	{
		int filter = server->getWordFilter().apply(this, pmMessage, FILTER_CHECK_PM);
		if (filter & FILTER_ACTION_WARN)
		{
			sendPacket({PLO_RC_ADMINMESSAGE, CString() << "Word Filter:\xa7Your PM could not be sent because it was caught by the word filter."});
			return true;
		}
	}

	// Always retokenize string, I don't believe our behavior is inline with official. It was escaping "\", so this unescapes that.
	pmMessage.gtokenizeI();

	// Send the message out.
	for (auto pmPlayerId : pmPlayers)
	{
		if (pmPlayerId >= 16000)
		{
			auto pmPlayer = getExternalPlayer(pmPlayerId);
			if (pmPlayer != nullptr) {
				serverlog.out("Sending PM to global player: %s.\n", pmPlayer->getNickname().text());
				pmMessage.guntokenizeI();
				pmExternalPlayer(pmPlayer->getServerName(), pmPlayer->getAccountName(), pmMessage);
				pmMessage.gtokenizeI();
			}
		}
		else
		{
			auto pmPlayer = server->getPlayer(pmPlayerId, PLTYPE_ANYPLAYER | PLTYPE_NPCSERVER);
			if (pmPlayer == nullptr || pmPlayer.get() == this) continue;

#ifdef V8NPCSERVER
			if (pmPlayer->isNPCServer())
			{
				server->handlePM(this, pmMessage.guntokenize());
				continue;
			}
#endif

			// Don't send to people who don't want mass messages.
			if (pmPlayerCount != 1 && (pmPlayer->getProp(PLPROP_ADDITFLAGS).readGUChar() & PLFLAG_NOMASSMESSAGE))
				continue;

			// Jailed people cannot send PMs to normal players.
			if (jailed && !isStaff() && !pmPlayer->isStaff())
			{
				sendPacket({PLO_PRIVATEMESSAGE, CString() >> (short)pmPlayer->getId() << "\"Server Message:\"," << "\"From jail you can only send PMs to admins (RCs).\""});
				continue;
			}

			// Send the message.
			pmPlayer->sendPacket({PLO_PRIVATEMESSAGE, CString() >> (short)id << pmMessageType << pmMessage});
		}
	}

	return true;
}

bool TPlayer::msgPLI_NPCWEAPONDEL(CString& pPacket)
{
	CString weapon = pPacket.readString("");
	for (auto i = weaponList.begin(); i != weaponList.end(); )
	{
		if (*i == weapon)
		{
			i = weaponList.erase(i);
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
#ifdef V8NPCSERVER
	// Disable if we have an NPC-Server.
	return true;
#endif

	unsigned char weaponType = pPacket.readGUChar();

	// Type 0 means it is a default weapon.
	if (weaponType == 0)
	{
		this->addWeapon(TLevelItem::getItemId(pPacket.readGChar()));
	}
	// NPC weapons.
	else
	{
		// Get the NPC id.
		unsigned int npcId = pPacket.readGUInt();
		auto npc = server->getNPC(npcId);
		if (npc == nullptr || npc->getLevel() == nullptr)
			return true;

		// Get the name of the weapon.
		CString name = npc->getWeaponName();
		if (name.length() == 0)
			return true;

		// See if we can find the weapon in the server weapon list.
		auto weapon = server->getWeapon(name.toString());

		// If weapon is nullptr, that means the weapon was not found.  Add the weapon to the list.
		if (weapon == nullptr)
		{
			weapon = std::make_shared<TWeapon>(server, name.toString(), npc->getImage(), std::string{ npc->getSource().getClientGS1() }, npc->getLevel()->getModTime(), true);
			server->NC_AddWeapon(weapon);
		}

		// Check and see if the weapon has changed recently.  If it has, we should
		// send the new NPC to everybody on the server.  After updating the script, of course.
		if (weapon->getModTime() < npc->getLevel()->getModTime())
		{
			// Update Weapon
			weapon->updateWeapon(npc->getImage(), std::string{ npc->getSource().getClientGS1() }, npc->getLevel()->getModTime());

			// Send to Players
			server->updateWeaponForPlayers(weapon);
		}

		// Send the weapon to the player now.
		if (!hasWeapon(weapon->getName()))
			addWeapon(weapon);
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
	for (auto & defaultFile : __defaultfiles)
	{
		if (file.match(defaultFile))
		{
			isDefault = true;
			break;
		}
	}

	// If the file on disk is different, send it to the player.
	file.setRead(0);
	if (!isDefault)
	{
		if (std::difftime(modTime, fModTime) != 0)
			return msgPLI_WANTFILE(file);
	}

	if (versionID < CLVER_2_1)
		sendPacket({PLO_FILESENDFAILED, CString() << file});
	else sendPacket({PLO_FILEUPTODATE, CString() << file});
	return true;
}

bool TPlayer::msgPLI_ADJACENTLEVEL(CString& pPacket)
{
	time_t modTime = pPacket.readGUInt5();
	CString levelName = pPacket.readString("");
	CString packet;
	auto adjacentLevel = TLevel::findLevel(levelName, server);

	if (!adjacentLevel)
		return true;

	if (curlevel.expired())
		return false;

	bool alreadyVisited = false;
	for (const auto& cl : cachedLevels)
	{
		if (auto clevel = cl->level.lock(); clevel == adjacentLevel)
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
	auto map = pmap.lock();
	if (map && map->getType() == MapType::GMAP)
		sendPacket({PLO_LEVELNAME, CString() << map->getMapName()});
	else sendPacket({PLO_LEVELNAME, CString() << getLevel()->getLevelName()});

	if (getLevel()->isPlayerLeader(id))
		sendPacket({PLO_ISLEADER, CString()});

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
	nPacket >> (short)((nid == -1) ? id : 0);	// If it came from an NPC, send 0 for the id.
	nPacket >> (char)(power * 2) >> (char)(loc[0] * 2) >> (char)(loc[1] * 2);
	if (nid != -1) nPacket >> (int)nid;

	server->sendPacketToLevelArea({PLO_HITOBJECTS, nPacket}, shared_from_this(), { id });
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
	// Read packet data
	unsigned int npcId = pPacket.readGUInt();
	float loc[2] = {
		(float)pPacket.readGUChar() / 2.0f,
		(float)pPacket.readGUChar() / 2.0f
	};
	CString action = pPacket.readString("").trim();

	// Split action data into tokens
	std::vector<CString> triggerActionData = action.gCommaStrTokens();
	if (triggerActionData.empty()) {
		return true;
	}

	// Grab action name
	std::string actualActionName = triggerActionData[0].toLower().toString();

	// (int)(loc[0]) % 64 == 0.0f, for gmap?
	// TODO(joey): move into trigger command dispatcher, some use private player vars.
	if (loc[0] == 0.0f && loc[1] == 0.0f)
	{
		CSettings& settings = server->getSettings();

		if ( settings.getBool("triggerhack_execscript", false))
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
					CString weapon_packet = CString()
							>> (char)wepname.length() << wepname
							>> (char)0 >> (char)wepimage.length() << wepimage
							>> (char)1 >> (short)wepscript.length() << wepscript;

					// Send it to the players now.
					if (actionParts[1] == "ALLPLAYERS")
						server->sendPacketToType(PLTYPE_ANYCLIENT, {PLO_NPCWEAPONADD, weapon_packet});
					else
					{
						auto p = server->getPlayer(actionParts[1], PLTYPE_ANYCLIENT);
						if (p) p->sendPacket({PLO_NPCWEAPONADD, weapon_packet});
					}
					grExecParameterList.clear();
				}
				return true;
			}
		}

		if ( settings.getBool("triggerhack_files", false))
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

				// Grab the filename.
				CString filename = action.subString(start, finish - start - 1);
				filename.removeAllI("../");
				filename.removeAllI("..\\");

				// Save the file.
				CString file = action.subString(finish) << "\r\n";
				file.save(server->getServerPath() << "logs/" << filename);
				return true;
			}
			else if (action.find("gr.readfile") == 0)
			{
				int start = action.find(",") + 1;
				if (start == 0) return true;
				int finish = action.find(",", start) + 1;
				if (finish == 0) return true;

				// Grab the filename.
				CString filename = action.subString(start, finish - start - 1);
				filename.removeAllI("../");
				filename.removeAllI("..\\");

				// Load the file.
				CString filedata;
				filedata.load(server->getServerPath() << "logs/" << filename);
				filedata.removeAllI("\r");

				// Tokenize it.
				std::vector<CString> tokens = filedata.tokenize("\n");

				// Find the line.
				int id = rand() % 0xFFFF;
				CString error;
				unsigned int line = strtoint(action.subString(finish));
				if (line >= tokens.size())
				{
					// We asked for a line that doesn't exist.  Mark it as an error!
					line = tokens.size() - 1;
					error << CString("1,") + line;
				}

				// Check if an error was set.
				if (error.isEmpty())
					error = "0";

				// Apply the ID.
				error = CString(id) << "," << error;

				// Send it back to the player.
				sendPacket({PLO_FLAGSET, CString() << "gr.fileerror=" << error});
				sendPacket({PLO_FLAGSET, CString() << "gr.filedata=" << tokens[line]});
			}
		}

		if ( settings.getBool("triggerhack_props", false))
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
						setProps(CString() >> (char)(__attrPackets[attrNum - 1]) >> (char)val.length() << val, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
					}
				}
			}
			if (action.find("gr.fullhearts") == 0)
			{
				int start = action.find(",");
				if (start != -1)
				{
					++start;
					int hearts = strtoint(action.subString(start).trim());
					setProps(CString() >> (char)PLPROP_MAXPOWER >> (char)hearts, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
				}
			}
		}

		if ( settings.getBool("triggerhack_levels", false))
		{
			if (action.find("gr.updatelevel") == 0)
			{
				auto level = getLevel();
				int start = action.find(",");
				if (start != -1)
				{
					++start;
					CString levelName = action.subString(start).trim();
					if (levelName.isEmpty())
						level->reload();
					else
					{
						TLevelPtr targetLevel;
						if (getExtension(levelName) == ".singleplayer")
							targetLevel = spLevels[removeExtension(levelName)];
						else targetLevel = server->getLevel(levelName.toString());
						if (targetLevel != nullptr)
							targetLevel->reload();
					}
				}
				else level->reload();
			}
		}
	}

	bool handled = server->getTriggerDispatcher().execute(actualActionName, this, triggerActionData);

	if (!handled)
	{
		if (auto level = getLevel(); level)
		{
#ifdef V8NPCSERVER
			// Send to server scripts
			auto npcList = level->findAreaNpcs(int(loc[0] * 16.0), int(loc[1] * 16.0), 8, 8);
			for (auto npcTouched : npcList)
				npcTouched->queueNpcTrigger(actualActionName, this, utilities::retokenizeArray(triggerActionData, 1));
#endif

			// Send to the level.
			server->sendPacketToOneLevel({PLO_TRIGGERACTION, CString() >> (short)id << (pPacket.text() + 1)}, level, { id });
		}
	}

	return true;
}

bool TPlayer::msgPLI_MAPINFO(CString& pPacket)
{
	// Don't know what this does exactly.  Might be gmap related.
	pPacket.readString("");
	return true;
}

void ShootPacketNew::debug() {
	printf("Shoot: %f, %f, %f with gani %s: (len=%d)\n", (float)pixelx/16.0f, (float)pixely / 16.0f, (float)pixelz / 16.0f, gani.text(), gani.length());
	printf("\t Offset: %d, %d\n", offsetx, offsety);
	printf("\t Angle: %d\n", sangle);
	printf("\t Z-Angle: %d\n", sanglez);
	printf("\t Power: %d\n", speed);
	printf("\t Gravity: %d\n", gravity);
	printf("\t Gani: %s (len: %d)\n", gani.text(), gani.length());
	printf("\t Shoot Params: %s (len: %d)\n", shootParams.text(), shootParams.length());
}

CString ShootPacketNew::constructShootV1() const {
	CString ganiTemp{};
	ganiTemp << gani;
	if (!ganiArgs.isEmpty()) {
		ganiTemp << "," << ganiArgs;
	}
	CString packet;
	packet.writeGInt(0); // shoot-id?
	packet.writeGChar(pixelx / 16);
	packet.writeGChar(pixely / 16);
	packet.writeGChar((pixelz / 16) + 50);
	packet.writeGChar(sangle);
	packet.writeGChar(sanglez);
	packet.writeGChar(speed);
	packet.writeGChar(ganiTemp.length());
	packet.write(ganiTemp);
	packet.writeGChar(shootParams.length());
	packet.write(shootParams);
	return packet;
}

CString ShootPacketNew::constructShootV2() const {
	CString ganiTemp{};
	ganiTemp << gani;
	if (!ganiArgs.isEmpty()) {
		ganiTemp << "," << ganiArgs;
	}
	CString packet;
	packet.writeGShort(pixelx);
	packet.writeGShort(pixely);
	packet.writeGShort(pixelz);
	packet.writeChar(offsetx + 32);
	packet.writeChar(offsety + 32);
	packet.writeGChar(sangle);
	packet.writeGChar(sanglez);
	packet.writeGChar(speed);
	packet.writeGChar(gravity);
	packet.writeGShort(ganiTemp.length());
	packet.write(ganiTemp);
	packet.writeGChar(shootParams.length());
	packet.write(shootParams);
	return packet;
}

bool TPlayer::msgPLI_SHOOT(CString& pPacket)
{
	ShootPacketNew newPacket{};
	int unknown = pPacket.readGInt();        // This may be a shoot id for the npc-server. (5/25d/19) joey: all my tests just give 0, my guess would be different types of projectiles, but it never came to fruition

	newPacket.pixelx = (int16_t)(16 * pPacket.readGChar()); // 16 * ((float)pPacket.readGUChar() / 2.0f);
	newPacket.pixely = (int16_t)(16 * pPacket.readGChar()); // 16 * ((float)pPacket.readGUChar() / 2.0f);
	newPacket.pixelz = (int16_t)(16 * (pPacket.readGChar() - 50)); // 16 * ((float)pPacket.readGUChar() / 2.0f);
	// TODO: calculate offsetx from pixelx/pixely/ - level offset
	newPacket.offsetx = 0;
	newPacket.offsety = 0;
	//if (newPacket.pixelx < 0) {
	//	newPacket.offsetx = -1;
	//}
	//if (newPacket.pixely < 0) {
	//	newPacket.offsety = -1;
	//}
	newPacket.sangle = (int8_t)pPacket.readGUChar();        // 0-pi = 0-220
	newPacket.sanglez = (int8_t)pPacket.readGUChar();        // 0-pi = 0-220
	newPacket.speed = (int8_t)pPacket.readGUChar();            // speed = pixels per 0.05 seconds.  In gscript, each value of 1 translates to 44 pixels.
	newPacket.gravity = 8;
	newPacket.gani = pPacket.readChars(pPacket.readGUChar());
	unsigned char someParam = pPacket.readGUChar(); // This seems to be the length of shootparams, but the client doesn't limit itself and sends the overflow anyway
	newPacket.shootParams = pPacket.readString("");

	CString oldPacketBuf = CString() >> (short)id << newPacket.constructShootV1();
	CString newPacketBuf = CString() >> (short)id << newPacket.constructShootV2();

	server->sendPacketToLevelArea({PLO_SHOOT, oldPacketBuf}, shared_from_this(), {id}, [](const auto pl) { return pl->getVersion() < CLVER_5_07; });
	server->sendPacketToLevelArea({PLO_SHOOT2, newPacketBuf}, shared_from_this(), {id}, [](const auto pl) { return pl->getVersion() >= CLVER_5_07; });

	// ActionProjectile on server.
	// TODO(joey): This is accurate, but have not figured out power/zangle stuff yet.

	//this.speed = (this.power > 0 ? 0 : 20 * 0.05);
	//this.horzspeed = cos(this.zangle) * this.speed;
	//this.vertspeed = sin(this.zangle) * this.speed;
	//this.newx = playerx + 1.5; // offset
	//this.newy = playery + 2; // offset
	//function CalcPos() {
	//	this.newx = this.newx + (cos(this.angle) * this.horzspeed);
	//	this.newy = this.newy - (sin(this.angle) * this.horzspeed);
	//	setplayerprop #c, Positions #v(this.newx), #v(this.newy);
	//	if (onwall(this.newx, this.newy)) {
	//		this.calcpos = 0;
	//		this.hittime = timevar2;
	//	}
	//}

	return true;
}

bool TPlayer::msgPLI_SHOOT2(CString& pPacket)
{
	ShootPacketNew newPacket{};
	newPacket.pixelx = (int16_t)pPacket.readGUShort();
	newPacket.pixely = (int16_t)pPacket.readGUShort();
	newPacket.pixelz = (int16_t)pPacket.readGUShort();
	newPacket.offsetx = pPacket.readGChar();		// level offset x
	newPacket.offsety = pPacket.readGChar();		// level offset y
	newPacket.sangle = (int8_t)pPacket.readGUChar();		// 0-pi = 0-220
	newPacket.sanglez = (int8_t)pPacket.readGUChar();		// 0-pi = 0-220
	newPacket.speed = (int8_t)pPacket.readGUChar();			// speed = pixels per 0.05 seconds.  In gscript, each value of 1 translates to 44 pixels.
	newPacket.gravity = (int8_t)pPacket.readGUChar();
	newPacket.gani = pPacket.readChars(pPacket.readGUShort());
	unsigned char someParam = pPacket.readGUChar(); // This seems to be the length of shootparams, but the client doesn't limit itself and sends the overflow anyway
	newPacket.shootParams = pPacket.readString("");

	CString oldPacketBuf = CString() >> (short)id << newPacket.constructShootV1();
	CString newPacketBuf = CString() >> (short)id << newPacket.constructShootV2();

	server->sendPacketToLevelArea({PLO_SHOOT, oldPacketBuf}, shared_from_this(), {id}, [](const auto pl) { return pl->getVersion() < CLVER_5_07; });
	server->sendPacketToLevelArea({PLO_SHOOT2, newPacketBuf}, shared_from_this(), {id}, [](const auto pl) { return pl->getVersion() >= CLVER_5_07; });

	return true;
}

bool TPlayer::msgPLI_SERVERWARP(CString& pPacket)
{
	CString servername = pPacket.readString("");
	server->getServerLog().out("%s is requesting serverwarp to %s", accountName.text(), servername.text());
	server->getServerList().sendPacket({SVO_SERVERINFO, CString() >> (short)id << servername});
	return true;
}

bool TPlayer::msgPLI_PROCESSLIST(CString& pPacket)
{
	std::vector<CString> processes = pPacket.readString("").guntokenize().tokenize("\n");
	return true;
}

bool TPlayer::msgPLI_UNKNOWN46(CString& pPacket)
{
#ifdef DEBUG
	printf("TODO: TPlayer::msgPLI_UNKNOWN46: ");
	CString packet = pPacket.readString("");
	for (int i = 0; i < packet.length(); ++i) printf( "%02x ", (unsigned char)packet[i] ); printf( "\n" );
#endif
	return true;
}


bool TPlayer::msgPLI_RAWDATA(CString& pPacket)
{
	nextIsRaw = true;
	rawPacketSize = (int)pPacket.readGUInt();
	return true;
}

bool TPlayer::msgPLI_PROFILEGET(CString& pPacket)
{
	// Send the packet ID for backwards compatibility.
	server->getServerList().sendPacket({SVO_GETPROF, CString() >> (short)id << pPacket});
	return true;
}

bool TPlayer::msgPLI_PROFILESET(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc != accountName) return true;

	// Old gserver would send the packet ID with pPacket so, for
	// backwards compatibility, do that here.
	server->getServerList().sendPacket({SVO_SETPROF, CString() << pPacket});
	return true;
}

bool TPlayer::msgPLI_RC_UNKNOWN162(CString& pPacket)
{
	// Stub.
	return true;
}

