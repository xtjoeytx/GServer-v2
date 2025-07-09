#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include <BabyDI.h>
#include <CString.h>

#include <Server.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/PropsContainer.h>

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
	value = gameValue.get<std::string>().value_or("");
}

std::format_context::iterator PropertyString::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "value: {}", value.empty() ? "(empty)" : value);
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
			image = std::format("sword{}.{}", powerVal, (server->Generation != ServerGeneration::ORIGINAL ? "png" : "gif"));
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
	image = gameValue.get<std::string>().value_or("");
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
			image = std::format("shield{}.{}", powerVal, (server->Generation != ServerGeneration::ORIGINAL ? "png" : "gif"));
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
	if (server->Generation == ServerGeneration::ORIGINAL)
	{
		if (!image.contains("."))
			image += ".gif";
	}
}

void PropertyShieldPower::apply(const GameValue& gameValue)
{
	image = gameValue.get<std::string>().value_or("");
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
			if (!image.contains("."))
				image += ".gif";
			bowGif = std::make_pair(std::move(image), 0);
		}
	}
}

void PropertyGaniOrBowGif::apply(const GameValue& gameValue)
{
	gani = gameValue.get<std::string>().value_or("");
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
	image = gameValue.get<std::string>().value_or("");
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
	if (!array.has_value() || array.value().size() != 2)
	{
		rating = 0;
		deviation = 0;
		return;
	}

	auto& values = array.value();
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
	npcId = static_cast<NPCID>(gameValue.get<double>().value_or(0));
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
	pixelCoordinate = static_cast<int16_t>(gameValue.get<double>().value_or(0) * 16);
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
	result.writeGCharUnsafe(pixelCoordinate / 8);
	return result;
}

void PropertyTileCoordinate::deserialize(CString& data)
{
	pixelCoordinate = static_cast<int16_t>(std::clamp(data.readGChar() * 8, -20 * 16, 84 * 16));
}

void PropertyTileCoordinate::apply(const GameValue& gameValue)
{
	pixelCoordinate = static_cast<int16_t>(gameValue.get<double>().value_or(0) * 16);
}

std::format_context::iterator PropertyTileCoordinate::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "tile: {:.2f} (pixel: {})", (pixelCoordinate / 16.0f), pixelCoordinate);
}

// -----------------------------------------------
// PropertyTileCoordinateZ

CString PropertyTileCoordinateZ::serialize() const
{
	return CString() >> (char)(std::min(85 * 2, std::max(-25 * 2, (pixelCoordinate / 8))) + 50);
}

void PropertyTileCoordinateZ::deserialize(CString& data)
{
	pixelCoordinate = (data.readGUChar() - 50) * 8;
}

void PropertyTileCoordinateZ::apply(const GameValue& gameValue)
{
	pixelCoordinate = static_cast<int16_t>(gameValue.get<double>().value_or(0) * 16);
}

std::format_context::iterator PropertyTileCoordinateZ::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "tile: {:.2f} (pixel: {})", (pixelCoordinate / 16.0f), pixelCoordinate);
}

// -----------------------------------------------
// PropertyGS1Script

CString PropertyGS1Script::serialize() const
{
	auto* server = BabyDI::Get<Server>();

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
	script = gameValue.get<std::string>().value_or("");
}

std::format_context::iterator PropertyGS1Script::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "script size: {}", script.size());
}

// -----------------------------------------------
// PropertyHurtDxDy

CString PropertyHurtDxDy::serialize() const
{
	auto clampedDX = std::clamp(hurtDX, -1.0f, 1.0f);
	auto clampedDY = std::clamp(hurtDY, -1.0f, 1.0f);

	// The range is from 0 - 64 with 32 being the center.
	// So a value of 32 is 0, a value of 0 is -32, and a value of 64 is +32.
	// This encodes the floating point in steps of 1/32.
	// Whether this represents pixels for 2 tiles, or just a way to encode floats, I am not sure.

	return CString() >> (char)((hurtDX * 32) + 32) >> (char)((hurtDY * 32) + 32);
}

void PropertyHurtDxDy::deserialize(CString& data)
{
	uint8_t dx = data.readGUChar();
	uint8_t dy = data.readGUChar();

	// Convert the values back to a float in the range of -1.0 to 1.0.
	hurtDX = (static_cast<float>(dx - 32)) / 32.0f;
	hurtDY = (static_cast<float>(dy - 32)) / 32.0f;
}

void PropertyHurtDxDy::apply(const GameValue& gameValue)
{
	auto array = gameValue.get<std::vector<double>>();
	if (!array.has_value() || array.value().size() != 2)
	{
		hurtDX = 0.0f;
		hurtDY = 0.0f;
		return;
	}

	auto& values = array.value();
	hurtDX = static_cast<float>(values[0]);
	hurtDY = static_cast<float>(values[1]);
}

std::format_context::iterator PropertyHurtDxDy::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "dx: {:.2f}, dy: {:.2f}", hurtDX, hurtDY);
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

	imagePart.position = { x ,y };
	imagePart.size = { width, height };
}

void PropertyImagePart::apply(const GameValue& gameValue)
{
	auto array = gameValue.get<std::vector<double>>();
	if (!array.has_value() || array.value().size() < 4)
		return;

	auto& values = array.value();
	imagePart.position = { static_cast<uint16_t>(values[0]), static_cast<uint16_t>(values[1]) };
	imagePart.size = { static_cast<uint8_t>(values[2]), static_cast<uint8_t>(values[3]) };
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
	auto value = static_cast<uint8_t>(gameValue.get<double>().value_or(0.0));
	sprite = value >> 2;
	direction = value & 0b0000'0011;
}

std::format_context::iterator PropertySprite::format(std::format_context& ctx) const
{
	return std::format_to(ctx.out(), "sprite: {}, direction: {}", sprite, direction);
}

////////////////////////////////////////////////////////////////////////////////

uint8_t Limits::applyMaxHitpoints(uint8_t maxHitpoints)
{
	auto server = BabyDI::Get<Server>();
	auto heartLimit = std::min(server->getSettings().getInt("heartlimit", 3), 20);
	return std::clamp(maxHitpoints, static_cast<uint8_t>(0), static_cast<uint8_t>(heartLimit));
}

int8_t Limits::applySwordPower(int8_t swordPower)
{
	auto server = BabyDI::Get<Server>();
	auto& settings = server->getSettings();
	int8_t minimum = (settings.getBool("healswords", false) ? -(settings.getInt("swordlimit", 3)) : 0);
	int8_t maximum = settings.getInt("swordlimit", 3);
	return std::clamp(swordPower, minimum, maximum);
}

uint8_t Limits::applyShieldPower(uint8_t shieldPower)
{
	auto server = BabyDI::Get<Server>();
	return std::clamp(shieldPower, static_cast<uint8_t>(0), static_cast<uint8_t>(server->getSettings().getInt("shieldlimit", 3)));
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
	for (const auto& [propId, resultTuple] : sendOrder)
	{
		auto& setResults = std::get<0>(resultTuple);

		// If the prop is not set, just get the prop from the player.
		std::shared_ptr<PropertyBase> base = std::get<1>(resultTuple);
		if (base == nullptr || setResults.resultFlags.test(SetResults::getLatestOnSend))
			base = getProp(propId);

		if (setResults.resultFlags.test(SetResults::sendToAll))
			outAll >> (char)propId << base->serialize();
		if (setResults.resultFlags.test(SetResults::sendToLevel))
			outLevel >> (char)propId << base->serialize();
		if (setResults.resultFlags.test(SetResults::sendToSource))
			outSource >> (char)propId << base->serialize();
	}
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
