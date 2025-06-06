#ifndef PROPSCONTAINER_H
#define PROPSCONTAINER_H

#include <bitset>
#include <concepts>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <common.h>

#include <CString.h>
#include <player/PlayerProps.h>
#include <utilities/inplace_vector.h>

namespace preagonal
{
class Player;
}

////////////////////////////////////////////////////////////////////////////////
namespace preagonal::props
{
////////////////////////////////////////////////////////////////////////////////

using GBYTE1  = uint8_t;
using GBYTE2 = uint16_t;
using GBYTE3 = uint32_t;
using GBYTE5  = int64_t;
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
	/// @brief The ID of the property that was set.
	uint8_t propId = 0;

	/// @brief The additional props to send back out as the result of setting this prop.
	std::inplace_vector<uint8_t, 3> resultPropIds{};

	/// @brief The results of the prop set.
	std::bitset<4> resultFlags{};

	/// @brief Result Flag - Pass the prop changes to everybody.
	static const size_t sendToAll = 0;

	/// @brief Result Flag - Pass the prop changes to the level.
	static const size_t sendToLevel = 1;

	/// @brief Result Flag - Pass the prop changes to the player.
	static const size_t sendToSelf = 2;

	/// @brief Result Flag - If true, the prop was invalid, so we should stop processing more props.
	static const size_t wasInvalid = 3;
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
	static auto apply(std::string_view value, size_t maxLength)
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
	virtual CString serialize() const = 0;
	virtual void deserialize(CString& data) = 0;
};

/// @brief A property that does not hold data.
struct PropertyVoid : public PropertyBase
{
	PropertyVoid() = default;

	virtual CString serialize() const override
	{
		return CString{}; // No data to serialize.
	}

	virtual void deserialize(CString& data) override
	{
		// No data to read, so do nothing.
	}
};

/// @brief A property that is encoded as a packed numeric value.
/// @tparam T The type of the numeric value, which must be an integral type that CString can easily serialize (1, 2, 3, or 5 bytes).
template<std::integral T>
struct PropertyNumeric : public PropertyBase
{
	PropertyNumeric(T value = T{}) : value(value) {}

	virtual CString serialize() const override
	{
		return CString() >> (T)value;
	}

	virtual void deserialize(CString& data) override
	{
		data.readGInto(value);
	}

	T value;
};

/// @brief A property that is encoded as a packed string value (length-prefixed).
struct PropertyString : public PropertyBase
{
	PropertyString() = default;
	PropertyString(const char* value) : value(value) {}
	PropertyString(std::string_view value) : value(std::string{ value }) {}
	PropertyString(std::string value) : value(std::move(value)) {}
	PropertyString(std::string&& value) : value(std::move(value)) {}

	virtual CString serialize() const override;
	virtual void deserialize(CString& data) override;

	std::string value;
};

/// @brief A property that combines sword power and sword image.
struct PropertySwordPower : public PropertyBase
{
	PropertySwordPower() = default;
	PropertySwordPower(int8_t power) : power(power) {}
	PropertySwordPower(std::string image) : image(std::move(image)) {}
	PropertySwordPower(std::string&& image) : image(std::move(image)) {}
	PropertySwordPower(std::string image, int8_t power) : image(std::move(image)), power(power) {}
	PropertySwordPower(std::string&& image, int8_t power) : image(std::move(image)), power(power) {}

	virtual CString serialize() const override;
	virtual void deserialize(CString& data) override;

	std::string image;
	std::optional<int8_t> power;
};

/// @brief A property that combines shield power and shield image.
struct PropertyShieldPower : public PropertyBase
{
	PropertyShieldPower() = default;
	PropertyShieldPower(uint8_t power) : power(power) {}
	PropertyShieldPower(std::string image) : image(std::move(image)) {}
	PropertyShieldPower(std::string&& image) : image(std::move(image)) {}
	PropertyShieldPower(std::string image, uint8_t power) : image(std::move(image)), power(power) {}
	PropertyShieldPower(std::string&& image, uint8_t power) : image(std::move(image)), power(power) {}

	virtual CString serialize() const override;
	virtual void deserialize(CString& data) override;

	std::string image;
	std::optional<uint8_t> power;
};

/// @brief A property that handles the BOWGIF / GANI properties, which changed with the 2.x clients.
struct PropertyGaniOrBowGif : public PropertyBase
{
	PropertyGaniOrBowGif() = default;
	PropertyGaniOrBowGif(std::string gani) : gani(std::move(gani)) {}
	PropertyGaniOrBowGif(uint8_t bowPower, std::string bowGif)
		: bowGif(std::make_pair(std::move(bowGif), bowPower)) {}
	PropertyGaniOrBowGif(std::string gani, uint8_t bowPower, std::string bowGif)
		: gani(std::move(gani)), bowGif(std::make_pair(std::move(bowGif), bowPower)) {}

	virtual CString serialize() const override;
	virtual void deserialize(CString& data) override;

	std::optional<std::string> gani;
	std::optional<std::pair<std::string, uint8_t>> bowGif;
};

/// @brief A property that handles the HEADGIF property.
struct PropertyHeadGif : public PropertyBase
{
	PropertyHeadGif() = default;
	PropertyHeadGif(uint8_t preset) : image(preset) {}
	PropertyHeadGif(std::string image) : image(std::move(image)) {}

	virtual CString serialize() const override;
	virtual void deserialize(CString& data) override;

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
	PropertyArray() = default;
	PropertyArray(std::array<T, N> values) : values(std::move(values)) {}
	PropertyArray(std::array<T, N>&& values) : values(std::move(values)) {}

	virtual CString serialize() const override
	{
		CString result;
		for (size_t i = 0; i < N; ++i)
		{
			result >> (T)values[i];
			if constexpr (StopIfFirstZero)
			{
				// If the first value is zero and we should stop, break early.
				if (i == 0 && values[i] == 0)
					break;
			}
		}
		return result;
	}

	virtual void deserialize(CString& data) override
	{
		for (size_t i = 0; i < N; ++i)
		{
			if (data.bytesLeft() < sizeof(T))
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

	std::array<T, N> values{};
};

/// @brief A property that stores an elo rating and its deviation.
struct PropertyEloRating : public PropertyBase
{
	PropertyEloRating() = default;
	PropertyEloRating(float rating, float deviation) : rating(rating), deviation(deviation) {}
	PropertyEloRating(uint32_t rating, uint32_t deviation) : rating(rating), deviation(deviation) {}

	virtual CString serialize() const override;
	virtual void deserialize(CString& data) override;

	float rating = 1500.0f;
	float deviation = 350.0f;
};

/// @brief A property that stores an attachment to an NPC.
struct PropertyAttachNPC : public PropertyBase
{
	PropertyAttachNPC() = default;
	PropertyAttachNPC(NPCID npcId) : npcId(npcId) {}
	PropertyAttachNPC(NPCID npcId, uint8_t type) : type(type), npcId(npcId) {}

	virtual CString serialize() const override;
	virtual void deserialize(CString& data) override;

	uint8_t type = 0;
	NPCID npcId = 0;
};

/// @brief A property that stores a pixel position.
struct PropertyPixelCoordinate : public PropertyBase
{
	PropertyPixelCoordinate() = default;
	PropertyPixelCoordinate(int16_t pixelCoordinate) : pixelCoordinate(pixelCoordinate) {}

	virtual CString serialize() const override;
	virtual void deserialize(CString& data) override;

	int16_t pixelCoordinate = 0;
};

/// @brief A property that serializes a coordinate in the old style.
struct PropertyTileCoordinate : public PropertyBase
{
	PropertyTileCoordinate() = default;
	PropertyTileCoordinate(int16_t pixelCoordinate) : pixelCoordinate(pixelCoordinate) {}
	PropertyTileCoordinate(float tileCoordinate) : pixelCoordinate(static_cast<int16_t>(tileCoordinate * 16)) {}

	virtual CString serialize() const override;
	virtual void deserialize(CString& data) override;

	int16_t pixelCoordinate = 0;
};

/// @brief A property that serializes a Z coordinate in the old style (offset of 25).
struct PropertyTileCoordinateZ : public PropertyBase
{
	PropertyTileCoordinateZ() = default;
	PropertyTileCoordinateZ(int16_t pixelCoordinate) : pixelCoordinate(pixelCoordinate) {}
	PropertyTileCoordinateZ(float tileCoordinate) : pixelCoordinate(static_cast<int16_t>(tileCoordinate * 16)) {}

	virtual CString serialize() const override;
	virtual void deserialize(CString& data) override;

	int16_t pixelCoordinate = 0;
};

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

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::props

#endif // PROPSCONTAINER_H
