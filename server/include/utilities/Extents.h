#ifndef EXTENTS_H
#define EXTENTS_H

#include <array>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

using namespace std::literals;

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept PixelBasedPosition = std::integral<T>;

template<typename T>
concept TileBasedPosition = std::floating_point<T>;

//----------------------------
// Position

template<typename T>
struct Position
{
	using ValueType = T;

	constexpr Position() : data{ T{}, T{}, T{} } {}
	constexpr Position(T x, T y) : data{ x, y, T{} } {}
	constexpr Position(T x, T y, T z) : data{ x, y, z } {}
	constexpr Position(const Position<T>& other) : data{ other.data } {}

	template<typename O>
	constexpr Position(const Position<O>& other) : data{ T{ other.data[0] }, T{ other.data[1] }, T{ other.data[2] } } {}

	constexpr bool operator==(const Position<T>& other) const
	{
		return data == other.data;
	}
	constexpr bool operator!=(const Position<T>& other) const
	{
		return data != other.data;
	}

	constexpr T& operator[](size_t index)
	{
		if (index >= 3) throw std::out_of_range("Index out of range for Position");
		return data[index];
	}
	constexpr const T& operator[](size_t index) const
	{
		if (index >= 3) throw std::out_of_range("Index out of range for Position");
		return data[index];
	}

	constexpr T& x() { return data[0]; }
	constexpr T& y() { return data[1]; }
	constexpr T& z() { return data[2]; }
	constexpr const T& x() const { return data[0]; }
	constexpr const T& y() const { return data[1]; }
	constexpr const T& z() const { return data[2]; }

	constexpr Position<T>& translate(T dx, T dy)
	{
		data[0] += dx;
		data[1] += dy;
		return *this;
	}
	constexpr Position<T>& translate(T dx, T dy, T dz)
	{
		data[0] += dx;
		data[1] += dy;
		data[2] += dz;
		return *this;
	}

	template<typename O>
	constexpr Position<T>& translate(const Position<O>& delta)
	{
		data[0] += static_cast<T>(delta.data[0]);
		data[1] += static_cast<T>(delta.data[1]);
		data[2] += static_cast<T>(delta.data[2]);
		return *this;
	}

	Position<T> translate(T dx, T dy) const
	{
		Position<T> result{ *this };
		result.translate(dx, dy);
		return result;
	}
	Position<T> translate(T dx, T dy, T dz) const
	{
		Position<T> result{ *this };
		result.translate(dx, dy, dz);
		return result;
	}

	template<typename O>
	Position<T> translate(const Position<O>& delta) const
	{
		Position<T> result{ *this };
		result.translate(delta);
		return result;
	}

	T length2D() const
	{
		return static_cast<T>(std::sqrt(data[0] * data[0] + data[1] * data[1]));
	}

	T length3D() const
	{
		return static_cast<T>(std::sqrt(data[0] * data[0] + data[1] * data[1] + data[2] * data[2]));
	}

	template<typename O>
	Position<T>& normalize2D(O length)
	{
		if (length == O{})
			throw std::invalid_argument("Cannot normalize a position with zero length.");
		data[0] = static_cast<T>(data[0] / length);
		data[1] = static_cast<T>(data[1] / length);
		return *this;
	}

	template<typename O>
	Position<T> normalize2D(O length) const
	{
		if (length == O{})
			throw std::invalid_argument("Cannot normalize a position with zero length.");
		Position<T> result{ *this };
		result.data[0] = static_cast<T>(result.data[0] / length);
		result.data[1] = static_cast<T>(result.data[1] / length);
		return result;
	}

	template<typename O>
	Position<T>& normalize3D(O length)
	{
		if (length == O{})
			throw std::invalid_argument("Cannot normalize a position with zero length.");
		data[0] = static_cast<T>(data[0] / length);
		data[1] = static_cast<T>(data[1] / length);
		data[2] = static_cast<T>(data[2] / length);
		return *this;
	}

	template<typename O>
	Position<T> normalize3D(O length) const
	{
		if (length == O{})
			throw std::invalid_argument("Cannot normalize a position with zero length.");
		Position<T> result{ *this };
		result.data[0] = static_cast<T>(result.data[0] / length);
		result.data[1] = static_cast<T>(result.data[1] / length);
		result.data[2] = static_cast<T>(result.data[2] / length);
		return result;
	}

	std::array<T, 3> data;
};

using PixelPosition = Position<int32_t>;
using LocalPixelPosition = Position<int16_t>;

using TilePosition = Position<float>;
using WholeTilePosition = Position<uint16_t>;
using LocalWholeTilePosition = Position<uint8_t>;

using MapPosition = Position<uint8_t>;

//----------------------------
// MovingPosition

template<typename T>
struct MovingPosition
{
	Position<T> position{};
	Position<T> velocity{};
};

//----------------------------
// Dimension

