#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <string_view>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <CSocket.h>

#include <CEncryption.h>
#include <CString.h>
#include <IConfig.h>
#include <IEnums.h>
#include <IUtil.h>

#include <Account.h>
#include <Server.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <level/Level.h>
#include <level/LevelItem.h>
#include <level/Map.h>
#include <misc/WordFilter.h>
#include <network/IPacketHandler.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <object/Weapon.h>
#include <player/PlayerClient.h>
#include <player/PlayerProps.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/PropertySerializers.h>
#include <utilities/StringUtils.h>
#include <utilities/std/inplace_vector.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

using PacketHandleFunc = HandlePacketResult (PlayerClient::*)(CString&);
using PacketHandleArray = std::array<PacketHandleFunc, 256>;

static PacketHandleArray GeneratePacketHandlers()
{
	PacketHandleArray handlers{};
	handlers.fill(nullptr);

	handlers[PLI_LEVELWARP] = &PlayerClient::msgPLI_LEVELWARP;
	handlers[PLI_BOARDMODIFY] = &PlayerClient::msgPLI_BOARDMODIFY;
	handlers[PLI_NPCPROPS] = &PlayerClient::msgPLI_NPCPROPS;
	handlers[PLI_BOMBADD] = &PlayerClient::msgPLI_BOMBADD;
	handlers[PLI_BOMBDEL] = &PlayerClient::msgPLI_BOMBDEL;
	handlers[PLI_HORSEADD] = &PlayerClient::msgPLI_HORSEADD;
	handlers[PLI_HORSEDEL] = &PlayerClient::msgPLI_HORSEDEL;
	handlers[PLI_ARROWADD] = &PlayerClient::msgPLI_ARROWADD;
	handlers[PLI_FIRESPY] = &PlayerClient::msgPLI_FIRESPY;
	handlers[PLI_THROWCARRIED] = &PlayerClient::msgPLI_THROWCARRIED;
	handlers[PLI_ITEMADD] = &PlayerClient::msgPLI_ITEMADD;
	handlers[PLI_ITEMDEL] = &PlayerClient::msgPLI_ITEMDEL;
	handlers[PLI_CLAIMPKER] = &PlayerClient::msgPLI_CLAIMPKER;
	handlers[PLI_BADDYPROPS] = &PlayerClient::msgPLI_BADDYPROPS;
	handlers[PLI_BADDYHURT] = &PlayerClient::msgPLI_BADDYHURT;
	handlers[PLI_BADDYADD] = &PlayerClient::msgPLI_BADDYADD;
	handlers[PLI_FLAGSET] = &PlayerClient::msgPLI_FLAGSET;
	handlers[PLI_FLAGDEL] = &PlayerClient::msgPLI_FLAGDEL;
	handlers[PLI_OPENCHEST] = &PlayerClient::msgPLI_OPENCHEST;
	handlers[PLI_PUTNPC] = &PlayerClient::msgPLI_PUTNPC;
	handlers[PLI_NPCDEL] = &PlayerClient::msgPLI_NPCDEL;
	handlers[PLI_WANTFILE] = &PlayerClient::msgPLI_WANTFILE;
	handlers[PLI_SHOWIMGPLAYER] = &PlayerClient::msgPLI_SHOWIMGPLAYER;
	handlers[PLI_HURTPLAYER] = &PlayerClient::msgPLI_HURTPLAYER;
	handlers[PLI_EXPLOSION] = &PlayerClient::msgPLI_EXPLOSION;
	handlers[PLI_PRIVATEMESSAGE] = &PlayerClient::msgPLI_PRIVATEMESSAGE;
	handlers[PLI_NPCWEAPONDEL] = &PlayerClient::msgPLI_NPCWEAPONDEL;
	handlers[PLI_LEVELWARPMOD] = &PlayerClient::msgPLI_LEVELWARP; // Shared with PLI_LEVELWARP
	handlers[PLI_ITEMTAKE] = &PlayerClient::msgPLI_ITEMDEL;       // Shared with PLI_ITEMDEL
	handlers[PLI_WEAPONADD] = &PlayerClient::msgPLI_WEAPONADD;
	handlers[PLI_UPDATEFILE] = &PlayerClient::msgPLI_UPDATEFILE;
	handlers[PLI_ADJACENTLEVEL] = &PlayerClient::msgPLI_ADJACENTLEVEL;
	handlers[PLI_HITOBJECTS] = &PlayerClient::msgPLI_HITOBJECTS;
	handlers[PLI_TRIGGERACTION] = &PlayerClient::msgPLI_TRIGGERACTION;
	handlers[PLI_TAMPERCHECK] = &PlayerClient::msgPLI_TAMPERCHECK;
	handlers[PLI_SHOOT] = &PlayerClient::msgPLI_SHOOT;
	handlers[PLI_SERVERWARP] = &PlayerClient::msgPLI_SERVERWARP;
	handlers[PLI_PROCESSLIST] = &PlayerClient::msgPLI_PROCESSLIST;
	handlers[PLI_ENTERLEVEL] = &PlayerClient::msgPLI_ENTERLEVEL;
	handlers[PLI_VERIFYWANTSEND] = &PlayerClient::msgPLI_VERIFYWANTSEND;
	handlers[PLI_SHOOT2] = &PlayerClient::msgPLI_SHOOT2;
	handlers[PLI_REQUESTUPDATEBOARD] = &PlayerClient::msgPLI_REQUESTUPDATEBOARD;
	handlers[PLI_UPDATEGANI] = &PlayerClient::msgPLI_UPDATEGANI;
	handlers[PLI_UPDATESCRIPT] = &PlayerClient::msgPLI_UPDATESCRIPT;
	handlers[PLI_UPDATEPACKAGEREQUESTFILE] = &PlayerClient::msgPLI_UPDATEPACKAGEREQUESTFILE;
	handlers[PLI_UPDATECLASS] = &PlayerClient::msgPLI_UPDATECLASS;

	return handlers;
}

///////////////////////////////////////////////////////////////////////////////

HandlePacketResult PlayerClient::handlePacket(const std::optional<uint8_t> id, CString& packet)
{
	static PacketHandleArray PacketHandlers = GeneratePacketHandlers();

	const auto handle = id.has_value() ? PacketHandlers[id.value()] : nullptr;
	if (handle == nullptr)
		return Player::handlePacket(id, packet);

	const auto result = (this->*handle)(packet);
	if (result == HandlePacketResult::Bubble)
		return Player::handlePacket(id, packet);

	return result;
}

///////////////////////////////////////////////////////////////////////////////

PlayerClient::PlayerClient(CSocket* pSocket, const PlayerID pId)
	: Player(pSocket, pId)
{
	m_lastMovement = m_lastSave = m_last1m = clock::now();
}

PlayerClient::~PlayerClient()
{
	PlayerClient::cleanup();
}

void PlayerClient::cleanup()
{
	if (m_id > 0 && m_server != nullptr && m_loaded)
	{
		// Adjust carried NPC location.
		if (m_carryNPC != 0)
		{
			if (const auto npc = m_server->getNPC(m_carryNPC); npc)
			{
				npc->sendPropsFromResults(
					npc->setPropWith<NPCProp::X2>(SetBy::CLIENT, static_cast<int16_t>(account.character.localPixelX + 8)),
					npc->setPropWith<NPCProp::Y2>(SetBy::CLIENT, static_cast<int16_t>(account.character.localPixelY + 16))
				);
			}
			m_carryNPC = 0;
		}
	}

	// Clean up.
	m_cachedStaticLevels.clear();
	m_cachedDynamicLevels.clear();
	m_singleplayerLevels.clear();

	Player::cleanup();
}

///////////////////////////////////////////////////////////////////////////////

void PlayerClient::doMain()
{
	Player::doMain();

	// Update the -gr_movement packets.
	if (!m_grMovementPackets.isEmpty())
	{
		if (!m_grMovementUpdated)
		{
			for (auto& i : m_grMovementPackets.tokenize("\n"))
				setPropsFromPacket(i, props::SetBy::CLIENT);
		}
		m_grMovementPackets.clear(42);
	}
	m_grMovementUpdated = false;
}

bool PlayerClient::doTimedEvents()
{
	if (!Player::doTimedEvents())
		return false;

	const auto currTime = m_server->getFrameStartTime();

	// Increase online time.
	++account.onlineSeconds;

	// Disconnect if no data has been sent or received in 5 minutes.
	if (timeDifference(currTime, m_lastData) > 300s)
	{
		log::printLine(log::server, "** [Disconnect] {}: Client has timed out.", account.name);
		return false;
	}

	// Disconnect if players are inactive.
	if (m_server->cached.enableIdleDisconnect.getValue())
	{
		const int maxnomovement = m_server->cached.idleTimeoutSeconds.getValue();
		if (timeDifference(currTime, m_lastMovement) > std::chrono::seconds{maxnomovement} && timeDifference(currTime, m_lastChat) > std::chrono::seconds{maxnomovement})
		{
			log::printLine(log::server, "** [Disconnect] {}: Client has been disconnected due to inactivity.", account.name);
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You have been disconnected due to inactivity.");
			return false;
		}
	}

	// Increase player AP.
	if (m_server->cached.enableAPSystem.getValue() && !m_currentLevel.expired())
	{
		if (const auto subLevel = getSubLevel(); subLevel != nullptr)
		{
			if (!(account.status & PLSTATUS_PAUSED) && !subLevel->isSparringZone)
			{
				if (account.apCounter > 0)
					--account.apCounter;
				else
				{
					if (account.character.ap < 100)
						sendPropsFromResults(setPropWith<PlayerProp::ALIGNMENT>(props::SetBy::SERVER, static_cast<uint8_t>(account.character.ap + 1)));

					if (account.character.ap < 20)
						account.apCounter = m_server->cached.apSystemThresholdSeconds[0].getValue();
					else if (account.character.ap < 40)
						account.apCounter = m_server->cached.apSystemThresholdSeconds[1].getValue();
					else if (account.character.ap < 60)
						account.apCounter = m_server->cached.apSystemThresholdSeconds[2].getValue();
					else if (account.character.ap < 80)
						account.apCounter = m_server->cached.apSystemThresholdSeconds[3].getValue();
					else
						account.apCounter = m_server->cached.apSystemThresholdSeconds[4].getValue();
				}
			}
		}
	}

	// Do singleplayer level events.
	{
		for (auto& level : m_singleplayerLevels | std::views::values)
		{
			if (level)
				level->doTimedEvents();
		}
	}

	// Save player account every 5 minutes.
	if (timeDifference(currTime, m_lastSave) > 300s)
	{
		m_lastSave = currTime;
		if (isClient() && m_loaded && !account.loadOnly)
			m_server->getAccountLoader().saveAccount(account);
	}

	// Events that happen every minute.
	if (timeDifference(currTime, m_last1m) > 60s)
	{
		m_last1m = currTime;
		InvalidPackets = 0;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool PlayerClient::handleLogin(CString& pPacket)
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
	log::print(log::server, "New login:   ");
	switch (m_type)
	{
		case PLTYPE_CLIENT:
			log::printLine(log::server, "Client");
			Encryption.setGen(ENCRYPT_GEN_2);
			break;
		case PLTYPE_CLIENT2:
			log::printLine(log::server, "New Client (2.19 - 2.21, 3 - 3.01)");
			Encryption.setGen(ENCRYPT_GEN_4);
			break;
		case PLTYPE_CLIENT3:
			log::printLine(log::server, "New Client (2.22+)");
			Encryption.setGen(ENCRYPT_GEN_5);
			break;
		case PLTYPE_WEB:
			log::printLine(log::server, "Web");
			Encryption.setGen(ENCRYPT_GEN_1);
			m_fileQueue.setCodec(ENCRYPT_GEN_1, 0);
			break;
		default:
			log::printLine(log::server, "Unknown ({})", m_type);
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Your client type is unknown.  Please inform the " << APP_VENDOR << " Team.  Type: " << CString((int)m_type) << ".");
			return false;
	}

	// Handle old clients.
	if (m_type == PLTYPE_CLIENT)
	{
		// Read Client-Version for v1.3 clients
		m_version = pPacket.readChars(8);
		m_versionId = getVersionID(m_version);

		// 1.41 registers itself as PLTYPE_CLIENT, but does include an encryption key.
		if (m_versionId == CLVER_UNKNOWN)
		{
			Encryption.setGen(ENCRYPT_GEN_3);
			pPacket.setRead(1);
		}
	}

	// Handle newer clients.
	if (m_versionId == CLVER_UNKNOWN)
	{
		m_encryptionKey = static_cast<unsigned char>(pPacket.readGChar());

		Encryption.reset(m_encryptionKey);
		if (Encryption.getGen() > ENCRYPT_GEN_3)
			m_fileQueue.setCodec(Encryption.getGen(), m_encryptionKey);

		// Read Client-Version
		m_version = pPacket.readChars(8);
		m_versionId = getVersionIDByVersion(m_version);
	}

	// Read Account & Password
	account.name = pPacket.readChars(pPacket.readGUChar()).toString();
	const CString password = pPacket.readChars(pPacket.readGUChar());

	// Client Identity: win,"",02e2465a2bf38f8a115f6208e9938ac8,ff144a9abb9eaff4b606f0336d6d8bc5,"6.2 9200 "
	//					{platform}, {mobile provides 'dc:id2'}, {md5hash:harddisk-id}, {md5hash:network-id}, {uname(release, version)}, {android-id}
	const CString identity = pPacket.readString("");

	{
		auto indent = log::server.indent();

		//log::printLine(log::server, "Key: {}", key);
		log::printLine(log::server, "Version:     {} ({})", m_version, getVersionString(m_version, m_type));
		log::printLine(log::server, "Account:     {}", account.name);
		if (!identity.isEmpty())
		{
			log::printLine(log::server, "Identity:    {}", identity);
			const auto identityTokens = identity.tokenize(",", true);
			account.platform = identityTokens[0];
		}
	}

	// Check if the specified client is allowed access.
	{
		auto& allowedVersions = m_server->getAllowedVersions();
		bool allowed = false;
		for (CString ver : allowedVersions)
		{
			if (ver.find(":") != -1)
			{
				CString ver1 = ver.readString(":").trim();
				CString ver2 = ver.readString("").trim();
				const int aVersion[2] = {getVersionID(ver1), getVersionID(ver2)};
				if (m_versionId >= aVersion[0] && m_versionId <= aVersion[1])
				{
					allowed = true;
					break;
				}
			}
			else
			{
				if (const int aVersion = getVersionID(ver); m_versionId == aVersion)
				{
					allowed = true;
					break;
				}
			}
		}
		if (!allowed)
		{
			log::printLine(log::rc, "** [Disconnect] '{}': Client version not allowed. (Version: {} {})", account.name, m_version, getVersionString(m_version, m_type));
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Your client version is not allowed on this server.\rAllowed: " << m_server->getAllowedVersionString());
			return false;
		}
	}

	// Check for available slots on the server.
	if (m_server->getPlayerList().size() >= m_server->cached.maxPlayers.getValue())
	{
		log::printLine(log::rc, "** [Disconnect] '{}': Server is full.", account.name);
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "This server has reached its player limit.");
		return false;
	}

	if (m_server->cached.localhostMode.getValue())
	{
		log::printLine(log::server, "[Login] Localhost mode: accepting '{}' without listserver verification.", account.name);
		return sendLogin();
	}

	// Verify login details with the serverlist.
	if (!m_server->getServerList().getConnected())
	{
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "The login server is offline.  Try again later.");
		return false;
	}

	m_server->getServerList().sendLoginPacketForPlayer(shared_from_this(), password, identity);
	return true;
}

