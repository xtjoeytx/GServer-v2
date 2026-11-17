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

//----------------------------------------------------------
//---[ Tags ]-----------------------------------------------

// clang-format off
struct with_positions_t { explicit with_positions_t() = default; };
inline constexpr with_positions_t with_positions{};

struct in_pixels { explicit in_pixels() = default; };
struct in_tiles { explicit in_tiles() = default; };
struct in_levels { explicit in_levels() = default; };
struct in_local_pixels { explicit in_local_pixels() = default; };
struct in_local_tiles { explicit in_local_tiles() = default; };
// clang-format on

//----------------------------------------------------------
//---[ Concepts ]-------------------------------------------

template<typename T>
concept PixelBasedPosition = std::same_as<T, in_pixels> || std::same_as<T, in_local_pixels>;

template<typename T>
concept TileBasedPosition = std::same_as<T, in_tiles> || std::same_as<T, in_local_tiles>;

//----------------------------------------------------------
//---[ Helpers ]--------------------------------------------

/// @brief Adds an epsilon value to a floating point number to help round to the nearest whole number.
static constexpr float fixEpsilon(const float val)
{
	if (val < 0)
		return val - std::numeric_limits<float>::epsilon();
	return val + std::numeric_limits<float>::epsilon();
}

//----------------------------------------------------------
//---[ Position ]-------------------------------------------

/// Represents a position in 3D space.
/// @tparam Unit The unit type of this position.
/// @tparam T The data type that holds the position.
template<typename Unit, typename T>
struct Position
{
	using UnitType = Unit;
	using ValueType = T;

	/// Gets a vector that points towards a direction.
	/// @param direction The player direction (0-3).
	/// @return A normalized vector that points towards the direction.
	static constexpr Position<Unit, T> playerDirectionVector(const uint8_t direction)
	{
		switch (direction)
		{
			case 0:
				return Position<Unit, T>{static_cast<T>(0), static_cast<T>(-1)};
			case 1:
				return Position<Unit, T>{static_cast<T>(-1), static_cast<T>(0)};
			case 2:
				return Position<Unit, T>{static_cast<T>(0), static_cast<T>(1)};
			case 3:
				return Position<Unit, T>{static_cast<T>(1), static_cast<T>(0)};
			default:
				return Position<Unit, T>{0, 0};
		}
	}

	constexpr Position() : data{T{}, T{}, T{}} {}
	constexpr Position(T x, T y) : data{x, y, T{}} {}
	constexpr Position(T x, T y, T z) : data{x, y, z} {}
	constexpr Position(const Position<Unit, T>& other) = default;

	template<typename O>
	// ReSharper disable once CppNonExplicitConvertingConstructor
	// NOLINTNEXTLINE(*-explicit-constructor)
	constexpr Position(const Position<Unit, O>& other) : data{T{other.data[0]}, T{other.data[1]}, T{other.data[2]}}
	{}

	constexpr bool operator==(const Position<Unit, T>& other) const
	{
		return data == other.data;
	}
	constexpr bool operator!=(const Position<Unit, T>& other) const
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
	[[nodiscard]] constexpr const T& x() const { return data[0]; }
	[[nodiscard]] constexpr const T& y() const { return data[1]; }
	[[nodiscard]] constexpr const T& z() const { return data[2]; }

	constexpr Position<Unit, T>& translate(T dx, T dy)
	{
		data[0] += dx;
		data[1] += dy;
		return *this;
	}
	constexpr Position<Unit, T>& translate(T dx, T dy, T dz)
	{
		data[0] += dx;
		data[1] += dy;
		data[2] += dz;
		return *this;
	}

	template<typename O>
	constexpr Position<Unit, T>& translate(const Position<Unit, O>& delta)
	{
		data[0] += static_cast<T>(delta.data[0]);
		data[1] += static_cast<T>(delta.data[1]);
		data[2] += static_cast<T>(delta.data[2]);
		return *this;
	}

	[[nodiscard]] constexpr Position<Unit, T> translate(T dx, T dy) const
	{
		Position<Unit, T> result{*this};
		result.translate(dx, dy);
		return result;
	}
	[[nodiscard]] constexpr Position<Unit, T> translate(T dx, T dy, T dz) const
	{
		Position<Unit, T> result{*this};
		result.translate(dx, dy, dz);
		return result;
	}

	template<typename O>
	constexpr Position<Unit, T> translate(const Position<Unit, O>& delta) const
	{
		Position<Unit, T> result{*this};
		result.translate(delta);
		return result;
	}

	[[nodiscard]] constexpr T length2D() const
	{
		return static_cast<T>(std::sqrt(data[0] * data[0] + data[1] * data[1]));
	}

	[[nodiscard]] constexpr T length3D() const
	{
		return static_cast<T>(std::sqrt(data[0] * data[0] + data[1] * data[1] + data[2] * data[2]));
	}

	template<typename O>
	constexpr Position<Unit, T>& normalize2D(O length)
	{
		if (length == O{})
			throw std::invalid_argument("Cannot normalize a position with zero length.");
		data[0] = static_cast<T>(data[0] / length);
		data[1] = static_cast<T>(data[1] / length);
		return *this;
	}

	template<typename O>
	constexpr Position<Unit, T> normalize2D(O length) const
	{
		if (length == O{})
			throw std::invalid_argument("Cannot normalize a position with zero length.");
		Position<Unit, T> result{*this};
		result.data[0] = static_cast<T>(result.data[0] / length);
		result.data[1] = static_cast<T>(result.data[1] / length);
		return result;
	}

	template<typename O>
	constexpr Position<Unit, T>& normalize3D(O length)
	{
		if (length == O{})
			throw std::invalid_argument("Cannot normalize a position with zero length.");
		data[0] = static_cast<T>(data[0] / length);
		data[1] = static_cast<T>(data[1] / length);
		data[2] = static_cast<T>(data[2] / length);
		return *this;
	}

	template<typename O>
	constexpr Position<Unit, T> normalize3D(O length) const
	{
		if (length == O{})
			throw std::invalid_argument("Cannot normalize a position with zero length.");
		Position<Unit, T> result{*this};
		result.data[0] = static_cast<T>(result.data[0] / length);
		result.data[1] = static_cast<T>(result.data[1] / length);
		result.data[2] = static_cast<T>(result.data[2] / length);
		return result;
	}

	std::array<T, 3> data;
};

using PixelPosition = Position<in_pixels, int32_t>;
using TilePosition = Position<in_tiles, float>;
using WholeTilePosition = Position<in_tiles, int16_t>;

using LocalPixelPosition = Position<in_local_pixels, int16_t>;
using LocalTilePosition = Position<in_local_tiles, float>;
using LocalWholeTilePosition = Position<in_local_tiles, int16_t>;

using MapPosition = Position<in_levels, int16_t>;

//----------------------------------------------------------
//---[ MovingPosition ]-------------------------------------

template<typename Unit, typename T>
struct MovingPosition
{
	Position<Unit, T> position{};
	Position<Unit, T> velocity{};
};

//----------------------------------------------------------
//---[ Dimension ]------------------------------------------

/// Represents a dimension in 3D space.
/// @tparam Unit The unit type of this dimension.
/// @tparam T The data type that holds the dimension.
template<typename Unit, typename T>
struct Dimension
{
	using UnitType = Unit;
	using ValueType = T;

	// clang-format off
	constexpr Dimension() requires PixelBasedPosition<T> : data{T{}, T{}, T{48}} {}
	constexpr Dimension() requires TileBasedPosition<T> : data{T{}, T{}, T{3}} {}
	constexpr Dimension() requires (!PixelBasedPosition<T> && !TileBasedPosition<T>) : data{T{}, T{}, T{0}} {}
	constexpr Dimension(T width, T height) requires PixelBasedPosition<T> : data{width, height, T{48}} {}
	constexpr Dimension(T width, T height) requires TileBasedPosition<T> : data{width, height, T{3}} {}
	constexpr Dimension(T width, T height) requires (!PixelBasedPosition<T> && !TileBasedPosition<T>) : data{width, height, T{0}} {}
	constexpr Dimension(T width, T height, T length) : data{width, height, length} {}
	constexpr Dimension(const Dimension<Unit, T>& other) = default;
	// clang-format on

	template<typename O>
	// ReSharper disable once CppNonExplicitConvertingConstructor
	// NOLINTNEXTLINE(*-explicit-constructor)
	constexpr Dimension(const Dimension<Unit, O>& other) : data{T{other.data[0]}, T{other.data[1]}, T{other.data[2]}}
	{}

	constexpr bool operator==(const Dimension<Unit, T>& other) const
	{
		return data == other.data;
	}
	constexpr bool operator!=(const Dimension<Unit, T>& other) const
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
	[[nodiscard]] constexpr const T& width() const { return data[0]; }
	[[nodiscard]] constexpr const T& height() const { return data[1]; }
	[[nodiscard]] constexpr const T& length() const { return data[2]; }

	std::array<T, 3> data;
};

using PixelDimension = Dimension<in_pixels, uint32_t>;
using TileDimension = Dimension<in_tiles, float>;
using WholeTileDimension = Dimension<in_tiles, uint16_t>;

using LocalPixelDimension = Dimension<in_local_pixels, uint32_t>;
using LocalTileDimension = Dimension<in_local_tiles, float>;
using LocalWholeTileDimension = Dimension<in_local_tiles, uint16_t>;

using MapDimension = Dimension<in_levels, uint8_t>;

//----------------------------------------------------------
//---[ Rectangle (area) ]-----------------------------------

/// Represents a rectangle in 3D space.
/// @tparam Unit The unit type of this rectangle.
/// @tparam P The data type that holds the position of the rectangle.
/// @tparam S The data type that holds the size of the rectangle.
template<typename Unit, typename P, typename S>
struct Rectangle
{
	using UnitType = Unit;
	using PositionValueType = P;
	using DimensionValueType = S;

	constexpr Rectangle() = default;
	constexpr Rectangle(Position<Unit, P> position, Dimension<Unit, S> size) : position(position), size(size) {}
	constexpr Rectangle(with_positions_t, Position<Unit, P> corner1, Position<Unit, P> corner2)
		: position(Position<Unit, P>{std::min(corner1.x(), corner2.x()), std::min(corner1.y(), corner2.y()), std::min(corner1.z(), corner2.z())}),
		  size(Dimension<Unit, S>{std::abs(corner2.x() - corner1.x()), std::abs(corner2.y() - corner1.y()), std::abs(corner2.z() - corner1.z())})
	{}

	[[nodiscard]] constexpr P left() const noexcept { return position.x(); }
	[[nodiscard]] constexpr P right() const noexcept { return position.x() + size.width(); }
	[[nodiscard]] constexpr P top() const noexcept { return position.y(); }
	[[nodiscard]] constexpr P bottom() const noexcept { return position.y() + size.height(); }
	[[nodiscard]] constexpr P ground() const noexcept { return position.z(); }
	[[nodiscard]] constexpr P sky() const noexcept { return position.z() + size.length(); }
	[[nodiscard]] constexpr Position<Unit, P> center() const noexcept { return {position.x() + static_cast<P>(size.width() / static_cast<S>(2)), position.y() + static_cast<P>(size.height() / static_cast<S>(2))}; }
	[[nodiscard]] constexpr Position<Unit, P> leftcenter() const noexcept { return {left(), center().y()}; }
	[[nodiscard]] constexpr Position<Unit, P> rightcenter() const noexcept { return {right(), center().y()}; }
	[[nodiscard]] constexpr Position<Unit, P> topcenter() const noexcept { return {center().x(), top()}; }
	[[nodiscard]] constexpr Position<Unit, P> bottomcenter() const noexcept { return {center().x(), bottom()}; }

	Position<Unit, P> position{};
	Dimension<Unit, S> size{};
};

using PixelRectangleArea = Rectangle<in_pixels, PixelPosition::ValueType, PixelDimension::ValueType>;
using TileRectangleArea = Rectangle<in_tiles, TilePosition::ValueType, TileDimension::ValueType>;
using WholeTileRectangleArea = Rectangle<in_tiles, WholeTilePosition::ValueType, WholeTileDimension::ValueType>;

using LocalPixelRectangleArea = Rectangle<in_local_pixels, LocalPixelPosition::ValueType, LocalPixelDimension::ValueType>;
using LocalTileRectangleArea = Rectangle<in_local_tiles, LocalTilePosition::ValueType, LocalTileDimension::ValueType>;
using LocalWholeTileRectangleArea = Rectangle<in_local_tiles, LocalWholeTilePosition::ValueType, LocalWholeTileDimension::ValueType>;

using ImagePartRectangle = Rectangle<in_pixels, uint16_t, uint8_t>;

//----------------------------------------------------------
//---[ Intersections ]--------------------------------------

/// @brief Checks if a position is contained within a rectangle.
/// @param pos The position that must be contained within a rectangle (on the border counts).
/// @param rect The rectangle that contains the point.
/// @return True if the point is contained within the rectangle, or on the border.
template<typename Unit, typename Pos, typename RectPos, typename RectDim>
inline constexpr bool positionInRectangle(const Position<Unit, Pos>& pos, const Rectangle<Unit, RectPos, RectDim>& rect)
{
	// clang-format off
	return pos.x() >= rect.left() && pos.x() <= rect.right()
		&& pos.y() >= rect.top() && pos.y() <= rect.bottom()
		&& pos.z() >= rect.ground() && pos.z() <= rect.sky();
	// clang-format on
}

/// @brief Checks if two rectangles touch or overlap each other.
/// @param first The first rectangle to check.
/// @param second The second rectangle to check.
/// @return True if the two rectangles touch or overlap each other.
template<typename Unit, typename RectPosL, typename RectDimL, typename RectPosR, typename RectDimR>
inline constexpr bool rectanglesIntersect(const Rectangle<Unit, RectPosL, RectDimL>& first, const Rectangle<Unit, RectPosR, RectDimR>& second)
{
	// clang-format off
	return (first.right() < second.left()
		|| second.right() < first.left()
		|| first.bottom() < second.top()
		|| second.bottom() < first.top()
		|| first.sky() < second.ground()
		|| second.sky() < first.ground()
		) == false;
	// clang-format on
}

/// @brief Checks if the child rectangle is fully contained within the parent.
/// @param child The child rectangle that must be enclosed within the parent rectangle.
/// @param parent The parent rectangle that fully contains the child.
/// @return True if the child rectangle is fully contained within the parent rectangle.
template<typename Unit, typename RectPosL, typename RectDimL, typename RectPosR, typename RectDimR>
inline constexpr bool rectangleContained(const Rectangle<Unit, RectPosL, RectDimL>& child, const Rectangle<Unit, RectPosR, RectDimR>& parent)
{
	// clang-format off
	return (child.left() >= parent.left()
		&& child.right() <= parent.right()
		&& child.top() >= parent.top()
		&& child.bottom() <= parent.bottom()
		&& child.ground() >= parent.ground()
		&& child.sky() <= parent.sky());
	// clang-format on
}

//----------------------------------------------------------
//---[ Translations ]---------------------------------------

/// Translates a position.
/// @param position The position to translate.
/// @param x The X direction to translate the position.
/// @param y The Y direction to translate the position.
/// @return A new translated position.
template<typename Unit, typename T, typename O>
inline constexpr Position<Unit, T> translatePosition(const Position<Unit, T>& position, O x, O y)
{
	return position.translate(static_cast<T>(x), static_cast<T>(y));
}

/// Translates a position.
/// @param position The position to translate.
/// @param x The X direction to translate the position.
/// @param y The Y direction to translate the position.
/// @param z The Z direction to translate the position.
/// @return A new translated position.
template<typename Unit, typename T, typename O>
inline constexpr Position<Unit, T> translatePosition(const Position<Unit, T>& position, O x, O y, O z)
{
	return position.translate(static_cast<T>(x), static_cast<T>(y), static_cast<T>(z));
}

/// Translates a position.
/// @param position The position to translate.
/// @param delta The delta used to translate the position.
/// @return A new translated position.
template<typename Unit, typename T, typename O>
inline constexpr Position<Unit, T> translatePosition(const Position<Unit, T>& position, const Position<Unit, O>& delta)
{
	return position.translate(delta);
}

//----------------------------------------------------------
//---[ Conversions ]----------------------------------------

