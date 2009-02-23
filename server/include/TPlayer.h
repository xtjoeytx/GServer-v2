#ifndef TPLAYER_H
#define TPLAYER_H

#include <time.h>
#include <vector>
#include "ICommon.h"
#include "IUtil.h"
#include "CSocket.h"
#include "CFileQueue.h"
#include "TServer.h"
#include "TAccount.h"
#include "TLevel.h"
#include "TWeapon.h"
#include "codec.h"

/*
	Enumerators
*/
// PLI_UNKNOWN## are packets sent by the client whose
// purpose is unknown.
enum
{
	PLI_LEVELWARP		= 0,
	PLI_BOARDMODIFY		= 1,
	PLI_PLAYERPROPS		= 2,
	PLI_NPCPROPS		= 3,
	PLI_BOMBADD			= 4,
	PLI_BOMBDEL			= 5,
	PLI_TOALL			= 6,
	PLI_HORSEADD		= 7,
	PLI_HORSEDEL		= 8,
	PLI_ARROWADD		= 9,
	PLI_FIRESPY			= 10,
	PLI_THROWCARRIED	= 11,
	PLI_ITEMADD			= 12,
	PLI_ITEMDEL			= 13,
	PLI_CLAIMPKER		= 14,
	PLI_BADDYPROPS		= 15,
	PLI_BADDYHURT		= 16,
	PLI_BADDYADD		= 17,
	PLI_FLAGSET			= 18,
	PLI_FLAGDEL			= 19,
	PLI_OPENCHEST		= 20,
	PLI_NPCADD			= 21,
	PLI_NPCDEL			= 22,
	PLI_WANTFILE		= 23,
	PLI_SHOWIMG			= 24,
	PLI_EMPTY25			= 25,
	PLI_HURTPLAYER		= 26,
	PLI_EXPLOSION		= 27,
	PLI_PRIVATEMESSAGE	= 28,
	PLI_NPCWEAPONDEL	= 29,
	PLI_LEVELWARPMOD	= 30,
	PLI_PACKETCOUNT		= 31,
	PLI_ITEMTAKE		= 32,
	PLI_WEAPONADD		= 33,
	PLI_UPDATEFILE		= 34,
	PLI_ADJACENTLEVEL	= 35,
	PLI_HITOBJECTS		= 36,
	PLI_LANGUAGE		= 37,
	PLI_TRIGGERACTION	= 38,
	PLI_MAPINFO			= 39,
	PLI_SHOOT			= 40,
	PLI_UNKNOWN46		= 46,	// Always is 1.  Might be a player count for the gmap level.
	PLI_UNKNOWN47		= 47,	// Seems to tell the server the modTime of update files.  Used for client updates.
	PLI_RAWDATA			= 50,
	PLI_RC_SERVEROPTIONSGET		= 51,
	PLI_RC_SERVEROPTIONSSET		= 52,
	PLI_RC_FOLDERCONFIGGET		= 53,
	PLI_RC_FOLDERCONFIGSET		= 54,
	PLI_RC_RESPAWNSET			= 55,
	PLI_RC_HORSELIFESET			= 56,
	PLI_RC_APINCREMENTSET		= 57,
	PLI_RC_BADDYRESPAWNSET		= 58,
	PLI_RC_PLAYERPROPSGET		= 59,
	PLI_RC_PLAYERPROPSSET		= 60,
	PLI_RC_DISCONNECTPLAYER		= 61,
	PLI_RC_UPDATELEVELS			= 62,
	PLI_RC_ADMINMESSAGE			= 63,
	PLI_RC_PRIVADMINMESSAGE		= 64,
	PLI_RC_LISTRCS				= 65,
	PLI_RC_DISCONNECTRC			= 66,
	PLI_RC_APPLYREASON			= 67,
	PLI_RC_SERVERFLAGSGET		= 68,
	PLI_RC_SERVERFLAGSSET		= 69,
	PLI_RC_ACCOUNTADD			= 70,
	PLI_RC_ACCOUNTDEL			= 71,
	PLI_RC_ACCOUNTLISTGET		= 72,
	PLI_RC_PLAYERPROPSGET2		= 73,	// Gets by player ID
	PLI_RC_PLAYERPROPSGET3		= 74,	// Gets by player account name.
	PLI_RC_PLAYERPROPSRESET		= 75,
	PLI_RC_PLAYERPROPSSET2		= 76,
	PLI_RC_ACCOUNTGET			= 77,
	PLI_RC_ACCOUNTSET			= 78,
	PLI_RC_CHAT					= 79,
	PLI_PROFILEGET		= 80,
	PLI_PROFILESET		= 81,
	PLI_RC_WARPPLAYER			= 82,
	PLI_RC_PLAYERRIGHTSGET		= 83,
	PLI_RC_PLAYERRIGHTSSET		= 84,
	PLI_RC_PLAYERCOMMENTSGET	= 85,
	PLI_RC_PLAYERCOMMENTSSET	= 86,
	PLI_RC_PLAYERBANGET			= 87,
	PLI_RC_PLAYERBANSET			= 88,
	PLI_RC_FILEBROWSER_START	= 89,
	PLI_RC_FILEBROWSER_CD		= 90,
	PLI_RC_FILEBROWSER_END		= 91,
	PLI_RC_FILEBROWSER_DOWN		= 92,
	PLI_RC_FILEBROWSER_UP		= 93,
	PLI_RC_FILEBROWSER_MOVE		= 96,
	PLI_RC_FILEBROWSER_DELETE	= 97,
	PLI_RC_FILEBROWSER_RENAME	= 98,
	PLI_UNKNOWN152		= 152,	// Gets a value from the GraalEngine (or a server-side NPC?) (probably a database)
	PLI_UNKNOWN154		= 154,	// Sets a value on the GraalEngine (or a server-side NPC?) (probably a database)
	PLI_UNKNOWN157		= 157,	// Something to do with ganis.
	PLI_UPDATESCRIPT	= 158,	// {158}{script} Requests a script from the server.
};

