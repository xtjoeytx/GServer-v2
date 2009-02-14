#include <vector>
#include "ICommon.h"
#include "main.h"
#include "TPlayer.h"
#include "TAccount.h"
#include "TLevel.h"

#define serverlog	server->getServerLog()
#define rclog		server->getRCLog()
extern bool __playerPropsRC[propscount];

const char* __admin[] = {
	"name", "description", "url", "maxplayers", "onlystaff", "underconstruction", "nofoldersconfig",
	"sharefolder", "language", "serverip", "serverport", "listip", "listport",
};


void TPlayer::setPropsRC(CString& pPacket, TPlayer* rc)
{
	bool hadBomb = false;
	CString outPacket;

	// Skip playerworld
	pPacket.readChars(pPacket.readGUChar());

	// Read props from the packet.
	CString props = pPacket.readChars(pPacket.readGUChar());

	// Send props out.
	setProps(props, true, true, rc);

	// Clear flags and weapons.
	for (std::vector<CString>::iterator i = flagList.begin(); i != flagList.end(); ++i)
		outPacket >> (char)PLO_FLAGDEL << *i << "\n";
	for (std::vector<CString>::iterator i = weaponList.begin(); i != weaponList.end(); ++i)
	{
		outPacket >> (char)PLO_NPCWEAPONDEL << *i << "\n";

		// Attempt to fix the funky client bomb capitalization issue.
		// Also fix the bomb coming back when you set the player props through RC.
		if ((*i) == "bomb")
		{
			outPacket >> (char)PLO_NPCWEAPONDEL << "Bomb\n";
			hadBomb = true;
		}
		if ((*i) == "Bomb")
			hadBomb = true;
	}
	sendPacket(outPacket);

	// If we never had the bomb, don't let it come back.
	if (hadBomb == false) allowBomb = false;

	// Clear the flags and re-populate the flag list.
	flagList.clear();
	for (int i = pPacket.readGUShort(); i > 0; --i)
	{
		unsigned char len = pPacket.readGUChar();
		if (len != 0)
			flagList.push_back(pPacket.readChars(len));
	}
	for (std::vector<CString>::iterator i = flagList.begin(); i != flagList.end(); ++i)
		sendPacket(CString() >> (char)PLO_FLAGSET << *i);

	// Clear the chests and re-populate the chest list.
	chestList.clear();
	for (int i = pPacket.readGUShort(); i > 0; --i)
	{
		unsigned char len = pPacket.readGUChar();
		char loc[2] = {pPacket.readGChar(), pPacket.readGChar()};
		chestList.push_back(CString() << CString((int)loc[0]) << ":" << CString((int)loc[1]) << ":" << pPacket.readChars(len - 2));
	}

	// Clear the weapons and re-populate the weapons list.
	weaponList.clear();
	for (int i = pPacket.readGUChar(); i > 0; --i)
	{
		unsigned char len = pPacket.readGUChar();
		if (len == 0) continue;
		CString wpn = pPacket.readChars(len);

		// Allow the bomb through if we are actually adding it.
		if (wpn == "bomb" || wpn == "Bomb")
		{
			hadBomb = true;
			allowBomb = true;
		}

		// Send the weapon to the player.
		TWeapon* weapon = server->getWeapon(wpn);
		if (weapon)
		{
			weaponList.push_back(weapon->getName());
			sendPacket(CString() << weapon->getWeaponPacket());
		}
	}

	// KILL THE BOMB DEAD
	if (hadBomb == false)
		sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << "Bomb");

	// Warp the player to his new location now.
	warp(levelName, x, y, 0);
}

CString TPlayer::getPropsRC()
{
	CString ret, props;
	ret >> (char)accountName.length() << accountName;
	ret >> (char)4 << "main";		// worldName

	// Add the props.
	for (int i = 0; i < propscount; ++i)
	{
		if (__playerPropsRC[i])
			props >> (char)i << getProp(i);
	}
	ret >> (char)props.length() << props;

	// Add the player's flags.
	ret >> (short)flagList.size();
	for (std::vector<CString>::iterator i = flagList.begin(); i != flagList.end(); ++i)
		ret >> (char)(*i).length() << (*i);

	// Add the player's chests.
	ret >> (short)chestList.size();
	for (std::vector<CString>::iterator i = chestList.begin(); i != chestList.end(); ++i)
	{
		std::vector<CString> chest = (*i).tokenize(":");
		if (chest.size() == 3)
		{
			CString chestData;
			chestData >> (char)atoi(chest[0].text()) >> (char)atoi(chest[1].text()) << chest[2];
			ret >> (char)chestData.length() << chestData;
		}
	}

	// Add the player's weapons.
	ret >> (char)weaponList.size();
	for (std::vector<CString>::iterator i = weaponList.begin(); i != weaponList.end(); ++i)
		ret >> (char)(*i).length() << (*i);

	return ret;
}

bool TPlayer::msgPLI_RC_SERVEROPTIONSGET(CString& pPacket)
{
	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to view the server options.", accountName.text());
		return true;
	}

	CString serverOptions;
	serverOptions.load(CString() << server->getServerPath() << "config/serveroptions.txt");
	serverOptions.removeAllI("\r");

	sendPacket(CString() >> (char)PLO_RC_SERVEROPTIONSGET << serverOptions.gtokenize());
	return true;
}

bool TPlayer::msgPLI_RC_SERVEROPTIONSSET(CString& pPacket)
{
	if (!isRC() || !hasRight(PLPERM_SETSERVEROPTIONS))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to set the server options.", accountName.text());
		else rclog.out("%s attempted to set the server options.", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " is not authorized to change the server options.");
		return true;
	}

	CSettings* settings = server->getSettings();
	CString options = pPacket.readString("");
	options.guntokenizeI();

	// If they don't have the modify staff account right, prevent them from changing admin-only options.
	if (!hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		std::vector<CString> oldOptions = CString::loadToken(CString() << server->getServerPath() << "config/serveroptions.txt", "\n", true);
		std::vector<CString> newOptions = options.tokenize("\n");
		options.clear();
		for (std::vector<CString>::iterator i = newOptions.begin(); i != newOptions.end(); ++i)
		{
			CString name = (*i).subString(0, (*i).find("="));
			name.trimI();

			// See if this command is an admin command.
			bool isAdmin = false;
			for (unsigned int j = 0; j < sizeof(__admin) / sizeof(char*); ++j)
				if (name == CString(__admin[j])) isAdmin = true;

			// If it is an admin command, replace it with the current value.
			if (isAdmin)
				(*i) = settings->getStr(name);

			// Add this line back into options.
			options << *i << "\n";
		}
	}

	// Save settings.
	options.replaceAllI("\n", "\r\n");
	options.save(CString() << server->getServerPath() << "config/serveroptions.txt");

	// Reload settings.
	settings->clear();
	settings->setSeparator("=");
	settings->loadFile(CString() << server->getServerPath() << "config/serveroptions.txt");
	if (!settings->isOpened())
		serverlog.out("** [Error] Could not open config/serveroptions.txt\n");

	rclog.out("%s has updated the server options.\n", accountName.text());
	server->sendPacketTo(CLIENTTYPE_RC, CString() >> (char)PLO_RC_CHAT << accountName << " has updated the server options.");

	return true;
}

bool TPlayer::msgPLI_RC_FOLDERCONFIGGET(CString& pPacket)
{
	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to get the folder config.", accountName.text());
		return true;
	}

	CString foldersConfig;
	foldersConfig.load(CString() << server->getServerPath() << "config/foldersconfig.txt");
	foldersConfig.removeAllI("\r");

	sendPacket(CString() >> (char)PLO_RC_FOLDERCONFIGGET << foldersConfig.gtokenize());
	return true;
}

bool TPlayer::msgPLI_RC_FOLDERCONFIGSET(CString& pPacket)
{
	if (!isRC() || !hasRight(PLPERM_SETFOLDEROPTIONS))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to set the folder config.", accountName.text());
		else rclog.out("%s attempted to set the folder config.", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " is not authorized to change the folder config.");
		return true;
	}

	// Save the folder config back to disk
	CString folders = pPacket.readString("");
	folders.guntokenizeI();
	folders.replaceAllI("\n", "\r\n");
	folders.save(CString() << server->getServerPath() << "config/foldersconfig.txt");

	// Update file system.
	if (server->getSettings()->getBool("nofoldersconfig", false) == true)
		server->loadAllFolders();
	else
		server->loadFolderConfig();

	rclog.out("%s updated the folder config.\n", accountName.text());
	server->sendPacketTo(CLIENTTYPE_RC, CString() >> (char)PLO_RC_CHAT << accountName << " updated the folder config.");
	return true;
}

bool TPlayer::msgPLI_RC_RESPAWNSET(CString& pPacket)
{
	// Deprecated
	return true;
}

bool TPlayer::msgPLI_RC_HORSELIFESET(CString& pPacket)
{
	// Deprecated
	return true;
}

bool TPlayer::msgPLI_RC_APINCREMENTSET(CString& pPacket)
{
	// Deprecated
	return true;
}

bool TPlayer::msgPLI_RC_BADDYRESPAWNSET(CString& pPacket)
{
	// Deprecated
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERPROPSGET(CString& pPacket)
{
	// Deprecated
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERPROPSSET(CString& pPacket)
{
	// Deprecated?

	TPlayer* p = server->getPlayer(pPacket.readGUShort());
	if (p == 0) return true;

	if (!isRC() || (p->getAccountName() != accountName && !hasRight(PLPERM_SETATTRIBUTES)) || (p->getAccountName() == accountName && !hasRight(PLPERM_SETSELFATTRIBUTES)))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to set a player's properties.", accountName.text());
		else rclog.out("%s attempted to set a player's properties.", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " is not authorized to set the properties of " << p->getAccountName());
		return true;
	}

	p->setPropsRC(pPacket, this);
	p->saveAccount();
	rclog.out("%s set the attributes of player %s\n", accountName.text(), p->getAccountName().text());
	server->sendPacketTo(CLIENTTYPE_RC, CString() >> (char)PLO_RC_CHAT << accountName << " set the attributes of player " << p->getAccountName());

	return true;
}

bool TPlayer::msgPLI_RC_DISCONNECTPLAYER(CString& pPacket)
{
	TPlayer* p = server->getPlayer(pPacket.readGUShort());
	if (p == 0) return true;

	if (!isRC() || !hasRight(PLPERM_DISCONNECT))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to disconnect %s.\n", accountName.text(), p->getAccountName().text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " is not authorized to disconnect players.");
		return true;
	}

	rclog.out("%s disconnected %s.\n", accountName.text(), p->getAccountName().text());
	server->sendPacketTo(CLIENTTYPE_RC, CString() >> (char)PLO_RC_CHAT << accountName << " disconnected " << p->getAccountName());

	p->sendPacket(CString() >> (char)PLO_DISCMESSAGE << "One of the server administrators, " << accountName << ", has disconnected you.");
	server->deletePlayer(p);
	return true;
}

bool TPlayer::msgPLI_RC_UPDATELEVELS(CString& pPacket)
{
	if (!isRC() || !hasRight(PLPERM_UPDATELEVEL))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to update levels.\n", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " is not authorized to update levels.");
		return true;
	}

	unsigned short levelCount = pPacket.readGUShort();
	for (int i = 0; i < levelCount; ++i)
	{
		TLevel* level = server->getLevel(pPacket.readChars(pPacket.readGUChar()));
		if (level) level->reload();
	}
	return true;
}

bool TPlayer::msgPLI_RC_ADMINMESSAGE(CString& pPacket)
{
	if (!isRC() || !hasRight(PLPERM_ADMINMSG))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to send an admin message.\n", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to send an admin message.");
		return true;
	}

	server->sendPacketToAll(CString() >> (char)PLO_RC_ADMINMESSAGE << "Admin " << accountName << ":\xa7" << pPacket.readString(""), this);
	return true;
}

bool TPlayer::msgPLI_RC_PRIVADMINMESSAGE(CString& pPacket)
{
	if (!isRC() || !hasRight(PLPERM_ADMINMSG))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to send an admin message.\n", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to send an admin message.");
		return true;
	}

	TPlayer* p = server->getPlayer(pPacket.readGUShort());
	if (p == 0) return true;

	p->sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE << "Admin " << accountName << ":\xa7" << pPacket.readString(""));
	return true;
}

bool TPlayer::msgPLI_RC_LISTRCS(CString& pPacket)
{
	// Deprecated
	return true;
}

bool TPlayer::msgPLI_RC_DISCONNECTRC(CString& pPacket)
{
	// Deprecated
	return true;
}

bool TPlayer::msgPLI_RC_APPLYREASON(CString& pPacket)
{
	// Deprecated
	return true;
}

