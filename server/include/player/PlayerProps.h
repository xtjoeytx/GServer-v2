#ifndef PLAYERPROPS_H
#define PLAYERPROPS_H

#include <algorithm>
#include <array>
#include <bitset>
#include <concepts>
#include <cstdint>

#include "BabyDI.h"

#include "utilities/inplace_vector.h"

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

enum class PlayerProp : uint8_t
{
	NICKNAME = 0,
	MAXPOWER = 1,
	CURPOWER = 2,
	RUPEESCOUNT = 3,
	ARROWSCOUNT = 4,
	BOMBSCOUNT = 5,
	GLOVEPOWER = 6,
	BOMBPOWER = 7,
	SWORDPOWER = 8,
	SHIELDPOWER = 9,
	GANI = 10, // PLPROP_BOWGIF in pre-2.x
	HEADGIF = 11,
	CURCHAT = 12,
	COLORS = 13,
	ID = 14,
	X = 15,
	Y = 16,
	SPRITE = 17,
	STATUS = 18,
	CARRYSPRITE = 19,
	CURLEVEL = 20,
	HORSEGIF = 21,
	HORSEBUSHES = 22,
	EFFECTCOLORS = 23,
	CARRYNPC = 24,
	APCOUNTER = 25,
	MAGICPOINTS = 26,
	KILLSCOUNT = 27,
	DEATHSCOUNT = 28,
	ONLINESECS = 29,
	IPADDR = 30,
	UDPPORT = 31,
	ALIGNMENT = 32,
	ADDITFLAGS = 33,
	ACCOUNTNAME = 34,
	BODYIMG = 35,
	RATING = 36,
	GATTRIB1 = 37,
	GATTRIB2 = 38,
	GATTRIB3 = 39,
	GATTRIB4 = 40,
	GATTRIB5 = 41,
	ATTACHNPC = 42,
	GMAPLEVELX = 43,
	GMAPLEVELY = 44,
	Z = 45,
	GATTRIB6 = 46,
	GATTRIB7 = 47,
	GATTRIB8 = 48,
	GATTRIB9 = 49,
	JOINLEAVELVL = 50,
	PCONNECTED = 51,
	PLANGUAGE = 52,
	PSTATUSMSG = 53,
	GATTRIB10 = 54,
	GATTRIB11 = 55,
	GATTRIB12 = 56,
	GATTRIB13 = 57,
	GATTRIB14 = 58,
	GATTRIB15 = 59,
	GATTRIB16 = 60,
	GATTRIB17 = 61,
	GATTRIB18 = 62,
	GATTRIB19 = 63,
	GATTRIB20 = 64,
	GATTRIB21 = 65,
	GATTRIB22 = 66,
	GATTRIB23 = 67,
	GATTRIB24 = 68,
	GATTRIB25 = 69,
	GATTRIB26 = 70,
	GATTRIB27 = 71,
	GATTRIB28 = 72,
	GATTRIB29 = 73,
	GATTRIB30 = 74,
	OSTYPE = 75,       // 2.19+
	TEXTCODEPAGE = 76, // 2.19+
	ONLINESECS2 = 77,
	X2 = 78,
	Y2 = 79,
	Z2 = 80,
	PLAYERLISTCATEGORY = 81, // {GCHAR flag} - flag 0 places in playerlist, flag 1 places in servers tab, flag 3 places in channels tab (unconfirmed)

	// In Graal v5, where players have the Graal######## accounts, this is their chosen account alias (community name.)
	COMMUNITYNAME = 82,

	PLAYERPROP_COUNT
};
constexpr int PLAYERPROP_COUNT = static_cast<int>(PlayerProp::PLAYERPROP_COUNT);

enum class PlayerListCategory : uint8_t
{
	PLAYERLIST = 0b0000,
	SERVERS    = 0b0001,
	CHANNELS   = 0b0011,
};

// Gani attributes in order of their property number.
inline constexpr std::array<int, 30> GaniAttributePropList = { 37, 38, 39, 40, 41, 46, 47, 48, 49, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74 };

using PropList = std::array<bool, PLAYERPROP_COUNT>;

// Sent to the player on login.
inline constexpr PropList loginPropsClientSelf =
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
inline constexpr PropList loginPropsClientOthers =
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
inline constexpr PropList loginPropsNC =
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
inline constexpr PropList loginPropsRC =
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
inline constexpr PropList clientPropsSharedLocal =
{
	true, false, true, false, false, false,   // 0-5
	false, false, true, true, true, true,     // 6-11
	true, true, false, true, true, true,      // 12-17
	true, true, true, true, false, true,      // 18-23
	true, false, false, false, false, false,  // 24-29
	true, true, true, false, true, true,      // 30-35
	true, true, true, true, true, true,       // 36-41
	true, true, true, true, true, true,       // 42-47
	true, true, true, false, false, true,     // 48-53
	true, true, true, true, true, true,       // 54-59
	true, true, true, true, true, true,       // 60-65
	true, true, true, true, true, true,       // 66-71
	true, true, true, false, false, false,    // 72-77
	false, false, false, true, true,          // 78-82
};

// When the RC views a player's account, these props are sent.
inline constexpr PropList clientPropsForRCView =
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

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // PLAYERPROPS_H
