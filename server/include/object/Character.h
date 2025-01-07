#ifndef CHARACTER_H
#define CHARACTER_H

#include <cstdint>
#include <string>

#include <CString.h>

struct Character
{
	int16_t pixelX = 488;	// 30.5
	int16_t pixelY = 480;	// 30
	int16_t pixelZ = 0;
	uint8_t hitpointsInHalves = 6;
	uint8_t bombs = 10;
	uint8_t arrows = 5;
	uint8_t bombPower = 1;
	uint8_t glovePower = 1;
	int8_t swordPower = 1;
	uint8_t shieldPower = 1;
	uint8_t bowPower = 1;
	uint8_t sprite = 2;
	uint8_t ap = 50;
	uint8_t mp = 0;
	std::array<uint8_t, 5> colors{ 2, 0, 10, 4, 18 };
	uint32_t gralats = 0;
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

#endif // CHARACTER_H
