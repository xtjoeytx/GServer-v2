#ifndef EXTENTS_H
#define EXTENTS_H

#include <array>
#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>

#include <utilities/CommonTypes.h>

using namespace std::literals;

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

//----------------------------
// Position

template<typename T>
struct Position
{
	Position() : data{ T{}, T{}, T{} } {}
	Position(T x, T y) : data{ x, y, T{} } {}
	Position(T x, T y, T z) : data{ x, y, z } {}

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

using PixelPosition = Position<int32_t>;
using LocalPixelPosition = Position<int16_t>;
using TilePosition = Position<float>;
using LocalWholeTilePosition = Position<uint8_t>;

//----------------------------
// Dimension

template<typename T>
struct Dimension
{
	Dimension() : data{ T{}, T{} } {}
	Dimension(T width, T height) : data{ width, height } {}

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

//----------------------------
// Rectangle (area)

template<typename P, typename S>
struct Rectangle
{
	Rectangle() {}
	Rectangle(Position<P> position, Dimension<S> size) : position(position), size(size) {}
	Position<P> position{};
	Dimension<S> size{};
};

using PixelRectangleArea = Rectangle<int32_t, uint16_t>;
using LocalPixelRectangleArea = Rectangle<int16_t, uint16_t>;

//----------------------------
// Intersections

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

//----------------------------
// Conversions

inline PixelPosition toPixelPosition(const PixelPosition& origin, std::floating_point auto x, std::floating_point auto y)
{
	// Enforce half tile increments.  We will never have a float position that isn't a half tile.
	int32_t halfTileX = static_cast<int32_t>(x * 2);
	int32_t halfTileY = static_cast<int32_t>(y * 2);
	return PixelPosition{ static_cast<int32_t>(origin.x() + (halfTileX * 8)), static_cast<int32_t>(origin.y() + (halfTileY * 8)), static_cast<int32_t>(origin.z())};
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

inline LocalPixelPosition toLocalPixelPosition(std::floating_point auto x, std::floating_point auto y)
{
	// Enforce half tile increments.  We will never have a float position that isn't a half tile.
	int16_t halfTileX = static_cast<int16_t>(x * 2);
	int16_t halfTileY = static_cast<int16_t>(y * 2);
	return LocalPixelPosition{ static_cast<int16_t>(halfTileX * 8), static_cast<int16_t>(halfTileY * 8) };
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
		return LocalWholeTilePosition{ static_cast<uint8_t>(x % 64), static_cast<uint8_t>(y % 64), z };
	}
	// Just convert the units.
	else
	{
		return LocalWholeTilePosition{ static_cast<uint8_t>(position.x()), static_cast<uint8_t>(position.y()), static_cast<uint8_t>(position.z()) };
	}
}

//----------------------------
// Math

template<typename Type>
inline Position<Type> operator*(const Position<Type>& left, int right)
{
	return Position<Type>{ static_cast<Type>(left.x() * right), static_cast<Type>(left.y() * right), static_cast<Type>(left.z() * right) };
}

template<typename Type>
inline Position<Type> operator+(const Position<Type>& left, int right)
{
	return Position<Type>{ static_cast<Type>(left.x() + right), static_cast<Type>(left.y() + right), static_cast<Type>(left.z() + right) };
}

template<typename Type>
inline Position<Type> operator-(const Position<Type>& left, int right)
{
	return Position<Type>{ static_cast<Type>(left.x() - right), static_cast<Type>(left.y() - right), static_cast<Type>(left.z() - right) };
}

template<typename Type>
inline Position<Type> operator/(const Position<Type>& left, int right)
{
	return Position<Type>{ static_cast<Type>(left.x() / right), static_cast<Type>(left.y() / right), static_cast<Type>(left.z() / right) };
}

template<typename Type, typename OtherType>
inline Position<Type> operator*(const Position<Type>& left, const Position<OtherType>& right)
{
	return Position<Type>{ static_cast<Type>(left.x() * right.x()), static_cast<Type>(left.y() * right.y()), static_cast<Type>(left.z() * right.z()) };
}

template<typename Type, typename OtherType>
inline Position<Type> operator+(const Position<Type>& left, const Position<OtherType>& right)
{
	return Position<Type>{ static_cast<Type>(left.x() + right.x()), static_cast<Type>(left.y() + right.y()), static_cast<Type>(left.z() + right.z()) };
}

template<typename Type, typename OtherType>
inline Position<Type> operator-(const Position<Type>& left, const Position<OtherType>& right)
{
	return Position<Type>{ static_cast<Type>(left.x() - right.x()), static_cast<Type>(left.y() - right.y()), static_cast<Type>(left.z() - right.z()) };
}

template<typename Type, typename OtherType>
inline Position<Type> operator/(const Position<Type>& left, const Position<OtherType>& right)
{
	return Position<Type>{ static_cast<Type>(left.x() / right.x()), static_cast<Type>(left.y() / right.y()), static_cast<Type>(left.z() / right.z()) };
}

//----------------------------

template<typename Type>
inline Dimension<Type> operator*(const Dimension<Type>& left, int right)
{
	return Dimension<Type>{ static_cast<Type>(left.x() * right), static_cast<Type>(left.y() * right), static_cast<Type>(left.z() * right) };
}

template<typename Type>
inline Dimension<Type> operator+(const Dimension<Type>& left, int right)
{
	return Dimension<Type>{ static_cast<Type>(left.x() + right), static_cast<Type>(left.y() + right), static_cast<Type>(left.z() + right) };
}

template<typename Type>
inline Dimension<Type> operator-(const Dimension<Type>& left, int right)
{
	return Dimension<Type>{ static_cast<Type>(left.x() - right), static_cast<Type>(left.y() - right), static_cast<Type>(left.z() - right) };
}

template<typename Type>
inline Dimension<Type> operator/(const Dimension<Type>& left, int right)
{
	return Dimension<Type>{ static_cast<Type>(left.x() / right), static_cast<Type>(left.y() / right), static_cast<Type>(left.z() / right) };
}

template<typename Type, typename OtherType>
inline Dimension<Type> operator*(const Dimension<Type>& left, const Dimension<Type>& right)
{
	return Dimension<Type>{ static_cast<Type>(left.x() * right.x()), static_cast<Type>(left.y() * right.y()), static_cast<Type>(left.z() * right.z()) };
}

template<typename Type, typename OtherType>
inline Dimension<Type> operator+(const Dimension<Type>& left, const Dimension<Type>& right)
{
	return Dimension<Type>{ static_cast<Type>(left.x() + right.x()), static_cast<Type>(left.y() + right.y()), static_cast<Type>(left.z() + right.z()) };
}

template<typename Type, typename OtherType>
inline Dimension<Type> operator-(const Dimension<Type>& left, const Dimension<Type>& right)
{
	return Dimension<Type>{ static_cast<Type>(left.x() - right.x()), static_cast<Type>(left.y() - right.y()), static_cast<Type>(left.z() - right.z()) };
}

template<typename Type, typename OtherType>
inline Dimension<Type> operator/(const Dimension<Type>& left, const Dimension<Type>& right)
{
	return Dimension<Type>{ static_cast<Type>(left.x() / right.x()), static_cast<Type>(left.y() / right.y()), static_cast<Type>(left.z() / right.z()) };
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // EXTENTS_H
