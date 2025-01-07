#ifndef PLAYERPROPS_H
#define PLAYERPROPS_H

#include <array>
#include <cstdint>
#include <concepts>
#include <algorithm>

#include "BabyDI.h"

class Server;

enum PlayerProp : int
{
	PLPROP_NICKNAME = 0,
	PLPROP_MAXPOWER = 1,
	PLPROP_CURPOWER = 2,
	PLPROP_RUPEESCOUNT = 3,
	PLPROP_ARROWSCOUNT = 4,
	PLPROP_BOMBSCOUNT = 5,
	PLPROP_GLOVEPOWER = 6,
	PLPROP_BOMBPOWER = 7,
	PLPROP_SWORDPOWER = 8,
	PLPROP_SHIELDPOWER = 9,
	PLPROP_GANI = 10, // PLPROP_BOWGIF in pre-2.x
	PLPROP_HEADGIF = 11,
	PLPROP_CURCHAT = 12,
	PLPROP_COLORS = 13,
	PLPROP_ID = 14,
	PLPROP_X = 15,
	PLPROP_Y = 16,
	PLPROP_SPRITE = 17,
	PLPROP_STATUS = 18,
	PLPROP_CARRYSPRITE = 19,
	PLPROP_CURLEVEL = 20,
	PLPROP_HORSEGIF = 21,
	PLPROP_HORSEBUSHES = 22,
	PLPROP_EFFECTCOLORS = 23,
	PLPROP_CARRYNPC = 24,
	PLPROP_APCOUNTER = 25,
	PLPROP_MAGICPOINTS = 26,
	PLPROP_KILLSCOUNT = 27,
	PLPROP_DEATHSCOUNT = 28,
	PLPROP_ONLINESECS = 29,
	PLPROP_IPADDR = 30,
	PLPROP_UDPPORT = 31,
	PLPROP_ALIGNMENT = 32,
	PLPROP_ADDITFLAGS = 33,
	PLPROP_ACCOUNTNAME = 34,
	PLPROP_BODYIMG = 35,
	PLPROP_RATING = 36,
	PLPROP_GATTRIB1 = 37,
	PLPROP_GATTRIB2 = 38,
	PLPROP_GATTRIB3 = 39,
	PLPROP_GATTRIB4 = 40,
	PLPROP_GATTRIB5 = 41,
	PLPROP_ATTACHNPC = 42,
	PLPROP_GMAPLEVELX = 43,
	PLPROP_GMAPLEVELY = 44,
	PLPROP_Z = 45,
	PLPROP_GATTRIB6 = 46,
	PLPROP_GATTRIB7 = 47,
	PLPROP_GATTRIB8 = 48,
	PLPROP_GATTRIB9 = 49,
	PLPROP_JOINLEAVELVL = 50,
	PLPROP_PCONNECTED = 51,
	PLPROP_PLANGUAGE = 52,
	PLPROP_PSTATUSMSG = 53,
	PLPROP_GATTRIB10 = 54,
	PLPROP_GATTRIB11 = 55,
	PLPROP_GATTRIB12 = 56,
	PLPROP_GATTRIB13 = 57,
	PLPROP_GATTRIB14 = 58,
	PLPROP_GATTRIB15 = 59,
	PLPROP_GATTRIB16 = 60,
	PLPROP_GATTRIB17 = 61,
	PLPROP_GATTRIB18 = 62,
	PLPROP_GATTRIB19 = 63,
	PLPROP_GATTRIB20 = 64,
	PLPROP_GATTRIB21 = 65,
	PLPROP_GATTRIB22 = 66,
	PLPROP_GATTRIB23 = 67,
	PLPROP_GATTRIB24 = 68,
	PLPROP_GATTRIB25 = 69,
	PLPROP_GATTRIB26 = 70,
	PLPROP_GATTRIB27 = 71,
	PLPROP_GATTRIB28 = 72,
	PLPROP_GATTRIB29 = 73,
	PLPROP_GATTRIB30 = 74,
	PLPROP_OSTYPE = 75,       // 2.19+
	PLPROP_TEXTCODEPAGE = 76, // 2.19+
	PLPROP_ONLINESECS2 = 77,
	PLPROP_X2 = 78,
	PLPROP_Y2 = 79,
	PLPROP_Z2 = 80,
	PLPROP_PLAYERLISTCATEGORY = 81, // {GCHAR flag} - flag 0 places in playerlist, flag 1 places in servers tab, flag 3 places in channels tab (unconfirmed)

	// In Graal v5, where players have the Graal######## accounts, this is their chosen account alias (community name.)
	PLPROP_COMMUNITYNAME = 82,
};
constexpr int PROPSCOUNT = 83;

enum class PlayerListCategory : uint8_t
{
	PLAYERLIST = 0b0000,
	SERVERS    = 0b0001,
	CHANNELS   = 0b0011,
};

struct PropLimits
{
	static constexpr uint8_t HeadImageLength = 123;
	static constexpr uint8_t BodyImageLength = 223;
	static constexpr uint8_t SwordImageLength = 223;
	static constexpr uint8_t ShieldImageLength = 223;
	static constexpr uint8_t HorseImageLength = 119;
	static constexpr uint8_t GaniLength = 223;
	static constexpr uint8_t ChatMessageLength = 223;
	static constexpr uint8_t MaxHitpoints = 20;
	static constexpr uint8_t MaxArrows = 99;
	static constexpr uint8_t MaxBombs = 99;
	static constexpr uint8_t MaxMP = 100;
	static constexpr uint8_t MaxAP = 100;

	// Sword, battleaxe, lizardsword, goldensword.
	static constexpr uint8_t MaxSwordPower = 20;

	// Shield, mirrorshield, lizardshield.
	static constexpr uint8_t MaxShieldPower = 3;

	// Glove1, glove2.
	static constexpr uint8_t MaxGlovePower = 2;

	// Bomb, joltbomb, superbomb.
	static constexpr uint8_t MaxBombPower = 3;

	// Bow, fireball, fireblast, nukeshot.
	static constexpr uint8_t MaxBowPower = 4;

	static auto apply(std::integral auto value, std::integral auto min, std::integral auto max) -> decltype(value)
	{
		using T = decltype(value);
		return std::clamp(value, static_cast<T>(min), static_cast<T>(max));
	}

	static auto apply(std::integral auto value, std::integral auto max) -> decltype(value)
	{
		using T = decltype(value);
		return std::clamp(value, static_cast<T>(0), static_cast<T>(max));
	}

	static auto apply(std::string_view value, size_t maxLength)
	{
		if (value.length() > maxLength)
			return value.substr(0, maxLength);
		return value;
	}

	static uint8_t applyMaxHitpoints(uint8_t maxHitpoints);
	static int8_t applySwordPower(int8_t swordPower);
	static uint8_t applyShieldPower(uint8_t shieldPower);

	BabyDI_INJECT(Server, m_server);
};

// Gani attributes in order of their property number.
constexpr std::array<int, 30> GaniAttributePropList = { 37, 38, 39, 40, 41, 46, 47, 48, 49, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74 };

using PropList = std::array<bool, PROPSCOUNT>;

// Sent to the player on login.
constexpr PropList loginPropsClientSelf = // bool __sendLogin[PROPSCOUNT] = {
{
	false, true, true, true, true, true,    // 0-5
	true, false, true, true, true, true,    // 6-11
	false, true, false, false, false, true, // 12-17
	true, false, false, true, true, true,   // 18-23
	false, true, true, false, false, false, // 24-29
	false, false, true, false, true, true,  // 30-35
	true, true, true, true, true, true,     // 36-41
	false, false, false, false, true, true, // 42-47
	true, true, false, false, false, false, // 48-53
	true, true, true, true, true, true,     // 54-59
	true, true, true, true, true, true,     // 60-65
	true, true, true, true, true, true,     // 66-71
	true, true, true, false, false, false,  // 72-77
	false, false, false, false, true,       // 78-82
};

