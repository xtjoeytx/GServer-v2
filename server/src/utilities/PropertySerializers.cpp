#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <BabyDI.h>
#include <CString.h>

#include <Server.h>
#include <object/Character.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/PropertySerializers.h>

////////////////////////////////////////////////////////////////////////////////
namespace preagonal::props
{
////////////////////////////////////////////////////////////////////////////////

int getServerGeneration()
{
	auto server = BabyDI::Get<Server>();
	return static_cast<int>(server->Generation);
}

////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------
// PropertyString

CString PropertyString::serialize() const
{
	return CString() >> (char)value.length() << value;
}

void PropertyString::deserialize(CString& data)
{
	value = data.readChars(data.readGUChar());
}

void PropertyString::apply(const GameValue& gameValue)
{
	value = std::move(gameValue.getCopy<std::string>().value_or(""s));
}

std::format_context::iterator PropertyString::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "value: {}", value.empty() ? "(empty)" : value);
}

// -----------------------------------------------
// PropertyLongString

CString PropertyLongString::serialize() const
{
	return CString() >> (short)value.length() << value;
}

void PropertyLongString::deserialize(CString& data)
{
	value = data.readChars(data.readGUShort());
}

// -----------------------------------------------
// PropertySwordPower

CString PropertySwordPower::serialize() const
{
	auto powerVal = power.value_or(0);
	if (powerVal == 0)
		return CString() >> (char)0;
	if (powerVal > 0 && powerVal <= 4 && image.empty())
		return CString() >> (char)powerVal;
	return CString() >> (char)(powerVal + 30) >> (char)image.length() << image;
}

void PropertySwordPower::deserialize(CString& data)
{
	uint8_t powerVal = 0;
	data.readGInto(powerVal);
	if (powerVal < 30)
	{
		powerVal = Limits::applySwordPower(powerVal);

		// For older clients, we use a default image name.
		if (powerVal > 0 && powerVal <= 4)
		{
			auto server = BabyDI::Get<Server>();
			image = std::format("sword{}.{}", powerVal, (server->Generation != ServerGeneration::CLASSIC ? "png" : "gif"));
		}
		else image.clear();

		power = powerVal;
		return;
	}

	// If the power is 30 or more, its sword power + custom image.
	powerVal = Limits::applySwordPower(powerVal - 30);
	power = powerVal;

	// Read the image name.
	// If there is no extension, assume its a .gif.
	image = data.readChars(data.readGUChar());
	if (!image.contains("."))
		image += ".gif";
}

void PropertySwordPower::apply(const GameValue& gameValue)
{
	image = std::move(gameValue.getCopy<std::string>().value_or(""s));
}

std::format_context::iterator PropertySwordPower::format(std::format_context& ctx) const
{
	if (power.value_or(0) > 0 && power.value_or(0) <= 4 && image.empty())
		return std::format_to(ctx.out(), "image: (preset {0}) sword{0}.{1}, power: {2}", power.value_or(0), (getServerGeneration() == 0 ? "gif" : "png"), power.value_or(0));
	if (power.has_value() && power.value() == 0)
		return std::format_to(ctx.out(), "image: (empty), power: 0");
	return std::format_to(ctx.out(), "image: {}, power: {}", (image.empty() ? "(empty)" : image), power.value_or(0));
}

// -----------------------------------------------
// PropertyShieldPower

CString PropertyShieldPower::serialize() const
{
	auto powerVal = power.value_or(0);
	if (powerVal == 0)
		return CString() >> (char)0;
	if (powerVal > 0 && powerVal <= 3 && image.empty())
		return CString() >> (char)powerVal;
	return CString() >> (char)(powerVal + 10) >> (char)image.length() << image;
}

void PropertyShieldPower::deserialize(CString& data)
{
	auto server = BabyDI::Get<Server>();

	uint8_t powerVal = 0;
	data.readGInto(powerVal);
	if (powerVal < 10)
	{
		powerVal = Limits::applyShieldPower(powerVal);

		// For older clients, we use a default image name.
		if (powerVal > 0 && powerVal <= 4)
			image = std::format("shield{}.{}", powerVal, (server->Generation != ServerGeneration::CLASSIC ? "png" : "gif"));
		else image.clear();

		power = powerVal;
		return;
	}

	// If the power is 10 or more, its shield power + custom image.
	powerVal = Limits::applyShieldPower(powerVal - 10);
	power = powerVal;

	// This fixes an odd bug with the 1.41 client.
	if (data.bytesLeft() == 0) return;

	// Read the image name.
	image = data.readChars(data.readGUChar());

	// If there is no extension, assume its a .gif, for 1.x servers.
	if (server->Generation == ServerGeneration::CLASSIC)
	{
		if (!image.contains("."))
			image += ".gif";
	}
}