bool PlayerClient::sendLogin()
{
	if (Player::sendLogin() == false)
		return false;

	// Unset loaded until after we have fully logged in and sent the level to the player.
	m_loaded = false;

	auto& settings = m_server->getSettings();
	bool hasNPCServer = m_server->hasNPCServer();

	// Recalculate player spar deviation.
	{
		// c = sqrt( (350*350 - 50*50) / t )
		// where t is the number of rating periods for deviation to go from 50 to 350.
		// t = 60 days for us.
		constexpr float c = 44.721f;
		const auto current_time = std::chrono::system_clock::now();
		const auto time_difference = current_time - account.lastSparTime;
		const auto days = std::chrono::duration_cast<std::chrono::days>(time_difference).count();
		if (days != 0)
		{
			// Find the new deviation.
			float deviate = std::min(350.0f, std::sqrt((account.eloDeviation * account.eloDeviation) + (c * c) * static_cast<float>(days)));

			// Set the new rating.
			account.eloDeviation = deviate;
			account.lastSparTime = current_time;
		}
	}

	// Stage 0 - Server info.
	{
		// Workaround for the 2.31 client.  It doesn't request the map file when used with setmap.
		// So, just send them all the maps loaded into the server.
		if (m_versionId == CLVER_2_31 || m_versionId == CLVER_1_411)
		{
			for (const auto& map : m_server->getMapList())
			{
				if (map->isBigMap())
					msgPLI_WANTFILE(CString() << map->getMapName());
			}
		}

		// Tell the client if the server is connected to the listserver.
		if (m_server->getServerList().getConnected())
			sendPacket(CString() >> (char)PLO_SERVERLISTCONNECTED);

		// Send the bigmap if it was set.
		if (m_versionId >= CLVER_2_1)
		{
			if (const CString bigmap = settings.get("bigmap").value_or(""); !bigmap.isEmpty())
			{
				if (const std::vector<CString> vbigmap = bigmap.tokenize(","); vbigmap.size() == 4)
					sendPacket(CString() >> (char)PLO_BIGMAP << vbigmap[0].trim() << "," << vbigmap[1].trim() << "," << vbigmap[2].trim() << "," << vbigmap[3].trim());
			}
		}

		// Send the minimap if it was set.
		if (m_versionId >= CLVER_2_1)
		{
			if (const CString minimap = settings.get("minimap").value_or(""); !minimap.isEmpty())
			{
				if (const std::vector<CString> vminimap = minimap.tokenize(","); vminimap.size() == 4)
					sendPacket(CString() >> (char)PLO_MINIMAP << vminimap[0].trim() << "," << vminimap[1].trim() << "," << vminimap[2].trim() << "," << vminimap[3].trim());
			}
		}

		// Send out RPG Window greeting.
		if (m_versionId >= CLVER_2_1)
			sendPacket(CString() >> (char)PLO_RPGWINDOW << "\"Welcome to " << settings.get("name").value_or("") << ".\",\"" << CString(APP_VENDOR) << " " << CString(APP_NAME) << " programmed by " << CString(APP_CREDITS) << ".\"");

		// Send the start message to the player.
		sendPacket(CString() >> (char)PLO_STARTMESSAGE << m_server->getServerMessage());

		// This will allow serverwarp and some other things, for some reason.
		sendPacket(CString() >> (char)PLO_SERVERTEXT);

		// Send out what guilds should be placed in the Staff section of the playerlist.
		CString guildPacket = CString() >> (char)PLO_STAFFGUILDS;
		guildPacket << string::toCSV(string::split(settings.get<std::string>("staffguilds").value_or(""), ","sv) | range::string_trim);
		sendPacket(guildPacket);

		// Send out the server's available status list options.
		if (m_versionId >= CLVER_2_1)
		{
			// graal doesn't quote these
			CString pliconPacket = CString() >> (char)PLO_STATUSLIST;
			pliconPacket << string::join(m_server->cached.playerStatusList.getValue() | range::string_trim, ","sv);
			sendPacket(pliconPacket);
		}

		// Ask for processes. This causes windows v6 clients to crash
		/*
		if (m_versionId < CLVER_6_015)
			sendPacket(CString() >> (char)PLO_LISTPROCESSES);
		*/

		m_fileQueue.sendCompress();
	}

	// Stage 1 - Player props
	{
		// Send the player his login props.
		sendPacket(CString() >> (char)PLO_PLAYERPROPS << getPropsPacketFromList(loginPropsClientSelf));

		m_fileQueue.sendCompress();
	}

	// Stage 2 - Flags
	{
		// If the gr.ip hack is enabled, add it to the player's flag list.
		if (settings.get<bool>("flaghack_ip").value_or(false) == true)
			setFlag("gr.ip", account.ipAddress, SetBy::SERVER);

		// Send the player's flags.
		for (const auto& [flag, value] : account.variables.store | variables::serializable)
		{
			if (auto serialized = account.variables.serializeModern(flag); serialized.has_value())
				sendPacket(CString() >> (char)PLO_FLAGSET << serialized.value());
		}

		// Send the server's flags to the player.
		for (const auto& flag : m_server->Scripting.variables.store | std::views::keys)
		{
			if (hasNPCServer && !flag.starts_with("serverr.")) continue;
			if (auto serialized = m_server->Scripting.variables.serializeModern(flag); serialized.has_value())
				sendPacket(CString() >> (char)PLO_FLAGSET << serialized.value());
		}

		m_fileQueue.sendCompress();
	}

	// Stage 3 - Weapons
	{
		// Sent to rc and client, but rc ignores it so...
		sendPacket(CString() >> (char)PLO_CLEARWEAPONS);

		// Delete the bomb and bow.  They get automagically added by the client for
		// God knows which reason.  Bomb and Bow must be capitalized.
		sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << "Bomb");
		sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << "Bow");

		// Send the player's weapons.
		for (const auto& weaponName : account.weapons)
		{
			auto weapon = m_server->getWeapon(weaponName);
			if (weapon == nullptr)
			{
				// Let's check to see if it is a default weapon.  If so, we can add it to the server now.
				if (auto itemType = LevelItem::getItemId(weaponName); itemType != LevelItemType::INVALID)
				{
					CString defWeapPacket = CString() >> (char)PLI_WEAPONADD >> (char)0 >> (char)LevelItem::getItemTypeId(itemType);
					defWeapPacket.readGChar();
					msgPLI_WEAPONADD(defWeapPacket);
					continue;
				}
				continue;
			}
			weapon->registerWeaponWithPlayer(shared_from_this());
		}

		// Send any protected weapons we do not have.
		for (const auto& weapon : m_server->cached.protectedWeapons.getValue())
		{
			if (!account.hasWeapon(weapon))
				this->addWeapon(weapon);
		}

		m_fileQueue.sendCompress();
	}

	// Stage 4 - Other player's props.
	{
		// Exchange props with everybody on the server.
		exchangeMyPropsWithOthers();

		// Record prop mod time.
		auto curTime = currentTime();
		std::ranges::for_each(modTime, [&curTime](auto& modTime)
		{
			modTime = curTime;
		});

		m_fileQueue.sendCompress(true);
	}

	// Stage 5 - The level.
	{
		// Send the level to the player.
		// warp will call sendCompress() for us.
		if (!warp(account.level, getGlobalPosition()) && m_currentLevel.expired())
		{
			log::printLine(log::rc, "** [Disconnect] '{}': No level available for player.", account.name);
			m_fileQueue.clearBuffers();
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "No level available.");
			log::printLine(log::server, "** Cannot find level for {}.", account.name);
			return false;
		}

		m_fileQueue.sendCompress();
	}

	// Queue up the login event.
	if (m_server->hasNPCServer())
	{
		auto npcServer = m_server->getNPCServer();
		npcServer->playerLogin(shared_from_this());
		npcServer->addEventToControlNPC(ScriptEventType::TRIGGERACTION, source::FromPlayer(m_id), "playeronline");
		npcServer->addEventToControlNPC(ScriptEventType::PLAYERLOGIN, source::FromPlayer(m_id));
	}

	// Fully loaded now.
	m_loaded = true;

	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool PlayerClient::processChat(const CString& pChat)
{
	std::vector<CString> chatParse = pChat.tokenizeConsole();
	if (chatParse.empty()) return false;
	bool processed = false;
	bool setcolorsallowed = m_server->getSettings().get<bool>("setcolorsallowed").value_or(true);

	if (chatParse[0] == "setnick")
	{
		processed = true;
		if (timeDifference(m_server->getFrameStartTime(), m_lastNick) >= 10s)
		{
			m_lastNick = m_server->getFrameStartTime();
			CString newName = pChat.subString(8).trim();

			// Word filter.
			if (const int filter = m_server->getWordFilter().apply(this, newName, FILTER_CHECK_NICK); filter & FILTER_ACTION_WARN)
			{
				setChat(newName);
				return true;
			}

			// SetBy::CLIENT so the server applies nickname restrictions on the player.
			auto result = setPropWith<PlayerProp::NICKNAME>(props::SetBy::CLIENT, newName.toString());
			result.resultFlags.set(props::SetResults::sendToSource);
			sendPropsFromResults(result);
		}
		else
		{
			setChat("Wait 10 seconds before changing your nick again!");
		}
	}
	else if (chatParse[0] == "sethead" && chatParse.size() == 2)
	{
		if (!m_server->getSettings().get<bool>("setheadallowed").value_or(true)) return false;
		processed = true;

		// Try to find the file.
		auto& filesystem = m_server->getFileSystem();
		auto file = filesystem.findi(fs::FileCategory::HEAD, chatParse[1].toStringView());
		if (file.empty())
		{
			int i = 0;
			const char* ext[] = {".png", ".mng", ".gif"};
			while (i < 3)
			{
				file = filesystem.findi(fs::FileCategory::HEAD, std::format("{}{}", chatParse[1].toStringView(), ext[i]));
				if (!file.empty())
				{
					chatParse[1] << ext[i];
					break;
				}
				++i;
			}
		}

		// Try to load the file.
		if (!file.empty())
			sendPropsFromResults(setPropWith<PlayerProp::HEADIMAGE>(props::SetBy::SERVER, chatParse[1].toString()));
		else
			m_server->getServerList().sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)m_id >> (char)0 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setbody" && chatParse.size() == 2)
	{
		if (m_server->getSettings().get<bool>("setbodyallowed").value_or(true) == false) return false;
		processed = true;

		// Check to see if it is a default body.
		bool isDefault = false;
		for (const auto& entry : DefaultBodies)
			if (chatParse[1].match(CString(entry.data())) == true) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			sendPropsFromResults(setPropWith<PlayerProp::BODYIMAGE>(props::SetBy::SERVER, chatParse[1].toString()));
			return false;
		}

		// Try to find the file.
		auto& filesystem = m_server->getFileSystem();
		auto file = filesystem.findi(fs::FileCategory::BODY, chatParse[1].toStringView());
		if (file.empty())
		{
			int i = 0;
			const char* ext[] = {".png", ".mng", ".gif"};
			while (i < 3)
			{
				file = filesystem.findi(fs::FileCategory::BODY, std::format("{}{}", chatParse[1].toStringView(), ext[i]));
				if (!file.empty())
				{
					chatParse[1] << ext[i];
					break;
				}
				++i;
			}
		}

		// Try to load the file.
		if (!file.empty())
			sendPropsFromResults(setPropWith<PlayerProp::BODYIMAGE>(props::SetBy::SERVER, chatParse[1].toString()));
		else
			m_server->getServerList().sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)m_id >> (char)1 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setsword" && chatParse.size() == 2)
	{
		if (!m_server->getSettings().get<bool>("setswordallowed").value_or(true)) return false;
		processed = true;

		// Check to see if it is a default sword.
		bool isDefault = false;
		for (const auto& entry : DefaultSwords)
			if (chatParse[1].match(CString(entry.data())) == true) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			sendPropsFromResults(setPropWith<PlayerProp::SWORDIMAGE>(props::SetBy::SERVER, chatParse[1].toString()));
			return false;
		}

		// Try to find the file.
		auto& filesystem = m_server->getFileSystem();
		auto file = filesystem.findi(fs::FileCategory::SWORD, chatParse[1].toStringView());
		if (file.empty())
		{
			int i = 0;
			const char* ext[] = {".png", ".mng", ".gif"};
			while (i < 3)
			{
				file = filesystem.findi(fs::FileCategory::SWORD, std::format("{}{}", chatParse[1].toStringView(), ext[i]));
				if (!file.empty())
				{
					chatParse[1] << ext[i];
					break;
				}
				++i;
			}
		}

		// Try to load the file.
		if (!file.empty())
			sendPropsFromResults(setPropWith<PlayerProp::SWORDIMAGE>(props::SetBy::SERVER, chatParse[1].toString()));
		else
			m_server->getServerList().sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)m_id >> (char)2 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setshield" && chatParse.size() == 2)
	{
		if (!m_server->getSettings().get<bool>("setshieldallowed").value_or(true)) return false;
		processed = true;

		// Check to see if it is a default shield.
		bool isDefault = false;
		for (const auto& entry : DefaultShields)
		{
			if (chatParse[1].match(CString(entry.data())) == true)
				isDefault = true;
		}

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			sendPropsFromResults(setPropWith<PlayerProp::SHIELDIMAGE>(props::SetBy::SERVER, chatParse[1].toString()));
			return false;
		}

		// Try to find the file.
		auto& filesystem = m_server->getFileSystem();
		auto file = filesystem.findi(fs::FileCategory::SHIELD, chatParse[1].toStringView());
		if (file.empty())
		{
			int i = 0;
			const char* ext[] = {".png", ".mng", ".gif"};
			while (i < 3)
			{
				file = filesystem.findi(fs::FileCategory::SHIELD, std::format("{}{}", chatParse[1].toStringView(), ext[i]));
				if (!file.empty())
				{
					chatParse[1] << ext[i];
					break;
				}
				++i;
			}
		}

		// Try to load the file.
		if (!file.empty())
			sendPropsFromResults(setPropWith<PlayerProp::SHIELDIMAGE>(props::SetBy::SERVER, chatParse[1].toString()));
		else
			m_server->getServerList().sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)m_id >> (char)3 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setskin" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 0
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		if (const signed char color = getColor(chatParse[1].toLower()); color != -1)
		{
			account.character.colors[ENUM(ColorSlots::SKIN)] = color;
			setPropsFromPacket(CString() >> (char)PlayerProp::COLORS >> (char)account.character.colors[0] >> (char)account.character.colors[1] >> (char)account.character.colors[2] >> (char)account.character.colors[3] >> (char)account.character.colors[4], props::SetBy::SERVER);
		}
	}
	else if (chatParse[0] == "setcoat" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 1
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		if (const signed char color = getColor(chatParse[1].toLower()); color != -1)
		{
			account.character.colors[ENUM(ColorSlots::COAT)] = color;
			setPropsFromPacket(CString() >> (char)PlayerProp::COLORS >> (char)account.character.colors[0] >> (char)account.character.colors[1] >> (char)account.character.colors[2] >> (char)account.character.colors[3] >> (char)account.character.colors[4], props::SetBy::SERVER);
		}
	}
	else if (chatParse[0] == "setsleeves" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 2
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		if (const signed char color = getColor(chatParse[1].toLower()); color != -1)
		{
			account.character.colors[ENUM(ColorSlots::SLEEVES)] = color;
			setPropsFromPacket(CString() >> (char)PlayerProp::COLORS >> (char)account.character.colors[0] >> (char)account.character.colors[1] >> (char)account.character.colors[2] >> (char)account.character.colors[3] >> (char)account.character.colors[4], props::SetBy::SERVER);
		}
	}
	else if (chatParse[0] == "setshoes" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 3
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		if (const signed char color = getColor(chatParse[1].toLower()); color != -1)
		{
			account.character.colors[ENUM(ColorSlots::SHOES)] = color;
			setPropsFromPacket(CString() >> (char)PlayerProp::COLORS >> (char)account.character.colors[0] >> (char)account.character.colors[1] >> (char)account.character.colors[2] >> (char)account.character.colors[3] >> (char)account.character.colors[4], props::SetBy::SERVER);
		}
	}
	else if (chatParse[0] == "setbelt" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 4
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		if (const signed char color = getColor(chatParse[1].toLower()); color != -1)
		{
			account.character.colors[ENUM(ColorSlots::BELT)] = color;
			setPropsFromPacket(CString() >> (char)PlayerProp::COLORS >> (char)account.character.colors[0] >> (char)account.character.colors[1] >> (char)account.character.colors[2] >> (char)account.character.colors[3] >> (char)account.character.colors[4], props::SetBy::SERVER);
		}
	}
	else if (chatParse[0] == "warpto")
	{
		if (m_server->getSettings().get<bool>("ignorewarpto").value_or(false))
			return false;

		processed = true;

		const bool warptoforall = m_server->getSettings().get<bool>("warptoforall").value_or(false);
		const bool warpto = m_server->getSettings().get<bool>("warpto").value_or(true);

		// Check if warpto has been disabled for staff.
		if (isStaff() && !warpto && !warptoforall)
		{
			setChat("(warping is disabled)");
			return true;
		}

		// To player
		if (chatParse.size() == 2)
		{
			// Permission check.
			if (!account.hasRight(PLPERM_WARPTOPLAYER) && !warptoforall)
			{
				setChat("(not authorized to warp)");
				return true;
			}

			auto player = m_server->getPlayer<PlayerClient>(chatParse[1], PLTYPE_ANYCLIENT);
			LevelPtr level;
			if (player && (level = player->getLevel()) != nullptr)
				warp(level->levelName, player->getLocalPosition());
		}
		// To location
		else
		{
			// Permission check.
			if (!account.hasRight(PLPERM_WARPTO) && !warptoforall)
			{
				setChat("(not authorized to warp)");
				return true;
			}

			// x y
			if (chatParse.size() == 3)
				setPropsFromPacket(CString() >> (char)PlayerProp::X >> (char)(strtofloat(chatParse[1]) * 2) >> (char)PlayerProp::Y >> (char)(strtofloat(chatParse[2]) * 2), props::SetBy::SERVER);
			// x y level
			else if (chatParse.size() == 4)
				warp(chatParse[3], {static_cast<int16_t>(string::toFloat(chatParse[1].toString()) * 16.0f), static_cast<int16_t>(string::toFloat(chatParse[2].toString()) * 16.0f)});
		}
	}
	else if (chatParse[0] == "summon" && chatParse.size() == 2)
	{
		processed = true;

		// Permission check.
		if (!account.hasRight(PLPERM_SUMMON))
		{
			setChat("(not authorized to summon)");
			return true;
		}

		auto p = m_server->getPlayer<PlayerClient>(chatParse[1], PLTYPE_ANYCLIENT);
		if (p) p->warp(account.level, getLocalPosition());
	}
	else if (chatParse[0] == "unstick" || chatParse[0] == "unstuck")
	{
		if (chatParse.size() == 2 && chatParse[1] == "me")
		{
			processed = true;

			// Check if the player is in a jailed level.
			if (isJailed())
				return false;

			const int unstickTime = m_server->cached.unstickMeSeconds.getValue();
			if (timeDifference(m_server->getFrameStartTime(), m_lastMovement) < std::chrono::seconds{unstickTime})
				setChat(CString() << "Don't move for " << CString(unstickTime) << " seconds before doing '" << pChat << "'!");
			else
			{
				m_lastMovement = m_server->getFrameStartTime();
				const auto& unstickLevel = m_server->cached.unstickMeLevel.getValue();
				const auto& unstickX = m_server->cached.unstickMeTile[0].getValue();
				const auto& unstickY = m_server->cached.unstickMeTile[1].getValue();
				warp(unstickLevel, {static_cast<int16_t>(unstickX * 16.0f), static_cast<int16_t>(unstickY * 16.0f)});
				setChat("Warped!");
			}
		}
	}
	else if (pChat == "update level" && account.hasRight(PLPERM_UPDATELEVEL))
	{
		processed = true;

		if (auto level = getLevel(); level != nullptr)
			(void)level->reload(getMapPosition());
	}
	else if (pChat == "showadmins" && m_server->getSettings().get<bool>("disableshowadmins").value_or(false) == false)
	{
		processed = true;

		// Search through the player list for all RC's.
		CString msg;
		{
			for (auto& playerList = m_server->getPlayerList(); auto& player : playerList | std::views::values)
			{
				// If an RC was found, add it to our string.
				if (player->getType() & PLTYPE_ANYRC)
					msg << (msg.length() == 0 ? "" : ", ") << player->account.name;
			}
		}
		if (msg.length() == 0)
			msg << "(no one)";
		setChat(CString("admins: ") << msg);
	}
	else if (chatParse[0] == "showguild")
	{
		processed = true;
		CString g = m_guild;

		// If a guild was specified, overwrite our guild with it.
		if (chatParse.size() == 2)
			g = chatParse[1];

		if (g.length() != 0)
		{
			CString msg;
			{
				for (auto& playerList = m_server->getPlayerList(); auto& player : playerList | std::views::values)
				{
					// If our guild matches, add it to our string.
					if (player->getGuild() == g)
						msg << (msg.length() == 0 ? "" : ", ") << CString(player->account.character.nickName).subString(0, static_cast<int>(player->account.character.nickName.find('('))).trimI();
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
		setChat(CString() << "kills: " << CString(static_cast<int>(account.kills)));
	}
	else if (pChat == "showdeaths")
	{
		processed = true;
		setChat(CString() << "deaths: " << CString(static_cast<int>(account.deaths)));
	}
	else if (pChat == "showonlinetime")
	{
		processed = true;
		const auto seconds = account.onlineSeconds % 60;
		const auto minutes = (account.onlineSeconds / 60) % 60;
		const auto hours = account.onlineSeconds / 3600;
		CString msg;
		if (hours != 0) msg << CString(hours) << "h ";
		if (minutes != 0 || hours != 0) msg << CString(minutes) << "m ";
		msg << CString(seconds) << "s";
		setChat(CString() << "onlinetime: " << msg);
	}
	else if (chatParse[0] == "toguild:")
	{
		processed = true;
		if (m_guild.length() == 0) return false;

		// Get the PM.
		CString pm = pChat.text() + 8;
		pm.trimI();
		if (pm.length() == 0) return false;

		// Send PM to guild members.
		int num = 0;
		{
			for (auto& playerList = m_server->getPlayerList(); auto& player : playerList | std::views::values)
			{
				// If our guild matches, send the PM.
				if (player->getGuild() == m_guild)
				{
					player->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)m_id << R"("","Guild message:",")" << pm << "\"");
					++num;
				}
			}
		}

		// Tell the player how many guild members received his message.
		setChat(CString() << "(" << CString(num) << " guild member" << (num != 0 ? "s" : "") << " received your message)");
	}
