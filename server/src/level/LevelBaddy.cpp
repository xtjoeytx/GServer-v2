#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string_view>
#include <string>

#include <BabyDI.h>
#include <CString.h>
#include <IEnums.h>

#include <Server.h>
#include <level/Level.h>
#include <level/LevelBaddy.h>

#include <level/LevelItem.h>
#include <random>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

constexpr int baddytypes = 10;
constexpr const char* baddyImages[baddytypes] = {
	"baddygray.png", "baddyblue.png", "baddyred.png", "baddyblue.png", "baddygray.png",
	"baddyhare.png", "baddyoctopus.png", "baddygold.png", "baddylizardon.png", "baddydragon.png"
};
constexpr BaddyMode baddyStartMode[baddytypes] = {
	BaddyMode::WALK, BaddyMode::WALK, BaddyMode::WALK, BaddyMode::WALK, BaddyMode::SWAMPSHOT,
	BaddyMode::HAREJUMP, BaddyMode::WALK, BaddyMode::WALK, BaddyMode::WALK, BaddyMode::WALK
};
constexpr int baddyPower[baddytypes] = {
	2, 3, 4, 3, 2,
	1, 1, 6, 12, 8
};

///////////////////////////////////////////////////////////////////////////////

BaddyType LevelBaddy::getBaddyTypeFromString(const std::string& type)
{
	// Try by name.
	for (unsigned int i = 0; i < BaddyNames.size(); ++i)
	{
		if (string::equalsi(BaddyNames[i], type))
		{
			// Handle spider.
			if (i == BADDYTYPE_COUNT)
				i = ENUM(BaddyType::OCTOPUS);

			return ENUM<BaddyType>(i);
		}
	}

	// Try by ID.
	if (uint32_t itemId = 0; string::toNumber(type, itemId) && itemId < BADDYTYPE_COUNT)
		return ENUM<BaddyType>(itemId);

	// Bad.
	return BaddyType::GRAYSOLDIER;
}

///////////////////////////////////////////////////////////////////////////////

LevelBaddy::LevelBaddy(const uint8_t id, const LocalPixelPosition& position, BaddyType type, const std::weak_ptr<Level>& level)
	: id(id), type(type), position(position), m_level(level), m_originalPosition(position)
{
	m_server = BabyDI::Get<Server>();
	assert(m_server != nullptr);
	if (PROPID(type) > baddytypes) type = BaddyType::GRAYSOLDIER;
	verses.resize(3);
	reset();
}

void LevelBaddy::reset()
{
	mode = baddyStartMode[PROPID(type)];
	power = baddyPower[PROPID(type)];
	image = baddyImages[PROPID(type)];
	position = m_originalPosition;
	direction = 2;
	headDirection = 2;
	animation = 0;
	m_hasCustomImage = false;
}

void LevelBaddy::dropItem() const
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution itemTypeDistribution(0, 11);

	// 41.66...% chance of a green gralat.
	// 41.66...% chance of something else.
	// 16.66...% chance of nothing.
	const auto itemId = itemTypeDistribution(gen);
	auto itemType = LevelItemType::INVALID;

	switch (itemId)
	{
		case 0: //GREENRUPEE
		case 1: //BLUERUPEE
		case 2: //REDRUPEE
		case 3: //BOMBS
		case 4: //DARTS
		case 5: //HEART
			itemType = LevelItem::getItemId(static_cast<int8_t>(itemId));
			break;

		default:
			if (itemId > 5 && itemId < 10)
				itemType = LevelItemType::GREENRUPEE;
			break;
	}

	if (itemType != LevelItemType::INVALID)
	{
		if (const auto lvl = m_level.lock(); lvl)
			lvl->addItem(inform_client, toPixelPosition({0, 0}, position), itemType);
	}
}

CString LevelBaddy::getProp(const BaddyProp propId) const
{
	switch (propId)
	{
		case BaddyProp::ID:
			return CString() >> (char)id;

		case BaddyProp::X:
			return CString() >> (char)(position.x() / 8);

		case BaddyProp::Y:
			return CString() >> (char)(position.y() / 8);

		case BaddyProp::TYPE:
			return CString() >> (char)PROPID(type);

		case BaddyProp::POWERIMAGE:
		{
			if (m_server->Generation == ServerGeneration::CLASSIC && image == baddyImages[PROPID(type)])
				return CString() >> (char)power >> (char)image.length() << string::replace(image, ".png", ".gif");
			else
				return CString() >> (char)power >> (char)image.length() << image;
		}

		case BaddyProp::MODE:
			return CString() >> (char)PROPID(mode);

		case BaddyProp::ANI:
			return CString() >> (char)animation;

		case BaddyProp::DIR:
			return CString() >> (char)(headDirection << 2 | direction);

		case BaddyProp::VERSESIGHT:
		case BaddyProp::VERSEHURT:
		case BaddyProp::VERSEATTACK:
		{
			const size_t verseId = PROPID(propId) - PROPID(BaddyProp::VERSESIGHT);
			if (verseId < verses.size())
				return CString() >> (char)verses[verseId].length() << verses[verseId];
			else
				return CString() >> (char)0;
		}

		default:;
	}
	return {};
}

CString LevelBaddy::getProps() const
{
	CString retVal;
	for (size_t i = 1; i < BADDYPROP_COUNT; i++)
		retVal >> (char)i << getProp(static_cast<BaddyProp>(i));
	return retVal;
}

void LevelBaddy::setPropsFromPacket(CString& pProps)
{
	int len = 0;
	while (pProps.bytesLeft())
	{
		const auto propId = static_cast<BaddyProp>(pProps.readGUChar());
		switch (propId)
		{
			case BaddyProp::ID:
				id = pProps.readGChar();
				break;

			case BaddyProp::X:
				position.x() = static_cast<int16_t>(std::clamp(pProps.readGChar() * 8, 0, 1016)); // 0 - 63.5
				break;

			case BaddyProp::Y:
				position.y() = static_cast<int16_t>(std::clamp(pProps.readGChar() * 8, 0, 1016)); // 0 - 63.5
				break;

			case BaddyProp::TYPE:
				type = static_cast<BaddyType>(pProps.readGChar());
				break;

			case BaddyProp::POWERIMAGE:
			{
				power = pProps.readGChar();
				if (pProps.bytesLeft() != 0)
				{
					CString newImage = pProps.readChars(pProps.readGUChar());

					if (newImage.isEmpty())
						image = baddyImages[PROPID(type)];
					else
					{
						// Why we need this I have no idea.
						// For some reason, the client resets the custom image when the baddy is hurt.
						if (m_hasCustomImage == false)
						{
							m_hasCustomImage = true;
							image = newImage;
						}
					}
				}
			}
			break;

			case BaddyProp::MODE:
			{
				mode = static_cast<BaddyMode>(pProps.readGChar());

				// Swamp soldiers can get stuck in a hurt animation and become invulnerable.
				auto fixStuckSwampSoldier = [this](int)
				{
					if (power == 1)
					{
						mode = BaddyMode::SWAMPSHOT;
						m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_BADDYPROPS >> (char)id >> (char)BaddyProp::MODE >> (char)mode, {0, 0}, m_level.lock());
					}
				};

				// Reset and respawn baddies.
				auto respawnBaddy = [this](int)
				{
					if (!canRespawn()) return;
					reset();
					m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_BADDYPROPS >> (char)id << getProps(), {0, 0}, m_level.lock());
				};

				// Set baddies to dead.
				auto setDead = [this, respawnBaddy](int)
				{
					mode = BaddyMode::DEAD;
					if (canRespawn())
					{
						timeout.callbackIterations = respawnBaddy;
						timeout.runOnceFor(std::chrono::seconds(m_server->getSettings().get<uint32_t>("baddyrespawntime").value_or(60)));
					}

					if (const auto level = m_level.lock(); level != nullptr)
					{
						// Set the baddy as dead for all the other players in the level.
						m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_BADDYPROPS >> (char)id >> (char)BaddyProp::MODE >> (char)mode, {0, 0}, level);

						// TODO(Nalin): Record the last player who hit the baddy so we can record the source properly.
						if (!level->hasLivingBaddies())
							m_server->queueNPCEventLocal(level, ScriptEventType::COMPUSDIED, source::FromLevel(level));
					}
				};

				if (type == BaddyType::SWAMPSOLDIER && mode == BaddyMode::HURT)
				{
					timeout.callbackIterations = fixStuckSwampSoldier;
					timeout.runOnceFor(2s);
				}
				else if (mode == BaddyMode::DIE)
				{
					// Drop items when dead.
					if (m_server->getSettings().get<bool>("baddyitems").value_or(false) == true)
						dropItem();

					// Set the baddy to dead after 2 seconds.
					timeout.callbackIterations = setDead;
					timeout.runOnceFor(2s);
				}
				else if (mode == BaddyMode::DEAD && m_canRespawn)
				{
					timeout.callbackIterations = respawnBaddy;
					timeout.runOnceFor(std::chrono::seconds(m_server->getSettings().get<uint32_t>("baddyrespawntime").value_or(60)));
				}
				break;
			}

			case BaddyProp::ANI:
				animation = pProps.readGChar();
				break;

			case BaddyProp::DIR:
				direction = pProps.readGChar();
				headDirection = direction >> 2;
				direction &= 0b11;
				break;

			case BaddyProp::VERSESIGHT:
			case BaddyProp::VERSEHURT:
			case BaddyProp::VERSEATTACK:
			{
				len = pProps.readGUChar();
				size_t verseId = PROPID(propId) - PROPID(BaddyProp::VERSESIGHT);
				if (verseId < verses.size())
					verses[verseId] = pProps.readChars(len);
			}

			default:;
		}
	}
}

void LevelBaddy::setImage(const std::string_view baddyImage)
{
	image = baddyImage;
	m_hasCustomImage = true;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
