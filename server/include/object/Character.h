#ifndef CHARACTER_H
#define CHARACTER_H

#include <cstdint>
#include <string>

#include <CString.h>

struct Character
{
	float hitpoints = 3;
	uint8_t bombs = 10;
	uint8_t arrows = 5;
	uint8_t bombPower = 1;
	uint8_t glovePower = 1;
	uint8_t swordPower = 1;
	uint8_t shieldPower = 1;
	uint8_t bowPower = 1;
	uint8_t sprite = 2;
	uint8_t ap = 50;
	uint8_t colors[5] = { 2, 0, 10, 4, 18 };
	uint32_t gralats = 0;
	std::string nickName{ "default" };
	CString gani{ "idle" };
	CString chatMessage;
	CString horseImage;
	CString headImage{ "head0.png" };
	CString bodyImage{ "body.png" };
	CString swordImage{ "sword1.png" };
	CString shieldImage{ "shield1.png" };
	CString bowImage{ "bow1.png" };
	CString ganiAttributes[30];
};

#endif // CHARACTER_H