/// Converts a position to a position based in pixels.
/// @tparam T The data type of the new pixel-based position.
/// @param position The position to convert.
/// @return A converted position.
template<typename T, typename Unit, typename O> requires (!std::same_as<Unit, in_local_pixels> && !std::same_as<Unit, in_local_tiles>)
inline constexpr Position<in_pixels, T> toPixelBasedPosition(const Position<Unit, O>& position)
{
	using DestPosition = Position<in_pixels, T>;
	static_assert(!std::same_as<Unit, in_local_pixels> && !std::same_as<Unit, in_local_tiles>, "To convert from local positions, use toPixelBasedPosition(origin, ...)");

	if constexpr (std::same_as<Unit, in_tiles>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestPosition{static_cast<T>(fixEpsilon(position.x()) * 16), static_cast<T>(fixEpsilon(position.y()) * 16), static_cast<T>(fixEpsilon(position.z()) * 16)};
		else return DestPosition{static_cast<T>(position.x() * 16), static_cast<T>(position.y() * 16), static_cast<T>(position.z() * 16)};
	}
	else if constexpr (std::same_as<Unit, in_levels>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestPosition{static_cast<T>(fixEpsilon(position.x()) * 1024), static_cast<T>(fixEpsilon(position.y()) * 1024), static_cast<T>(fixEpsilon(position.z()) * 1024)};
		else return DestPosition{static_cast<T>(position.x() * 1024), static_cast<T>(position.y() * 1024), static_cast<T>(position.z() * 1024)};
	}
	else if constexpr (std::same_as<Unit, in_pixels>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestPosition{static_cast<T>(fixEpsilon(position.x())), static_cast<T>(fixEpsilon(position.y())), static_cast<T>(fixEpsilon(position.z()))};
	}
	return DestPosition{static_cast<T>(position.x()), static_cast<T>(position.y()), static_cast<T>(position.z())};
}

/// Converts a local position to a global position based in pixels.
/// @tparam T The data type of the new pixel-based position.
/// @param origin The global origin to base the local coordinates on.
/// @param position The position to convert.
/// @return A converted position.
template<typename T, typename Unit, typename O> requires (std::same_as<Unit, in_local_pixels> || std::same_as<Unit, in_local_tiles>)
inline constexpr Position<in_pixels, T> toPixelBasedPosition(const Position<in_pixels, T>& origin, const Position<Unit, O>& position)
{
	if constexpr (std::same_as<Unit, in_local_pixels>)
	{
		Position<in_pixels, O> ret{position.x(), position.y(), position.z()};
		return origin.translate(ret.x(), ret.y(), ret.z());
	}
	if constexpr (std::same_as<Unit, in_local_tiles>)
	{
		Position<in_tiles, O> ret{position.x(), position.y(), position.z()};
		return origin.translate(toPixelBasedPosition<T>(ret));
	}
	return {};
}

/// Converts a position to a position based in tiles.
/// @tparam T The data type of the new tile-based position.
/// @param position The position to convert.
/// @return A converted position.
template<typename T, typename Unit, typename O> requires (!std::same_as<Unit, in_local_pixels> && !std::same_as<Unit, in_local_tiles>)
inline constexpr Position<in_tiles, T> toTileBasedPosition(const Position<Unit, O>& position)
{
	using DestPosition = Position<in_tiles, T>;
	static_assert(!std::same_as<Unit, in_local_pixels> && !std::same_as<Unit, in_local_tiles>, "To convert from local positions, use toTileBasedPosition(origin, ...)");

	if constexpr (std::same_as<Unit, in_levels>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestPosition{static_cast<T>(fixEpsilon(position.x()) * 64), static_cast<T>(fixEpsilon(position.y()) * 64), static_cast<T>(fixEpsilon(position.z()) * 64)};
		else return DestPosition{static_cast<T>(position.x() * 64), static_cast<T>(position.y() * 64), static_cast<T>(position.z() * 64)};
	}
	else if constexpr (std::same_as<Unit, in_pixels>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestPosition{static_cast<T>(fixEpsilon(position.x())) / 16, static_cast<T>(fixEpsilon(position.y())) / 16, static_cast<T>(fixEpsilon(position.z())) / 16};
		else return DestPosition{static_cast<T>(position.x()) / 16, static_cast<T>(position.y()) / 16, static_cast<T>(position.z()) / 16};
	}
	else if constexpr (std::same_as<Unit, in_tiles>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestPosition{static_cast<T>(fixEpsilon(position.x())), static_cast<T>(fixEpsilon(position.y())), static_cast<T>(fixEpsilon(position.z()))};
	}
	return DestPosition{static_cast<T>(position.x()), static_cast<T>(position.y()), static_cast<T>(position.z())};
}

/// Converts a local position to a global position based in tiles.
/// @tparam T The data type of the new tile-based position.
/// @param origin The global origin to base the local coordinates on.
/// @param position The position to convert.
/// @return A converted position.
template<typename T, typename Unit, typename O> requires (std::same_as<Unit, in_local_pixels> || std::same_as<Unit, in_local_tiles>)
inline constexpr Position<in_tiles, T> toTileBasedPosition(const Position<in_tiles, T>& origin, const Position<Unit, O>& position)
{
	if constexpr (std::same_as<Unit, in_local_pixels>)
	{
		Position<in_pixels, O> ret{position.x(), position.y(), position.z()};
		return origin.translate(toTileBasedPosition<T>(ret));
	}
	if constexpr (std::same_as<Unit, in_local_tiles>)
	{
		Position<in_tiles, O> ret{position.x(), position.y(), position.z()};
		return origin.translate(ret.x(), ret.y(), ret.z());
	}
	return {};
}

/// Converts a position to a position based on level positions in a map.
/// @tparam T The data type of the new level-based position.
/// @param position The position to convert.
/// @return A converted position.
template<typename T, typename Unit, typename O>
inline constexpr Position<in_levels, T> toMapBasedPosition(const Position<Unit, O>& position)
{
	using DestPosition = Position<in_levels, T>;
	if constexpr (std::same_as<Unit, in_pixels>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestPosition{static_cast<T>(fixEpsilon(position.x())) / 1024, static_cast<T>(fixEpsilon(position.y())) / 1024, static_cast<T>(fixEpsilon(position.z())) / 1024};
		else return DestPosition{static_cast<T>(position.x()) / 1024, static_cast<T>(position.y()) / 1024, static_cast<T>(position.z()) / 1024};
	}
	else if constexpr (std::same_as<Unit, in_tiles>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestPosition{static_cast<T>(fixEpsilon(position.x())) / 64, static_cast<T>(fixEpsilon(position.y())) / 64, static_cast<T>(fixEpsilon(position.z())) / 64};
		return DestPosition{static_cast<T>(position.x()) / 64, static_cast<T>(position.y()) / 64, static_cast<T>(position.z()) / 64};
	}
	else if constexpr (std::same_as<Unit, in_levels>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestPosition{static_cast<T>(fixEpsilon(position.x())), static_cast<T>(fixEpsilon(position.y())), static_cast<T>(fixEpsilon(position.z()))};
	}
	return DestPosition{static_cast<T>(position.x()), static_cast<T>(position.y()), static_cast<T>(position.z())};
}

/// Converts a global position to a local position based in pixels.
/// @tparam T The data type of the new pixel-based position.
/// @param position The position to convert.
/// @return A converted position.
template<typename T, typename Unit, typename O>
inline constexpr Position<in_local_pixels, T> toLocalPixelBasedPosition(const Position<Unit, O>& position)
{
	using DestPosition = Position<in_local_pixels, T>;
	static_assert(!std::same_as<Unit, in_levels>, "Cannot convert map coordinates to local pixels.");

	// Same units, just cast.
	if constexpr (std::same_as<Unit, in_local_pixels>)
		return DestPosition{static_cast<T>(position.x()), static_cast<T>(position.y()), static_cast<T>(position.z())};

	Position<in_pixels, T> ret;
	if constexpr (std::same_as<Unit, in_local_tiles>)
		ret = toPixelBasedPosition<T>(Position<in_pixels, T>{0, 0}, position);
	else ret = toPixelBasedPosition<T>(position);

	if constexpr (std::floating_point<T>)
	{
		ret.x() = std::fmod(ret.x(), 1024.0f);
		ret.y() = std::fmod(ret.y(), 1024.0f);
	}
	else
	{
		ret.x() = ret.x() % 1024;
		ret.y() = ret.y() % 1024;
	}
	return DestPosition{ret.x(), ret.y(), ret.z()};
}

