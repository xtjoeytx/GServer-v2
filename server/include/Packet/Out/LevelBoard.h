#ifndef LEVELBOARD_H
#define LEVELBOARD_H

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "Packet/PacketBase.h"
#include "IEnums.h"

#include "Level/LevelBoardChange.h"


struct LevelBoard : public PacketBase<LevelBoard>
{
	const LevelBoardChange* levelBoardChange = nullptr;

public:
	static const uint8_t ID = PLO_LEVELBOARD;

	static LevelBoard deserialize(CString& buffer) noexcept
	{
		return {};
	}

	static LevelBoard deserialize(CString&& buffer) noexcept
	{
		return {};
	}

	[[nodiscard]]
	CString serialize(int clientVersion = CLVER_2_17) const noexcept
	{
		CString packet;

		packet.writeGChar(ID);

		if (levelBoardChange != nullptr)
			packet.write(levelBoardChange->getBoardStr());

		return packet;
	}
};

#endif // LEVELBOARD_H
