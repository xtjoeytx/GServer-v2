#include <vector>
#include <map>
#include <sys/stat.h>
#include <stdio.h>
#include "ICommon.h"
#include "main.h"
#include "TPlayer.h"
#include "TAccount.h"
#include "TLevel.h"

#define serverlog	server->getServerLog()
#define rclog		server->getRCLog()

// {115}
bool TPlayer::msgPLI_NC_WEAPONLISTGET(CString& pPacket)
{
	if (!isNC())
	{
		rclog.out("[Hack] %s attempted to get the weapon list through NC.\n", accountName.text());
		return true;
	}

	CFileSystem fs(server);
	fs.addDir("weapons", "*");

	// Assemble a list of all the weapons.
	CString retVal;
	for (std::map<CString, CString>::iterator i = fs.getFileList()->begin(); i != fs.getFileList()->end(); ++i)
	{
		CString weapon;
		weapon.load(i->second);
		weapon.setRead(weapon.find("NEWWEAPON ") + 10);
		CString weaponName = weapon.readString(",");

		retVal >> (char)weaponName.length() << weaponName;
	}

	sendPacket(CString() >> (char)PLO_NC_WEAPONLISTGET << retVal);

	return true;
}

// {116}{weapon}
bool TPlayer::msgPLI_NC_WEAPONGET(CString& pPacket)
{
	if (!isNC())
	{
		rclog.out("[Hack] %s attempted to get a weapon through NC.\n", accountName.text());
		return true;
	}

	CString weapon = pPacket.readString("");
	TWeapon* weap = TWeapon::loadWeapon(weapon, server);

	sendPacket(CString() >> (char)PLO_NC_WEAPONGET >> (char)weap->getName().length() << weap->getName()
		>> (char)weap->getImage().length() << weap->getImage() << weap->getServerScript() << weap->getClientScript());

	delete weap;

	return true;
}

// {117}{CHAR weapon length}{weapon}{CHAR image length}{image}{code}
bool TPlayer::msgPLI_NC_WEAPONADD(CString& pPacket)
{
	if (!isNC())
	{
		rclog.out("[Hack] %s attempted to add a weapon through NC.\n", accountName.text());
		return true;
	}

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

	return true;
}

// {118}{weapon}
bool TPlayer::msgPLI_NC_WEAPONDELETE(CString& pPacket)
{
	if (!isNC())
	{
		rclog.out("[Hack] %s attempted to delete a weapon through NC.\n", accountName.text());
		return true;
	}

	CString weapon = pPacket.readString("");
	CString weapontxt(weapon);
	weapontxt << ".txt";
	weapontxt.replaceAllI("*", "@");
	weapontxt.replaceAllI("/", "_");

	// Attempt to remove the weapon.
	CString weaponpath = CString() << server->getServerPath() << "weapons/" << weapontxt;
	CFileSystem::fixPathSeparators(&weaponpath);
	if (remove(weaponpath.text()) == 0)
	{
		// Delete the weapon for all players.
		std::vector<TPlayer*>* playerList = server->getPlayerList();
		for (std::vector<TPlayer*>::iterator i = playerList->begin(); i != playerList->end(); ++i)
		{
			TPlayer* player = *i;
			if (!player->isClient()) continue;

			if (player->hasWeapon(weapon))
				player->sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << weapon);
		}

		// If we are looking at the File Browser, reload it.
		if (isFtp) msgPLI_RC_FILEBROWSER_START(CString() << "");

		rclog.out("%s deleted weapon %s\n", accountName.text(), weapon.text());
	}
	return true;
}
