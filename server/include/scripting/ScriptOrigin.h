#ifndef SCRIPTORIGIN_H
#define SCRIPTORIGIN_H

#include <format>
#include <string>

#include "object/NPC.h"
#include "object/Weapon.h"
#include "level/Level.h"
#include "scripting/ScriptClass.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal::scripting
{

///////////////////////////////////////////////////////////////////////////////

std::string getErrorOrigin(const NPC& npc)
{
	std::string origin;

	switch (npc.type)
	{
		// Database npcs don't need to include their location, so we are returning here
		// while the other two cases will append the level to the origin.
		case NPCType::DBNPC:
			return npc.name;

		case NPCType::LEVELNPC:
			origin = "level npc";
			break;

		case NPCType::PUTNPC:
			origin = "local npc";
			break;
	}

	// Compiling before its assigned an npc id, so this requires some reworking to make work
	// origin.append(std::format("[{}]", npc.getId()));

	auto level = npc.level.lock();
	if (level)
		origin.append(std::format(" at {}, {:.2f}, {:.2f}", level->getLevelName().text(), npc.character.pixelX / 16.0, npc.character.pixelY / 16.0));

	return origin;
}

std::string getErrorOrigin(const ScriptClass& cls)
{
	return std::format("Class {}", cls.getName());
}

std::string getErrorOrigin(const Weapon& npc)
{
	return std::format("Weapon {}", npc.getName());
}

///////////////////////////////////////////////////////////////////////////////

} // namespace preagonal::scripting

#endif // SCRIPTORIGIN_H
