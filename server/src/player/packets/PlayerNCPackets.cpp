#include <algorithm>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <CString.h>
#include <IEnums.h>
#include <IUtil.h>

#include <Server.h>
#include <level/Level.h>
#include <network/IPacketHandler.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Weapon.h>
#include <player/PlayerNC.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/PropertySerializers.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

HandlePacketResult PlayerNC::msgPLI_RC_CHAT(CString& pPacket)
{
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_NPCGET(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to get a database npc.", account.name);
		return HandlePacketResult::Handled;
	}

	// RC3 keeps sending empty packets of this, yet still uses NPCGET to fetch npcs. Maybe its for pinging the server
	// for updated level information on database npcs? Just a thought..
	// 5/26/2019 - confirmed, this is the npc-server pinging the gserver.
	if (pPacket.bytesLeft())
	{
		const NPCID npcId = pPacket.readGUInt();

		const auto npc = m_server->getNPC(npcId);
		if (npc != nullptr)
		{
			auto dump = npc->getVariableDump();
			sendPacket(CString() >> (char)PLO_NC_NPCATTRIBUTES << string::toCSV(dump));
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_NPCDELETE(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to delete a database npc.", account.name);
		return HandlePacketResult::Handled;
	}

	const NPCID npcId = pPacket.readGUInt();
	const auto npc = m_server->getNPC(npcId);

	if (npc != nullptr && npc->storageType == NPCStorageType::DATABASE)
	{
		m_server->getNPCServer()->deleteNPC(npcId);
		m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_NPCDELETE >> (int)npcId);

		const std::string logMsg = std::format("NPC {} deleted by {}", npc->name, account.name);
		log::printLine(log::npc, logMsg);
		m_server->sendToNC(logMsg);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_NPCRESET(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to reset a database npc.", account.name);
		return HandlePacketResult::Handled;
	}

	const NPCID npcId = pPacket.readGUInt();

	const auto npc = m_server->getNPC(npcId);
	if (npc != nullptr && npc->storageType == NPCStorageType::DATABASE)
	{
		m_server->sendPacketToAll(CString() >> (char)PLO_NPCDEL >> (int)npcId);
		npc->resetToInitialState();
		npc->scripting.events.addEvent(ScriptEventType::CREATED, source::FromServer());

		const std::string logMsg = std::format("NPC script of {} reset by {}", npc->name, account.name);
		log::printLine(log::npc, logMsg);
		m_server->sendToNC(logMsg);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_NPCSCRIPTGET(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to get a database npc script.", account.name);
		return HandlePacketResult::Handled;
	}

	// {160}{INT id}{GSTRING script}
	const NPCID npcId = pPacket.readGUInt();
	const auto npc = m_server->getNPC(npcId);
	if (npc != nullptr)
	{
		const std::string tokenizedScript = string::toCSV(npc->getScript().getOriginalSource(), "\n");
		sendPacket(CString() >> (char)PLO_NC_NPCSCRIPT >> (int)npcId << tokenizedScript);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_NPCWARP(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to warp a database npc.", account.name);
		return HandlePacketResult::Handled;
	}

	const NPCID npcId = pPacket.readGUInt();
	PropertyTileCoordinate tileX{static_cast<float>(pPacket.readGUChar()) / 2.0f};
	PropertyTileCoordinate tileY{static_cast<float>(pPacket.readGUChar()) / 2.0f};
	const std::string npcLevel = pPacket.readString("").trimI().toString();

	const auto npc = m_server->getNPC(npcId);
	if (npc == nullptr)
		return HandlePacketResult::Handled;

	// Warping to a different level entirely.
	if (npcLevel != npc->getLevelName())
	{
		if (const auto newLevel = m_server->getLoadedLevel(npcLevel, npc->getLevel()); newLevel != nullptr)
			npc->warp(newLevel, {tileX.pixelCoordinate, tileY.pixelCoordinate});
	}
	// Changing position in the current level.
	else
	{
		// clang-format off
		npc->sendPropsFromResults(
			npc->setPropWith<NPCProp::X2>(SetBy::SERVER, tileX.pixelCoordinate),
			npc->setPropWith<NPCProp::Y2>(SetBy::SERVER, tileY.pixelCoordinate)
		);
		// clang-format on
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_NPCFLAGSGET(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to get a database npc flags.", account.name);
		return HandlePacketResult::Handled;
	}

	const NPCID npcId = pPacket.readGUInt();
	const auto npc = m_server->getNPC(npcId);
	if (npc != nullptr)
	{
		std::vector<std::string> flagList;
		for (auto& [flag, value] : npc->scripting.variables.store | variables::only_flags)
		{
			if (value->value.has<bool>() && !value->value.has<std::string>() && value->value.getCopy<bool>().value_or(false))
				flagList.push_back(flag);
			else if (value->value.has<std::string>())
				flagList.push_back(std::format("{}={}", flag, value->value.get<std::string>().value().get()));
		}

		sendPacket(CString() >> (char)PLO_NC_NPCFLAGS >> (int)npcId << string::toCSV(flagList));
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_NPCSCRIPTSET(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to set a database npc script.", account.name);
		return HandlePacketResult::Handled;
	}

	const NPCID npcId = pPacket.readGUInt();
	const CString npcScript = pPacket.readString("").guntokenize();

	// TODO: Validate permissions

	const auto npc = m_server->getNPC(npcId);
	if (npc != nullptr)
	{
		const auto lastUpdateTime = npc->lastUpdateTime;

		npc->setScript(npcScript.toStringView());
		npc->scripting.events.addEvent(ScriptEventType::CREATED, source::FromServer());

		const std::string logMsg = std::format("NPC script of {} updated by {}", npc->name, account.name);
		log::printLine(log::npc, logMsg);
		m_server->sendToNC(logMsg);

		npc->sendScriptUpdatesToLevel(lastUpdateTime);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_NPCFLAGSSET(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to set a database npc flags.", account.name);
		return HandlePacketResult::Handled;
	}

	const NPCID npcId = pPacket.readGUInt();

	if (const auto npc = m_server->getNPC(npcId); npc != nullptr)
	{
		std::vector<std::string> addedFlags;
		std::vector<std::string> updatedFlags;
		std::vector<std::string> deletedFlags;
		auto incomingFlags = string::fromCSV(pPacket.readString("").toString());

		// Go through the existing flags and delete/update them.
		auto it = npc->scripting.variables.store.begin();
		while (it != npc->scripting.variables.store.end())
		{
			auto& storeFlagName = it->first;

			// Ignore temporary variables and non-flag variables.
			if (auto storeFlag = it->second; storeFlag != nullptr && storeFlag->lifetime == variables::Lifetime::PERMANENT && storeFlag->value.testAsFlag())
			{
				auto flagBeingSet = std::ranges::find_if(incomingFlags, [&storeFlagName](const std::string& flag)
				{
					const auto incomingFlagValue = string::retrieveLine(flag, '=');
					return incomingFlagValue == storeFlagName;
				});

				// Store flag is not found in the incoming flags, so delete it.
				if (flagBeingSet == std::ranges::end(incomingFlags))
				{
					if (it->second->has<std::string>())
						deletedFlags.emplace_back(std::format("flag deleted:\t{}={}", storeFlagName, it->second->getCopy<std::string>().value_or(std::string{})));
					else deletedFlags.emplace_back(std::format("flag deleted:\t{}", storeFlagName));

					it = npc->scripting.variables.store.erase(it);
					continue;
				}

				// Store flag was found, so check if we need to modify it.
				if (auto existingValueWrap = it->second->get<std::string>(); existingValueWrap.has_value())
				{
					auto& existingValue = existingValueWrap.value().get();
					const auto equalPos = flagBeingSet->find('=');
					std::string flagValue{string::trimMutate(flagBeingSet->substr(equalPos + 1))};

					// It was modified!
					if (existingValue != flagValue)
					{
						updatedFlags.emplace_back(std::format("flag updated:\t{}={} -> {}", storeFlagName, existingValue, flagValue));
						it->second->set(flagValue);
						incomingFlags.erase(flagBeingSet);
					}
					// It was not modified, so remove it from the incoming flags so we don't add it again.
					else
					{
						incomingFlags.erase(flagBeingSet);
					}

					++it;
					continue;
				}
			}

			// This flag will be added.
			++it;
		}

		// Add new flags.
		for (std::string_view flag : incomingFlags)
		{
			if (const auto equalPos = flag.find('='); equalPos == std::string::npos)
			{
				npc->scripting.variables.add(flag, GameValue{true});
				addedFlags.emplace_back(std::format("flag added:\t{}", flag));
			}
			else
			{
				auto flagName = string::trim(flag.substr(0, equalPos));
				auto flagValue = string::trim(flag.substr(equalPos + 1));
				npc->scripting.variables.add(flagName, GameValue{std::string{flagValue}});
				addedFlags.emplace_back(std::format("flag added:\t{}={}", flagName, flagValue));
			}
		}

		// Save the NPC.
		m_server->getNPCLoader().saveNPC(npc);

		// Announce changes.
		// clang-format off
		const CString updateMsg = std::format("NPC flags of {} updated by {}", npc->name, account.name);
		m_server->sendToNC(updateMsg);
		log::printLine(log::npc, updateMsg);
		if (!addedFlags.empty())
		{
			std::ranges::for_each(addedFlags, [&](const std::string& message) { m_server->sendToNC(message); });
			log::printLine(log::npc, string::join(addedFlags, "\n"sv));
		}
		if (!updatedFlags.empty())
		{
			std::ranges::for_each(updatedFlags, [&](const std::string& message) { m_server->sendToNC(message); });
			log::printLine(log::npc, string::join(updatedFlags, "\n"sv));
		}
		if (!deletedFlags.empty())
		{
			std::ranges::for_each(deletedFlags, [&](const std::string& message) { m_server->sendToNC(message); });
			log::printLine(log::npc, string::join(deletedFlags, "\n"sv));
		}
		// clang-format on
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_NPCADD(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to add a database npc.", account.name);
		return HandlePacketResult::Handled;
	}

	if (!m_server->hasNPCServer())
		return HandlePacketResult::Handled;

	const auto packetData = pPacket.readString("").toString();
	const auto npcData = string::fromCSV(packetData);
	const auto& npcName = npcData[0];
	const auto npcId = string::toNumber<NPCID>(npcData[1]);
	const auto& npcType = npcData[2];
	const auto& npcScripter = npcData[3];
	const auto& npcLevel = npcData[4];
	auto npcX = string::toFloat(npcData[5]);
	auto npcY = string::toFloat(npcData[6]);

	if (npcName.empty())
	{
		m_server->sendToNC("Error adding database npc: NPC name cannot be empty");
		return HandlePacketResult::Handled;
	}

	if (m_server->getNPCServer()->getNPCByName(npcName) != nullptr)
	{
		m_server->sendToNC("Error adding database npc: NPC name already exists");
		return HandlePacketResult::Handled;
	}

	// First check if the level belongs to a gmap, then just try to load it.
	LevelPtr level = nullptr;
	if (!npcLevel.empty())
	{
		if (level = m_server->findGmapForLevel(npcLevel, nullptr); level != nullptr)
		{
			if (const auto map = level->getMap(); map != nullptr)
			{
				auto position = map->getLevelPosition(npcLevel).value_or(MapPosition{0, 0});
				npcX += static_cast<float>(position.x()) * Level::tilesPerSubLevel().width();
				npcY += static_cast<float>(position.y()) * Level::tilesPerSubLevel().height();
			}
		}

		if (level == nullptr)
			level = m_server->getLoadedLevelNoHint(npcLevel);
	}

	if (npcId < NPCID_GEN_MANUAL)
	{
		m_server->sendToNC(std::format("Error adding database npc: NPC ID must be greater than {}", NPCID_GEN_MANUAL));
		return HandlePacketResult::Handled;
	}

	if (m_server->getNPC(npcId) != nullptr)
	{
		m_server->sendToNC("Error adding database npc: NPC ID already exists");
		return HandlePacketResult::Handled;
	}

	const auto newNPC = m_server->getNPCServer()->addNPC(npcName, npcId, npcType, npcScripter, level, {npcX, npcY});
	if (newNPC != nullptr)
	{
		// Persist NPC
		newNPC->recordInitialState();
		m_server->getNPCLoader().saveNPC(newNPC);

		// Logging
		const std::string logMsg = std::format("NPC {} added by {}", newNPC->name, account.name);
		log::printLine(log::npc, logMsg);
		m_server->sendToNC(logMsg);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_CLASSEDIT(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to edit a class.", account.name);
		return HandlePacketResult::Handled;
	}

	if (!m_server->hasNPCServer())
		return HandlePacketResult::Handled;

	// {112}{class}
	CString className = pPacket.readString("");
	if (const auto classObj = m_server->getNPCServer()->getClass(className.text()); classObj != nullptr)
		sendPacket(CString() >> (char)PLO_NC_CLASSGET >> (char)className.length() << className << string::toCSV(classObj->getScript().getOriginalSource()));

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_CLASSADD(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to add a class.", account.name);
		return HandlePacketResult::Handled;
	}

	if (!m_server->hasNPCServer())
		return HandlePacketResult::Handled;

	// {113}{CHAR name length}{name}{GSTRING script}
	std::string className = pPacket.readChars(pPacket.readGUChar()).toString();
	const auto classCode = string::join(string::fromCSV(pPacket.readString("").toString()), "\n"sv);

	bool hasClass = false;
	if (const auto classObj = m_server->getNPCServer()->getClass(className); classObj != nullptr)
	{
		hasClass = true;
		classObj->setScript(classCode);
		m_server->getNPCServer()->updateClass(className, classCode);
		m_server->updateClassForPlayers(classObj);
	}

	if (!hasClass)
	{
		m_server->getNPCServer()->addClass(className, classCode);
		m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_CLASSADD << className);
	}

	// Logging
	const std::string logMsg = std::format("Script {} {} by {}", className, (!hasClass ? "added" : "updated"), account.name);
	log::printLine(log::npc, logMsg);
	m_server->sendToNC(logMsg);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_CLASSDELETE(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to delete a class.", account.name);
		return HandlePacketResult::Handled;
	}

	if (!m_server->hasNPCServer())
		return HandlePacketResult::Handled;

	std::string className = pPacket.readString("").toString();

	CString logMsg;
	if (m_server->getNPCServer()->deleteClass(className))
	{
		m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_CLASSDELETE << className);
		logMsg << account.name << " has deleted class " << className << "\n";
	}
	else
	{
		logMsg << "error: " << className << " does not exist on this server!\n";
	}

	// Logging
	log::print(log::npc, logMsg.toString());
	m_server->sendToNC(logMsg);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_LOCALNPCSGET(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to view level npcs.", account.name);
		return HandlePacketResult::Handled;
	}

	// {114}{level}
	const CString level = pPacket.readString("");
	if (level.isEmpty())
		return HandlePacketResult::Handled;

	if (const auto npcLevel = m_server->getLoadedLevelNoHint(level.toString()); npcLevel != nullptr)
	{
		CString npcDump;
		// Variables dump from level mapname (level.nw)
		npcDump << "Variables dump from level " << npcLevel->levelName << "\n";

		for (const auto npcId : npcLevel->getNPCs())
		{
			// clang-format off
			const auto npc = m_server->getNPC(npcId);
			npcDump << "\n" << string::join(npc->getVariableDump(), "\n") << "\n";
			// clang-format on
		}

		sendPacket(CString() >> (char)PLO_NC_LEVELDUMP << npcDump.gtokenize());
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_WEAPONLISTGET(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to view the weapon list.", account.name);
		return HandlePacketResult::Handled;
	}

	// Start our packet.
	CString ret;
	ret >> (char)PLO_NC_WEAPONLISTGET;

	// Iterate weapon list and send names
	for (const auto& [weaponName, weapon] : m_server->getWeaponList())
	{
		if (weapon->isDefault())
			continue;

		ret >> (char)weaponName.length() << weaponName;
	}

	sendPacket(ret);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_WEAPONGET(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to view a weapon.", account.name);
		return HandlePacketResult::Handled;
	}

	// {116}{weapon}
	const CString weaponName = pPacket.readString("");
	const auto weapon = m_server->getWeapon(weaponName.toString());
	if (weapon == nullptr || weapon->isDefault())
	{
		m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << account.name << " prob: weapon " << weaponName << " doesn't exist");
		return HandlePacketResult::Handled;
	}

	std::string script = weapon->getScript().getOriginalSource();
	std::ranges::replace(script, '\n', '\xa7');

	// clang-format off
	if (getVersion() < NCVER_2_1)
	{
		sendPacket(CString() >> (char)PLO_NPCWEAPONADD
			>> (char)weaponName.length() << weaponName
			>> (char)0 >> (char)weapon->image.length() << weapon->image
			>> (char)1 >> (short)script.length()
			<< script);
	}
	else
	{
		sendPacket(CString() >> (char)PLO_NC_WEAPONGET
			>> (char)weaponName.length() << weaponName
			>> (char)weapon->image.length() << weapon->image
			<< script);
	}
	// clang-format on

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_WEAPONADD(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to add a weapon.", account.name);
		return HandlePacketResult::Handled;
	}

	// {117}{CHAR weapon length}{weapon}{CHAR image length}{image}{code}
	std::string weaponName = pPacket.readChars(pPacket.readGUChar()).toString();
	std::string weaponImage = pPacket.readChars(pPacket.readGUChar()).toString();
	std::string weaponCode = pPacket.readString("").toString();

	std::ranges::replace(weaponCode, '\xa7', '\n');

	CString actionTaken;

	// Find Weapon
	const auto weaponObj = m_server->getWeapon(weaponName);
	if (weaponObj != nullptr)
	{
		// default weapon, don't update!
		if (weaponObj->isDefault())
			return HandlePacketResult::Handled;

		// Update Weapon
		weaponObj->updateWeapon(std::move(weaponImage), std::move(weaponCode)).saveWeapon();

		// Update Player-Weapons
		m_server->updateWeaponForPlayers(weaponObj);

		actionTaken = "updated";
	}
	else
	{
		// add weapon
		const auto weapon = std::make_shared<Weapon>(weaponName, std::move(weaponImage), std::move(weaponCode));
		weapon->saveWeapon();
		if (m_server->NC_AddWeapon(weapon) == true)
			actionTaken = "added";
	}

	// TODO(joey): Log message should come before the script is executed
	if (!actionTaken.isEmpty())
	{
		CString logMsg;
		logMsg << "Weapon/GUI-script " << weaponName << " " << actionTaken << " by " << account.name << "\n";
		log::print(log::npc, logMsg.toString());
		m_server->sendToNC(logMsg);
	}

	// Send the updated weapon list to the player.
	msgPLI_NC_WEAPONLISTGET(pPacket);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_WEAPONDELETE(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to delete a weapon.", account.name);
		return HandlePacketResult::Handled;
	}

	// {118}{weapon}
	const CString weaponName = pPacket.readString("");

	bool deleted = false;
	CString logMsg;
	if (m_server->NC_DelWeapon(weaponName.toString()))
	{
		logMsg << "Weapon " << weaponName << " deleted by " << account.name << "\n";
		deleted = true;
	}
	else
	{
		logMsg << account.name << " prob: weapon " << weaponName << " doesn't exist\n";
	}

	// Logging
	log::print(log::npc, logMsg.toString());
	m_server->sendToNC(logMsg);

	// Send the updated weapon list to the player.
	if (deleted)
		msgPLI_NC_WEAPONLISTGET(pPacket);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerNC::msgPLI_NC_LEVELLISTGET(CString& pPacket)
{
	if (!isNC())
	{
		log::printLine(log::npc, "[Hack] {} attempted to view the level list.", account.name);
		return HandlePacketResult::Handled;
	}

	// Start our packet.
	CString ret;

	if (const auto& levelList = m_server->getLevelList(); !levelList.empty())
	{
		for (const auto& level : levelList | std::views::values)
			ret << level->levelName << "\n";
	}

	sendPacket(CString() >> (char)PLO_NC_LEVELLIST << ret.gtokenize());
	return HandlePacketResult::Handled;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
