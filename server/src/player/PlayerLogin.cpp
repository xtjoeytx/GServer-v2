#include <IDebug.h>

#include <math.h>
#include <vector>
#include <chrono>
#include <format>

#include <IUtil.h>

#include "IConfig.h"

#include "Server.h"
#include "object/NPC.h"
#include "object/Player.h"
#include "object/PlayerLogin.h"
#include "object/PlayerClient.h"
#include "object/PlayerRC.h"
#include "object/Weapon.h"
#include "player/PlayerProps.h"
#include "level/Level.h"
#include "level/Map.h"
#include "utilities/StringUtils.h"

using namespace graal::utilities;

#define serverlog m_server->getServerLog()
#define rclog m_server->getRCLog()
//extern bool __sendLogin[propscount];
//extern bool __getLogin[propscount];
//extern bool __getLoginNC[propscount];
//extern bool __getRCLogin[propscount];

///////////////////////////////////////////////////////////////////////////////

PlayerLogin::PlayerLogin(CSocket* pSocket, uint16_t pId)
	: Player(pSocket, pId)
{
}

PlayerLogin::~PlayerLogin()
{
}

///////////////////////////////////////////////////////////////////////////////

bool PlayerLogin::onRecv()
{
	Player::onRecv();
	return PacketCount == 0;
}

///////////////////////////////////////////////////////////////////////////////

HandlePacketResult PlayerLogin::handlePacket(std::optional<uint8_t> id, CString& packet)
{
	// TODO: Websocket stuff somewhere.
	// TODO: We should find a way to make sure our outgoing packets get sent before the disconnect.
	if (msgLoginPacket(packet) == HandlePacketResult::Failed)
		disconnect();
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerLogin::msgLoginPacket(CString& pPacket)
{
	// TODO(joey): Hijack type based on what graal sends, rather than use it directly.
	m_type = (1 << pPacket.readGChar());

	// Create our appropriate player.
	std::shared_ptr<Player> player = nullptr;
	if (m_type & PLTYPE_ANYCLIENT)
		player = std::make_shared<PlayerClient>(m_playerSock, m_id);
	else if (m_type & PLTYPE_ANYRC)
		player = std::make_shared<PlayerRC>(m_playerSock, m_id);
	else if (m_type & PLTYPE_ANYNC)
		;
	else if (m_type & PLTYPE_NPCSERVER)
		;
	else
	{
		serverlog.out(":: New login, but unknown player type: %d\n", m_type);
		return HandlePacketResult::Failed;
	}

	// Update the new player's current packet state to match ours.
	player->PacketCount = 1;
	player->setReceivedBuffer(m_recvBuffer);

	// Remove ourselves from the server.
	// We need to null our socket to avoid being passed data by the socket manager.
	auto self = shared_from_this();
	m_server->swapPlayer(self, player);
	m_playerSock = nullptr;

	// Pass the login to the new player.
	pPacket.setRead(0);
	if (player != nullptr && !player->handleLogin(pPacket))
		return HandlePacketResult::Failed;

	return HandlePacketResult::Handled;
}

///////////////////////////////////////////////////////////////////////////////

bool Player::sendLoginNC()
{
#if V8NPCSERVER
	// Send database npcs
	auto& npcList = m_server->getNPCNameList();
	for (auto& [npcName, npcPtr]: npcList)
	{
		auto npc = npcPtr.lock();
		if (npc == nullptr) continue;

		CString npcPacket = CString() >> (char)PLO_NC_NPCADD >> (int)npc->getId() >> (char)NPCPROP_NAME << npc->getProp(NPCPROP_NAME) >> (char)NPCPROP_TYPE << npc->getProp(NPCPROP_TYPE) >> (char)NPCPROP_CURLEVEL << npc->getProp(NPCPROP_CURLEVEL);
		sendPacket(npcPacket);
	}

	// Send classes
	CString classPacket;
	auto& classList = m_server->getClassList();
	for (auto it = classList.begin(); it != classList.end(); ++it)
		classPacket >> (char)PLO_NC_CLASSADD << it->first << "\n";
	sendPacket(classPacket);

	// Send list of currently connected NC's
	auto& playerList = m_server->getPlayerList();
	for (auto& [playerId, player]: playerList)
	{
		if (player.get() != this && player->isNC())
			sendPacket(CString() >> (char)PLO_RC_CHAT << "New NC: " << player->getAccountName());
	}

	// Announce to other nc's that we logged in
	m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << "New NC: " << m_accountName, this);

	m_loaded = true;
#endif
	return true;
}
