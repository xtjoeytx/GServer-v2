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
	{ T::ID } -> std::same_as<const uint8_t>;
	{ T::deserialize(std::declval<CString&&>()) } -> std::same_as<T>;
	{ t.serialize() } -> std::same_as<CString>;
};

template <class T>
struct PacketBase
{
	//static const uint8_t ID = T::ID;

	static T deserialize(CString& buffer) noexcept
	{
		return T::deserialize(std::forward(buffer));
	}
	
	static T deserialize(CString&& buffer) noexcept
	{
		return T::deserialize(std::forward(buffer));
	}

	[[nodiscard]]
	CString serialize(int clientVersion = CLVER_2_17) const noexcept
	{
		return static_cast<const T*>(this)->serialize(clientVersion);
	}
};

#endif // PACKETBASE_H
