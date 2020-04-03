#include "IDebug.h"
#include "IConfig.h"

#include "CCommon.h"
#include "TServerList.h"
#include "IEnums.h"
#include "TServer.h"
#include "IUtil.h"
#include "TPlayer.h"

/*
	Pointer-Functions for Packets
*/
bool TServerList::created = false;
typedef void (TServerList::*TSLSock)(CString&);
std::vector<TSLSock> TSLFunc(256, &TServerList::msgSVI_NULL);

void TServerList::createFunctions()
{
	if (TServerList::created)
		return;

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
	TSLFunc[SVI_SERVERINFO] = &TServerList::msgSVI_SERVERINFO;
	TSLFunc[SVI_REQUESTTEXT] = &TServerList::msgSVI_REQUESTTEXT;
	TSLFunc[SVI_SENDTEXT] = &TServerList::msgSVI_SENDTEXT;
	TSLFunc[SVI_PMPLAYER] = &TServerList::msgSVI_PMPLAYER;

	// Finished
	TServerList::created = true;
}

/*
	Constructor - Deconstructor
*/
TServerList::TServerList(TServer *server)
: _server(server), _fileQueue(&sock), nextIsRaw(false), rawPacketSize(0)
{
	sock.setProtocol(SOCKET_PROTOCOL_TCP);
	sock.setType(SOCKET_TYPE_CLIENT);
	sock.setDescription("listserver");

	lastData = lastPing = lastTimer = lastPlayerSync = time(0);

	// Create Functions
	if (!TServerList::created)
		TServerList::createFunctions();
}

TServerList::~TServerList()
{
}

/*
	Socket-Control Functions
*/
bool TServerList::getConnected() const
{
	return (sock.getState() == SOCKET_STATE_CONNECTED);
}

bool TServerList::onRecv()
{
	// Grab the data from the socket and put it into our receive buffer.
	unsigned int size = 0;
	char* data = sock.getData(&size);
	if (size != 0)
		rBuffer.write(data, size);
	else if (sock.getState() == SOCKET_STATE_DISCONNECTED)
		return false;

	main();

	return true;
}

bool TServerList::onSend()
{
	_fileQueue.sendCompress();
	return true;
}

bool TServerList::canRecv()
{
	if (sock.getState() == SOCKET_STATE_DISCONNECTED) return false;
	return true;
}

bool TServerList::main()
{
	if (!getConnected())
		return false;

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
		if ((unsigned int)len > (unsigned int)rBuffer.length() - 2)
			break;

		// decompress packet
		unBuffer = rBuffer.readChars(len);
		rBuffer.removeI(0, len + 2);
		unBuffer.zuncompressI();

		// well theres your buffer
		if (!parsePacket(unBuffer))
			return false;
	}

	_server->getSocketManager()->updateSingle(this, false, true);

	return getConnected();
}

bool TServerList::doTimedEvents()
{
	lastTimer = time(0);

	bool isConnected = getConnected();

	if (!isConnected)
		connectServer();

	// Send a ping every 30 seconds.
	/*
	if ((int)difftime(lastTimer, lastPing) >= 30 && isConnected)
	{
		lastPing = lastTimer;

		sendPacket(CString() >> (char)SVO_PING);

		std::vector<TPlayer *> playerListTmp = *_server->getPlayerList();
		for (std::vector<TPlayer *>::const_iterator i = playerListTmp.begin(); i != playerListTmp.end(); ++i)
		{
			std::vector<CString> pmServers = (*i)->getPMServerList();

			if (!pmServers.empty())
			{
				for (std::vector<CString>::const_iterator ij = pmServers.begin(); ij != pmServers.end(); ++ij)
				{
					sendPacket(CString() >> (char)SVO_REQUESTLIST >> (short)(*i)->getId() << CString(CString() << (*i)->getAccountName() << "\n" << "GraalEngine" << "\n" << "pmserverplayers" << "\n" << (ij)->text() << "\n").gtokenizeI());
				}
			}
		}
	}
	*/

	// Synchronize players every minute.
	/*
	if ((int)difftime(lastTimer, lastPlayerSync) >= 60)
	{
		lastPlayerSync = lastTimer;

		if (isConnected)
			sendPlayers();
		//else
		//	connectServer();
	}
	*/

	return true;
}

bool TServerList::init(const CString& pServerIp, const CString& pServerPort)
{
	// Initialize the socket.
	return sock.init(pServerIp.text(), pServerPort.text()) == 0;
}

