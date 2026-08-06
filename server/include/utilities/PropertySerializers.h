#ifndef PROPSCONTAINER_H
#define PROPSCONTAINER_H

#include <algorithm>
#include <array>
#include <bitset>
#include <concepts>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <CString.h>

#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/std/inplace_vector.h>

namespace preagonal
{
class Player;
}

////////////////////////////////////////////////////////////////////////////////
namespace preagonal::props
{
////////////////////////////////////////////////////////////////////////////////

using GBYTE1 = uint8_t;
using GBYTE2 = uint16_t;
using GBYTE3 = uint32_t;
using GBYTE5 = long long int;
using GBYTE1_signed = int8_t;
using GBYTE2_signed = int16_t;
using GBYTE3_signed = int32_t;

//////////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////////

/// @brief Helper function to avoid #include'ing Server.h in this file.
/// @return ServerGeneration in integer format.
int getServerGeneration();

//////////////////////////////////////////////////
// SetBy and SetResults
//////////////////////////////////////////////////

/// @brief Used to control how properties are forwarded to clients.
///
/// A property that is set by the server is generally forwarded to the client, while a prop that is set by the client is not sent back.
enum class SetBy
{
	CLIENT,
	SERVER
};

/// @brief Contains the results of setting a property.
struct SetResults
{
	using ResultFlagType = std::bitset<5>;

	/// @brief The ID of the property that was set.
	uint8_t propId = 0;

	/// @brief The additional props to send back out as the result of setting this prop.
	std::inplace_vector<uint8_t, 10> resultPropIds{};

	/// @brief The results of the prop set.
	ResultFlagType resultFlags{};

	/// @brief Result Flag - Pass the prop changes to everybody.
	static constexpr size_t sendToAll = 0;

	/// @brief Result Flag - Pass the prop changes to the level.
	static constexpr size_t sendToLevel = 1;

	/// @brief Result Flag - Pass the prop changes back to the source.
	static constexpr size_t sendToSource = 2;

	/// @brief Result Flag - If this prop is being sent, a fresh copy should be acquired.
	static constexpr size_t getLatestOnSend = 3;

	/// @brief Result Flag - If true, the prop was invalid, so we should stop processing more props.
	static constexpr size_t wasInvalid = 4;
};

//////////////////////////////////////////////////
// Applying Limits
//////////////////////////////////////////////////

/// @brief Apply length or value limits to a property value.
struct Limits
{
	static constexpr uint8_t HeadImageLength = 123;
	static constexpr uint8_t BodyImageLength = 223;
	static constexpr uint8_t SwordImageLength = 223;
	static constexpr uint8_t ShieldImageLength = 223;
	static constexpr uint8_t HorseImageLength = 119;
	static constexpr uint8_t GaniLength = 223;
	static constexpr uint8_t ChatMessageLength = 223;
	static constexpr uint8_t MaxHitpoints = 20;
	static constexpr uint8_t MaxArrows = 99;
	static constexpr uint8_t MaxBombs = 99;
	static constexpr uint8_t MaxMP = 100;
	static constexpr uint8_t MaxAP = 100;

	/// @brief Sword, battleaxe, lizardsword, goldensword.
	static constexpr uint8_t MaxSwordPower = 20;

	/// @brief Shield, mirrorshield, lizardshield.
	static constexpr uint8_t MaxShieldPower = 3;

	/// @brief None?, ?, glove1, glove2.
	static constexpr uint8_t MaxGlovePower = 3;

	/// @brief Bomb, joltbomb, superbomb.
	static constexpr uint8_t MaxBombPower = 3;

	/// @brief Bow, fireball, fireblast, nukeshot.
	static constexpr uint8_t MaxBowPower = 4;

	/// @brief Applies a limit to a value, clamping it between min and max.
	/// @param value The input value to limit.
	/// @param min The lower bounds of the limit.
	/// @param max The upper bounds of the limit.
	/// @return The clamped value.
	static auto apply(std::integral auto value, std::integral auto min, std::integral auto max) -> decltype(value)
	{
		using T = decltype(value);
		return std::clamp(value, static_cast<T>(min), static_cast<T>(max));
	}

