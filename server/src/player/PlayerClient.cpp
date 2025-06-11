#include <cmath>
#include <IEnums.h>

#include "IConfig.h"

#include "Server.h"
#include "level/Level.h"
#include "level/LevelItem.h"
#include "level/Map.h"
#include "object/NPC.h"
#include "object/Player.h"
#include "object/Weapon.h"
#include "player/PlayerClient.h"
#include "player/PlayerProps.h"
#include "utilities/Log.h"
#include "utilities/StringUtils.h"

///////////////////////////////////////////////////////////////////////////////

CString _zlibFix(
	"//#CLIENTSIDE\xa7"
	"if(playerchats) {\xa7"
	"  this.chr = {ascii(#e(0,1,#c)),0,0,0,0};\xa7"
	"  for(this.c=0;this.c<strlen(#c)*(strlen(#c)>=11);this.c++) {\xa7"
	"    this.chr[2] = ascii(#e(this.c,1,#c));\xa7"
	"    this.chr[3] += 1*(this.chr[2]==this.chr[0]);\xa7"
	"    if(!(this.chr[2] in {this.chr[0],this.chr[1]})) {\xa7"
	"      if(this.chr[1]==0) {\xa7"
	"        if(this.chr[2]!=this.chr[0]) this.chr[1]=this.chr[2];\xa7"
	"      } else break; //[A][B][C]\xa7"
	"    }\xa7"
	"    this.chr[4] += 1*(this.chr[2]==this.chr[1]);\xa7"
	"    if(this.chr[1]>0 && this.chr[3] in |2,10|) break; //[1<A<11][B]\xa7"
	"    if(this.chr[3]>=11 && this.chr[4]>1) break; //[A>=11][B>1]\xa7"
	"  }\xa7"
	"  if(this.c>0 && this.c == strlen(#c)) setplayerprop #c,\xa0#c\xa0; //Pad\xa7"
	"}\xa7");

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

using PacketHandleFunc = HandlePacketResult(PlayerClient::*)(CString&);
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
	handlers[PLI_SHOWIMG] = &PlayerClient::msgPLI_SHOWIMG;
	handlers[PLI_HURTPLAYER] = &PlayerClient::msgPLI_HURTPLAYER;
	handlers[PLI_EXPLOSION] = &PlayerClient::msgPLI_EXPLOSION;
	handlers[PLI_NPCWEAPONDEL] = &PlayerClient::msgPLI_NPCWEAPONDEL;
	handlers[PLI_LEVELWARPMOD] = &PlayerClient::msgPLI_LEVELWARP;	// Shared with PLI_LEVELWARP
	handlers[PLI_ITEMTAKE] = &PlayerClient::msgPLI_ITEMDEL;			// Shared with PLI_ITEMDEL
	handlers[PLI_WEAPONADD] = &PlayerClient::msgPLI_WEAPONADD;
	handlers[PLI_UPDATEFILE] = &PlayerClient::msgPLI_UPDATEFILE;
	handlers[PLI_ADJACENTLEVEL] = &PlayerClient::msgPLI_ADJACENTLEVEL;
	handlers[PLI_HITOBJECTS] = &PlayerClient::msgPLI_HITOBJECTS;
	handlers[PLI_TRIGGERACTION] = &PlayerClient::msgPLI_TRIGGERACTION;
	handlers[PLI_MAPINFO] = &PlayerClient::msgPLI_MAPINFO;
	handlers[PLI_SHOOT] = &PlayerClient::msgPLI_SHOOT;
	handlers[PLI_SERVERWARP] = &PlayerClient::msgPLI_SERVERWARP;
	handlers[PLI_PROCESSLIST] = &PlayerClient::msgPLI_PROCESSLIST;
	handlers[PLI_UNKNOWN46] = &PlayerClient::msgPLI_UNKNOWN46;
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

HandlePacketResult PlayerClient::handlePacket(std::optional<uint8_t> id, CString& packet)
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

PlayerClient::PlayerClient(CSocket* pSocket, PlayerID pId)
	: Player(pSocket, pId)
{
	m_lastMovement = m_lastSave = m_last1m = time(0);
}

