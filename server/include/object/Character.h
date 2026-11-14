#ifndef CHARACTER_H
#define CHARACTER_H

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

/// @brief Represents a character.
struct Character
{
	int16_t localPixelX = 488;	// 30.5
	int16_t localPixelY = 480;	// 30
	std::optional<int16_t> localPixelZ;
	uint8_t mapX = 0;
	uint8_t mapY = 0;
	uint8_t ap = 50;
	uint8_t mp = 0;
	uint32_t gralats = 0;
	uint8_t hitpointsInHalves = 6;
	uint8_t hurtDeltaInHalves = 0;
	uint8_t bombs = 10;
	uint8_t arrows = 5;
	uint8_t bombPower = 1;
	uint8_t glovePower = 1; // NPC: 0-2, Player: 0-3
	int8_t swordPower = 1;
	uint8_t shieldPower = 1;
	uint8_t bowPower = 1;
	uint8_t sprite = 0;
	uint8_t direction = 2; // 0: up, 1: left, 2: down, 3: right
	clock::time_point lastHurtTime = clock::time_point::min();
	std::array<int8_t, 2> hurtPushDeltaInHalfPixels{0, 0};
	std::array<uint8_t, 8> colors{2, 0, 10, 4, 18, 18, 18, 18}; // 0-19 are ClassicColors, 20+ are HTMLColors
	std::string nickName{};
	std::string gani{"idle"};
	std::string chatMessage;
	std::string horseImage;
	std::string headImage{"head0.png"};
	std::string bodyImage{"body.png"};
	std::string swordImage{"sword1.png"};
	std::string shieldImage{"shield1.png"};
	std::string bowImage{"bow1.png"};
	std::array<std::string, 30> ganiAttributes;

	[[nodiscard]] LocalPixelPosition getLocalPosition() const noexcept
	{
		return {localPixelX, localPixelY, localPixelZ.value_or(0)};
	}

	[[nodiscard]] PixelPosition getGlobalPosition() const noexcept
	{
		return {static_cast<int32_t>((mapX * 1024) + localPixelX), static_cast<int32_t>((mapY * 1024) + localPixelY), static_cast<int32_t>(localPixelZ.value_or(0))};
	}

	[[nodiscard]] TilePosition getTilePosition() const noexcept
	{
		return {static_cast<float>(mapX * 64) + (static_cast<float>(localPixelX) / 16.0f), static_cast<float>(mapY * 64) + (static_cast<float>(localPixelY) / 16.0f), static_cast<float>(localPixelZ.value_or(0)) / 16.0f};
	}

	[[nodiscard]] MapPosition getMapPosition() const noexcept
	{
		return {mapX, mapY, 0};
	}

	// Represents a Z value that is not set.
	constexpr static int16_t InvalidZTiles = -51;
	constexpr static int16_t InvalidZPixels = InvalidZTiles * 16_i16;
	constexpr static std::array<int16_t, 2> ValidZRangeTiles = {-50, 170};
	constexpr static std::array<int16_t, 2> ValidZRangePixels = {ValidZRangeTiles[0] * 16_i16, ValidZRangeTiles[1] * 16_i16};
};

//----------------------------

/// @brief Defines the color slots for a character's appearance.
enum class ColorSlots : uint8_t
{
	SKIN = 0,
	COAT,
	SLEEVES,
	SHOES,
	BELT,
	// newworld
	PULLOVER,
	PANTS,
	BORDER,
	//
	COUNT
};
constexpr size_t COLORSLOTS_COUNT = static_cast<size_t>(ColorSlots::COUNT);

/// @brief Defines the palette indices for the character's color palette in classic mode.
enum class BodyPaletteIndicesClassic : uint8_t
{
	SKIN = 5,
	COAT = 6,
	SLEEVES = 4,
	TRANSPARENCY = 2,
	SHOES = 3,
	BELT = 1,
	BORDER = 0,
};
constexpr size_t BODYPALETTEINDICESCLASSIC_COUNT = 7;

/// @brief Defines the palette indices for the character's color palette in new world mode.
enum class BodyPaletteIndicesNewWorld : uint8_t
{
	SKIN = 0,
	COAT = 1,
	SLEEVES = 2,
	SHOES = 3,
	BELT = 4,
	PULLOVER = 5,
	PANTS = 6,
	BORDER = 7,
	TRANSPARENCY = 8,
};
constexpr size_t BODYPALETTEINDICESNEWWORLD_COUNT = 9;
constexpr size_t NW_PALETTE_DARKER_MULT = 2;
constexpr size_t NW_PALETTE_LIGHTER_MULT = 3;

