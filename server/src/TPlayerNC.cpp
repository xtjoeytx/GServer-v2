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
	TPlayer *npcServer = server->getNPCServer();
	if (npcServer != 0)
	{
		CString npcServerIp = server->getAdminSettings()->getStr("ns_ip", "AUTO");
		if (npcServerIp == "AUTO")
			npcServerIp = npcServer->getSocket()->getRemoteIp();
		sendPacket(CString() >> (char)PLO_NPCSERVERADDR >> (short)npcServer->getId() << npcServerIp << "," << CString(server->getNCPort()));
	}
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
	packet >> (char)NCO_GMAPLIST;
	for (std::vector<CString>::iterator i = gmaps.begin(); i != gmaps.end(); ++i)
		packet >> (short)(*i).length() << (*i);

	// Send it.
	sendPacket(packet);
}


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
				unsigned short finish_pos[] = {start_pos[0] + delta[0], start_pos[1] + delta[1]};

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
					server->sendPacketToLevel(CString() >> (char)PLO_MOVE2 << (pPacket.text() + 2), npc->getLevel()->getMap(), npc->getLevel(), false, true);
			}
		}
	}

	return true;
}