bool TServerList::connectServer()
{
	CSettings* settings = _server->getSettings();

	if (getConnected())
		return true;

	// Connect to Server
	if (sock.connect() != 0)
		return false;

	_server->getSocketManager()->registerSocket((CSocketStub*)this);

	_server->getServerLog().out("[%s] :: %s - Connected.\n", _server->getName().text(), sock.getDescription());

	// Get Some Stuff
	CString name(settings->getStr("name"));
	CString desc(settings->getStr("description"));
	CString language(settings->getStr("language", "English"));
	CString version(GSERVER_VERSION);
	CString url(settings->getStr("url", "http://www.graal.in/"));
	CString ip(settings->getStr("serverip", "AUTO"));
	CString port(settings->getStr("serverport", "14900"));
	CString localip(settings->getStr("localip"));

	// Grab the local ip.
	if (localip.isEmpty() || localip == "AUTO")
		localip = sock.getLocalIp();
	if (localip == "127.0.1.1" || localip == "127.0.0.1")
	{
		_server->getServerLog().out(CString() << "[" << _server->getName().text() << "] ** [WARNING] Socket returned " << localip << " for its local ip!  Not sending local ip to serverlist.\n");
		localip.clear();
	}

	// TODO(joey): Some packets were being queued up from the server before we were connected, and would spam the serverlist
	// upon connection. Clearing the outgoing buffer upon connection
	_fileQueue.clearBuffers();

	// Use the new protocol for communicating with the listserver
	_fileQueue.setCodec(ENCRYPT_GEN_1, 0);
	sendPacket(CString() >> (char)SVO_REGISTERV3 << version, true);
	_fileQueue.setCodec(ENCRYPT_GEN_2, 0);

	// Send before SVO_NEWSERVER or else we will get an incorrect name.
	CSettings* adminsettings = _server->getAdminSettings();
	sendPacket(CString() >> (char)SVO_SERVERHQPASS << adminsettings->getStr("hq_password"));

	// Send server info.
	sendPacket(CString() >> (char)SVO_NEWSERVER
		>> (char)name.length() << name
		>> (char)desc.length() << desc
		>> (char)language.length() << language
		>> (char)version.length() << version
		>> (char)url.length() << url
		>> (char)ip.length() << ip
		>> (char)port.length() << port
		>> (char)localip.length() << localip);

	// Set the level now.
	if(_server->getSettings()->getBool("onlystaff", false))
		sendPacket(CString() >> (char)SVO_SERVERHQLEVEL >> (char)0);
	else sendPacket(CString() >> (char)SVO_SERVERHQLEVEL >> (char)adminsettings->getInt("hq_level", 1));

	// Send Players
	sendPlayers();

	// Return Connection-Status
	return getConnected();
}

void TServerList::sendPacket(CString& pPacket, bool sendNow)
{
	// empty buffer?
	if (pPacket.isEmpty())
		return;

	// append '\n'
	if (pPacket[pPacket.length()-1] != '\n')
		pPacket.writeChar('\n');

	// append buffer
	_fileQueue.addPacket(pPacket);

	// send buffer now?
	if (sendNow)
		_fileQueue.sendCompress();
}

/*
	Altering Player Information
*/
void TServerList::addPlayer(TPlayer *player)
{
	assert(player != nullptr);

	CString dataPacket;
	dataPacket >> (char)SVO_PLYRADD >> (short)player->getId() >> (char)player->getType();
	dataPacket >> (char)PLPROP_ACCOUNTNAME << player->getProp(PLPROP_ACCOUNTNAME);
	dataPacket >> (char)PLPROP_NICKNAME << player->getProp(PLPROP_NICKNAME);
	dataPacket >> (char)PLPROP_CURLEVEL << player->getProp(PLPROP_CURLEVEL);
	dataPacket >> (char)PLPROP_X << player->getProp(PLPROP_X);
	dataPacket >> (char)PLPROP_Y << player->getProp(PLPROP_Y);
	dataPacket >> (char)PLPROP_ALIGNMENT << player->getProp(PLPROP_ALIGNMENT);
	sendPacket(dataPacket);
}

void TServerList::deletePlayer(TPlayer *player)
{
	assert(player != nullptr);

	sendPacket(CString() >> (char)SVO_PLYRREM >> (short)player->getId());
}

