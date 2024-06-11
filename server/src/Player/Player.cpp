#include "IDebug.h"
#include <IConfig.h>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "Account.h"
#include "IConfig.h"
#include "IEnums.h"
#include "IUtil.h"
#include "Level.h"
#include "Map.h"
#include "NPC.h"
#include "Player.h"
#include "Server.h"
#include "Weapon.h"
#include "utilities/stringutils.h"

/*
	Logs
*/
#define serverlog m_server->getServerLog()
#define rclog m_server->getRCLog()

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
bool __sendLogin[propscount] = {
	false, true, true, true, true, true,    // 0-5
	true, false, true, true, true, true,    // 6-11
	false, true, false, false, false, true, // 12-17
	true, false, false, true, true, true,   // 18-23
	false, true, true, false, false, false, // 24-29
	false, false, true, false, true, true,  // 30-35
	true, true, true, true, true, true,     // 36-41
	false, false, false, false, true, true, // 42-47
	true, true, false, false, false, false, // 48-53
	true, true, true, true, true, true,     // 54-59
	true, true, true, true, true, true,     // 60-65
	true, true, true, true, true, true,     // 66-71
	true, true, true, false, false, false,  // 72-77
	false, false, false, false, true,       // 78-82
};

bool __getLogin[propscount] = {
	true, false, false, false, false, false, // 0-5
	false, false, true, true, true, true,    // 6-11
	true, true, false, true, true, true,     // 12-17
	true, true, true, true, false, false,    // 18-23
	true, false, false, false, false, false, // 24-29
	true, true, true, false, true, true,     // 30-35
	true, true, true, true, true, true,      // 36-41
	false, true, true, true, true, true,     // 42-47
	true, true, true, false, false, true,    // 48-53
	true, true, true, true, true, true,      // 54-59
	true, true, true, true, true, true,      // 60-65
	true, true, true, true, true, true,      // 66-71
	true, true, true, false, false, false,   // 72-77
	true, true, true, false, true,           // 78-82
};

// Turn prop 14 off to see the npc-server's profile.
bool __getLoginNC[propscount] = {
	true, true, true, true, true, true,   // 0-5
	true, true, true, true, true, true,   // 6-11
	true, true, true, true, true, true,   // 12-17
	true, true, true, true, true, true,   // 18-23
	true, true, true, true, true, true,   // 24-29
	true, false, true, true, true, true,  // 30-35
	true, true, true, true, true, true,   // 36-41
	false, true, true, true, true, true,  // 42-47
	true, true, true, false, true, true,  // 48-53
	true, true, true, true, true, true,   // 54-59
	true, true, true, true, true, true,   // 60-65
	true, true, true, true, true, true,   // 66-71
	true, true, true, true, false, false, // 72-77
	true, true, true, false, false,       // 78-82
};

bool __getRCLogin[propscount] = {
	true, false, false, false, false, false,  // 0-5
	false, false, false, false, false, true,  // 6-11
	false, false, false, false, false, false, // 12-17
	true, false, true, false, false, false,   // 18-23
	false, false, false, false, false, false, // 24-29
	true, true, false, false, true, false,    // 30-35
	false, false, false, false, false, false, // 36-41
	false, false, false, false, false, false, // 42-47
	false, false, false, false, false, true,  // 48-53
	false, false, false, false, false, false, // 54-59
	false, false, false, false, false, false, // 60-65
	false, false, false, false, false, false, // 66-71
	false, false, false, false, false, false, // 72-77
	false, false, false, false, true,         // 78-82
};

bool __sendLocal[propscount] = {
	false, false, true, false, false, false, // 0-5
	false, false, true, true, true, true,    // 6-11
	true, true, false, true, true, true,     // 12-17
	true, true, true, true, false, false,    // 18-23
	true, true, false, false, false, false,  // 24-29
	true, true, true, false, true, true,     // 30-35
	true, true, true, true, true, true,      // 36-41
	false, true, true, true, true, true,     // 42-47
	true, true, true, false, false, true,    // 48-53
	true, true, true, true, true, true,      // 54-59
	true, true, true, true, true, true,      // 60-65
	true, true, true, true, true, true,      // 66-71
	true, true, true, false, false, false,   // 72-77
	true, true, true, false, true,           // 78-82
};

bool __playerPropsRC[propscount] = {
	true, true, true, true, true, true,       // 0-5
	true, false, true, true, true, true,      // 6-11
	false, true, false, true, true, false,    // 12-17
	true, false, true, false, false, false,   // 18-23
	false, false, true, true, true, true,     // 24-29
	true, false, true, false, true, true,     // 30-35
	true, false, false, false, false, false,  // 36-41
	false, false, false, false, false, false, // 42-47
	false, false, false, false, false, false, // 48-53
	false, false, false, false, false, false, // 54-59
	false, false, false, false, false, false, // 60-65
	false, false, false, false, false, false, // 66-71
	false, false, false, false, false, false, // 72-77
	false, false, false, false, false,        // 78-82
};

/*
	Pointer-Functions for Packets
*/
bool Player::created = false;
typedef bool (Player::*TPLSock)(CString&);
std::vector<TPLSock> TPLFunc(256, &Player::msgPLI_NULL);

void Player::createFunctions()
{
	if (Player::created)
		return;

	// now set non-nulls
	TPLFunc[PLI_LEVELWARP] = &Player::msgPLI_LEVELWARP;
	TPLFunc[PLI_BOARDMODIFY] = &Player::msgPLI_BOARDMODIFY;
	TPLFunc[PLI_REQUESTUPDATEBOARD] = &Player::msgPLI_REQUESTUPDATEBOARD;
	TPLFunc[PLI_PLAYERPROPS] = &Player::msgPLI_PLAYERPROPS;
	TPLFunc[PLI_NPCPROPS] = &Player::msgPLI_NPCPROPS;
	TPLFunc[PLI_BOMBADD] = &Player::msgPLI_BOMBADD;
	TPLFunc[PLI_BOMBDEL] = &Player::msgPLI_BOMBDEL;
	TPLFunc[PLI_TOALL] = &Player::msgPLI_TOALL;
	TPLFunc[PLI_HORSEADD] = &Player::msgPLI_HORSEADD;
	TPLFunc[PLI_HORSEDEL] = &Player::msgPLI_HORSEDEL;
	TPLFunc[PLI_ARROWADD] = &Player::msgPLI_ARROWADD;
	TPLFunc[PLI_FIRESPY] = &Player::msgPLI_FIRESPY;
	TPLFunc[PLI_THROWCARRIED] = &Player::msgPLI_THROWCARRIED;
	TPLFunc[PLI_ITEMADD] = &Player::msgPLI_ITEMADD;
	TPLFunc[PLI_ITEMDEL] = &Player::msgPLI_ITEMDEL;
	TPLFunc[PLI_CLAIMPKER] = &Player::msgPLI_CLAIMPKER;
	TPLFunc[PLI_BADDYPROPS] = &Player::msgPLI_BADDYPROPS;
	TPLFunc[PLI_BADDYHURT] = &Player::msgPLI_BADDYHURT;
	TPLFunc[PLI_BADDYADD] = &Player::msgPLI_BADDYADD;
	TPLFunc[PLI_FLAGSET] = &Player::msgPLI_FLAGSET;
	TPLFunc[PLI_FLAGDEL] = &Player::msgPLI_FLAGDEL;
	TPLFunc[PLI_OPENCHEST] = &Player::msgPLI_OPENCHEST;
	TPLFunc[PLI_PUTNPC] = &Player::msgPLI_PUTNPC;
	TPLFunc[PLI_NPCDEL] = &Player::msgPLI_NPCDEL;
	TPLFunc[PLI_WANTFILE] = &Player::msgPLI_WANTFILE;
	TPLFunc[PLI_SHOWIMG] = &Player::msgPLI_SHOWIMG;

	TPLFunc[PLI_HURTPLAYER] = &Player::msgPLI_HURTPLAYER;
	TPLFunc[PLI_EXPLOSION] = &Player::msgPLI_EXPLOSION;
	TPLFunc[PLI_PRIVATEMESSAGE] = &Player::msgPLI_PRIVATEMESSAGE;
	TPLFunc[PLI_NPCWEAPONDEL] = &Player::msgPLI_NPCWEAPONDEL;
	TPLFunc[PLI_LEVELWARPMOD] = &Player::msgPLI_LEVELWARP; // Shared with PLI_LEVELWARP
	TPLFunc[PLI_PACKETCOUNT] = &Player::msgPLI_PACKETCOUNT;
	TPLFunc[PLI_ITEMTAKE] = &Player::msgPLI_ITEMDEL; // Shared with PLI_ITEMDEL
	TPLFunc[PLI_WEAPONADD] = &Player::msgPLI_WEAPONADD;
	TPLFunc[PLI_UPDATEFILE] = &Player::msgPLI_UPDATEFILE;
	TPLFunc[PLI_ADJACENTLEVEL] = &Player::msgPLI_ADJACENTLEVEL;
	TPLFunc[PLI_HITOBJECTS] = &Player::msgPLI_HITOBJECTS;
	TPLFunc[PLI_LANGUAGE] = &Player::msgPLI_LANGUAGE;
	TPLFunc[PLI_TRIGGERACTION] = &Player::msgPLI_TRIGGERACTION;
	TPLFunc[PLI_MAPINFO] = &Player::msgPLI_MAPINFO;
	TPLFunc[PLI_SHOOT] = &Player::msgPLI_SHOOT;
	TPLFunc[PLI_SHOOT2] = &Player::msgPLI_SHOOT2;
	TPLFunc[PLI_SERVERWARP] = &Player::msgPLI_SERVERWARP;

	TPLFunc[PLI_PROCESSLIST] = &Player::msgPLI_PROCESSLIST;

	TPLFunc[PLI_UNKNOWN46] = &Player::msgPLI_UNKNOWN46;
	TPLFunc[PLI_VERIFYWANTSEND] = &Player::msgPLI_VERIFYWANTSEND;
	TPLFunc[PLI_UPDATECLASS] = &Player::msgPLI_UPDATECLASS;
	TPLFunc[PLI_RAWDATA] = &Player::msgPLI_RAWDATA;

	TPLFunc[PLI_RC_SERVEROPTIONSGET] = &Player::msgPLI_RC_SERVEROPTIONSGET;
	TPLFunc[PLI_RC_SERVEROPTIONSSET] = &Player::msgPLI_RC_SERVEROPTIONSSET;
	TPLFunc[PLI_RC_FOLDERCONFIGGET] = &Player::msgPLI_RC_FOLDERCONFIGGET;
	TPLFunc[PLI_RC_FOLDERCONFIGSET] = &Player::msgPLI_RC_FOLDERCONFIGSET;
	TPLFunc[PLI_RC_RESPAWNSET] = &Player::msgPLI_RC_RESPAWNSET;
	TPLFunc[PLI_RC_HORSELIFESET] = &Player::msgPLI_RC_HORSELIFESET;
	TPLFunc[PLI_RC_APINCREMENTSET] = &Player::msgPLI_RC_APINCREMENTSET;
	TPLFunc[PLI_RC_BADDYRESPAWNSET] = &Player::msgPLI_RC_BADDYRESPAWNSET;
	TPLFunc[PLI_RC_PLAYERPROPSGET] = &Player::msgPLI_RC_PLAYERPROPSGET;
	TPLFunc[PLI_RC_PLAYERPROPSSET] = &Player::msgPLI_RC_PLAYERPROPSSET;
	TPLFunc[PLI_RC_DISCONNECTPLAYER] = &Player::msgPLI_RC_DISCONNECTPLAYER;
	TPLFunc[PLI_RC_UPDATELEVELS] = &Player::msgPLI_RC_UPDATELEVELS;
	TPLFunc[PLI_RC_ADMINMESSAGE] = &Player::msgPLI_RC_ADMINMESSAGE;
	TPLFunc[PLI_RC_PRIVADMINMESSAGE] = &Player::msgPLI_RC_PRIVADMINMESSAGE;
	TPLFunc[PLI_RC_LISTRCS] = &Player::msgPLI_RC_LISTRCS;
	TPLFunc[PLI_RC_DISCONNECTRC] = &Player::msgPLI_RC_DISCONNECTRC;
	TPLFunc[PLI_RC_APPLYREASON] = &Player::msgPLI_RC_APPLYREASON;
	TPLFunc[PLI_RC_SERVERFLAGSGET] = &Player::msgPLI_RC_SERVERFLAGSGET;
	TPLFunc[PLI_RC_SERVERFLAGSSET] = &Player::msgPLI_RC_SERVERFLAGSSET;
	TPLFunc[PLI_RC_ACCOUNTADD] = &Player::msgPLI_RC_ACCOUNTADD;
	TPLFunc[PLI_RC_ACCOUNTDEL] = &Player::msgPLI_RC_ACCOUNTDEL;
	TPLFunc[PLI_RC_ACCOUNTLISTGET] = &Player::msgPLI_RC_ACCOUNTLISTGET;
	TPLFunc[PLI_RC_PLAYERPROPSGET2] = &Player::msgPLI_RC_PLAYERPROPSGET2;
	TPLFunc[PLI_RC_PLAYERPROPSGET3] = &Player::msgPLI_RC_PLAYERPROPSGET3;
	TPLFunc[PLI_RC_PLAYERPROPSRESET] = &Player::msgPLI_RC_PLAYERPROPSRESET;
	TPLFunc[PLI_RC_PLAYERPROPSSET2] = &Player::msgPLI_RC_PLAYERPROPSSET2;
	TPLFunc[PLI_RC_ACCOUNTGET] = &Player::msgPLI_RC_ACCOUNTGET;
	TPLFunc[PLI_RC_ACCOUNTSET] = &Player::msgPLI_RC_ACCOUNTSET;
	TPLFunc[PLI_RC_CHAT] = &Player::msgPLI_RC_CHAT;
	TPLFunc[PLI_PROFILEGET] = &Player::msgPLI_PROFILEGET;
	TPLFunc[PLI_PROFILESET] = &Player::msgPLI_PROFILESET;
	TPLFunc[PLI_RC_WARPPLAYER] = &Player::msgPLI_RC_WARPPLAYER;
	TPLFunc[PLI_RC_PLAYERRIGHTSGET] = &Player::msgPLI_RC_PLAYERRIGHTSGET;
	TPLFunc[PLI_RC_PLAYERRIGHTSSET] = &Player::msgPLI_RC_PLAYERRIGHTSSET;
	TPLFunc[PLI_RC_PLAYERCOMMENTSGET] = &Player::msgPLI_RC_PLAYERCOMMENTSGET;
	TPLFunc[PLI_RC_PLAYERCOMMENTSSET] = &Player::msgPLI_RC_PLAYERCOMMENTSSET;
	TPLFunc[PLI_RC_PLAYERBANGET] = &Player::msgPLI_RC_PLAYERBANGET;
	TPLFunc[PLI_RC_PLAYERBANSET] = &Player::msgPLI_RC_PLAYERBANSET;
	TPLFunc[PLI_RC_FILEBROWSER_START] = &Player::msgPLI_RC_FILEBROWSER_START;
	TPLFunc[PLI_RC_FILEBROWSER_CD] = &Player::msgPLI_RC_FILEBROWSER_CD;
	TPLFunc[PLI_RC_FILEBROWSER_END] = &Player::msgPLI_RC_FILEBROWSER_END;
	TPLFunc[PLI_RC_FILEBROWSER_DOWN] = &Player::msgPLI_RC_FILEBROWSER_DOWN;
	TPLFunc[PLI_RC_FILEBROWSER_UP] = &Player::msgPLI_RC_FILEBROWSER_UP;
	TPLFunc[PLI_NPCSERVERQUERY] = &Player::msgPLI_NPCSERVERQUERY;
	TPLFunc[PLI_RC_FILEBROWSER_MOVE] = &Player::msgPLI_RC_FILEBROWSER_MOVE;
	TPLFunc[PLI_RC_FILEBROWSER_DELETE] = &Player::msgPLI_RC_FILEBROWSER_DELETE;
	TPLFunc[PLI_RC_FILEBROWSER_RENAME] = &Player::msgPLI_RC_FILEBROWSER_RENAME;
	TPLFunc[PLI_RC_LARGEFILESTART] = &Player::msgPLI_RC_LARGEFILESTART;
	TPLFunc[PLI_RC_LARGEFILEEND] = &Player::msgPLI_RC_LARGEFILEEND;
	TPLFunc[PLI_RC_FOLDERDELETE] = &Player::msgPLI_RC_FOLDERDELETE;
	TPLFunc[PLI_REQUESTTEXT] = &Player::msgPLI_REQUESTTEXT;
	TPLFunc[PLI_SENDTEXT] = &Player::msgPLI_SENDTEXT;
	TPLFunc[PLI_UPDATEGANI] = &Player::msgPLI_UPDATEGANI;
	TPLFunc[PLI_UPDATESCRIPT] = &Player::msgPLI_UPDATESCRIPT;
	TPLFunc[PLI_UPDATEPACKAGEREQUESTFILE] = &Player::msgPLI_UPDATEPACKAGEREQUESTFILE;
	TPLFunc[PLI_RC_UNKNOWN162] = &Player::msgPLI_RC_UNKNOWN162;

	// NPC-Server Functions
#ifdef V8NPCSERVER
	TPLFunc[PLI_NC_NPCGET] = &Player::msgPLI_NC_NPCGET;
	TPLFunc[PLI_NC_NPCDELETE] = &Player::msgPLI_NC_NPCDELETE;
	TPLFunc[PLI_NC_NPCRESET] = &Player::msgPLI_NC_NPCRESET;
	TPLFunc[PLI_NC_NPCSCRIPTGET] = &Player::msgPLI_NC_NPCSCRIPTGET;
	TPLFunc[PLI_NC_NPCWARP] = &Player::msgPLI_NC_NPCWARP;
	TPLFunc[PLI_NC_NPCFLAGSGET] = &Player::msgPLI_NC_NPCFLAGSGET;
	TPLFunc[PLI_NC_NPCSCRIPTSET] = &Player::msgPLI_NC_NPCSCRIPTSET;
	TPLFunc[PLI_NC_NPCFLAGSSET] = &Player::msgPLI_NC_NPCFLAGSSET;
	TPLFunc[PLI_NC_NPCADD] = &Player::msgPLI_NC_NPCADD;
	TPLFunc[PLI_NC_CLASSEDIT] = &Player::msgPLI_NC_CLASSEDIT;
	TPLFunc[PLI_NC_CLASSADD] = &Player::msgPLI_NC_CLASSADD;
	TPLFunc[PLI_NC_LOCALNPCSGET] = &Player::msgPLI_NC_LOCALNPCSGET;
	TPLFunc[PLI_NC_WEAPONLISTGET] = &Player::msgPLI_NC_WEAPONLISTGET;
	TPLFunc[PLI_NC_WEAPONGET] = &Player::msgPLI_NC_WEAPONGET;
	TPLFunc[PLI_NC_WEAPONADD] = &Player::msgPLI_NC_WEAPONADD;
	TPLFunc[PLI_NC_WEAPONDELETE] = &Player::msgPLI_NC_WEAPONDELETE;
	TPLFunc[PLI_NC_CLASSDELETE] = &Player::msgPLI_NC_CLASSDELETE;
	TPLFunc[PLI_NC_LEVELLISTGET] = &Player::msgPLI_NC_LEVELLISTGET;
#endif

	// Finished
	Player::created = true;
}