/// Converts a global position to a local position based in tiles.
/// @tparam T The data type of the new tile-based position.
/// @param position The position to convert.
/// @return A converted position.
template<typename T, typename Unit, typename O>
inline constexpr Position<in_local_tiles, T> toLocalTileBasedPosition(const Position<Unit, O>& position)
{
	using DestPosition = Position<in_local_tiles, T>;
	static_assert(!std::same_as<Unit, in_levels>, "Cannot convert map coordinates to local tiles.");

	// Same units, just cast.
	if constexpr (std::same_as<Unit, in_local_tiles>)
	{
		return DestPosition{static_cast<T>(position.x()), static_cast<T>(position.y()), static_cast<T>(position.z())};
	}
	else if constexpr (!std::same_as<Unit, in_local_tiles>)
	{
		Position<in_tiles, T> ret;
		if constexpr (std::same_as<Unit, in_local_pixels>)
			ret = toTileBasedPosition<T>(Position<in_tiles, T>{0, 0}, position);
		else ret = toTileBasedPosition<T>(position);

		if constexpr (std::floating_point<T>)
		{
			ret.x() = std::fmod(ret.x(), 64.0f);
			ret.y() = std::fmod(ret.y(), 64.0f);
		}
		else
		{
			ret.x() = ret.x() % 64;
			ret.y() = ret.y() % 64;
		}
		return DestPosition{ret.x(), ret.y(), ret.z()};
	}
	return DestPosition{static_cast<T>(position.x()), static_cast<T>(position.y()), static_cast<T>(position.z())};
}

// ------

/// Converts a position to a PixelPosition.
/// @param position The position to convert.
/// @return A converted position.
template<typename Unit, typename O>
inline constexpr PixelPosition toPixelPosition(const Position<Unit, O>& position)
{
	return toPixelBasedPosition<PixelPosition::ValueType>(position);
}

/// Converts a local position to a PixelPosition.
/// @param origin The global origin to base the local coordinates on.
/// @param position The position to convert.
/// @return A converted position.
template<typename Unit, typename O>
inline constexpr PixelPosition toPixelPosition(const PixelPosition& origin, const Position<Unit, O>& position)
{
	return toPixelBasedPosition<PixelPosition::ValueType>(origin, position);
}

/// Converts a position to a LocalPixelPosition.
/// @param position The position to convert.
/// @return A converted position.
template<typename Unit, typename O>
inline constexpr LocalPixelPosition toLocalPixelPosition(const Position<Unit, O>& position)
{
	return toLocalPixelBasedPosition<LocalPixelPosition::ValueType>(position);
}

/// Converts a tile position to a PixelPosition, limiting to half-tile increments.
/// @param x The X tile position to convert.
/// @param y The Y tile position to convert.
/// @return A converted position.
inline constexpr PixelPosition toPixelPositionWithHalfTilePrecision(std::floating_point auto x, std::floating_point auto y)
{
	// Enforce half tile increments.  We will never have a float position that isn't a half tile.
	const auto halfTileX = static_cast<PixelPosition::ValueType>(fixEpsilon(x) * 2);
	const auto halfTileY = static_cast<PixelPosition::ValueType>(fixEpsilon(y) * 2);
	return PixelPosition{static_cast<PixelPosition::ValueType>(halfTileX * 8), static_cast<PixelPosition::ValueType>(halfTileY * 8), static_cast<PixelPosition::ValueType>(0)};
}

/// Converts a TilePosition to a PixelPosition, limiting to half-tile increments.
/// @param position The position to convert.
/// @return A converted position.
inline constexpr PixelPosition toPixelPositionWithHalfTilePrecision(const TilePosition& position)
{
	return PixelPosition{static_cast<PixelPosition::ValueType>(fixEpsilon(position.x()) * 2) * 8, static_cast<PixelPosition::ValueType>(fixEpsilon(position.y()) * 2) * 8, static_cast<PixelPosition::ValueType>(fixEpsilon(position.z()) * 16)};
}

/// Converts a tile position to a LocalPixelPosition.
/// @param x The X tile position to convert.
/// @param y The Y tile position to convert.
/// @return A converted position.
inline constexpr LocalPixelPosition toLocalPixelPositionWithHalfTilePrecision(std::floating_point auto x, std::floating_point auto y)
{
	// Enforce half tile increments.  We will never have a float position that isn't a half tile.
	const auto halfTileX = static_cast<LocalPixelPosition::ValueType>(fixEpsilon(x) * 2);
	const auto halfTileY = static_cast<LocalPixelPosition::ValueType>(fixEpsilon(y) * 2);
	return LocalPixelPosition{static_cast<LocalPixelPosition::ValueType>((halfTileX * 8) % 1024), static_cast<LocalPixelPosition::ValueType>((halfTileY * 8) % 1024)};
}

// ------

/// Converts a position to a TilePosition.
/// @param position The position to convert.
/// @return A converted position.
template<typename Unit, typename O>
inline constexpr TilePosition toTilePosition(const Position<Unit, O>& position)
{
	return toTileBasedPosition<TilePosition::ValueType>(position);
}

/// Converts a local position to a TilePosition.
/// @param origin The global origin to base the local coordinates on.
/// @param position The position to convert.
/// @return A converted position.
template<typename Unit, typename O>
inline constexpr TilePosition toTilePosition(const TilePosition& origin, const Position<Unit, O>& position)
{
	return toTileBasedPosition<TilePosition::ValueType>(origin, position);
}

/// Converts a position to a WholeTilePosition.
/// @param position The position to convert.
/// @return A converted position.
template<typename Unit, typename O>
inline constexpr WholeTilePosition toWholeTilePosition(const Position<Unit, O>& position)
{
	return toTileBasedPosition<WholeTilePosition::ValueType>(position);
}

/// Converts a position to a LocalTilePosition.
/// @param position The position to convert.
/// @return A converted position.
template<typename Unit, typename O>
inline constexpr LocalTilePosition toLocalTilePosition(const Position<Unit, O>& position)
{
	return toLocalTileBasedPosition<LocalTilePosition::ValueType, Unit, O>(position);
}

/// Converts a position to a LocalWholeTilePosition.
/// @param position The position to convert.
/// @return A converted position.
template<typename Unit, typename O>
inline constexpr LocalWholeTilePosition toLocalWholeTilePosition(const Position<Unit, O>& position)
{
	return toLocalTileBasedPosition<LocalWholeTilePosition::ValueType, Unit, O>(position);
}

// ------

/// Converts a position to a MapPosition.
/// @param position The position to convert.
/// @return A converted position.
template<typename Unit, typename O>
inline constexpr MapPosition toMapPosition(const Position<Unit, O>& position)
{
	return toMapBasedPosition<MapPosition::ValueType>(position);
}

// ------

/// Converts a dimension to a dimension based in pixels.
/// @tparam T The data type of the new pixel-based dimension.
/// @param dimension The dimension to convert.
/// @return A converted dimension.
template<typename T, typename Unit, typename O>
inline constexpr Dimension<in_pixels, T> toPixelBasedDimension(const Dimension<Unit, O>& dimension)
{
	using DestDimension = Dimension<in_pixels, T>;
	if constexpr (std::same_as<Unit, in_tiles> || std::same_as<Unit, in_local_tiles>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestDimension{static_cast<T>(fixEpsilon(dimension.width()) * 16), static_cast<T>(fixEpsilon(dimension.height()) * 16), static_cast<T>(fixEpsilon(dimension.length()) * 16)};
		else return DestDimension{static_cast<T>(dimension.width() * 16), static_cast<T>(dimension.height() * 16), static_cast<T>(dimension.length() * 16)};
	}
	else if constexpr (std::same_as<Unit, in_levels>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestDimension{static_cast<T>(fixEpsilon(dimension.width()) * 1024), static_cast<T>(fixEpsilon(dimension.height()) * 1024), static_cast<T>(fixEpsilon(dimension.length()) * 1024)};
		else return DestDimension{static_cast<T>(dimension.width() * 1024), static_cast<T>(dimension.height() * 1024), static_cast<T>(dimension.length() * 1024)};
	}
	else if constexpr (std::same_as<Unit, in_pixels> || std::same_as<Unit, in_local_pixels>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestDimension{static_cast<T>(fixEpsilon(dimension.width())), static_cast<T>(fixEpsilon(dimension.height())), static_cast<T>(fixEpsilon(dimension.length()))};
	}
	return DestDimension{static_cast<T>(dimension.width()), static_cast<T>(dimension.height()), static_cast<T>(dimension.length())};
}