void TServerList::sendPlayers()
{
	// Clears the serverlist players
	sendPacket(CString() >> (char)SVO_SETPLYR);

	// Adds the players to the serverlist
	auto playerList = _server->getPlayerList();
	for (auto & it : *playerList)
	{
		TPlayer *player = it;
		if (!player->isNC())
			addPlayer(it);
	}
}

void TServerList::handleText(const CString& data)
{
	CString dataTokenStr = data.guntokenize();
	std::vector<CString> params = data.gCommaStrTokens();

	//printf("handleText:%s\n", dataTokenStr.text());
	//for (int i = 0; i < params.size(); i++)
	//	printf("Param %d: %s\n", i, params[i].text());

	if (params.size() >= 3)
	{
		if (params[0] == "GraalEngine")
		{
			if (params[1] == "irc")
			{
				if (params.size() == 6 && params[2] == "privmsg")
				{
					//CString fromPlayer = params[3];
					std::string channel = params[4].guntokenize().text();
					//CString message = params[5];

					auto playerList = _server->getPlayerList();
					for (auto pl : *playerList)
					{
							if (pl->inChatChannel(channel))
							pl->sendPacket(CString() >> (char)PLO_SERVERTEXT << data);
					}
				}
			}
		}
		else if (params.size() >= 4 && params[0] == "Listserver")
		{
			if (params[1] == "Modify" && params[2] == "Server")
			{
				std::string serverName = params[3].guntokenize().text();

				for (int i = 4; i < params.size(); i++)
				{
					params[i].guntokenizeI();
					while (params[i].bytesLeft())
					{
						CString key = params[i].readString("=");
						CString val = params[i].readString("");

						if (key == "players")
						{
							int pcount = strtoint(val);
							if (pcount < 0)
								serverListCount.erase(serverName);
							else
							{
								serverListCount[serverName] = pcount;
							}

							// update shit
							printf("Playercount for %s is %d\n", serverName.c_str(), strtoint(val));
						}

					}
				}
			}
		}
	}
}

void TServerList::sendText(const CString& data)
{
	CString dataPacket;
	dataPacket.writeGChar(SVO_SENDTEXT);
	dataPacket << data;
	sendPacket(dataPacket);
}

void TServerList::sendText(const std::vector<CString>& stringList)
{
	CString dataPacket;
	dataPacket.writeGChar(SVO_SENDTEXT);
	for (const auto & string : stringList)
		dataPacket << string.gtokenize();
	sendPacket(dataPacket);
}

void TServerList::sendTextForPlayer(TPlayer *player, const CString& data)
{
	assert(player != nullptr);

	CString dataPacket;
	dataPacket.writeGChar(SVO_REQUESTLIST);
	dataPacket >> (short)player->getId() << data;
	sendPacket(dataPacket);
}

void TServerList::sendServerHQ()
{
	CSettings* adminsettings = _server->getAdminSettings();
	sendPacket(CString() >> (char)SVO_SERVERHQPASS << adminsettings->getStr("hq_password"));
	if(_server->getSettings()->getBool("onlystaff", false))
		sendPacket(CString() >> (char)SVO_SERVERHQLEVEL >> (char)0);
	else sendPacket(CString() >> (char)SVO_SERVERHQLEVEL >> (char)adminsettings->getInt("hq_level", 1));
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
	sendPacket(CString() >> (char)SVO_SETNAME << pServerName);
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
bool TServerList::parsePacket(CString& pPacket)
{
	while (pPacket.bytesLeft() > 0)
	{
		CString curPacket;
		if (nextIsRaw)
		{
			nextIsRaw = false;
			curPacket = pPacket.readChars(rawPacketSize);
		}
		else curPacket = pPacket.readString("\n");

		// read id & packet
		unsigned char id = curPacket.readGUChar();

		// valid packet, call function
		(*this.*TSLFunc[id])(curPacket);
	}

	return true;
}

void TServerList::msgSVI_NULL(CString& pPacket)
{
	pPacket.setRead(0);
	_server->getServerLog().out("[%s] Unknown Serverlist Packet: %i (%s)\n", _server->getName().text(), pPacket.readGUChar(), pPacket.text()+1);
}

void TServerList::msgSVI_VERIACC(CString& pPacket)
{
	_server->getServerLog().out("[%s] ** SVI_VERIACC is deprecated.  It should not be used.\n", _server->getName().text());
}

void TServerList::msgSVI_VERIGUILD(CString& pPacket)
{
	unsigned short playerID = pPacket.readGUShort();
	CString nickname = pPacket.readChars(pPacket.readGUChar());

	TPlayer* p = _server->getPlayer(playerID, PLTYPE_ANYPLAYER);
	if (p)
	{
		// Create the prop packet.
		CString prop = CString() >> (char)PLPROP_NICKNAME >> (char)nickname.length() << nickname;

		// Assign the nickname to the player.
		p->setNick(nickname, true);
		p->sendPacket(CString() >> (char)PLO_PLAYERPROPS << prop);

		// Tell everybody else the new nickname.
		_server->sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)playerID << prop, p);
	}
}

