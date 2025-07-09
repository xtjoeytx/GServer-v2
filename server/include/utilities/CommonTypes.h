#ifndef COMMONTYPES_H
#define COMMONTYPES_H

#include <array>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <ranges>
#include <ratio>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <variant>

#include <utilities/StringUtils.h>

using namespace std::literals;

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

//----------------------------
// Aliases

template<class T>
using string_map = std::unordered_map<std::string, T, string::string_hash, string::string_hash_equal>;

template<class T>
using hash_map = std::unordered_map<size_t, T, string::string_hash, string::hash_string_equal>;

//----------------------------
// ID types

using PlayerID = uint16_t;
using NPCID = uint32_t;

//----------------------------
// ID constants

inline constexpr PlayerID NPCServerPlayerID = 2;

//----------------------------
// ID start constants

// Player IDs 0 and 1 break things, and 2 is reserved for the NPC server player.
inline constexpr PlayerID PLAYERID_GEN = 3;

// Player IDs 16000 and up is used for players on other servers and "IRC"-channels.
// The players from other servers should be unique lists for each player as they are fetched depending on
// what the player chooses to see (buddies, "global guilds" tab, "other servers" tab)
inline constexpr PlayerID PLAYERID_GEN_EXTERNAL = 16000;

// NPC IDs under 1000 can't be deleted, so require manual assignment.
inline constexpr NPCID NPCID_GEN_MANUAL = 3;
inline constexpr NPCID NPCID_GEN_LOCAL = 300;
inline constexpr NPCID NPCID_GEN_DATABASE = 10000;
inline constexpr NPCID NPCID_GEN_DATABASE_LOCALN = 100000;
inline constexpr uint8_t BADDYID_GEN = 1;

//----------------------------
// User-defined literals

inline constexpr uint8_t operator""_ui8(unsigned long long val)
{
	return static_cast<uint8_t>(val);
}

//----------------------------
// Property helpers

inline static constexpr uint8_t PROPID(auto prop)
{
	return static_cast<uint8_t>(prop);
}

template<typename T>
inline static constexpr std::optional<T> PROPOPT(T prop)
{
	return std::make_optional<T>(prop);
}

template<typename T>
inline static constexpr std::optional<T> PROPOPT(std::optional<T> prop)
{
	return prop;
}

//----------------------------
// Time helpers

namespace chrono = std::chrono;
using clock = std::chrono::system_clock;
using clock_duration_double = std::chrono::duration<double, std::chrono::system_clock::period>;
using duration_seconds_double = std::chrono::duration<double>;
using duration_milli_double = std::chrono::duration<double, std::milli>;
using duration_nano_double = std::chrono::duration<double, std::nano>;

inline clock::time_point currentTime()
{
	return clock::now();
}

inline clock::time_point convertFromTimeT(time_t time)
{
	return clock::from_time_t(time);
}

//----------------------------
// Concepts

template<class P>
concept Pair = requires(P p)
{
	typename P::first_type;
	typename P::second_type;
	{ p.first } -> std::same_as<typename P::first_type>;
	{ p.second } -> std::same_as<typename P::second_type>;
};

template<class Va, class Tp>
concept VariantContainsType = requires(Va v, Tp t)
{
	{ std::holds_alternative<Tp>(v) } -> std::same_as<bool>;
};

template<typename R, typename T>
concept RangeOf = std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, T>;

template<class... Ts>
concept AllSame = sizeof...(Ts) < 2 ||
	std::conjunction_v<
	std::is_same<std::tuple_element_t<0, std::tuple<Ts...>>, Ts>...
	>;

template<class O, class... Ts>
concept AllSameAs = sizeof...(Ts) < 2 ||
	(std::conjunction_v<std::is_same<std::tuple_element_t<0, std::tuple<Ts...>>, Ts>...>
		&& std::same_as<O, std::tuple_element_t<0, std::tuple<Ts...>>>);

//----------------------------
// Variant helpers

template<class... Ts>
struct visit_functions : Ts...
{
	using Ts::operator()...;
};

//----------------------------
// Range helpers

auto toRange(AllSame auto&&... range)
{
	return std::array{ std::forward<decltype(range)>(range)... };
}

//----------------------------
// Floating point helpers

inline static bool DoubleIsZero(double value)
{
	return std::abs(value) < std::numeric_limits<double>::epsilon();
}

inline static bool DoublesAreSame(double left, double right)
{
	return std::abs(left - right) < std::numeric_limits<double>::epsilon();
}

//----------------------------
// Dimensions and Positions

// TODO: Move to somewhere appropriate.
template<typename T>
struct Position
{
	Position() : data(T{}, T{}, T{}) {}
	Position(T x, T y) : data(x, y, T{}) {}
	Position(T x, T y, T z) : data(x, y, z) {}

	bool operator==(const Position<T>& other) const
	{
		return data == other.data;
	}
	bool operator!=(const Position<T>& other) const
	{
		return data != other.data;
	}

	T& operator[](size_t index)
	{
		if (index >= 3) throw std::out_of_range("Index out of range for Position");
		return data[index];
	}
	const T& operator[](size_t index) const
	{
		if (index >= 3) throw std::out_of_range("Index out of range for Position");
		return data[index];
	}

	T& x() { return data[0]; }
	T& y() { return data[1]; }
	T& z() { return data[2]; }
	const T& x() const { return data[0]; }
	const T& y() const { return data[1]; }
	const T& z() const { return data[2]; }

	Position<T>& translate(T dx, T dy)
	{
		data[0] += dx;
		data[1] += dy;
		return *this;
	}
	Position<T>& translate(T dx, T dy, T dz)
	{
		data[0] += dx;
		data[1] += dy;
		data[2] += dz;
		return *this;
	}

	Position<T> translate(T dx, T dy) const
	{
		Position<T> result = *this;
		result.translate(dx, dy);
		return result;
	}
	Position<T> translate(T dx, T dy, T dz) const
	{
		Position<T> result = *this;
		result.translate(dx, dy, dz);
		return result;
	}

	std::array<T, 3> data;
};

using PixelPosition = Position<int16_t>;
using TilePosition = Position<float>;
using WholeTilePosition = Position<int8_t>;

template<typename T>
struct Dimension
{
	Dimension() : data(T{}, T{}) {}
	Dimension(T width, T height) : data(width, height) {}

	bool operator==(const Dimension<T>& other) const
	{
		return data == other.data;
	}
	bool operator!=(const Dimension<T>& other) const
	{
		return data != other.data;
	}

	T& operator[](size_t index)
	{
		if (index >= 2) throw std::out_of_range("Index out of range for Dimension");
		return data[index];
	}
	const T& operator[](size_t index) const
	{
		if (index >= 2) throw std::out_of_range("Index out of range for Dimension");
		return data[index];
	}

	T& width() { return data[0]; }
	T& height() { return data[1]; }
	const T& width() const { return data[0]; }
	const T& height() const { return data[1]; }

	std::array<T, 2> data;
};

template<typename P, typename S>
struct Rectangle
{
	Rectangle() {}
	Rectangle(Position<P> position, Dimension<S> size) : position(position), size(size) {}
	Position<P> position{};
	Dimension<S> size{};
};

template<typename Pos, typename RectPos, typename RectDim>
inline constexpr bool positionInRectangle(const Position<Pos>& pos, const Rectangle<RectPos, RectDim>& rect)
{
	return pos.x() >= rect.position.x() && pos.x() <= (rect.position.x() + rect.size.width())
		&& pos.y() >= rect.position.y() && pos.y() <= (rect.position.y() + rect.size.height());
}

template<typename RectPosL, typename RectDimL, typename RectPosR, typename RectDimR>
inline constexpr bool rectanglesIntersect(const Rectangle<RectPosL, RectDimL>& left, const Rectangle<RectPosR, RectDimR>& right)
{
	if (left.position.x() + left.size.width() < right.position.x()
		|| right.position.x() + right.size.width() < left.position.x()
		|| left.position.y() + left.size.height() < right.position.y()
		|| right.position.y() + right.size.height() < left.position.y())
	{
		return false;
	}
	return true;
}

template<typename T>
inline Position<T> translatePosition(const Position<T>& position, T x, T y)
{
	return position.translate(x, y);
}

template<typename T>
inline Position<T> translatePosition(const Position<T>& position, T x, T y, T z)
{
	return position.translate(x, y, z);
}

inline PixelPosition toPixelPosition(float x, float y)
{
	// Enforce half tile increments.  We will never have a float position that isn't a half tile.
	int16_t halfTileX = static_cast<int16_t>(x * 2);
	int16_t halfTileY = static_cast<int16_t>(y * 2);
	return PixelPosition{ static_cast<int16_t>(halfTileX * 8), static_cast<int16_t>(halfTileY * 8) };
}

template<typename Type>
inline constexpr PixelPosition toPixelPosition(const Position<Type>& position)
{
	if constexpr (std::same_as<Type, int16_t>)
	{
		return position;
	}
	else if constexpr (std::same_as<Type, int8_t> || std::same_as<Type, float>)
	{
		return PixelPosition{ static_cast<int16_t>(position.x() * 16), static_cast<int16_t>(position.y() * 16) };
	}
	else
	{
		return PixelPosition{ static_cast<int16_t>(position.x()), static_cast<int16_t>(position.y()) };
	}
}

template<typename Type>
inline constexpr TilePosition toTilePosition(const Position<Type>& position)
{
	if constexpr (std::same_as<Type, int16_t>)
	{
		return TilePosition{ static_cast<float>(position.x()) / 16.0f, static_cast<float>(position.y()) / 16.0f };
	}
	else if constexpr (std::same_as<Type, float>)
	{
		return position;
	}
	else
	{
		return TilePosition{ static_cast<float>(position.x()), static_cast<float>(position.y()) };
	}
}

template<typename Type>
inline constexpr WholeTilePosition toWholeTilePosition(const Position<Type>& position)
{
	if constexpr (std::same_as<Type, int16_t>)
	{
		return WholeTilePosition{ static_cast<int8_t>(position.x()) / 16.0f, static_cast<int8_t>(position.y()) / 16.0f };
	}
	else if constexpr (std::same_as<Type, int8_t>)
	{
		return position;
	}
	else
	{
		return WholeTilePosition{ static_cast<int8_t>(position.x()), static_cast<int8_t>(position.y()) };
	}
}

//----------------------------
// Tags

struct inform_client_t { explicit inform_client_t() = default; };
inline constexpr inform_client_t inform_client{};

////////////////////////////////////////////////////////////////////////////////
}; // end namespace preagonal

#endif // COMMONTYPES_H
