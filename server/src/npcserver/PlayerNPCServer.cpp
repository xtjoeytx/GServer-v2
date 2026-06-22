#include <array>
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
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>

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
	static const std::array<std::string_view, 2> privateMessageHeaders{
		"#bPrivate message:#b"sv,
		"#bMass message:#b"sv,
	};

	if (auto player = m_server->getPlayer(from); player != nullptr)
	{
		if (!privateMessage.empty())
			player->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)m_id << player->translate(privateMessage));

		// Strip out the header if one is there.
		for (const auto& header : privateMessageHeaders)
		{
			if (message.starts_with(header))
			{
				message = message.substr(header.size());
				break;
			}
		}

		m_server->getNPCServer()->addEventToControlNPC(ScriptEventType::PRIVATEMESSAGE, source::FromPlayer(from), "pm"s, player->account.name, std::string{message});
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
