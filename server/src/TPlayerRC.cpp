#include "IDebug.h"
#include <vector>
#include <map>
#include <sys/stat.h>
#if defined(_WIN32) || defined(_WIN64)
	#include <direct.h>
	#define mkdir _mkdir
	#define rmdir _rmdir
#else
	#include <unistd.h>
#endif
#include <stdio.h>

#include "TServer.h"
#include "TPlayer.h"
#include "IEnums.h"
#include "TLevel.h"

#define serverlog	server->getServerLog()
#define rclog		server->getRCLog()
#define nclog		server->getNPCLog()
extern bool __playerPropsRC[propscount];

// Admin-only server options.  They are protected from being changed by people without the
// 'change staff account' right.
const char* __admin[] = {
	"name", "description", "url", "serverip", "serverport", "localip", "listip", "listport",
	"maxplayers", "onlystaff", "nofoldersconfig", "oldcreated", "serverside",
	"triggerhack_weapons", "triggerhack_guilds", "triggerhack_groups", "triggerhack_files",
	"triggerhack_rc", "flaghack_movement", "flaghack_ip",
	"sharefolder", "language"
};

// Files that are protected from being downloaded by people without the
// 'change staff account' right.
const char* __protectedFiles[] = {
	"accounts/defaultaccount.txt", "config/adminconfig.txt", "config/allowedversions.txt",
	"config/rchelp.txt",
};

// List of important files.
const char* __importantFiles[] = {
	"accounts/defaultaccount.txt", "config/adminconfig.txt", "config/allowedversions.txt",
	"config/foldersconfig.txt", "config/ipbans.txt", "config/rchelp.txt",
	"config/rcmessage.txt", "config/rules.txt", "config/servermessage.html",
	"config/serveroptions.txt",
};

const int __importantFileRights[] = {
	PLPERM_MODIFYSTAFFACCOUNT, PLPERM_MODIFYSTAFFACCOUNT, PLPERM_MODIFYSTAFFACCOUNT,
	PLPERM_SETFOLDEROPTIONS, PLPERM_MODIFYSTAFFACCOUNT, PLPERM_MODIFYSTAFFACCOUNT,
	PLPERM_MODIFYSTAFFACCOUNT, PLPERM_MODIFYSTAFFACCOUNT, PLPERM_SETSERVEROPTIONS,
	PLPERM_SETSERVEROPTIONS,
};

static void updateFile(TPlayer* player, TServer* server, CString& dir, CString& file);

void TPlayer::setPropsRC(CString& pPacket, TPlayer* rc)
{
	bool hadBomb = false, hadBow = false;
	CString outPacket;

	// Skip playerworld
	pPacket.readChars(pPacket.readGUChar());

	// Read props from the packet.
	CString props = pPacket.readChars(pPacket.readGUChar());

	// Send props out.
	setProps(props, (id != -1 ? PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF : 0), rc);

	// Clear flags
	for (auto i = flagList.begin(); i != flagList.end(); ++i)
	{
		outPacket >> (char)PLO_FLAGDEL << i->first;
		if (!i->second.isEmpty()) outPacket << "=" << i->second;
		outPacket << "\n";
	}

	// Clear Weapons
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

		// Do the same thing with the bow.
		if ((*i) == "bow")
		{
			outPacket >> (char)PLO_NPCWEAPONDEL << "Bow\n";
			hadBow = true;
		}
		if ((*i) == "Bow")
			hadBow = true;
	}
	if (id != -1) sendPacket(outPacket);

	// Clear the flags and re-populate the flag list.
	flagList.clear();
	auto flagCount = pPacket.readGUShort();
	while (flagCount > 0)
	{
		CString flag = pPacket.readChars(pPacket.readGUChar());
		std::string name = flag.readString("=").text();
		CString val = flag.readString("");

		setFlag(name, val, (id != -1));
		--flagCount;
	}

	// Clear the chests and re-populate the chest list.
	chestList.clear();
	auto chestCount = pPacket.readGUShort();
	while (chestCount > 0)
	{
		unsigned char len = pPacket.readGUChar();
		char loc[2] = {pPacket.readGChar(), pPacket.readGChar()};
		chestList.push_back(CString() << CString((int)loc[0]) << ":" << CString((int)loc[1]) << ":" << pPacket.readChars(len - 2));
		--chestCount;
	}

	// Clear the weapons and re-populate the weapons list.
	weaponList.clear();
	auto weaponCount = pPacket.readGUChar();
	while (weaponCount > 0)
	{
		unsigned char len = pPacket.readGUChar();
		if (len == 0) continue;
		CString wpn = pPacket.readChars(len);

		// Allow the bomb through if we are actually adding it.
		if (wpn == "bomb" || wpn == "Bomb")
			hadBomb = true;

		// Allow the bow through if we are actually adding it.
		if (wpn == "bow" || wpn == "Bow")
			hadBow = true;

		// Send the weapon to the player.
		this->addWeapon(wpn);
		--weaponCount;
	}

	// KILL THE BOMB DEAD
	if (id != -1)
	{
		if (!hadBomb)
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
	for (auto i = flagList.begin(); i != flagList.end(); ++i)
	{
		CString flag = i->first;
		if (!i->second.isEmpty()) flag << "=" << i->second;
		if (flag.length() > 0xDF) flag.removeI(0xDF);
		ret >> (char)flag.length() << flag;
	}

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
	for (auto & i : weaponList)
		ret >> (char)i.length() << i;

	return ret;
}

bool TPlayer::msgPLI_RC_SERVEROPTIONSGET(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to view the server options.", accountName.text());
		return true;
	}

	CSettings* settings = server->getSettings();

	sendPacket(CString() >> (char)PLO_RC_SERVEROPTIONSGET << settings->getSettings().gtokenize());
	return true;
}

bool TPlayer::msgPLI_RC_SERVEROPTIONSSET(CString& pPacket)
{
	if (isClient() || !hasRight(PLPERM_SETSERVEROPTIONS))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set the server options.", accountName.text());
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
		std::vector<CString> newOptions = options.tokenize("\n");
		options.clear();
		for (auto & newOption : newOptions)
		{
			CString name = newOption.subString(0, newOption.find("="));
			name.trimI();

			// See if this command is an admin command.
			bool isAdmin = false;
			for (auto & j : __admin)
				if (name == CString(j)) isAdmin = true;

			// If it is an admin command, replace it with the current value.
			if (isAdmin)
				newOption = CString() << name << " = " << settings->getStr(name);

			// Add this line back into options.
			options << newOption << "\n";
		}
	}

	// Save settings.
	settings->loadSettings(options, true, true);

	// Reload settings.
	server->loadSettings();
	server->loadMaps();
	rclog.out("%s has updated the server options.\n", accountName.text());

	// Send RC Information
	CString outPacket = CString() >> (char)PLO_RC_CHAT << accountName << " has updated the server options.";
	std::vector<TPlayer *> *playerList = server->getPlayerList();
	for (auto i = playerList->begin(); i != playerList->end(); ++i)
	{
		if ((*i)->getType() & PLTYPE_ANYRC)
		{
			(*i)->sendPacket(outPacket);
#ifdef V8NPCSERVER
			if (hasRight(PLPERM_NPCCONTROL))
				(*i)->sendNCAddr();
#endif
		}
	}

	return true;
}

bool TPlayer::msgPLI_RC_FOLDERCONFIGGET(CString& pPacket)
{
	if (isClient())
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
	if (isClient() || !hasRight(PLPERM_SETFOLDEROPTIONS))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set the folder config.", accountName.text());
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
	server->loadFileSystem();

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

	TPlayer* p = server->getPlayer(pPacket.readGUShort(), PLTYPE_ANYPLAYER);
	if (p == 0) return true;

	if (isClient() || (p->getAccountName() != accountName && !hasRight(PLPERM_SETATTRIBUTES)) || (p->getAccountName() == accountName && !hasRight(PLPERM_SETSELFATTRIBUTES)))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set a player's properties.", accountName.text());
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
	TPlayer* p = server->getPlayer(pPacket.readGUShort(), PLTYPE_ANYPLAYER);
	if (p == 0) return true;

	if (isClient() || !hasRight(PLPERM_DISCONNECT))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to disconnect %s.\n", accountName.text(), p->getAccountName().text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " is not authorized to disconnect players.");
		return true;
	}

	CString reason = pPacket.readString("");
	if (!reason.isEmpty())
		rclog.out("%s disconnected %s: %s\n", accountName.text(), p->getAccountName().text(), reason.text());
	else rclog.out("%s disconnected %s.\n", accountName.text(), p->getAccountName().text());
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " disconnected " << p->getAccountName());

	CString disconnectMessage = CString() << "One of the server administrators, " << accountName << ", has disconnected you";
	if (!reason.isEmpty())
		disconnectMessage << " for the following reason: " << reason;
	else disconnectMessage << ".";
	p->sendPacket(CString() >> (char)PLO_DISCMESSAGE << disconnectMessage);
	server->deletePlayer(p);
	return true;
}

bool TPlayer::msgPLI_RC_UPDATELEVELS(CString& pPacket)
{
	if (isClient() || !hasRight(PLPERM_UPDATELEVEL))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to update levels.\n", accountName.text());
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
	if (isClient() || !hasRight(PLPERM_ADMINMSG))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to send an admin message.\n", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to send an admin message.");
		return true;
	}

	server->sendPacketToAll(CString() >> (char)PLO_RC_ADMINMESSAGE << "Admin " << accountName << ":\xa7" << pPacket.readString(""), this);
	return true;
}

bool TPlayer::msgPLI_RC_PRIVADMINMESSAGE(CString& pPacket)
{
	if (isClient() || !hasRight(PLPERM_ADMINMSG))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to send an admin message.\n", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to send an admin message.");
		return true;
	}

	TPlayer* p = server->getPlayer(pPacket.readGUShort(), PLTYPE_ANYPLAYER);
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
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to view the server flags.\n", accountName.text());
		return true;
	}
	CString ret;
	ret >> (char)PLO_RC_SERVERFLAGSGET >> (short)server->getServerFlags()->size();
	for (auto i = server->getServerFlags()->begin(); i != server->getServerFlags()->end(); ++i)
	{
		CString flag = CString() << i->first << "=" << i->second;
		ret >> (char)flag.length() << flag;
	}
	sendPacket(ret);
	return true;
}