	/// @brief Clamps an integral value to the range [0, max].
	/// @param value The integral value to clamp.
	/// @param max The upper bound for the value.
	/// @return The value clamped to the range between 0 and max (inclusive).
	static auto apply(std::integral auto value, std::integral auto max) -> decltype(value)
	{
		using T = decltype(value);
		return std::clamp(value, static_cast<T>(0), static_cast<T>(max));
	}

	/// @brief Truncates a string view to a specified maximum length.
	/// @param value The input string view to be truncated if necessary.
	/// @param maxLength The maximum allowed length for the returned string view.
	/// @return A string view containing at most maxLength characters from the input.
	static auto apply(const std::string_view value, const size_t maxLength)
	{
		if (value.length() > maxLength)
			return value.substr(0, maxLength);
		return value;
	}

	/// @brief Applies the maximum hitpoints value (as determined by the server options) and returns the result.
	/// @param maxHitpoints The maximum hitpoints value to apply.
	/// @return The applied maximum hitpoints value, clamped to the maximum allowed.
	static uint8_t applyMaxHitpoints(uint8_t maxHitpoints);

	/// @brief Applies the maximum sword power value (as determined by the server options) and returns the result.
	/// @param swordPower The sword power value to apply.
	/// @return The applied sword power value, clamped to the maximum allowed.
	static int8_t applySwordPower(int8_t swordPower);

	/// @brief Applies the maximum shield power value (as determined by the server options) and returns the result.
	/// @param shieldPower The shield power value to apply.
	/// @return The applied shield power value, clamped to the maximum allowed.
	static uint8_t applyShieldPower(uint8_t shieldPower);
};

//////////////////////////////////////////////////
// Property Containers
//////////////////////////////////////////////////

struct PropertyBase
{
	virtual ~PropertyBase() = default;
	[[nodiscard]] virtual CString serialize() const = 0;
	virtual void deserialize(CString& data) = 0;
	virtual void apply(const GameValue& gameValue) = 0;
	virtual std::format_context::iterator format(std::format_context& ctx) const = 0;
};

/// @brief A property that does not hold data.
struct PropertyVoid : public PropertyBase
{
	PropertyVoid() = default;

	[[nodiscard]] CString serialize() const override
	{
		return CString{}; // No data to serialize.
	}

	void deserialize(CString& data) override
	{
		// No data to read, so do nothing.
	}

	void apply(const GameValue& gameValue) override
	{
		// No data to apply, so do nothing.
	}

	std::format_context::iterator format(std::format_context& ctx) const override
	{
		return std::format_to(ctx.out(), "(void)");
	}
};

struct PropertyUnsafeByte : public PropertyBase
{
	explicit PropertyUnsafeByte(const uint8_t value = 0) : value(value) {}

	[[nodiscard]] CString serialize() const override
	{
		CString result;
		result.writeGCharUnsafe(value);
		return result;
	}

	void deserialize(CString& data) override
	{
		data.readGInto(value);
	}

	void apply(const GameValue& gameValue) override
	{
		value = static_cast<uint8_t>(gameValue.getCopy<double>().value_or(0.0));
	}

	std::format_context::iterator format(std::format_context& ctx) const override
	{
		return std::format_to(ctx.out(), "value: {}", value);
	}

	uint8_t value;
};

/// @brief A property that is encoded as a packed numeric value.
/// @tparam T The type of the numeric value, which must be an integral type that CString can easily serialize (1, 2, 3, or 5 bytes).
template<std::integral T>
struct PropertyNumeric : public PropertyBase
{
	explicit PropertyNumeric(T value = T{}) : value(value) {}

	[[nodiscard]] CString serialize() const override
	{
		return CString() >> static_cast<T>(value);
	}

	void deserialize(CString& data) override
	{
		data.readGInto(value);
	}