/*
	Constructor - Deconstructor
*/
Player::Player(Server* pServer, CSocket* pSocket, uint16_t pId)
	: Account(pServer),
	  m_playerSock(pSocket), m_encryptionKey(0),
	  m_os("wind"), m_envCodePage(1252),
	  m_id(pId), m_type(PLTYPE_AWAIT), m_versionId(CLVER_UNKNOWN),
	  m_carryNpcId(0), m_carryNpcThrown(false), m_loaded(false),
	  m_nextIsRaw(false), m_rawPacketSize(0), m_isFtp(false),
	  m_grMovementUpdated(false),
	  m_fileQueue(pSocket),
	  m_packetCount(0), m_firstLevel(true), m_invalidPackets(0)
#ifdef V8NPCSERVER
	  ,
	  m_processRemoval(false)
#endif
{
	m_lastData = m_lastMovement = m_lastSave = m_last1m = time(0);
	m_lastChat = m_lastMessage = m_lastNick = 0;
	m_isExternal = false;
	m_serverName = m_server->getName();
	m_nextExternalPlayerId = 16000;

	srand((unsigned int)time(0));

	// Create Functions
	if (!Player::created)
		Player::createFunctions();
}

Player::~Player()
{
	cleanup();
}

void Player::cleanup()
{
	if (m_playerSock == nullptr)
		return;

	// Send all unsent data (for disconnect messages and whatnot).
	m_fileQueue.sendCompress();

	if (m_id >= 0 && m_server != nullptr && m_loaded)
	{
		// Save account.
		if (isClient() && !m_isLoadOnly)
			saveAccount();

		// Remove from the level.
		if (!m_currentLevel.expired()) leaveLevel();

		// Announce our departure to other clients.
		if (!isNC())
		{
			m_server->sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id >> (char)PLPROP_PCONNECTED, this);
			m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_DELPLAYER >> (short)m_id, this);
		}

		if (!m_accountName.isEmpty())
		{
			if (isRC())
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "RC Disconnected: " << m_accountName, this);
			else if (isNC())
				m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << "NC Disconnected: " << m_accountName, this);
		}

		// Log.
		if (isClient())
			serverlog.out("[%s] :: Client disconnected: %s\n", m_server->getName().text(), m_accountName.text());
		else if (isRC())
			serverlog.out("[%s] :: RC disconnected: %s\n", m_server->getName().text(), m_accountName.text());
		else if (isNC())
			serverlog.out("[%s] :: NC disconnected: %s\n", m_server->getName().text(), m_accountName.text());
	}

	// Clean up.
	m_cachedLevels.clear();
	m_singleplayerLevels.clear();

	if (m_playerSock)
		delete m_playerSock;
	m_playerSock = nullptr;

#ifdef V8NPCSERVER
	if (m_scriptObject)
	{
		m_scriptObject.reset();
	}
#endif
}

bool Player::onRecv()
{
	// If our socket is gone, delete ourself.
	if (m_playerSock == 0 || m_playerSock->getState() == SOCKET_STATE_DISCONNECTED)
		return false;

	// Grab the data from the socket and put it into our receive buffer.
	unsigned int size = 0;
	char* data = m_playerSock->getData(&size);
	if (size != 0)
	{
		m_recvBuffer.write(data, size);
#if defined(WOLFSSL_ENABLED)
		if (this->m_playerSock->webSocket)
			if (webSocketFixIncomingPacket(m_recvBuffer) < 0) return true;
#endif
	}
	else if (m_playerSock->getState() == SOCKET_STATE_DISCONNECTED)
		return false;

	// Do the main function.
	return doMain();
}

bool Player::onSend()
{
	if (m_playerSock == 0 || m_playerSock->getState() == SOCKET_STATE_DISCONNECTED)
		return false;

	// Send data.
	m_fileQueue.sendCompress();

	return true;
}

void Player::onUnregister()
{
	// Called when onSend() or onRecv() returns false.
	m_server->deletePlayer(shared_from_this());
}

bool Player::canRecv()
{
	if (m_playerSock->getState() == SOCKET_STATE_DISCONNECTED) return false;
	return true;
}

bool Player::canSend()
{
	return m_fileQueue.canSend();
}

/*
	Socket-Control Functions
*/
bool Player::doMain()
{
	// definitions
	CString unBuffer;

	// parse data
	m_recvBuffer.setRead(0);
	while (m_recvBuffer.length() > 1)
	{
#if defined(WOLFSSL_ENABLED)
		if (!this->m_playerSock->webSocket && m_recvBuffer.findi("GET /") > -1 && m_recvBuffer.findi("HTTP/1.1\r\n") > -1)
		{

			CString webSocketKeyHeader = "Sec-WebSocket-Key:";
			if (m_recvBuffer.findi(webSocketKeyHeader) < 0)
			{
				CString simpleHtml = CString() << "<html><head><title>" APP_VENDOR " " APP_NAME " v" APP_VERSION "</title></head><body><h1>Welcome to " << m_server->getSettings().getStr("name") << "!</h1>" << m_server->getServerMessage().replaceAll("my server", m_server->getSettings().getStr("name")).text() << "<p style=\"font-style: italic;font-weight: bold;\">Powered by " APP_VENDOR " " APP_NAME "<br/>Programmed by " << CString(APP_CREDITS) << "</p></body></html>";
				CString webResponse = CString() << "HTTP/1.1 200 OK\r\nServer: " APP_VENDOR " " APP_NAME " v" APP_VERSION "\r\nContent-Length: " << CString(simpleHtml.length()) << "\r\nContent-Type: text/html\r\n\r\n"
												<< simpleHtml << "\r\n";
				unsigned int dsize = webResponse.length();
				this->m_playerSock->sendData(webResponse.text(), &dsize);
				return false;
			}
			this->m_playerSock->webSocket = true;
			// Get the WebSocket handshake key
			m_recvBuffer.setRead(m_recvBuffer.findi(webSocketKeyHeader));
			CString webSocketKey = m_recvBuffer.readString("\r").subString(webSocketKeyHeader.length() + 1).trimI();

			// Append GUID
			webSocketKey << "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

			// Calculate sha1 has of key + GUID and base64 encode it for sending back
			webSocketKey.sha1I().base64encodeI();
			webSocketKeyHeader.clear();

			CString webSockHandshake = CString() << "HTTP/1.1 101 Switching Protocols\r\n"
												 << "Upgrade: websocket\r\n"
												 << "Connection: Upgrade\r\n"
												 << "Sec-WebSocket-Protocol: binary\r\n"
												 << "Sec-WebSocket-Accept: "
												 << webSocketKey
												 << "\r\n\r\n";

			unsigned int dsize = webSockHandshake.length();

			this->m_playerSock->sendData(webSockHandshake.text(), &dsize);

			m_recvBuffer.removeI(0, m_recvBuffer.length());
			return true;
		}
#endif
		// New data.
		m_lastData = time(0);

		// packet length
		auto len = (unsigned short)m_recvBuffer.readShort();
		if ((unsigned int)len > (unsigned int)m_recvBuffer.length() - 2)
			break;

		// get packet
		unBuffer = m_recvBuffer.readChars(len);
		m_recvBuffer.removeI(0, len + 2);

		// decrypt packet
		switch (m_encryptionCodecIn.getGen())
		{
			case ENCRYPT_GEN_1: // Gen 1 is not encrypted or compressed.
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
	if (!m_grMovementPackets.isEmpty())
	{
		if (!m_grMovementUpdated)
		{
			std::vector<CString> pack = m_grMovementPackets.tokenize("\n");
			for (auto& i: pack)
				setProps(i, PLSETPROPS_FORWARD);
		}
		m_grMovementPackets.clear(42);
	}
	m_grMovementUpdated = false;

	m_server->getSocketManager().updateSingle(this, false, true);
	return true;
}

bool Player::doTimedEvents()
{
	time_t currTime = time(0);

	// If we are disconnected, delete ourself!
	if (m_playerSock == 0 || m_playerSock->getState() == SOCKET_STATE_DISCONNECTED)
	{
		m_server->deletePlayer(shared_from_this());
		return false;
	}

	// Only run for clients.
	if (!isClient()) return true;

	// Increase online time.
	m_onlineTime++;

	// Disconnect if players are inactive.
	CSettings& settings = m_server->getSettings();
	if (settings.getBool("disconnectifnotmoved"))
	{
		int maxnomovement = settings.getInt("maxnomovement", 1200);
		if (((int)difftime(currTime, m_lastMovement) > maxnomovement) && ((int)difftime(currTime, m_lastChat) > maxnomovement))
		{
			serverlog.out("[%s] Client %s has been disconnected due to inactivity.\n", m_server->getName().text(), m_accountName.text());
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You have been disconnected due to inactivity.");
			return false;
		}
	}

	// Disconnect if no data has been received in 5 minutes.
	if ((int)difftime(currTime, m_lastData) > 300)
	{
		serverlog.out("[%s] Client %s has timed out.\n", m_server->getName().text(), m_accountName.text());
		return false;
	}

	// Increase player AP.
	if (settings.getBool("apsystem") && !m_currentLevel.expired())
	{
		auto level = getLevel();
		if (level)
		{
			if (!(m_status & PLSTATUS_PAUSED) && !level->isSparringZone())
				m_apCounter--;

			if (m_apCounter <= 0)
			{
				if (m_ap < 100)
				{
					m_ap++;
					setProps(CString() >> (char)PLPROP_ALIGNMENT >> (char)m_ap, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
				}
				if (m_ap < 20) m_apCounter = settings.getInt("aptime0", 30);
				else if (m_ap < 40)
					m_apCounter = settings.getInt("aptime1", 90);
				else if (m_ap < 60)
					m_apCounter = settings.getInt("aptime2", 300);
				else if (m_ap < 80)
					m_apCounter = settings.getInt("aptime3", 600);
				else
					m_apCounter = settings.getInt("aptime4", 1200);
			}
		}
	}

	// Do singleplayer level events.
	{
		for (auto& spLevel: m_singleplayerLevels)
		{
			auto& level = spLevel.second;
			if (level)
				level->doTimedEvents();
		}
	}

	// Save player account every 5 minutes.
	if ((int)difftime(currTime, m_lastSave) > 300)
	{
		m_lastSave = currTime;
		if (isClient() && m_loaded && !m_isLoadOnly) saveAccount();
	}

	// Events that happen every minute.
	if ((int)difftime(currTime, m_last1m) > 60)
	{
		m_last1m = currTime;
		m_invalidPackets = 0;
	}

	return true;
}

void Player::disconnect()
{
	m_server->deletePlayer(shared_from_this());
	//m_server->getSocketManager()->unregisterSocket(this);
}

bool Player::parsePacket(CString& pPacket)
{
	// First packet is always unencrypted zlib.  Read it in a special way.
	if (m_type == PLTYPE_AWAIT)
	{
		m_packetCount++;
		if (!msgPLI_LOGIN(CString() << pPacket.readString("\n")))
			return false;
	}

	while (pPacket.bytesLeft() > 0)
	{
		// Grab a packet out of the input stream.
		CString curPacket;
		if (m_nextIsRaw)
		{
			m_nextIsRaw = false;
			curPacket = pPacket.readChars(m_rawPacketSize);

			// The client and RC versions above 1.1 append a \n to the end of the packet.
			// Remove it now.
			if (isClient() || (isRC() && m_versionId > RCVER_1_1))
			{
				if (curPacket[curPacket.length() - 1] == '\n')
					curPacket.removeI(curPacket.length() - 1);
			}
		}
		else
			curPacket = pPacket.readString("\n");

		// Generation 3 encrypts individual packets so decrypt it now.
		if (m_encryptionCodecIn.getGen() == ENCRYPT_GEN_3)
			decryptPacket(curPacket);

		// Get the packet id.
		unsigned char id = curPacket.readGUChar();

		// RC version 1.1 adds a "\n" string to the end of file uploads instead of a newline character.
		// This causes issues because it messes with the packet order.
		if (isRC() && m_versionId == RCVER_1_1 && id == PLI_RC_FILEBROWSER_UP)
		{
			curPacket.removeI(curPacket.length() - 1);
			curPacket.setRead(1);
			pPacket.readChar(); // Read out the n that got left behind.
		}

		// Call the function assigned to the packet id.
		m_packetCount++;
		//printf("Packet: (%i) %s\n", id, curPacket.text() + 1);

		// Forwards packets from server back to client as rc chat (for debugging)
		//sendPacket(CString() >> (char)PLO_RC_CHAT << "Server Data [" << CString(id) << "]:" << (curPacket.text() + 1));
		if (!(*this.*TPLFunc[id])(curPacket))
			return false;
	}

	return true;
}

void Player::decryptPacket(CString& pPacket)
{
	// Version 1.41 - 2.18 encryption
	// Was already decompressed so just decrypt the packet.
	if (m_encryptionCodecIn.getGen() == ENCRYPT_GEN_3)
	{
		if (!isClient())
			return;

		m_encryptionCodecIn.decrypt(pPacket);
	}

	// Version 2.19+ encryption.
	// Encryption happens before compression and depends on the compression used so
	// first decrypt and then decompress.
	if (m_encryptionCodecIn.getGen() == ENCRYPT_GEN_4)
	{
		// Decrypt the packet.
		m_encryptionCodecIn.limitFromType(COMPRESS_BZ2);
		m_encryptionCodecIn.decrypt(pPacket);

		// Uncompress packet.
		pPacket.bzuncompressI();
	}
	else if (m_encryptionCodecIn.getGen() >= ENCRYPT_GEN_5)
	{
		// Find the compression type and remove it.
		int pType = pPacket.readChar();
		pPacket.removeI(0, 1);

		// Decrypt the packet.
		m_encryptionCodecIn.limitFromType(pType); // Encryption is partially related to compression.
		m_encryptionCodecIn.decrypt(pPacket);

		// Uncompress packet
		if (pType == COMPRESS_ZLIB)
			pPacket.zuncompressI();
		else if (pType == COMPRESS_BZ2)
			pPacket.bzuncompressI();
		else if (pType != COMPRESS_UNCOMPRESSED)
			serverlog.out("[%s] ** [ERROR] Client gave incorrect packet compression type! [%d]\n", m_server->getName().text(), pType);
	}
}

void Player::sendPacket(CString pPacket, bool appendNL)
{
	// empty buffer?
	if (pPacket.isEmpty())
		return;

	// append '\n'
	if (appendNL)
	{
		if (pPacket[pPacket.length() - 1] != '\n')
			pPacket.writeChar('\n');
	}

	// append buffer
	m_fileQueue.addPacket(pPacket);
}

bool Player::sendFile(const CString& pFile)
{
	// Add the filename to the list of known files so we can resend the file
	// to the client if it gets changed after it was originally sent
	if (isClient())
		m_knownFiles.insert(pFile.toString());

	FileSystem* fileSystem = m_server->getFileSystem();

	// Find file.
	CString path = fileSystem->find(pFile);
	if (path.isEmpty())
	{
		sendPacket(CString() >> (char)PLO_FILESENDFAILED << pFile);

		return false;
	}

	// Strip filename from the path.
	path.removeI(path.findl(FileSystem::getPathSeparator()) + 1);
	if (path.find(m_server->getServerPath()) != -1)
		path.removeI(0, m_server->getServerPath().length());

	// Send the file now.
	return this->sendFile(path, pFile);
}

bool Player::sendFile(const CString& pPath, const CString& pFile)
{
	CString filepath = m_server->getServerPath() << pPath << pFile;
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
	if (fileData.length() > 3145728) // 3MB
		serverlog.out("[%s] [WARNING] Sending a large file (over 3MB): %s\n", m_server->getName().text(), pFile.text());

	// See if we have enough room in the packet for the file.
	// If not, we need to send it as a big file.
	// 1 (PLO_FILE) + 5 (modTime) + 1 (file.length()) + file.length() + 1 (\n)
	bool isBigFile = false;
	int packetLength = 1 + 5 + 1 + pFile.length() + 1;
	if (fileData.length() > 32000)
		isBigFile = true;

	// Clients before 2.14 didn't support large files.
	if (isClient() && m_versionId < CLVER_2_14)
	{
		if (m_versionId < CLVER_2_1) packetLength -= 5; // modTime isn't sent.
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
		sendPacket(CString() >> (char)PLO_LARGEFILESIZE >> (long long)fileData.length());
	}

	// Send the file now.
	while (fileData.length() != 0)
	{
		int sendSize = clip(32000, 0, fileData.length());
		if (isClient() && m_versionId < CLVER_2_14) sendSize = fileData.length();

		// Older client versions didn't send the modTime.
		if (isClient() && m_versionId < CLVER_2_1)
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

bool Player::testSign()
{
	CSettings& settings = m_server->getSettings();
	if (!settings.getBool("serverside", false)) return true; // TODO: NPC server check instead

	// Check for sign collisions.
	if ((m_sprite % 4) == 0)
	{
		auto level = getLevel();
		if (level)
		{
			auto signs = level->getSigns();
			for (auto sign: signs)
			{
				float signLoc[] = { (float)sign->getX(), (float)sign->getY() };
				if (m_y == signLoc[1] && inrange(m_x, signLoc[0] - 1.5f, signLoc[0] + 0.5f))
				{
					sendPacket(CString() >> (char)PLO_SAY2 << sign->getUText().replaceAll("\n", "#b"));
				}
			}
		}
	}
	return true;
}

void Player::testTouch()
{
#ifdef V8NPCSERVER
	static const int touchtestd[] = { 24, 16, 0, 32, 24, 56, 48, 32 };
	int dir = m_sprite % 4;

	int pixelX = int(m_x * 16.0);
	int pixelY = int(m_y * 16.0);

	auto level = getLevel();
	auto npcList = level->testTouch(pixelX + touchtestd[dir * 2], pixelY + touchtestd[dir * 2 + 1]);
	for (const auto& npc: npcList)
	{
		npc->queueNpcAction("npc.playertouchsme", this);
	}
#endif
}

void Player::dropItemsOnDeath()
{
	if (!m_server->getSettings().getBool("dropitemsdead", true))
		return;

	int mindeathgralats = m_server->getSettings().getInt("mindeathgralats", 1);
	int maxdeathgralats = m_server->getSettings().getInt("maxdeathgralats", 50);

	// Determine how many gralats to remove from the account.
	int drop_gralats = 0;
	if (maxdeathgralats > 0)
	{
		drop_gralats = rand() % maxdeathgralats;
		clip(drop_gralats, mindeathgralats, maxdeathgralats);
		if (drop_gralats > m_gralatCount) drop_gralats = m_gralatCount;
	}

	// Determine how many arrows and bombs to remove from the account.
	int drop_arrows = rand() % 4;
	int drop_bombs = rand() % 4;
	if ((drop_arrows * 5) > m_arrowCount) drop_arrows = m_arrowCount / 5;
	if ((drop_bombs * 5) > m_bombCount) drop_bombs = m_bombCount / 5;

	// Remove gralats/bombs/arrows.
	m_gralatCount -= drop_gralats;
	m_arrowCount -= (drop_arrows * 5);
	m_bombCount -= (drop_bombs * 5);
	sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PLPROP_RUPEESCOUNT >> (int)m_gralatCount >> (char)PLPROP_ARROWSCOUNT >> (char)m_arrowCount >> (char)PLPROP_BOMBSCOUNT >> (char)m_bombCount);

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

		float pX = m_x + 1.5f + (rand() % 8) - 2.0f;
		float pY = m_y + 2.0f + (rand() % 8) - 2.0f;

		CString packet = CString() >> (char)PLI_ITEMADD >> (char)(pX * 2) >> (char)(pY * 2) >> (char)item;
		packet.readGChar(); // So msgPLI_ITEMADD works.

		msgPLI_ITEMADD(packet);
		sendPacket(CString() >> (char)PLO_ITEMADD << packet.subString(1));
	}

	// Add arrows and bombs to the level.
	for (int i = 0; i < drop_arrows; ++i)
	{
		float pX = m_x + 1.5f + (rand() % 8) - 2.0f;
		float pY = m_y + 2.0f + (rand() % 8) - 2.0f;

		CString packet = CString() >> (char)PLI_ITEMADD >> (char)(pX * 2) >> (char)(pY * 2) >> (char)4; // 4 = arrows
		packet.readGChar();                                                                             // So msgPLI_ITEMADD works.

		msgPLI_ITEMADD(packet);
		sendPacket(CString() >> (char)PLO_ITEMADD << packet.subString(1));
	}
	for (int i = 0; i < drop_bombs; ++i)
	{
		float pX = m_x + 1.5f + (rand() % 8) - 2.0f;
		float pY = m_y + 2.0f + (rand() % 8) - 2.0f;

		CString packet = CString() >> (char)PLI_ITEMADD >> (char)(pX * 2) >> (char)(pY * 2) >> (char)3; // 3 = bombs
		packet.readGChar();                                                                             // So msgPLI_ITEMADD works.

		msgPLI_ITEMADD(packet);
		sendPacket(CString() >> (char)PLO_ITEMADD << packet.subString(1));
	}
}

bool Player::processChat(CString pChat)
{
	std::vector<CString> chatParse = pChat.tokenizeConsole();
	if (chatParse.size() == 0) return false;
	bool processed = false;
	bool setcolorsallowed = m_server->getSettings().getBool("setcolorsallowed", true);

	if (chatParse[0] == "setnick")
	{
		processed = true;
		if ((int)difftime(time(0), m_lastNick) >= 10)
		{
			m_lastNick = time(0);
			CString newName = pChat.subString(8).trim();

			// Word filter.
			int filter = m_server->getWordFilter().apply(this, newName, FILTER_CHECK_NICK);
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
		if (!m_server->getSettings().getBool("setheadallowed", true)) return false;
		processed = true;

		// Get the appropriate filesystem.
		FileSystem* filesystem = m_server->getFileSystem();
		if (!m_server->getSettings().getBool("nofoldersconfig", false))
			filesystem = m_server->getFileSystem(FS_HEAD);

		// Try to find the file.
		CString file = filesystem->findi(chatParse[1]);
		if (file.length() == 0)
		{
			int i = 0;
			const char* ext[] = { ".png", ".mng", ".gif" };
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
			m_server->getServerList().sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)m_id >> (char)0 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setbody" && chatParse.size() == 2)
	{
		if (m_server->getSettings().getBool("setbodyallowed", true) == false) return false;
		processed = true;

		// Check to see if it is a default body.
		bool isDefault = false;
		for (unsigned int i = 0; i < sizeof(__defaultbodies) / sizeof(char*); ++i)
			if (chatParse[1].match(CString(__defaultbodies[i])) == true) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			setProps(CString() >> (char)PLPROP_BODYIMG >> (char)chatParse[1].length() << chatParse[1], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
			return false;
		}

		// Get the appropriate filesystem.
		FileSystem* filesystem = m_server->getFileSystem();
		if (!m_server->getSettings().getBool("nofoldersconfig", false))
			filesystem = m_server->getFileSystem(FS_BODY);

		// Try to find the file.
		CString file = filesystem->findi(chatParse[1]);
		if (file.length() == 0)
		{
			int i = 0;
			const char* ext[] = { ".png", ".mng", ".gif" };
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
			m_server->getServerList().sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)m_id >> (char)1 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setsword" && chatParse.size() == 2)
	{
		if (!m_server->getSettings().getBool("setswordallowed", true)) return false;
		processed = true;

		// Check to see if it is a default sword.
		bool isDefault = false;
		for (unsigned int i = 0; i < sizeof(__defaultswords) / sizeof(char*); ++i)
			if (chatParse[1].match(CString(__defaultswords[i])) == true) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			setProps(CString() >> (char)PLPROP_SWORDPOWER >> (char)(m_swordPower + 30) >> (char)chatParse[1].length() << chatParse[1], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
			return false;
		}

		// Get the appropriate filesystem.
		FileSystem* filesystem = m_server->getFileSystem();
		if (!m_server->getSettings().getBool("nofoldersconfig", false))
			filesystem = m_server->getFileSystem(FS_SWORD);

		// Try to find the file.
		CString file = filesystem->findi(chatParse[1]);
		if (file.length() == 0)
		{
			int i = 0;
			const char* ext[] = { ".png", ".mng", ".gif" };
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
			setProps(CString() >> (char)PLPROP_SWORDPOWER >> (char)(m_swordPower + 30) >> (char)chatParse[1].length() << chatParse[1], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		else
			m_server->getServerList().sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)m_id >> (char)2 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setshield" && chatParse.size() == 2)
	{
		if (!m_server->getSettings().getBool("setshieldallowed", true)) return false;
		processed = true;

		// Check to see if it is a default shield.
		bool isDefault = false;
		for (unsigned int i = 0; i < sizeof(__defaultshields) / sizeof(char*); ++i)
			if (chatParse[1].match(CString(__defaultshields[i])) == true) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			setProps(CString() >> (char)PLPROP_SHIELDPOWER >> (char)(m_shieldPower + 10) >> (char)chatParse[1].length() << chatParse[1], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
			return false;
		}

		// Get the appropriate filesystem.
		FileSystem* filesystem = m_server->getFileSystem();
		if (!m_server->getSettings().getBool("nofoldersconfig", false))
			filesystem = m_server->getFileSystem(FS_SHIELD);

		// Try to find the file.
		CString file = filesystem->findi(chatParse[1]);
		if (file.length() == 0)
		{
			int i = 0;
			const char* ext[] = { ".png", ".mng", ".gif" };
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
			setProps(CString() >> (char)PLPROP_SHIELDPOWER >> (char)(m_shieldPower + 10) >> (char)chatParse[1].length() << chatParse[1], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		else
			m_server->getServerList().sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)m_id >> (char)3 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setskin" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 0
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		signed char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			m_colors[0] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)m_colors[0] >> (char)m_colors[1] >> (char)m_colors[2] >> (char)m_colors[3] >> (char)m_colors[4], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
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
			m_colors[1] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)m_colors[0] >> (char)m_colors[1] >> (char)m_colors[2] >> (char)m_colors[3] >> (char)m_colors[4], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
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
			m_colors[2] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)m_colors[0] >> (char)m_colors[1] >> (char)m_colors[2] >> (char)m_colors[3] >> (char)m_colors[4], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
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
			m_colors[3] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)m_colors[0] >> (char)m_colors[1] >> (char)m_colors[2] >> (char)m_colors[3] >> (char)m_colors[4], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
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
			m_colors[4] = color;
			setProps(CString() >> (char)PLPROP_COLORS >> (char)m_colors[0] >> (char)m_colors[1] >> (char)m_colors[2] >> (char)m_colors[3] >> (char)m_colors[4], PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		}
	}
	else if (chatParse[0] == "warpto")
	{
		processed = true;

		// To player
		if (chatParse.size() == 2)
		{
			// Permission check.
			if (!hasRight(PLPERM_WARPTOPLAYER) && !m_server->getSettings().getBool("warptoforall", false))
			{
				setChat("(not authorized to warp)");
				return true;
			}

			auto player = m_server->getPlayer(chatParse[1], PLTYPE_ANYCLIENT);
			if (player && player->getLevel())
				warp(player->getLevel()->getLevelName(), player->getX(), player->getY());
		}
		// To x/y location
		else if (chatParse.size() == 3)
		{
			// Permission check.
			if (!hasRight(PLPERM_WARPTO) && !m_server->getSettings().getBool("warptoforall", false))
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
			if (!hasRight(PLPERM_WARPTO) && !m_server->getSettings().getBool("warptoforall", false))
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

		auto p = m_server->getPlayer(chatParse[1], PLTYPE_ANYCLIENT);
		if (p) p->warp(m_levelName, m_x, m_y);
	}
	else if (chatParse[0] == "unstick" || chatParse[0] == "unstuck")
	{
		if (chatParse.size() == 2 && chatParse[1] == "me")
		{
			processed = true;

			// Check if the player is in a jailed level.
			std::vector<CString> jailList = m_server->getSettings().getStr("jaillevels").tokenize(",");
			for (std::vector<CString>::iterator i = jailList.begin(); i != jailList.end(); ++i)
				if (i->trim() == m_levelName) return false;

			int unstickTime = m_server->getSettings().getInt("unstickmetime", 30);
			if ((int)difftime(time(0), m_lastMovement) >= unstickTime)
			{
				m_lastMovement = time(0);
				CString unstickLevel = m_server->getSettings().getStr("unstickmelevel", "onlinestartlocal.nw");
				float unstickX = m_server->getSettings().getFloat("unstickmex", 30.0f);
				float unstickY = m_server->getSettings().getFloat("unstickmey", 30.5f);
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
			auto& playerList = m_server->getPlayerList();
			for (auto& [pid, player]: playerList)
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
		CString g = m_guild;

		// If a guild was specified, overwrite our guild with it.
		if (chatParse.size() == 2)
			g = chatParse[1];

		if (g.length() != 0)
		{
			CString msg;
			{
				auto& playerList = m_server->getPlayerList();
				for (auto& [pid, player]: playerList)
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
		setChat(CString() << "kills: " << CString((int)m_kills));
	}
	else if (pChat == "showdeaths")
	{
		processed = true;
		setChat(CString() << "deaths: " << CString((int)m_deaths));
	}
	else if (pChat == "showonlinetime")
	{
		processed = true;
		int seconds = m_onlineTime % 60;
		int minutes = (m_onlineTime / 60) % 60;
		int hours = m_onlineTime / 3600;
		CString msg;
		if (hours != 0) msg << CString(hours) << "h ";
		if (minutes != 0 || hours != 0) msg << CString(minutes) << "m ";
		msg << CString(seconds) << "s";
		setChat(CString() << "onlinetime: " << msg);
	}
	else if (chatParse[0] == "toguild:")
	{
		processed = true;
		if (m_guild.length() == 0) return false;

		// Get the PM.
		CString pm = pChat.text() + 8;
		pm.trimI();
		if (pm.length() == 0) return false;

		// Send PM to guild members.
		int num = 0;
		{
			auto& playerList = m_server->getPlayerList();
			for (auto& [pid, player]: playerList)
			{
				// If our guild matches, send the PM.
				if (player->getGuild() == m_guild)
				{
					player->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)m_id << "\"\",\"Guild message:\",\"" << pm << "\"");
					++num;
				}
			}
		}

		// Tell the player how many guild members received his message.
		setChat(CString() << "(" << CString(num) << " guild member" << (num != 0 ? "s" : "") << " received your message)");
	}

	return processed;
}

bool Player::isAdminIp()
{
	std::vector<CString> adminIps = m_adminIp.tokenize(",");
	for (std::vector<CString>::iterator i = adminIps.begin(); i != adminIps.end(); ++i)
	{
		if (m_accountIpStr.match(*i))
			return true;
	}

	return false;
}

bool Player::isStaff()
{
	return m_server->isStaff(m_accountName);
}

/*
	Player: Set Properties
*/
bool Player::warp(const CString& pLevelName, float pX, float pY, time_t modTime)
{
	CSettings& settings = m_server->getSettings();

	// Save our current level.
	auto currentLevel = m_currentLevel.lock();

	// Find the level.
	auto newLevel = Level::findLevel(pLevelName, m_server);

	// If we are warping to the same level, just update the player's location.
	if (currentLevel != nullptr && newLevel == currentLevel)
	{
		setProps(CString() >> (char)PLPROP_X >> (char)(pX * 2) >> (char)PLPROP_Y >> (char)(pY * 2), PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		return true;
	}

	// Find the unstickme level.
	auto unstickLevel = Level::findLevel(settings.getStr("unstickmelevel", "onlinestartlocal.nw"), m_server);
	float unstickX = settings.getFloat("unstickmex", 30.0f);
	float unstickY = settings.getFloat("unstickmey", 35.0f);

	// Leave our current level.
	leaveLevel();

	// See if the new level is on a gmap.
	m_pmap.reset();
	if (newLevel)
		m_pmap = newLevel->getMap();

	// Set x/y location.
	float oldX = m_x, oldY = m_y;
	m_x = pX;
	m_y = pY;

	// Try warping to the new level.
	bool warpSuccess = setLevel(pLevelName, modTime);
	if (!warpSuccess)
	{
		// Failed, so try warping back to our old level.
		bool warped = true;
		if (currentLevel == nullptr) warped = false;
		else
		{
			m_x = oldX;
			m_y = oldY;
			m_pmap = currentLevel->getMap();
			warped = setLevel(currentLevel->getLevelName());
		}
		if (!warped)
		{
			// Failed, so try warping to the unstick level.  If that fails, we disconnect.
			if (unstickLevel == 0) return false;

			// Try to warp to the unstick me level.
			m_x = unstickX;
			m_y = unstickY;
			m_pmap = unstickLevel->getMap();
			if (!setLevel(unstickLevel->getLevelName()))
				return false;
		}
	}

	return warpSuccess;
}

std::shared_ptr<Level> Player::getLevel() const
{
	if (isHiddenClient()) return {};

	auto pLevel = m_currentLevel.lock();
	if (pLevel) return pLevel;

	if (isClient() && m_server->warpPlayerToSafePlace(m_id))
	{
		return m_currentLevel.lock();
	}

	return {};
}

bool Player::setLevel(const CString& pLevelName, time_t modTime)
{
	// Open Level
	auto newLevel = Level::findLevel(pLevelName, m_server);
	if (newLevel == nullptr)
	{
		sendPacket(CString() >> (char)PLO_WARPFAILED << pLevelName);
		return false;
	}
	m_currentLevel = newLevel;

	// Check if the level is a singleplayer level.
	// If so, see if we have been there before.  If not, duplicate it.
	if (newLevel->isSingleplayer())
	{
		auto nl = (m_singleplayerLevels.find(newLevel->getLevelName()) != m_singleplayerLevels.end() ? m_singleplayerLevels[newLevel->getLevelName()] : nullptr);
		if (nl == nullptr)
		{
			newLevel = newLevel->clone();
			m_currentLevel = newLevel;
			m_singleplayerLevels[newLevel->getLevelName()] = newLevel;
		}
		else
			m_currentLevel = nl;
	}

	// Check if the map is a group map.
	if (auto map = m_pmap.lock(); map && map->isGroupMap())
	{
		if (!m_levelGroup.isEmpty())
		{
			// If any players are in this level, they might have been cached on the client.  Solve this by manually removing them.
			auto& plist = newLevel->getPlayers();
			for (auto id: plist)
			{
				auto p = m_server->getPlayer(id);
				sendPacket(p->getProps(0, 0) >> (char)PLPROP_CURLEVEL >> (char)(newLevel->getLevelName().length() + 1 + 7) << newLevel->getLevelName() << ".unknown" >> (char)PLPROP_X << p->getProp(PLPROP_X) >> (char)PLPROP_Y << p->getProp(PLPROP_Y));
			}

			// Set the correct level now.
			const auto& levelName = newLevel->getLevelName();
			auto& groupLevels = m_server->getGroupLevels();
			auto [start, end] = groupLevels.equal_range(levelName.toString());
			while (start != end)
			{
				if (auto nl = start->second.lock(); nl)
				{
					if (nl->getLevelName() == levelName)
					{
						m_currentLevel = nl;
						break;
					}
				}
				++start;
			}
			if (start == end)
			{
				newLevel = newLevel->clone();
				m_currentLevel = newLevel;
				newLevel->setLevelName(levelName);
				groupLevels.insert(std::make_pair(levelName.toString(), newLevel));
			}
		}
	}

	// Add myself to the level playerlist.
	newLevel->addPlayer(m_id);
	m_levelName = newLevel->getLevelName();

	// Tell the client their new level.
	if (modTime == 0 || m_versionId < CLVER_2_1)
	{
		if (auto map = m_pmap.lock(); map && map->getType() == MapType::GMAP && m_versionId >= CLVER_2_1)
		{
			sendPacket(CString() >> (char)PLO_PLAYERWARP2 >> (char)(m_x * 2) >> (char)(m_y * 2) >> (char)(m_z + 50) >> (char)newLevel->getMapX() >> (char)newLevel->getMapY()
																																						<< map->getMapName());
		}
		else
			sendPacket(CString() >> (char)PLO_PLAYERWARP >> (char)(m_x * 2) >> (char)(m_y * 2) << m_levelName);
	}

	// Send the level now.
	bool succeed = true;
	if (m_versionId >= CLVER_2_1)
		succeed = sendLevel(newLevel, modTime, false);
	else
		succeed = sendLevel141(newLevel, modTime, false);

	if (!succeed)
	{
		sendPacket(CString() >> (char)PLO_WARPFAILED << pLevelName);
		return false;
	}

	// If the level is a sparring zone and you have 100 AP, change AP to 99 and
	// the apcounter to 1.
	if (newLevel->isSparringZone() && m_ap == 100)
	{
		m_ap = 99;
		m_apCounter = 1;
		setProps(CString() >> (char)PLPROP_ALIGNMENT >> (char)m_ap, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
	}

	// Inform everybody as to the client's new location.  This will update the minimap.
	CString minimap = this->getProps(0, 0) >> (char)PLPROP_CURLEVEL << this->getProp(PLPROP_CURLEVEL) >> (char)PLPROP_X << this->getProp(PLPROP_X) >> (char)PLPROP_Y << this->getProp(PLPROP_Y);
	for (auto& [pid, player]: m_server->getPlayerList())
	{
		if (pid == this->getId())
			continue;
		if (auto map = m_pmap.lock(); map && map->isGroupMap() && m_levelGroup != player->getGroup())
			continue;

		player->sendPacket(minimap);
	}
	//m_server->sendPacketToAll(this->getProps(0, 0) >> (char)PLPROP_CURLEVEL << this->getProp(PLPROP_CURLEVEL) >> (char)PLPROP_X << this->getProp(PLPROP_X) >> (char)PLPROP_Y << this->getProp(PLPROP_Y), this);

	return true;
}

bool Player::sendLevel(std::shared_ptr<Level> pLevel, time_t modTime, bool fromAdjacent)
{
	if (pLevel == nullptr) return false;
	CSettings& settings = m_server->getSettings();

	// Send Level
	sendPacket(CString() >> (char)PLO_LEVELNAME << pLevel->getLevelName());
	time_t l_time = getCachedLevelModTime(pLevel.get());
	if (modTime == -1) modTime = pLevel->getModTime();
	if (l_time == 0)
	{
		if (modTime != pLevel->getModTime())
		{
			sendPacket(CString() >> (char)PLO_RAWDATA >> (int)((1 + (64 * 64 * 2) + 1)));
			sendPacket(CString() << pLevel->getBoardPacket());

			for (const auto& layers: pLevel->getLayers())
			{
				if (layers.first == 0) continue;
				CString layer = pLevel->getLayerPacket(layers.first);
				sendPacket(CString() >> (char)PLO_RAWDATA >> (int)layer.length());
				sendPacket(layer);
			}
		}

		// Send links, signs, and mod time.
		sendPacket(CString() >> (char)PLO_LEVELMODTIME >> (long long)pLevel->getModTime());
		sendPacket(CString() << pLevel->getLinksPacket());
		sendPacket(CString() << pLevel->getSignsPacket(this));
	}

	// Send board changes, chests, horses, and baddies.
	if (!fromAdjacent)
	{
		sendPacket(CString() << pLevel->getBoardChangesPacket(l_time));
		sendPacket(CString() << pLevel->getChestPacket(this));
		sendPacket(CString() << pLevel->getHorsePacket());
		sendPacket(CString() << pLevel->getBaddyPacket(m_versionId));
	}

	// If we are on a gmap, change our level back to the gmap.
	if (auto map = m_pmap.lock(); map && map->getType() == MapType::GMAP)
		sendPacket(CString() >> (char)PLO_LEVELNAME << map->getMapName());

	// Tell the client if there are any ghost players in the level.
	// We don't support trial accounts so pass 0 (no ghosts) instead of 1 (ghosts present).
	sendPacket(CString() >> (char)PLO_GHOSTICON >> (char)0);

	if (!fromAdjacent || !m_pmap.expired())
	{
		// If we are the leader, send it now.
		if (pLevel->isPlayerLeader(getId()) || pLevel->isSingleplayer() == true)
			sendPacket(CString() >> (char)PLO_ISLEADER);
	}

	// Send new world time.
	sendPacket(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(m_server->getNWTime()));
	if (!fromAdjacent || !m_pmap.expired())
	{
		// Send NPCs.
		if (auto map = m_pmap.lock(); map && map->getType() == MapType::GMAP)
		{
			sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << map->getMapName());

			auto val = pLevel->getNpcsPacket(l_time, m_versionId);
			sendPacket(val);

			/*sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << m_pmap->getMapName());
			CString pmapLevels = m_pmap->getLevels();
			Level* tmpLvl;
			while (pmapLevels.bytesLeft() > 0)
			{
				CString tmpLvlName = pmapLevels.readString("\n");
				tmpLvl = Level::findLevel(tmpLvlName.guntokenizeI(), server);
				if (tmpLvl != NULL)
					sendPacket(CString() << tmpLvl->getNpcsPacket(l_time, m_versionId));
			}*/
		}
		else
		{
			sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << pLevel->getLevelName());
			sendPacket(CString() << pLevel->getNpcsPacket(l_time, m_versionId));
		}
	}

	// Send connecting player props to players in nearby levels.
	if (auto level = m_currentLevel.lock(); level && !level->isSingleplayer())
	{
		// Send my props.
		m_server->sendPacketToLevelArea(this->getProps(__getLogin, sizeof(__getLogin) / sizeof(bool)), this->shared_from_this(), { m_id });

		// Get other player props.
		if (auto map = m_pmap.lock(); map)
		{
			auto sgmap{ this->getMapPosition() };
			auto isGroupMap = map->isGroupMap();

			for (const auto& [otherid, other]: m_server->getPlayerList())
			{
				if (m_id == otherid) continue;
				if (!other->isClient()) continue;

				auto othermap = other->getMap().lock();
				if (!othermap || othermap != map) continue;
				if (isGroupMap && this->getGroup() != other->getGroup()) continue;

				// Check if they are nearby before sending the packet.
				auto ogmap{ other->getMapPosition() };
				if (abs(ogmap.first - sgmap.first) < 2 && abs(ogmap.second - sgmap.second) < 2)
					this->sendPacket(other->getProps(__getLogin, sizeof(__getLogin) / sizeof(bool)));
			}
		}
		else
		{
			for (auto otherid: level->getPlayers())
			{
				if (m_id == otherid) continue;
				auto other = m_server->getPlayer(otherid);
				this->sendPacket(other->getProps(__getLogin, sizeof(__getLogin) / sizeof(bool)));
			}
		}
	}

	return true;
}

bool Player::sendLevel141(std::shared_ptr<Level> pLevel, time_t modTime, bool fromAdjacent)
{
	if (pLevel == nullptr) return false;
	CSettings& settings = m_server->getSettings();

	time_t l_time = getCachedLevelModTime(pLevel.get());
	if (modTime == -1) modTime = pLevel->getModTime();
	if (l_time != 0)
	{
		sendPacket(CString() << pLevel->getBoardChangesPacket(l_time));
	}
	else
	{
		if (modTime != pLevel->getModTime())
		{
			sendPacket(CString() >> (char)PLO_RAWDATA >> (int)(1 + (64 * 64 * 2) + 1));
			sendPacket(CString() << pLevel->getBoardPacket());

			if (m_firstLevel)
				sendPacket(CString() >> (char)PLO_LEVELNAME << pLevel->getLevelName());
			m_firstLevel = false;

			// Send links, signs, and mod time.
			if (!settings.getBool("serverside", false)) // TODO: NPC server check instead.
			{
				sendPacket(CString() << pLevel->getLinksPacket());
				sendPacket(CString() << pLevel->getSignsPacket(this));
			}
			sendPacket(CString() >> (char)PLO_LEVELMODTIME >> (long long)pLevel->getModTime());
		}
		else
			sendPacket(CString() >> (char)PLO_LEVELBOARD);

		if (!fromAdjacent)
		{
			sendPacket(CString() << pLevel->getBoardChangesPacket2(l_time));
			sendPacket(CString() << pLevel->getChestPacket(this));
		}
	}

	// Send board changes, chests, horses, and baddies.
	if (!fromAdjacent)
	{
		sendPacket(CString() << pLevel->getHorsePacket());
		sendPacket(CString() << pLevel->getBaddyPacket(m_versionId));
	}

	if (fromAdjacent == false)
	{
		// If we are the leader, send it now.
		if (pLevel->isPlayerLeader(getId()) || pLevel->isSingleplayer() == true)
			sendPacket(CString() >> (char)PLO_ISLEADER);
	}

	// Send new world time.
	sendPacket(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(m_server->getNWTime()));

	// Send NPCs.
	if (!fromAdjacent)
		sendPacket(CString() << pLevel->getNpcsPacket(l_time, m_versionId));

	// Send connecting player props to players in nearby levels.
	if (!pLevel->isSingleplayer() && !fromAdjacent)
	{
		m_server->sendPacketToLevelArea(this->getProps(__getLogin, sizeof(__getLogin) / sizeof(bool)), this->shared_from_this(), { m_id });

		for (auto id: pLevel->getPlayers())
		{
			if (id == getId()) continue;

			auto player = m_server->getPlayer(id);
			this->sendPacket(player->getProps(__getLogin, sizeof(__getLogin) / sizeof(bool)));
		}
	}

	return true;
}

bool Player::leaveLevel(bool resetCache)
{
	// Make sure we are on a level first.
	auto levelp = m_currentLevel.lock();
	if (!levelp) return true;

	// Save the time we left the level for the client-side caching.
	bool found = false;
	for (auto& cl: m_cachedLevels)
	{
		auto cllevel = cl->level.lock();
		if (cllevel == levelp)
		{
			cl->modTime = (resetCache ? 0 : time(0));
			found = true;
			break;
		}
	}
	if (!found) m_cachedLevels.push_back(std::make_unique<CachedLevel>(m_currentLevel, time(0)));

	// Remove self from list of players in level.
	levelp->removePlayer(m_id);

	// Send PLO_ISLEADER to new level leader.
	if (auto& levelPlayerList = levelp->getPlayers(); !levelPlayerList.empty())
	{
		auto leader = m_server->getPlayer(levelPlayerList.front());
		leader->sendPacket(CString() >> (char)PLO_ISLEADER);
	}

	// Tell everyone I left.
	// This prop isn't used at all???  Maybe it is required for 1.41?
	//	if (m_pmap && m_pmap->getType() != MAPTYPE_GMAP)
	{
		m_server->sendPacketToLevelArea(this->getProps(0, 0) >> (char)PLPROP_JOINLEAVELVL >> (char)0, this->shared_from_this(), { m_id });

		for (auto& [pid, player]: m_server->getPlayerList())
		{
			if (pid == getId()) continue;
			if (player->getLevel() != getLevel()) continue;
			this->sendPacket(player->getProps(0, 0) >> (char)PLPROP_JOINLEAVELVL >> (char)0);
		}
	}

	// Set the level pointer to 0.
	m_currentLevel.reset();

	return true;
}

time_t Player::getCachedLevelModTime(const Level* level) const
{
	for (auto& cl: m_cachedLevels)
	{
		auto cllevel = cl->level.lock();
		if (cllevel && cllevel.get() == level)
			return cl->modTime;
	}
	return 0;
}

void Player::resetLevelCache(const Level* level)
{
	for (auto& cl: m_cachedLevels)
	{
		auto cllevel = cl->level.lock();
		if (cllevel && cllevel.get() == level)
		{
			cl->modTime = 0;
			return;
		}
	}
}

std::pair<int, int> Player::getMapPosition() const
{
	if (m_currentLevel.expired()) return { 0, 0 };
	if (m_pmap.expired()) return { 0, 0 };

	auto level = getLevel();
	auto map = m_pmap.lock();
	if (!level || !map) return { 0, 0 };

	switch (map->getType())
	{
		case MapType::BIGMAP:
			return { level->getMapX(), level->getMapY() };
		default:
		case MapType::GMAP:
			return { getProp(PLPROP_GMAPLEVELX).readGUChar(), getProp(PLPROP_GMAPLEVELY).readGUChar() };
	}

	return { 0, 0 };
}

void Player::setChat(const CString& pChat)
{
	setProps(CString() >> (char)PLPROP_CURCHAT >> (char)pChat.length() << pChat, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

void Player::setNick(CString pNickName, bool force)
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
		m_nickName = pNickName;
		this->m_guild = guild;
		return;
	}

	// If a player has put a * before his nick, remove it.
	while (!nick.isEmpty() && nick[0] == '*')
		nick.removeI(0, 1);

	// If the nickname is now empty, set it to unknown.
	if (nick.isEmpty()) nick = "unknown";

	// If the nickname is equal to the account name, add the *.
	if (nick == m_accountName)
		newNick = CString("*");

	// Add the nick name.
	newNick << nick;

	// If a guild was specified, add the guild.
	if (guild.length() != 0)
	{
		// Read the guild list.
		FileSystem guildFS(m_server);
		guildFS.addDir("guilds");
		CString guildList = guildFS.load(CString() << "guild" << guild << ".txt");
		if (guildList.isEmpty())
			guildList = guildFS.load(CString() << "guild" << guild.replaceAll(" ", "_") << ".txt");

		// Find the account in the guild list.
		// Will also return -1 if the guild does not exist.
		if (guildList.findi(m_accountName) != -1)
		{
			guildList.setRead(guildList.findi(m_accountName));
			CString line = guildList.readString("\n");
			line.removeAllI("\r");
			if (line.find(":") != -1)
			{
				std::vector<CString> line2 = line.tokenize(":");
				if ((line2[1])[0] == '*') line2[1].removeI(0, 1);
				if ((line2[1]) == nick) // Use nick instead of newNick because nick doesn't include the *
				{
					m_nickName = newNick;
					m_nickName << " (" << guild << ")";
					this->m_guild = guild;
					return;
				}
			}
			else
			{
				m_nickName = newNick;
				m_nickName << " (" << guild << ")";
				this->m_guild = guild;
				return;
			}
		}
		else
			m_nickName = newNick;

		// See if we can ask if it is a global guild.
		bool askGlobal = m_server->getSettings().getBool("globalguilds", true);
		if (!askGlobal)
		{
			// Check for whitelisted global guilds.
			std::vector<CString> allowed = m_server->getSettings().getStr("allowedglobalguilds").tokenize(",");
			if (std::find(allowed.begin(), allowed.end(), guild) != allowed.end())
				askGlobal = true;
		}

		// See if it is a global guild.
		if (askGlobal)
		{
			m_server->getServerList().sendPacket(
				CString() >> (char)SVO_VERIGUILD >> (short)m_id >> (char)m_accountName.length() << m_accountName >> (char)newNick.length() << newNick >> (char)guild.length() << guild);
		}
	}
	else
	{
		// Save it.
		m_nickName = newNick;
		this->m_guild.clear();
	}

	if (m_isExternal)
	{
		m_nickName = pNickName;
	}
}

bool Player::addWeapon(LevelItemType defaultWeapon)
{
	// Allow Default Weapons..?
	CSettings& settings = m_server->getSettings();
	if (!settings.getBool("defaultweapons", true))
		return false;

	auto weapon = m_server->getWeapon(LevelItem::getItemName(defaultWeapon));
	if (!weapon)
	{
		weapon = std::make_shared<Weapon>(m_server, defaultWeapon);
		m_server->NC_AddWeapon(weapon);
	}

	return this->addWeapon(weapon);
}

bool Player::addWeapon(const std::string& name)
{
	auto weapon = m_server->getWeapon(name);
	return this->addWeapon(weapon);
}

bool Player::addWeapon(std::shared_ptr<Weapon> weapon)
{
	if (weapon == nullptr) return false;

	// See if the player already has the weapon.
	if (vecSearch<CString>(m_weaponList, weapon->getName()) == -1)
	{
		m_weaponList.push_back(weapon->getName());
		if (m_id == -1) return true;

		// Send weapon.
		sendPacket(weapon->getWeaponPacket(m_versionId));
	}

	return true;
}

bool Player::deleteWeapon(LevelItemType defaultWeapon)
{
	auto weapon = m_server->getWeapon(LevelItem::getItemName(defaultWeapon));
	return this->deleteWeapon(weapon);
}

bool Player::deleteWeapon(const std::string& name)
{
	auto weapon = m_server->getWeapon(name);
	return this->deleteWeapon(weapon);
}

bool Player::deleteWeapon(std::shared_ptr<Weapon> weapon)
{
	if (weapon == nullptr) return false;

	// Remove the weapon.
	if (vecRemove<CString>(m_weaponList, weapon->getName()))
	{
		if (m_id == -1) return true;

		// Send delete notice.
		sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << weapon->getName());
	}

	return true;
}

void Player::disableWeapons()
{
	this->m_status &= ~PLSTATUS_ALLOWWEAPONS;
	sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PLPROP_STATUS << getProp(PLPROP_STATUS));
}

void Player::enableWeapons()
{
	this->m_status |= PLSTATUS_ALLOWWEAPONS;
	sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PLPROP_STATUS << getProp(PLPROP_STATUS));
}

void Player::freezePlayer()
{
	sendPacket(CString() >> (char)PLO_FREEZEPLAYER2);
}

void Player::unfreezePlayer()
{
	sendPacket(CString() >> (char)PLO_UNFREEZEPLAYER);
}

void Player::sendRPGMessage(const CString& message)
{
	sendPacket(CString() >> (char)PLO_RPGWINDOW << message.gtokenize());
}

void Player::sendSignMessage(const CString& message)
{
	sendPacket(CString() >> (char)PLO_SAY2 << message.replaceAll("\n", "#b"));
}

void Player::setAni(CString gani)
{
	if (gani.length() > 223)
		gani.remove(223);

	CString propPackage;
	propPackage >> (char)PLPROP_GANI >> (char)gani.length() << gani;
	setProps(propPackage, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

/*
	Player: Flag Functions
*/

void Player::deleteFlag(const std::string& pFlagName, bool sendToPlayer)
{
	Account::deleteFlag(pFlagName);

	if (sendToPlayer)
	{
		sendPacket(CString() >> (char)PLO_FLAGDEL << pFlagName);
	}
}

void Player::setFlag(const std::string& pFlagName, const CString& pFlagValue, bool sendToPlayer)
{
	// Call Default Set Flag
	Account::setFlag(pFlagName, pFlagValue);

	// Send to Player
	if (sendToPlayer)
	{
		if (pFlagValue.isEmpty())
			sendPacket(CString() >> (char)PLO_FLAGSET << pFlagName);
		else
			sendPacket(CString() >> (char)PLO_FLAGSET << pFlagName << "=" << pFlagValue);
	}
}

/*
	Player: Packet functions
*/
bool Player::msgPLI_NULL(CString& pPacket)
{
	pPacket.setRead(0);
	printf("Unknown Player Packet: %u (%s)\n", (unsigned int)pPacket.readGUChar(), pPacket.text() + 1);
	for (int i = 0; i < pPacket.length(); ++i) printf("%02x ", (unsigned char)((pPacket.text())[i]));
	printf("\n");

	// If we are getting a whole bunch of invalid packets, something went wrong.  Disconnect the player.
	m_invalidPackets++;
	if (m_invalidPackets > 5)
	{
		serverlog.out("[%s] Player %s is sending invalid packets.\n", m_server->getName().text(), m_nickName.text());
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Disconnected for sending invalid packets.");
		return false;
	}

	return true;
}

bool Player::msgPLI_LOGIN(CString& pPacket)
{
	// Read Player-Ip
	m_accountIpStr = m_playerSock->getRemoteIp();
#ifdef HAVE_INET_PTON
	inet_pton(AF_INET, m_accountIpStr.text(), &m_accountIp);
#else
	m_accountIp = inet_addr(m_accountIpStr.text());
#endif

	// TODO(joey): Hijack type based on what graal sends, rather than use it directly.

	// Read Client-Type
	serverlog.out("[%s] :: New login:\t", m_server->getName().text());
	m_type = (1 << pPacket.readGChar());
	bool getKey = false;
	switch (m_type)
	{
		case PLTYPE_CLIENT:
			serverlog.append("Client\n");
			m_encryptionCodecIn.setGen(ENCRYPT_GEN_2);
			break;
		case PLTYPE_RC:
			serverlog.append("RC\n");
			m_encryptionCodecIn.setGen(ENCRYPT_GEN_3);
			break;
		case PLTYPE_NPCSERVER:
			serverlog.append("NPCSERVER\n");
			m_encryptionCodecIn.setGen(ENCRYPT_GEN_3);
			break;
		case PLTYPE_NC:
			serverlog.append("NC\n");
			m_encryptionCodecIn.setGen(ENCRYPT_GEN_3);
			getKey = false;
			break;
		case PLTYPE_CLIENT2:
			serverlog.append("New Client (2.19 - 2.21, 3 - 3.01)\n");
			m_encryptionCodecIn.setGen(ENCRYPT_GEN_4);
			break;
		case PLTYPE_CLIENT3:
			serverlog.append("New Client (2.22+)\n");
			m_encryptionCodecIn.setGen(ENCRYPT_GEN_5);
			break;
		case PLTYPE_RC2:
			serverlog.append("New RC (2.22+)\n");
			m_encryptionCodecIn.setGen(ENCRYPT_GEN_5);
			getKey = true;
			break;
		case PLTYPE_WEB:
			serverlog.append("Web\n");
			m_encryptionCodecIn.setGen(ENCRYPT_GEN_1);
			m_fileQueue.setCodec(ENCRYPT_GEN_1, m_encryptionKey);
			getKey = false;
			break;
		default:
			serverlog.append("Unknown (%d)\n", m_type);
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Your client type is unknown.  Please inform the " << APP_VENDOR << " Team.  Type: " << CString((int)m_type) << ".");
			return false;
			break;
	}

	if (m_type == PLTYPE_CLIENT)
	{
		// Read Client-Version for v1.3 clients
		m_version = pPacket.readChars(8);
		m_versionId = getVersionID(m_version);

		if (m_versionId == CLVER_UNKNOWN)
		{
			m_encryptionCodecIn.setGen(ENCRYPT_GEN_3);
			pPacket.setRead(1);
		}
	}

	if (m_versionId == CLVER_UNKNOWN)
	{
		// Get Iterator-Key
		// 2.19+ RC and any client should get the key.
		if ((isClient() && m_type != PLTYPE_WEB) || (isRC() && m_encryptionCodecIn.getGen() > ENCRYPT_GEN_3) || getKey == true)
		{
			m_encryptionKey = (unsigned char)pPacket.readGChar();

			m_encryptionCodecIn.reset(m_encryptionKey);
			if (m_encryptionCodecIn.getGen() > ENCRYPT_GEN_3)
				m_fileQueue.setCodec(m_encryptionCodecIn.getGen(), m_encryptionKey);
		}

		// Read Client-Version
		m_version = pPacket.readChars(8);
		m_versionId = getVersionIDByVersion(m_version);
	}

	// Read Account & Password
	m_accountName = pPacket.readChars(pPacket.readGUChar());
	CString password = pPacket.readChars(pPacket.readGUChar());

	// Client Identity: win,"",02e2465a2bf38f8a115f6208e9938ac8,ff144a9abb9eaff4b606f0336d6d8bc5,"6.2 9200 "
	//					{platform}, {mobile provides 'dc:id2'}, {md5hash:harddisk-id}, {md5hash:network-id}, {uname(release, version)}, {android-id}
	CString identity = pPacket.readString("");

	//serverlog.out("[%s]    Key: %d\n", m_server->getName().text(), key);
	serverlog.out("[%s]    Version:\t%s (%s)\n", m_server->getName().text(), m_version.text(), getVersionString(m_version, m_type));
	serverlog.out("[%s]    Account:\t%s\n", m_server->getName().text(), m_accountName.text());
	if (!identity.isEmpty())
	{
		serverlog.out("[%s]    Identity:\t%s\n", m_server->getName().text(), identity.text());
		auto identityTokens = identity.tokenize(",", true);
		m_os = identityTokens[0];
	}

	// Check for available slots on the server.
	if (m_server->getPlayerList().size() >= (unsigned int)m_server->getSettings().getInt("maxplayers", 128))
	{
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "This server has reached its player limit.");
		return false;
	}

	// Check if they are ip-banned or not.
	if (m_server->isIpBanned(m_playerSock->getRemoteIp()) && !hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You have been banned from this server.");
		return false;
	}

	// Check if the specified client is allowed access.
	if (isClient())
	{
		auto& allowedVersions = m_server->getAllowedVersions();
		bool allowed = false;
		for (auto ver: allowedVersions)
		{
			if (ver.find(":") != -1)
			{
				CString ver1 = ver.readString(":").trim();
				CString ver2 = ver.readString("").trim();
				int aVersion[2] = { getVersionID(ver1), getVersionID(ver2) };
				if (m_versionId >= aVersion[0] && m_versionId <= aVersion[1])
				{
					allowed = true;
					break;
				}
			}
			else
			{
				int aVersion = getVersionID(ver);
				if (m_versionId == aVersion)
				{
					allowed = true;
					break;
				}
			}
		}
		if (!allowed)
		{
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Your client version is not allowed on this server.\rAllowed: " << m_server->getAllowedVersionString());
			return false;
		}
	}

	// Verify login details with the serverlist.
	// TODO: localhost mode.
	if (!m_server->getServerList().getConnected())
	{
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "The login server is offline.  Try again later.");
		return false;
	}

	m_server->getServerList().sendLoginPacketForPlayer(shared_from_this(), password, identity);
	return true;
}

int Player::getVersionIDByVersion(const CString& versionInput) const
{
	if (isClient()) return getVersionID(versionInput);
	else if (isNC())
		return getNCVersionID(versionInput);
	else if (isRC())
		return getRCVersionID(versionInput);
	else
		return CLVER_UNKNOWN;
}

bool Player::msgPLI_LEVELWARP(CString& pPacket)
{
	time_t modTime = 0;

	if (pPacket[0] - 32 == PLI_LEVELWARPMOD)
		modTime = (time_t)pPacket.readGUInt5();

	float loc[2] = { (float)(pPacket.readGChar() / 2.0f), (float)(pPacket.readGChar() / 2.0f) };
	CString newLevel = pPacket.readString("");
	warp(newLevel, loc[0], loc[1], modTime);

	return true;
}

bool Player::msgPLI_BOARDMODIFY(CString& pPacket)
{
	CSettings& settings = m_server->getSettings();
	signed char loc[2] = { pPacket.readGChar(), pPacket.readGChar() };
	signed char dim[2] = { pPacket.readGChar(), pPacket.readGChar() };
	CString tiles = pPacket.readString("");

	// Alter level data.
	auto level = getLevel();
	if (level->alterBoard(tiles, loc[0], loc[1], dim[0], dim[1], this))
		m_server->sendPacketToOneLevel(CString() >> (char)PLO_BOARDMODIFY << (pPacket.text() + 1), level);

	if (loc[0] < 0 || loc[0] > 63 || loc[1] < 0 || loc[1] > 63) return true;

	// Older clients drop items clientside.
	if (m_versionId < CLVER_2_1)
		return true;

	// Lay items when you destroy objects.
	short oldTile = (getLevel()->getTiles())[loc[0] + (loc[1] * 64)];
	bool bushitems = settings.getBool("bushitems", true);
	bool vasesdrop = settings.getBool("vasesdrop", true);
	int tiledroprate = settings.getInt("tiledroprate", 50);
	LevelItemType dropItem = LevelItemType::INVALID;

	// Bushes, grass, swamp.
	if ((oldTile == 2 || oldTile == 0x1a4 || oldTile == 0x1ff ||
		 oldTile == 0x3ff) &&
		bushitems)
	{
		if (tiledroprate > 0)
		{
			if ((rand() % 100) < tiledroprate)
			{
				dropItem = LevelItem::getItemId(rand() % 6);
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
		CString packet = CString() >> (char)(loc[0] * 2) >> (char)(loc[1] * 2) >> (char)LevelItem::getItemTypeId(dropItem);
		CString packet2 = CString() >> (char)PLI_ITEMADD << packet;
		packet2.readGChar(); // So msgPLI_ITEMADD works.

		spawnLevelItem(packet2, false);

		if (getVersion() <= CLVER_5_12)
			sendPacket(CString() >> (char)PLO_ITEMADD << packet);
	}

	return true;
}

bool Player::msgPLI_REQUESTUPDATEBOARD(CString& pPacket)
{
	// {130}{CHAR level length}{level}{INT5 modtime}{SHORT x}{SHORT y}{SHORT width}{SHORT height}
	CString level = pPacket.readChars(pPacket.readGUChar());

	time_t modTime = (time_t)pPacket.readGUInt5();

	short x = pPacket.readGShort();
	short y = pPacket.readGShort();
	short w = pPacket.readGShort();
	short h = pPacket.readGShort();

	// TODO: What to return?
	serverlog.out("[%s] :: Received PLI_REQUESTUPDATEBOARD - level: %s - x: %d - y: %d - w: %d - h: %d - modtime: %d\n", m_server->getName().text(), level.text(), x, y, w, h, modTime);

	return true;
}

bool Player::msgPLI_PLAYERPROPS(CString& pPacket)
{
	setProps(pPacket, PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD);
	return true;
}

bool Player::msgPLI_NPCPROPS(CString& pPacket)
{
	// Dont accept npc-properties from clients when an npc-server is present
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
	auto npc = m_server->getNPC(npcId);
	if (!npc)
		return true;

	if (npc->getLevel() != level)
		return true;

	CString packet = CString() >> (char)PLO_NPCPROPS >> (int)npcId;
	packet << npc->setProps(npcProps, m_versionId);
	m_server->sendPacketToLevelArea(packet, shared_from_this(), { m_id });

	return true;
}

bool Player::msgPLI_BOMBADD(CString& pPacket)
{
	// TODO(joey): gmap support
	unsigned char loc[2] = { pPacket.readGUChar(), pPacket.readGUChar() };
	//float loc[2] = {(float)pPacket.readGChar() / 2.0f, (float)pPacket.readGChar() / 2.0f};
	unsigned char player_power = pPacket.readGUChar();
	unsigned char player = player_power >> 2;
	unsigned char power = player_power & 0x03;
	unsigned char timeToExplode = pPacket.readGUChar(); // How many 0.05 sec increments until it explodes.  Defaults to 55 (2.75 seconds.)

	/*
	printf("Place bomb\n");
	printf("Position: (%d, %d)\n", loc[0], loc[1]);
	//printf("Position: (%0.2f, %0.2f)\n", loc[0], loc[1]);
	printf("Player (?): %d\n", player);
	printf("Bomb Power: %d\n", power);
	printf("Bomb Explode Timer: %d\n", timeToExplode);
	//for (int i = 0; i < pPacket.length(); ++i) printf( "%02x ", (unsigned char)pPacket[i] ); printf( "\n" );
	*/

	m_server->sendPacketToOneLevel(CString() >> (char)PLO_BOMBADD >> (short)m_id << (pPacket.text() + 1), m_currentLevel, { m_id });
	return true;
}

bool Player::msgPLI_BOMBDEL(CString& pPacket)
{
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_BOMBDEL << (pPacket.text() + 1), m_currentLevel, { m_id });
	return true;
}

bool Player::msgPLI_TOALL(CString& pPacket)
{
	// Check if the player is in a jailed level.
	std::vector<CString> jailList = m_server->getSettings().getStr("jaillevels").tokenize(",");
	if (std::find_if(jailList.begin(), jailList.end(), [&levelName = this->m_levelName](CString& level)
					 {
						 return level.trim() == levelName;
					 }) != jailList.end())
		return true;

	CString message = pPacket.readString(pPacket.readGUChar());

	// Word filter.
	int filter = m_server->getWordFilter().apply(this, message, FILTER_CHECK_TOALL);
	if (filter & FILTER_ACTION_WARN)
	{
		setChat(message);
		return true;
	}

	for (auto& [pid, player]: m_server->getPlayerList())
	{
		if (pid == m_id) continue;

		// See if the player is allowing toalls.
		unsigned char flags = strtoint(player->getProp(PLPROP_ADDITFLAGS));
		if (flags & PLFLAG_NOTOALL) continue;

		player->sendPacket(CString() >> (char)PLO_TOALL >> (short)m_id >> (char)message.length() << message);
	}
	return true;
}

bool Player::msgPLI_HORSEADD(CString& pPacket)
{
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_HORSEADD << (pPacket.text() + 1), m_currentLevel, { m_id });

	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };
	unsigned char dir_bush = pPacket.readGUChar();
	char hdir = dir_bush & 0x03;
	char hbushes = dir_bush >> 2;
	CString image = pPacket.readString("");

	auto level = getLevel();
	level->addHorse(image, loc[0], loc[1], hdir, hbushes);
	return true;
}

bool Player::msgPLI_HORSEDEL(CString& pPacket)
{
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_HORSEDEL << (pPacket.text() + 1), m_currentLevel, { m_id });

	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };

	auto level = getLevel();
	level->removeHorse(loc[0], loc[1]);
	return true;
}

bool Player::msgPLI_ARROWADD(CString& pPacket)
{
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_ARROWADD >> (short)m_id << (pPacket.text() + 1), m_currentLevel, { m_id });
	return true;
}

bool Player::msgPLI_FIRESPY(CString& pPacket)
{
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_FIRESPY >> (short)m_id << (pPacket.text() + 1), m_currentLevel, { m_id });
	return true;
}

bool Player::msgPLI_THROWCARRIED(CString& pPacket)
{
	// TODO: Remove when an npcserver is created.
	if (!m_server->getSettings().getBool("duplicatecanbecarried", false) && m_carryNpcId != 0)
	{
		auto npc = m_server->getNPC(m_carryNpcId);
		if (npc)
		{
			m_carryNpcThrown = true;

			// Add the NPC back to the level if it never left.
			auto level = getLevel();
			if (npc->getLevel() == level)
				level->addNPC(npc);
		}
	}
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_THROWCARRIED >> (short)m_id << (pPacket.text() + 1), m_currentLevel, { m_id });
	return true;
}

bool Player::removeItem(LevelItemType itemType)
{
	switch (itemType)
	{
		case LevelItemType::GREENRUPEE: // greenrupee
		case LevelItemType::BLUERUPEE:  // bluerupee
		case LevelItemType::REDRUPEE:   // redrupee
		case LevelItemType::GOLDRUPEE:  // goldrupee
		{
			int gralatsRequired;
			if (itemType == LevelItemType::GOLDRUPEE) gralatsRequired = 100;
			else if (itemType == LevelItemType::REDRUPEE)
				gralatsRequired = 30;
			else if (itemType == LevelItemType::BLUERUPEE)
				gralatsRequired = 5;
			else
				gralatsRequired = 1;

			if (m_gralatCount >= gralatsRequired)
			{
				m_gralatCount -= gralatsRequired;
				return true;
			}

			return false;
		}

		case LevelItemType::BOMBS:
		{
			if (m_bombCount >= 5)
			{
				m_bombCount -= 5;
				return true;
			}
			return false;
		}

		case LevelItemType::DARTS:
		{
			if (m_arrowCount >= 5)
			{
				m_arrowCount -= 5;
				return true;
			}
			return false;
		}

		case LevelItemType::HEART:
		{
			if (m_hitpoints > 1.0f)
			{
				m_hitpoints -= 1.0f;
				return true;
			}
			return false;
		}

#ifndef V8NPCSERVER
		// NOTE: not receiving PLI_ITEMTAKE for >2.31, so we will not remove the item
		// same is true for sword/shield. assuming its true for the weapon-items, but
		// its currently not tested.
		case LevelItemType::GLOVE1:
		case LevelItemType::GLOVE2:
		{
			if (m_glovePower > 1)
			{
				m_glovePower--;
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
			if (m_status & PLSTATUS_HASSPIN)
			{
				m_status &= ~PLSTATUS_HASSPIN;
				return true;
			}
			return false;
		}
	}

	return false;
}

bool Player::msgPLI_ITEMADD(CString& pPacket)
{
	return spawnLevelItem(pPacket, true);
}

bool Player::spawnLevelItem(CString& pPacket, bool playerDrop)
{
	// TODO(joey): serverside item checking
	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };
	unsigned char item = pPacket.readGUChar();

	LevelItemType itemType = LevelItem::getItemId(item);
	if (itemType != LevelItemType::INVALID)
	{
#ifdef V8NPCSERVER
		if (removeItem(itemType) || !playerDrop)
		{
#endif
			auto level = getLevel();
			if (level->addItem(loc[0], loc[1], itemType))
			{
				m_server->sendPacketToOneLevel(CString() >> (char)PLO_ITEMADD << (pPacket.text() + 1), level, { m_id });
			}
			else
			{
				sendPacket(CString() >> (char)PLO_ITEMDEL << (pPacket.text() + 1));
			}

#ifdef V8NPCSERVER
		}
#endif
	}

	return true;
}

