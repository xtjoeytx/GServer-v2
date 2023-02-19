
#ifndef GS2EMU_TPACKET_H
#define GS2EMU_TPACKET_H

#include <utility>

#include "IEnums.h"
#include "CString.h"


template <class T>
struct TPacket {
	T		Id;
	CString	Data;
};

typedef TPacket<ServerToPlayer> PlayerOutPacket;
typedef std::vector<PlayerOutPacket> PlayerOutPackets;
typedef TPacket<PlayerToServer> PlayerInPacket;
typedef std::vector<PlayerInPacket> PlayerInPackets;

#endif //GS2EMU_TPACKET_H
