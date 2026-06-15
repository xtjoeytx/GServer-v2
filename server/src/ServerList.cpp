#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <string_view>
#include <string>
#include <vector>

#include <CSocket.h>

#include <CEncryption.h>
#include <CString.h>
#include <IEnums.h>
#include <IUtil.h>

#include <IConfig.h>

#include <Server.h>
#include <ServerList.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <npcserver/NPCServer.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <player/PlayerProps.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>
#include <utilities/PropertySerializers.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

/*
	Pointer-Functions for Packets
*/
bool ServerList::created = false;
typedef void (ServerList::*TSLSock)(CString&);
std::vector<TSLSock> TSLFunc(256, &ServerList::msgSVI_NULL);

void ServerList::createFunctions()
{
	if (ServerList::created)
		return;

	// now set non-nulls
	TSLFunc[SVI_VERIACC] = &ServerList::msgSVI_VERIACC;
	TSLFunc[SVI_VERIGUILD] = &ServerList::msgSVI_VERIGUILD;
	TSLFunc[SVI_FILESTART] = &ServerList::msgSVI_FILESTART;
	TSLFunc[SVI_FILEDATA] = &ServerList::msgSVI_FILEDATA;
	TSLFunc[SVI_FILEEND] = &ServerList::msgSVI_FILEEND;
	TSLFunc[SVI_VERSIONOLD] = &ServerList::msgSVI_VERSIONOLD;
	TSLFunc[SVI_VERSIONCURRENT] = &ServerList::msgSVI_VERSIONCURRENT;
	TSLFunc[SVI_PROFILE] = &ServerList::msgSVI_PROFILE;
	TSLFunc[SVI_ERRMSG] = &ServerList::msgSVI_ERRMSG;
	TSLFunc[SVI_VERIACC2] = &ServerList::msgSVI_VERIACC2;
	TSLFunc[SVI_FILESTART2] = &ServerList::msgSVI_FILESTART2;
	TSLFunc[SVI_FILEDATA2] = &ServerList::msgSVI_FILEDATA2;
	TSLFunc[SVI_FILEEND2] = &ServerList::msgSVI_FILEEND2;
	TSLFunc[SVI_PING] = &ServerList::msgSVI_PING;
	TSLFunc[SVI_RAWDATA] = &ServerList::msgSVI_RAWDATA;
	TSLFunc[SVI_FILESTART3] = &ServerList::msgSVI_FILESTART3;
	TSLFunc[SVI_FILEDATA3] = &ServerList::msgSVI_FILEDATA3;
	TSLFunc[SVI_FILEEND3] = &ServerList::msgSVI_FILEEND3;
	TSLFunc[SVI_SERVERINFO] = &ServerList::msgSVI_SERVERINFO;
	TSLFunc[SVI_REQUESTTEXT] = &ServerList::msgSVI_REQUESTTEXT;
	TSLFunc[SVI_SENDTEXT] = &ServerList::msgSVI_SENDTEXT;
	TSLFunc[SVI_PMPLAYER] = &ServerList::msgSVI_PMPLAYER;
	TSLFunc[SVI_ASSIGNPCID] = &ServerList::msgSVI_ASSIGNPCID;

	// Finished
	ServerList::created = true;
}

/*
	Constructor - Deconstructor
*/
ServerList::ServerList()
	: m_fileQueue(&m_socket)
{
	m_socket.setProtocol(SOCKET_PROTOCOL_TCP);
	m_socket.setType(SOCKET_TYPE_CLIENT);
	m_socket.setDescription("listserver");

	m_lastData = m_lastTimer = precise_clock::now();

	// Create Functions
	if (!ServerList::created)
		ServerList::createFunctions();
}

ServerList::~ServerList()
{
}

/*
	Socket-Control Functions
*/
bool ServerList::getConnected() const
{
	return (m_socket.getState() == SOCKET_STATE_CONNECTED);
}

bool ServerList::onRecv()
{
	// Grab the data from the socket and put it into our receive buffer.
	unsigned int size = 0;
	char* data = m_socket.getData(&size);
	if (size != 0)
		m_readBuffer.write(data, size);
	else if (m_socket.getState() == SOCKET_STATE_DISCONNECTED)
		return false;

	main();

	return true;
}

bool ServerList::onSend()
{
	m_fileQueue.sendCompress();
	return true;
}

bool ServerList::canRecv()
{
	if (m_socket.getState() == SOCKET_STATE_DISCONNECTED) return false;
	return true;
}

void ServerList::onUnregister()
{
	log::printLine(log::server, "{} - Disconnected.", m_socket.getDescription());
}

bool ServerList::main(precise_clock::time_point time)
{
	if (!getConnected())
		return false;

	// definitions
	CString unBuffer;
	m_readBuffer.setRead(0);

	// New data.
	if (m_readBuffer.length() > 1)
		m_lastData = time;

	// parse data
	while (m_readBuffer.length() > 1)
	{
		// packet length
		unsigned short len = (unsigned short)m_readBuffer.readShort();
		if ((unsigned int)len > (unsigned int)m_readBuffer.length() - 2)
			break;

		// decompress packet
		unBuffer = m_readBuffer.readChars(len);
		m_readBuffer.removeI(0, len + 2);
		unBuffer.zuncompressI();

		// well theres your buffer
		if (!parsePacket(unBuffer))
			return false;
	}

	m_server->getSocketManager().updateSingle(this, false, true);

	return getConnected();
}

// Called every second by Server
bool ServerList::doTimedEvents(precise_clock::time_point time)
{
	m_lastTimer = time;

	bool isConnected = getConnected();

	// Send a keep-alive packet every 60 seconds.
	// We send SVO_SETIP rather than SVO_PING because SVO_PING is actually a PONG, and the listserver uses the time between SVI_PING and SVO_PING to determine latency.
	// Sending the IP again is a safe keep-alive packet.
	if (isConnected && timeDifference<std::chrono::seconds, precise_clock>(m_lastPingTime, time) >= 60s)
	{
		m_lastPingTime = time;

		auto& settings = m_server->getSettings();
		CString ip(settings.get<std::string>("serverip").value_or("AUTO"s));
		sendPacket(CString() >> (char)SVO_SETIP >> (char)ip.length() << ip);
	}

	// Reconnect to the listserver, with connection backoff to prevent a flood of connections
	if (!isConnected)
	{
		if (timePassed<precise_clock>(m_lastTimer, m_nextConnectionAttempt))
		{
			if (!connectServer())
			{
				if (m_connectionAttempts < 8)
					m_connectionAttempts += 1;

				auto waitTime = std::min(uint32_t(std::pow(2u, m_connectionAttempts)), 300u);
				m_nextConnectionAttempt = m_lastTimer + std::chrono::seconds(waitTime);
			}
			else
				m_connectionAttempts = 0;
		}
	}

	return true;
}

bool ServerList::connectServer()
{
	auto& settings = m_server->getSettings();

	if (getConnected())
		return true;

	log::printLine(log::server, "Initializing {} socket.", m_socket.getDescription());

	// Initialize the socket
	if (m_socket.init(settings.get<std::string>("listip").value_or(""s).c_str(), settings.get<std::string>("listport").value_or(""s).c_str()) != 0)
	{
		log::printLine(log::server, "[Error] Could not initialize {} socket.", m_socket.getDescription());
		return false;
	}

	// Connect to Server
	if (m_socket.connect() != 0)
	{
		log::printLine(log::server, "[Error] Could not connect {} socket.", m_socket.getDescription());
		return false;
	}

	m_server->getSocketManager().registerSocket((CSocketStub*)this);
	log::printLine(log::server, "{} - Connected to {}:{}.", m_socket.getDescription(), m_socket.getRemoteIp(), m_socket.getRemotePort());

	// Get Some Stuff
	CString name(settings.get<std::string>("name").value_or(""s));
	CString desc(settings.get<std::string>("description").value_or(""s));
	CString language(settings.get<std::string>("language").value_or("English"s));
	CString version(APP_VERSION);
	CString url(settings.get<std::string>("url").value_or("http://www.graal.in/"s));
	CString ip(settings.get<std::string>("serverip").value_or("AUTO"s));
	CString port(settings.get<std::string>("serverport").value_or("14900"s));
	CString localip(settings.get<std::string>("localip").value_or(""s));

	// Grab the local ip.
	if (localip.isEmpty() || localip == "AUTO")
		localip = m_socket.getLocalIp();
	if (localip == "127.0.1.1" || localip == "127.0.0.1")
	{
		log::printLine(log::server, "** [WARNING] Socket returned {} for its local ip!  Not sending local ip to serverlist.", localip);
		localip.clear();
	}

	// TODO(joey): Some packets were being queued up from the server before we were connected, and would spam the serverlist
	// upon connection. Clearing the outgoing buffer upon connection
	m_fileQueue.clearBuffers();

	// Use the new protocol for communicating with the listserver
	m_fileQueue.setCodec(ENCRYPT_GEN_1, 0);
	sendPacket(CString() >> (char)SVO_REGISTERV3 << version, true);
	m_fileQueue.setCodec(ENCRYPT_GEN_2, 0);

	// Send before SVO_NEWSERVER or else we will get an incorrect name.
	auto& adminsettings = m_server->getAdminSettings();
	sendPacket(CString() >> (char)SVO_SERVERHQPASS << adminsettings.get<std::string>("hq_password").value_or(""s));

	// Send server info.
	sendPacket(CString() >> (char)SVO_NEWSERVER >> (char)name.length() << name >> (char)desc.length() << desc >> (char)language.length() << language >> (char)version.length() << version >> (char)url.length() << url >> (char)ip.length() << ip >> (char)port.length() << port >> (char)localip.length() << localip);

	// Set the level now.
	if (m_server->getSettings().get<bool>("onlystaff").value_or(false))
		sendPacket(CString() >> (char)SVO_SERVERHQLEVEL >> (char)0);
	else
		sendPacket(CString() >> (char)SVO_SERVERHQLEVEL >> (char)adminsettings.get<int>("hq_level").value_or(1));

	sendVersionConfig();

	// Send Players
	sendPlayers();

	// Set the ping time so we start pings 60 seconds from now.
	m_lastPingTime = m_server->getFrameStartTimeHighPrecision();

	// Return Connection-Status
	return getConnected();
}