template<typename T>
struct Dimension
{
	constexpr Dimension() requires PixelBasedPosition<T> : data{ T{}, T{}, T{48} } {}
	constexpr Dimension() requires TileBasedPosition<T> : data{ T{}, T{}, T{3} } {}
	constexpr Dimension(T width, T height) requires PixelBasedPosition<T> : data{ width, height, T{48} } {}
	constexpr Dimension(T width, T height) requires TileBasedPosition<T> : data{ width, height, T{3} } {}
	constexpr Dimension(T width, T height, T length) : data{ width, height, length } {}
	constexpr Dimension(const Dimension<T>& other) : data{ other.data } {}

	template<typename O>
	constexpr Dimension(const Dimension<O>& other) : data{ T{ other.data[0] }, T{ other.data[1] }, T{ other.data[2] } } {}

	constexpr bool operator==(const Dimension<T>& other) const
	{
		return data == other.data;
	}
	constexpr bool operator!=(const Dimension<T>& other) const
	{
		return data != other.data;
	}

	constexpr T& operator[](size_t index)
	{
		if (index >= 3) throw std::out_of_range("Index out of range for Dimension");
		return data[index];
	}
	constexpr const T& operator[](size_t index) const
	{
		if (index >= 3) throw std::out_of_range("Index out of range for Dimension");
		return data[index];
	}

	constexpr T& width() { return data[0]; }
	constexpr T& height() { return data[1]; }
	constexpr T& length() { return data[2]; }
	constexpr const T& width() const { return data[0]; }
	constexpr const T& height() const { return data[1]; }
	constexpr const T& length() const { return data[2]; }

	std::array<T, 3> data;
};

//----------------------------
// Rectangle (area)

template<typename P, typename S>
struct Rectangle
{
	constexpr Rectangle() {}
	constexpr Rectangle(Position<P> position, Dimension<S> size) : position(position), size(size) {}

	constexpr P left() const noexcept { return position.x(); }
	constexpr P right() const noexcept { return position.x() + size.width(); }
	constexpr P top() const noexcept { return position.y(); }
	constexpr P bottom() const noexcept { return position.y() + size.height(); }
	constexpr P ground() const noexcept { return position.z(); }
	constexpr P sky() const noexcept { return position.z() + size.length(); }
	constexpr Position<P> center() const noexcept { return { position.x() + static_cast<P>(size.width() / (S)2), position.y() + static_cast<P>(size.height() / (S)2) }; }

	Position<P> position{};
	Dimension<S> size{};
};

using PixelRectangleArea = Rectangle<int32_t, uint16_t>;
using LocalPixelRectangleArea = Rectangle<int16_t, uint16_t>;

using TileRectangleArea = Rectangle<float, float>;
using WholeTileRectangleArea = Rectangle<uint16_t, uint8_t>;
using LocalWholeTileRectangleArea = Rectangle<uint8_t, uint8_t>;

using ImagePartRectangle = Rectangle<uint16_t, uint8_t>;

//----------------------------
// Intersections

template<typename Pos, typename RectPos, typename RectDim>
inline constexpr bool positionInRectangle(const Position<Pos>& pos, const Rectangle<RectPos, RectDim>& rect)
{
	return pos.x() >= rect.left() && pos.x() <= rect.right()
		&& pos.y() >= rect.top() && pos.y() <= rect.bottom()
		&& pos.z() >= rect.ground() && pos.z() <= rect.sky();
}

template<typename RectPosL, typename RectDimL, typename RectPosR, typename RectDimR>
inline constexpr bool rectanglesIntersect(const Rectangle<RectPosL, RectDimL>& first, const Rectangle<RectPosR, RectDimR>& second)
{
	return (first.right() < second.left()
		|| second.right() < first.left()
		|| first.bottom() < second.top()
		|| second.bottom() < first.top()
		|| first.sky() < second.ground()
		|| second.sky() < first.ground()
		) == false;
}

template<typename RectPosL, typename RectDimL, typename RectPosR, typename RectDimR>
inline constexpr bool rectangleContained(const Rectangle<RectPosL, RectDimL>& child, const Rectangle<RectPosR, RectDimR>& parent)
{
	return (child.left() >= parent.left()
		&& child.right() <= parent.right()
		&& child.top() >= parent.top()
		&& child.bottom() <= parent.bottom()
		&& child.ground() >= parent.ground()
		&& child.sky() <= parent.sky());
}

//----------------------------
// Translations

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

template<typename T, typename O>
inline Position<T> translatePosition(const Position<T>& position, const Position<O>& delta)
{
	return position.translate(delta);
}

//----------------------------
// Conversions

