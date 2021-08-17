#include "IDebug.h"
#include <vector>
#include <math.h>

#include "IEnums.h"
#include "TServer.h"
#include "TPlayer.h"
#include "TWeapon.h"
#include "TNPC.h"
#include "TLevel.h"

#define serverlog	server->getServerLog()
#define npclog		server->getNPCLog()
#define rclog		server->getRCLog()

typedef bool (TPlayer::*TPLSock)(CString&);
extern std::vector<TPLSock> TPLFunc;		// From TPlayer.cpp

#ifdef V8NPCSERVER
bool TPlayer::msgPLI_NC_NPCGET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to get a database npc.\n", accountName.text());
		return false;
	}

	// RC3 keeps sending empty packets of this, yet still uses NPCGET to fetch npcs. Maybe its for pinging the server
	// for updated level information on database npcs? Just a thought..
	// 5/26/2019 - confirmed, this is the npc-server pinging the gserver.
	if (pPacket.bytesLeft())
	{
		unsigned int npcId = pPacket.readGUInt();

		TNPC *npc = server->getNPC(npcId);
		if (npc != nullptr)
		{
			CString npcDump = npc->getVariableDump();
			sendPacket(CString() >> (char)PLO_NC_NPCATTRIBUTES << npcDump.gtokenize());
		}
	}

	return true;
}

bool TPlayer::msgPLI_NC_NPCDELETE(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to delete a database npc.\n", accountName.text());
		return false;
	}

	unsigned int npcId = pPacket.readGUInt();
	TNPC *npc = server->getNPC(npcId);

	if (npc != nullptr && npc->getType() == NPCType::DBNPC)
	{
		CString npcName = npc->getName();
		bool result = server->deleteNPC(npc, true);
		if (result)
		{
			server->sendPacketTo(PLTYPE_ANYNC, CString() >> (char)PLO_NC_NPCDELETE >> (int)npcId);

			CString logMsg;
			logMsg << "NPC " << npcName << " deleted by " << accountName << "\n";
			npclog.out(logMsg);
			server->sendToNC(logMsg);
		}
	}

	return true;
}

bool TPlayer::msgPLI_NC_NPCRESET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to reset a database npc.\n", accountName.text());
		return false;
	}

	unsigned int npcId = pPacket.readGUInt();

	TNPC *npc = server->getNPC(npcId);
	if (npc != nullptr && npc->getType() == NPCType::DBNPC)
	{
		npc->resetNPC();

		CString logMsg;
		logMsg << "NPC script of " << npc->getName() << " reset by " << accountName << "\n";
		npclog.out(logMsg);
		server->sendToNC(logMsg);
	}

	return true;
}

bool TPlayer::msgPLI_NC_NPCSCRIPTGET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to get a database npc script.\n", accountName.text());
		return false;
	}

	// {160}{INT id}{GSTRING script}
	unsigned int npcId = pPacket.readGUInt();
	TNPC *npc = server->getNPC(npcId);
	if (npc != nullptr)
	{
		CString code = npc->getSource().getSource();
		sendPacket(CString() >> (char)PLO_NC_NPCSCRIPT >> (int)npcId << code.gtokenize());
	}
//	else printf("npc doesn't exist\n");

	return true;
}

bool TPlayer::msgPLI_NC_NPCWARP(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to warp a database npc.\n", accountName.text());
		return false;
	}

	unsigned int npcId = pPacket.readGUInt();
	float npcX = (float)pPacket.readGUChar() / 2.0f;
	float npcY = (float)pPacket.readGUChar() / 2.0f;
	CString npcLevel = pPacket.readString("");

	TNPC *npc = server->getNPC(npcId);
	if (npc != nullptr)
	{
		TLevel *newLevel = server->getLevel(npcLevel);
		if (newLevel != nullptr)
			npc->warpNPC(newLevel, npcX, npcY);
	}

	return true;
}

bool TPlayer::msgPLI_NC_NPCFLAGSGET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to get a database npc flags.\n", accountName.text());
		return false;
	}

	unsigned int npcId = pPacket.readGUInt();
	TNPC *npc = server->getNPC(npcId);
	if (npc != nullptr)
	{
		CString flagListStr;

		auto flagList = npc->getFlagList();
		for (auto it = flagList->begin(); it != flagList->end(); ++it)
			flagListStr << (*it).first << "=" << (*it).second << "\n";

		sendPacket(CString() >> (char)PLO_NC_NPCFLAGS >> (int)npcId << flagListStr.gtokenize());
	}

	return true;
}