void PlayerClient::cleanup()
{
	if (m_id >= 0 && m_server != nullptr && m_loaded)
	{
		// Queue up the logout event.
		m_server->queueNPCEvent({}, ScriptEventType::PLAYERLOGOUT, source::FromPlayer(m_id));

		// Remove from the level.
		leaveLevel();

		// Adjust carried NPC location.
		if (m_carryNpcId != 0)
		{
			if (auto npc = m_server->getNPC(m_carryNpcId); npc)
			{
				auto curtime = time(0);
				npc->sendPropsFromResults(
					npc->setPropWith<NPCProp::X2>(SetBy::CLIENT, static_cast<int16_t>(account.character.pixelX + 8)),
					npc->setPropWith<NPCProp::Y2>(SetBy::CLIENT, static_cast<int16_t>(account.character.pixelY + 16))
				);
			}
			m_carryNpcId = 0;
		}
	}

	// Clean up.
	m_cachedLevels.clear();
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
			std::vector<CString> pack = m_grMovementPackets.tokenize("\n");
			for (auto& i : pack)
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

	time_t currTime = time(0);

	// Increase online time.
	++account.onlineSeconds;

	// Disconnect if no data has been received in 5 minutes.
	if ((int)difftime(currTime, m_lastData) > 300)
	{
		log::printLine(log::server, "Client {} has timed out.", account.name);
		return false;
	}

	// Disconnect if players are inactive.
	CSettings& settings = m_server->getSettings();
	if (settings.getBool("disconnectifnotmoved"))
	{
		int maxnomovement = settings.getInt("maxnomovement", 1200);
		if (((int)difftime(currTime, m_lastMovement) > maxnomovement) && ((int)difftime(currTime, m_lastChat) > maxnomovement))
		{
			log::printLine(log::server, "Client {} has been disconnected due to inactivity.", account.name);
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You have been disconnected due to inactivity.");
			return false;
		}
	}

	// Increase player AP.
	if (settings.getBool("apsystem") && !m_currentLevel.expired())
	{
		auto level = getLevel();
		if (level)
		{
			if (!(account.status & PLSTATUS_PAUSED) && !level->isSparringZone())
				--account.apCounter;

			if (account.apCounter <= 0)
			{
				if (account.character.ap < 100)
					sendPropsFromResults(setPropWith<PlayerProp::ALIGNMENT>(props::SetBy::SERVER, static_cast<uint8_t>(account.character.ap + 1)));

				if (account.character.ap < 20)
					account.apCounter = settings.getInt("aptime0", 30);
				else if (account.character.ap < 40)
					account.apCounter = settings.getInt("aptime1", 90);
				else if (account.character.ap < 60)
					account.apCounter = settings.getInt("aptime2", 300);
				else if (account.character.ap < 80)
					account.apCounter = settings.getInt("aptime3", 600);
				else
					account.apCounter = settings.getInt("aptime4", 1200);
			}
		}
	}

	// Do singleplayer level events.
	{
		for (auto& spLevel : m_singleplayerLevels)
		{
			auto& level = spLevel.second;
			if (level)
				level->doTimedEvents();
		}
	}

	// Save player account every 5 minutes.
	if ((int)difftime(currTime, m_lastSave) > 300)
	{
		m_lastSave = currTime;
		if (isClient() && m_loaded && !account.loadOnly)
			m_server->getAccountLoader().saveAccount(account);
	}

	// Events that happen every minute.
	if ((int)difftime(currTime, m_last1m) > 60)
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
	log::print(log::server, ":: New login:   ");
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

		// Unknown why we did this.
		if (m_versionId == CLVER_UNKNOWN)
		{
			Encryption.setGen(ENCRYPT_GEN_3);
			pPacket.setRead(1);
		}
	}
	// Handle newer clients.
	else
	{
		m_encryptionKey = (unsigned char)pPacket.readGChar();

		Encryption.reset(m_encryptionKey);
		if (Encryption.getGen() > ENCRYPT_GEN_3)
			m_fileQueue.setCodec(Encryption.getGen(), m_encryptionKey);

		// Read Client-Version
		m_version = pPacket.readChars(8);
		m_versionId = getVersionIDByVersion(m_version);
	}

	// Read Account & Password
	account.name = pPacket.readChars(pPacket.readGUChar()).toString();
	CString password = pPacket.readChars(pPacket.readGUChar());

	// Client Identity: win,"",02e2465a2bf38f8a115f6208e9938ac8,ff144a9abb9eaff4b606f0336d6d8bc5,"6.2 9200 "
	//					{platform}, {mobile provides 'dc:id2'}, {md5hash:harddisk-id}, {md5hash:network-id}, {uname(release, version)}, {android-id}
	CString identity = pPacket.readString("");

	//log::printLine(log::server, "   Key: {}", key);
	log::printLine(log::server, "   Version:     {} ({})", m_version, getVersionString(m_version, m_type));
	log::printLine(log::server, "   Account:     {}", account.name);
	if (!identity.isEmpty())
	{
		log::printLine(log::server, "   Identity:    {}", identity);
		auto identityTokens = identity.tokenize(",", true);
		m_os = identityTokens[0];
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
				int aVersion[2] = { getVersionID(ver1), getVersionID(ver2) };
				if (m_versionId >= aVersion[0] && m_versionId <= aVersion[1])
				{
					allowed = true;
					break;
				}
			}
			else
			{
				int aVersion = getVersionID(ver);
				if (m_versionId == aVersion)
				{
					allowed = true;
					break;
				}
			}
		}
		if (!allowed)
		{
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Your client version is not allowed on this server.\rAllowed: " << m_server->getAllowedVersionString());
			return false;
		}
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

