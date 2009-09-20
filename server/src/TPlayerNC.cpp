#include "IDebug.h"
#include <vector>
#include <math.h>
#include "ICommon.h"
#include "TPlayer.h"
#include "TAccount.h"
#include "CSocket.h"
#include "TServerList.h"

#define serverlog	server->getServerLog()
#define npclog		server->getNPCLog()
#define rclog		server->getRCLog()

typedef bool (TPlayer::*TPLSock)(CString&);
extern std::vector<TPLSock> TPLFunc;		// From TPlayer.cpp

/*
	NPC-Server Requests
*/
enum
{
	NCI_NPCLOG				= 0,
	NCI_GETWEAPONS			= 1,
	NCI_GETLEVELS			= 2,
	NCI_SENDPM				= 3,
	NCI_SENDTORC			= 4,
	NCI_WEAPONADD			= 5,
	NCI_WEAPONDEL			= 6,
	NCI_PLAYERPROPSSET		= 7,
	NCI_PLAYERWEAPONSGET	= 8,
	NCI_PLAYERPACKET		= 9,
	NCI_PLAYERWEAPONADD		= 10,
	NCI_PLAYERWEAPONDEL		= 11,
	NCI_LEVELGET			= 12,
};

enum
{
	NCO_PLAYERWEAPONS		= 0,
	NCO_PLAYERWEAPONADD		= 1,
	NCO_PLAYERWEAPONDEL		= 2,
};

/*
	NPC-Server Functionality
*/

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
		CString npcServerIp = server->getAdminSettings()->getStr("ns_ip");
		if (npcServerIp == "AUTO")
			npcServerIp = npcServer->getSocket()->getRemoteIp();
		sendPacket(CString() >> (char)PLO_NPCSERVERADDR >> (short)npcServer->getId() << npcServerIp << "," << CString(server->getNCPort()));
	}
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
			server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << nickName << ": " << pPacket.readString(""));
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
		}

		case NCI_PLAYERPACKET:
		{
			unsigned short pid = pPacket.readGUShort();
			TPlayer* pl = server->getPlayer(pid);
			if (pl != 0)
			{
				CString packet = pPacket.readString("");
				unsigned char id = packet.readGUChar();

				// Check if it is a valid packet id.
				if (id >= (unsigned char)TPLFunc.size())
					return true;

				// Call the function assigned to the packet id.
				if (!(*pl.*TPLFunc[id])(packet))
					server->deletePlayer(pl);
			}
		}

		case NCI_PLAYERWEAPONADD:
		{
			unsigned short pid = pPacket.readGUShort();
			TPlayer* pl = server->getPlayer(pid);
			if (pl != 0)
				pl->addWeapon(pPacket.readString(""));
		}

		case NCI_PLAYERWEAPONDEL:
		{
			unsigned short pid = pPacket.readGUShort();
			TPlayer* pl = server->getPlayer(pid);
			if (pl != 0)
				pl->deleteWeapon(pPacket.readString(""));
		}

		case NCI_LEVELGET:
		{
			server->NC_SendLevel(TLevel::findLevel(pPacket.readString(""), server));
		}
	}

	return true;
}