void TServerList::msgSVI_FILESTART(CString& pPacket)
{
	_server->getServerLog().out("[%s] ** SVI_FILESTART is deprecated.  It should not be used.\n", _server->getName().text());
}

void TServerList::msgSVI_FILEEND(CString& pPacket)
{
	_server->getServerLog().out("[%s] ** SVI_FILEEND is deprecated.  It should not be used.\n", _server->getName().text());
}

void TServerList::msgSVI_FILEDATA(CString& pPacket)
{
	_server->getServerLog().out("[%s] ** SVI_FILEDATA is deprecated.  It should not be used.\n", _server->getName().text());
}

void TServerList::msgSVI_VERSIONOLD(CString& pPacket)
{
	_server->getServerLog().out("[%s] :: You are running an old version of the Graal Reborn gserver.\n"
		":: An updated version is available online.\n", _server->getName().text());
}

void TServerList::msgSVI_VERSIONCURRENT(CString& pPacket)
{
	// Don't bother telling them they are running the latest version.
}

void TServerList::msgSVI_PROFILE(CString& pPacket)
{
	unsigned short requestPlayer = pPacket.readGUShort();
	CString targetPlayer = pPacket.readChars(pPacket.readGUChar());

	TPlayer* p1 = _server->getPlayer(requestPlayer, PLTYPE_ANYPLAYER);
	if (p1 == 0)
		return;

	TPlayer *p2 = _server->getPlayer(targetPlayer, PLTYPE_ANYPLAYER | PLTYPE_NPCSERVER);
	if (p2 == 0)
		return;

	// Start the profile string.
	CString profile;
	profile << p2->getProp(PLPROP_ACCOUNTNAME) << pPacket.readString("");

	// Add the time to the profile string.
	int time = p2->getOnlineTime();
	CString line = CString() << CString((int)time/3600) << " hrs "
		<< CString((int)(time/60)%60) << " mins "
		<< CString((int)time%60) << " secs";
	profile >> (char)line.length() << line;

	// Do the old profile method for the old clients.
	if (p1->isClient() && p1->getVersion() < CLVER_2_1)
	{
		CString val;

		val = CString((int)p2->getKills());
		profile >> (char)val.length() << val;

		val = CString((int)p2->getDeaths());
		profile >> (char)val.length() << val;

		val = CString((int)p2->getProp(PLPROP_MAXPOWER).readGUChar());
		profile >> (char)val.length() << val;

		int rating = p2->getProp(PLPROP_RATING).readGUInt();
		val = CString((int)((rating >> 9) & 0xFFF)) << "/" << CString((int)(rating & 0x1FF));
		profile >> (char)val.length() << val;

		val = CString((int)p2->getProp(PLPROP_ALIGNMENT).readGUChar());
		profile >> (char)val.length() << val;

		val = CString((int)p2->getProp(PLPROP_RUPEESCOUNT).readGUInt());
		profile >> (char)val.length() << val;

		val = CString((int)(p2->getProp(PLPROP_SWORDPOWER).readGUChar() - 30));
		profile >> (char)val.length() << val;

		bool canSpin = ((p2->getProp(PLPROP_STATUS).readGUChar() & PLSTATUS_HASSPIN) != 0 ? true : false);
		if (canSpin) val = "true"; else val = "false";
		profile >> (char)val.length() << val;
	}
	else if (!p2->isNPCServer())
	{
		// Add all the specified variables to the profile string.
		CString profileVars = _server->getSettings()->getStr("profilevars");
		if (profileVars.length() != 0)
		{
			std::vector<CString> vars = profileVars.tokenize(",");
			for (std::vector<CString>::iterator i = vars.begin(); i != vars.end(); ++i)
			{
				CString name = i->readString(":=").trim();
				CString val = i->readString("").trim();

				// Built-in values.
				if (val == "playerkills")
					val = CString((unsigned int)(p2->getKills()));
				else if (val == "playerdeaths")
					val = CString((unsigned int)(p2->getDeaths()));
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
					// Find if String-Array
					int pos[3] = {0, 0, 0};
					pos[0] = val.findl('{');
					pos[1] = val.find('}', pos[0]);
					pos[2] = (pos[0] >= 0 && pos[1] > 0 ? strtoint(val.subString(pos[0]+1, pos[1]-1)) : -1);

					// Find Flag Name / Value
					CString flagName = val.subString(0, pos[0]);
					val = p2->getFlag(flagName.text());

					// If String-Array, Get Index
					if (pos[2] >= 0)
					{
						std::vector<CString> temp = val.guntokenize().tokenize("\n");
						if ((int)temp.size() > pos[2])
							val = temp[pos[2]];
					}
				}

				// Add it to the profile now.
				profile >> (char)(name.length() + val.length() + 2) << name << ":=" << val;
			}
		}
	}

	// Send the profiles.
	p1->sendPacket(CString() >> (char)PLO_PROFILE << profile);
}

