#ifndef LEVELBOARD_H
#define LEVELBOARD_H

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "PacketBase.h"
#include "IEnums.h"

#include "Level/LevelBoardChange.h"


struct LevelWarp : public PacketBase<LevelWarp>
{
	LevelBoardChange& levelBoardChange;

public:
	static const uint8_t ID = PLO_LEVELBOARD;

	bool deserialize(CString&& buffer) noexcept
	{
		return false;
	}

	[[nodiscard]]
	CString serialize(int clientVersion = CLVER_2_17) const noexcept
	{
		CString packet;

		packet.writeGChar(ID);
		packet.write(levelBoardChange.getBoardStr());

		return packet;
	}
};

#endif // LEVELBOARD_H
