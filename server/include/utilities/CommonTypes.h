#ifndef COMMONTYPES_H
#define COMMONTYPES_H

#include <array>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <ratio>
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

inline constexpr uint16_t operator""_ui16(unsigned long long val)
{
	return static_cast<uint16_t>(val);
}

inline constexpr int16_t operator""_i16(unsigned long long val)
{
	return static_cast<int16_t>(val);
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
using precise_clock = std::chrono::high_resolution_clock;
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

template<std::integral T>
inline static T DoubleAsIntegralFloor(double value)
{
	return static_cast<T>(value + std::numeric_limits<double>::epsilon());
}

//----------------------------
// Pointer helpers

template<class T>
inline auto toWeakPtr(std::shared_ptr<T>& ptr)
{
	return std::weak_ptr<T>(ptr);
}

//----------------------------
// Tags

struct inform_client_t { explicit inform_client_t() = default; };
inline constexpr inform_client_t inform_client{};

////////////////////////////////////////////////////////////////////////////////
}; // end namespace preagonal

#endif // COMMONTYPES_H