bool PlayerClient::sendLogin()
{
	if (Player::sendLogin() == false)
		return false;

	auto& settings = m_server->getSettings();

	// Recalculate player spar deviation.
	{
		// c = sqrt( (350*350 - 50*50) / t )
		// where t is the number of rating periods for deviation to go from 50 to 350.
		// t = 60 days for us.
		const float c = 44.721f;
		auto current_time = std::chrono::system_clock::now();
		auto time_difference = current_time - account.lastSparTime;
		auto days = std::chrono::duration_cast<std::chrono::days>(time_difference).count();
		if (days != 0)
		{
			// Find the new deviation.
			float deviate = std::min(350.0f, static_cast<float>(sqrt((account.eloDeviation * account.eloDeviation) + (c * c) * days)));

			// Set the new rating.
			account.eloDeviation = deviate;
			account.lastSparTime = current_time;
		}
	}

	// Send the player his login props.
	sendPacket(CString() >> (char)PLO_PLAYERPROPS << getPropsPacketFromList(loginPropsClientSelf));

	// Workaround for the 2.31 client.  It doesn't request the map file when used with setmap.
	// So, just send them all the maps loaded into the server.
	if (m_versionId == CLVER_2_31 || m_versionId == CLVER_1_411)
	{
		for (const auto& map : m_server->getMapList())
		{
			if (map->getType() == MapType::BIGMAP)
				msgPLI_WANTFILE(CString() << map->getMapName());
		}
	}

	// Sent to rc and client, but rc ignores it so...
	sendPacket(CString() >> (char)PLO_CLEARWEAPONS);

	// If the gr.ip hack is enabled, add it to the player's flag list.
	if (settings.getBool("flaghack_ip", false) == true)
		this->setFlag("gr.ip", account.ipAddress, true);

	// Send the player's flags.
	for (const auto& [flag, value] : account.variables.store)
	{
		if (auto serialized = account.variables.serializeModern(flag); serialized.has_value())
			sendPacket(CString() >> (char)PLO_FLAGSET << serialized.value());
	}

	// Send the server's flags to the player.
	for (const auto& [flag, value] : m_server->Scripting.variables.store)
	{
		if (auto serialized = m_server->Scripting.variables.serializeModern(flag); serialized.has_value())
			sendPacket(CString() >> (char)PLO_FLAGSET << serialized.value());
	}

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
		sendPacket(weapon->getWeaponPacket(m_versionId));
	}

	// Send any protected weapons we do not have.
	auto protectedWeapons = m_server->getSettings().getStr("protectedweapons").gCommaStrTokens();
	std::erase_if(protectedWeapons, [this](CString& val)
				  {
					  return std::find(account.weapons.begin(), account.weapons.end(), val) != account.weapons.end();
				  });
	for (auto& weaponName : protectedWeapons)
		this->addWeapon(weaponName.toString());

	// Send the zlib fixing NPC to client versions 2.21 - 2.31.
	if (m_versionId >= CLVER_2_21 && m_versionId <= CLVER_2_31)
	{
		sendPacket(CString() >> (char)PLO_NPCWEAPONADD >> (char)12 << "-gr_zlib_fix" >> (char)0 >> (char)1 << "-" >> (char)1 >> (short)_zlibFix.length() << _zlibFix);
	}

	// Was blank.  Sent before weapon list.
	sendPacket(CString() >> (char)PLO_UNKNOWN190);

	// Send the level to the player.
	// warp will call sendCompress() for us.
	if (!warp(account.level, getX(), getY()) && m_currentLevel.expired())
	{
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "No level available.");
		log::printLine(log::server, "** Cannot find level for {}.", account.name);
		return false;
	}

	// Send the bigmap if it was set.
	if (m_versionId >= CLVER_2_1)
	{
		CString bigmap = settings.getStr("bigmap");
		if (!bigmap.isEmpty())
		{
			std::vector<CString> vbigmap = bigmap.tokenize(",");
			if (vbigmap.size() == 4)
				sendPacket(CString() >> (char)PLO_BIGMAP << vbigmap[0].trim() << "," << vbigmap[1].trim() << "," << vbigmap[2].trim() << "," << vbigmap[3].trim());
		}
	}

	// Send the minimap if it was set.
	if (m_versionId >= CLVER_2_1)
	{
		CString minimap = settings.getStr("minimap");
		if (!minimap.isEmpty())
		{
			std::vector<CString> vminimap = minimap.tokenize(",");
			if (vminimap.size() == 4)
				sendPacket(CString() >> (char)PLO_MINIMAP << vminimap[0].trim() << "," << vminimap[1].trim() << "," << vminimap[2].trim() << "," << vminimap[3].trim());
		}
	}

	// Send out RPG Window greeting.
	if (m_versionId >= CLVER_2_1)
		sendPacket(CString() >> (char)PLO_RPGWINDOW << "\"Welcome to " << settings.getStr("name") << ".\",\"" << CString(APP_VENDOR) << " " << CString(APP_NAME) << " programmed by " << CString(APP_CREDITS) << ".\"");

	// Send the start message to the player.
	sendPacket(CString() >> (char)PLO_STARTMESSAGE << m_server->getServerMessage());

	// This will allow serverwarp and some other things, for some reason.
	sendPacket(CString() >> (char)PLO_SERVERTEXT);

	// Send out what guilds should be placed in the Staff section of the playerlist.
	std::vector<CString> guilds = settings.getStr("staffguilds").tokenize(",");
	CString guildPacket = CString() >> (char)PLO_STAFFGUILDS;
	for (std::vector<CString>::iterator i = guilds.begin(); i != guilds.end(); ++i)
		guildPacket << "\"" << ((CString)(*i)).trim() << "\",";
	sendPacket(guildPacket.remove(guildPacket.length() - 1, 1));

	// Send out the server's available status list options.
	if (m_versionId >= CLVER_2_1)
	{
		// graal doesn't quote these
		CString pliconPacket = CString() >> (char)PLO_STATUSLIST;
		for (const auto& status : m_server->getStatusList())
			pliconPacket << status.trim() << ",";

		sendPacket(pliconPacket.remove(pliconPacket.length() - 1, 1));
	}

	// Exchange props with everybody on the server.
	exchangeMyPropsWithOthers();

	// Record prop mod time.
	auto curTime = currentTime();
	std::ranges::for_each(m_modTime, [&curTime](auto& modTime) { modTime = curTime; });

	// Ask for processes. This causes windows v6 clients to crash
	if (m_versionId < CLVER_6_015)
		sendPacket(CString() >> (char)PLO_LISTPROCESSES);

	m_fileQueue.sendCompress(true);

	// Queue up the login event.
	m_server->queueNPCEvent({}, ScriptEventType::PLAYERLOGIN, source::FromPlayer(m_id));

	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool PlayerClient::processChat(const CString& pChat)
{
	std::vector<CString> chatParse = pChat.tokenizeConsole();
	if (chatParse.size() == 0) return false;
	bool processed = false;
	bool setcolorsallowed = m_server->getSettings().getBool("setcolorsallowed", true);

	if (chatParse[0] == "setnick")
	{
		processed = true;
		if ((int)difftime(time(0), m_lastNick) >= 10)
		{
			m_lastNick = time(0);
			CString newName = pChat.subString(8).trim();

			// Word filter.
			int filter = m_server->getWordFilter().apply(this, newName, FILTER_CHECK_NICK);
			if (filter & FILTER_ACTION_WARN)
			{
				setChat(newName);
				return true;
			}

			sendPropsFromResults(setPropWith<PlayerProp::NICKNAME>(props::SetBy::SERVER, newName.toString()));
		}
		else
			setChat("Wait 10 seconds before changing your nick again!");
	}
	else if (chatParse[0] == "sethead" && chatParse.size() == 2)
	{
		if (!m_server->getSettings().getBool("setheadallowed", true)) return false;
		processed = true;

		// Get the appropriate filesystem.
		FileSystem* filesystem = m_server->getFileSystem();
		if (!m_server->getSettings().getBool("nofoldersconfig", false))
			filesystem = m_server->getFileSystem(FS_HEAD);

		// Try to find the file.
		CString file = filesystem->findi(chatParse[1]);
		if (file.length() == 0)
		{
			int i = 0;
			const char* ext[] = { ".png", ".mng", ".gif" };
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
			sendPropsFromResults(setPropWith<PlayerProp::HEADGIF>(props::SetBy::SERVER, chatParse[1].toString()));
		else
			m_server->getServerList().sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)m_id >> (char)0 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setbody" && chatParse.size() == 2)
	{
		if (m_server->getSettings().getBool("setbodyallowed", true) == false) return false;
		processed = true;

		// Check to see if it is a default body.
		bool isDefault = false;
		for (const auto& entry : DefaultBodies)
			if (chatParse[1].match(CString(entry.data())) == true) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			sendPropsFromResults(setPropWith<PlayerProp::BODYIMG>(props::SetBy::SERVER, chatParse[1].toString()));
			return false;
		}

		// Get the appropriate filesystem.
		FileSystem* filesystem = m_server->getFileSystem();
		if (!m_server->getSettings().getBool("nofoldersconfig", false))
			filesystem = m_server->getFileSystem(FS_BODY);

		// Try to find the file.
		CString file = filesystem->findi(chatParse[1]);
		if (file.length() == 0)
		{
			int i = 0;
			const char* ext[] = { ".png", ".mng", ".gif" };
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
			sendPropsFromResults(setPropWith<PlayerProp::BODYIMG>(props::SetBy::SERVER, chatParse[1].toString()));
		else
			m_server->getServerList().sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)m_id >> (char)1 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setsword" && chatParse.size() == 2)
	{
		if (!m_server->getSettings().getBool("setswordallowed", true)) return false;
		processed = true;

		// Check to see if it is a default sword.
		bool isDefault = false;
		for (const auto& entry : DefaultSwords)
			if (chatParse[1].match(CString(entry.data())) == true) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			sendPropsFromResults(setPropWith<PlayerProp::SWORDPOWER>(props::SetBy::SERVER, chatParse[1].toString()));
			return false;
		}

		// Get the appropriate filesystem.
		FileSystem* filesystem = m_server->getFileSystem();
		if (!m_server->getSettings().getBool("nofoldersconfig", false))
			filesystem = m_server->getFileSystem(FS_SWORD);

		// Try to find the file.
		CString file = filesystem->findi(chatParse[1]);
		if (file.length() == 0)
		{
			int i = 0;
			const char* ext[] = { ".png", ".mng", ".gif" };
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
			sendPropsFromResults(setPropWith<PlayerProp::SWORDPOWER>(props::SetBy::SERVER, chatParse[1].toString()));
		else
			m_server->getServerList().sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)m_id >> (char)2 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setshield" && chatParse.size() == 2)
	{
		if (!m_server->getSettings().getBool("setshieldallowed", true)) return false;
		processed = true;

		// Check to see if it is a default shield.
		bool isDefault = false;
		for (const auto& entry : DefaultShields)
			if (chatParse[1].match(CString(entry.data())) == true) isDefault = true;

		// Don't search for the file if it is one of the defaults.  This protects against
		// malicious gservers.
		if (isDefault)
		{
			sendPropsFromResults(setPropWith<PlayerProp::SHIELDPOWER>(props::SetBy::SERVER, chatParse[1].toString()));
			return false;
		}

		// Get the appropriate filesystem.
		FileSystem* filesystem = m_server->getFileSystem();
		if (!m_server->getSettings().getBool("nofoldersconfig", false))
			filesystem = m_server->getFileSystem(FS_SHIELD);

		// Try to find the file.
		CString file = filesystem->findi(chatParse[1]);
		if (file.length() == 0)
		{
			int i = 0;
			const char* ext[] = { ".png", ".mng", ".gif" };
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
			sendPropsFromResults(setPropWith<PlayerProp::SHIELDPOWER>(props::SetBy::SERVER, chatParse[1].toString()));
		else
			m_server->getServerList().sendPacket(CString() >> (char)SVO_GETFILE3 >> (short)m_id >> (char)3 >> (char)chatParse[1].length() << chatParse[1]);
	}
	else if (chatParse[0] == "setskin" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 0
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		signed char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			account.character.colors[0] = color;
			setPropsFromPacket(CString() >> (char)PlayerProp::COLORS >> (char)account.character.colors[0] >> (char)account.character.colors[1] >> (char)account.character.colors[2] >> (char)account.character.colors[3] >> (char)account.character.colors[4], props::SetBy::SERVER);
		}
	}
	else if (chatParse[0] == "setcoat" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 1
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		signed char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			account.character.colors[1] = color;
			setPropsFromPacket(CString() >> (char)PlayerProp::COLORS >> (char)account.character.colors[0] >> (char)account.character.colors[1] >> (char)account.character.colors[2] >> (char)account.character.colors[3] >> (char)account.character.colors[4], props::SetBy::SERVER);
		}
	}
	else if (chatParse[0] == "setsleeves" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 2
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		signed char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			account.character.colors[2] = color;
			setPropsFromPacket(CString() >> (char)PlayerProp::COLORS >> (char)account.character.colors[0] >> (char)account.character.colors[1] >> (char)account.character.colors[2] >> (char)account.character.colors[3] >> (char)account.character.colors[4], props::SetBy::SERVER);
		}
	}
	else if (chatParse[0] == "setshoes" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 3
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		signed char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			account.character.colors[3] = color;
			setPropsFromPacket(CString() >> (char)PlayerProp::COLORS >> (char)account.character.colors[0] >> (char)account.character.colors[1] >> (char)account.character.colors[2] >> (char)account.character.colors[3] >> (char)account.character.colors[4], props::SetBy::SERVER);
		}
	}
	else if (chatParse[0] == "setbelt" && chatParse.size() == 2 && setcolorsallowed)
	{
		processed = true;

		// id: 4
		if (chatParse[1].toLower() == "grey") chatParse[1] = "gray";
		signed char color = getColor(chatParse[1].toLower());
		if (color != -1)
		{
			account.character.colors[4] = color;
			setPropsFromPacket(CString() >> (char)PlayerProp::COLORS >> (char)account.character.colors[0] >> (char)account.character.colors[1] >> (char)account.character.colors[2] >> (char)account.character.colors[3] >> (char)account.character.colors[4], props::SetBy::SERVER);
		}
	}
	else if (chatParse[0] == "warpto")
	{
		processed = true;

		// To player
		if (chatParse.size() == 2)
		{
			// Permission check.
			if (!account.hasRight(PLPERM_WARPTOPLAYER) && !m_server->getSettings().getBool("warptoforall", false))
			{
				setChat("(not authorized to warp)");
				return true;
			}

			auto player = m_server->getPlayer<PlayerClient>(chatParse[1], PLTYPE_ANYCLIENT);
			if (player && player->getLevel())
				warp(player->getLevel()->getLevelName(), player->getX(), player->getY());
		}
		// To x/y location
		else if (chatParse.size() == 3)
		{
			// Permission check.
			if (!account.hasRight(PLPERM_WARPTO) && !m_server->getSettings().getBool("warptoforall", false))
			{
				setChat("(not authorized to warp)");
				return true;
			}

			setPropsFromPacket(CString() >> (char)PlayerProp::X >> (char)(strtofloat(chatParse[1]) * 2) >> (char)PlayerProp::Y >> (char)(strtofloat(chatParse[2]) * 2), props::SetBy::SERVER);
		}
		// To x/y level
		else if (chatParse.size() == 4)
		{
			// Permission check.
			if (!account.hasRight(PLPERM_WARPTO) && !m_server->getSettings().getBool("warptoforall", false))
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
		if (!account.hasRight(PLPERM_SUMMON))
		{
			setChat("(not authorized to summon)");
			return true;
		}

		auto p = m_server->getPlayer<PlayerClient>(chatParse[1], PLTYPE_ANYCLIENT);
		if (p) p->warp(account.level, getX(), getY());
	}
	else if (chatParse[0] == "unstick" || chatParse[0] == "unstuck")
	{
		if (chatParse.size() == 2 && chatParse[1] == "me")
		{
			processed = true;

			// Check if the player is in a jailed level.
			std::vector<CString> jailList = m_server->getSettings().getStr("jaillevels").tokenize(",");
			for (std::vector<CString>::iterator i = jailList.begin(); i != jailList.end(); ++i)
				if (i->trim() == account.level) return false;

			int unstickTime = m_server->getSettings().getInt("unstickmetime", 30);
			if ((int)difftime(time(0), m_lastMovement) >= unstickTime)
			{
				m_lastMovement = time(0);
				CString unstickLevel = m_server->getSettings().getStr("unstickmelevel", "onlinestartlocal.nw");
				float unstickX = m_server->getSettings().getFloat("unstickmex", 30.0f);
				float unstickY = m_server->getSettings().getFloat("unstickmey", 30.5f);
				warp(unstickLevel, unstickX, unstickY);
				setChat("Warped!");
			}
			else
				setChat(CString() << "Don't move for " << CString(unstickTime) << " seconds before doing '" << pChat << "'!");
		}
	}
	else if (pChat == "update level" && account.hasRight(PLPERM_UPDATELEVEL))
	{
		processed = true;
		if (auto level = getLevel(); level)
			level->reload();
	}
	else if (pChat == "showadmins")
	{
		processed = true;

		// Search through the player list for all RC's.
		CString msg;
		{
			auto& playerList = m_server->getPlayerList();
			for (auto& [pid, player] : playerList)
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
				auto& playerList = m_server->getPlayerList();
				for (auto& [pid, player] : playerList)
				{
					// If our guild matches, add it to our string.
					if (player->getGuild() == g)
						msg << (msg.length() == 0 ? "" : ", ") << CString(player->account.character.nickName).subString(0, player->account.character.nickName.find('(')).trimI();
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
		setChat(CString() << "kills: " << CString((int)account.kills));
	}
	else if (pChat == "showdeaths")
	{
		processed = true;
		setChat(CString() << "deaths: " << CString((int)account.deaths));
	}
	else if (pChat == "showonlinetime")
	{
		processed = true;
		int seconds = account.onlineSeconds % 60;
		int minutes = (account.onlineSeconds / 60) % 60;
		int hours = account.onlineSeconds / 3600;
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
			auto& playerList = m_server->getPlayerList();
			for (auto& [pid, player] : playerList)
			{
				// If our guild matches, send the PM.
				if (player->getGuild() == m_guild)
				{
					player->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)m_id << "\"\",\"Guild message:\",\"" << pm << "\"");
					++num;
				}
			}
		}

		// Tell the player how many guild members received his message.
		setChat(CString() << "(" << CString(num) << " guild member" << (num != 0 ? "s" : "") << " received your message)");
	}

	return processed;
}