	void apply(const GameValue& gameValue) override
	{
		value = static_cast<T>(gameValue.getCopy<double>().value_or(0.0));
	}

	std::format_context::iterator format(std::format_context& ctx) const override
	{
		return std::format_to(ctx.out(), "value: {}", value);
	}

	T value;
};

/// @brief A property that is encoded as a packed string value (length-prefixed).
struct PropertyString : public PropertyBase
{
	PropertyString() = default;
	explicit PropertyString(const char* value) : value(value) {}
	explicit PropertyString(const std::string_view value) : value(value) {}
	explicit PropertyString(const std::string& value) : value(value) {}
	explicit PropertyString(std::string&& value) noexcept : value(std::move(value)) {}

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
	void apply(const GameValue& gameValue) override;
	std::format_context::iterator format(std::format_context& ctx) const override;

	std::string value;
};

/// @brief A property that is encoded as a packed string value (length-prefixed) where the length is 2 bytes.
struct PropertyLongString : public PropertyString
{
	using PropertyString::PropertyString;

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
};

/// @brief A property that combines sword power and sword image.
struct PropertySwordPower : public PropertyBase
{
	PropertySwordPower() = default;
	explicit PropertySwordPower(int8_t power) : power(power) {}
	explicit PropertySwordPower(const std::string& image) : image(image) {}
	explicit PropertySwordPower(const std::string& image, int8_t power) : image(image), power(power) {}
	explicit PropertySwordPower(std::string&& image) noexcept : image(std::move(image)) {}
	explicit PropertySwordPower(std::string&& image, int8_t power) noexcept : image(std::move(image)), power(power) {}

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
	void apply(const GameValue& gameValue) override;
	std::format_context::iterator format(std::format_context& ctx) const override;

	std::string image;
	std::optional<int8_t> power;
};

/// @brief A property that combines shield power and shield image.
struct PropertyShieldPower : public PropertyBase
{
	PropertyShieldPower() = default;
	explicit PropertyShieldPower(uint8_t power) : power(power) {}
	explicit PropertyShieldPower(const std::string& image) : image(image) {}
	explicit PropertyShieldPower(const std::string& image, uint8_t power) : image(image), power(power) {}
	explicit PropertyShieldPower(std::string&& image) noexcept : image(std::move(image)) {}
	explicit PropertyShieldPower(std::string&& image, uint8_t power) noexcept : image(std::move(image)), power(power) {}

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
	void apply(const GameValue& gameValue) override;
	std::format_context::iterator format(std::format_context& ctx) const override;

	std::string image;
	std::optional<uint8_t> power;
};

/// @brief A property that handles the BOWGIF / GANI properties, which changed with the 2.x clients.
struct PropertyGaniOrBowGif : public PropertyBase
{
	PropertyGaniOrBowGif() = default;
	explicit PropertyGaniOrBowGif(std::string_view gani) : gani(gani) {}
	explicit PropertyGaniOrBowGif(uint8_t bowPower)
		: bowGif(std::make_pair(std::string{}, bowPower)) {}
	explicit PropertyGaniOrBowGif(uint8_t bowPower, std::string_view bowGif)
		: bowGif(std::make_pair(std::string{ bowGif }, bowPower)) {}
	explicit PropertyGaniOrBowGif(std::string_view gani, uint8_t bowPower, std::string_view bowGif)
		: gani(gani), bowGif(std::make_pair(std::string{ bowGif }, bowPower)) {}

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
	void apply(const GameValue& gameValue) override;
	std::format_context::iterator format(std::format_context& ctx) const override;

	std::optional<std::string> gani;
	std::optional<std::pair<std::string, uint8_t>> bowGif;
};

/// @brief A property that handles the HEADGIF property.
struct PropertyHeadGif : public PropertyBase
{
	PropertyHeadGif() = default;
	explicit PropertyHeadGif(uint8_t preset) : image(preset) {}
	explicit PropertyHeadGif(const std::string& image) : image(image) {}
	explicit PropertyHeadGif(std::string&& image) noexcept : image(std::move(image)) {}

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
	void apply(const GameValue& gameValue) override;
	std::format_context::iterator format(std::format_context& ctx) const override;

