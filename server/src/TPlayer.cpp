#include <time.h>
#include <math.h>
#include <sys/stat.h>
#include "ICommon.h"
#include "CSocket.h"
#include "TServer.h"
#include "TPlayer.h"
#include "TAccount.h"
#include "codec.h"

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
	"arrow.wav", "arrowon.wav", "axe.wav", "bomb.wav", "chest.wav", "compudead.wav", "crush.wav", "dead.wav", "extra.wav", "fire.wav", "frog.wav", "frog2.wav", "goal.wav", "horse.wav", "horse2.wav", "item.wav", "item2.wav", "jump.wav", "lift.wav", "lift2.wav", "nextpage.wav", "put.wav", "sign.wav", "steps.wav", "steps2.wav", "stonemove.wav", "sword.wav", "swordon.wav", "thunder.wav", "water.wav",
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
	false, true,  true,  true,  false, false, // 42-47
	false, false, true,  false, false, true,  // 48-53
	false, false, false, false, false, false, // 54-59
	false, false, false, false, false, false, // 60-65
	false, false, false, false, false, false, // 66-71
	false, false, false, false, false, false, // 72-77
	true,  true,  true,  false, true, // 78-82
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
	false, true,  true,  true,  false, false, // 42-47
	false, false, true,  false, false, false, // 48-53
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
std::vector<TPLSock> TPLFunc;

void createPLFunctions()
{
	// kinda like a memset-ish thing y'know
	for (int i = 0; i < 200; i++)
		TPLFunc.push_back(&TPlayer::msgPLI_NULL);

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
	TPLFunc[PLI_NPCADD] = &TPlayer::msgPLI_NPCADD;
	TPLFunc[PLI_NPCDEL] = &TPlayer::msgPLI_NPCDEL;
	TPLFunc[PLI_WANTFILE] = &TPlayer::msgPLI_WANTFILE;
	TPLFunc[PLI_SHOWIMG] = &TPlayer::msgPLI_SHOWIMG;

	TPLFunc[PLI_HURTPLAYER] = &TPlayer::msgPLI_HURTPLAYER;
	TPLFunc[PLI_EXPLOSION] = &TPlayer::msgPLI_EXPLOSION;
	TPLFunc[PLI_PRIVATEMESSAGE] = &TPlayer::msgPLI_PRIVATEMESSAGE;
	TPLFunc[PLI_NPCWEAPONDEL] = &TPlayer::msgPLI_NPCWEAPONDEL;
	TPLFunc[PLI_LEVELWARPMOD] = &TPlayer::msgPLI_LEVELWARP;	// Shared with PLI_LEVELWARP

	TPLFunc[PLI_WEAPONADD] = &TPlayer::msgPLI_WEAPONADD;
	TPLFunc[PLI_ITEMTAKE] = &TPlayer::msgPLI_ITEMDEL;			// Shared with PLI_ITEMDEL
	TPLFunc[PLI_UPDATEFILE] = &TPlayer::msgPLI_UPDATEFILE;
	TPLFunc[PLI_ADJACENTLEVEL] = &TPlayer::msgPLI_ADJACENTLEVEL;
	TPLFunc[PLI_HITOBJECTS] = &TPlayer::msgPLI_HITOBJECTS;
	TPLFunc[PLI_LANGUAGE] = &TPlayer::msgPLI_LANGUAGE;
	TPLFunc[PLI_TRIGGERACTION] = &TPlayer::msgPLI_TRIGGERACTION;
	TPLFunc[PLI_MAPINFO] = &TPlayer::msgPLI_MAPINFO;
	TPLFunc[PLI_SHOOT] = &TPlayer::msgPLI_SHOOT;

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

	TPLFunc[PLI_NC_WEAPONLISTGET] = &TPlayer::msgPLI_NC_WEAPONLISTGET;
	TPLFunc[PLI_NC_WEAPONGET] = &TPlayer::msgPLI_NC_WEAPONGET;
	TPLFunc[PLI_NC_WEAPONADD] = &TPlayer::msgPLI_NC_WEAPONADD;
	TPLFunc[PLI_NC_WEAPONDELETE] = &TPlayer::msgPLI_NC_WEAPONDELETE;
}


/*
	Constructor - Deconstructor
*/
TPlayer::TPlayer(TServer* pServer, CSocket* pSocket, int pId)
: TAccount(pServer),
playerSock(pSocket), key(0),
os("wind"), codepage(1252), level(0),
id(pId), type(PLTYPE_AWAIT), versionID(CLVER_2_17), allowBomb(false),
pmap(0), carryNpcId(0), carryNpcThrown(false), loaded(false),
nextIsRaw(false), rawPacketSize(0), isFtp(false),
grMovementUpdated(false),
fileQueue(pSocket)
{
	lastData = lastMovement = lastChat = lastMessage = lastNick = lastSave = time(0);
	srand((unsigned int)time(0));
}

TPlayer::~TPlayer()
{
	// Send all unsent data (for disconnect messages and whatnot).
	fileQueue.sendCompress();

	if (id >= 0 && server != 0)
	{
		// Save account.
		if (isClient() && loaded && !isLoadOnly)
			saveAccount();

		// Remove from the level.
		if (level) leaveLevel();

		// Announce our departure to other clients.
		server->sendPacketTo(PLTYPE_ANYCLIENT, CString() >> (char)PLO_OTHERPLPROPS >> (short)id >> (char)PLPROP_PCONNECTED, this);
		server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_DELPLAYER >> (short)id, this);
	}

	// Clean up.
	for (std::vector<SCachedLevel*>::iterator i = cachedLevels.begin(); i != cachedLevels.end(); )
	{
		SCachedLevel* cl = *i;
		delete cl;
		i = cachedLevels.erase(i);
	}

	if (playerSock)
		delete playerSock;
}

bool TPlayer::onRecv()
{
	if (playerSock == 0)
		return false;
	if (playerSock->getState() == SOCKET_STATE_DISCONNECTED)
		return false;

	// Grab the data from the socket and put it into our receive buffer.
	unsigned int size = 0;
	char* data = playerSock->getData(&size);
	if (size != 0)
		rBuffer.write(data, size);
	else if (playerSock->getState() == SOCKET_STATE_DISCONNECTED)
	{
		server->deletePlayer(this);
		return false;
	}

	// Do the main function.
	if (doMain() == false)
	{
		server->deletePlayer(this);
		return false;
	}

	return true;
}