// Sent to nearby players when a player logs in.
constexpr PropList loginPropsClientOthers = // bool __getLogin[PROPSCOUNT] = {
{
	true, false, false, false, false, false, // 0-5
	false, false, true, true, true, true,    // 6-11
	true, true, false, true, true, true,     // 12-17
	true, true, true, true, false, false,    // 18-23
	true, false, false, false, false, false, // 24-29
	true, true, true, false, true, true,     // 30-35
	true, true, true, true, true, true,      // 36-41
	false, true, true, true, true, true,     // 42-47
	true, true, true, false, false, true,    // 48-53
	true, true, true, true, true, true,      // 54-59
	true, true, true, true, true, true,      // 60-65
	true, true, true, true, true, true,      // 66-71
	true, true, true, false, false, false,   // 72-77
	true, true, true, false, true,           // 78-82
};

// Login props for NC that get sent to other players (currently unused, most likely incorrect).
constexpr PropList loginPropsNC =
{
	true, true, true, true, true, true,   // 0-5
	true, true, true, true, true, true,   // 6-11
	true, true, true, true, true, true,   // 12-17
	true, true, true, true, true, true,   // 18-23
	true, true, true, true, true, true,   // 24-29
	true, false, true, true, true, true,  // 30-35
	true, true, true, true, true, true,   // 36-41
	false, true, true, true, true, true,  // 42-47
	true, true, true, false, true, true,  // 48-53
	true, true, true, true, true, true,   // 54-59
	true, true, true, true, true, true,   // 60-65
	true, true, true, true, true, true,   // 66-71
	true, true, true, true, false, false, // 72-77
	true, true, true, false, false,       // 78-82
};

// Login props for RC that get sent to other players.
constexpr PropList loginPropsRC = // bool __getRCLogin[PROPSCOUNT] = {
{
	true, false, false, false, false, false,  // 0-5
	false, false, false, false, false, true,  // 6-11
	false, false, false, false, false, false, // 12-17
	true, false, true, false, false, false,   // 18-23
	false, false, false, false, false, false, // 24-29
	true, true, false, false, true, false,    // 30-35
	false, false, false, false, false, false, // 36-41
	false, false, false, false, false, false, // 42-47
	false, false, false, false, false, true,  // 48-53
	false, false, false, false, false, false, // 54-59
	false, false, false, false, false, false, // 60-65
	false, false, false, false, false, false, // 66-71
	false, false, false, false, false, false, // 72-77
	false, false, false, false, true,         // 78-82
};

// When one of these props change, they are sent to nearby players.
constexpr PropList clientPropsSharedLocal = // bool __sendLocal[PROPSCOUNT] = {
{
	false, false, true, false, false, false, // 0-5
	false, false, true, true, true, true,    // 6-11
	true, true, false, true, true, true,     // 12-17
	true, true, true, true, false, false,    // 18-23
	true, true, false, false, false, false,  // 24-29
	true, true, true, false, true, true,     // 30-35
	true, true, true, true, true, true,      // 36-41
	false, true, true, true, true, true,     // 42-47
	true, true, true, false, false, true,    // 48-53
	true, true, true, true, true, true,      // 54-59
	true, true, true, true, true, true,      // 60-65
	true, true, true, true, true, true,      // 66-71
	true, true, true, false, false, false,   // 72-77
	true, true, true, false, true,           // 78-82
};

// When the RC views a player's account, these props are sent.
constexpr PropList clientPropsForRCView = //bool __playerPropsRC[PROPSCOUNT] = {
{
	true, true, true, true, true, true,       // 0-5
	true, false, true, true, true, true,      // 6-11
	false, true, false, true, true, false,    // 12-17
	true, false, true, false, false, false,   // 18-23
	false, false, true, true, true, true,     // 24-29
	true, false, true, false, true, true,     // 30-35
	true, false, false, false, false, false,  // 36-41
	false, false, false, false, false, false, // 42-47
	false, false, false, false, false, false, // 48-53
	false, false, false, false, false, false, // 54-59
	false, false, false, false, false, false, // 60-65
	false, false, false, false, false, false, // 66-71
	false, false, false, false, false, false, // 72-77
	false, false, false, false, false,        // 78-82
};

#endif // PLAYERPROPS_H
