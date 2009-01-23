#include "ICommon.h"
#include "IUtil.h"
#include "CSocket.h"
#include "CSettings.h"
#include "CLog.h"
#include "TServerList.h"
#include "TPlayer.h"

/*
	Pointer-Functions for Packets
*/
std::vector<TSLSock> TSLFunc;

void createSLFunctions()
{
	// kinda like a memset-ish thing y'know
	for (int i = 0; i < 101; i++)
		TSLFunc.push_back(&TServerList::msgSVI_NULL);

	// now set non-nulls
	TSLFunc[SVI_VERIACC] = &TServerList::msgSVI_VERIACC;
	TSLFunc[SVI_VERIGUILD] = &TServerList::msgSVI_VERIGUILD;
	TSLFunc[SVI_FILESTART] = &TServerList::msgSVI_FILESTART;
	TSLFunc[SVI_FILEDATA] = &TServerList::msgSVI_FILEDATA;
	TSLFunc[SVI_FILEEND] = &TServerList::msgSVI_FILEEND;
	TSLFunc[SVI_VERSIONOLD] = &TServerList::msgSVI_VERSIONOLD;
	TSLFunc[SVI_VERSIONCURRENT] = &TServerList::msgSVI_VERSIONCURRENT;
	TSLFunc[SVI_PROFILE] = &TServerList::msgSVI_PROFILE;
	TSLFunc[SVI_ERRMSG] = &TServerList::msgSVI_ERRMSG;
	TSLFunc[SVI_VERIACC2] = &TServerList::msgSVI_VERIACC2;
	TSLFunc[SVI_FILESTART2] = &TServerList::msgSVI_FILESTART2;
	TSLFunc[SVI_FILEDATA2] = &TServerList::msgSVI_FILEDATA2;
	TSLFunc[SVI_FILEEND2] = &TServerList::msgSVI_FILEEND2;
	TSLFunc[SVI_PING] = &TServerList::msgSVI_PING;
	TSLFunc[SVI_RAWDATA] = &TServerList::msgSVI_RAWDATA;
	TSLFunc[SVI_FILESTART3] = &TServerList::msgSVI_FILESTART3;
	TSLFunc[SVI_FILEDATA3] = &TServerList::msgSVI_FILEDATA3;
	TSLFunc[SVI_FILEEND3] = &TServerList::msgSVI_FILEEND3;
}

/*
	Constructor - Deconstructor
*/
TServerList::TServerList()
: isConnected(false), nextIsRaw(false), rawPacketSize(0)

{
	sock.setProtocol(SOCKET_PROTOCOL_TCP);
	sock.setType(SOCKET_TYPE_CLIENT);
	sock.setDescription("listserver");

	lastData = lastPing = lastTimer = time(0);
}

TServerList::~TServerList()
{
}

/*
	Socket-Control Functions
*/
bool TServerList::getConnected() const
{
	return isConnected;
}

bool TServerList::onRecv()
{
	// Grab the data from the socket and put it into our receive buffer.
	unsigned int size = 0;
	char* data = sock.getData(&size);
	if (size != 0)
		rBuffer.write(data, size);

	if (!main())
		connectServer();

	return true;
}

bool TServerList::onSend()
{
	sendCompress();
	return true;
}

bool TServerList::canSend()
{
	if (sBuffer.isEmpty()) return false;
	return true;
}

bool TServerList::main()
{
	if (!getConnected())
		return false;

	// definitions
	CString line;
	int lineEnd;

	// do we have enough data to parse?
	rBuffer.setRead(0);
	while (rBuffer.length() != 0)
	{
		// Read a packet.
		if (!nextIsRaw)
		{
			if ((lineEnd = rBuffer.find("\n")) == -1) break;
			line = rBuffer.readString("\n");
			rBuffer.removeI(0, line.length() + 1);	// +1 for \n
		}
		else
		{
			if (rBuffer.length() < rawPacketSize) break;

			// Read the packet in and remove the terminating \n.
			line = rBuffer.readChars(rawPacketSize);
			rBuffer.removeI(0, line.length());
			line.removeI(line.length() - 1, 1);
			nextIsRaw = false;
		}

		// Parse the packet.
		parsePacket(line);

		// update last data
		lastData = time(0);
	}

	return getConnected();
}

