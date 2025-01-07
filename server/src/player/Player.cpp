#include <IDebug.h>

#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <array>
#include <type_traits>

#include <IEnums.h>
#include <IUtil.h>

#include "IConfig.h"

#include "Account.h"
#include "Server.h"
#include "object/NPC.h"
#include "object/Player.h"
#include "object/PlayerClient.h" // Need to remove once we don't need to use std::dynamic_pointer_cast.
#include "object/Weapon.h"
#include "level/Level.h"
#include "level/Map.h"
#include "utilities/StringUtils.h"

using namespace graal::utilities;

/*
	Logs
*/
#define serverlog m_server->getServerLog()
#define rclog m_server->getRCLog()


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

Player::Player(CSocket* pSocket, uint16_t pId)
	: m_playerSock(pSocket), m_id(pId), m_fileQueue(pSocket)
{
	m_lastData = time(0);
	m_serverName = m_server->getName();

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
			m_server->sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id >> (char)PLPROP_PCONNECTED, this);
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
			serverlog.out(":: Client disconnected: %s\n", account.name.c_str());
		else if (isRC())
			serverlog.out(":: RC disconnected: %s\n", account.name.c_str());
		else if (isNC())
			serverlog.out(":: NC disconnected: %s\n", account.name.c_str());
	}

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

CString Player::getProp(int pPropId) const
{
	CString packet;
	getProp(packet, pPropId);
	return packet;
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
		serverlog.out("[WARNING] Sending a large file (over 3MB): %s\n", pFile.text());

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
			rclog.out("Attempted RC login by %s.\n", account.name.c_str());
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You do not have RC rights.");
			return false;
		}
	}

	// NPC-Control login, then return.
	if (isNC())
		return sendLoginNC();

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
#ifdef V8NPCSERVER
		// If we have an NPC Server, send this to prevent clients from sending
		// npc props it modifies.
		//
		// NOTE: This may have been deprecated after v5/v6, don't see it in iLogs
		sendPacket(CString() >> (char)PLO_HASNPCSERVER);
#endif
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
	m_server->playerLoggedIn(shared_from_this());

	// Set loaded to true so our account is saved when we leave.
	// This also lets us send data.
	m_loaded = true;

	if (isNC())
		return sendLoginNC();

	return true;
}

///////////////////////////////////////////////////////////////////////////////

// Exchange props with everybody on the server.
void Player::exchangeMyPropsWithOthers()
{
	// RC props are send differently.
	CString myRCProps;
	myRCProps >> (char)PLO_ADDPLAYER >> (short)getId() >> (char)account.name.length() << account.name >> (char)PLPROP_CURLEVEL << getProp(PLPROP_CURLEVEL) >> (char)PLPROP_PSTATUSMSG << getProp(PLPROP_PSTATUSMSG) >> (char)PLPROP_NICKNAME << getProp(PLPROP_NICKNAME) >> (char)PLPROP_COMMUNITYNAME << getProp(PLPROP_COMMUNITYNAME);

	// Get our client props.
	CString myClientProps = (isClient() ? getProps(loginPropsClientOthers) : getProps(loginPropsRC));

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
			sendPacket(player->isClient() ? player->getProps(loginPropsClientOthers) : player->getProps(loginPropsRC));
		else
		{
			// TODO: Make sure this works when levels get fixed.
			// Level name.  If no level, send an empty space.
			//CString levelName = (player-getLevel() ? player->getLevel()->getLevelName() : " ");
			CString levelName = player->account.level;

			// Get the other player's RC props.
			sendPacket(CString() >> (char)PLO_ADDPLAYER >> (short)player->getId() >> (char)player->account.name.length() << player->account.name >> (char)PLPROP_CURLEVEL >> (char)levelName.length() << levelName >> (char)PLPROP_PSTATUSMSG << player->getProp(PLPROP_PSTATUSMSG) >> (char)PLPROP_NICKNAME << player->getProp(PLPROP_NICKNAME) >> (char)PLPROP_COMMUNITYNAME << player->getProp(PLPROP_COMMUNITYNAME));
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
	setProps(CString() >> (char)PLPROP_CURCHAT >> (char)pChat.length() << pChat, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

///////////////////////////////////////////////////////////////////////////////

void Player::deleteFlag(const std::string& pFlagName, bool sendToPlayer)
{
	account.flags.erase(pFlagName);

	if (sendToPlayer)
	{
		sendPacket(CString() >> (char)PLO_FLAGDEL << pFlagName);
	}
}

void Player::setFlag(const std::string& pFlagName, const CString& pFlagValue, bool sendToPlayer)
{
	// Call Default Set Flag
	account.flags.insert(std::make_pair(pFlagName, pFlagValue.toString()));

	// Send to Player
	if (sendToPlayer)
	{
		if (pFlagValue.isEmpty())
			sendPacket(CString() >> (char)PLO_FLAGSET << pFlagName);
		else
			sendPacket(CString() >> (char)PLO_FLAGSET << pFlagName << "=" << pFlagValue);
	}
}

CString Player::getFlag(const std::string& pFlagName) const
{
	auto it = account.flags.find(pFlagName);
	if (it == account.flags.end())
		return CString();
	return it->second;
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
	if (std::erase(account.weapons, weapon->getName()) != 0)
	{
		if (m_id == -1) return true;

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
		serverlog.out("Player %s is sending invalid packets.\n", account.character.nickName.c_str());
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
	setProps(pPacket, PLSETPROPS_SETBYPLAYER | PLSETPROPS_FORWARD);
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
		unsigned char flags = strtoint(player->getProp(PLPROP_ADDITFLAGS));
		if (flags & PLFLAG_NOTOALL) continue;

		player->sendPacket(CString() >> (char)PLO_TOALL >> (short)m_id >> (char)message.length() << message);
	}
	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_PRIVATEMESSAGE(CString& pPacket)
{
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
				serverlog.out("Sending PM to global player: %s.\n", pmPlayer->account.nickname.c_str());
				pmMessage.guntokenizeI();
				pmExternalPlayer(pmPlayer->getServerName(), pmPlayer->account.name, pmMessage);
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
		serverlog.out(":: Warning - Player %s had an invalid packet count.\n", account.name.c_str());
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