bool TPlayer::msgPLI_RC_SERVERFLAGSSET(CString& pPacket)
{
	if (isClient() || !hasRight(PLPERM_SETSERVERFLAGS))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set the server flags.\n", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set the server flags.");
		return true;
	}

	unsigned short count = pPacket.readGUShort();
	std::unordered_map<std::string, CString> * serverFlags = server->getServerFlags();

	// Save server flags.
	std::unordered_map<std::string, CString> oldFlags = *serverFlags;

	// Delete server flags.
	serverFlags->clear();

	// Assemble the new server flags.
	for (unsigned int i = 0; i < count; ++i)
		server->setFlag(pPacket.readChars(pPacket.readGUChar()), false);

	// Send flag changes to all players.
	for (auto i = serverFlags->begin(); i != serverFlags->end(); ++i)
	{
		bool found = false;
		for (auto j = oldFlags.begin(); j != oldFlags.end();)
		{
			// Flag name
			if (i->first == j->first)
			{
				// Check to see if the values are the same.
				// If they are, set found to true so we don't send it to the player again.
				if (i->second == j->second)
					found = true;
				oldFlags.erase(j++);
				if (found) break;
			}
			else ++j;
		}

		// If we didn't find a match, this is either a new flag, or a changed flag.
		if (!found)
		{
			if (i->second.isEmpty())
				server->sendPacketTo(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGSET << i->first);
			else
				server->sendPacketTo(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGSET << i->first << "=" << i->second);
		}
	}

	// If any flags were deleted, tell that to the players now.
	for (auto i = oldFlags.begin(); i != oldFlags.end(); ++i)
		server->sendPacketTo(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGDEL << i->first);

	rclog.out("%s has updated the server flags.\n", accountName.text());
	server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << accountName << " has updated the server flags.");
	return true;
}

bool TPlayer::msgPLI_RC_ACCOUNTADD(CString& pPacket)
{
	if (isClient() || !hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to add a new account.\n", accountName.text());
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
	if (isClient() || !hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to delete an account.\n", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to delete accounts.");
		return true;
	}

	// Get the account.
	// Prevent the defaultaccount from being deleted.
	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);
	if (server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
		return true;

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
	if (isClient())
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
	for (std::map<CString, CString>::iterator i = fs->getFileList().begin(); i != fs->getFileList().end(); ++i)
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
	TPlayer* p = server->getPlayer(pPacket.readGUShort(), PLTYPE_ANYPLAYER | PLTYPE_NPCSERVER);
	if (p == 0) return true;

	if (isClient() || !hasRight(PLPERM_VIEWATTRIBUTES))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to view the props of player %s.\n", accountName.text(), p->getAccountName().text());
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
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	TPlayer* p = server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == 0)
	{
		if (server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return true;

		offline = true;
		p = new TPlayer(server, 0, -1);
		if (!p->loadAccount(acc))
		{
			delete p;
			return true;
		}
	}

	if (isClient() || !hasRight(PLPERM_VIEWATTRIBUTES))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to view the props of player %s.\n", accountName.text(), p->getAccountName().text());
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
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !hasRight(PLPERM_RESETATTRIBUTES))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to reset the account: %s\n", accountName.text(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to reset accounts.\n");
		return true;
	}

	// Get the player.  Create a new player if they are offline.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == 0)
	{
		if (server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return true;

		offline = true;
		p = new TPlayer(server, 0, -1);
		if (!p->loadAccount(acc))
		{
			delete p;
			return true;
		}
	}

	// Save RC stuff.
	CString adminip = p->getAdminIp();
	int rights = p->getAdminRights();
	std::vector<CString> folders;
	for (std::vector<CString>::iterator i = p->getFolderList()->begin(); i != p->getFolderList()->end(); ++i)
		folders.push_back(*i);

	// Reset the player.
	p->reset();

	// Add the RC stuff back in.
	p->setAdminIp(adminip);
	p->setAdminRights(rights);
	std::vector<CString>* pFolders = p->getFolderList();
	for (std::vector<CString>::iterator i = folders.begin(); i != folders.end(); ++i)
		pFolders->push_back(*i);

	// Save the account.
	p->saveAccount();

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
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	TPlayer* p = server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == 0)
	{
		if (server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return true;

		offline = true;
		p = new TPlayer(server, 0, -1);
		if (!p->loadAccount(acc))
		{
			delete p;
			return true;
		}
	}

	if (isClient() || (p->getAccountName() != accountName && !hasRight(PLPERM_SETATTRIBUTES)) || (p->getAccountName() == accountName && !hasRight(PLPERM_SETSELFATTRIBUTES)))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set a player's properties.", accountName.text());
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
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient())
	{
		rclog.out("[Hack] %s attempted to view the account: %s\n", accountName.text(), acc.text());
		return true;
	}

	// Get the player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == 0)
	{
		if (server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return true;

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
	if (acc.length() == 0) return true;
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to edit the account: %s\n", accountName.text(), acc.text());
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
	TPlayer* p = server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == 0)
	{
		if (server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return true;

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
	TPlayer* pRC = server->getPlayer(acc, PLTYPE_ANYRC);
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
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to send a message to RC.\n", accountName.text());
		return true;
	}

	CString message = pPacket.readString("");
	if (message.isEmpty()) return true;
	std::vector<CString> words = message.tokenize();

#ifdef V8NPCSERVER
	if (isNC())
	{
		// TODO(joey): All RC's with NC support are sending two messages at a time.
		//  Can use this section for npc-server related commands though.
		//server->sendToNC(CString(nickName) << ": " << message);
		return true;
	}
#endif

	if (words[0].text()[0] != '/')
	{
		server->sendToRC(CString(nickName) << ": " << message);
		return true;
	}
	else
	{
#ifndef NDEBUG
		if (words[0] == "/sendtext") {
			sendPacket(CString() >> (char)PLO_SERVERTEXT << message.subString(10) << "\n");
		}
		else
#endif
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
			server->loadServerMessage();
			server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " refreshed the server message.");
			rclog.out("%s refreshed the server message.\n", accountName.text());
		}
		else if (words[0] == "/refreshfilesystem" && words.size() == 1)
		{
			server->loadFileSystem();
			server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " refreshed the server file list.");
			rclog.out("%s refreshed the server file list.\n", accountName.text());
		}
		else if (words[0] == "/updatelevel" && words.size() != 1 && hasRight(PLPERM_UPDATELEVEL))
		{
			std::vector<CString> levels = words[1].tokenize(",");
			for (std::vector<CString>::const_iterator i = levels.begin(); i != levels.end(); ++i)
			{
				TLevel* level = server->getLevel(*i);
				if (level)
				{
					server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " updated level: " << level->getLevelName());
					rclog.out("%s updated level: %s\n", accountName.text(), level->getLevelName().text());
					level->reload();
				}
			}
		}
		else if (words[0] == "/updatelevelall" && words.size() == 1 && hasRight(PLPERM_UPDATELEVEL))
		{
			rclog.out("%s updated all the levels", accountName.text());
			int count = 0;
			std::vector<TLevel*>* levels = server->getLevelList();
			for (std::vector<TLevel*>::iterator i = levels->begin(); i != levels->end(); ++i)
			{
				(*i)->reload();
				++count;
			}
			server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " updated all the levels (" << CString((int)count) << " levels updated).");
			rclog.out(" (%d levels updated).\n", count);
		}
		else if (words[0] == "/restartserver" && words.size() == 1 && hasRight(PLPERM_MODIFYSTAFFACCOUNT))
		{
			server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " restarted the server.");
			rclog.out("%s restarted the server.\n", accountName.text());
			server->restart();
		}
		else if (words[0] == "/reloadserver" && words.size() == 1 && hasRight(PLPERM_MODIFYSTAFFACCOUNT))
		{
			server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " reloaded the server configuration files.");
			rclog.out("%s reloaded the server configuration files.\n", accountName.text());
			server->loadConfigFiles();
		}
		else if (words[0] == "/updateserverhq" && words.size() == 1 && hasRight(PLPERM_MODIFYSTAFFACCOUNT))
		{
			server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " sent ServerHQ updates.");
			rclog.out("%s sent ServerHQ updates.\n", accountName.text());
			server->loadAdminSettings();
			server->getServerList()->sendServerHQ();
		}
		else if (words[0] == "/reloadwordfilter" && words.size() == 1)
		{
			server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " reloaded the word filter.");
			rclog.out("%s reloaded the word filter.\n", accountName.text());
			server->loadWordFilter();
		}
		else if (words[0] == "/reloadipbans" && words.size() == 1)
		{
			server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " reloaded the ip bans.");
			rclog.out("%s reloaded the ip bans.\n", accountName.text());
			server->loadIPBans();
		}
		else if (words[0] == "/reloadweapons" && words.size() == 1)
		{
			server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " reloaded the weapons.");
			rclog.out("%s reloaded the weapons.\n", accountName.text());
			server->loadWeapons(true);
		}
