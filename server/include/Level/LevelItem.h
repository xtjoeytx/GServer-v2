#ifndef TLEVELITEM_H
#define TLEVELITEM_H

#include "CString.h"
#include "CTimeout.h"
#include <ctime>

enum class LevelItemType
{
	INVALID = -1,

	GREENRUPEE   = 0,
	BLUERUPEE    = 1,
	REDRUPEE     = 2,
	BOMBS        = 3,
	DARTS        = 4,
	HEART        = 5,
	GLOVE1       = 6,
	BOW          = 7,
	BOMB         = 8,
	SHIELD       = 9,
	SWORD        = 10,
	FULLHEART    = 11,
	SUPERBOMB    = 12,
	BATTLEAXE    = 13,
	GOLDENSWORD  = 14,
	MIRRORSHIELD = 15,
	GLOVE2       = 16,
	LIZARDSHIELD = 17,
	LIZARDSWORD  = 18,
	GOLDRUPEE    = 19,
	FIREBALL     = 20,
	FIREBLAST    = 21,
	NUKESHOT     = 22,
	JOLTBOMB     = 23,
	SPINATTACK   = 24
};

class TPlayer;
class TLevelItem
{
public:
	TLevelItem(float pX, float pY, LevelItemType pItem) : x(pX), y(pY), item(pItem), modTime(time(0))
	{
		timeout.setTimeout(10);
	}

	// Return the packet to be sent to the player.
	CString getItemStr() const;

	// Get functions.
	float getX() const { return x; }
	float getY() const { return y; }
	LevelItemType getItem() const { return item; }
	time_t getModTime() const { return modTime; }

	CTimeout timeout;

	// Static functions.
	static LevelItemType getItemId(signed char itemId);
	static LevelItemType getItemId(const std::string& pItemName);
	static std::string getItemName(LevelItemType itemId);
	static CString getItemPlayerProp(LevelItemType itemType, TPlayer* player);
	static CString getItemPlayerProp(const std::string& pItemName, TPlayer* player);
	static constexpr auto getItemTypeId(LevelItemType val);

	static bool isRupeeType(LevelItemType itemType);
	static uint16_t GetRupeeCount(LevelItemType type);

private:
	float x;
	float y;
	LevelItemType item;
	time_t modTime;
};

inline CString TLevelItem::getItemPlayerProp(const std::string& pItemName, TPlayer* player)
{
	return getItemPlayerProp(TLevelItem::getItemId(pItemName), player);
}

constexpr auto TLevelItem::getItemTypeId(LevelItemType val)
{
	return static_cast<std::underlying_type<LevelItemType>::type>(val);
}

inline uint16_t TLevelItem::GetRupeeCount(LevelItemType type)
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

inline bool TLevelItem::isRupeeType(LevelItemType itemType)
{
	return GetRupeeCount(itemType) > 0;
}

#endif // TLEVELITEM_H
