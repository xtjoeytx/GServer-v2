#include <vector>
#include <math.h>
#include "ICommon.h"
#include "TPlayer.h"
#include "TAccount.h"
#include "CSocket.h"
#include "TServerList.h"

#define serverlog	server->getServerLog()
#define rclog		server->getRCLog()

/*
	NPC-Server Requests
*/
enum
{
	NCREQ_WEAPONS = 0,
	NCREQ_LEVELS  = 1,
	NCREQ_SENDPM  = 2,
	NCREQ_SENDRC  = 3,
	NCREQ_WEPADD  = 4,
};

/*
	NPC-Server Functionality
*/

// Send Weapons
void TPlayer::sendNC_Weapons()
{
	std::vector<TWeapon *> *weaponList = server->getWeaponList();
	for (std::vector<TWeapon *>::const_iterator i = weaponList->begin(); i != weaponList->end(); ++i)
	{
		if ((*i)->isDefault())
			continue;

		CString weaponName = (*i)->getName();
		CString imageName  = (*i)->getImage();
		CString scriptData = (*i)->getFullScript();
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
			/*
			CString name = pPacket.readChars(pPacket.readGUChar());
			CString image = pPacket.readChars(pPacket.readGUChar());
			CString script = pPacket.readString("");

			// See if the weapon already exists.
			std::vector<TWeapon*>* weaponList = server->getWeaponList();
			for (std::vector<TWeapon*>::iterator i = weaponList->begin(); i != weaponList->end(); ++i)
			{
				// We found the weapon.  Update it.
				TWeapon* weapon = *i;
				if (weapon->isDefault()) continue;
				if (weapon->getName() == name)
				{
					// Separate clientside and serverside script.
					if (script.find("//#CLIENTSIDE") != -1)
					{
						CString serverScript = script.readString("//#CLIENTSIDE");
						CString clientScript = script.readString("");
						weapon->setServerScript(serverScript);
						weapon->setClientScript(clientScript);
					}
					else weapon->setClientScript(CString() << "//#CLIENTSIDE\xa7" << script);

					// Save our weapon.
					weapon->setFullScript(script);
					weapon->setImage(image);
					weapon->saveWeapon(server);

					// See if we need to update the weapon for any players.
					std::vector<TPlayer*>* playerList = server->getPlayerList();
					for (std::vector<TPlayer*>::iterator j = playerList->begin(); j != playerList->end(); ++j)
					{
						TPlayer* player = *j;
						if (!player->isClient()) continue;

						// If the player has the weapon, send them the new version.
						if (player->hasWeapon(weapon->getName()))
						{
							player->sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << weapon->getName());
							player->sendPacket(CString() << weapon->getWeaponPacket());
						}
					}
					rclog.out("%s updated weapon %s\n", accountName.text(), name.text());
					return true;
				}
			}

			// The weapon wasn't found.  Add a new weapon.
			TWeapon* weapon = new TWeapon(name, image, script, 0, server->getSettings()->getBool("trimnpccode", false));
			weapon->saveWeapon(server);
			weaponList->push_back(weapon);
			rclog.out("%s added weapon %s\n", accountName.text(), name.text());
			*/
			break;
		}
	}

	return true;
}