#ifdef V8NPCSERVER
		else if (words[0] == "/savenpcs" && words.size() == 1)
		{
			server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << accountName << " saved npc to disk.");
			nclog.out("%s saved the npcs to disk.\n", accountName.text());
			server->saveNpcs();
		}
		else if (words[0] == "/stats" && words.size() == 1)
		{
			auto npcStats = server->calculateNpcStats();

			sendPacket(CString() >> (char)PLO_RC_CHAT << "Top scripts using the most execution time (in the last min)");

			int idx = 0;
			for (auto it = npcStats.begin(); it != npcStats.end(); ++it)
			{
				idx++;
				sendPacket(CString() >> (char)PLO_RC_CHAT << CString(idx) << ". 	" << CString((*it).first) << "	" << (*it).second);
				if (idx == 50)
					break;
			}
		}
#endif
		else if(words[0] == "/find" && words.size() > 1)
		{
			std::map<CString, CString> found;

			// Assemble the search string.
			CString search(words[1]);
			for (unsigned int i = 2; i < words.size(); ++i)
				search << " " << words[i];

			// Search for the files.
			for (unsigned int i = 0; i < FS_COUNT; ++i)
			{
				auto& fileList = server->getFileSystem(i)->getFileList();
				CString fs("none");
				if (i == 0) fs = "all";
				if (i == 1) fs = "file";
				if (i == 2) fs = "level";
				if (i == 3) fs = "head";
				if (i == 4) fs = "body";
				if (i == 5) fs = "sword";
				if (i == 6) fs = "shield";

				for (std::map<CString, CString>::const_iterator i = fileList.begin(); i != fileList.end(); ++i)
				{
					if (i->first.match(search))
						found[i->second.removeAll(server->getServerPath())] = fs;
				}
			}

			// Return a list of files found.
			for (std::map<CString, CString>::const_iterator i = found.begin(); i != found.end(); ++i)
			{
				sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: File found (" << search << "): " << i->first << " [" << i->second << "]");
			}

			// No files found.
			if (found.size() == 0)
				sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: No files found matching: " << search);
		}
	}

	return true;
}