/// @brief Defines the classic colors for a character's appearance.
enum class ClassicColors : uint8_t
{
	WHITE = 0,
	YELLOW,
	ORANGE,
	PINK,
	RED,
	DARKRED,
	LIGHTGREEN,
	GREEN,
	DARKGREEN,
	LIGHTBLUE,
	BLUE,
	DARKBLUE,
	BROWN,
	CYNOBER,
	PURPLE,
	DARKPURPLE,
	LIGHTGRAY,
	GRAY,
	BLACK,
	TRANSPARENT,
	//
	COUNT
};
constexpr size_t CLASSICCOLORS_COUNT = static_cast<size_t>(ClassicColors::COUNT);

/// @brief Gets the name of a classic color.
inline std::string_view getClassicColorName(const ClassicColors color)
{
	static const std::unordered_map<ClassicColors, std::string_view> colorNames =
	{
		{ClassicColors::WHITE, "white"sv},
		{ClassicColors::YELLOW, "yellow"sv},
		{ClassicColors::ORANGE, "orange"sv},
		{ClassicColors::PINK, "pink"sv},
		{ClassicColors::RED, "red"sv},
		{ClassicColors::DARKRED, "darkred"sv},
		{ClassicColors::LIGHTGREEN, "lightgreen"sv},
		{ClassicColors::GREEN, "green"sv},
		{ClassicColors::DARKGREEN, "darkgreen"sv},
		{ClassicColors::LIGHTBLUE, "lightblue"sv},
		{ClassicColors::BLUE, "blue"sv},
		{ClassicColors::DARKBLUE, "darkblue"sv},
		{ClassicColors::BROWN, "brown"sv},
		{ClassicColors::CYNOBER, "cynober"sv},
		{ClassicColors::PURPLE, "purple"sv},
		{ClassicColors::DARKPURPLE, "darkpurple"sv},
		{ClassicColors::LIGHTGRAY, "lightgray"sv},
		{ClassicColors::GRAY, "gray"sv},
		{ClassicColors::BLACK, "black"sv},
		{ClassicColors::TRANSPARENT, "transparent"sv},
	};

	if (colorNames.contains(color))
		return colorNames.at(color);

	return {};
}

/// @brief Gets the ClassicColors from a color name.
inline std::optional<ClassicColors> getClassicColorFromName(const std::string_view color)
{
	static const std::unordered_map<std::string_view, ClassicColors> colorNames =
	{
		{"white"sv, ClassicColors::WHITE},
		{"yellow"sv, ClassicColors::YELLOW},
		{"orange"sv, ClassicColors::ORANGE},
		{"pink"sv, ClassicColors::PINK},
		{"red"sv, ClassicColors::RED},
		{"darkred"sv, ClassicColors::DARKRED},
		{"lightgreen"sv, ClassicColors::LIGHTGREEN},
		{"green"sv, ClassicColors::GREEN},
		{"darkgreen"sv, ClassicColors::DARKGREEN},
		{"lightblue"sv, ClassicColors::LIGHTBLUE},
		{"blue"sv, ClassicColors::BLUE},
		{"darkblue"sv, ClassicColors::DARKBLUE},
		{"brown"sv, ClassicColors::BROWN},
		{"cynober"sv, ClassicColors::CYNOBER},
		{"purple"sv, ClassicColors::PURPLE},
		{"darkpurple"sv, ClassicColors::DARKPURPLE},
		{"lightgray"sv, ClassicColors::LIGHTGRAY},
		{"gray"sv, ClassicColors::GRAY},
		{"black"sv, ClassicColors::BLACK},
		{"transparent"sv, ClassicColors::TRANSPARENT},
	};

	if (colorNames.contains(color))
		return colorNames.at(color);
	if (const auto lower = string::toLower(color); colorNames.contains(lower))
		return colorNames.at(lower);

	return std::nullopt;
}

//----------------------------

