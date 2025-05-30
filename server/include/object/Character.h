#ifndef CHARACTER_H
#define CHARACTER_H

#include <cstdint>
#include <string>
#include <array>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

struct Character
{
	int16_t pixelX = 488;	// 30.5
	int16_t pixelY = 480;	// 30
	int16_t pixelZ = 0;
	uint8_t ap = 50;
	uint8_t mp = 0;
	uint32_t gralats = 0;
	uint8_t hitpointsInHalves = 6;
	uint8_t bombs = 10;
	uint8_t arrows = 5;
	uint8_t bombPower = 1;
	uint8_t glovePower = 1;
	int8_t swordPower = 1;
	uint8_t shieldPower = 1;
	uint8_t bowPower = 1;
	uint8_t sprite = 2;
	std::array<uint8_t, 5> colors{ 2, 0, 10, 4, 18 };
	std::string nickName{ "default" };
	std::string gani{ "idle" };
	std::string chatMessage;
	std::string horseImage;
	std::string headImage{ "head0.png" };
	std::string bodyImage{ "body.png" };
	std::string swordImage{ "sword1.png" };
	std::string shieldImage{ "shield1.png" };
	std::string bowImage{ "bow1.png" };
	std::string ganiAttributes[30];
};

//----------------------------

#undef TRANSPARENT
enum class CharacterColors : uint8_t
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
constexpr size_t CHARACTERCOLORS_COUNT = static_cast<size_t>(CharacterColors::COUNT);

inline std::string getCharacterColorName(CharacterColors color)
{
	static const std::unordered_map<CharacterColors, std::string> colorNames =
	{
		{ CharacterColors::WHITE, "white" },
		{ CharacterColors::YELLOW, "yellow" },
		{ CharacterColors::ORANGE, "orange" },
		{ CharacterColors::PINK, "pink" },
		{ CharacterColors::RED, "red" },
		{ CharacterColors::DARKRED, "darkred" },
		{ CharacterColors::LIGHTGREEN, "lightgreen" },
		{ CharacterColors::GREEN, "green" },
		{ CharacterColors::DARKGREEN, "darkgreen" },
		{ CharacterColors::LIGHTBLUE, "lightblue" },
		{ CharacterColors::BLUE, "blue" },
		{ CharacterColors::DARKBLUE, "darkblue" },
		{ CharacterColors::BROWN, "brown" },
		{ CharacterColors::CYNOBER, "cynober" },
		{ CharacterColors::PURPLE, "purple" },
		{ CharacterColors::DARKPURPLE, "darkpurple" },
		{ CharacterColors::LIGHTGRAY, "lightgray" },
		{ CharacterColors::GRAY, "gray" },
		{ CharacterColors::BLACK, "black" },
		{ CharacterColors::TRANSPARENT, "transparent" },
	};

	if (colorNames.find(color) != colorNames.end())
		return colorNames.at(color);

	return {};
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // CHARACTER_H
