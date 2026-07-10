#ifndef LEVELITEM_H
#define LEVELITEM_H

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#include <string>
#include <type_traits>

#include <CString.h>

#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/generator/TimeoutGenerator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

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

inline constexpr std::array<std::string_view, 25> ItemNames =
{
	"greenrupee"sv, "bluerupee"sv, "redrupee"sv, "bombs"sv, "darts"sv,
	"heart"sv, "glove1"sv, "bow"sv, "bomb"sv, "shield"sv,
	"sword"sv, "fullheart"sv, "superbomb"sv, "battleaxe"sv, "goldensword"sv,
	"mirrorshield"sv, "glove2"sv, "lizardshield"sv, "lizardsword"sv, "goldrupee"sv,
	"fireball"sv, "fireblast"sv, "nukeshot"sv, "joltbomb"sv, "spinattack"sv
};

/// @brief Level items disappear after 8.2 seconds;
constexpr clock::duration LevelItemTimeout = std::chrono::milliseconds(8200);

class Player;
struct LevelItem
{
	static LevelItemType getItemId(signed char itemId);
	static LevelItemType getItemId(const std::string& pItemName);
	static std::string getItemName(LevelItemType itemId);
	static CString getItemPlayerProp(LevelItemType itemType, Player* player);
	static CString getItemPlayerProp(const std::string& pItemName, Player* player);
	static constexpr auto getItemTypeId(LevelItemType val);
	static bool isRupeeType(LevelItemType itemType);
	static uint16_t GetRupeeCount(LevelItemType type);

	float getTileX() const { return position.x() / 16.0f; }
	float getTileY() const { return position.y() / 16.0f; }

	PixelPosition position;
	LevelItemType item;
	clock::time_point modTime;
	TimeoutGenerator timeout;

	[[inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;
};

//----------------------------

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

//----------------------------

inline void LevelItem::constructScriptParameters()
{
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"x"sv, std::nullopt, std::ref(position.x()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"y"sv, std::nullopt, std::ref(position.y()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::TimeoutProperty{"time"sv, std::ref(timeout)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"type"sv, std::nullopt, std::ref(item)});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELITEM_H
