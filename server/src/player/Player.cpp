#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <CSocket.h>

#include <CString.h>

#include <BabyDI.h>
#include <IEnums.h>
#include <IUtil.h>

#include <Account.h>
#include <Server.h>
#include <filesystem/File.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <level/Level.h>
#include <level/LevelItem.h>
#include <misc/WordFilter.h>
#include <network/IPacketHandler.h>
#include <npcserver/NPCServer.h>
#include <object/Character.h>
#include <object/Player.h>
#include <object/Weapon.h>
#include <player/PlayerClient.h>
#include <player/PlayerProps.h>
#include <scripting/IScriptEngine.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/manager/GuildManager.h>
#include <utilities/manager/ITranslationManager.h>
#include <utilities/PropertySerializers.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

// clang-format off
static constexpr std::array<std::string_view, 8> colorPropertyNames = { "#C0"sv, "#C1"sv, "#C2"sv, "#C3"sv, "#C4"sv, "#C5"sv, "#C6"sv, "#C7"sv };
static constexpr std::array<std::string_view, 30> ganiAttributePropertyNames = {
	"#P1"sv, "#P2"sv, "#P3"sv, "#P4"sv, "#P5"sv, "#P6"sv, "#P7"sv, "#P8"sv, "#P9"sv, "#P10"sv,
	"#P11"sv, "#P12"sv, "#P13"sv, "#P14"sv, "#P15"sv, "#P16"sv, "#P17"sv, "#P18"sv, "#P19"sv, "#P20"sv,
	"#P21"sv, "#P22"sv, "#P23"sv, "#P24"sv, "#P25"sv, "#P26"sv, "#P27"sv, "#P28"sv, "#P29"sv, "#P30"sv,
};
// clang-format on

//----------------------------

#ifdef PACKETLOGGING
	#define FOR_OUTPUT_PACKETS(DO)     \
		DO(PLO_LEVELBOARD)             \
		DO(PLO_LEVELLINK)              \
		DO(PLO_BADDYPROPS)             \
		DO(PLO_NPCPROPS)               \
		DO(PLO_LEVELCHEST)             \
		DO(PLO_LEVELSIGN)              \
		DO(PLO_LEVELNAME)              \
		DO(PLO_BOARDMODIFY)            \
		DO(PLO_OTHERPLPROPS)           \
		DO(PLO_PLAYERPROPS)            \
		DO(PLO_ISLEADER)               \
		DO(PLO_BOMBADD)                \
		DO(PLO_BOMBDEL)                \
		DO(PLO_TOALL)                  \
		DO(PLO_PLAYERWARP)             \
		DO(PLO_WARPFAILED)             \
		DO(PLO_DISCMESSAGE)            \
		DO(PLO_HORSEADD)               \
		DO(PLO_HORSEDEL)               \
		DO(PLO_ARROWADD)               \
		DO(PLO_FIRESPY)                \
		DO(PLO_THROWCARRIED)           \
		DO(PLO_ITEMADD)                \
		DO(PLO_ITEMDEL)                \
		DO(PLO_NPCMOVED)               \
		DO(PLO_SIGNATURE)              \
		DO(PLO_NPCACTION)              \
		DO(PLO_BADDYHURT)              \
		DO(PLO_FLAGSET)                \
		DO(PLO_NPCDEL)                 \
		DO(PLO_FILESENDFAILED)         \
		DO(PLO_FLAGDEL)                \
		DO(PLO_SHOWIMGPLAYER)          \
		DO(PLO_NPCWEAPONADD)           \
		DO(PLO_NPCWEAPONDEL)           \
		DO(PLO_RC_ADMINMESSAGE)        \
		DO(PLO_EXPLOSION)              \
		DO(PLO_PRIVATEMESSAGE)         \
		DO(PLO_PUSHAWAY)               \
		DO(PLO_LEVELMODTIME)           \
		DO(PLO_HURTPLAYER)             \
		DO(PLO_STARTMESSAGE)           \
		DO(PLO_NEWWORLDTIME)           \
		DO(PLO_DEFAULTWEAPON)          \
		DO(PLO_HASNPCSERVER)           \
		DO(PLO_FILEUPTODATE)           \
		DO(PLO_HITOBJECTS)             \
		DO(PLO_STAFFGUILDS)            \
		DO(PLO_TRIGGERACTION)          \
		DO(PLO_PLAYERWARP2)            \
		DO(PLO_RC_ACCOUNTADD)          \
		DO(PLO_RC_ACCOUNTSTATUS)       \
		DO(PLO_RC_ACCOUNTNAME)         \
		DO(PLO_RC_ACCOUNTDEL)          \
		DO(PLO_RC_ACCOUNTPROPS)        \
		DO(PLO_ADDPLAYER)              \
		DO(PLO_DELPLAYER)              \
		DO(PLO_RC_ACCOUNTPROPSGET)     \
		DO(PLO_RC_ACCOUNTCHANGE)       \
		DO(PLO_RC_PLAYERPROPSCHANGE)   \
		DO(PLO_UNKNOWN60)              \
		DO(PLO_RC_SERVERFLAGSGET)      \
		DO(PLO_RC_PLAYERRIGHTSGET)     \
		DO(PLO_RC_PLAYERCOMMENTSGET)   \
		DO(PLO_RC_PLAYERBANGET)        \
		DO(PLO_RC_FILEBROWSER_DIRLIST) \
		DO(PLO_RC_FILEBROWSER_DIR)     \
		DO(PLO_RC_FILEBROWSER_MESSAGE) \
		DO(PLO_LARGEFILESTART)         \
		DO(PLO_LARGEFILEEND)           \
		DO(PLO_RC_ACCOUNTLISTGET)      \
		DO(PLO_RC_PLAYERPROPS)         \
		DO(PLO_RC_PLAYERPROPSGET)      \
		DO(PLO_RC_ACCOUNTGET)          \
		DO(PLO_RC_CHAT)                \
		DO(PLO_PROFILE)                \
		DO(PLO_RC_SERVEROPTIONSGET)    \
		DO(PLO_RC_FOLDERCONFIGGET)     \
		DO(PLO_NC_CONTROL)             \
		DO(PLO_NPCSERVERADDR)          \
		DO(PLO_NC_LEVELLIST)           \
		DO(PLO_UNKNOWN81)              \
		DO(PLO_SERVERTEXT)             \
		DO(PLO_UNKNOWN83)              \
		DO(PLO_LARGEFILESIZE)          \
		DO(PLO_RAWDATA)                \
		DO(PLO_BOARDPACKET)            \
		DO(PLO_FILE)                   \
		DO(PLO_RC_MAXUPLOADFILESIZE)   \
		DO(PLO_UNKNOWN104)             \
		DO(PLO_UPDATEPACKAGESIZE)      \
		DO(PLO_UPDATEPACKAGEDONE)      \
		DO(PLO_BOARDLAYER)             \
		DO(PLO_UNKNOWN109)             \
		DO(PLO_SETNETCOOKIE)           \
		DO(PLO_UNKNOWN124)             \
		DO(PLO_NPCBYTECODE)            \
		DO(PLO_UNKNOWN132)             \
		DO(PLO_UNKNOWN133)             \
		DO(PLO_GANISCRIPT)             \
		DO(PLO_NPCWEAPONSCRIPT)        \
		DO(PLO_NPCDEL2)                \
		DO(PLO_HIDENPCS)               \
		DO(PLO_SAY2)                   \
		DO(PLO_FREEZEPLAYER2)          \
		DO(PLO_UNFREEZEPLAYER)         \
		DO(PLO_SETACTIVELEVEL)         \
		DO(PLO_NC_NPCATTRIBUTES)       \
		DO(PLO_NC_NPCADD)              \
		DO(PLO_NC_NPCDELETE)           \
		DO(PLO_NC_NPCSCRIPT)           \
		DO(PLO_NC_NPCFLAGS)            \
		DO(PLO_NC_CLASSGET)            \
		DO(PLO_NC_CLASSADD)            \
		DO(PLO_NC_LEVELDUMP)           \
		DO(PLO_MOVE)                   \
		DO(PLO_SHOWIMGNPC)             \
		DO(PLO_NC_WEAPONLISTGET)       \
		DO(PLO_UNKNOWN168)             \
		DO(PLO_UNKNOWN169)             \
		DO(PLO_GHOSTMODE)              \
		DO(PLO_BIGMAP)                 \
		DO(PLO_MINIMAP)                \
		DO(PLO_GHOSTTEXT)              \
		DO(PLO_GHOSTICON)              \
		DO(PLO_SHOOT)                  \
		DO(PLO_DISABLECLASSICMODE)     \
		DO(PLO_FULLSTOP2)              \
		DO(PLO_SERVERWARP)             \
		DO(PLO_RPGWINDOW)              \
		DO(PLO_STATUSLIST)             \
		DO(PLO_UNKNOWN181)             \
		DO(PLO_LISTPROCESSES)          \
		DO(PLO_HASPROCESSRUNNING)      \
		DO(PLO_TAKESCREENSHOT)         \
		DO(PLO_BOARDHEIGHTS)           \
		DO(PLO_BOARDMODIFY2)           \
		DO(PLO_UPDATEPACKAGEISUPDATED) \
		DO(PLO_NC_CLASSDELETE)         \
		DO(PLO_MOVE2)                  \
		DO(PLO_SERVERLISTCONNECTED)    \
		DO(PLO_SHOOT2)                 \
		DO(PLO_NC_WEAPONGET)           \
		DO(PLO_UNKNOWN193)             \
		DO(PLO_CLEARWEAPONS)           \
		DO(PLO_LOADGANI)               \
		DO(PLO_LOADSCRIPT)             \
		DO(PLO_SERVEROPTIONS)          \
		DO(PLO_SET_ENC_KEY)            \
		DO(PLO_BUNDLE)
	#define FILL_OUTPUT_ARRAY(name) names[(uint8_t)name] = #name;

static constexpr std::array<std::string, 255> FillOutputPacketNamesArray()
{
	std::array<std::string, 255> names;
	names.fill("(unknown packet)");
	FOR_OUTPUT_PACKETS(FILL_OUTPUT_ARRAY)
	return names;
}

std::array<std::string, 255> OutputPacketNamesArray = FillOutputPacketNamesArray();
#endif

///////////////////////////////////////////////////////////////////////////////

CString ShootPacketWrapper::constructShootV1() const
{
	CString packet;
	packet.writeGInt(source);
	packet.writeGChar((position.x() % 1024) / 8);
	packet.writeGChar((position.y() % 1024) / 8);
	packet.writeGChar((position.z() / 16) + 50);
	packet.writeGChar(sangle);
	packet.writeGChar(sanglez);
	packet.writeGChar(power);
	packet.writeGChar(gani.length());
	packet.write(gani);
	packet.writeGChar(shootParams.length());
	packet.write(shootParams);
	return packet;
}

CString ShootPacketWrapper::constructShootV2() const
{
	CString packet;
	packet.writeGShort(position.x());
	packet.writeGShort(position.y());
	packet.writeGShort(position.z());
	packet.writeChar(offsetx + 32);
	packet.writeChar(offsety + 32);
	packet.writeGChar(sangle);
	packet.writeGChar(sanglez);
	packet.writeGChar(power);
	packet.writeGChar(gravity);
	packet.writeGShort(gani.length());
	packet.write(gani);
	packet.writeGChar(shootParams.length());
	packet.write(shootParams);
	return packet;
}

///////////////////////////////////////////////////////////////////////////////

using PacketHandleFunc = HandlePacketResult (Player::*)(CString&);
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

	m_lastData = clock::now();

	auto handle = id.has_value() ? PacketHandlers[id.value()] : &Player::msgPLI_NULL;
	return (this->*handle)(packet);
}

///////////////////////////////////////////////////////////////////////////////

Player::Player(CSocket* pSocket, PlayerID pId)
	: m_playerSock(pSocket), m_id(pId), m_fileQueue(pSocket)
{
	m_server = BabyDI::Get<Server>();
	m_lastData = clock::now();
	m_serverName = m_server->getName();

	account.variables.defaultLifetime = variables::Lifetime::PERMANENT;

	srand((unsigned int)time(0));
}

Player::~Player()
{
	cleanup();
}