bool TPlayer::onSend()
{
	if (playerSock == 0)
		return false;
	if (playerSock->getState() == SOCKET_STATE_DISCONNECTED)
		return false;

	// Send data.
	fileQueue.sendCompress();

	return true;
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

	// Only run for clients.
	if (!isClient()) return true;

	// Increase online time.
	onlineTime++;

	// Disconnect if players are inactive.
	CSettings* settings = server->getSettings();
	if (settings->getBool("disconnectifnotmoved"))
	{
		if ((int)difftime(currTime, lastMovement) > settings->getInt("maxnomovement", 1200))
		{
			serverlog.out("Client %s has been disconnected due to inactivity.\n", accountName.text());
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You have been disconnected due to inactivity.");
			return false;
		}
	}

	// Disconnect if no data has been received in 5 minutes.
	if ((int)difftime(currTime, lastData) > 300)
	{
		serverlog.out("Client %s has timed out.\n", accountName.text());
		return false;
	}

	// Increase player AP.
	if (settings->getBool("apsystem") && level != 0)
	{
		if (!(status & PLSTATUS_PAUSED) && level->getSparringZone() == false)
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
}

bool TPlayer::parsePacket(CString& pPacket)
{
	// First packet is always unencrypted zlib.  Read it in a special way.
	if (type == PLTYPE_AWAIT)
	{
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

			if (isClient())
			{
				if (curPacket[curPacket.length() - 1] == '\n')
					curPacket.removeI(curPacket.length() - 1);
			}
			else if (isRC() && versionID > RCVER_1_1)
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
		int id = curPacket.readGUChar();

		// Check if it is a valid packet id.
		if (id >= (unsigned char)TPLFunc.size())
			return false;

		// Call the function assigned to the packet id.
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
	}
}

void TPlayer::sendPacket(CString pPacket)
{
	// empty buffer?
	if (pPacket.isEmpty())
		return;

	// append '\n'
	if (pPacket[pPacket.length()-1] != '\n')
		pPacket.writeChar('\n');

	// append buffer
	fileQueue.addPacket(pPacket);
}

bool TPlayer::sendFile(const CString& pFile)
{
	CFileSystem* fileSystem = server->getFileSystem();

	// Find file.
	CString path = fileSystem->find(pFile);
	if (path.isEmpty()) return false;

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
		if (isClient() && versionID < CLVER_2_14) sendSize = fileData.length();
		sendPacket(CString() >> (char)PLO_RAWDATA >> (int)(packetLength + sendSize));
		sendPacket(CString() >> (char)PLO_FILE >> (long long)modTime >> (char)pFile.length() << pFile << fileData.subString(0, sendSize) << "\n");
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

			TPlayer* player = server->getPlayer(chatParse[1]);
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

			setProps(CString() >> (char)PLPROP_X >> (char)(strtoint(chatParse[1]) * 2) >> (char)PLPROP_Y >> (char)(strtoint(chatParse[2]) * 2), true, true);
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

		TPlayer* p = server->getPlayer(chatParse[1]);
		if (p) p->warp(levelName, x, y);
	}
	else if (chatParse[0] == "unstick" || chatParse[0] == "unstuck")
	{
		processed = true;

		if (chatParse.size() == 2 && chatParse[1] == "me")
		{
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

bool TPlayer::isStaff()
{
	CString staff(server->getSettings()->getStr("staff"));
	std::vector<CString> staffList = staff.tokenize(",");
	for (std::vector<CString>::iterator i = staffList.begin(); i != staffList.end(); ++i)
	{
		if (accountName.toLower() == (*i).trim().toLower()) return true;
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

	// Add myself to the level playerlist.
	level->addPlayer(this);
	levelName = level->getLevelName();

	// Tell the client their new level.
	if (modTime == 0 || versionID < CLVER_2_1)
	{
		if (pmap && pmap->getType() == MAPTYPE_GMAP)
		{
			gmaplevelx = pmap->getLevelX(levelName);
			gmaplevely = pmap->getLevelY(levelName);
			sendPacket(CString() >> (char)PLO_PLAYERWARP2
				>> (char)(x * 2) >> (char)(y * 2) >> (char)((z * 2) + 50)
				>> (char)gmaplevelx >> (char)gmaplevely
				<< pmap->getMapName());
		}
		else
			sendPacket(CString() >> (char)PLO_PLAYERWARP >> (char)(x * 2) >> (char)(y * 2) << levelName);
	}
	if (sendLevel(level, modTime, false) == false)
	{
		sendPacket(CString() >> (char)PLO_WARPFAILED << pLevelName);
		return false;
	}

	// If the level is a sparring zone and you have 100 AP, change AP to 99 and
	// the apcounter to 1.
	if (level->getSparringZone() && ap == 100)
	{
		ap = 99;
		apCounter = 1;
		setProps(CString() >> (char)PLPROP_ALIGNMENT >> (char)ap, true, true);
	}

	// Inform everybody as to the client's new location.  This will update the minimap.
	server->sendPacketToAll(this->getProps(0,0) >> (char)PLPROP_CURLEVEL << this->getProp(PLPROP_CURLEVEL) >> (char)PLPROP_X << this->getProp(PLPROP_X) >> (char)PLPROP_Y << this->getProp(PLPROP_Y), this);

	return true;
}

bool TPlayer::sendLevel(TLevel* pLevel, time_t modTime, bool skipActors)
{
	if (pLevel == 0) return false;
	CSettings* settings = server->getSettings();

	// Send Level
	sendPacket(CString() >> (char)PLO_LEVELNAME << pLevel->getLevelName());
	time_t l_time = getCachedLevelModTime(pLevel);
	if (modTime == -1) modTime = pLevel->getModTime();
	if (l_time == 0 || versionID < CLVER_2_1)
	{
		if (modTime != pLevel->getModTime() || versionID < CLVER_2_1)
		{
			sendPacket(CString() >> (char)PLO_RAWDATA >> (int)(1+(64*64*2)+1));
			sendPacket(CString() << pLevel->getBoardPacket());
		}

		// Send links, signs, and mod time.
		sendPacket(CString() >> (char)PLO_LEVELMODTIME >> (long long)pLevel->getModTime());
		if (settings->getBool("serverside", false) == false)	// TODO: NPC server check instead.
		{
			sendPacket(CString() << pLevel->getLinksPacket());
			sendPacket(CString() << pLevel->getSignsPacket());
		}
	}

	// Send board changes, chests, horses, and baddies.
	sendPacket(CString() << pLevel->getBoardChangesPacket(l_time));
	if (skipActors == false)
	{
		sendPacket(CString() << pLevel->getChestPacket(this));
		sendPacket(CString() << pLevel->getHorsePacket());
		sendPacket(CString() << pLevel->getBaddyPacket());
	}

	// If we are on a gmap, change our level back to the gmap.
	if (pmap && pmap->getType() == MAPTYPE_GMAP)
		sendPacket(CString() >> (char)PLO_LEVELNAME << pmap->getMapName());

	// Tell the client if there are any ghost players in the level.
	// Graal Reborn doesn't support trial accounts so pass 0 (no ghosts) instead of 1 (ghosts present).
	//sendPacket(CString() >> (char)PLO_GHOSTICON >> (char)0);

	if (skipActors == false)
	{
		// If we are the leader, send it now.
		if (pLevel->getPlayer(0) == this)
			sendPacket(CString() >> (char)PLO_ISLEADER);
	}

	// Send new world time.
	sendPacket(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(server->getNWTime()));

	// NPCs like to cause 1.41 to crash for some reason.
	//if (versionID < CLVER_2_1) skipActors = true;

	if (skipActors == false)
	{
		// Send NPCs.
		if (pmap && pmap->getType() == MAPTYPE_GMAP)
			sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << pmap->getMapName());
		else if (versionID > CLVER_1_411)
			sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << pLevel->getLevelName());
		sendPacket(CString() << pLevel->getNpcsPacket(l_time, versionID));
	}

	// Do props stuff.
	// Maps send to players in adjacent levels too.
	if (pmap)
	{
		server->sendPacketToLevel(this->getProps(__getLogin, sizeof(__getLogin)/sizeof(bool)), pmap, this, false);
		std::vector<TPlayer*>* playerList = server->getPlayerList();
		for (std::vector<TPlayer*>::iterator i = playerList->begin(); i != playerList->end(); ++i)
		{
			TPlayer* player = (TPlayer*)*i;
			if (player == this || player->getMap() != pmap) continue;

			if (pmap->getType() == MAPTYPE_GMAP)
			{
				int ogmap[2] = {player->getProp(PLPROP_GMAPLEVELX).readGUChar(), player->getProp(PLPROP_GMAPLEVELY).readGUChar()};
				if (abs(ogmap[0] - gmaplevelx) < 2 && abs(ogmap[1] - gmaplevely) < 2)
					this->sendPacket(player->getProps(__getLogin, sizeof(__getLogin)/sizeof(bool)));
			}
			else if (pmap->getType() == MAPTYPE_BIGMAP)
			{
				if (player->getLevel() == 0) continue;
				int ogmap[2] = {pmap->getLevelX(player->getLevel()->getLevelName()), pmap->getLevelY(player->getLevel()->getLevelName())};
				int sgmap[2] = {pmap->getLevelX(pLevel->getLevelName()), pmap->getLevelY(pLevel->getLevelName())};
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
	// 1.41r1 and below don't cache things (probably).
	if (versionID < CLVER_2_1) return 0;
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

CString TPlayer::getFlag(const CString& flag) const
{
	for (std::vector<CString>::const_iterator i = flagList.begin(); i != flagList.end(); ++i)
	{
		CString val = *i;
		CString name = val.readString("=").trim();
		if (name == flag)
			return val.readString("").trim();
	}
	return CString();
}

void TPlayer::setChat(const CString& pChat)
{
	setProps(CString() >> (char)PLPROP_CURCHAT >> (char)pChat.length() << pChat, true, true);
}

void TPlayer::setNick(const CString& pNickName, bool force)
{
	CString nickname = pNickName;
	if (force)
	{
		nickName = pNickName;
		CString nick = nickname.readString("(").trim();
		CString guild = nickname.readString(")");
		this->guild = guild;
		return;
	}

	CString newNick;
	CString nick = nickname.readString("(").trim();
	CString guild = nickname.readString(")");
//	CString guild = CString("(") << nickname.readString(")") << ")";

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
	if (guild.length() != 0)
	{
		// Read the guild list.
		CFileSystem guildFS(server);
		guildFS.addDir("guilds");
		CString guildList = guildFS.load(CString() << "guild" << guild << ".txt");

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

		// See if it is a global guild.
		server->getServerList()->sendPacket(
			CString() >> (char)SVO_VERIGUILD >> (short)id
			>> (char)accountName.length() << accountName
			>> (char)newNick.length() << newNick
			>> (char)guild.length() << guild
			);
	}
	else
	{
		// Save it.
		nickName = newNick;
	}
}

bool TPlayer::addWeapon(int defaultWeapon)
{
	CSettings* settings = server->getSettings();
	std::vector<TWeapon*>* sWeaponList = server->getWeaponList();

	if (settings->getBool("defaultweapons", true) == false)
		return false;

	if (defaultWeapon == 8 && allowBomb == false)
	{
		allowBomb = true;
		return false;
	}

	TWeapon* weapon = server->getWeapon(TLevelItem::getItemName(defaultWeapon));
	if (weapon == 0)
	{
		weapon = new TWeapon(defaultWeapon);
		sWeaponList->push_back(weapon);
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
	if (weapon == 0) return false;

	// See if the player already has the weapon.
	if (vecSearch<CString>(weaponList, weapon->getName()) == -1)
	{
		weaponList.push_back(weapon->getName());
		sendPacket(CString() << weapon->getWeaponPacket());
	}

	return true;
}

bool TPlayer::addWeapon(TWeapon* weapon)
{
	if (weapon == 0) return false;

	// See if the player already has the weapon.
	if (vecSearch<CString>(weaponList, weapon->getName()) == -1)
	{
		weaponList.push_back(weapon->getName());
		sendPacket(CString() << weapon->getWeaponPacket());
	}

	return true;
}


/*
	TPlayer: Packet functions
*/
bool TPlayer::msgPLI_NULL(CString& pPacket)
{
	pPacket.setRead(0);
	printf("Unknown Player Packet: %i (%s)\n", pPacket.readGUChar(), pPacket.text()+1);
	for (int i = 0; i < pPacket.length(); ++i) printf("%02x ", (unsigned char)((pPacket.text())[i])); printf("\n");
	return true;
}

bool TPlayer::msgPLI_LOGIN(CString& pPacket)
{
	// Read Player-Ip
	accountIpStr = playerSock->getRemoteIp();
	accountIp = inet_addr(accountIpStr.text());

	// Read Client-Type
	serverlog.out(":: New login:\t");
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
		case PLTYPE_NC:
			serverlog.out("NC\n");
			in_codec.setGen(ENCRYPT_GEN_3);
			getKey = false;
			break;
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
	if (isClient()) versionID = getVersionID(version);
	else if (isRC()) versionID = getRCVersionID(version);
	else versionID = 0;

	// Read Account & Password
	accountName = pPacket.readChars(pPacket.readGUChar());
	CString password = pPacket.readChars(pPacket.readGUChar());

	//serverlog.out("Key: %d\n", key);
	serverlog.out("   Version:\t%s\n", version.text());
	serverlog.out("   Account:\t%s\n", accountName.text());
	//serverlog.out("Password: %s\n", password.text());

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
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Your client version is not allowed on this server.");
			return false;
		}
	}

	// Verify login details with the serverlist.
	// TODO: localhost mode.
	if (!isNC())
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
		server->sendPacketToLevel(CString() >> (char)PLO_BOARDMODIFY << pPacket.text() + 1, 0, level);

	if (loc[0] < 0 || loc[0] > 63 || loc[1] < 0 || loc[1] > 63) return true;

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

	CString packet = CString() >> (char)PLO_NPCPROPS << pPacket.text() + 1;
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

	for (int i = 0; i < pPacket.length(); ++i) printf( "%02x ", (unsigned char)pPacket[i] ); printf( "\n" );
	server->sendPacketToLevel(CString() >> (char)PLO_BOMBADD >> (short)id << pPacket.text() + 1, 0, level, this);
	return true;
}

bool TPlayer::msgPLI_BOMBDEL(CString& pPacket)
{
	server->sendPacketToLevel(CString() >> (char)PLO_BOMBDEL << pPacket.text() + 1, 0, level, this);
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
	server->sendPacketToLevel(CString() >> (char)PLO_HORSEADD << pPacket.text() + 1, 0, level, this);

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
	server->sendPacketToLevel(CString() >> (char)PLO_HORSEDEL << pPacket.text() + 1, 0, level, this);

	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};

	level->removeHorse(loc[0], loc[1]);
	return true;
}

bool TPlayer::msgPLI_ARROWADD(CString& pPacket)
{
	server->sendPacketToLevel(CString() >> (char)PLO_ARROWADD >> (short)id << pPacket.text() + 1, 0, level, this);
	return true;
}

bool TPlayer::msgPLI_FIRESPY(CString& pPacket)
{
	server->sendPacketToLevel(CString() >> (char)PLO_FIRESPY >> (short)id << pPacket.text() + 1, 0, level, this);
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
	server->sendPacketToLevel(CString() >> (char)PLO_THROWCARRIED >> (short)id << pPacket.text() + 1, 0, level, this);
	return true;
}

bool TPlayer::msgPLI_ITEMADD(CString& pPacket)
{
	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};
	unsigned char item = pPacket.readGUChar();

	level->addItem(loc[0], loc[1], item);
	server->sendPacketToLevel(CString() >> (char)PLO_ITEMADD << pPacket.text() + 1, 0, level, this);
	return true;
}

bool TPlayer::msgPLI_ITEMDEL(CString& pPacket)
{
	server->sendPacketToLevel(CString() >> (char)PLO_ITEMDEL << pPacket.text() + 1, 0, level, this);

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
	if (level->getSparringZone())
	{
		// Get some stats we are going to use.
		// Need to parse the other player's PLPROP_RATING.
		unsigned int otherRating = player->getProp(PLPROP_RATING).readGUInt();
		float oldStats[4] = { rating, deviation, (float)((otherRating >> 9) & 0xFFF), (float)(otherRating & 0x1FF) };

		// If the IPs are the same, don't update the rating to prevent cheating.
		if (this->getProp(PLPROP_IPADDR).readGInt5() == player->getProp(PLPROP_IPADDR).readGInt5()) return true;

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
			this->setProps(CString() >> (char)PLPROP_RATING >> (int)0, true);
		}
		if (oldStats[2] != tWinRating || oldStats[3] != tWinDeviation)
		{
			player->setRating((int)tWinRating, (int)tWinDeviation);
			player->setProps(CString() >> (char)PLPROP_RATING >> (int)0, true);
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
	leader->sendPacket(CString() >> (char)PLO_BADDYHURT << pPacket.text() + 1);
	return true;
}

bool TPlayer::msgPLI_BADDYADD(CString& pPacket)
{
	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};
	unsigned char bType = pPacket.readGUChar();
	CString bImage = pPacket.readChars(pPacket.bytesLeft() - 1);
	unsigned char bPower = pPacket.readGUChar();
	bPower = MIN(bPower, 12);		// Hard-limit to 6 hearts.

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
	CString flagPacket = pPacket.readString("");
	CString flagName, flagValue, flagNew;
	if (flagPacket.find("=") != -1)
	{
		flagName = flagPacket.readString("=").trim();
		flagValue = flagPacket.readString("").trim();
		flagNew = CString() << flagName << "=" << flagValue;
	}
	else
	{
		flagName = flagPacket;
		flagNew = flagPacket;
	}

	// Add a little hack for our special gr.strings.
	if (flagName.find("gr.") != -1)
	{
		// gr.x and gr.y are used by the -gr_movement NPC to help facilitate smoother
		// movement amongst pre-2.3 clients.
		if (flagName == "gr.x")
		{
			if (versionID >= CLVER_2_3) return true;
			float pos = (float)atof(flagValue.text());
			if (pos != x)
			{
				grMovementPackets >> (char)PLPROP_X >> (char)(pos * 2.0f) << "\n";
//				x = pos;
//				setProps(CString() >> (char)PLPROP_X >> (char)(x * 2.0f), true, false);
			}
			return true;
		}
		else if (flagName == "gr.y")
		{
			if (versionID >= CLVER_2_3) return true;
			float pos = (float)atof(flagValue.text());
			if (pos != y)
			{
				//printf("gr.y: %.2f, adding\n", pos);
				grMovementPackets >> (char)PLPROP_Y >> (char)(pos * 2.0f) << "\n";
//				y = pos;
//				setProps(CString() >> (char)PLPROP_Y >> (char)(y * 2.0f), true, false);
			}
			return true;
		}
		else if (flagName == "gr.z")
		{
			if (versionID >= CLVER_2_3) return true;
			float pos = (float)atof(flagValue.text());
			if (pos != z)
			{
				grMovementPackets >> (char)PLPROP_Z >> (char)((pos + 25.0f) * 2.0f) << "\n";
//				z = pos;
//				setProps(CString() >> (char)PLPROP_Z >> (char)((z + 25.0f) * 2.0f), true, false);
			}
			return true;
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
		server->addFlag(flagNew);
		return true;
	}

	// Loop for flags now.
	for (std::vector<CString>::iterator i = flagList.begin(); i != flagList.end(); ++i)
	{
		CString tflagName = *i;
		if (i->find("=") != -1)
			tflagName.removeI(i->find("="));

		if (tflagName == flagName)
		{
			// A flag with a value of 0 means we should unset it.
			if (flagValue.length() == 0)
			{
				flagList.erase(i);
				return true;
			}

			// If we didn't unset it, alter the existing flag.
			*i = flagNew;
			return true;
		}
	}

	// We didn't find a pre-existing flag so let's create a new one.
	flagList.push_back(flagNew);
	return true;
}

bool TPlayer::msgPLI_FLAGDEL(CString& pPacket)
{
	CSettings* settings = server->getSettings();

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

	// Loop for flags now.
	for (std::vector<CString>::iterator i = flagList.begin(); i != flagList.end(); ++i)
	{
		CString tflagName = *i;
		if (i->find("=") != -1)
			tflagName.removeI(i->find("="));

		if (tflagName == flagName)
		{
			flagList.erase(i);
			return true;
		}
	}

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

bool TPlayer::msgPLI_NPCADD(CString& pPacket)
{
	CSettings* settings = server->getSettings();

	CString nimage = pPacket.readChars(pPacket.readGUChar());
	CString ncode = pPacket.readChars(pPacket.readGUChar());
	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};

	// See if putnpc is allowed.
	if (settings->getBool("putnpcenabled") == false)
		return true;

	// Add NPC to level
	TNPC* npc = server->addNPC(nimage, ncode, loc[0], loc[1], level, false);

	return true;
}

bool TPlayer::msgPLI_NPCDEL(CString& pPacket)
{
	unsigned int nid = pPacket.readGUInt();

	// Remove the NPC.
	server->deleteNPC(nid, level);
	return true;
}

bool TPlayer::msgPLI_WANTFILE(CString& pPacket)
{
	CFileSystem* fileSystem = server->getFileSystem();

	// Get file.
	CString file = pPacket.readString("");

	// Send file.
	this->sendFile(file);
	return true;
}

bool TPlayer::msgPLI_SHOWIMG(CString& pPacket)
{
	server->sendPacketToLevel(CString() >> (char)PLO_SHOWIMG >> (short)id << pPacket.text() + 1, pmap, level, this);
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
	int sendLimit = 4;
	if ((int)difftime(time(0), lastMessage) <= 4)
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
	pmMessage.guntokenizeI();
	int filter = server->getWordFilter()->apply(this, pmMessage, FILTER_CHECK_PM);
	if (filter & FILTER_ACTION_WARN)
	{
		sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE <<
			"Word Filter:\xa7Your PM could not be sent because it was caught by the word filter.");
		return true;
	}
	pmMessage.gtokenizeI();

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
			i = weaponList.erase(i);
		else ++i;
	}
	return true;
}

bool TPlayer::msgPLI_WEAPONADD(CString& pPacket)
{
	CSettings* settings = server->getSettings();
	unsigned char type = pPacket.readGUChar();

	// Type 0 means it is a default weapon.
	if (type == 0)
	{
		this->addWeapon(pPacket.readGChar());
	}
	// NPC weapons.
	else
	{
		// TODO: If NPC-Server is running, don't allow any of this.

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
		TWeapon* weapon = server->getWeapon(name);

		// If weapon is 0, that means the NPC was not found.  Add the NPC to the list.
		bool newWeapon = false;
		std::vector<TWeapon*>* sweaponList = server->getWeaponList();
		if (weapon == 0)
		{
			newWeapon = true;
			weapon = new TWeapon(name, npc->getProp(NPCPROP_IMAGE).subString(1), npc->getProp(NPCPROP_SCRIPT).subString(2), npc->getPropModTime(NPCPROP_SCRIPT));
			sweaponList->push_back(weapon);
		}

		// Check and see if the weapon has changed recently.  If it has, we should
		// send the new NPC to everybody on the server.  After updating the script, of course.
		bool foundThis = false;
		if (weapon->getModTime() < npc->getPropModTime(NPCPROP_SCRIPT))
		{
			newWeapon = true;		// Lets the new code get saved.
			weapon->setClientScript(npc->getClientScript());
			weapon->setModTime(npc->getPropModTime(NPCPROP_SCRIPT));
			std::vector<TPlayer*>* playerList = server->getPlayerList();
			for (std::vector<TPlayer*>::iterator i = playerList->begin(); i != playerList->end(); ++i)
			{
				TPlayer* player = *i;
				if (player->hasWeapon(weapon->getName()))
				{
					if (player == this) foundThis = true;
					player->sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << weapon->getName());
					player->sendPacket(CString() << weapon->getWeaponPacket());
				}
			}
		}

		// Send the weapon to the player now.
		if (foundThis == false)
			this->addWeapon(weapon);

		// Save weapon.
		if (newWeapon)
			weapon->saveWeapon(server);
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
	if (isDefault == false && fModTime > modTime)
		return msgPLI_WANTFILE(file);

	sendPacket(CString() >> (char)PLO_FILEUPTODATE << file);
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
	sendLevel(adjacentLevel, modTime, (pmap ? false : true));

	// Set our old level back to normal.
	sendPacket(CString() >> (char)PLO_LEVELNAME << level->getLevelName());
	if (pmap && pmap->getType() == MAPTYPE_GMAP)
		sendPacket(CString() >> (char)PLO_LEVELNAME << pmap->getMapName());
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
	return true;
}

bool TPlayer::msgPLI_TRIGGERACTION(CString& pPacket)
{
	unsigned int npcId = pPacket.readGUInt();
	float loc[2] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};
	CString action = pPacket.readString("");

	if (loc[0] == 0.0f && loc[1] == 0.0f)
	{
		if (action.find("gr.addweapon") == 0)
		{
			std::vector<CString> actionParts = action.tokenize(",");
			if (actionParts.size() == 2)
				this->addWeapon(actionParts[1]);
		}
	}

	// We don't have an NPCserver, so, for now, just pass it along.
	CString packet;
	packet >> (char)PLO_TRIGGERACTION >> (short)id << pPacket.text() + 1;
	server->sendPacketToLevel(packet, 0, level, this);
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
	server->sendPacketToLevel(CString() >> (char)PLO_SHOOT >> (short)id << pPacket.text() + 1, pmap, this, false);

//	printf("shoot: %s\n", pPacket.text());
//	for (int i = 0; i < pPacket.length(); ++i) printf("%02x ", (unsigned char)pPacket[i]); printf("\n");
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

bool TPlayer::msgPLI_NPCSERVERQUERY(CString& pPacket)
{
	unsigned short pid = pPacket.readGUShort();
	CString message = pPacket.readString("");
	CSettings* settings = server->getSettings();

	// TODO: Check if player is the NPC Server.

	// Enact upon the message.
	if (message == "location")
	{
		sendPacket(CString() >> (char)PLO_NPCSERVERADDR >> (short)pid << settings->getStr("serverip") << "," << settings->getStr("serverport"));
	}
	return true;
}
