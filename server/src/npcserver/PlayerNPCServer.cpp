#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <CSocket.h>

#include <CString.h>
#include <IEnums.h>

#include <Server.h>
#include <network/IPacketHandler.h>
#include <npcserver/NPCServer.h>
#include <npcserver/PlayerNPCServer.h>
#include <object/Player.h>
#include <utilities/CommonTypes.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

HandlePacketResult PlayerNPCServer::handlePacket(std::optional<uint8_t> id, CString& packet)
{
	return HandlePacketResult::Failed;
}

///////////////////////////////////////////////////////////////////////////////

PlayerNPCServer::PlayerNPCServer(CSocket* pSocket, PlayerID pId)
	: Player(pSocket, pId)
{
}

PlayerNPCServer::~PlayerNPCServer()
{
}

///////////////////////////////////////////////////////////////////////////////

bool PlayerNPCServer::onRecv()
{
	return false;
}

void PlayerNPCServer::onUnregister()
{
}

///////////////////////////////////////////////////////////////////////////////

void PlayerNPCServer::sendPrivateMessage(PlayerID from, std::string_view message)
{
	if (auto player = m_server->getPlayer(from); player != nullptr)
	{
		if (!privateMessage.empty())
			player->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)m_id << player->translate(privateMessage));

		m_server->getNPCServer()->addEventToControlNPC(ScriptEventType::PRIVATEMESSAGE, source::FromPlayer(from), "pm"s, player->account.name, std::string{ message });
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
