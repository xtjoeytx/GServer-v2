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

	// RC3 keeps sending empty packets of this, yet still uses NPCGET to fetch npcs.
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
	printf("NPC Delete: %d\n", npcId);

	return true;
}

bool TPlayer::msgPLI_NC_NPCRESET(CString& pPacket)
{
	if (!isNC())
	{
		npclog.out("[Hack] %s attempted to reset a database npc.\n", accountName.text());
		return false;
	}

	// NPC `NAME` reset by `ACCOUNT`
	int npcId = pPacket.readGUInt();
	printf("NPC Reset: %d\n", npcId);

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
		if (newLevel == 0) {
			printf("Err finding level\n");
			return true;
		}
		
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
		npc->setScriptCode(npcScript, true);
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
	CString npcName = npcData.readString("\n");
	CString npcId = npcData.readString("\n");
	CString npcType = npcData.readString("\n");
	CString npcScripter = npcData.readString("\n");
	CString npcLevel = npcData.readString("\n");
	CString npcX = npcData.readString("\n");
	CString npcY = npcData.readString("\n");

	TLevel *level = server->getLevel(npcLevel);
	if (level == nullptr)
	{
		server->sendToNC("Error adding database npc: Level does not exist");
		return true;
	}

	TNPC *newNpc = server->addServerNpc(strtoint(npcId), npcName.text(), npcType.text(), npcScripter.text(), strtofloat(npcX), strtofloat(npcY), level, true);
	if (newNpc != nullptr)
	{
		// Send packet to npc controls about new npc
		CString ret;
		ret >> (char)PLO_NC_NPCADD >> (int)strtoint(npcId) >> (char)50 >> (char)npcName.length() << npcName >> (char)51 >> (char)npcType.length() << npcType >> (char)52 >> (char)npcLevel.length() << npcLevel;
		server->sendPacketTo(PLTYPE_ANYNC, ret);
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
	printf("Get weapon: %s\n", weaponName.text());

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

// Send Maps
void TPlayer::sendNC_Maps()
{
	std::vector<TMap *> * mapList = server->getMapList();
	for (std::vector<TMap *>::const_iterator i = mapList->begin(); i != mapList->end(); ++i)
		server->NC_SendMap(*i);
}

// Send Levels
void TPlayer::sendNC_Levels()
{
	std::vector<TLevel *> * levelList = server->getLevelList();
	for (std::vector<TLevel *>::const_iterator i = levelList->begin(); i != levelList->end(); ++i)
	{
		(*i)->reload();
		server->NC_SendLevel(*i);
	}
}

// Send Weapons
void TPlayer::sendNC_Weapons()
{
	std::map<CString, TWeapon *> * weaponList = server->getWeaponList();
	for (std::map<CString, TWeapon *>::const_iterator i = weaponList->begin(); i != weaponList->end(); ++i)
	{
		if (i->second->isDefault())
			continue;

		CString weaponName = i->second->getName();
		CString imageName  = i->second->getImage();
		CString scriptData = i->second->getFullScript();
		sendPacket(CString() >> (char)PLO_NPCWEAPONADD >> (char)weaponName.length() << weaponName >> (char)imageName.length() << imageName << scriptData);
	}
}

// Send's NC Address/Port to Player (RC Only)
void TPlayer::sendNCAddr()
{
	// RC's only!
	if (!isRC() || !hasRight(PLPERM_NPCCONTROL))
		return;

	// Grab NPCServer & Send
#ifdef V8NPCSERVER
	CString npcServerIp = server->getAdminSettings()->getStr("ns_ip", "127.0.0.1");
	sendPacket(CString() >> (char)PLO_NPCSERVERADDR >> (short)0 << npcServerIp << "," << CString(server->getNCPort()));
#else
	TPlayer *npcServer = server->getNPCServer();
	if (npcServer != 0)
	{
		CString npcServerIp = server->getAdminSettings()->getStr("ns_ip", "AUTO");
		if (npcServerIp == "AUTO")
			npcServerIp = npcServer->getSocket()->getRemoteIp();
		sendPacket(CString() >> (char)PLO_NPCSERVERADDR >> (short)npcServer->getId() << npcServerIp << "," << CString(server->getNCPort()));
	}
#endif
}

void TPlayer::sendNC_GMapList()
{
	if (!isNPCServer())
		return;

	// Get the gmap list.
	CSettings* settings = server->getSettings();
	std::vector<CString> gmaps = settings->getStr("maps").guntokenize().tokenize("\n");

	// Assemble the gmap packet.
	CString packet;
	packet >> (char)PLO_NC_CONTROL >> (char)NCO_GMAPLIST >> (short)0;
	for (std::vector<CString>::iterator i = gmaps.begin(); i != gmaps.end(); ++i)
		packet >> (short)(*i).length() << (*i);

	// Send it.
	sendPacket(packet);
}

#ifndef V8NPCSERVER
// Request Query from NPC-Server
bool TPlayer::msgPLI_NC_QUERY(CString& pPacket)
{
	if (!isNPCServer())
		return true;

	// NPC-Server Packets
	int type = pPacket.readGUChar();
	switch (type)
	{
		// NPC-Server Log
		case NCI_NPCLOG:
			npclog.out(pPacket.readString(""));
			break;

		// Send Weapons to NPC-Server
		case NCI_GETWEAPONS:
			sendNC_Weapons();
			break;

		// Send Levels to NPC-Server
		case NCI_GETLEVELS:
			break;

		// Send PM
		case NCI_SENDPM:
		{
			TPlayer *player = server->getPlayer(pPacket.readGUShort());
			if (player != 0)
				player->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)id << pPacket.readString(""));
			break;
		}

		// Send RC
		case NCI_SENDTORC:
			server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << pPacket.readString(""));
			break;

		// Weapon Add/Update
		case NCI_WEAPONADD:
		{
			// Packet Data
			CString weaponName  = pPacket.readChars(pPacket.readGUChar());
			CString weaponImage = pPacket.readChars(pPacket.readGUChar());
			CString weaponCode  = pPacket.readString("");
			
			// Find Weapon
			TWeapon *weaponObj = server->getWeapon(weaponName);
			if (weaponObj != 0)
			{
				// default weapon, don't update!
				if (weaponObj->isDefault())
					break;

				// Update Weapon
				weaponObj->updateWeapon(server, weaponImage, weaponCode);
			
				// Update Player-Weapons
				server->NC_UpdateWeapon(weaponObj);
				break;
			}

			server->NC_AddWeapon(new TWeapon(server, weaponName, weaponImage, weaponCode, 0, true));
			break;
		}

		// Weapon Delete
		case NCI_WEAPONDEL:
			server->NC_DelWeapon(pPacket.readString(""));
			break;

		// Player Props
		case NCI_PLAYERPROPSSET:
		{
			TPlayer *pl = server->getPlayer(pPacket.readGUShort());
			if (pl != 0)
				pl->setProps(pPacket, true, true, this);
			break;
		}

		case NCI_PLAYERWEAPONSGET:
		{
			unsigned short pid = pPacket.readGUShort();
			TPlayer* pl = server->getPlayer(pid);
			if (pl != 0)
			{
				CString w;
				std::vector<CString>* weapons = pl->getWeaponList();
				for (std::vector<CString>::iterator i = weapons->begin(); i != weapons->end(); ++i)
				{
					if (!w.isEmpty()) w << ",";
					w << "\"" << i->replaceAll("\"", "\"\"") << "\"";
				}

				sendPacket(CString() >> (char)PLO_NC_CONTROL >> (char)NCO_PLAYERWEAPONS >> (short)pid << w);
			}
			break;
		}

		case NCI_PLAYERPACKET:
		{
			unsigned short pid = pPacket.readGUShort();
			TPlayer* pl = server->getPlayer(pid);
			if (pl != 0)
			{
				CString packet = pPacket.readString("");
				unsigned char id = packet.readGUChar();

				// Call the function assigned to the packet id.
				if (!(*pl.*TPLFunc[id])(packet))
					server->deletePlayer(pl);
			}
			break;
		}

		case NCI_PLAYERWEAPONADD:
		{
			unsigned short pid = pPacket.readGUShort();
			TPlayer* pl = server->getPlayer(pid);
			if (pl != 0)
				pl->addWeapon(pPacket.readString(""));
			break;
		}

		case NCI_PLAYERWEAPONDEL:
		{
			unsigned short pid = pPacket.readGUShort();
			TPlayer* pl = server->getPlayer(pid);
			if (pl != 0)
				pl->deleteWeapon(pPacket.readString(""));
			break;
		}

		case NCI_LEVELGET:
		{
			server->NC_SendLevel(TLevel::findLevel(pPacket.readString(""), server));
			break;
		}

		case NCI_NPCPROPSSET:
		{
			unsigned int npcId = pPacket.readGUInt();
			CString npcProps = pPacket.readString("");

			TNPC* npc = server->getNPC(npcId);
			if (npc == 0) return true;

			// Set the npc's props.
			npc->setProps(npcProps);

			// Find the level.
			TLevel* level = npc->getLevel();
			TMap* map = 0;
			if (level != 0) map = level->getMap();

			// Send the props.
			server->sendPacketToLevel(CString() >> (char)PLO_NPCPROPS >> (int)npcId << npcProps, map, level, 0, true);
			break;
		}

		case NCI_NPCWARP:
		{
			unsigned int npcId = pPacket.readGUInt();
			float loc[] = {(float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f};
			CString levelName = pPacket.readString("");

			TNPC* npc = server->getNPC(npcId);
			if (npc == 0) return true;
			if (npc->isLevelNPC()) return true;

			TLevel* levelNew = TLevel::findLevel(levelName, server);
			if (levelNew == 0) return true;
			TMap* mapNew = levelNew->getMap();

			// Remove the NPC.
			TLevel* levelOld = npc->getLevel();
			if (levelOld)
			{
				levelOld->removeNPC(npc);
				server->sendPacketToAll(CString() >> (char)PLO_NPCDEL2 >> (char)levelOld->getLevelName().length() << levelOld->getLevelName() >> (int)npc->getId());
			}

			// Set its new level.
			npc->setLevel(levelNew);
			server->sendPacketToLevel(CString() >> (char)PLO_NPCPROPS >> (int)npc->getId() << npc->getProps(0), mapNew, levelNew, this, true);
			break;
		}

		// Send RPG Message
		case NCI_SENDRPGMESSAGE:
		{
			TPlayer *player = server->getPlayer(pPacket.readGUShort());
			if (player != 0 && player->isClient() && player->getVersion() >= CLVER_2_1)
				player->sendPacket(CString() >> (char)PLO_RPGWINDOW << "\"" << pPacket.readString("") << "\"");
			break;
		}
		
		// NPCServer -> Player -- Set Flag
		case NCI_PLAYERFLAGSET:
		{
			TPlayer* pl = server->getPlayer(pPacket.readGUShort());
			if (pl != 0)
			{
				CString flagName  = pPacket.readString("=");
				CString flagValue = pPacket.readString("");
				pl->setFlag(flagName, flagValue, true, false);
			}
			break;
		}

		// NPCServer -> Player --> Sign Message
		case NCI_SAY2SIGN:
		{
			TPlayer* pl = server->getPlayer(pPacket.readGUShort());
			if (pl != 0)
				pl->sendPacket(CString() >> (char)PLO_SAY2 << pPacket.readString("").replaceAll("\n", "#b"));
			break;
		}

		case NCI_PLAYERSTATUSSET:
		{
			TPlayer* pl = server->getPlayer(pPacket.readGUShort());
			if (pl != 0)
			{
				unsigned char operation = pPacket.readGUChar();
				unsigned char status = (unsigned char)pl->getStatus();
				switch (operation)
				{
					default:
					case 0:
						status |= pPacket.readGUChar();
						break;
					case 1:
						status &= ~(pPacket.readGUChar());
						break;
				}
				pl->setProps(CString() >> (char)PLPROP_STATUS >> (char)status, true, true);
			}
		}

		case NCI_NPCMOVE:
		{
			TNPC* npc = server->getNPC(pPacket.readGUInt());
			if (npc != 0)
			{
				// Retrieve information from the packet.
				short start_pos[] = {pPacket.readGShort(), pPacket.readGShort()};
				short delta[] = {pPacket.readGShort(), pPacket.readGShort()};
				//unsigned short time = pPacket.readGUShort();
				//unsigned char options = pPacket.readGUChar();

				// Process the positions.
				start_pos[0] = (short)((unsigned short)start_pos[0] >> 1) * ((start_pos[0] & 0x0001) ? -1 : 1);
				start_pos[1] = (short)((unsigned short)start_pos[1] >> 1) * ((start_pos[1] & 0x0001) ? -1 : 1);
				delta[0] = (short)((unsigned short)delta[0] >> 1) * ((delta[0] & 0x0001) ? -1 : 1);
				delta[1] = (short)((unsigned short)delta[1] >> 1) * ((delta[1] & 0x0001) ? -1 : 1);

				// Calculate the finish location.
				unsigned short finish_pos[] = {static_cast<unsigned short>(start_pos[0] + delta[0]), static_cast<unsigned short>(start_pos[1] + delta[1])};

				// Repackage the positions.
				start_pos[0] = (short)((unsigned short)abs(start_pos[0]) << 1) | (start_pos[0] < 0 ? 0x0001 : 0x0000);
				start_pos[1] = (short)((unsigned short)abs(start_pos[1]) << 1) | (start_pos[1] < 0 ? 0x0001 : 0x0000);
				delta[0] = (short)((unsigned short)abs(delta[0]) << 1) | (delta[0] < 0 ? 0x0001 : 0x0000);
				delta[1] = (short)((unsigned short)abs(delta[1]) << 1) | (delta[1] < 0 ? 0x0001 : 0x0000);

				// Update the NPC's position now.
				// Don't send to any players nearby as they will get the move event.
				npc->setProps(CString() >> (char)NPCPROP_X2 >> (short)finish_pos[0] >> (char)NPCPROP_Y2 >> (short)finish_pos[1]);

				// Send the packet to all nearby players now.
				if (npc->getLevel() != 0)
					server->sendPacketToLevel(CString() >> (char)PLO_MOVE2 << (pPacket.text() + 2), npc->getLevel()->getMap(), npc->getLevel(), nullptr, true);
			}
		}
	}

	return true;
}
#endif
