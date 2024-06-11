#ifndef TLEVELITEM_H
#define TLEVELITEM_H

#include "CString.h"
#include "CTimeout.h"
#include <ctime>

enum class LevelItemType
{
	INVALID = -1,

	GREENRUPEE = 0,
	BLUERUPEE = 1,
	REDRUPEE = 2,
	BOMBS = 3,
	DARTS = 4,
	HEART = 5,
	GLOVE1 = 6,
	BOW = 7,
	BOMB = 8,
	SHIELD = 9,
	SWORD = 10,
	FULLHEART = 11,
	SUPERBOMB = 12,
	BATTLEAXE = 13,
	GOLDENSWORD = 14,
	MIRRORSHIELD = 15,
	GLOVE2 = 16,
	LIZARDSHIELD = 17,
	LIZARDSWORD = 18,
	GOLDRUPEE = 19,
	FIREBALL = 20,
	FIREBLAST = 21,
	NUKESHOT = 22,
	JOLTBOMB = 23,
	SPINATTACK = 24
};

class Player;

class LevelItem
{
public:
	LevelItem(float pX, float pY, LevelItemType pItem) : m_x(pX), m_y(pY), m_item(pItem), m_modTime(time(0))
	{
		timeout.setTimeout(10);
	}

	// Return the packet to be sent to the player.
	CString getItemStr() const;

	// Get functions.
	float getX() const { return m_x; }

	float getY() const { return m_y; }

	LevelItemType getItem() const { return m_item; }

	time_t getModTime() const { return m_modTime; }

	CTimeout timeout;

	// Static functions.
	static LevelItemType getItemId(signed char itemId);

	static LevelItemType getItemId(const std::string& pItemName);

	static std::string getItemName(LevelItemType itemId);

	static CString getItemPlayerProp(LevelItemType itemType, Player* player);

	static CString getItemPlayerProp(const std::string& pItemName, Player* player);

	static constexpr auto getItemTypeId(LevelItemType val);

	static bool isRupeeType(LevelItemType itemType);

	static uint16_t GetRupeeCount(LevelItemType type);

private:
	float m_x;
	float m_y;
	LevelItemType m_item;
	time_t m_modTime;
};

inline CString LevelItem::getItemPlayerProp(const std::string& pItemName, Player* player)
{
	return getItemPlayerProp(LevelItem::getItemId(pItemName), player);
}

constexpr auto LevelItem::getItemTypeId(LevelItemType val)
{
	return static_cast<std::underlying_type<LevelItemType>::type>(val);
}

inline uint16_t LevelItem::GetRupeeCount(LevelItemType type)
{
	switch (type)
	{
		case LevelItemType::GREENRUPEE:
			return 1;
		case LevelItemType::BLUERUPEE:
			return 5;
		case LevelItemType::REDRUPEE:
			return 30;
		case LevelItemType::GOLDRUPEE:
			return 100;
		default:
			return 0;
	}
}

inline bool LevelItem::isRupeeType(LevelItemType itemType)
{
	return GetRupeeCount(itemType) > 0;
}

#endif // TLEVELITEM_H