void ServerList::sendVersionConfig()
{
	if (!getConnected())
		return;

	// Send allowed versions to the listserver
	CString versionNames;
	auto& versionList = m_server->getAllowedVersions();
	for (const auto& version : versionList)
	{
		if (!versionNames.isEmpty())
			versionNames << ",";

		versionNames << version.gtokenize();
	}

	sendText(std::format("Listserver,settings,allowedversions,{}", versionNames.text()));
}

void ServerList::sendPacket(CString& pPacket, bool sendNow)
{
	// empty buffer?
	if (pPacket.isEmpty())
		return;

	// append '\n'
	if (pPacket[pPacket.length() - 1] != '\n')
		pPacket.writeChar('\n');

	// append buffer
	m_fileQueue.addPacket(pPacket);

	// send buffer now?
	if (sendNow)
		m_fileQueue.sendCompress();
}

/*
	Altering Player Information
*/
void ServerList::addPlayer(std::shared_ptr<Player> player)
{
	assert(player != nullptr);

	if (player->isNC() || player->isNPCServer())
		return;

	CString dataPacket;
	dataPacket >> (char)SVO_PLYRADD >> (short)player->getId() >> (char)player->getType();
	dataPacket >> (char)PlayerProp::ACCOUNTNAME << player->getProp<PlayerProp::ACCOUNTNAME>().serialize();
	dataPacket >> (char)PlayerProp::NICKNAME << player->getProp<PlayerProp::NICKNAME>().serialize();
	dataPacket >> (char)PlayerProp::CURLEVEL << player->getProp<PlayerProp::CURLEVEL>().serialize();
	dataPacket >> (char)PlayerProp::X << player->getProp<PlayerProp::X>().serialize();
	dataPacket >> (char)PlayerProp::Y << player->getProp<PlayerProp::Y>().serialize();
	dataPacket >> (char)PlayerProp::ALIGNMENT << player->getProp<PlayerProp::ALIGNMENT>().serialize();
	dataPacket >> (char)PlayerProp::IPADDR << player->getProp<PlayerProp::IPADDR>().serialize();
	sendPacket(dataPacket);
}

void ServerList::deletePlayer(std::shared_ptr<Player> player)
{
	assert(player != nullptr);

	sendPacket(CString() >> (char)SVO_PLYRREM >> (short)player->getId());
}

void ServerList::sendPlayers()
{
	// Clears the serverlist players
	sendPacket(CString() >> (char)SVO_SETPLYR);

	// Adds the players to the serverlist
	auto& playerList = m_server->getPlayerList();
	for (auto& [id, player] : playerList)
	{
		if (!player->isNC() && !player->isNPCServer())
		{
			addPlayer(player);
			player->sendPacket(CString() >> (char)PLO_SERVERLISTCONNECTED);
		}
	}
}

