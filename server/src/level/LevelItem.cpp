#include <cstddef>
#include <cstdint>
#include <string>

#include <CString.h>
#include <IEnums.h>

#include <BabyDI.h>
#include <Server.h>
#include <level/LevelItem.h>
#include <object/Player.h>
#include <player/PlayerProps.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

LevelItemType LevelItem::getItemId(const signed char itemId)
{
	if (itemId < 0 || static_cast<size_t>(itemId) >= ItemNames.size())
		return LevelItemType::INVALID;

	return ENUM<LevelItemType>(itemId);
}

LevelItemType LevelItem::getItemId(const std::string& pItemName)
{
	// Try by name.
	for (unsigned int i = 0; i < ItemNames.size(); ++i)
	{
		if (ItemNames[i] == pItemName)
			return ENUM<LevelItemType>(static_cast<int>(i));
	}

	// Try by ID.
	if (uint32_t itemId = 0; string::toNumber(pItemName, itemId) && itemId < ItemNames.size())
		return ENUM<LevelItemType>(static_cast<int>(itemId));

	// Bad item.
	return LevelItemType::INVALID;
}

std::string LevelItem::getItemName(const LevelItemType itemId)
{
	const size_t id = LevelItem::getItemTypeId(itemId);
	if (id >= ItemNames.size()) return {};
	return std::string(ItemNames[id]);
}

CString LevelItem::getItemPlayerProp(const LevelItemType itemType, Player* player)
{
	switch (itemType)
	{
		case LevelItemType::GREENRUPEE: // greenrupee
		case LevelItemType::BLUERUPEE:  // bluerupee
		case LevelItemType::REDRUPEE:   // redrupee
		case LevelItemType::GOLDRUPEE:  // goldrupee
		{
			auto rupeeCount = player->account.character.gralats;
			if (itemType == LevelItemType::GOLDRUPEE)
				rupeeCount += 100;
			else if (itemType == LevelItemType::REDRUPEE)
				rupeeCount += 30;
			else if (itemType == LevelItemType::BLUERUPEE)
				rupeeCount += 5;
			else
				rupeeCount += 1;

			rupeeCount = std::clamp(rupeeCount, 0_ui32, 9999999_ui32);
			return CString() >> (char)PlayerProp::GRALATS >> (int)rupeeCount;
		}

		case LevelItemType::BOMBS: // bombs
		{
			const auto bombCount = std::clamp(player->account.character.bombs + 5, 0, 99);
			return CString() >> (char)PlayerProp::BOMBS >> (char)bombCount;
		}

		case LevelItemType::DARTS: // darts
		{
			const auto arrowCount = std::clamp(player->account.character.arrows + 5, 0, 99);
			return CString() >> (char)PlayerProp::ARROWS >> (char)arrowCount;
		}

		case LevelItemType::HEART: // heart
		{
			const auto newPower = std::clamp(player->account.character.hitpointsInHalves + 2, 0, player->account.maxHitpoints * 2);
			return CString() >> (char)PlayerProp::HALFHEARTS >> (char)(newPower);
		}

		case LevelItemType::GLOVE1: // glove1
		case LevelItemType::GLOVE2: // glove2
		{
			auto glovePower = player->account.character.glovePower;
			if (itemType == LevelItemType::GLOVE2)
				glovePower = 3;
			else if (glovePower < 2)
				glovePower = 2;

			return CString() >> (char)PlayerProp::GLOVEPOWER >> (char)glovePower;
		}

		case LevelItemType::BOMB:      // bomb
		case LevelItemType::SUPERBOMB: // superbomb
		case LevelItemType::JOLTBOMB:  // joltbomb
		{
			if (const auto server = BabyDI::Get<Server>(); server != nullptr && server->Generation == ServerGeneration::CLASSIC && player->getVersion() < CLVER_1_20)
			{
				auto bombPower = player->account.character.bombPower;
				if (itemType == LevelItemType::BOMB && bombPower < 1)
					bombPower = 1;
				else if (itemType == LevelItemType::SUPERBOMB && bombPower < 2)
					bombPower = 2;
				else if (itemType == LevelItemType::JOLTBOMB && bombPower < 3)
					bombPower = 3;

				return CString() >> (char)PlayerProp::BOMBPOWER >> (char)bombPower;
			}

			player->addWeapon(itemType);
			return {};
		}

		case LevelItemType::BOW:       // bow
		case LevelItemType::FIREBALL:  // fireball
		case LevelItemType::FIREBLAST: // fireblast
		case LevelItemType::NUKESHOT:  // nukeshot
		{
			if (const auto server = BabyDI::Get<Server>(); server != nullptr && server->Generation == ServerGeneration::CLASSIC && player->getVersion() < CLVER_1_20)
			{
				auto bowPower = player->account.character.bowPower;
				if (itemType == LevelItemType::BOW && bowPower < 1)
					bowPower = 1;
				else if (itemType == LevelItemType::FIREBALL && bowPower < 2)
					bowPower = 2;
				else if (itemType == LevelItemType::FIREBLAST && bowPower < 3)
					bowPower = 3;
				else if (itemType == LevelItemType::NUKESHOT && bowPower < 3)
					bowPower = 4;

				return CString() >> (char)PlayerProp::GANI >> (char)bowPower;
			}

			player->addWeapon(itemType);
			return {};
		}

		case LevelItemType::SHIELD:       // shield
		case LevelItemType::MIRRORSHIELD: // mirrorshield
		case LevelItemType::LIZARDSHIELD: // lizardshield
		{
			uint8_t newShieldPower = 1;
			if (itemType == LevelItemType::LIZARDSHIELD)
				newShieldPower = 3;
			else if (itemType == LevelItemType::MIRRORSHIELD)
				newShieldPower = 2;

			if (player->account.character.shieldPower > newShieldPower)
				newShieldPower = player->account.character.shieldPower;

			return CString() >> (char)PlayerProp::SHIELDIMAGE >> (char)newShieldPower;
		}

		case LevelItemType::SWORD:       // sword
		case LevelItemType::BATTLEAXE:   // battleaxe
		case LevelItemType::LIZARDSWORD: // lizardsword
		case LevelItemType::GOLDENSWORD: // goldensword
		{
			auto swordPower = player->account.character.swordPower;
			if (itemType == LevelItemType::GOLDENSWORD) swordPower = 4;
			else if (itemType == LevelItemType::LIZARDSWORD)
				swordPower = static_cast<int8_t>(swordPower < 3 ? 3 : swordPower);
			else if (itemType == LevelItemType::BATTLEAXE)
				swordPower = static_cast<int8_t>(swordPower < 2 ? 2 : swordPower);
			else
				swordPower = static_cast<int8_t>(swordPower < 1 ? 1 : swordPower);

			return CString() >> (char)PlayerProp::SWORDIMAGE >> (char)swordPower;
		}

		case LevelItemType::FULLHEART: // fullheart
		{
			const auto heartMax = std::clamp(player->account.maxHitpoints + 1, 0, 20); // Hard limit of 20 hearts.
			return CString() >> (char)PlayerProp::FULLHEARTS >> (char)heartMax >> (char)PlayerProp::HALFHEARTS >> (char)(heartMax * 2);
		}

		case LevelItemType::SPINATTACK: // spinattack
		{
			auto status = player->getProp<PlayerProp::STATUS>().value;
			if (status & PLSTATUS_HASSPIN) return {};
			status |= PLSTATUS_HASSPIN;
			return CString() >> (char)PlayerProp::STATUS >> (char)status;
		}

		default:
			break;
	}

	return {};
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal
