#include <IDebug.h>
#include <IEnums.h>

#include "object/Player.h"
#include "level/LevelItem.h"

const char* __itemList[] = {
	"greenrupee",   // 0
	"bluerupee",    // 1
	"redrupee",     // 2
	"bombs",        // 3
	"darts",        // 4
	"heart",        // 5
	"glove1",       // 6
	"bow",          // 7
	"bomb",         // 8
	"shield",       // 9
	"sword",        // 10
	"fullheart",    // 11
	"superbomb",    // 12
	"battleaxe",    // 13
	"goldensword",  // 14
	"mirrorshield", // 15
	"glove2",       // 16
	"lizardshield", // 17
	"lizardsword",  // 18
	"goldrupee",    // 19
	"fireball",     // 20
	"fireblast",    // 21
	"nukeshot",     // 22
	"joltbomb",     // 23
	"spinattack"    // 24
};

const int __itemCount = (sizeof(__itemList) / sizeof(const char*));

CString LevelItem::getItemStr() const
{
	return CString() >> (char)PLO_ITEMADD >> (char)m_x >> (char)m_y >> (char)m_item;
}

LevelItemType LevelItem::getItemId(signed char itemId)
{
	if (itemId < 0 || itemId >= __itemCount)
		return LevelItemType::INVALID;

	return LevelItemType(itemId);
}

LevelItemType LevelItem::getItemId(const std::string& pItemName)
{
	for (unsigned int i = 0; i < __itemCount; ++i)
	{
		if (__itemList[i] == pItemName)
			return LevelItemType(i);
	}

	return LevelItemType::INVALID;
}

std::string LevelItem::getItemName(LevelItemType itemId)
{
	auto id = LevelItem::getItemTypeId(itemId);
	if (id < 0 || id >= __itemCount) return {};
	return std::string(__itemList[id]);
}

CString LevelItem::getItemPlayerProp(LevelItemType itemType, Player* player)
{
	switch (itemType)
	{
		case LevelItemType::GREENRUPEE: // greenrupee
		case LevelItemType::BLUERUPEE:  // bluerupee
		case LevelItemType::REDRUPEE:   // redrupee
		case LevelItemType::GOLDRUPEE:  // goldrupee
		{
			int rupeeCount = player->account.character.gralats;
			if (itemType == LevelItemType::GOLDRUPEE) rupeeCount += 100;
			else if (itemType == LevelItemType::REDRUPEE)
				rupeeCount += 30;
			else if (itemType == LevelItemType::BLUERUPEE)
				rupeeCount += 5;
			else
				rupeeCount += 1;

			rupeeCount = clip(rupeeCount, 0, 9999999);
			return CString() >> (char)PLPROP_RUPEESCOUNT >> (int)rupeeCount;
		}

		case LevelItemType::BOMBS: // bombs
		{
			int bombCount = clip(player->account.character.bombs + 5, 0, 99);
			return CString() >> (char)PLPROP_BOMBSCOUNT >> (char)bombCount;
		}

		case LevelItemType::DARTS: // darts
		{
			int arrowCount = clip(player->account.character.arrows + 5, 0, 99);
			return CString() >> (char)PLPROP_ARROWSCOUNT >> (char)arrowCount;
		}

		case LevelItemType::HEART: // heart
		{
			uint8_t newPower = clip(player->account.character.hitpointsInHalves + 1, 0, player->account.maxHitpoints * 2);
			return CString() >> (char)PLPROP_CURPOWER >> (char)(newPower);
		}

		case LevelItemType::GLOVE1: // glove1
		case LevelItemType::GLOVE2: // glove2
		{
			auto glovePower = player->account.character.glovePower;
			if (itemType == LevelItemType::GLOVE2)
				glovePower = 3;
			else if (glovePower < 2)
				glovePower = 2;

			return CString() >> (char)PLPROP_GLOVEPOWER >> (char)glovePower;
		}

		case LevelItemType::BOW:       // bow
		case LevelItemType::BOMB:      // bomb
		case LevelItemType::SUPERBOMB: // superbomb
		case LevelItemType::FIREBALL:  // fireball
		case LevelItemType::FIREBLAST: // fireblast
		case LevelItemType::NUKESHOT:  // nukeshot
		case LevelItemType::JOLTBOMB:  // joltbomb
		{
			player->addWeapon(itemType);
			return {};
		}

		case LevelItemType::SHIELD:       // shield
		case LevelItemType::MIRRORSHIELD: // mirrorshield
		case LevelItemType::LIZARDSHIELD: // lizardshield
		{
			int newShieldPower = 1;
			if (itemType == LevelItemType::LIZARDSHIELD)
				newShieldPower = 3;
			else if (itemType == LevelItemType::MIRRORSHIELD)
				newShieldPower = 2;

			if (player->account.character.shieldPower > newShieldPower)
				newShieldPower = player->account.character.shieldPower;

			return CString() >> (char)PLPROP_SHIELDPOWER >> (char)newShieldPower;
		}

		case LevelItemType::SWORD:       // sword
		case LevelItemType::BATTLEAXE:   // battleaxe
		case LevelItemType::LIZARDSWORD: // lizardsword
		case LevelItemType::GOLDENSWORD: // goldensword
		{
			char swordPower = (char)player->account.character.swordPower;
			if (itemType == LevelItemType::GOLDENSWORD) swordPower = 4;
			else if (itemType == LevelItemType::LIZARDSWORD)
				swordPower = (swordPower < 3 ? 3 : swordPower);
			else if (itemType == LevelItemType::BATTLEAXE)
				swordPower = (swordPower < 2 ? 2 : swordPower);
			else
				swordPower = (swordPower < 1 ? 1 : swordPower);
			return CString() >> (char)PLPROP_SWORDPOWER >> (char)swordPower;
		}

		case LevelItemType::FULLHEART: // fullheart
		{
			char heartMax = clip(player->account.maxHitpoints + 1, 0, 20); // Hard limit of 20 hearts.
			return CString() >> (char)PLPROP_MAXPOWER >> (char)heartMax >> (char)PLPROP_CURPOWER >> (char)(heartMax * 2);
		}

		case LevelItemType::SPINATTACK: // spinattack
		{
			CString playerProp = player->getProp(PLPROP_STATUS);
			char status = playerProp.readGChar();
			if (status & PLSTATUS_HASSPIN) return {};
			status |= PLSTATUS_HASSPIN;
			return CString() >> (char)PLPROP_STATUS >> (char)status;
		}

		default:
			break;
	}

	return {};
}
