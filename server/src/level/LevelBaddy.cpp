#include <cstdlib>
#include <memory>
#include <string_view>

#include <BabyDI.h>
#include <CString.h>
#include <IEnums.h>
#include <IUtil.h>

#include <Server.h>
#include <level/Level.h>
#include <level/LevelBaddy.h>
#include <level/LevelItem.h>
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
constexpr const int baddyPower[baddytypes] = {
	2, 3, 4, 3, 2,
	1, 1, 6, 12, 8
};

LevelBaddy::LevelBaddy(const PixelPosition& position, BaddyType type, std::weak_ptr<Level> level)
	: type(type), position(position), m_level(level), m_originalPosition(position)
{
	if (PROPID(type) > baddytypes) type = BaddyType::GRAYSOLDIER;
	verses.resize(3);
	reset();
}

void LevelBaddy::reset()
{
	mode = baddyStartMode[PROPID(type)];
	power = baddyPower[PROPID(type)];
	image = baddyImages[PROPID(type)];
	direction = 2;
	headDirection = 2;
	animation = 0;
	m_hasCustomImage = false;
}

void LevelBaddy::dropItem() const
{
	// 41.66...% chance of a green gralat.
	// 41.66...% chance of something else.
	// 16.66...% chance of nothing.
	int itemId = rand() % 12;
	LevelItemType itemType = LevelItemType::INVALID;

	switch (itemId)
	{
		case 0: //GREENRUPEE
		case 1: //BLUERUPEE
		case 2: //REDRUPEE
		case 3: //BOMBS
		case 4: //DARTS
		case 5: //HEART
			itemType = LevelItem::getItemId(itemId);
			break;

		default:
			if (itemId > 5 && itemId < 10)
				itemType = LevelItemType::GREENRUPEE;
			break;
	}

	if (itemType != LevelItemType::INVALID)
	{
		if (auto lvl = m_level.lock(); lvl)
			lvl->addItem(inform_client, position, itemType);
	}
}

CString LevelBaddy::getProp(BaddyProp propId) const
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
			auto server = BabyDI::Get<Server>();
			if (server->Generation == ServerGeneration::ORIGINAL && image == baddyImages[PROPID(type)])
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
			auto verseId = PROPID(propId) - PROPID(BaddyProp::VERSESIGHT);
			if (verseId < verses.size())
				return CString() >> (char)verses[verseId].length() << verses[verseId];
			else
				return CString() >> (char)0;
		}
	}
	return CString();
}

CString LevelBaddy::getProps() const
{
	CString retVal;
	for (int i = 1; i < BADDYPROP_COUNT; i++)
		retVal >> (char)i << getProp(static_cast<BaddyProp>(i));
	return retVal;
}

void LevelBaddy::setPropsFromPacket(CString& pProps)
{
	auto server = BabyDI::Get<Server>();
	int len = 0;
	while (pProps.bytesLeft())
	{
		BaddyProp propId = static_cast<BaddyProp>(pProps.readGUChar());
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
				auto fixStuckSwampSolder = [this, server](int)
				{
					if (power == 1)
					{
						mode = BaddyMode::SWAMPSHOT;
						server->sendPacketToOneLevel(CString() >> (char)PLO_BADDYPROPS >> (char)id >> (char)BaddyProp::MODE >> (char)mode, m_level);
					}
				};

				// Reset and respawn baddies.
				auto respawnBaddy = [this, server](int)
				{
					if (!canRespawn()) return;
					reset();
					server->sendPacketToOneLevel(CString() >> (char)PLO_BADDYPROPS >> (char)id << getProps(), m_level);
				};

				// Set baddies to dead.
				auto setDead = [this, server, respawnBaddy](int)
				{
					mode = BaddyMode::DEAD;
					if (canRespawn())
					{
						timeout.callbackIterations = respawnBaddy;
						timeout.startFor(std::chrono::seconds(server->getSettings().getInt("baddyrespawntime", 60)));
					}

					// Set the baddy as dead for all the other players in the level.
					server->sendPacketToOneLevel(CString() >> (char)PLO_BADDYPROPS >> (char)id >> (char)BaddyProp::MODE >> (char)mode, m_level);

					// TODO(Nalin): Record the last player who hit the baddy so we can record the source properly.
					if (auto level = m_level.lock(); level != nullptr)
					{
						if (!level->hasLivingBaddies())
							server->queueNPCEvent(level, ScriptEventType::COMPUSDIED, source::FromLevel(level));
					}
				};

				if (type == BaddyType::SWAMPSOLDIER && mode == BaddyMode::HURT)
				{
					timeout.callbackIterations = fixStuckSwampSolder;
					timeout.startFor(2s);
				}
				else if (mode == BaddyMode::DIE)
				{
					// Drop items when dead.
					if (server->getSettings().getBool("baddyitems", false) == true)
						dropItem();

					// Set the baddy to dead after 2 seconds.
					timeout.callbackIterations = setDead;
					timeout.startFor(2s);
				}
				else if (mode == BaddyMode::DEAD && m_canRespawn)
				{
					timeout.callbackIterations = respawnBaddy;
					timeout.startFor(std::chrono::seconds(server->getSettings().getInt("baddyrespawntime", 60)));
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
				auto verseId = PROPID(propId) - PROPID(BaddyProp::VERSESIGHT);
				if (verseId < verses.size())
					verses[verseId] = pProps.readChars(len);
			}
		}
	}
}

void LevelBaddy::setImage(std::string_view image)
{
	image = image;
	m_hasCustomImage = true;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
