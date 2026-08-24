#ifndef COMMONTYPES_H
#define COMMONTYPES_H

#include <any>
#include <array>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <ratio>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <version>

#include <utilities/StringUtils.h>

using namespace std::literals;

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

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

// clang-format off
template<class... Ts>
concept AllSame = sizeof...(Ts) < 2 ||
	std::conjunction_v<
		std::is_same<std::tuple_element_t<0, std::tuple<Ts...>>, Ts>...
	>;

template<class O, class... Ts>
concept AllSameAs = sizeof...(Ts) < 2 ||
	(std::conjunction_v<std::is_same<std::tuple_element_t<0, std::tuple<Ts...>>, Ts>...>
		&& std::same_as<O, std::tuple_element_t<0, std::tuple<Ts...>>>);
// clang-format on

template<typename T>
concept IsEnum = std::is_enum_v<T>;

template<typename T>
concept ContainerLike = std::ranges::range<T> && std::ranges::sized_range<T>;

template<typename T>
concept ContainerLikeNotString = std::ranges::range<T> && std::ranges::sized_range<T> && !string::StringViewIshVariant<T> && !string::PointerToConstCharString<T>;

template<typename T>
concept MapContainer = requires(T t)
{
	typename T::key_type;
	typename T::mapped_type;
	typename T::value_type;
	{ t.find(std::declval<typename T::key_type>()) } -> std::same_as<typename T::iterator>;
	{ t.try_emplace(std::declval<typename T::key_type>(), std::declval<typename T::mapped_type>()) } -> std::same_as<std::pair<typename T::iterator, bool>>;
};

template<typename T>
concept EraseableContainer = requires(T t, typename T::iterator iter)
{
	typename T::value_type;
	typename T::iterator;
	t.erase(iter, iter);
};

template<typename T, typename C>
concept CheckFunction = requires(T t, typename C::value_type a)
{
	typename C::value_type;
	{ t(a) } -> std::convertible_to<bool>;
};

//----------------------------
// Aliases

template<class T>
using string_map = std::unordered_map<std::string, T, string::string_hash, string::string_hash_equal>;

template<class T>
using string_multimap = std::unordered_multimap<std::string, T, string::string_hash, string::string_hash_equal>;

template<class T>
using string_ordered_multimap = std::multimap<std::string, T, std::less<>>;

template<class T>
using hash_map = std::unordered_map<size_t, T, string::string_hash, string::hash_string_equal>;

using string_set = std::unordered_set<std::string, string::string_hash, string::string_hash_equal>;

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
inline constexpr NPCID NPCID_GEN_DELETEABLE = 1000;
inline constexpr NPCID NPCID_GEN_DATABASE = 10000;
inline constexpr NPCID NPCID_GEN_DATABASE_LOCALN = 100000;
inline constexpr uint8_t BADDYID_GEN = 1;

//----------------------------
// User-defined literals

inline constexpr uint8_t operator""_ui8(unsigned long long val)
{
	return static_cast<uint8_t>(val);
}

inline constexpr int8_t operator""_i8(unsigned long long val)
{
	return static_cast<int8_t>(val);
}

inline constexpr uint16_t operator""_ui16(unsigned long long val)
{
	return static_cast<uint16_t>(val);
}

inline constexpr int16_t operator""_i16(unsigned long long val)
{
	return static_cast<int16_t>(val);
}

inline constexpr uint32_t operator""_ui32(unsigned long long val)
{
	return static_cast<uint32_t>(val);
}

inline constexpr int32_t operator""_i32(unsigned long long val)
{
	return static_cast<int32_t>(val);
}

inline constexpr uint64_t operator""_ui64(unsigned long long val)
{
	return static_cast<uint64_t>(val);
}