///////////////////////////////////////////////////////////////////////////////

bool PlayerClient::warp(const CString& pLevelName, float pX, float pY, time_t modTime)
{
	CSettings& settings = m_server->getSettings();

	// Save our current level.
	auto currentLevel = m_currentLevel.lock();

	// Find the level.
	auto newLevel = Level::findLevel(pLevelName, m_server);

	// If we are warping to the same level, just update the player's location.
	if (currentLevel != nullptr && newLevel == currentLevel)
	{
		setPropsFromPacket(CString() >> (char)PlayerProp::X >> (char)(pX * 2) >> (char)PlayerProp::Y >> (char)(pY * 2), props::SetBy::SERVER);
		return true;
	}

	// Find the unstickme level.
	auto unstickLevel = Level::findLevel(settings.getStr("unstickmelevel", "onlinestartlocal.nw"), m_server);
	float unstickX = settings.getFloat("unstickmex", 30.0f);
	float unstickY = settings.getFloat("unstickmey", 35.0f);

	// See if the new level is on a gmap.
	m_pmap.reset();
	if (newLevel)
		m_pmap = newLevel->getMap();

	// Set x/y location.
	float oldX = getX(), oldY = getY();
	account.character.pixelX = pX * 16;
	account.character.pixelY = pY * 16;

	// Try warping to the new level.
	bool warpSuccess = setLevel(pLevelName, modTime);
	if (!warpSuccess)
	{
		// Failed, so try warping back to our old level.
		bool warped = true;
		if (currentLevel == nullptr) warped = false;
		else
		{
			account.character.pixelX = oldX * 16;
			account.character.pixelY = oldY * 16;
			m_pmap = currentLevel->getMap();
			warped = setLevel(currentLevel->getLevelName());
		}
		if (!warped)
		{
			// Failed, so try warping to the unstick level.  If that fails, we disconnect.
			if (unstickLevel == 0) return false;

			// Try to warp to the unstick me level.
			account.character.pixelX = unstickX * 16;
			account.character.pixelY = unstickY * 16;
			m_pmap = unstickLevel->getMap();
			if (!setLevel(unstickLevel->getLevelName()))
				return false;
		}
	}

	// TODO(Nalin): Should this happen on any warp?  Should it only trigger the player's weapons?
	// m_server->queueNPCEvent(m_currentLevel.lock(), ScriptEventType::WARPED, source::FromPlayer(m_id));

	return warpSuccess;
}

std::shared_ptr<Level> PlayerClient::getLevel() const
{
	if (isHiddenClient()) return {};

	auto pLevel = m_currentLevel.lock();
	if (pLevel) return pLevel;

	/*
	// TODO: This is causing problems with update level.  The old level is gone, so everybody will hit this during
	// the level send when we try to tell other players we left the level.
	if (isClient() && m_server->warpPlayerToSafePlace(m_id))
	{
		return m_currentLevel.lock();
	}
	*/

	return {};
}

bool PlayerClient::setLevel(const CString& pLevelName, time_t modTime)
{
	// Open Level
	auto newLevel = Level::findLevel(pLevelName, m_server);
	if (newLevel == nullptr)
	{
		sendPacket(CString() >> (char)PLO_WARPFAILED << pLevelName);
		return false;
	}

	leaveLevel();
	m_currentLevel = newLevel;

	// Check if the level is a singleplayer level.
	// If so, see if we have been there before.  If not, duplicate it.
	if (newLevel->isSingleplayer())
	{
		auto nl = (m_singleplayerLevels.find(newLevel->getLevelName()) != m_singleplayerLevels.end() ? m_singleplayerLevels[newLevel->getLevelName()] : nullptr);
		if (nl == nullptr)
		{
			newLevel = newLevel->clone();
			m_currentLevel = newLevel;
			m_singleplayerLevels[newLevel->getLevelName()] = newLevel;
		}
		else
			m_currentLevel = nl;
	}

	// Check if the map is a group map.
	if (auto map = m_pmap.lock(); map && map->isGroupMap())
	{
		if (!m_levelGroup.isEmpty())
		{
			// If any players are in this level, they might have been cached on the client.  Solve this by manually removing them.
			auto& plist = newLevel->getPlayers();
			for (auto id : plist)
			{
				auto p = m_server->getPlayer(id);
				sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)p->getId()
					>> (char)PlayerProp::CURLEVEL >> (char)(newLevel->getLevelName().length() + 1 + 7) << newLevel->getLevelName() << ".unknown"
					>> (char)PlayerProp::X << p->getProp<PlayerProp::X>().serialize()
					>> (char)PlayerProp::Y << p->getProp<PlayerProp::Y>().serialize());
			}

			// Set the correct level now.
			const auto& levelName = newLevel->getLevelName();
			auto& groupLevels = m_server->getGroupLevels();
			auto [start, end] = groupLevels.equal_range(levelName.toString());
			while (start != end)
			{
				if (auto nl = start->second.lock(); nl)
				{
					if (nl->getLevelName() == levelName)
					{
						m_currentLevel = nl;
						break;
					}
				}
				++start;
			}
			if (start == end)
			{
				newLevel = newLevel->clone();
				m_currentLevel = newLevel;
				newLevel->setLevelName(levelName);
				groupLevels.insert(std::make_pair(levelName.toString(), newLevel));
			}
		}
	}

	// Add myself to the level playerlist.
	newLevel->addPlayer(m_id);
	account.level = newLevel->getLevelName().toStringView();

	// Tell the client their new level.
	if (modTime == 0 || m_versionId < CLVER_2_1)
	{
		if (auto map = m_pmap.lock(); map && map->getType() == MapType::GMAP && m_versionId >= CLVER_2_1)
			sendPacket(CString() >> (char)PLO_PLAYERWARP2 << getProp<PlayerProp::X>().serialize() << getProp<PlayerProp::Y>().serialize() << getProp<PlayerProp::Z>().serialize() >> (char)newLevel->getMapX() >> (char)newLevel->getMapY() << map->getMapName());
		else
			sendPacket(CString() >> (char)PLO_PLAYERWARP << getProp<PlayerProp::X>().serialize() << getProp<PlayerProp::Y>().serialize() << account.level);
	}

	// Send the level now.
	bool succeed = true;
	if (m_versionId >= CLVER_2_1)
		succeed = sendLevel(newLevel, modTime, false);
	else
		succeed = sendLevel141(newLevel, modTime, false);

	if (!succeed)
	{
		leaveLevel();
		sendPacket(CString() >> (char)PLO_WARPFAILED << pLevelName);
		return false;
	}

	// If the level is a sparring zone and you have 100 AP, change AP to 99 and
	// the apcounter to 1.
	if (newLevel->isSparringZone() && account.character.ap == 100)
	{
		account.apCounter = 1;
		sendPropsFromResults(setPropWith<PlayerProp::ALIGNMENT>(props::SetBy::SERVER, 99_ui8));
	}

	// Inform everybody as to the client's new location.  This will update the minimap.
	CString minimap = CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id
		>> (char)PlayerProp::CURLEVEL << getProp<PlayerProp::CURLEVEL>().serialize()
		>> (char)PlayerProp::X << getProp<PlayerProp::X>().serialize()
		>> (char)PlayerProp::Y << getProp<PlayerProp::Y>().serialize();

	for (const auto& [pid, player] : players_of_type<PlayerClient>(m_server->getPlayerList()))
	{
		if (pid == this->getId())
			continue;
		if (auto map = m_pmap.lock(); map && map->isGroupMap() && m_levelGroup != player->getGroup())
			continue;

		player->sendPacket(minimap);
	}

	return true;
}