bool TPlayer::msgPLI_NC_NPCSCRIPTSET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to set a database npc script.\n", accountName.text());
		return false;
	}

	unsigned int npcId = pPacket.readGUInt();
	CString npcScript = pPacket.readString("").guntokenize();

	TNPC *npc = server->getNPC(npcId);
	if (npc != nullptr)
	{
		npc->setScriptCode(convertCString(npcScript));
		npc->saveNPC();

		CString logMsg;
		logMsg << "NPC script of " << npc->getName() << " updated by " << accountName << "\n";
		npclog.out(logMsg);
		server->sendToNC(logMsg);
	}

	return true;
}

bool TPlayer::msgPLI_NC_NPCFLAGSSET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to set a database npc flags.\n", accountName.text());
		return false;
	}

	unsigned int npcId = pPacket.readGUInt();
	CString npcFlags = pPacket.readString("").guntokenize();

	TNPC *npc = server->getNPC(npcId);
	if (npc != nullptr)
	{
		auto flagList = npc->getFlagList();
		auto newFlags = npcFlags.tokenize("\n");
		
		CString addedFlagMsg, deletedFlagMsg;
		std::unordered_map<std::string, CString> newFlagList;

		// Iterate the new list of flags from the client
		for (auto it = newFlags.begin(); it != newFlags.end(); ++it)
		{
			CString flag = *it;
			std::string flagName = flag.readString("=").text();
			CString flagValue = flag.readString("");

			// Check if the flag is a new flag, or if it has been updated
			auto oldFlag = flagList->find(flagName);
			if (oldFlag == flagList->end())
				addedFlagMsg << "flag added:\t" << flagName << "=" << flagValue << "\n";
			else if (oldFlag->second != flagValue)
			{
				addedFlagMsg << "flag added:\t" << flagName << "=" << flagValue << "\n";
				deletedFlagMsg << "flag deleted:\t" << oldFlag->first << "=" << oldFlag->second << "\n";
				flagList->erase(oldFlag);
			}

			// Add the flag to the new list
			newFlagList[flagName] = flagValue;
		}

		// Iterate the old flag list, and find any flags not present in the new flag list.
		for (auto it = flagList->begin(); it != flagList->end(); ++it)
		{
			auto newFlag = newFlagList.find(it->first);
			if (newFlag == newFlagList.end())
				deletedFlagMsg << "flag deleted:\t" << it->first << "=" << it->second << "\n";
		}

		// Update flag list, and save the changes
		*flagList = std::move(newFlagList);
		npc->saveNPC();

		// Logging
		CString updateMsg, logMsg;
		updateMsg << "NPC flags of " << npc->getName() << " updated by " << accountName;
		logMsg << updateMsg << "\n" << addedFlagMsg << deletedFlagMsg;
		npclog.out(logMsg);
		server->sendToNC(updateMsg);
	}

	return true;
}

bool TPlayer::msgPLI_NC_NPCADD(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to add a database npc.\n", accountName.text());
		return false;
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
		return true;

	TLevel *level = server->getLevel(npcLevel);
	if (level == nullptr)
	{
		server->sendToNC("Error adding database npc: Level does not exist");
		return true;
	}

	TNPC *newNpc = server->addServerNpc(strtoint(npcId), (float)strtofloat(npcX), (float)strtofloat(npcY), level, true);

	if (newNpc != nullptr)
	{
		server->assignNPCName(newNpc, npcName.text());

		CString npcProps = CString()
				>> (char)NPCPROP_NAME << newNpc->getProp(NPCPROP_NAME)
				>> (char)NPCPROP_TYPE >> (char)npcType.length() << npcType
				>> (char)NPCPROP_CURLEVEL << newNpc->getProp(NPCPROP_CURLEVEL);

		// NOTE: This can't be sent to other clients, so rather than assemble the same packet twice just set the scripter separately.
		newNpc->setProps(CString() >> (char)NPCPROP_SCRIPTER >> (char)npcScripter.length() << npcScripter << npcProps);

		// Send packet to npc controls about new npc
		server->sendPacketTo(PLTYPE_ANYNC, CString() >> (char)PLO_NC_NPCADD >> (int)newNpc->getId() << npcProps);

		// Persist NPC
		newNpc->saveNPC();

		// Logging
		CString logMsg;
		logMsg << "NPC " << newNpc->getName() << " added by " << accountName << "\n";
		npclog.out(logMsg);
		server->sendToNC(logMsg);
	}

	return true;
}

