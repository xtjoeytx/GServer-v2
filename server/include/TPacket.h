
#ifndef GS2EMU_TPACKET_H
#define GS2EMU_TPACKET_H

#include "IEnums.h"
#include <utility>

template<class T>
struct TPacket
{
	T Id;
	CString Data;
};

/* Player packets */
typedef TPacket<ServerToPlayer> PlayerOutPacket;
typedef std::vector<PlayerOutPacket> PlayerOutPackets;
typedef TPacket<PlayerToServer> PlayerInPacket;
typedef std::vector<PlayerInPacket> PlayerInPackets;

/* ListServer packets */
typedef TPacket<ServerToListServer> ListServerOutPacket;
typedef std::vector<ListServerOutPacket> ListServerOutPackets;
typedef TPacket<ListServerToServer> ListServerInPacket;
typedef std::vector<ListServerInPacket> ListServerInPackets;

#endif //GS2EMU_TPACKET_H
