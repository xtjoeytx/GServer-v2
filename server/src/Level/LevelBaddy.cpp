#include "IDebug.h"
#include "IEnums.h"
#include "Server.h"
#include "IUtil.h"
#include "LevelBaddy.h"
#include "Level.h"

const int baddytypes = 10;
const char* baddyImages[baddytypes] = {
	"baddygray.png", "baddyblue.png", "baddyred.png", "baddyblue.png", "baddygray.png",
	"baddyhare.png", "baddyoctopus.png", "baddygold.png", "baddylizardon.png", "baddydragon.png"
};
const char baddyStartMode[baddytypes] = {
	BDMODE_WALK, BDMODE_WALK, BDMODE_WALK, BDMODE_WALK,	BDMODE_SWAMPSHOT,
	BDMODE_HAREJUMP, BDMODE_WALK, BDMODE_WALK, BDMODE_WALK, BDMODE_WALK
};
const int baddyPower[baddytypes] = {
	2, 3, 4, 3, 2,
	1, 1, 6, 12, 8
};


TLevelBaddy::TLevelBaddy(const float pX, const float pY, const unsigned char pType, std::weak_ptr<TLevel> pLevel, TServer* pServer)
: level(pLevel), server(pServer), type(pType), id(0),
startX(pX), startY(pY),
respawn(true), setImage(false)
{
	if (pType > baddytypes) type = 0;
	verses.resize(3);
	reset();
}

void TLevelBaddy::reset()
{
	mode = baddyStartMode[(int)type];
	x = startX;
	y = startY;
	power = baddyPower[(int)type];
	image = baddyImages[(int)type];
	dir = (2 << 2) | 2;			// Both head/body direction is encoded in dir.
	ani = 0;
	setImage = false;
}

void TLevelBaddy::dropItem()
{
	// 41.66...% chance of a green gralat.
	// 41.66...% chance of something else.
	// 16.66...% chance of nothing.
	int itemId = rand()%12;
	LevelItemType itemType = LevelItemType::INVALID;

	switch (itemId)
	{
		case 0:	//GREENRUPEE
		case 1:	//BLUERUPEE
		case 2:	//REDRUPEE
		case 3:	//BOMBS
		case 4:	//DARTS
		case 5:	//HEART
			itemType = TLevelItem::getItemId(itemId);
			break;

		default:
			if (itemId > 5 && itemId < 10)
				itemType = LevelItemType::GREENRUPEE;
			break;
	}

	if (itemType != LevelItemType::INVALID)
	{
		if (auto lvl = level.lock(); lvl)
		{
			if (lvl->addItem(this->x, this->y, itemType))
				server->sendPacketToOneLevel(CString() >> (char)PLO_ITEMADD >> (char)(this->x * 2) >> (char)(this->y * 2) >> (char)TLevelItem::getItemTypeId(itemType), level);
		}
	}
}

CString TLevelBaddy::getProp(const int propId, int clientVersion) const
{
	switch (propId)
	{
		case BDPROP_ID:
		return CString() >> (char)id;

		case BDPROP_X:
		return CString() >> (char)(x * 2);

		case BDPROP_Y:
		return CString() >> (char)(y * 2);

		case BDPROP_TYPE:
		return CString() >> (char)type;

		case BDPROP_POWERIMAGE:
		{
			if (clientVersion < CLVER_2_1 && image == baddyImages[(int)type])
				return CString() >> (char)power >> (char)image.length() << image.replaceAll(".png", ".gif");
			else return CString() >> (char)power >> (char)image.length() << image;
		}

		case BDPROP_MODE:
		return CString() >> (char)mode;

		case BDPROP_ANI:
		return CString() >> (char)ani;

		case BDPROP_DIR:
		return CString() >> (char)dir;

		case BDPROP_VERSESIGHT:
		case BDPROP_VERSEHURT:
		case BDPROP_VERSEATTACK:
		{
			unsigned int verseId = propId - BDPROP_VERSESIGHT;
			if (verseId < verses.size())
				return CString() >> (char)verses[verseId].length() << verses[verseId];
			else return CString() >> (char)0;
		}
	}
	return CString();
}

CString TLevelBaddy::getProps(int clientVersion) const
{
	CString retVal;
	for (int i = 1; i < BDPROP_COUNT; i++)
		retVal >> (char)i << getProp(i, clientVersion);
	return retVal;
}

void TLevelBaddy::setProps(CString &pProps)
{
	int len = 0;
	while (pProps.bytesLeft())
	{
		unsigned char propId = pProps.readGUChar();
		switch (propId)
		{
			case BDPROP_ID:
				id = pProps.readGChar();
			break;

			case BDPROP_X:
				x = (float)pProps.readGChar() / 2.0f;
				x = clip(x, 0.0f, 63.5f);
			break;

			case BDPROP_Y:
				y = (float)pProps.readGChar() / 2.0f;
				y = clip(y, 0.0f, 63.5f);
			break;

			case BDPROP_TYPE:
				type = pProps.readGChar();
			break;

			case BDPROP_POWERIMAGE:
			{
				power = pProps.readGChar();
				if (pProps.bytesLeft() != 0)
				{
					CString newImage = pProps.readChars(pProps.readGUChar());

					if (newImage.isEmpty())
						image = baddyImages[(int)type];
					else
					{
						// Why we need this I have no idea.
						// For some reason, the client resets the custom image when the baddy is hurt.
						if (setImage == false)
						{
							setImage = true;
							image = newImage;
						}
					}
				}
			}
			break;

			case BDPROP_MODE:
				mode = pProps.readGChar();
				if (type == 4 && mode == BDMODE_HURT)
				{
					// Workaround for buggy client.  In 2 seconds, set us back to BDMODE_SWAMPSHOT from
					// inside TLevel.cpp.
					timeout.setTimeout(2);
				}
				else if (mode == BDMODE_DIE)
				{
					// In 2 seconds, set our mode to BDMODE_DEAD inside TLevel.cpp.
					timeout.setTimeout(2);

					// Drop items when dead.
					if (server->getSettings().getBool("baddyitems", false) == true)
						dropItem();
				}
				else if (mode == BDMODE_DEAD)
				{
					if (respawn)
						timeout.setTimeout(server->getSettings().getInt("baddyrespawntime", 60));
					else
					{
						if (auto lvl = level.lock(); lvl)
							lvl->removeBaddy(id);
						else delete this;
						return;
					}
				}
			break;

			case BDPROP_ANI:
				ani = pProps.readGChar();
			break;

			case BDPROP_DIR:
				dir = pProps.readGChar();
			break;

			case BDPROP_VERSESIGHT:
			case BDPROP_VERSEHURT:
			case BDPROP_VERSEATTACK:
			{
				len = pProps.readGUChar();
				unsigned int verseId = propId - BDPROP_VERSESIGHT;
				if (verseId < verses.size())
					verses[verseId] = pProps.readChars(len);
			}
		}
	}
}
