#include <cstdint>
#include <optional>

#include <CSocket.h>
#include <CString.h>

#include <network/IPacketHandler.h>
#include <npcserver/PlayerNPCServer.h>
#include <object/Player.h>
#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
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

HandlePacketResult PlayerNPCServer::handlePacket(std::optional<uint8_t> id, CString& packet)
{
	return HandlePacketResult::Failed;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