bool TServerList::doTimedEvents()
{
	lastTimer = time(0);

	// Send a ping every 30 seconds.
	if ((int)difftime(lastTimer, lastPing) >= 30)
	{
		lastPing = lastTimer;
		sendPacket(CString() >> (char)SVO_PING);
	}

	return true;
}

bool TServerList::init(const CString& pServerIp, const CString& pServerPort)
{
	// Initialize the socket.
	if (sock.init(pServerIp.text(), pServerPort.text()) != 0)
		return false;

	return true;
}

bool TServerList::connectServer()
{
	CSettings* settings = server->getSettings();

	if (isConnected == true)
		return true;

	// Connect to Server
	if (sock.connect() == 0)
		isConnected = true;
	else
		return false;

	server->getServerLog().out(":: %s - Connected.\n", sock.getDescription());

	// Set Some Stuff
	setName(settings->getStr("name"));
	setDesc(settings->getStr("description"));
	setUrl(settings->getStr("url"));
	setVersion(GSERVER_VERSION);
	setIp(settings->getStr("serverip", "AUTO"));
	CString localip = sock.getLocalIp();
	if (localip == "127.0.1.1" || localip == "127.0.0.1")
		server->getServerLog().out(CString() << "** [WARNING] Socket returned " << localip << " for its local ip!  Not sending local ip to serverlist.");
	else
		sendPacket(CString() >> (char)SVO_SETLOCALIP << sock.getLocalIp());
	setPort(settings->getStr("serverport", "14900"));

	// Send Players
	sendPlayers();

	// Return Connection-Status
	return getConnected();
}

void TServerList::sendPacket(CString& pPacket)
{
	// empty buffer?
	if (pPacket.isEmpty())
		return;

	// append '\n'
	if (pPacket[pPacket.length()-1] != '\n')
		pPacket.writeChar('\n');

	// append buffer
	sBuffer.write(pPacket);
}

/*
	Altering Player Information
*/
void TServerList::addPlayer(TPlayer *pPlayer)
{
	sendPacket(CString() >> (char)SVO_PLYRADD
		<< pPlayer->getProp(PLPROP_ACCOUNTNAME)
		<< pPlayer->getProp(PLPROP_NICKNAME)
		<< pPlayer->getProp(PLPROP_CURLEVEL)
		<< pPlayer->getProp(PLPROP_X)
		<< pPlayer->getProp(PLPROP_Y)
		<< pPlayer->getProp(PLPROP_ALIGNMENT)
		>> (char)pPlayer->getType());
}

void TServerList::remPlayer(const CString& pAccountName, int pType)
{
	sendPacket(CString() >> (char)SVO_PLYRREM >> (char)pType << pAccountName);
}

void TServerList::sendPlayers()
{
	// Definition
	CString playerPacket;
	int playerCount = 0;

	// Iterate Playerlist
	for (std::vector<TPlayer *>::iterator i = server->getPlayerList()->begin(); i != server->getPlayerList()->end();)
	{
		TPlayer *pPlayer = (TPlayer*)*i;
		if (pPlayer == 0)
			continue;

		// Add to Count
		playerCount++;

		// Write Player-Packet
		playerPacket.write(pPlayer->getProp(PLPROP_ACCOUNTNAME)
		<< pPlayer->getProp(PLPROP_NICKNAME)
		<< pPlayer->getProp(PLPROP_CURLEVEL)
		<< pPlayer->getProp(PLPROP_X)
		<< pPlayer->getProp(PLPROP_Y)
		<< pPlayer->getProp(PLPROP_ALIGNMENT));
	}

	// Write Playercount
	sendPacket(CString() >> (char)SVO_SETPLYR >> (char)playerCount << playerPacket);
}

/*
	Altering Server-Information
*/
void TServerList::setDesc(const CString& pServerDesc)
{
	sendPacket(CString() >> (char)SVO_SETDESC << pServerDesc);
}

void TServerList::setIp(const CString& pServerIp)
{
	sendPacket(CString() >> (char)SVO_SETIP << pServerIp);
}

void TServerList::setName(const CString& pServerName)
{
	bool uc = server->getSettings()->getBool("underconstruction", false);
	sendPacket(CString() >> (char)SVO_SETNAME << (uc ? "U " : "") << pServerName);
}

void TServerList::setPort(const CString& pServerPort)
{
	sendPacket(CString() >> (char)SVO_SETPORT << pServerPort);
}

void TServerList::setUrl(const CString& pServerUrl)
{
	sendPacket(CString() >> (char)SVO_SETURL << pServerUrl);
}

void TServerList::setVersion(const CString& pServerVersion)
{
	sendPacket(CString() >> (char)SVO_SETVERS << pServerVersion);
}

/*
	Packet-Functions
*/
void TServerList::parsePacket(CString& pPacket)
{
	// read id & packet
	unsigned char id = pPacket.readGUChar();

	// id exists?
	if (id >= (unsigned char)TSLFunc.size())
		return;

	// valid packet, call function
	(*this.*TSLFunc[id])(pPacket);
}

void TServerList::sendCompress()
{
	// empty buffer?
	if (sBuffer.isEmpty())
		return;

	// send buffer
	unsigned int size = sBuffer.length();
	sBuffer.removeI(0, sock.sendData(sBuffer.text(), &size));
}

void TServerList::msgSVI_NULL(CString& pPacket)
{
	pPacket.setRead(0);
	server->getServerLog().out("Unknown Serverlist Packet: %i (%s)\n", pPacket.readGUChar(), pPacket.text()+1);
}

void TServerList::msgSVI_VERIACC(CString& pPacket)
{
	server->getServerLog().out("** SVI_VERIACC is deprecated.  It should not be used.\n");
}

void TServerList::msgSVI_VERIGUILD(CString& pPacket)
{
	unsigned short playerID = pPacket.readGUShort();
	CString nickname = pPacket.readChars(pPacket.readGUChar());

	TPlayer* p = server->getPlayer(playerID);
	if (p)
	{
		// Create the prop packet.
		CString prop = CString() >> (char)PLPROP_NICKNAME >> (char)nickname.length() << nickname;

		// Assign the nickname to the player.
		p->setNick(nickname, true);
		p->sendPacket(CString() >> (char)PLO_PLAYERPROPS << prop);

		// Tell everybody else the new nickname.
		server->sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)playerID << prop, p);
	}
}

void TServerList::msgSVI_FILESTART(CString& pPacket)
{
	server->getServerLog().out("** SVI_FILESTART is deprecated.  It should not be used.\n");
}

void TServerList::msgSVI_FILEEND(CString& pPacket)
{
	server->getServerLog().out("** SVI_FILEEND is deprecated.  It should not be used.\n");
}

void TServerList::msgSVI_FILEDATA(CString& pPacket)
{
	server->getServerLog().out("** SVI_FILEDATA is deprecated.  It should not be used.\n");
}

void TServerList::msgSVI_VERSIONOLD(CString& pPacket)
{
	server->getServerLog().out(":: You are running an old version of the Graal Reborn gserver.\n"
		":: An updated version is available online.\n");
}

void TServerList::msgSVI_VERSIONCURRENT(CString& pPacket)
{
	// Don't bother telling them they are running the latest version.
}