#ifdef DEBUG
	else if (pChat == "savenpcs" && m_server->hasNPCServer())
	{
		processed = true;
		m_server->getNPCServer()->saveNPCs();
		setChat(CString() << "(saved npcs)");
	}
#endif

	return processed;
}

///////////////////////////////////////////////////////////////////////////////

void PlayerClient::setGroup(const std::string_view group)
{
	// Clear any cached level data from the client that belongs to the old group.
	if (!account.groupName.empty())
		resetLevelCache(account.groupName);

	// Finally, set the new group.
	if (group.empty())
		account.groupName.clear();
	else
		account.groupName = std::format("gr.{}", string::toLower(group));
}

///////////////////////////////////////////////////////////////////////////////

double PlayerClient::getCalculatedTileZ() const noexcept
{
	const auto level = getLevel();
	if (level == nullptr || !level->hasTerrain())
		return account.character.localPixelZ.value_or(0) / 16.0;

	const PixelPosition testPosition = account.character.getGlobalPosition().translate(24, 48);
	const auto terrainHeight = level->getHeightAt(testPosition);
	const auto currentZ = account.character.localPixelZ.value_or(0) / 16.0;
	return std::max(terrainHeight, currentZ);
}

///////////////////////////////////////////////////////////////////////////////

std::string PlayerClient::getLevelName() const
{
	const auto level = getLevel();
	if (level == nullptr)
		return account.level;

	return level->levelName;
}

std::shared_ptr<Level> PlayerClient::getLevel() const
{
	if (isHiddenClient())
		return nullptr;

	return m_currentLevel.lock();
}