inline constexpr PixelPosition toPixelPosition(const PixelPosition& origin, std::floating_point auto x, std::floating_point auto y)
{
	// Enforce half tile increments.  We will never have a float position that isn't a half tile.
	int32_t halfTileX = static_cast<int32_t>(x * 2);
	int32_t halfTileY = static_cast<int32_t>(y * 2);
	return PixelPosition{ static_cast<int32_t>(origin.x() + (halfTileX * 8)), static_cast<int32_t>(origin.y() + (halfTileY * 8)), static_cast<int32_t>(origin.z()) };
}

inline constexpr PixelPosition toPixelPosition(const TilePosition& position)
{
	return PixelPosition{ static_cast<int32_t>(position.x() * 16), static_cast<int32_t>(position.y() * 16), static_cast<int32_t>(position.z() * 16) };
}

template<typename Type>
inline constexpr PixelPosition toPixelPosition(const PixelPosition& origin, const Position<Type>& position)
{
	// Same coordinates.
	if constexpr (std::same_as<Type, int32_t>)
	{
		return origin + position;
	}
	// Tiles to pixels.
	else if constexpr (std::same_as<Type, uint8_t> || std::same_as<Type, float>)
	{
		return PixelPosition{ origin.x() + static_cast<int32_t>(position.x() * 16), origin.y() + static_cast<int32_t>(position.y() * 16), origin.z() + static_cast<int32_t>(position.z() * 16) };
	}
	// Just convert the units.
	else
	{
		return PixelPosition{ origin.x() + static_cast<int32_t>(position.x()), origin.y() + static_cast<int32_t>(position.y()), origin.z() + static_cast<int32_t>(position.z()) };
	}
}

inline constexpr LocalPixelPosition toLocalPixelPosition(std::floating_point auto x, std::floating_point auto y)
{
	// Enforce half tile increments.  We will never have a float position that isn't a half tile.
	int16_t halfTileX = static_cast<int16_t>(x * 2);
	int16_t halfTileY = static_cast<int16_t>(y * 2);
	return LocalPixelPosition{ static_cast<int16_t>((halfTileX * 8) % 1024), static_cast<int16_t>((halfTileY * 8) % 1024) };
}

template<typename Type>
inline constexpr LocalPixelPosition toLocalPixelPosition(const Position<Type>& position)
{
	// Same coordinates.
	if constexpr (std::same_as<Type, int16_t>)
	{
		return position;
	}
	// Global pixel position to local.
	else if constexpr (std::same_as<Type, int32_t>)
	{
		return LocalPixelPosition{ static_cast<int16_t>(position.x() % 1024), static_cast<int16_t>(position.y() % 1024), static_cast<int16_t>(position.z()) };
	}
	// Tiles to local pixels.
	else if constexpr (std::same_as<Type, uint8_t> || std::same_as<Type, float>)
	{
		return LocalPixelPosition{ static_cast<int16_t>(static_cast<int16_t>(position.x() * 16) % 1024), static_cast<int16_t>(static_cast<int16_t>(position.y() * 16) % 1024), static_cast<int16_t>(position.z() * 16) };
	}
	// Just convert the units.
	else
	{
		return LocalPixelPosition{ static_cast<int16_t>(position.x()), static_cast<int16_t>(position.y()), static_cast<int16_t>(position.z()) };
	}
}

template<typename Type>
inline constexpr TilePosition toTilePosition(const Position<Type>& position)
{
	// Pixels to tiles.
	if constexpr (std::same_as<Type, int16_t> || std::same_as<Type, int32_t>)
	{
		return TilePosition{ static_cast<float>(position.x()) / 16.0f, static_cast<float>(position.y()) / 16.0f, static_cast<float>(position.z()) / 16.0f };
	}
	// Same coordinates.
	else if constexpr (std::same_as<Type, float>)
	{
		return position;
	}
	// Just convert the units.
	else
	{
		return TilePosition{ static_cast<float>(position.x()), static_cast<float>(position.y()), static_cast<float>(position.z()) };
	}
}

template<typename Type>
inline constexpr LocalWholeTilePosition toLocalWholeTilePosition(const Position<Type>& position)
{
	// Pixels to local whole tiles.
	if constexpr (std::same_as<Type, int16_t> || std::same_as<Type, int32_t>)
	{
		auto x = static_cast<int32_t>((position.x() % 1024) / 16);
		auto y = static_cast<int32_t>((position.y() % 1024) / 16);
		auto z = static_cast<int32_t>(position.z() / 16);
		return LocalWholeTilePosition{ static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(z) };
	}
	// Same coordinates.
	else if constexpr (std::same_as<Type, uint8_t>)
	{
		return position;
	}
	// Tiles to local whole tiles.
	else if constexpr (std::same_as<Type, float>)
	{
		auto x = static_cast<int32_t>(position.x() + std::numeric_limits<float>::epsilon());
		auto y = static_cast<int32_t>(position.y() + std::numeric_limits<float>::epsilon());
		auto z = static_cast<int32_t>(position.z() + std::numeric_limits<float>::epsilon());
		return LocalWholeTilePosition{ static_cast<uint8_t>(x % 64), static_cast<uint8_t>(y % 64), static_cast<uint8_t>(z) };
	}
	// Whole tiles to local whole tiles.
	else if constexpr (std::same_as<Type, uint16_t>)
	{
		auto x = static_cast<int32_t>(position.x());
		auto y = static_cast<int32_t>(position.y());
		auto z = static_cast<int32_t>(position.z());
		return LocalWholeTilePosition{ static_cast<uint8_t>(x % 64), static_cast<uint8_t>(y % 64), static_cast<uint8_t>(z) };
	}
	// Just convert the units.
	else
	{
		return LocalWholeTilePosition{ static_cast<uint8_t>(position.x()), static_cast<uint8_t>(position.y()), static_cast<uint8_t>(position.z()) };
	}
}

