#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <array>
#include <type_traits>

#include <IEnums.h>
#include <IUtil.h>

#include <IConfig.h>

#include <Account.h>
#include <Server.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <object/Weapon.h>
#include <player/PlayerClient.h> // Need to remove once we don't need to use std::dynamic_pointer_cast.
#include <level/Level.h>
#include <level/Map.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

#ifdef PACKETLOGGING
#define FOR_OUTPUT_PACKETS(DO) \
	DO(PLO_LEVELBOARD) \
	DO(PLO_LEVELLINK) \
	DO(PLO_BADDYPROPS) \
	DO(PLO_NPCPROPS) \
	DO(PLO_LEVELCHEST) \
	DO(PLO_LEVELSIGN) \
	DO(PLO_LEVELNAME) \
	DO(PLO_BOARDMODIFY) \
	DO(PLO_OTHERPLPROPS) \
	DO(PLO_PLAYERPROPS) \
	DO(PLO_ISLEADER) \
	DO(PLO_BOMBADD) \
	DO(PLO_BOMBDEL) \
	DO(PLO_TOALL) \
	DO(PLO_PLAYERWARP) \
	DO(PLO_WARPFAILED) \
	DO(PLO_DISCMESSAGE) \
	DO(PLO_HORSEADD) \
	DO(PLO_HORSEDEL) \
	DO(PLO_ARROWADD) \
	DO(PLO_FIRESPY) \
	DO(PLO_THROWCARRIED) \
	DO(PLO_ITEMADD) \
	DO(PLO_ITEMDEL) \
	DO(PLO_NPCMOVED) \
	DO(PLO_SIGNATURE) \
	DO(PLO_NPCACTION) \
	DO(PLO_BADDYHURT) \
	DO(PLO_FLAGSET) \
	DO(PLO_NPCDEL) \
	DO(PLO_FILESENDFAILED) \
	DO(PLO_FLAGDEL) \
	DO(PLO_SHOWIMG) \
	DO(PLO_NPCWEAPONADD) \
	DO(PLO_NPCWEAPONDEL) \
	DO(PLO_RC_ADMINMESSAGE) \
	DO(PLO_EXPLOSION) \
	DO(PLO_PRIVATEMESSAGE) \
	DO(PLO_PUSHAWAY) \
	DO(PLO_LEVELMODTIME) \
	DO(PLO_HURTPLAYER) \
	DO(PLO_STARTMESSAGE) \
	DO(PLO_NEWWORLDTIME) \
	DO(PLO_DEFAULTWEAPON) \
	DO(PLO_HASNPCSERVER) \
	DO(PLO_FILEUPTODATE) \
	DO(PLO_HITOBJECTS) \
	DO(PLO_STAFFGUILDS) \
	DO(PLO_TRIGGERACTION) \
	DO(PLO_PLAYERWARP2) \
	DO(PLO_RC_ACCOUNTADD) \
	DO(PLO_RC_ACCOUNTSTATUS) \
	DO(PLO_RC_ACCOUNTNAME) \
	DO(PLO_RC_ACCOUNTDEL) \
	DO(PLO_RC_ACCOUNTPROPS) \
	DO(PLO_ADDPLAYER) \
	DO(PLO_DELPLAYER) \
	DO(PLO_RC_ACCOUNTPROPSGET) \
	DO(PLO_RC_ACCOUNTCHANGE) \
	DO(PLO_RC_PLAYERPROPSCHANGE) \
	DO(PLO_UNKNOWN60) \
	DO(PLO_RC_SERVERFLAGSGET) \
	DO(PLO_RC_PLAYERRIGHTSGET) \
	DO(PLO_RC_PLAYERCOMMENTSGET) \
	DO(PLO_RC_PLAYERBANGET) \
	DO(PLO_RC_FILEBROWSER_DIRLIST) \
	DO(PLO_RC_FILEBROWSER_DIR) \
	DO(PLO_RC_FILEBROWSER_MESSAGE) \
	DO(PLO_LARGEFILESTART) \
	DO(PLO_LARGEFILEEND) \
	DO(PLO_RC_ACCOUNTLISTGET) \
	DO(PLO_RC_PLAYERPROPS) \
	DO(PLO_RC_PLAYERPROPSGET) \
	DO(PLO_RC_ACCOUNTGET) \
	DO(PLO_RC_CHAT) \
	DO(PLO_PROFILE) \
	DO(PLO_RC_SERVEROPTIONSGET) \
	DO(PLO_RC_FOLDERCONFIGGET) \
	DO(PLO_NC_CONTROL) \
	DO(PLO_NPCSERVERADDR) \
	DO(PLO_NC_LEVELLIST) \
	DO(PLO_UNKNOWN81) \
	DO(PLO_SERVERTEXT) \
	DO(PLO_UNKNOWN83) \
	DO(PLO_LARGEFILESIZE) \
	DO(PLO_RAWDATA) \
	DO(PLO_BOARDPACKET) \
	DO(PLO_FILE) \
	DO(PLO_RC_MAXUPLOADFILESIZE) \
	DO(PLO_UNKNOWN104) \
	DO(PLO_UPDATEPACKAGESIZE) \
	DO(PLO_UPDATEPACKAGEDONE) \
	DO(PLO_BOARDLAYER) \
	DO(PLO_UNKNOWN109) \
	DO(PLO_UNKNOWN111) \
	DO(PLO_UNKNOWN124) \
	DO(PLO_NPCBYTECODE) \
	DO(PLO_UNKNOWN132) \
	DO(PLO_UNKNOWN133) \
	DO(PLO_GANISCRIPT) \
	DO(PLO_NPCWEAPONSCRIPT) \
	DO(PLO_NPCDEL2) \
	DO(PLO_HIDENPCS) \
	DO(PLO_SAY2) \
	DO(PLO_FREEZEPLAYER2) \
	DO(PLO_UNFREEZEPLAYER) \
	DO(PLO_SETACTIVELEVEL) \
	DO(PLO_NC_NPCATTRIBUTES) \
	DO(PLO_NC_NPCADD) \
	DO(PLO_NC_NPCDELETE) \
	DO(PLO_NC_NPCSCRIPT) \
	DO(PLO_NC_NPCFLAGS) \
	DO(PLO_NC_CLASSGET) \
	DO(PLO_NC_CLASSADD) \
	DO(PLO_NC_LEVELDUMP) \
	DO(PLO_MOVE) \
	DO(PLO_UNKNOWN166) \
	DO(PLO_NC_WEAPONLISTGET) \
	DO(PLO_UNKNOWN168) \
	DO(PLO_UNKNOWN169) \
	DO(PLO_GHOSTMODE) \
	DO(PLO_BIGMAP) \
	DO(PLO_MINIMAP) \
	DO(PLO_GHOSTTEXT) \
	DO(PLO_GHOSTICON) \
	DO(PLO_SHOOT) \
	DO(PLO_FULLSTOP) \
	DO(PLO_FULLSTOP2) \
	DO(PLO_SERVERWARP) \
	DO(PLO_RPGWINDOW) \
	DO(PLO_STATUSLIST) \
	DO(PLO_UNKNOWN181) \
	DO(PLO_LISTPROCESSES) \
	DO(PLO_UNKNOWN183) \
	DO(PLO_UNKNOWN184) \
	DO(PLO_UNKNOWN185) \
	DO(PLO_UNKNOWN186) \
	DO(PLO_UPDATEPACKAGEISUPDATED) \
	DO(PLO_NC_CLASSDELETE) \
	DO(PLO_MOVE2) \
	DO(PLO_UNKNOWN190) \
	DO(PLO_SHOOT2) \
	DO(PLO_NC_WEAPONGET) \
	DO(PLO_UNKNOWN193) \
	DO(PLO_CLEARWEAPONS) \
	DO(PLO_UNKNOWN195) \
	DO(PLO_UNKNOWN197) \
	DO(PLO_UNKNOWN198) \
	DO(PLO_SET_ENC_KEY) \
	DO(PLO_BUNDLE)
