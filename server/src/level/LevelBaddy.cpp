#include <IEnums.h>
#include <IUtil.h>

#include "Server.h"
#include "level/Level.h"
#include "level/LevelBaddy.h"

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

LevelBaddy::LevelBaddy(float x, float y, BaddyType type, std::weak_ptr<Level> level)
	: m_level(level), type(type), m_originalX(x), m_originalY(y)
{
	if (PROPID(type) > baddytypes) type = BaddyType::GRAYSOLDIER;
	verses.resize(3);
	reset();
}

void LevelBaddy::reset()
{
	mode = baddyStartMode[PROPID(type)];
	x = m_originalX;
	y = m_originalY;
	power = baddyPower[PROPID(type)];
	image = baddyImages[PROPID(type)];
	direction = (2 << 2) | 2; // Both head/body direction is encoded in dir.
	animation = 0;
	m_hasCustomImage = false;
}

void LevelBaddy::dropItem()
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
		{
			if (lvl->addItem(this->x, this->y, itemType))
				m_server->sendPacketToOneLevel(CString() >> (char)PLO_ITEMADD >> (char)(this->x * 2) >> (char)(this->y * 2) >> (char)LevelItem::getItemTypeId(itemType), m_level);
		}
	}
}

CString LevelBaddy::getProp(BaddyProp propId, int clientVersion) const
{
	switch (propId)
	{
		case BaddyProp::ID:
			return CString() >> (char)id;

		case BaddyProp::X:
			return CString() >> (char)(x * 2);

		case BaddyProp::Y:
			return CString() >> (char)(y * 2);

		case BaddyProp::TYPE:
			return CString() >> (char)PROPID(type);

		case BaddyProp::POWERIMAGE:
		{
			if (clientVersion < CLVER_2_1 && image == baddyImages[PROPID(type)])
				return CString() >> (char)power >> (char)image.length() << string::replace(image, ".png", ".gif");
			else
				return CString() >> (char)power >> (char)image.length() << image;
		}

		case BaddyProp::MODE:
			return CString() >> (char)PROPID(mode);

		case BaddyProp::ANI:
			return CString() >> (char)animation;

		case BaddyProp::DIR:
			return CString() >> (char)direction;

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

CString LevelBaddy::getProps(int clientVersion) const
{
	CString retVal;
	for (int i = 1; i < BADDYPROP_COUNT; i++)
		retVal >> (char)i << getProp(static_cast<BaddyProp>(i), clientVersion);
	return retVal;
}

void LevelBaddy::setPropsFromPacket(CString& pProps)
{
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
				x = (float)pProps.readGChar() / 2.0f;
				x = clip(x, 0.0f, 63.5f);
				break;

			case BaddyProp::Y:
				y = (float)pProps.readGChar() / 2.0f;
				y = clip(y, 0.0f, 63.5f);
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
				mode = static_cast<BaddyMode>(pProps.readGChar());
				if (type == BaddyType::SWAMPSOLDIER && mode == BaddyMode::HURT)
				{
					// Workaround for buggy client.  In 2 seconds, set us back to BaddyMode::SWAMPSHOT from inside Level.cpp.
					timeout.setTimeout(2);
				}
				else if (mode == BaddyMode::DIE)
				{
					// In 2 seconds, set our mode to BaddyMode::DEAD inside Level.cpp.
					timeout.setTimeout(2);

					// Drop items when dead.
					if (m_server->getSettings().getBool("baddyitems", false) == true)
						dropItem();
				}
				else if (mode == BaddyMode::DEAD)
				{
					if (m_canRespawn)
						timeout.setTimeout(m_server->getSettings().getInt("baddyrespawntime", 60));
				}
				break;

			case BaddyProp::ANI:
				animation = pProps.readGChar();
				break;

			case BaddyProp::DIR:
				direction = pProps.readGChar();
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