bool TPlayer::msgPLI_RC_WARPPLAYER(CString& pPacket)
{
	if (isClient() || !hasRight(PLPERM_WARPTOPLAYER))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to warp a player.\n", accountName.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to warp players.\n");
		return true;
	}

	TPlayer* p = server->getPlayer(pPacket.readGUShort(), PLTYPE_ANYPLAYER);
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
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || (acc != accountName && !hasRight(PLPERM_SETRIGHTS)))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to get the rights of %s\n", accountName.text(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to view that player's rights.");
		return true;
	}

//	int rights = 0;
	CString folders, ip;

	// Get player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == 0)
	{
		if (server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return true;

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
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !hasRight(PLPERM_SETRIGHTS))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set the rights of %s\n", accountName.text(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set player rights.");
		return true;
	}

	// Get player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == 0)
	{
		if (server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return true;

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

	int changed_rights = adminRights ^ n_adminRights;
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
	TPlayer* pRC = server->getPlayer(acc, PLTYPE_ANYRC);
	if (pRC)
	{
		pRC->loadAccount(acc, true);

#ifdef V8NPCSERVER
		if (changed_rights & PLPERM_NPCCONTROL)
		{
			if (!(n_adminRights & PLPERM_NPCCONTROL))
			{
				TPlayer *pNC = server->getPlayer(acc, PLTYPE_ANYNC);
				if (pNC)
					pNC->disconnect();
			}
			else pRC->sendNCAddr();
		}
#endif

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
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient())
	{
		rclog.out("[Hack] %s attempted to get the comments of %s\n", accountName.text(), acc.text());
		return true;
	}

	// Get player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == 0)
	{
		if (server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return true;

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
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !hasRight(PLPERM_SETCOMMENTS))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set the comments of %s\n", accountName.text(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set player comments.");
		return true;
	}

	// Get player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == 0)
	{
		if (server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return true;

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
	TPlayer* pRC = server->getPlayer(acc, PLTYPE_ANYRC);
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
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient())
	{
		rclog.out("[Hack] %s attempted to view the ban of %s\n", accountName.text(), acc.text());
		return true;
	}

	// Get player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == 0)
	{
		if (server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return true;

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
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !hasRight(PLPERM_BAN))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set the ban of %s\n", accountName.text(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set player bans.");
		return true;
	}

	// Get player.
	bool offline = false;
	TPlayer* p = server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == 0)
	{
		if (server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return true;

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
	TPlayer* pRC = server->getPlayer(acc, PLTYPE_ANYRC);
	if (pRC)
	{
		pRC->loadAccount(acc);
	}

	// If the player was just now banned, kick him off the server.
	if (banned && !offline)
	{
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
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to open the File Browser.\n", accountName.text());
		return true;
	}

	// If the player has no folder rights, don't open the File Browser.
	if (folderList.size() == 0)
		return true;

	// Get folder list to send to the client.
	CString folders;
	for (std::vector<CString>::iterator i = folderList.begin(); i != folderList.end(); ++i)
		folders << *i << "\n";

	// Send the folder list and the welcome message.
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_DIRLIST << folders.gtokenize());
	if (!isFtp) sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Welcome to the File Browser.");

	// Create a folder map.
	std::map<CString, CString> folderMap;
	for (std::vector<CString>::iterator i = folderList.begin(); i != folderList.end(); ++i)
	{
		CString rights("r");
		CString wild("*");
		CString folder(*i);
		rights = folder.readString(" ").trim().toLower();
		folder.removeI(0, folder.readPos());
		folder.replaceAllI("\\", "/");
		folder.trimI();
		if (folder[folder.length() - 1] != '/')
		{
			int pos = folder.findl('/');
			if (pos != -1)
			{
				wild = folder.subString(pos + 1);
				folder.removeI(pos + 1);
			}
		}
		folderMap[folder] << rights << ":" << wild << "\n";
	}

	// See if we can use our lastFolder.  If we can't, use the first folder.
	if (folderMap.find(lastFolder) == folderMap.end())
		lastFolder = folderMap.begin()->first;

	// Create the file system.
	CFileSystem fs(server);
	fs.addDir(lastFolder);

	// Construct the file list.
	CString files;
	std::vector<CString> wildcards = folderMap[lastFolder].tokenize("\n");
	for (std::vector<CString>::iterator i = wildcards.begin(); i != wildcards.end(); ++i)
	{
		CString rights = (*i).readString(":");
		CString wildcard = (*i).readString("");
		(*i).setRead(0);
		for (std::map<CString, CString>::iterator j = fs.getFileList().begin(); j != fs.getFileList().end(); ++j)
		{
			// See if the file matches the wildcard.
			if (!j->first.match(wildcard))
				continue;

			CString name = j->first;
			CString dir;

			// Add the file now.
			int size = fs.getFileSize(j->first);
			time_t mod = fs.getModTime(j->first);
			dir >> (char)j->first.length() << j->first >> (char)rights.length() << rights >> (long long)size >> (long long)mod;
			files << " " >> (char)dir.length() << dir;
		}
	}

	// Send packet.
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_DIR >> (char)lastFolder.length() << lastFolder << files);
	isFtp = true;

	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_CD(CString& pPacket)
{
	if (isClient()) return true;

	CString newFolder = pPacket.readString("");
	CString newRights, wildcard;
	newFolder.setRead(0);

	// Create a folder map.
	std::map<CString, CString> folderMap;
	for (std::vector<CString>::iterator i = folderList.begin(); i != folderList.end(); ++i)
	{
		CString rights("r");
		CString wild("*");
		CString folder(*i);
		rights = folder.readString(" ").trim().toLower();
		folder.removeI(0, folder.readPos());
		folder.replaceAllI("\\", "/");
		folder.trimI();
		if (folder[folder.length() - 1] != '/')
		{
			int pos = folder.findl('/');
			if (pos != -1)
			{
				wild = folder.subString(pos + 1);
				folder.removeI(pos + 1);
			}
		}
		folderMap[folder] << rights << ":" << wild << "\n";
	}

	// See if newFolder is part of the folder map.
	// If it isn't, return.
	if (folderMap.find(newFolder) == folderMap.end())
		return true;
	else lastFolder = newFolder;

	// Create the file system.
	CFileSystem fs(server);
	fs.addDir(lastFolder);

	// Make sure our folder exists.
	CString mkdir_path = CString() << server->getServerPath();
	std::vector<CString> f = lastFolder.tokenize('/');
	for (std::vector<CString>::iterator i = f.begin(); i != f.end(); ++i)
	{
		if (i->isEmpty()) continue;
		mkdir_path << *i << '/';

#if defined(_WIN32) || defined(_WIN64)
		mkdir(mkdir_path.text());
#else
		mkdir(mkdir_path.text(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
	}

	// Construct the file list.
	// file packet: {CHAR name_length}{STRING name}{CHAR rights_length}{STRING rights}{INT5 file_size}{INT5 file_mod_time}
	// files: {CHAR file_packet_length}{file_packet}[space]{CHAR file_packet_length}{file_packet}[space]
	CString files;
	std::vector<CString> wildcards = folderMap[lastFolder].tokenize("\n");
	for (std::vector<CString>::iterator i = wildcards.begin(); i != wildcards.end(); ++i)
	{
		CString rights = (*i).readString(":");
		CString wildcard = (*i).readString("");
		(*i).setRead(0);
		for (std::map<CString, CString>::iterator j = fs.getFileList().begin(); j != fs.getFileList().end(); ++j)
		{
			// See if the file matches the wildcard.
			if (!j->first.match(wildcard))
				continue;

			CString name = j->first;
			CString dir;

			// Add the file now.
			int size = fs.getFileSize(j->first);
			time_t mod = fs.getModTime(j->first);
			dir >> (char)j->first.length() << j->first >> (char)rights.length() << rights >> (long long)size >> (long long)mod;
			files << " " >> (char)dir.length() << dir;
		}
	}

	// Send packet.
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Folder changed to " << lastFolder);
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_DIR >> (char)lastFolder.length() << lastFolder << files);

	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_END(CString& pPacket)
{
	if (isClient()) return true;
	isFtp = false;

	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_DOWN(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to download a file through the File Browser.\n", accountName.text());
		return true;
	}

	// Send file.
	CString file = pPacket.readString("");
	CString filepath = CString() << server->getServerPath() << lastFolder << file;
	CString checkFile = CString() << lastFolder << file;

	// Don't let us download/view important files.
	if (!hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		for (unsigned int j = 0; j < sizeof(__protectedFiles) / sizeof(const char*); ++j)
		{
			if (checkFile == CString(__protectedFiles[j]))
			{
				sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Insufficient rights to download/view " << checkFile);
				return true;
			}
		}
	}

	this->sendFile(lastFolder, file);

	rclog.out("%s downloaded file %s\n", accountName.text(), file.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Downloaded file " << file);

	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_UP(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to upload a file through the File Browser.\n", accountName.text());
		return true;
	}

	CString file = pPacket.readChars(pPacket.readGUChar());
	CString filepath = CString() << server->getServerPath() << lastFolder;
	CString fileData = pPacket.subString(pPacket.readPos());
	CString checkFile = CString() << lastFolder << file;

	// Check if this is a protected file.
	bool isProtected = false;
	int fileID = -1;
	for (int i = 0; i < sizeof(__importantFiles) / sizeof(const char*); ++i)
	{
		if (checkFile == CString(__importantFiles[i]))
		{
			fileID = i;
			isProtected = true;
			break;
		}
	}

	// If this file is protected, see if we have permission to upload this file.
	bool hasPermission = true;
	if (isProtected)
	{
		hasPermission = hasRight(PLPERM_MODIFYSTAFFACCOUNT);
		if (!hasPermission)
		{
			if (fileID < (sizeof(__importantFileRights) / sizeof(const int)))
				hasPermission = hasRight(__importantFileRights[fileID]);
		}
	}

	// Don't let us upload/overwrite important files.
	if (isProtected && !hasPermission)
	{
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Insufficent rights to upload " << checkFile);
		return true;
	}

	// See if we are uploading a large file or not.
	if (rcLargeFiles.find(file) == rcLargeFiles.end())
	{
		// Normal file. Save it and display our message.
		fileData.save(filepath << file);

		rclog.out("%s uploaded file %s\n", accountName.text(), file.text());
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Uploaded file " << file);

		if (file.find(".gupd") != -1)
			server->sendPacketToAll(CString() >> (char)PLO_UPDATEPACKAGEISUPDATED << file, this);

		// Update file.
		updateFile(this, server, lastFolder, file);
	}
	else
	{
		// Large file.  Store the data in memory.
		rcLargeFiles[file] << fileData;
	}

	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_MOVE(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to move a file through the File Browser.\n", accountName.text());
		return true;
	}

	CString source;
	CString destination;
	CString dir(pPacket.readChars(pPacket.readGUChar()));
	CString file(pPacket.readString(""));

	// Fix file.
	file.removeAllI("\"");

	// Add trailing directory slash if it is missing.
	if (dir[dir.length() - 1] != '\\' && dir[dir.length() - 1] != '/')
		dir << "/";

	// Assemble destination and source.
	destination << dir << file;
	source << lastFolder << file;

	// Don't let us move important files.
	for (unsigned int j = 0; j < sizeof(__importantFiles) / sizeof(const char*); ++j)
	{
		if (source == CString(__importantFiles[j]))
		{
			sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Not allowed to move file " << source);
			return true;
		}
	}

	rclog.out("%s moved file %s to %s\n", accountName.text(), source.text(), destination.text());

	// Add working directory.
	source = CString(server->getServerPath()) << source;
	destination = CString(server->getServerPath()) << destination;
	CFileSystem::fixPathSeparators(source);
	CFileSystem::fixPathSeparators(destination);

	// Save the new file now.
	CString temp;
	temp.load(source);
	if (temp.save(destination) == false)
		return true;

	// Remove the old file.
#if defined(WIN32) || defined(WIN64)
	wchar_t* wcstr = 0;

	// Determine if the filename is UTF-8 encoded.
	int wcsize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, source.text(), source.length(), 0, 0);
	if (wcsize != 0)
	{
		wcstr = new wchar_t[wcsize + 1];
		memset((void *)wcstr, 0, (wcsize + 1) * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, source.text(), source.length(), wcstr, wcsize);
	}
	else
	{
		wcstr = new wchar_t[source.length() + 1];
		for (int i = 0; i < source.length(); ++i)
			wcstr[i] = (unsigned char)source[i];
		wcstr[source.length()] = 0;
	}

	// Remove the file now.
	_wremove(wcstr);
	delete[] wcstr;
#else
	remove(source.text());
#endif

	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_DELETE(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to delete a file through the File Browser.\n", accountName.text());
		return true;
	}

	CString file = pPacket.readString("");
	CString filePath = CString() << server->getServerPath() << lastFolder << file;
	CString checkFile = CString() << lastFolder << file;
	CFileSystem::fixPathSeparators(filePath);

	// Don't let us delete important files.
	for (unsigned int j = 0; j < sizeof(__importantFiles) / sizeof(const char*); ++j)
	{
		if (checkFile == CString(__importantFiles[j]))
		{
			sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Not allowed to delete file " << checkFile);
			return true;
		}
	}

#if defined(WIN32) || defined(WIN64)
	wchar_t* wcstr = 0;

	// Determine if the filename is UTF-8 encoded.
	int wcsize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filePath.text(), filePath.length(), 0, 0);
	if (wcsize != 0)
	{
		wcstr = new wchar_t[wcsize + 1];
		memset((void *)wcstr, 0, (wcsize + 1) * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, filePath.text(), filePath.length(), wcstr, wcsize);
	}
	else
	{
		wcstr = new wchar_t[filePath.length() + 1];
		for (int i = 0; i < filePath.length(); ++i)
			wcstr[i] = (unsigned char)filePath[i];
		wcstr[filePath.length()] = 0;
	}

	// Remove the file now.
	_wremove(wcstr);
	delete[] wcstr;
#else
	remove(filePath.text());
#endif

	rclog.out("%s deleted file %s\n", accountName.text(), file.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Deleted file " << file);

	return true;
}

bool TPlayer::msgPLI_RC_FILEBROWSER_RENAME(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to rename a file through the File Browser.\n", accountName.text());
		return true;
	}

	CString f1 = pPacket.readChars(pPacket.readGUChar());
	CString f2 = pPacket.readChars(pPacket.readGUChar());
	CString f1path = CString() << server->getServerPath() << lastFolder << f1;
	CString f2path = CString() << server->getServerPath() << lastFolder << f2;
	CString checkFile1 = CString() << lastFolder << f1;
	CString checkFile2 = CString() << lastFolder << f2;
	CFileSystem::fixPathSeparators(f1path);
	CFileSystem::fixPathSeparators(f2path);


	// Don't let us rename/overwrite important files.
	for (unsigned int j = 0; j < sizeof(__importantFiles) / sizeof(const char*); ++j)
	{
		if (checkFile1 == CString(__importantFiles[j]))
		{
			sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Not allowed to rename/overwrite file " << checkFile1 << " or " << checkFile2);
			return true;
		}
	}

	// Renaming our logs?  First, we need to close them.
	if (lastFolder == "logs/")
	{
		if (f1 == "rclog.txt") rclog.close();
		else if (f1 == "serverlog.txt") serverlog.close();
	}

	// Do the renaming.
#if defined(WIN32) || defined(WIN64)
	// Convert to wchar_t because the filename might be UTF-8 encoded.
	wchar_t* f1_wcstr = 0;
	wchar_t* f2_wcstr = 0;

	// Test f1path.
	int f1_wcsize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, f1path.text(), f1path.length(), 0, 0);
	if (f1_wcsize != 0)
	{
		f1_wcstr = new wchar_t[f1_wcsize + 1];
		memset((void *)f1_wcstr, 0, (f1_wcsize + 1) * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, f1path.text(), f1path.length(), f1_wcstr, f1_wcsize);
	}
	else
	{
		f1_wcstr = new wchar_t[f1path.length() + 1];
		for (int i = 0; i < f1path.length(); ++i)
			f1_wcstr[i] = (unsigned char)f1path[i];
		f1_wcstr[f1path.length()] = 0;
	}

	// Test f2path.
	int f2_wcsize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, f2path.text(), f2path.length(), 0, 0);
	if (f2_wcsize != 0)
	{
		f2_wcstr = new wchar_t[f2_wcsize + 1];
		memset((void *)f2_wcstr, 0, (f2_wcsize + 1) * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, f2path.text(), f2path.length(), f2_wcstr, f2_wcsize);
	}
	else
	{
		f2_wcstr = new wchar_t[f2path.length() + 1];
		for (int i = 0; i < f2path.length(); ++i)
			f2_wcstr[i] = f2path[i];
		f2_wcstr[f2path.length()] = 0;
	}

	// Rename.
	_wrename(f1_wcstr, f2_wcstr);
	delete[] f1_wcstr;
	delete[] f2_wcstr;
#else
	// Linux uses UTF-8 filenames.
	rename(f1path.text(), f2path.text());
#endif

	// Renaming our logs?  We can open them now.
	if (lastFolder == "logs/")
	{
		if (f1 == "rclog.txt") rclog.open();
		else if (f1 == "serverlog.txt") serverlog.open();
	}

	rclog.out("%s renamed file %s to %s\n", accountName.text(), f1.text(), f2.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Renamed file " << f1 << " to " << f2);

	return true;
}

bool TPlayer::msgPLI_RC_LARGEFILESTART(CString& pPacket)
{
	if (isClient())
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
	if (isClient())
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

	// Update file.
	updateFile(this, server, lastFolder, file);

	rclog.out("%s uploaded large file %s\n", accountName.text(), file.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Uploaded large file " << file);

	return true;
}

bool TPlayer::msgPLI_RC_FOLDERDELETE(CString& pPacket)
{
	CString folder = pPacket.readString("");
	CString folderpath = CString() << server->getServerPath() << folder;
	CFileSystem::fixPathSeparators(folderpath);
	folderpath.removeI(folderpath.length() -1);
	if (isClient())
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


bool TPlayer::msgPLI_NPCSERVERQUERY(CString& pPacket)
{
#ifdef V8NPCSERVER
	// Read Packet Data
	unsigned short pid = pPacket.readGUShort();
	CString message = pPacket.readString("");

	// Enact upon the message.
	if (message == "location")
		sendNCAddr();
#endif

	return true;
}

void updateFile(TPlayer* player, TServer* server, CString& dir, CString& file)
{
	CSettings* settings = server->getSettings();
	CString fullPath(dir);
	fullPath << file;

	// Find the file extension.
	CString ext = getExtension(file);

	// Check and see if it is an account.
	if (dir == "accounts/")
	{
		CFileSystem* fs = server->getAccountsFileSystem();
		if (fs->find(file).isEmpty())
			fs->addFile(CString() << dir << file);
		return;
	}

	// If folder config is off, add it to the file list.
	if ( settings->getBool("nofoldersconfig", false))
	{
		CFileSystem* fs = server->getFileSystem();
		if (fs->find(file).isEmpty())
			fs->addFile(CString() << dir << file);
	}
	// If folder config is on, try to find which file system to add it to.
	else
	{
		std::vector<CString> foldersConfig = CString::loadToken(CString() << server->getServerPath() << "config/foldersconfig.txt", "\n", true);
		for (auto & folderConfig : foldersConfig)
		{
			CString type = folderConfig.readString(" ").trim();
			CString folder("world/");
			folder << folderConfig.readString("").trim();

			if (fullPath.match(folder))
			{
				CFileSystem* fs = server->getFileSystemByType(type);
				CFileSystem* fs2 = server->getFileSystem();

				// See if it exists in that file system.
				if (fs->find(file).isEmpty())
				{
					if (fs2->find(file).isEmpty())
						fs2->addFile(fullPath);

					fs->addFile(fullPath);
					//printf("adding %s to %s\n", file.text(), type.text());
					break;
				}
			}
		}
	}

	// If it is a level, see if we can update it.
	// TODO: Should combine all server options loading/saving into one function in TServer.
	if (ext == ".nw" || ext == ".graal" || ext == ".zelda")
	{
		TLevel* l = TLevel::findLevel(file, server);
		if (l) l->reload();
	}
	else if (ext == ".dump" || dir.findi(CString("weapons")) > -1)
		server->loadWeapons(true);
	else if (file == "serveroptions.txt")
	{
		server->loadSettings();
		server->loadMaps();
	}
	else if (file == "adminconfig.txt")
		server->loadAdminSettings();
	else if (file == "allowedversions.txt")
		server->loadAllowedVersions();
	else if (file == "foldersconfig.txt")
		server->loadFileSystem();
	else if (file == "serverflags.txt")
		server->loadServerFlags();
	else if (file == "servermessage.html")
		server->loadServerMessage();
	else if (file == "ipbans.txt")
		server->loadIPBans();
	else if (file == "rules.txt")
		server->loadWordFilter();
}