void TServerList::msgSVI_PROFILE(CString& pPacket)
{
	TPlayer* p1 = server->getPlayer(pPacket.readGUShort());
	TPlayer* p2 = server->getPlayer(pPacket.readChars(pPacket.readGUChar()));
	if (p1 == 0 || p2 == 0) return;

	// Start the profile string.
	CString profile;
	profile << p2->getProp(PLPROP_ACCOUNTNAME) << pPacket.readString("");

	// Add the time to the profile string.
	int time = p2->getProp(PLPROP_ONLINESECS).readGInt();
	CString line = CString() << CString((int)time/3600) << " hrs "
		<< CString((int)(time/60)%60) << " mins "
		<< CString((int)time%60) << " secs";
	profile >> (char)line.length() << line;

	// Add all the specified variables to the profile string.
	CString profileVars = server->getSettings()->getStr("profilevars");
	if (profileVars.length() != 0)
	{
		std::vector<CString> vars = profileVars.tokenize(",");
		for (std::vector<CString>::iterator i = vars.begin(); i != vars.end(); ++i)
		{
			CString name = i->readString(":=").trim();
			CString val = i->readString("").trim();

			// Built-in values.
			if (val == "playerkills")
				val = CString((unsigned int)(p2->getProp(PLPROP_KILLSCOUNT).readGUInt()));
			else if (val == "playerdeaths")
				val = CString((unsigned int)(p2->getProp(PLPROP_DEATHSCOUNT).readGUInt()));
			else if (val == "playerfullhearts")
				val = CString((int)p2->getProp(PLPROP_MAXPOWER).readGUChar());
			else if (val == "playerrating")
			{
				int rating = p2->getProp(PLPROP_RATING).readGUInt();
				val = CString((int)((rating >> 9) & 0xFFF)) << "/" << CString((int)(rating & 0x1FF));
			}
			else if (val == "playerap")
				val = CString((int)p2->getProp(PLPROP_ALIGNMENT).readGChar());
			else if (val == "playerrupees")
				val = CString((int)p2->getProp(PLPROP_RUPEESCOUNT).readGUInt());
			else if (val == "playerswordpower")
			{
				char sp = p2->getProp(PLPROP_SWORDPOWER).readGChar();
				if (sp > 4) sp -= 30;
				val = CString((int)sp);
			}
			else if (val == "canspin")
				val = ((p2->getProp(PLPROP_STATUS).readGUChar() & PLSTATUS_HASSPIN) ? "true" : "false");
			else if (val == "playerhearts")
			{
				unsigned char power = p2->getProp(PLPROP_CURPOWER).readGUChar();
				val = CString((int)(power / 2));
				if (power % 2 == 1) val << ".5";
			}
			else if (val == "playerdarts")
				val = CString((int)p2->getProp(PLPROP_ARROWSCOUNT).readGUChar());
			else if (val == "playerbombs")
				val = CString((int)p2->getProp(PLPROP_BOMBSCOUNT).readGUChar());
			else if (val == "playermp")
				val = CString((int)p2->getProp(PLPROP_MAGICPOINTS).readGUChar());
			else if (val == "playershieldpower")
			{
				char sp = p2->getProp(PLPROP_SHIELDPOWER).readGChar();
				if (sp > 3) sp -= 10;
				val = CString((int)sp);
			}
			else if (val == "playerglovepower")
				val = CString((int)p2->getProp(PLPROP_GLOVEPOWER).readGUChar());
			else
			{
				// Flag values.
				CString flag = p2->getFlag(val);
				if (flag.length() != 0) val = flag;
			}

			// Add it to the profile now.
			profile >> (char)(name.length() + val.length() + 2) << name << ":=" << val;
		}
	}

	// Send the profiles.
	p1->sendPacket(CString() >> (char)PLO_PROFILE << profile);
}

void TServerList::msgSVI_ERRMSG(CString& pPacket)
{
	server->getServerLog().out("%s\n", pPacket.readString("").text());
}

void TServerList::msgSVI_VERIACC2(CString& pPacket)
{
	CString account = pPacket.readChars(pPacket.readGUChar());
	unsigned short id = pPacket.readGUShort();
	unsigned char type = pPacket.readGUChar();
	CString message = pPacket.readString("");

	// Get the player.
	TPlayer* player = server->getPlayer(id);
	if (player == 0) return;

	// Overwrite the player's account name with the one from the gserver.
	player->setAccountName(account);

	// If we did not get the success message, inform the client of his failure.
	if (message != "SUCCESS")
	{
		player->sendPacket(CString() >> (char)PLO_DISCMESSAGE << message);
		player->setId(0);	// Prevent saving of the account.
		player->disconnect();
		return;
	}

	// Send the player his account.  If it fails, disconnect him.
	if (player->sendLogin() == false)
	{
		player->sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Failed to send login information.");
		player->setId(0);	// Prevent saving of the account.
		player->disconnect();
	}
}

void TServerList::msgSVI_FILESTART2(CString& pPacket)
{
	server->getServerLog().out("** SVI_FILESTART2 is deprecated.  It should not be used.\n");
}

void TServerList::msgSVI_FILEDATA2(CString& pPacket)
{
	server->getServerLog().out("** SVI_FILEDATA2 is deprecated.  It should not be used.\n");
}

void TServerList::msgSVI_FILEEND2(CString& pPacket)
{
	server->getServerLog().out("** SVI_FILEEND2 is deprecated.  It should not be used.\n");
}

void TServerList::msgSVI_PING(CString& pPacket)
{
	// Sent every 60 seconds.  Do nothing.
}