void ServerList::handleText(const CString& data)
{
	CString dataTokenStr = data.guntokenize();
	std::vector<CString> params = data.gCommaStrTokens();

	if (params.size() >= 3)
	{
		if (params[0] == "GraalEngine")
		{
			if (params[1] == "irc")
			{
				if (params.size() == 6 && params[2] == "privmsg")
				{
					std::string channel = params[4].guntokenize().text();
					CString tmpData = CString(",irc,privmsg,") << params[3].gtokenize() << "," << params[4].gtokenize() << "," << params[5].gtokenize();

					auto& playerList = m_server->getPlayerList();
					for (auto& [id, pl] : playerList)
					{
						if (pl->inChatChannel(channel))
						{
							CString weapon = pl->isClient() ? "-Serverlist_Chat" : "GraalEngine";
							pl->sendPacket(CString() >> (char)PLO_SERVERTEXT << weapon << tmpData);
						}
					}
				}
			}
		}
		else if (params[0] == "Listserver")
		{
			if (params.size() == 3 && params[1] == "SetRemoteIp")
			{
				m_serverRemoteIp = params[2].text();
				log::printLine(log::server, "listserver - Remote IP identified as '{}'.", m_serverRemoteIp);

				if (m_server->hasNPCServer())
					m_server->getNPCServer()->setRemoteIp(m_serverRemoteIp);
			}
			else if (params.size() >= 4)
			{
				if (params[1] == "Modify" && params[2] == "Server")
				{
					std::string serverName = params[3].guntokenize().text();

					for (size_t i = 4; i < params.size(); i++)
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
									m_serverListCount.erase(serverName);
								else
								{
									m_serverListCount[serverName] = pcount;
								}
							}
						}
					}
				}
			}
		}
	}
}

void ServerList::sendText(const CString& data)
{
	CString dataPacket;
	dataPacket.writeGChar(SVO_SENDTEXT);
	dataPacket << data;
	sendPacket(dataPacket);
}

void ServerList::sendText(const std::vector<CString>& stringList)
{
	CString dataPacket;
	dataPacket.writeGChar(SVO_SENDTEXT);
	for (const auto& string : stringList)
		dataPacket << string.gtokenize();
	sendPacket(dataPacket);
}

void ServerList::sendTextForPlayer(std::shared_ptr<Player> player, const CString& data)
{
	assert(player != nullptr);

	CString dataPacket;
	dataPacket.writeGChar(SVO_REQUESTLIST);
	dataPacket >> (short)player->getId() << data;
	sendPacket(dataPacket);
}

void ServerList::sendLoginPacketForPlayer(std::shared_ptr<Player> player, const CString& password, const CString& identity)
{
	sendPacket(CString() >> (char)SVO_VERIACC2 >> (char)player->account.name.length() << player->account.name >> (char)password.length() << password >> (short)player->getId() >> (char)player->getType() >> (short)identity.length() << identity);
}

void ServerList::sendServerHQ()
{
	auto& adminsettings = m_server->getAdminSettings();
	sendPacket(CString() >> (char)SVO_SERVERHQPASS << adminsettings.get<std::string>("hq_password").value_or(""s));
	if (m_server->getSettings().get<bool>("onlystaff").value_or(false))
		sendPacket(CString() >> (char)SVO_SERVERHQLEVEL >> (char)0);
	else
		sendPacket(CString() >> (char)SVO_SERVERHQLEVEL >> (char)adminsettings.get<int>("hq_level").value_or(1));
}

/*
	Packet-Functions
*/
bool ServerList::parsePacket(CString& pPacket)
{
	while (pPacket.bytesLeft() > 0)
	{
		CString curPacket;
		if (m_nextIsRaw)
		{
			m_nextIsRaw = false;
			curPacket = pPacket.readChars(m_rawPacketSize);
		}
		else
			curPacket = pPacket.readString("\n");

		// read id & packet
		unsigned char id = curPacket.readGUChar();

		// valid packet, call function
		(*this.*TSLFunc[id])(curPacket);
	}

	return true;
}

void ServerList::msgSVI_NULL(CString& pPacket)
{
	pPacket.setRead(0);
	log::printLine(log::server, "Unknown Serverlist Packet: %i (%s)\n", pPacket.readGUChar(), pPacket.text() + 1);
}

void ServerList::msgSVI_VERIACC(CString& pPacket)
{
	log::printLine(log::server, "** SVI_VERIACC is deprecated.  It should not be used.\n");
}