void PropertyShieldPower::apply(const GameValue& gameValue)
{
	image = std::move(gameValue.getCopy<std::string>().value_or(""s));
}

std::format_context::iterator PropertyShieldPower::format(std::format_context& ctx) const
{
	if (power.value_or(0) > 0 && power.value_or(0) <= 3 && image.empty())
		return std::format_to(ctx.out(), "image: (preset {0}) shield{0}.{1}, power: {2}", power.value_or(0), (getServerGeneration() == 0 ? "gif" : "png"), power.value_or(0));
	if (power.has_value() && power.value() == 0)
		return std::format_to(ctx.out(), "image: (empty), power: 0");
	return std::format_to(ctx.out(), "image: {}, power: {}", (image.empty() ? "(empty)" : image), power.value_or(0));
}

// -----------------------------------------------
// PropertyGaniOrBowGif

CString PropertyGaniOrBowGif::serialize() const
{
	if (gani.has_value())
		return CString() >> (char)gani->length() << *gani;
	else if (bowGif.has_value())
	{
		auto& [image, preset] = *bowGif;
		if (image.empty() && preset < 10)
			return CString() >> (char)preset;

		return CString() >> (char)(10 + image.length()) << image;
	}
	return CString();
}

void PropertyGaniOrBowGif::deserialize(CString& data)
{
	// Gani for later clients.
	if (getServerGeneration() != 0)
	{
		gani = data.readChars(data.readGUChar());
	}
	// Graal 1.411 and earlier clients used BOWGIF instead of GANI.
	else
	{
		uint8_t preset = data.readGUChar();
		if (preset < 10)
		{
			// If the preset is less than 10, its a bow preset.
			bowGif = std::make_pair(std::string(), preset);
		}
		else
		{
			// Otherwise, its a custom bow image.
			auto image = data.readChars(preset - 10);
			if (!image.isEmpty() && !image.contains("."))
				image += ".gif";
			bowGif = std::make_pair(std::move(image), 0);
		}
	}
}

void PropertyGaniOrBowGif::apply(const GameValue& gameValue)
{
	gani = std::move(gameValue.getCopy<std::string>().value_or(""s));
}

std::format_context::iterator PropertyGaniOrBowGif::format(std::format_context& ctx) const
{
	if (gani.has_value())
		return std::format_to(ctx.out(), "gani: {}", (gani.value().empty() ? "(empty)" : gani.value()));
	else if (bowGif.has_value())
		return std::format_to(ctx.out(), "bowGif: {}, bowPower: {}", (bowGif.value().first.empty() ? "(empty)" : bowGif.value().first), bowGif.value().second);
	else
		return std::format_to(ctx.out(), "(empty)");
}

// -----------------------------------------------
// PropertyHeadGif

CString PropertyHeadGif::serialize() const
{
	if (std::holds_alternative<uint8_t>(image))
	{
		auto preset = std::min(static_cast<uint8_t>(99), std::get<uint8_t>(image));
		return CString() >> (char)preset;
	}

	auto& headImage = std::get<std::string>(image);
	return CString() >> (char)(100 + headImage.length()) << headImage;
}

void PropertyHeadGif::deserialize(CString& data)
{
	auto length = data.readGUChar();
	if (length < 100)
	{
		image = length;
		return;
	}

	auto headImage = data.readChars(length - 100);

	if (getServerGeneration() == 0)
	{
		if (!headImage.contains("."))
			headImage += ".gif";
	}

	image = std::move(headImage.toString());
}

void PropertyHeadGif::apply(const GameValue& gameValue)
{
	image = std::move(gameValue.getCopy<std::string>().value_or(""s));
}

std::format_context::iterator PropertyHeadGif::format(std::format_context& ctx) const
{
	if (std::holds_alternative<uint8_t>(image))
	{
		uint8_t preset = std::get<uint8_t>(image);
		return std::format_to(ctx.out(), "head: (preset {0}) head{0}.{1}", preset, (getServerGeneration() == 0 ? "gif" : "png"));
	}
	else
	{
		auto& head = std::get<std::string>(image);
		return std::format_to(ctx.out(), "head: {}", (head.empty() ? "(empty)" : head));
	}
}

// -----------------------------------------------
// PropertyEloRating

CString PropertyEloRating::serialize() const
{
	auto packed = ((static_cast<uint32_t>(rating) & 0xFFF) << 9) | (static_cast<uint32_t>(deviation) & 0x1FF);
	return CString().writeGInt(packed);
}

void PropertyEloRating::deserialize(CString& data)
{
	uint32_t packed = data.readGInt();
	rating = ((packed >> 9) & 0xFFF);
	deviation = (packed & 0x1FF);
}

void PropertyEloRating::apply(const GameValue& gameValue)
{
	auto array = gameValue.get<std::vector<double>>();
	if (!array.has_value() || array.value().get().size() != 2)
	{
		rating = 0;
		deviation = 0;
		return;
	}

	auto& values = array.value().get();
	rating = static_cast<float>(values[0]);
	deviation = static_cast<float>(values[1]);
}

std::format_context::iterator PropertyEloRating::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "rating: {:.2f}, deviation: {:.2f}", rating, deviation);
}

// -----------------------------------------------
// PropertyAttachNPC

CString PropertyAttachNPC::serialize() const
{
	return CString() >> (char)type >> (int)npcId;
}

void PropertyAttachNPC::deserialize(CString& data)
{
	type = data.readGUChar();
	npcId = data.readGInt();
}

void PropertyAttachNPC::apply(const GameValue& gameValue)
{
	npcId = static_cast<NPCID>(gameValue.getCopy<double>().value_or(0));
}

std::format_context::iterator PropertyAttachNPC::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "type: {}, NPCID: {}", type, static_cast<uint32_t>(npcId));
}

// -----------------------------------------------
// PropertyPixelCoordinate

CString PropertyPixelCoordinate::serialize() const
{
	uint16_t val = (uint16_t)std::abs(pixelCoordinate) << 1;
	if (pixelCoordinate < 0)
		val |= 0x0001;
	return CString() >> (short)val;
}

void PropertyPixelCoordinate::deserialize(CString& data)
{
	auto len = data.readGUShort();
	pixelCoordinate = (len >> 1);

	// If the first bit is 1, our pixelCoordinate is negative.
	if ((uint16_t)len & 0x0001)
		pixelCoordinate = -pixelCoordinate;
}

void PropertyPixelCoordinate::apply(const GameValue& gameValue)
{
	pixelCoordinate = static_cast<int16_t>(gameValue.getCopy<double>().value_or(0) * 16);
}

std::format_context::iterator PropertyPixelCoordinate::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "pixel: {} (tile: {:.2f})", pixelCoordinate, (pixelCoordinate / 16.0f));
}

// -----------------------------------------------
// PropertyTileCoordinate

CString PropertyTileCoordinate::serialize() const
{
	CString result;

	// Writing 223 will break the packet flow (as it will overflow to the newline char), so avoid doing that.
	// 223 will be -11 and 224 will be -10.5.
	uint8_t halftile = static_cast<uint8_t>(pixelCoordinate / 8);
	if (halftile == 223)
		halftile = 224;

	result.writeGCharUnsafe(halftile);
	return result;
}

void PropertyTileCoordinate::deserialize(CString& data)
{
	int16_t halftile = 0;
	uint8_t read = data.readGChar();
	if (read >= 216)
		halftile = static_cast<int8_t>(read);
	else halftile = read;

	pixelCoordinate = static_cast<int16_t>(halftile * 8);
}

void PropertyTileCoordinate::apply(const GameValue& gameValue)
{
	pixelCoordinate = static_cast<int16_t>(gameValue.getCopy<double>().value_or(0) * 16);
}

std::format_context::iterator PropertyTileCoordinate::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "tile: {:.2f} (pixel: {})", (pixelCoordinate / 16.0f), pixelCoordinate);
}

// -----------------------------------------------
// PropertyTileCoordinateZ

CString PropertyTileCoordinateZ::serialize() const
{
	return CString() >> (char)(std::min(170, std::max(-50, (pixelCoordinate / 16))) + 50);
}

void PropertyTileCoordinateZ::deserialize(CString& data)
{
	pixelCoordinate = (data.readGUChar() - 50) * 16;
}

void PropertyTileCoordinateZ::apply(const GameValue& gameValue)
{
	pixelCoordinate = static_cast<int16_t>(gameValue.getCopy<double>().value_or(0) * 16);
}

std::format_context::iterator PropertyTileCoordinateZ::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "tile: {:.2f} (pixel: {})", (pixelCoordinate / 16.0f), pixelCoordinate);
}

// -----------------------------------------------
// PropertyGS1Script

CString PropertyGS1Script::serialize() const
{
	auto server = BabyDI::Get<Server>();

	// Modern sends scripts in a different way.
	if (server->Generation == ServerGeneration::MODERN)
		return CString() >> (short)0;

	return CString() >> (short)(script.length() > 0x705F ? 0x705F : script.length()) << script.substr(0, 0x705F);
}

void PropertyGS1Script::deserialize(CString& data)
{
	auto length = data.readGUShort();
	script = data.readChars(length);
}

void PropertyGS1Script::apply(const GameValue& gameValue)
{
	script = std::move(gameValue.getCopy<std::string>().value_or(""s));
}

std::format_context::iterator PropertyGS1Script::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "script size: {}", script.size());
}

// -----------------------------------------------
// PropertyHurtDxDy

PropertyHurtDxDy::PropertyHurtDxDy(float dx, float dy)
{
	hurtDX = static_cast<int8_t>(std::clamp(dx, -1.0f, 1.0f) * 32);
	hurtDY = static_cast<int8_t>(std::clamp(dy, -1.0f, 1.0f) * 32);
}

PropertyHurtDxDy::PropertyHurtDxDy(int8_t dx, int8_t dy)
{
	hurtDX = std::clamp(dx, static_cast<int8_t>(-32), 32_i8);
	hurtDY = std::clamp(dy, static_cast<int8_t>(-32), 32_i8);
}

PropertyHurtDxDy::PropertyHurtDxDy(const Position<float>& displacement)
{
	hurtDX = static_cast<int8_t>((std::clamp(displacement.x(), -9.0f, 9.0f) / 9.0f) * 32);
	hurtDY = static_cast<int8_t>((std::clamp(displacement.y(), -9.0f, 9.0f) / 9.0f) * 32);
}

PropertyHurtDxDy::PropertyHurtDxDy(const Position<int16_t>& displacement)
{
	hurtDX = (std::clamp(displacement.x(), static_cast<int16_t>(-144), 144_i16) * 32) / 144_i16;
	hurtDY = (std::clamp(displacement.y(), static_cast<int16_t>(-144), 144_i16) * 32) / 144_i16;
}

CString PropertyHurtDxDy::serialize() const
{
	auto clampedDX = std::clamp(hurtDX, static_cast<int8_t>(-32), 32_i8);
	auto clampedDY = std::clamp(hurtDY, static_cast<int8_t>(-32), 32_i8);

	// The range is from 0 - 64 with 32 being the center.
	// So a value of 32 is 0, a value of 0 is -32, and a value of 64 is +32.
	// 32 represents 9 tiles of displacement.

	return CString() >> (char)(clampedDX + 32) >> (char)(clampedDY + 32);
}

void PropertyHurtDxDy::deserialize(CString& data)
{
	int8_t dx = data.readGChar();
	int8_t dy = data.readGChar();

	// Recenter the values around 0.
	hurtDX = static_cast<int8_t>(dx - 32);
	hurtDY = static_cast<int8_t>(dy - 32);
}

void PropertyHurtDxDy::apply(const GameValue& gameValue)
{
	auto array = gameValue.get<std::vector<double>>();
	if (!array.has_value() || array.value().get().size() != 2)
	{
		hurtDX = 0;
		hurtDY = 0;
		return;
	}

	auto& values = array.value().get();
	float dx = std::clamp(static_cast<float>(values[0]), -1.0f, 1.0f);
	float dy = std::clamp(static_cast<float>(values[1]), -1.0f, 1.0f);
	hurtDX = static_cast<int8_t>(dx * 32);
	hurtDY = static_cast<int8_t>(dy * 32);
}

std::format_context::iterator PropertyHurtDxDy::format(std::format_context& ctx) const
{
	auto [dx, dy] = getAsTiles();
	return std::format_to(ctx.out(), "dx: {:.2f}, dy: {:.2f}", dx, dy);
}

std::pair<float, float> PropertyHurtDxDy::getAsTiles() const
{
	std::pair<float, float> result;
	result.first = std::clamp(hurtDX / 32.0f, -1.0f, 1.0f);
	result.second = std::clamp(hurtDY / 32.0f, -1.0f, 1.0f);
	return result;
}

// -----------------------------------------------
// PropertyImagePart

CString PropertyImagePart::serialize() const
{
	return CString() >> (short)imagePart.position.x() >> (short)imagePart.position.y() >> (char)imagePart.size.width() >> (char)imagePart.size.height();
}

void PropertyImagePart::deserialize(CString& data)
{
	uint16_t x = 0, y = 0;
	uint8_t width = 0, height = 0;

	x = data.readGUShort();
	y = data.readGUShort();
	width = data.readGUChar();
	height = data.readGUChar();

	imagePart.position = {std::clamp(x, 0_ui16, 16000_ui16), std::clamp(y, 0_ui16, 16000_ui16)};
	imagePart.size = {std::clamp(width, 0_ui8, 220_ui8), std::clamp(height, 0_ui8, 220_ui8)};
}

void PropertyImagePart::apply(const GameValue& gameValue)
{
	auto array = gameValue.get<std::vector<double>>();
	if (!array.has_value() || array.value().get().size() < 4)
		return;

	auto& values = array.value().get();
	imagePart.position = {static_cast<uint16_t>(values[0]), static_cast<uint16_t>(values[1])};
	imagePart.size = {static_cast<uint8_t>(values[2]), static_cast<uint8_t>(values[3])};
}

std::format_context::iterator PropertyImagePart::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "pos: ({}, {}), size: ({}, {})", imagePart.position.x(), imagePart.position.y(), imagePart.size.width(), imagePart.size.height());
}

// -----------------------------------------------
// PropertySprite

PropertySprite::PropertySprite(uint8_t sprite)
{
	this->sprite = sprite >> 2;
	this->direction = sprite & 0b0000'0011;
}

CString PropertySprite::serialize() const
{
	return CString() >> (char)((sprite << 2) | direction);
}

void PropertySprite::deserialize(CString& data)
{
	uint8_t spriteDir = 0;
	data.readGInto(spriteDir);

	// The first 6 bits are the sprite, the last 2 bits are the direction.
	sprite = spriteDir >> 2;
	direction = spriteDir & 0b0000'0011;
}

void PropertySprite::apply(const GameValue& gameValue)
{
	auto value = static_cast<uint8_t>(gameValue.getCopy<double>().value_or(0.0));
	sprite = value >> 2;
	direction = value & 0b0000'0011;
}

std::format_context::iterator PropertySprite::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "sprite: {}, direction: {}", sprite, direction);
}

// -----------------------------------------------
// PropertyColors

CString PropertyColors::serialize() const
{
	size_t count = getColorCount();
	size_t maxValue = getMaxColorValue();
	CString result;
	for (size_t i = 0; i < count; ++i)
	{
		result >> std::clamp((ValueType)values[i], static_cast<ValueType>(0), static_cast<ValueType>(maxValue));
	}
	return result;
}

void PropertyColors::deserialize(CString& data)
{
	size_t count = getColorCount();
	size_t maxValue = getMaxColorValue();
	for (size_t i = 0; i < count; ++i)
	{
		if (static_cast<size_t>(data.bytesLeft()) < sizeof(ValueType))
			throw std::runtime_error("Not enough data to deserialize PropertyArray.");
		data.readGInto(values[i]);
		values[i] = std::clamp(values[i], static_cast<ValueType>(0), static_cast<ValueType>(maxValue));
	}
}

void PropertyColors::apply(const GameValue& gameValue)
{
	auto value = gameValue.get<std::vector<double>>();
	if (value.has_value())
	{
		auto& vec = value.value().get();

		// Convert all values to type T and insert into the values array.
		size_t count = getColorCount();
		size_t maxValue = getMaxColorValue();
		for (size_t i = 0; i < count && i < vec.size(); ++i)
		{
			values[i] = std::clamp(static_cast<ValueType>(vec[i]), static_cast<ValueType>(0), static_cast<ValueType>(maxValue));
		}
	}
}

std::format_context::iterator PropertyColors::format(std::format_context& ctx) const
{
	std::ostringstream out;
	size_t count = getColorCount();

	for (size_t i = 0; i < count; ++i)
	{
		out << std::format("{}", values[i]);
		if (i < count - 1)
			out << ", ";
	}

	return std::format_to(ctx.out(), "values: [{}]", out.str());
}

int PropertyColors::getColorCount() const noexcept
{
	auto server = BabyDI::Get<Server>();
	return server->isNewWorldMode() ? 8 : 5;
}

size_t PropertyColors::getMaxColorValue() const noexcept
{
	auto server = BabyDI::Get<Server>();
	size_t colorCount = CLASSICCOLORS_COUNT;
	if (server->Generation == ServerGeneration::MODERN && server->cached.enableExBodyColors.getValue())
		colorCount += HTMLCOLORS_COUNT;
	return colorCount;
}

////////////////////////////////////////////////////////////////////////////////

uint8_t Limits::applyMaxHitpoints(uint8_t maxHitpoints)
{
	auto server = BabyDI::Get<Server>();
	auto heartLimit = std::min(server->cached.maxHeartLimit.getValue(), 20_ui8);
	return std::clamp(maxHitpoints, 0_ui8, heartLimit);
}

int8_t Limits::applySwordPower(int8_t swordPower)
{
	auto server = BabyDI::Get<Server>();
	int8_t minimum = (server->cached.enableHealingSwords.getValue() ? -(server->cached.swordPowerLimit.getValue()) : 0);
	int8_t maximum = server->cached.swordPowerLimit.getValue();
	return std::clamp(swordPower, minimum, maximum);
}

uint8_t Limits::applyShieldPower(uint8_t shieldPower)
{
	auto server = BabyDI::Get<Server>();
	return std::clamp(shieldPower, 0_ui8, server->cached.shieldPowerLimit.getValue());
}

////////////////////////////////////////////////////////////////////////////////

void collectPacketsFromResults(const PropertySendResults& results, CString& outAll, CString& outLevel, CString& outSource, PropertyContainerGetter getProp)
{
	// The map allows us to to sort the results by increasing ID order.  If the client receives a prop it doesn't understand, it stops processing them.
	// This ensures that all the props the client CAN read come before the ones it can't.
	// Using a map also allows us to avoid duplicates, as the key is the prop ID.
	static std::map<uint8_t, std::tuple<SetResults, std::shared_ptr<PropertyBase>>, std::less<>> sendOrder;

	// Add all the results to the send order.
	sendOrder.clear();
	for (const auto& [result, prop] : results)
	{
		if (result.resultFlags.test(SetResults::wasInvalid))
			continue;

		sendOrder[result.propId] = std::make_tuple(result, prop);
		for (const auto& additionalPropId : result.resultPropIds)
			sendOrder[additionalPropId] = std::make_tuple(result, nullptr);
	}

	// Loop through all the sorted results and add them to the buffers.
	for (auto& [propId, resultTuple] : sendOrder)
	{
		auto& setResults = std::get<0>(resultTuple);
		std::shared_ptr<PropertyBase> base = std::get<1>(resultTuple);
		if (base == nullptr && !getProp)
			continue;

		// We want to support sending different props for different destinations.
		// As long as we have destinations to send to, we will keep sending the prop.  By default, it will send to every destination.
		// If we have SetResults::getLatestOnSend set, then the callback will return which destinations the prop is for.
		// We can then mark those destinations off and keep looping.

		auto& destinationFlags = setResults.resultFlags;
		SetResults::ResultFlagType sendFlags{setResults.resultFlags};
		size_t loopCount = 0;
		while (destinationFlags.test(SetResults::sendToAll) || destinationFlags.test(SetResults::sendToLevel) || destinationFlags.test(SetResults::sendToSource))
		{
			// If the base prop is null, or if we need to get the latest value on send, execute the callback to get the latest prop value.
			if (base == nullptr || setResults.resultFlags.test(SetResults::getLatestOnSend))
			{
				sendFlags = destinationFlags;
				base = getProp(propId, sendFlags);
			}

			// Send to each destination and mark that it got sent.
			if (sendFlags.test(SetResults::sendToAll))
			{
				outAll >> (char)propId << base->serialize();
				destinationFlags.reset(SetResults::sendToAll);
			}
			if (sendFlags.test(SetResults::sendToLevel))
			{
				outLevel >> (char)propId << base->serialize();
				destinationFlags.reset(SetResults::sendToLevel);
			}
			if (sendFlags.test(SetResults::sendToSource))
			{
				outSource >> (char)propId << base->serialize();
				destinationFlags.reset(SetResults::sendToSource);
			}

			// Sanity check to prevent infinite loops.  If we loop more times than there are possible destinations, something went wrong.
			if (++loopCount > setResults.resultFlags.size())
				break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::props
