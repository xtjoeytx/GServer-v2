#ifndef PLAYERNC_H
#define PLAYERNC_H

#include "object/Player.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

class PlayerNC : public Player
{
public:
	PlayerNC(CSocket* pSocket, uint16_t pId) : Player(pSocket, pId) {}
	virtual ~PlayerNC() {}
	//virtual void cleanup() override;

public:
	virtual bool handleLogin(CString& pPacket) override;
	virtual bool sendLogin() override;

protected:
	virtual HandlePacketResult handlePacket(std::optional<uint8_t> id, CString& packet) override;

public:
	/*
	HandlePacketResult msgPLI_NC_NPCGET(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCDELETE(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCRESET(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCSCRIPTGET(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCWARP(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCFLAGSGET(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCSCRIPTSET(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCFLAGSSET(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCADD(CString& pPacket);
	HandlePacketResult msgPLI_NC_CLASSEDIT(CString& pPacket);
	HandlePacketResult msgPLI_NC_CLASSADD(CString& pPacket);
	HandlePacketResult msgPLI_NC_LOCALNPCSGET(CString& pPacket);
	HandlePacketResult msgPLI_NC_WEAPONLISTGET(CString& pPacket);
	HandlePacketResult msgPLI_NC_WEAPONGET(CString& pPacket);
	HandlePacketResult msgPLI_NC_WEAPONADD(CString& pPacket);
	HandlePacketResult msgPLI_NC_WEAPONDELETE(CString& pPacket);
	HandlePacketResult msgPLI_NC_CLASSDELETE(CString& pPacket);
	HandlePacketResult msgPLI_NC_LEVELLISTGET(CString& pPacket);
	*/
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // PLAYERNC_H