bool Player::msgPLI_ITEMDEL(CString& pPacket)
{
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_ITEMDEL << (pPacket.text() + 1), m_currentLevel, { m_id });

	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };

	// Remove the item from the level, getting the type of the item in the process.
	auto level = getLevel();
	LevelItemType item = level->removeItem(loc[0], loc[1]);
	if (item == LevelItemType::INVALID) return true;

	// If this is a PLI_ITEMTAKE packet, give the item to the player.
	if (pPacket[0] - 32 == PLI_ITEMTAKE)
		this->setProps(CString() << LevelItem::getItemPlayerProp(item, this), PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);

	return true;
}

bool Player::msgPLI_CLAIMPKER(CString& pPacket)
{
	// Get the player who killed us.
	unsigned int pId = pPacket.readGUShort();
	auto killer = m_server->getPlayer(pId, PLTYPE_ANYCLIENT);
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
		float oldStats[4] = { m_eloRating, m_eloDeviation, (float)((otherRating >> 9) & 0xFFF), (float)(otherRating & 0x1FF) };

		// If the IPs are the same, don't update the rating to prevent cheating.
		if (CString(m_playerSock->getRemoteIp()) == CString(killer->getSocket()->getRemoteIp()))
			return true;

		float gSpar[2] = { static_cast<float>(1.0f / pow((1.0f + 3.0f * pow(0.0057565f, 2) * (pow(oldStats[3], 2)) / pow(3.14159265f, 2)), 0.5f)),   //Winner
						   static_cast<float>(1.0f / pow((1.0f + 3.0f * pow(0.0057565f, 2) * (pow(oldStats[1], 2)) / pow(3.14159265f, 2)), 0.5f)) }; //Loser
		float ESpar[2] = { 1.0f / (1.0f + pow(10.0f, (-gSpar[1] * (oldStats[2] - oldStats[0]) / 400.0f))),                                           //Winner
						   1.0f / (1.0f + pow(10.0f, (-gSpar[0] * (oldStats[0] - oldStats[2]) / 400.0f))) };                                         //Loser
		float dSpar[2] = { static_cast<float>(1.0f / (pow(0.0057565f, 2) * pow(gSpar[0], 2) * ESpar[0] * (1.0f - ESpar[0]))),                        //Winner
						   static_cast<float>(1.0f / (pow(0.0057565f, 2) * pow(gSpar[1], 2) * ESpar[1] * (1.0f - ESpar[1]))) };                      //Loser

		float tWinRating = oldStats[2] + (0.0057565f / (1.0f / powf(oldStats[3], 2) + 1.0f / dSpar[0])) * (gSpar[0] * (1.0f - ESpar[0]));
		float tLoseRating = oldStats[0] + (0.0057565f / (1.0f / powf(oldStats[1], 2) + 1.0f / dSpar[1])) * (gSpar[1] * (0.0f - ESpar[1]));
		float tWinDeviation = powf((1.0f / (1.0f / powf(oldStats[3], 2) + 1 / dSpar[0])), 0.5f);
		float tLoseDeviation = powf((1.0f / (1.0f / powf(oldStats[1], 2) + 1 / dSpar[1])), 0.5f);

		// Cap the rating.
		tWinRating = clip(tWinRating, 0.0f, 4000.0f);
		tLoseRating = clip(tLoseRating, 0.0f, 4000.0f);
		tWinDeviation = clip(tWinDeviation, 50.0f, 350.0f);
		tLoseDeviation = clip(tLoseDeviation, 50.0f, 350.0f);

		// Update the Ratings.
		// setProps will cause it to grab the new rating and send it to everybody in the level.
		// Therefore, just pass a dummy value.  setProps doesn't alter your rating for packet hacking reasons.
		if (oldStats[0] != tLoseRating || oldStats[1] != tLoseDeviation)
		{
			setRating((int)tLoseRating, (int)tLoseDeviation);
			this->setProps(CString() >> (char)PLPROP_RATING >> (int)0, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		}
		if (oldStats[2] != tWinRating || oldStats[3] != tWinDeviation)
		{
			killer->setRating((int)tWinRating, (int)tWinDeviation);
			killer->setProps(CString() >> (char)PLPROP_RATING >> (int)0, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		}
		this->setLastSparTime(time(0));
		killer->setLastSparTime(time(0));
	}
	else
	{
		CSettings& settings = m_server->getSettings();

		// Give a kill to the player who killed me.
		if (!settings.getBool("dontchangekills", false))
			killer->setKills(killer->getProp(PLPROP_KILLSCOUNT).readGInt() + 1);

		// Now, adjust their AP if allowed.
		if (settings.getBool("apsystem", true))
		{
			signed char oAp = killer->getProp(PLPROP_ALIGNMENT).readGChar();

			// If I have 20 or more AP, they lose AP.
			if (oAp > 0 && m_ap > 19)
			{
				int aptime[] = { settings.getInt("aptime0", 30), settings.getInt("aptime1", 90),
								 settings.getInt("aptime2", 300), settings.getInt("aptime3", 600),
								 settings.getInt("aptime4", 1200) };
				oAp -= (((oAp / 20) + 1) * (m_ap / 20));
				if (oAp < 0) oAp = 0;
				killer->setApCounter((oAp < 20 ? aptime[0] : (oAp < 40 ? aptime[1] : (oAp < 60 ? aptime[2] : (oAp < 80 ? aptime[3] : aptime[4])))));
				killer->setProps(CString() >> (char)PLPROP_ALIGNMENT >> (char)oAp, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
			}
		}
	}

	return true;
}

bool Player::msgPLI_BADDYPROPS(CString& pPacket)
{
	auto level = getLevel();
	if (level == nullptr) return true;

	unsigned char id = pPacket.readGUChar();
	CString props = pPacket.readString("");

	// Get the baddy.
	LevelBaddy* baddy = level->getBaddy(id);
	if (baddy == 0) return true;

	// Get the leader.
	auto leaderId = level->getPlayers().front();
	auto leader = m_server->getPlayer(leaderId);

	// Set the props and send to everybody in the level, except the leader.
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_BADDYPROPS >> (char)id << props, level, { leaderId });
	baddy->setProps(props);
	return true;
}

bool Player::msgPLI_BADDYHURT(CString& pPacket)
{
	auto level = getLevel();
	auto leaderId = level->getPlayers().front();
	auto leader = m_server->getPlayer(leaderId);
	if (leader == nullptr) return true;
	leader->sendPacket(CString() >> (char)PLO_BADDYHURT << (pPacket.text() + 1));
	return true;
}

bool Player::msgPLI_BADDYADD(CString& pPacket)
{
	// Don't add a baddy if we aren't in a level!
	if (m_currentLevel.expired())
		return true;

	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };
	unsigned char bType = pPacket.readGUChar();
	unsigned char bPower = pPacket.readGUChar();
	CString bImage = pPacket.readString("");
	bPower = MIN(bPower, 12); // Hard-limit to 6 hearts.

	// Fix the image for 1.41 clients.
	if (!bImage.isEmpty() && getExtension(bImage).isEmpty())
		bImage << ".gif";

	// Add the baddy.
	auto level = getLevel();
	LevelBaddy* baddy = level->addBaddy(loc[0], loc[1], bType);
	if (baddy == 0) return true;

	// Set the baddy props.
	baddy->setRespawn(false);
	baddy->setProps(CString() >> (char)BDPROP_POWERIMAGE >> (char)bPower >> (char)bImage.length() << bImage);

	// Send the props to everybody in the level.
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_BADDYPROPS >> (char)baddy->getId() << baddy->getProps(), level);
	return true;
}

