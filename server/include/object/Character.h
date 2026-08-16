#ifndef CHARACTER_H
#define CHARACTER_H

#include <array>
#include <chrono>
#include <cstdint>
#include <string_view>
#include <string>
#include <unordered_map>

#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

struct Character
{
	int16_t localPixelX = 488;	// 30.5
	int16_t localPixelY = 480;	// 30
	int16_t localPixelZ = 0;
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
	uint8_t glovePower = 1;	// NPC: 0-2, Player: 0-3
	int8_t swordPower = 1;
	uint8_t shieldPower = 1;
	uint8_t bowPower = 1;
	uint8_t sprite = 0;
	uint8_t direction = 2;	// 0: up, 1: left, 2: down, 3: right
	clock::time_point lastHurtTime = clock::time_point::min();
	std::array<int8_t, 2> hurtPushDeltaInHalfPixels{ 0, 0 };
	std::array<uint8_t, 8> colors{ 2, 0, 10, 4, 18, 18, 18, 18 };  // 0-19 are ClassicColors, 20+ are HTMLColors
	std::string nickName{};
	std::string gani{ "idle" };
	std::string chatMessage;
	std::string horseImage;
	std::string headImage{ "head0.png" };
	std::string bodyImage{ "body.png" };
	std::string swordImage{ "sword1.png" };
	std::string shieldImage{ "shield1.png" };
	std::string bowImage{ "bow1.png" };
	std::array<std::string, 30> ganiAttributes;

	[[nodiscard]] LocalPixelPosition getLocalPosition() const noexcept
	{
		return {localPixelX, localPixelY, localPixelZ};
	}

	[[nodiscard]] PixelPosition getGlobalPosition() const noexcept
	{
		return {static_cast<int32_t>((mapX * 1024) + localPixelX), static_cast<int32_t>((mapY * 1024) + localPixelY), static_cast<int32_t>(localPixelZ)};
	}

	[[nodiscard]] TilePosition getTilePosition() const noexcept
	{
		return {static_cast<float>(mapX * 64) + (static_cast<float>(localPixelX) / 16.0f), static_cast<float>(mapY * 64) + (static_cast<float>(localPixelY) / 16.0f), static_cast<float>(localPixelZ) / 16.0f};
	}

	[[nodiscard]] MapPosition getMapPosition() const noexcept
	{
		return {mapX, mapY, 0};
	}
};

//----------------------------

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

	COUNT
};
constexpr size_t CLASSICCOLORS_COUNT = static_cast<size_t>(ClassicColors::COUNT);

inline std::string_view getClassicColorName(ClassicColors color)
{
	static const std::unordered_map<ClassicColors, std::string_view> colorNames =
	{
		{ ClassicColors::WHITE, "white"sv },
		{ ClassicColors::YELLOW, "yellow"sv },
		{ ClassicColors::ORANGE, "orange"sv },
		{ ClassicColors::PINK, "pink"sv },
		{ ClassicColors::RED, "red"sv },
		{ ClassicColors::DARKRED, "darkred"sv },
		{ ClassicColors::LIGHTGREEN, "lightgreen"sv },
		{ ClassicColors::GREEN, "green"sv },
		{ ClassicColors::DARKGREEN, "darkgreen"sv },
		{ ClassicColors::LIGHTBLUE, "lightblue"sv },
		{ ClassicColors::BLUE, "blue"sv },
		{ ClassicColors::DARKBLUE, "darkblue"sv },
		{ ClassicColors::BROWN, "brown"sv },
		{ ClassicColors::CYNOBER, "cynober"sv },
		{ ClassicColors::PURPLE, "purple"sv },
		{ ClassicColors::DARKPURPLE, "darkpurple"sv },
		{ ClassicColors::LIGHTGRAY, "lightgray"sv },
		{ ClassicColors::GRAY, "gray"sv },
		{ ClassicColors::BLACK, "black"sv },
		{ ClassicColors::TRANSPARENT, "transparent"sv },
	};

	if (colorNames.contains(color))
		return colorNames.at(color);

	return {};
}

//----------------------------

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