void ServerList::msgSVI_VERIGUILD(CString& pPacket)
{
	unsigned short playerID = pPacket.readGUShort();
	CString nickname = pPacket.readChars(pPacket.readGUChar());

	auto p = m_server->getPlayer(playerID, PLTYPE_ANYPLAYER);
	if (p)
	{
		// Create the prop packet.
		CString prop = CString() >> (char)PlayerProp::NICKNAME >> (char)nickname.length() << nickname;

		// Assign the nickname to the player.
		p->setNick(nickname, true);
		p->sendPacket(CString() >> (char)PLO_PLAYERPROPS << prop);

		// Tell everybody else the new nickname.
		m_server->sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)playerID << prop, {p->getId()});
	}
}

void ServerList::msgSVI_FILESTART(CString& pPacket)
{
	log::printLine(log::server, "** SVI_FILESTART is deprecated.  It should not be used.");
}

void ServerList::msgSVI_FILEEND(CString& pPacket)
{
	log::printLine(log::server, "** SVI_FILEEND is deprecated.  It should not be used.");
}

void ServerList::msgSVI_FILEDATA(CString& pPacket)
{
	log::printLine(log::server, "** SVI_FILEDATA is deprecated.  It should not be used.");
}

void ServerList::msgSVI_VERSIONOLD(CString& pPacket)
{
	log::printLine(log::server, "You are running an old version of {} {}.", APP_VENDOR, APP_NAME);
	log::printLine(log::server, "An updated version is available online.");
}

void ServerList::msgSVI_VERSIONCURRENT(CString& pPacket)
{
	// Don't bother telling them they are running the latest version.
}

void ServerList::msgSVI_PROFILE(CString& pPacket)
{
	unsigned short requestPlayer = pPacket.readGUShort();
	CString targetPlayer = pPacket.readChars(pPacket.readGUChar());

	auto p1 = m_server->getPlayer(requestPlayer, PLTYPE_ANYPLAYER);
	if (p1 == nullptr)
		return;

	auto p2 = m_server->getPlayer(targetPlayer, PLTYPE_ANYPLAYER | PLTYPE_NPCSERVER);
	if (p2 == nullptr)
		return;

	// Start the profile string.
	CString profile;
	profile << p2->getProp<PlayerProp::ACCOUNTNAME>().serialize() << pPacket.readString("");

	// Add the time to the profile string.
	auto time = p2->account.onlineSeconds;
	CString line = CString() << CString((uint32_t)time / 3600) << " hrs "
							 << CString((uint32_t)(time / 60) % 60) << " mins "
							 << CString((uint32_t)time % 60) << " secs";
	profile >> (char)line.length() << line;

	// Do the old profile method for the old clients.
	if (p1->isClient() && p1->getVersion() < CLVER_2_1)
	{
		CString val;

		val = CString((int)p2->account.kills);
		profile >> (char)val.length() << val;

		val = CString((int)p2->account.deaths);
		profile >> (char)val.length() << val;

		val = CString((int)p2->getProp<PlayerProp::MAXPOWER>().value);
		profile >> (char)val.length() << val;

		auto rating = p2->getProp<PlayerProp::RATING>();
		val = CString((int)rating.rating) << "/" << CString((int)rating.deviation);
		profile >> (char)val.length() << val;

		val = CString((int)p2->getProp<PlayerProp::ALIGNMENT>().value);
		profile >> (char)val.length() << val;

		val = CString((int)p2->getProp<PlayerProp::RUPEESCOUNT>().value);
		profile >> (char)val.length() << val;

		val = CString((int)p2->getProp<PlayerProp::SWORDPOWER>().power.value_or(1));
		profile >> (char)val.length() << val;

		bool canSpin = ((p2->getProp<PlayerProp::STATUS>().value & PLSTATUS_HASSPIN) != 0 ? true : false);
		val = (canSpin) ? "true" : "false";
		profile >> (char)val.length() << val;
	}
	else if (!p2->isNPCServer())
	{
		// Add all the specified variables to the profile string.
		for (const auto& profilevar : m_server->cached.playerProfileVariables.value.value())
		{
			auto tokens = string::splitToVectorByString(profilevar, ":="sv);
			if (tokens.size() != 2)
				continue;

			CString name = string::trim(tokens[0]);
			CString val = string::trim(tokens[1]);

			// Built-in values.
			if (val == "playerkills")
				val = CString(p2->account.kills);
			else if (val == "playerdeaths")
				val = CString(p2->account.deaths);
			else if (val == "playerfullhearts")
				val = CString(p2->getProp<PlayerProp::MAXPOWER>().value);
			else if (val == "playerrating")
			{
				auto rating = p2->getProp<PlayerProp::RATING>();
				val = CString(rating.rating) << "/" << CString(rating.deviation);
			}
			else if (val == "playerap")
				val = CString(p2->getProp<PlayerProp::ALIGNMENT>().value);
			else if (val == "playerrupees")
				val = CString(p2->getProp<PlayerProp::RUPEESCOUNT>().value);
			else if (val == "playerswordpower")
				val = CString(p2->getProp<PlayerProp::SWORDPOWER>().power.value_or(1));
			else if (val == "canspin")
				val = ((p2->getProp<PlayerProp::STATUS>().value & PLSTATUS_HASSPIN) ? "true" : "false");
			else if (val == "playerhearts")
			{
				auto power = p2->getProp<PlayerProp::CURPOWER>().value;
				val = CString(power / 2);
				if (power % 2 == 1) val << ".5";
			}
			else if (val == "playerdarts")
				val = CString(p2->getProp<PlayerProp::ARROWSCOUNT>().value);
			else if (val == "playerbombs")
				val = CString(p2->getProp<PlayerProp::BOMBSCOUNT>().value);
			else if (val == "playermp")
				val = CString(p2->getProp<PlayerProp::MAGICPOINTS>().value);
			else if (val == "playershieldpower")
				val = CString(p2->getProp<PlayerProp::SHIELDPOWER>().power.value_or(1));
			else if (val == "playerglovepower")
				val = CString(p2->getProp<PlayerProp::GLOVEPOWER>().value);
			else
			{
				// Find if String-Array
				int pos[3] = {0, 0, 0};
				pos[0] = val.findl('{');
				pos[1] = val.find('}', pos[0]);
				pos[2] = (pos[0] >= 0 && pos[1] > 0 ? strtoint(val.subString(pos[0] + 1, pos[1] - 1)) : -1);

				// Find Flag Name / Value
				CString flagName = val.subString(0, pos[0]);
				auto flagMaybe = p2->account.variables.get(flagName.toStringView());
				if (auto flag = flagMaybe.lock(); flag != nullptr)
					val = flag->get<std::string>().value_or(std::string{});

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

	// Send the profiles.
	p1->sendPacket(CString() >> (char)PLO_PROFILE << profile);
}

void ServerList::msgSVI_ERRMSG(CString& pPacket)
{
	log::printLine(log::server, "{} - [Error] {}", m_socket.getDescription(), pPacket.readString("").text());
}

void ServerList::msgSVI_VERIACC2(CString& pPacket)
{
	CString account = pPacket.readChars(pPacket.readGUChar());
	unsigned short id = pPacket.readGUShort();
	[[maybe_unused]] unsigned char type = pPacket.readGUChar();
	CString message = pPacket.readString("");

	// Get the player.
	auto player = m_server->getPlayer(id, PLTYPE_ANYPLAYER | PLTYPE_ANYNC);
	if (player == nullptr) return;

	// Overwrite the player's account name with the one from the listserver.
	player->account.name = account.toString();

	// If we did not get the success message, inform the client of his failure.
	if (message != "SUCCESS")
	{
		player->sendPacket(CString() >> (char)PLO_DISCMESSAGE << message);
		player->account.loadOnly = true; // Prevent saving of the account.
		player->disconnect();
		return;
	}

	// Send the player his account.  If it fails, disconnect him.
	if (player->sendLogin() == false)
	{
		//player->sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Failed to send login information.");
		player->account.loadOnly = true; // Prevent saving of the account.
		player->disconnect();
	}
}

void ServerList::msgSVI_FILESTART2(CString& pPacket)
{
	log::printLine(log::server, "** SVI_FILESTART2 is deprecated.  It should not be used.\n");
}

void ServerList::msgSVI_FILEDATA2(CString& pPacket)
{
	log::printLine(log::server, "** SVI_FILEDATA2 is deprecated.  It should not be used.\n");
}

void ServerList::msgSVI_FILEEND2(CString& pPacket)
{
	log::printLine(log::server, "** SVI_FILEEND2 is deprecated.  It should not be used.\n");
}

void ServerList::msgSVI_PING(CString& pPacket)
{
	// When server pings, we pong
	sendPacket(CString() >> (char)SVO_PING);
}

void ServerList::msgSVI_RAWDATA(CString& pPacket)
{
	//m_nextIsRaw = true;
	//m_rawPacketSize = pPacket.readGInt();
}

void ServerList::msgSVI_FILESTART3(CString& pPacket)
{
	unsigned char pTy = pPacket.readGUChar();
	std::filesystem::path filename{"world/global/"};
	CString blank;
	switch (pTy)
	{
		case SVF_HEAD:
			filename /= "heads";
			break;
		case SVF_BODY:
			filename /= "bodies";
			break;
		case SVF_SWORD:
			filename /= "swords";
			break;
		case SVF_SHIELD:
			filename /= "shields";
			break;
	}
	filename /= std::format("{}.partial", pPacket.readChars(pPacket.readGUChar()).toString());
	blank.save(filename.string());
}

void ServerList::msgSVI_FILEDATA3(CString& pPacket)
{
	[[maybe_unused]] unsigned char pTy = pPacket.readGUChar();
	fs::FileCategory category = fs::FileCategory::ALL;
	switch (pTy)
	{
		case SVF_HEAD:
			category = fs::FileCategory::HEAD;
			break;
		case SVF_BODY:
			category = fs::FileCategory::BODY;
			break;
		case SVF_SWORD:
			category = fs::FileCategory::SWORD;
			break;
		case SVF_SHIELD:
			category = fs::FileCategory::SHIELD;
			break;
	}

	auto filename = std::format("{}.partial", pPacket.readChars(pPacket.readGUChar()).toString());
	auto fileData = m_server->getFileSystem().info(category, filename);
	if (fileData == nullptr) return;

	CString data;
	data.load(fileData->file.string());
	data << pPacket.readChars(pPacket.bytesLeft()); // Read the rest of the packet.
	data.save(fileData->file.string());
}

void ServerList::msgSVI_FILEEND3(CString& pPacket)
{
	unsigned short pid = pPacket.readGUShort();
	unsigned char type = pPacket.readGUChar();
	unsigned char doCompress = pPacket.readGUChar();
	time_t modTime = pPacket.readGUInt5();
	unsigned int fileLength = pPacket.readGUInt5();
	CString shortName = pPacket.readString("");

	fs::FileCategory category = fs::FileCategory::ALL;
	switch (type)
	{
		case SVF_HEAD:
			category = fs::FileCategory::HEAD;
			break;
		case SVF_BODY:
			category = fs::FileCategory::BODY;
			break;
		case SVF_SWORD:
			category = fs::FileCategory::SWORD;
			break;
		case SVF_SHIELD:
			category = fs::FileCategory::SHIELD;
			break;
	}

	auto fileName = std::format("{}.partial", shortName.toString());
	auto fileData = m_server->getFileSystem().info(category, fileName);
	if (fileData == nullptr)
		return;

	// Uncompress the file if compressed.
	if (doCompress == 1)
	{
		CString fileData;
		fileData.load(fileName);
		fileData.zuncompressI(fileLength);
		fileData.save(fileName);
	}

	// Set the file mod time.
	fileData->setModTime(clock::from_time_t(modTime));

	// Rename the file.
	auto newFileName = shortName.toString();
	std::filesystem::rename(fileData->file, fileData->file.parent_path() / newFileName);

	// Set the player props.
	// TODO(joey): Confirm if we can use ANYCLIENT instead
	if (auto p = m_server->getPlayer(pid, PLTYPE_ANYPLAYER); p)
	{
		props::SetResults result;
		switch (type)
		{
			case SVF_HEAD:
				result = p->setPropWith<PlayerProp::HEADGIF>(props::SetBy::SERVER, shortName.toString());
				break;

			case SVF_BODY:
				result = p->setPropWith<PlayerProp::BODYIMG>(props::SetBy::SERVER, shortName.toString());
				break;

			case SVF_SWORD:
				result = p->setPropWith<PlayerProp::SWORDPOWER>(props::SetBy::SERVER, shortName.toString());
				break;

			case SVF_SHIELD:
				result = p->setPropWith<PlayerProp::SHIELDPOWER>(props::SetBy::SERVER, shortName.toString());
				break;
		}

		// Send the prop.
		uint8_t propId = result.resultPropIds.front();
		CString prop = p->getProp((PlayerProp)propId)->serialize();
		if (result.resultFlags.test(props::SetResults::sendToAll))
			m_server->sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)pid >> (char)propId << prop);
		if (auto player = std::dynamic_pointer_cast<PlayerClient>(p); p && result.resultFlags.test(props::SetResults::sendToLevel))
			m_server->sendPacketToNearby(CString() >> (char)PLO_OTHERPLPROPS >> (short)pid >> (char)propId << prop, player->account.character.getGlobalPosition(), player->getLevel(), {pid});
		if (result.resultFlags.test(props::SetResults::sendToSource))
			p->sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)propId << prop);
	}
}