/// Converts a dimension to a dimension based in tiles.
/// @tparam T The data type of the new tile-based dimension.
/// @param dimension The dimension to convert.
/// @return A converted dimension.
template<typename T, typename Unit, typename O>
inline constexpr Dimension<in_tiles, T> toTileBasedDimension(const Dimension<Unit, O>& dimension)
{
	using DestDimension = Dimension<in_tiles, T>;
	if constexpr (std::same_as<Unit, in_levels>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestDimension{static_cast<T>(fixEpsilon(dimension.width()) * 64), static_cast<T>(fixEpsilon(dimension.height()) * 64), static_cast<T>(fixEpsilon(dimension.length()) * 64)};
		else return DestDimension{static_cast<T>(dimension.width() * 64), static_cast<T>(dimension.height() * 64), static_cast<T>(dimension.length() * 64)};
	}
	else if constexpr (std::same_as<Unit, in_pixels>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestDimension{static_cast<T>(fixEpsilon(dimension.width())) / 16, static_cast<T>(fixEpsilon(dimension.height())) / 16, static_cast<T>(fixEpsilon(dimension.length())) / 16};
		else return DestDimension{static_cast<T>(dimension.width()) / 16, static_cast<T>(dimension.height()) / 16, static_cast<T>(dimension.length()) / 16};
	}
	else if constexpr (std::same_as<Unit, in_tiles>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestDimension{static_cast<T>(fixEpsilon(dimension.width())), static_cast<T>(fixEpsilon(dimension.height())), static_cast<T>(fixEpsilon(dimension.length()))};
	}
	return DestDimension{static_cast<T>(dimension.width()), static_cast<T>(dimension.height()), static_cast<T>(dimension.length())};
}

/// Converts a dimension to a dimension based on level position in a map.
/// @tparam T The data type of the new level-based dimension.
/// @param dimension The dimension to convert.
/// @return A converted dimension.
template<typename T, typename Unit, typename O>
inline constexpr Dimension<in_levels, T> toMapBasedDimension(const Dimension<Unit, O>& dimension)
{
	using DestDimension = Dimension<in_levels, T>;
	if constexpr (std::same_as<Unit, in_pixels>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestDimension{static_cast<T>(fixEpsilon(dimension.width() + 1023)) / 1024, static_cast<T>(fixEpsilon(dimension.length() + 1023)) / 1024, static_cast<T>(fixEpsilon(dimension.length() + 1023)) / 1024};
		else return DestDimension{static_cast<T>((dimension.width() + 1023) / 1024), static_cast<T>((dimension.length() + 1023) / 1024), static_cast<T>((dimension.length() + 1023) / 1024)};
	}
	else if constexpr (std::same_as<Unit, in_tiles>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestDimension{static_cast<T>(fixEpsilon(dimension.width() + 63)) / 64, static_cast<T>(fixEpsilon(dimension.length() + 63)) / 64, static_cast<T>(fixEpsilon(dimension.length() + 63)) / 64};
		else return DestDimension{static_cast<T>((dimension.width() + 63)) / 64, static_cast<T>((dimension.length() + 63)) / 64, static_cast<T>((dimension.length() + 63)) / 64};
	}
	else if constexpr (std::same_as<Unit, in_levels>)
	{
		if constexpr (std::floating_point<O> && std::integral<T>)
			return DestDimension{static_cast<T>(fixEpsilon(dimension.width())), static_cast<T>(fixEpsilon(dimension.length())), static_cast<T>(fixEpsilon(dimension.length()))};
	}
	return DestDimension{static_cast<T>(dimension.width()), static_cast<T>(dimension.length()), static_cast<T>(dimension.length())};
}

/// Converts a dimension to a dimension based in local pixels.
/// @tparam T The data type of the new pixel-based dimension.
/// @param dimension The dimension to convert.
/// @return A converted dimension.
template<typename T, typename Unit, typename O>
inline constexpr Dimension<in_local_pixels, T> toLocalPixelBasedDimension(const Dimension<Unit, O>& dimension)
{
	using DestDimension = Dimension<in_local_pixels, T>;
	const auto ret = toPixelBasedDimension<T>(dimension);
	return DestDimension(ret.width(), ret.height(), ret.length());
}

/// Converts a dimension to a dimension based in local tiles.
/// @tparam T The data type of the new tile-based dimension.
/// @param dimension The dimension to convert.
/// @return A converted dimension.
template<typename T, typename Unit, typename O>
inline constexpr Dimension<in_local_tiles, T> toLocalTileBasedDimension(const Dimension<Unit, O>& dimension)
{
	using DestDimension = Dimension<in_local_tiles, T>;
	const auto ret = toTileBasedDimension<T>(dimension);
	return DestDimension(ret.width(), ret.height(), ret.length());
}

// ------

/// Converts a dimension to a PixelDimension.
/// @param dimension The dimension to convert.
/// @return A converted dimension.
template<typename Unit, typename O>
inline constexpr PixelDimension toPixelDimension(const Dimension<Unit, O>& dimension)
{
	return toPixelBasedDimension<PixelDimension::ValueType>(dimension);
}

/// Converts a dimension to a TileDimension.
/// @param dimension The dimension to convert.
/// @return A converted dimension.
template<typename Unit, typename O>
inline constexpr TileDimension toTileDimension(const Dimension<Unit, O>& dimension)
{
	return toTileBasedDimension<TileDimension::ValueType>(dimension);
}

/// Converts a dimension to a LocalPixelDimension.
/// @param dimension The dimension to convert.
/// @return A converted dimension.
template<typename Unit, typename O>
inline constexpr LocalPixelDimension toLocalPixelDimension(const Dimension<Unit, O>& dimension)
{
	return toLocalPixelBasedDimension<LocalPixelDimension::ValueType>(dimension);
}

/// Converts a dimension to a LocalTileDimension.
/// @param dimension The dimension to convert.
/// @return A converted dimension.
template<typename Unit, typename O>
inline constexpr LocalTileDimension toLocalTileDimension(const Dimension<Unit, O>& dimension)
{
	return toLocalTileBasedDimension<LocalTileDimension::ValueType>(dimension);
}

/// Converts a dimension to a MapDimension.
/// @param dimension The dimension to convert.
/// @return A converted dimension.
template<typename Unit, typename O>
inline constexpr MapDimension toMapDimension(const Dimension<Unit, O>& dimension)
{
	return toMapBasedDimension<MapDimension::ValueType>(dimension);
}

// ------

/// Converts a rectangle to a rectangle based in pixels.
/// @tparam SS The data type for the returned rectangle's position.
/// @tparam SD The data type for the returned rectangle's dimension.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename SS, typename SD, typename OUnit, typename OS, typename OD>
inline constexpr Rectangle<in_pixels, SS, SD> toPixelBasedRectangleArea(const Rectangle<OUnit, OS, OD>& rect)
{
	static_assert(!std::same_as<OUnit, in_local_pixels> && !std::same_as<OUnit, in_local_tiles>, "To convert local coordinates, an origin must be specified.");

	using DestRect = Rectangle<in_pixels, SS, SD>;
	const auto position = toPixelBasedPosition<SS>(rect.position);
	const auto dimension = toPixelBasedDimension<SD>(rect.size);
	return DestRect{position, dimension};
}

/// Converts a rectangle to a rectangle based in pixels.
/// @tparam SS The data type for the returned rectangle's position.
/// @tparam SD The data type for the returned rectangle's dimension.
/// @param origin The global origin to base the local coordinates on.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename SS, typename SD, typename OUnit, typename OS, typename OD>
inline constexpr Rectangle<in_pixels, SS, SD> toPixelBasedRectangleArea(const Position<in_pixels, SS>& origin, const Rectangle<OUnit, OS, OD>& rect)
{
	using DestRect = Rectangle<in_pixels, SS, SD>;
	const auto position = toPixelBasedPosition<SS>(origin, rect.position);
	const auto dimension = toPixelBasedDimension<SD>(rect.size);
	return DestRect{position, dimension};
}

/// Converts a rectangle to a rectangle based in tiles.
/// @tparam SS The data type for the returned rectangle's position.
/// @tparam SD The data type for the returned rectangle's dimension.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename SS, typename SD, typename OUnit, typename OS, typename OD>
inline constexpr Rectangle<in_tiles, SS, SD> toTileBasedRectangleArea(const Rectangle<OUnit, OS, OD>& rect)
{
	static_assert(!std::same_as<OUnit, in_local_pixels> && !std::same_as<OUnit, in_local_tiles>, "To convert local coordinates, an origin must be specified.");

	using DestRect = Rectangle<in_tiles, SS, SD>;
	const auto position = toTileBasedPosition<SS>(rect.position);
	const auto dimension = toTileBasedDimension<SD>(rect.size);
	return DestRect{position, dimension};
}

/// Converts a rectangle to a rectangle based in tiles.
/// @tparam SS The data type for the returned rectangle's position.
/// @tparam SD The data type for the returned rectangle's dimension.
/// @param origin The global origin to base the local coordinates on.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename SS, typename SD, typename OUnit, typename OS, typename OD>
inline constexpr Rectangle<in_tiles, SS, SD> toTileBasedRectangleArea(const Position<in_tiles, SS>& origin, const Rectangle<OUnit, OS, OD>& rect)
{
	using DestRect = Rectangle<in_tiles, SS, SD>;
	const auto position = toTileBasedPosition<SS>(origin, rect.position);
	const auto dimension = toTileBasedDimension<SD>(rect.size);
	return DestRect{position, dimension};
}

/// Converts a rectangle to a rectangle based in local pixels.
/// @tparam SS The data type for the returned rectangle's position.
/// @tparam SD The data type for the returned rectangle's dimension.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename SS, typename SD, typename OUnit, typename OS, typename OD>
inline constexpr Rectangle<in_local_pixels, SS, SD> toLocalPixelBasedRectangleArea(const Rectangle<OUnit, OS, OD>& rect)
{
	using DestRect = Rectangle<in_local_pixels, SS, SD>;
	const auto position = toLocalPixelBasedPosition<SS>(rect.position);
	const auto dimension = toLocalPixelBasedDimension<SD>(rect.size);
	return DestRect{position, dimension};
}

/// Converts a rectangle to a rectangle based in local tiles.
/// @tparam SS The data type for the returned rectangle's position.
/// @tparam SD The data type for the returned rectangle's dimension.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename SS, typename SD, typename OUnit, typename OS, typename OD>
inline constexpr Rectangle<in_local_tiles, SS, SD> toLocalTileBasedRectangleArea(const Rectangle<OUnit, OS, OD>& rect)
{
	using DestRect = Rectangle<in_local_tiles, SS, SD>;
	const auto position = toLocalTileBasedPosition<SS>(rect.position);
	const auto dimension = toLocalTileBasedDimension<SD>(rect.size);
	return DestRect{position, dimension};
}

/// Converts a rectangle to a rectangle based on the position of levels in a map.
/// @tparam SS The data type for the returned rectangle's position.
/// @tparam SD The data type for the returned rectangle's dimension.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename SS, typename SD, typename OUnit, typename OS, typename OD>
inline constexpr Rectangle<in_levels, SS, SD> toMapBasedRectangleArea(const Rectangle<OUnit, OS, OD>& rect)
{
	using DestRect = Rectangle<in_levels, SS, SD>;
	const auto position = toMapBasedPosition<SS>(rect.position);
	const auto dimension = toMapBasedDimension<SD>(rect.size);
	return DestRect{position, dimension};
}

// ------

/// Converts a rectangle to a PixelRectangleArea.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename Unit, typename P, typename D>
inline constexpr PixelRectangleArea toPixelRectangleArea(const Rectangle<Unit, P, D>& rect)
{
	return toPixelBasedRectangleArea<PixelRectangleArea::PositionValueType, PixelRectangleArea::DimensionValueType, Unit, P, D>(rect);
}

/// Converts a rectangle to a PixelRectangleArea.
/// @param origin The global origin to base the local coordinates on.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename Unit, typename P, typename D>
inline constexpr PixelRectangleArea toPixelRectangleArea(const PixelPosition& origin, const Rectangle<Unit, P, D>& rect)
{
	return toPixelBasedRectangleArea<PixelRectangleArea::PositionValueType, PixelRectangleArea::DimensionValueType, Unit, P, D>(origin, rect);
}

/// Converts a rectangle to a TileRectangleArea.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename Unit, typename P, typename D>
inline constexpr TileRectangleArea toTileRectangleArea(const Rectangle<Unit, P, D>& rect)
{
	return toTileBasedRectangleArea<TileRectangleArea::PositionValueType, TileRectangleArea::DimensionValueType, Unit, P, D>(rect);
}

/// Converts a rectangle to a TileRectangleArea.
/// @param origin The global origin to base the local coordinates on.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename Unit, typename P, typename D>
inline constexpr TileRectangleArea toTileRectangleArea(const TilePosition& origin, const Rectangle<Unit, P, D>& rect)
{
	return toTileBasedRectangleArea<TileRectangleArea::PositionValueType, TileRectangleArea::DimensionValueType, Unit, P, D>(origin, rect);
}

/// Converts a rectangle to a WholeTileRectangleArea.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename Unit, typename P, typename D>
inline constexpr WholeTileRectangleArea toWholeTileRectangleArea(const Rectangle<Unit, P, D>& rect)
{
	const auto tilePosition = toTilePosition(rect.position);
	const auto tileDimension = toTileDimension(rect.size);
	const auto wholeTilePosition = toWholeTilePosition(rect.position);
	const WholeTileDimension wholeTileDimension {
		static_cast<WholeTileDimension::ValueType>(std::ceilf(tilePosition.x() - wholeTilePosition.x() + tileDimension.width())),
		static_cast<WholeTileDimension::ValueType>(std::ceilf(tilePosition.y() - wholeTilePosition.y() + tileDimension.height()))
	};
	return WholeTileRectangleArea{wholeTilePosition, wholeTileDimension};
}

/// Converts a rectangle to a WholeTileRectangleArea.
/// @param origin The global origin to base the local coordinates on.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename Unit, typename P, typename D>
inline constexpr WholeTileRectangleArea toWholeTileRectangleArea(const WholeTilePosition& origin, const Rectangle<Unit, P, D>& rect)
{
	const auto tilePosition = toTilePosition(toWholeTilePosition(origin), rect.position);
	const auto tileDimension = toTileDimension(rect.size);
	const auto wholeTilePosition = toWholeTilePosition(origin, rect.position);
	const WholeTileDimension wholeTileDimension {
		static_cast<WholeTileDimension::ValueType>(std::ceilf(tilePosition.x() - wholeTilePosition.x() + tileDimension.width())),
		static_cast<WholeTileDimension::ValueType>(std::ceilf(tilePosition.y() - wholeTilePosition.y() + tileDimension.height()))
	};
	return WholeTileRectangleArea{wholeTilePosition, wholeTileDimension};
}

/// Converts a rectangle to a LocalPixelRectangleArea.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename Unit, typename P, typename D>
inline constexpr LocalPixelRectangleArea toLocalPixelRectangleArea(const Rectangle<Unit, P, D>& rect)
{
	return toLocalPixelBasedRectangleArea<LocalPixelRectangleArea::PositionValueType, LocalPixelRectangleArea::DimensionValueType, Unit, P, D>(rect);
}

/// Converts a rectangle to a LocalWholeTileRectangleArea.
/// @param rect The rectangle to convert.
/// @return A converted rectangle.
template<typename Unit, typename P, typename D>
inline constexpr LocalWholeTileRectangleArea toLocalWholeTileRectangleArea(const Rectangle<Unit, P, D>& rect)
{
	return toLocalTileBasedRectangleArea<LocalWholeTileRectangleArea::PositionValueType, LocalWholeTileRectangleArea::DimensionValueType, Unit, P, D>(rect);
}

/// Clips a rectangle to a specific level area.
/// @param origin The map area that the rectangle will be clipped against.
/// @param rect The rectangle to clip.
/// @return A local, clipped rectangle.
template<typename Unit, typename P, typename D>
inline constexpr LocalWholeTileRectangleArea clipLocalWholeTileRectangleArea(const MapPosition& origin, const Rectangle<Unit, P, D>& rect)
{
	// Same coordinates.
	if constexpr (std::same_as<Unit, in_local_tiles> && std::same_as<P, LocalWholeTilePosition::ValueType>)
	{
		const LocalWholeTileDimension size{static_cast<LocalWholeTileDimension::ValueType>(rect.size.width()), static_cast<LocalWholeTileDimension::ValueType>(rect.size.height()), static_cast<LocalWholeTileDimension::ValueType>(rect.size.length())};
		return LocalWholeTileRectangleArea{rect.position, size};
	}

	const auto pixelOrigin = toPixelPosition(origin);
	auto pixelRect = toPixelRectangleArea(rect);

	// If the relative position to the origin is negative, we need to adjust it to be within the local level.
	if (pixelRect.position.x() < pixelOrigin.x())
	{
		pixelRect.size.width() -= (pixelOrigin.x() - pixelRect.position.x());
		pixelRect.position.x() = pixelOrigin.x();
	}
	if (pixelRect.position.y() < pixelOrigin.y())
	{
		pixelRect.size.height() -= (pixelOrigin.y() - pixelRect.position.y());
		pixelRect.position.y() = pixelOrigin.y();
	}

	// If the boundaries are out of the local level, adjust them to fit.
	const auto displacement = pixelRect.position - pixelOrigin;
	constexpr PixelPosition::ValueType maxPixels = 64 * 16;
	if (displacement.x() + pixelRect.size.width() > maxPixels) pixelRect.size.width() = maxPixels - displacement.x();
	if (displacement.y() + pixelRect.size.height() > maxPixels) pixelRect.size.height() = maxPixels - displacement.y();

	return toLocalWholeTileRectangleArea(pixelRect);
}

//----------------------------------------------------------
// ---[ Math ]----------------------------------------------

template<typename Unit, typename T, std::integral OtherType>
inline constexpr Position<Unit, T> operator*(const Position<Unit, T>& left, const OtherType& right)
{
	return Position<Unit, T>{static_cast<T>(left.x() * right), static_cast<T>(left.y() * right), static_cast<T>(left.z() * right)};
}

template<typename Unit, typename T, std::integral OtherType>
inline constexpr Position<Unit, T> operator+(const Position<Unit, T>& left, const OtherType& right)
{
	return Position<Unit, T>{static_cast<T>(left.x() + right), static_cast<T>(left.y() + right), static_cast<T>(left.z() + right)};
}

template<typename Unit, typename T, std::integral OtherType>
inline constexpr Position<Unit, T> operator-(const Position<Unit, T>& left, const OtherType& right)
{
	return Position<Unit, T>{static_cast<T>(left.x() - right), static_cast<T>(left.y() - right), static_cast<T>(left.z() - right)};
}

template<typename Unit, typename T, std::integral OtherType>
inline constexpr Position<Unit, T> operator/(const Position<Unit, T>& left, const OtherType& right)
{
	return Position<Unit, T>{static_cast<T>(left.x() / right), static_cast<T>(left.y() / right), static_cast<T>(left.z() / right)};
}

// ------

template<typename Unit, typename T, std::floating_point OtherType>
inline constexpr Position<Unit, OtherType> operator*(const Position<Unit, T>& left, const OtherType& right)
{
	return Position<Unit, OtherType>{static_cast<OtherType>(left.x() * right), static_cast<OtherType>(left.y() * right), static_cast<OtherType>(left.z() * right)};
}

template<typename Unit, typename T, std::floating_point OtherType>
inline constexpr Position<Unit, OtherType> operator+(const Position<Unit, T>& left, const OtherType& right)
{
	return Position<Unit, OtherType>{static_cast<OtherType>(left.x() + right), static_cast<OtherType>(left.y() + right), static_cast<OtherType>(left.z() + right)};
}

template<typename Unit, typename T, std::floating_point OtherType>
inline constexpr Position<Unit, OtherType> operator-(const Position<Unit, T>& left, const OtherType& right)
{
	return Position<Unit, OtherType>{static_cast<OtherType>(left.x() - right), static_cast<OtherType>(left.y() - right), static_cast<OtherType>(left.z() - right)};
}

template<typename Unit, typename T, std::floating_point OtherType>
inline constexpr Position<Unit, OtherType> operator/(const Position<Unit, T>& left, const OtherType& right)
{
	return Position<Unit, OtherType>{static_cast<OtherType>(left.x() / right), static_cast<OtherType>(left.y() / right), static_cast<OtherType>(left.z() / right)};
}

// ------

template<typename Unit, typename T, typename OtherType>
inline constexpr Position<Unit, T> operator*(const Position<Unit, T>& left, const Position<Unit, OtherType>& right)
{
	return Position<Unit, T>{static_cast<T>(left.x() * right.x()), static_cast<T>(left.y() * right.y()), static_cast<T>(left.z() * right.z())};
}

template<typename Unit, typename T, typename OtherType>
inline constexpr Position<Unit, T> operator+(const Position<Unit, T>& left, const Position<Unit, OtherType>& right)
{
	return Position<Unit, T>{static_cast<T>(left.x() + right.x()), static_cast<T>(left.y() + right.y()), static_cast<T>(left.z() + right.z())};
}

template<typename Unit, typename T, typename OtherType>
inline constexpr Position<Unit, T> operator-(const Position<Unit, T>& left, const Position<Unit, OtherType>& right)
{
	return Position<Unit, T>{static_cast<T>(left.x() - right.x()), static_cast<T>(left.y() - right.y()), static_cast<T>(left.z() - right.z())};
}

template<typename Unit, typename T, typename OtherType>
inline constexpr Position<Unit, T> operator/(const Position<Unit, T>& left, const Position<Unit, OtherType>& right)
{
	return Position<Unit, T>{static_cast<T>(left.x() / right.x()), static_cast<T>(left.y() / right.y()), static_cast<T>(left.z() / right.z())};
}

// ------

template<typename Unit, typename T, std::integral OtherType>
inline constexpr Dimension<Unit, T> operator*(const Dimension<Unit, T>& left, const OtherType& right)
{
	return Dimension<Unit, T>{static_cast<T>(left.width() * right), static_cast<T>(left.height() * right), static_cast<T>(left.length() * right)};
}

template<typename Unit, typename T, std::integral OtherType>
inline constexpr Dimension<Unit, T> operator+(const Dimension<Unit, T>& left, const OtherType& right)
{
	return Dimension<Unit, T>{static_cast<T>(left.width() + right), static_cast<T>(left.height() + right), static_cast<T>(left.length() + right)};
}

template<typename Unit, typename T, std::integral OtherType>
inline constexpr Dimension<Unit, T> operator-(const Dimension<Unit, T>& left, const OtherType& right)
{
	return Dimension<Unit, T>{static_cast<T>(left.width() - right), static_cast<T>(left.height() - right), static_cast<T>(left.length() - right)};
}

template<typename Unit, typename T, std::integral OtherType>
inline constexpr Dimension<Unit, T> operator/(const Dimension<Unit, T>& left, const OtherType& right)
{
	return Dimension<Unit, T>{static_cast<T>(left.width() / right), static_cast<T>(left.height() / right), static_cast<T>(left.length() / right)};
}

// ------

template<typename Unit, typename T, std::floating_point OtherType>
inline constexpr Dimension<Unit, OtherType> operator*(const Dimension<Unit, T>& left, const OtherType& right)
{
	return Dimension<Unit, OtherType>{static_cast<OtherType>(left.width() * right), static_cast<OtherType>(left.height() * right), static_cast<OtherType>(left.length() * right)};
}

template<typename Unit, typename T, std::floating_point OtherType>
inline constexpr Dimension<Unit, OtherType> operator+(const Dimension<Unit, T>& left, const OtherType& right)
{
	return Dimension<Unit, OtherType>{static_cast<OtherType>(left.width() + right), static_cast<OtherType>(left.height() + right), static_cast<OtherType>(left.length() + right)};
}

template<typename Unit, typename T, std::floating_point OtherType>
inline constexpr Dimension<Unit, OtherType> operator-(const Dimension<Unit, T>& left, const OtherType& right)
{
	return Dimension<Unit, OtherType>{static_cast<OtherType>(left.width() - right), static_cast<OtherType>(left.height() - right), static_cast<OtherType>(left.length() - right)};
}

template<typename Unit, typename T, std::floating_point OtherType>
inline constexpr Dimension<Unit, OtherType> operator/(const Dimension<Unit, T>& left, const OtherType& right)
{
	return Dimension<Unit, OtherType>{static_cast<OtherType>(left.width() / right), static_cast<OtherType>(left.height() / right), static_cast<OtherType>(left.length() / right)};
}

// ------

template<typename Unit, typename T, typename OtherType>
inline constexpr Dimension<Unit, T> operator*(const Dimension<Unit, T>& left, const Dimension<Unit, OtherType>& right)
{
	return Dimension<Unit, T>{static_cast<T>(left.width() * right.width()), static_cast<T>(left.height() * right.height()), static_cast<T>(left.length() * right.length())};
}

template<typename Unit, typename T, typename OtherType>
inline constexpr Dimension<Unit, T> operator+(const Dimension<Unit, T>& left, const Dimension<Unit, OtherType>& right)
{
	return Dimension<Unit, T>{static_cast<T>(left.width() + right.width()), static_cast<T>(left.height() + right.height()), static_cast<T>(left.length() + right.length())};
}

template<typename Unit, typename T, typename OtherType>
inline constexpr Dimension<Unit, T> operator-(const Dimension<Unit, T>& left, const Dimension<Unit, OtherType>& right)
{
	return Dimension<Unit, T>{static_cast<T>(left.width() - right.width()), static_cast<T>(left.height() - right.height()), static_cast<T>(left.length() - right.length())};
}

template<typename Unit, typename T, typename OtherType>
inline constexpr Dimension<Unit, T> operator/(const Dimension<Unit, T>& left, const Dimension<Unit, OtherType>& right)
{
	return Dimension<Unit, T>{static_cast<T>(left.width() / right.width()), static_cast<T>(left.height() / right.height()), static_cast<T>(left.length() / right.length())};
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

// Structured bindings support.
namespace std
{
////////////////////////////////////////////////////////////////////////////////

// Position
template<typename Unit, typename T>
struct tuple_size<preagonal::Position<Unit, T>> : public std::integral_constant<size_t, 3>
{
};

template<size_t I, typename Unit, typename T>
struct tuple_element<I, preagonal::Position<Unit, T>>
{
public:
	using type = T;
};

// Dimension
template<typename Unit, typename T>
struct tuple_size<preagonal::Dimension<Unit, T>> : public std::integral_constant<size_t, 3>
{
};

template<size_t I, typename Unit, typename T>
struct tuple_element<I, preagonal::Dimension<Unit, T>>
{
public:
	using type = T;
};

// Rectangle
template<typename Unit, typename P, typename D>
struct tuple_size<preagonal::Rectangle<Unit, P, D>> : public std::integral_constant<size_t, 2>
{
};

template<size_t I, typename Unit, typename P, typename D>
struct tuple_element<I, preagonal::Rectangle<Unit, P, D>> : conditional<I == 0, P, D>
{
	static_assert(I < 2, "Index out of bounds for tuple_element<Rectangle>");
};

////////////////////////////////////////////////////////////////////////////////
} // end namespace std
////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

// clang-format off
template<size_t I, typename Unit, typename T>
constexpr T& get(preagonal::Position<Unit, T>& vec) { return vec.data[I]; }

template<size_t I, typename Unit, typename T>
constexpr const T& get(const preagonal::Position<Unit, T>& vec) { return vec.data[I]; }

template<size_t I, typename Unit, typename T>
constexpr T& get(preagonal::Dimension<Unit, T>& vec) { return vec.data[I]; }

template<size_t I, typename Unit, typename T>
constexpr const T& get(const preagonal::Dimension<Unit, T>& vec) { return vec.data[I]; }
// clang-format on

template<size_t I, typename Unit, typename P, typename D>
constexpr std::tuple_element_t<I, preagonal::Rectangle<Unit, P, D>>& get(preagonal::Rectangle<Unit, P, D>& rect)
{
	static_assert(I < 2, "Index out of bounds for get<Rectangle>");

	if constexpr (I == 0)
		return rect.position;
	else if constexpr (I == 1)
		return rect.size;

	return {};
}

template<size_t I, typename Unit, typename P, typename D>
constexpr std::tuple_element_t<I, preagonal::Rectangle<Unit, P, D>>&& get(const preagonal::Rectangle<Unit, P, D>& rect)
{
	static_assert(I < 2, "Index out of bounds for get<Rectangle>");

	if constexpr (I == 0)
		return rect.position;
	else if constexpr (I == 1)
		return rect.size;

	return {};
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

//////////////////////////////////////////////////
// Printing
//////////////////////////////////////////////////

template<typename Unit>
constexpr std::string_view getUnitString()
{
	if constexpr (std::same_as<Unit, preagonal::in_pixels> || std::same_as<Unit, preagonal::in_local_pixels>)
		return "pixels"sv;
	else if constexpr (std::same_as<Unit, preagonal::in_tiles> || std::same_as<Unit, preagonal::in_local_tiles>)
		return "tiles"sv;
	else if constexpr (std::same_as<Unit, preagonal::in_levels>)
		return "levels"sv;
	return {};
}

template<typename Unit, std::integral T>
struct std::formatter<preagonal::Position<Unit, T>> : std::formatter<std::string>
{
	auto format(const preagonal::Position<Unit, T>& pos, std::format_context& ctx) const
	{
		constexpr std::string_view units = getUnitString<Unit>();
		if (units.empty())
			return std::format_to(ctx.out(), "{}, {}, {}", pos.x(), pos.y(), pos.z());
		return std::format_to(ctx.out(), "[{}] {}, {}, {}", units, pos.x(), pos.y(), pos.z());
	}
};

template<typename Unit, std::floating_point T>
struct std::formatter<preagonal::Position<Unit, T>> : std::formatter<std::string>
{
	auto format(const preagonal::Position<Unit, T>& pos, std::format_context& ctx) const
	{
		constexpr std::string_view units = getUnitString<Unit>();
		if (units.empty())
			return std::format_to(ctx.out(), "{:04.2f}, {:04.2f}, {:04.2f}", pos.x(), pos.y(), pos.z());
		return std::format_to(ctx.out(), "[{}] {:04.2f}, {:04.2f}, {:04.2f}", units, pos.x(), pos.y(), pos.z());
	}
};

template<typename Unit, std::integral T>
struct std::formatter<preagonal::Dimension<Unit, T>> : std::formatter<std::string>
{
	auto format(const preagonal::Dimension<Unit, T>& dim, std::format_context& ctx) const
	{
		constexpr std::string_view units = getUnitString<Unit>();
		if (units.empty())
			return std::format_to(ctx.out(), "{}, {}, {}", dim.width(), dim.height(), dim.length());
		return std::format_to(ctx.out(), "[{}] {}, {}, {}", units, dim.width(), dim.height(), dim.length());
	}
};

template<typename Unit, std::floating_point T>
struct std::formatter<preagonal::Dimension<Unit, T>> : std::formatter<std::string>
{
	auto format(const preagonal::Dimension<Unit, T>& dim, std::format_context& ctx) const
	{
		constexpr std::string_view units = getUnitString<Unit>();
		if (units.empty())
			return std::format_to(ctx.out(), "{:04.2f}, {:04.2f}, {:04.2f}", dim.width(), dim.height(), dim.length());
		return std::format_to(ctx.out(), "[{}] {:04.2f}, {:04.2f}, {:04.2f}", units, dim.width(), dim.height(), dim.length());
	}
};

#endif // EXTENTS_H