	std::variant<uint8_t, std::string> image;
};

/// @brief A property that handles multiple sequential values of the same type.
///
/// If `stopIfFirstIsZero` is true, the deserialization will stop early if the first value is zero. This handles the EFFECTCOLORS property.
/// @tparam T The type of the values in the array.
/// @tparam N The number of values in the array.
/// @tparam StopIfFirstZero If true, the deserialization will stop early if the first value is zero.
template<typename T, size_t N, bool StopIfFirstZero = false>
struct PropertyArray : public PropertyBase
{
	using ValueType = T;

	PropertyArray() = default;
	explicit PropertyArray(const std::array<T, N>& input) : values(input) {}
	explicit PropertyArray(std::array<T, N>&& input) noexcept : values(std::move(input)) {}
	explicit PropertyArray(std::ranges::input_range auto&& input) noexcept
	{
		std::ranges::copy(input | std::views::take(N), values.begin());
	}

	[[nodiscard]] CString serialize() const override
	{
		CString result;
		for (size_t i = 0; i < N; ++i)
		{
			result >> static_cast<T>(values[i]);
			if constexpr (StopIfFirstZero)
			{
				// If the first value is zero and we should stop, break early.
				if (i == 0 && values[i] == 0)
					break;
			}
		}
		return result;
	}

	void deserialize(CString& data) override
	{
		for (size_t i = 0; i < N; ++i)
		{
			if (static_cast<size_t>(data.bytesLeft()) < sizeof(T))
				throw std::runtime_error("Not enough data to deserialize PropertyArray.");
			data.readGInto(values[i]);

			if constexpr (StopIfFirstZero)
			{
				// If the first value is zero and we should stop, break early.
				if (i == 0 && values[i] == 0)
					break;
			}
		}
	}

    void apply(const GameValue& gameValue) override
    {
		if (gameValue.get<std::vector<double>>().has_value())
		{
			const auto vec = gameValue.get<std::vector<double>>();
			if (!vec.has_value())
				return;

			// Convert all values to type T and insert into the values array.
			for (size_t i = 0; i < N && i < vec.value().get().size(); ++i)
			{
				if constexpr (std::is_integral_v<T>)
				{
					values[i] = static_cast<T>(vec.value().get()[i]);
				}
				else
				{
					values[i] = T(vec->get()[i]);
				}
			}
		}
    }

	std::format_context::iterator format(std::format_context& ctx) const override
	{
		std::ostringstream out;
		for (size_t i = 0; i < N; ++i)
		{
			out << std::format("{}", values[i]);
			if constexpr (StopIfFirstZero)
			{
				if (i == 0 && values[i] == 0)
					break;
			}

			if (i < N - 1)
				out << ", ";
		}

		return std::format_to(ctx.out(), "values: [{}]", out.str());
	}

	std::array<T, N> values{};
};

/// @brief A property that stores an elo rating and its deviation.
struct PropertyEloRating : public PropertyBase
{
	PropertyEloRating() = default;
	PropertyEloRating(const float rating, const float deviation) : rating(rating), deviation(deviation) {}
	PropertyEloRating(const uint32_t rating, const uint32_t deviation) : rating(static_cast<float>(rating)), deviation(static_cast<float>(deviation)) {}

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
	void apply(const GameValue& gameValue) override;
	std::format_context::iterator format(std::format_context& ctx) const override;

	float rating = 1500.0f;
	float deviation = 350.0f;
};

/// @brief A property that stores an attachment to an NPC.
struct PropertyAttachNPC : public PropertyBase
{
	PropertyAttachNPC() = default;
	explicit PropertyAttachNPC(const NPCID npcId) : npcId(npcId) {}
	PropertyAttachNPC(const NPCID npcId, const uint8_t type) : type(type), npcId(npcId) {}

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
	void apply(const GameValue& gameValue) override;
	std::format_context::iterator format(std::format_context& ctx) const override;

