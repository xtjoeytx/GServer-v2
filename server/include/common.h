#ifndef COMMON_H
#define COMMON_H

#include <utility>
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <stdexcept>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <variant>

// TODO: Replace time.h with <chrono> across the program.
#include <chrono>
#include <time.h>

#include "BabyDI.h"

using namespace std::literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

using PlayerID = uint16_t;
using NPCID = uint32_t;

inline constexpr PlayerID NPCServerPlayerID = 2;
inline constexpr PlayerID EXTERNALPLAYERID_INIT = 16000;

//-----------------------------------------------

inline constexpr uint8_t operator""_ui8(unsigned long long val)
{
	return static_cast<uint8_t>(val);
}

//-----------------------------------------------

inline static constexpr uint8_t PROPID(auto prop)
{
	return static_cast<uint8_t>(prop);
}

//-----------------------------------------------

namespace chrono = std::chrono;
using clock = std::chrono::system_clock;

inline clock::time_point currentTime()
{
	return clock::now();
}

inline clock::time_point convertFromTimeT(time_t time)
{
	return clock::from_time_t(time);
}

//-----------------------------------------------

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
concept all_same = sizeof...(Ts) < 2 ||
	std::conjunction_v<
		std::is_same<std::tuple_element_t<0, std::tuple<Ts...>>, Ts>...
	>;

template<class O, class... Ts>
concept all_same_as = sizeof...(Ts) < 2 ||
	(std::conjunction_v<std::is_same<std::tuple_element_t<0, std::tuple<Ts...>>, Ts>...>
		&& std::same_as<O, std::tuple_element_t<0, std::tuple<Ts...>>>);

//-----------------------------------------------

template<class... Ts>
struct visit_functions : Ts...
{
	using Ts::operator()...;
};

//-----------------------------------------------

// TODO: Move to somewhere appropriate.
template <typename T>
struct Position
{
	Position() : data(T{}, T{}) {}
	Position(T x, T y) : data(x, y) {}
	std::tuple<T, T> data;
	T x() const { return std::get<0>(data); }
	T y() const { return std::get<1>(data); }
};

template <typename T>
struct Dimension
{
	Dimension() : data(T{}, T{}) {}
	Dimension(T width, T height) : data(width, height) {}
	std::tuple<T, T> data;
	T width() const { return std::get<0>(data); }
	T height() const { return std::get<1>(data); }
};

template <typename P, typename S>
struct Rectangle
{
	Rectangle() {}
	Rectangle(Position<P> position, Dimension<S> size) : position(position), size(size) {}
	Position<P> position{};
	Dimension<S> size{};
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#undef ERROR

#endif // COMMON_H
