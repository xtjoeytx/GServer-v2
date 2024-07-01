#ifndef PACKETBASE_H
#define PACKETBASE_H

#include <cstdint>
#include <vector>
#include <concepts>

#include "IUtil.h"
#include "CString.h"


template <typename T>
concept IsPacket = requires(T t)
{
	{ T::ID } -> std::same_as<uint8_t>;
	{ t.deserialize(std::declval<CString&&>()) } -> std::same_as<bool>;
	{ t.serialize() } -> std::same_as<CString>;
};

template <IsPacket T>
struct PacketBase
{
	static const uint8_t ID = T::ID;

	bool deserialize(CString&& buffer) noexcept
	{
		return static_cast<T*>(this)->deserialize(buffer);
	}

	[[nodiscard]]
	CString serialize(int clientVersion = CLVER_2_17) const noexcept
	{
		return static_cast<const T*>(this)->serialize(clientVersion);
	}
};

#endif // PACKETBASE_H