#define FILL_OUTPUT_ARRAY(name) names[(uint8_t)name] = #name;

constexpr std::array<std::string, 255> FillPutputPacketNamesArray()
{
	std::array<std::string, 255> names;
	names.fill("(unknown packet)");
	FOR_OUTPUT_PACKETS(FILL_OUTPUT_ARRAY)
	return names;
}

std::array<std::string, 255> OutputPacketNamesArray = FillPutputPacketNamesArray();
#endif

///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////

using PacketHandleFunc = HandlePacketResult(Player::*)(CString&);
using PacketHandleArray = std::array<PacketHandleFunc, 256>;

static PacketHandleArray GeneratePacketHandlers()
{
	PacketHandleArray handlers{};
	handlers.fill(&Player::msgPLI_NULL);

	handlers[PLI_PLAYERPROPS] = &Player::msgPLI_PLAYERPROPS;
	handlers[PLI_TOALL] = &Player::msgPLI_TOALL;
	handlers[PLI_PRIVATEMESSAGE] = &Player::msgPLI_PRIVATEMESSAGE;
	handlers[PLI_PACKETCOUNT] = &Player::msgPLI_PACKETCOUNT;
	handlers[PLI_LANGUAGE] = &Player::msgPLI_LANGUAGE;
	handlers[PLI_PROFILEGET] = &Player::msgPLI_PROFILEGET;
	handlers[PLI_PROFILESET] = &Player::msgPLI_PROFILESET;
	handlers[PLI_REQUESTTEXT] = &Player::msgPLI_REQUESTTEXT;
	handlers[PLI_SENDTEXT] = &Player::msgPLI_SENDTEXT;

	return handlers;
}

///////////////////////////////////////////////////////////////////////////////

HandlePacketResult Player::handlePacket(std::optional<uint8_t> id, CString& packet)
{
	static PacketHandleArray PacketHandlers = GeneratePacketHandlers();

	m_lastData = time(0);

	auto handle = id.has_value() ? PacketHandlers[id.value()] : &Player::msgPLI_NULL;
	return (this->*handle)(packet);
}

///////////////////////////////////////////////////////////////////////////////

Player::Player(CSocket* pSocket, PlayerID pId)
	: m_playerSock(pSocket), m_id(pId), m_fileQueue(pSocket)
{
	m_server = BabyDI::Get<Server>();
	m_lastData = time(0);
	m_serverName = m_server->getName();
	m_modTime.fill(clock::time_point::min());
	m_savedModTime.fill(clock::time_point::min());

	srand((unsigned int)time(0));
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
		if (isClient() && !account.loadOnly)
			m_server->getAccountLoader().saveAccount(account);

		// Announce our departure to other clients.
		if (!isNC())
		{
			m_server->sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id >> (char)PlayerProp::PCONNECTED, this);
			m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_DELPLAYER >> (short)m_id, this);
		}

		if (!account.name.empty())
		{
			if (isRC())
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "RC Disconnected: " << account.name, this);
			else if (isNC())
				m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << "NC Disconnected: " << account.name, this);
		}

		// Log.
		if (isClient())
			log::printLine(log::server, ":: Client disconnected: {}", account.name);
		else if (isRC())
			log::printLine(log::server, ":: RC disconnected: {}", account.name);
		else if (isNC())
			log::printLine(log::server, ":: NC disconnected: {}", account.name);
	}

	if (m_playerSock)
		delete m_playerSock;
	m_playerSock = nullptr;
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

	// Hold ourself just in case we are deleted.
	auto self = shared_from_this();

	// Do the main function.
	doMain();
	if (m_playerSock != nullptr)
		m_server->getSocketManager().updateSingle(this, false, true);

	return true;
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
void Player::doMain()
{
	// Process the packet data.
	processBuffer(m_recvBuffer);
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
	
	m_fileQueue.sendCompress();

	return true;
}

