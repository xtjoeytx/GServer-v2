#include <math.h>
#include <vector>
#include <chrono>
#include <format>

#include <IUtil.h>

#include "IConfig.h"

#include "Server.h"
#include "object/NPC.h"
#include "object/Weapon.h"
#include "player/PlayerClient.h"
#include "player/PlayerLogin.h"
#include "player/PlayerNC.h"
#include "player/PlayerRC.h"
#include "player/PlayerProps.h"
#include "level/Level.h"
#include "level/Map.h"
#include "utilities/Log.h"
#include "utilities/StringUtils.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

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
	msgLoginPacket(packet);
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
		player = std::make_shared<PlayerNC>(m_playerSock, m_id);
	else if (m_type & PLTYPE_NPCSERVER)
		;
	else
	{
		log::printLine(log::server, ":: New login, but unknown player type: {}", m_type);
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
	{
		m_fileQueue.sendCompress(true);
		player->disconnect();
		return HandlePacketResult::Failed;
	}

	return HandlePacketResult::Handled;
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal
