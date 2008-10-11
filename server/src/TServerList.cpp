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
	for (int i = 0; i < 100; i++)
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
	TSLFunc[SVI_PING] = &TServerList::msgSVI_PING;
}

/*
	Constructor - Deconstructor
*/
TServerList::TServerList()
: isConnected(false)
{
	sock.setProtocol(SOCKET_PROTOCOL_TCP);
	sock.setType(SOCKET_TYPE_CLIENT);
	sock.setOptions(SOCKET_OPTION_NONBLOCKING);
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
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return isConnected;
}

bool TServerList::main()
{
	if (!getConnected())
		return false;

	// definitions
	CString line;
	int lineEnd;

	// receive
	if (sock.getData() == -1)
	{
		isConnected = false;
		return getConnected();
	}

	// grab the data now
	rBuffer.write(sock.getBuffer());
	sock.getBuffer().clear();

	// do we have enough data to parse?
	rBuffer.setRead(0);
	if (rBuffer.length() > 0)
	{
		// parse data
		if ((lineEnd = rBuffer.findl('\n')) == -1)
			return getConnected();

		line = rBuffer.subString(0, lineEnd + 1);
		rBuffer.removeI(0, line.length());

		// parse each packet seperately
		std::vector<CString> lines = line.tokenize("\n");
		for (unsigned int i = 0; i < lines.size(); i++)
			parsePacket(lines[i]);

		// update last data
		lastData = time(0);
	}

	// Every second, do some events.
	if (time(0) != lastTimer) doTimedEvents();

	// send out buffer
	sendCompress();
	return getConnected();
}

bool TServerList::doTimedEvents()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
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
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// Initialize the socket.
	if (sock.init(pServerIp, pServerPort) != 0)
		return false;

	return true;
}

bool TServerList::connectServer()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	CSettings* settings = server->getSettings();

	if (isConnected == true)
		return true;

	// Connect to Server
	if (sock.connect() == 0)
		isConnected = true;
	else
		return false;

	server->getServerLog().out("%s - Connected.\n", sock.getDescription());

	// Set Some Stuff
	setName(settings->getStr("name"));
	setDesc(settings->getStr("description"));
	setUrl(settings->getStr("url"));
	setVersion(GSERVER_VERSION);
	setIp(settings->getStr("serverip", "AUTO"));
	setPort(settings->getStr("serverport", "14900"));
	sendCompress();

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
	boost::mutex::scoped_lock lock_sendPacket(m_sendPacket);
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
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// Definition
	CString playerPacket;
	int playerCount = 0;

	// Iterate Playerlist
	boost::recursive_mutex::scoped_lock lock_playerList(server->m_playerList);
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
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	sendPacket(CString() >> (char)SVO_SETDESC << pServerDesc);
}

void TServerList::setIp(const CString& pServerIp)
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	sendPacket(CString() >> (char)SVO_SETIP << pServerIp);
}

void TServerList::setName(const CString& pServerName)
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	bool uc = server->getSettings()->getBool("underconstruction", false);
	sendPacket(CString() >> (char)SVO_SETNAME << (uc ? "U " : "") << pServerName);
}

void TServerList::setPort(const CString& pServerPort)
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	sendPacket(CString() >> (char)SVO_SETPORT << pServerPort);
}

void TServerList::setUrl(const CString& pServerUrl)
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	sendPacket(CString() >> (char)SVO_SETURL << pServerUrl);
}

void TServerList::setVersion(const CString& pServerVersion)
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
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
	boost::mutex::scoped_lock lock_sendPacket(m_sendPacket);
	boost::mutex::scoped_lock lock_sendCompress(m_sendCompress);

	// empty buffer?
	if (sBuffer.isEmpty())
		return;

	// send buffer
	sock.sendData(sBuffer);

	// clear buffer
	sBuffer.clear();
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
	server->getServerLog().out("TODO: TServerList::msgSVI_VERIGUILD\n");
}

void TServerList::msgSVI_FILESTART(CString& pPacket)
{
	server->getServerLog().out("TODO: TServerList::msgSVI_FILESTART\n");
}

void TServerList::msgSVI_FILEDATA(CString& pPacket)
{
	server->getServerLog().out("TODO: TServerList::msgSVI_FILEDATA\n");
}

void TServerList::msgSVI_FILEEND(CString& pPacket)
{
	server->getServerLog().out("TODO: TServerList::msgSVI_FILEEND\n");
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
	server->getServerLog().out("TODO: TServerList::msgSVI_PROFILE\n");
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

	// If we did not get the success message, inform the client of his failure.
	if (message != "SUCCESS")
	{
		player->sendPacket(CString() >> (char)PLO_DISCMESSAGE << message);
		player->sendCompress();
		player->disconnect();
		return;
	}

	// Send the player his account.  If it fails, disconnect him.
	if (player->sendLogin() == false)
	{
		player->sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Failed to send login information.");
		player->disconnect();
	}
}

void TServerList::msgSVI_PING(CString& pPacket)
{
	// Sent every 60 seconds.  Do nothing.
}
