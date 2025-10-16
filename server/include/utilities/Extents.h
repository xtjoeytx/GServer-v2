#ifndef EXTENTS_H
#define EXTENTS_H

#include <array>
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
	constexpr Position() : data{ T{}, T{}, T{} } {}
	constexpr Position(T x, T y) : data{ x, y, T{} } {}
	constexpr Position(T x, T y, T z) : data{ x, y, z } {}

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

	std::array<T, 3> data;
};

using PixelPosition = Position<int32_t>;
using LocalPixelPosition = Position<int16_t>;
using TilePosition = Position<float>;
using LocalWholeTilePosition = Position<uint8_t>;

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

	Position<P> position{};
	Dimension<S> size{};
};

using PixelRectangleArea = Rectangle<int32_t, uint16_t>;
using LocalPixelRectangleArea = Rectangle<int16_t, uint16_t>;
using LocalWholeTileRectangleArea = Rectangle<uint8_t, uint8_t>;
using ImagePartRectangle = Rectangle<uint16_t, uint8_t>;
using TileRectangleArea = Rectangle<float, float>;
using WholeTileRectangleArea = Rectangle<float, uint8_t>;

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
	return LocalPixelPosition{ static_cast<int16_t>(halfTileX * 8) % 1024, static_cast<int16_t>(halfTileY * 8) % 1024 };
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
		return LocalPixelPosition{ static_cast<int16_t>(position.x() * 16) % 1024, static_cast<int16_t>(position.y() * 16) % 1024, static_cast<int16_t>(position.z() * 16) };
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
	// Just convert the units.
	else
	{
		return LocalWholeTilePosition{ static_cast<uint8_t>(position.x()), static_cast<uint8_t>(position.y()), static_cast<uint8_t>(position.z()) };
	}
}

inline constexpr std::pair<uint8_t, uint8_t> toMapPosition(const PixelPosition& position)
{
	return { static_cast<uint8_t>(position.x() / 1024), static_cast<uint8_t>(position.y() / 1024) };
}

//----------------------------

inline constexpr PixelRectangleArea toPixelRectangleArea(const TileRectangleArea& rect)
{
	Dimension<uint16_t> size{ static_cast<uint16_t>(rect.size.width() * 16), static_cast<uint16_t>(rect.size.height() * 16), static_cast<uint16_t>(rect.size.length() * 16) };
	return PixelRectangleArea{ toPixelPosition(rect.position), size };
}

template<typename P, typename S>
inline constexpr PixelRectangleArea toPixelRectangleArea(const PixelPosition& origin, const Rectangle<P, S>& rect)
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
inline constexpr LocalWholeTileRectangleArea toLocalWholeTileRectangleArea(const PixelPosition& origin, const Rectangle<P, S>& rect)
{
	// Same coordinates.
	if constexpr (std::same_as<P, uint8_t>)
	{
		Dimension<uint8_t> size{ static_cast<uint8_t>(rect.size.width()), static_cast<uint8_t>(rect.size.height()), static_cast<uint8_t>(rect.size.length()) };
		return LocalWholeTileRectangleArea{ rect.position, size };
	}
	// Tiles to local whole tiles.
	else if constexpr (std::same_as<P, float>)
	{
		auto x = static_cast<int32_t>(rect.position.x() + std::numeric_limits<float>::epsilon());
		auto y = static_cast<int32_t>(rect.position.y() + std::numeric_limits<float>::epsilon());
		auto z = static_cast<int32_t>(rect.position.z() + std::numeric_limits<float>::epsilon());
		auto width = static_cast<int32_t>(rect.size.width() + std::numeric_limits<float>::epsilon());
		auto height = static_cast<int32_t>(rect.size.height() + std::numeric_limits<float>::epsilon());
		auto length = static_cast<int32_t>(rect.size.length() + std::numeric_limits<float>::epsilon());

		// If the relative position to the origin is negative, we need to adjust it to be within the local level.
		if (x * 16 < origin.x())
		{
			width -= (origin.x() - (x * 16)) / 16;
			x = 0;
		}
		if (y * 16 < origin.y())
		{
			height -= (origin.y() - (y * 16)) / 16;
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
	// Pixels to local whole tiles.
	else if constexpr (std::same_as<P, int16_t> || std::same_as<P, int32_t>)
	{
		auto x = static_cast<int32_t>(rect.position.x() / 16);
		auto y = static_cast<int32_t>(rect.position.y() / 16);
		auto z = static_cast<int32_t>(rect.position.z() / 16);
		auto width = static_cast<int32_t>(rect.size.width() / 16);
		auto height = static_cast<int32_t>(rect.size.height() / 16);
		auto length = static_cast<int32_t>(rect.size.length() / 16);

		// If the relative position to the origin is negative, we need to adjust it to be within the local level.
		if (x * 16 < origin.x()) x = 0;
		if (y * 16 < origin.y()) y = 0;

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
	// Just convert the units.
	else
	{
		Position<uint8_t> pos{ static_cast<uint8_t>(rect.position.x()), static_cast<uint8_t>(rect.position.y()), static_cast<uint8_t>(rect.position.z()) };
		Dimension<uint8_t> size{ static_cast<uint8_t>(rect.size.width()), static_cast<uint8_t>(rect.size.height()), static_cast<uint8_t>(rect.size.length()) };
		return LocalWholeTileRectangleArea{ pos, size };
	}
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
constexpr T&& get(const preagonal::Position<T>& vec) { return vec.data[I]; }

template<size_t I, typename T>
constexpr T& get(preagonal::Dimension<T>& vec) { return vec.data[I]; }

template<size_t I, typename T>
constexpr T&& get(const preagonal::Dimension<T>& vec) { return vec.data[I]; }

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
