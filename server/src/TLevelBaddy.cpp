#include "ICommon.h"
#include "TLevelBaddy.h"
#include "TServer.h"
#include "TLevel.h"

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
	1, 1, 6, 12 ,8
};

const int baddypropcount = 11;
const bool baddyPropsReinit[baddypropcount] = {
	false, true, true, true, true,
	true, true, true, false, false, false
};

TLevelBaddy::TLevelBaddy(const float pX, const float pY, const char pType, TLevel* pLevel, TServer* pServer)
: level(pLevel), server(pServer), type(pType), id(0),
startX(pX), startY(pY),
respawn(true)
{
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
}

CString TLevelBaddy::getProp(const int propId) const
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
		return CString() >> (char)power >> (char)image.length() << image;

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

CString TLevelBaddy::getProps() const
{
	CString retVal;
	for (int i = 1; i < baddypropcount; i++)
		retVal >> (char)i << getProp(i);
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
				// Old gserver allowed the ID to be changed.  I doubt that is okay.
				printf( "TLevelBaddy::setProps(), BDPROP_ID is being changed when it shouldn't be.\n" );
				pProps.readGChar();
				//id = pProps.readGChar();
			break;

			case BDPROP_X:
				x = (float)pProps.readGChar() / 2.0f;
				x = clip(x, 0, 63);
			break;

			case BDPROP_Y:
				y = (float)pProps.readGChar() / 2.0f;
				y = clip(y, 0, 63);
			break;

			case BDPROP_TYPE:
				type = pProps.readGChar();
			break;

			case BDPROP_POWERIMAGE:
				if (pProps.bytesLeft() > 1)
				{
					power = pProps.readGChar();
					image = pProps.readChars(pProps.readGUChar());
				}
			break;

			case BDPROP_MODE:
				mode = pProps.readGChar();
				if (mode == BDMODE_DIE)
				{
					if (respawn)
						timeout.setTimeout(server->getSettings()->getInt("baddyrespawntime", 60));
					else
						level->removeBaddy(id);
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