void Player::cleanup()
{
	// Send all unsent data (for disconnect messages and whatnot).
	if (m_playerSock != nullptr)
		m_fileQueue.sendCompress();

	if (m_id > 0 && m_server != nullptr && m_loaded)
	{
		// Save account.
		if (isClient() && !account.loadOnly)
			m_server->getAccountLoader().saveAccount(account);

		// Announce our departure to other clients.
		if (!isNC())
		{
			m_server->sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id >> (char)PlayerProp::DISCONNECT, this);
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
			log::printLine(log::server, "Client disconnected: [{}] {}", m_id, account.name);
		else if (isRC())
			log::printLine(log::server, "RC disconnected: [{}] {}", m_id, account.name);
		else if (isNC())
			log::printLine(log::server, "NC disconnected: [{}] {}", m_id, account.name);

		// Get rid of the player now.
		m_server->getPlayerIdGenerator().freeId(m_id);
		m_server->getPlayerList().erase(m_id);

		m_loaded = false;
	}

	if (m_playerSock)
	{
		m_server->getSocketManager().unregisterSocket(this);
		delete m_playerSock;
	}
	m_playerSock = nullptr;
}

bool Player::onRecv()
{
	// If our socket is gone, delete ourselves.
	if (m_playerSock == nullptr || m_playerSock->getState() == SOCKET_STATE_DISCONNECTED)
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

	// Hold ourselves just in case we are deleted.
	auto self = shared_from_this();

	// Do the main function.
	doMain();
	if (m_playerSock != nullptr)
		m_server->getSocketManager().updateSingle(this, false, true);

	return m_playerSock != nullptr;
}

bool Player::onSend()
{
	if (m_playerSock == nullptr || m_playerSock->getState() == SOCKET_STATE_DISCONNECTED)
	{
		m_fileQueue.clearBuffers();
		return false;
	}

	// Send data.
	m_fileQueue.sendCompress();

	// Update last send time.
	m_lastData = clock::now();

	return true;
}

void Player::onUnregister()
{
	if (m_loaded)
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
	if (m_playerSock == 0 || m_playerSock->getState() == SOCKET_STATE_DISCONNECTED)
	{
		m_fileQueue.clearBuffers();
		return false;
	}

	m_fileQueue.sendCompress();
	return true;
}

void Player::disconnect()
{
	m_fileQueue.sendCompress();
	m_server->deletePlayer(shared_from_this());
}

void Player::sendPacket(CString pPacket, bool appendNL)
{
	// empty buffer?
	if (pPacket.isEmpty())
		return;

	// Not connected?
	if (m_playerSock == nullptr || m_playerSock->getState() == SOCKET_STATE_DISCONNECTED)
		return;

#ifdef PACKETLOGGING
	// This will suck as long as we have gs2lib.
	uint32_t pid = static_cast<uint32_t>(static_cast<uint8_t>(pPacket[0]) - 32);
	log::printLine(log::networkdump, "< Out Packet to {}: [{}] {} ({} bytes)", account.name, pid, OutputPacketNamesArray[pid], pPacket.length());
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

std::pair<bool, bool> Player::sendFile(const std::filesystem::path& file)
{
	// Add the filename to the list of known files so we can resend the file
	// to the client if it gets changed after it was originally sent
	if (auto client = std::dynamic_pointer_cast<PlayerClient>(shared_from_this()); isClient() && client != nullptr)
	{
		client->m_knownFiles.insert(fs::getANSIFileName(file));
	}

	auto& filesystem = m_server->getFileSystem();
	std::string filename = fs::getANSIFileName(file);

	auto sendFailure = [this, &filename](std::string_view message) -> bool
	{
		if (!message.empty())
			log::printLine(log::server, "[WARNING] {}: {}", message, filename);
		sendPacket(CString() >> (char)PLO_FILESENDFAILED << filename);
		return false;
	};

	std::vector<char> fileData;

	// Get the file mod time.
	time_t modTime = 0;

	// Find the file.
	if (std::filesystem::exists(file))
	{
		fs::File openedFile{file};
		fileData = std::move(openedFile.read());
		modTime = clock::to_time_t(toSystemClock(std::filesystem::last_write_time(file)));
	}
	else
	{
		auto info = filesystem.infoi(fs::FileCategory::ALL, file.filename());
		if (info == nullptr)
			return {sendFailure("File not found when trying to send to player"), false};

		// Open the file and read the data.
		{
			auto openedFile = info->openFile();
			if (openedFile == nullptr)
				return {sendFailure("File failed to load"), false};

			fileData = std::move(openedFile->read());
		}

		modTime = clock::to_time_t(info->getModTime());
	}

	// Warn for very large files.  These are the cause of many bug reports.
	if (fileData.size() > 3145728) // 3MB
		log::printLine(log::server, "[WARNING] Sending a large file (over 3MB): {}", filename);

	// See if we have enough room in the packet for the file.
	// If not, we need to send it as a big file.
	// 1 (PLO_FILE) + 5 (modTime) + 1 (file.length()) + file.length() + 1 (\n)
	bool isBigFile = false;
	size_t packetLength = (size_t)1 + 5 + 1 + filename.length() + 1;
	if (fileData.size() > 32000)
		isBigFile = true;

	// Clients before 2.14 didn't support large files.
	if (isClient() && m_versionId < CLVER_2_14)
	{
		if (m_versionId < CLVER_2_1) packetLength -= 5; // modTime isn't sent.
		if (fileData.size() > 64000)
			return {sendFailure("File too large for client version"), isBigFile};

		isBigFile = false;
	}

	// If we are sending a big file, let the client know now.
	if (isBigFile)
	{
		sendPacket(CString() >> (char)PLO_LARGEFILESTART << filename);
		sendPacket(CString() >> (char)PLO_LARGEFILESIZE >> (long long)fileData.size());
	}

	// Send the file now.
	std::span<char> fileDataSpan{fileData};
	while (!fileDataSpan.empty())
	{
		int sendSize = std::clamp((int)fileDataSpan.size(), 0, 32000);
		if (isClient() && m_versionId < CLVER_2_14) sendSize = fileData.size();

		// Older client versions didn't send the modTime.
		if (isClient() && m_versionId < CLVER_2_1)
		{
			// We don't add a \n to the end of the packet, so subtract 1 from the packet length.
			sendPacket(CString() >> (char)PLO_RAWDATA >> (int)(packetLength - 1 + sendSize));
			sendPacket(CString() >> (char)PLO_FILE >> (char)filename.length() << filename << std::string_view{fileDataSpan.subspan(0, sendSize)}, false);
		}
		else
		{
			sendPacket(CString() >> (char)PLO_RAWDATA >> (int)(packetLength + sendSize));
			sendPacket(CString() >> (char)PLO_FILE >> (long long)modTime >> (char)filename.length() << filename << std::string_view{fileDataSpan.subspan(0, sendSize)} << "\n", false);
		}

		fileDataSpan = fileDataSpan.subspan(sendSize);
	}

	// If we had sent a large file, let the client know we finished sending it.
	if (isBigFile) sendPacket(CString() >> (char)PLO_LARGEFILEEND << filename);

	return {true, isBigFile};
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
		log::printLine(log::server, "** [Disconnect] '{}': Attempted login from banned IP: {}", account.name, m_playerSock->getRemoteIp());
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You have been banned from this server.");
		return false;
	}

	// Check to see if the player is banned or not.
	if (account.banned && !account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		log::printLine(log::server, "** [Disconnect] '{}': Attempted login from banned account. (IP: {})", account.name, m_playerSock->getRemoteIp());
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You have been banned.  Reason: " << string::join(string::fromCSV(account.banReason), "\r"));
		return false;
	}

	// If we are an RC, check to see if we can log in.
	if (isRC() || isNC())
	{
		// Check and see if we are allowed in.
		if (!isStaff() || !isAdminIp())
		{
			log::printLine(log::rc, "** [Disconnect] '{}': Attempted RC login.", account.name);
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You do not have RC rights.");
			return false;
		}
	}

	// Check to see if we can log in if we are a client.
	if (isClient())
	{
		// Staff only.
		if (m_server->getSettings().get<bool>("onlystaff").value_or(false) && !isStaff())
		{
			log::printLine(log::rc, "** [Disconnect] '{}': Server is staff only.", account.name);
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "This server is currently restricted to staff only.");
			return false;
		}

		// Check and see if we are allowed in.
		if (!account.adminIpRange.empty() && account.adminIpRange[0] != "0.0.0.0" && !isAdminIp())
		{
			log::printLine(log::rc, "** [Disconnect] '{}': IP does not match the allowed list. (IP: {})", account.name, m_playerSock->getRemoteIp());
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
		sendPacket(CString() >> (char)PLO_DISABLECLASSICMODE);
		sendPacket(CString() >> (char)PLO_GHOSTICON >> (char)1);
	}

	if (isClient())
	{
		// Tell the client if we have an npc-server, which disables certain features on the client (like sending NPC prop modifications).
		// Later clients don't send this because all client-side functionality was removed.
		// There isn't any harm in always sending it, though.
		if (m_server->hasNPCServer())
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

			if (string::equalsi(otherAccount, account.name) && meClient == themClient && otherID != m_id)
			{
				if (std::chrono::duration_cast<std::chrono::seconds>(m_server->getFrameStartTime() - player->getLastData()) > 30s)
				{
					player->sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Someone else has logged into your account.");
					player->disconnect();
				}
				else
				{
					log::printLine(log::rc, "** [Disconnect] '{}': Attempted double login.", account.name);
					sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Account is already in use.");
					return false;
				}
			}
		}
	}

	// Tell the serverlist the player is logged in.
	if (!isNPCServer())
		m_server->recordPlayerLoggedIn(shared_from_this());

	// Set loaded to true so our account is saved when we leave.
	// This also lets us send data.
	m_loaded = true;

	// Mark our login time.
	loginTime = m_server->getNWTime();
	lastDeadTime = loginTime;

	return true;
}