	uint8_t type = 0;
	NPCID npcId = 0;
};

/// @brief A property that stores a pixel position.
struct PropertyPixelCoordinate : public PropertyBase
{
	PropertyPixelCoordinate() = default;
	explicit PropertyPixelCoordinate(const int16_t pixelCoordinate) : pixelCoordinate(pixelCoordinate) {}

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
	void apply(const GameValue& gameValue) override;
	std::format_context::iterator format(std::format_context& ctx) const override;

	int16_t pixelCoordinate = 0;
};

/// @brief A property that serializes a coordinate in the old style.
struct PropertyTileCoordinate : public PropertyBase
{
	PropertyTileCoordinate() = default;
	explicit PropertyTileCoordinate(const int16_t pixelCoordinate) : pixelCoordinate(pixelCoordinate) {}
	explicit PropertyTileCoordinate(const float tileCoordinate) : pixelCoordinate(static_cast<int16_t>(tileCoordinate * 16)) {}

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
	void apply(const GameValue& gameValue) override;
	std::format_context::iterator format(std::format_context& ctx) const override;

	int16_t pixelCoordinate = 0;
};

/// @brief A property that serializes a Z coordinate in the old style (offset of 50).
struct PropertyTileCoordinateZ : public PropertyBase
{
	PropertyTileCoordinateZ() = default;
	explicit PropertyTileCoordinateZ(const int16_t pixelCoordinate) : pixelCoordinate(pixelCoordinate) {}
	explicit PropertyTileCoordinateZ(const float tileCoordinate) : pixelCoordinate(static_cast<int16_t>(tileCoordinate * 16)) {}
	explicit PropertyTileCoordinateZ(const double tileCoordinate) : pixelCoordinate(static_cast<int16_t>(tileCoordinate * 16)) {}

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
	void apply(const GameValue& gameValue) override;
	std::format_context::iterator format(std::format_context& ctx) const override;

	int16_t pixelCoordinate = 0;
};

/// @brief A property that stores an old GS1 script.
struct PropertyGS1Script : public PropertyBase
{
	PropertyGS1Script() = default;
	explicit PropertyGS1Script(const std::string_view script) : script(script) {}
	explicit PropertyGS1Script(const std::string& script) : script(script) {}
	explicit PropertyGS1Script(std::string&& script) noexcept : script(std::move(script)) {}

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
	void apply(const GameValue& gameValue) override;
	std::format_context::iterator format(std::format_context& ctx) const override;

	std::string script;
};

/// @brief A property that stores a hurt direction (dx, dy) for an NPC.
template<int8_t MidPoint = 32, float TileDistance = 9.0f>
struct PropertyHurtDxDy : public PropertyBase
{
	PropertyHurtDxDy() = default;

	/// @brief Displacement from -1.0 to 1.0.
	explicit PropertyHurtDxDy(const float dx, const float dy)
	{
		hurtDX = static_cast<int8_t>(std::clamp(dx, -1.0f, 1.0f) * MidPoint);
		hurtDY = static_cast<int8_t>(std::clamp(dy, -1.0f, 1.0f) * MidPoint);
	}

	/// @brief Displacement from -MidPoint to MidPoint.
	explicit PropertyHurtDxDy(const int8_t dx, const int8_t dy)
	{
		hurtDX = std::clamp(dx, static_cast<int8_t>(-MidPoint), MidPoint);
		hurtDY = std::clamp(dy, static_cast<int8_t>(-MidPoint), MidPoint);
	}

	/// @brief Displacement in tiles from -TileDistance to TileDistance.
	explicit PropertyHurtDxDy(const Position<float>& displacement)
	{
		hurtDX = static_cast<int8_t>((std::clamp(displacement.x(), -TileDistance, TileDistance) / TileDistance) * MidPoint);
		hurtDY = static_cast<int8_t>((std::clamp(displacement.y(), -TileDistance, TileDistance) / TileDistance) * MidPoint);
	}

	/// @brief Displacement in pixel tiles from -TileDistance*16 to TileDistance*16.
	explicit PropertyHurtDxDy(const Position<int16_t>& displacement)
	{
		int16_t pixels = TileDistance * 16;
		hurtDX = (std::clamp(displacement.x(), static_cast<int16_t>(-pixels), pixels) * MidPoint) / pixels;
		hurtDY = (std::clamp(displacement.y(), static_cast<int16_t>(-pixels), pixels) * MidPoint) / pixels;
	}

	[[nodiscard]] CString serialize() const override
	{
		const auto clampedDX = std::clamp(hurtDX, static_cast<int8_t>(-MidPoint), MidPoint);
		const auto clampedDY = std::clamp(hurtDY, static_cast<int8_t>(-MidPoint), MidPoint);

		// The range is from 0 - 2*MidPoint, with MidPoint being the center.
		// So a value of MidPoint is 0, a value of 0 is -MidPoint, and a value of 2*MidPoint is +MidPoint.

		return CString() >> (char)(clampedDX + MidPoint) >> (char)(clampedDY + MidPoint);
	}

	void deserialize(CString& data) override
	{
		const int8_t dx = data.readGChar();
		const int8_t dy = data.readGChar();

		// Recenter the values around 0.
		hurtDX = static_cast<int8_t>(dx - MidPoint);
		hurtDY = static_cast<int8_t>(dy - MidPoint);
	}

	void apply(const GameValue& gameValue) override
	{
		const auto array = gameValue.get<std::vector<double>>();
		if (!array.has_value() || array.value().get().size() != 2)
		{
			hurtDX = 0;
			hurtDY = 0;
			return;
		}

		auto& values = array.value().get();
		const float dx = std::clamp(static_cast<float>(values[0]), -1.0f, 1.0f);
		const float dy = std::clamp(static_cast<float>(values[1]), -1.0f, 1.0f);
		hurtDX = static_cast<int8_t>(dx * MidPoint);
		hurtDY = static_cast<int8_t>(dy * MidPoint);
	}

	std::format_context::iterator format(std::format_context& ctx) const override
	{
		auto [dx, dy] = getAsTiles();
		return std::format_to(ctx.out(), "dx: {:.2f}, dy: {:.2f}", dx, dy);
	}

	[[nodiscard]] std::pair<float, float> getAsTiles() const
	{
		std::pair<float, float> result;
		result.first = std::clamp(TileDistance * (hurtDX / static_cast<float>(MidPoint)), -TileDistance, TileDistance);
		result.second = std::clamp(TileDistance * (hurtDY / static_cast<float>(MidPoint)), -TileDistance, TileDistance);
		return result;
	}

	static int8_t midpoint() noexcept { return MidPoint; }
	static float tileDistance() noexcept { return TileDistance; }

	int8_t hurtDX = 0;
	int8_t hurtDY = 0;
};

/// @brief A property that stores a rectangle for an image part.
struct PropertyImagePart : public PropertyBase
{
	PropertyImagePart() = default;
	PropertyImagePart(uint16_t x, uint16_t y, uint8_t width, uint8_t height)
		: imagePart({ x, y }, { width, height }) {}
	explicit PropertyImagePart(const ImagePartRectangle& imagePart) : imagePart(imagePart) {}
	explicit PropertyImagePart(ImagePartRectangle&& imagePart) noexcept : imagePart(std::move(imagePart)) {}

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
	void apply(const GameValue& gameValue) override;
	std::format_context::iterator format(std::format_context& ctx) const override;

	ImagePartRectangle imagePart;
};

/// @brief A property that stores a sprite and its direction.
struct PropertySprite : public PropertyBase
{
	PropertySprite() = default;
	explicit PropertySprite(const uint8_t sprite);
	PropertySprite(const uint8_t sprite, const uint8_t direction) : sprite(sprite), direction(direction) {}

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
	void apply(const GameValue& gameValue) override;
	std::format_context::iterator format(std::format_context& ctx) const override;

	uint8_t sprite = 0;
	uint8_t direction = 2;
};

/// @brief A property that stores an array of colors.
struct PropertyColors : public PropertyArray<GBYTE1, 8>
{
	PropertyColors() = default;
	explicit PropertyColors(const std::array<GBYTE1, 8>& input) : PropertyArray(input) {}
	explicit PropertyColors(std::array<GBYTE1, 8>&& input) noexcept : PropertyArray(input) {}
	explicit PropertyColors(std::ranges::input_range auto&& input) noexcept : PropertyArray(std::forward<decltype(input)>(input)) {}

	[[nodiscard]] CString serialize() const override;
	void deserialize(CString& data) override;
	void apply(const GameValue& gameValue) override;
	std::format_context::iterator format(std::format_context& ctx) const override;

	static int getColorCount() noexcept;
	static size_t getMaxColorValue() noexcept;
};

// Renames these properties so they can be used inside the X-macro.
using PropertyEffectColors = PropertyArray<GBYTE1, 5, true>;

//////////////////////////////////////////////////
// Concepts
//////////////////////////////////////////////////

template<typename T>
concept PropertyContainer = requires(T t, CString& c)
{
	std::is_base_of_v<PropertyBase, T>;
	{ t.serialize() } -> std::convertible_to<CString>;
	t.deserialize(c);
};

//////////////////////////////////////////////////
// Sending Results
//////////////////////////////////////////////////

using PropertySendResults = std::vector<std::pair<SetResults, std::shared_ptr<PropertyBase>>>;
using PropertyContainerGetter = std::function<std::shared_ptr<PropertyBase>(uint8_t, SetResults::ResultFlagType&)>;

void collectPacketsFromResults(const PropertySendResults& results, CString& outAll, CString& outLevel, CString& outSource, const PropertyContainerGetter& getProp);

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::props

//////////////////////////////////////////////////
// Printing
//////////////////////////////////////////////////

template <>
struct std::formatter<preagonal::props::PropertyBase> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyBase* prop, std::format_context& ctx) { return prop->format(ctx); }
};

template <>
struct std::formatter<std::shared_ptr<preagonal::props::PropertyBase>> : std::formatter<std::string>
{
	static auto format(const std::shared_ptr<preagonal::props::PropertyBase>& prop, std::format_context& ctx) { return prop->format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertyVoid> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyVoid& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <typename T>
struct std::formatter<preagonal::props::PropertyNumeric<T>> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyNumeric<T>& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertyString> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyString& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertyLongString> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyLongString& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertySwordPower> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertySwordPower& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertyShieldPower> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyShieldPower& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertyGaniOrBowGif> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyGaniOrBowGif& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertyHeadGif> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyHeadGif& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <typename T, size_t N, bool StopIfFirstZero>
struct std::formatter<preagonal::props::PropertyArray<T, N, StopIfFirstZero>> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyArray<T, N, StopIfFirstZero>& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertyEloRating> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyEloRating& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertyAttachNPC> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyAttachNPC& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertyPixelCoordinate> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyPixelCoordinate& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertyTileCoordinate> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyTileCoordinate& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertyTileCoordinateZ> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyTileCoordinateZ& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertyGS1Script> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyGS1Script& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <uint8_t MidPoint>
struct std::formatter<preagonal::props::PropertyHurtDxDy<MidPoint>> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyHurtDxDy<MidPoint>& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertyImagePart> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertyImagePart& prop, std::format_context& ctx) { return prop.format(ctx); }
};

template <>
struct std::formatter<preagonal::props::PropertySprite> : std::formatter<std::string>
{
	static auto format(const preagonal::props::PropertySprite& prop, std::format_context& ctx) { return prop.format(ctx); }
};

#endif // PROPSCONTAINER_H
