
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

#endif //GS2EMU_TPACKET_H
