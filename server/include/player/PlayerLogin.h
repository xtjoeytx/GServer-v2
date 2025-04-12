#ifndef PLAYERLOGIN_H
#define PLAYERLOGIN_H

#include "object/Player.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

class PlayerLogin : public Player
{
public:
	PlayerLogin(CSocket* pSocket, uint16_t pId);
	virtual ~PlayerLogin() override;

public:
	virtual bool onRecv() override;
	virtual void onUnregister() override {}

protected:
	virtual HandlePacketResult handlePacket(std::optional<uint8_t> id, CString& packet) override;

public:
	HandlePacketResult msgLoginPacket(CString& pPacket);
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // PLAYERLOGIN_H