#include "TScriptClass.h"

bool TPlayer::msgPLI_NC_CLASSEDIT(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to edit a class.\n", accountName.text());
		return false;
	}

	// {112}{class}
	CString className = pPacket.readString("");
	auto classObj = server->getClass(className.text());

	if (classObj != nullptr)
	{
		CString classCode(classObj->source());

		CString ret;
		ret >> (char)PLO_NC_CLASSGET >> (char)className.length() << className << classCode.gtokenize();
		sendPacket(ret);
	}

	return true;
}

bool TPlayer::msgPLI_NC_CLASSADD(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to add a class.\n", accountName.text());
		return false;
	}

	// {113}{CHAR name length}{name}{GSTRING script}
	std::string className = pPacket.readChars(pPacket.readGUChar()).text();
	CString classCode = pPacket.readString("").guntokenize();

	bool hasClass = server->hasClass(className);
	server->updateClass(className, classCode.text());

	if (!hasClass)
	{
		CString ret;
		ret >> (char)PLO_NC_CLASSADD << className;
		server->sendPacketTo(PLTYPE_ANYNC, ret);
	}

	// Logging
	CString logMsg;
	logMsg << "Script " << className << " " << (!hasClass ? "added" : "updated") << " by " << accountName << "\n";
	npclog.out(logMsg);
	server->sendToNC(logMsg);
	return true;
}

bool TPlayer::msgPLI_NC_CLASSDELETE(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to delete a class.\n", accountName.text());
		return false;
	}

	std::string className = pPacket.readString("").text();
	
	CString logMsg;
	if (server->deleteClass(className))
	{
		CString ret;
		ret >> (char)PLO_NC_CLASSDELETE << className;
		server->sendPacketTo(PLTYPE_ANYNC, ret);
		logMsg << accountName << " has deleted class " << className << "\n";
	}
	else
		logMsg << "error: " << className << " does not exist on this server!\n";

	// Logging
	npclog.out(logMsg);
	server->sendToNC(logMsg);
	return true;
}

bool TPlayer::msgPLI_NC_LOCALNPCSGET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to view level npcs.\n", accountName.text());
		return false;
	}

	// {114}{level}
	CString level = pPacket.readString("");
	if (level.isEmpty())
		return true;

	TLevel *npcLevel = server->getLevel(level);
	if (npcLevel != nullptr)
	{
		CString npcDump;
		// Variables dump from level mapname (level.nw) 
		npcDump << "Variables dump from level " << npcLevel->getLevelName() << "\n";

		auto npcList = npcLevel->getLevelNPCs();
		for (auto it = npcList->begin(); it != npcList->end(); ++it)
			npcDump << "\n" << (*it)->getVariableDump() << "\n";

		sendPacket(CString() >> (char)PLO_NC_LEVELDUMP << npcDump.gtokenize());
	}

	return true;
}

bool TPlayer::msgPLI_NC_WEAPONLISTGET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to view the weapon list.\n", accountName.text());
		return false;
	}

	// Start our packet.
	CString ret;
	ret >> (char)PLO_NC_WEAPONLISTGET;

	// Iterate weapon list and send names
	auto weaponList = server->getWeaponList();
	for (auto it = weaponList->begin(); it != weaponList->end(); ++it)
	{
		if (it->second->isDefault())
			continue;

		CString weaponName = it->second->getName();
		ret >> (char)weaponName.length() << weaponName;
	}
	
	sendPacket(ret);
	return true;
}