inline constexpr int64_t operator""_i64(unsigned long long val)
{
	return static_cast<int64_t>(val);
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

inline static constexpr auto ENUM(IsEnum auto e)
{
	return static_cast<std::underlying_type_t<decltype(e)>>(e);
}

template<IsEnum E>
inline static constexpr E ENUM(std::underlying_type_t<E> value)
{
	return static_cast<E>(value);
}

//----------------------------
// Time helpers

namespace chrono = std::chrono;
using clock = std::chrono::system_clock;
using precise_clock = std::chrono::steady_clock;
using clock_duration_double = std::chrono::duration<double, std::chrono::system_clock::period>;
using duration_seconds_double = std::chrono::duration<double>;
using duration_milli_double = std::chrono::duration<double, std::milli>;
using duration_nano_double = std::chrono::duration<double, std::nano>;

/// @brief Gets the current time as a std::chrono::system_clock::time_point.
/// @return The current time.
inline clock::time_point currentTime()
{
	return clock::now();
}

/// @brief Converts a time_t value to a std::chrono::system_clock::time_point.
/// @param time The time_t value to convert.
/// @return A time_point representing the same point in time as the time_t value.
inline clock::time_point convertFromTimeT(const time_t time)
{
	return clock::from_time_t(time);
}

/// @brief Calculates the absolute time difference between two time points.
/// @tparam T The duration type for the result. Defaults to std::chrono::seconds.
/// @param time1 The first time point.
/// @param time2 The second time point.
/// @return The absolute duration between the two time points, or the maximum duration value if either time point is uninitialized (minimum value).
template<typename T = std::chrono::seconds, typename C = clock>
inline T timeDifference(const typename C::time_point& time1, const typename C::time_point& time2)
{
	if (time1 == C::time_point::min() || time2 == C::time_point::min())
		return T::max();
	return std::chrono::duration_cast<T>(time2 >= time1 ? time2 - time1 : time1 - time2);
}

/// @brief Checks if the specified future time has passed relative to the current time.
/// @tparam C The clock type. Defaults to std::chrono::system_clock.
/// @param currentTime The current time point.
/// @param futureTime The future time point to check.
/// @return True if the future time has passed, false otherwise.
template<typename C = clock>
inline bool timePassed(const typename C::time_point& currentTime, const typename C::time_point& futureTime)
{
	if (currentTime == C::time_point::min() || futureTime == C::time_point::min())
		return false;
	return futureTime <= currentTime;
}

/// @brief Converts a std::filesystem::file_time_type to a std::chrono::system_clock::time_point.
/// @param fileTime The file time to convert.
/// @return A time_point representing the same point in time as the file time, but in the system clock's time domain.
inline clock::time_point toSystemClock(const std::filesystem::file_time_type& fileTime)
{
#if __cpp_lib_chrono < 201907L
	// Clang doesn't support clock_cast, so convert to UTC, then the system clock.
	return std::chrono::file_clock::to_sys(fileTime));
#else
	return std::chrono::clock_cast<clock>(fileTime);
#endif
}

/// @brief Converts a std::chrono::system_clock::time_point to a std::filesystem::file_time_type.
/// @param systemTime The system clock time to convert.
/// @return A file_time_type representing the same point in time as the system clock time, but in the file clock's time domain.
inline std::filesystem::file_time_type toFileClock(const clock::time_point& systemTime)
{
#if __cpp_lib_chrono < 201907L
	// Clang doesn't support clock_cast, so convert to UTC, then the system clock.
	return std::chrono::file_clock::from_sys(systemTime);
#else
	return std::chrono::clock_cast<std::filesystem::file_time_type::clock>(systemTime);
#endif
}

//----------------------------
// Variant helpers

template<class... Ts>
struct visit_functions : Ts...
{
	using Ts::operator()...;
};

//----------------------------
// Container helpers