enum
{
	PLO_LEVELBOARD		= 0,
	PLO_LEVELLINK		= 1,
	PLO_BADDYPROPS		= 2,
	PLO_NPCPROPS		= 3,
	PLO_LEVELCHEST		= 4,
	PLO_LEVELSIGN		= 5,
	PLO_LEVELNAME		= 6,
	PLO_BOARDMODIFY		= 7,
	PLO_OTHERPLPROPS	= 8,
	PLO_PLAYERPROPS		= 9,
	PLO_ISLEADER		= 10,
	PLO_BOMBADD			= 11,
	PLO_BOMBDEL			= 12,
	PLO_TOALL			= 13,
	PLO_PLAYERWARP		= 14,
	PLO_WARPFAILED		= 15,
	PLO_DISCMESSAGE		= 16,
	PLO_HORSEADD		= 17,
	PLO_HORSEDEL		= 18,
	PLO_ARROWADD		= 19,
	PLO_FIRESPY			= 20,
	PLO_THROWCARRIED	= 21,
	PLO_ITEMADD			= 22,
	PLO_ITEMDEL			= 23,
	PLO_NPCMOVED		= 24,	// What does this do?
	PLO_SIGNATURE		= 25,
	PLO_NPCACTION		= 26,	// What does this do?
	PLO_BADDYHURT		= 27,
	PLO_FLAGSET			= 28,
	PLO_NPCDEL			= 29,
	PLO_FILESENDFAILED	= 30,
	PLO_FLAGDEL			= 31,
	PLO_SHOWIMG			= 32,
	PLO_NPCWEAPONADD	= 33,
	PLO_NPCWEAPONDEL	= 34,
	PLO_RC_ADMINMESSAGE	= 35,
	PLO_EXPLOSION		= 36,
	PLO_PRIVATEMESSAGE	= 37,
	PLO_PUSHAWAY		= 38,	// What does this do?
	PLO_LEVELMODTIME	= 39,
	PLO_HURTPLAYER		= 40,
	PLO_STARTMESSAGE	= 41,
	PLO_NEWWORLDTIME	= 42,
	PLO_DEFAULTWEAPON	= 43,
	PLO_HASNPCSERVER	= 44,	// If sent, the client won't update npc props.
	PLO_FILEUPTODATE	= 45,
	PLO_HITOBJECTS		= 46,
	PLO_STAFFGUILDS		= 47,
	PLO_TRIGGERACTION	= 48,
	PLO_PLAYERWARP2		= 49,	// Bytes 1-3 are x/y/z. 4 = level x in gmap, 5 = level y in gmap.
	PLO_ADDPLAYER		= 55,
	PLO_DELPLAYER		= 56,
	PLO_RC_SERVERFLAGSGET		= 61,
	PLO_RC_PLAYERRIGHTSGET		= 62,
	PLO_RC_PLAYERCOMMENTSGET	= 63,
	PLO_RC_PLAYERBANGET			= 64,
	PLO_RC_FILEBROWSER_DIRLIST	= 65,
	PLO_RC_FILEBROWSER_DIR		= 66,
	PLO_RC_FILEBROWSER_MESSAGE	= 67,
	PLO_LARGEFILESTART	= 68,
	PLO_LARGEFILEEND	= 69,
	PLO_RC_ACCOUNTLISTGET		= 70,
	PLO_RC_PLAYERPROPSGET		= 72,
	PLO_RC_ACCOUNTGET			= 73,
	PLO_RC_CHAT			= 74,
	PLO_PROFILE			= 75,
	PLO_RC_SERVEROPTIONSGET		= 76,
	PLO_RC_FOLDERCONFIGGET		= 77,
	PLO_NPCSERVERADDR	= 79,
	PLO_UNKNOWN82		= 82,	// Answers PLI_UNKNOWN152's request.
	PLO_LARGEFILESIZE	= 84,
	PLO_RAWDATA			= 100,
	PLO_BOARDPACKET		= 101,
	PLO_FILE			= 102,
	PLO_NPCBYTECODE		= 131,	// Compiled Torque-script for an NPC.
	PLO_NPCDEL2			= 150,	// {150}{CHAR level_length}{level}{INT3 npcid}
	PLO_HIDENPCS		= 151,
	PLO_SAY				= 153,	// {153}{text}
	PLO_FREEZEPLAYER2	= 154,	// Blank.
	PLO_UNFREEZEPLAYER	= 155,	// Blank.
	PLO_SETACTIVELEVEL	= 156,	// Sets the level to receive chests, baddies, NPCs, etc.
	PLO_GHOSTMODE		= 170,
	PLO_BIGMAP			= 172,	// [172] zodiacminimap.txt,zodiacworldminimap3.png,10,10
	PLO_GHOSTICON		= 174,	// Pass 1 to enable the ghost icon
	PLO_SHOOT			= 175,
	PLO_FULLSTOP		= 176,	// Sending this causes the entire client to not respond to normal input and it hides the HUD.
	PLO_FULLSTOP2		= 177,	// Sending this causes the entire client to not respond to normal input and it hides the HUD.
	PLO_RPGWINDOW		= 179,
	PLO_STATUSLIST		= 180,
	PLO_LISTPROCESSES	= 182,
	PLO_EMPTY190		= 190,	// Was blank.  Sent before weapon list.
	PLO_EMPTY194		= 194,	// Was blank.  Sent before weapon list.