bool TPlayer::msgPLI_NC_WEAPONGET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to view a weapon.\n", accountName.text());
		return false;
	}

	// {116}{weapon}
	CString weaponName = pPacket.readString("");

	TWeapon *weapon = server->getWeapon(weaponName);
	if (weapon != 0 && !weapon->isDefault())
	{
		std::string script = weapon->getFullScript();
		std::replace(script.begin(), script.end(), '\n', '\xa7');
		
		if (getVersion() < NCVER_2_1)
		{
			sendPacket(CString() >> (char)PLO_NPCWEAPONADD
				>> (char)weaponName.length() << weaponName
				>> (char)0 >> (char)weapon->getImage().length() << weapon->getImage()
				>> (char)1 >> (short)script.length() << script);
		}
		else
		{
			sendPacket(CString() >> (char)PLO_NC_WEAPONGET >>
				(char)weaponName.length() << weaponName >>
				(char)weapon->getImage().length() << weapon->getImage() <<
				script);
		}
	}
	else server->sendPacketTo(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << accountName << " prob: weapon " << weaponName << " doesn't exist");

	return true;
}

bool TPlayer::msgPLI_NC_WEAPONADD(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to add a weapon.\n", accountName.text());
		return false;
	}

	// {117}{CHAR weapon length}{weapon}{CHAR image length}{image}{code}
	std::string weaponName = convertCString(pPacket.readChars(pPacket.readGUChar()));
	std::string weaponImage = convertCString(pPacket.readChars(pPacket.readGUChar()));
	std::string weaponCode = convertCString(pPacket.readString(""));

	std::replace(weaponCode.begin(), weaponCode.end(), '\xa7', '\n');

	CString actionTaken;

	// Find Weapon
	TWeapon *weaponObj = server->getWeapon(weaponName);
	if (weaponObj != 0)
	{
		// default weapon, don't update!
		if (weaponObj->isDefault())
			return true;

		// Update Weapon
		weaponObj->updateWeapon(std::move(weaponImage), std::move(weaponCode));

		// Update Player-Weapons
		server->updateWeaponForPlayers(weaponObj);

		actionTaken = "updated";
	}
	else
	{
		// add weapon
		bool success = server->NC_AddWeapon(new TWeapon(server, weaponName, std::move(weaponImage), std::move(weaponCode), 0, true));
		if (success)
			actionTaken = "added";
	}

	// TODO(joey): Log message should come before the script is executed
	if (!actionTaken.isEmpty())
	{
		CString logMsg;
		logMsg << "Weapon/GUI-script " << weaponName << " " << actionTaken << " by " << accountName << "\n";
		npclog.out(logMsg);
		server->sendToNC(logMsg);
	}

	return true;
}

bool TPlayer::msgPLI_NC_WEAPONDELETE(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to delete a weapon.\n", accountName.text());
		return false;
	}

	// {118}{weapon}
	CString weaponName = pPacket.readString("");
	
	CString logMsg;
	if (server->NC_DelWeapon(weaponName))
		logMsg << "Weapon " << weaponName << " deleted by " << accountName << "\n";
	else
		logMsg << accountName << " prob: weapon " << weaponName << " doesn't exist\n";

	// Logging
	npclog.out(logMsg);
	server->sendToNC(logMsg);
	return true;
}

bool TPlayer::msgPLI_NC_LEVELLISTGET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to view the level list.\n", accountName.text());
		return false;
	}

	// Start our packet.
	CString ret;

	auto levelList = server->getLevelList();
	if (!levelList->empty())
	{
		for (auto it = levelList->begin(); it != levelList->end(); ++it)
			ret << (*it)->getActualLevelName() << "\n";
	}

	sendPacket(CString() >> (char)PLO_NC_LEVELLIST << ret.gtokenize());
	return true;
}

// Send's NC Address/Port to Player (RC Only)
void TPlayer::sendNCAddr()
{
	// RC's only!
	if (!isRC() || !hasRight(PLPERM_NPCCONTROL))
		return;

	TPlayer *npcServer = server->getNPCServer();
	if (npcServer != nullptr)
	{
		// Grab NPCServer & Send
		CString npcServerIp = server->getAdminSettings()->getStr("ns_ip", "auto").toLower();
		if (npcServerIp == "auto") {
			npcServerIp = server->getServerList()->getServerIP();

			// Fix for localhost setups
			if (accountIpStr == playerSock->getLocalIp())
				npcServerIp = accountIpStr;
		}

		sendPacket(CString() >> (char)PLO_NPCSERVERADDR >> (short)npcServer->getId() << npcServerIp << "," << CString(server->getNCPort()));
	}
}

#endif