namespace util
{

template<EraseableContainer T>
inline void truncateContainerWhen(T& container, CheckFunction<T> auto&& func)
{
	container.erase(std::ranges::find_if(container, func), std::end(container));
};

inline std::string constructScriptName(const std::string& scriptIdentifier, const std::string& name)
{
	constexpr size_t maxIdentifierLength = 24;
	auto tablength = static_cast<size_t>(std::round(static_cast<float>(maxIdentifierLength - scriptIdentifier.length()) / 4.0f));

	// Fudge the numbers a little bit to make it look nicer in RC2.
	if (scriptIdentifier.length() < 8)
		++tablength;
	if (scriptIdentifier.length() > 16)
		++tablength;

	return std::format("{}{}{}", scriptIdentifier, std::string(tablength, '\t'), name);
}

} // end namespace util

//----------------------------
// Range helpers

inline static auto toRange(AllSame auto&&... range)
{
	return std::array{std::forward<decltype(range)>(range)...};
}

template<AllSame... Ts>
inline static auto toNonOwningRange(Ts&&... range)
{
	using CommonType = std::common_type_t<Ts...>;
	CommonType arr[] = {std::forward<Ts>(range)...};
	return std::views::counted(arr, sizeof...(Ts));
}

// clang-format off
inline auto removeNulls = std::views::filter([](auto&& ptr) { return ptr != nullptr; });
inline auto toSharedPtr = std::views::transform([](auto&& ptr) { return ptr.lock(); });
inline auto toAny = std::views::transform([](const auto& value) { return std::any{value}; });
// clang-format on

//----------------------------
// Floating point helpers

inline static bool DoubleIsZero(const double value)
{
	return std::abs(value) < std::numeric_limits<double>::epsilon();
}

inline static bool DoublesAreSame(const double left, const double right)
{
	// Graal uses 0.0001 as the threshold for comparing doubles.
	return std::abs(left - right) < 0.0001;
	//return std::abs(left - right) < std::numeric_limits<double>::epsilon();
}