void TServerList::msgSVI_ERRMSG(CString& pPacket)
{
	_server->getServerLog().out("[%s] %s\n", _server->getName().text(), pPacket.readString("").text());
}

void TServerList::msgSVI_VERIACC2(CString& pPacket)
{
	CString account = pPacket.readChars(pPacket.readGUChar());
	unsigned short id = pPacket.readGUShort();
	unsigned char type = pPacket.readGUChar();
	CString message = pPacket.readString("");

	// Get the player.
	TPlayer* player = _server->getPlayer(id, PLTYPE_ANYPLAYER | PLTYPE_ANYNC);
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
		//player->sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Failed to send login information.");
		player->setId(0);	// Prevent saving of the account.
		player->disconnect();
	}
}

void TServerList::msgSVI_FILESTART2(CString& pPacket)
{
	_server->getServerLog().out("[%s] ** SVI_FILESTART2 is deprecated.  It should not be used.\n", _server->getName().text());
}

void TServerList::msgSVI_FILEDATA2(CString& pPacket)
{
	_server->getServerLog().out("[%s] ** SVI_FILEDATA2 is deprecated.  It should not be used.\n", _server->getName().text());
}

void TServerList::msgSVI_FILEEND2(CString& pPacket)
{
	_server->getServerLog().out("[%s] ** SVI_FILEEND2 is deprecated.  It should not be used.\n", _server->getName().text());
}

void TServerList::msgSVI_PING(CString& pPacket)
{
	// When server pings, we pong
	sendPacket(CString() >> (char)SVO_PING);
}

void TServerList::msgSVI_RAWDATA(CString& pPacket)
{
	//nextIsRaw = true;
	//rawPacketSize = pPacket.readGInt();
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
	blank.save(CString() << _server->getServerPath() << filename);
	_server->getFileSystem()->addFile(filename);
}