void ServerList::msgSVI_SERVERINFO(CString& pPacket)
{
	int pid = pPacket.readGUShort();
	CString serverpacket = pPacket.readString("");

	// A hack to allow v5 clients to serverwarp to servers
	auto player = m_server->getPlayer(pid, PLTYPE_ANYCLIENT);
	if (player && player->getVersion() >= CLVER_2_1)
		player->sendPacket(CString() >> (char)PLO_SERVERWARP << serverpacket);
}

void ServerList::msgSVI_REQUESTTEXT(CString& pPacket)
{
	unsigned short playerId = pPacket.readGUShort();
	CString message = pPacket.readString("");

	CString data = message.guntokenize();
	std::vector<CString> params = data.tokenize("\n");

	CString weapon = data.readString("\n");
	CString type = data.readString("\n");
	CString option = data.readString("\n");
	CString paramsData = data.readString("");

	auto player = m_server->getPlayer(playerId);
	if (player != nullptr)
	{
		if (params.size() > 3)
		{
			if (params[0] == "GraalEngine")
			{
				if (params[1] == "irc")
				{
					// Listserver can confirm this stuff, and use it for having a count of players in channels
					weapon = player->isClient() ? "-Serverlist_Chat" : "GraalEngine";

					if (params[2] == "join")
					{
						CString channel = params[3].guntokenize();
						if (player->addChatChannel(channel.text()))
							player->sendPacket(CString() >> (char)PLO_SERVERTEXT << weapon << ",irc,join," << params[3].gtokenize());
					}
					else if (params[2] == "part")
					{
						CString channel = params[3].guntokenize();
						if (player->inChatChannel(channel.text()))
							player->sendPacket(CString() >> (char)PLO_SERVERTEXT << weapon << ",irc,part," << params[3].gtokenize());
					}
				}
			}
		}
	}

	/*
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
		m_server->sendPacketToAll(CCommon::triggerAction(0, 0, "clientside", "-Serverlist_v4", serverIds.gtokenizeI()));
		m_server->sendPacketToAll(CCommon::triggerAction(0, 0, "clientside", "-Serverlist_v4", serverPCount.gtokenizeI()));
		serverIds.clear();
		serverNames.clear();
		serverPCount.clear();
	}
	*/

	player = m_server->getPlayer(playerId, PLTYPE_ANYPLAYER);
	if (player)
	{
		if (type == "pmserverplayers")
		{
			player->updatePMPlayers(option, paramsData);
		}
		else
		{
			//log::printLine(log::server, "[OUT] [RequestText] %s\n", message.text());

			if (player->getVersion() >= CLVER_4_0211 || player->getVersion() > RCVER_1_1)
				player->sendPacket(CString() >> (char)PLO_SERVERTEXT << message);
		}
	}
}

void ServerList::msgSVI_SENDTEXT(CString& pPacket)
{
	CString data = pPacket.readString("");
	handleText(data);
}

void ServerList::msgSVI_PMPLAYER(CString& pPacket)
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

	auto p = m_server->getPlayer(account2, PLTYPE_ANYPLAYER);
	if (p)
	{
		p->addPMServer(servername);
		p->updatePMPlayers(servername, player);
		auto tmpPlyr = p->getExternalPlayer(account);
		p->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)tmpPlyr->getId() << pmMessageType << message3, true);
	}

	message2 = "";
}

void ServerList::msgSVI_ASSIGNPCID(CString& pPacket)
{
	uint16_t id = pPacket.readGUShort();
	uint8_t type = pPacket.readGUChar();
	CString pcId = pPacket.readChars(pPacket.readGUChar());

	// Get the player, this should be a player who has not been loaded with the playerid of `id`
	auto player = m_server->getPlayer(id, type);
	if (!player || player->isLoaded())
		return;

	player->setDeviceId(std::stoll(pcId.text()));
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
