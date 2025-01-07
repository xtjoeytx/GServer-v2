#include <fmt/format.h>
#include <map>
#include <stdio.h>
#include <sys/stat.h>
#include <vector>
#include <array>
#include <type_traits>

#include <IDebug.h>
#include <IEnums.h>

#include "IConfig.h"

#include "Server.h"
#include "object/PlayerRC.h"
#include "level/Level.h"
#include "network/IPacketHandler.h"
#include "utilities/TimeUnits.h"

#define serverlog m_server->getServerLog()
#define rclog m_server->getRCLog()
#define nclog m_server->getNPCLog()
//extern bool __playerPropsRC[propscount];

///////////////////////////////////////////////////////////////////////////////

using PacketHandleFunc = HandlePacketResult(PlayerRC::*)(CString&);
using PacketHandleArray = std::array<PacketHandleFunc, 256>;

static PacketHandleArray GeneratePacketHandlers()
{
	PacketHandleArray handlers{};
	handlers.fill(nullptr);

	handlers[PLI_RC_SERVEROPTIONSGET] = &PlayerRC::msgPLI_RC_SERVEROPTIONSGET;
	handlers[PLI_RC_SERVEROPTIONSSET] = &PlayerRC::msgPLI_RC_SERVEROPTIONSSET;
	handlers[PLI_RC_FOLDERCONFIGGET] = &PlayerRC::msgPLI_RC_FOLDERCONFIGGET;
	handlers[PLI_RC_FOLDERCONFIGSET] = &PlayerRC::msgPLI_RC_FOLDERCONFIGSET;
	handlers[PLI_RC_RESPAWNSET] = &PlayerRC::msgPLI_RC_RESPAWNSET;
	handlers[PLI_RC_HORSELIFESET] = &PlayerRC::msgPLI_RC_HORSELIFESET;
	handlers[PLI_RC_APINCREMENTSET] = &PlayerRC::msgPLI_RC_APINCREMENTSET;
	handlers[PLI_RC_BADDYRESPAWNSET] = &PlayerRC::msgPLI_RC_BADDYRESPAWNSET;
	handlers[PLI_RC_PLAYERPROPSGET] = &PlayerRC::msgPLI_RC_PLAYERPROPSGET;
	handlers[PLI_RC_PLAYERPROPSSET] = &PlayerRC::msgPLI_RC_PLAYERPROPSSET;
	handlers[PLI_RC_DISCONNECTPLAYER] = &PlayerRC::msgPLI_RC_DISCONNECTPLAYER;
	handlers[PLI_RC_UPDATELEVELS] = &PlayerRC::msgPLI_RC_UPDATELEVELS;
	handlers[PLI_RC_ADMINMESSAGE] = &PlayerRC::msgPLI_RC_ADMINMESSAGE;
	handlers[PLI_RC_PRIVADMINMESSAGE] = &PlayerRC::msgPLI_RC_PRIVADMINMESSAGE;
	handlers[PLI_RC_LISTRCS] = &PlayerRC::msgPLI_RC_LISTRCS;
	handlers[PLI_RC_DISCONNECTRC] = &PlayerRC::msgPLI_RC_DISCONNECTRC;
	handlers[PLI_RC_APPLYREASON] = &PlayerRC::msgPLI_RC_APPLYREASON;
	handlers[PLI_RC_SERVERFLAGSGET] = &PlayerRC::msgPLI_RC_SERVERFLAGSGET;
	handlers[PLI_RC_SERVERFLAGSSET] = &PlayerRC::msgPLI_RC_SERVERFLAGSSET;
	handlers[PLI_RC_ACCOUNTADD] = &PlayerRC::msgPLI_RC_ACCOUNTADD;
	handlers[PLI_RC_ACCOUNTDEL] = &PlayerRC::msgPLI_RC_ACCOUNTDEL;
	handlers[PLI_RC_ACCOUNTLISTGET] = &PlayerRC::msgPLI_RC_ACCOUNTLISTGET;
	handlers[PLI_RC_PLAYERPROPSGET2] = &PlayerRC::msgPLI_RC_PLAYERPROPSGET2;
	handlers[PLI_RC_PLAYERPROPSGET3] = &PlayerRC::msgPLI_RC_PLAYERPROPSGET3;
	handlers[PLI_RC_PLAYERPROPSRESET] = &PlayerRC::msgPLI_RC_PLAYERPROPSRESET;
	handlers[PLI_RC_PLAYERPROPSSET2] = &PlayerRC::msgPLI_RC_PLAYERPROPSSET2;
	handlers[PLI_RC_ACCOUNTGET] = &PlayerRC::msgPLI_RC_ACCOUNTGET;
	handlers[PLI_RC_ACCOUNTSET] = &PlayerRC::msgPLI_RC_ACCOUNTSET;
	handlers[PLI_RC_CHAT] = &PlayerRC::msgPLI_RC_CHAT;
	handlers[PLI_RC_WARPPLAYER] = &PlayerRC::msgPLI_RC_WARPPLAYER;
	handlers[PLI_RC_PLAYERRIGHTSGET] = &PlayerRC::msgPLI_RC_PLAYERRIGHTSGET;
	handlers[PLI_RC_PLAYERRIGHTSSET] = &PlayerRC::msgPLI_RC_PLAYERRIGHTSSET;
	handlers[PLI_RC_PLAYERCOMMENTSGET] = &PlayerRC::msgPLI_RC_PLAYERCOMMENTSGET;
	handlers[PLI_RC_PLAYERCOMMENTSSET] = &PlayerRC::msgPLI_RC_PLAYERCOMMENTSSET;
	handlers[PLI_RC_PLAYERBANGET] = &PlayerRC::msgPLI_RC_PLAYERBANGET;
	handlers[PLI_RC_PLAYERBANSET] = &PlayerRC::msgPLI_RC_PLAYERBANSET;
	handlers[PLI_RC_FILEBROWSER_START] = &PlayerRC::msgPLI_RC_FILEBROWSER_START;
	handlers[PLI_RC_FILEBROWSER_CD] = &PlayerRC::msgPLI_RC_FILEBROWSER_CD;
	handlers[PLI_RC_FILEBROWSER_END] = &PlayerRC::msgPLI_RC_FILEBROWSER_END;
	handlers[PLI_RC_FILEBROWSER_DOWN] = &PlayerRC::msgPLI_RC_FILEBROWSER_DOWN;
	handlers[PLI_RC_FILEBROWSER_UP] = &PlayerRC::msgPLI_RC_FILEBROWSER_UP;
	handlers[PLI_NPCSERVERQUERY] = &PlayerRC::msgPLI_NPCSERVERQUERY;
	handlers[PLI_RC_FILEBROWSER_MOVE] = &PlayerRC::msgPLI_RC_FILEBROWSER_MOVE;
	handlers[PLI_RC_FILEBROWSER_DELETE] = &PlayerRC::msgPLI_RC_FILEBROWSER_DELETE;
	handlers[PLI_RC_FILEBROWSER_RENAME] = &PlayerRC::msgPLI_RC_FILEBROWSER_RENAME;
	handlers[PLI_RC_LARGEFILESTART] = &PlayerRC::msgPLI_RC_LARGEFILESTART;
	handlers[PLI_RC_LARGEFILEEND] = &PlayerRC::msgPLI_RC_LARGEFILEEND;
	handlers[PLI_RC_FOLDERDELETE] = &PlayerRC::msgPLI_RC_FOLDERDELETE;
	handlers[PLI_RC_UNKNOWN162] = &PlayerRC::msgPLI_RC_UNKNOWN162;

	return handlers;
}

///////////////////////////////////////////////////////////////////////////////

HandlePacketResult PlayerRC::handlePacket(std::optional<uint8_t> id, CString& packet)
{
	static PacketHandleArray PacketHandlers = GeneratePacketHandlers();

	auto handle = id.has_value() ? PacketHandlers[id.value()] : nullptr;
	if (handle == nullptr)
		return Player::handlePacket(id, packet);

	auto result = (this->*handle)(packet);
	if (result == HandlePacketResult::Bubble)
		return Player::handlePacket(id, packet);

	return result;
}

///////////////////////////////////////////////////////////////////////////////

void PlayerRC::cleanup()
{
	Player::cleanup();
}

///////////////////////////////////////////////////////////////////////////////

bool PlayerRC::handleLogin(CString& pPacket)
{
	// Read Player-Ip
	account.ipAddress = m_playerSock->getRemoteIp();
#ifdef HAVE_INET_PTON
	inet_pton(AF_INET, account.ipAddress.c_str(), &m_accountIp);
#else
	m_accountIp = inet_addr(account.ipAddress.c_str());
#endif

	// TODO(joey): Hijack type based on what graal sends, rather than use it directly.
	m_type = (1 << pPacket.readGChar());

	// Set the encryptions.
	serverlog.out(":: New login:   ");
	switch (m_type)
	{
	case PLTYPE_RC:
		serverlog.append("RC\n");
		Encryption.setGen(ENCRYPT_GEN_2);
		break;
	case PLTYPE_NC:
		serverlog.append("NC\n");
		Encryption.setGen(ENCRYPT_GEN_2);
		break;
	case PLTYPE_RC2:
		serverlog.append("New RC (2.22+)\n");
		Encryption.setGen(ENCRYPT_GEN_5);
		break;
	default:
		serverlog.append("Unknown (%d)\n", m_type);
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Your client type is unknown.  Please inform the " << APP_VENDOR << " Team.  Type: " << CString((int)m_type) << ".");
		return false;
	}

	// Newer RC clients have an encryption key.
	if (Encryption.getGen() > ENCRYPT_GEN_3)
	{
		m_encryptionKey = (unsigned char)pPacket.readGChar();

		Encryption.reset(m_encryptionKey);
		if (Encryption.getGen() > ENCRYPT_GEN_3)
			m_fileQueue.setCodec(Encryption.getGen(), m_encryptionKey);
	}

	// Read Client-Version
	m_version = pPacket.readChars(8);
	m_versionId = getVersionIDByVersion(m_version);

	// Read Account & Password
	account.name = pPacket.readChars(pPacket.readGUChar()).toString();
	CString password = pPacket.readChars(pPacket.readGUChar());

	// Client Identity: win,"",02e2465a2bf38f8a115f6208e9938ac8,ff144a9abb9eaff4b606f0336d6d8bc5,"6.2 9200 "
	//					{platform}, {mobile provides 'dc:id2'}, {md5hash:harddisk-id}, {md5hash:network-id}, {uname(release, version)}, {android-id}
	CString identity = pPacket.readString("");

	//serverlog.out("   Key: %d\n", key);
	serverlog.out("   Version:     %s (%s)\n", m_version.text(), getVersionString(m_version, m_type));
	serverlog.out("   Account:     %s\n", account.name.c_str());
	if (!identity.isEmpty())
	{
		serverlog.out("   Identity:    %s\n", identity.text());
		auto identityTokens = identity.tokenize(",", true);
		m_os = identityTokens[0];
	}

	// Check for available slots on the server.
	if (m_server->getPlayerList().size() >= (unsigned int)m_server->getSettings().getInt("maxplayers", 128))
	{
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "This server has reached its player limit.");
		return false;
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

bool PlayerRC::sendLogin()
{
	if (Player::sendLogin() == false)
		return false;

	auto& settings = m_server->getSettings();

	// This packet clears the players weapons on the client, but official
	// also sends it to the RC's so we are maintaining the same behavior
	sendPacket(CString() >> (char)PLO_CLEARWEAPONS);

	// If no nickname was specified, set the nickname to the account name.
	if (account.character.nickName.empty())
		account.character.nickName = std::format("*{}", account.name);
	account.level = " ";

	// Set the head to the server's set staff head.
	account.character.headImage = m_server->getSettings().getStr("staffhead", "head25.png").toStringView();

	// Send the RC join message to the RC.
	std::vector<CString> rcmessage = CString::loadToken(m_server->getServerPath() << "config/rcmessage.txt", "\n", true);
	for (const auto& i : rcmessage)
		sendPacket(CString() >> (char)PLO_RC_CHAT << i);

	sendPacket(CString() >> (char)PLO_UNKNOWN190);

	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "New RC: " << account.name);

	// Send out what guilds should be placed in the Staff section of the playerlist.
	std::vector<CString> guilds = settings.getStr("staffguilds").tokenize(",");
	CString guildPacket = CString() >> (char)PLO_STAFFGUILDS;
	for (std::vector<CString>::iterator i = guilds.begin(); i != guilds.end(); ++i)
		guildPacket << "\"" << ((CString)(*i)).trim() << "\",";
	sendPacket(guildPacket.remove(guildPacket.length() - 1, 1));

	// Send out the server's available status list options.
	{
		// graal doesn't quote these
		CString pliconPacket = CString() >> (char)PLO_STATUSLIST;
		for (const auto& status : m_server->getStatusList())
			pliconPacket << status.trim() << ",";

		sendPacket(pliconPacket.remove(pliconPacket.length() - 1, 1));
	}

	// This comes after status icons for RC
	sendPacket(CString() >> (char)PLO_RC_MAXUPLOADFILESIZE >> (long long)(1048576 * 20));

	// Then during iterating the playerlist to send players to the rc client, it sends addplayer followed by rc chat per person.
	// TODO: Was this unimplemented?

	// Exchange props with everybody on the server.
	exchangeMyPropsWithOthers();

	// If we are an RC, announce the list of currently logged in RCs.
	CString rcsOnline;
	for (const auto& [pid, player] : players_of_type<PlayerRC>(m_server->getPlayerList()))
		rcsOnline << (rcsOnline.isEmpty() ? "" : ", ") << player->account.name;
	if (!rcsOnline.isEmpty())
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Currently online: " << rcsOnline);

	return true;
}