///////////////////////////////////////////////////////////////////////////////

// Exchange props with everybody on the server.
void Player::exchangeMyPropsWithOthers()
{
	// RC props are sent differently.
	CString myRCProps;
	myRCProps >> (char)PLO_ADDPLAYER >> (short)getId() >> (char)account.name.length() << account.name >> (char)PlayerProp::CURLEVEL << getProp<PlayerProp::CURLEVEL>().serialize() >> (char)PlayerProp::PLAYERLISTSTATUS << getProp<PlayerProp::PLAYERLISTSTATUS>().serialize() >> (char)PlayerProp::NICKNAME << getProp<PlayerProp::NICKNAME>().serialize() >> (char)PlayerProp::COMMUNITYNAME << getProp<PlayerProp::COMMUNITYNAME>().serialize();

	CString toOthers = CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id;
	CString joinLevel = CString() >> (char)PlayerProp::JOINLEAVELVL >> (char)1;
	CString myClientProps = (isClient() ? getPropsPacketFromList(loginPropsClientOthers) : getPropsPacketFromList(loginPropsRC));

	CString rcsOnline;
	auto& playerList = m_server->getPlayerList();
	for (const auto& [pid, player] : playerList)
	{
		if (player.get() == this) continue;

		// Don't send npc-control players to others
		if (player->isNC()) continue;

		// Send the other player my props.
		bool sameLevel = (player->account.level == account.level);
		if (player->isClient())
			player->sendPacket(CString() << toOthers << (sameLevel ? joinLevel : "") << myClientProps);
		else
			player->sendPacket(myRCProps);

		// Add Player / RC.
		if (isClient())
		{
			// clang-format off
			sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)player->getId()
				<< (sameLevel ? joinLevel : "")
				<< (player->isClient() ? player->getPropsPacketFromList(loginPropsClientOthers) : player->getPropsPacketFromList(loginPropsRC)));
			// clang-format on
		}
		else
		{
			// clang-format off
			// Get the other player's RC props.
			sendPacket(CString() >> (char)PLO_ADDPLAYER >> (short)player->getId() >> (char)player->account.name.length() << player->account.name
				>> (char)PlayerProp::CURLEVEL << player->getProp<PlayerProp::CURLEVEL>().serialize()
				>> (char)PlayerProp::PLAYERLISTSTATUS << player->getProp<PlayerProp::PLAYERLISTSTATUS>().serialize()
				>> (char)PlayerProp::NICKNAME << player->getProp<PlayerProp::NICKNAME>().serialize()
				>> (char)PlayerProp::COMMUNITYNAME << player->getProp<PlayerProp::COMMUNITYNAME>().serialize());
			// clang-format on
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

bool Player::isAdminIp()
{
	for (const auto& ipMask : account.adminIpRange)
	{
		if (string::match(std::string_view{ account.ipAddress }, std::string_view{ ipMask }))
			return true;
	}
	return false;
}

bool Player::isStaff()
{
	return m_server->isStaff(account.name);
}

bool Player::isJailed()
{
	if (!m_server->cached.jailLevels)
		return false;

	const auto& levels = m_server->cached.jailLevels.getValue();
	auto jailed = std::ranges::find_if(levels, [this](const std::string& level)
	{
		return string::equalsi(account.level, string::trim(level));
	});

	return jailed != levels.end();
}

///////////////////////////////////////////////////////////////////////////////

double Player::getCalculatedTileZ() const noexcept
{
	return account.character.localPixelZ / 16.0;
}

std::shared_ptr<Level> Player::getLevel() const
{
	return m_server->getStubbedLevel(account.level, account.groupName);
}

///////////////////////////////////////////////////////////////////////////////

void Player::setNick(CString pNickName, bool force)
{
	auto desiredNickname = string::trim(pNickName.toStringView());
	std::string prefix, guildName;

	// Determine the guild, if one was supplied.
	auto guildStart = desiredNickname.find('(');
	if (guildStart != std::string_view::npos)
	{
		auto guildEnd = desiredNickname.find(')', guildStart);
		if (guildEnd == std::string_view::npos)
			guildName = desiredNickname.substr(guildStart + 1);
		else
			guildName = desiredNickname.substr(guildStart + 1, guildEnd - guildStart - 1);
	}

	// If we are forcing a nickname change, do it now and return early.
	if (force || m_isExternal || (guildName == "RC" && isRC()))
	{
		account.character.nickName = pNickName.toString();
		this->m_guild = guildName;
		return;
	}

	// Determine the nickname part.
	auto nickNamePart = desiredNickname;
	{
		// If a guild was supplied, remove it from the nickname.
		if (guildStart != std::string_view::npos)
			nickNamePart = desiredNickname.substr(0, guildStart);

		// Remove a * if it was supplied (as it denotes that the nickname is equal to the account name, which we will figure out later).
		if (nickNamePart.starts_with('*'))
			nickNamePart.remove_prefix(1);
	}
	nickNamePart = string::trim(nickNamePart);

	// If the nickname is empty, set it to "unknown".
	if (nickNamePart.empty())
		nickNamePart = "unknown";

	// If the nickname is equal to the account name, set the prefix.
	if (nickNamePart == account.name)
		prefix = "*";

	// If we had a guild, check our permissions.
	if (!guildName.empty())
	{
		// Check if the player is in the guild.
		auto guildManager = BabyDI::Get<GuildManager>();
		if (guildManager->verifyPlayerInGuild(guildName, account.name, nickNamePart))
		{
			account.character.nickName = std::format("{}{} ({})", prefix, nickNamePart, guildName);
			m_guild = guildName;
			return;
		}

		// Not in a local guild, so see if we can ask the listserver if they are in a global guild.
		bool askGlobal = m_server->getSettings().get<bool>("globalguilds").value_or(true);
		if (!askGlobal)
		{
			// Check for whitelisted global guilds.
			askGlobal = std::ranges::contains(string::split(m_server->getSettings().get<std::string>("allowedglobalguilds").value_or(""), ","sv), guildName);
		}

		// See if it is a global guild.
		if (askGlobal)
		{
			// clang-format off
			m_server->getServerList().sendPacket(
				CString() >> (char)SVO_VERIGUILD >> (short)m_id
				>> (char)account.name.length() << account.name
				>> (char)nickNamePart.length() << nickNamePart
				>> (char)guildName.length() << guildName);
			// clang-format on
		}
	}

	account.character.nickName = std::format("{}{}", prefix, nickNamePart);
	m_guild.clear();
}

void Player::setChat(const CString& pChat)
{
	sendPropsFromResults(setPropWith<PlayerProp::CURCHAT>(props::SetBy::SERVER, pChat.toString()));
}

///////////////////////////////////////////////////////////////////////////////

bool Player::deleteFlag(std::string_view flagName, const SetBy setBy)
{
	if (setBy == SetBy::SERVER)
		sendPacket(CString() >> (char)PLO_FLAGDEL << flagName);
	return account.variables.remove(flagName);
}

bool Player::setFlag(std::string_view flagPair, const SetBy setBy)
{
	if (!flagPair.contains('='))
		return setFlag(flagPair, std::nullopt, setBy);

	auto separator = flagPair.find('=');
	auto flagName = string::trim(flagPair.substr(0, separator));
	auto flagValue = string::trim(flagPair.substr(separator + 1));
	return setFlag(flagName, std::string{flagValue}, setBy);
}

bool Player::setFlag(std::string_view flagName, std::optional<std::string> flagValue, const SetBy setBy)
{
	if (!flagValue.has_value())
	{
		if (setBy == SetBy::SERVER)
			sendPacket(CString() >> (char)PLO_FLAGSET << flagName);
		account.variables.add(flagName, true);
	}
	else
	{
		if (setBy == SetBy::SERVER)
			sendPacket(CString() >> (char)PLO_FLAGSET << flagName << "=" << flagValue.value());

		std::string flag{flagName};
		if (flagValue.value().empty())
			return deleteFlag(flag, setBy);
		account.variables.add(flag, flagValue.value());
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool Player::addWeapon(LevelItemType defaultWeapon)
{
	// Allow Default Weapons..?
	if (!m_server->cached.enableDefaultWeapons.getValue())
		return false;

	auto weapon = m_server->getWeapon(LevelItem::getItemName(defaultWeapon));
	if (!weapon)
	{
		weapon = std::make_shared<Weapon>(defaultWeapon);
		m_server->NC_AddWeapon(weapon);
	}

	return this->addWeapon(weapon);
}

bool Player::addWeapon(std::string_view name)
{
	auto weapon = m_server->getWeapon(name);
	return this->addWeapon(weapon);
}

bool Player::addWeapon(std::shared_ptr<Weapon> weapon)
{
	if (weapon == nullptr) return false;

	// See if the player already has the weapon.
	if (!account.hasWeapon(weapon->name))
	{
		account.weapons.push_back(weapon->name);
		if (m_id == 0) return true;
	}

	// Send weapon.
	weapon->registerWeaponWithPlayer(shared_from_this());

	return true;
}

bool Player::deleteWeapon(LevelItemType defaultWeapon)
{
	auto weapon = m_server->getWeapon(LevelItem::getItemName(defaultWeapon));
	return this->deleteWeapon(weapon);
}

bool Player::deleteWeapon(std::string_view name)
{
	auto weapon = m_server->getWeapon(name);
	return this->deleteWeapon(weapon);
}

bool Player::deleteWeapon(std::shared_ptr<Weapon> weapon)
{
	if (weapon == nullptr) return false;

	// Remove the weapon.
	if (std::erase(account.weapons, weapon->name) != 0)
	{
		if (m_id == 0) return true;

		// Send delete notice.
		sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << weapon->name);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

std::string Player::translate(std::string_view key) const
{
	auto translationManager = BabyDI::Get<ITranslationManager>();
	return std::string{translationManager->getText(getLanguage(), key)};
}

///////////////////////////////////////////////////////////////////////////////

void Player::sendPrivateMessage(PlayerID from, std::string_view message)
{
	if (message.empty())
		return;

	// TODO: This is really hacky.  More effort into reverse engineering private messages is required.
	bool isMultiLine = true; // message.find('\n') != std::string_view::npos || message.find("#b") != std::string_view::npos;

	auto convertedMessage = string::replace(message, "\n", "#b");
	auto lines = string::splitByString(convertedMessage, "#b"sv, false);
	auto finalMessage = string::toCSV(lines, true);

	// For some reason, if there are multiple lines, the client strips out the first line.
	// If we don't start with a blank line, add one to avoid message loss.
	// TODO: Hacky!  Figure out why this happens.
	if (isMultiLine && !finalMessage.starts_with(",") && !finalMessage.starts_with("\"\","))
		finalMessage = "\"\"," + finalMessage;

	sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)from << finalMessage);
}

///////////////////////////////////////////////////////////////////////////////

bool Player::warp(std::string_view levelName, const PixelPosition& position, std::optional<clock::time_point> clientCachedTime)
{
	if (auto level = m_server->getLoadedLevel(levelName, shared_from_this()); level != nullptr)
		return enterLevel(level, position, clientCachedTime);
	return false;
}

bool Player::warp(std::shared_ptr<Level> level, const PixelPosition& position, std::optional<clock::time_point> clientCachedTime)
{
	return enterLevel(level, position, clientCachedTime);
}

bool Player::enterLevel(std::shared_ptr<Level> level, const PixelPosition& position, std::optional<clock::time_point> clientCachedTime)
{
	auto localPosition = toLocalPixelPosition(position);
	auto mapPosition = toMapPosition(position);

	// Sanity check.
	if (!level->isGmap())
		mapPosition = {0, 0};

	return enterLevel(level, mapPosition, localPosition, clientCachedTime);
}

bool Player::enterLevel(std::shared_ptr<Level> level, const MapPosition& mapPosition, const LocalPixelPosition& position, std::optional<clock::time_point> clientCachedTime)
{
	auto now = m_server->getFrameStartTime();

	// If we are already on the level, set the position and abort.
	if (account.level == level->levelName)
	{
		// clang-format off
		sendPropsFromResults(
			setPropWith<PlayerProp::X2>(props::SetBy::SERVER, position.x()),
			setPropWith<PlayerProp::Y2>(props::SetBy::SERVER, position.y()),
			setPropWith<PlayerProp::GMAPLEVELX>(props::SetBy::SERVER, mapPosition.x()),
			setPropWith<PlayerProp::GMAPLEVELY>(props::SetBy::SERVER, mapPosition.y())
		);
		// clang-format on

		return true;
	}

	// Set position.
	account.character.localPixelX = position.x();
	account.character.localPixelY = position.y();
	modTime[PROPID(PlayerProp::X)] = now;
	modTime[PROPID(PlayerProp::X2)] = now;
	modTime[PROPID(PlayerProp::Y)] = now;
	modTime[PROPID(PlayerProp::Y2)] = now;

	// Set map position.
	account.character.mapX = mapPosition.x();
	account.character.mapY = mapPosition.y();
	modTime[PROPID(PlayerProp::GMAPLEVELX)] = now;
	modTime[PROPID(PlayerProp::GMAPLEVELY)] = now;

	// Enter the level.
	return enterLevel(level, clientCachedTime);
}

bool Player::enterLevel(std::shared_ptr<Level> level, std::optional<clock::time_point> clientCachedTime)
{
	return true;
}

bool Player::leaveLevel(bool keepLevelReference)
{
	auto now = m_server->getFrameStartTime();

	account.level.clear();
	account.character.mapX = 0;
	account.character.mapY = 0;

	modTime[PROPID(PlayerProp::CURLEVEL)] = now;
	modTime[PROPID(PlayerProp::GMAPLEVELX)] = now;
	modTime[PROPID(PlayerProp::GMAPLEVELY)] = now;

	return true;
}

bool Player::leaveSubLevel(std::shared_ptr<SubLevel> subLevel)
{
	return true;
}

bool Player::sendStaticLevelData(std::shared_ptr<StaticLevelData> staticLevelData, std::shared_ptr<SubLevel> subLevel, std::optional<clock::time_point> clientCachedTime)
{
	return true;
}

bool Player::sendDynamicLevelData(std::shared_ptr<Level> level, std::optional<clock::time_point> clientCachedTime)
{
	return true;
}

bool Player::sendNearbyObjects(std::shared_ptr<Level> level)
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void Player::constructScriptParameters()
{
	if (!scriptParameters.empty())
		return;

	// clang-format off
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"id"sv, std::nullopt, std::ref(m_id)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"hurtpower"sv, std::nullopt, std::ref(account.character.hurtDeltaInHalves)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"attachid"sv, std::nullopt, std::ref(m_attachNPC)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"lastdead"sv, std::nullopt, std::ref(lastDeadTime)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"logintime"sv, std::nullopt, std::ref(loginTime)});

	bind::bindPropertyAsReadOnly(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"attachtype"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return m_attachNPC == 0 ? 0.0 : 1.0; }
	});

	bind::bindPropertyAsReadOnly(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"saysnumber"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter
		{
			// If we have an NPC-Server, we can process the chat message as a math expression and return the result.
			if (m_server->hasNPCServer())
			{
				auto npcServer = m_server->getNPCServer();
				if (auto engine = npcServer->scripting.getDefaultScriptEngine(); engine != nullptr)
				{
					// Retrieve up to the first space.
					auto line = string::retrieveLine(std::string_view{ account.character.chatMessage }, ' ');

					// GS1: Then retrieve up to a question mark, since ternary's are not supported.
					if (engine->getEngineName() == "GS1"sv)
						line = string::retrieveLine(line, '?');

					if (auto result = engine->processMathExpression(line, source::FromPlayer(m_id)); result.has_value())
						return result.value();
				}
			}

			// Fallback just processes the number.
			return string::toDouble(account.character.chatMessage);
		}
	});

	// trial, classic, vip, gold
	bind::bindPropertyAsReadOnly(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"upgradestatus"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return isGuest() ? "trial"s : "classic"s; }
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::DivideByIntegralProperty{"z"sv, std::ref(modTime[PROPID(PlayerProp::Z2)]), std::ref(account.character.localPixelZ), 16});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::DivideByIntegralProperty{"hearts"sv, std::ref(modTime[PROPID(PlayerProp::CURPOWER)]), std::ref(account.character.hitpointsInHalves), 2});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::DivideByIntegralProperty{"hp"sv, std::ref(modTime[PROPID(PlayerProp::CURPOWER)]), std::ref(account.character.hitpointsInHalves), 2});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"fullhearts"sv, std::ref(modTime[PROPID(PlayerProp::MAXPOWER)]), std::ref(account.maxHitpoints)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"maxhp"sv, std::ref(modTime[PROPID(PlayerProp::MAXPOWER)]), std::ref(account.maxHitpoints)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"mp"sv, std::ref(modTime[PROPID(PlayerProp::MAGICPOINTS)]), std::ref(account.character.mp)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"ap"sv, std::ref(modTime[PROPID(PlayerProp::ALIGNMENT)]), std::ref(account.character.ap)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"rupees"sv, std::ref(modTime[PROPID(PlayerProp::RUPEESCOUNT)]), std::ref(account.character.gralats)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"gralats"sv, std::ref(modTime[PROPID(PlayerProp::RUPEESCOUNT)]), std::ref(account.character.gralats)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"bombs"sv, std::ref(modTime[PROPID(PlayerProp::BOMBSCOUNT)]), std::ref(account.character.bombs)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"darts"sv, std::ref(modTime[PROPID(PlayerProp::ARROWSCOUNT)]), std::ref(account.character.arrows)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"glovepower"sv, std::ref(modTime[PROPID(PlayerProp::GLOVEPOWER)]), std::ref(account.character.glovePower)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"swordpower"sv, std::ref(modTime[PROPID(PlayerProp::SWORDPOWER)]), std::ref(account.character.swordPower)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"shieldpower"sv, std::ref(modTime[PROPID(PlayerProp::SHIELDPOWER)]), std::ref(account.character.shieldPower)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"shootpower"sv, std::ref(modTime[PROPID(PlayerProp::GANI)]), std::ref(account.character.bowPower)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#1"sv, std::ref(modTime[PROPID(PlayerProp::SWORDPOWER)]), std::ref(account.character.swordImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#2"sv, std::ref(modTime[PROPID(PlayerProp::SHIELDPOWER)]), std::ref(account.character.shieldImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#3"sv, std::ref(modTime[PROPID(PlayerProp::HEADGIF)]), std::ref(account.character.headImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#5"sv, std::ref(modTime[PROPID(PlayerProp::HORSEGIF)]), std::ref(account.character.horseImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#7"sv, std::ref(modTime[PROPID(PlayerProp::GANI)]), std::ref(account.character.bowImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#8"sv, std::ref(modTime[PROPID(PlayerProp::BODYIMG)]), std::ref(account.character.bodyImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#c"sv, std::ref(modTime[PROPID(PlayerProp::CURCHAT)]), std::ref(account.character.chatMessage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#m"sv, std::ref(modTime[PROPID(PlayerProp::GANI)]), std::ref(account.character.gani)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#n"sv, std::ref(modTime[PROPID(PlayerProp::NICKNAME)]), std::ref(account.character.nickName)});

	// colors
	for (size_t i = 0; i < account.character.colors.size(); ++i)
		bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{colorPropertyNames[i], std::ref(modTime[PROPID(PlayerProp::COLORS)]), std::ref(account.character.colors[i])});

	// gani attributes
	for (size_t i = 0; i < 30; ++i)
		bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{ganiAttributePropertyNames[i], std::ref(modTime[GaniAttributePropList[i]]), std::ref(account.character.ganiAttributes[i])});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"x"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return (double)account.character.getTilePosition().x(); },
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				auto globalPosition = account.character.getGlobalPosition();
				globalPosition.x() = value->get() * 16;
				account.character.localPixelX = toLocalPixelPosition(globalPosition).x();

				// Update the location props.
				auto now = m_server->getFrameStartTime();
				modTime[PROPID(PlayerProp::X)] = now;
				modTime[PROPID(PlayerProp::X2)] = now;

				// Fix the map position if applicable.
				if (auto levelPtr = getLevel(); levelPtr != nullptr && levelPtr->isGmap())
				{
					if (auto mapX = toMapPosition(globalPosition).x(); mapX != account.character.mapX)
					{
						account.character.mapX = mapX;
						modTime[PROPID(PlayerProp::GMAPLEVELX)] = now;
					}
				}
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"y"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return account.character.getTilePosition().y(); },
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				auto globalPosition = account.character.getGlobalPosition();
				globalPosition.y() = value->get() * 16;
				account.character.localPixelY = toLocalPixelPosition(globalPosition).y();

				// Update the location props.
				auto now = m_server->getFrameStartTime();
				modTime[PROPID(PlayerProp::Y)] = now;
				modTime[PROPID(PlayerProp::Y2)] = now;

				// Fix the map position if applicable.
				if (auto levelPtr = getLevel(); levelPtr != nullptr && levelPtr->isGmap())
				{
					if (auto mapY = toMapPosition(globalPosition).y(); mapY != account.character.mapY)
					{
						account.character.mapY = mapY;
						modTime[PROPID(PlayerProp::GMAPLEVELY)] = now;
					}
				}
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"headset"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter
		{
			int headSet = -1;
			if (account.character.headImage.starts_with("head"))
				string::toNumber(account.character.headImage.substr(4), headSet);
			return static_cast<double>(headSet);
		},
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			static double noHeadSet = -1.0;
			static auto noHeadRef = std::ref(noHeadSet);
			auto value = std::get_if<std::reference_wrapper<double>>(&incoming);
			if (value == nullptr)
				value = &noHeadRef;

			auto headSet = std::clamp(static_cast<int>(value->get()), -1, 99);
			if (headSet != -1)
			{
				account.character.headImage = std::format("head{}.{}", headSet, (m_server->Generation == ServerGeneration::CLASSIC ? "gif" : "png"));
				modTime[PROPID(PlayerProp::HEADGIF)] = m_server->getFrameStartTime();
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"sprite"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return static_cast<double>(account.character.sprite); },
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				account.character.sprite = static_cast<uint8_t>(value->get());
				if (account.character.sprite >= 4 && m_server->Generation != ServerGeneration::CLASSIC)
				{
					account.character.gani = std::format("def[{}]", account.character.sprite);
					modTime[PROPID(PlayerProp::GANI)] = m_server->getFrameStartTime();
				}
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"dir"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return static_cast<double>(account.character.direction); },
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				account.character.direction = std::clamp(static_cast<uint8_t>(value->get()), 0_ui8, 3_ui8);
				modTime[PROPID(PlayerProp::SPRITE)] = m_server->getFrameStartTime();
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"kills"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return static_cast<double>(account.kills); },
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				if (!m_server->getSettings().get<bool>("dontchangekills").value_or(false))
					account.kills = static_cast<uint32_t>(std::max(0.0, value->get()));
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"deaths"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return static_cast<double>(account.deaths); },
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				if (!m_server->getSettings().get<bool>("dontchangekills").value_or(false))
					account.deaths = static_cast<uint32_t>(std::max(0.0, value->get()));
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"rating"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return static_cast<double>(account.eloRating); },
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
				account.eloRating = static_cast<uint32_t>(std::max(0.0, value->get()));
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"ratingd"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return static_cast<double>(account.eloDeviation); },
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				if (!m_server->getSettings().get<bool>("dontupdateratingd").value_or(false))
					account.eloDeviation = static_cast<float>(std::clamp(value->get(), 0.0, 350.0));
			}
		}
	});

	// GR extensions.
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"carrysprite"sv, std::ref(modTime[PROPID(PlayerProp::CARRYSPRITE)]), std::ref(m_carrySprite)});

	// clang-format on
}