	// Seems to register NPCs or something on the client.
	// Also is related to PLI_UPDATESCRIPT as it sends the last modification time of the NPC/weapon.  The v5 client stores weapon scripts offline.
	PLO_EMPTY197		= 197,	// Seems to register npcs on the client.  Also is used by client to see if it needs to get a newer version of the offline cache of the NPC.
};

enum
{
	PLFLAG_NOMASSMESSAGE	= 0x01,
	PLFLAG_NOTOALL			= 0x04,
};

enum
{
	PLSTATUS_PAUSED			= 0x01,
	PLSTATUS_HIDDEN			= 0x02,
	PLSTATUS_MALE			= 0x04,
	PLSTATUS_DEAD			= 0x08,
	PLSTATUS_ALLOWWEAPONS	= 0x10,
	PLSTATUS_HASSPIN		= 0x40,
};

enum
{
	PLTYPE_AWAIT		= (int)(-1),
	PLTYPE_CLIENT		= (int)(1 << 0),
	PLTYPE_RC			= (int)(1 << 1),
	PLTYPE_NC			= (int)(1 << 3),
	PLTYPE_CLIENT2		= (int)(1 << 5),
	PLTYPE_RC2			= (int)(1 << 6),
	PLTYPE_ANYCLIENT	= (int)(PLTYPE_CLIENT | PLTYPE_CLIENT2),
	PLTYPE_ANYRC		= (int)(PLTYPE_RC | PLTYPE_RC2),
};

