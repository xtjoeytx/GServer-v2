#include <vector>
#include <map>
#include <sys/stat.h>
#if defined(_WIN32) || defined(_WIN64)
	#include <direct.h>
	#define rmdir _rmdir
#else
	#include <unistd.h>
#endif
#include <stdio.h>
#include "ICommon.h"
#include "main.h"
#include "TPlayer.h"
#include "TAccount.h"
#include "TLevel.h"

#define serverlog	server->getServerLog()
#define rclog		server->getRCLog()
extern bool __playerPropsRC[propscount];

const char* __admin[] = {
	"name", "description", "url", "maxplayers", "onlystaff", "nofoldersconfig",
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
	setProps(props, (id != -1 ? true : false), (id != -1 ? true : false), rc);

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
	if (id != -1) sendPacket(outPacket);

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
	if (id != -1)
	{
		for (std::vector<CString>::iterator i = flagList.begin(); i != flagList.end(); ++i)
			sendPacket(CString() >> (char)PLO_FLAGSET << *i);
	}

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
			if (id != -1) sendPacket(CString() << weapon->getWeaponPacket());
		}
	}

	// KILL THE BOMB DEAD
	if (id != -1)
	{
		if (hadBomb == false)
			sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << "Bomb");
	}

	// Warp the player to his new location now.
	if (id != -1) warp(levelName, x, y, 0);
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
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " has updated the server options.");

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
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " updated the folder config.");
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
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " set the attributes of player " << p->getAccountName());

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
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " disconnected " << p->getAccountName());

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
	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to view the server flags.\n", accountName.text());
		return true;
	}
	CString ret;
	ret >> (char)PLO_RC_SERVERFLAGSGET >> (short)server->getServerFlags()->size();
	for (std::vector<CString>::iterator i = server->getServerFlags()->begin(); i != server->getServerFlags()->end(); ++i)
		ret >> (char)(*i).length() << (*i);
	sendPacket(ret);
	return true;
}

bool TPlayer::msgPLI_RC_SERVERFLAGSSET(CString& pPacket)
{
	if (!isRC() || !hasRight(PLPERM_SETSERVERFLAGS))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to set the server flags.\n");
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set the server flags.");
		return true;
	}

	unsigned short count = pPacket.readGUShort();
	std::vector<CString>* serverFlags = server->getServerFlags();

	// Save server flags.
	std::vector<CString> oldFlags = *serverFlags;

	// Delete server flags.
	serverFlags->clear();

	// Assemble the new server flags.
	for (unsigned int i = 0; i < count; ++i)
	{
		CString flag = pPacket.readChars(pPacket.readGUChar());
		serverFlags->push_back(flag);
	}

	// Send flag changes to all players.
	for (std::vector<CString>::iterator i = serverFlags->begin(); i != serverFlags->end(); ++i)
	{
		bool found = false;
		for (std::vector<CString>::iterator j = oldFlags.begin(); j != oldFlags.end();)
		{
			if (*i == *j)
			{
				found = true;
				j = oldFlags.erase(j);
				break;
			}
			else ++j;
		}

		// If we didn't find a match, this is either a new flag, or a changed flag.
		if (!found)
		{
			server->sendPacketTo(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGSET << *i);
		}
	}

	// If any flags were deleted, tell that to the players now.
	for (std::vector<CString>::iterator i = oldFlags.begin(); i != oldFlags.end(); ++i)
	{
		server->sendPacketTo(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGDEL << (*i).readString("=").trim());
	}

	rclog.out("%s has updated the server flags.\n", accountName.text());
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " has updated the server flags.");
	return true;
}

bool TPlayer::msgPLI_RC_ACCOUNTADD(CString& pPacket)
{
	if (!isRC() || !hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to add a new account.\n", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to create new accounts.");
		return true;
	}

	CString acc = pPacket.readChars(pPacket.readGUChar());
	CString pass = pPacket.readChars(pPacket.readGUChar());
	CString email = pPacket.readChars(pPacket.readGUChar());
	bool banned = (pPacket.readGUChar() != 0);
	bool onlyLoad = (pPacket.readGUChar() != 0);
	pPacket.readGUChar();		// Admin level, deprecated.

	TAccount newAccount(server);
	newAccount.loadAccount(acc);
	newAccount.setBanned(banned);
	newAccount.setLoadOnly(onlyLoad);
	newAccount.setEmail(email);
	newAccount.saveAccount();

	rclog.out("%s has created a new account: %s\n", accountName.text(), acc.text());
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " has created a new account: " << acc);
	return true;
}

bool TPlayer::msgPLI_RC_ACCOUNTDEL(CString& pPacket)
{
	if (!isRC() || !hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to delete an account.\n", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to delete accounts.");
		return true;
	}

	// Get the account.
	// Prevent the defaultaccount from being deleted.
	CString acc = pPacket.readString("");
	if (acc == "defaultaccount")
	{
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not allowed to delete the default account.");
		return true;
	}

	// See if the account exists.
	CString accfile = CString(acc) << ".txt";
	CString accpath = server->getAccountsFileSystem()->find(accfile);
	if (accpath.isEmpty()) return true;

	// Remove the account from the file system.
	server->getAccountsFileSystem()->removeFile(accfile);

	// Delete the file now.
	remove(accpath.text());
	rclog.out("%s has deleted the account: %s\n", accountName.text(), acc.text());
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " has deleted the account: " << acc);
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
	TPlayer* p = server->getPlayer(acc, false);
	if (p == 0)
	{
		offline = true;
		p = new TPlayer(server, 0, -1);
		if (!p->loadAccount(acc))
		{
			delete p;
			return true;
		}
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
	CString acc = pPacket.readString("");

	if (!isRC() || !hasRight(PLPERM_RESETATTRIBUTES))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to reset the account: %s\n", accountName.text(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to reset accounts.\n");
		return true;
	}

	// Get the player.  Create a new player if they are offline.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, false);
	if (p == 0)
	{
		offline = true;
		p = new TPlayer(server, 0, -1);
		if (!p->loadAccount(acc))
		{
			delete p;
			return true;
		}
	}

	// Reset the player.
	p->reset();

	// If the player is online, boot him from the server.
	if (!offline)
	{
		p->sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Your account was reset by " << accountName);
		p->setLoaded(false);	// Don't save the account when the player quits.
		server->deletePlayer(p);
	}

	// Log it.
	rclog.out("%s has reset the attributes of account: %s\n", accountName.text(), acc.text());
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " has reset the attributes of account: " << acc);

	// Clean up.
	if (offline) delete p;

	return true;
}

bool TPlayer::msgPLI_RC_PLAYERPROPSSET2(CString& pPacket)
{
	bool offline = false;
	CString acc = pPacket.readChars(pPacket.readGUChar());
	TPlayer* p = server->getPlayer(acc, false);
	if (p == 0)
	{
		offline = true;
		p = new TPlayer(server, 0, -1);
		if (!p->loadAccount(acc))
		{
			delete p;
			return true;
		}
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
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " set the attributes of player " << p->getAccountName());
	if (offline) delete p;

	return true;
}

bool TPlayer::msgPLI_RC_ACCOUNTGET(CString& pPacket)
{
	CString acc = pPacket.readString("");

	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to view the account: %s\n", accountName.text(), acc.text());
		return true;
	}

	// Get the player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, false);
	if (p == 0)
	{
		offline = true;
		p = new TPlayer(server, 0, -1);
		if (!p->loadAccount(acc))
		{
			delete p;
			return true;
		}
	}

	sendPacket(CString() >> (char)PLO_RC_ACCOUNTGET >> (char)acc.length() << acc
		>> (char)0 /*>> (char)password_length << password*/
		>> (char)p->getEmail().length() << p->getEmail()
		>> (char)(p->getBanned() ? 1 : 0)
		>> (char)(p->getLoadOnly() ? 1 : 0)
		>> (char)0 /*admin level*/
		>> (char)4 << "main"
		>> (char)p->getBanLength().length() << p->getBanLength()
		>> (char)p->getBanReason().length() << p->getBanReason());

	if (offline) delete p;
	return true;
}

bool TPlayer::msgPLI_RC_ACCOUNTSET(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());

	if (!isRC() || !hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to edit the account: %s\n", accountName.text(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to edit accounts.\n");
		return true;
	}

	CString pass = pPacket.readChars(pPacket.readGUChar());
	CString email = pPacket.readChars(pPacket.readGUChar());
	bool banned = (pPacket.readGUChar() != 0 ? true : false);
	bool loadOnly = (pPacket.readGUChar() != 0 ? true : false);
	pPacket.readGUChar();						// admin level
	pPacket.readChars(pPacket.readGUChar());	// world
	CString banreason = pPacket.readChars(pPacket.readGUChar());

	// Get player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, false);
	if (p == 0)
	{
		offline = true;
		p = new TPlayer(server, 0, -1);
		if (!p->loadAccount(acc))
		{
			delete p;
			return true;
		}
	}

	// Set the new account stuff.
	p->setEmail(email);
	p->setLoadOnly(loadOnly);
	if (hasRight(PLPERM_BAN))
	{
		p->setBanned(banned);
		p->setBanReason(banreason);
	}
	p->saveAccount();

	// If the account is currently on RC, reload it.
	TPlayer* pRC = server->getRC(acc);
	if (pRC)
	{
		pRC->loadAccount(acc);
	}

	// If the player was just now banned, kick him off the server.
	if (hasRight(PLPERM_BAN) && banned && !offline)
	{
		p->setLoaded(false);
		p->sendPacket(CString() >> (char)PLO_DISCMESSAGE << accountName << " has banned you.  Reason: " << banreason.guntokenize().replaceAll("\n", "\r"));
		server->deletePlayer(p);
	}

	rclog.out("%s has modified the account: %s\n", accountName.text(), acc.text());
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " has modified the account: " << acc);

	if (offline) delete p;
	return true;
}

bool TPlayer::msgPLI_RC_CHAT(CString& pPacket)
{
	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to send a message to RC.\n", accountName.text());
		return true;
	}

	CString message = pPacket.readString("");
	if (message.isEmpty()) return true;
	std::vector<CString> words = message.tokenize();

	if (words[0].text()[0] != '/')
	{
		server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << nickName << ": " << message);
		return true;
	}
	else
	{
		if (words[0] == "/help" && words.size() == 1)
		{
			std::vector<CString> commands = CString::loadToken(CString() << server->getServerPath() << "config/rchelp.txt", "\n", true);
			for (std::vector<CString>::iterator i = commands.begin(); i != commands.end(); ++i)
				sendPacket(CString() >> (char)PLO_RC_CHAT << (*i));
		}
		else if (words[0] == "/open" && words.size() != 1)
		{
			message.setRead(0);
			message.readString(" ");
			CString acc = message.readString("");
			return msgPLI_RC_PLAYERPROPSGET3(CString() >> (char)acc.length() << acc);
		}
		else if (words[0] == "/openacc" && words.size() != 1)
		{
			message.setRead(0);
			message.readString(" ");
			CString acc = message.readString("");
			return msgPLI_RC_ACCOUNTGET(CString() << acc);
		}
		else if (words[0] == "/opencomments" && words.size() != 1)
		{
			message.setRead(0);
			message.readString(" ");
			CString acc = message.readString("");
			return msgPLI_RC_PLAYERCOMMENTSGET(CString() << acc);
		}
		else if (words[0] == "/openban" && words.size() != 1)
		{
			message.setRead(0);
			message.readString(" ");
			CString acc = message.readString("");
			return msgPLI_RC_PLAYERBANGET(CString() << acc);
		}
		else if (words[0] == "/openrights" && words.size() != 1)
		{
			message.setRead(0);
			message.readString(" ");
			CString acc = message.readString("");
			return msgPLI_RC_PLAYERRIGHTSGET(CString() << acc);
		}
		else if (words[0] == "/reset" && words.size() != 1)
		{
			message.setRead(0);
			message.readString(" ");
			CString acc = message.readString("");
			return msgPLI_RC_PLAYERPROPSRESET(CString() << acc);
		}
		else if (words[0] == "/refreshservermessage" && words.size() == 1)
		{
			CString* servermessage = server->getServerMessage();
			servermessage->load(CString() << server->getServerPath() << "config/servermessage.html");
			servermessage->removeAllI("\r");
			servermessage->replaceAllI("\n", " ");
			sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: Refreshed server message.");
		}
		else if (words[0] == "/updatelevel" && words.size() != 1 && hasRight(PLPERM_UPDATELEVEL))
		{
			for (unsigned int i = 1; i < words.size(); ++i)
			{
				TLevel* level = server->getLevel(words[i]);
				if (level) level->reload();
			}
			sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: Updated level.");
		}
		else if (words[0] == "/updatelevelall" && words.size() == 1 && hasRight(PLPERM_UPDATELEVEL))
		{
			std::vector<TLevel*>* levels = server->getLevelList();
			for (std::vector<TLevel*>::iterator i = levels->begin(); i != levels->end(); ++i)
			{
				(*i)->reload();
			}
			sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: Updated all the levels.");
		}
		else if (words[0] == "/reloadserver" && words.size() == 1 && hasRight(PLPERM_MODIFYSTAFFACCOUNT))
		{
			server->loadConfigFiles();
			sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: Reloaded server configuration files.");
		}
		else if (words[0] == "/updateserverhq" && words.size() == 1 && hasRight(PLPERM_MODIFYSTAFFACCOUNT))
		{
			server->getServerList()->sendServerHQ();
			sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: Sent ServerHQ updates.");
		}
	}

	return true;
}

bool TPlayer::msgPLI_RC_WARPPLAYER(CString& pPacket)
{
	if (!isRC() || !hasRight(PLPERM_WARPTOPLAYER))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to warp a player.\n", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to warp players.\n");
		return true;
	}

	TPlayer* p = server->getPlayer(pPacket.readGUShort());
	if (p == 0) return true;

	float loc[2] = { (float)(pPacket.readGChar())/2.0f, (float)(pPacket.readGChar())/2.0f };
	CString wLevel = pPacket.readString("");
	p->warp(wLevel, loc[0], loc[1]);

	rclog.out("%s has warped %s to %s (%.2f, %.2f)\n", accountName.text(), p->getAccountName().text(), wLevel.text(), loc[0], loc[1]);
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERRIGHTSGET(CString& pPacket)
{
	CString acc = pPacket.readString("");

	if (!isRC() || (acc != accountName && !hasRight(PLPERM_SETRIGHTS)))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to get the rights of %s\n", accountName.text(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to view that player's rights.");
		return true;
	}

//	int rights = 0;
	CString folders, ip;

	// Get player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, false);
	if (p == 0)
	{
		offline = true;
		p = new TPlayer(server, 0, -1);
		if (!p->loadAccount(acc))
		{
			delete p;
			return true;
		}
	}

	// Get the folder list.
	for (std::vector<CString>::iterator i = p->getFolderList()->begin(); i != p->getFolderList()->end(); ++i)
		folders << *i << "\n";
	folders.gtokenizeI();

	// Send the packet.
	sendPacket(CString() >> (char)PLO_RC_PLAYERRIGHTSGET >> (char)acc.length() << acc
		>> (long long)p->getAdminRights()
		>> (char)p->getAdminIp().length() << p->getAdminIp()
		>> (short)folders.length() << folders);

	if (offline) delete p;
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERRIGHTSSET(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());

	if (!isRC() || !hasRight(PLPERM_SETRIGHTS))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to set the rights of %s\n", accountName.text(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set player rights.");
		return true;
	}

	// Get player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, false);
	if (p == 0)
	{
		offline = true;
		p = new TPlayer(server, 0, -1);
		if (!p->loadAccount(acc))
		{
			delete p;
			return true;
		}
	}

	// Don't allow RCs to give rights that they don't have.
	// Only affect people who don't have PLPERM_MODIFYSTAFFACCOUNT.
	int n_adminRights = (int)pPacket.readGUInt5();
	if (!hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		for (int i = 0; i < 20; ++i)
		{
			if ((adminRights & (1 << i)) == 0)
				n_adminRights &= ~(1 << i);
		}
	}

	// Don't allow you to remove your own PLPERM_MODIFYSTAFFACCOUNT or PLPERM_SETRIGHTS.
	if (p == this)
	{
		if ((n_adminRights & PLPERM_MODIFYSTAFFACCOUNT) == 0)
			n_adminRights |= PLPERM_MODIFYSTAFFACCOUNT;
		if ((n_adminRights & PLPERM_SETRIGHTS) == 0)
			n_adminRights |= PLPERM_SETRIGHTS;
	}

	p->setAdminRights(n_adminRights);
	p->setAdminIp(pPacket.readChars(pPacket.readGUChar()));

	// Untokenize and load the directories.
	CString folders = pPacket.readChars(pPacket.readGUShort());
	folders.guntokenizeI();
	std::vector<CString>* fList = p->getFolderList();
	fList->clear();
	*fList = folders.tokenize("\n");

	// Remove any invalid directories.
	for (std::vector<CString>::iterator i = fList->begin(); i != fList->end();)
	{
		if ((*i).find(":") != -1 || (*i).find("..") != -1 || (*i).find(" /*") != -1)
			i = fList->erase(i);
		else ++i;
	}

	// Save the account.
	p->saveAccount();

	// If the account is currently on RC, reload it.
	TPlayer* pRC = server->getRC(acc);
	if (pRC)
	{
		pRC->loadAccount(acc);

		// If they are using the File Browser, reload it.
		if (pRC->isUsingFileBrowser())
			pRC->msgPLI_RC_FILEBROWSER_START(CString() << "");
	}

	rclog.out("%s has set the rights of %s\n", accountName.text(), acc.text());
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " has set the rights of " << acc);

	if (offline) delete p;
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERCOMMENTSGET(CString& pPacket)
{
	CString acc = pPacket.readString("");

	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to get the comments of %s\n", accountName.text(), acc.text());
		return true;
	}

	// Get player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, false);
	if (p == 0)
	{
		offline = true;
		p = new TPlayer(server, 0, -1);
		if (!p->loadAccount(acc))
		{
			delete p;
			return true;
		}
	}

	sendPacket(CString() >> (char)PLO_RC_PLAYERCOMMENTSGET >> (char)acc.length() << acc << p->getComments());

	if (offline) delete p;
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERCOMMENTSSET(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());

	if (!isRC() || !hasRight(PLPERM_SETCOMMENTS))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to set the comments of %s\n", accountName.text(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set player comments.");
		return true;
	}

	// Get player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, false);
	if (p == 0)
	{
		offline = true;
		p = new TPlayer(server, 0, -1);
		if (!p->loadAccount(acc))
		{
			delete p;
			return true;
		}
	}

	CString comment = pPacket.readString("");
	p->setComments(comment);
	p->saveAccount();

	// If the account is currently on RC, reload it.
	TPlayer* pRC = server->getRC(acc);
	if (pRC)
	{
		pRC->loadAccount(acc);
	}

	rclog.out("%s has set the comments of %s\n", accountName.text(), acc.text());
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " has set the comments of " << acc);

	if (offline) delete p;
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERBANGET(CString& pPacket)
{
	CString acc = pPacket.readString("");

	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to view the ban of %s\n", accountName.text(), acc.text());
		return true;
	}

	// Get player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, false);
	if (p == 0)
	{
		offline = true;
		p = new TPlayer(server, 0, -1);
		if (!p->loadAccount(acc))
		{
			delete p;
			return true;
		}
	}

	sendPacket(CString() >> (char)PLO_RC_PLAYERBANGET >> (char)acc.length() << acc >> (char)(p->getBanned() ? 1 : 0) << p->getBanReason());

	if (offline) delete p;
	return true;
}

bool TPlayer::msgPLI_RC_PLAYERBANSET(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());

	if (!isRC() || !hasRight(PLPERM_BAN))
	{
		if (!isRC()) rclog.out("[Hack] %s attempted to set the ban of %s\n", accountName.text(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set player bans.");
		return true;
	}

	// Get player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, false);
	if (p == 0)
	{
		offline = true;
		p = new TPlayer(server, 0, -1);
		if (!p->loadAccount(acc))
		{
			delete p;
			return true;
		}
	}

	bool banned = (pPacket.readGUChar() == 0 ? false : true);
	CString reason = pPacket.readString("");

	p->setBanned(banned);
	p->setBanReason(reason);
	p->saveAccount();

	// If the account is currently on RC, reload it.
	TPlayer* pRC = server->getRC(acc);
	if (pRC)
	{
		pRC->loadAccount(acc);
	}

	// If the player was just now banned, kick him off the server.
	if (banned && !offline)
	{
		p->setLoaded(false);
		p->sendPacket(CString() >> (char)PLO_DISCMESSAGE << accountName << " has banned you.  Reason: " << reason.guntokenize().replaceAll("\n", "\r"));
		server->deletePlayer(p);
	}

	rclog.out("%s has set the ban of %s\n", accountName.text(), acc.text());
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " has set the ban of " << acc);

	if (offline) delete p;
	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_START(CString& pPacket)
{
	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to open the File Browser.\n", accountName.text());
		return true;
	}

	// If the player has no folder rights, don't open the File Browser.
	if (folderList.size() == 0)
		return true;

	// Get folder list.
	CString folders;
	for (std::vector<CString>::iterator i = folderList.begin(); i != folderList.end(); ++i)
		folders << *i << "\n";

	// Send the folder list and the welcome message.
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_DIRLIST << folders.gtokenize());
	if (!isFtp) sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Welcome to the File Browser.");

	// Grab the first rights and folder for use as our default view, just in case
	// the player doesn't have the ability to view their lastFolder anymore.
	CString viewFolder, viewRights, viewWild;
	CString rights = folderList[0].readString(" ");
	CString folder = folderList[0].readString("");
	folder.replaceAllI("\\", "/");
	folderList[0].setRead(0);

	// Create the defaults.
	viewRights = rights;
	int wcl = folder.findl('/');
	if (wcl == -1) viewWild = "*";
	else viewWild = folder.subString(wcl + 1);
	viewFolder = CString() << folder.remove(wcl) << "/";

	// Try to find our last folder.
	if (lastFolder.length() != 0)
	{
		for (std::vector<CString>::iterator i = folderList.begin(); i != folderList.end(); ++i)
		{
			CString rights = (*i).readString(" ");
			CString folder = (*i).readString("");
			folder.replaceAllI("\\", "/");
			CString wild;
			int wcl = folder.findl('/');
			if (wcl == -1) wild = "*";
			else wild = folder.subString(wcl + 1);
			folder.removeI(wcl);
			folder << "/";
			(*i).setRead(0);

			if (lastFolder == folder)
			{
				viewRights = rights;
				viewFolder = folder;
				viewWild = wild;
				break;
			}
		}
	}
	else lastFolder = viewFolder;

	// Construct our file list.
	CString files;
	CFileSystem fs(server);
	fs.addDir(viewFolder, viewWild);
	for (std::map<CString, CString>::iterator i = fs.getFileList()->begin(); i != fs.getFileList()->end(); ++i)
	{
		CString name = i->first;
		CString dir;
		int size = fs.getFileSize(i->first);
		time_t mod = fs.getModTime(i->first);
		dir >> (char)i->first.length() << i->first >> (char)viewRights.length() << viewRights >> (long long)size >> (long long)mod;
		files << " " >> (char)dir.length() << dir;
	}

	// Send packet.
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_DIR >> (char)viewFolder.length() << viewFolder << files);
	isFtp = true;

	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_CD(CString& pPacket)
{
	if (!isRC()) return true;

	CString newFolder = pPacket.readString("");
	CString newRights, wildcard;
	newFolder.setRead(0);

	bool found = false;
	for (std::vector<CString>::iterator i = folderList.begin(); i != folderList.end(); ++i)
	{
		CString rights = (*i).readString(" ");
		CString folder = (*i).readString("");
		folder.replaceAllI("\\", "/");
		CString wild;
		int wcl = folder.findl('/');
		if (wcl == -1) wild = "*";
		else wild = folder.subString(wcl + 1);
		folder.removeI(wcl);
		folder << "/";
		(*i).setRead(0);

		if (newFolder == folder)
		{
			newFolder = folder;
			newRights = rights;
			wildcard = wild;
			found = true;
			break;
		}
	}
	if (!found) return true;

	lastFolder = newFolder;

	// Construct our file list.
	CString files;
	CFileSystem fs(server);
	fs.addDir(lastFolder, wildcard);
	for (std::map<CString, CString>::iterator i = fs.getFileList()->begin(); i != fs.getFileList()->end(); ++i)
	{
		CString dir;
		int size = fs.getFileSize(i->first);
		time_t mod = fs.getModTime(i->first);
		dir >> (char)i->first.length() << i->first >> (char)newRights.length() << newRights >> (long long)size >> (long long)mod;
		files << " " >> (char)dir.length() << dir;
	}

	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Folder changed to " << lastFolder);
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_DIR >> (char)lastFolder.length() << lastFolder << files);

	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_END(CString& pPacket)
{
	if (!isRC()) return true;
	isFtp = false;

	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_DOWN(CString& pPacket)
{
	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to download a file through the File Browser.\n", accountName.text());
		return true;
	}

	// Load file.
	CString file = pPacket.readString("");
	CString filepath = CString() << server->getServerPath() << lastFolder << file;
	CString fileData;
	fileData.load(filepath);

	time_t modTime = 0;
	struct stat fileStat;
	if (stat(filepath.text(), &fileStat) != -1)
		modTime = fileStat.st_mtime;

	if (fileData.length() == 0) return true;

	// See if we have enough room in the packet for the file.
	// If not, we need to send it as a big file.
	// 1 (PLO_FILE) + 5 (modTime) + 1 (file.length()) + file.length() + 1 (\n)
	bool isBigFile = false;
	int packetLength = 1 + 5 + 1 + file.length() + 1;
	if (fileData.length() > 32000)
		isBigFile = true;

	// If we are sending a big file, let the client know now.
	if (isBigFile)
	{
		sendPacket(CString() >> (char)PLO_LARGEFILESTART << file);
		sendPacket(CString() >> (char)PLO_LARGEFILESIZE >> (long long)fileData.length());
	}

	// Send the file now.
	while (fileData.length() != 0)
	{
		int sendSize = clip(32000, 0, fileData.length());
		sendPacket(CString() >> (char)PLO_RAWDATA >> (int)(packetLength + sendSize));
		sendPacket(CString() >> (char)PLO_FILE >> (long long)modTime >> (char)file.length() << file << fileData.subString(0, sendSize) << "\n");
		fileData.removeI(0, sendSize);
	}

	// If we had sent a large file, let the client know we finished sending it.
	if (isBigFile) sendPacket(CString() >> (char)PLO_LARGEFILEEND << file);

	rclog.out("%s downloaded file %s\n", accountName.text(), file.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Downloaded file " << file);

	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_UP(CString& pPacket)
{
	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to upload a file through the File Browser.\n", accountName.text());
		return true;
	}

	CString file = pPacket.readChars(pPacket.readGUChar());
	CString filepath = CString() << server->getServerPath() << lastFolder << file;
	CString fileData = pPacket.subString(pPacket.readPos());

	// See if we are uploading a large file or not.
	if (rcLargeFiles.find(file) == rcLargeFiles.end())
	{
		// Normal file.  Save it and display our message.
		fileData.save(filepath);

		rclog.out("%s uploaded file %s\n", accountName.text(), file.text());
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Uploaded file " << file);
	}
	else
	{
		// Large file.  Store the data in memory.
		rcLargeFiles[file] << fileData;
	}

	// TODO: update server files.

	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_MOVE(CString& pPacket)
{
	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to move a file through the File Browser.\n", accountName.text());
		return true;
	}

	CString f1, f2, f3, f4;
	f1 = pPacket.readChars(pPacket.readGUChar());
	f2 = pPacket.readString("");
	f2.removeAllI("\"");

	f1 << f2;
	f4 << lastFolder << f2;

	rclog.out("%s moved file %s to %s\n", accountName.text(), f4.text(), f1.text());

	f4 = CString() << server->getServerPath() << f4;
	f3.load(f4);
	f3.save(f1);
	remove(f4.text());

	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_DELETE(CString& pPacket)
{
	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to delete a file through the File Browser.\n", accountName.text());
		return true;
	}

	CString file = pPacket.readString("");
	CString filePath = CString() << server->getServerPath() << lastFolder << file;
	CFileSystem::fixPathSeparators(&filePath);

	// Don't let us delete defaultaccount.
	if (file == "defaultaccount.txt")
	{
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Not allowed to delete defaultaccount.txt");
		return true;
	}

	// Remove the file.
	remove(filePath.text());

	rclog.out("%s deleted file %s\n", accountName.text(), file.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Deleted file " << file);

	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_RENAME(CString& pPacket)
{
	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to rename a file through the File Browser.\n", accountName.text());
		return true;
	}

	CString f1 = pPacket.readChars(pPacket.readGUChar());
	CString f2 = pPacket.readChars(pPacket.readGUChar());
	CString f1path = CString() << server->getServerPath() << lastFolder << f1;
	CString f2path = CString() << server->getServerPath() << lastFolder << f2;
	CFileSystem::fixPathSeparators(&f1path);
	CFileSystem::fixPathSeparators(&f2path);

	// Don't let us rename defaultaccount.
	if (f1 == "defaultaccount.txt" || f2 == "defaultaccount.txt")
	{
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Not allowed to delete defaultaccount.txt");
		return true;
	}

	// Do the renaming.
	rename(f1path.text(), f2path.text());
	rclog.out("%s renamed file %s to %s\n", accountName.text(), f1.text(), f2.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Renamed file " << f1 << " to " << f2);

	return true;
}

bool TPlayer::msgPLI_RC_LARGEFILESTART(CString& pPacket)
{
	if (!isRC())
	{
		rclog.out("[Hack] %s is attempting to upload a file through the File Browser.\n", accountName.text());
		return true;
	}

	CString file = pPacket.readString("");
	rcLargeFiles[file] = CString();

	return true;
}

bool TPlayer::msgPLI_RC_LARGEFILEEND(CString& pPacket)
{
	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to upload a file through the File Browser.\n", accountName.text());
		return true;
	}

	CString file = pPacket.readString("");
	CString filepath = CString() << server->getServerPath() << lastFolder << file;

	// Save the file.
	rcLargeFiles[file].save(filepath);

	// Remove the data from memory.
	for (std::map<CString, CString>::iterator i = rcLargeFiles.begin(); i != rcLargeFiles.end(); ++i)
	{
		if (i->first == file)
		{
			rcLargeFiles.erase(i);
			break;
		}
	}

	rclog.out("%s uploaded large file %s\n", accountName.text(), file.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Uploaded large file " << file);

	return true;
}

bool TPlayer::msgPLI_RC_FOLDERDELETE(CString& pPacket)
{
	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to delete a folder through the File Browser.\n", accountName.text());
		return true;
	}

	CString folder = pPacket.readString("");
	CString folderpath = CString() << server->getServerPath() << folder;
	CFileSystem::fixPathSeparators(&folderpath);
	folderpath.removeI(folderpath.length() -1);
	if (!isRC())
	{
		rclog.out("[Hack] %s attempted to delete a folder through the File Browser: %s\n", accountName.text(), folder.text());
		return true;
	}

	// Try to remove folder.
	if (rmdir(folderpath.text()))
	{
		perror("Error removing folder");
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Error removing " << folder << ".  Folder may not exist or may not be empty.");
		return true;
	}

	rclog.out("%s removed folder %s\n", accountName.text(), folder.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Folder " << folder << " has been removed.\n");
	msgPLI_RC_FILEBROWSER_START(CString() << "");

	return true;
}