void TServerList::msgSVI_RAWDATA(CString& pPacket)
{
	nextIsRaw = true;
	rawPacketSize = pPacket.readGInt();
}

void TServerList::msgSVI_FILESTART3(CString& pPacket)
{
	unsigned char pTy = pPacket.readGUChar();
	CString blank, filename = CString() << "world/global/";
	switch (pTy)
	{
		case SVF_HEAD:
			filename << "heads/";
			break;
		case SVF_BODY:
			filename << "bodies/";
			break;
		case SVF_SWORD:
			filename << "swords/";
			break;
		case SVF_SHIELD:
			filename << "shields/";
			break;
	}
	filename << pPacket.readChars(pPacket.readGUChar());
	CFileSystem::fixPathSeparators(&filename);
	blank.save(CString() << server->getServerPath() << filename);
	server->getFileSystem()->addFile(filename);
}

void TServerList::msgSVI_FILEDATA3(CString& pPacket)
{
	unsigned char pTy = pPacket.readGUChar();
	CString filename = server->getFileSystem()->find(pPacket.readChars(pPacket.readGUChar()));
	if (filename.length() == 0) return;
	CString filedata;
	filedata.load(filename);
	filedata << pPacket.readChars(pPacket.bytesLeft());	// Read the rest of the packet.
	filedata.save(filename);
}

void TServerList::msgSVI_FILEEND3(CString& pPacket)
{
	unsigned short pid = pPacket.readGUShort();
	unsigned char type = pPacket.readGUChar();
	unsigned char doCompress = pPacket.readGUChar();
	time_t modTime = pPacket.readGUInt5();
	unsigned int fileLength = pPacket.readGUInt5();
	CString shortName = pPacket.readString("");

	// If we have folder config enabled, we need to add the file to the appropriate
	// file system.
	bool foldersconfig = !server->getSettings()->getBool("nofoldersconfig", false);
	CFileSystem* fileSystem = 0;
	CString typeString;
	switch (type)
	{
		case SVF_HEAD:
			typeString = "heads/";
			if (foldersconfig) fileSystem = server->getFileSystem(FS_HEAD);
			break;
		case SVF_BODY:
			typeString = "bodies/";
			if (foldersconfig) fileSystem = server->getFileSystem(FS_BODY);
			break;
		case SVF_SWORD:
			typeString = "swords/";
			if (foldersconfig) fileSystem = server->getFileSystem(FS_SWORD);
			break;
		case SVF_SHIELD:
			typeString = "shields/";
			if (foldersconfig) fileSystem = server->getFileSystem(FS_SHIELD);
			break;
	}
	CString fileName = server->getFileSystem()->find(shortName);

	// Add the file to the filesystem.
	if (fileSystem)
		fileSystem->addFile(CString() << "world/global/" << typeString << shortName);

	// Uncompress the file if compressed.
	if (doCompress == 1)
	{
		CString fileData;
		fileData.load(fileName);
		fileData.zuncompressI(fileLength);
		fileData.save(fileName);
	}

	// Set the file mod time.
	if (server->getFileSystem()->setModTime(shortName, modTime) == false)
		server->getServerLog().out("** [WARNING] Could not set modification time on file %s\n", fileName.text());

	// Set the player props.
	TPlayer* p = server->getPlayer(pid);
	if (p)
	{
		switch (type)
		{
			case SVF_HEAD:
				p->setProps(CString() >> (char)PLPROP_HEADGIF >> (char)(shortName.length() + 100) << shortName, true, true);
				break;

			case SVF_BODY:
				p->setProps(CString() >> (char)PLPROP_BODYIMG >> (char)shortName.length() << shortName, true, true);
				break;

			case SVF_SWORD:
			{
				CString prop = p->getProp(PLPROP_SWORDPOWER);
				p->setProps(CString() >> (char)PLPROP_SWORDPOWER >> (char)prop.readGUChar() >> (char)shortName.length() << shortName, true, true);
				break;
			}

			case SVF_SHIELD:
			{
				CString prop = p->getProp(PLPROP_SHIELDPOWER);
				p->setProps(CString() >> (char)PLPROP_SHIELDPOWER >> (char)prop.readGUChar() >> (char)shortName.length() << shortName, true, true);
				break;
			}
		}
	}
}