bool Player::msgPLI_FLAGSET(CString& pPacket)
{
	CSettings& settings = m_server->getSettings();
	CString flagPacket = pPacket.readString("");
	CString flagName, flagValue;
	if (flagPacket.find("=") != -1)
	{
		flagName = flagPacket.readString("=");
		flagValue = flagPacket.readString("");

		// If the value is empty, delete the flag instead.
		if (flagValue.isEmpty())
		{
			pPacket.setRead(1); // Don't let us read the packet ID.
			return msgPLI_FLAGDEL(pPacket);
		}
	}
	else
		flagName = flagPacket;

	// Add a little hack for our special gr.strings.
	if (flagName.find("gr.") != -1)
	{
		if (flagName == "gr.fileerror" || flagName == "gr.filedata")
			return true;

		if (settings.getBool("flaghack_movement", true))
		{
			// gr.x and gr.y are used by the -gr_movement NPC to help facilitate smoother
			// movement amongst pre-2.3 clients.
			if (flagName == "gr.x")
			{
				if (m_versionId >= CLVER_2_3) return true;
				float pos = (float)atof(flagValue.text());
				if (pos != m_x)
					m_grMovementPackets >> (char)PLPROP_X >> (char)(pos * 2.0f) << "\n";
				return true;
			}
			else if (flagName == "gr.y")
			{
				if (m_versionId >= CLVER_2_3) return true;
				float pos = (float)atof(flagValue.text());
				if (pos != m_y)
					m_grMovementPackets >> (char)PLPROP_Y >> (char)(pos * 2.0f) << "\n";
				return true;
			}
			else if (flagName == "gr.z")
			{
				if (m_versionId >= CLVER_2_3) return true;
				float pos = (float)atof(flagValue.text());
				if (pos != m_z)
					m_grMovementPackets >> (char)PLPROP_Z >> (char)((pos + 0.5f) + 50.0f) << "\n";
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
		m_server->setFlag(flagName.text(), flagValue);
		return true;
	}

	// Set Flag
	this->setFlag(flagName.text(), flagValue, (m_versionId > CLVER_2_31));
	return true;
}

bool Player::msgPLI_FLAGDEL(CString& pPacket)
{
	CString flagPacket = pPacket.readString("");
	std::string flagName;
	if (flagPacket.find("=") != -1)
		flagName = flagPacket.readString("=").trim().text();
	else
		flagName = flagPacket.text();

	// this.flags should never be in any server flag list, so just exit.
	if (flagName.find("this.") != std::string::npos) return true;

	// Don't allow anybody to alter read-only strings.
	if (flagName.find("clientr.") != std::string::npos) return true;
	if (flagName.find("serverr.") != std::string::npos) return true;

	// Server flags are handled differently than client flags.
	// TODO: check serveroptions
	if (flagName.find("server.") != std::string::npos)
	{
		m_server->deleteFlag(flagName);
		return true;
	}

	// Remove Flag
	this->deleteFlag(flagName);
	return true;
}

bool Player::msgPLI_OPENCHEST(CString& pPacket)
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
				setProps(CString() << LevelItem::getItemPlayerProp(chestItem, this), PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
				sendPacket(CString() >> (char)PLO_LEVELCHEST >> (char)1 >> (char)cX >> (char)cY);
				m_chestList.push_back(chestStr);
			}
		}
	}

	return true;
}