struct SCachedLevel
{
	SCachedLevel(TLevel* pLevel, time_t pModTime) : level(pLevel), modTime(pModTime) { }
	TLevel* level;
	time_t modTime;
};

class TMap;
class TPlayer : public TAccount, public CSocketStub
{
	public:
		// Required by CSocketStub.
		bool onRecv();
		bool onSend();
		SOCKET getSocketHandle()	{ return playerSock->getHandle(); }
		bool canSend();

		// Constructor - Deconstructor
		TPlayer(TServer* pServer, CSocket* pSocket, int pId);
		~TPlayer();
		void operator()();

		// Manage Account
		inline bool isLoggedIn() const;
		bool sendLogin();

		// Get Properties
		TLevel* getLevel()		{ return level; }
		TMap* getMap()			{ return pmap; }
		int getId() const;
		int getType() const;
		time_t getLastData() const		{ return lastData; }
		CString getFlag(const CString& flag) const;
		CString getGuild() const		{ return guild; }
		int getVersion() const			{ return versionID; }
		CString getVersionStr() const	{ return version; }
		bool isUsingFileBrowser() const	{ return isFtp; }

		// Set Properties
		void setChat(const CString& pChat);
		void setNick(CString& pNickName, bool force = false);
		void setId(int pId);
		void setLoaded(bool loaded)		{ this->loaded = loaded; }

		// Level manipulation
		bool warp(const CString& pLevelName, float pX, float pY, time_t modTime = 0);
		bool setLevel(const CString& pLevelName, time_t modTime = 0);
		bool sendLevel(TLevel* pLevel, time_t modTime, bool skipActors = false);
		bool leaveLevel(bool resetCache = false);
		time_t getCachedLevelModTime(const TLevel* level) const;

		// Prop-Manipulation
		CString getProp(int pPropId);
		void setProps(CString& pPacket, bool pForward = false, bool pForwardToSelf = false, TPlayer* rc = 0);
		void sendProps(const bool *pProps, int pCount);
		CString getProps(const bool *pProps, int pCount);
		void setPropsRC(CString& pPacket, TPlayer* rc);
		CString getPropsRC();

		// Socket-Functions
		bool doMain();
		void sendPacket(CString pPacket);

		// Misc functions.
		bool doTimedEvents();
		void disconnect();
		void processChat(CString pChat);
		bool isStaff();
		bool isRC()				{ return (type & PLTYPE_ANYRC ? true : false); }
		bool isClient()			{ return (type & PLTYPE_ANYCLIENT ? true : false); }

		// Packet-Functions
		bool msgPLI_NULL(CString& pPacket);
		bool msgPLI_LOGIN(CString& pPacket);

		bool msgPLI_LEVELWARP(CString& pPacket);
		bool msgPLI_BOARDMODIFY(CString& pPacket);
		bool msgPLI_PLAYERPROPS(CString& pPacket);
		bool msgPLI_NPCPROPS(CString& pPacket);
		bool msgPLI_BOMBADD(CString& pPacket);
		bool msgPLI_BOMBDEL(CString& pPacket);
		bool msgPLI_TOALL(CString& pPacket);
		bool msgPLI_HORSEADD(CString& pPacket);
		bool msgPLI_HORSEDEL(CString& pPacket);
		bool msgPLI_ARROWADD(CString& pPacket);
		bool msgPLI_FIRESPY(CString& pPacket);
		bool msgPLI_THROWCARRIED(CString& pPacket);
		bool msgPLI_ITEMADD(CString& pPacket);
		bool msgPLI_ITEMDEL(CString& pPacket);
		bool msgPLI_CLAIMPKER(CString& pPacket);
		bool msgPLI_BADDYPROPS(CString& pPacket);
		bool msgPLI_BADDYHURT(CString& pPacket);
		bool msgPLI_BADDYADD(CString& pPacket);
		bool msgPLI_FLAGSET(CString& pPacket);
		bool msgPLI_FLAGDEL(CString& pPacket);
		bool msgPLI_OPENCHEST(CString& pPacket);
		bool msgPLI_NPCADD(CString& pPacket);
		bool msgPLI_NPCDEL(CString& pPacket);
		bool msgPLI_WANTFILE(CString& pPacket);
		bool msgPLI_SHOWIMG(CString& pPacket);
		// PLI_EMPTY25
		bool msgPLI_HURTPLAYER(CString& pPacket);
		bool msgPLI_EXPLOSION(CString& pPacket);
		bool msgPLI_PRIVATEMESSAGE(CString& pPacket);
		bool msgPLI_NPCWEAPONDEL(CString& pPacket);
		bool msgPLI_WEAPONADD(CString& pPacket);
		bool msgPLI_UPDATEFILE(CString& pPacket);
		bool msgPLI_ADJACENTLEVEL(CString& pPacket);
		bool msgPLI_HITOBJECTS(CString& pPacket);
		bool msgPLI_LANGUAGE(CString& pPacket);
		bool msgPLI_TRIGGERACTION(CString& pPacket);
		bool msgPLI_MAPINFO(CString& pPacket);
		bool msgPLI_SHOOT(CString& pPacket);
		bool msgPLI_UNKNOWN46(CString& pPacket);
		bool msgPLI_RAWDATA(CString& pPacket);

