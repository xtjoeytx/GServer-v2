#ifndef SCRIPTTYPES_H
#define SCRIPTTYPES_H

#include <cstdint>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

/// The type of script.
enum class ScriptType
{
	NPC,
	CLASS,
	WEAPON,
	SERVER,

	COUNT
};
constexpr size_t SCRIPTTYPE_COUNT = static_cast<size_t>(ScriptType::COUNT);

/// The script events known by the server.
enum class ScriptEventType : uint8_t
{
	CUSTOM = 0,
	CREATED,
	INITIALIZED,
	PLAYERLOGIN,
	PLAYERLOGOUT,
	PLAYERENTERS,
	PLAYERLEAVES,
	PLAYERTOUCHSME,
	PLAYERTOUCHSOTHER,
	PLAYERLAYSITEM,
	PLAYERCHATS,
	PLAYERDIES,
	PLAYERENDREADING,
	WEAPONFIRED,
	FIREDONHORSE,
	COMPUSDIED,
	WARPED,
	NPCWARPED,
	EXPLODED,
	WASHIT,
	WASSHOT,
	WASPELT,
	TIMEOUT,
	//
	SERVERLISTCONNECT,

	COUNT
};
constexpr size_t SCRIPTEVENTTYPE_COUNT = static_cast<size_t>(ScriptEventType::COUNT);

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SCRIPTTYPES_H
