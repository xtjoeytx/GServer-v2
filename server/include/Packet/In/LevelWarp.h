#ifndef LEVELWARP_H
#define LEVELWARP_H

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "PacketBase.h"
#include "IEnums.h"


struct LevelWarp : public PacketBase<LevelWarp>
{
	//time_t modificationTime = 0;
	float x = 0.0f;
	float y = 0.0f;
	std::string level;

public:
	static const uint8_t ID = PLI_LEVELWARP;

	bool deserialize(CString&& buffer) noexcept
	{
		/*
		uint8_t packetType = buffer.readGUChar();
		if (packetType == PLI_LEVELWARPMOD)
			modificationTime = (time_t)buffer.readGUInt5();
		*/

		x = (float)(buffer.readGChar() / 2.0f);
		y = (float)(buffer.readGChar() / 2.0f);
		level = buffer.readString("");

		return true;
	}

	[[nodiscard]]
	CString serialize(int clientVersion = CLVER_2_17) const noexcept
	{
		return {};
	}
};

#endif // LEVELWARP_H