/// @brief Defines the HTML colors for a character's appearance.
enum class HTMLColors : uint8_t
{
	ALICEBLUE = 0,
	ANTIQUEWHITE,
	AQUA,
	AQUAMARINE,
	AZURE,
	BEIGE,
	BISQUE,
	BLACK,
	BLANCHEDALMOND,
	BLUE,
	BLUEVIOLET,
	BROWN,
	BURLYWOOD,
	CADETBLUE,
	CHARTREUSE,
	CHOCOLATE,
	CORAL,
	CORNFLOWERBLUE,
	CORNSILK,
	CRIMSON,
	CYAN,
	DARKBLUE,
	DARKCYAN,
	DARKGOLDENROD,
	DARKGRAY,
	DARKGREEN,
	DARKGREY,
	DARKKHAKI,
	DARKMAGENTA,
	DARKOLIVEGREEN,
	DARKORANGE,
	DARKORCHID,
	DARKRED,
	DARKSALMON,
	DARKSEAGREEN,
	DARKSLATEBLUE,
	DARKSLATEGRAY,
	DARKSLATEGREY,
	DARKTURQUOISE,
	DARKVIOLET,
	DEEPPINK,
	DEEPSKYBLUE,
	DIMGRAY,
	DIMGREY,
	DODGERBLUE,
	FELDSPAR,
	FIREBRICK,
	FLORALWHITE,
	FORESTGREEN,
	FUCHSIA,
	GAINSBORO,
	GHOSTWHITE,
	GOLD,
	GOLDENROD,
	GRAY,
	GREEN,
	GREENYELLOW,
	GREY,
	HONEYDEW,
	HOTPINK,
	INDIANRED,
	INDIGO,
	IVORY,
	KHAKI,
	LAVENDER,
	LAVENDERBLUSH,
	LAWNGREEN,
	LEMONCHIFFON,
	LIGHTBLUE,
	LIGHTCORAL,
	LIGHTCYAN,
	LIGHTGOLDENRODYELLOW,
	LIGHTGRAY,
	LIGHTGREEN,
	LIGHTGREY,
	LIGHTPINK,
	LIGHTSALMON,
	LIGHTSEAGREEN,
	LIGHTSKYBLUE,
	LIGHTSLATEBLUE,
	LIGHTSLATEGRAY,
	LIGHTSLATEGREY,
	LIGHTSTEELBLUE,
	LIGHTYELLOW,
	LIME,
	LIMEGREEN,
	LINEN,
	MAGENTA,
	MAROON,
	MEDIUMAQUAMARINE,
	MEDIUMBLUE,
	MEDIUMORCHID,
	MEDIUMPURPLE,
	MEDIUMSEAGREEN,
	MEDIUMSLATEBLUE,
	MEDIUMSPRINGGREEN,
	MEDIUMTURQUOISE,
	MEDIUMVIOLETRED,
	MIDNIGHTBLUE,
	MINTCREAM,
	MISTYROSE,
	MOCCASIN,
	NAVAJOWHITE,
	NAVY,
	OLDLACE,
	OLIVE,
	OLIVEDRAB,
	ORANGE,
	ORANGERED,
	ORCHID,
	PALEGOLDENROD,
	PALEGREEN,
	PALETURQUOISE,
	PALEVIOLETRED,
	PAPAYAWHIP,
	PEACHPUFF,
	PERU,
	PINK,
	PLUM,
	POWDERBLUE,
	PURPLE,
	RED,
	ROSYBROWN,
	ROYALBLUE,
	SADDLEBROWN,
	SALMON,
	SANDYBROWN,
	SEAGREEN,
	SEASHELL,
	SIENNA,
	SILVER,
	SKYBLUE,
	SLATEBLUE,
	SLATEGRAY,
	SLATEGREY,
	SNOW,
	SPRINGGREEN,
	STEELBLUE,
	TAN,
	TEAL,
	THISTLE,
	TOMATO,
	TURQUOISE,
	VIOLET,
	VIOLETRED,
	WHEAT,
	WHITE,
	WHITESMOKE,
	YELLOW,
	YELLOWGREEN,

	COUNT
};
constexpr size_t HTMLCOLORS_COUNT = static_cast<size_t>(HTMLColors::COUNT);

