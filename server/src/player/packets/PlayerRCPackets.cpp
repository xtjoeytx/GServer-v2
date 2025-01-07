#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#define mkdir _mkdir
#define rmdir _rmdir
#else
#include <unistd.h>
#endif

#include <fmt/format.h>

#include <CString.h>

#include "IConfig.h"

#include "Server.h"
#include "level/Level.h"
#include "object/PlayerClient.h"
#include "object/PlayerRC.h"
#include "utilities/TimeUnits.h"
#include "utilities/StringUtils.h"

///////////////////////////////////////////////////////////////////////////////

#define serverlog m_server->getServerLog()
#define rclog m_server->getRCLog()

///////////////////////////////////////////////////////////////////////////////

static void updateFile(Player* player, Server* server, const CString& dir, const CString& file)
{
	auto& settings = server->getSettings();
	CString fullPath(dir);
	fullPath << file;

	// Find the file extension.
	CString ext = getExtension(file);

	// Check and see if it is an account.
	if (dir == "accounts/")
	{
		FileSystem* fs = server->getAccountsFileSystem();
		if (fs->find(file).isEmpty())
			fs->addFile(CString() << dir << file);
		return;
	}

	bool isNewFile = false;

	// If folder config is off, add it to the file list.
	if (settings.getBool("nofoldersconfig", false))
	{
		FileSystem* fs = server->getFileSystem();
		if (fs->find(file).isEmpty())
		{
			fs->addFile(CString() << dir << file);
			isNewFile = true;
		}
	}
	// If folder config is on, try to find which file system to add it to.
	else
	{
		std::vector<CString> foldersConfig = CString::loadToken(server->getServerPath() << "config/foldersconfig.txt", "\n", true);
		for (auto& folderConfig : foldersConfig)
		{
			CString type = folderConfig.readString(" ").trim();
			CString folder("world/");
			folder << folderConfig.readString("").trim();

			if (fullPath.match(folder))
			{
				FileSystem* fs = server->getFileSystemByType(type);
				FileSystem* fs2 = server->getFileSystem();

				// See if it exists in that file system.
				if (fs->find(file).isEmpty())
				{
					if (fs2->find(file).isEmpty())
						fs2->addFile(fullPath);

					fs->addFile(fullPath);
					isNewFile = true;
					//printf("adding %s to %s\n", file.text(), type.text());
					break;
				}
			}
		}
	}

	// If it is a level, see if we can update it.
	// TODO: Should combine all server options loading/saving into one function in Server.
	if (ext == ".nw" || ext == ".graal" || ext == ".zelda")
	{
		auto l = Level::findLevel(file, server);
		if (l) l->reload();
	}
	else if (ext == ".gupd")
		server->getPackageManager().findOrAddResource(file.text())->reload(server);
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
	else
	{
		// Check if this is a file that previously existed on the server so we
		// can notify existing clients that the file was updated.
		auto fileSystem = server->getFileSystem(FS_FILE);
		if (!isNewFile && !fileSystem->find(file).isEmpty())
		{
			// Game files
			const auto& playerList = server->getPlayerList();
			auto fileName = file.toString();

			CString updatePacket;
			updatePacket >> (char)PLO_UPDATEPACKAGEISUPDATED << file << "\n";

			// Ganis need to be recompiled on update
			CString bytecodePacket;
			if (ext == ".gani")
			{
				auto& aniManager = server->getAnimationManager();

				// delete the resource
				aniManager.deleteResource(fileName);

				// reload the resource to compile the bytecode again
				auto findAni = aniManager.findOrAddResource(fileName);
				if (findAni)
					bytecodePacket << findAni->getBytecodePacket();
			}

			// Send the update packet to any v4+ clients that have seen this file
			for (const auto& [pid, pl] : players_of_type<PlayerClient>(playerList))
			{
				if (pl->getVersion() >= CLVER_4_0211)
				{
					if (pl->hasSeenFile(fileName))
						pl->sendPacket(updatePacket);

					// Send GS2 gani scripts
					if (!bytecodePacket.isEmpty())
						pl->sendPacket(bytecodePacket);
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

HandlePacketResult PlayerRC::msgPLI_RC_SERVEROPTIONSGET(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to view the server options.", account.name.c_str());
		return HandlePacketResult::Handled;
	}

	auto& settings = m_server->getSettings();

	sendPacket(CString() >> (char)PLO_RC_SERVEROPTIONSGET << settings.getSettings().gtokenize());
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_SERVEROPTIONSSET(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_SETSERVEROPTIONS))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set the server options.", account.name.c_str());
		else
			rclog.out("%s attempted to set the server options.", account.name.c_str());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to change the server options.");
		return HandlePacketResult::Handled;
	}

	auto& settings = m_server->getSettings();
	CString options = pPacket.readString("");
	options.guntokenizeI();

	// If they don't have the modify staff account right, prevent them from changing admin-only options.
	if (!account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		std::vector<CString> newOptions = options.tokenize("\n");
		options.clear();
		for (auto& newOption : newOptions)
		{
			CString name = newOption.subString(0, newOption.find("="));
			name.trimI();

			// See if this command is an admin command.
			bool isAdmin = false;
			for (const auto& j : AdminServerOptions)
				if (name == j) isAdmin = true;

			// If it is an admin command, replace it with the current value.
			if (isAdmin)
				newOption = CString() << name << " = " << settings.getStr(name);

			// Add this line back into options.
			options << newOption << "\n";
		}
	}

	// Save settings.
	settings.loadSettings(options, true, true);

	// Reload settings.
	m_server->loadSettings();
	m_server->loadMaps();
	rclog.out("%s has updated the server options.\n", account.name.c_str());

	// Send RC Information
	CString outPacket = CString() >> (char)PLO_RC_CHAT << account.name << " has updated the server options.";
	auto& playerList = m_server->getPlayerList();
	for (auto& [pid, player] : playerList)
	{
		if (player->getType() & PLTYPE_ANYRC)
		{
			player->sendPacket(outPacket);
#ifdef V8NPCSERVER
			if (account.hasRight(PLPERM_NPCCONTROL))
				player->sendNCAddr();
#endif
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FOLDERCONFIGGET(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to get the folder config.", account.name.c_str());
		return HandlePacketResult::Handled;
	}

	CString foldersConfig;
	foldersConfig.load(m_server->getServerPath() << "config/foldersconfig.txt");
	foldersConfig.removeAllI("\r");

	sendPacket(CString() >> (char)PLO_RC_FOLDERCONFIGGET << foldersConfig.gtokenize());
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FOLDERCONFIGSET(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_SETFOLDEROPTIONS))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set the folder config.", account.name.c_str());
		else
			rclog.out("%s attempted to set the folder config.", account.name.c_str());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to change the folder config.");
		return HandlePacketResult::Handled;
	}

	// Save the folder config back to disk
	CString folders = pPacket.readString("");
	folders.guntokenizeI();
	folders.replaceAllI("\n", "\r\n");
	folders.save(m_server->getServerPath() << "config/foldersconfig.txt");

	// Update file system.
	m_server->loadFileSystem();

	rclog.out("%s updated the folder config.\n", account.name.c_str());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " updated the folder config.");
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_RESPAWNSET(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_HORSELIFESET(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_APINCREMENTSET(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_BADDYRESPAWNSET(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERPROPSGET(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERPROPSSET(CString& pPacket)
{
	// Deprecated?

	auto p = m_server->getPlayer(pPacket.readGUShort(), PLTYPE_ANYPLAYER);
	if (p == nullptr) return HandlePacketResult::Handled;

	if (isClient() || (p->account.name != account.name && !account.hasRight(PLPERM_SETATTRIBUTES)) || (p->account.name == account.name && !account.hasRight(PLPERM_SETSELFATTRIBUTES)))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set a player's properties.", account.name.c_str());
		else
			rclog.out("%s attempted to set a player's properties.", account.name.c_str());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to set the properties of " << p->account.name);
		return HandlePacketResult::Handled;
	}

	p->setPropsRC(pPacket, this);
	m_server->getAccountLoader().saveAccount(p->account);

	rclog.out("%s set the attributes of player %s\n", account.name.c_str(), p->account.name.c_str());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " set the attributes of player " << p->account.name);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_DISCONNECTPLAYER(CString& pPacket)
{
	auto p = m_server->getPlayer(pPacket.readGUShort(), PLTYPE_ANYPLAYER);
	if (p == nullptr) return HandlePacketResult::Handled;

	if (isClient() || !account.hasRight(PLPERM_DISCONNECT))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to disconnect %s.\n", account.name.c_str(), p->account.name.c_str());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to disconnect players.");
		return HandlePacketResult::Handled;
	}

	CString reason = pPacket.readString("");
	if (!reason.isEmpty())
		rclog.out("%s disconnected %s: %s\n", account.name.c_str(), p->account.name.c_str(), reason.text());
	else
		rclog.out("%s disconnected %s.\n", account.name.c_str(), p->account.name.c_str());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " disconnected " << p->account.name);

	CString disconnectMessage = CString() << "One of the server administrators, " << account.name << ", has disconnected you";
	if (!reason.isEmpty())
		disconnectMessage << " for the following reason: " << reason;
	else
		disconnectMessage << ".";
	p->sendPacket(CString() >> (char)PLO_DISCMESSAGE << disconnectMessage);
	m_server->deletePlayer(p);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_UPDATELEVELS(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_UPDATELEVEL))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to update levels.\n", account.name.c_str());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to update levels.");
		return HandlePacketResult::Handled;
	}

	unsigned short levelCount = pPacket.readGUShort();
	for (int i = 0; i < levelCount; ++i)
	{
		auto level = m_server->getLevel(pPacket.readChars(pPacket.readGUChar()).toString());
		if (level) level->reload();
	}
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ADMINMESSAGE(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_ADMINMSG))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to send an admin message.\n", account.name.c_str());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to send an admin message.");
		return HandlePacketResult::Handled;
	}

	m_server->sendPacketToAll(CString() >> (char)PLO_RC_ADMINMESSAGE << "Admin " << account.name << ":\xa7" << pPacket.readString(""), { m_id });
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PRIVADMINMESSAGE(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_ADMINMSG))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to send an admin message.\n", account.name.c_str());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to send an admin message.");
		return HandlePacketResult::Handled;
	}

	auto p = m_server->getPlayer(pPacket.readGUShort(), PLTYPE_ANYPLAYER);
	if (p == nullptr) return HandlePacketResult::Handled;

	p->sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE << "Admin " << account.name << ":\xa7" << pPacket.readString(""));
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_LISTRCS(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_DISCONNECTRC(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_APPLYREASON(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_SERVERFLAGSGET(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to view the server flags.\n", account.name.c_str());
		return HandlePacketResult::Handled;
	}
	CString ret;
	ret >> (char)PLO_RC_SERVERFLAGSGET >> (short)m_server->getServerFlags().size();
	for (const auto& [flag, value] : m_server->getServerFlags())
	{
		CString flagString = CString() << flag << "=" << value;
		ret >> (char)flagString.length() << flagString;
	}
	sendPacket(ret);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_SERVERFLAGSSET(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_SETSERVERFLAGS))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set the server flags.\n", account.name.c_str());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set the server flags.");
		return HandlePacketResult::Handled;
	}

	unsigned short count = pPacket.readGUShort();
	auto& serverFlags = m_server->getServerFlags();

	// Save server flags.
	std::unordered_map<std::string, CString> oldFlags = serverFlags;

	// Delete server flags.
	serverFlags.clear();

	// Assemble the new server flags.
	for (unsigned int i = 0; i < count; ++i)
		m_server->setFlag(pPacket.readChars(pPacket.readGUChar()), false);

	// Send flag changes to all players.
	for (auto i = serverFlags.begin(); i != serverFlags.end(); ++i)
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
			else
				++j;
		}

		// If we didn't find a match, this is either a new flag, or a changed flag.
		if (!found)
		{
			if (i->second.isEmpty())
				m_server->sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGSET << i->first);
			else
				m_server->sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGSET << i->first << "=" << i->second);
		}
	}

	// If any flags were deleted, tell that to the players now.
	for (auto i = oldFlags.begin(); i != oldFlags.end(); ++i)
		m_server->sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGDEL << i->first);

	rclog.out("%s has updated the server flags.\n", account.name.c_str());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has updated the server flags.");
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ACCOUNTADD(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to add a new account.\n", account.name.c_str());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to create new accounts.");
		return HandlePacketResult::Handled;
	}

	std::string acc = pPacket.readChars(pPacket.readGUChar()).toString();
	std::string pass = pPacket.readChars(pPacket.readGUChar()).toString();
	std::string email = pPacket.readChars(pPacket.readGUChar()).toString();
	bool banned = (pPacket.readGUChar() != 0);
	bool onlyLoad = (pPacket.readGUChar() != 0);
	pPacket.readGUChar(); // Admin level, deprecated.

	Account newAccount;
	m_server->getAccountLoader().loadAccount(acc, newAccount);
	newAccount.banned = banned;
	newAccount.email = email;
	newAccount.loadOnly = onlyLoad;
	m_server->getAccountLoader().saveAccount(newAccount);

	rclog.out("%s has created a new account: %s\n", account.name.c_str(), acc.c_str());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has created a new account: " << acc);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ACCOUNTDEL(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to delete an account.\n", account.name.c_str());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to delete accounts.");
		return HandlePacketResult::Handled;
	}

	// Get the account.
	// Prevent the defaultaccount from being deleted.
	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);
	if (m_server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
		return HandlePacketResult::Handled;

	if (acc == "defaultaccount")
	{
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not allowed to delete the default account.");
		return HandlePacketResult::Handled;
	}

	// See if the account exists.
	CString accfile = CString(acc) << ".txt";
	CString accpath = m_server->getAccountsFileSystem()->find(accfile);
	if (accpath.isEmpty()) return HandlePacketResult::Handled;

	// Remove the account from the file system.
	m_server->getAccountsFileSystem()->removeFile(accfile);

	// Delete the file now.
	remove(accpath.text());
	rclog.out("%s has deleted the account: %s\n", account.name.c_str(), acc.text());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has deleted the account: " << acc);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ACCOUNTLISTGET(CString& pPacket)
{
	using namespace graal::utilities;

	if (isClient())
	{
		rclog.out("[Hack] %s attempted to view the account listing.\n", account.name.c_str());
		return HandlePacketResult::Handled;
	}

	CString name = pPacket.readChars(pPacket.readGUChar());
	std::string conditions = pPacket.readChars(pPacket.readGUChar()).toString();

	// Fix up name searching.
	name.replaceAllI("%", "*");
	if (name.length() == 0)
		name = "*";

	// Start our packet.
	CString ret;
	ret >> (char)PLO_RC_ACCOUNTLISTGET;

	// Search through all the accounts.
	FileSystem* fs = m_server->getAccountsFileSystem();
	for (std::map<CString, CString>::iterator i = fs->getFileList().begin(); i != fs->getFileList().end(); ++i)
	{
		CString acc = removeExtension(i->first);
		if (acc.isEmpty()) continue;
		if (!acc.match(name)) continue;
		if (conditions.length() != 0)
		{
			if (m_server->getAccountLoader().checkSearchConditions(i->second.toStringView(), string::splitHard(conditions, std::string_view(","))))
				ret >> (char)acc.length() << acc;
		}
		else
			ret >> (char)acc.length() << acc;
	}

	sendPacket(ret);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERPROPSGET2(CString& pPacket)
{
	auto p = m_server->getPlayer(pPacket.readGUShort(), PLTYPE_ANYPLAYER | PLTYPE_NPCSERVER);
	if (p == nullptr) return HandlePacketResult::Handled;

	if (isClient() || !account.hasRight(PLPERM_VIEWATTRIBUTES))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to view the props of player %s.\n", account.name.c_str(), p->account.name.c_str());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to view player props.");
		return HandlePacketResult::Handled;
	}

	sendPacket(CString() >> (char)PLO_RC_PLAYERPROPSGET >> (short)p->getId() << p->getPropsRC());
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERPROPSGET3(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (m_server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	if (isClient() || !account.hasRight(PLPERM_VIEWATTRIBUTES))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to view the props of player %s.\n", account.name.c_str(), p->account.name.c_str());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to view player props.");
		return HandlePacketResult::Handled;
	}

	sendPacket(CString() >> (char)PLO_RC_PLAYERPROPSGET >> (short)p->getId() << p->getPropsRC());
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERPROPSRESET(CString& pPacket)
{
	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !account.hasRight(PLPERM_RESETATTRIBUTES))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to reset the account: %s\n", account.name.c_str(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to reset accounts.\n");
		return HandlePacketResult::Handled;
	}

	// Get the player.  Create a new player if they are offline.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (m_server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	// Save RC stuff.
	std::vector<std::string> adminip = p->account.adminIpRange;
	uint32_t rights = p->account.adminRights;
	std::vector<std::string> folders{ std::from_range, p->account.folderList };

	// Reset the player.
	m_server->getAccountLoader().loadAccount("defaultaccount", p->account);
	p->account.name = acc.toStringView();
	m_server->getAccountLoader().saveAccount(p->account);

	// Add the RC stuff back in.
	p->account.adminIpRange = adminip;
	p->account.adminRights = rights;
	p->account.folderList = folders;

	// Save the account.
	m_server->getAccountLoader().saveAccount(p->account);

	// If the player is online, boot him from the server.
	if (p->getId() != 0)
	{
		p->sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Your account was reset by " << account.name);
		p->setLoaded(false); // Don't save the account when the player quits.
		m_server->deletePlayer(p);
	}

	// Log it.
	rclog.out("%s has reset the attributes of account: %s\n", account.name.c_str(), acc.text());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has reset the attributes of account: " << acc);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERPROPSSET2(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (m_server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	if (isClient() || (p->account.name != account.name && !account.hasRight(PLPERM_SETATTRIBUTES)) || (p->account.name == account.name && !account.hasRight(PLPERM_SETSELFATTRIBUTES)))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set a player's properties.", account.name.c_str());
		else
			rclog.out("%s attempted to set a player's properties.", account.name.c_str());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to set the properties of " << p->account.name);
		return HandlePacketResult::Handled;
	}

	// Only people with PLPERM_MODIFYSTAFFACCOUNT can alter the default account.
	if (!account.hasRight(PLPERM_MODIFYSTAFFACCOUNT) && acc == "defaultaccount")
	{
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to modify the default account.");
		return HandlePacketResult::Handled;
	}

	p->setPropsRC(pPacket, this);
	m_server->getAccountLoader().saveAccount(p->account);
	rclog.out("%s set the attributes of player %s\n", account.name.c_str(), p->account.name.c_str());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " set the attributes of player " << p->account.name);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ACCOUNTGET(CString& pPacket)
{
	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient())
	{
		rclog.out("[Hack] %s attempted to view the account: %s\n", account.name.c_str(), acc.text());
		return HandlePacketResult::Handled;
	}

	// Get the player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (m_server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	sendPacket(CString() >> (char)PLO_RC_ACCOUNTGET >> (char)acc.length() << acc >> (char)0                                                      /*>> (char)password_length << password*/
			   >> (char)p->account.email.size() << p->account.email >> (char)(p->account.banned ? 1 : 0) >> (char)(p->account.loadOnly ? 1 : 0) >> (char)0 /*admin level*/
			   >> (char)4 << "main" >> (char)p->account.banLength.size() << p->account.banLength >> (char)p->account.banReason.size() << p->account.banReason);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ACCOUNTSET(CString& pPacket)
{
	using namespace graal::utilities;

	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc.length() == 0) return HandlePacketResult::Handled;
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to edit the account: %s\n", account.name.c_str(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to edit accounts.\n");
		return HandlePacketResult::Handled;
	}

	std::string pass = pPacket.readChars(pPacket.readGUChar()).toString();
	std::string email = pPacket.readChars(pPacket.readGUChar()).toString();
	bool banned = (pPacket.readGUChar() != 0 ? true : false);
	bool loadOnly = (pPacket.readGUChar() != 0 ? true : false);
	pPacket.readGUChar();                    // admin level
	pPacket.readChars(pPacket.readGUChar()); // world
	std::string banreason = pPacket.readChars(pPacket.readGUChar()).toString();

	// Get player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (m_server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	// Set the new account stuff.
	p->account.email = email;
	p->account.loadOnly = loadOnly;
	if (account.hasRight(PLPERM_BAN))
	{
		p->account.banned = banned;
		p->account.banReason = banreason;
	}
	m_server->getAccountLoader().saveAccount(p->account);

	// If the account is currently on RC, reload it.
	if (auto pRC = m_server->getPlayer(acc, PLTYPE_ANYRC); pRC)
	{
		m_server->getAccountLoader().loadAccount(acc.toStringView(), pRC->account);
	}

	// If the player was just now banned, kick him off the server.
	if (account.hasRight(PLPERM_BAN) && banned && p->getId() != 0)
	{
		auto reason = string::join(string::fromCSV(banreason), std::string_view{ "\r" });

		p->setLoaded(false);
		p->sendPacket(CString() >> (char)PLO_DISCMESSAGE << account.name << " has banned you.  Reason: " << reason);
		m_server->deletePlayer(p);
	}

	rclog.out("%s has modified the account: %s\n", account.name.c_str(), acc.text());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has modified the account: " << acc);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_CHAT(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to send a message to RC.\n", account.name.c_str());
		return HandlePacketResult::Handled;
	}

	CString message = pPacket.readString("");
	if (message.isEmpty()) return HandlePacketResult::Handled;
	std::vector<CString> words = message.tokenize();

