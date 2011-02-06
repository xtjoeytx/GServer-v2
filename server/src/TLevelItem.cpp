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

CString TLevelItem::getItemStr() const
{
	return CString() >> (char)PLO_ITEMADD >> (char)x >> (char)y >> (char)item;
}

int TLevelItem::getItemId(const CString& pItemName)
{
	for (unsigned int i = 0; i < sizeof(__itemList) / sizeof(const char *); ++i)
	{
		if (__itemList[i] == pItemName)
			return i;
	}

	return -1;
}

CString TLevelItem::getItemName(const unsigned char id)
{
	if (id >= sizeof(__itemList) / sizeof(char*)) return CString();
	return CString(__itemList[id]);
}

CString TLevelItem::getItemPlayerProp(const signed char pItemId, TPlayer* player)
{
	return TLevelItem::getItemPlayerProp(__itemList[(int)pItemId], player);
}

CString TLevelItem::getItemPlayerProp(const CString& pItemName, TPlayer* player)
{
	int itemID = TLevelItem::getItemId(pItemName);
	if (itemID == -1) return CString();

	switch (itemID)
	{
		case 0:		// greenrupee
		case 1:		// bluerupee
		case 2:		// redrupee
		case 19:	// goldrupee
		{
			CString playerProp = player->getProp(PLPROP_RUPEESCOUNT);
			int rupeeCount = playerProp.readGInt();
			if (itemID == 19) rupeeCount += 100;
			else if (itemID == 2) rupeeCount += 30;
			else if (itemID == 1) rupeeCount += 5;
			else rupeeCount += 1;
			rupeeCount = clip(rupeeCount, 0, 9999999);
			return CString() >> (char)PLPROP_RUPEESCOUNT >> (int)rupeeCount;
		}

		case 3:		// bombs
		{
			CString playerProp = player->getProp(PLPROP_BOMBSCOUNT);
			char bombCount = playerProp.readGChar() + 5;
			bombCount = clip(bombCount, 0, 99);
			return CString() >> (char)PLPROP_BOMBSCOUNT >> (char)bombCount;
		}

		case 4:		// darts
		{
			CString playerProp = player->getProp(PLPROP_ARROWSCOUNT);
			char arrowCount = playerProp.readGChar() + 5;
			arrowCount = clip(arrowCount, 0, 99);
			return CString() >> (char)PLPROP_ARROWSCOUNT >> (char)arrowCount;
		}

		case 5:		// heart
		{
			CString playerProp = player->getProp(PLPROP_CURPOWER);
			char heartCount = playerProp.readGChar() + (1 * 2);
			playerProp = player->getProp(PLPROP_MAXPOWER);
			char heartMax = playerProp.readGChar() * 2;
			heartCount = clip(heartCount, 0, heartMax);
			return CString() >> (char)PLPROP_CURPOWER >> (char)heartCount;
		}

		case 6:		// glove1
		case 16:	// glove2
		{
			CString playerProp = player->getProp(PLPROP_GLOVEPOWER);
			char glovePower = playerProp.readGChar();
			if (itemID == 16) glovePower = 3;
			else glovePower = (glovePower < 2 ? 2 : glovePower);
			return CString() >> (char)PLPROP_GLOVEPOWER >> (char)glovePower;
		}

		case 7:		// bow
		case 8:		// bomb
		{
			player->msgPLI_WEAPONADD(CString() >> (char)0 >> (char)itemID);
			break;
		}

		case 9:		// shield
		case 15:	// mirrorshield
		case 17:	// lizardshield
		{
			char shieldPower = player->getShieldPower();
			if (itemID == 17) shieldPower = 3;
			else if (itemID == 15) shieldPower = (shieldPower < 2 ? 2 : shieldPower);
			else shieldPower = (shieldPower < 1 ? 1 : shieldPower);
			return CString() >> (char)PLPROP_SHIELDPOWER >> (char)shieldPower;
		}

		case 10:	// sword
		case 13:	// battleaxe
		case 18:	// lizardsword
		case 14:	// goldensword
		{
			char swordPower = (char)player->getSwordPower();
			if (itemID == 14) swordPower = 4;
			else if (itemID == 18) swordPower = (swordPower < 3 ? 3 : swordPower);
			else if (itemID == 13) swordPower = (swordPower < 2 ? 2 : swordPower);
			else swordPower = (swordPower < 1 ? 1 : swordPower);
			return CString() >> (char)PLPROP_SWORDPOWER >> (char)swordPower;
		}

		case 11:	// fullheart
		{
			CString playerProp = player->getProp(PLPROP_MAXPOWER);
			unsigned char heartMax = playerProp.readGUChar() + 1;
			heartMax = clip(heartMax, 0, 20);		// Hard limit of 20 hearts.
			return CString() >> (char)PLPROP_MAXPOWER >> (char)heartMax >> (char)PLPROP_CURPOWER >> (char)(heartMax * 2);
		}

		case 12:	// superbomb
		case 20:	// fireball
		case 21:	// fireblast
		case 22:	// nukeshot
		case 23:	// joltbomb
		{
			player->msgPLI_WEAPONADD(CString() >> (char)0 >> (char)itemID);
			break;
		}

		case 24:	// spinattack
		{
			CString playerProp = player->getProp(PLPROP_STATUS);
			char status = playerProp.readGChar();
			if (status & 64) return CString();
			status |= 64;
			return CString() >> (char)PLPROP_STATUS >> (char)status;
		}
	}

	return CString();
}