bool TPlayer::msgPLI_RC_SERVERFLAGSGET(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_SERVERFLAGSSET(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_ACCOUNTADD(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_ACCOUNTDEL(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_ACCOUNTLISTGET(CString& pPacket)
{
	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to view the account listing.\n", accountName.text());
		return true;
	}

	CString name = pPacket.readChars(pPacket.readGUChar());
	CString conditions = pPacket.readChars(pPacket.readGUChar());

	// Fix up name searching.
	name.replaceAllI("%", "*");
	if (name.length() == 0)
		name = "*";

	// Start our packet.
	CString ret;
	ret >> (char)PLO_RC_ACCOUNTLISTGET;

	// Search through all the accounts.
	CFileSystem* fs = server->getAccountsFileSystem();
	for (std::map<CString, CString>::iterator i = fs->getFileList()->begin(); i != fs->getFileList()->end(); ++i)
	{
		CString acc = removeExtension(i->first);
		if (acc.isEmpty()) continue;
		if (!acc.match(name)) continue;
		if (conditions.length() != 0)
		{
			if (TAccount::meetsConditions(i->second, conditions))
				ret >> (char)acc.length() << acc;
		}
		else ret >> (char)acc.length() << acc;
	}

	sendPacket(ret);
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERPROPSGET2(CString& pPacket)
{
	TPlayer* p = server->getPlayer(pPacket.readGUShort());
	if (p == 0) return true;

	if (!isRC() || !hasRight(PLPERM_VIEWATTRIBUTES))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to view the props of player %s.\n", accountName.text(), p->getAccountName().text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to view player props.");
		return true;
	}

	sendPacket(CString() >> (char)PLO_RC_PLAYERPROPSGET >> (short)p->getId() << p->getPropsRC());
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERPROPSGET3(CString& pPacket)
{
	bool offline = false;
	CString acc = pPacket.readChars(pPacket.readGUChar());
	TPlayer* p = server->getPlayer(acc);
	if (p == 0)
	{
		offline = true;
		p = new TPlayer(server, 0, -1);
		p->loadAccount(acc);
	}

	if (!isRC() || !hasRight(PLPERM_VIEWATTRIBUTES))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to view the props of player %s.\n", accountName.text(), p->getAccountName().text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to view player props.");
		if (offline) delete p;
		return true;
	}

	sendPacket(CString() >> (char)PLO_RC_PLAYERPROPSGET >> (short)p->getId() << p->getPropsRC());
	if (offline) delete p;
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERPROPSRESET(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERPROPSSET2(CString& pPacket)
{
	bool offline = false;
	CString acc = pPacket.readChars(pPacket.readGUChar());
	TPlayer* p = server->getPlayer(acc);
	if (p == 0)
	{
		offline = true;
		p = new TPlayer(server, 0, -1);
		p->loadAccount(acc);
	}

	if (!isRC() || (p->getAccountName() != accountName && !hasRight(PLPERM_SETATTRIBUTES)) || (p->getAccountName() == accountName && !hasRight(PLPERM_SETSELFATTRIBUTES)))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to set a player's properties.", accountName.text());
		else rclog.out("%s attempted to set a player's properties.", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " is not authorized to set the properties of " << p->getAccountName());
		if (offline) delete p;
		return true;
	}

	// Only people with PLPERM_MODIFYSTAFFACCOUNT can alter the default account.
	if (!hasRight(PLPERM_MODIFYSTAFFACCOUNT) && acc == "defaultaccount")
	{
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to modify the default account.");
		if (offline) delete p;
		return true;
	}

	p->setPropsRC(pPacket, this);
	p->saveAccount();
	rclog.out("%s set the attributes of player %s\n", accountName.text(), p->getAccountName().text());
	server->sendPacketTo(CLIENTTYPE_RC, CString() >> (char)PLO_RC_CHAT << accountName << " set the attributes of player " << p->getAccountName());
	if (offline) delete p;

	return true;
}

bool TPlayer::msgPLI_RC_ACCOUNTGET(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_ACCOUNTSET(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_CHAT(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_WARPPLAYER(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERRIGHTSGET(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERRIGHTSSET(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERCOMMENTSGET(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERCOMMENTSSET(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERBANGET(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERBANSET(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_START(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_CD(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_END(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_DOWN(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_UP(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_MOVE(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_DELETE(CString& pPacket)
{
	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_RENAME(CString& pPacket)
{
	return true;
}
