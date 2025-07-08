#include <algorithm>
#include <cstdint>
#include <format>
#include <iterator>
#include <memory>
#include <string_view>
#include <string>
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
#include <utilities/Log.h>
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
		NPCID npcId = pPacket.readGUInt();

		auto npc = m_server->getNPC(npcId);
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

	NPCID npcId = pPacket.readGUInt();
	auto npc = m_server->getNPC(npcId);

	if (npc != nullptr && npc->storageType == NPCStorageType::DATABASE)
	{
		m_server->getNPCServer()->deleteNPC(npcId);
		m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_NPCDELETE >> (int)npcId);

		std::string logMsg = std::format("NPC {} deleted by {}", npc->name, account.name);
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

	NPCID npcId = pPacket.readGUInt();

	auto npc = m_server->getNPC(npcId);
	if (npc != nullptr && npc->storageType == NPCStorageType::DATABASE)
	{
		npc->resetToInitialState();
		npc->scripting.events.addEvent(ScriptEventType::CREATED, source::FromServer());

		std::string logMsg = std::format("NPC script of {} reset by {}", npc->name, account.name);
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
	NPCID npcId = pPacket.readGUInt();
	auto npc = m_server->getNPC(npcId);
	if (npc != nullptr)
	{
		std::string tokenizedScript = string::toCSV(npc->getScript().getOriginalSource(), '\n');
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

	NPCID npcId = pPacket.readGUInt();
	int16_t npcX = static_cast<int16_t>(pPacket.readGUChar() * 8);
	int16_t npcY = static_cast<int16_t>(pPacket.readGUChar() * 8);
	CString npcLevel = pPacket.readString("");

	auto npc = m_server->getNPC(npcId);
	if (npc != nullptr)
	{
		if (auto newLevel = m_server->getLevel(npcLevel.toString()); newLevel != nullptr)
			npc->warp(newLevel, npcX, npcY);
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

	NPCID npcId = pPacket.readGUInt();
	auto npc = m_server->getNPC(npcId);
	if (npc != nullptr)
	{
		std::vector<std::string> flagList;
		for (auto& [flag, value] : npc->scripting.variables.store | variables::only_flags)
		{
			if (value->has<bool>() && !value->has<std::string>() && value->get<bool>().value_or(false))
				flagList.push_back(flag);
			else if (value->has<std::string>())
				flagList.push_back(std::format("{}={}", flag, value->get<std::string>().value_or(std::string{})));
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

	NPCID npcId = pPacket.readGUInt();
	CString npcScript = pPacket.readString("").guntokenize();

	// TODO: Validate permissions

	auto npc = m_server->getNPC(npcId);
	if (npc != nullptr)
	{
		npc->setScript(npcScript.toStringView());
		m_server->getNPCLoader().saveNPC(npc);
		npc->scripting.events.addEvent(ScriptEventType::CREATED, source::FromServer());

		std::string logMsg = std::format("NPC script of {} updated by {}", npc->name, account.name);
		log::printLine(log::npc, logMsg);
		m_server->sendToNC(logMsg);
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

	NPCID npcId = pPacket.readGUInt();

	if (auto npc = m_server->getNPC(npcId); npc != nullptr)
	{
		std::vector<std::string> addedFlags;
		std::vector<std::string> updatedFlags;
		std::vector<std::string> deletedFlags;
		auto npcFlags = string::fromCSV(pPacket.readString("").toString());

		// Remove any flags that do not contain an equal sign, as these are not valid flags for NPCs.
		std::erase_if(npcFlags, [](std::string& flag) { return !flag.contains('='); });

		// Go through the existing flags and delete/update them.
		auto it = npc->scripting.variables.store.begin();
		while (it != npc->scripting.variables.store.end())
		{
			// Ignore temporary variables and non-flag variables.
			if (auto var = it->second; var != nullptr && !var->temporary && var->testAsFlag())
			{
				auto flagBeingSet = std::ranges::find_if(npcFlags, [&it](std::string& flag) { return flag.starts_with(it->first); });

				// Not in range, delete it.
				if (flagBeingSet == std::ranges::end(npcFlags))
				{
					deletedFlags.emplace_back(std::format("flag deleted:\t{}={}", it->first, it->second->get<std::string>().value_or(std::string{})));
					it = npc->scripting.variables.store.erase(it);
				}
				// Is in range, check if updated.
				else
				{
					auto existingValue = it->second->get<std::string>().value_or(std::string{});
					auto equalPos = flagBeingSet->find('=');
					std::string flagValue{ string::trimMutate(flagBeingSet->substr(equalPos + 1)) };
					if (existingValue != flagValue)
					{
						updatedFlags.emplace_back(std::format("flag updated:\t{}={} -> {}", it->first, existingValue, flagValue));
						it->second->assign(flagValue);
						npcFlags.erase(flagBeingSet);
						++it;
					}
				}
			}
			else ++it;
		}

		// Add new flags.
		for (std::string_view flag : npcFlags)
		{
			auto equalPos = flag.find('=');
			if (equalPos == std::string::npos)
				continue;

			auto flagName = string::trim(flag.substr(0, equalPos));
			auto flagValue = string::trim(flag.substr(equalPos + 1));
			npc->scripting.variables.add(flagName, GameValue{ std::string{ flagValue } });
			addedFlags.emplace_back(std::format("flag added:\t{}={}", flagName, flagValue));
		}

		// Save the NPC.
		m_server->getNPCLoader().saveNPC(npc);

		// Announce changes.
		CString updateMsg = std::format("NPC flags of {} updated by {}", npc->name, account.name);
		m_server->sendToNC(updateMsg);
		log::printLine(log::npc, updateMsg);
		if (!addedFlags.empty())
			log::printLine(log::npc, string::join(addedFlags, "\n"sv));
		if (!updatedFlags.empty())
			log::printLine(log::npc, string::join(updatedFlags, "\n"sv));
		if (!deletedFlags.empty())
			log::printLine(log::npc, string::join(deletedFlags, "\n"sv));
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

	auto packetData = pPacket.readString("").toString();
	auto npcData = string::fromCSV(packetData);
	auto& npcName = npcData[0];
	auto npcId = string::toNumber<NPCID>(npcData[1]);
	auto& npcType = npcData[2];
	auto& npcScripter = npcData[3];
	auto& npcLevel = npcData[4];
	auto npcX = string::toFloat(npcData[5]);
	auto npcY = string::toFloat(npcData[6]);

	if (npcName.empty())
	{
		m_server->sendToNC("Error adding database npc: NPC name cannot be empty");
		return HandlePacketResult::Handled;
	}

	auto level = m_server->getLevel(npcLevel);
	if (level == nullptr)
	{
		m_server->sendToNC("Error adding database npc: Level does not exist");
		return HandlePacketResult::Handled;
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

	auto newNPC = m_server->getNPCServer()->addNPC(npcName, npcId, npcType, npcScripter, level, { npcX, npcY });
	if (newNPC != nullptr)
	{
		// Persist NPC
		m_server->getNPCLoader().saveNPC(newNPC);

		// Logging
		std::string logMsg = std::format("NPC {} added by {}", newNPC->name, account.name);
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
	if (auto classObj = m_server->getNPCServer()->getClass(className.text()).lock(); classObj != nullptr)
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
	CString classCode = pPacket.readString("").guntokenize();

	bool hasClass = false;
	if (auto classObj = m_server->getNPCServer()->getClass(className).lock(); classObj != nullptr)
	{
		hasClass = true;
		classObj->setScript(classCode.toStringView());
		m_server->getNPCServer()->updateClass(className, classCode.toString());
		m_server->updateClassForPlayers(classObj);
	}

	if (!hasClass)
	{
		m_server->getNPCServer()->addClass(className, classCode.toStringView());
		m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_CLASSADD << className);
	}

	// Logging
	std::string logMsg = std::format("Script {} {} by {}", className, (!hasClass ? "added" : "updated"), account.name);
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
		CString ret;
		ret >> (char)PLO_NC_CLASSDELETE << className;
		m_server->sendPacketToType(PLTYPE_ANYNC, ret);
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
	CString level = pPacket.readString("");
	if (level.isEmpty())
		return HandlePacketResult::Handled;

	auto npcLevel = m_server->getLevel(level.toString());
	if (npcLevel != nullptr)
	{
		CString npcDump;
		// Variables dump from level mapname (level.nw)
		npcDump << "Variables dump from level " << npcLevel->levelName << "\n";

		for (auto npcId : npcLevel->getNPCs())
		{
			auto npc = m_server->getNPC(npcId);
			npcDump << "\n"
				<< string::join(npc->getVariableDump(), "\n") << "\n";
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
	CString weaponName = pPacket.readString("");
	auto weapon = m_server->getWeapon(weaponName.toString());
	if (weapon == nullptr || weapon->isDefault())
	{
		m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << account.name << " prob: weapon " << weaponName << " doesn't exist");
		return HandlePacketResult::Handled;
	}

	std::string script = weapon->getScript().getOriginalSource();
	std::replace(script.begin(), script.end(), '\n', '\xa7');

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

	std::replace(weaponCode.begin(), weaponCode.end(), '\xa7', '\n');

	CString actionTaken;

	// Find Weapon
	auto weaponObj = m_server->getWeapon(weaponName);
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
		auto weapon = std::make_shared<Weapon>(weaponName, std::move(weaponImage), std::move(weaponCode));
		weapon->saveWeapon();
		bool success = m_server->NC_AddWeapon(weapon);
		if (success)
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
	CString weaponName = pPacket.readString("");

	CString logMsg;
	if (m_server->NC_DelWeapon(weaponName.toString()))
		logMsg << "Weapon " << weaponName << " deleted by " << account.name << "\n";
	else
		logMsg << account.name << " prob: weapon " << weaponName << " doesn't exist\n";

	// Logging
	log::print(log::npc, logMsg.toString());
	m_server->sendToNC(logMsg);
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

	auto& levelList = m_server->getLevelList();
	if (!levelList.empty())
	{
		for (const auto& level : levelList)
			ret << level.second->getOriginalLevelName() << "\n";
	}

	sendPacket(CString() >> (char)PLO_NC_LEVELLIST << ret.gtokenize());
	return HandlePacketResult::Handled;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