////////////////////////////////////////////////////////////////////////////////

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
		// clang-format off
		CString simpleHtml = CString()
			<< "<html><head><title>" APP_VENDOR " " APP_NAME " v" APP_VERSION "</title></head><body><h1>Welcome to "
			<< m_server->getSettings().get("name").value_or("")
			<< "!</h1>"
			<< m_server->getServerMessage().replaceAll("my server", m_server->getSettings().get("name").value_or("")).text()
			<< "<p style=\"font-style: italic;font-weight: bold;\">Powered by " APP_VENDOR " " APP_NAME "<br/>Programmed by "
			<< CString(APP_CREDITS) << "</p></body></html>";
		CString webResponse = CString()
			<< "HTTP/1.1 200 OK\r\nServer: " APP_VENDOR " " APP_NAME " v" APP_VERSION "\r\nContent-Length: "
			<< CString(simpleHtml.length())
			<< "\r\nContent-Type: text/html\r\n\r\n"
			<< simpleHtml << "\r\n";
		// clang-format on
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

	// clang-format off
	CString webSockHandshake = CString()
		<< "HTTP/1.1 101 Switching Protocols\r\n"
		<< "Upgrade: websocket\r\n"
		<< "Connection: Upgrade\r\n"
		<< "Sec-WebSocket-Protocol: binary\r\n"
		<< "Sec-WebSocket-Accept: "
		<< webSocketKey
		<< "\r\n\r\n";
	// clang-format on

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
	if (isJailed())
		return HandlePacketResult::Handled;

	CString message = pPacket.readString(pPacket.readGUChar());

	// Word filter.
	int filter = m_server->getWordFilter().apply(this, message, FILTER_CHECK_TOALL);
	if (filter & FILTER_ACTION_WARN)
	{
		setChat(message);
		return HandlePacketResult::Handled;
	}

	for (auto& [pid, player] : m_server->getPlayerList())
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
	// Check if the player is in a jailed level.
	bool jailed = isJailed();

	// Get the players this message was addressed to.
	std::vector<PlayerID> pmPlayers;
	auto pmPlayerCount = pPacket.readGUShort();
	for (auto i = 0; i < pmPlayerCount; ++i)
		pmPlayers.push_back(static_cast<PlayerID>(pPacket.readGUShort()));

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

	// Construct our message.
	std::string constructedMessage = std::format("#b{}:#b{}", pmPlayerCount > 1 ? "Mass message" : "Private message", pmMessage.replaceAll("\n", "#b").toString());

	// Send the message out.
	for (auto pmPlayerId : pmPlayers)
	{
		if (pmPlayerId >= PLAYERID_GEN_EXTERNAL)
		{
			auto pmPlayer = getExternalPlayer(pmPlayerId);
			if (pmPlayer != nullptr)
			{
				log::printLine(log::server, "Sending PM to global player: {}.", pmPlayer->account.character.nickName);

				// Don't send the fully constructed message to external players, just keep it formatted as-is.
				pmExternalPlayer(pmPlayer->getServerName(), pmPlayer->account.name, pmMessage);
			}
		}
		else
		{
			auto pmPlayer = m_server->getPlayer(pmPlayerId, PLTYPE_ANYPLAYER | PLTYPE_NPCSERVER);
			if (pmPlayer == nullptr || pmPlayer.get() == this) continue;

			// Don't send to people who don't want mass messages.
			if (pmPlayerCount != 1 && (pmPlayer->getProp<PlayerProp::ADDITFLAGS>().value & PLFLAG_NOMASSMESSAGE))
				continue;

			// Jailed people cannot send PMs to normal players.
			if (jailed && !isStaff() && !pmPlayer->isStaff())
			{
				sendPrivateMessage(pmPlayer->getId(), pmPlayer->translate("Server Message:#bFrom jail you can only send PMs to admins (RCs)."));
				continue;
			}

			// Send the message.
			pmPlayer->sendPrivateMessage(m_id, constructedMessage);
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_PACKETCOUNT(CString& pPacket)
{
	unsigned short count = pPacket.readGUShort();
	if (count != PacketCount || PacketCount > 10000)
	{
		log::printLine(log::server, "Warning - Player {} had an invalid packet count.", account.name);
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