bool Player::msgPLI_PUTNPC(CString& pPacket)
{
#ifdef V8NPCSERVER
	// Disable if we have an NPC-Server.
	return true;
#endif

	CSettings& settings = m_server->getSettings();

	CString nimage = pPacket.readChars(pPacket.readGUChar());
	CString ncode = pPacket.readChars(pPacket.readGUChar());
	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };

	// See if putnpc is allowed.
	if (!settings.getBool("putnpcenabled"))
		return true;

	// Load the code.
	CString code = m_server->getFileSystem(0)->load(ncode);
	code.removeAllI("\r");

	// Add NPC to level
	m_server->addNPC(nimage, code, loc[0], loc[1], m_currentLevel, false, true);

	return true;
}

bool Player::msgPLI_NPCDEL(CString& pPacket)
{
#ifdef V8NPCSERVER
	// Disable if we have an NPC-Server.
	return true;
#endif

	unsigned int nid = pPacket.readGUInt();

	// Remove the NPC.
	if (auto npc = m_server->getNPC(nid); npc)
		m_server->deleteNPC(npc, !m_currentLevel.expired());

	return true;
}

bool Player::msgPLI_WANTFILE(CString& pPacket)
{
	// Get file.
	CString file = pPacket.readString("");

	// If we are the 1.41 client, make sure a file extension was sent.
	if (m_versionId < CLVER_2_1 && getExtension(file).isEmpty())
		file << ".gif";

	//printf("WANTFILE: %s\n", file.text());

	// Send file.
	this->sendFile(file);
	return true;
}

bool Player::msgPLI_SHOWIMG(CString& pPacket)
{
	m_server->sendPacketToLevelArea(CString() >> (char)PLO_SHOWIMG >> (short)m_id << (pPacket.text() + 1), this->shared_from_this(), { m_id });
	return true;
}

bool Player::msgPLI_HURTPLAYER(CString& pPacket)
{
	unsigned short pId = pPacket.readGUShort();
	char hurtdx = pPacket.readGChar();
	char hurtdy = pPacket.readGChar();
	unsigned char power = pPacket.readGUChar();
	unsigned int npc = pPacket.readGUInt();

	// Get the victim.
	auto victim = m_server->getPlayer(pId, PLTYPE_ANYCLIENT);
	if (victim == 0) return true;

	// If they are paused, they don't get hurt.
	if (victim->getProp(PLPROP_STATUS).readGChar() & PLSTATUS_PAUSED) return true;

	// Send the packet.
	victim->sendPacket(CString() >> (char)PLO_HURTPLAYER >> (short)m_id >> (char)hurtdx >> (char)hurtdy >> (char)power >> (int)npc);

	return true;
}

bool Player::msgPLI_EXPLOSION(CString& pPacket)
{
	CSettings& settings = m_server->getSettings();
	if (settings.getBool("noexplosions", false)) return true;

	unsigned char eradius = pPacket.readGUChar();
	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };
	unsigned char epower = pPacket.readGUChar();

	// Send the packet out.
	CString packet = CString() >> (char)PLO_EXPLOSION >> (short)m_id >> (char)eradius >> (char)(loc[0] * 2) >> (char)(loc[1] * 2) >> (char)epower;
	m_server->sendPacketToOneLevel(packet, m_currentLevel, { m_id });

	return true;
}

bool Player::msgPLI_PRIVATEMESSAGE(CString& pPacket)
{
	// TODO(joey): Is this needed?
	const int sendLimit = 4;
	if (isClient() && (int)difftime(time(0), m_lastMessage) <= 4)
	{
		sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE << "Server message:\xa7You can only send messages once every " << CString((int)sendLimit) << " seconds.");
		return true;
	}
	m_lastMessage = time(0);

	// Check if the player is in a jailed level.
	std::vector<CString> jailList = m_server->getSettings().getStr("jaillevels").tokenize(",");
	bool jailed = false;
	for (std::vector<CString>::iterator i = jailList.begin(); i != jailList.end(); ++i)
	{
		if (i->trim() == m_levelName)
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
	else
		pmMessageType << "\"Private message:\",";

	// Grab the message.
	CString pmMessage = pPacket.readString("");
	int messageLimit = 1024;
	if (pmMessage.length() > messageLimit)
	{
		sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE << "Server message:\xa7There is a message limit of " << CString((int)messageLimit) << " characters.");
		return true;
	}

	// Word filter.
	pmMessage.guntokenizeI();
	if (isClient())
	{
		int filter = m_server->getWordFilter().apply(this, pmMessage, FILTER_CHECK_PM);
		if (filter & FILTER_ACTION_WARN)
		{
			sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE << "Word Filter:\xa7Your PM could not be sent because it was caught by the word filter.");
			return true;
		}
	}

	// Always retokenize string, I don't believe our behavior is inline with official. It was escaping "\", so this unescapes that.
	pmMessage.gtokenizeI();

	// Send the message out.
	for (auto pmPlayerId: pmPlayers)
	{
		if (pmPlayerId >= 16000)
		{
			auto pmPlayer = getExternalPlayer(pmPlayerId);
			if (pmPlayer != nullptr)
			{
				serverlog.out("Sending PM to global player: %s.\n", pmPlayer->getNickname().text());
				pmMessage.guntokenizeI();
				pmExternalPlayer(pmPlayer->getServerName(), pmPlayer->getAccountName(), pmMessage);
				pmMessage.gtokenizeI();
			}
		}
		else
		{
			auto pmPlayer = m_server->getPlayer(pmPlayerId, PLTYPE_ANYPLAYER | PLTYPE_NPCSERVER);
			if (pmPlayer == nullptr || pmPlayer.get() == this) continue;

#ifdef V8NPCSERVER
			if (pmPlayer->isNPCServer())
			{
				m_server->handlePM(this, pmMessage.guntokenize());
				continue;
			}
#endif

			// Don't send to people who don't want mass messages.
			if (pmPlayerCount != 1 && (pmPlayer->getProp(PLPROP_ADDITFLAGS).readGUChar() & PLFLAG_NOMASSMESSAGE))
				continue;

			// Jailed people cannot send PMs to normal players.
			if (jailed && !isStaff() && !pmPlayer->isStaff())
			{
				sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)pmPlayer->getId() << "\"Server Message:\","
																							 << "\"From jail you can only send PMs to admins (RCs).\"");
				continue;
			}

			// Send the message.
			pmPlayer->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)m_id << pmMessageType << pmMessage);
		}
	}

	return true;
}

bool Player::msgPLI_NPCWEAPONDEL(CString& pPacket)
{
	CString weapon = pPacket.readString("");
	for (std::vector<CString>::iterator i = m_weaponList.begin(); i != m_weaponList.end();)
	{
		if (*i == weapon)
		{
			i = m_weaponList.erase(i);
		}
		else
			++i;
	}
	return true;
}

bool Player::msgPLI_PACKETCOUNT(CString& pPacket)
{
	unsigned short count = pPacket.readGUShort();
	if (count != m_packetCount || m_packetCount > 10000)
	{
		serverlog.out("[%s] :: Warning - Player %s had an invalid packet count.\n", m_server->getName().text(), m_accountName.text());
	}
	m_packetCount = 0;

	return true;
}

