#ifndef LEVELWARP_H
#define LEVELWARP_H

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "Packet/PacketBase.h"
#include "IEnums.h"


struct LevelWarp : public PacketBase<LevelWarp>
{
	//time_t modificationTime = 0;
	float x = 0.0f;
	float y = 0.0f;
	CString level;

public:
	static const uint8_t ID = PLI_LEVELWARP;

	static LevelWarp deserialize(CString& buffer) noexcept
	{
		LevelWarp packet;

		/*
		uint8_t packetType = buffer.readGUChar();
		if (packetType == PLI_LEVELWARPMOD)
			modificationTime = (time_t)buffer.readGUInt5();
		*/

		packet.x = (float)(buffer.readGChar() / 2.0f);
		packet.y = (float)(buffer.readGChar() / 2.0f);
		packet.level = buffer.readString("");

		return packet;
	}

	static LevelWarp deserialize(CString&& buffer) noexcept
	{
		return deserialize(buffer);
	}

	[[nodiscard]]
	CString serialize(int clientVersion = CLVER_2_17) const noexcept
	{
		return {};
	}
};

#endif // LEVELWARP_H
