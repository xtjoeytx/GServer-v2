#include <math.h>
#include <vector>

#include <IDebug.h>
#include <IEnums.h>

#include "Server.h"
#include "object/NPC.h"
#include "object/Player.h"
#include "object/Weapon.h"
#include "level/Level.h"

#define serverlog m_server->getServerLog()
#define npclog m_server->getNPCLog()
#define rclog m_server->getRCLog()

#ifdef V8NPCSERVER
HandlePacketResult Player::msgPLI_NC_NPCGET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to get a database npc.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	// RC3 keeps sending empty packets of this, yet still uses NPCGET to fetch npcs. Maybe its for pinging the server
	// for updated level information on database npcs? Just a thought..
	// 5/26/2019 - confirmed, this is the npc-server pinging the gserver.
	if (pPacket.bytesLeft())
	{
		unsigned int npcId = pPacket.readGUInt();

		auto npc = m_server->getNPC(npcId);
		if (npc != nullptr)
		{
			CString npcDump = npc->getVariableDump();
			sendPacket(CString() >> (char)PLO_NC_NPCATTRIBUTES << npcDump.gtokenize());
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_NPCDELETE(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to delete a database npc.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	unsigned int npcId = pPacket.readGUInt();
	auto npc = m_server->getNPC(npcId);

	if (npc != nullptr && npc->getType() == NPCType::DBNPC)
	{
		CString npcName = npc->getName();
		bool result = m_server->deleteNPC(npc, true);
		if (result)
		{
			m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_NPCDELETE >> (int)npcId);

			CString logMsg;
			logMsg << "NPC " << npcName << " deleted by " << m_accountName << "\n";
			npclog.out(logMsg);
			m_server->sendToNC(logMsg);
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_NPCRESET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to reset a database npc.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	unsigned int npcId = pPacket.readGUInt();

	auto npc = m_server->getNPC(npcId);
	if (npc != nullptr && npc->getType() == NPCType::DBNPC)
	{
		npc->resetNPC();

		CString logMsg;
		logMsg << "NPC script of " << npc->getName() << " reset by " << m_accountName << "\n";
		npclog.out(logMsg);
		m_server->sendToNC(logMsg);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_NPCSCRIPTGET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to get a database npc script.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	// {160}{INT id}{GSTRING script}
	unsigned int npcId = pPacket.readGUInt();
	auto npc = m_server->getNPC(npcId);
	if (npc != nullptr)
	{
		CString code = npc->getSource().getSource();
		sendPacket(CString() >> (char)PLO_NC_NPCSCRIPT >> (int)npcId << code.gtokenize());
	}
	//	else printf("npc doesn't exist\n");

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_NPCWARP(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to warp a database npc.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	unsigned int npcId = pPacket.readGUInt();
	float npcX = (float)pPacket.readGUChar() / 2.0f;
	float npcY = (float)pPacket.readGUChar() / 2.0f;
	CString npcLevel = pPacket.readString("");

	auto npc = m_server->getNPC(npcId);
	if (npc != nullptr)
	{
		auto newLevel = m_server->getLevel(npcLevel.toString());
		if (newLevel != nullptr)
			npc->warpNPC(newLevel, int(npcX * 16.0), int(npcY * 16.0));
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_NPCFLAGSGET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to get a database npc flags.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	unsigned int npcId = pPacket.readGUInt();
	auto npc = m_server->getNPC(npcId);
	if (npc != nullptr)
	{
		CString flagListStr;

		auto& flagList = npc->getFlagList();
		for (const auto& [flag, value]: flagList)
			flagListStr << flag << "=" << value << "\n";

		sendPacket(CString() >> (char)PLO_NC_NPCFLAGS >> (int)npcId << flagListStr.gtokenize());
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_NPCSCRIPTSET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to set a database npc script.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	unsigned int npcId = pPacket.readGUInt();
	CString npcScript = pPacket.readString("").guntokenize();

	// TODO: Validate permissions

	auto npc = m_server->getNPC(npcId);
	if (npc != nullptr)
	{
		npc->setScriptCode(npcScript.toString());
		npc->saveNPC();

		CString logMsg;
		logMsg << "NPC script of " << npc->getName() << " updated by " << m_accountName << "\n";
		npclog.out(logMsg);
		m_server->sendToNC(logMsg);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_NPCFLAGSSET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to set a database npc flags.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	unsigned int npcId = pPacket.readGUInt();
	CString npcFlags = pPacket.readString("").guntokenize();

	auto npc = m_server->getNPC(npcId);
	if (npc != nullptr)
	{
		auto& flagList = npc->getFlagList();
		auto newFlags = npcFlags.tokenize("\n");

		CString addedFlagMsg, deletedFlagMsg;
		std::unordered_map<std::string, CString> newFlagList;

		// Iterate the new list of flags from the client
		for (auto& flag: newFlags)
		{
			std::string flagName = flag.readString("=").text();
			CString flagValue = flag.readString("");

			// Check if the flag is a new flag, or if it has been updated
			auto oldFlag = flagList.find(flagName);
			if (oldFlag == flagList.end())
				addedFlagMsg << "flag added:\t" << flagName << "=" << flagValue << "\n";
			else if (oldFlag->second != flagValue)
			{
				addedFlagMsg << "flag added:\t" << flagName << "=" << flagValue << "\n";
				deletedFlagMsg << "flag deleted:\t" << oldFlag->first << "=" << oldFlag->second << "\n";
				flagList.erase(oldFlag);
			}

			// Add the flag to the new list
			newFlagList[flagName] = flagValue;
		}

		// Iterate the old flag list, and find any flags not present in the new flag list.
		for (const auto& [flag, value]: flagList)
		{
			auto newFlag = newFlagList.find(flag);
			if (newFlag == newFlagList.end())
				deletedFlagMsg << "flag deleted:\t" << flag << "=" << value << "\n";
		}

		// Update flag list, and save the changes
		flagList = std::move(newFlagList);
		npc->saveNPC();

		// Logging
		CString updateMsg, logMsg;
		updateMsg << "NPC flags of " << npc->getName() << " updated by " << m_accountName;
		logMsg << updateMsg << "\n"
			   << addedFlagMsg << deletedFlagMsg;
		npclog.out(logMsg);
		m_server->sendToNC(updateMsg);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_NPCADD(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to add a database npc.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	CString npcData = pPacket.readString("").guntokenize();
	CString npcName = npcData.readString("\n").trim();
	CString npcId = npcData.readString("\n");
	CString npcType = npcData.readString("\n");
	CString npcScripter = npcData.readString("\n");
	CString npcLevel = npcData.readString("\n");
	CString npcX = npcData.readString("\n");
	CString npcY = npcData.readString("\n");

	// Require a name
	if (npcName.isEmpty())
		return HandlePacketResult::Handled;

	auto level = m_server->getLevel(npcLevel.toString());
	if (level == nullptr)
	{
		m_server->sendToNC("Error adding database npc: Level does not exist");
		return HandlePacketResult::Handled;
	}

	auto newNpc = m_server->addServerNpc(strtoint(npcId), (float)strtofloat(npcX), (float)strtofloat(npcY), level, true);
	if (newNpc != nullptr)
	{
		m_server->assignNPCName(newNpc, npcName.toString());

		CString npcProps = CString() >> (char)NPCPROP_NAME << newNpc->getProp(NPCPROP_NAME) >> (char)NPCPROP_TYPE >> (char)npcType.length() << npcType >> (char)NPCPROP_CURLEVEL << newNpc->getProp(NPCPROP_CURLEVEL);

		// NOTE: This can't be sent to other clients, so rather than assemble the same packet twice just set the scripter separately.
		newNpc->setProps(CString() >> (char)NPCPROP_SCRIPTER >> (char)npcScripter.length() << npcScripter << npcProps);

		// Send packet to npc controls about new npc
		m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_NPCADD >> (int)newNpc->getId() << npcProps);

		// Persist NPC
		newNpc->saveNPC();

		// Logging
		CString logMsg;
		logMsg << "NPC " << newNpc->getName() << " added by " << m_accountName << "\n";
		npclog.out(logMsg);
		m_server->sendToNC(logMsg);
	}

	return HandlePacketResult::Handled;
}

#include "scripting/ScriptClass.h"

HandlePacketResult Player::msgPLI_NC_CLASSEDIT(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to edit a class.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	// {112}{class}
	CString className = pPacket.readString("");
	auto classObj = m_server->getClass(className.text());

	if (classObj != nullptr)
	{
		CString classCode(classObj->getSource().getSource());

		CString ret;
		ret >> (char)PLO_NC_CLASSGET >> (char)className.length() << className << classCode.gtokenize();
		sendPacket(ret);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_CLASSADD(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to add a class.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	// {113}{CHAR name length}{name}{GSTRING script}
	std::string className = pPacket.readChars(pPacket.readGUChar()).text();
	CString classCode = pPacket.readString("").guntokenize();

	bool hasClass = m_server->hasClass(className);
	m_server->updateClass(className, classCode.text());

	// Update Player-Weapons
	m_server->updateClassForPlayers(m_server->getClass(className));

	if (!hasClass)
	{
		CString ret;
		ret >> (char)PLO_NC_CLASSADD << className;
		m_server->sendPacketToType(PLTYPE_ANYNC, ret);
	}

	// Logging
	CString logMsg;
	logMsg << "Script " << className << " " << (!hasClass ? "added" : "updated") << " by " << m_accountName << "\n";
	npclog.out(logMsg);
	m_server->sendToNC(logMsg);
	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_CLASSDELETE(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to delete a class.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	std::string className = pPacket.readString("").text();

	CString logMsg;
	if (m_server->deleteClass(className))
	{
		CString ret;
		ret >> (char)PLO_NC_CLASSDELETE << className;
		m_server->sendPacketToType(PLTYPE_ANYNC, ret);
		logMsg << m_accountName << " has deleted class " << className << "\n";
	}
	else
		logMsg << "error: " << className << " does not exist on this server!\n";

	// Logging
	npclog.out(logMsg);
	m_server->sendToNC(logMsg);
	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_LOCALNPCSGET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to view level npcs.\n", m_accountName.text());
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
		npcDump << "Variables dump from level " << npcLevel->getLevelName() << "\n";

		for (auto npcId: npcLevel->getNPCs())
		{
			auto npc = m_server->getNPC(npcId);
			npcDump << "\n"
					<< npc->getVariableDump() << "\n";
		}

		sendPacket(CString() >> (char)PLO_NC_LEVELDUMP << npcDump.gtokenize());
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_WEAPONLISTGET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to view the weapon list.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	// Start our packet.
	CString ret;
	ret >> (char)PLO_NC_WEAPONLISTGET;

	// Iterate weapon list and send names
	for (const auto& [weaponName, weapon]: m_server->getWeaponList())
	{
		if (weapon->isDefault())
			continue;

		ret >> (char)weaponName.length() << weaponName;
	}

	sendPacket(ret);
	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_WEAPONGET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to view a weapon.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	// {116}{weapon}
	CString weaponName = pPacket.readString("");

	auto weapon = m_server->getWeapon(weaponName.toString());
	if (weapon != nullptr && !weapon->isDefault())
	{
		std::string script = weapon->getFullScript();
		std::replace(script.begin(), script.end(), '\n', '\xa7');

		if (getVersion() < NCVER_2_1)
		{
			sendPacket(CString() >> (char)PLO_NPCWEAPONADD >> (char)weaponName.length() << weaponName >> (char)0 >> (char)weapon->getImage().length() << weapon->getImage() >> (char)1 >> (short)script.length() << script);
		}
		else
		{
			sendPacket(CString() >> (char)PLO_NC_WEAPONGET >>
					   (char)weaponName.length() << weaponName >>
					   (char)weapon->getImage().length() << weapon->getImage() << script);
		}
	}
	else
		m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << m_accountName << " prob: weapon " << weaponName << " doesn't exist");

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_WEAPONADD(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to add a weapon.\n", m_accountName.text());
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
		weaponObj->updateWeapon(std::move(weaponImage), std::move(weaponCode));

		// Update Player-Weapons
		m_server->updateWeaponForPlayers(weaponObj);

		actionTaken = "updated";
	}
	else
	{
		// add weapon
		auto weapon = std::make_shared<Weapon>(weaponName, std::move(weaponImage), std::move(weaponCode), 0, true);
		bool success = m_server->NC_AddWeapon(weapon);
		if (success)
			actionTaken = "added";
	}

	// TODO(joey): Log message should come before the script is executed
	if (!actionTaken.isEmpty())
	{
		CString logMsg;
		logMsg << "Weapon/GUI-script " << weaponName << " " << actionTaken << " by " << m_accountName << "\n";
		npclog.out(logMsg);
		m_server->sendToNC(logMsg);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_WEAPONDELETE(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to delete a weapon.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	// {118}{weapon}
	CString weaponName = pPacket.readString("");

	CString logMsg;
	if (m_server->NC_DelWeapon(weaponName.toString()))
		logMsg << "Weapon " << weaponName << " deleted by " << m_accountName << "\n";
	else
		logMsg << m_accountName << " prob: weapon " << weaponName << " doesn't exist\n";

	// Logging
	npclog.out(logMsg);
	m_server->sendToNC(logMsg);
	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_NC_LEVELLISTGET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to view the level list.\n", m_accountName.text());
		return HandlePacketResult::Handled;
	}

	// Start our packet.
	CString ret;

	auto& levelList = m_server->getLevelList();
	if (!levelList.empty())
	{
		for (const auto& level: levelList)
			ret << level->getActualLevelName() << "\n";
	}

	sendPacket(CString() >> (char)PLO_NC_LEVELLIST << ret.gtokenize());
	return HandlePacketResult::Handled;
}

// Send's NC Address/Port to Player (RC Only)
void Player::sendNCAddr()
{
	// RC's only!
	if (!isRC() || !hasRight(PLPERM_NPCCONTROL))
		return;

	auto npcServer = m_server->getNPCServer();
	if (npcServer != nullptr)
	{
		// Grab NPCServer & Send
		CString npcServerIp = m_server->getAdminSettings().getStr("ns_ip", "auto").toLower();
		if (npcServerIp == "auto")
		{
			npcServerIp = m_server->getServerList().getServerIP();

			// Fix for localhost setups
			if (m_accountIpStr == m_playerSock->getLocalIp())
				npcServerIp = m_accountIpStr;
		}

		sendPacket(CString() >> (char)PLO_NPCSERVERADDR >> (short)npcServer->getId() << npcServerIp << "," << CString(m_server->getNCPort()));
	}
}

#endif