std::shared_ptr<SubLevel> PlayerClient::getSubLevel() const
{
	if (const auto level = getLevel(); level != nullptr)
		return level->getSubLevelAtPosition(getMapPosition());

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

void PlayerClient::setPosition(const PixelPosition& position)
{
	auto localPosition = toLocalPixelPosition(position);
	if (const auto level = getLevel(); level != nullptr)
	{
		std::inplace_vector<props::SetResults, 4> propResults{
			setPropWith<PlayerProp::X2>(props::SetBy::SERVER, localPosition.x()),
			setPropWith<PlayerProp::Y2>(props::SetBy::SERVER, localPosition.y())
		};

		if (level->isGmap())
		{
			const auto destMapPosition = toMapPosition(position);
			propResults.push_back(setPropWith<PlayerProp::GMAPLEVELX>(props::SetBy::SERVER, destMapPosition.x()));
			propResults.push_back(setPropWith<PlayerProp::GMAPLEVELY>(props::SetBy::SERVER, destMapPosition.y()));
		}

		sendPropsFromResults(propResults);
	}
}

bool PlayerClient::warp(const std::string_view levelName, const PixelPosition& position, const std::optional<clock::time_point> clientCachedTime)
{
	// Find the level.
	const auto newLevel = m_server->getLoadedLevel(levelName, shared_from_this());

	// Check if the new level exists.
	if (newLevel == nullptr)
	{
		sendPacket(CString() >> (char)PLO_WARPFAILED << levelName);
		return false;
	}

	// If this is a gmap and the level names don't match, fix the position of the warp.
	if (newLevel->isGmap() && newLevel->levelName != levelName)
	{
		const auto subLevel = newLevel->getSubLevelByName(levelName);
		if (subLevel != nullptr)
		{
			const auto origin = Level::getSubLevelOrigin(subLevel).value_or(PixelPosition{});
			return warp(newLevel, position.translate(origin), clientCachedTime);
		}
	}

	return warp(newLevel, position, clientCachedTime);
}

bool PlayerClient::warp(const std::shared_ptr<Level>& level, const PixelPosition& position, const std::optional<clock::time_point> clientCachedTime)
{
	// If we are warping to the same level, just update the player's location.
	auto localPosition = toLocalPixelPosition(position);
	if (!m_currentLevel.expired() && account.level == level->levelName)
	{
		setPosition(position);
		return true;
	}

	// Set the player's position.
	account.character.localPixelX = localPosition.x();
	account.character.localPixelY = localPosition.y();

	// Tell the client their new level.
	if (level->isGmap())
	{
		// We have to do this manually since if we set it via setPropWith, it will cause a second level warp.
		auto mapPosition = toMapPosition(position);
		account.character.mapX = mapPosition.x();
		account.character.mapY = mapPosition.y();
		this->modTime[PROPID(PlayerProp::GMAPLEVELX)] = m_server->getFrameStartTime();
		this->modTime[PROPID(PlayerProp::GMAPLEVELY)] = m_server->getFrameStartTime();
		sendPacket(CString() >> (char)PLO_PLAYERWARP2 << getProp<PlayerProp::X>().serialize() << getProp<PlayerProp::Y>().serialize() << getProp<PlayerProp::Z>().serialize() >> (char)mapPosition.x() >> (char)mapPosition.y() << level->levelName);
	}
	else
	{
		// Reset the map position to 0 if we are warping to a non-gmap level.
		if (account.character.mapX != 0)
			this->modTime[PROPID(PlayerProp::GMAPLEVELX)] = m_server->getFrameStartTime();
		if (account.character.mapY != 0)
			this->modTime[PROPID(PlayerProp::GMAPLEVELY)] = m_server->getFrameStartTime();
		account.character.mapX = account.character.mapY = 0;

		sendPacket(CString() >> (char)PLO_PLAYERWARP << getProp<PlayerProp::X>().serialize() << getProp<PlayerProp::Y>().serialize() << level->levelName);
	}

	// Enter the level.
	return enterLevel(level, clientCachedTime);
}

bool PlayerClient::enterLevel(const std::shared_ptr<Level>& level, const std::optional<clock::time_point> clientCachedTime)
{
	const auto currentLevel = getLevel();
	const bool sameLevel = currentLevel == level;

	// Leave the current level if we are changing levels and add ourself to the new one.
	if (!sameLevel)
	{
		leaveLevel();

		m_currentLevel = level;
		level->addPlayer(m_id);
		account.level = level->levelName;
	}

	// Send the level now.
	const auto subLevel = level->getSubLevelAtPosition(getMapPosition());
	bool succeed = sendStaticLevelData(subLevel->staticData.lock(), subLevel, clientCachedTime);
	succeed = succeed && sendDynamicLevelData(level, clientCachedTime);

	// If we failed, leave the level and inform the client.
	if (!succeed)
	{
		leaveLevel();
		sendPacket(CString() >> (char)PLO_WARPFAILED << level->levelName);
		return false;
	}

	// If we are entering the level for the first time, send our Z prop to the client.
	// For some reason the client ignores the Z prop on login until they have fully entered a level.
	if (!m_loaded)
	{
		sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PlayerProp::Z << getProp<PlayerProp::Z>().serialize());
	}

	// If the level is a sparring zone and you have 100 AP, change AP to 99 and
	// the apcounter to 1.
	if (level->isSparringZone(getMapPosition()) && account.character.ap == 100)
	{
		account.apCounter = 1;
		sendPropsFromResults(setPropWith<PlayerProp::ALIGNMENT>(props::SetBy::SERVER, 99_ui8));
	}

	// Inform everybody as to the client's new location.  This will update the minimap.
	const CString minimap = CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id
		>> (char)PlayerProp::LEVEL << getProp<PlayerProp::LEVEL>().serialize()
		>> (char)PlayerProp::X << getProp<PlayerProp::X>().serialize()
		>> (char)PlayerProp::Y << getProp<PlayerProp::Y>().serialize();

	for (const auto& [pid, player] : players_of_type<PlayerClient>(m_server->getPlayerList()))
	{
		if (pid == this->getId())
			continue;

		player->sendPacket(minimap);
	}

	// Update RCs.
	const CString myRCProps = CString() >> (char)PLO_ADDPLAYER >> (short)getId() >> (char)account.name.length() << account.name
		>> (char)PlayerProp::LEVEL << getProp<PlayerProp::LEVEL>().serialize()
		>> (char)PlayerProp::PLAYERLISTSTATUS << getProp<PlayerProp::PLAYERLISTSTATUS>().serialize()
		>> (char)PlayerProp::NICKNAME << getProp<PlayerProp::NICKNAME>().serialize()
		>> (char)PlayerProp::COMMUNITYNAME << getProp<PlayerProp::COMMUNITYNAME>().serialize();
	m_server->sendPacketToType(PLTYPE_ANYCONTROL, myRCProps, this);

	return true;
}

bool PlayerClient::leaveLevel(const bool keepLevelReference)
{
	// Make sure we are on a level first.
	const auto levelp = m_currentLevel.lock();
	if (!levelp) return true;
	auto [subLevel, levelData] = levelp->getSubLevelAndStaticDataAtPosition(getMapPosition());
	if (levelData == nullptr)
		return false;

	// Leave the sub-level (cache the time).
	leaveSubLevel(subLevel);

	// Remove self from list of players in level.
	levelp->removePlayer(m_id);

	// Send PLO_ISLEADER to new level leader.
	if (const auto map = levelp->getMap(); map == nullptr || !map->isGmap())
	{
		if (const auto& levelPlayerList = levelp->getPlayers(); !levelPlayerList.empty())
		{
			if (const auto leader = m_server->getPlayer<PlayerClient>(levelPlayerList.front()); leader != nullptr)
				leader->informPlayerIsLevelLeader();
		}
	}

	// If I am carrying an NPC, tell others the NPC left the level.
	if (m_carryNPC != 0)
	{
		if (const auto npc = m_server->getNPC(m_carryNPC); npc)
		{
			levelp->removeNPC(m_carryNPC);
			const CString deletePacket = CString() >> (char)PLO_NPCDEL >> (int)m_carryNPC;
			m_server->sendPacketToLevelAndPastVisitorsAfter(levelData.get(), npc->lastUpdateTime, deletePacket);
		}
	}

	// Tell everyone I left.
	{
		m_server->sendPacketToNearby(CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id >> (char)PlayerProp::JOINLEAVELVL >> (char)0, getGlobalPosition(), getLevel(), {m_id});

		for (const auto& [pid, player] : players_of_type<PlayerClient>(m_server->getPlayerList()))
		{
			if (pid == getId()) continue;
			if (player->getLevel() != getLevel()) continue;
			sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)player->getId() >> (char)PlayerProp::JOINLEAVELVL >> (char)0);
		}
	}

	// Clear the level.
	if (!keepLevelReference)
		m_currentLevel.reset();

	return true;
}

bool PlayerClient::leaveSubLevel(const std::shared_ptr<SubLevel>& subLevel)
{
	if (subLevel == nullptr) return false;
	const auto staticData = subLevel->staticData.lock();
	if (staticData == nullptr) return false;

	auto curTime = m_server->getFrameStartTime();
	auto cacheLevel = [&curTime]<typename T>(std::vector<std::unique_ptr<CachedLevel<T>>>& cache, const std::shared_ptr<T>& level)
	{
		// Save the time we left the level for the client-side caching.
		bool found = false;
		for (auto& cl : cache)
		{
			if (const auto cllevel = cl->level.lock(); cllevel == level)
			{
				cl->lastEnteredTime = curTime;
				found = true;
				break;
			}
		}

		if (!found)
			cache.push_back(std::make_unique<CachedLevel<T>>(CachedLevel<T>{.level = level, .lastEnteredTime = curTime}));
	};

	// Static levels.
	cacheLevel(m_cachedStaticLevels, staticData);

	// Dynamic levels.
	std::string groupName = account.groupName;
	if (!groupName.empty())
	{
		if (const auto level = subLevel->parentLevel.lock(); level != nullptr && !level->isGroupMap)
			groupName.clear();
	}
	cacheLevel(m_cachedDynamicLevels[groupName], subLevel);

	return true;
}

bool PlayerClient::sendStaticLevelData(const std::shared_ptr<StaticLevelData>& staticLevelData, const std::shared_ptr<SubLevel>& subLevel, std::optional<clock::time_point> clientCachedTime)
{
	if (staticLevelData == nullptr)
		return false;

	const PlayerPtr self = shared_from_this();
	auto levelModTime = staticLevelData->modTime;
	const auto cachedModTime = getLevelLastEnteredTime(staticLevelData.get());
	if (!clientCachedTime.has_value()) clientCachedTime = levelModTime;

	bool sentBoard = false;

	// Tell the client that the following data is for the specified level.
	// Gmaps consist of multiple levels so this is the name of the sub-level.
	sendPacket(CString() >> (char)PLO_LEVELNAME << staticLevelData->levelName);

	// If we haven't loaded yet, send a blank board packet first.
	// This solves a client bug where it will animate the board tiles before checking if it is a type 1 tileset.
	// The blank board packet makes the client go through the tiledef loading routine before the actual board tiles are sent.
	if (!m_loaded)
		sendPacket(CString() >> (char)PLO_LEVELBOARD);

	// If we have not entered this level during this session, send board data.
	// Also send if the client sends a cache time that doesn't match the level.
	// Clients will cache level data so this can be skipped if nothing has changed.
	if (!cachedModTime.has_value() || clientCachedTime.value() < levelModTime)
	{
		// Send board data (tiles, layers, heights).
		if (subLevel != nullptr)
		{
			subLevel->sendBoardToPlayer(self);
			subLevel->sendBoardLayersToPlayer(self);
			subLevel->sendBoardHeightsToPlayer(self);
		}
		else
		{
			staticLevelData->sendBoardToPlayer(self);
			staticLevelData->sendBoardLayersToPlayer(self);
			// If we are here, we aren't on a gmap so we have no heights to send.
		}

		// Mark that we sent board data.
		// This is important because the client will get stuck on "Loading" until it gets board data,
		// so we need to know if we should send a blank board packet later.
		sentBoard = true;

		// Send links (if applicable).
		// We need to always send map links for bigmaps due to overflow issues that easily occur while waiting for the warp.
		// (The client may start to go beyond the edge of the level and cause an integer overflow in their position).
		if (!m_server->hasNPCServer() || m_server->cached.forceClientsideLinks.getValue() || (subLevel && subLevel->isOnBigMap))
			staticLevelData->sendLinksToPlayer(self, false);

		// Send signs (if applicable).
		if (!m_server->hasNPCServer() || m_server->cached.forceClientsideSigns.getValue())
			staticLevelData->sendSignsToPlayer(self);
	}

	// Send chests.
	// Always send chests since RC could have modified them.
	staticLevelData->sendChestsToPlayer(self);

	// The client will get stuck on "Loading" until it gets board data.
	// So, if we did not send any data, send an empty packet so the client knows it can move on.
	if (!sentBoard)
		sendPacket(CString() >> (char)PLO_LEVELBOARD);

	// Send the level mod time so the client can cache it.
	sendPacket(CString() >> (char)PLO_LEVELMODTIME >> (long long)clock::to_time_t(levelModTime));

	// Fix the level name.
	// If the player is on a gmap, we need to set the level back to the gmap.
	// If the player is the leader on their level, also send the isleader packet.
	sendPacket(CString() >> (char)PLO_LEVELNAME << getLevelName());
	checkAndInformIfLevelLeader();

	return true;
}

