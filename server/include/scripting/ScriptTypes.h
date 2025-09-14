#ifndef SCRIPTTYPES_H
#define SCRIPTTYPES_H

#include <any>
#include <cstdint>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// ScriptType
////////////////////////////////////////////////////////////

/// @brief The type of script.
enum class ScriptType
{
	NPC,
	CLASS,
	WEAPON,
	SERVER,

	COUNT
};
constexpr size_t SCRIPTTYPE_COUNT = static_cast<size_t>(ScriptType::COUNT);


////////////////////////////////////////////////////////////
// ScriptObject
////////////////////////////////////////////////////////////

/// @brief Identifies an object type that may be used by a scripting language.
enum class ScriptObjectType
{
	SERVER,
	NPC,
	PLAYER,
	WEAPON,
	LEVEL,
	BADDY,
	BOMB,
	ARROW,
	ITEM,
	EXPLOSION,
	HORSE,
	SIGN,

	COUNT
};

/// @brief Binds a source object type with an identifier.
/// 
/// The first element is the identifier, which may be an id or a hash.
using ScriptObject = std::pair<size_t, ScriptObjectType>;

namespace source
{
/// @brief Creates a ScriptObject for an NPC with the given id.
constexpr ScriptObject FromNPC(size_t id)
{
	return std::make_pair(id, ScriptObjectType::NPC);
}

/// @brief Creates a ScriptObject for a player with the given id.
constexpr ScriptObject FromPlayer(size_t id)
{
	return std::make_pair(id, ScriptObjectType::PLAYER);
}

/// @brief Creates a ScriptObject for the server.
constexpr ScriptObject FromServer()
{
	return std::make_pair(static_cast<size_t>(0), ScriptObjectType::SERVER);
}

// Weapon and Level in their respective headers.
} // end namespace source


////////////////////////////////////////////////////////////
// ScriptEvent
////////////////////////////////////////////////////////////

/// @brief The script events known by the server.
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
	COMPUSDIED,
	NPCWARPED,
	EXPLODED,
	WASHIT,
	WASSHOT,
	WASPELT,
	TIMEOUT,
	PRIVATEMESSAGE,
	MOVEMENTFINISHED,
	//
	SERVERLISTCONNECT,
	TRIGGERACTION,

	COUNT
};
constexpr size_t SCRIPTEVENTTYPE_COUNT = static_cast<size_t>(ScriptEventType::COUNT);

/// @brief Represents an event in a scripting system, including its type, the source that initiated it, and any associated arguments.
struct ScriptEvent
{
	ScriptEventType type;
	ScriptObject initiator;
	std::vector<std::any> args;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SCRIPTTYPES_H
