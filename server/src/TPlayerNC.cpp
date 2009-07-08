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

/*
	NPC-Server Requests
*/
enum
{
	NCREQ_NPCLOG	= 0,
	NCREQ_WEAPONS	= 1,
	NCREQ_LEVELS	= 2,
	NCREQ_SENDPM	= 3,
	NCREQ_SENDRC	= 4,
	NCREQ_WEPADD	= 5,
	NCREQ_WEPDEL	= 6,
	NCREQ_SETPROPS	= 7,
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
		case NCREQ_NPCLOG:
			npclog.out(pPacket.readString(""));
			break;

		// Send Weapons to NPC-Server
		case NCREQ_WEAPONS:
			sendNC_Weapons();
			break;

		// Send Levels to NPC-Server
		case NCREQ_LEVELS:
			break;

		// Send PM
		case NCREQ_SENDPM:
		{
			TPlayer *player = server->getPlayer(pPacket.readGUShort());
			if (player != 0)
				player->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)id << pPacket.readString(""));
			break;
		}

		// Send RC
		case NCREQ_SENDRC:
			server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << nickName << ": " << pPacket.readString(""));
			break;

		// Weapon Add/Update
		case NCREQ_WEPADD:
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
		case NCREQ_WEPDEL:
			server->NC_DelWeapon(pPacket.readString(""));
			break;

		// Player Props
		case NCREQ_SETPROPS:
		{
			TPlayer *pl = server->getPlayer(pPacket.readGShort());
			if (pl != 0)
				pl->setProps(pPacket, true, true, this);
			break;
		}
	}

	return true;
}