void TServerList::msgSVI_FILEDATA3(CString& pPacket)
{
	unsigned char pTy = pPacket.readGUChar();
	CString filename = _server->getFileSystem()->find(pPacket.readChars(pPacket.readGUChar()));
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
	bool foldersconfig = !_server->getSettings()->getBool("nofoldersconfig", false);
	CFileSystem* fileSystem = 0;
	CString typeString;
	switch (type)
	{
		case SVF_HEAD:
			typeString = "heads/";
			if (foldersconfig) fileSystem = _server->getFileSystem(FS_HEAD);
			break;
		case SVF_BODY:
			typeString = "bodies/";
			if (foldersconfig) fileSystem = _server->getFileSystem(FS_BODY);
			break;
		case SVF_SWORD:
			typeString = "swords/";
			if (foldersconfig) fileSystem = _server->getFileSystem(FS_SWORD);
			break;
		case SVF_SHIELD:
			typeString = "shields/";
			if (foldersconfig) fileSystem = _server->getFileSystem(FS_SHIELD);
			break;
	}
	CString fileName = _server->getFileSystem()->find(shortName);

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
	if (_server->getFileSystem()->setModTime(shortName, modTime) == false)
		_server->getServerLog().out("[%s] ** [WARNING] Could not set modification time on file %s\n", _server->getName().text(), fileName.text());

	// Set the player props.
	// TODO(joey): Confirm if we can use ANYCLIENT instead
	TPlayer* p = _server->getPlayer(pid, PLTYPE_ANYPLAYER);
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

void TServerList::msgSVI_SERVERINFO(CString& pPacket)
{
	int pid = pPacket.readGUShort();
	CString serverpacket = pPacket.readString("");
/*
 * This is an old hack, disabling for now
	TPlayer *player = _server->getPlayer(pid, PLTYPE_ANYCLIENT);
	if (player && player->getVersion() >= CLVER_2_1)
		player->sendPacket(CString() >> (char)PLO_SERVERWARP << serverpacket);
*/
}

void TServerList::msgSVI_REQUESTTEXT(CString& pPacket)
{
	unsigned short playerId = pPacket.readGUShort();
	CString message = pPacket.readString("");

	CString data = message.guntokenize();
	std::vector<CString> params = data.tokenize("\n");

	CString weapon = data.readString("\n");
	CString type = data.readString("\n");
	CString option = data.readString("\n");
	CString paramsData = data.readString("");

	TPlayer *player = _server->getPlayer(playerId);
	if (player != nullptr)
	{
		if (params.size() >= 3)
		{
			if (params[0] == "GraalEngine")
			{
				if (params[1] == "irc")
				{
					// Listserver can confirm this stuff, and use it for having a count of players in channels
					if (params[2] == "join")
					{
						CString channel = params[3].guntokenize();
						if (player->addChatChannel(channel.text()))
							player->sendPacket(CString() >> (char)PLO_SERVERTEXT << "GraalEngine,irc,join," << params[3].gtokenize());
					}
					else if (params[2] == "part")
					{
						CString channel = params[3].guntokenize();
						if (player->inChatChannel(channel.text()))
							player->sendPacket(CString() >> (char)PLO_SERVERTEXT << "GraalEngine,irc,part," << params[3].gtokenize());
					}
				}
			}
		}
	}

	if (type == "lister" && option == "simpleserverlist")
	{
		CString serverIds = "updateservernames\n", serverNames = "", serverPCount = "updateserverplayers\n";
        int serverCount = 0;
		while (paramsData.bytesLeft() > 0)
		{
			CString serverData = paramsData.readString("\n").guntokenizeI();
			serverIds << serverData.readString("\n") << "\n";
			serverNames << serverData.readString("\n") << "\n";
			serverPCount << serverData.readString("\n") << "\n";
			serverData.clear();
			serverCount++;
		}

		serverIds = CString() << std::to_string(serverCount) << "\n" << serverIds;
        serverPCount = CString() << std::to_string(serverCount) << "\n" << serverPCount;

		// TODO(joey): This is spamming clients non-stop!!!!!
		_server->sendPacketToAll(CCommon::triggerAction(0, 0, "clientside", "-Serverlist_v4", serverIds.gtokenizeI()));
		_server->sendPacketToAll(CCommon::triggerAction(0, 0, "clientside", "-Serverlist_v4", serverPCount.gtokenizeI()));
		serverIds.clear();
		serverNames.clear();
		serverPCount.clear();
	}

	player = _server->getPlayer(playerId, PLTYPE_ANYPLAYER);
	if (player)
	{
		if (type == "pmserverplayers")
		{
			player->updatePMPlayers(option, paramsData);
		}
		else
		{
			_server->getServerLog().out("[OUT] [RequestText] %s\n", message.text());

			if (player->getVersion() >= CLVER_4_0211 || player->getVersion() > RCVER_1_1)
				player->sendPacket(CString() >> (char)PLO_SERVERTEXT << message);
		}
	}
}

void TServerList::msgSVI_SENDTEXT(CString& pPacket)
{
	CString data = pPacket.readString("");
	handleText(data);
}

void TServerList::msgSVI_PMPLAYER(CString& pPacket)
{
	CString message = pPacket.readString("");
	CString data = message.guntokenize();

	CString servername = data.readString("\n");
	CString account = data.readString("\n");
	CString nick = data.readString("\n");
	CString weapon = data.readString("\n");
	CString type = data.readString("\n");
	CString account2 = data.readString("\n");

	CString message2 = data.readString("");
	CString message3 = message2.gtokenizeI();

	CString player = CString(CString() << account << "\n" << nick << "\n").gtokenizeI() << "\n";
	CString pmMessageType("\"\",");
	pmMessageType << "\"Private message:\",";

	TPlayer *p = _server->getPlayer(account2, PLTYPE_ANYPLAYER);
	if (p)
	{
		p->addPMServer(servername);
		p->updatePMPlayers(servername, player);
		TPlayer* tmpPlyr = p->getExternalPlayer(account);
		p->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)tmpPlyr->getId() << pmMessageType << message3,true);
	}

	message2 = "";
}
