#ifndef SCRIPTTYPES_H
#define SCRIPTTYPES_H

#include <cstdint>
#include <string_view>

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
	TRIGGERACTION,

	COUNT
};
constexpr size_t SCRIPTEVENTTYPE_COUNT = static_cast<size_t>(ScriptEventType::COUNT);

constexpr ScriptEventType scriptEventTypeFromName(std::string_view name)
{
	if (name == "created") return ScriptEventType::CREATED;
	if (name == "initialized") return ScriptEventType::INITIALIZED;
	if (name == "playerlogin") return ScriptEventType::PLAYERLOGIN;
	if (name == "playerlogout") return ScriptEventType::PLAYERLOGOUT;
	if (name == "playerenters") return ScriptEventType::PLAYERENTERS;
	if (name == "playerleaves") return ScriptEventType::PLAYERLEAVES;
	if (name == "playertouchsme") return ScriptEventType::PLAYERTOUCHSME;
	if (name == "playertouchsother") return ScriptEventType::PLAYERTOUCHSOTHER;
	if (name == "playerlaysitem") return ScriptEventType::PLAYERLAYSITEM;
	if (name == "playerchats") return ScriptEventType::PLAYERCHATS;
	if (name == "playerdies") return ScriptEventType::PLAYERDIES;
	if (name == "playerendreading") return ScriptEventType::PLAYERENDREADING;
	if (name == "weaponfired") return ScriptEventType::WEAPONFIRED;
	if (name == "firedonhorse") return ScriptEventType::FIREDONHORSE;
	if (name == "compusdied") return ScriptEventType::COMPUSDIED;
	if (name == "warped") return ScriptEventType::WARPED;
	if (name == "npcwarped") return ScriptEventType::NPCWARPED;
	if (name == "exploded") return ScriptEventType::EXPLODED;
	if (name == "washit") return ScriptEventType::WASHIT;
	if (name == "wasshot") return ScriptEventType::WASSHOT;
	if (name == "waspelt") return ScriptEventType::WASPELT;
	if (name == "timeout") return ScriptEventType::TIMEOUT;
	return ScriptEventType::CUSTOM;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SCRIPTTYPES_H