bool PlayerClient::sendDynamicLevelData(const std::shared_ptr<Level>& level, std::optional<clock::time_point> clientCachedTime)
{
	if (level == nullptr) return false;

	// Get the sub-level and static data we are on.
	auto [subLevel, staticLevelData] = level->getSubLevelAndStaticDataAtPosition(getMapPosition());
	if (subLevel == nullptr || staticLevelData == nullptr)
		return false;

	const PlayerPtr self = shared_from_this();
	const auto cachedModTime = getLevelLastEnteredTime(subLevel.get());

	// Send board changes, horses, and baddies.
	subLevel->sendBoardChangesToPlayer(self, cachedModTime);
	if (!level->isGmap())
	{
		level->sendHorsesToPlayer(self);
		level->sendBaddiesToPlayer(self);
	}

	// Tell the client if there are any ghost players in the level.
	// We don't support trial accounts so pass 0 (no ghosts) instead of 1 (ghosts present).
	sendPacket(CString() >> (char)PLO_GHOSTICON >> (char)0);

	// If we are the leader, send it now.
	checkAndInformIfLevelLeader();

	// Send NPCs.
	sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << level->levelName);
	level->sendNPCsToPlayer(self, cachedModTime);

	// Move the carry NPC to the new level.
	if (m_carryNPC != 0)
	{
		if (const auto npc = m_server->getNPC(m_carryNPC); npc)
		{
			if (npc->level != level->levelName)
			{
				level->addNPC(m_carryNPC);
				npc->character.mapX = account.character.mapX;
				npc->character.mapY = account.character.mapY;

				// setLevel should refresh all of the modTimes.
				npc->setLevel(level);
			}
			else if (level->isGmap())
			{
				npc->sendPropsFromResults(
					npc->setPropWith<NPCProp::GMAPLEVELX>(props::SetBy::SERVER, account.character.mapX),
					npc->setPropWith<NPCProp::GMAPLEVELY>(props::SetBy::SERVER, account.character.mapY)
				);
			}

			// Send the carry NPC props to other players.
			if (!level->isSinglePlayer)
			{
				const CString carryNPCProps = CString() >> (char)PLO_NPCPROPS >> (int)m_carryNPC << npc->getAllPropsPacket();
				m_server->sendPacketToNearby(carryNPCProps, getGlobalPosition(), level, {m_id});
			}
		}
	}

	// Send connecting player props to players in nearby levels.
	if (!level->isSinglePlayer)
	{
		const CString myProps = CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id >> (char)PlayerProp::JOINLEAVELVL >> (char)1 << getPropsPacketFromList(loginPropsClientOthers);
		for (const auto& playerId : level->findInRangePlayersForCommunication(getGlobalPosition()))
		{
			if (playerId == m_id) continue;
			if (auto other = m_server->getPlayer(playerId); other != nullptr)
			{
				if (!other->isClient()) continue;

				// Exchange props.
				other->sendPacket(myProps);
				this->sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)other->getId() >> (char)PlayerProp::JOINLEAVELVL >> (char)1 << other->getPropsPacketFromList(loginPropsClientOthers));
			}
		}
	}

	return true;
}

void PlayerClient::checkAndInformIfLevelLeader()
{
	if (m_currentLevel.expired())
		return;

	const auto level = getLevel();
	if (level == nullptr)
		return;

	if (level->isSinglePlayer || level->isPlayerLeader(m_id) || level->isGmap())
		sendPacket(CString() >> (char)PLO_ISLEADER);
}

void PlayerClient::informPlayerIsLevelLeader()
{
	sendPacket(CString() >> (char)PLO_ISLEADER);
}

///////////////////////////////////////////////////////////////////////////////

std::optional<clock::time_point> PlayerClient::getLevelLastEnteredTime(const StaticLevelData* level) const
{
	if (level == nullptr)
		return std::nullopt;

	for (auto& cl : m_cachedStaticLevels)
	{
		auto cllevel = cl->level.lock();
		if (cllevel && cllevel.get() == level)
		{
			if (cl->lastEnteredTime == clock::time_point::min())
				return std::nullopt;
			return cl->lastEnteredTime;
		}
	}

	return std::nullopt;
}

std::optional<clock::time_point> PlayerClient::getLevelLastEnteredTime(const SubLevel* level, std::string_view group) const
{
	if (level == nullptr)
		return std::nullopt;

	const auto groupLevels = m_cachedDynamicLevels.find(group);
	if (groupLevels == m_cachedDynamicLevels.end())
		return std::nullopt;

	for (auto& cl : groupLevels->second)
	{
		auto cllevel = cl->level.lock();
		if (cllevel && cllevel.get() == level)
		{
			if (cl->lastEnteredTime == clock::time_point::min())
				return std::nullopt;
			return cl->lastEnteredTime;
		}
	}

	return std::nullopt;
}

void PlayerClient::resetLevelCache(const StaticLevelData* level) const
{
	if (level == nullptr) return;
	for (auto& cl : m_cachedStaticLevels)
	{
		auto cllevel = cl->level.lock();
		if (cllevel && cllevel.get() == level)
		{
			cl->lastEnteredTime = clock::time_point::min();
			return;
		}
	}
}

void PlayerClient::resetLevelCache(const SubLevel* level, const std::string_view group)
{
	if (level == nullptr)
		return;

	const auto groupLevels = m_cachedDynamicLevels.find(group);
	if (groupLevels == m_cachedDynamicLevels.end())
		return;

	for (const auto& cl : groupLevels->second)
	{
		auto cllevel = cl->level.lock();
		if (cllevel && cllevel.get() == level)
		{
			cl->lastEnteredTime = clock::time_point::min();
			return;
		}
	}
}

void PlayerClient::resetLevelCache(const std::string_view group)
{
	const auto groupLevels = m_cachedDynamicLevels.find(group);
	if (groupLevels == m_cachedDynamicLevels.end())
		return;

	// Collect a list of all the NPCs in the group that need to be deleted.
	std::unordered_set<NPCID> npcsToReset;
	for (const auto& cl : groupLevels->second)
	{
		if (auto subLevel = cl->level.lock(); subLevel != nullptr)
		{
			if (auto level = subLevel->parentLevel.lock(); level != nullptr)
			{
				auto& npcs = level->getNPCs();
				npcsToReset.insert(npcs.begin(), npcs.end());
			}
		}
	}

	// Send the delete packet so the client forgets about them.
	for (const auto npcId : npcsToReset)
		sendPacket(CString() >> (char)PLO_NPCDEL >> (int)npcId);

	// Finally, clear the cache for the group.
	m_cachedDynamicLevels.erase(std::string{group});
}

///////////////////////////////////////////////////////////////////////////////

void PlayerClient::disableWeapons()
{
	this->account.status &= ~PLSTATUS_ALLOWWEAPONS;
	sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PlayerProp::STATUS << getProp<PlayerProp::STATUS>().serialize());
}

void PlayerClient::enableWeapons()
{
	this->account.status |= PLSTATUS_ALLOWWEAPONS;
	sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PlayerProp::STATUS << getProp<PlayerProp::STATUS>().serialize());
}

void PlayerClient::freezePlayer()
{
	sendPacket(CString() >> (char)PLO_FREEZEPLAYER2);
}

void PlayerClient::unfreezePlayer()
{
	sendPacket(CString() >> (char)PLO_UNFREEZEPLAYER);
}

void PlayerClient::sendRPGMessage(std::string message)
{
	string::replaceMutate(message, "\n", "#b");
	const auto translated = translate(message);
	sendPacket(CString() >> (char)PLO_RPGWINDOW << string::toCSV(string::splitByString(translated, "#b"sv, false)));
}

void PlayerClient::sendSignMessage(std::string message)
{
	string::replaceMutate(message, "\n", "#b");
	sendPacket(CString() >> (char)PLO_SAY2 << translate(message));
}

///////////////////////////////////////////////////////////////////////////////

void PlayerClient::testForTouch(SetResults& result, uint8_t movementDirection)
{
	if (!m_server->hasNPCServer())
		return;

	// Don't allow improper directions.
	movementDirection %= 4;

	// Test for signs.
	if (testForSigns(result, movementDirection))
		return;

	// Set for links.
	if (testForLinks(result, movementDirection))
		return;

	// Subtract an extra 1 pixel from the top touch test since the 2.31 client was rendering the location of the NPC weirdly.
	// When a pixel coordinate of 223 was sent (tile 13.9375), the client would render the NPC at y=14 and break the collision detection.
	// Oddly enough, it renders in the correct spot on a reconnect.  By allowing a single extra pixel on the touch test, this problem is resolved.
	static Position<int16_t> touchTest[] = {{24, 16 - 1}, {0, 32}, {24, 56}, {48, 32}};

	// Get the bounding box to test with.
	PixelRectangleArea testBox{getGlobalPosition().translate(touchTest[movementDirection].x(), touchTest[movementDirection].y()), {0, 0, 48}};
	if (m_server->cached.playerTouchesMeNoZ.getValue())
	{
		// If the server is set to ignore Z axis for touch, do so by providing a box of max length in the Z axis.
		testBox.position.z() = std::numeric_limits<int16_t>::min();
		testBox.size.length() = std::numeric_limits<uint16_t>::max();
	}

	if (const auto level = getLevel(); level != nullptr)
	{
		// Test for NPC touch.
		bool touchedNPC = false;
		for (const auto& npcId : level->findIntersectingNPCsForCollision(testBox))
		{
			if (auto npc = m_server->getNPC(npcId); npc != nullptr)
			{
				npc->scripting.events.addEvent(ScriptEventType::PLAYERTOUCHSME, source::FromPlayer(m_id));
				touchedNPC = true;
			}
		}
		if (touchedNPC)
		{
			const auto eventDistance = m_server->cached.eventDistance.getValue();
			for (const auto& npcId : level->findInRangeNPCsByDistance(testBox.position, eventDistance))
			{
				auto intersectingNPCs = level->findIntersectingNPCsForCollision(testBox);
				if (!std::ranges::contains(intersectingNPCs, npcId))
				{
					if (auto npc = m_server->getNPC(npcId); npc != nullptr)
						npc->scripting.events.addEvent(ScriptEventType::PLAYERTOUCHSOTHER, source::FromPlayer(m_id));
				}
			}
		}
	}
}

bool PlayerClient::testForSigns(SetResults& result, const uint8_t movementDirection)
{
	if (!m_server->hasNPCServer() || m_server->cached.forceClientsideSigns.getValue())
		return false;

	// Test for signs.
	if (account.character.direction == 0 && movementDirection == 0)
	{
		if (const auto level = getLevel(); level != nullptr)
		{
			for (const auto& sign : level->getSigns())
			{
				LocalPixelPosition signPos = toLocalPixelPosition(sign.getTileX(), sign.getTileY());
				if (account.character.localPixelY == signPos.y() && account.character.localPixelX >= signPos.x() - 24 && account.character.localPixelX <= signPos.x() + 8)
				{
					sendSignMessage(sign.text);
					return true;
				}
			}
		}
	}
	return false;
}

bool PlayerClient::testForLinks(SetResults& result, const uint8_t movementDirection)
{
	static Position<int16_t> touchTest[] = {{24, 16}, {0, 32}, {24, 56}, {48, 32}};

	if (!m_server->hasNPCServer() || m_server->cached.forceClientsideLinks.getValue())
		return false;

	// If we have no level, we can't test anything!
	const auto level = getLevel();
	if (level == nullptr)
		return false;

	// If this is a bigmap, we forced clientside links to fix issues where the client wraps the X/Y values and ends up in weird spots.
	// So don't check.
	const auto map = level->getMap();
	if (map && map->isBigMap())
		return false;

	// Test for links.
	const PixelPosition testPos = getGlobalPosition().translate(touchTest[movementDirection].x(), touchTest[movementDirection].y());
	const TilePosition testPosTiles = toTilePosition(testPos);
	if (const auto linkTouched = level->getLink(testPosTiles, map != nullptr); linkTouched.has_value())
	{
		const auto& destLevelName = linkTouched.value()->getDestinationLevel();

		// Check if the destination level is on the level's map.
		if (const auto destSubLevel = level->getSubLevelByName(destLevelName); destSubLevel != nullptr)
		{
			const auto pos = linkTouched.value()->getDestinationForCharacter(account.character, source::FromPlayer(m_id));
			const auto levelData = destSubLevel->staticData.lock();
			warp(level->levelName, level->convertToMapPosition(destSubLevel->mapPosition.value_or(MapPosition{0, 0}), pos), getLevelLastEnteredTime(levelData.get()));
			return true;
		}
		// Level is outside of the map, so search normally.
		else if (const auto newLevel = m_server->getLoadedLevel(destLevelName, shared_from_this()); newLevel != nullptr)
		{
			PixelPosition origin{};
			if (newLevel->isGmap())
			{
				if (const auto subLevel = newLevel->getSubLevelByName(destLevelName); subLevel != nullptr)
					origin = Level::getSubLevelOrigin(subLevel).value_or(PixelPosition{});
			}

			const auto pos = toPixelPosition(origin, linkTouched.value()->getDestinationForCharacter(account.character, source::FromPlayer(m_id)));
			const auto levelData = newLevel->getStaticLevelDataByName(destLevelName);
			warp(newLevel->levelName, pos, getLevelLastEnteredTime(levelData.get()));
			return true;
		}
	}

	return false;
}

void PlayerClient::dropItemsOnDeath()
{
	if (!m_server->getSettings().get<bool>("dropitemsdead").value_or(true))
		return;

	const auto level = getLevel();
	if (level == nullptr)
		return;

	const auto mindeathgralats = m_server->getSettings().get<uint32_t>("mindeathgralats").value_or(1);
	const auto maxdeathgralats = m_server->getSettings().get<uint32_t>("maxdeathgralats").value_or(50);
	const auto& allowedDeathDrops = m_server->getAllowedDeathDrops();

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> gralatDistribution(mindeathgralats, maxdeathgralats);
	std::uniform_int_distribution<uint32_t> itemDistribution(0, 3);
	std::uniform_int_distribution<int8_t> displacementDistribution(-4, 4);

	// Determine how many gralats to remove from the account.
	uint32_t drop_gralats = 0;
	if (maxdeathgralats > 0)
		drop_gralats = std::min(gralatDistribution(gen), account.character.gralats);

	// Determine how many arrows and bombs to remove from the account.
	auto drop_arrows = itemDistribution(gen);
	auto drop_bombs = itemDistribution(gen);
	if ((drop_arrows * 5) > account.character.arrows) drop_arrows = account.character.arrows / 5;
	if ((drop_bombs * 5) > account.character.bombs) drop_bombs = account.character.bombs / 5;

	// Check if we can drop arrows and bombs.
	if (!std::ranges::contains(allowedDeathDrops, LevelItemType::DARTS))
		drop_arrows = 0;
	if (!std::ranges::contains(allowedDeathDrops, LevelItemType::BOMBS))
		drop_bombs = 0;

	// Remove gralats/bombs/arrows.
	account.character.gralats -= drop_gralats;
	account.character.arrows -= (drop_arrows * 5);
	account.character.bombs -= (drop_bombs * 5);
	sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PlayerProp::GRALATS >> (int)account.character.gralats >> (char)PlayerProp::ARROWS >> (char)account.character.arrows >> (char)PlayerProp::BOMBS >> (char)account.character.bombs);

	// Check which gralats we can drop.
	const bool canDropGold = std::ranges::contains(allowedDeathDrops, LevelItemType::GOLDRUPEE);
	const bool canDropRed = std::ranges::contains(allowedDeathDrops, LevelItemType::REDRUPEE);
	const bool canDropBlue = std::ranges::contains(allowedDeathDrops, LevelItemType::BLUERUPEE);
	const bool canDropGreen = std::ranges::contains(allowedDeathDrops, LevelItemType::GREENRUPEE);
	if (!canDropGold && !canDropRed && !canDropBlue && !canDropGreen)
		drop_gralats = 0;

	// Add gralats to the level.
	TilePosition localTilePos = toTilePosition(getLocalPosition());
	while (drop_gralats != 0)
	{
		char item = 0;
		if (canDropGold && (drop_gralats % 100 != drop_gralats))
		{
			drop_gralats -= 100;
			item = 19;
		}
		else if (canDropRed && (drop_gralats % 30 != drop_gralats))
		{
			drop_gralats -= 30;
			item = 2;
		}
		else if (canDropBlue && (drop_gralats % 5 != drop_gralats))
		{
			drop_gralats -= 5;
			item = 1;
		}
		else if (drop_gralats != 0)
		{
			if (!canDropGreen)
				break;

			--drop_gralats;
			item = 0;
		}

		const float pX = localTilePos.x() + 1.5f + static_cast<float>(displacementDistribution(gen));
		const float pY = localTilePos.y() + 2.0f + static_cast<float>(displacementDistribution(gen));

		level->addItem(inform_client, toPixelPosition(getSubLevelOrigin(), pX, pY), static_cast<LevelItemType>(item));
	}

	// Add arrows and bombs to the level.
	for (unsigned int i = 0; i < drop_arrows; ++i)
	{
		const float pX = localTilePos.x() + 1.5f + static_cast<float>(displacementDistribution(gen));
		const float pY = localTilePos.y() + 2.0f + static_cast<float>(displacementDistribution(gen));

		level->addItem(inform_client, toPixelPosition(getSubLevelOrigin(), pX, pY), LevelItemType::DARTS);
	}
	for (unsigned int i = 0; i < drop_bombs; ++i)
	{
		const float pX = localTilePos.x() + 1.5f + static_cast<float>(displacementDistribution(gen));
		const float pY = localTilePos.y() + 2.0f + static_cast<float>(displacementDistribution(gen));

		level->addItem(inform_client, toPixelPosition(getSubLevelOrigin(), pX, pY), LevelItemType::BOMBS);
	}
}

bool PlayerClient::dropItem(const PixelPosition& position, const LevelItemType item)
{
	if (removeItem(item))
	{
		if (const auto level = getLevel(); level != nullptr && level->addItem(position, item))
			return true;
	}
	return false;
}