void Player::disconnect()
{
	m_server->deletePlayer(shared_from_this());
	//m_server->getSocketManager()->unregisterSocket(this);
}

void Player::sendPacket(CString pPacket, bool appendNL)
{
	// empty buffer?
	if (pPacket.isEmpty())
		return;

#ifdef PACKETLOGGING
	// This will suck as long as we have gs2lib.
	uint32_t pid = static_cast<uint32_t>(static_cast<uint8_t>(pPacket[0]) - 32);
	log::printLine(log::networkdump, "< Out Packet: [{}] {} ({} bytes)", pid, OutputPacketNamesArray[pid], pPacket.length());
	log::print(log::networkdump, "{}", pPacket.text());
	if (pPacket[pPacket.length() - 1] != '\n')
		log::print(log::networkdump, "\n");
	for (int i = 0; i < pPacket.length(); ++i)
		log::print(log::networkdump, "{:02x} ", (unsigned char)((pPacket.text())[i]));
	log::print(log::networkdump, "\n\n");
#endif

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
	if (auto client = std::dynamic_pointer_cast<PlayerClient>(shared_from_this()); isClient() && client != nullptr)
	{
		client->m_knownFiles.insert(pFile.toString());
	}

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
	auto current_path = std::filesystem::current_path().string() + (char)std::filesystem::path::preferred_separator;
	if (path.find(current_path.c_str()) != -1)
		path.removeI(0, current_path.length());

	// Send the file now.
	return this->sendFile(path, pFile);
}

bool Player::sendFile(const CString& pPath, const CString& pFile)
{
	CString filepath = CString() << pPath << pFile;
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
		log::printLine(log::server, "[WARNING] Sending a large file (over 3MB): {}", pFile);

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

///////////////////////////////////////////////////////////////////////////////

bool Player::handleLogin(CString& pPacket)
{
	return false;
}

bool Player::sendLogin()
{
	// Load the account.
	m_server->getAccountLoader().loadAccount(account.name, account);

	// Check if they are ip-banned or not.
	if (m_server->isIpBanned(m_playerSock->getRemoteIp()) && !account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You have been banned from this server.");
		return false;
	}

	// Check to see if the player is banned or not.
	if (account.banned && !account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You have been banned.  Reason: " << string::join(string::fromCSV(account.banReason), "\r"));
		return false;
	}

	// If we are an RC, check to see if we can log in.
	if (isRC() || isNC())
	{
		// Check and see if we are allowed in.
		if (!isStaff() || !isAdminIp())
		{
			log::printLine(log::rc, "Attempted RC login by {}.", account.name);
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You do not have RC rights.");
			return false;
		}
	}

	// Check to see if we can log in if we are a client.
	if (isClient())
	{
		// Staff only.
		if (m_server->getSettings().getBool("onlystaff", false) && !isStaff())
		{
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "This server is currently restricted to staff only.");
			return false;
		}

		// Check and see if we are allowed in.
		if (!isAdminIp())
		{
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Your IP doesn't match one of the allowed IPs for this account.");
			return false;
		}
	}

	// Server Signature
	// 0x49 (73) is used to tell the client that more than eight
	// players will be playing.
	sendPacket(CString() >> (char)PLO_SIGNATURE >> (char)73);

	// TODO: Don't hardcode this.
	if (m_server->getName().findi("login") > -1)
	{
		sendPacket(CString() >> (char)PLO_FULLSTOP);
		sendPacket(CString() >> (char)PLO_GHOSTICON >> (char)1);
	}

	if (isClient())
	{
		// Tell the client if we have an npc-server, which disables certain features on the client (like sending NPC prop modifications).
		// Later clients don't send this because all client-side functionality was removed.
		// There isn't any harm in always sending it, though.
		if (m_server->isNpcServerEnabled())
			sendPacket(CString() >> (char)PLO_HASNPCSERVER);

		// This seems to inform the client that they have logged in.
		sendPacket(CString() >> (char)PLO_UNKNOWN168);
	}

	// Check if the account is already in use.
	bool isGuest = account.loadOnly && account.communityName == "guest";
	if (!isGuest)
	{
		auto& playerList = m_server->getPlayerList();
		for (auto& [pid, player] : playerList)
		{
			std::string otherAccount = player->account.name;
			PlayerID otherID = player->getId();

			int meClient = ((m_type & PLTYPE_ANYCLIENT) ? 0 : ((m_type & PLTYPE_ANYRC) ? 1 : 2));
			int themClient = ((player->getType() & PLTYPE_ANYCLIENT) ? 0 : ((player->getType() & PLTYPE_ANYRC) ? 1 : 2));

			if (string::comparei(otherAccount, account.name) == 0 && meClient == themClient && otherID != m_id)
			{
				if ((int)difftime(time(0), player->getLastData()) > 30)
				{
					player->sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Someone else has logged into your account.");
					player->disconnect();
				}
				else
				{
					sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Account is already in use.");
					return false;
				}
			}
		}
	}

	// TODO(joey): Placing this here so warp doesn't queue events for this player before
	//	the login is finished. The server should get first dibs on the player.
	m_server->recordPlayerLoggedIn(shared_from_this());

	// Set loaded to true so our account is saved when we leave.
	// This also lets us send data.
	m_loaded = true;

	return true;
}

///////////////////////////////////////////////////////////////////////////////

// Exchange props with everybody on the server.
void Player::exchangeMyPropsWithOthers()
{
	// RC props are sent differently.
	CString myRCProps;
	myRCProps >> (char)PLO_ADDPLAYER >> (short)getId() >> (char)account.name.length() << account.name
		>> (char)PlayerProp::CURLEVEL << getProp<PlayerProp::CURLEVEL>().serialize()
		>> (char)PlayerProp::PSTATUSMSG << getProp<PlayerProp::PSTATUSMSG>().serialize()
		>> (char)PlayerProp::NICKNAME << getProp<PlayerProp::NICKNAME>().serialize()
		>> (char)PlayerProp::COMMUNITYNAME << getProp<PlayerProp::COMMUNITYNAME>().serialize();

	// Get our client props.
	CString myClientProps = CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id << (isClient() ? getPropsPacketFromList(loginPropsClientOthers) : getPropsPacketFromList(loginPropsRC));

	CString rcsOnline;
	auto& playerList = m_server->getPlayerList();
	for (const auto& [pid, player] : playerList)
	{
		if (player.get() == this) continue;

		// Don't send npc-control players to others
		if (player->isNC()) continue;

		// Send the other player my props.
		// Send my flags to the npcserver.
		player->sendPacket(player->isClient() ? myClientProps : myRCProps);

		// Add Player / RC.
		if (isClient())
			sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)player->getId() << (player->isClient() ? player->getPropsPacketFromList(loginPropsClientOthers) : player->getPropsPacketFromList(loginPropsRC)));
		else
		{
			// TODO: Make sure this works when levels get fixed.
			// Level name.  If no level, send an empty space.
			//CString levelName = (player-getLevel() ? player->getLevel()->getLevelName() : " ");
			CString levelName = player->account.level;

			// Get the other player's RC props.
			sendPacket(CString() >> (char)PLO_ADDPLAYER >> (short)player->getId() >> (char)player->account.name.length() << player->account.name
				>> (char)PlayerProp::CURLEVEL << player->getProp<PlayerProp::CURLEVEL>().serialize()
				>> (char)PlayerProp::PSTATUSMSG << player->getProp<PlayerProp::PSTATUSMSG>().serialize()
				>> (char)PlayerProp::NICKNAME << player->getProp<PlayerProp::NICKNAME>().serialize()
				>> (char)PlayerProp::COMMUNITYNAME << player->getProp<PlayerProp::COMMUNITYNAME>().serialize());
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

bool Player::isAdminIp()
{
	for (const auto& ipMask : account.adminIpRange)
	{
		if (ipMask == "0.0.0.0")
			return true;
		if (CString(account.ipAddress).match(ipMask))
			return true;
	}
	return false;
}

bool Player::isStaff()
{
	return m_server->isStaff(account.name);
}

/*
	Player: Set Properties
*/

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
		account.character.nickName = pNickName.toString();
		this->m_guild = guild;
		return;
	}

	// If a player has put a * before his nick, remove it.
	while (!nick.isEmpty() && nick[0] == '*')
		nick.removeI(0, 1);

	// If the nickname is now empty, set it to unknown.
	if (nick.isEmpty()) nick = "unknown";

	// If the nickname is equal to the account name, add the *.
	if (nick == account.name)
		newNick = CString("*");

	// Add the nick name.
	newNick << nick;

	// If a guild was specified, add the guild.
	if (guild.length() != 0)
	{
		// Read the guild list.
		FileSystem guildFS;
		guildFS.addDir("guilds");
		CString guildList = guildFS.load(CString() << "guild" << guild << ".txt");
		if (guildList.isEmpty())
			guildList = guildFS.load(CString() << "guild" << guild.replaceAll(" ", "_") << ".txt");

		// Find the account in the guild list.
		// Will also return -1 if the guild does not exist.
		if (guildList.findi(std::string_view{ account.name }) != -1)
		{
			guildList.setRead(guildList.findi(std::string_view{ account.name }));
			CString line = guildList.readString("\n");
			line.removeAllI("\r");
			if (line.find(":") != -1)
			{
				std::vector<CString> line2 = line.tokenize(":");
				if ((line2[1])[0] == '*') line2[1].removeI(0, 1);
				if ((line2[1]) == nick) // Use nick instead of newNick because nick doesn't include the *
				{
					newNick << " (" << guild << ")";
					account.character.nickName = newNick.toString();
					this->m_guild = guild;
					return;
				}
			}
			else
			{
				newNick << " (" << guild << ")";
				account.character.nickName = newNick.toString();
				this->m_guild = guild;
				return;
			}
		}
		else
			account.character.nickName = newNick.toString();

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
				CString() >> (char)SVO_VERIGUILD >> (short)m_id >> (char)account.name.length() << account.name >> (char)newNick.length() << newNick >> (char)guild.length() << guild);
		}
	}
	else
	{
		// Save it.
		account.character.nickName = newNick.toString();
		this->m_guild.clear();
	}

	if (m_isExternal)
	{
		account.character.nickName = pNickName.toString();
	}
}