bool Player::msgPLI_WEAPONADD(CString& pPacket)
{
#ifdef V8NPCSERVER
	// Disable if we have an NPC-Server.
	return true;
#endif

	unsigned char type = pPacket.readGUChar();

	// Type 0 means it is a default weapon.
	if (type == 0)
	{
		this->addWeapon(LevelItem::getItemId(pPacket.readGChar()));
	}
	// NPC weapons.
	else
	{
		// Get the NPC id.
		unsigned int npcId = pPacket.readGUInt();
		auto npc = m_server->getNPC(npcId);
		if (npc == nullptr || npc->getLevel() == nullptr)
			return true;

		// Get the name of the weapon.
		CString name = npc->getWeaponName();
		if (name.length() == 0)
			return true;

		// See if we can find the weapon in the server weapon list.
		auto weapon = m_server->getWeapon(name.toString());

		// If weapon is nullptr, that means the weapon was not found.  Add the weapon to the list.
		if (weapon == nullptr)
		{
			weapon = std::make_shared<Weapon>(m_server, name.toString(), npc->getImage(), std::string{ npc->getSource().getClientGS1() }, npc->getLevel()->getModTime(), true);
			m_server->NC_AddWeapon(weapon);
		}

		// Check and see if the weapon has changed recently.  If it has, we should
		// send the new NPC to everybody on the server.  After updating the script, of course.
		if (weapon->getModTime() < npc->getLevel()->getModTime())
		{
			// Update Weapon
			weapon->updateWeapon(npc->getImage(), std::string{ npc->getSource().getClientGS1() }, npc->getLevel()->getModTime());

			// Send to Players
			m_server->updateWeaponForPlayers(weapon);
		}

		// Send the weapon to the player now.
		if (!hasWeapon(weapon->getName()))
			this->addWeapon(weapon);
	}

	return true;
}

bool Player::msgPLI_UPDATEFILE(CString& pPacket)
{
	FileSystem* fileSystem = m_server->getFileSystem();

	// Get the packet data and file mod time.
	time_t modTime = pPacket.readGUInt5();
	CString file = pPacket.readString("");
	time_t fModTime = fileSystem->getModTime(file);

	// If we are the 1.41 client, make sure a file extension was sent.
	if (m_versionId < CLVER_2_1 && getExtension(file).isEmpty())
		file << ".gif";

	//printf("UPDATEFILE: %s\n", file.text());

	// Make sure it isn't one of the default files.
	bool isDefault = false;
	for (auto& defaultFile: __defaultfiles)
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

	if (m_versionId < CLVER_2_1)
		sendPacket(CString() >> (char)PLO_FILESENDFAILED << file);
	else
		sendPacket(CString() >> (char)PLO_FILEUPTODATE << file);
	return true;
}

bool Player::msgPLI_ADJACENTLEVEL(CString& pPacket)
{
	time_t modTime = pPacket.readGUInt5();
	CString levelName = pPacket.readString("");
	CString packet;
	auto adjacentLevel = Level::findLevel(levelName, m_server);

	if (!adjacentLevel)
		return true;

	if (m_currentLevel.expired())
		return false;

	bool alreadyVisited = false;
	for (const auto& cl: m_cachedLevels)
	{
		if (auto clevel = cl->level.lock(); clevel == adjacentLevel)
		{
			alreadyVisited = true;
			break;
		}
	}

	// Send the level.
	if (m_versionId >= CLVER_2_1)
		sendLevel(adjacentLevel, modTime, true);
	else
		sendLevel141(adjacentLevel, modTime, true);

	// Set our old level back to normal.
	//sendPacket(CString() >> (char)PLO_LEVELNAME << level->getLevelName());
	auto map = m_pmap.lock();
	if (map && map->getType() == MapType::GMAP)
		sendPacket(CString() >> (char)PLO_LEVELNAME << map->getMapName());
	else
		sendPacket(CString() >> (char)PLO_LEVELNAME << getLevel()->getLevelName());

	if (getLevel()->isPlayerLeader(m_id))
		sendPacket(CString() >> (char)PLO_ISLEADER);

	return true;
}

bool Player::msgPLI_HITOBJECTS(CString& pPacket)
{
	float power = (float)pPacket.readGChar() / 2.0f;
	float loc[2] = { (float)pPacket.readGChar() / 2.0f, (float)pPacket.readGChar() / 2.0f };
	int nid = (pPacket.bytesLeft() != 0) ? pPacket.readGUInt() : -1;

	// Construct the packet.
	// {46}{SHORT player_id / 0 for NPC}{CHAR power}{CHAR x}{CHAR y}[{INT npc_id}]
	CString nPacket;
	nPacket >> (char)PLO_HITOBJECTS;
	nPacket >> (short)((nid == -1) ? m_id : 0); // If it came from an NPC, send 0 for the id.
	nPacket >> (char)(power * 2) >> (char)(loc[0] * 2) >> (char)(loc[1] * 2);
	if (nid != -1) nPacket >> (int)nid;

	m_server->sendPacketToLevelArea(nPacket, shared_from_this(), { m_id });
	return true;
}

bool Player::msgPLI_LANGUAGE(CString& pPacket)
{
	m_language = pPacket.readString("");
	if (m_language.isEmpty())
		m_language = "English";
	return true;
}

bool Player::msgPLI_TRIGGERACTION(CString& pPacket)
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
	if (triggerActionData.empty())
	{
		return true;
	}

	// Grab action name
	std::string actualActionName = triggerActionData[0].toLower().toString();

	// (int)(loc[0]) % 64 == 0.0f, for gmap?
	// TODO(joey): move into trigger command dispatcher, some use private player vars.
	if (loc[0] == 0.0f && loc[1] == 0.0f)
	{
		CSettings& settings = m_server->getSettings();

		if (settings.getBool("triggerhack_execscript", false))
		{
			if (action.find("gr.es_clear") == 0)
			{
				// Clear the parameters.
				m_grExecParameterList.clear();
				return true;
			}
			else if (action.find("gr.es_set") == 0)
			{
				// Add the parameter to our saved parameter list.
				CString parameters = action.subString(9);
				if (m_grExecParameterList.isEmpty())
					m_grExecParameterList = parameters;
				else
					m_grExecParameterList << "," << parameters;
				return true;
			}
			else if (action.find("gr.es_append") == 0)
			{
				// Append doesn't add the beginning comma.
				CString parameters = action.subString(9);
				if (m_grExecParameterList.isEmpty())
					m_grExecParameterList = parameters;
				else
					m_grExecParameterList << parameters;
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
					FileSystem execscripts(m_server);
					execscripts.addDir("execscripts");
					CString wepscript = execscripts.load(actionParts[2]);

					// Check to see if we were able to load the weapon.
					if (wepscript.isEmpty())
					{
						serverlog.out("[%s] Error: Player %s tried to load execscript %s, but the script was not found.\n", m_server->getName().text(), m_accountName.text(), actionParts[2].text());
						return true;
					}

					// Format the weapon script properly.
					wepscript.removeAllI("\r");
					wepscript.replaceAllI("\n", "\xa7");

					// Replace parameters.
					std::vector<CString> parameters = m_grExecParameterList.tokenize(",");
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
					CString weapon_packet = CString() >> (char)PLO_NPCWEAPONADD >> (char)wepname.length() << wepname >> (char)0 >> (char)wepimage.length() << wepimage >> (char)1 >> (short)wepscript.length() << wepscript;

					// Send it to the players now.
					if (actionParts[1] == "ALLPLAYERS")
						m_server->sendPacketToType(PLTYPE_ANYCLIENT, weapon_packet);
					else
					{
						auto p = m_server->getPlayer(actionParts[1], PLTYPE_ANYCLIENT);
						if (p) p->sendPacket(weapon_packet);
					}
					m_grExecParameterList.clear();
				}
				return true;
			}
		}

		if (settings.getBool("triggerhack_files", false))
		{
			if (action.find("gr.appendfile") == 0)
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
				file.load(m_server->getServerPath() << "logs/" << filename);

				// Save the file.
				file << action.subString(finish) << "\r\n";
				file.save(m_server->getServerPath() << "logs/" << filename);
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
				file.save(m_server->getServerPath() << "logs/" << filename);
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
				filedata.load(m_server->getServerPath() << "logs/" << filename);
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
				sendPacket(CString() >> (char)PLO_FLAGSET << "gr.fileerror=" << error);
				sendPacket(CString() >> (char)PLO_FLAGSET << "gr.filedata=" << tokens[line]);
			}
		}

		if (settings.getBool("triggerhack_props", false))
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

		if (settings.getBool("triggerhack_levels", false))
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
						LevelPtr targetLevel;
						if (getExtension(levelName) == ".singleplayer")
							targetLevel = m_singleplayerLevels[removeExtension(levelName)];
						else
							targetLevel = m_server->getLevel(levelName.toString());
						if (targetLevel != nullptr)
							targetLevel->reload();
					}
				}
				else
					level->reload();
			}
		}
	}

	bool handled = m_server->getTriggerDispatcher().execute(actualActionName, this, triggerActionData);

	if (!handled)
	{
		if (auto level = getLevel(); level)
		{
#ifdef V8NPCSERVER
			// Send to server scripts
			auto npcList = level->findAreaNpcs(int(loc[0] * 16.0), int(loc[1] * 16.0), 8, 8);
			for (auto npcTouched: npcList)
				npcTouched->queueNpcTrigger(actualActionName, this, utilities::retokenizeArray(triggerActionData, 1));
#endif

			// Send to the level.
			m_server->sendPacketToOneLevel(CString() >> (char)PLO_TRIGGERACTION >> (short)m_id << (pPacket.text() + 1), level, { m_id });
		}
	}

	return true;
}

bool Player::msgPLI_MAPINFO(CString& pPacket)
{
	// Don't know what this does exactly.  Might be gmap related.
	pPacket.readString("");
	return true;
}

void ShootPacketNew::debug()
{
	printf("Shoot: %f, %f, %f with gani %s: (len=%d)\n", (float)pixelx / 16.0f, (float)pixely / 16.0f, (float)pixelz / 16.0f, gani.text(), gani.length());
	printf("\t Offset: %d, %d\n", offsetx, offsety);
	printf("\t Angle: %d\n", sangle);
	printf("\t Z-Angle: %d\n", sanglez);
	printf("\t Power: %d\n", speed);
	printf("\t Gravity: %d\n", gravity);
	printf("\t Gani: %s (len: %d)\n", gani.text(), gani.length());
	printf("\t Shoot Params: %s (len: %d)\n", shootParams.text(), shootParams.length());
}

CString ShootPacketNew::constructShootV1() const
{
	CString ganiTemp{};
	ganiTemp << gani;
	if (!ganiArgs.isEmpty())
	{
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

CString ShootPacketNew::constructShootV2() const
{
	CString ganiTemp{};
	ganiTemp << gani;
	if (!ganiArgs.isEmpty())
	{
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

bool Player::msgPLI_SHOOT(CString& pPacket)
{
	ShootPacketNew newPacket{};
	int unknown = pPacket.readGInt(); // May be a shoot id for the npc-server. (5/25d/19) joey: all my tests just give 0, my guess would be different types of projectiles but it never came to fruition

	newPacket.pixelx = 16 * pPacket.readGChar();        // 16 * ((float)pPacket.readGUChar() / 2.0f);
	newPacket.pixely = 16 * pPacket.readGChar();        // 16 * ((float)pPacket.readGUChar() / 2.0f);
	newPacket.pixelz = 16 * (pPacket.readGChar() - 50); // 16 * ((float)pPacket.readGUChar() / 2.0f);
	// TODO: calculate offsetx from pixelx/pixely/ - level offset
	newPacket.offsetx = 0;
	newPacket.offsety = 0;
	//if (newPacket.pixelx < 0) {
	//	newPacket.offsetx = -1;
	//}
	//if (newPacket.pixely < 0) {
	//	newPacket.offsety = -1;
	//}
	newPacket.sangle = pPacket.readGUChar();  // 0-pi = 0-220
	newPacket.sanglez = pPacket.readGUChar(); // 0-pi = 0-220
	newPacket.speed = pPacket.readGUChar();   // speed = pixels per 0.05 seconds.  In gscript, each value of 1 translates to 44 pixels.
	newPacket.gravity = 8;
	newPacket.gani = pPacket.readChars(pPacket.readGUChar());
	unsigned char someParam = pPacket.readGUChar(); // This seems to be the length of shootparams, but the client doesn't limit itself and sends the overflow anyway
	newPacket.shootParams = pPacket.readString("");

	CString oldPacketBuf = CString() >> (char)PLO_SHOOT >> (short)m_id << newPacket.constructShootV1();
	CString newPacketBuf = CString() >> (char)PLO_SHOOT2 >> (short)m_id << newPacket.constructShootV2();

	m_server->sendPacketToLevelArea(oldPacketBuf, shared_from_this(), { m_id }, [](const auto pl)
									{
										return pl->getVersion() < CLVER_5_07;
									});
	m_server->sendPacketToLevelArea(newPacketBuf, shared_from_this(), { m_id }, [](const auto pl)
									{
										return pl->getVersion() >= CLVER_5_07;
									});

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

bool Player::msgPLI_SHOOT2(CString& pPacket)
{
	ShootPacketNew newPacket{};
	newPacket.pixelx = pPacket.readGUShort();
	newPacket.pixely = pPacket.readGUShort();
	newPacket.pixelz = pPacket.readGUShort();
	newPacket.offsetx = pPacket.readGChar();  // level offset x
	newPacket.offsety = pPacket.readGChar();  // level offset y
	newPacket.sangle = pPacket.readGUChar();  // 0-pi = 0-220
	newPacket.sanglez = pPacket.readGUChar(); // 0-pi = 0-220
	newPacket.speed = pPacket.readGUChar();   // speed = pixels per 0.05 seconds.  In gscript, each value of 1 translates to 44 pixels.
	newPacket.gravity = pPacket.readGUChar();
	newPacket.gani = pPacket.readChars(pPacket.readGUShort());
	unsigned char someParam = pPacket.readGUChar(); // This seems to be the length of shootparams, but the client doesn't limit itself and sends the overflow anyway
	newPacket.shootParams = pPacket.readString("");

	CString oldPacketBuf = CString() >> (char)PLO_SHOOT >> (short)m_id << newPacket.constructShootV1();
	CString newPacketBuf = CString() >> (char)PLO_SHOOT2 >> (short)m_id << newPacket.constructShootV2();

	m_server->sendPacketToLevelArea(oldPacketBuf, shared_from_this(), { m_id }, [](const auto pl)
									{
										return pl->getVersion() < CLVER_5_07;
									});
	m_server->sendPacketToLevelArea(newPacketBuf, shared_from_this(), { m_id }, [](const auto pl)
									{
										return pl->getVersion() >= CLVER_5_07;
									});

	return true;
}

bool Player::msgPLI_SERVERWARP(CString& pPacket)
{
	CString servername = pPacket.readString("");
	m_server->getServerLog().out("%s is requesting serverwarp to %s", m_accountName.text(), servername.text());
	m_server->getServerList().sendPacket(CString() >> (char)SVO_SERVERINFO >> (short)m_id << servername);
	return true;
}

bool Player::msgPLI_PROCESSLIST(CString& pPacket)
{
	std::vector<CString> processes = pPacket.readString("").guntokenize().tokenize("\n");
	return true;
}

bool Player::msgPLI_UNKNOWN46(CString& pPacket)
{
#ifdef DEBUG
	printf("TODO: Player::msgPLI_UNKNOWN46: ");
	CString packet = pPacket.readString("");
	for (int i = 0; i < packet.length(); ++i) printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif
	return true;
}

bool Player::msgPLI_RAWDATA(CString& pPacket)
{
	m_nextIsRaw = true;
	m_rawPacketSize = pPacket.readGUInt();
	return true;
}

bool Player::msgPLI_PROFILEGET(CString& pPacket)
{
	// Send the packet ID for backwards compatibility.
	m_server->getServerList().sendPacket(CString() >> (char)SVO_GETPROF >> (short)m_id << pPacket);
	return true;
}

bool Player::msgPLI_PROFILESET(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc != m_accountName) return true;

	// Old gserver would send the packet ID with pPacket so, for
	// backwards compatibility, do that here.
	m_server->getServerList().sendPacket(CString() >> (char)SVO_SETPROF << pPacket);
	return true;
}

bool Player::msgPLI_RC_UNKNOWN162(CString& pPacket)
{
	// Stub.
	return true;
}
