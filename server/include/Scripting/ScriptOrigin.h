#ifndef SCRIPTORIGIN_H
#define SCRIPTORIGIN_H

#pragma once

#include "Level.h"
#include "NPC.h"
#include "ScriptClass.h"
#include "Weapon.h"
#include <fmt/format.h>
#include <string>

namespace scripting
{
	std::string getErrorOrigin(const NPC& npc)
	{
		std::string origin;

		switch (npc.getType())
		{
			// Database npcs don't need to include their location, so we are returning here
			// while the other two cases will append the level to the origin.
			case NPCType::DBNPC:
				return npc.getName();

			case NPCType::LEVELNPC:
				origin = "level npc";
				break;

			case NPCType::PUTNPC:
				origin = "local npc";
				break;
		}

		// Compiling before its assigned an npc id, so this requires some reworking to make work
		// origin.append(fmt::format("[{}]", npc.getId()));

		auto level = npc.getLevel();
		if (level)
			origin.append(fmt::format(" at {}, {:.2f}, {:.2f}", level->getLevelName().text(), npc.getX() / 16.0,
									  npc.getY() / 16.0));

		return origin;
	}

	std::string getErrorOrigin(const ScriptClass& cls)
	{
		return fmt::format("Class {}", cls.getName());
	}

	std::string getErrorOrigin(const Weapon& npc)
	{
		return fmt::format("Weapon {}", npc.getName());
	}
} // namespace scripting

#endif