inline std::string_view getHTMLColorName(const HTMLColors color)
{
	static const std::unordered_map<HTMLColors, std::string_view> colorNames =
	{
		{ HTMLColors::ALICEBLUE, "aliceblue"sv },
		{ HTMLColors::ANTIQUEWHITE, "antiquewhite"sv },
		{ HTMLColors::AQUA, "aqua"sv },
		{ HTMLColors::AQUAMARINE, "aquamarine"sv },
		{ HTMLColors::AZURE, "azure"sv },
		{ HTMLColors::BEIGE, "beige"sv },
		{ HTMLColors::BISQUE, "bisque"sv },
		{ HTMLColors::BLACK, "black"sv },
		{ HTMLColors::BLANCHEDALMOND, "blanchedalmond"sv },
		{ HTMLColors::BLUE, "blue"sv },
		{ HTMLColors::BLUEVIOLET, "blueviolet"sv },
		{ HTMLColors::BROWN, "brown"sv },
		{ HTMLColors::BURLYWOOD, "burlywood"sv },
		{ HTMLColors::CADETBLUE, "cadetblue"sv },
		{ HTMLColors::CHARTREUSE, "chartreuse"sv },
		{ HTMLColors::CHOCOLATE, "chocolate"sv },
		{ HTMLColors::CORAL, "coral"sv },
		{ HTMLColors::CORNFLOWERBLUE, "cornflowerblue"sv },
		{ HTMLColors::CORNSILK, "cornsilk"sv },
		{ HTMLColors::CRIMSON, "crimson"sv },
		{ HTMLColors::CYAN, "cyan"sv },
		{ HTMLColors::DARKBLUE, "darkblue"sv },
		{ HTMLColors::DARKCYAN, "darkcyan"sv },
		{ HTMLColors::DARKGOLDENROD, "darkgoldenrod"sv },
		{ HTMLColors::DARKGRAY, "darkgray"sv },
		{ HTMLColors::DARKGREEN, "darkgreen"sv },
		{ HTMLColors::DARKGREY, "darkgrey"sv },
		{ HTMLColors::DARKKHAKI, "darkkhaki"sv },
		{ HTMLColors::DARKMAGENTA, "darkmagenta"sv },
		{ HTMLColors::DARKOLIVEGREEN, "darkolivegreen"sv },
		{ HTMLColors::DARKORANGE, "darkorange"sv },
		{ HTMLColors::DARKORCHID, "darkorchid"sv },
		{ HTMLColors::DARKRED, "darkred"sv },
		{ HTMLColors::DARKSALMON, "darksalmon"sv },
		{ HTMLColors::DARKSEAGREEN, "darkseagreen"sv },
		{ HTMLColors::DARKSLATEBLUE, "darkslateblue"sv },
		{ HTMLColors::DARKSLATEGRAY, "darkslategray"sv },
		{ HTMLColors::DARKSLATEGREY, "darkslategrey"sv },
		{ HTMLColors::DARKTURQUOISE, "darkturquoise"sv },
		{ HTMLColors::DARKVIOLET, "darkviolet"sv },
		{ HTMLColors::DEEPPINK, "deeppink"sv },
		{ HTMLColors::DEEPSKYBLUE, "deepskyblue"sv },
		{ HTMLColors::DIMGRAY, "dimgray"sv },
		{ HTMLColors::DIMGREY, "dimgrey"sv },
		{ HTMLColors::DODGERBLUE, "dodgerblue"sv },
		{ HTMLColors::FELDSPAR, "feldspar"sv },
		{ HTMLColors::FIREBRICK, "firebrick"sv },
		{ HTMLColors::FLORALWHITE, "floralwhite"sv },
		{ HTMLColors::FORESTGREEN, "forestgreen"sv },
		{ HTMLColors::FUCHSIA, "fuchsia"sv },
		{ HTMLColors::GAINSBORO, "gainsboro"sv },
		{ HTMLColors::GHOSTWHITE, "ghostwhite"sv },
		{ HTMLColors::GOLD, "gold"sv },
		{ HTMLColors::GOLDENROD, "goldenrod"sv },
		{ HTMLColors::GRAY, "gray"sv },
		{ HTMLColors::GREEN, "green"sv },
		{ HTMLColors::GREENYELLOW, "greenyellow"sv },
		{ HTMLColors::GREY, "grey"sv },
		{ HTMLColors::HONEYDEW, "honeydew"sv },
		{ HTMLColors::HOTPINK, "hotpink"sv },
		{ HTMLColors::INDIANRED, "indianred"sv },
		{ HTMLColors::INDIGO, "indigo"sv },
		{ HTMLColors::IVORY, "ivory"sv },
		{ HTMLColors::KHAKI, "khaki"sv },
		{ HTMLColors::LAVENDER, "lavender"sv },
		{ HTMLColors::LAVENDERBLUSH, "lavenderblush"sv },
		{ HTMLColors::LAWNGREEN, "lawngreen"sv },
		{ HTMLColors::LEMONCHIFFON, "lemonchiffon"sv },
		{ HTMLColors::LIGHTBLUE, "lightblue"sv },
		{ HTMLColors::LIGHTCORAL, "lightcoral"sv },
		{ HTMLColors::LIGHTCYAN, "lightcyan"sv },
		{ HTMLColors::LIGHTGOLDENRODYELLOW, "lightgoldenrodyellow"sv },
		{ HTMLColors::LIGHTGRAY, "lightgray"sv },
		{ HTMLColors::LIGHTGREEN, "lightgreen"sv },
		{ HTMLColors::LIGHTGREY, "lightgrey"sv },
		{ HTMLColors::LIGHTPINK, "lightpink"sv },
		{ HTMLColors::LIGHTSALMON, "lightsalmon"sv },
		{ HTMLColors::LIGHTSEAGREEN, "lightseagreen"sv },
		{ HTMLColors::LIGHTSKYBLUE, "lightskyblue"sv },
		{ HTMLColors::LIGHTSLATEBLUE, "lightslateblue"sv },
		{ HTMLColors::LIGHTSLATEGRAY, "lightslategray"sv },
		{ HTMLColors::LIGHTSLATEGREY, "lightslategrey"sv },
		{ HTMLColors::LIGHTSTEELBLUE, "lightsteelblue"sv },
		{ HTMLColors::LIGHTYELLOW, "lightyellow"sv },
		{ HTMLColors::LIME, "lime"sv },
		{ HTMLColors::LIMEGREEN, "limegreen"sv },
		{ HTMLColors::LINEN, "linen"sv },
		{ HTMLColors::MAGENTA, "magenta"sv },
		{ HTMLColors::MAROON, "maroon"sv },
		{ HTMLColors::MEDIUMAQUAMARINE, "mediumaquamarine"sv },
		{ HTMLColors::MEDIUMBLUE, "mediumblue"sv },
		{ HTMLColors::MEDIUMORCHID, "mediumorchid"sv },
		{ HTMLColors::MEDIUMPURPLE, "mediumpurple"sv },
		{ HTMLColors::MEDIUMSEAGREEN, "mediumseagreen"sv },
		{ HTMLColors::MEDIUMSLATEBLUE, "mediumslateblue"sv },
		{ HTMLColors::MEDIUMSPRINGGREEN, "mediumspringgreen"sv },
		{ HTMLColors::MEDIUMTURQUOISE, "mediumturquoise"sv },
		{ HTMLColors::MEDIUMVIOLETRED, "mediumvioletred"sv },
		{ HTMLColors::MIDNIGHTBLUE, "midnightblue"sv },
		{ HTMLColors::MINTCREAM, "mintcream"sv },
		{ HTMLColors::MISTYROSE, "mistyrose"sv },
		{ HTMLColors::MOCCASIN, "moccasin"sv },
		{ HTMLColors::NAVAJOWHITE, "navajowhite"sv },
		{ HTMLColors::NAVY, "navy"sv },
		{ HTMLColors::OLDLACE, "oldlace"sv },
		{ HTMLColors::OLIVE, "olive"sv },
		{ HTMLColors::OLIVEDRAB, "olivedrab"sv },
		{ HTMLColors::ORANGE, "orange"sv },
		{ HTMLColors::ORANGERED, "orangered"sv },
		{ HTMLColors::ORCHID, "orchid"sv },
		{ HTMLColors::PALEGOLDENROD, "palegoldenrod"sv },
		{ HTMLColors::PALEGREEN, "palegreen"sv },
		{ HTMLColors::PALETURQUOISE, "paleturquoise"sv },
		{ HTMLColors::PALEVIOLETRED, "palevioletred"sv },
		{ HTMLColors::PAPAYAWHIP, "papayawhip"sv },
		{ HTMLColors::PEACHPUFF, "peachpuff"sv },
		{ HTMLColors::PERU, "peru"sv },
		{ HTMLColors::PINK, "pink"sv },
		{ HTMLColors::PLUM, "plum"sv },
		{ HTMLColors::POWDERBLUE, "powderblue"sv },
		{ HTMLColors::PURPLE, "purple"sv },
		{ HTMLColors::RED, "red"sv },
		{ HTMLColors::ROSYBROWN, "rosybrown"sv },
		{ HTMLColors::ROYALBLUE, "royalblue"sv },
		{ HTMLColors::SADDLEBROWN, "saddlebrown"sv },
		{ HTMLColors::SALMON, "salmon"sv },
		{ HTMLColors::SANDYBROWN, "sandybrown"sv },
		{ HTMLColors::SEAGREEN, "seagreen"sv },
		{ HTMLColors::SEASHELL, "seashell"sv },
		{ HTMLColors::SIENNA, "sienna"sv },
		{ HTMLColors::SILVER, "silver"sv },
		{ HTMLColors::SKYBLUE, "skyblue"sv },
		{ HTMLColors::SLATEBLUE, "slateblue"sv },
		{ HTMLColors::SLATEGRAY, "slategray"sv },
		{ HTMLColors::SLATEGREY, "slategrey"sv },
		{ HTMLColors::SNOW, "snow"sv },
		{ HTMLColors::SPRINGGREEN, "springgreen"sv },
		{ HTMLColors::STEELBLUE, "steelblue"sv },
		{ HTMLColors::TAN, "tan"sv },
		{ HTMLColors::TEAL, "teal"sv },
		{ HTMLColors::THISTLE, "thistle"sv },
		{ HTMLColors::TOMATO, "tomato"sv },
		{ HTMLColors::TURQUOISE, "turquoise"sv },
		{ HTMLColors::VIOLET, "violet"sv },
		{ HTMLColors::VIOLETRED, "violetred"sv },
		{ HTMLColors::WHEAT, "wheat"sv },
		{ HTMLColors::WHITE, "white"sv },
		{ HTMLColors::WHITESMOKE, "whitesmoke"sv },
		{ HTMLColors::YELLOW, "yellow"sv },
		{ HTMLColors::YELLOWGREEN, "yellowgreen"sv },
	};

	if (colorNames.contains(color))
		return colorNames.at(color);

	return {};
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // CHARACTER_H
