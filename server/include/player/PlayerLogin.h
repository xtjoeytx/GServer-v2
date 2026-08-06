#ifndef PLAYERLOGIN_H
#define PLAYERLOGIN_H

#include <cstdint>
#include <optional>
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

class PlayerLogin : public Player
{
public:
	PlayerLogin(CSocket* pSocket, PlayerID pId);
	~PlayerLogin() override = default;

public:
	bool onRecv() override;
	void onUnregister() override {}

protected:
	std::string_view whoAmI() const noexcept override;
	HandlePacketResult handlePacket(std::optional<uint8_t> id, CString& packet) override;

public:
	HandlePacketResult msgLoginPacket(CString& pPacket);
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // PLAYERLOGIN_H