bool PlayerClient::sendLevel(std::shared_ptr<Level> pLevel, time_t modTime, bool fromAdjacent)
{
	if (pLevel == nullptr) return false;
	CSettings& settings = m_server->getSettings();

	// Send Level
	sendPacket(CString() >> (char)PLO_LEVELNAME << pLevel->getLevelName());
	time_t l_time = getCachedLevelModTime(pLevel.get());
	if (modTime == -1) modTime = pLevel->getModTime();
	if (l_time == 0)
	{
		if (modTime != pLevel->getModTime())
		{
			sendPacket(CString() >> (char)PLO_RAWDATA >> (int)((1 + (64 * 64 * 2) + 1)));
			sendPacket(CString() << pLevel->getBoardPacket());

			for (const auto& layers : pLevel->getLayers())
			{
				if (layers.first == 0) continue;
				CString layer = pLevel->getLayerPacket(layers.first);
				sendPacket(CString() >> (char)PLO_RAWDATA >> (int)layer.length());
				sendPacket(layer);
			}
		}

		// Send links, signs, and mod time.
		sendPacket(CString() << pLevel->getLinksPacket());
		sendPacket(CString() << pLevel->getSignsPacket(this));
		sendPacket(CString() >> (char)PLO_LEVELMODTIME >> (long long)pLevel->getModTime());
	}

	// Send board changes, chests, horses, and baddies.
	if (!fromAdjacent)
	{
		sendPacket(CString() << pLevel->getBoardChangesPacket(l_time));
		sendPacket(CString() << pLevel->getChestPacket(this));
		sendPacket(CString() << pLevel->getHorsePacket());
		sendPacket(CString() << pLevel->getBaddyPacket(m_versionId));
	}

	auto map = m_pmap.lock();

	// If we are on a gmap, change our level back to the gmap.
	if (map && map->getType() == MapType::GMAP)
		sendPacket(CString() >> (char)PLO_LEVELNAME << map->getMapName());

	// Tell the client if there are any ghost players in the level.
	// We don't support trial accounts so pass 0 (no ghosts) instead of 1 (ghosts present).
	sendPacket(CString() >> (char)PLO_GHOSTICON >> (char)0);

	if (!m_server->isNpcServerEnabled())
	{
		if (!fromAdjacent || !m_pmap.expired())
		{
			// If we are the leader, send it now.
			if (pLevel->isPlayerLeader(getId()) || pLevel->isSingleplayer() == true)
				sendPacket(CString() >> (char)PLO_ISLEADER);
		}
	}

	// Send new world time.
	sendPacket(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(m_server->getNWTime()));

	// Send NPCs.
	if (!fromAdjacent || !m_pmap.expired())
	{
		if (map != nullptr && map->getType() == MapType::GMAP)
			sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << map->getMapName());
		else
			sendPacket(CString() >> (char)PLO_SETACTIVELEVEL << pLevel->getLevelName());

		pLevel->sendNpcsToPlayer(shared_from_this(), convertFromTimeT(l_time));
	}

	// Move the carry NPC to the new level.
	if (m_carryNpcId != 0)
	{
		pLevel->addNPC(m_carryNpcId);
		if (auto npc = m_server->getNPC(m_carryNpcId); npc)
		{
			npc->level = pLevel;

			// Send the carry NPC props to other players.
			if (!pLevel->isSingleplayer())
			{
				CString carryNpcProps = CString() >> (char)PLO_NPCPROPS >> (int)m_carryNpcId << npc->getAllPropsPacket();
				m_server->sendPacketToLevelArea(carryNpcProps, self(), { m_id });
			}
		}
	}

	// Send connecting player props to players in nearby levels.
	if (!pLevel->isSingleplayer())
	{
		// Send my props.
		m_server->sendPacketToLevelArea(CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id << this->getPropsPacketFromList(loginPropsClientOthers), self(), {m_id});

		// Get other player props.
		if (auto map = m_pmap.lock(); map)
		{
			auto sgmap{ this->getMapPosition() };
			auto isGroupMap = map->isGroupMap();

			for (const auto& [otherid, other] : players_of_type<PlayerClient>(m_server->getPlayerList()))
			{
				if (m_id == otherid) continue;
				if (!other->isClient()) continue;

				auto othermap = other->getMap().lock();
				if (!othermap || othermap != map) continue;
				if (isGroupMap && this->getGroup() != other->getGroup()) continue;

				// Check if they are nearby before sending the packet.
				auto ogmap{ other->getMapPosition() };
				if (abs(ogmap.first - sgmap.first) < 2 && abs(ogmap.second - sgmap.second) < 2)
					this->sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)other->getId() << other->getPropsPacketFromList(loginPropsClientOthers));
			}
		}
		else
		{
			for (auto otherid : pLevel->getPlayers())
			{
				if (m_id == otherid) continue;
				auto other = m_server->getPlayer(otherid);
				this->sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)other->getId() << other->getPropsPacketFromList(loginPropsClientOthers));
			}
		}
	}

	return true;
}

