#include <map>
#include <stdio.h>
#include <sys/stat.h>
#include <vector>
#include <array>
#include <type_traits>

#include <IEnums.h>

#include <IConfig.h>

#include <Server.h>
#include <level/Level.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <player/PlayerNC.h>
#include <network/IPacketHandler.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

using PacketHandleFunc = HandlePacketResult(PlayerNC::*)(CString&);
using PacketHandleArray = std::array<PacketHandleFunc, 256>;

static PacketHandleArray GeneratePacketHandlers()
{
	PacketHandleArray handlers{};
	handlers.fill(nullptr);

	/*
	handlers[PLI_NC_NPCGET] = &PlayerNC::msgPLI_NC_NPCGET;
	handlers[PLI_NC_NPCDELETE] = &PlayerNC::msgPLI_NC_NPCDELETE;
	handlers[PLI_NC_NPCRESET] = &PlayerNC::msgPLI_NC_NPCRESET;
	handlers[PLI_NC_NPCSCRIPTGET] = &PlayerNC::msgPLI_NC_NPCSCRIPTGET;
	handlers[PLI_NC_NPCWARP] = &PlayerNC::msgPLI_NC_NPCWARP;
	handlers[PLI_NC_NPCFLAGSGET] = &PlayerNC::msgPLI_NC_NPCFLAGSGET;
	handlers[PLI_NC_NPCSCRIPTSET] = &PlayerNC::msgPLI_NC_NPCSCRIPTSET;
	handlers[PLI_NC_NPCFLAGSSET] = &PlayerNC::msgPLI_NC_NPCFLAGSSET;
	handlers[PLI_NC_NPCADD] = &PlayerNC::msgPLI_NC_NPCADD;
	handlers[PLI_NC_CLASSEDIT] = &PlayerNC::msgPLI_NC_CLASSEDIT;
	handlers[PLI_NC_CLASSADD] = &PlayerNC::msgPLI_NC_CLASSADD;
	handlers[PLI_NC_LOCALNPCSGET] = &PlayerNC::msgPLI_NC_LOCALNPCSGET;
	handlers[PLI_NC_WEAPONLISTGET] = &PlayerNC::msgPLI_NC_WEAPONLISTGET;
	handlers[PLI_NC_WEAPONGET] = &PlayerNC::msgPLI_NC_WEAPONGET;
	handlers[PLI_NC_WEAPONADD] = &PlayerNC::msgPLI_NC_WEAPONADD;
	handlers[PLI_NC_WEAPONDELETE] = &PlayerNC::msgPLI_NC_WEAPONDELETE;
	handlers[PLI_NC_CLASSDELETE] = &PlayerNC::msgPLI_NC_CLASSDELETE;
	handlers[PLI_NC_LEVELLISTGET] = &PlayerNC::msgPLI_NC_LEVELLISTGET;
	*/

	return handlers;
}

///////////////////////////////////////////////////////////////////////////////

HandlePacketResult PlayerNC::handlePacket(std::optional<uint8_t> id, CString& packet)
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

bool PlayerNC::handleLogin(CString& pPacket)
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
		case PLTYPE_NC:
			log::printLine(log::server, "NC");
			Encryption.setGen(ENCRYPT_GEN_2);
			break;
		default:
			log::printLine(log::server, "Unknown ({})", m_type);
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

	//log::printLine(log::server, "   Key: {}", key);
	log::printLine(log::server, "   Version:     {} ({})", m_version, getVersionString(m_version, m_type));
	log::printLine(log::server, "   Account:     {}", account.name);
	if (!identity.isEmpty())
	{
		log::printLine(log::server, "   Identity:    {}", identity);
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

bool PlayerNC::sendLogin()
{
	if (Player::sendLogin() == false)
		return false;

	if (auto npcServer = m_server->getNPCServer(); npcServer != nullptr)
	{
		// Send database npcs
		auto& npcList = npcServer->getGlobalNPCList();
		for (auto& [npcName, npcPtr] : npcList)
		{
			auto npc = npcPtr.lock();
			if (npc == nullptr) continue;

			CString npcPacket = CString() >> (char)PLO_NC_NPCADD >> (int)npc->id
				>> (char)NPCProp::NAME << npc->getProp<NPCProp::NAME>().serialize()
				>> (char)NPCProp::TYPE << npc->getProp<NPCProp::TYPE>().serialize()
				>> (char)NPCProp::CURLEVEL << npc->getProp<NPCProp::CURLEVEL>().serialize();
			sendPacket(npcPacket);
		}

		// Send classes
		CString classPacket;
		auto& classList = npcServer->getClassList();
		for (auto it = classList.begin(); it != classList.end(); ++it)
			classPacket >> (char)PLO_NC_CLASSADD << it->first << "\n";
		sendPacket(classPacket);
	}

	// Send list of currently connected NC's
	auto& playerList = m_server->getPlayerList();
	for (auto& [playerId, player] : playerList)
	{
		if (player.get() != this && player->isNC())
			sendPacket(CString() >> (char)PLO_RC_CHAT << "New NC: " << player->account.name);
	}

	// Announce to other nc's that we logged in
	m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << "New NC: " << account.name, this);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
