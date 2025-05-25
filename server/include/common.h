#ifndef COMMON_H
#define COMMON_H

#include <utility>
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ctime>
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

inline constexpr PlayerID NPCServerPlayerID = 0;

inline static constexpr uint8_t PROPID(auto prop)
{
	return static_cast<uint8_t>(prop);
}
using prop_access = std::variant<int8_t*, int16_t*, uint8_t*, uint16_t*, uint32_t*, float*, std::string*, std::pair<float*, float*>>;

//-----------------------------------------------

inline int64_t currentTimeInSeconds()
{
	using namespace std::chrono;
	return static_cast<int64_t>(duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
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