bool PlayerClient::sendLevel141(std::shared_ptr<Level> pLevel, time_t modTime, bool fromAdjacent)
{
	if (pLevel == nullptr) return false;
	CSettings& settings = m_server->getSettings();

	time_t l_time = getCachedLevelModTime(pLevel.get());
	if (modTime == -1) modTime = pLevel->getModTime();
	if (l_time != 0)
	{
		sendPacket(CString() << pLevel->getBoardChangesPacket(l_time));
	}
	else
	{
		if (modTime != pLevel->getModTime())
		{
			sendPacket(CString() >> (char)PLO_RAWDATA >> (int)(1 + (64 * 64 * 2) + 1));
			sendPacket(CString() << pLevel->getBoardPacket());

			if (m_firstLevel)
				sendPacket(CString() >> (char)PLO_LEVELNAME << pLevel->getLevelName());
			m_firstLevel = false;

			// Send links, signs, and mod time.
			if (!settings.getBool("serverside", false)) // TODO: NPC server check instead.
			{
				sendPacket(CString() << pLevel->getLinksPacket());
				sendPacket(CString() << pLevel->getSignsPacket(this));
			}
			sendPacket(CString() >> (char)PLO_LEVELMODTIME >> (long long)pLevel->getModTime());
		}
		else
			sendPacket(CString() >> (char)PLO_LEVELBOARD);

		if (!fromAdjacent)
		{
			sendPacket(CString() << pLevel->getBoardChangesPacket2(l_time));
			sendPacket(CString() << pLevel->getChestPacket(this));
		}
	}

	// Send board changes, chests, horses, and baddies.
	if (!fromAdjacent)
	{
		sendPacket(CString() << pLevel->getHorsePacket());
		sendPacket(CString() << pLevel->getBaddyPacket(m_versionId));
	}

	if (fromAdjacent == false)
	{
		// If we are the leader, send it now.
		if (pLevel->isPlayerLeader(getId()) || pLevel->isSingleplayer() == true)
			sendPacket(CString() >> (char)PLO_ISLEADER);
	}

	// Send new world time.
	sendPacket(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(m_server->getNWTime()));

	// Send NPCs.
	if (!fromAdjacent)
		pLevel->sendNpcsToPlayer(shared_from_this(), convertFromTimeT(l_time));

	// Send connecting player props to players in nearby levels.
	if (!pLevel->isSingleplayer() && !fromAdjacent)
	{
		m_server->sendPacketToLevelArea(CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id << getPropsPacketFromList(loginPropsClientOthers), self(), {m_id});

		for (auto id : pLevel->getPlayers())
		{
			if (id == getId()) continue;

			auto player = m_server->getPlayer(id);
			this->sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)player->getId() << player->getPropsPacketFromList(loginPropsClientOthers));
		}
	}

	return true;
}

bool PlayerClient::leaveLevel(bool resetCache)
{
	// Make sure we are on a level first.
	auto levelp = m_currentLevel.lock();
	if (!levelp) return true;

	// Save the time we left the level for the client-side caching.
	bool found = false;
	for (auto& cl : m_cachedLevels)
	{
		auto cllevel = cl->level.lock();
		if (cllevel == levelp)
		{
			cl->modTime = (resetCache ? 0 : time(0));
			found = true;
			break;
		}
	}
	if (!found) m_cachedLevels.push_back(std::make_unique<CachedLevel>(m_currentLevel, time(0)));

	// Remove self from list of players in level.
	levelp->removePlayer(m_id);

	// Send PLO_ISLEADER to new level leader.
	if (auto& levelPlayerList = levelp->getPlayers(); !levelPlayerList.empty())
	{
		auto leader = m_server->getPlayer(levelPlayerList.front());
		leader->sendPacket(CString() >> (char)PLO_ISLEADER);
	}

	// If I am carrying an NPC, tell others the NPC left the level.
	if (m_carryNpcId != 0)
	{
		if (auto npc = m_server->getNPC(m_carryNpcId); npc)
		{
			levelp->removeNPC(m_carryNpcId);
			CString deletePacket = CString() >> (char)PLO_NPCDEL << (short)m_carryNpcId;
			m_server->sendPacketToOneLevel(deletePacket, levelp, { m_id });
		}
	}

	// Tell everyone I left.
	// This prop isn't used at all???  Maybe it is required for 1.41?
	//	if (m_pmap && m_pmap->getType() != MAPTYPE_GMAP)
	{
		m_server->sendPacketToLevelArea(CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id >> (char)PlayerProp::JOINLEAVELVL >> (char)0, self(), { m_id });

		for (const auto& [pid, player] : players_of_type<PlayerClient>(m_server->getPlayerList()))
		{
			if (pid == getId()) continue;
			if (player->getLevel() != getLevel()) continue;
			this->sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)player->getId() >> (char)PlayerProp::JOINLEAVELVL >> (char)0);
		}
	}

	// Set the level pointer to 0.
	m_currentLevel.reset();

	return true;
}

time_t PlayerClient::getCachedLevelModTime(const Level* level) const
{
	for (auto& cl : m_cachedLevels)
	{
		auto cllevel = cl->level.lock();
		if (cllevel && cllevel.get() == level)
			return cl->modTime;
	}
	return 0;
}

void PlayerClient::resetLevelCache(const Level* level)
{
	for (auto& cl : m_cachedLevels)
	{
		auto cllevel = cl->level.lock();
		if (cllevel && cllevel.get() == level)
		{
			cl->modTime = 0;
			return;
		}
	}
}

std::pair<int, int> PlayerClient::getMapPosition() const
{
	if (m_currentLevel.expired()) return { 0, 0 };
	if (m_pmap.expired()) return { 0, 0 };

	auto level = getLevel();
	auto map = m_pmap.lock();
	if (!level || !map) return { 0, 0 };

	switch (map->getType())
	{
	case MapType::BIGMAP:
		return { level->getMapX(), level->getMapY() };
	default:
	case MapType::GMAP:
		return { getProp<PlayerProp::GMAPLEVELX>().value, getProp<PlayerProp::GMAPLEVELY>().value };
	}

	return { 0, 0 };
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

void PlayerClient::sendRPGMessage(std::string_view message)
{
	sendPacket(CString() >> (char)PLO_RPGWINDOW << string::toCSV(message));
}

void PlayerClient::sendSignMessage(std::string message)
{
	
	sendPacket(CString() >> (char)PLO_SAY2 << string::replaceMutate(message, "\n", "#b"));
}

///////////////////////////////////////////////////////////////////////////////

bool PlayerClient::testSign()
{
	CSettings& settings = m_server->getSettings();
	if (!settings.getBool("serverside", false)) return true; // TODO: NPC server check instead

	// Check for sign collisions.
	if ((account.character.sprite % 4) == 0)
	{
		auto level = getLevel();
		if (level)
		{
			const auto& signs = level->getSigns();
			for (const auto& sign : signs)
			{
				float signLoc[] = { (float)sign->getX(), (float)sign->getY() };
				if (getY() == signLoc[1] && inrange(getX(), signLoc[0] - 1.5f, signLoc[0] + 0.5f))
				{
					sendPacket(CString() >> (char)PLO_SAY2 << sign->getUText().replaceAll("\n", "#b"));
				}
			}
		}
	}
	return true;
}

void PlayerClient::testTouch()
{
}

void PlayerClient::dropItemsOnDeath()
{
	if (!m_server->getSettings().getBool("dropitemsdead", true))
		return;

	int mindeathgralats = m_server->getSettings().getInt("mindeathgralats", 1);
	int maxdeathgralats = m_server->getSettings().getInt("maxdeathgralats", 50);

	// Determine how many gralats to remove from the account.
	int drop_gralats = 0;
	if (maxdeathgralats > 0)
	{
		drop_gralats = rand() % maxdeathgralats;
		clip(drop_gralats, mindeathgralats, maxdeathgralats);
		if (drop_gralats > account.character.gralats) drop_gralats = account.character.gralats;
	}

	// Determine how many arrows and bombs to remove from the account.
	int drop_arrows = rand() % 4;
	int drop_bombs = rand() % 4;
	if ((drop_arrows * 5) > account.character.arrows) drop_arrows = account.character.arrows / 5;
	if ((drop_bombs * 5) > account.character.bombs) drop_bombs = account.character.bombs / 5;

	// Remove gralats/bombs/arrows.
	account.character.gralats -= drop_gralats;
	account.character.arrows -= (drop_arrows * 5);
	account.character.bombs -= (drop_bombs * 5);
	sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PlayerProp::RUPEESCOUNT >> (int)account.character.gralats >> (char)PlayerProp::ARROWSCOUNT >> (char)account.character.arrows >> (char)PlayerProp::BOMBSCOUNT >> (char)account.character.bombs);

	// Add gralats to the level.
	while (drop_gralats != 0)
	{
		char item = 0;
		if (drop_gralats % 100 != drop_gralats)
		{
			drop_gralats -= 100;
			item = 19;
		}
		else if (drop_gralats % 30 != drop_gralats)
		{
			drop_gralats -= 30;
			item = 2;
		}
		else if (drop_gralats % 5 != drop_gralats)
		{
			drop_gralats -= 5;
			item = 1;
		}
		else if (drop_gralats != 0)
		{
			--drop_gralats;
			item = 0;
		}

		float pX = getX() + 1.5f + (rand() % 8) - 2.0f;
		float pY = getY() + 2.0f + (rand() % 8) - 2.0f;

		CString packet = CString() >> (char)PLI_ITEMADD >> (char)(pX * 2) >> (char)(pY * 2) >> (char)item;
		packet.readGChar(); // So msgPLI_ITEMADD works.

		msgPLI_ITEMADD(packet);
		sendPacket(CString() >> (char)PLO_ITEMADD << packet.subString(1));
	}

	// Add arrows and bombs to the level.
	for (int i = 0; i < drop_arrows; ++i)
	{
		float pX = getX() + 1.5f + (rand() % 8) - 2.0f;
		float pY = getY() + 2.0f + (rand() % 8) - 2.0f;

		CString packet = CString() >> (char)PLI_ITEMADD >> (char)(pX * 2) >> (char)(pY * 2) >> (char)4; // 4 = arrows
		packet.readGChar();                                                                             // So msgPLI_ITEMADD works.

		msgPLI_ITEMADD(packet);
		sendPacket(CString() >> (char)PLO_ITEMADD << packet.subString(1));
	}
	for (int i = 0; i < drop_bombs; ++i)
	{
		float pX = getX() + 1.5f + (rand() % 8) - 2.0f;
		float pY = getY() + 2.0f + (rand() % 8) - 2.0f;

		CString packet = CString() >> (char)PLI_ITEMADD >> (char)(pX * 2) >> (char)(pY * 2) >> (char)3; // 3 = bombs
		packet.readGChar();                                                                             // So msgPLI_ITEMADD works.

		msgPLI_ITEMADD(packet);
		sendPacket(CString() >> (char)PLO_ITEMADD << packet.subString(1));
	}
}

// TODO(Nalin): This should be on the Level.  The client version should be something like dropItem.
bool PlayerClient::spawnLevelItem(CString& pPacket, bool playerDrop)
{
	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };
	unsigned char item = pPacket.readGUChar();

	LevelItemType itemType = LevelItem::getItemId(item);
	if (itemType != LevelItemType::INVALID)
	{
		auto level = getLevel();
		if (level->addItem(loc[0], loc[1], itemType))
		{
			m_server->sendPacketToOneLevel(CString() >> (char)PLO_ITEMADD << (pPacket.text() + 1), level, { m_id });
		}
		else
		{
			sendPacket(CString() >> (char)PLO_ITEMDEL << (pPacket.text() + 1));
		}
	}

	return true;
}

bool PlayerClient::removeItem(LevelItemType itemType)
{
	switch (itemType)
	{
	case LevelItemType::GREENRUPEE: // greenrupee
	case LevelItemType::BLUERUPEE:  // bluerupee
	case LevelItemType::REDRUPEE:   // redrupee
	case LevelItemType::GOLDRUPEE:  // goldrupee
	{
		int gralatsRequired;
		if (itemType == LevelItemType::GOLDRUPEE) gralatsRequired = 100;
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

	/*
case LevelItemType::BOW:		// bow
case LevelItemType::BOMB:		// bomb
	return false;

case LevelItemType::SUPERBOMB:	// superbomb
case LevelItemType::FIREBALL:	// fireball
case LevelItemType::FIREBLAST:	// fireblast
case LevelItemType::NUKESHOT:	// nukeshot
case LevelItemType::JOLTBOMB:	// joltbomb
	return false;

case LevelItemType::SHIELD:			// shield
case LevelItemType::MIRRORSHIELD:	// mirrorshield
case LevelItemType::LIZARDSHIELD:	// lizardshield
	return false;

case LevelItemType::SWORD:			// sword
case LevelItemType::BATTLEAXE:		// battleaxe
case LevelItemType::LIZARDSWORD:	// lizardsword
case LevelItemType::GOLDENSWORD:	// goldensword
	return false;

case LevelItemType::FULLHEART:	// fullheart
	return false;
*/

	case LevelItemType::SPINATTACK:
	{
		if (account.status & PLSTATUS_HASSPIN)
		{
			account.status &= ~PLSTATUS_HASSPIN;
			return true;
		}
		return false;
	}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