		bool msgPLI_RC_SERVEROPTIONSGET(CString& pPacket);
		bool msgPLI_RC_SERVEROPTIONSSET(CString& pPacket);
		bool msgPLI_RC_FOLDERCONFIGGET(CString& pPacket);
		bool msgPLI_RC_FOLDERCONFIGSET(CString& pPacket);
		bool msgPLI_RC_RESPAWNSET(CString& pPacket);
		bool msgPLI_RC_HORSELIFESET(CString& pPacket);
		bool msgPLI_RC_APINCREMENTSET(CString& pPacket);
		bool msgPLI_RC_BADDYRESPAWNSET(CString& pPacket);
		bool msgPLI_RC_PLAYERPROPSGET(CString& pPacket);
		bool msgPLI_RC_PLAYERPROPSSET(CString& pPacket);
		bool msgPLI_RC_DISCONNECTPLAYER(CString& pPacket);
		bool msgPLI_RC_UPDATELEVELS(CString& pPacket);
		bool msgPLI_RC_ADMINMESSAGE(CString& pPacket);
		bool msgPLI_RC_PRIVADMINMESSAGE(CString& pPacket);
		bool msgPLI_RC_LISTRCS(CString& pPacket);
		bool msgPLI_RC_DISCONNECTRC(CString& pPacket);
		bool msgPLI_RC_APPLYREASON(CString& pPacket);
		bool msgPLI_RC_SERVERFLAGSGET(CString& pPacket);
		bool msgPLI_RC_SERVERFLAGSSET(CString& pPacket);
		bool msgPLI_RC_ACCOUNTADD(CString& pPacket);
		bool msgPLI_RC_ACCOUNTDEL(CString& pPacket);
		bool msgPLI_RC_ACCOUNTLISTGET(CString& pPacket);
		bool msgPLI_RC_PLAYERPROPSGET2(CString& pPacket);
		bool msgPLI_RC_PLAYERPROPSGET3(CString& pPacket);
		bool msgPLI_RC_PLAYERPROPSRESET(CString& pPacket);
		bool msgPLI_RC_PLAYERPROPSSET2(CString& pPacket);
		bool msgPLI_RC_ACCOUNTGET(CString& pPacket);
		bool msgPLI_RC_ACCOUNTSET(CString& pPacket);
		bool msgPLI_RC_CHAT(CString& pPacket);
		bool msgPLI_PROFILEGET(CString& pPacket);
		bool msgPLI_PROFILESET(CString& pPacket);
		bool msgPLI_RC_WARPPLAYER(CString& pPacket);
		bool msgPLI_RC_PLAYERRIGHTSGET(CString& pPacket);
		bool msgPLI_RC_PLAYERRIGHTSSET(CString& pPacket);
		bool msgPLI_RC_PLAYERCOMMENTSGET(CString& pPacket);
		bool msgPLI_RC_PLAYERCOMMENTSSET(CString& pPacket);
		bool msgPLI_RC_PLAYERBANGET(CString& pPacket);
		bool msgPLI_RC_PLAYERBANSET(CString& pPacket);
		bool msgPLI_RC_FILEBROWSER_START(CString& pPacket);
		bool msgPLI_RC_FILEBROWSER_CD(CString& pPacket);
		bool msgPLI_RC_FILEBROWSER_END(CString& pPacket);
		bool msgPLI_RC_FILEBROWSER_DOWN(CString& pPacket);
		bool msgPLI_RC_FILEBROWSER_UP(CString& pPacket);
		bool msgPLI_RC_FILEBROWSER_MOVE(CString& pPacket);
		bool msgPLI_RC_FILEBROWSER_DELETE(CString& pPacket);
		bool msgPLI_RC_FILEBROWSER_RENAME(CString& pPacket);

	private:
		// Login functions.
		bool sendLoginClient();
		bool sendLoginRC();

		// Packet functions.
		bool parsePacket(CString& pPacket);
		void decryptPacket(CString& pPacket);

		// Socket Variables
		CSocket *playerSock;
		CString rBuffer;

		// Encryption
		unsigned char key;
		codec in_codec;

		// Variables
		CString version, os;
		int codepage;
		TLevel *level;
		int id, type, versionID;
		time_t lastData, lastMovement, lastChat, lastNick, lastMessage, lastSave;
		std::vector<SCachedLevel*> cachedLevels;
		bool allowBomb;
		TMap* pmap;
		int carryNpcId;
		bool carryNpcThrown;
		CString guild;
		bool loaded;
		bool nextIsRaw;
		int rawPacketSize;
		bool isFtp;

		// File queue.
		CFileQueue fileQueue;
};

inline bool TPlayer::isLoggedIn() const
{
	return (type != PLTYPE_AWAIT && id > 0);
}

inline int TPlayer::getId() const
{
	return id;
}

inline int TPlayer::getType() const
{
	return type;
}

inline void TPlayer::setId(int pId)
{
	id = pId;
}

// Packet-Functions
typedef bool (TPlayer::*TPLSock)(CString&);
void createPLFunctions();

#endif // TPLAYER_H