/// @brief Gets the name of an HTML color.
inline std::string_view getHTMLColorName(const HTMLColors color)
{
	static const std::unordered_map<HTMLColors, std::string_view> colorNames =
	{
		{HTMLColors::ALICEBLUE, "aliceblue"sv},
		{HTMLColors::ANTIQUEWHITE, "antiquewhite"sv},
		{HTMLColors::AQUA, "aqua"sv},
		{HTMLColors::AQUAMARINE, "aquamarine"sv},
		{HTMLColors::AZURE, "azure"sv},
		{HTMLColors::BEIGE, "beige"sv},
		{HTMLColors::BISQUE, "bisque"sv},
		{HTMLColors::BLACK, "black"sv},
		{HTMLColors::BLANCHEDALMOND, "blanchedalmond"sv},
		{HTMLColors::BLUE, "blue"sv},
		{HTMLColors::BLUEVIOLET, "blueviolet"sv},
		{HTMLColors::BROWN, "brown"sv},
		{HTMLColors::BURLYWOOD, "burlywood"sv},
		{HTMLColors::CADETBLUE, "cadetblue"sv},
		{HTMLColors::CHARTREUSE, "chartreuse"sv},
		{HTMLColors::CHOCOLATE, "chocolate"sv},
		{HTMLColors::CORAL, "coral"sv},
		{HTMLColors::CORNFLOWERBLUE, "cornflowerblue"sv},
		{HTMLColors::CORNSILK, "cornsilk"sv},
		{HTMLColors::CRIMSON, "crimson"sv},
		{HTMLColors::CYAN, "cyan"sv},
		{HTMLColors::DARKBLUE, "darkblue"sv},
		{HTMLColors::DARKCYAN, "darkcyan"sv},
		{HTMLColors::DARKGOLDENROD, "darkgoldenrod"sv},
		{HTMLColors::DARKGRAY, "darkgray"sv},
		{HTMLColors::DARKGREEN, "darkgreen"sv},
		{HTMLColors::DARKGREY, "darkgrey"sv},
		{HTMLColors::DARKKHAKI, "darkkhaki"sv},
		{HTMLColors::DARKMAGENTA, "darkmagenta"sv},
		{HTMLColors::DARKOLIVEGREEN, "darkolivegreen"sv},
		{HTMLColors::DARKORANGE, "darkorange"sv},
		{HTMLColors::DARKORCHID, "darkorchid"sv},
		{HTMLColors::DARKRED, "darkred"sv},
		{HTMLColors::DARKSALMON, "darksalmon"sv},
		{HTMLColors::DARKSEAGREEN, "darkseagreen"sv},
		{HTMLColors::DARKSLATEBLUE, "darkslateblue"sv},
		{HTMLColors::DARKSLATEGRAY, "darkslategray"sv},
		{HTMLColors::DARKSLATEGREY, "darkslategrey"sv},
		{HTMLColors::DARKTURQUOISE, "darkturquoise"sv},
		{HTMLColors::DARKVIOLET, "darkviolet"sv},
		{HTMLColors::DEEPPINK, "deeppink"sv},
		{HTMLColors::DEEPSKYBLUE, "deepskyblue"sv},
		{HTMLColors::DIMGRAY, "dimgray"sv},
		{HTMLColors::DIMGREY, "dimgrey"sv},
		{HTMLColors::DODGERBLUE, "dodgerblue"sv},
		{HTMLColors::FELDSPAR, "feldspar"sv},
		{HTMLColors::FIREBRICK, "firebrick"sv},
		{HTMLColors::FLORALWHITE, "floralwhite"sv},
		{HTMLColors::FORESTGREEN, "forestgreen"sv},
		{HTMLColors::FUCHSIA, "fuchsia"sv},
		{HTMLColors::GAINSBORO, "gainsboro"sv},
		{HTMLColors::GHOSTWHITE, "ghostwhite"sv},
		{HTMLColors::GOLD, "gold"sv},
		{HTMLColors::GOLDENROD, "goldenrod"sv},
		{HTMLColors::GRAY, "gray"sv},
		{HTMLColors::GREEN, "green"sv},
		{HTMLColors::GREENYELLOW, "greenyellow"sv},
		{HTMLColors::GREY, "grey"sv},
		{HTMLColors::HONEYDEW, "honeydew"sv},
		{HTMLColors::HOTPINK, "hotpink"sv},
		{HTMLColors::INDIANRED, "indianred"sv},
		{HTMLColors::INDIGO, "indigo"sv},
		{HTMLColors::IVORY, "ivory"sv},
		{HTMLColors::KHAKI, "khaki"sv},
		{HTMLColors::LAVENDER, "lavender"sv},
		{HTMLColors::LAVENDERBLUSH, "lavenderblush"sv},
		{HTMLColors::LAWNGREEN, "lawngreen"sv},
		{HTMLColors::LEMONCHIFFON, "lemonchiffon"sv},
		{HTMLColors::LIGHTBLUE, "lightblue"sv},
		{HTMLColors::LIGHTCORAL, "lightcoral"sv},
		{HTMLColors::LIGHTCYAN, "lightcyan"sv},
		{HTMLColors::LIGHTGOLDENRODYELLOW, "lightgoldenrodyellow"sv},
		{HTMLColors::LIGHTGRAY, "lightgray"sv},
		{HTMLColors::LIGHTGREEN, "lightgreen"sv},
		{HTMLColors::LIGHTGREY, "lightgrey"sv},
		{HTMLColors::LIGHTPINK, "lightpink"sv},
		{HTMLColors::LIGHTSALMON, "lightsalmon"sv},
		{HTMLColors::LIGHTSEAGREEN, "lightseagreen"sv},
		{HTMLColors::LIGHTSKYBLUE, "lightskyblue"sv},
		{HTMLColors::LIGHTSLATEBLUE, "lightslateblue"sv},
		{HTMLColors::LIGHTSLATEGRAY, "lightslategray"sv},
		{HTMLColors::LIGHTSLATEGREY, "lightslategrey"sv},
		{HTMLColors::LIGHTSTEELBLUE, "lightsteelblue"sv},
		{HTMLColors::LIGHTYELLOW, "lightyellow"sv},
		{HTMLColors::LIME, "lime"sv},
		{HTMLColors::LIMEGREEN, "limegreen"sv},
		{HTMLColors::LINEN, "linen"sv},
		{HTMLColors::MAGENTA, "magenta"sv},
		{HTMLColors::MAROON, "maroon"sv},
		{HTMLColors::MEDIUMAQUAMARINE, "mediumaquamarine"sv},
		{HTMLColors::MEDIUMBLUE, "mediumblue"sv},
		{HTMLColors::MEDIUMORCHID, "mediumorchid"sv},
		{HTMLColors::MEDIUMPURPLE, "mediumpurple"sv},
		{HTMLColors::MEDIUMSEAGREEN, "mediumseagreen"sv},
		{HTMLColors::MEDIUMSLATEBLUE, "mediumslateblue"sv},
		{HTMLColors::MEDIUMSPRINGGREEN, "mediumspringgreen"sv},
		{HTMLColors::MEDIUMTURQUOISE, "mediumturquoise"sv},
		{HTMLColors::MEDIUMVIOLETRED, "mediumvioletred"sv},
		{HTMLColors::MIDNIGHTBLUE, "midnightblue"sv},
		{HTMLColors::MINTCREAM, "mintcream"sv},
		{HTMLColors::MISTYROSE, "mistyrose"sv},
		{HTMLColors::MOCCASIN, "moccasin"sv},
		{HTMLColors::NAVAJOWHITE, "navajowhite"sv},
		{HTMLColors::NAVY, "navy"sv},
		{HTMLColors::OLDLACE, "oldlace"sv},
		{HTMLColors::OLIVE, "olive"sv},
		{HTMLColors::OLIVEDRAB, "olivedrab"sv},
		{HTMLColors::ORANGE, "orange"sv},
		{HTMLColors::ORANGERED, "orangered"sv},
		{HTMLColors::ORCHID, "orchid"sv},
		{HTMLColors::PALEGOLDENROD, "palegoldenrod"sv},
		{HTMLColors::PALEGREEN, "palegreen"sv},
		{HTMLColors::PALETURQUOISE, "paleturquoise"sv},
		{HTMLColors::PALEVIOLETRED, "palevioletred"sv},
		{HTMLColors::PAPAYAWHIP, "papayawhip"sv},
		{HTMLColors::PEACHPUFF, "peachpuff"sv},
		{HTMLColors::PERU, "peru"sv},
		{HTMLColors::PINK, "pink"sv},
		{HTMLColors::PLUM, "plum"sv},
		{HTMLColors::POWDERBLUE, "powderblue"sv},
		{HTMLColors::PURPLE, "purple"sv},
		{HTMLColors::RED, "red"sv},
		{HTMLColors::ROSYBROWN, "rosybrown"sv},
		{HTMLColors::ROYALBLUE, "royalblue"sv},
		{HTMLColors::SADDLEBROWN, "saddlebrown"sv},
		{HTMLColors::SALMON, "salmon"sv},
		{HTMLColors::SANDYBROWN, "sandybrown"sv},
		{HTMLColors::SEAGREEN, "seagreen"sv},
		{HTMLColors::SEASHELL, "seashell"sv},
		{HTMLColors::SIENNA, "sienna"sv},
		{HTMLColors::SILVER, "silver"sv},
		{HTMLColors::SKYBLUE, "skyblue"sv},
		{HTMLColors::SLATEBLUE, "slateblue"sv},
		{HTMLColors::SLATEGRAY, "slategray"sv},
		{HTMLColors::SLATEGREY, "slategrey"sv},
		{HTMLColors::SNOW, "snow"sv},
		{HTMLColors::SPRINGGREEN, "springgreen"sv},
		{HTMLColors::STEELBLUE, "steelblue"sv},
		{HTMLColors::TAN, "tan"sv},
		{HTMLColors::TEAL, "teal"sv},
		{HTMLColors::THISTLE, "thistle"sv},
		{HTMLColors::TOMATO, "tomato"sv},
		{HTMLColors::TURQUOISE, "turquoise"sv},
		{HTMLColors::VIOLET, "violet"sv},
		{HTMLColors::VIOLETRED, "violetred"sv},
		{HTMLColors::WHEAT, "wheat"sv},
		{HTMLColors::WHITE, "white"sv},
		{HTMLColors::WHITESMOKE, "whitesmoke"sv},
		{HTMLColors::YELLOW, "yellow"sv},
		{HTMLColors::YELLOWGREEN, "yellowgreen"sv},
	};

	if (colorNames.contains(color))
		return colorNames.at(color);

	return {};
}

