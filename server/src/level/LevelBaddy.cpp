#include <IDebug.h>
#include <IEnums.h>
#include <IUtil.h>

#include "Server.h"
#include "level/Level.h"
#include "level/LevelBaddy.h"

const int baddytypes = 10;
const char* baddyImages[baddytypes] = {
	"baddygray.png", "baddyblue.png", "baddyred.png", "baddyblue.png", "baddygray.png",
	"baddyhare.png", "baddyoctopus.png", "baddygold.png", "baddylizardon.png", "baddydragon.png"
};
const char baddyStartMode[baddytypes] = {
	BDMODE_WALK, BDMODE_WALK, BDMODE_WALK, BDMODE_WALK, BDMODE_SWAMPSHOT,
	BDMODE_HAREJUMP, BDMODE_WALK, BDMODE_WALK, BDMODE_WALK, BDMODE_WALK
};
const int baddyPower[baddytypes] = {
	2, 3, 4, 3, 2,
	1, 1, 6, 12, 8
};

LevelBaddy::LevelBaddy(const float pX, const float pY, const unsigned char pType, std::weak_ptr<Level> pLevel, Server* pServer)
	: m_level(pLevel), m_server(pServer), m_type(pType), m_id(0),
	  m_startX(pX), m_startY(pY),
	  m_canRespawn(true), m_hasCustomImage(false)
{
	if (pType > baddytypes) m_type = 0;
	m_verses.resize(3);
	reset();
}

void LevelBaddy::reset()
{
	m_mode = baddyStartMode[(int)m_type];
	m_x = m_startX;
	m_y = m_startY;
	m_power = baddyPower[(int)m_type];
	m_image = baddyImages[(int)m_type];
	m_dir = (2 << 2) | 2; // Both head/body direction is encoded in dir.
	m_ani = 0;
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
			if (lvl->addItem(this->m_x, this->m_y, itemType))
				m_server->sendPacketToOneLevel(CString() >> (char)PLO_ITEMADD >> (char)(this->m_x * 2) >> (char)(this->m_y * 2) >> (char)LevelItem::getItemTypeId(itemType), m_level);
		}
	}
}

CString LevelBaddy::getProp(const int propId, int clientVersion) const
{
	switch (propId)
	{
		case BDPROP_ID:
			return CString() >> (char)m_id;

		case BDPROP_X:
			return CString() >> (char)(m_x * 2);

		case BDPROP_Y:
			return CString() >> (char)(m_y * 2);

		case BDPROP_TYPE:
			return CString() >> (char)m_type;

		case BDPROP_POWERIMAGE:
		{
			if (clientVersion < CLVER_2_1 && m_image == baddyImages[(int)m_type])
				return CString() >> (char)m_power >> (char)m_image.length() << m_image.replaceAll(".png", ".gif");
			else
				return CString() >> (char)m_power >> (char)m_image.length() << m_image;
		}

		case BDPROP_MODE:
			return CString() >> (char)m_mode;

		case BDPROP_ANI:
			return CString() >> (char)m_ani;

		case BDPROP_DIR:
			return CString() >> (char)m_dir;

		case BDPROP_VERSESIGHT:
		case BDPROP_VERSEHURT:
		case BDPROP_VERSEATTACK:
		{
			unsigned int verseId = propId - BDPROP_VERSESIGHT;
			if (verseId < m_verses.size())
				return CString() >> (char)m_verses[verseId].length() << m_verses[verseId];
			else
				return CString() >> (char)0;
		}
	}
	return CString();
}

CString LevelBaddy::getProps(int clientVersion) const
{
	CString retVal;
	for (int i = 1; i < BDPROP_COUNT; i++)
		retVal >> (char)i << getProp(i, clientVersion);
	return retVal;
}

void LevelBaddy::setProps(CString& pProps)
{
	int len = 0;
	while (pProps.bytesLeft())
	{
		unsigned char propId = pProps.readGUChar();
		switch (propId)
		{
			case BDPROP_ID:
				m_id = pProps.readGChar();
				break;

			case BDPROP_X:
				m_x = (float)pProps.readGChar() / 2.0f;
				m_x = clip(m_x, 0.0f, 63.5f);
				break;

			case BDPROP_Y:
				m_y = (float)pProps.readGChar() / 2.0f;
				m_y = clip(m_y, 0.0f, 63.5f);
				break;

			case BDPROP_TYPE:
				m_type = pProps.readGChar();
				break;

			case BDPROP_POWERIMAGE:
			{
				m_power = pProps.readGChar();
				if (pProps.bytesLeft() != 0)
				{
					CString newImage = pProps.readChars(pProps.readGUChar());

					if (newImage.isEmpty())
						m_image = baddyImages[(int)m_type];
					else
					{
						// Why we need this I have no idea.
						// For some reason, the client resets the custom image when the baddy is hurt.
						if (m_hasCustomImage == false)
						{
							m_hasCustomImage = true;
							m_image = newImage;
						}
					}
				}
			}
			break;

			case BDPROP_MODE:
				m_mode = pProps.readGChar();
				if (m_type == 4 && m_mode == BDMODE_HURT)
				{
					// Workaround for buggy client.  In 2 seconds, set us back to BDMODE_SWAMPSHOT from
					// inside Level.cpp.
					timeout.setTimeout(2);
				}
				else if (m_mode == BDMODE_DIE)
				{
					// In 2 seconds, set our mode to BDMODE_DEAD inside Level.cpp.
					timeout.setTimeout(2);

					// Drop items when dead.
					if (m_server->getSettings().getBool("baddyitems", false) == true)
						dropItem();
				}
				else if (m_mode == BDMODE_DEAD)
				{
					if (m_canRespawn)
						timeout.setTimeout(m_server->getSettings().getInt("baddyrespawntime", 60));
					else
					{
						if (auto lvl = m_level.lock(); lvl)
							lvl->removeBaddy(m_id);
						else
							delete this;
						return;
					}
				}
				break;

			case BDPROP_ANI:
				m_ani = pProps.readGChar();
				break;

			case BDPROP_DIR:
				m_dir = pProps.readGChar();
				break;

			case BDPROP_VERSESIGHT:
			case BDPROP_VERSEHURT:
			case BDPROP_VERSEATTACK:
			{
				len = pProps.readGUChar();
				unsigned int verseId = propId - BDPROP_VERSESIGHT;
				if (verseId < m_verses.size())
					m_verses[verseId] = pProps.readChars(len);
			}
		}
	}
}