template<typename Type>
inline constexpr WholeTilePosition toWholeTilePosition(const Position<Type>& position)
{
	// Pixels to whole tiles.
	if constexpr (std::same_as<Type, int16_t> || std::same_as<Type, int32_t>)
	{
		auto x = static_cast<uint16_t>(position.x() / 16);
		auto y = static_cast<uint16_t>(position.y() / 16);
		auto z = static_cast<uint16_t>(position.z() / 16);
		return WholeTilePosition{ static_cast<uint16_t>(x), static_cast<uint16_t>(y), static_cast<uint16_t>(z) };
	}
	// Same coordinates.
	else if constexpr (std::same_as<Type, uint16_t>)
	{
		return position;
	}
	// Tiles to whole tiles.
	else if constexpr (std::same_as<Type, float>)
	{
		auto x = static_cast<int32_t>(position.x() + std::numeric_limits<float>::epsilon());
		auto y = static_cast<int32_t>(position.y() + std::numeric_limits<float>::epsilon());
		auto z = static_cast<int32_t>(position.z() + std::numeric_limits<float>::epsilon());
		return WholeTilePosition{ static_cast<uint16_t>(x), static_cast<uint16_t>(y), static_cast<uint16_t>(z) };
	}
	// Just convert the units.
	else
	{
		return WholeTilePosition{ static_cast<uint16_t>(position.x()), static_cast<uint16_t>(position.y()), static_cast<uint16_t>(position.z()) };
	}
}

inline constexpr MapPosition toMapPosition(const PixelPosition& position)
{
	return { static_cast<uint8_t>(position.x() / 1024), static_cast<uint8_t>(position.y() / 1024) };
}

inline constexpr MapPosition toMapPosition(const TilePosition& position)
{
	return { static_cast<uint8_t>((position.x() + std::numeric_limits<float>::epsilon()) / 64), static_cast<uint8_t>((position.y() + std::numeric_limits<float>::epsilon()) / 64) };
}

inline constexpr MapPosition toMapPosition(const WholeTilePosition& position)
{
	return { static_cast<uint8_t>(position.x() / 64), static_cast<uint8_t>(position.y() / 64) };
}

//----------------------------

inline constexpr PixelRectangleArea toPixelRectangleArea(const TileRectangleArea& rect)
{
	Dimension<uint16_t> size{ static_cast<uint16_t>(rect.size.width() * 16), static_cast<uint16_t>(rect.size.height() * 16), static_cast<uint16_t>(rect.size.length() * 16) };
	return PixelRectangleArea{ toPixelPosition(rect.position), size };
}

template<typename P, typename S>
inline constexpr PixelRectangleArea toPixelRectangleArea(const MapPosition& origin, const Rectangle<P, S>& rect)
{
	// Same coordinates.
	if constexpr (std::same_as<P, int32_t>)
	{
		Dimension<uint16_t> size{ static_cast<uint16_t>(rect.size.width()), static_cast<uint16_t>(rect.size.height()), static_cast<uint16_t>(rect.size.length()) };
		return PixelRectangleArea{ rect.position, size };
	}
	// Tiles to pixels.
	else if constexpr (std::same_as<P, float>)
	{
		return toPixelRectangleArea(rect);
	}
	// Local tiles to pixels.
	else if constexpr (std::same_as<P, uint8_t>)
	{
		Dimension<uint16_t> size{ static_cast<uint16_t>(rect.size.width() * 16), static_cast<uint16_t>(rect.size.height() * 16), static_cast<uint16_t>(rect.size.length() * 16) };
		return PixelRectangleArea{ toPixelPosition(origin, rect.position), size };
	}
	// Just convert the units.
	else
	{
		Dimension<uint16_t> size{ static_cast<uint16_t>(rect.size.width()), static_cast<uint16_t>(rect.size.height()), static_cast<uint16_t>(rect.size.length()) };
		return PixelRectangleArea{ toPixelPosition(origin, rect.position), size };
	}
}

template<typename P, typename S>
inline constexpr LocalWholeTileRectangleArea toLocalWholeTileRectangleArea(const Rectangle<P, S>& rect)
{
	// Same coordinates.
	if constexpr (std::same_as<P, uint8_t>)
	{
		Dimension<uint8_t> size{ static_cast<uint8_t>(rect.size.width()), static_cast<uint8_t>(rect.size.height()), static_cast<uint8_t>(rect.size.length()) };
		return LocalWholeTileRectangleArea{ rect.position, size };
	}

	int32_t x;
	int32_t y;
	int32_t z;
	int32_t width;
	int32_t height;
	int32_t length;

	// Tiles to local whole tiles.
	if constexpr (std::same_as<P, float>)
	{
		x = static_cast<int32_t>(rect.position.x() + std::numeric_limits<float>::epsilon());
		y = static_cast<int32_t>(rect.position.y() + std::numeric_limits<float>::epsilon());
		z = static_cast<int32_t>(rect.position.z() + std::numeric_limits<float>::epsilon());
		width = static_cast<int32_t>(rect.size.width() + std::numeric_limits<float>::epsilon());
		height = static_cast<int32_t>(rect.size.height() + std::numeric_limits<float>::epsilon());
		length = static_cast<int32_t>(rect.size.length() + std::numeric_limits<float>::epsilon());
	}
	// Whole tiles to local whole tiles.
	else if constexpr (std::same_as<P, uint16_t>)
	{
		x = static_cast<int32_t>(rect.position.x());
		y = static_cast<int32_t>(rect.position.y());
		z = static_cast<int32_t>(rect.position.z());
		width = static_cast<int32_t>(rect.size.width());
		height = static_cast<int32_t>(rect.size.height());
		length = static_cast<int32_t>(rect.size.length());
	}
	// Pixels to local whole tiles.
	else if constexpr (std::same_as<P, int16_t> || std::same_as<P, int32_t>)
	{
		x = static_cast<int32_t>(rect.position.x() / 16);
		y = static_cast<int32_t>(rect.position.y() / 16);
		z = static_cast<int32_t>(rect.position.z() / 16);
		width = static_cast<int32_t>(rect.size.width() / 16);
		height = static_cast<int32_t>(rect.size.height() / 16);
		length = static_cast<int32_t>(rect.size.length() / 16);
	}
	// Just convert the units.
	else
	{
		Position<uint8_t> pos{ static_cast<uint8_t>(rect.position.x()), static_cast<uint8_t>(rect.position.y()), static_cast<uint8_t>(rect.position.z()) };
		Dimension<uint8_t> size{ static_cast<uint8_t>(rect.size.width()), static_cast<uint8_t>(rect.size.height()), static_cast<uint8_t>(rect.size.length()) };
		return LocalWholeTileRectangleArea{ pos, size };
	}

	LocalWholeTilePosition pos{ static_cast<uint8_t>(x % 64), static_cast<uint8_t>(y % 64), static_cast<uint8_t>(z) };
	Dimension<uint8_t> size{ static_cast<uint8_t>(width), static_cast<uint8_t>(height), static_cast<uint8_t>(length) };
	return LocalWholeTileRectangleArea{ pos, size };
}

template<typename P, typename S>
inline constexpr LocalWholeTileRectangleArea clipLocalWholeTileRectangleArea(const MapPosition& origin, const Rectangle<P, S>& rect)
{
	// Same coordinates.
	if constexpr (std::same_as<P, uint8_t>)
	{
		Dimension<uint8_t> size{ static_cast<uint8_t>(rect.size.width()), static_cast<uint8_t>(rect.size.height()), static_cast<uint8_t>(rect.size.length()) };
		return LocalWholeTileRectangleArea{ rect.position, size };
	}

	int32_t x;
	int32_t y;
	int32_t z;
	int32_t width;
	int32_t height;
	int32_t length;
	PixelPosition pixelOrigin = PixelPosition{ static_cast<int32_t>(origin.x()) * 1024, static_cast<int32_t>(origin.y()) * 1024, 0 };

	// Tiles to local whole tiles.
	if constexpr (std::same_as<P, float>)
	{
		x = static_cast<int32_t>(rect.position.x() + std::numeric_limits<float>::epsilon());
		y = static_cast<int32_t>(rect.position.y() + std::numeric_limits<float>::epsilon());
		z = static_cast<int32_t>(rect.position.z() + std::numeric_limits<float>::epsilon());
		width = static_cast<int32_t>(rect.size.width() + std::numeric_limits<float>::epsilon());
		height = static_cast<int32_t>(rect.size.height() + std::numeric_limits<float>::epsilon());
		length = static_cast<int32_t>(rect.size.length() + std::numeric_limits<float>::epsilon());
	}
	// Whole tiles to local whole tiles.
	else if constexpr (std::same_as<P, uint16_t>)
	{
		x = static_cast<int32_t>(rect.position.x());
		y = static_cast<int32_t>(rect.position.y());
		z = static_cast<int32_t>(rect.position.z());
		width = static_cast<int32_t>(rect.size.width());
		height = static_cast<int32_t>(rect.size.height());
		length = static_cast<int32_t>(rect.size.length());
	}
	// Pixels to local whole tiles.
	else if constexpr (std::same_as<P, int16_t> || std::same_as<P, int32_t>)
	{
		x = static_cast<int32_t>(rect.position.x() / 16);
		y = static_cast<int32_t>(rect.position.y() / 16);
		z = static_cast<int32_t>(rect.position.z() / 16);
		width = static_cast<int32_t>(rect.size.width() / 16);
		height = static_cast<int32_t>(rect.size.height() / 16);
		length = static_cast<int32_t>(rect.size.length() / 16);
	}
	// Just convert the units.
	else
	{
		Position<uint8_t> pos{ static_cast<uint8_t>(rect.position.x()), static_cast<uint8_t>(rect.position.y()), static_cast<uint8_t>(rect.position.z()) };
		Dimension<uint8_t> size{ static_cast<uint8_t>(rect.size.width()), static_cast<uint8_t>(rect.size.height()), static_cast<uint8_t>(rect.size.length()) };
		return LocalWholeTileRectangleArea{ pos, size };
	}

	// If the relative position to the origin is negative, we need to adjust it to be within the local level.
	if (x * 16 < pixelOrigin.x())
	{
		width -= (pixelOrigin.x() - (x * 16)) / 16;
		x = 0;
	}
	if (y * 16 < pixelOrigin.y())
	{
		height -= (pixelOrigin.y() - (y * 16)) / 16;
		y = 0;
	}

	// Adjust the position to fit within the local level.
	x %= 64;
	y %= 64;

	// If the boundaries are out of the local level, adjust them to fit.
	if (x + width > 64) width = 64 - x;
	if (y + height > 64) height = 64 - y;

	LocalWholeTilePosition pos{ static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(z) };
	Dimension<uint8_t> size{ static_cast<uint8_t>(width), static_cast<uint8_t>(height), static_cast<uint8_t>(length) };
	return LocalWholeTileRectangleArea{ pos, size };
}

inline constexpr WholeTileRectangleArea toWholeTileRectangleArea(const PixelRectangleArea& rect)
{
	return WholeTileRectangleArea{ toWholeTilePosition(rect.position), Dimension<uint8_t>(static_cast<uint8_t>(rect.size.width() / 16), static_cast<uint8_t>(rect.size.height() / 16)) };
}

inline constexpr WholeTileRectangleArea toWholeTileRectangleArea(const TileRectangleArea& rect)
{
	auto width = static_cast<uint8_t>(rect.size.width() + std::numeric_limits<float>::epsilon());
	auto height = static_cast<uint8_t>(rect.size.height() + std::numeric_limits<float>::epsilon());
	return WholeTileRectangleArea{ toWholeTilePosition(rect.position), Dimension<uint8_t>(width, height) };
}

//----------------------------
// Math

template<typename Type, std::integral OtherType>
inline constexpr Position<Type> operator*(const Position<Type>& left, const OtherType& right)
{
	return Position<Type>{ static_cast<Type>(left.x() * right), static_cast<Type>(left.y() * right), static_cast<Type>(left.z() * right) };
}

template<typename Type, std::integral OtherType>
inline constexpr Position<Type> operator+(const Position<Type>& left, const OtherType& right)
{
	return Position<Type>{ static_cast<Type>(left.x() + right), static_cast<Type>(left.y() + right), static_cast<Type>(left.z() + right) };
}

template<typename Type, std::integral OtherType>
inline constexpr Position<Type> operator-(const Position<Type>& left, const OtherType& right)
{
	return Position<Type>{ static_cast<Type>(left.x() - right), static_cast<Type>(left.y() - right), static_cast<Type>(left.z() - right) };
}

template<typename Type, std::integral OtherType>
inline constexpr Position<Type> operator/(const Position<Type>& left, const OtherType& right)
{
	return Position<Type>{ static_cast<Type>(left.x() / right), static_cast<Type>(left.y() / right), static_cast<Type>(left.z() / right) };
}

//

template<typename Type, std::floating_point OtherType>
inline constexpr Position<OtherType> operator*(const Position<Type>& left, const OtherType& right)
{
	return Position<OtherType>{ static_cast<OtherType>(left.x() * right), static_cast<OtherType>(left.y() * right), static_cast<OtherType>(left.z() * right) };
}

template<typename Type, std::floating_point OtherType>
inline constexpr Position<OtherType> operator+(const Position<Type>& left, const OtherType& right)
{
	return Position<OtherType>{ static_cast<OtherType>(left.x() + right), static_cast<OtherType>(left.y() + right), static_cast<OtherType>(left.z() + right) };
}

template<typename Type, std::floating_point OtherType>
inline constexpr Position<OtherType> operator-(const Position<Type>& left, const OtherType& right)
{
	return Position<OtherType>{ static_cast<OtherType>(left.x() - right), static_cast<OtherType>(left.y() - right), static_cast<OtherType>(left.z() - right) };
}

template<typename Type, std::floating_point OtherType>
inline constexpr Position<OtherType> operator/(const Position<Type>& left, const OtherType& right)
{
	return Position<OtherType>{ static_cast<OtherType>(left.x() / right), static_cast<OtherType>(left.y() / right), static_cast<OtherType>(left.z() / right) };
}

//

template<typename Type, typename OtherType>
inline constexpr Position<Type> operator*(const Position<Type>& left, const Position<OtherType>& right)
{
	return Position<Type>{ static_cast<Type>(left.x() * right.x()), static_cast<Type>(left.y() * right.y()), static_cast<Type>(left.z() * right.z()) };
}

template<typename Type, typename OtherType>
inline constexpr Position<Type> operator+(const Position<Type>& left, const Position<OtherType>& right)
{
	return Position<Type>{ static_cast<Type>(left.x() + right.x()), static_cast<Type>(left.y() + right.y()), static_cast<Type>(left.z() + right.z()) };
}

template<typename Type, typename OtherType>
inline constexpr Position<Type> operator-(const Position<Type>& left, const Position<OtherType>& right)
{
	return Position<Type>{ static_cast<Type>(left.x() - right.x()), static_cast<Type>(left.y() - right.y()), static_cast<Type>(left.z() - right.z()) };
}

template<typename Type, typename OtherType>
inline constexpr Position<Type> operator/(const Position<Type>& left, const Position<OtherType>& right)
{
	return Position<Type>{ static_cast<Type>(left.x() / right.x()), static_cast<Type>(left.y() / right.y()), static_cast<Type>(left.z() / right.z()) };
}

//----------------------------

template<typename Type, std::integral OtherType>
inline constexpr Dimension<Type> operator*(const Dimension<Type>& left, const OtherType& right)
{
	return Dimension<Type>{ static_cast<Type>(left.width() * right), static_cast<Type>(left.height() * right), static_cast<Type>(left.length() * right) };
}

template<typename Type, std::integral OtherType>
inline constexpr Dimension<Type> operator+(const Dimension<Type>& left, const OtherType& right)
{
	return Dimension<Type>{ static_cast<Type>(left.width() + right), static_cast<Type>(left.height() + right), static_cast<Type>(left.length() + right) };
}

template<typename Type, std::integral OtherType>
inline constexpr Dimension<Type> operator-(const Dimension<Type>& left, const OtherType& right)
{
	return Dimension<Type>{ static_cast<Type>(left.width() - right), static_cast<Type>(left.height() - right), static_cast<Type>(left.length() - right) };
}

template<typename Type, std::integral OtherType>
inline constexpr Dimension<Type> operator/(const Dimension<Type>& left, const OtherType& right)
{
	return Dimension<Type>{ static_cast<Type>(left.width() / right), static_cast<Type>(left.height() / right), static_cast<Type>(left.length() / right) };
}

//

template<typename Type, std::floating_point OtherType>
inline constexpr Dimension<OtherType> operator*(const Dimension<Type>& left, const OtherType& right)
{
	return Dimension<OtherType>{ static_cast<OtherType>(left.width() * right), static_cast<OtherType>(left.height() * right), static_cast<OtherType>(left.length() * right) };
}

template<typename Type, std::floating_point OtherType>
inline constexpr Dimension<OtherType> operator+(const Dimension<Type>& left, const OtherType& right)
{
	return Dimension<OtherType>{ static_cast<OtherType>(left.width() + right), static_cast<OtherType>(left.height() + right), static_cast<OtherType>(left.length() + right) };
}

template<typename Type, std::floating_point OtherType>
inline constexpr Dimension<OtherType> operator-(const Dimension<Type>& left, const OtherType& right)
{
	return Dimension<OtherType>{ static_cast<OtherType>(left.width() - right), static_cast<OtherType>(left.height() - right), static_cast<OtherType>(left.length() - right) };
}

template<typename Type, std::floating_point OtherType>
inline constexpr Dimension<OtherType> operator/(const Dimension<Type>& left, const OtherType& right)
{
	return Dimension<OtherType>{ static_cast<OtherType>(left.width() / right), static_cast<OtherType>(left.height() / right), static_cast<OtherType>(left.length() / right) };
}

//

template<typename Type, typename OtherType>
inline constexpr Dimension<Type> operator*(const Dimension<Type>& left, const Dimension<OtherType>& right)
{
	return Dimension<Type>{ static_cast<Type>(left.width() * right.width()), static_cast<Type>(left.height() * right.height()), static_cast<Type>(left.length() * right.length()) };
}

template<typename Type, typename OtherType>
inline constexpr Dimension<Type> operator+(const Dimension<Type>& left, const Dimension<OtherType>& right)
{
	return Dimension<Type>{ static_cast<Type>(left.width() + right.width()), static_cast<Type>(left.height() + right.height()), static_cast<Type>(left.length() + right.length()) };
}

template<typename Type, typename OtherType>
inline constexpr Dimension<Type> operator-(const Dimension<Type>& left, const Dimension<OtherType>& right)
{
	return Dimension<Type>{ static_cast<Type>(left.width() - right.width()), static_cast<Type>(left.height() - right.height()), static_cast<Type>(left.length() - right.length()) };
}

template<typename Type, typename OtherType>
inline constexpr Dimension<Type> operator/(const Dimension<Type>& left, const Dimension<OtherType>& right)
{
	return Dimension<Type>{ static_cast<Type>(left.width() / right.width()), static_cast<Type>(left.height() / right.height()), static_cast<Type>(left.length() / right.length()) };
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

// Structured bindings support.
namespace std
{
////////////////////////////////////////////////////////////////////////////////

// Position
template<typename T>
class tuple_size<preagonal::Position<T>> : public std::integral_constant<size_t, 3> {};

template<size_t I, typename T>
class tuple_element<I, preagonal::Position<T>> { public: using type = T; };

// Dimension
template<typename T>
class tuple_size<preagonal::Dimension<T>> : public std::integral_constant<size_t, 3> {};

template<size_t I, typename T>
class tuple_element<I, preagonal::Dimension<T>> { public: using type = T; };

// Rectangle
template<typename P, typename D>
class tuple_size<preagonal::Rectangle<P, D>> : public std::integral_constant<size_t, 2> {};

template<size_t I, typename P, typename D>
class tuple_element<I, preagonal::Rectangle<P, D>> : conditional<I == 0, P, D>
{
	static_assert(I < 2, "Index out of bounds for tuple_element<Rectangle>");
};

////////////////////////////////////////////////////////////////////////////////
}
////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

template<size_t I, typename T>
constexpr T& get(preagonal::Position<T>& vec) { return vec.data[I]; }

template<size_t I, typename T>
constexpr const T& get(const preagonal::Position<T>& vec) { return vec.data[I]; }

template<size_t I, typename T>
constexpr T& get(preagonal::Dimension<T>& vec) { return vec.data[I]; }

template<size_t I, typename T>
constexpr const T& get(const preagonal::Dimension<T>& vec) { return vec.data[I]; }

template <size_t I, typename P, typename D>
std::tuple_element_t<I, preagonal::Rectangle<P, D>>& get(preagonal::Rectangle<P, D>& rect)
{
	static_assert(I < 2, "Index out of bounds for get<Rectangle>");

	if constexpr (I == 0)
		return rect.position;
	else if constexpr (I == 1)
		return rect.size;
}

template <size_t I, typename P, typename D>
std::tuple_element_t<I, preagonal::Rectangle<P, D>>&& get(const preagonal::Rectangle<P, D>& rect)
{
	static_assert(I < 2, "Index out of bounds for get<Rectangle>");

	if constexpr (I == 0)
		return rect.position;
	else if constexpr (I == 1)
		return rect.size;
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

//////////////////////////////////////////////////
// Printing
//////////////////////////////////////////////////

template<std::integral T>
struct std::formatter<preagonal::Position<T>> : std::formatter<std::string>
{
	auto format(const preagonal::Position<T>& pos, std::format_context& ctx) const
	{
		return std::format_to(ctx.out(), "{},{},{}", pos.x(), pos.y(), pos.z());
	}
};

template<std::floating_point T>
struct std::formatter<preagonal::Position<T>> : std::formatter<std::string>
{
	auto format(const preagonal::Position<T>& pos, std::format_context& ctx) const
	{
		return std::format_to(ctx.out(), "{:04.2f},{:04.2f},{:04.2f}", pos.x(), pos.y(), pos.z());
	}
};

template<std::integral T>
struct std::formatter<preagonal::Dimension<T>> : std::formatter<std::string>
{
	auto format(const preagonal::Dimension<T>& dim, std::format_context& ctx) const
	{
		return std::format_to(ctx.out(), "{},{},{}", dim.width(), dim.height(), dim.length());
	}
};

template<std::floating_point T>
struct std::formatter<preagonal::Dimension<T>> : std::formatter<std::string>
{
	auto format(const preagonal::Dimension<T>& dim, std::format_context& ctx) const
	{
		return std::format_to(ctx.out(), "{:04.2f},{:04.2f},{:04.2f}", dim.width(), dim.height(), dim.length());
	}
};

#endif // EXTENTS_H