#ifdef V8NPCSERVER
	if (isNC())
	{
		// TODO(joey): All RC's with NC support are sending two messages at a time.
		//  Can use this section for npc-server related commands though.
		//m_server->sendToNC(CString(account.character.nickName) << ": " << message);
		return HandlePacketResult::Handled;
	}
#endif

	if (words[0].text()[0] != '/')
	{
		m_server->sendToRC(CString(account.character.nickName) << ": " << message);
		return HandlePacketResult::Handled;
	}
	else
	{
#ifndef NDEBUG
		if (words[0] == "/sendtext")
		{
			sendPacket(CString() >> (char)PLO_SERVERTEXT << message.subString(10) << "\n");
		}
		else
#endif
			if (words[0] == "/help" && words.size() == 1)
			{
				std::vector<CString> commands = CString::loadToken(m_server->getServerPath() << "config/rchelp.txt", "\n", true);
				for (auto& command : commands)
					sendPacket(CString() >> (char)PLO_RC_CHAT << command);
			}
			else if (words[0] == "/version" && words.size() == 1)
			{
				sendPacket(CString() >> (char)PLO_RC_CHAT << APP_NAME << " version: " << APP_VERSION);
			}
			else if (words[0] == "/credits" && words.size() == 1)
			{
				sendPacket(CString() >> (char)PLO_RC_CHAT << "Programmed by " << APP_CREDITS);
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
			else if (words[0] == "/openaccess" && words.size() != 1)
			{
				message.setRead(0);
				message.readString(" ");

				CString acc = message.readString("");
				auto pl = m_server->getPlayer(acc, PLTYPE_ANYPLAYER);
				if (pl)
					sendPacket(CString() >> (char)PLO_SERVERTEXT << "GraalEngine,lister,ban," << pl->account.name << "," << std::to_string(pl->getDeviceId()));
				else
				{
					// TODO: player not logged in, load from offline?
				}
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
				m_server->loadServerMessage();
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " refreshed the server message.");
				rclog.out("%s refreshed the server message.\n", account.name.c_str());
			}

			else if (words[0] == "/refreshfilesystem" && words.size() == 1)
			{
				m_server->loadFileSystem();
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " refreshed the server file list.");
				rclog.out("%s refreshed the server file list.\n", account.name.c_str());
			}
			else if (words[0] == "/updatelevel" && words.size() != 1 && account.hasRight(PLPERM_UPDATELEVEL))
			{
				std::vector<CString> levels = words[1].tokenize(",");
				for (auto& l : levels)
				{
					auto level = m_server->getLevel(l.toString());
					if (level)
					{
						m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " updated level: " << level->getLevelName());
						rclog.out("%s updated level: %s\n", account.name.c_str(), level->getLevelName().text());
						level->reload();
					}
				}
			}
			else if (words[0] == "/updatelevelall" && words.size() == 1 && account.hasRight(PLPERM_UPDATELEVEL))
			{
				rclog.out("%s updated all the levels", account.name.c_str());
				int count = 0;
				auto& levels = m_server->getLevelList();
				for (auto& level : levels)
				{
					level->reload();
					++count;
				}
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " updated all the levels (" << CString((int)count) << " levels updated).");
				rclog.out(" (%d levels updated).\n", count);
			}
			else if (words[0] == "/restartserver" && words.size() == 1 && account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
			{
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " restarted the server.");
				rclog.out("%s restarted the server.\n", account.name.c_str());
				m_server->restart();
			}
			else if (words[0] == "/reloadserver" && words.size() == 1 && account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
			{
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " reloaded the server configuration files.");
				rclog.out("%s reloaded the server configuration files.\n", account.name.c_str());
				m_server->loadConfigFiles();
			}
			else if (words[0] == "/updateserverhq" && words.size() == 1 && account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
			{
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " sent ServerHQ updates.");
				rclog.out("%s sent ServerHQ updates.\n", account.name.c_str());
				m_server->loadAdminSettings();
				m_server->getServerList().sendServerHQ();
			}
			else if (words[0] == "/serveruptime" && words.size() == 1)
			{
				auto time_units = utilities::TimeUnits(std::time(nullptr) - m_server->getServerStartTime());

				constexpr auto format_time_fn = [](std::string& m, const uint64_t t, const char* fmtStr)
					{
						if (t > 0)
						{
							m.append(fmt::format(" {} {}", t, fmtStr));
							if (t > 1)
								m.append("s");
						}
					};

				std::string msg;
				format_time_fn(msg, time_units.days, "day");
				format_time_fn(msg, time_units.hours, "hour");
				format_time_fn(msg, time_units.minutes, "minute");
				if (time_units.days == 0)
					format_time_fn(msg, time_units.seconds, "second");

				sendPacket(CString() >> (char)PLO_RC_CHAT << "Server Uptime:" << msg);
			}
			else if (words[0] == "/reloadwordfilter" && words.size() == 1)
			{
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " reloaded the word filter.");
				rclog.out("%s reloaded the word filter.\n", account.name.c_str());
				m_server->loadWordFilter();
			}
			else if (words[0] == "/reloadipbans" && words.size() == 1)
			{
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " reloaded the ip bans.");
				rclog.out("%s reloaded the ip bans.\n", account.name.c_str());
				m_server->loadIPBans();
			}
			else if (words[0] == "/reloadweapons" && words.size() == 1)
			{
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " reloaded the weapons.");
				rclog.out("%s reloaded the weapons.\n", account.name.c_str());
				m_server->loadWeapons(true);
			}