void Player::setChat(const CString& pChat)
{
	sendPropsFromResults(setPropWith<PlayerProp::CURCHAT>(props::SetBy::SERVER, pChat.toString()));
}

///////////////////////////////////////////////////////////////////////////////

bool Player::deleteFlag(std::string_view flagName, bool sendToPlayer)
{
	bool result = account.variables.remove(flagName);

	if (sendToPlayer)
	{
		sendPacket(CString() >> (char)PLO_FLAGDEL << flagName);
	}

	return result;
}

bool Player::setFlag(std::string_view flagPair, bool sendToPlayers)
{
	if (!flagPair.contains('='))
		return setFlag(flagPair, std::nullopt, sendToPlayers);

	auto separator = flagPair.find('=');
	auto flagName = string::trim(flagPair.substr(0, separator));
	auto flagValue = string::trim(flagPair.substr(separator + 1));
	return setFlag(flagName, std::string{ flagValue }, sendToPlayers);
}

bool Player::setFlag(std::string_view flagName, std::optional<std::string> flagValue, bool sendToPlayer)
{
	if (!flagValue.has_value())
	{
		account.variables.add(flagName, true);
		sendPacket(CString() >> (char)PLO_FLAGSET << flagName);
	}
	else
	{
		if (flagValue.value().empty())
			return deleteFlag(flagName, sendToPlayer);

		account.variables.add(flagName, flagValue.value());
		sendPacket(CString() >> (char)PLO_FLAGSET << flagName << "=" << flagValue.value());
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Player::addWeapon(LevelItemType defaultWeapon)
{
	// Allow Default Weapons..?
	CSettings& settings = m_server->getSettings();
	if (!settings.getBool("defaultweapons", true))
		return false;

	auto weapon = m_server->getWeapon(LevelItem::getItemName(defaultWeapon));
	if (!weapon)
	{
		weapon = std::make_shared<Weapon>(defaultWeapon);
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
	if (!account.hasWeapon(weapon->getName()))
	{
		account.weapons.push_back(weapon->getName());
		if (m_id == 0) return true;

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
	if (std::erase(account.weapons, weapon->getName()) != 0)
	{
		if (m_id == 0) return true;

		// Send delete notice.
		sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << weapon->getName());
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

CString Player::translate(const CString& pKey) const
{
	return m_server->TS_Translate(account.language, pKey);
}

///////////////////////////////////////////////////////////////////////////////

void Player::sendPrivateMessage(PlayerID from, std::string_view message)
{
	if (message.empty())
		return;

	auto lines = string::splitHard(message, "\n"sv);
	auto finalMessage = string::toCSV(lines, true);

	sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)from << finalMessage);
}

///////////////////////////////////////////////////////////////////////////////

bool Player::setLevel(const CString& pLevelName, time_t modTime)
{
	// TODO: Check if level exists.
	account.level = pLevelName.toString();
	return true;
}

/*
	Player: Packet functions
*/
HandlePacketResult Player::msgPLI_NULL(CString& pPacket)
{
	pPacket.setRead(0);
	printf("Unknown Player Packet: %u (%s)\n", (unsigned int)pPacket.readGUChar(), pPacket.text() + 1);
	for (int i = 0; i < pPacket.length(); ++i) printf("%02x ", (unsigned char)((pPacket.text())[i]));
	printf("\n");

	// If we are getting a whole bunch of invalid packets, something went wrong.  Disconnect the player.
	InvalidPackets++;
	if (InvalidPackets > 5)
	{
		log::printLine(log::server, "Player {} is sending invalid packets.", account.character.nickName);
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Disconnected for sending invalid packets.");
		return HandlePacketResult::Failed;
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_LOGIN(CString& pPacket)
{
#if defined(WOLFSSL_ENABLED)
	if (!this->m_playerSock->webSocket && pPacket.findi("GET /") > -1 && pPacket.findi("HTTP/1.1\r\n") > -1)
	{
		return msgWebSocketInit(pPacket);
	}
#endif

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgWebSocketInit(CString& pPacket)
{
#if defined(WOLFSSL_ENABLED)
	CString webSocketKeyHeader = "Sec-WebSocket-Key:";
	if (pPacket.findi(webSocketKeyHeader) < 0)
	{
		CString simpleHtml = CString() << "<html><head><title>" APP_VENDOR " " APP_NAME " v" APP_VERSION "</title></head><body><h1>Welcome to " << m_server->getSettings().getStr("name") << "!</h1>" << m_server->getServerMessage().replaceAll("my server", m_server->getSettings().getStr("name")).text() << "<p style=\"font-style: italic;font-weight: bold;\">Powered by " APP_VENDOR " " APP_NAME "<br/>Programmed by " << CString(APP_CREDITS) << "</p></body></html>";
		CString webResponse = CString() << "HTTP/1.1 200 OK\r\nServer: " APP_VENDOR " " APP_NAME " v" APP_VERSION "\r\nContent-Length: " << CString(simpleHtml.length()) << "\r\nContent-Type: text/html\r\n\r\n"
			<< simpleHtml << "\r\n";
		unsigned int dsize = webResponse.length();
		this->m_playerSock->sendData(webResponse.text(), &dsize);
		return HandlePacketResult::Bubble;
	}
	this->m_playerSock->webSocket = true;
	// Get the WebSocket handshake key
	pPacket.setRead(pPacket.findi(webSocketKeyHeader));
	CString webSocketKey = pPacket.readString("\r").subString(webSocketKeyHeader.length() + 1).trimI();

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
#endif
	return HandlePacketResult::Bubble;
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

HandlePacketResult Player::msgPLI_PLAYERPROPS(CString& pPacket)
{
	setPropsFromPacket(pPacket, props::SetBy::CLIENT);
	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_TOALL(CString& pPacket)
{
	// Check if the player is in a jailed level.
	std::vector<CString> jailList = m_server->getSettings().getStr("jaillevels").tokenize(",");
	if (std::find_if(jailList.begin(), jailList.end(), [&levelName = account.level](CString& level)
					 {
						 return level.trim() == levelName;
					 }) != jailList.end())
		return HandlePacketResult::Handled;

	CString message = pPacket.readString(pPacket.readGUChar());

	// Word filter.
	int filter = m_server->getWordFilter().apply(this, message, FILTER_CHECK_TOALL);
	if (filter & FILTER_ACTION_WARN)
	{
		setChat(message);
		return HandlePacketResult::Handled;
	}

	for (auto& [pid, player]: m_server->getPlayerList())
	{
		if (pid == m_id) continue;

		// See if the player is allowing toalls.
		auto flags = player->getProp<PlayerProp::ADDITFLAGS>().value;
		if (flags & PLFLAG_NOTOALL) continue;

		player->sendPacket(CString() >> (char)PLO_TOALL >> (short)m_id >> (char)message.length() << message);
	}
	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_PRIVATEMESSAGE(CString& pPacket)
{
	// Get the players this message was addressed to.
	std::vector<PlayerID> pmPlayers;
	auto pmPlayerCount = pPacket.readGUShort();
	for (auto i = 0; i < pmPlayerCount; ++i)
		pmPlayers.push_back(static_cast<PlayerID>(pPacket.readGUShort()));

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
		return HandlePacketResult::Handled;
	}

	// Word filter.
	pmMessage.guntokenizeI();
	if (isClient())
	{
		int filter = m_server->getWordFilter().apply(this, pmMessage, FILTER_CHECK_PM);
		if (filter & FILTER_ACTION_WARN)
		{
			sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE << "Word Filter:\xa7Your PM could not be sent because it was caught by the word filter.");
			return HandlePacketResult::Handled;
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
				log::printLine(log::server, "Sending PM to global player: {}.", pmPlayer->account.character.nickName);
				pmMessage.guntokenizeI();
				pmExternalPlayer(pmPlayer->getServerName(), pmPlayer->account.name, pmMessage);
				pmMessage.gtokenizeI();
			}
		}
		else
		{
			auto pmPlayer = m_server->getPlayer(pmPlayerId, PLTYPE_ANYPLAYER | PLTYPE_NPCSERVER);
			if (pmPlayer == nullptr || pmPlayer.get() == this) continue;

			// Don't send to people who don't want mass messages.
			if (pmPlayerCount != 1 && (pmPlayer->getProp<PlayerProp::ADDITFLAGS>().value & PLFLAG_NOMASSMESSAGE))
				continue;

			// Send the message.
			pmPlayer->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)m_id << pmMessageType << pmMessage);
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_PACKETCOUNT(CString& pPacket)
{
	unsigned short count = pPacket.readGUShort();
	if (count != PacketCount || PacketCount > 10000)
	{
		log::printLine(log::server, ":: Warning - Player {} had an invalid packet count.", account.name);
	}
	PacketCount = 0;

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_LANGUAGE(CString& pPacket)
{
	CString language = pPacket.readString("");
	if (language.isEmpty())
		language = "English";
	account.language = language.toString();
	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_PROFILEGET(CString& pPacket)
{
	// Send the packet ID for backwards compatibility.
	m_server->getServerList().sendPacket(CString() >> (char)SVO_GETPROF >> (short)m_id << pPacket);
	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_PROFILESET(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc != account.name) return HandlePacketResult::Handled;

	// Old gserver would send the packet ID with pPacket so, for
	// backwards compatibility, do that here.
	m_server->getServerList().sendPacket(CString() >> (char)SVO_SETPROF << pPacket);
	return HandlePacketResult::Handled;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
