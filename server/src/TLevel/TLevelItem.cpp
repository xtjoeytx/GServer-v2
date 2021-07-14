#include "IDebug.h"
#include "TLevelItem.h"
#include "IEnums.h"
#include "TPlayer.h"

const char *__itemList[] = {
	"greenrupee",		// 0
	"bluerupee",		// 1
	"redrupee",			// 2
	"bombs",			// 3
	"darts",			// 4
	"heart",			// 5
	"glove1",			// 6
	"bow",				// 7
	"bomb",				// 8
	"shield",			// 9
	"sword",			// 10
	"fullheart",		// 11
	"superbomb",		// 12
	"battleaxe",		// 13
	"goldensword",		// 14
	"mirrorshield",		// 15
	"glove2",			// 16
	"lizardshield",		// 17
	"lizardsword",		// 18
	"goldrupee",		// 19
	"fireball",			// 20
	"fireblast",		// 21
	"nukeshot",			// 22
	"joltbomb",			// 23
	"spinattack"		// 24
};

const int __itemCount = (sizeof(__itemList) / sizeof(const char *));

CString TLevelItem::getItemStr() const
{
	return CString() >> (char)PLO_ITEMADD >> (char)x >> (char)y >> (char)item;
}

LevelItemType TLevelItem::getItemId(signed char itemId)
{
	if (itemId < 0 || itemId >= __itemCount)
		return LevelItemType::INVALID;

	return LevelItemType(itemId);
}

LevelItemType TLevelItem::getItemId(const CString& pItemName)
{
	for (unsigned int i = 0; i < __itemCount; ++i)
	{
		if (__itemList[i] == pItemName)
			return LevelItemType(i);
	}

	return LevelItemType::INVALID;
}

CString TLevelItem::getItemName(LevelItemType itemId)
{
	auto id = TLevelItem::getItemTypeId(itemId);
	if (id < 0 || id >= __itemCount) return CString();
	return CString(__itemList[id]);
}

CString TLevelItem::getItemPlayerProp(LevelItemType itemType, TPlayer* player)
{
	switch (itemType)
	{
		case LevelItemType::GREENRUPEE:		// greenrupee
		case LevelItemType::BLUERUPEE:		// bluerupee
		case LevelItemType::REDRUPEE:		// redrupee
		case LevelItemType::GOLDRUPEE:		// goldrupee
		{
			CString playerProp = player->getProp(PLPROP_RUPEESCOUNT);
			int rupeeCount = playerProp.readGInt();
			if (itemType == LevelItemType::GOLDRUPEE) rupeeCount += 100;
			else if (itemType == LevelItemType::REDRUPEE) rupeeCount += 30;
			else if (itemType == LevelItemType::BLUERUPEE) rupeeCount += 5;
			else rupeeCount += 1;
			rupeeCount = clip(rupeeCount, 0, 9999999);
			return CString() >> (char)PLPROP_RUPEESCOUNT >> (int)rupeeCount;
		}

		case LevelItemType::BOMBS:		// bombs
		{
			CString playerProp = player->getProp(PLPROP_BOMBSCOUNT);
			char bombCount = playerProp.readGChar() + 5;
			bombCount = clip(bombCount, 0, 99);
			return CString() >> (char)PLPROP_BOMBSCOUNT >> (char)bombCount;
		}

		case LevelItemType::DARTS:		// darts
		{
			CString playerProp = player->getProp(PLPROP_ARROWSCOUNT);
			char arrowCount = playerProp.readGChar() + 5;
			arrowCount = clip(arrowCount, 0, 99);
			return CString() >> (char)PLPROP_ARROWSCOUNT >> (char)arrowCount;
		}

		case LevelItemType::HEART:		// heart
		{
			CString playerProp = player->getProp(PLPROP_CURPOWER);
			char heartCount = playerProp.readGChar() + (1 * 2);
			playerProp = player->getProp(PLPROP_MAXPOWER);
			char heartMax = playerProp.readGChar() * 2;
			heartCount = clip(heartCount, 0, heartMax);
			return CString() >> (char)PLPROP_CURPOWER >> (char)heartCount;
		}

		case LevelItemType::GLOVE1:		// glove1
		case LevelItemType::GLOVE2:		// glove2
		{
			CString playerProp = player->getProp(PLPROP_GLOVEPOWER);
			char glovePower = playerProp.readGChar();
			if (itemType == LevelItemType::GLOVE2) glovePower = 3;
			else glovePower = (glovePower < 2 ? 2 : glovePower);
			return CString() >> (char)PLPROP_GLOVEPOWER >> (char)glovePower;
		}

		case LevelItemType::BOW:		// bow
		case LevelItemType::BOMB:		// bomb

		case LevelItemType::SUPERBOMB:	// superbomb
		case LevelItemType::FIREBALL:	// fireball
		case LevelItemType::FIREBLAST:	// fireblast
		case LevelItemType::NUKESHOT:	// nukeshot
		case LevelItemType::JOLTBOMB:	// joltbomb
		{
			player->msgPLI_WEAPONADD(CString() >> (char)0 >> (char)TLevelItem::getItemTypeId(itemType));
			break;
		}

		case LevelItemType::SHIELD:			// shield
		case LevelItemType::MIRRORSHIELD:	// mirrorshield
		case LevelItemType::LIZARDSHIELD:	// lizardshield
		{
			char shieldPower = player->getShieldPower();
			if (itemType == LevelItemType::LIZARDSHIELD) shieldPower = 3;
			else if (itemType == LevelItemType::MIRRORSHIELD) shieldPower = (shieldPower < 2 ? 2 : shieldPower);
			else shieldPower = (shieldPower < 1 ? 1 : shieldPower);
			return CString() >> (char)PLPROP_SHIELDPOWER >> (char)shieldPower;
		}

		case LevelItemType::SWORD:			// sword
		case LevelItemType::BATTLEAXE:		// battleaxe
		case LevelItemType::LIZARDSWORD:	// lizardsword
		case LevelItemType::GOLDENSWORD:	// goldensword
		{
			char swordPower = (char)player->getSwordPower();
			if (itemType == LevelItemType::GOLDENSWORD) swordPower = 4;
			else if (itemType == LevelItemType::LIZARDSWORD) swordPower = (swordPower < 3 ? 3 : swordPower);
			else if (itemType == LevelItemType::BATTLEAXE) swordPower = (swordPower < 2 ? 2 : swordPower);
			else swordPower = (swordPower < 1 ? 1 : swordPower);
			return CString() >> (char)PLPROP_SWORDPOWER >> (char)swordPower;
		}

		case LevelItemType::FULLHEART:	// fullheart
		{
			CString playerProp = player->getProp(PLPROP_MAXPOWER);
			unsigned char heartMax = playerProp.readGUChar() + 1;
			heartMax = clip(heartMax, 0, 20);		// Hard limit of 20 hearts.
			return CString() >> (char)PLPROP_MAXPOWER >> (char)heartMax >> (char)PLPROP_CURPOWER >> (char)(heartMax * 2);
		}

		case LevelItemType::SPINATTACK:	// spinattack
		{
			CString playerProp = player->getProp(PLPROP_STATUS);
			char status = playerProp.readGChar();
			if (status & PLSTATUS_HASSPIN) return CString();
			status |= PLSTATUS_HASSPIN;
			return CString() >> (char)PLPROP_STATUS >> (char)status;
		}
	}

	return CString();
}
