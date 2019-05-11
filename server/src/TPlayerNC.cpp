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
	if (pPacket.bytesLeft())
	{
		int npcId = pPacket.readGUInt();
		printf("NPC Get: %d\n", npcId);
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

	int npcId = pPacket.readGUInt();
	TNPC *npc = server->getNPC(npcId);

	if (npc != 0 && !npc->isLevelNPC())
	{
		bool result = server->deleteNPC(npc, npc->getLevel(), true);
		if (result)
			server->sendPacketTo(PLTYPE_ANYNC, CString() >> (char)PLO_NC_NPCDELETE >> (int)npcId);
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

	int npcId = pPacket.readGUInt();

	TNPC *npc = server->getNPC(npcId);
	if (npc != 0 && !npc->isLevelNPC())
	{
		npc->resetNPC();
		server->sendToNC(CString("NPC ") << npc->getName() << " reset by " << accountName);
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
	int npcId = pPacket.readGUInt();
	TNPC *npc = server->getNPC(npcId);
	if (npc != nullptr)
	{
		CString code = npc->getScriptCode();
		sendPacket(CString() >> (char)PLO_NC_NPCSCRIPT >> (int)npcId << code.replaceAll("\xa7", "\n").gtokenize());
	}
//	else printf("no weapons exist\n");

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

	int npcId = pPacket.readGUInt();
	printf("NPC Get Flags: %d\n", npcId);

	// TODO(joey): temporarily used weapons to get this setup
	/*
	auto weaponList = server->getWeaponList();
	if (!weaponList->empty())
	{
		auto it = weaponList->begin();
		TWeapon *weapon = it->second;
		if (weapon != 0)
		{
			CString ret;
			ret >> (char)PLO_NC_NPCFLAGS >> (int)npcId << weapon->getFullScript().replaceAll("\xa7", "\n").gtokenize();
			sendPacket(ret);
		}
	}
	else printf("no weapons exist\n");
	*/

	return true;
}

bool TPlayer::msgPLI_NC_NPCSCRIPTSET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to set a database npc script.\n", accountName.text());
		return false;
	}

	int npcId = pPacket.readGUInt();
	CString npcScript = pPacket.readString("").guntokenize();

	TNPC *npc = server->getNPC(npcId);
	if (npc != nullptr)
	{
		npc->setScriptCode(npcScript);
		npc->saveNPC();

		server->sendToNC(CString("Script ") << npc->getName() << " updated by " << accountName);
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

	int npcId = pPacket.readGUInt();
	CString npcFlags = pPacket.readString("").guntokenize();
	printf("NPC Set Flags: %d\n", npcId);
	printf("NPC Flags: %s\n", npcFlags.text());

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

	// TODO(joey): unique names for db-npcs!

	TLevel *level = server->getLevel(npcLevel);
	if (level == nullptr)
	{
		server->sendToNC("Error adding database npc: Level does not exist");
		return true;
	}

	TNPC *newNpc = server->addServerNpc(strtoint(npcId), strtofloat(npcX), strtofloat(npcY), level, true);

	if (newNpc != nullptr)
	{
		CString npcProps = CString()
				>> (char)NPCPROP_NAME >> (char)npcName.length() << npcName
				>> (char)NPCPROP_TYPE >> (char)npcType.length() << npcType
				>> (char)NPCPROP_CURLEVEL << newNpc->getProp(NPCPROP_CURLEVEL);

		// NOTE: This can't be sent to other clients, so rather than assemble the same packet twice just set the scripter separately.
		newNpc->setProps(CString() >> (char)NPCPROP_SCRIPTER >> (char)npcScripter.length() << npcScripter << npcProps);

		// Send packet to npc controls about new npc
		server->sendPacketTo(PLTYPE_ANYNC, CString() >> (char)PLO_NC_NPCADD >> (int)newNpc->getId() << npcProps);

		// Persist NPC
		newNpc->setPersist(true);
		newNpc->saveNPC();
	}

	return true;
}

bool TPlayer::msgPLI_NC_CLASSEDIT(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to add a class.\n", accountName.text());
		return false;
	}

	// {112}{class}
	CString className = pPacket.readString("");
	printf("Update Class by Name: %s\n", className.text());

	// {162}{CHAR name length}{name}{GSTRING script}
	CString testScript = CString() << "function onCreated() {"
			<< "  var test = \"hey\" + \"you\""
			<< "}";

	CString ret;
	ret >> (char)PLO_NC_CLASSGET >> (char)className.length() << className << testScript.gtokenize();
	sendPacket(ret);
	return true;
}

