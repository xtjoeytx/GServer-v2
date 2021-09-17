#ifndef SCRIPTORIGIN_H
#define SCRIPTORIGIN_H

#pragma once

#include <string>
#include <fmt/format.h>
#include "TNPC.h"
#include "TScriptClass.h"
#include "TWeapon.h"
#include "TLevel.h"

namespace scripting
{
	template<typename Obj>
	std::string getErrorOrigin(const Obj& obj) {
		static_assert(false == true);
		return {};
	}

	template <>
	std::string getErrorOrigin<TNPC>(const TNPC& npc)
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
			origin.append(fmt::format(" at {}, {:.2f}, {:.2f}", level->getLevelName().text(), npc.getX(), npc.getY()));

		return std::move(origin);
	}

	template <>
	std::string getErrorOrigin<TScriptClass>(const TScriptClass& cls)
	{
		return fmt::format("Class {}", cls.getName());
	}

	template <>
	std::string getErrorOrigin<TWeapon>(const TWeapon& npc)
	{
		return fmt::format("Weapon {}", npc.getName());
	}
}

#endif
