#ifndef COMMON_H
#define COMMON_H

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

// TODO: Replace time.h with <chrono> across the program.
#include <chrono>
#include <time.h>

#include "BabyDI.h"

///////////////////////////////////////////////////////////////////////////////

using namespace std::literals;

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

using PlayerID = uint16_t;
using NPCID = uint32_t;

inline static constexpr uint8_t PROPID(auto prop)
{
	return static_cast<uint8_t>(prop);
}

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

#endif // COMMON_H