/// @brief Gets the HTMLColors from a color name.
inline std::optional<HTMLColors> getHTMLColorFromName(const std::string_view color)
{
	static const std::unordered_map<std::string_view, HTMLColors> colorNames =
	{
		{"aliceblue"sv, HTMLColors::ALICEBLUE},
		{"antiquewhite"sv, HTMLColors::ANTIQUEWHITE},
		{"aqua"sv, HTMLColors::AQUA},
		{"aquamarine"sv, HTMLColors::AQUAMARINE},
		{"azure"sv, HTMLColors::AZURE},
		{"beige"sv, HTMLColors::BEIGE},
		{"bisque"sv, HTMLColors::BISQUE},
		{"black"sv, HTMLColors::BLACK},
		{"blanchedalmond"sv, HTMLColors::BLANCHEDALMOND},
		{"blue"sv, HTMLColors::BLUE},
		{"blueviolet"sv, HTMLColors::BLUEVIOLET},
		{"brown"sv, HTMLColors::BROWN},
		{"burlywood"sv, HTMLColors::BURLYWOOD},
		{"cadetblue"sv, HTMLColors::CADETBLUE},
		{"chartreuse"sv, HTMLColors::CHARTREUSE},
		{"chocolate"sv, HTMLColors::CHOCOLATE},
		{"coral"sv, HTMLColors::CORAL},
		{"cornflowerblue"sv, HTMLColors::CORNFLOWERBLUE},
		{"cornsilk"sv, HTMLColors::CORNSILK},
		{"crimson"sv, HTMLColors::CRIMSON},
		{"cyan"sv, HTMLColors::CYAN},
		{"darkblue"sv, HTMLColors::DARKBLUE},
		{"darkcyan"sv, HTMLColors::DARKCYAN},
		{"darkgoldenrod"sv, HTMLColors::DARKGOLDENROD},
		{"darkgray"sv, HTMLColors::DARKGRAY},
		{"darkgreen"sv, HTMLColors::DARKGREEN},
		{"darkgrey"sv, HTMLColors::DARKGREY},
		{"darkkhaki"sv, HTMLColors::DARKKHAKI},
		{"darkmagenta"sv, HTMLColors::DARKMAGENTA},
		{"darkolivegreen"sv, HTMLColors::DARKOLIVEGREEN},
		{"darkorange"sv, HTMLColors::DARKORANGE},
		{"darkorchid"sv, HTMLColors::DARKORCHID},
		{"darkred"sv, HTMLColors::DARKRED},
		{"darksalmon"sv, HTMLColors::DARKSALMON},
		{"darkseagreen"sv, HTMLColors::DARKSEAGREEN},
		{"darkslateblue"sv, HTMLColors::DARKSLATEBLUE},
		{"darkslategray"sv, HTMLColors::DARKSLATEGRAY},
		{"darkslategrey"sv, HTMLColors::DARKSLATEGREY},
		{"darkturquoise"sv, HTMLColors::DARKTURQUOISE},
		{"darkviolet"sv, HTMLColors::DARKVIOLET},
		{"deeppink"sv, HTMLColors::DEEPPINK},
		{"deepskyblue"sv, HTMLColors::DEEPSKYBLUE},
		{"dimgray"sv, HTMLColors::DIMGRAY},
		{"dimgrey"sv, HTMLColors::DIMGREY},
		{"dodgerblue"sv, HTMLColors::DODGERBLUE},
		{"feldspar"sv, HTMLColors::FELDSPAR},
		{"firebrick"sv, HTMLColors::FIREBRICK},
		{"floralwhite"sv, HTMLColors::FLORALWHITE},
		{"forestgreen"sv, HTMLColors::FORESTGREEN},
		{"fuchsia"sv, HTMLColors::FUCHSIA},
		{"gainsboro"sv, HTMLColors::GAINSBORO},
		{"ghostwhite"sv, HTMLColors::GHOSTWHITE},
		{"gold"sv, HTMLColors::GOLD},
		{"goldenrod"sv, HTMLColors::GOLDENROD},
		{"gray"sv, HTMLColors::GRAY},
		{"green"sv, HTMLColors::GREEN},
		{"greenyellow"sv, HTMLColors::GREENYELLOW},
		{"grey"sv, HTMLColors::GREY},
		{"honeydew"sv, HTMLColors::HONEYDEW},
		{"hotpink"sv, HTMLColors::HOTPINK},
		{"indianred"sv, HTMLColors::INDIANRED},
		{"indigo"sv, HTMLColors::INDIGO},
		{"ivory"sv, HTMLColors::IVORY},
		{"khaki"sv, HTMLColors::KHAKI},
		{"lavender"sv, HTMLColors::LAVENDER},
		{"lavenderblush"sv, HTMLColors::LAVENDERBLUSH},
		{"lawngreen"sv, HTMLColors::LAWNGREEN},
		{"lemonchiffon"sv, HTMLColors::LEMONCHIFFON},
		{"lightblue"sv, HTMLColors::LIGHTBLUE},
		{"lightcoral"sv, HTMLColors::LIGHTCORAL},
		{"lightcyan"sv, HTMLColors::LIGHTCYAN},
		{"lightgoldenrodyellow"sv, HTMLColors::LIGHTGOLDENRODYELLOW},
		{"lightgray"sv, HTMLColors::LIGHTGRAY},
		{"lightgreen"sv, HTMLColors::LIGHTGREEN},
		{"lightgrey"sv, HTMLColors::LIGHTGREY},
		{"lightpink"sv, HTMLColors::LIGHTPINK},
		{"lightsalmon"sv, HTMLColors::LIGHTSALMON},
		{"lightseagreen"sv, HTMLColors::LIGHTSEAGREEN},
		{"lightskyblue"sv, HTMLColors::LIGHTSKYBLUE},
		{"lightslateblue"sv, HTMLColors::LIGHTSLATEBLUE},
		{"lightslategray"sv, HTMLColors::LIGHTSLATEGRAY},
		{"lightslategrey"sv, HTMLColors::LIGHTSLATEGREY},
		{"lightsteelblue"sv, HTMLColors::LIGHTSTEELBLUE},
		{"lightyellow"sv, HTMLColors::LIGHTYELLOW},
		{"lime"sv, HTMLColors::LIME},
		{"limegreen"sv, HTMLColors::LIMEGREEN},
		{"linen"sv, HTMLColors::LINEN},
		{"magenta"sv, HTMLColors::MAGENTA},
		{"maroon"sv, HTMLColors::MAROON},
		{"mediumaquamarine"sv, HTMLColors::MEDIUMAQUAMARINE},
		{"mediumblue"sv, HTMLColors::MEDIUMBLUE},
		{"mediumorchid"sv, HTMLColors::MEDIUMORCHID},
		{"mediumpurple"sv, HTMLColors::MEDIUMPURPLE},
		{"mediumseagreen"sv, HTMLColors::MEDIUMSEAGREEN},
		{"mediumslateblue"sv, HTMLColors::MEDIUMSLATEBLUE},
		{"mediumspringgreen"sv, HTMLColors::MEDIUMSPRINGGREEN},
		{"mediumturquoise"sv, HTMLColors::MEDIUMTURQUOISE},
		{"mediumvioletred"sv, HTMLColors::MEDIUMVIOLETRED},
		{"midnightblue"sv, HTMLColors::MIDNIGHTBLUE},
		{"mintcream"sv, HTMLColors::MINTCREAM},
		{"mistyrose"sv, HTMLColors::MISTYROSE},
		{"moccasin"sv, HTMLColors::MOCCASIN},
		{"navajowhite"sv, HTMLColors::NAVAJOWHITE},
		{"navy"sv, HTMLColors::NAVY},
		{"oldlace"sv, HTMLColors::OLDLACE},
		{"olive"sv, HTMLColors::OLIVE},
		{"olivedrab"sv, HTMLColors::OLIVEDRAB},
		{"orange"sv, HTMLColors::ORANGE},
		{"orangered"sv, HTMLColors::ORANGERED},
		{"orchid"sv, HTMLColors::ORCHID},
		{"palegoldenrod"sv, HTMLColors::PALEGOLDENROD},
		{"palegreen"sv, HTMLColors::PALEGREEN},
		{"paleturquoise"sv, HTMLColors::PALETURQUOISE},
		{"palevioletred"sv, HTMLColors::PALEVIOLETRED},
		{"papayawhip"sv, HTMLColors::PAPAYAWHIP},
		{"peachpuff"sv, HTMLColors::PEACHPUFF},
		{"peru"sv, HTMLColors::PERU},
		{"pink"sv, HTMLColors::PINK},
		{"plum"sv, HTMLColors::PLUM},
		{"powderblue"sv, HTMLColors::POWDERBLUE},
		{"purple"sv, HTMLColors::PURPLE},
		{"red"sv, HTMLColors::RED},
		{"rosybrown"sv, HTMLColors::ROSYBROWN},
		{"royalblue"sv, HTMLColors::ROYALBLUE},
		{"saddlebrown"sv, HTMLColors::SADDLEBROWN},
		{"salmon"sv, HTMLColors::SALMON},
		{"sandybrown"sv, HTMLColors::SANDYBROWN},
		{"seagreen"sv, HTMLColors::SEAGREEN},
		{"seashell"sv, HTMLColors::SEASHELL},
		{"sienna"sv, HTMLColors::SIENNA},
		{"silver"sv, HTMLColors::SILVER},
		{"skyblue"sv, HTMLColors::SKYBLUE},
		{"slateblue"sv, HTMLColors::SLATEBLUE},
		{"slategray"sv, HTMLColors::SLATEGRAY},
		{"slategrey"sv, HTMLColors::SLATEGREY},
		{"snow"sv, HTMLColors::SNOW},
		{"springgreen"sv, HTMLColors::SPRINGGREEN},
		{"steelblue"sv, HTMLColors::STEELBLUE},
		{"tan"sv, HTMLColors::TAN},
		{"teal"sv, HTMLColors::TEAL},
		{"thistle"sv, HTMLColors::THISTLE},
		{"tomato"sv, HTMLColors::TOMATO},
		{"turquoise"sv, HTMLColors::TURQUOISE},
		{"violet"sv, HTMLColors::VIOLET},
		{"violetred"sv, HTMLColors::VIOLETRED},
		{"wheat"sv, HTMLColors::WHEAT},
		{"white"sv, HTMLColors::WHITE},
		{"whitesmoke"sv, HTMLColors::WHITESMOKE},
		{"yellow"sv, HTMLColors::YELLOW},
		{"yellowgreen"sv, HTMLColors::YELLOWGREEN},
	};

	if (colorNames.contains(color))
		return colorNames.at(color);
	if (const auto lower = string::toLower(color); colorNames.contains(lower))
		return colorNames.at(lower);

	return std::nullopt;
}

//----------------------------

/// @brief Gets the name of a color from a property color ID.
inline std::string_view getColorNameFromId(const uint8_t colorId)
{
	if (colorId >= CLASSICCOLORS_COUNT + HTMLCOLORS_COUNT)
		return {};
	if (colorId >= CLASSICCOLORS_COUNT)
		return getHTMLColorName(ENUM<HTMLColors>(colorId - CLASSICCOLORS_COUNT));
	return getClassicColorName(ENUM<ClassicColors>(colorId));
}

/// @brief Gets a property color ID from a color name.
inline uint8_t getColorIdFromName(const std::string_view name)
{
	if (const auto color = getClassicColorFromName(name); color.has_value())
		return ENUM(color.value());
	if (const auto color = getHTMLColorFromName(name); color.has_value())
		return ENUM(color.value()) + CLASSICCOLORS_COUNT;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // CHARACTER_H