bool TPlayer::msgPLI_NC_CLASSADD(CString& pPacket)
{
	// {113}{CHAR name length}{name}{GSTRING script}
	CString className = pPacket.readChars(pPacket.readGUChar());
	CString classData = pPacket.readString("").guntokenize();

	printf("Class %s => \n%s\n", className.text(), classData.text());

	CString ret;
	ret >> (char)PLO_NC_CLASSADD << className;
	sendPacket(ret);


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

	CString msg =  "Variables dump from npc\n\nASDFG";
	msg.gtokenizeI();

	CString ret;
	ret >> (char)PLO_NC_LEVELDUMP << msg;
	sendPacket(ret);

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
		// TODO(joey): this isnt working on versions < RC 2.05
		sendPacket(CString() >> (char)PLO_NC_WEAPONGET >>
			(char)weaponName.length() << weaponName >>
			(char)weapon->getImage().length() << weapon->getImage() <<
			weapon->getFullScript()); // .replaceAll("\n", "\xa7")); // getFullScript() returns a string already processed
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
	CString weaponName = pPacket.readChars(pPacket.readGUChar());
	CString weaponImage = pPacket.readChars(pPacket.readGUChar());
	CString weaponCode = pPacket.readString("");

	// Weapon `NAME` added/updated by `ACCOUNT`

	// Find Weapon
	TWeapon *weaponObj = server->getWeapon(weaponName);
	if (weaponObj != 0)
	{
		// default weapon, don't update!
		if (weaponObj->isDefault())
			return true;

		// Update Weapon
		weaponObj->updateWeapon(server, weaponImage, weaponCode);

		// Update Player-Weapons
		server->NC_UpdateWeapon(weaponObj);
		server->sendPacketTo(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << "Weapon/GUI-script " << weaponName << " updated by " << accountName);
		return true;
	}

	// add weapon
	bool success = server->NC_AddWeapon(new TWeapon(server, weaponName, weaponImage, weaponCode, 0, true));
	if (success)
		server->sendPacketTo(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << "Weapon/GUI-script " << weaponName << " added by " << accountName);
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
	
	if (server->NC_DelWeapon(weaponName))
		server->sendPacketTo(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << "Weapon " << weaponName << " deleted by " << accountName);
	else
		server->sendPacketTo(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << accountName << " prob: weapon " << weaponName << " doesn't exist");

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
	CString ret = CString() >> (char)PLO_NC_LEVELLIST;

	auto levelList = server->getLevelList();
	if (!levelList->empty())
	{
		for (auto it = levelList->begin(); it != levelList->end(); ++it)
			ret << (*it)->getActualLevelName() << "\n";
	}

	sendPacket(ret);
	return true;
}
#endif

/*
	NPC-Server Functionality
*/

// Send's NC Address/Port to Player (RC Only)
#ifdef V8NPCSERVER
void TPlayer::sendNCAddr()
{
	// RC's only!
	if (!isRC() || !hasRight(PLPERM_NPCCONTROL))
		return;

	// Grab NPCServer & Send
	CString npcServerIp = server->getAdminSettings()->getStr("ns_ip", "127.0.0.1");
	sendPacket(CString() >> (char)PLO_NPCSERVERADDR >> (short)0 << npcServerIp << "," << CString(server->getNCPort()));
}
#endif