#ifdef V8NPCSERVER
			else if (words[0] == "/savenpcs" && words.size() == 1)
			{
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " saved npc to disk.");
				nclog.out("%s saved the npcs to disk.\n", account.name.c_str());
				m_server->saveNpcs();
			}
			else if (words[0] == "/stats" && words.size() == 1)
			{
				auto npcStats = m_server->calculateNpcStats();

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
			else if (words[0] == "/find" && words.size() > 1)
			{
				std::map<CString, CString> found;

				// Assemble the search string.
				CString search(words[1]);
				for (unsigned int i = 2; i < words.size(); ++i)
					search << " " << words[i];

				// Search for the files.
				for (unsigned int i = 0; i < FS_COUNT; ++i)
				{
					auto& fileList = m_server->getFileSystem(i)->getFileList();
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
							found[i->second.removeAll(m_server->getServerPath())] = fs;
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

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_WARPPLAYER(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_WARPTOPLAYER))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to warp a player.\n", account.name.c_str());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to warp players.\n");
		return HandlePacketResult::Handled;
	}

	auto p = m_server->getPlayer<PlayerClient>(pPacket.readGUShort(), PLTYPE_ANYPLAYER);
	if (p == nullptr) return HandlePacketResult::Handled;

	float loc[2] = { (float)(pPacket.readGChar()) / 2.0f, (float)(pPacket.readGChar()) / 2.0f };
	CString wLevel = pPacket.readString("");
	p->warp(wLevel, loc[0], loc[1]);

	rclog.out("%s has warped %s to %s (%.2f, %.2f)\n", account.name.c_str(), p->account.name.c_str(), wLevel.text(), loc[0], loc[1]);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERRIGHTSGET(CString& pPacket)
{
	using namespace graal::utilities;

	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || (acc != account.name && !account.hasRight(PLPERM_SETRIGHTS)))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to get the rights of %s\n", account.name.c_str(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to view that player's rights.");
		return HandlePacketResult::Handled;
	}

	// Get player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (m_server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	// Get the folder list.
	auto folders = string::toCSV(p->account.folderList);

	// Send the packet.
	auto adminIp = string::join(p->account.adminIpRange);
	sendPacket(CString() >> (char)PLO_RC_PLAYERRIGHTSGET >> (char)acc.length() << acc >> (long long)p->account.adminRights >> (char)adminIp.size() << adminIp >> (short)folders.length() << folders);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERRIGHTSSET(CString& pPacket)
{
	using namespace graal::utilities;

	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !account.hasRight(PLPERM_SETRIGHTS))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set the rights of %s\n", account.name.c_str(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set player rights.");
		return HandlePacketResult::Handled;
	}

	// Get player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (m_server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	// Don't allow RCs to give rights that they don't have.
	// Only affect people who don't have PLPERM_MODIFYSTAFFACCOUNT.
	int n_adminRights = (int)pPacket.readGUInt5();
	if (!account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		for (int i = 0; i < 20; ++i)
		{
			if ((account.adminRights & (1 << i)) == 0)
				n_adminRights &= ~(1 << i);
		}
	}

	// Don't allow you to remove your own PLPERM_MODIFYSTAFFACCOUNT or PLPERM_SETRIGHTS.
	if (p.get() == this)
	{
		if ((n_adminRights & PLPERM_MODIFYSTAFFACCOUNT) == 0)
			n_adminRights |= PLPERM_MODIFYSTAFFACCOUNT;
		if ((n_adminRights & PLPERM_SETRIGHTS) == 0)
			n_adminRights |= PLPERM_SETRIGHTS;
	}

	int changed_rights = account.adminRights ^ n_adminRights;
	p->account.adminRights = n_adminRights;

	std::string adminIp = pPacket.readChars(pPacket.readGUChar()).toString();
	p->account.adminIpRange.clear();
	std::ranges::copy(string::split(adminIp, std::string_view{ "," }) | string::as_string, std::back_inserter(p->account.adminIpRange));

	// Untokenize and load the directories.
	std::vector<std::string> folders = string::fromCSV(pPacket.readChars(pPacket.readGUShort()).toString());

	// Remove any invalid directories.
	for (auto i = folders.begin(); i != folders.end();)
	{
		if ((*i).find(":") != -1 || (*i).find("..") != -1 || (*i).find(" /*") != -1)
			i = folders.erase(i);
		else
			++i;
	}

	// Assign the directories to the account.
	p->account.folderList = folders;

	// Save the account.
	m_server->getAccountLoader().saveAccount(p->account);

	// If the account is currently on RC, reload it.
	if (auto pRC = m_server->getPlayer<PlayerRC>(acc, PLTYPE_ANYRC); pRC)
	{
		std::string nickname = pRC->account.character.nickName;
		m_server->getAccountLoader().loadAccount(acc.toStringView(), pRC->account);
		pRC->account.character.nickName = nickname;

#ifdef V8NPCSERVER
		if (changed_rights & PLPERM_NPCCONTROL)
		{
			if (!(n_adminRights & PLPERM_NPCCONTROL))
			{
				if (auto pNC = m_server->getPlayer(acc, PLTYPE_ANYNC); pNC)
					pNC->disconnect();
			}
			else
				pRC->sendNCAddr();
		}
#endif

		// If they are using the File Browser, reload it.
		if (pRC->isUsingFileBrowser())
			pRC->msgPLI_RC_FILEBROWSER_START(CString() << "");
	}

	rclog.out("%s has set the rights of %s\n", account.name.c_str(), acc.text());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has set the rights of " << acc);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERCOMMENTSGET(CString& pPacket)
{
	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient())
	{
		rclog.out("[Hack] %s attempted to get the comments of %s\n", account.name.c_str(), acc.text());
		return HandlePacketResult::Handled;
	}

	// Get player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (m_server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	sendPacket(CString() >> (char)PLO_RC_PLAYERCOMMENTSGET >> (char)acc.length() << acc << p->account.comments);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERCOMMENTSSET(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !account.hasRight(PLPERM_SETCOMMENTS))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set the comments of %s\n", account.name.c_str(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set player comments.");
		return HandlePacketResult::Handled;
	}

	// Get player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (m_server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	CString comment = pPacket.readString("");
	p->account.comments = comment.toStringView();
	m_server->getAccountLoader().saveAccount(p->account);

	// If the account is currently on RC, reload it.
	if (auto pRC = m_server->getPlayer(acc, PLTYPE_ANYRC); pRC)
	{
		m_server->getAccountLoader().loadAccount(acc.toStringView(), pRC->account);
	}

	rclog.out("%s has set the comments of %s\n", account.name.c_str(), acc.text());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has set the comments of " << acc);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERBANGET(CString& pPacket)
{
	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient())
	{
		rclog.out("[Hack] %s attempted to view the ban of %s\n", account.name.c_str(), acc.text());
		return HandlePacketResult::Handled;
	}

	// Get player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (m_server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	sendPacket(CString() >> (char)PLO_RC_PLAYERBANGET >> (char)acc.length() << acc >> (char)(p->account.banned ? 1 : 0) << p->account.banReason);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERBANSET(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !account.hasRight(PLPERM_BAN))
	{
		if (isClient()) rclog.out("[Hack] %s attempted to set the ban of %s\n", account.name.c_str(), acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set player bans.");
		return HandlePacketResult::Handled;
	}

	// Get player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (m_server->getAccountsFileSystem()->findi(CString(acc) << ".txt").isEmpty())
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	bool banned = (pPacket.readGUChar() == 0 ? false : true);
	CString reason = pPacket.readString("");
	p->account.banned = banned;
	p->account.banReason = reason.toStringView();
	m_server->getAccountLoader().saveAccount(p->account);

	// If the account is currently on RC, reload it.
	if (auto pRC = m_server->getPlayer(acc, PLTYPE_ANYRC); pRC)
	{
		m_server->getAccountLoader().loadAccount(acc.toStringView(), pRC->account);
	}

	// If the player was just now banned, kick him off the server.
	if (banned && p->getId() != 0)
	{
		p->sendPacket(CString() >> (char)PLO_DISCMESSAGE << account.name << " has banned you.  Reason: " << reason.guntokenize().replaceAll("\n", "\r"));
		m_server->deletePlayer(p);
	}

	rclog.out("%s has set the ban of %s\n", account.name.c_str(), acc.text());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has set the ban of " << acc);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_START(CString& pPacket)
{
	using namespace graal::utilities;

	if (isClient())
	{
		rclog.out("[Hack] %s attempted to open the File Browser.\n", account.name.c_str());
		return HandlePacketResult::Handled;
	}

	// If the player has no folder rights, don't open the File Browser.
	if (account.folderList.size() == 0)
		return HandlePacketResult::Handled;

	// Get folder list to send to the client.
	auto folders = string::toCSV(account.folderList);

	// Send the folder list and the welcome message.
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_DIRLIST << folders);
	if (!m_isFtp) sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Welcome to the File Browser.");

	// Create a folder map.
	std::map<CString, CString> folderMap;
	for (auto i = account.folderList.begin(); i != account.folderList.end(); ++i)
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
	if (folderMap.find(account.lastFolderAccessed) == folderMap.end())
		account.lastFolderAccessed = folderMap.begin()->first.toStringView();

	// Create the file system.
	FileSystem fs;
	fs.addDir(account.lastFolderAccessed);

	// Construct the file list.
	CString files;
	std::vector<CString> wildcards = folderMap[account.lastFolderAccessed].tokenize("\n");
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
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_DIR >> (char)account.lastFolderAccessed.length() << account.lastFolderAccessed << files);
	m_isFtp = true;

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_CD(CString& pPacket)
{
	using namespace graal::utilities;

	if (isClient()) return HandlePacketResult::Handled;

	CString newFolder = pPacket.readString("");
	CString newRights, wildcard;
	newFolder.setRead(0);

	// Create a folder map.
	std::map<CString, CString> folderMap;
	for (auto i = account.folderList.begin(); i != account.folderList.end(); ++i)
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
		return HandlePacketResult::Handled;
	else
		account.lastFolderAccessed = newFolder.toStringView();

	// Create the file system.
	FileSystem fs;
	fs.addDir(account.lastFolderAccessed);

	// Make sure our folder exists.
	CString mkdir_path = m_server->getServerPath();
	auto f = string::splitHard(account.lastFolderAccessed, std::string_view{ "/" });
	for (auto i = f.begin(); i != f.end(); ++i)
	{
		if (i->empty()) continue;
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
	std::vector<CString> wildcards = folderMap[account.lastFolderAccessed].tokenize("\n");
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
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Folder changed to " << account.lastFolderAccessed);
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_DIR >> (char)account.lastFolderAccessed.length() << account.lastFolderAccessed << files);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_END(CString& pPacket)
{
	if (isClient()) return HandlePacketResult::Handled;
	m_isFtp = false;

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_DOWN(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to download a file through the File Browser.\n", account.name.c_str());
		return HandlePacketResult::Handled;
	}

	// Send file.
	CString file = pPacket.readString("");
	CString filepath = m_server->getServerPath() << account.lastFolderAccessed << file;
	CString checkFile = CString() << account.lastFolderAccessed << file;

	// Don't let us download/view important files.
	if (!account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		for (const auto& file : ProtectedFiles)
		{
			if (checkFile == file)
			{
				sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Insufficient rights to download/view " << checkFile);
				return HandlePacketResult::Handled;
			}
		}
	}

	this->sendFile(account.lastFolderAccessed, file);

	rclog.out("%s downloaded file %s\n", account.name.c_str(), file.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Downloaded file " << file);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_UP(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to upload a file through the File Browser.\n", account.name.c_str());
		return HandlePacketResult::Handled;
	}

	CString file = pPacket.readChars(pPacket.readGUChar());
	CString filepath = m_server->getServerPath() << account.lastFolderAccessed;
	CString fileData = pPacket.subString(pPacket.readPos());
	CString checkFile = CString() << account.lastFolderAccessed << file;

	// Check if this is a protected file.
	bool isProtected = false;
	int fileID = -1;
	for (int i = 0; i < ImportantFiles.size(); ++i)
	{
		if (checkFile == ImportantFiles[i])
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
		hasPermission = account.hasRight(PLPERM_MODIFYSTAFFACCOUNT);
		if (!hasPermission)
		{
			if (fileID < ImportantFileRights.size())
				hasPermission = account.hasRight(ImportantFileRights[fileID]);
		}
	}

	// Don't let us upload/overwrite important files.
	if (isProtected && !hasPermission)
	{
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Insufficent rights to upload " << checkFile);
		return HandlePacketResult::Handled;
	}

	// See if we are uploading a large file or not.
	if (m_rcLargeFiles.find(file) == m_rcLargeFiles.end())
	{
		// Normal file. Save it and display our message.
		fileData.save(filepath << file);

		rclog.out("%s uploaded file %s\n", account.name.c_str(), file.text());
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Uploaded file " << file);

		// Update file.
		updateFile(this, m_server, account.lastFolderAccessed, file);
	}
	else
	{
		// Large file.  Store the data in memory.
		m_rcLargeFiles[file] << fileData;
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_MOVE(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to move a file through the File Browser.\n", account.name.c_str());
		return HandlePacketResult::Handled;
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
	source << account.lastFolderAccessed << file;

	// Don't let us move important files.
	for (const auto& file : ImportantFiles)
	{
		if (source == file)
		{
			sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Not allowed to move file " << source);
			return HandlePacketResult::Handled;
		}
	}

	rclog.out("%s moved file %s to %s\n", account.name.c_str(), source.text(), destination.text());

	// Add working directory.
	source = CString(m_server->getServerPath()) << source;
	destination = CString(m_server->getServerPath()) << destination;
	FileSystem::fixPathSeparators(source);
	FileSystem::fixPathSeparators(destination);

	// Save the new file now.
	CString temp;
	temp.load(source);
	if (temp.save(destination) == false)
		return HandlePacketResult::Handled;

	// Remove the old file.
#if defined(WIN32) || defined(WIN64)
	wchar_t* wcstr = 0;

	// Determine if the filename is UTF-8 encoded.
	int wcsize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, source.text(), source.length(), 0, 0);
	if (wcsize != 0)
	{
		wcstr = new wchar_t[wcsize + 1];
		memset((void*)wcstr, 0, (wcsize + 1) * sizeof(wchar_t));
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

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_DELETE(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to delete a file through the File Browser.\n", account.name.c_str());
		return HandlePacketResult::Handled;
	}

	CString file = pPacket.readString("");
	CString filePath = m_server->getServerPath() << account.lastFolderAccessed << file;
	CString checkFile = CString() << account.lastFolderAccessed << file;
	FileSystem::fixPathSeparators(filePath);

	// Don't let us delete important files.
	for (const auto& file : ImportantFiles)
	{
		if (checkFile == file)
		{
			sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Not allowed to delete file " << checkFile);
			return HandlePacketResult::Handled;
		}
	}

#if defined(WIN32) || defined(WIN64)
	wchar_t* wcstr = 0;

	// Determine if the filename is UTF-8 encoded.
	int wcsize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filePath.text(), filePath.length(), 0, 0);
	if (wcsize != 0)
	{
		wcstr = new wchar_t[wcsize + 1];
		memset((void*)wcstr, 0, (wcsize + 1) * sizeof(wchar_t));
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

	rclog.out("%s deleted file %s\n", account.name.c_str(), file.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Deleted file " << file);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_RENAME(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to rename a file through the File Browser.\n", account.name.c_str());
		return HandlePacketResult::Handled;
	}

	CString f1 = pPacket.readChars(pPacket.readGUChar());
	CString f2 = pPacket.readChars(pPacket.readGUChar());
	CString f1path = m_server->getServerPath() << account.lastFolderAccessed << f1;
	CString f2path = m_server->getServerPath() << account.lastFolderAccessed << f2;
	CString checkFile1 = CString() << account.lastFolderAccessed << f1;
	CString checkFile2 = CString() << account.lastFolderAccessed << f2;
	FileSystem::fixPathSeparators(f1path);
	FileSystem::fixPathSeparators(f2path);

	// Don't let us rename/overwrite important files.
	for (const auto& file : ImportantFiles)
	{
		if (checkFile1 == file)
		{
			sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Not allowed to rename/overwrite file " << checkFile1 << " or " << checkFile2);
			return HandlePacketResult::Handled;
		}
	}

	// Renaming our logs?  First, we need to close them.
	if (account.lastFolderAccessed == "logs/")
	{
		if (f1 == "rclog.txt") rclog.close();
		else if (f1 == "serverlog.txt")
			serverlog.close();
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
		memset((void*)f1_wcstr, 0, (f1_wcsize + 1) * sizeof(wchar_t));
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
		memset((void*)f2_wcstr, 0, (f2_wcsize + 1) * sizeof(wchar_t));
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
	if (account.lastFolderAccessed == "logs/")
	{
		if (f1 == "rclog.txt") rclog.open();
		else if (f1 == "serverlog.txt")
			serverlog.open();
	}

	rclog.out("%s renamed file %s to %s\n", account.name.c_str(), f1.text(), f2.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Renamed file " << f1 << " to " << f2);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_LARGEFILESTART(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s is attempting to upload a file through the File Browser.\n", account.name.c_str());
		return HandlePacketResult::Handled;
	}

	CString file = pPacket.readString("");
	m_rcLargeFiles[file] = CString();

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_LARGEFILEEND(CString& pPacket)
{
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to upload a file through the File Browser.\n", account.name.c_str());
		return HandlePacketResult::Handled;
	}

	CString file = pPacket.readString("");
	CString filepath = m_server->getServerPath() << account.lastFolderAccessed << file;

	// Save the file.
	m_rcLargeFiles[file].save(filepath);

	// Remove the data from memory.
	for (std::map<CString, CString>::iterator i = m_rcLargeFiles.begin(); i != m_rcLargeFiles.end(); ++i)
	{
		if (i->first == file)
		{
			m_rcLargeFiles.erase(i);
			break;
		}
	}

	// Update file.
	updateFile(this, m_server, account.lastFolderAccessed, file);

	rclog.out("%s uploaded large file %s\n", account.name.c_str(), file.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Uploaded large file " << file);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FOLDERDELETE(CString& pPacket)
{
	CString folder = pPacket.readString("");
	CString folderpath = m_server->getServerPath() << folder;
	FileSystem::fixPathSeparators(folderpath);
	folderpath.removeI(folderpath.length() - 1);
	if (isClient())
	{
		rclog.out("[Hack] %s attempted to delete a folder through the File Browser: %s\n", account.name.c_str(), folder.text());
		return HandlePacketResult::Handled;
	}

	// Try to remove folder.
	if (rmdir(folderpath.text()))
	{
		perror("Error removing folder");
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Error removing " << folder << ".  Folder may not exist or may not be empty.");
		return HandlePacketResult::Handled;
	}

	rclog.out("%s removed folder %s\n", account.name.c_str(), folder.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Folder " << folder << " has been removed.\n");
	msgPLI_RC_FILEBROWSER_START(CString() << "");

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_NPCSERVERQUERY(CString& pPacket)
{
#ifdef V8NPCSERVER
	// Read Packet Data
	unsigned short pid = pPacket.readGUShort();
	CString message = pPacket.readString("");

	// Enact upon the message.
	if (message == "location")
		sendNCAddr();
#endif

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_UNKNOWN162(CString& pPacket)
{
	// Stub.
	return HandlePacketResult::Handled;
}
