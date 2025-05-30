#include <sys/stat.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#define mkdir _mkdir
#define rmdir _rmdir
#else
#include <unistd.h>
#endif

#include <format>

#include <CString.h>

#include "IConfig.h"

#include "Server.h"
#include "level/Level.h"
#include "player/PlayerClient.h"
#include "player/PlayerRC.h"
#include "utilities/Log.h"
#include "utilities/StringUtils.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

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
		std::vector<CString> foldersConfig = CString::loadToken("config/foldersconfig.txt", "", true);
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
					//printf("adding {} to {}", file.text(), type.text());
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
		log::printLine(log::rc, "[Hack] {} attempted to view the server options.", account.name);
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
		if (isClient())
			log::printLine(log::rc, "[Hack] {} attempted to set the server options.", account.name);
		else
			log::printLine(log::rc, "{} attempted to set the server options.", account.name);

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
	log::printLine(log::rc, "{} has updated the server options.", account.name);

	// Send RC Information
	CString outPacket = CString() >> (char)PLO_RC_CHAT << account.name << " has updated the server options.";
	auto& playerList = m_server->getPlayerList();
	for (auto& [pid, player] : playerList)
	{
		if (player->getType() & PLTYPE_ANYRC)
		{
			player->sendPacket(outPacket);

			// TODO(NPCSERVER): Send the NC address information.
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FOLDERCONFIGGET(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to get the folder config.", account.name);
		return HandlePacketResult::Handled;
	}

	CString foldersConfig;
	foldersConfig.load("config/foldersconfig.txt");
	foldersConfig.removeAllI("\r");

	sendPacket(CString() >> (char)PLO_RC_FOLDERCONFIGGET << foldersConfig.gtokenize());
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FOLDERCONFIGSET(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_SETFOLDEROPTIONS))
	{
		if (isClient())
			log::printLine(log::rc, "[Hack] {} attempted to set the folder config.", account.name);
		else
			log::printLine(log::rc, "{} attempted to set the folder config.", account.name);

		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to change the folder config.");
		return HandlePacketResult::Handled;
	}

	// Save the folder config back to disk
	CString folders = pPacket.readString("");
	folders.guntokenizeI();
	folders.replaceAllI("", "\r\n");
	folders.save("config/foldersconfig.txt");

	// Update file system.
	m_server->loadFileSystem();

	log::printLine(log::rc, "{} updated the folder config.", account.name);
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
		if (isClient())
			log::printLine(log::rc, "[Hack] {} attempted to set a player's properties.", account.name);
		else
			log::printLine(log::rc, "{} attempted to set a player's properties.", account.name);

		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to set the properties of " << p->account.name);
		return HandlePacketResult::Handled;
	}

	p->setPropsFromRCPacket(pPacket, this);
	m_server->getAccountLoader().saveAccount(p->account);

	log::printLine(log::rc, "{} set the attributes of player {}", account.name, p->account.name);
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " set the attributes of player " << p->account.name);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_DISCONNECTPLAYER(CString& pPacket)
{
	auto p = m_server->getPlayer(pPacket.readGUShort(), PLTYPE_ANYPLAYER);
	if (p == nullptr) return HandlePacketResult::Handled;

	if (isClient() || !account.hasRight(PLPERM_DISCONNECT))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to disconnect {}.", account.name, p->account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to disconnect players.");
		return HandlePacketResult::Handled;
	}

	CString reason = pPacket.readString("");
	if (!reason.isEmpty())
		log::printLine(log::rc, "{} disconnected {}: {}", account.name, p->account.name, reason.text());
	else
		log::printLine(log::rc, "{} disconnected {}.", account.name, p->account.name);

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
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to update levels.", account.name);
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
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to send an admin message.", account.name);
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
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to send an admin message.", account.name);
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
		log::printLine(log::rc, "[Hack] {} attempted to view the server flags.", account.name);
		return HandlePacketResult::Handled;
	}
	CString ret;
	ret >> (char)PLO_RC_SERVERFLAGSGET >> (short)m_server->Flags.container.size();
	for (const auto& [flag, value] : m_server->Flags.container)
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
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to set the server flags.", account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set the server flags.");
		return HandlePacketResult::Handled;
	}

	unsigned short count = pPacket.readGUShort();
	auto& serverFlags = m_server->Flags.container;

	// Save server flags.
	auto oldFlags = serverFlags;

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
			if (i->second.empty())
				m_server->sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGSET << i->first);
			else
				m_server->sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGSET << i->first << "=" << i->second);
		}
	}

	// If any flags were deleted, tell that to the players now.
	for (auto i = oldFlags.begin(); i != oldFlags.end(); ++i)
		m_server->sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_FLAGDEL << i->first);

	log::printLine(log::rc, "{} has updated the server flags.", account.name);
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has updated the server flags.");
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ACCOUNTADD(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to add a new account.", account.name);
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

	log::printLine(log::rc, "{} has created a new account: {}", account.name, acc.c_str());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has created a new account: " << acc);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ACCOUNTDEL(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to delete an account.", account.name);
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
	log::printLine(log::rc, "{} has deleted the account: {}", account.name, acc.text());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has deleted the account: " << acc);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ACCOUNTLISTGET(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to view the account listing.", account.name);
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
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to view the props of player {}.", account.name, p->account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to view player props.");
		return HandlePacketResult::Handled;
	}

	sendPacket(CString() >> (char)PLO_RC_PLAYERPROPSGET >> (short)p->getId() << p->getPropsForRCPacket());
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
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to view the props of player {}.", account.name, p->account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to view player props.");
		return HandlePacketResult::Handled;
	}

	sendPacket(CString() >> (char)PLO_RC_PLAYERPROPSGET >> (short)p->getId() << p->getPropsForRCPacket());
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERPROPSRESET(CString& pPacket)
{
	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !account.hasRight(PLPERM_RESETATTRIBUTES))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to reset the account: {}", account.name, acc.text());
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
	std::vector<std::string> folders;
	std::ranges::copy(p->account.folderList, std::back_inserter(folders));

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
	log::printLine(log::rc, "{} has reset the attributes of account: {}", account.name, acc.text());
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
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to set a player's properties.", account.name);
		else
			log::printLine(log::rc, "{} attempted to set a player's properties.", account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to set the properties of " << p->account.name);
		return HandlePacketResult::Handled;
	}

	// Only people with PLPERM_MODIFYSTAFFACCOUNT can alter the default account.
	if (!account.hasRight(PLPERM_MODIFYSTAFFACCOUNT) && acc == "defaultaccount")
	{
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to modify the default account.");
		return HandlePacketResult::Handled;
	}

	p->setPropsFromRCPacket(pPacket, this);
	m_server->getAccountLoader().saveAccount(p->account);
	log::printLine(log::rc, "{} set the attributes of player {}", account.name, p->account.name);
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
		log::printLine(log::rc, "[Hack] {} attempted to view the account: {}", account.name, acc.text());
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
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc.length() == 0) return HandlePacketResult::Handled;
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to edit the account: {}", account.name, acc.text());
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

	log::printLine(log::rc, "{} has modified the account: {}", account.name, acc.text());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has modified the account: " << acc);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_CHAT(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to send a message to RC.", account.name);
		return HandlePacketResult::Handled;
	}

	if (isNC())
	{
		// TODO(joey): All RC's with NC support are sending two messages at a time.
		//  Can use this section for npc-server related commands though.
		//m_server->sendToNC(CString(account.character.nickName) << ": " << message);
		return HandlePacketResult::Handled;
	}

	CString message = pPacket.readString("");
	if (message.isEmpty()) return HandlePacketResult::Handled;
	std::vector<CString> words = message.tokenize();

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
				std::vector<CString> commands = CString::loadToken("config/rchelp.txt", "", true);
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
				log::printLine(log::rc, "{} refreshed the server message.", account.name);
			}

			else if (words[0] == "/refreshfilesystem" && words.size() == 1)
			{
				m_server->loadFileSystem();
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " refreshed the server file list.");
				log::printLine(log::rc, "{} refreshed the server file list.", account.name);
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
						log::printLine(log::rc, "{} updated level: {}", account.name, level->getLevelName().text());
						level->reload();
					}
				}
			}
			else if (words[0] == "/updatelevelall" && words.size() == 1 && account.hasRight(PLPERM_UPDATELEVEL))
			{
				log::print(log::rc, "{} updated all the levels", account.name);
				int count = 0;
				auto& levels = m_server->getLevelList();
				for (auto& [name, level] : levels)
				{
					level->reload();
					++count;
				}
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " updated all the levels (" << CString((int)count) << " levels updated).");
				log::printLine(log::rc, " ({} levels updated).", count);
			}
			else if (words[0] == "/restartserver" && words.size() == 1 && account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
			{
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " restarted the server.");
				log::printLine(log::rc, "{} restarted the server.", account.name);
				m_server->restart();
			}
			else if (words[0] == "/reloadserver" && words.size() == 1 && account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
			{
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " reloaded the server configuration files.");
				log::printLine(log::rc, "{} reloaded the server configuration files.", account.name);
				m_server->loadConfigFiles();
			}
			else if (words[0] == "/updateserverhq" && words.size() == 1 && account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
			{
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " sent ServerHQ updates.");
				log::printLine(log::rc, "{} sent ServerHQ updates.", account.name);
				m_server->loadAdminSettings();
				m_server->getServerList().sendServerHQ();
			}
			else if (words[0] == "/serveruptime" && words.size() == 1)
			{
				auto time_diff = std::chrono::system_clock::now() - m_server->getServerStartTime();

				constexpr auto format_time_fn = [](std::string& m, const uint64_t t, const char* fmtStr)
				{
					if (t > 0)
					{
						m.append(std::format(" {} {}", t, fmtStr));
						if (t > 1)
							m.append("s");
					}
				};

				auto days = std::chrono::duration_cast<std::chrono::days>(time_diff).count();
				auto hours = std::chrono::duration_cast<std::chrono::hours>(time_diff).count() % 24;
				auto minutes = std::chrono::duration_cast<std::chrono::minutes>(time_diff).count() % 60;
				auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time_diff).count() % 60;

				std::string msg;
				format_time_fn(msg, days, "day");
				format_time_fn(msg, hours, "hour");
				format_time_fn(msg, minutes, "minute");
				if (days == 0)
					format_time_fn(msg, seconds, "second");

				sendPacket(CString() >> (char)PLO_RC_CHAT << "Server Uptime:" << msg);
			}
			else if (words[0] == "/reloadwordfilter" && words.size() == 1)
			{
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " reloaded the word filter.");
				log::printLine(log::rc, "{} reloaded the word filter.", account.name);
				m_server->loadWordFilter();
			}
			else if (words[0] == "/reloadipbans" && words.size() == 1)
			{
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " reloaded the ip bans.");
				log::printLine(log::rc, "{} reloaded the ip bans.", account.name);
				m_server->loadIPBans();
			}
			else if (words[0] == "/reloadweapons" && words.size() == 1)
			{
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " reloaded the weapons.");
				log::printLine(log::rc, "{} reloaded the weapons.", account.name);
				m_server->loadWeapons(true);
			}
			else if (words[0] == "/savenpcs" && words.size() == 1)
			{
				m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " saved npc to disk.");
				log::printLine(log::npc, "{} saved the npcs to disk.", account.name);
				// TODO(NPCSERVER): Save NPCs to disk.
				//m_server->saveNpcs();
			}
			else if (words[0] == "/stats" && words.size() == 1)
			{
				// TODO(NPCSERVER): Execution stats.
				//auto npcStats = m_server->calculateNpcStats();

				sendPacket(CString() >> (char)PLO_RC_CHAT << "Top scripts using the most execution time (in the last min)");

				/*
				int idx = 0;
				for (auto it = npcStats.begin(); it != npcStats.end(); ++it)
				{
					idx++;
					sendPacket(CString() >> (char)PLO_RC_CHAT << CString(idx) << ". 	" << CString((*it).first) << "	" << (*it).second);
					if (idx == 50)
						break;
				}
				*/
			}
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

					auto current_path = std::filesystem::current_path().string() + (char)std::filesystem::path::preferred_separator;
					for (std::map<CString, CString>::const_iterator i = fileList.begin(); i != fileList.end(); ++i)
					{
						if (i->first.match(search))
							found[i->second.removeAll(current_path)] = fs;
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
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to warp a player.", account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to warp players.\n");
		return HandlePacketResult::Handled;
	}

	auto p = m_server->getPlayer<PlayerClient>(pPacket.readGUShort(), PLTYPE_ANYPLAYER);
	if (p == nullptr) return HandlePacketResult::Handled;

	float loc[2] = { (float)(pPacket.readGChar()) / 2.0f, (float)(pPacket.readGChar()) / 2.0f };
	CString wLevel = pPacket.readString("");
	p->warp(wLevel, loc[0], loc[1]);

	log::printLine(log::rc, "{} has warped {} to {} (:.2f, :.2f)", account.name, p->account.name, wLevel.text(), loc[0], loc[1]);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERRIGHTSGET(CString& pPacket)
{
	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || (acc != account.name && !account.hasRight(PLPERM_SETRIGHTS)))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to get the rights of {}", account.name, acc.text());
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
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !account.hasRight(PLPERM_SETRIGHTS))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to set the rights of {}", account.name, acc.text());
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
	std::ranges::copy(string::splitHard(adminIp, std::string_view{ "," }), std::back_inserter(p->account.adminIpRange));

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

		if (changed_rights & PLPERM_NPCCONTROL)
		{
			if (!(n_adminRights & PLPERM_NPCCONTROL))
			{
				if (auto pNC = m_server->getPlayer(acc, PLTYPE_ANYNC); pNC)
					pNC->disconnect();
			}
			// TODO(NPCSERVER): Send NC address to RC.
			//else
			//	pRC->sendNCAddr();
		}

		// If they are using the File Browser, reload it.
		if (pRC->isUsingFileBrowser())
			pRC->msgPLI_RC_FILEBROWSER_START(CString() << "");
	}

	log::printLine(log::rc, "{} has set the rights of {}", account.name, acc.text());
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
		log::printLine(log::rc, "[Hack] {} attempted to get the comments of {}", account.name, acc.text());
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
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to set the comments of {}", account.name, acc.text());
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

	log::printLine(log::rc, "{} has set the comments of {}", account.name, acc.text());
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
		log::printLine(log::rc, "[Hack] {} attempted to view the ban of {}", account.name, acc.text());
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
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to set the ban of {}", account.name, acc.text());
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
		p->sendPacket(CString() >> (char)PLO_DISCMESSAGE << account.name << " has banned you.  Reason: " << reason.guntokenize().replaceAll("", "\r"));
		m_server->deletePlayer(p);
	}

	log::printLine(log::rc, "{} has set the ban of {}", account.name, acc.text());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has set the ban of " << acc);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_START(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to open the File Browser.", account.name);
		return HandlePacketResult::Handled;
	}

	// If the player has no folder rights, don't open the File Browser.
	if (account.folderList.size() == 0)
		return HandlePacketResult::Handled;

	// Get folder list to send to the client.
	auto folders = string::toCSV(account.folderList, true);

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
	CString mkdir_path = std::filesystem::current_path().string() + (char)std::filesystem::path::preferred_separator;
	auto f = string::splitHard(account.lastFolderAccessed, std::string_view{ "/" });
	for (auto i = f.begin(); i != f.end(); ++i)
	{
		if (i->empty()) continue;
		mkdir_path << *i << '/';

#if defined(_WIN32) || defined(_WIN64)
		(void)mkdir(mkdir_path.text());
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
		log::printLine(log::rc, "[Hack] {} attempted to download a file through the File Browser.", account.name);
		return HandlePacketResult::Handled;
	}

	// Send file.
	CString file = pPacket.readString("");
	CString filepath = CString() << account.lastFolderAccessed << file;
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

	log::printLine(log::rc, "{} downloaded file {}", account.name, file.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Downloaded file " << file);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_UP(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to upload a file through the File Browser.", account.name);
		return HandlePacketResult::Handled;
	}

	CString file = pPacket.readChars(pPacket.readGUChar());
	CString filepath = account.lastFolderAccessed;
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

		log::printLine(log::rc, "{} uploaded file {}", account.name, file.text());
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
		log::printLine(log::rc, "[Hack] {} attempted to move a file through the File Browser.", account.name);
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

	log::printLine(log::rc, "{} moved file {} to {}", account.name, source.text(), destination.text());

	// Save the new file now.
	CString temp;
	temp.load(source);
	if (temp.save(destination) == false)
		return HandlePacketResult::Handled;

	// Remove the old file.
	std::filesystem::remove(source.toStringView());

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_DELETE(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to delete a file through the File Browser.", account.name);
		return HandlePacketResult::Handled;
	}

	CString file = pPacket.readString("");
	std::filesystem::path filePath = std::filesystem::path{ account.lastFolderAccessed } / file.toStringView();

	// Don't let us delete important files.
	CString checkFile = filePath.string();
	for (const auto& file : ImportantFiles)
	{
		if (checkFile == file)
		{
			sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Not allowed to delete file " << checkFile);
			return HandlePacketResult::Handled;
		}
	}

	// Deleting our logs?  First, we need to close them.
	if (account.lastFolderAccessed == "logs/")
	{
		if (file == "rclog.txt") log::rc.close();
		else if (file == "serverlog.txt")
			log::server.close();
		else if (file == "npclog.txt")
			log::npc.close();
		else if (file == "scriptlog.txt")
			log::script.close();
	}

	// Do the deleting.
	std::error_code ec;
	std::filesystem::remove(filePath, ec);

	// Deleting our logs?  We can open them now.
	if (account.lastFolderAccessed == "logs/")
	{
		if (file == "rclog.txt") log::rc.reload();
		else if (file == "serverlog.txt")
			log::server.reload();
		else if (file == "npclog.txt")
			log::npc.reload();
		else if (file == "scriptlog.txt")
			log::script.reload();
	}

	// If we got an error, record it now.
	if (ec)
	{
		log::printLine(log::rc, "Error deleting file: {}", ec.message());
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Error deleting file: " << ec.message());
		return HandlePacketResult::Handled;
	}

	log::printLine(log::rc, "{} deleted file {}", account.name, file.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Deleted file " << file);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_RENAME(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to rename a file through the File Browser.", account.name);
		return HandlePacketResult::Handled;
	}

	CString f1 = pPacket.readChars(pPacket.readGUChar());
	CString f2 = pPacket.readChars(pPacket.readGUChar());

	std::filesystem::path oldPath = std::filesystem::path{ account.lastFolderAccessed } / f1.toStringView();
	std::filesystem::path newPath = std::filesystem::path{ account.lastFolderAccessed } / f2.toStringView();

	// Don't let us rename/overwrite important files.
	CString checkFile1 = oldPath.string();
	CString checkFile2 = newPath.string();
	for (const auto& file : ImportantFiles)
	{
		if (checkFile1 == file || checkFile2 == file)
		{
			sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Not allowed to rename/overwrite file " << checkFile1 << " or " << checkFile2);
			return HandlePacketResult::Handled;
		}
	}

	// Renaming our logs?  First, we need to close them.
	if (account.lastFolderAccessed == "logs/")
	{
		if (f1 == "rclog.txt") log::rc.close();
		else if (f1 == "serverlog.txt")
			log::server.close();
		else if (f1 == "npclog.txt")
			log::npc.close();
		else if (f1 == "scriptlog.txt")
			log::script.close();
	}

	// Do the renaming.
	std::error_code ec;
	std::filesystem::rename(oldPath, newPath, ec);

	// Renaming our logs?  We can open them now.
	if (account.lastFolderAccessed == "logs/")
	{
		if (f1 == "rclog.txt") log::rc.reload();
		else if (f1 == "serverlog.txt")
			log::server.reload();
		else if (f1 == "npclog.txt")
			log::npc.reload();
		else if (f1 == "scriptlog.txt")
			log::script.reload();
	}

	// If we got an error, record it now.
	if (ec)
	{
		log::printLine(log::rc, "Error renaming file: {}", ec.message());
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Error renaming file: " << ec.message());
		return HandlePacketResult::Handled;
	}

	log::printLine(log::rc, "{} renamed file {} to {}", account.name, f1.text(), f2.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Renamed file " << f1 << " to " << f2);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_LARGEFILESTART(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} is attempting to upload a file through the File Browser.", account.name);
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
		log::printLine(log::rc, "[Hack] {} attempted to upload a file through the File Browser.", account.name);
		return HandlePacketResult::Handled;
	}

	CString file = pPacket.readString("");
	CString filepath = CString() << account.lastFolderAccessed << file;

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

	log::printLine(log::rc, "{} uploaded large file {}", account.name, file.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Uploaded large file " << file);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FOLDERDELETE(CString& pPacket)
{
	CString folder = pPacket.readString("");
	CString folderpath = folder;
	FileSystem::fixPathSeparators(folderpath);
	folderpath.removeI(folderpath.length() - 1);
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to delete a folder through the File Browser: {}", account.name, folder.text());
		return HandlePacketResult::Handled;
	}

	// Try to remove folder.
	if (rmdir(folderpath.text()))
	{
		perror("Error removing folder");
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Error removing " << folder << ".  Folder may not exist or may not be empty.");
		return HandlePacketResult::Handled;
	}

	log::printLine(log::rc, "{} removed folder {}", account.name, folder.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Folder " << folder << " has been removed.\n");
	msgPLI_RC_FILEBROWSER_START(CString() << "");

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_NPCSERVERQUERY(CString& pPacket)
{
	// Read Packet Data
	unsigned short pid = pPacket.readGUShort();
	CString message = pPacket.readString("");

	// TODO(NPCSERVER): Send NC address.
	// Enact upon the message.
	//if (message == "location")
	//	sendNCAddr();

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_UNKNOWN162(CString& pPacket)
{
	// Stub.
	return HandlePacketResult::Handled;
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal
