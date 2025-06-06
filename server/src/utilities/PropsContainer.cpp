#include <algorithm>

#include <utilities/PropsContainer.h>

#include <Server.h>
#include <object/Player.h>

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

// -----------------------------------------------
// PropertySwordPower

CString PropertySwordPower::serialize() const
{
	auto powerVal = power.value_or(1);
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
			image = std::format("sword{}{}", powerVal, ".png");
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

// -----------------------------------------------
// PropertyShieldPower

CString PropertyShieldPower::serialize() const
{
	auto powerVal = power.value_or(1);
	if (powerVal > 0 && powerVal <= 3 && image.empty())
		return CString() >> (char)powerVal;
	return CString() >> (char)(powerVal + 10) >> (char)image.length() << image;
}

void PropertyShieldPower::deserialize(CString& data)
{
	uint8_t powerVal = 0;
	data.readGInto(powerVal);
	if (powerVal < 10)
	{
		powerVal = Limits::applyShieldPower(powerVal);

		// For older clients, we use a default image name.
		if (powerVal > 0 && powerVal <= 4)
			image = std::format("shield{}{}", powerVal, ".png");
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
	// If there is no extension, assume its a .gif.
	image = data.readChars(data.readGUChar());
	if (!image.contains("."))
		image += ".gif";
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
	if (!headImage.contains("."))
		headImage += ".gif";
	image = std::move(headImage.toString());
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

// -----------------------------------------------
// PropertyTileCoordinate

CString PropertyTileCoordinate::serialize() const
{
	return CString() >> (char)(pixelCoordinate / 8);
}

void PropertyTileCoordinate::deserialize(CString& data)
{
	pixelCoordinate = data.readGUChar() * 8;
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
} // end namespace preagonal