template<std::integral T>
inline static T DoubleAsIntegralFloor(const double value)
{
	if (value < 0.0)
		return static_cast<T>(value - std::numeric_limits<double>::epsilon());
	else
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
// Other helpers

inline constexpr bool inRange(std::integral auto value, std::integral auto min, std::integral auto max)
{
	return value >= min && value < max;
}

inline constexpr bool inRangeInclusive(std::integral auto value, std::integral auto min, std::integral auto max)
{
	return value >= min && value <= max;
}

inline constexpr bool inRangeExclusive(std::integral auto value, std::integral auto min, std::integral auto max)
{
	return value > min && value < max;
}

template<typename C, typename... Pack>
inline constexpr bool inList(C&& check, Pack&&... values)
{
	return ((check == values) || ...);
}

template<typename C, typename T, std::size_t N>
inline constexpr bool inList(C&& check, const std::array<T, N>& arr)
{
	return std::apply([&](const auto&... values)
	{
		return inList(std::forward<C>(check), values...);
	}, arr);
}

//----------------------------
// Tags

//clang-format off
struct inform_client_t { explicit inform_client_t() = default; };
inline constexpr inform_client_t inform_client{};

struct clear_container_t { explicit clear_container_t() = default; };
inline constexpr clear_container_t clear_container{};
// clang-format on

//----------------------------
// RAII structs

template<typename T>
struct SetAndRestore
{
	SetAndRestore(T& var, T newValue)
		: m_var(var), m_oldValue(var)
	{
		var = newValue;
	}
	~SetAndRestore()
	{
		m_var = m_oldValue;
	}

private:
	T& m_var;
	T m_oldValue;
};

//----------------------------
// Enums

enum class CarryObjectSprite : uint8_t
{
	BOMB = 0,
	BUSH = 1,
	STONE = 3,
	VASE = 5,
	SIGN = 7,
	SUPERBOMB = 61,
	JOLTBOMB = 87,
	HOTJOLTBOMB = 88,
	HOTBOMB = 200,
	BLACKSTONE = 201,
	NPC = 251,
	NONE = 255
};

enum class CarryObjectType : uint8_t
{
	NONE = 0,
	BOMB = 1,
	BUSH = 2,
	STONE = 3,
	VASE = 4,
	SIGN = 5,
	SUPERBOMB = 6,
	JOLTBOMB = 7,
	HOTJOLTBOMB = 8,
	HOTBOMB = 9,
	BLACKSTONE = 10,
	NPC = 11,
	PLAYER = 12,
	//
	COUNT
};

// clang-format off
inline constexpr std::array<CarryObjectSprite, static_cast<size_t>(CarryObjectType::COUNT)> carryObjectSpriteForType{
	CarryObjectSprite::NONE,		// CarryObjectType::NONE
	CarryObjectSprite::BOMB,		// CarryObjectType::BOMB
	CarryObjectSprite::BUSH,		// CarryObjectType::BUSH
	CarryObjectSprite::STONE,		// CarryObjectType::STONE
	CarryObjectSprite::VASE,		// CarryObjectType::VASE
	CarryObjectSprite::SIGN,		// CarryObjectType::SIGN
	CarryObjectSprite::SUPERBOMB,	// CarryObjectType::SUPERBOMB
	CarryObjectSprite::JOLTBOMB,	// CarryObjectType::JOLTBOMB
	CarryObjectSprite::HOTJOLTBOMB, // CarryObjectType::HOTJOLTBOMB
	CarryObjectSprite::HOTBOMB,		// CarryObjectType::HOTBOMB
	CarryObjectSprite::BLACKSTONE,	// CarryObjectType::BLACKSTONE
	CarryObjectSprite::NPC,			// CarryObjectType::NPC
	CarryObjectSprite::NPC,			// CarryObjectType::PLAYER
};
// clang-format on

inline constexpr CarryObjectType getCarryObjectType(const CarryObjectSprite sprite)
{
	for (size_t i = 0; i < carryObjectSpriteForType.size(); ++i)
	{
		if (carryObjectSpriteForType[i] == sprite)
			return static_cast<CarryObjectType>(i);
	}
	return CarryObjectType::NONE;
}

////////////////////////////////////////////////////////////////////////////////
}; // end namespace preagonal
////////////////////////////////////////////////////////////////////////////////
namespace preagonal::string
{
////////////////////////////////////////////////////////////////////////////////

/* These functions need to be here since putting them under StringUtils.h results in a cyclic dependency. */

/// @brief Converts a range of strings to a single CSV-formatted string, quoting fields as needed.
/// @param force_quoted If true, all fields will be quoted regardless of content.
/// @param args Multiple string-like objects, of the same type, that will be concatenated together in a CSV format.
/// @return A std::string containing the CSV-formatted representation of the input range, with fields separated by commas and quoted as necessary.
template<StringViewIshVariant... Args>
auto toCSVFromPack(const bool force_quoted, Args&&... args)
{
	auto view = preagonal::toNonOwningRange<Args...>(std::forward<Args>(args)...);
	return toCSV(std::move(view), force_quoted);
}

/// @brief Converts a range of strings to a single CSV-formatted string, quoting fields as needed, not forcing quotes.
/// @param args Multiple string-like objects, of the same type, that will be concatenated together in a CSV format.
/// @return A std::string containing the CSV-formatted representation of the input range, with fields separated by commas and quoted as necessary.
template<StringViewIshVariant... Args>
auto toCSVFromPack(Args&&... args)
{
	auto view = preagonal::toNonOwningRange<Args...>(std::forward<Args>(args)...);
	return toCSV(std::move(view), false);
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::string
////////////////////////////////////////////////////////////////////////////////

//----------------------------
// Macros

#ifdef DEBUG
	#include <utilities/Log.h>
	#define DEBUGPRINT(...)                              \
		do { log::printLine(log::server, __VA_ARGS__); } \
		while (false)
#else
	#define DEBUGPRINT(...)
#endif

#define RETURN_CONSTRUCTPROPSFOR_CONSTEXPR(prop, type, ...) \
	if constexpr (P == prop) return type{values...};
#define RETURN_GETPROP_CONSTEXPR(prop, type, ...) \
	if constexpr (P == prop) return type{__VA_ARGS__};

#endif // COMMONTYPES_H