bool PlayerClient::removeItem(const LevelItemType itemType)
{
	switch (itemType)
	{
		case LevelItemType::GREENRUPEE: // greenrupee
		case LevelItemType::BLUERUPEE:  // bluerupee
		case LevelItemType::REDRUPEE:   // redrupee
		case LevelItemType::GOLDRUPEE:  // goldrupee
		{
			uint32_t gralatsRequired;
			if (itemType == LevelItemType::GOLDRUPEE)
				gralatsRequired = 100;
			else if (itemType == LevelItemType::REDRUPEE)
				gralatsRequired = 30;
			else if (itemType == LevelItemType::BLUERUPEE)
				gralatsRequired = 5;
			else
				gralatsRequired = 1;

			if (account.character.gralats >= gralatsRequired)
			{
				account.character.gralats -= gralatsRequired;
				return true;
			}

			return false;
		}

		case LevelItemType::BOMBS:
		{
			if (account.character.bombs >= 5)
			{
				account.character.bombs -= 5;
				return true;
			}
			return false;
		}

		case LevelItemType::DARTS:
		{
			if (account.character.arrows >= 5)
			{
				account.character.arrows -= 5;
				return true;
			}
			return false;
		}

		case LevelItemType::HEART:
		{
			if (account.character.hitpointsInHalves > 2)
			{
				account.character.hitpointsInHalves -= 2;
				return true;
			}
			return false;
		}

		case LevelItemType::FULLHEART:
		{
			if (account.maxHitpoints > 1)
			{
				--account.maxHitpoints;
				return true;
			}
			return false;
		}

		// NOTE: not receiving PLI_ITEMTAKE for >2.31, so we will not remove the item
		// same is true for sword/shield. assuming its true for the weapon-items, but
		// its currently not tested.
		case LevelItemType::GLOVE1:
		case LevelItemType::GLOVE2:
		{
			if (account.character.glovePower > 1)
			{
				account.character.glovePower--;
				return true;
			}
			return false;
		}

		case LevelItemType::BOW:
		case LevelItemType::FIREBALL:
		case LevelItemType::FIREBLAST:
		case LevelItemType::NUKESHOT:
		{
			constexpr std::array<std::pair<LevelItemType, uint8_t>, 4> items = {{
				{LevelItemType::BOW, 1},
				{LevelItemType::FIREBALL, 2},
				{LevelItemType::FIREBLAST, 3},
				{LevelItemType::NUKESHOT, 4}
			}};

			const auto& [item, power] = *std::ranges::find_if(items, [itemType](const auto& pair) { return pair.first == itemType; });
			if (account.character.bowPower >= power)
			{
				--account.character.bowPower;
				return true;
			}
			return false;
		}

		case LevelItemType::BOMB:
		case LevelItemType::SUPERBOMB:
		case LevelItemType::JOLTBOMB:
		{
			constexpr std::array<std::pair<LevelItemType, uint8_t>, 3> items = {{
				{LevelItemType::BOMB, 1},
				{LevelItemType::SUPERBOMB, 2},
				{LevelItemType::JOLTBOMB, 3}
			}};

			const auto& [item, power] = *std::ranges::find_if(items, [itemType](const auto& pair) { return pair.first == itemType; });
			if (account.character.bombPower >= power)
			{
				--account.character.bombPower;
				return true;
			}
			return false;
		}

		case LevelItemType::SHIELD:
		case LevelItemType::MIRRORSHIELD:
		case LevelItemType::LIZARDSHIELD:
		{
			constexpr std::array<std::pair<LevelItemType, uint8_t>, 3> items = {{
				{LevelItemType::SHIELD, 1},
				{LevelItemType::MIRRORSHIELD, 2},
				{LevelItemType::LIZARDSHIELD, 3}
			}};

			const auto& [item, power] = *std::ranges::find_if(items, [itemType](const auto& pair) { return pair.first == itemType; });
			if (account.character.shieldPower >= power)
			{
				--account.character.shieldPower;
				if (!account.character.shieldImage.empty() && account.character.shieldImage == std::format("shield_{}.{}", power, Limits::defaultImageExtension()))
					account.character.shieldImage.clear();
				return true;
			}
			return false;
		}

		case LevelItemType::SWORD:
		case LevelItemType::BATTLEAXE:
		case LevelItemType::LIZARDSWORD:
		case LevelItemType::GOLDENSWORD:
		{
			constexpr std::array<std::pair<LevelItemType, uint8_t>, 4> items = {{
				{LevelItemType::SWORD, 1},
				{LevelItemType::BATTLEAXE, 2},
				{LevelItemType::LIZARDSWORD, 3},
				{LevelItemType::GOLDENSWORD, 4}
			}};

			const auto& [item, power] = *std::ranges::find_if(items, [itemType](const auto& pair) { return pair.first == itemType; });
			if (account.character.swordPower >= power)
			{
				--account.character.swordPower;
				if (!account.character.swordImage.empty() && account.character.swordImage == std::format("sword_{}.{}", power, Limits::defaultImageExtension()))
					account.character.swordImage.clear();
				return true;
			}
			return false;
		}

		case LevelItemType::SPINATTACK:
		{
			if (account.status & PLSTATUS_HASSPIN)
			{
				account.status &= ~PLSTATUS_HASSPIN;
				return true;
			}
			return false;
		}

		default:;
	}

	return false;
}

props::SetResults PlayerClient::addItem(const LevelItemType itemType, const props::SetBy setBy)
{
	switch (itemType)
	{
		case LevelItemType::GREENRUPEE:
		case LevelItemType::BLUERUPEE:
		case LevelItemType::REDRUPEE:
		case LevelItemType::GOLDRUPEE:
			return setPropWith<PlayerProp::GRALATS>(setBy, account.character.gralats + LevelItem::GetRupeeCount(itemType));

		case LevelItemType::BOMBS:
			return setPropWith<PlayerProp::BOMBS>(setBy, std::min(99_ui8, static_cast<uint8_t>(account.character.bombs + 5)));

		case LevelItemType::DARTS:
			return setPropWith<PlayerProp::ARROWS>(setBy, std::min(99_ui8, static_cast<uint8_t>(account.character.arrows + 5)));

		case LevelItemType::HEART:
		{
			const uint8_t maxHearts = static_cast<uint8_t>(std::min(account.maxHitpoints, static_cast<uint8_t>(m_server->cached.maxHeartLimit.getValue())) * 2);
			return setPropWith<PlayerProp::BOMBS>(setBy, std::min(maxHearts, static_cast<uint8_t>(account.character.hitpointsInHalves + 2)));
		}

		case LevelItemType::FULLHEART:
		{
			const uint8_t maxHearts = static_cast<uint8_t>(std::min(account.maxHitpoints, static_cast<uint8_t>(m_server->cached.maxHeartLimit.getValue())) * 2);
			return setPropWith<PlayerProp::FULLHEARTS>(setBy, std::min(maxHearts, static_cast<uint8_t>(account.maxHitpoints + 1)));
		}

		case LevelItemType::GLOVE1:
		case LevelItemType::GLOVE2:
		{
			const auto power = itemType == LevelItemType::GLOVE1 ? 2_ui8 : 3_ui8;
			auto prop = getProp<PlayerProp::GLOVEPOWER>();
			if (prop.value < power)
				prop.value = Limits::apply(power, Limits::MaxPlayerGlovePower);
			return setProp<PlayerProp::GLOVEPOWER>(setBy, prop);
		}

		case LevelItemType::BOW:
		case LevelItemType::FIREBALL:
		case LevelItemType::FIREBLAST:
		case LevelItemType::NUKESHOT:
		{
			constexpr std::array<std::pair<LevelItemType, uint8_t>, 4> items = {{
				{LevelItemType::BOW, 1},
				{LevelItemType::FIREBALL, 2},
				{LevelItemType::FIREBLAST, 3},
				{LevelItemType::NUKESHOT, 4},
			}};
			if (m_server->Generation == ServerGeneration::CLASSIC)
			{
				const auto& [item, power] = *std::ranges::find_if(items, [itemType](const auto& pair) { return pair.first == itemType; });
				auto prop = getProp<PlayerProp::GANI>();
				if (prop.bowGif.has_value())
				{
					auto& currentPower = prop.bowGif.value().second;
					if (currentPower < power)
						currentPower = Limits::apply(power, Limits::MaxBowPower);
				}
				else prop.bowGif = std::make_pair(""s, power);
				return setPropWith<PlayerProp::GANI>(setBy, prop);
			}
		}

		case LevelItemType::BOMB:
		case LevelItemType::SUPERBOMB:
		case LevelItemType::JOLTBOMB:
		{
			constexpr std::array<std::pair<LevelItemType, uint8_t>, 3> items = {{
				{LevelItemType::BOMB, 1},
				{LevelItemType::SUPERBOMB, 2},
				{LevelItemType::JOLTBOMB, 3},
			}};
			const auto& [item, power] = *std::ranges::find_if(items, [itemType](const auto& pair) { return pair.first == itemType; });
			auto prop = getProp<PlayerProp::BOMBPOWER>();
			if (prop.value < power)
				prop.value = Limits::apply(power, Limits::MaxBombPower);
			return setPropWith<PlayerProp::BOMBPOWER>(setBy, prop);
		}

		case LevelItemType::SHIELD:
		case LevelItemType::MIRRORSHIELD:
		case LevelItemType::LIZARDSHIELD:
		{
			constexpr std::array<std::pair<LevelItemType, uint8_t>, 3> items = {{
				{LevelItemType::SHIELD, 1},
				{LevelItemType::MIRRORSHIELD, 2},
				{LevelItemType::LIZARDSHIELD, 3},
			}};
			const auto& [item, power] = *std::ranges::find_if(items, [itemType](const auto& pair) { return pair.first == itemType; });
			auto prop = getProp<PlayerProp::SHIELDIMAGE>();
			if (prop.power < power)
			{
				if (!prop.image.empty() && prop.image == std::format("shield_{}.{}", power, Limits::defaultImageExtension()))
					prop.image.clear();
				prop.power = Limits::applyShieldPower(power);
			}
			return setPropWith<PlayerProp::SHIELDIMAGE>(setBy, prop);
		}

		case LevelItemType::SWORD:
		case LevelItemType::BATTLEAXE:
		case LevelItemType::LIZARDSWORD:
		case LevelItemType::GOLDENSWORD:
		{
			constexpr std::array<std::pair<LevelItemType, int8_t>, 4> items = {{
				{LevelItemType::SWORD, 1},
				{LevelItemType::BATTLEAXE, 2},
				{LevelItemType::LIZARDSWORD, 3},
				{LevelItemType::GOLDENSWORD, 4},
			}};
			const auto& [item, power] = *std::ranges::find_if(items, [itemType](const auto& pair) { return pair.first == itemType; });
			auto prop = getProp<PlayerProp::SWORDIMAGE>();
			if (prop.power < power)
			{
				if (!prop.image.empty() && prop.image == std::format("sword_{}.{}", power, Limits::defaultImageExtension()))
					prop.image.clear();
				prop.power = Limits::applySwordPower(power);
			}
			return setPropWith<PlayerProp::SWORDIMAGE>(setBy, prop);
		}

		case LevelItemType::SPINATTACK:
			return setPropWith<PlayerProp::STATUS>(setBy, static_cast<uint8_t>(account.status | PLSTATUS_HASSPIN));

		default:;
	}

	return {};
}

void PlayerClient::addItem(inform_client_t, const LevelItemType itemType, const props::SetBy setBy)
{
	sendPropsFromResults(addItem(itemType, setBy));
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
