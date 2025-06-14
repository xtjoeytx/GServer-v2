#ifndef PLAYERNPCSERVER_H
#define PLAYERNPCSERVER_H

#include <object/Player.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class PlayerNPCServer : public Player
{
public:
	PlayerNPCServer(CSocket* pSocket, PlayerID pId);
	virtual ~PlayerNPCServer() override;

public:
	virtual bool onRecv() override;
	virtual void onUnregister() override;

protected:
	virtual HandlePacketResult handlePacket(std::optional<uint8_t> id, CString& packet) override;

public:
	//HandlePacketResult msgLoginPacket(CString& pPacket);
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // PLAYERNPCSERVER_H
