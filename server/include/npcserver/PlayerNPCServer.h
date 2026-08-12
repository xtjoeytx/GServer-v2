#ifndef PLAYERNPCSERVER_H
#define PLAYERNPCSERVER_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <CSocket.h>

#include <CString.h>

#include <network/IPacketHandler.h>
#include <object/Player.h>
#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class PlayerNPCServer : public Player
{
public:
	PlayerNPCServer(CSocket* pSocket, PlayerID pId);
	~PlayerNPCServer() override = default;

public:
	bool onRecv() override;
	void onUnregister() override;

public:
	void sendPrivateMessage(PlayerID from, std::string_view message) override;

public:
	std::string privateMessage;

protected:
	HandlePacketResult handlePacket(std::optional<uint8_t> id, CString& packet) override;

public:
	//HandlePacketResult msgLoginPacket(CString& pPacket);
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // PLAYERNPCSERVER_H
